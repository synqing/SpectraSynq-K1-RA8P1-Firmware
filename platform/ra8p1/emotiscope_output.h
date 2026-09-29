#pragma once
#include <cstdint>
#include "core/visual/ws2816_pack.h"
namespace k1::titan {
// Geometry adapter only: nearest original pixel, preserving both endpoints and
// the centre pair. No interpolation, second gamma, palette, or brightness here.
inline unsigned emotiscopeSourceIndex(unsigned destination) noexcept {
  return (destination * 127U + 79U) / 159U;
}
inline core::visual::Pixel16 emotiscopeWirePixel(const std::uint8_t* rgb,
                                                unsigned destination) noexcept {
  const unsigned p = emotiscopeSourceIndex(destination) * 3U;
  return {std::uint16_t(rgb[p] * 257U), std::uint16_t(rgb[p+1U] * 257U),
          std::uint16_t(rgb[p+2U] * 257U)};
}
// RGB8 expanded for the 16-bit wire format; this is not authored TRUE16.
// Output-precision candidate: same nearest mapping, 16-bit source, no x257.
inline core::visual::Pixel16 emotiscopeWirePixel16(const std::uint16_t* rgb16,
                                                  unsigned destination) noexcept {
  const unsigned p = emotiscopeSourceIndex(destination) * 3U;
  return {rgb16[p], rgb16[p+1U], rgb16[p+2U]};
}
}
