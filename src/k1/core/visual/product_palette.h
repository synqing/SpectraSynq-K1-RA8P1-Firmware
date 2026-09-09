#pragma once

#include <array>
#include <cstdint>

#include "core/pixel.h"
#include "core/visual/product_catalogue.h"

namespace k1::core::visual {

struct ProductPaletteStop final {
  std::uint8_t position;
  Pixel8 colour;
};

struct ProductPaletteDescriptor final {
  std::uint16_t id;
  const char* name;
  const ProductPaletteStop* stops;
  std::uint8_t stop_count;
  const Pixel8* compiled_fastled16;
};

struct PaletteLinearRgb final {
  float red;
  float green;
  float blue;
};

[[nodiscard]] const std::array<ProductPaletteDescriptor,
                               kProductPaletteCount>&
productPaletteCatalogue() noexcept;
[[nodiscard]] const ProductPaletteDescriptor& productPalette(
    std::uint16_t id) noexcept;

[[nodiscard]] PaletteLinearRgb sampleProductPaletteHd(
    std::uint16_t palette_id, float phase, float level = 1.0F) noexcept;
[[nodiscard]] Pixel8 sampleProductPaletteFastLed16(
    std::uint16_t palette_id, std::uint8_t index,
    std::uint8_t brightness = 255U) noexcept;

[[nodiscard]] Pixel8 quantiseLinearRgb(PaletteLinearRgb colour) noexcept;
[[nodiscard]] Pixel8 addSaturating(Pixel8 left, Pixel8 right) noexcept;
[[nodiscard]] Pixel8 scalePixel(Pixel8 colour, float level) noexcept;

}  // namespace k1::core::visual
