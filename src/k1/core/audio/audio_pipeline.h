#pragma once

#include <cstdint>

#include "contract/audio_features_v1.h"
#include "core/audio/gdft_goertzel.h"
#include "core/audio/gdft_postprocess.h"
#include "core/audio/musical_saliency.h"
#include "core/audio/musical_time.h"
#include "core/audio/onset_beat.h"
#include "core/audio/tempo_tracker.h"

namespace k1::core::audio {

struct AudioPipelineInput final {
    const GdftSampleWindow* samples = nullptr;
    std::uint32_t sequence = 0;
    std::uint32_t frame_ms = 0;
    std::uint64_t capture_time_us = 0;
    std::uint64_t publish_time_us = 0;
    float peak_scaled = 0.0F;
    float vu_level = 0.0F;
    bool silence = true;
    GdftPostprocessConfig gdft{};
    // Optional canonical media coordinate for this analysis hop. Legacy/fixture
    // callers may omit it; production derives it from hop_end_sample_index.
    AudioTime media_time{};
    bool media_time_valid = false;
};

struct AudioPipelineOutput final {
    contract::AudioFeaturesV1 features{};
    TempoTrackerEvent tempo{};
    OnsetBeatFrame onset{};
    SaliencyUpdate saliency{};
    MusicalTime musical_time{};
    BeatEvent predicted_next_beat{};
    bool musical_time_valid = false;
    bool predicted_next_beat_valid = false;
    std::uint32_t gdft_overflow_count = 0;
    bool valid = false;
};

// Single-owner RT1062 AP composition. AudioFeaturesV1 remains the local
// snapshot/onset seam; tempo and saliency retain their legacy sidecar ownership.
class AudioPipeline final {
  public:
    AudioPipeline() noexcept;

    void reset() noexcept;
    [[nodiscard]] AudioPipelineOutput process(
        const AudioPipelineInput& input) noexcept;

    // Read-only view of the incumbent tempo state for observers such as the
    // TempoFieldV1 sidecar (core/audio/tempo_field.h). Const by construction:
    // an observer cannot feed back into the winner/flywheel trajectory.
    [[nodiscard]] const TempoTrackerState& tempoTrackerState() const noexcept {
        return tempo_state_;
    }

  private:
    GdftConfiguration gdft_configuration_{};
    GdftPostprocessState gdft_state_{};
    TempoTrackerState tempo_state_{};
    OnsetBeat onset_{};
    MusicalSaliency saliency_{};
    MusicalTime musical_time_{};
};

}  // namespace k1::core::audio
