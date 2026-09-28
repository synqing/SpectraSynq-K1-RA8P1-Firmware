#pragma once
// Behavioural adaptations of Connor Nishijima's GPL-3.0 Bloom implementations.
// SensoryBridge 5f329261987144242be4c3729f6d344631d3be1e lightshow_modes.h
// and led_utilities.h; Emotiscope 634b7ed8eefa2f1913ba1a1e7b29c116f22714d4
// src/light_modes/active/bloom.h and src/leds.h. Original notices/licences are
// retained in docs/effects-language-001/reference/donor-local/*/LICENSE.
// This boundary accepts donor-ready colour/VU. It does NOT reproduce donor AP,
// HSV construction, output treatment or physical LED output. SB uses retained
// FixedPoints 1.1.2 Q15.16 arithmetic; dependency provenance is recorded separately.
#include <array>
#include <algorithm>
#include <cmath>
#include "vendor/FixedPoints/src/FixedPoints.h"
#include "vendor/FixedPoints/src/FixedPointsCommon.h"
#include "core/visual/ribbon_material.h"

namespace k1::reference {
using Rgb=k1::core::visual::RibbonRgb;
using Image=std::array<Rgb,128>;
inline Rgb scaled(Rgb c,float x) { return {c.red*x,c.green*x,c.blue*x}; }
inline void plus(Rgb& a,Rgb b) { a.red+=b.red; a.green+=b.green; a.blue+=b.blue; }

// Preserve the original scatter order and origin replacement. A different
// resampler that matches only centroid motion does not implement this kernel.
class SensoryRgbBloom {
 public:
  struct FixedRgb { SQ15x16 r{},g{},b{}; };
  std::array<FixedRgb,128> fixed_history{};
  Image history{}, frame{};
  void step(Rgb deposited_colour,float mood) {
    std::array<FixedRgb,128> fixed_frame{};
    const float position=.250F+1.750F*mood;
    const int whole=static_cast<int>(position);
    const SQ15x16 right=position-whole,left=1.0-right,alpha=.99;
    for(int i=0;i<128;++i) {
      const int lo=i+whole,hi=lo+1;
      if(lo>=0 && lo<128) {
        fixed_frame[lo].r+=fixed_history[i].r*left*alpha;
        fixed_frame[lo].g+=fixed_history[i].g*left*alpha;
        fixed_frame[lo].b+=fixed_history[i].b*left*alpha;
      }
      if(hi>=0 && hi<128) {
        fixed_frame[hi].r+=fixed_history[i].r*right*alpha;
        fixed_frame[hi].g+=fixed_history[i].g*right*alpha;
        fixed_frame[hi].b+=fixed_history[i].b*right*alpha;
      }
    }
    fixed_frame[63]=fixed_frame[64]={deposited_colour.red,deposited_colour.green,deposited_colour.blue};
    fixed_history=fixed_frame; // Original retains BEFORE outer fade and mirroring.
    for(unsigned i=0;i<128;++i) history[i]={float(fixed_history[i].r),float(fixed_history[i].g),float(fixed_history[i].b)};
    for(unsigned i=0;i<32;++i) {
      const float p=i/31.0F;
      fixed_frame[127-i].r*=p*p; fixed_frame[127-i].g*=p*p; fixed_frame[127-i].b*=p*p;
    }
    for(unsigned i=0;i<128;++i) frame[i]={float(fixed_frame[i].r),float(fixed_frame[i].g),float(fixed_frame[i].b)};
    for(unsigned i=0;i<64;++i) frame[i]=frame[127-i];
  }
};

class EmotiscopeScalarBloom {
 public:
  std::array<float,128> history{};
  Image frame{};
  // Colours supplied by an explicitly separate current-palette adapter. For
  // exact donor feature tests, set both endpoints to the intended donor colour.
  void step(float vu,float speed,const std::array<Rgb,64>& current_colour) {
    std::array<float,128> next{};
    const float position=.125F+.875F*speed;
    const int whole=static_cast<int>(position);
    const float right=position-whole,left=1-right;
    for(int i=0;i<128;++i) {
      const int lo=i+whole,hi=lo+1;
      if(lo>=0 && lo<128) next[lo]+=history[i]*left*.99F;
      if(hi>=0 && hi<128) next[hi]+=history[i]*right*.99F;
    }
    next[0]=std::min(1.0F,vu);
    for(unsigned i=0;i<64;++i) {
      const Rgb c=scaled(current_colour[i],std::clamp(next[i],0.0F,1.0F));
      frame[64+i]=frame[63-i]=c;
    }
    history=next;
  }
};

// Declared comparison-only remap: radial 0..63 to logical 0..79, linear
// interpolation. This is neither a second product channel nor donor parity.
inline k1::core::visual::RibbonFrame toK1(const Image& image) {
  k1::core::visual::RibbonFrame out{};
  for(unsigned d=0;d<k1::core::visual::kPixelsPerHalf;++d) {
    const float x=d*63.0F/(k1::core::visual::kPixelsPerHalf-1);
    const unsigned i=static_cast<unsigned>(x),j=std::min(i+1,63U);
    Rgb c=scaled(image[64+i],1-(x-i)); plus(c,scaled(image[64+j],x-i));
    const auto pair=k1::core::visual::mirroredPair(d); out[pair.left]=out[pair.right]=c;
  }
  return out;
}
} // namespace k1::reference
