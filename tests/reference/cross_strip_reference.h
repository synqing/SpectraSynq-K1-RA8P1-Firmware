#pragma once
// Adapted from Lightwave CrossStripWaveInterferenceEffect at60009324ec3e7785c11db010343189d34e7314e0.
// Apache-2.0 source, LICENSE and NOTICE retained in reference/lightwave-cross-strip.
// Exact kernel output at the CRGB boundary, widened by257. No precision recovery
// or complete Lightwave renderer/output-treatment parity is claimed.
#include "core/visual/paired_wave_scene.h"
#include <algorithm>
#include <cmath>
namespace k1::reference {
class CrossStripReference final {
 public:
  float phase_offset=.375F,wavelength=12.F,drift_hz=.12F,contrast=.88F;
  unsigned speed=50,intensity=255,brightness=24;
  const core::visual::PairedFrame16& step(float dt) noexcept {
    phase_+=std::clamp(dt,.0001F,.05F)*drift_hz*(.65F+(float(speed)/50.F)*.70F);
    phase_-=std::floor(phase_);
    const float base=.14F*(1-contrast),gain=.55F+.45F*(float(intensity)/255.F);
    for(unsigned d=0;d<80;++d) {
      const float cycle=float(d)/std::clamp(wavelength,8.F,24.F)-phase_;
      const auto colour=[&](float phase,bool bottom) {
        const float f=phase-std::floor(phase);
        const float tooth=f<.5F?f*2:(1-f)*2;
        const auto value=unsigned(std::clamp((base+tooth*contrast)*gain*(float(brightness)/255.F),0.F,1.F)*255.F+.5F);
        const auto scale=[&](unsigned c){return std::uint16_t(((c*(value+1))>>8)*257);};
        return core::visual::Pixel16{scale(bottom?255:0),scale(bottom?132:176),scale(bottom?18:255)};
      };
      const auto p=core::visual::mirroredPair(d);
      frame_.top[p.left]=frame_.top[p.right]=colour(cycle,false);
      frame_.bottom[p.left]=frame_.bottom[p.right]=colour(cycle+phase_offset,true);
    }
    return frame_;
  }
 private:
  float phase_=0;
  core::visual::PairedFrame16 frame_{};
};
} // namespace k1::reference
