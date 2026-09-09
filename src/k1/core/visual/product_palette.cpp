#include "core/visual/product_palette.h"

#include <cmath>

namespace k1::core::visual {
namespace {

float finiteClamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

float wrappedPhase(float phase) noexcept {
  if (!std::isfinite(phase)) {
    return 0.0F;
  }
  phase -= ::floorf(phase);
  return phase < 0.0F ? phase + 1.0F : phase;
}

std::uint8_t quantiseUnit(const float value) noexcept {
  return static_cast<std::uint8_t>(finiteClamp01(value) * 255.0F);
}

std::uint8_t scale8Fixed(const std::uint8_t value,
                         const std::uint8_t scale) noexcept {
  return static_cast<std::uint8_t>(
      (static_cast<std::uint16_t>(value) *
       static_cast<std::uint16_t>(scale + 1U)) >>
      8U);
}

std::uint8_t blendFastLed(const std::uint8_t first,
                          const std::uint8_t second,
                          const std::uint8_t fraction) noexcept {
  const std::uint8_t second_weight =
      static_cast<std::uint8_t>(fraction << 4U);
  const std::uint8_t first_weight =
      static_cast<std::uint8_t>(255U - second_weight);
  return static_cast<std::uint8_t>(scale8Fixed(first, first_weight) +
                                   scale8Fixed(second, second_weight));
}

std::uint8_t applyFastLedBrightness(const std::uint8_t value,
                                    const std::uint8_t brightness) noexcept {
  if (brightness == 255U) {
    return value;
  }
  if (brightness == 0U || value == 0U) {
    return 0U;
  }
  const std::uint8_t adjusted = static_cast<std::uint8_t>(brightness + 1U);
  return scale8Fixed(value, adjusted);
}

}  // namespace

PaletteLinearRgb sampleProductPaletteHd(const std::uint16_t palette_id,
                                        float phase,
                                        const float level) noexcept {
  const ProductPaletteDescriptor& palette = productPalette(palette_id);
  phase = wrappedPhase(phase);
  const float bounded_level = finiteClamp01(level);
  if (palette.stop_count == 0U || palette.stops == nullptr) {
    return {};
  }

  std::uint8_t first = 0U;
  while (static_cast<std::uint8_t>(first + 1U) < palette.stop_count &&
         static_cast<float>(palette.stops[first + 1U].position) / 255.0F <
             phase) {
    ++first;
  }

  const ProductPaletteStop* selected = &palette.stops[first];
  PaletteLinearRgb colour{};
  if (first + 1U >= palette.stop_count ||
      phase <= static_cast<float>(palette.stops[0].position) / 255.0F) {
    if (first + 1U >= palette.stop_count) {
      selected = &palette.stops[palette.stop_count - 1U];
    }
    colour = {static_cast<float>(selected->colour.red) / 255.0F,
              static_cast<float>(selected->colour.green) / 255.0F,
              static_cast<float>(selected->colour.blue) / 255.0F};
  } else {
    const ProductPaletteStop& next = palette.stops[first + 1U];
    const float first_position =
        static_cast<float>(selected->position) / 255.0F;
    const float next_position = static_cast<float>(next.position) / 255.0F;
    const float span = next_position - first_position;
    float amount = span > 1.0e-6F ? (phase - first_position) / span : 0.0F;
    amount = finiteClamp01(amount);
    colour = {
        (static_cast<float>(selected->colour.red) +
         (static_cast<float>(next.colour.red) -
          static_cast<float>(selected->colour.red)) * amount) / 255.0F,
        (static_cast<float>(selected->colour.green) +
         (static_cast<float>(next.colour.green) -
          static_cast<float>(selected->colour.green)) * amount) / 255.0F,
        (static_cast<float>(selected->colour.blue) +
         (static_cast<float>(next.colour.blue) -
          static_cast<float>(selected->colour.blue)) * amount) / 255.0F};
  }
  colour.red *= bounded_level;
  colour.green *= bounded_level;
  colour.blue *= bounded_level;
  return colour;
}

Pixel8 sampleProductPaletteFastLed16(const std::uint16_t palette_id,
                                     const std::uint8_t index,
                                     const std::uint8_t brightness) noexcept {
  const ProductPaletteDescriptor& palette = productPalette(palette_id);
  const std::uint8_t slot = static_cast<std::uint8_t>(index >> 4U);
  const std::uint8_t fraction = static_cast<std::uint8_t>(index & 0x0FU);
  Pixel8 colour = palette.compiled_fastled16[slot];
  if (fraction != 0U) {
    const Pixel8& next = palette.compiled_fastled16[
        slot == 15U ? 0U : static_cast<std::uint8_t>(slot + 1U)];
    colour = {blendFastLed(colour.red, next.red, fraction),
              blendFastLed(colour.green, next.green, fraction),
              blendFastLed(colour.blue, next.blue, fraction)};
  }
  colour.red = applyFastLedBrightness(colour.red, brightness);
  colour.green = applyFastLedBrightness(colour.green, brightness);
  colour.blue = applyFastLedBrightness(colour.blue, brightness);
  return colour;
}

Pixel8 quantiseLinearRgb(const PaletteLinearRgb colour) noexcept {
  return {quantiseUnit(colour.red), quantiseUnit(colour.green),
          quantiseUnit(colour.blue)};
}

Pixel8 addSaturating(const Pixel8 left, const Pixel8 right) noexcept {
  const auto channel = [](const std::uint8_t a, const std::uint8_t b) {
    const std::uint16_t sum = static_cast<std::uint16_t>(a) + b;
    return static_cast<std::uint8_t>(sum > 255U ? 255U : sum);
  };
  return {channel(left.red, right.red), channel(left.green, right.green),
          channel(left.blue, right.blue)};
}

Pixel8 scalePixel(const Pixel8 colour, const float level) noexcept {
  const float bounded = finiteClamp01(level);
  return {static_cast<std::uint8_t>(static_cast<float>(colour.red) * bounded),
          static_cast<std::uint8_t>(static_cast<float>(colour.green) * bounded),
          static_cast<std::uint8_t>(static_cast<float>(colour.blue) * bounded)};
}

}  // namespace k1::core::visual
