#pragma once

#include <array>
#include <cstdint>

#include "contract/audio_features_v1.h"

namespace k1::core::audio {

struct ChordDetection final {
    contract::ChordTypeV1 type = contract::ChordTypeV1::kNone;
    std::uint8_t root_a_origin = 0;
    float confidence = 0.0F;
    float root_strength = 0.0F;
    float third_strength = 0.0F;
    float fifth_strength = 0.0F;
};

// Stateless production K1_CHORD_V2 detector over A-origin pitch classes.
[[nodiscard]] ChordDetection detectChord(
    const std::array<float, contract::kChromaBinCount>& chroma) noexcept;

}  // namespace k1::core::audio
