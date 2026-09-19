/* Ported from DualMCU pin 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a core/visual/frame_blend.cpp — Titan D3 output composition. */
#include "core/visual/frame_blend.h"

#include <cmath>
#include <cstddef>

namespace k1::core::visual {
using k1::core::Pixel8;
using k1::core::PixelSpan;
namespace {

std::uint8_t blendChannel(const std::uint8_t previous,
                          const std::uint8_t current,
                          const float current_mix) noexcept {
  const float previous_mix = 1.0F - current_mix;
  int value = static_cast<int>(static_cast<float>(previous) * previous_mix +
                               static_cast<float>(current) * current_mix +
                               0.5F);
  if (value < 0) {
    value = 0;
  } else if (value > 255) {
    value = 255;
  }
  return static_cast<std::uint8_t>(value);
}

}  // namespace

void applyFrameBlending(const PixelSpan pixels, const PixelSpan previous,
                        const std::uint8_t mood,
                        const float delta_seconds) noexcept {
  if (pixels.data() == nullptr || previous.data() == nullptr ||
      pixels.size() == 0U || pixels.size() != previous.size()) {
    return;
  }
  if (!std::isfinite(delta_seconds) || delta_seconds < 0.0F) {
    return;
  }
  if (mood == 0U) {
    for (std::size_t index = 0U; index < pixels.size(); ++index) {
      previous[index] = pixels[index];
    }
    return;
  }

  const float coefficient =
      (static_cast<float>(mood) / 255.0F) * 0.92F;
  float corrected = coefficient;
  if (delta_seconds > 0.0F) {
    corrected = coefficient < 1.0e-6F
                    ? 0.0F
                    : ::powf(coefficient, delta_seconds * 120.0F);
  }
  if (corrected < 0.0F) {
    corrected = 0.0F;
  } else if (corrected > 1.0F) {
    corrected = 1.0F;
  }
  const float current_mix = 1.0F - corrected;
  for (std::size_t index = 0U; index < pixels.size(); ++index) {
    const Pixel8 blended{
        blendChannel(previous[index].red, pixels[index].red, current_mix),
        blendChannel(previous[index].green, pixels[index].green, current_mix),
        blendChannel(previous[index].blue, pixels[index].blue, current_mix)};
    pixels[index] = blended;
    previous[index] = blended;
  }
}

}  // namespace k1::core::visual
