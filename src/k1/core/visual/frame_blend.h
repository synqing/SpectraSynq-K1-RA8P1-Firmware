/* Ported from DualMCU pin 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a core/visual/frame_blend.h — Titan D3 output composition. */
#pragma once

#include <cstdint>

#include "core/pixel.h"

namespace k1::core::visual {
using k1::core::PixelSpan;

void applyFrameBlending(PixelSpan pixels, PixelSpan previous,
                        std::uint8_t mood,
                        float delta_seconds) noexcept;

}  // namespace k1::core::visual
