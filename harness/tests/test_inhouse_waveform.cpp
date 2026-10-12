#include "core/visual/inhouse_waveform.h"
#include "oracle.h"
#include "inhouse_audio.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace k1::core::visual;
static unsigned level(const Pixel16& p){return unsigned(p.red)+p.green+p.blue;}
static void confirmed_quiet_and_wake(){
 InhouseWaveformRenderer r, peer, control;InhouseWaveformAudio a;InhouseWaveformControls c;
 Pixel16 out[160]{}, other[160]{}, expected_peer[160]{};
 a.valid=true;a.epoch=9;a.analysis_end_sample_exclusive=96;a.peak_scaled=1.f;a.vu_level=.025f;
 a.observed_peak_scaled=1.f;a.peak_summary_valid=true;a.spectrum.fill(.4f);
 for(unsigned frame=1;frame<=250;++frame){
   const uint64_t now=frame*10000ULL;
   a.generation=frame;a.capture_us=now-2000;a.publish_us=now-1000;
   a.silence=frame>20;
   const auto births=r.observation().injection_count;
   assert(r.render(a,c,now,out,160));
   if(a.silence){assert(r.observation().injection_count==births);assert(r.observation().deposit_strength==0.f);}
   auto live=a;live.silence=false;
   assert(peer.render(live,c,now,other,160));assert(control.render(live,c,now,expected_peer,160));
   assert(std::memcmp(other,expected_peer,sizeof(other))==0);
 }
 for(const auto& p:out)assert(level(p)==0);
 // A fresh weak publication reopens births immediately, without waiting on VU/chroma.
 a.silence=false;a.peak_scaled=.021f;a.vu_level=0.f;a.spectrum.fill(0.f);a.peak_summary_valid=false;
 a.generation=251;a.capture_us=2508000;a.publish_us=2509000;
 const auto before=r.observation().injection_count;
 assert(r.render(a,c,2510000,out,160));assert(r.observation().injection_count>before);
 assert(r.observation().deposit_strength>0.f);
}
static void real_pcm_quiet_and_wake(){
 k1::titan::InhouseAudio producer;assert(producer.configure({}));
 InhouseWaveformRenderer renderer;Pixel16 out[160]{};uint64_t seq=0;
 auto push=[&](int16_t value){
   k1_audio_hop_t h{};h.contract_version=2;h.samples=64;h.analysis_rate_hz=12800;h.stream_epoch=1;
   h.hop_sequence=++seq;h.analysis_begin=(seq-1)*64;h.analysis_end_exclusive=seq*64;
   for(auto& pcm:h.pcm)pcm=value;
   const bool ready=producer.consume(h,seq*5000,true);producer.stampPublication(seq*5000);
   if(ready){const auto births=renderer.observation().injection_count;
     assert(renderer.render(producer.latest(),{},seq*5000,out,160));
     if(producer.latest().silence)assert(renderer.observation().injection_count==births);
   }
   return ready;
 };
 // The measured quiet failure has positive RMS and a healthy normalised peak.
 for(unsigned i=0;i<1300;++i)push(655); // raw RMS about .02, >5 s dwell
 assert(producer.latest().silence && producer.latest().peak_scaled>.02f);
 for(const auto& p:out)assert(level(p)==0);
 for(unsigned i=0;i<30;++i)push(2588); // RMS .079 stays below declared .08 exit
 assert(producer.latest().silence); // Honest limitation: this weak level cannot wake.
 unsigned analyses=0;const auto attack_begin=seq;const auto births=renderer.observation().injection_count;
 while(producer.latest().silence && analyses<3)if(push(2654))++analyses; // RMS .081
 // Partial 64-to-96 assembly can first publish a mixed below-threshold block;
 // the source classifier then uses the previous full block's RMS.
 assert(!producer.latest().silence && analyses<=3);
 std::printf("PCM wake: .079 remains quiet; .081 clears after %u analyses / %llu us at this hop alignment\n",analyses,(unsigned long long)((seq-attack_begin)*5000));
 assert(renderer.observation().injection_count>births);
 for(unsigned i=0;i<300;++i){push(655);assert(!producer.latest().silence);} // 1.5 s inter-beat gap
}
int main(int argc,char** argv){
 assert(argc==2 || argc==3);FILE* frames=std::fopen(argv[1],"wb");assert(frames);
 InhouseWaveformRenderer renderer, peer, peer_control;
 InhouseWaveformControls controls;InhouseWaveformAudio audio;
 Pixel16 output[160]{}, other[160]{}, control[160]{};
 ChannelEffectState source_state;source_state.wfhyb_last_ms=10;CRGB16 source_history[160]{};
 unsigned maximum_error=0,lit_subbyte=0;
 uint32_t quiet_births=0;uint64_t quiet_energy=UINT64_MAX;unsigned quiet_differences=0;
 for(unsigned frame=0;frame<900;++frame){
   if(frame==300)controls.palette_id=11;
   const uint64_t time=10000ULL*(frame+2U);
   audio.valid=true;audio.epoch=1;audio.generation=frame+1;
   audio.analysis_begin_sample=frame*96;audio.analysis_end_sample_exclusive=(frame+1)*96;
   audio.capture_us=time-2000;audio.publish_us=time-1000;
   audio.silence=frame>=700;audio.peak_scaled=frame<700 ? .03f+.92f*std::abs(std::sin(float(frame)*.041f)) : 0.f;
   audio.vu_level=audio.peak_scaled*.7f;
   for(unsigned bin=0;bin<80;++bin)audio.spectrum[bin]=frame<700 ? .02f+.7f*std::max(0.f,std::sin(float(bin)*.43f+float(frame)*.02f)) : 0.f;
   assert(renderer.render(audio,controls,time,output,160));
   if(argc==3 && frame==0)output[69].red=output[90].red=65535; // Planted output corruption.
   assert(peer.render(audio,{},time,other,160));assert(peer_control.render(audio,{},time,control,160));
   assert(std::memcmp(other,control,sizeof(other))==0);
   const auto& observed=renderer.observation();
   for(unsigned bin=0;bin<80;++bin)k1_vp_spectrogram[bin]=SQ15x16(audio.spectrum[bin]);
   get_smooth_spectrogram();make_smooth_chromagram();
   for(unsigned c=0;c<12;++c){assert(float(chromagram_smooth[c])==observed.chroma[c]);assert(float(chromagram_pregate[c])==observed.chroma_pregate[c]);}
   (void)palette_chroma_colour_with_offset(cached_gradient_palette(uint8_t(controls.palette_id),false),SQ15x16(0.f),.05f,0.f);
   assert(k1_palette_held_u==observed.held_coordinate);
   assert(k1_palette_held_u_valid==observed.held_coordinate_valid);
   snap={audio.peak_scaled,audio.vu_level,audio.silence};input.peak=audio.peak_scaled;
   clock_ms=uint32_t(time/1000);palette_id=controls.palette_id;
   light_mode_waveform_hybrid_k1(source_history,source_state);
   assert(std::abs(source_state.wfhyb_peak_last-observed.peak_stage2)<.000001f);
   if(frame==700)quiet_births=observed.injection_count;
   if(frame>=700){assert(observed.deposit_strength==0.f);assert(observed.injection_count==quiet_births);}
   uint64_t frame_energy=0;
   for(unsigned i=0;i<160;++i){
     frame_energy+=level(output[i]);
     const unsigned expected[3]={unsigned(float(leds_16[i].r)*65535.f),unsigned(float(leds_16[i].g)*65535.f),unsigned(float(leds_16[i].b)*65535.f)};
     const unsigned actual[3]={output[i].red,output[i].green,output[i].blue};
     for(unsigned c=0;c<3;++c){const unsigned error=unsigned(std::abs(int(expected[c])-int(actual[c])));if(frame<700)maximum_error=std::max(maximum_error,error);if(error!=0 && frame>=700)++quiet_differences;if(error!=0 && frame<700){std::fprintf(stderr,"frame=%u pixel=%u c=%u expected=%u actual=%u error=%u\n",frame,i,c,expected[c],actual[c],error);std::fprintf(stderr,"source dot %.9f %.9f %.9f actual %.9f %.9f %.9f\n",source_state.wfhyb_dot_r,source_state.wfhyb_dot_g,source_state.wfhyb_dot_b,observed.smoothed_colour.red,observed.smoothed_colour.green,observed.smoothed_colour.blue);return 2;}if(actual[c]>0 && actual[c]<257)++lit_subbyte;}
     assert(std::memcmp(&output[i],&output[159-i],sizeof(Pixel16))==0);
   }
   if(frame>=700){assert(frame_energy<=quiet_energy);quiet_energy=frame_energy;}
   std::fwrite(output,sizeof(output),1,frames);
 }
 std::fclose(frames);assert(lit_subbyte>100);assert(quiet_differences>0);assert(quiet_energy==0);
 // Stale/regressive inputs drain instead of keeping the last loud sample live.
 for(unsigned i=902;i<1200;++i)assert(renderer.render(audio,controls,i*10000ULL,output,160));
 for(const auto& pixel:output)assert(level(pixel)==0);
 assert(!renderer.render(audio,controls,11990000,output,160));
 assert(!renderer.render(audio,controls,12000000,output,159));
 // First frame is exactly two adjacent amplitude-position deposits per half.
 InhouseWaveformRenderer first;audio={};audio.valid=true;audio.epoch=1;audio.generation=1;
 audio.analysis_end_sample_exclusive=96;audio.capture_us=1000;audio.publish_us=2000;audio.peak_scaled=.8f;audio.vu_level=.5f;audio.silence=false;
 assert(first.render(audio,controls,10000,output,160));unsigned upper_lit=0;
 for(unsigned i=80;i<160;++i)if(level(output[i]))++upper_lit;
 assert(upper_lit==2);assert(first.observation().position>80.f);
 assert(first.observation().injection_count==2 && first.observation().deposit_strength>0.f);
 const float expected=.8f*(1.f-std::exp(-.01f/.016f))*(1.f-std::exp(-.01f/.023f));
 assert(std::abs(first.observation().peak_stage2-expected)<1e-6f);
 // Strict invalid-data behaviour: a loud but invalid publication births nothing.
 InhouseWaveformRenderer invalid;audio.valid=false;audio.peak_scaled=1.f;
 assert(invalid.render(audio,controls,10000,output,160));
 for(const auto& p:output)assert(level(p)==0);
 audio.valid=true;audio.capture_us=1000;audio.publish_us=2000;
 assert(invalid.render(audio,controls,40000,output,160));
 assert(!invalid.observation().input_fresh);
 assert(invalid.observation().injection_count==0 && invalid.observation().deposit_strength==0.f);
 for(const auto& p:output)assert(level(p)==0);
 // Retained interval summary carries a missed narrow peak without inventing VU.
 InhouseWaveformRenderer interval;audio.capture_us=49000;audio.publish_us=49500;
 audio.peak_scaled=0.f;audio.vu_level=0.f;audio.silence=false;
 audio.observed_peak_scaled=.8f;audio.peak_summary_valid=true;
 assert(interval.render(audio,controls,50000,output,160));
 assert(std::abs(interval.observation().peak_stage2-expected)<1e-6f);
 const auto before_invalid=interval.observation().injection_count;
 audio.epoch=0;audio.generation=999;audio.capture_us=59000;audio.publish_us=59500;
 assert(interval.render(audio,controls,60000,output,160));
 assert(!interval.observation().input_fresh); // A prior epoch cannot replay.
 assert(interval.observation().injection_count==before_invalid && interval.observation().deposit_strength==0.f);
 audio.epoch=2;audio.generation=1;audio.capture_us=69000;audio.publish_us=69500;
 assert(interval.render(audio,controls,70000,output,160));
 assert(interval.observation().input_fresh); // A genuine new epoch is admitted.
 // Non-finite caller data cannot reach integer conversion or output.
 InhouseWaveformRenderer nonfinite;audio.peak_summary_valid=false;
 audio.peak_scaled=std::numeric_limits<float>::quiet_NaN();
 audio.spectrum.fill(std::numeric_limits<float>::infinity());
 assert(nonfinite.render(audio,controls,70000,output,160));
 for(const auto& p:output)assert(level(p)==0);
 confirmed_quiet_and_wake();
 real_pcm_quiet_and_wake();
 std::printf("actual source effect: 900 frames, exact audible comparison 700 frames, deliberate quiet divergence 200 frames; max audible u16 error=%u; subbyte=%u; quiet differing channels=%u; confirmed quiet stops births, weak wake, PCM classifier and peer state verified\n",maximum_error,lit_subbyte,quiet_differences);
}
