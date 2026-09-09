#pragma once

#include <array>
#include <cstdint>

#include "contract/audio_features_v1.h"

namespace k1::core::audio {

struct OnsetBeatFrame final {
    std::uint32_t event_id = 0;
    std::uint32_t event_ms = 0;
    std::uint32_t event_age_ms = 0;
    bool onset = false;
    bool bass_onset = false;
    bool beat = false;
    float onset_strength = 0.0F;
    float bass_onset_strength = 0.0F;
    float beat_phase = 0.0F;
    float beat_confidence = 0.0F;

    bool transient = false;
    bool kick = false;
    bool snare = false;
    bool hihat = false;
    float transient_strength = 0.0F;
    float kick_strength = 0.0F;
    float snare_strength = 0.0F;
    float hihat_strength = 0.0F;
    float transient_level = 0.0F;
    float kick_level = 0.0F;
    float snare_level = 0.0F;
    float hihat_level = 0.0F;
    std::uint32_t transient_event_id = 0;
    std::uint32_t kick_event_id = 0;
    std::uint32_t snare_event_id = 0;
    std::uint32_t hihat_event_id = 0;
};

// Production K1_ONSET_V2 detector and its unchanged legacy IOI beat-lock tail.
// Single-owner state replaces Arduino publication locks; no allocation occurs.
class OnsetBeat final {
  public:
    OnsetBeat() noexcept = default;

    void reset() noexcept;
    [[nodiscard]] OnsetBeatFrame update(
        const contract::AudioFeaturesV1& audio) noexcept;
    [[nodiscard]] const OnsetBeatFrame& read() const noexcept { return event_; }

  private:
    struct BandState final {
        float flux_mean = 0.0F;
        float previous_flux = 0.0F;
        float previous2_flux = 0.0F;
        std::uint32_t last_trigger_frame = 0;
    };

    struct DetectorV2 final {
        std::array<float, contract::kSpectrumBinCount> previous_spectrum{};
        bool has_previous = false;
        std::array<float, 16> flux_ring{};
        std::uint8_t flux_write_index = 0;
        std::uint8_t flux_count = 0;
        std::array<float, 16> envelope_ring{};
        std::uint8_t envelope_write_index = 0;
        std::uint32_t last_event_frame = 0;
        std::uint32_t frame_count = 0;
        BandState bass{};
        BandState mid{};
        BandState high{};
        float transient_level = 0.0F;
        float kick_level = 0.0F;
        float snare_level = 0.0F;
        float hihat_level = 0.0F;
        std::uint32_t transient_id = 0;
        std::uint32_t kick_id = 0;
        std::uint32_t snare_id = 0;
        std::uint32_t hihat_id = 0;
    };

    [[nodiscard]] float bandFlux(const float* spectrum,
                                 const float* previous,
                                 std::uint8_t low,
                                 std::uint8_t high) const noexcept;
    [[nodiscard]] float median(const float* ring, std::uint8_t count) const noexcept;
    [[nodiscard]] float thresholdEnvelope(float flux) noexcept;
    [[nodiscard]] float peakPick(float envelope, bool emit) noexcept;
    [[nodiscard]] float bandTrigger(BandState& state,
                                    float flux,
                                    float threshold_multiplier,
                                    float alpha,
                                    std::uint8_t refractory_frames,
                                    bool emit) noexcept;
    [[nodiscard]] bool runV2(const contract::AudioFeaturesV1& audio,
                             OnsetBeatFrame& event) noexcept;
    void decayBeatLock() noexcept;
    void noteAcceptedInterval(std::uint32_t interval_ms) noexcept;

    DetectorV2 detector_{};
    OnsetBeatFrame event_{};
    std::uint32_t onset_last_ms_ = 0;
    std::uint32_t last_accept_ms_ = 0;
    std::uint32_t last_interval_ms_ = 0;
    std::uint32_t interval_estimate_ms_ = 0;
    std::uint8_t stable_intervals_ = 0;
    float novelty_fast_ = 0.0F;
    float novelty_slow_ = 0.0F;
    float low_fast_ = 0.0F;
    float low_slow_ = 0.0F;
    float peak_fast_ = 0.0F;
    float peak_slow_ = 0.0F;
    float previous_novelty_ = 0.0F;
    float previous_low_energy_ = 0.0F;
    float previous_peak_ = 0.0F;
    bool primed_ = false;
};

}  // namespace k1::core::audio
