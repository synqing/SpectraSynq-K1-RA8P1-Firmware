#include "core/audio/gdft_goertzel.h"

#include <cmath>
#include <limits>

namespace k1::core::audio {
namespace {

// Production source snapshots at legacy commit
// c3e13ffdf1a3f8174b2c1dd3bf0af39dbe0c7b4c:
//   audio/k1_gdft_lane4_exact.h
//     SHA-256 01c7e5519f9338e38bb58e910007d31b0cbd895bb27d72ea7d0c81ff561742fc
//   system/system.h
//     SHA-256 5199b2b4b0e4f85bd75119253e21d982d830b159989f8902a29c86b133c027af
//   system/constants.h
//     SHA-256 3f1510e462ecf8216650fe30ddcd3e5eed8aa2246715fc7c46ce1aa353027891

constexpr std::uint8_t kNoteOffset = 12;
constexpr std::uint8_t kX2CrossoverBin = 40;
constexpr std::uint16_t kMaximumBlockSize = 2000;
constexpr double kPi = 3.14159265358979323846;

constexpr std::array<float, 96> kNotes = {
    55.00000F, 58.27047F, 61.73541F, 65.40639F, 69.29566F, 73.41619F,
    77.78175F, 82.40689F, 87.30706F, 92.49861F, 97.99886F, 103.8262F,
    110.0000F, 116.5409F, 123.4708F, 130.8128F, 138.5913F, 146.8324F,
    155.5635F, 164.8138F, 174.6141F, 184.9972F, 195.9977F, 207.6523F,
    220.0000F, 233.0819F, 246.9417F, 261.6256F, 277.1826F, 293.6648F,
    311.1270F, 329.6276F, 349.2282F, 369.9944F, 391.9954F, 415.3047F,
    440.0000F, 466.1638F, 493.8833F, 523.2511F, 554.3653F, 587.3295F,
    622.2540F, 659.2551F, 698.4565F, 739.9888F, 783.9909F, 830.6094F,
    880.0000F, 932.3275F, 987.7666F, 1046.502F, 1108.731F, 1174.659F,
    1244.508F, 1318.510F, 1396.913F, 1479.978F, 1567.982F, 1661.219F,
    1760.000F, 1864.655F, 1975.533F, 2093.005F, 2217.461F, 2349.318F,
    2489.016F, 2637.020F, 2793.825F, 2959.956F, 3135.964F, 3322.437F,
    3520.000F, 3729.310F, 3951.065F, 4186.009F, 4434.922F, 4698.636F,
    4978.032F, 5274.041F, 5587.652F, 5919.911F, 6271.927F, 6644.875F,
    7040.000F, 7458.620F, 7902.130F, 8372.018F, 8869.844F, 9397.272F,
    9956.064F, 10548.08F, 11175.30F, 11839.82F, 12543.85F, 13289.75F,
};

struct LaneState final {
    std::int32_t coefficient_q14 = 0;
    std::uint16_t block_size = 0;
    std::int32_t q1 = 0;
    std::int32_t q2 = 0;
};

void step(LaneState& state,
          std::int32_t sample,
          std::uint32_t& overflow_count) noexcept {
    const std::int64_t multiplication = static_cast<std::int64_t>(
        wrapToInt32(static_cast<std::int64_t>(state.coefficient_q14) * state.q1));
    const std::int64_t q0_64 =
        (static_cast<std::int64_t>(sample) >> 6) +
        (multiplication >> 14) - static_cast<std::int64_t>(state.q2);
    if (q0_64 > std::numeric_limits<std::int32_t>::max() ||
        q0_64 < std::numeric_limits<std::int32_t>::min()) {
        ++overflow_count;
    }
    // The legacy ARM target narrows modulo 2^32. Spell that policy explicitly
    // so host and target overflow paths cannot diverge by implementation.
    const std::int32_t q0 = wrapToInt32(q0_64);
    state.q2 = state.q1;
    state.q1 = q0;
}

void finishBin(const LaneState& state,
               const GdftBinConfig& configuration,
               GdftBinResult& result) noexcept {
    const std::int32_t q1_square = wrapToInt32(static_cast<std::int64_t>(state.q1) * state.q1);
    const std::int32_t q2_square = wrapToInt32(static_cast<std::int64_t>(state.q2) * state.q2);
    const std::int64_t multiplication = static_cast<std::int64_t>(
        wrapToInt32(static_cast<std::int64_t>(state.coefficient_q14) * state.q1));
    const std::int32_t cross = wrapToInt32((multiplication >> 14) * state.q2);
    const std::int32_t legacy_magnitude_squared = wrapToInt32(
        static_cast<std::int64_t>(q1_square) + q2_square - cross);
    if (legacy_magnitude_squared <= 0) {
        result = {};
        return;
    }
    result.q1 = state.q1;
    result.q2 = state.q2;
    result.magnitude_squared = legacy_magnitude_squared;
    // The production source stores sqrtf() into int32_t magnitudes[] before
    // normalisation. Preserve that truncation at this semantic boundary.
    const double magnitude_root =
        std::sqrt(static_cast<double>(legacy_magnitude_squared));
    const std::int32_t production_magnitude =
        magnitude_root >
                static_cast<double>(
                    std::numeric_limits<std::int32_t>::max())
            ? std::numeric_limits<std::int32_t>::max()
            : static_cast<std::int32_t>(magnitude_root);
    result.magnitude = static_cast<float>(production_magnitude);
    result.normalised_magnitude =
        result.magnitude * configuration.inverse_block_size_half;
}

}  // namespace

GdftConfiguration makeGdftConfiguration(const AudioRateConfiguration rate) noexcept {
    GdftConfiguration configuration{};
    for (std::uint16_t bin = 0; bin < kGdftBinCount; ++bin) {
        const std::uint16_t note_index = bin + kNoteOffset;
        const float target = kNotes[note_index];
        const float left = bin == 0 ? target : kNotes[note_index - 1U];
        const float right = bin == kGdftBinCount - 1U
                                ? kNotes[note_index - 1U]
                                : kNotes[note_index + 1U];
        const float left_distance = std::fabs(target - left);
        const float right_distance = std::fabs(target - right);
        const float maximum_distance =
            left_distance > right_distance ? left_distance : right_distance;
        const float resolution_divisor =
            bin < kX2CrossoverBin ? 2.0F : 1.0F;
        std::uint16_t block_size = static_cast<std::uint16_t>(
            static_cast<float>(rate.sample_rate_hz) /
            (maximum_distance * resolution_divisor));
        if (block_size > kMaximumBlockSize) block_size = kMaximumBlockSize;
        const float rounded_bin = static_cast<float>(static_cast<int>(
            0.5F + block_size * target /
                         static_cast<float>(rate.sample_rate_hz)));
        const float omega = static_cast<float>(
            2.0 * kPi * rounded_bin / static_cast<double>(block_size));
        configuration[bin].target_frequency_hz = target;
        configuration[bin].coefficient_q14 = static_cast<std::int32_t>(
            static_cast<float>(1U << 14U) * (2.0F * std::cos(omega)));
        configuration[bin].block_size = block_size;
        configuration[bin].inverse_block_size_half =
            block_size > 0 ? 2.0F / static_cast<float>(block_size) : 0.0F;
    }
    return configuration;
}

GdftConfiguration makeProductionGdftConfiguration() noexcept {
    return makeGdftConfiguration(kProductionAudioRate);
}

void analyseGdftRaw(const GdftSampleWindow& samples,
                    const GdftConfiguration& configuration,
                    GdftRawFrame& output) noexcept {
    analyseGdftRaw(samples, configuration, kProductionAudioRate, output);
}

void analyseGdftRaw(const GdftSampleWindow& samples,
                    const GdftConfiguration& configuration,
                    const AudioRateConfiguration rate,
                    GdftRawFrame& output) noexcept {
    output = {};
    const float nyquist_hz = static_cast<float>(rate.sample_rate_hz) * 0.5F;
    std::uint8_t safe_count = 0;
    while (safe_count < kGdftBinCount &&
           configuration[safe_count].target_frequency_hz <= nyquist_hz) {
        ++safe_count;
    }
    output.nyquist_safe_bin_high = safe_count;

    for (std::uint16_t lane_base = 0; lane_base < safe_count; lane_base += 4U) {
        const std::uint16_t remaining =
            static_cast<std::uint16_t>(safe_count) - lane_base;
        const std::uint8_t lane_count =
            static_cast<std::uint8_t>(remaining < 4U ? remaining : 4U);
        std::array<LaneState, 4> lanes{};
        std::uint16_t common_prefix =
            std::numeric_limits<std::uint16_t>::max();
        for (std::uint8_t lane = 0; lane < lane_count; ++lane) {
            const auto& bin = configuration[lane_base + lane];
            lanes[lane].coefficient_q14 = bin.coefficient_q14;
            lanes[lane].block_size = bin.block_size;
            if (bin.block_size < common_prefix) common_prefix = bin.block_size;
        }
        for (std::uint16_t sample_age = 0;
             sample_age < common_prefix;
             ++sample_age) {
            const std::int32_t sample =
                samples[kGdftSampleHistoryLength - 1U - sample_age];
            step(lanes[0], sample, output.q0_overflow_count);
            if (lane_count > 1U) step(lanes[1], sample, output.q0_overflow_count);
            if (lane_count > 2U) step(lanes[2], sample, output.q0_overflow_count);
            if (lane_count > 3U) step(lanes[3], sample, output.q0_overflow_count);
        }
        for (std::uint8_t lane = 0; lane < lane_count; ++lane) {
            for (std::uint16_t sample_age = common_prefix;
                 sample_age < lanes[lane].block_size;
                 ++sample_age) {
                step(lanes[lane],
                     samples[kGdftSampleHistoryLength - 1U - sample_age],
                     output.q0_overflow_count);
            }
            finishBin(lanes[lane],
                      configuration[lane_base + lane],
                      output.bins[lane_base + lane]);
        }
    }
}

}  // namespace k1::core::audio
