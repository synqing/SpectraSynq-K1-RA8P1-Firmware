#pragma once
#include "emotiscope_port.h"
#include <cmath>
#include <cstdint>
#include <cstring>

namespace composed_timing {
// Optional counted grid, NOT a bar/downbeat detector. configure() receives the
// actual donor tempi[i].target_tempo_hz after init_tempo_goertzel_constants()
// (emotiscope_donor/tempo.h); no duplicate bin-to-frequency model lives here.
class Clock {
 public:
  bool configure(const float target_hz[96]) noexcept {
    if(!target_hz)return false;
    for(unsigned i=0;i<96;++i)
      if(!finite(target_hz[i]) || target_hz[i]<.25f || target_hz[i]>10.0f)return false;
    for(unsigned i=0;i<96;++i)frequencies_[i]=target_hz[i];
    configured_=true;return true;
  }
  void update(const K1EmotiscopeAudioPublication& p,std::uint32_t now_ms) noexcept {
    boundary_=false;
    if(seen_ && now_ms==last_ms_)return;
    const std::uint32_t elapsed=seen_?now_ms-last_ms_:0u;
    last_ms_=now_ms;seen_=true;
    // Modular forward time works over uint32 wrap. Regressions and large gaps
    // cannot replay missed grid events or retain a stale lock.
    const bool discontinuity=elapsed>250u;
    if(discontinuity){locked_=false;challenger_=-1;challenger_samples_=0;}
    if(selected_>=0 && elapsed<0x80000000u){
      const float advance=float(elapsed)*.001f*frequency_;
      turn_+=advance;
    }
    const bool new_publication=!publication_seen_ || p.sequence!=last_sequence_;
    const bool new_audio=!audio_seen_ || p.audio_hops!=last_audio_hops_;
    if(new_audio){audio_seen_=true;last_audio_hops_=p.audio_hops;audio_ms_=now_ms;}
    publication_seen_=true;last_sequence_=p.sequence;
    bool supported=false,seeded=false;
    if(configured_ && !discontinuity && new_publication && p.valid && now_ms-audio_ms_<=200u){
      float sum=0,best=0;int best_bin=-1;bool magnitudes_valid=true;
      for(unsigned i=0;i<96;++i){
        const float x=p.tempi_smooth[i];
        if(!finite(x)||x<0){magnitudes_valid=false;break;}
        sum+=x;if(x>best){best=x;best_bin=int(i);}
      }
      if(magnitudes_valid && finite(sum) && sum>0.000001f && best_bin>=0 &&
         finite(p.tempo_confidence) && p.tempo_confidence>=0 && p.tempo_confidence<=1){
        const float score=minimum(best/sum,p.tempo_confidence);
        if(score>=.08f && phaseValid(p,best_bin)){
          const bool stronger=selected_<0 || best_bin==selected_ ||
              best>p.tempi_smooth[selected_]*1.25f;
          if(stronger && best_bin!=selected_){
            if(challenger_!=best_bin){challenger_=best_bin;challenger_ms_=now_ms;challenger_samples_=1;}
            else if(challenger_samples_<255u)++challenger_samples_;
            const auto dwell=selected_<0?600u:800u;
            if(now_ms-challenger_ms_>=dwell && challenger_samples_>=8u){
              const bool first=selected_<0;
              selected_=best_bin;frequency_=frequencies_[selected_];
              // Only the first acquisition may seed phase. Later candidate
              // switches keep the oscillator continuous and slew below.
              if(first){turn_=targetTurn(p.tempi_phase[selected_]);seeded=true;}
              challenger_=-1;challenger_samples_=0;
            }
          }else{challenger_=-1;challenger_samples_=0;}
        }else{challenger_=-1;challenger_samples_=0;}
        if(selected_>=0 && phaseValid(p,selected_)){
          const float selected_score=minimum(p.tempi_smooth[selected_]/sum,p.tempo_confidence);
          if(selected_score>=.08f){
            supported=true;last_supported_ms_=now_ms;confidence_=selected_score;locked_=true;
            // Slew at <=20% oscillator speed, never jump or reverse phase.
            float error=targetTurn(p.tempi_phase[selected_])-wrap(turn_);
            if(error>.5f)error-=1.f;if(error<-.5f)error+=1.f;
            const float limit=float(elapsed)*.001f*frequency_*.2f;
            const float correction=clamp(error*minimum(float(elapsed)/500.f,1.f),-limit,limit);
            turn_+=correction;
          }
        }
      }else{challenger_=-1;challenger_samples_=0;}
    }else if(!p.valid || now_ms-audio_ms_>200u){challenger_=-1;challenger_samples_=0;}
    if(!supported && selected_>=0){
      const auto age=now_ms-last_supported_ms_;
      if(age>1500u || discontinuity){locked_=false;confidence_=0;}
    }
    boundary_=selected_>=0 && !discontinuity && !seeded && turn_>=1.f;
    turn_=wrap(turn_);
  }
  bool locked()const noexcept{return locked_;}
  // Valid only for this update tick; duplicate update calls clear the pulse.
  // When unlocked this is merely a free-running local count, never beat proof.
  bool boundary()const noexcept{return boundary_;}
  float confidence()const noexcept{return locked_?confidence_*(1.f-minimum(float(last_ms_-last_supported_ms_)/1500.f,1.f)):0.f;}
  int candidate()const noexcept{return selected_;}
 private:
  static bool finite(float x)noexcept{std::uint32_t b;std::memcpy(&b,&x,4);return (b&0x7f800000u)!=0x7f800000u;}
  static float minimum(float a,float b)noexcept{return a<b?a:b;}
  static float clamp(float x,float lo,float hi)noexcept{return x<lo?lo:x>hi?hi:x;}
  static float wrap(float x)noexcept{return x-std::floor(x);}
  static float targetTurn(float phase)noexcept{return wrap((phase-1.57079632679f)/6.28318530718f);}
  static bool phaseValid(const K1EmotiscopeAudioPublication& p,int i)noexcept{
    return finite(p.tempi_phase[i]) && p.tempi_phase[i]>=-3.141593f && p.tempi_phase[i]<=3.141593f &&
      finite(p.tempi_beat[i]) && std::fabs(std::sin(p.tempi_phase[i])-p.tempi_beat[i])<=.15f;
  }
  float frequencies_[96]{},turn_=0,frequency_=1,confidence_=0;
  std::uint32_t last_ms_=0,last_sequence_=0,last_audio_hops_=0,audio_ms_=0;
  std::uint32_t challenger_ms_=0,last_supported_ms_=0;
  int selected_=-1,challenger_=-1;
  unsigned challenger_samples_=0;
  bool configured_=false,seen_=false,publication_seen_=false,audio_seen_=false;
  bool locked_=false,boundary_=false;
};
static_assert(sizeof(Clock)<=512,"Tempo clock must remain bounded and allocation-free");
} // namespace composed_timing
