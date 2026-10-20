#include "emotiscope_ml.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

static uint32_t clock_call=0u;
static uint32_t late_clock(){ return ++clock_call==1u?50000u:150001u; }
static uint32_t cycle_call=0u;
static uint32_t cycles(){ return 100u*(++cycle_call); }
static K1EmotiscopeAudioPublication publication(uint32_t sequence,uint32_t time){
  K1EmotiscopeAudioPublication p{};
  p.valid=1u;p.sequence=sequence;p.audio_hops=64u+sequence;p.now_us=time;
  for(unsigned i=0;i<64;++i){
    p.spectrum[i]=p.spectrum_smooth[i]=float((i%7u)+1u)/10.0f;
    p.frequency_magnitude[i]=p.spectrum[i];
  }
  p.spectrum[0]=p.spectrum_smooth[0]=p.frequency_magnitude[0]=0.0f;
  return p;
}
static void equal(const K1EmotiscopeAudioPublication& a,const K1EmotiscopeAudioPublication& b){
  assert(std::memcmp(&a,&b,sizeof(a))==0);
}
int main(int argc,char** argv){
  // Frozen independent fixture driver: stdin64 raw smooth bins, stdout8+64.
  if(argc==2 && std::string(argv[1])=="--infer"){
    float input[64]{},latent[8]{},output[64]{};
    for(float& value:input)if(std::scanf("%f",&value)!=1)return 2;
    if(!EmotiscopeMl::infer(input,latent,output))return 3;
    for(float value:latent)std::printf("%.9g ",double(value));
    for(float value:output)std::printf("%.9g ",double(value));
    std::puts("");return 0;
  }
  EmotiscopeMl ml;
  auto p=publication(1u,0u);
  ml.prepare(p,0u,true);assert(!ml.result().valid && ml.result().inference_count==0u);
  p=publication(2u,10000u);ml.prepare(p,10000u,true,cycles);
  assert(ml.result().valid && ml.result().inference_count==1u && ml.result().last_cycles==100u);
  assert(ml.result().model_id==EmotiscopeMl::model_id && ml.result().input_sequence==2u);
  float captured_input[64]{};
  assert(ml.inference_input(captured_input,p.now_us));
  assert(std::memcmp(captured_input,p.spectrum_smooth,sizeof(captured_input))==0);
  assert(!ml.inference_input(captured_input,p.now_us+100001u));
  const auto first=ml.result();
  ml.prepare(p,10000u,true,cycles);assert(ml.result().inference_count==1u);
  p=publication(3u,49999u);ml.prepare(p,p.now_us,true);assert(ml.result().inference_count==1u);
  p=publication(4u,50000u);ml.prepare(p,p.now_us,true);assert(ml.result().inference_count==2u);
  const auto fresh_result=ml.result();
  auto frozen_audio=p;frozen_audio.sequence=5u;frozen_audio.now_us=90000u;
  ml.prepare(frozen_audio,frozen_audio.now_us,true);assert(ml.result().inference_count==2u);
  frozen_audio.sequence=6u;frozen_audio.now_us=140000u;
  ml.prepare(frozen_audio,frozen_audio.now_us,true);assert(ml.result().valid);
  frozen_audio.sequence=7u;frozen_audio.now_us=150001u;
  ml.prepare(frozen_audio,frozen_audio.now_us,true);assert(!ml.result().valid&&ml.result().inference_count==2u);
  // Start a separate continuation after the explicit frozen-audio expiry.
  ml=EmotiscopeMl{};ml.prepare(publication(3u,40000u),40000u,true);
  ml.prepare(p,p.now_us,true);assert(ml.result().valid);
  assert(ml.result().model_id==fresh_result.model_id);
  K1EmotiscopeAudioPublication out{};
  assert(!EmotiscopeMl::apply(p,ml.result(),{0u,1.0f},p.now_us,out));equal(p,out);
  assert(!EmotiscopeMl::apply(p,ml.result(),{1u,0.0f},p.now_us,out));equal(p,out);
  assert(EmotiscopeMl::apply(p,ml.result(),{1u,1.0f},p.now_us,out));
  assert(out.spectrum[0]==0.0f && out.vu_level==p.vu_level);
  assert(out.spectrum[1]/p.spectrum[1]!=out.spectrum[2]/p.spectrum[2]);
  for(unsigned i=0;i<64;++i)assert(out.spectrum[i]>=0.0f&&out.spectrum[i]<=p.spectrum[i]);
  const float offset=EmotiscopeMl::palette_offset(p,ml.result(),{1u,1.0f},p.now_us);
  assert(std::isfinite(offset)&&std::fabs(offset)<=0.0625f&&offset!=0.0f);
  assert(EmotiscopeMl::palette_offset(p,ml.result(),{0u,1.0f},p.now_us)==0.0f);
  auto invalid_alias=p;invalid_alias.frequency_magnitude[63]=std::numeric_limits<float>::quiet_NaN();
  const auto original_alias=invalid_alias;
  assert(!EmotiscopeMl::apply(invalid_alias,ml.result(),{1u,1.0f},p.now_us,invalid_alias));
  equal(invalid_alias,original_alias);
  auto bad=ml.result();++bad.model_id;
  assert(!EmotiscopeMl::apply(p,bad,{1u,1.0f},p.now_us,out));equal(p,out);
  bad=ml.result();bad.latent[0]=std::numeric_limits<float>::quiet_NaN();
  assert(!EmotiscopeMl::apply(p,bad,{1u,1.0f},p.now_us,out));equal(p,out);
  bad=ml.result();bad.reconstruction[3]=std::numeric_limits<float>::infinity();
  assert(!EmotiscopeMl::apply(p,bad,{1u,1.0f},p.now_us,out));equal(p,out);
  bad=ml.result();bad.now_us=p.now_us+1u;
  assert(!EmotiscopeMl::usable(p,bad,p.now_us));
  bad=ml.result();bad.input_sequence=p.sequence+1u;
  assert(!EmotiscopeMl::usable(p,bad,p.now_us));
  auto later=p;later.now_us=first.now_us+100000u;later.sequence=100u;
  assert(EmotiscopeMl::usable(later,first,later.now_us));
  ++later.now_us;assert(!EmotiscopeMl::usable(later,first,later.now_us));
  assert(!EmotiscopeMl::apply(later,first,{1u,1.0f},later.now_us,out));equal(later,out);
  ml.prepare(p,p.now_us,false);assert(!ml.result().valid);
  assert(!ml.inference_input(captured_input,p.now_us));
  ml.prepare(p,p.now_us,true);assert(!ml.result().valid);
  p=publication(5u,60000u);ml.prepare(p,p.now_us,true);assert(!ml.result().valid);
  auto invalid=p;invalid.sequence=6u;invalid.now_us=70000u;invalid.valid=0u;
  ml.prepare(invalid,invalid.now_us,true);assert(!ml.result().valid);
  p=publication(7u,80000u);ml.prepare(p,p.now_us,true);assert(!ml.result().valid);
  p=publication(8u,90000u);ml.prepare(p,p.now_us,true);assert(ml.result().valid);
  invalid=publication(7u,90001u);ml.prepare(invalid,invalid.now_us,true);assert(!ml.result().valid);
  ml=EmotiscopeMl{};ml.reset();assert(!ml.result().valid);
  p=publication(1u,0u);ml.prepare(p,p.now_us,true);
  p=publication(2u,50000u);clock_call=0u;const auto executions=ml.result().inference_count;
  ml.prepare(p,p.now_us,true,nullptr,late_clock);
  assert(!ml.result().valid&&ml.result().inference_count==executions+1u);
  assert(ml.result().rejected_results>0u);
  EmotiscopeMl toggled;
  auto toggle_pub=publication(1u,0u);toggled.prepare(toggle_pub,0u,true);
  toggle_pub=publication(2u,10000u);toggled.prepare(toggle_pub,10000u,true);
  assert(toggled.result().inference_count==1u);
  toggled.prepare(toggle_pub,10000u,false);
  toggle_pub=publication(3u,20000u);toggled.prepare(toggle_pub,20000u,true);
  toggle_pub=publication(4u,30000u);toggled.prepare(toggle_pub,30000u,true);
  assert(!toggled.result().valid&&toggled.result().inference_count==1u);
  toggled.reset();toggle_pub=publication(5u,40000u);toggled.prepare(toggle_pub,40000u,true);
  toggle_pub=publication(6u,50000u);toggled.prepare(toggle_pub,50000u,true);
  assert(toggled.result().valid&&toggled.result().inference_count==2u);
  EmotiscopeMl reordered;
  for(const auto& q : {publication(10u,0u),publication(11u,10000u)})reordered.prepare(q,q.now_us,true);
  assert(reordered.result().valid&&reordered.result().input_sequence==11u);
  for(const auto& q : {publication(9u,20000u),publication(8u,30000u),publication(9u,50000u)}){
    reordered.prepare(q,q.now_us,true);assert(!reordered.result().valid);
  }
  auto recovered=publication(12u,60000u);reordered.prepare(recovered,recovered.now_us,true);
  assert(!reordered.result().valid);
  recovered=publication(13u,70000u);reordered.prepare(recovered,recovered.now_us,true);
  assert(reordered.result().valid&&reordered.result().input_sequence==13u);
  recovered=publication(14u,180001u);reordered.prepare(recovered,recovered.now_us,true);
  assert(!reordered.result().valid);
  recovered=publication(15u,190001u);reordered.prepare(recovered,recovered.now_us,true);
  assert(reordered.result().valid);
  // Ordinary unsigned timestamp and publication wraps retain cadence.
  EmotiscopeMl wrapped;
  p=publication(0xfffffffdu,0xffff5000u);wrapped.prepare(p,p.now_us,true);
  p=publication(0xfffffffeu,0xffff7000u);wrapped.prepare(p,p.now_us,true);assert(wrapped.result().valid);
  p=publication(1u,0x2000u);wrapped.prepare(p,p.now_us,true);assert(wrapped.result().valid);
  float latent[8]{},reconstruction[64]{};
  auto endpoints=p;
  endpoints.spectrum_smooth[0]=std::numeric_limits<float>::quiet_NaN();
  endpoints.spectrum_smooth[63]=-1.0f;
  assert(EmotiscopeMl::infer(endpoints.spectrum_smooth,latent,reconstruction));
  endpoints.spectrum_smooth[1]=-1.0f;
  assert(!EmotiscopeMl::infer(endpoints.spectrum_smooth,latent,reconstruction));
  endpoints.spectrum_smooth[1]=1.0001f;
  assert(!EmotiscopeMl::infer(endpoints.spectrum_smooth,latent,reconstruction));
  endpoints.spectrum_smooth[1]=std::numeric_limits<float>::quiet_NaN();
  assert(!EmotiscopeMl::infer(endpoints.spectrum_smooth,latent,reconstruction));
  std::puts("ML focused assertions completed: inference, cadence, consumer and invalid fallback");
}
