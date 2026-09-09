#include "core/audio/musical_saliency.h"

#include <cmath>

namespace k1::core::audio {
namespace {

// Production K1_CHORD_V2 source snapshot:
//   SpectraSynq_K1_Firmware@c3e13ffdf1a3f8174b2c1dd3bf0af39dbe0c7b4c
//   SPECTRASYNQ_K1_FIRMWARE/audio/k1_musical_saliency.cpp
//   SHA-256 e9fdb6e89358b8a63127adb6c509355f684b37b035a09838ce552bbec6e41a2e
// Arithmetic and state-update order are preserved. Critical-section
// publication is deliberately outside this portable single-owner core.

constexpr float kMillisecondsToSeconds = 0.001F;
constexpr float kHarmonicRiseTime = 0.15F;
constexpr float kHarmonicFallTime = 0.80F;
constexpr float kRhythmicRiseTime = 0.05F;
constexpr float kRhythmicFallTime = 0.30F;
constexpr float kTimbralRiseTime = 0.10F;
constexpr float kTimbralFallTime = 0.50F;
constexpr float kDynamicRiseTime = 0.08F;
constexpr float kDynamicFallTime = 0.40F;
constexpr float kFluxDerivativeThreshold = 0.05F;
constexpr float kRmsDerivativeThreshold = 0.02F;
constexpr float kHarmonicWeight = 0.25F;
constexpr float kRhythmicWeight = 0.30F;
constexpr float kTimbralWeight = 0.20F;
constexpr float kDynamicWeight = 0.25F;
constexpr float kAdaptiveFloorRetentionPer8Ms = 0.995F;
constexpr float kAdaptiveFloorReferenceSeconds = 0.008F;

float clamp01(float value) noexcept {
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    if (value < 0.0F) {
        return 0.0F;
    }
    if (value > 1.0F) {
        return 1.0F;
    }
    return value;
}

float asymmetricSmooth(float current,
                       float target,
                       float rise_time,
                       float fall_time,
                       float delta_seconds) noexcept {
    const float time =
        delta_seconds > 0.0F ? delta_seconds : kMillisecondsToSeconds;
    if (rise_time < kMillisecondsToSeconds) {
        rise_time = kMillisecondsToSeconds;
    }
    if (fall_time < kMillisecondsToSeconds) {
        fall_time = kMillisecondsToSeconds;
    }

    const float rise_alpha = 1.0F - std::exp(-time / rise_time);
    const float fall_alpha = 1.0F - std::exp(-time / fall_time);
    const float alpha = target >= current ? rise_alpha : fall_alpha;
    return current + (target - current) * alpha;
}

}  // namespace

void MusicalSaliency::reset() noexcept {
    previous_frame_ms_ = 0U;
    previous_chroma_strength_ = 0.0F;
    previous_novelty_ = 0.0F;
    previous_fast_flux_novelty_ = 0.0F;
    previous_energy_ = 0.0F;
    adaptive_floor_ = 0.0F;
    last_event_ms_ = 0;
    previous_chord_root_ = 0xFFU;
    previous_chord_type_ = 0xFFU;
    axis_ = {};
    warmed_ = false;
}

SaliencyUpdate MusicalSaliency::update(
    const contract::AudioFeaturesV1& audio) noexcept {
    SaliencyUpdate update{};
    update.axis = axis_;
    if (!std::isfinite(audio.novelty) ||
        !std::isfinite(audio.spectral_energy) ||
        !std::isfinite(audio.chroma_strength)) {
        return update;
    }
    update.input_accepted = true;

    const std::uint32_t now_ms = audio.source_frame_ms;
    if (!warmed_) {
        previous_frame_ms_ = now_ms;
        previous_chroma_strength_ = audio.chroma_strength;
        previous_novelty_ = audio.novelty;
        previous_fast_flux_novelty_ = audio.novelty;
        previous_energy_ = audio.spectral_energy;
        adaptive_floor_ = 0.0F;
        last_event_ms_ = 0;
        previous_chord_root_ = 0xFFU;
        previous_chord_type_ = 0xFFU;
        warmed_ = true;
        axis_ = {};
        update.axis = axis_;
        return update;
    }

    if ((audio.event_flags & contract::kEventSilence) != 0U) {
        SaliencyAxisFrame quiet{};
        quiet.dominant_type = SaliencyType::kDynamic;
        previous_chroma_strength_ = audio.chroma_strength;
        previous_novelty_ = audio.novelty;
        previous_fast_flux_novelty_ = audio.novelty;
        previous_energy_ = audio.spectral_energy;
        previous_frame_ms_ = now_ms;
        previous_chord_root_ = 0xFFU;
        previous_chord_type_ = 0xFFU;
        axis_ = quiet;
        update.axis = axis_;
        return update;
    }

    float delta_ms = static_cast<float>(now_ms - previous_frame_ms_);
    if (delta_ms <= 1.0F) {
        delta_ms = 1.0F;
    }
    const float delta_seconds = delta_ms * kMillisecondsToSeconds;

    float harmonic_raw = audio.chord_confidence * 0.3F;
    if (audio.chord_confidence > 0.3F) {
        const std::uint8_t current_type =
            static_cast<std::uint8_t>(audio.chord_type);
        if (audio.chord_root_a_origin != previous_chord_root_) {
            harmonic_raw = 1.0F;
            previous_chord_root_ = audio.chord_root_a_origin;
        } else if (current_type != previous_chord_type_ &&
                   audio.chord_type != contract::ChordTypeV1::kNone) {
            harmonic_raw = std::fmax(harmonic_raw, 0.6F);
        }
        previous_chord_type_ = current_type;
    }
    harmonic_raw = clamp01(harmonic_raw);

    const float flux_delta = std::fabs(audio.novelty - previous_novelty_);
    const float timbral_raw = clamp01(flux_delta / kFluxDerivativeThreshold);
    const float energy_delta =
        std::fabs(audio.spectral_energy - previous_energy_);
    const float dynamic_raw = clamp01(energy_delta / kRmsDerivativeThreshold);

    float rhythmic_raw = 0.0F;
    const bool has_beat = (audio.event_flags & contract::kEventBeat) != 0U;
    if (has_beat && std::isfinite(audio.beat_confidence)) {
        rhythmic_raw = clamp01(0.8F * audio.beat_confidence + 0.5F);
    } else {
        const float fast_flux =
            std::fabs(audio.novelty - previous_fast_flux_novelty_);
        rhythmic_raw = clamp01(0.5F * fast_flux);
    }

    SaliencyAxisFrame frame{};
    frame.harmonic_novelty = harmonic_raw;
    frame.rhythmic_novelty = rhythmic_raw;
    frame.timbral_novelty = timbral_raw;
    frame.dynamic_novelty = dynamic_raw;
    frame.harmonic_novelty_smooth = asymmetricSmooth(
        axis_.harmonic_novelty_smooth,
        harmonic_raw,
        kHarmonicRiseTime,
        kHarmonicFallTime,
        delta_seconds);
    frame.rhythmic_novelty_smooth = asymmetricSmooth(
        axis_.rhythmic_novelty_smooth,
        rhythmic_raw,
        kRhythmicRiseTime,
        kRhythmicFallTime,
        delta_seconds);
    frame.timbral_novelty_smooth = asymmetricSmooth(
        axis_.timbral_novelty_smooth,
        timbral_raw,
        kTimbralRiseTime,
        kTimbralFallTime,
        delta_seconds);
    frame.dynamic_novelty_smooth = asymmetricSmooth(
        axis_.dynamic_novelty_smooth,
        dynamic_raw,
        kDynamicRiseTime,
        kDynamicFallTime,
        delta_seconds);
    frame.overall_saliency = clamp01(
        frame.harmonic_novelty_smooth * kHarmonicWeight +
        frame.rhythmic_novelty_smooth * kRhythmicWeight +
        frame.timbral_novelty_smooth * kTimbralWeight +
        frame.dynamic_novelty_smooth * kDynamicWeight);

    frame.dominant_type = SaliencyType::kDynamic;
    if (frame.harmonic_novelty_smooth >= frame.rhythmic_novelty_smooth &&
        frame.harmonic_novelty_smooth >= frame.timbral_novelty_smooth &&
        frame.harmonic_novelty_smooth >= frame.dynamic_novelty_smooth) {
        frame.dominant_type = SaliencyType::kHarmonic;
    } else if (frame.rhythmic_novelty_smooth >=
                   frame.harmonic_novelty_smooth &&
               frame.rhythmic_novelty_smooth >=
                   frame.timbral_novelty_smooth &&
               frame.rhythmic_novelty_smooth >=
                   frame.dynamic_novelty_smooth) {
        frame.dominant_type = SaliencyType::kRhythmic;
    } else if (frame.timbral_novelty_smooth >=
                   frame.harmonic_novelty_smooth &&
               frame.timbral_novelty_smooth >=
                   frame.rhythmic_novelty_smooth &&
               frame.timbral_novelty_smooth >=
                   frame.dynamic_novelty_smooth) {
        frame.dominant_type = SaliencyType::kTimbral;
    }

    const float adaptive_threshold = clamp01(adaptive_floor_ + 0.15F);
    SaliencyEvent event{};
    event.frame_ms = audio.source_frame_ms;
    event.salient =
        frame.overall_saliency > adaptive_threshold &&
        (now_ms - last_event_ms_) > 120U;
    event.overall_saliency = frame.overall_saliency;
    event.adaptive_threshold = adaptive_threshold;
    const std::uint32_t event_age_ms = now_ms - last_event_ms_;
    event.age_ms = static_cast<std::uint16_t>(
        event_age_ms > 0xFFFFU ? 0xFFFFU : event_age_ms);
    event.flags = 0;
    if (event.salient) {
        last_event_ms_ = now_ms;
    }

    previous_chroma_strength_ = audio.chroma_strength;
    previous_novelty_ = audio.novelty;
    previous_fast_flux_novelty_ = audio.novelty;
    previous_energy_ = audio.spectral_energy;
    previous_frame_ms_ = now_ms;
    const float adaptive_retention = std::pow(
        kAdaptiveFloorRetentionPer8Ms,
        delta_seconds / kAdaptiveFloorReferenceSeconds);
    adaptive_floor_ = adaptive_retention * adaptive_floor_ +
                      (1.0F - adaptive_retention) * frame.overall_saliency;
    axis_ = frame;

    update.axis = frame;
    update.event = event;
    update.event_emitted = event.salient;
    return update;
}

}  // namespace k1::core::audio
