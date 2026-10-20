#include "../../platform/ra8p1/emotiscope_port.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
static float inverse_tone(float x){
  return x<.75f?x:float(.75+.25*std::atanh((double(x)-.75)/.25));
}
static float tone(float x){
  return x<.75f?x:float(.75+.25*std::tanh((double(x)-.75)*4.0));
}
int main(){
  assert(k1_emotiscope_init(0xACE1u));
  K1EmotiscopeConfig cfg;k1_emotiscope_default_config(&cfg);
  cfg.background=0;cfg.screensaver=false;
  assert(k1_emotiscope_set_config(&cfg));
  float chunk[64];uint8_t rgb[384];double t=0;
  auto fill=[&](){
    for(unsigned i=0;i<64;++i,t+=1.0/12800.0)
      chunk[i]=float((std::fmod(t,.5)<.06?.5:.12)*
                    (std::sin(t*6.283185307179586*110.0)+
                     .3*std::sin(t*6.283185307179586*440.0)));
  };
  for(unsigned hop=1;hop<=4000;++hop){
    fill();assert(k1_emotiscope_push_pcm64(chunk,hop*5000));
    if(hop%2==0)assert(k1_emotiscope_render(hop*5000,rgb));
  }
  K1EmotiscopeSnapshot before{},gap{},after{};
  K1EmotiscopeStatus status{};
  k1_emotiscope_snapshot(&before);k1_emotiscope_status(&status);
  assert(status.valid);
  k1_emotiscope_audio_gap();k1_emotiscope_snapshot(&gap);k1_emotiscope_status(&status);
  assert(!status.valid && status.audio_gaps==1 && status.fresh_hops==0);
  assert(status.dominant_tempo_bpm==0 && status.tempo_confidence==0);
  assert(std::memcmp(before.post_gamma_rgb,gap.post_gamma_rgb,sizeof(gap.post_gamma_rgb))==0);
  assert(std::memcmp(before.tempi_phase,gap.tempi_phase,sizeof(gap.tempi_phase))==0);
  assert(std::memcmp(before.tempi_smooth,gap.tempi_smooth,sizeof(gap.tempi_smooth))==0);
  for(float x:gap.spectrogram_smooth)assert(x==0);
  for(float x:gap.chromagram)assert(x==0);
  K1EmotiscopeConfig readback{};k1_emotiscope_get_config(&readback);
  assert(std::memcmp(&cfg,&readback,sizeof(cfg))==0);
  assert(k1_emotiscope_render(20010000,rgb));
  k1_emotiscope_snapshot(&after);
  // Invert the bound pre-gap final float to recover its retained LPF field.
  // Zero deposits must decay that field, not rebuild a fresh black field.
  const float alpha=1.0-std::exp(float(-6.28318530718*7.75/100.0));
  const float white[]={1.0f,.9375f,.84f};
  unsigned nonzero=0;
  for(unsigned i=0;i<384;++i){
    float old_lpf=inverse_tone(std::sqrt(before.post_gamma_rgb[i])/white[i%3]);
    float next=tone(old_lpf*(1.0f-alpha))*white[i%3];next*=next;
    assert(std::fabs(after.post_gamma_rgb[i]-next)<2e-6f);
    nonzero+=rgb[i]!=0;
  }
  assert(nonzero>0);
  for(unsigned fresh=1;fresh<=64;++fresh){
    fill();assert(k1_emotiscope_push_pcm64(chunk,20010000+fresh*5000));
    if(fresh%2==0)assert(k1_emotiscope_render(20010000+fresh*5000,rgb));
    k1_emotiscope_status(&status);
    assert(status.fresh_hops==fresh);
    assert(status.valid==(fresh==64));
    if(fresh<64)for(float x:status.spectrum_smooth)assert(x==0);
  }
  float energy=0;for(float x:status.spectrum_smooth)energy+=x;assert(energy>0);
  std::printf("gap continuity: retained field=%u lit RGB channels, rewarm=64 chunks/320 ms, no VP/config/phase reset\n",nonzero);
}
