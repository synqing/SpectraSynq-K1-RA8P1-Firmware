#pragma once

#include <cstdint>
#include <type_traits>

namespace k1::contract {

inline constexpr std::uint8_t kAudioFeatureSchemaMajor = 1;
inline constexpr std::uint8_t kAudioFeatureSchemaMinor = 0;
inline constexpr std::uint8_t kSpectrumBinCount = 80;
inline constexpr std::uint8_t kChromaBinCount = 12;

enum AudioValidityFlag : std::uint32_t {
    kValidBaseSnapshot = 1U << 0U,
    kValidSpectrum = 1U << 1U,
    kValidChroma = 1U << 2U,
    kValidChord = 1U << 3U,
    kValidOnsetV2 = 1U << 4U,
};

enum AudioEventFlag : std::uint32_t {
    kEventSilence = 1U << 0U,
    kEventOnset = 1U << 1U,
    kEventBassOnset = 1U << 2U,
    kEventBeat = 1U << 3U,
    kEventTransient = 1U << 4U,
    kEventKick = 1U << 5U,
    kEventSnare = 1U << 6U,
    kEventHihat = 1U << 7U,
};

enum class ChordTypeV1 : std::uint8_t {
    kNone = 0,
    kMajor = 1,
    kMinor = 2,
    kDiminished = 3,
    kAugmented = 4,
};

// Logical semantic frame shared by RT1062 AP, RT1062 VP, and host harnesses.
// This is not an inter-MCU wire structure and must never be sent to the S3.
struct AudioFeaturesV1 final {
    std::uint32_t sequence = 0;
    std::uint32_t source_frame_ms = 0;
    std::uint64_t capture_time_us = 0;
    std::uint64_t publish_time_us = 0;
    std::uint32_t validity_flags = 0;
    std::uint32_t event_flags = kEventSilence;

    float peak_scaled = 0.0F;
    float vu_level = 0.0F;
    float novelty = 0.0F;
    float spectral_energy = 0.0F;
    float low_energy = 0.0F;
    float mid_energy = 0.0F;
    float high_energy = 0.0F;
    float chroma_strength = 0.0F;

    std::uint8_t nyquist_safe_bin_hi = 0;
    float spectrum[kSpectrumBinCount] = {};
    float chroma_a_origin[kChromaBinCount] = {};

    ChordTypeV1 chord_type = ChordTypeV1::kNone;
    std::uint8_t chord_root_a_origin = 0;
    float chord_confidence = 0.0F;
    float chord_root_strength = 0.0F;
    float chord_third_strength = 0.0F;
    float chord_fifth_strength = 0.0F;

    std::uint32_t onset_event_id = 0;
    std::uint32_t onset_event_ms = 0;
    std::uint32_t onset_event_age_ms = 0;
    float onset_strength = 0.0F;
    float bass_onset_strength = 0.0F;
    float beat_phase = 0.0F;
    float beat_confidence = 0.0F;

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

static_assert(std::is_standard_layout_v<AudioFeaturesV1>);
static_assert(std::is_trivially_copyable_v<AudioFeaturesV1>);

}  // namespace k1::contract
