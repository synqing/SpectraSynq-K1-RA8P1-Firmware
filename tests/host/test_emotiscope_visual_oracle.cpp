#include "../../platform/ra8p1/emotiscope_port.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>
struct RGB{float r,g,b;};
static uint32_t rng=0xACE1u;
static uint32_t rand32(){uint32_t x=rng;x^=x<<13;x^=x>>17;x^=x<<5;return rng=x;}
static RGB from_green_hsv(float v) {
  // Independent specialization of the declared donor default HSV(.33,.75,v).
  float chroma=v*.75f, offset=v-chroma;
  float red=chroma*(1.0f-std::fabs(std::fmod(.33f*6.0f,2.0f)-1.0f));
  return {red+offset,chroma+offset,offset};
}
static float tonemap(float v){
  if(v<.75f)return v;
  float x=(v-.75f)*4.0;return .75+.25*std::tanh(x);
}
int main(){
  assert(k1_emotiscope_init(0xACE1u));
  K1EmotiscopeConfig cfg;k1_emotiscope_default_config(&cfg);
  cfg.screensaver=false;assert(k1_emotiscope_set_config(&cfg));
  // Donor initializes 666 noise bits before its 384 dither channels.
  for(unsigned i=0;i<666;++i)rand32();
  float dither[384];for(float& v:dither)v=float(rand32()%255u)/255.0;
  RGB history[128]={};
  float master=0;
  K1EmotiscopeSnapshot before{},after{};
  uint8_t rgb[384];
  float chunk[64];
  float max_diff=0;
  uint32_t compared=0,byte_errors=0,nonzero=0;
  double t=0;
  for(uint32_t hop=1;hop<=6000;++hop){
    for(unsigned j=0;j<64;++j,t+=1.0/12800.0){
      double gain=std::fmod(t,.5)<.05?.60:.10;
      chunk[j]=float(gain*(.60*std::sin(t*6.283185307179586*110.0)
                       +.30*std::sin(t*6.283185307179586*440.0)));
    }
    assert(k1_emotiscope_push_pcm64(chunk,hop*5000));
    if(hop%2)continue;
    k1_emotiscope_snapshot(&before);
    assert(k1_emotiscope_render(hop*5000,rgb));
    k1_emotiscope_snapshot(&after);
    const float cutoff=.5+(1.0-std::sqrt(.25f))*14.5;
    const float alpha=1.0-std::exp(float(-6.28318530718*cutoff/100.0f));
    const float ai=1.0-alpha;
    if(hop*5>=1000 && master<1.0)master+=.001;
    const float m=std::min(1.0f,master);
    for(unsigned i=0;i<128;++i){
      unsigned bin=i<64?63-i:i-64;
      RGB v=from_green_hsv(before.spectrogram_smooth[bin]);
      RGB bg=from_green_hsv(1);
      v.r+=bg.r*.0625f;v.g+=bg.g*.0625f;v.b+=bg.b*.0625f;
      v.r=v.r*alpha+history[i].r*ai;
      v.g=v.g*alpha+history[i].g*ai;
      v.b=v.b*alpha+history[i].b*ai;
      history[i]=v;
      v.r=tonemap(v.r);v.g=tonemap(v.g);v.b=tonemap(v.b);
      v.g*=.9375f;v.b*=.84f;
      v.r*=m;v.g*=m;v.b*=m;
      v.r*=v.r;v.g*=v.g;v.b*=v.b;
      const float channels[]={v.r,v.g,v.b};
      for(unsigned c=0;c<3;++c){
        unsigned n=3*i+c;
        float diff=std::fabs(channels[c]-after.post_gamma_rgb[n]);
        max_diff=std::max(max_diff,diff);
        // Rounding agreement is assessed separately from float stage tolerance.
        assert(diff<2e-6f);
        // Quantizer oracle consumes the independently validated post-gamma float.
        // Its exact byte test is separate from the floating-stage tolerance.
        float scaled=after.post_gamma_rgb[n]*255.0f;
        uint8_t expected=uint8_t(scaled);
        float error=scaled-expected;
        if(error>=.055f)dither[n]+=error;
        if(dither[n]>=1.0f){++expected;dither[n]-=1.0f;}
        byte_errors+=expected!=rgb[n];
        nonzero+=rgb[n]!=0;++compared;
      }
    }
  }
  // The independent numeric derivation uses the same float storage boundaries.
  assert(byte_errors==0);
  assert(nonzero>compared/3);
  std::printf("donor Spectrum complete pipeline oracle: channels=%u max_float_error=%.9g byte_errors=%u nonzero=%u\n",compared,max_diff,byte_errors,nonzero);
}
