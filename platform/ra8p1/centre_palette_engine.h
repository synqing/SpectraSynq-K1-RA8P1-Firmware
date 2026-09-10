#pragma once
#include <cmath>
#include <cstdint>
#include "palette_transition.h"
#include "core/visual/channel_render_state.h"

namespace k1::titan {
// One radial coordinate and one signed travel direction. Both physical halves
// receive the same sample; no effect can accidentally become a left-to-right wipe.
inline constexpr unsigned kCentreEffectFirst = 100U;
inline constexpr unsigned kCentreEffectCount = 4U;
inline bool isCentreEffect(unsigned mode) noexcept {
  return mode >= kCentreEffectFirst && mode < kCentreEffectFirst + kCentreEffectCount;
}
inline const char* centreEffectName(unsigned mode) noexcept {
  switch (mode) {
    case 100: return "RIBBONS";
    case 101: return "AURORA_LAYERS";
    case 102: return "EMBER_TRAILS";
    case 103: return "PULSE_WAVES";
    default: return "K1_EXISTING_OR_PREVIEW";
  }
}
inline unsigned radialIndex(unsigned pixel) noexcept {
  return pixel < 80U ? 79U - pixel : pixel - 80U;
}
inline float centreTravelCoordinate(unsigned radial, bool inward,
                                    std::uint64_t now_us,
                                    std::uint32_t travel_ms) noexcept {
  const float distance = float(inward ? 79U-radial : radial) / 79.0F;
  const std::uint64_t period_us = std::uint64_t(travel_ms) * 1000U;
  // Reduce integer time before conversion, retaining precision at long uptime.
  const float travel = float(now_us % (period_us * 16U)) / float(period_us);
  return distance - travel;
}
struct CentreLayers { float body, accent, base; };
inline CentreLayers centreLayers(unsigned mode, float u) noexcept {
  constexpr float tau = 6.283185307179586F;
  const auto wave = [tau](float x) { return 0.5F + 0.5F * ::cosf(tau * x); };
  const auto square = [](float x) { return x*x; };
  switch (mode) {
    case 100: {
      const float broad = square(square(wave(u)));
      const float fine = square(square(square(wave(u * 2.0F + 0.18F))));
      return {0.06F + 0.60F*broad, 0.27F*fine, 0.015F};
    }
    case 101: {
      const float curtain = wave(u * 0.5F);
      const float fold = square(wave(u + 0.22F));
      const float silk = square(square(wave(u * 1.5F + 0.61F)));
      return {0.10F + 0.40F*curtain*fold, 0.20F*silk, 0.04F};
    }
    case 102: {
      const float p = u - ::floorf(u);
      const float distance = p < 0.5F ? p : 1.0F-p;
      const float head = ::expf(-distance*distance*1800.0F);
      // The tail is behind the crest in origin-to-destination coordinates.
      const float tail = ::expf(-9.0F*(1.0F-p));
      return {0.04F + 0.48F*tail, 0.43F*head, 0.01F};
    }
    case 103: {
      const float distance = u - 2.0F * ::floorf((u+1.0F)/2.0F);
      const float halo = ::expf(-distance*distance*24.0F);
      const float crest = ::expf(-distance*distance*220.0F);
      return {0.60F*halo, 0.32F*crest, 0.005F};
    }
    default: return {};
  }
}
inline void renderCentreEffect(core::visual::ChannelRenderState& channel,
                               const core::visual::PaletteTransition& palette,
                               std::uint64_t now_us, std::uint32_t travel_ms,
                               bool inward, unsigned previous_mode=0U,
                               float effect_mix=1.0F) noexcept {
  using namespace core::visual;
  const auto base = palette.hd(0.07F);
  const auto accent = palette.hd(palette.brightestPhase());
  const auto mode = channel.controls().mode_id;
  for (unsigned radial=0; radial<80U; ++radial) {
    const float u = centreTravelCoordinate(radial,inward,now_us,travel_ms);
    auto weights = centreLayers(mode,u);
    if (isCentreEffect(previous_mode) && effect_mix < 1.0F) {
      const auto old = centreLayers(previous_mode,u);
      weights = {old.body+(weights.body-old.body)*effect_mix,
                 old.accent+(weights.accent-old.accent)*effect_mix,
                 old.base+(weights.base-old.base)*effect_mix};
    }
    // Body, ground and crest all come from the selected palette mixture.
    // Floating-point composition lasts until the existing RGB8 frame boundary.
    const auto body = palette.hd(0.25F + u * 0.5F);
    const auto pixel = quantiseLinearRgb({
        body.red*weights.body + accent.red*weights.accent + base.red*weights.base,
        body.green*weights.body + accent.green*weights.accent + base.green*weights.base,
        body.blue*weights.body + accent.blue*weights.accent + base.blue*weights.base});
    channel.frame()[79U-radial] = channel.frame()[80U+radial] = pixel;
  }
}
} // namespace k1::titan
