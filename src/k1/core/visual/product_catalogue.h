#pragma once

#include <array>
#include <cstdint>

namespace k1::core::visual {

inline constexpr std::uint16_t kProductModeCount = 38U;
inline constexpr std::uint16_t kProductPaletteCount = 44U;
inline constexpr std::uint16_t kDefaultProductModeId = 3U;
inline constexpr std::uint16_t kDefaultProductPaletteId = 0U;

struct ProductModeDescriptor final {
  std::uint16_t id;
  const char* name;
  bool product_enabled;
};

[[nodiscard]] const std::array<ProductModeDescriptor, kProductModeCount>&
productModeCatalogue() noexcept;
[[nodiscard]] const ProductModeDescriptor* productMode(
    std::uint16_t id) noexcept;
[[nodiscard]] bool productModeEnabled(std::uint16_t id) noexcept;
[[nodiscard]] std::uint16_t sanitiseProductMode(std::uint16_t id) noexcept;
[[nodiscard]] std::uint16_t sanitiseProductPalette(
    std::uint16_t id) noexcept;

}  // namespace k1::core::visual
