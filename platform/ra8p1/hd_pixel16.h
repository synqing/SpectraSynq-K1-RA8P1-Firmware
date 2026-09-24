#pragma once

#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"

#include <cstdint>
#include <cstring>

namespace k1::titan {

// Bit-level finiteness test: true unless the biased exponent field is all
// ones (Inf or NaN). Deliberately not std::isfinite(): under -ffast-math
// (-ffinite-math-only) the compiler is licensed to assume every value is
// finite and fold isfinite() to a constant true, silently changing
// quantiseHd16's +Inf case from 0 to 65535 (>=1.0F is true for +Inf) with no
// diagnostic. Pure integer bit manipulation has no such licence to fold.
// tests/host/test_hd_pixel16.cpp (scripts/test_hd_pixel16.py) compiles and
// runs this file under -ffast-math specifically to prove that.
inline bool isFiniteBits(float value) noexcept {
  std::uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  return (bits & 0x7F800000U) != 0x7F800000U;
}

// 16-bit quantiser for the HD palette sampler. Do not use Pixel8*257 here:
// that is REPLICATE8 and is what made WS2816 look 8-bit.
//
// DUR-010 (DualMCU docs/plans/deferred-upgrade-register.md): quantiseHd16 is
// NOT the same law as core::visual::wide::quantiseUnorm16 (imported at
// PIN_TIT2). Confirmed divergence, host-tested in
// tests/host/test_hd_pixel16.cpp: +Inf -> 0 here (isFiniteBits catches it,
// same as NaN) vs 65535 under quantiseUnorm16's own documented "+Inf ->
// 65535"; both agree on NaN (-> 0). Half-way boundaries also differ: this
// function rounds via a float32 multiply-add (`value * 65535.0F + 0.5F`,
// then a truncating cast); quantiseUnorm16 rounds via exact integer
// arithmetic on the IEEE-754 significand/exponent. 2026-09-24 disposition
// (register row, Captain-confirmed): quantiseHd16 has no live caller in this
// repository -- only its host test -- and the native path quantises once
// with the exact law at the wide endpoint (E5, core/visual/wide/wide_endpoint.cpp).
// DUR-010 is therefore "no live caller; not implemented": no switch is
// offered here, because a switch nothing reads is not a feature. The
// divergence vectors below remain the DUR-010 evidence.
inline core::visual::Pixel16 quantiseHd16(core::visual::PaletteLinearRgb colour) noexcept {
  const auto channel = [](float value) -> std::uint16_t {
    if (!isFiniteBits(value) || value <= 0.0F) return 0U;
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
