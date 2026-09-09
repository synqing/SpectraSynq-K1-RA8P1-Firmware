#include "core/audio/tempo_acf.h"

#include <cmath>

// The frozen production trajectory requires statement-order arithmetic in the
// tempo lane even when the surrounding firmware retains its legacy fast-math
// profile. GCC's translation-unit override keeps this local to tempo; the
// native parity environment supplies the equivalent command-line option.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-unsafe-math-optimizations")
#endif

namespace k1::core::audio {
namespace {

// Source snapshot:
//   SpectraSynq_K1_Firmware@c3e13ffdf1a3f8174b2c1dd3bf0af39dbe0c7b4c
//   SPECTRASYNQ_K1_FIRMWARE/audio/k1_tempo.cpp
//   SHA-256 bc4a68338014d802c8f28b9deb58568a7c76fe1aa97acfce583dd4334fb21898
// Equations, constants, loop order, rectification, and normalisation remain
// identical. Only firmware globals are replaced by explicit arguments/output.

constexpr std::size_t kAcfTableLength = 200;
constexpr float kTempoLowBpm = 60.0F;
constexpr float kNoveltyRateHz =
    audioNoveltyRateHz(kProductionAudioRate);
constexpr int kCombTeeth = 4;
constexpr float kCombWeights[kCombTeeth] = {1.0F, 0.8F, 0.7F, 0.6F};

float acfAt(const std::array<float, kAcfTableLength>& acf,
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

}  // namespace

void computeTempoAcf(const TempoNoveltyRing& novelty_ring,
                     std::uint16_t oldest_index,
                     float novelty_scale,
                     TempoAcfOutput& output) noexcept {
    computeTempoAcfAtRate(novelty_ring, oldest_index, novelty_scale,
                          kNoveltyRateHz, output);
}

void computeTempoAcf(const TempoNoveltyRing& novelty_ring,
                     std::uint16_t oldest_index,
                     float novelty_scale,
                     const AudioRateConfiguration rate,
                     TempoAcfOutput& output) noexcept {
    const float novelty_rate_hz =
        static_cast<float>(rate.sample_rate_hz) /
        static_cast<float>(rate.hop_samples) / 3.0F;
    computeTempoAcfAtRate(novelty_ring, oldest_index, novelty_scale,
                          novelty_rate_hz, output);
}

void computeTempoAcfAtRate(const TempoNoveltyRing& novelty_ring,
                           std::uint16_t oldest_index,
                           float novelty_scale,
                           const float novelty_rate_hz,
                           TempoAcfOutput& output) noexcept {
    const float kNoveltyRateHz = novelty_rate_hz;
    std::array<float, kTempoAcfHistoryLength> work{};
    float mean = 0.0F;
    for (std::uint16_t index = 0; index < kTempoAcfHistoryLength; ++index) {
        const float value =
            novelty_ring[static_cast<std::uint16_t>(oldest_index + index) %
                         kTempoAcfHistoryLength] * novelty_scale;
        work[index] = value;
        mean += value;
    }
    mean /= static_cast<float>(kTempoAcfHistoryLength);
    for (float& value : work) value -= mean;
    const int minimum_lag = static_cast<int>(
        std::floor(novelty_rate_hz * 60.0F / 160.0F)) - 1;
    const int maximum_lag = static_cast<int>(
        std::ceil(kCombTeeth * novelty_rate_hz * 60.0F / 55.0F)) + 1;
    int lag_count = maximum_lag - minimum_lag + 1;
    if (lag_count > static_cast<int>(kAcfTableLength)) lag_count = kAcfTableLength;
    if (lag_count < 1) lag_count = 1;
    std::array<float, kAcfTableLength> acf{};
    for (int row = 0; row < lag_count; ++row) {
        const int lag = minimum_lag + row;
        float sum = 0.0F;
        for (std::uint16_t sample = static_cast<std::uint16_t>(lag);
             sample < kTempoAcfHistoryLength;
             ++sample) {
            sum += work[sample] *
                   work[static_cast<std::uint16_t>(sample - lag)];
        }
        acf[static_cast<std::size_t>(row)] = sum;
    }

    float maximum_comb = 1.0e-12F;
    float maximum_point = 1.0e-12F;
    for (std::uint16_t bin = 0; bin < kTempoAcfBinCount; ++bin) {
        const float bpm = kTempoLowBpm + static_cast<float>(bin);
        const float real_lag = kNoveltyRateHz * 60.0F / bpm;
        float comb = 0.0F;
        float point = 0.0F;
        for (int tooth = 0; tooth < kCombTeeth; ++tooth) {
            float value = acfAt(acf,
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
}

}  // namespace k1::core::audio
