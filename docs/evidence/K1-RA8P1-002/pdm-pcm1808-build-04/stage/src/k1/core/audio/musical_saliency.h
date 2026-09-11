#pragma once

#include <cstdint>

#include "contract/audio_features_v1.h"

namespace k1::core::audio {

enum class SaliencyType : std::uint8_t {
    kHarmonic = 0,
    kRhythmic = 1,
    kTimbral = 2,
    kDynamic = 3,
};

struct SaliencyAxisFrame final {
    float harmonic_novelty = 0.0F;
    float rhythmic_novelty = 0.0F;
    float timbral_novelty = 0.0F;
    float dynamic_novelty = 0.0F;

    float harmonic_novelty_smooth = 0.0F;
    float rhythmic_novelty_smooth = 0.0F;
    float timbral_novelty_smooth = 0.0F;
    float dynamic_novelty_smooth = 0.0F;

    float overall_saliency = 0.0F;
    SaliencyType dominant_type = SaliencyType::kDynamic;
};

struct SaliencyEvent final {
    std::uint32_t frame_ms = 0;
    bool salient = false;
    float overall_saliency = 0.0F;
    float adaptive_threshold = 0.0F;
    std::uint16_t age_ms = 0;
    std::uint16_t flags = 0;
};

struct SaliencyUpdate final {
    SaliencyAxisFrame axis{};
    SaliencyEvent event{};
    bool input_accepted = false;
    bool event_emitted = false;
};

class MusicalSaliency final {
  public:
    MusicalSaliency() noexcept = default;

    void reset() noexcept;
    [[nodiscard]] SaliencyUpdate update(
        const contract::AudioFeaturesV1& audio) noexcept;

  private:
    std::uint32_t previous_frame_ms_ = 0U;
    float previous_chroma_strength_ = 0.0F;
    float previous_novelty_ = 0.0F;
    float previous_fast_flux_novelty_ = 0.0F;
    float previous_energy_ = 0.0F;
    float adaptive_floor_ = 0.0F;
    std::uint32_t last_event_ms_ = 0;
    std::uint8_t previous_chord_root_ = 0xFFU;
    std::uint8_t previous_chord_type_ = 0xFFU;
    SaliencyAxisFrame axis_{};
    bool warmed_ = false;
};

}  // namespace k1::core::audio
