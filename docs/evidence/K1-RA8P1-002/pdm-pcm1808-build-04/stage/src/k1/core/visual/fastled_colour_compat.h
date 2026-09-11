#pragma once

#include <cstdint>

#include "core/pixel.h"

namespace k1::core::visual {

struct Hsv8 final {
  std::uint8_t hue;
  std::uint8_t saturation;
  std::uint8_t value;
};

[[nodiscard]] Hsv8 rgbToFastLedHsvApproximate(Pixel8 colour) noexcept;
[[nodiscard]] Pixel8 fastLedHsvToRgbRainbow(Hsv8 colour) noexcept;
[[nodiscard]] Pixel8 forceFastLedSaturation(Pixel8 colour,
                                            std::uint8_t saturation) noexcept;
[[nodiscard]] Pixel8 forceFastLedHue(Pixel8 colour,
                                     std::uint8_t hue) noexcept;

}  // namespace k1::core::visual
