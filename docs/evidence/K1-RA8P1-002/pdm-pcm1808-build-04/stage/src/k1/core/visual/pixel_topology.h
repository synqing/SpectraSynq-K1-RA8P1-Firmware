#pragma once

#include <cstddef>
#include <cstdint>

namespace k1::core::visual {

enum class PixelChannelId : std::uint8_t {
  kChannelA = 0U,
  kChannelB = 1U,
};

inline constexpr std::size_t kChannelCount = 2U;
inline constexpr std::size_t kPixelsPerChannel = 160U;
inline constexpr std::size_t kPixelsPerHalf = 80U;
inline constexpr std::size_t kCentreLeft = 79U;
inline constexpr std::size_t kCentreRight = 80U;

struct MirroredPair final {
  std::size_t left;
  std::size_t right;
};

[[nodiscard]] constexpr bool validChannel(
    const PixelChannelId channel) noexcept {
  return channel == PixelChannelId::kChannelA ||
         channel == PixelChannelId::kChannelB;
}

[[nodiscard]] constexpr bool validDistance(
    const std::size_t distance) noexcept {
  return distance < kPixelsPerHalf;
}

[[nodiscard]] constexpr MirroredPair mirroredPair(
    const std::size_t distance) noexcept {
  return {kCentreLeft - distance, kCentreRight + distance};
}

}  // namespace k1::core::visual
