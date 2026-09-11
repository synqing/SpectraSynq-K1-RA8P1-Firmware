#pragma once

#include "core/visual/pixel_topology.h"

#include <cstddef>
#include <cstdint>

namespace k1::core::visual {

struct Pixel16 final {
  std::uint16_t red = 0;
  std::uint16_t green = 0;
  std::uint16_t blue = 0;
};

inline constexpr std::size_t kPackedBytesPerPixel = 6U;
inline constexpr std::size_t kPackedBytesPerLane =
    kPixelsPerHalf * kPackedBytesPerPixel;  // 480
// Do not add kPackedSlotsPerLane (= 160 wire slots). It collides with
// kPixelsPerChannel = 160 and is unused.

// Wire bytes are G_hi, G_lo, R_hi, R_lo, B_hi, B_lo. Legacy packs the same
// six bytes into FastLED CRGB(G_hi, G_lo, R_hi) / CRGB(R_lo, B_hi, B_lo)
// and they land unswapped only because that path drives a raw RGB-ordered
// controller. This bit-bang path has no reordering layer, so the raw byte
// stream already matches the wire. Do not "fix" a GRB swap that is not there.

inline void packPixel(const Pixel16& px, std::uint8_t out[6]) noexcept {
  out[0] = static_cast<std::uint8_t>(px.green >> 8);
  out[1] = static_cast<std::uint8_t>(px.green & 0xFF);
  out[2] = static_cast<std::uint8_t>(px.red >> 8);
  out[3] = static_cast<std::uint8_t>(px.red & 0xFF);
  out[4] = static_cast<std::uint8_t>(px.blue >> 8);
  out[5] = static_cast<std::uint8_t>(px.blue & 0xFF);
}

inline bool splitChannel160(const Pixel16* pixels, std::size_t count,
                            std::uint8_t* lane_a,
                            std::uint8_t* lane_b) noexcept {
  if (pixels == nullptr || lane_a == nullptr || lane_b == nullptr) {
    return false;
  }
  if (count != kPixelsPerChannel) {
    return false;
  }
  for (std::size_t i = 0; i < kPixelsPerHalf; ++i) {
    packPixel(pixels[i], lane_a + i * 6U);
    packPixel(pixels[kPixelsPerHalf + i], lane_b + i * 6U);
  }
  return true;
}

}  // namespace k1::core::visual
