#pragma once

#include "core/visual/product_palette.h"
#include "core/visual/wide/wide_endpoint.h"
#include "core/visual/ws2816_pack.h"

#include <cmath>
#include <cstdint>

namespace k1::titan {

// 16-bit quantiser for the HD palette sampler. Do not use Pixel8*257 here:
// that is REPLICATE8 and is what made WS2816 look 8-bit.
//
// DUR-010 (DualMCU docs/plans/deferred-upgrade-register.md): this is NOT the
// same law as core::visual::wide::quantiseUnorm16 (imported at PIN_TIT2, see
// docs/reference-import-receipt-tit2.md). Confirmed divergence, host-tested
// in tests/host/test_hd_pixel16.cpp:
//   - +Inf: quantiseHd16 -> 0 (its isfinite() guard catches +Inf, same as
//     NaN); quantiseUnorm16 -> 65535 (its own doc comment: "+Inf -> 65535",
//     since only its explicit >=1.0F-equivalent check, not isfinite(), gates
//     the top end). NaN: both -> 0 (no divergence there, despite the task
//     framing -- verified below).
//   - Half-way boundaries: quantiseHd16 rounds via a float32 multiply-add
//     (`value * 65535.0F + 0.5F`, then a truncating cast), so a value whose
//     exact product lands one ULP either side of a half-integer can round
//     differently than exact arithmetic would. quantiseUnorm16 rounds via
//     exact integer arithmetic on the IEEE-754 significand/exponent -- no
//     float rounding or contraction can move a value across a code boundary.
// Neither law is changed here. quantiseHd16 (below) is untouched byte for
// byte; quantiseHd16Exact is new, and quantiseHd16Selected's default (false)
// reproduces quantiseHd16 exactly. No call site in this repository passes
// use_exact_law=true yet -- turning it on is a separate, deliberate choice
// for whichever lane picks up DUR-010.
inline core::visual::Pixel16 quantiseHd16(core::visual::PaletteLinearRgb colour) noexcept {
  const auto channel = [](float value) -> std::uint16_t {
    if (!std::isfinite(value) || value <= 0.0F) return 0U;
    if (value >= 1.0F) return 65535U;
    return static_cast<std::uint16_t>(value * 65535.0F + 0.5F);
  };
  return {channel(colour.red), channel(colour.green), channel(colour.blue)};
}

// The exact law, reusing DualMCU's own quantiseUnorm16 rather than
// reimplementing it (avoids drift between the two copies of "the" exact
// rounding rule).
inline core::visual::Pixel16 quantiseHd16Exact(core::visual::PaletteLinearRgb colour) noexcept {
  return {core::visual::wide::quantiseUnorm16(colour.red),
          core::visual::wide::quantiseUnorm16(colour.green),
          core::visual::wide::quantiseUnorm16(colour.blue)};
}

// DUR-010 selector. Default (use_exact_law = false) is byte-identical to
// quantiseHd16 for every input (host-tested, tests/host/test_hd_pixel16.cpp).
inline core::visual::Pixel16 quantiseHd16Selected(core::visual::PaletteLinearRgb colour,
                                                  bool use_exact_law) noexcept {
  return use_exact_law ? quantiseHd16Exact(colour) : quantiseHd16(colour);
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
