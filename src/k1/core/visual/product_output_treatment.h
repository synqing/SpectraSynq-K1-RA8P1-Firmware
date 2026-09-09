#pragma once

#include <cstdint>

#include "core/pixel.h"

namespace k1::core::visual {

struct ChannelVisualControls;

struct ProductOutputTreatmentState final {
  // Stored dither_phase is the used index. DualMCU has no legacy-synchronised
  // frame counter. The legacy increment-before walk is a rotation of the same
  // four phases; per-frame-pair byte parity is the criterion (G0.2 / WP0).
  std::uint8_t dither_phase = 0U;
};

void applyProductOutputTreatment(
    PixelSpan frame,
    const ChannelVisualControls& controls,
    ProductOutputTreatmentState& state) noexcept;

}  // namespace k1::core::visual
