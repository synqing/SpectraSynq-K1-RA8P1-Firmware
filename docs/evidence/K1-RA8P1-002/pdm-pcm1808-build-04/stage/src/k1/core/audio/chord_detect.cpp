#include "core/audio/chord_detect.h"

#include <cmath>

namespace k1::core::audio {
namespace {

float clamp01(float value) noexcept {
    if (!std::isfinite(value) || value < 0.0F) return 0.0F;
    if (value > 1.0F) return 1.0F;
    return value;
}

}  // namespace

ChordDetection detectChord(
    const std::array<float, contract::kChromaBinCount>& chroma) noexcept {
    ChordDetection result{};
    std::uint8_t root = 0;
    float root_value = chroma[0];
    float total_energy = chroma[0];
    for (std::uint8_t index = 1; index < contract::kChromaBinCount; ++index) {
        total_energy += chroma[index];
        if (chroma[index] > root_value) {
            root_value = chroma[index];
            root = index;
        }
    }
    result.root_a_origin = root;
    result.root_strength = root_value;

    const float minor_third = chroma[(root + 3U) % 12U];
    const float major_third = chroma[(root + 4U) % 12U];
    const float perfect_fifth = chroma[(root + 7U) % 12U];
    const float diminished_fifth = chroma[(root + 6U) % 12U];
    const float augmented_fifth = chroma[(root + 8U) % 12U];
    const bool has_minor_third = minor_third > major_third;
    result.third_strength = has_minor_third ? minor_third : major_third;

    if (perfect_fifth >= diminished_fifth &&
        perfect_fifth >= augmented_fifth) {
        result.fifth_strength = perfect_fifth;
        result.type = has_minor_third ? contract::ChordTypeV1::kMinor
                                      : contract::ChordTypeV1::kMajor;
    } else if (diminished_fifth > perfect_fifth &&
               diminished_fifth > augmented_fifth) {
        result.fifth_strength = diminished_fifth;
        result.type = contract::ChordTypeV1::kDiminished;
    } else {
        result.fifth_strength = augmented_fifth;
        result.type = contract::ChordTypeV1::kAugmented;
    }

    const float triad_energy = result.root_strength + result.third_strength +
                               result.fifth_strength;
    if (total_energy > 0.01F) {
        result.confidence =
            clamp01((triad_energy / total_energy) / 0.4F);
    } else {
        result.confidence = 0.0F;
        result.type = contract::ChordTypeV1::kNone;
    }
    if (result.confidence < 0.3F) {
        result.type = contract::ChordTypeV1::kNone;
    }
    return result;
}

}  // namespace k1::core::audio
