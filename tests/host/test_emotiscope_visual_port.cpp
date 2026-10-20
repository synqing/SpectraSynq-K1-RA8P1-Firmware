#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#if __has_include("../../platform/ra8p1/emotiscope_port.h")
#include "../../platform/ra8p1/emotiscope_port.h"
int main() {
  assert(k1_emotiscope_init(0xACE1u));
  assert(!k1_emotiscope_init(0xACE1u)); // boot-once, never fake resetting function statics
  K1EmotiscopeConfig config;
  k1_emotiscope_default_config(&config);
  assert(config.current_mode == 1 && config.mirror_mode);
  assert(config.brightness == 1.0f && config.background == 0.25f);
  assert(k1_emotiscope_set_config(&config));
  K1EmotiscopeConfig bad=config; bad.current_mode=99;
  assert(!k1_emotiscope_set_config(&bad));
  bad=config; bad.softness=NAN;
  assert(!k1_emotiscope_set_config(&bad));
  float chunk[64]={}; chunk[7]=NAN;
  assert(!k1_emotiscope_push_pcm64(chunk,5000));
  K1EmotiscopeSnapshot before; k1_emotiscope_snapshot(&before);
  assert(before.audio_hops==0 && before.rejected_audio_chunks==1);
  uint8_t rgb[384]={}, previous[384]={};
  uint32_t changed=0, nonblack=0, frames=0;
  double t=0;
  for (uint32_t hop=1;hop<=6000;++hop) {
    for (unsigned i=0;i<64;++i,t+=1.0/12800.0) {
      const double beat=std::fmod(t,0.5);
      const double level=(beat<0.04 ? 0.55:0.07);
      chunk[i]=static_cast<float>(level*(std::sin(t*6.283185307179586*110.0)+
             0.4*std::sin(t*6.283185307179586*440.0))*
             (0.65+0.35*std::sin(t*6.283185307179586*0.33)));
    }
    assert(k1_emotiscope_push_pcm64(chunk,hop*5000));
    if (hop%2==0) {
      if (hop%800==0) { config.current_mode=(hop/800)%7; assert(k1_emotiscope_set_config(&config)); }
      assert(k1_emotiscope_render(hop*5000,rgb));
      unsigned sum=0;for(unsigned i=0;i<384;++i)sum+=rgb[i];
      nonblack+=sum!=0;changed+=std::memcmp(previous,rgb,sizeof(rgb))!=0;
      std::memcpy(previous,rgb,sizeof(rgb));++frames;
    }
  }
  K1EmotiscopeSnapshot after; k1_emotiscope_snapshot(&after);
  assert(after.audio_hops==6000 && after.render_frames==3000);
  assert(after.nonfinite_output_frames==0);
  assert(nonblack>2500 && changed>2000);
  assert(after.spectrogram_smooth[0]>=0.0f);
  for(unsigned i=0;i<64;++i)assert(std::isfinite(after.spectrogram_smooth[i]));
  for(unsigned i=0;i<96;++i)assert(std::isfinite(after.tempi_magnitude[i]));
  std::printf("visual portable engine: frames=%u changed=%u nonblack=%u all seven active modes exercised\n",frames,changed,nonblack);
}
#else
int main() { std::fprintf(stderr,"FAIL: native donor AP/VP engine is not implemented\n"); return 1; }
#endif
