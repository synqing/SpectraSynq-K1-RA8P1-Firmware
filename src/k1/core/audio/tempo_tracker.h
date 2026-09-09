#pragma once

#include "core/audio/tempo_acf.h"
#include "core/audio/audio_rate_config.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace k1::core::audio {

inline constexpr std::size_t kTempoTrackerBinCount = 96;

struct TempoTrackerInput final {
    std::uint32_t frame_ms = 0;
    float novelty = 0.0F;
    bool silence = false;
};

struct TempoTrackerEvent final {
    float bpm = 0.0F;
    float phase01 = 0.0F;
    float confidence = 0.0F;
    bool beat_tick = false;
    bool locked = false;
    float beat_strength = 0.0F;
    // True only on the 3x-decimated novelty/tempo update. The two intervening
    // AP frames deliberately publish the previous tempo state with updated=false.
    bool updated = false;
};

struct TempoTrackerBin final {
    float target_bpm = 0.0F;
    float target_hz = 0.0F;
    float coefficient = 0.0F;
    float sine = 0.0F;
    float cosine = 0.0F;
    std::uint32_t block_size = 0;
    float phase = 0.0F;
    float phase_radians_per_second = 0.0F;
    float magnitude = 0.0F;
    float magnitude_raw = 0.0F;
};

struct TempoTrackerState final {
    std::array<TempoTrackerBin, kTempoTrackerBinCount> bins{};
    std::array<float, kTempoTrackerBinCount> smoothed{};
    std::array<float, kTempoTrackerBinCount> tempo_prior{};
    std::array<float, kTempoTrackerBinCount> confidence_prior{};
    TempoNoveltyRing novelty_history{};
    TempoAcfOutput acf{};

    std::uint16_t history_index = 0;
    float novelty_scale = 1.0F;
    std::uint8_t scale_count = 0;
    std::uint16_t calculation_bin = 0;
    std::uint16_t winner_bin = kTempoTrackerBinCount / 2U;
    std::uint16_t candidate_bin = kTempoTrackerBinCount / 2U;
    std::uint8_t candidate_frames = 0;
    float power_sum = 0.0F;
    float confidence = 0.0F;

    float confidence_ema = 0.0F;
    bool locked_v2 = false;
    std::uint16_t confidence_updates = 0;
    std::uint16_t subfloor_updates = 0;
    std::uint16_t beats_seen = 0;

    float flywheel_phase01 = 0.0F;
    float flywheel_run_bpm = 0.0F;
    bool flywheel_beat_tick = false;
    float onset_floor = 0.0F;
    float onset_deviation = 0.0F;
    float previous_novelty = 0.0F;
    float previous_previous_novelty = 0.0F;
    bool have_previous_novelty = false;
    float beats_since_tick = 1.0F;
    std::uint16_t coast_updates_remaining = 0;
    bool was_locked = false;
    bool flywheel_primed = false;
    float last_phase_advance = 0.0F;

    bool silence_detected = false;
    float silence_level = 0.0F;
    bool input_primed = false;
    float novelty_accumulator = 0.0F;
    float novelty_rate_hz = audioNoveltyRateHz(kProductionAudioRate);
    std::uint32_t last_emit_ms = 0;
    std::uint16_t frame_counter = 0;
    TempoTrackerEvent event{};
};

struct TempoTrackerDebug final {
    std::uint16_t bank_peak_bin = 0;
    float confidence_internal = 0.0F;
};

// Production source authority:
// SpectraSynq_K1_Firmware@c3e13ffdf1a3f8174b2c1dd3bf0af39dbe0c7b4c
// SPECTRASYNQ_K1_FIRMWARE/audio/k1_tempo.cpp. The portable form freezes the
// production K1_TEMPO_CONF_V2 + K1_TEMPO_FLYWHEEL_V2 path behind explicit state.
void initialiseTempoTracker(TempoTrackerState& state) noexcept;
void initialiseTempoTracker(TempoTrackerState& state,
                            AudioRateConfiguration rate) noexcept;
void resetTempoTracker(TempoTrackerState& state) noexcept;
void updateTempoTracker(TempoTrackerState& state,
                        const TempoTrackerInput& input) noexcept;

[[nodiscard]] TempoTrackerEvent readTempoTracker(
    const TempoTrackerState& state) noexcept;
[[nodiscard]] TempoTrackerDebug readTempoTrackerDebug(
    const TempoTrackerState& state) noexcept;

}  // namespace k1::core::audio
