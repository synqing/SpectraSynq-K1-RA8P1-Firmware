#include "core/audio/gdft_postprocess.h"

#include <cmath>

namespace k1::core::audio {
namespace {

constexpr std::int32_t kQ16Scale = 65536;
constexpr double kPi = 3.14159265358979323846;
constexpr float kMagnitudeAttack = 0.3F;
constexpr float kMagnitudeRelease = 0.3F;
constexpr float kStaticNoiseGain = 1.5F;
constexpr float kDtReference = 0.010F;
constexpr float kDtMinimum = 0.004F;
constexpr float kDtMaximum = 0.020F;
constexpr float kAttackReference = 0.28F;
constexpr float kReleaseReference = 0.02F;
constexpr float kNoiseReference = 0.001F;
constexpr float kGainReference = 0.05F;

std::int32_t qFromFloat(float value) noexcept {
    return static_cast<std::int32_t>(value * static_cast<float>(kQ16Scale));
}

std::int32_t qFromInteger(std::int32_t value) noexcept {
    return value * kQ16Scale;
}

float qToFloat(std::int32_t value) noexcept {
    return static_cast<float>(value) / static_cast<float>(kQ16Scale);
}

std::int32_t qMultiply(std::int32_t left, std::int32_t right) noexcept {
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(left) * right) >> 16);
}

std::int32_t qDivide(std::int32_t left, std::int32_t right) noexcept {
    if (right == 0) return 0;
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(left) * kQ16Scale) / right);
}

float clampFloat(float value, float low, float high) noexcept {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float alphaFromReference(float dt, float reference_alpha) noexcept {
    const float tau =
        kDtReference * (1.0F - reference_alpha) / reference_alpha;
    return dt / (tau + dt);
}

std::uint8_t agcBand(float frequency_hz) noexcept {
    if (frequency_hz < 200.0F) return 0;
    if (frequency_hz < 800.0F) return 1;
    if (frequency_hz < 3000.0F) return 2;
    return 3;
}

std::int32_t spectralTilt(float frequency_hz) noexcept {
    if (frequency_hz < 200.0F) return qFromFloat(1.30F);
    if (frequency_hz > 3000.0F) return qFromFloat(0.85F);
    return qFromFloat(1.0F);
}

}  // namespace

void processGdftPostprocess(const GdftRawFrame& raw,
                            const GdftConfiguration& configuration,
                            const GdftPostprocessConfig& config,
                            GdftPostprocessState& state,
                            GdftPostprocessFrame& output) noexcept {
    output = {};

    const std::size_t safe_bins = raw.nyquist_safe_bin_high < kGdftBinCount
                                      ? raw.nyquist_safe_bin_high
                                      : kGdftBinCount;
    for (std::size_t bin = 0; bin < safe_bins; ++bin) {
        const float magnitude = raw.bins[bin].normalised_magnitude;
        const float coefficient =
            magnitude > state.magnitude_average[bin]
                ? kMagnitudeAttack
                : kMagnitudeRelease;
        state.magnitude_average[bin] =
            magnitude * coefficient +
            state.magnitude_average[bin] * (1.0F - coefficient);
    }
    for (std::size_t bin = safe_bins; bin < kGdftBinCount; ++bin) {
        state.magnitude_average[bin] = 0.0F;
    }

    if (config.static_noise_subtraction_enabled &&
        config.noise_calibration_complete) {
        const std::int32_t noise_gain = qFromFloat(kStaticNoiseGain);
        for (std::size_t bin = 0; bin < kGdftBinCount; ++bin) {
            state.magnitude_average[bin] -=
                qToFloat(qMultiply(state.static_noise_q16[bin], noise_gain));
            if (state.magnitude_average[bin] < 0.0F) {
                state.magnitude_average[bin] = 0.0F;
            }
        }
    }

    const float mood = config.bloom_mode ? 1.0F : config.mood;
    const float cutoff_hz = 1.0F + 10.0F * mood;
    const std::uint32_t frame_rate =
        config.frame_rate_hz == 0 ? 1U : config.frame_rate_hz;
    const float exponent = static_cast<float>(
        -2.0 * kPi * cutoff_hz / static_cast<double>(frame_rate));
    const float low_pass_alpha = 1.0F - std::exp(exponent);
    for (std::size_t bin = 0; bin < kGdftBinCount; ++bin) {
        output.magnitude_final[bin] =
            (1.0F - low_pass_alpha) * state.magnitude_last[bin] +
            low_pass_alpha * state.magnitude_average[bin];
        state.magnitude_last[bin] = output.magnitude_final[bin];
    }

    const float dt = clampFloat(config.frame_dt_seconds, kDtMinimum, kDtMaximum);
    const std::int32_t attack_alpha =
        qFromFloat(alphaFromReference(dt, kAttackReference));
    const std::int32_t release_alpha =
        qFromFloat(alphaFromReference(dt, kReleaseReference));
    const std::int32_t noise_alpha =
        qFromFloat(alphaFromReference(dt, kNoiseReference));
    const std::int32_t gain_alpha =
        qFromFloat(alphaFromReference(dt, kGainReference));
    const std::int32_t epsilon = qFromFloat(0.001F);
    const std::int32_t maximum_gain = qFromFloat(10.0F);

    std::int32_t effective_target = qFromFloat(0.25F);
    std::int32_t gain_floor = qFromFloat(0.1F);
    float loud_trim = 1.0F;
    if (config.loud_guard_enabled) {
        loud_trim = clampFloat(config.loud_gdft_trim, 0.32F, 1.0F);
        const float floor_mix = (loud_trim - 0.32F) / (1.0F - 0.32F);
        const float loud_floor = 0.020F + (0.1F - 0.020F) * floor_mix;
        effective_target =
            qMultiply(effective_target, qFromFloat(loud_trim));
        gain_floor = qFromFloat(clampFloat(loud_floor, 0.020F, 0.1F));
    }

    std::array<std::int32_t, kGdftAgcBandCount> band_sum{};
    std::array<std::uint16_t, kGdftAgcBandCount> band_count{};
    for (std::size_t bin = 0; bin < kGdftBinCount; ++bin) {
        const std::uint8_t band =
            agcBand(configuration[bin].target_frequency_hz);
        band_sum[band] += qFromFloat(output.magnitude_final[bin]);
        ++band_count[band];
    }

    for (std::size_t band = 0; band < kGdftAgcBandCount; ++band) {
        auto& band_state = state.agc_bands[band];
        const std::int32_t signal =
            band_count[band] == 0
                ? 0
                : qDivide(band_sum[band], qFromInteger(band_count[band]));
        const std::int32_t envelope_delta = signal - band_state.envelope_q16;
        band_state.envelope_q16 += qMultiply(
            envelope_delta,
            signal > band_state.envelope_q16 ? attack_alpha : release_alpha);

        if (band_state.envelope_q16 <
            qMultiply(band_state.noise_floor_q16, qFromFloat(2.0F))) {
            band_state.noise_floor_q16 += qMultiply(
                band_state.envelope_q16 - band_state.noise_floor_q16,
                noise_alpha);
        }
        if (band_state.noise_floor_q16 < epsilon) {
            band_state.noise_floor_q16 = epsilon;
        }

        const std::int32_t open_threshold =
            qMultiply(band_state.noise_floor_q16, qFromFloat(4.0F));
        const std::int32_t close_threshold =
            qMultiply(band_state.noise_floor_q16, qFromFloat(2.5F));
        if (band_state.gated &&
            band_state.envelope_q16 > open_threshold) {
            band_state.gated = false;
        }
        if (!band_state.gated &&
            band_state.envelope_q16 < close_threshold) {
            band_state.gated = true;
        }

        if (!band_state.gated) {
            std::int32_t target_gain = qDivide(
                effective_target, band_state.envelope_q16 + epsilon);
            if (target_gain > maximum_gain) target_gain = maximum_gain;
            if (target_gain < gain_floor) target_gain = gain_floor;
            band_state.gain_q16 +=
                qMultiply(target_gain - band_state.gain_q16, gain_alpha);
        }
        output.band_gain[band] = qToFloat(band_state.gain_q16);
        output.band_energy[band] = qToFloat(band_state.envelope_q16);
        output.band_noise_floor[band] =
            qToFloat(band_state.noise_floor_q16);
    }

    const std::int32_t half = qFromFloat(0.5F);
    const std::int32_t one = qFromFloat(1.0F);
    const std::int32_t loud_depth = qFromFloat(1.0F - loud_trim);
    for (std::size_t bin = 0; bin < kGdftBinCount; ++bin) {
        const std::uint8_t band =
            agcBand(configuration[bin].target_frequency_hz);
        std::int32_t value = qMultiply(
            qMultiply(qFromFloat(output.magnitude_final[bin]),
                      state.agc_bands[band].gain_q16),
            spectralTilt(configuration[bin].target_frequency_hz));
        if (value > half) {
            const std::int32_t excess = value - half;
            value = half + qDivide(excess, one + excess);
        }

        if (config.loud_guard_enabled && loud_trim < 0.999F) {
            if (config.loud_guard_mode == 0) {
                value -= qMultiply(loud_depth, qFromFloat(0.10F));
            } else {
                value -= qMultiply(
                    loud_depth,
                    qFromFloat(0.03F) +
                        qMultiply(value, qFromFloat(0.12F)));
            }
            if (value < 0) value = 0;

            const std::int32_t knee = qFromFloat(0.45F);
            const std::int32_t ceiling =
                qFromFloat(0.92F) -
                qMultiply(loud_depth, qFromFloat(0.20F));
            if (value > knee && ceiling > knee) {
                const std::int32_t excess = value - knee;
                const std::int32_t span = ceiling - knee;
                value = knee + qDivide(qMultiply(span, excess), span + excess);
            }
        }
        if (value > one) value = one;
        if (value < 0) value = 0;
        output.spectrum_q16[bin] = value;
        output.spectrum[bin] = qToFloat(value);
    }

    std::int32_t novelty_sum = 0;
    const std::uint8_t previous_index = state.spectral_history_index == 0
                                            ? kGdftSpectralHistoryLength - 1U
                                            : state.spectral_history_index - 1U;
    for (std::size_t bin = 0; bin < kGdftBinCount; ++bin) {
        std::int32_t difference =
            output.spectrum_q16[bin] -
            state.spectral_history_q16[previous_index][bin];
        if (difference < 0) difference = 0;
        novelty_sum += difference;
        state.spectral_history_q16[state.spectral_history_index][bin] =
            output.spectrum_q16[bin];
    }
    const std::int32_t novelty_mean =
        qDivide(novelty_sum, qFromInteger(kGdftBinCount));
    output.novelty_q16 = qFromFloat(std::sqrt(qToFloat(novelty_mean)));
    output.novelty = qToFloat(output.novelty_q16);
    state.spectral_history_index = static_cast<std::uint8_t>(
        (state.spectral_history_index + 1U) % kGdftSpectralHistoryLength);
}

}  // namespace k1::core::audio
