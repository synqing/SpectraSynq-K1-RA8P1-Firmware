#ifndef K1_MUSICAL_CONDITIONING_H
#define K1_MUSICAL_CONDITIONING_H
#include "emotiscope_port.h"
#include <cmath>
#include <cstring>

/* Board-owned interpretation of one immutable donor publication. Frequency
 * boundaries are in Hz, evaluated against actual producer target labels at init.
 * Goertzel integer-window resonance centres need not equal these target labels.
 * Neither role claims an instrument, tempo lock or learned semantic label. */
class K1MusicalConditioning {
 public:
  bool configure(const K1EmotiscopeMusicalConfig &c) {
    if(c.role>2u || !finite(c.strength) || c.strength<0.0f || c.strength>1.0f ||
       !finite(c.attack_ms) || c.attack_ms<1.0f || c.attack_ms>1000.0f ||
       !finite(c.release_ms) || c.release_ms<1.0f || c.release_ms>3000.0f)return false;
    const bool changed=c.role!=config_.role || c.strength!=config_.strength ||
      c.attack_ms!=config_.attack_ms || c.release_ms!=config_.release_ms;
    if(c.role!=config_.role)reset_history();
    if(changed)cached_valid_=false;
    config_=c;return true;
  }
  bool initialise_frequencies(const float hz[64]) {
    if(!hz)return false;
    for(unsigned i=0;i<64;++i)
      if(!finite(hz[i]) || hz[i]<=0.0f || (i && hz[i]<=hz[i-1]))return false;
    for(unsigned i=0;i<64;++i){
      // Body fades smoothly over 180..700 Hz; detail rises over 250..1400 Hz.
      // These are interpretation crossovers, not invented donor bin centres.
      body_[i]=1.0f-clamp((hz[i]-180.0f)/520.0f);
      detail_[i]=clamp((hz[i]-250.0f)/1150.0f);
    }
    frequencies_valid_=true;return true;
  }
  /* now_us must be the current frame clock, not a cached result timestamp.
   * Unsigned differences support clock wrap; backwards/expired results fail.
   * raw/out must be distinct. Duplicate generations never advance history/events. */
  bool apply(const K1EmotiscopeAudioPublication &raw,
             K1EmotiscopeAudioPublication &out,float dt,uint32_t now_us) {
    if(&out==&raw)return false;
    std::memcpy(&out,&raw,sizeof(out));
    if(config_.role==0u || config_.strength==0.0f)return false;
    const uint32_t sample_time=raw.amplitude_valid?raw.audio_time_us:raw.now_us;
    const uint32_t age=now_us-sample_time;
    // Publication can advance on a render clock while PCM is no longer arriving.
    // Track producer hop progress separately from the publication sequence.
    if(!audio_seen_ || raw.audio_hops!=progress_hops_){
      audio_seen_=true;progress_hops_=raw.audio_hops;audio_progress_us_=sample_time;
    }
    const bool audio_expired=now_us-audio_progress_us_>100000u;
    const bool discontinuity=seen_ &&
      (int32_t(raw.sequence-last_sequence_)<0 ||
       int32_t(raw.audio_hops-last_hops_)<0 || sample_time-last_time_>250000u);
    if(!frequencies_valid_ || !raw.valid || raw.sequence==0u || age>100000u || audio_expired ||
       !valid_features(raw)){
      reset_history();return false;
    }
    if(consumed_ && raw.audio_hops==last_hops_ && sample_time==last_time_){
      // Repeated render at the same generation consumes the cached shaped data.
      // A config edit on a duplicate publication falls back to raw until the
      // next generation; it cannot replay an event or return the old setting.
      if(!cached_valid_)return false;
      copy_features(cached_,out);return true;
    }
    if(discontinuity)reset_history();
    // Each follower integrates the time since its own last consumed audio hop.
    if(seen_)dt=float(sample_time-last_time_)*.000001f;
    if(!finite(dt) || dt<=0.0f || dt>0.25f){reset_history();return false;}
    const bool first_observation=!seen_;
    seen_=true;consumed_=true;last_sequence_=raw.sequence;last_hops_=raw.audio_hops;last_time_=sample_time;
    const float attack=1.0f-std::exp(-dt*1000.0f/config_.attack_ms);
    const float release=1.0f-std::exp(-dt*1000.0f/config_.release_ms);
    float body_energy=0.0f,body_mass=0.0f,detail_energy=0.0f,detail_mass=0.0f;
    for(unsigned i=0;i<64;++i){
      body_energy+=clamp(raw.spectrum[i])*body_[i];body_mass+=body_[i];
      detail_energy+=clamp(raw.spectrum_smooth[i])*detail_[i];detail_mass+=detail_[i];
    }
    body_energy/=body_mass>0.0f?body_mass:1.0f;
    if(raw.amplitude_valid)body_energy=std::fmax(body_energy,clamp(raw.body_peak_energy));
    detail_energy/=detail_mass>0.0f?detail_mass:1.0f;
    // Convert positive change to a fixed 10 ms reference interval. Otherwise
    // the same ramp at 50/100/200 Hz would create different event strengths.
    // The first observation establishes a baseline; it is not a detected rise.
    const float rising=first_observation?0.0f:clamp((body_energy-previous_body_)*0.01f/dt);
    previous_body_=body_energy;
    const float transient=clamp(rising*4.0f+clamp(raw.novelty_normalised)*body_energy);
    const float target=config_.role==1u?transient:detail_energy;
    value_+=(target-value_)*(target>value_?attack:release);
    // Hysteresis gives each board its own rising-event cursor. Novelty contrast
    // alone does not establish acoustic silence, so it never gates valid input.
    if(!event_high_ && target>=0.08f){++event_cursor_;event_high_=true;}
    else if(event_high_ && target<=0.03f)event_high_=false;
    const float strength=config_.strength;
    for(unsigned i=0;i<64;++i){
      const float input=clamp(raw.spectrum[i]);
      envelope_[i]+=(input-envelope_[i])*(input>envelope_[i]?attack:release);
      // Role 1 sharpens low-band changes; role 2 retains independent spectral
      // envelopes and high-band detail. This changes contour/history, not RGB gain.
      const float shaped=config_.role==1u?
        input*(0.2f+0.8f*body_[i])*(0.35f+0.65f*value_):
        envelope_[i]*(0.2f+0.8f*detail_[i]);
      out.spectrum[i]=raw.spectrum[i]+strength*(shaped-raw.spectrum[i]);
      const float smooth=config_.role==1u?shaped:
        envelope_[i]*(0.2f+0.8f*detail_[i]);
      out.spectrum_smooth[i]=raw.spectrum_smooth[i]+strength*(smooth-raw.spectrum_smooth[i]);
      const float ratio=input>0.000001f?clamp(shaped/input):0.0f;
      out.frequency_magnitude[i]=raw.frequency_magnitude[i]*(1.0f-strength+strength*ratio);
    }
    for(unsigned i=0;i<12;++i){
      const float input=clamp(raw.chromagram[i]);
      chroma_[i]+=(input-chroma_[i])*(input>chroma_[i]?attack:release);
      const float shaped=config_.role==1u?input*value_:chroma_[i];
      out.chromagram[i]=raw.chromagram[i]+strength*(shaped-raw.chromagram[i]);
    }
    const float vu_input=clamp(config_.role==1u && raw.amplitude_valid?std::fmax(raw.vu_level,raw.vu_peak):raw.vu_level);
    // The VU envelope below owns the selected attack/release for injected level.
    // Feeding the already-filtered value_ through it again delays short hits and
    // can keep level rising after the transient ends. Filter the raw role target
    // once here; value_ still owns spectral/chroma/event interpretation above.
    const float vu_target=vu_input*(config_.role==1u?transient:detail_energy);
    vu_envelope_+=(vu_target-vu_envelope_)*(vu_target>vu_envelope_?attack:release);
    // Analog uses the level as a moving position/extent. Shape its temporal
    // input, keeping the actual raw VU measurement and quality fields intact.
    out.vu_level=raw.vu_level+strength*(vu_envelope_-raw.vu_level);
    out.tempi_power_sum=0.00000001f;
    for(unsigned i=0;i<96;++i){
      const float input=clamp(raw.tempi_smooth[i]);
      tempo_envelope_[i]+=(input-tempo_envelope_[i])*(input>tempo_envelope_[i]?attack:release);
      const float change=first_observation?0.0f:clamp((input-previous_tempo_[i])*0.04f/dt);
      previous_tempo_[i]=input;
      // Preserve actual candidate phase/beat and quality. Candidate-specific
      // histories alter the Hype/Metronome mixture without inventing tempo lock.
      const float shaped=config_.role==1u?input*(0.2f+0.8f*clamp(change+value_)):
        tempo_envelope_[i]*(0.35f+0.65f*detail_energy);
      out.tempi_smooth[i]=raw.tempi_smooth[i]+strength*(shaped-raw.tempi_smooth[i]);
      out.tempi_power_sum+=out.tempi_smooth[i];
    }
    out.novelty_normalised=raw.novelty_normalised+
      strength*((config_.role==1u?value_:clamp(value_*0.25f))-raw.novelty_normalised);
    std::memcpy(&cached_,&out,sizeof(cached_));cached_valid_=true;return true;
  }
  bool apply(const K1EmotiscopeAudioPublication &raw,
             K1EmotiscopeAudioPublication &out,float dt) {
    return apply(raw,out,dt,raw.now_us);
  }
  // Explicit capture-gap reset preserves configuration, crossovers and the
  // monotonically advancing event cursor; no prior shaped cache survives.
  void reset(){reset_history();}
  uint32_t event_cursor()const{return event_cursor_;}
  float value()const{return value_;}
  K1EmotiscopeMusicalConfig config()const{return config_;}
 private:
  static bool finite(float x){return std::isfinite(x);}
  static float clamp(float x){return x<0.0f?0.0f:(x>1.0f?1.0f:x);}
  static bool valid_features(const K1EmotiscopeAudioPublication &p){
    if(p.amplitude_valid && (!finite(p.pcm_peak_squared)||p.pcm_peak_squared<0 ||
       !finite(p.body_peak_energy)||p.body_peak_energy<0 || !finite(p.vu_peak)||p.vu_peak<0))return false;
    if(!finite(p.novelty_normalised)||!finite(p.vu_level)||p.vu_level<0.0f)return false;
    for(unsigned i=0;i<64;++i)
      if(!finite(p.spectrum[i]) || p.spectrum[i]<0.0f ||
         !finite(p.spectrum_smooth[i]) || p.spectrum_smooth[i]<0.0f ||
         !finite(p.frequency_magnitude[i]) || p.frequency_magnitude[i]<0.0f)return false;
    for(float x:p.chromagram)if(!finite(x)||x<0.0f)return false;
    for(unsigned i=0;i<96;++i)
      if(!finite(p.tempi_smooth[i])||p.tempi_smooth[i]<0.0f||
         !finite(p.tempi_phase[i])||!finite(p.tempi_beat[i]))return false;
    return true;
  }
  static void copy_features(const K1EmotiscopeAudioPublication &a,K1EmotiscopeAudioPublication &b){
    std::memcpy(b.spectrum,a.spectrum,sizeof(b.spectrum));
    std::memcpy(b.spectrum_smooth,a.spectrum_smooth,sizeof(b.spectrum_smooth));
    std::memcpy(b.frequency_magnitude,a.frequency_magnitude,sizeof(b.frequency_magnitude));
    std::memcpy(b.chromagram,a.chromagram,sizeof(b.chromagram));
    b.novelty_normalised=a.novelty_normalised;b.vu_level=a.vu_level;
    b.tempi_power_sum=a.tempi_power_sum;
    std::memcpy(b.tempi_smooth,a.tempi_smooth,sizeof(b.tempi_smooth));
  }
  void reset_history(){
    std::memset(envelope_,0,sizeof(envelope_));std::memset(chroma_,0,sizeof(chroma_));
    std::memset(tempo_envelope_,0,sizeof(tempo_envelope_));
    std::memset(previous_tempo_,0,sizeof(previous_tempo_));vu_envelope_=0.0f;
    value_=0.0f;previous_body_=0.0f;seen_=false;event_high_=false;cached_valid_=false;
  }
  K1EmotiscopeMusicalConfig config_{0u,0.0f,40.0f,160.0f};
  float body_[64]{},detail_[64]{},envelope_[64]{},chroma_[12]{};
  float value_=0.0f,previous_body_=0.0f,vu_envelope_=0.0f;
  float tempo_envelope_[96]{},previous_tempo_[96]{};
  uint32_t event_cursor_=0u,last_sequence_=0u,last_hops_=0u,last_time_=0u;
  uint32_t progress_hops_=0u,audio_progress_us_=0u;
  bool frequencies_valid_=false,seen_=false,event_high_=false,consumed_=false,cached_valid_=false,audio_seen_=false;
  K1EmotiscopeAudioPublication cached_{};
};
using EmotiscopeMusicalConditioning=K1MusicalConditioning;
#endif
