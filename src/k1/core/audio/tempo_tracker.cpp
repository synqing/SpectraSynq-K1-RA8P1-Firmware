#include "core/audio/tempo_tracker.h"

#include <cmath>
#include <cstdint>

// Keep the byte-exact tempo oracle local instead of weakening fast-math for
// unrelated DSP translation units such as the production GDFT.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-unsafe-math-optimizations")
#endif

namespace k1::core::audio {
namespace {

// Constants and statement order mirror the production V2 confidence and
// flywheel path at the source revision named in tempo_tracker.h.
constexpr float kTempoLowBpm = 60.0F;
constexpr std::uint16_t kNoveltyDecimation = 3;
constexpr float kNoveltyDecay = 0.999F;
constexpr float kBeatShiftPercent = 0.08F;
constexpr float kLockConfidence = 0.60F;
constexpr int kConfidenceLobe = 6;
constexpr float kTactusBpm = 88.0F;
constexpr float kTactusSigma = 0.75F;
constexpr float kConfidenceBpm = 120.0F;
constexpr float kConfidenceSigma = 0.9F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kTwoPi = 2.0F * kPi;

constexpr float kConfidenceShareLow = 0.01F;
constexpr float kConfidenceShareHigh = 0.04F;
constexpr float kConfidenceWeightShare = 0.40F;
constexpr float kConfidenceWeightProminence = 0.35F;
constexpr float kConfidenceWeightPeriodicity = 0.25F;
constexpr float kConfidenceRelease = 0.42F;
constexpr float kConfidenceFloor = 0.20F;
constexpr std::uint16_t kConfidenceWatchdogUpdates = 12;
constexpr float kConfidenceTauSeconds = 0.150F;
constexpr std::uint16_t kConfidenceWarmupUpdates = 512;

constexpr float kFlywheelKp = 0.25F;
constexpr float kFlywheelKi = 0.002F;
constexpr float kFlywheelMaximumCorrection = 0.10F;
constexpr float kFlywheelFrequencyPull = 0.04F;
constexpr float kFlywheelOnsetK = 1.20F;
constexpr float kFlywheelOnsetFloorTauSeconds = 0.30F;
constexpr std::uint16_t kFlywheelCoastBeats = 8;
constexpr float kFlywheelRefractoryBeats = 0.45F;

float clamp(float value, float low, float high) noexcept {
    if (!std::isfinite(value)) return low;
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

std::uint16_t validWinner(const TempoTrackerState& state) noexcept {
    return state.winner_bin < kTempoTrackerBinCount
               ? state.winner_bin
               : static_cast<std::uint16_t>(kTempoTrackerBinCount / 2U);
}

void updateScale(TempoTrackerState& state, float tau) noexcept {
    float maximum = 0.0F;
    for (float value : state.novelty_history) {
        if (value > maximum) maximum = value;
    }
    if (maximum < 1.0e-10F) maximum = 1.0e-10F;
    const float target = 1.0F / (maximum * 0.5F);
    state.novelty_scale =
        state.novelty_scale * (1.0F - tau) + target * tau;
}

void checkSilence(TempoTrackerState& state) noexcept {
    float minimum = 1.0F;
    float maximum = 0.0F;
    for (std::uint16_t index = 0; index < 128U; ++index) {
        const auto ring_index = static_cast<std::uint16_t>(
            (kTempoAcfHistoryLength + state.history_index - 128U + index) %
            kTempoAcfHistoryLength);
        const float scaled = clamp(
            state.novelty_history[ring_index] * state.novelty_scale, 0.0F, 1.0F);
        const float processed = (scaled < 0.5F ? scaled : 0.5F) * 2.0F;
        const float value = sqrtf(processed);
        if (value > maximum) maximum = value;
        if (value < minimum) minimum = value;
    }
    const float silence_raw = 1.0F - fabsf(maximum - minimum);
    constexpr float kSilenceEnter = 0.5F;
    if (silence_raw > kSilenceEnter) {
        state.silence_detected = true;
        state.silence_level = clamp(
            (silence_raw - kSilenceEnter) /
                fmaxf(1.0F - kSilenceEnter, 1.0e-6F),
            0.0F,
            1.0F);
    } else {
        state.silence_detected = false;
        state.silence_level = 0.0F;
    }
}

float computeMagnitude(TempoTrackerState& state, std::uint16_t bin) noexcept {
    auto block_size = state.bins[bin].block_size;
    if (block_size > kTempoAcfHistoryLength) block_size = kTempoAcfHistoryLength;
    float q1 = 0.0F;
    float q2 = 0.0F;
    const float coefficient = state.bins[bin].coefficient;
    for (std::uint32_t index = 0; index < block_size; ++index) {
        const auto ring_index = static_cast<std::uint16_t>(
            (kTempoAcfHistoryLength + state.history_index - block_size + index) %
            kTempoAcfHistoryLength);
        const float sample = clamp(
            state.novelty_history[ring_index] * state.novelty_scale, 0.0F, 4.0F);
        const float q0 = coefficient * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }
    const float real = q1 - q2 * state.bins[bin].cosine;
    const float imaginary = q2 * state.bins[bin].sine;
    float phase = atan2f(imaginary, real) + kPi * kBeatShiftPercent;
    if (phase > kPi) {
        phase -= kTwoPi;
    } else if (phase < -kPi) {
        phase += kTwoPi;
    }
    state.bins[bin].phase = phase;
    float magnitude_squared =
        q1 * q1 + q2 * q2 - q1 * q2 * coefficient;
    if (magnitude_squared < 0.0F) magnitude_squared = 0.0F;
    return sqrtf(magnitude_squared) /
           (static_cast<float>(block_size) * 0.5F);
}

float selectionScore(const TempoTrackerState& state,
                     std::uint16_t bin) noexcept {
    if (state.acf.valid) {
        return state.acf.comb_salience[bin] * state.tempo_prior[bin];
    }
    float magnitude = state.smoothed[bin];
    if (magnitude < 0.0F) magnitude = 0.0F;
    return sqrtf(sqrtf(magnitude)) * state.tempo_prior[bin];
}

float confidenceScore(const TempoTrackerState& state,
                      std::uint16_t bin) noexcept {
    if (state.acf.valid) {
        return state.acf.point_salience[bin] * state.confidence_prior[bin];
    }
    float magnitude = state.smoothed[bin];
    if (magnitude < 0.0F) magnitude = 0.0F;
    return sqrtf(sqrtf(magnitude)) * state.confidence_prior[bin];
}

void updateWinner(TempoTrackerState& state) noexcept {
    if (state.winner_bin >= kTempoTrackerBinCount) {
        state.winner_bin = kTempoTrackerBinCount / 2U;
        state.candidate_bin = state.winner_bin;
        state.candidate_frames = 0;
    }
    std::uint16_t best_bin = 0;
    float best_magnitude = -1.0F;
    for (std::uint16_t bin = 0; bin < kTempoTrackerBinCount; ++bin) {
        const float score = selectionScore(state, bin);
        if (score > best_magnitude) {
            best_magnitude = score;
            best_bin = bin;
        }
    }
    if (best_bin != state.winner_bin) {
        const float current = selectionScore(state, state.winner_bin);
        if (best_magnitude > current * 1.1F) {
            if (best_bin == state.candidate_bin) {
                if (state.candidate_frames < 255U) ++state.candidate_frames;
                if (state.candidate_frames >= 5U) {
                    state.winner_bin = best_bin;
                    state.candidate_frames = 0;
                }
            } else {
                state.candidate_bin = best_bin;
                state.candidate_frames = 1;
            }
        } else {
            state.candidate_frames = 0;
        }
    } else {
        state.candidate_frames = 0;
    }
    if (state.winner_bin >= kTempoTrackerBinCount) {
        state.winner_bin = kTempoTrackerBinCount / 2U;
    }
}

void updateConfidence(TempoTrackerState& state, float delta_seconds) noexcept {
    const std::uint16_t winner = validWinner(state);
    float quality = 0.0F;
    if (state.acf.valid) {
        const float peak = confidenceScore(state, winner);
        float outside_sum = 0.0F;
        float sum = 0.0F;
        int outside_count = 0;
        for (std::uint16_t bin = 0; bin < kTempoTrackerBinCount; ++bin) {
            const float score = confidenceScore(state, bin);
            sum += score;
            int distance = static_cast<int>(bin) - static_cast<int>(winner);
            if (distance < 0) distance = -distance;
            if (distance > kConfidenceLobe) {
                outside_sum += score;
                ++outside_count;
            }
        }
        const float peak_share = sum > 1.0e-9F ? peak / sum : 0.0F;
        const float share = clamp(
            (peak_share - kConfidenceShareLow) /
                (kConfidenceShareHigh - kConfidenceShareLow),
            0.0F,
            1.0F);
        const float background = outside_count > 0
                                     ? outside_sum / static_cast<float>(outside_count)
                                     : 0.0F;
        const float prominence = clamp(
            (peak - background) / (peak + 1.0e-9F), 0.0F, 1.0F);
        const float periodicity =
            clamp(state.acf.comb_salience[winner], 0.0F, 1.0F);
        quality = clamp(kConfidenceWeightShare * share +
                            kConfidenceWeightProminence * prominence +
                            kConfidenceWeightPeriodicity * periodicity,
                        0.0F,
                        1.0F);
    }
    const float nominal_alpha =
        1.0F - expf(-(1.0F / state.novelty_rate_hz) / kConfidenceTauSeconds);
    const float alpha = delta_seconds > 0.0F && delta_seconds < 1.0F
                            ? 1.0F - expf(-delta_seconds /
                                          kConfidenceTauSeconds)
                            : nominal_alpha;
    state.confidence_ema =
        state.confidence_ema * (1.0F - alpha) + quality * alpha;
    state.confidence_ema = clamp(state.confidence_ema, 0.0F, 1.0F);
    state.confidence = state.confidence_ema;
    if (state.confidence_updates < 0xFFFFU) ++state.confidence_updates;
    const bool warm = state.confidence_updates >= kConfidenceWarmupUpdates;
    if (state.locked_v2 && warm &&
        state.confidence_ema < kConfidenceFloor) {
        if (state.subfloor_updates < 0xFFFFU) ++state.subfloor_updates;
    } else {
        state.subfloor_updates = 0;
    }
    if (!state.locked_v2) {
        if (state.confidence_ema >= kLockConfidence && warm &&
            state.beats_seen >= 2U) {
            state.locked_v2 = true;
        }
    } else if (state.confidence_ema < kConfidenceRelease) {
        state.locked_v2 = false;
    }
    if (state.subfloor_updates >= kConfidenceWatchdogUpdates) {
        state.locked_v2 = false;
        state.confidence_ema = 0.0F;
        state.confidence = 0.0F;
        state.subfloor_updates = 0;
    }
}

void updateTempoBank(TempoTrackerState& state,
                     float delta_seconds) noexcept {
    const std::uint16_t bin0 = state.calculation_bin;
    const std::uint16_t bin1 = static_cast<std::uint16_t>(
        (state.calculation_bin + 1U) % kTempoTrackerBinCount);
    state.bins[bin0].magnitude_raw = computeMagnitude(state, bin0);
    state.bins[bin1].magnitude_raw = computeMagnitude(state, bin1);
    state.calculation_bin = static_cast<std::uint16_t>(
        (state.calculation_bin + 2U) % kTempoTrackerBinCount);

    float maximum = 0.01F;
    for (const TempoTrackerBin& bin : state.bins) {
        if (bin.magnitude_raw > maximum) maximum = bin.magnitude_raw;
    }
    const float autoranger = 1.0F / maximum;
    state.power_sum = 1.0e-8F;
    for (std::uint16_t bin = 0; bin < kTempoTrackerBinCount; ++bin) {
        float scaled = state.bins[bin].magnitude_raw * autoranger;
        scaled *= scaled;
        scaled *= scaled;
        state.bins[bin].magnitude = scaled;
        if (scaled > 0.005F) {
            state.smoothed[bin] =
                state.smoothed[bin] * 0.975F + scaled * 0.025F;
            state.power_sum += state.smoothed[bin];
            state.bins[bin].phase +=
                state.bins[bin].phase_radians_per_second * delta_seconds;
            while (state.bins[bin].phase > kPi) state.bins[bin].phase -= kTwoPi;
            while (state.bins[bin].phase < -kPi) state.bins[bin].phase += kTwoPi;
        } else {
            state.smoothed[bin] *= 0.995F;
        }
    }
    updateWinner(state);
    updateConfidence(state, delta_seconds);
}

float computeOnsetPhase(TempoTrackerState& state, float novelty) noexcept {
    const float alpha =
        1.0F - expf(-(1.0F / state.novelty_rate_hz) /
                     kFlywheelOnsetFloorTauSeconds);
    const float difference = novelty - state.onset_floor;
    state.onset_floor += alpha * difference;
    const float absolute = difference < 0.0F ? -difference : difference;
    state.onset_deviation +=
        alpha * (absolute - state.onset_deviation);
    float onset_phase = -1.0F;
    if (state.have_previous_novelty) {
        const bool peak = state.previous_novelty >=
                              state.previous_previous_novelty &&
                          state.previous_novelty > novelty;
        const float threshold =
            state.onset_floor + kFlywheelOnsetK * state.onset_deviation;
        if (peak && state.previous_novelty > threshold &&
            !state.silence_detected) {
            onset_phase = state.flywheel_phase01 - state.last_phase_advance;
            while (onset_phase < 0.0F) onset_phase += 1.0F;
            while (onset_phase >= 1.0F) onset_phase -= 1.0F;
        }
    }
    state.previous_previous_novelty = state.previous_novelty;
    state.previous_novelty = novelty;
    state.have_previous_novelty = true;
    return onset_phase;
}

void advanceFlywheel(TempoTrackerState& state,
                     float delta_seconds,
                     float novelty) noexcept {
    const std::uint16_t winner = validWinner(state);
    const float winner_bpm = state.bins[winner].target_bpm;
    if (!state.flywheel_primed && winner_bpm > 1.0F) {
        state.flywheel_run_bpm = winner_bpm;
        state.flywheel_phase01 = 0.0F;
        state.flywheel_primed = true;
    }
    if (state.flywheel_run_bpm < 1.0F) {
        state.flywheel_run_bpm = winner_bpm > 1.0F ? winner_bpm : 1.0F;
    }
    state.flywheel_run_bpm +=
        (winner_bpm - state.flywheel_run_bpm) * 0.10F;
    const float advance =
        (state.flywheel_run_bpm / 60.0F) * delta_seconds;
    state.last_phase_advance = advance;
    state.flywheel_phase01 += advance;
    state.beats_since_tick += advance;

    const float onset_phase = computeOnsetPhase(state, novelty);
    if (state.locked_v2 && onset_phase >= 0.0F) {
        float error = onset_phase;
        if (error > 0.5F) error -= 1.0F;
        float correction = -kFlywheelKp * error;
        if (correction > kFlywheelMaximumCorrection) {
            correction = kFlywheelMaximumCorrection;
        }
        if (correction < -kFlywheelMaximumCorrection) {
            correction = -kFlywheelMaximumCorrection;
        }
        state.flywheel_phase01 += correction;
        state.flywheel_run_bpm +=
            kFlywheelKi * (-error) * winner_bpm;
    }
    const float minimum_bpm =
        winner_bpm * (1.0F - kFlywheelFrequencyPull);
    const float maximum_bpm =
        winner_bpm * (1.0F + kFlywheelFrequencyPull);
    if (state.flywheel_run_bpm < minimum_bpm) {
        state.flywheel_run_bpm = minimum_bpm;
    } else if (state.flywheel_run_bpm > maximum_bpm) {
        state.flywheel_run_bpm = maximum_bpm;
    }

    bool wrapped = false;
    while (state.flywheel_phase01 >= 1.0F) {
        state.flywheel_phase01 -= 1.0F;
        wrapped = true;
    }
    while (state.flywheel_phase01 < 0.0F) state.flywheel_phase01 += 1.0F;
    if (state.was_locked && !state.locked_v2) {
        const float beat_seconds = 60.0F / state.flywheel_run_bpm;
        float coast = beat_seconds * state.novelty_rate_hz *
                      static_cast<float>(kFlywheelCoastBeats);
        if (coast > 65000.0F) coast = 65000.0F;
        state.coast_updates_remaining = static_cast<std::uint16_t>(coast);
    }
    state.was_locked = state.locked_v2;
    if (!state.locked_v2 && state.coast_updates_remaining > 0U) {
        --state.coast_updates_remaining;
    }
    state.flywheel_beat_tick = false;
    if (wrapped && state.beats_since_tick >= kFlywheelRefractoryBeats) {
        if (!state.silence_detected && state.beats_seen < 0xFFFFU) {
            ++state.beats_seen;
        }
        const bool may_emit =
            (state.locked_v2 || state.coast_updates_remaining > 0U) &&
            !state.silence_detected;
        if (may_emit) {
            state.flywheel_beat_tick = true;
            state.beats_since_tick = 0.0F;
        }
    }
}

TempoTrackerEvent buildOutput(const TempoTrackerState& state) noexcept {
    const std::uint16_t winner = validWinner(state);
    float confidence = state.confidence;
    if (state.silence_detected) {
        confidence *= 1.0F - state.silence_level;
    }
    TempoTrackerEvent event{};
    event.bpm = state.bins[winner].target_bpm;
    event.phase01 = clamp(state.flywheel_phase01, 0.0F, 1.0F);
    event.confidence = clamp(confidence, 0.0F, 1.0F);
    event.beat_tick =
        state.flywheel_beat_tick && !state.silence_detected;
    event.locked = state.locked_v2 && !state.silence_detected;
    event.beat_strength = state.smoothed[winner];
    return event;
}

}  // namespace

void resetTempoTracker(TempoTrackerState& state) noexcept {
    state.smoothed.fill(0.0F);
    state.novelty_history.fill(0.0F);
    state.acf = {};
    for (TempoTrackerBin& bin : state.bins) {
        bin.phase = 0.0F;
        bin.magnitude = 0.0F;
        bin.magnitude_raw = 0.0F;
    }
    state.history_index = 0;
    state.novelty_scale = 1.0F;
    state.scale_count = 0;
    state.calculation_bin = 0;
    state.winner_bin = kTempoTrackerBinCount / 2U;
    state.candidate_bin = kTempoTrackerBinCount / 2U;
    state.candidate_frames = 0;
    state.power_sum = 0.0F;
    state.confidence = 0.0F;
    state.confidence_ema = 0.0F;
    state.locked_v2 = false;
    state.confidence_updates = 0;
    state.subfloor_updates = 0;
    state.beats_seen = 0;
    state.flywheel_phase01 = 0.0F;
    state.flywheel_run_bpm = 0.0F;
    state.flywheel_beat_tick = false;
    state.onset_floor = 0.0F;
    state.onset_deviation = 0.0F;
    state.previous_novelty = 0.0F;
    state.previous_previous_novelty = 0.0F;
    state.have_previous_novelty = false;
    state.beats_since_tick = 1.0F;
    state.coast_updates_remaining = 0;
    state.was_locked = false;
    state.flywheel_primed = false;
    state.last_phase_advance = 0.0F;
    state.silence_detected = false;
    state.silence_level = 0.0F;
    state.input_primed = false;
    state.novelty_accumulator = 0.0F;
    state.last_emit_ms = 0;
    state.frame_counter = 0;
    state.event = {};
}

void initialiseTempoTracker(TempoTrackerState& state) noexcept {
    initialiseTempoTracker(state, kProductionAudioRate);
}

void initialiseTempoTracker(TempoTrackerState& state,
                            const AudioRateConfiguration rate) noexcept {
    state = {};
    state.novelty_rate_hz =
        static_cast<float>(rate.sample_rate_hz) /
        static_cast<float>(rate.hop_samples) /
        static_cast<float>(kNoveltyDecimation);
    for (std::uint16_t index = 0; index < kTempoTrackerBinCount; ++index) {
        const float bpm = kTempoLowBpm + static_cast<float>(index);
        const float hz = bpm / 60.0F;
        TempoTrackerBin& bin = state.bins[index];
        bin.target_bpm = bpm;
        bin.target_hz = hz;
        bin.phase_radians_per_second = kTwoPi * hz;
        const float tactus = log2f(bpm / kTactusBpm) / kTactusSigma;
        state.tempo_prior[index] = expf(-0.5F * tactus * tactus);
        const float confidence =
            log2f(bpm / kConfidenceBpm) / kConfidenceSigma;
        state.confidence_prior[index] =
            expf(-0.5F * confidence * confidence);
        const float left_hz =
            (kTempoLowBpm + static_cast<float>(index == 0U ? 0U : index - 1U)) /
            60.0F;
        const float right_hz =
            (kTempoLowBpm +
             static_cast<float>(index == kTempoTrackerBinCount - 1U
                                    ? kTempoTrackerBinCount - 1U
                                    : index + 1U)) /
            60.0F;
        const float left_distance = fabsf(left_hz - hz);
        const float right_distance = fabsf(right_hz - hz);
        float maximum_distance =
            left_distance > right_distance ? left_distance : right_distance;
        if (maximum_distance < 1.0e-6F) maximum_distance = 1.0e-6F;
        std::uint32_t block = static_cast<std::uint32_t>(
            state.novelty_rate_hz / (maximum_distance * 0.5F));
        if (block > kTempoAcfHistoryLength) block = kTempoAcfHistoryLength;
        if (block < 32U) block = 32U;
        bin.block_size = block;
        const float omega = kTwoPi * hz / state.novelty_rate_hz;
        bin.cosine = cosf(omega);
        bin.sine = sinf(omega);
        bin.coefficient = 2.0F * bin.cosine;
    }
    resetTempoTracker(state);
}

void updateTempoTracker(TempoTrackerState& state,
                        const TempoTrackerInput& input) noexcept {
    state.event.updated = false;
    const std::uint32_t now_ms = input.frame_ms;
    float novelty = clamp(input.novelty, 0.0F, 1.0F);
    if (input.silence) novelty = 0.0F;
    if (!state.input_primed) {
        state.input_primed = true;
        state.last_emit_ms = now_ms;
        state.novelty_accumulator = novelty;
        state.frame_counter = 0;
        return;
    }
    if (novelty > state.novelty_accumulator) {
        state.novelty_accumulator = novelty;
    }
    if (++state.frame_counter < kNoveltyDecimation) {
        if (state.event.beat_tick) state.event.beat_tick = false;
        return;
    }
    state.frame_counter = 0;
    const std::uint32_t elapsed =
        now_ms >= state.last_emit_ms ? now_ms - state.last_emit_ms : 0U;
    float delta_seconds = static_cast<float>(elapsed) / 1000.0F;
    if (delta_seconds <= 0.0F || delta_seconds > 1.0F) {
        delta_seconds = 1.0F / state.novelty_rate_hz;
    }
    state.last_emit_ms = now_ms;
    const float sample = state.novelty_accumulator;
    state.novelty_accumulator = 0.0F;
    for (float& value : state.novelty_history) value *= kNoveltyDecay;
    state.novelty_history[state.history_index] = sample;
    state.history_index = static_cast<std::uint16_t>(
        (state.history_index + 1U) % kTempoAcfHistoryLength);
    if (++state.scale_count >= 3U) {
        updateScale(state, 0.3F);
        state.scale_count = 0;
    }
    checkSilence(state);
    computeTempoAcfAtRate(state.novelty_history,
                          state.history_index,
                          state.novelty_scale,
                          state.novelty_rate_hz,
                          state.acf);
    updateTempoBank(state, delta_seconds);
    advanceFlywheel(
        state, delta_seconds, sample * state.novelty_scale);
    state.event = buildOutput(state);
    state.event.updated = true;
}

TempoTrackerEvent readTempoTracker(const TempoTrackerState& state) noexcept {
    return state.event;
}

TempoTrackerDebug readTempoTrackerDebug(
    const TempoTrackerState& state) noexcept {
    TempoTrackerDebug debug{};
    float maximum = -1.0F;
    for (std::uint16_t bin = 0; bin < kTempoTrackerBinCount; ++bin) {
        if (state.smoothed[bin] > maximum) {
            maximum = state.smoothed[bin];
            debug.bank_peak_bin = bin;
        }
    }
    debug.confidence_internal = state.confidence;
    return debug;
}

}  // namespace k1::core::audio
