#pragma once

#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"

#include <cmath>
#include <cstdint>

namespace k1::titan {

// 16-bit quantiser for the HD palette sampler. Do not use Pixel8*257 here:
// that is REPLICATE8 and is what made WS2816 look 8-bit.
inline core::visual::Pixel16 quantiseHd16(core::visual::PaletteLinearRgb colour) noexcept {
  const auto channel = [](float value) -> std::uint16_t {
    if (!std::isfinite(value) || value <= 0.0F) return 0U;
    if (value >= 1.0F) return 65535U;
    return static_cast<std::uint16_t>(value * 65535.0F + 0.5F);
  };
  return {channel(colour.red), channel(colour.green), channel(colour.blue)};
}

inline core::visual::Pixel16 liftPixel8(core::Pixel8 pixel) noexcept {
  return {static_cast<std::uint16_t>(std::uint16_t(pixel.red) * 257U),
          static_cast<std::uint16_t>(std::uint16_t(pixel.green) * 257U),
          static_cast<std::uint16_t>(std::uint16_t(pixel.blue) * 257U)};
}

inline std::uint16_t scale16(std::uint16_t value, std::uint32_t brightness) noexcept {
  if (brightness >= 255U) return value;
  if (brightness == 0U || value == 0U) return 0U;
  return static_cast<std::uint16_t>((std::uint32_t(value) * brightness) / 255U);
}

inline std::uint8_t dither16to8(std::uint16_t value, unsigned index,
                                std::uint8_t phase) noexcept {
  const std::uint8_t high = static_cast<std::uint8_t>(value >> 8);
  const std::uint8_t fraction = static_cast<std::uint8_t>(value);
  constexpr std::uint8_t threshold[4] = {32U, 96U, 160U, 224U};
  if (fraction >= threshold[(index + phase) & 3U] && high < 255U) {
    return static_cast<std::uint8_t>(high + 1U);
  }
  return high;
}

}  // namespace k1::titan
