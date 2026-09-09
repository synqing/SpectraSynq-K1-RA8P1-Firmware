#include "core/audio/audio_pipeline.h"

#include <array>
#include <cmath>

#include "core/audio/chord_detect.h"

namespace k1::core::audio {
namespace {

float nonnegative(float value) noexcept {
    if (!std::isfinite(value) || value < 0.0F) return 0.0F;
    return value;
}

void publishOnset(const OnsetBeatFrame& onset,
                  contract::AudioFeaturesV1& features) noexcept {
    features.onset_event_id = onset.event_id;
    features.onset_event_ms = onset.event_ms;
    features.onset_event_age_ms = onset.event_age_ms;
    features.onset_strength = onset.onset_strength;
    features.bass_onset_strength = onset.bass_onset_strength;
    features.beat_phase = onset.beat_phase;
    features.beat_confidence = onset.beat_confidence;
    features.transient_strength = onset.transient_strength;
    features.kick_strength = onset.kick_strength;
    features.snare_strength = onset.snare_strength;
    features.hihat_strength = onset.hihat_strength;
    features.transient_level = onset.transient_level;
    features.kick_level = onset.kick_level;
    features.snare_level = onset.snare_level;
    features.hihat_level = onset.hihat_level;
    features.transient_event_id = onset.transient_event_id;
    features.kick_event_id = onset.kick_event_id;
    features.snare_event_id = onset.snare_event_id;
    features.hihat_event_id = onset.hihat_event_id;
    if (onset.onset) features.event_flags |= contract::kEventOnset;
    if (onset.bass_onset) features.event_flags |= contract::kEventBassOnset;
    if (onset.beat) features.event_flags |= contract::kEventBeat;
    if (onset.transient) features.event_flags |= contract::kEventTransient;
    if (onset.kick) features.event_flags |= contract::kEventKick;
    if (onset.snare) features.event_flags |= contract::kEventSnare;
    if (onset.hihat) features.event_flags |= contract::kEventHihat;
}

}  // namespace

AudioPipeline::AudioPipeline() noexcept
    : gdft_configuration_(makeProductionGdftConfiguration()) {
    initialiseTempoTracker(tempo_state_);
    onset_.reset();
    saliency_.reset();
    musical_time_ = {};
}

void AudioPipeline::reset() noexcept {
    gdft_state_ = {};
    resetTempoTracker(tempo_state_);
    onset_.reset();
    saliency_.reset();
    musical_time_ = {};
}

AudioPipelineOutput AudioPipeline::process(
    const AudioPipelineInput& input) noexcept {
    AudioPipelineOutput output{};
    if (input.samples == nullptr) return output;

    GdftRawFrame raw{};
    analyseGdftRaw(*input.samples, gdft_configuration_, raw);
    GdftPostprocessFrame gdft{};
    processGdftPostprocess(
        raw, gdft_configuration_, input.gdft, gdft_state_, gdft);

    auto& features = output.features;
    features.sequence = input.sequence;
    features.source_frame_ms = input.frame_ms;
    features.capture_time_us = input.capture_time_us;
    features.publish_time_us = input.publish_time_us;
    features.validity_flags = contract::kValidBaseSnapshot |
                              contract::kValidSpectrum |
                              contract::kValidChroma |
                              contract::kValidChord |
                              contract::kValidOnsetV2;
    features.event_flags = input.silence
                               ? static_cast<std::uint32_t>(
                                     contract::kEventSilence)
                               : 0U;
    features.peak_scaled = nonnegative(input.peak_scaled);
    features.vu_level = nonnegative(input.vu_level);
    features.novelty = nonnegative(gdft.novelty);
    features.nyquist_safe_bin_hi = raw.nyquist_safe_bin_high;

    float low_sum = 0.0F;
    float mid_sum = 0.0F;
    float high_sum = 0.0F;
    for (std::uint8_t bin = 0; bin < contract::kSpectrumBinCount; ++bin) {
        const float value = nonnegative(gdft.spectrum[bin]);
        features.spectrum[bin] = value;
        if (bin < contract::kSpectrumBinCount / 3U) {
            low_sum += value;
        } else if (bin < contract::kSpectrumBinCount * 2U / 3U) {
            mid_sum += value;
        } else {
            high_sum += value;
        }
    }
    constexpr float kLowCount =
        static_cast<float>(contract::kSpectrumBinCount / 3U);
    constexpr float kMidCount = static_cast<float>(
        contract::kSpectrumBinCount * 2U / 3U -
        contract::kSpectrumBinCount / 3U);
    constexpr float kHighCount = static_cast<float>(
        contract::kSpectrumBinCount -
        contract::kSpectrumBinCount * 2U / 3U);
    features.low_energy = low_sum / kLowCount;
    features.mid_energy = mid_sum / kMidCount;
    features.high_energy = high_sum / kHighCount;
    features.spectral_energy =
        (low_sum + mid_sum + high_sum) /
        static_cast<float>(contract::kSpectrumBinCount);

    std::array<float, contract::kChromaBinCount> chroma{};
    for (std::uint8_t bin = 0; bin < raw.nyquist_safe_bin_high; ++bin) {
        chroma[bin % contract::kChromaBinCount] += features.spectrum[bin];
    }
    float chroma_sum = 0.0F;
    float chroma_max = 0.0F;
    for (std::uint8_t index = 0; index < contract::kChromaBinCount; ++index) {
        features.chroma_a_origin[index] = chroma[index];
        chroma_sum += chroma[index];
        if (chroma[index] > chroma_max) chroma_max = chroma[index];
    }
    features.chroma_strength =
        chroma_sum > 0.0001F ? chroma_max / chroma_sum : 0.0F;
    const ChordDetection chord = detectChord(chroma);
    features.chord_type = chord.type;
    features.chord_root_a_origin = chord.root_a_origin;
    features.chord_confidence = chord.confidence;
    features.chord_root_strength = chord.root_strength;
    features.chord_third_strength = chord.third_strength;
    features.chord_fifth_strength = chord.fifth_strength;

    output.onset = onset_.update(features);
    publishOnset(output.onset, features);
    output.saliency = saliency_.update(features);
    updateTempoTracker(tempo_state_,
                       TempoTrackerInput{input.frame_ms,
                                         features.novelty,
                                         input.silence});
    output.tempo = readTempoTracker(tempo_state_);

    if (input.media_time_valid) {
      if (musical_time_.anchor.epoch_id != input.media_time.epoch_id &&
          musical_time_.valid()) {
        invalidateForEpochChange(musical_time_, input.media_time.epoch_id);
      }
      if (output.tempo.updated) {
        if (output.tempo.locked) {
          static_cast<void>(bindTempoPhase(
              musical_time_, input.media_time, output.tempo.bpm,
              output.tempo.phase01, output.tempo.confidence,
              output.tempo.beat_tick));
        } else {
          musical_time_.locked = false;
          musical_time_.confidence = output.tempo.confidence;
        }
      }
      if (musical_time_.valid() && comparable(musical_time_, input.media_time)) {
        output.musical_time = musical_time_;
        output.musical_time_valid = true;
        FramePositionQ32 next{};
        std::uint64_t next_index = 0U;
        if (nextBeatFrame(musical_time_, input.media_time.frame_index, next,
                          &next_index)) {
          output.predicted_next_beat.epoch_id = input.media_time.epoch_id;
          output.predicted_next_beat.beat_event_frame = next;
          output.predicted_next_beat.beat_result_available_frame =
              input.media_time.frame_index;
          output.predicted_next_beat.beat_index = next_index;
          output.predicted_next_beat.confidence = musical_time_.confidence;
          output.predicted_next_beat.predicted = true;
          output.predicted_next_beat_valid = true;
        }
      }
    }

    output.gdft_overflow_count = raw.q0_overflow_count;
    output.valid = true;
    return output;
}

}  // namespace k1::core::audio
