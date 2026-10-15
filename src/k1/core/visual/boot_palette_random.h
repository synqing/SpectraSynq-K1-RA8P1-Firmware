#pragma once
// Random start-up palettes (Captain, 15 Oct 2026). Once per power-on, after
// kBootPaletteFrames frames, pick one warm/cool pair from kBootPalettePairs and
// a random edge for the warm one. The seed mixes the low bits of the live
// microphone spectrum and loudness (mic self-noise differs every power-on, even
// in a silent room) with the render loop's timestamps. The two edges never
// share a colour family. A host palette change before the pick cancels it.
#include <cstdint>
#include <cstring>

#include "core/visual/boot_palette_pairs.h"

namespace k1::core::visual {

inline constexpr unsigned kBootPaletteFrames = 50U;  // 0.5 s at the 10 ms render period

struct BootPaletteSeed final {
  std::uint32_t hash = 2166136261U;  // FNV-1a offset basis
  unsigned frames = 0U;
  bool done = false;
};

inline void bootPaletteMix(BootPaletteSeed& s, std::uint32_t word) noexcept {
  for (unsigned i = 0; i < 4U; ++i) {
    s.hash ^= (word >> (8U * i)) & 0xFFU;
    s.hash *= 16777619U;  // FNV-1a prime
  }
}

inline void bootPaletteMixFloat(BootPaletteSeed& s, float value) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof bits);
  bootPaletteMix(s, bits);
}

struct BootPaletteChoice final {
  std::uint32_t lower;  // board 0
  std::uint32_t upper;  // board 1
};

// Final avalanche so neighbouring hashes land on unrelated pairs.
inline BootPaletteChoice bootPalettePick(std::uint32_t hash) noexcept {
  std::uint32_t x = hash;
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  const auto& pair = kBootPalettePairs[x % kBootPalettePairs.size()];
  const bool warm_upper = ((x / kBootPalettePairs.size()) & 1U) != 0U;
  return warm_upper ? BootPaletteChoice{pair.cool, pair.warm} : BootPaletteChoice{pair.warm, pair.cool};
}

}  // namespace k1::core::visual
