#include "core/audio/tempo_acf_slice.h"

#include <cmath>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-unsafe-math-optimizations")
#endif

namespace k1::core::audio {

constexpr float kTempoLowBpm = 60.0F;
constexpr int kCombTeeth = 4;
constexpr float kCombWeights[kCombTeeth] = {1.0F, 0.8F, 0.7F, 0.6F};

static float acfAt(const std::array<float, kTempoAcfLagCap>& acf,
            int lag_count,
            int lag_low,
            float real_lag) noexcept {
    const int lag_floor = static_cast<int>(std::floor(real_lag));
    const float fraction = real_lag - static_cast<float>(lag_floor);
    const int centre = lag_floor - lag_low;
    if (centre >= 1 && centre < lag_count - 1) {
        const float previous = acf[static_cast<std::size_t>(centre - 1)];
        const float current = acf[static_cast<std::size_t>(centre)];
        const float next = acf[static_cast<std::size_t>(centre + 1)];
        return current + 0.5F * fraction * (next - previous) +
               0.5F * fraction * fraction *
                   (next - 2.0F * current + previous);
    }
    if (centre >= 0 && centre < lag_count) {
        return acf[static_cast<std::size_t>(centre)];
    }
    return 0.0F;
}

void tempoAcfSliceBegin(TempoAcfSliceJob& job,
                        const TempoNoveltyRing& novelty_ring,
                        std::uint16_t oldest_index,
                        float novelty_scale,
                        float novelty_rate_hz,
                        float delta_seconds,
                        float sample) noexcept {
    job = TempoAcfSliceJob{};
    job.active = true;
    job.novelty_rate_hz = novelty_rate_hz;
    job.delta_seconds = delta_seconds;
    job.sample = sample;
    float mean = 0.0F;
    for (std::uint16_t index = 0; index < kTempoAcfHistoryLength; ++index) {
        const float value =
            novelty_ring[static_cast<std::uint16_t>(oldest_index + index) %
                         kTempoAcfHistoryLength] * novelty_scale;
        job.work[index] = value;
        mean += value;
    }
    mean /= static_cast<float>(kTempoAcfHistoryLength);
    for (float& value : job.work) value -= mean;
    const int minimum_lag = static_cast<int>(
        std::floor(novelty_rate_hz * 60.0F / 160.0F)) - 1;
    const int maximum_lag = static_cast<int>(
        std::ceil(kCombTeeth * novelty_rate_hz * 60.0F / 55.0F)) + 1;
    int lag_count = maximum_lag - minimum_lag + 1;
    if (lag_count > static_cast<int>(kTempoAcfLagCap)) lag_count = kTempoAcfLagCap;
    if (lag_count < 1) lag_count = 1;
    job.minimum_lag = minimum_lag;
    job.lag_count = lag_count;
    job.row = 0;
    job.lags_done = false;
}

bool tempoAcfSlicePump(TempoAcfSliceJob& job, unsigned max_rows) noexcept {
    if (!job.active || job.lags_done || max_rows == 0U) {
        return job.lags_done;
    }
    unsigned done = 0;
    while (job.row < static_cast<std::uint16_t>(job.lag_count) && done < max_rows) {
        const int lag = job.minimum_lag + static_cast<int>(job.row);
        float sum = 0.0F;
        for (std::uint16_t sample = static_cast<std::uint16_t>(lag);
             sample < kTempoAcfHistoryLength;
             ++sample) {
            sum += job.work[sample] *
                   job.work[static_cast<std::uint16_t>(sample - lag)];
        }
        job.acf[static_cast<std::size_t>(job.row)] = sum;
        job.row = static_cast<std::uint16_t>(job.row + 1U);
        ++done;
    }
    if (job.row >= static_cast<std::uint16_t>(job.lag_count)) {
        job.lags_done = true;
    }
    return job.lags_done;
}

void tempoAcfSliceFinish(TempoAcfSliceJob& job, TempoAcfOutput& output) noexcept {
    const float kNoveltyRateHz = job.novelty_rate_hz;
    const int lag_count = job.lag_count;
    const int minimum_lag = job.minimum_lag;
    float maximum_comb = 1.0e-12F;
    float maximum_point = 1.0e-12F;
    for (std::uint16_t bin = 0; bin < kTempoAcfBinCount; ++bin) {
        const float bpm = kTempoLowBpm + static_cast<float>(bin);
        const float real_lag = kNoveltyRateHz * 60.0F / bpm;
        float comb = 0.0F;
        float point = 0.0F;
        for (int tooth = 0; tooth < kCombTeeth; ++tooth) {
            float value = acfAt(job.acf,
                                lag_count,
                                minimum_lag,
                                real_lag * static_cast<float>(tooth + 1));
            if (value < 0.0F) {
                value = 0.0F;
            }
            if (tooth == 0) {
                point = value;
            }
            comb += kCombWeights[tooth] * value;
        }
        output.comb_salience[bin] = comb;
        output.point_salience[bin] = point;
        if (comb > maximum_comb) {
            maximum_comb = comb;
        }
        if (point > maximum_point) {
            maximum_point = point;
        }
    }
    const float inverse_comb = 1.0F / maximum_comb;
    const float inverse_point = 1.0F / maximum_point;
    for (std::uint16_t bin = 0; bin < kTempoAcfBinCount; ++bin) {
        output.comb_salience[bin] *= inverse_comb;
        output.point_salience[bin] *= inverse_point;
    }
    output.valid = maximum_comb > 1.0e-6F;
    job.active = false;
}

}  // namespace k1::core::audio
