#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "core/audio/audio_rate_config.h"

namespace k1::core::audio {

inline constexpr std::size_t kGdftBinCount = 80;
inline constexpr std::size_t kGdftSampleHistoryLength = 4096;

using GdftSampleWindow =
    std::array<std::int16_t, kGdftSampleHistoryLength>;

struct GdftBinConfig final {
    float target_frequency_hz = 0.0F;
    std::int32_t coefficient_q14 = 0;
    std::uint16_t block_size = 0;
    float inverse_block_size_half = 0.0F;
};

struct GdftBinResult final {
    std::int32_t q1 = 0;
    std::int32_t q2 = 0;
    std::int64_t magnitude_squared = 0;
    float magnitude = 0.0F;
    float normalised_magnitude = 0.0F;
};

struct GdftRawFrame final {
    std::array<GdftBinResult, kGdftBinCount> bins{};
    std::uint8_t nyquist_safe_bin_high = 0;
    std::uint32_t q0_overflow_count = 0;
};

using GdftConfiguration = std::array<GdftBinConfig, kGdftBinCount>;

[[nodiscard]] GdftConfiguration makeProductionGdftConfiguration() noexcept;
[[nodiscard]] GdftConfiguration makeGdftConfiguration(
    AudioRateConfiguration rate) noexcept;

// Two's-complement narrowing used by the donor int32 recurrence and
// magnitude-squared expressions. Host and target must share this helper so a
// later "cleanup" cannot silently widen the product back to int64.
[[nodiscard]] constexpr std::int32_t wrapToInt32(
    const std::int64_t value) noexcept {
    const std::uint32_t low = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(value) & 0xFFFFFFFFULL);
    if (low <= static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())) {
        return static_cast<std::int32_t>(low);
    }
    return std::numeric_limits<std::int32_t>::min() +
           static_cast<std::int32_t>(low - 0x80000000U);
}

// Exact production four-lane direct recurrence. Independent bins consume their
// common newest-sample prefix together, then each lane completes its own tail.
void analyseGdftRaw(const GdftSampleWindow& samples,
                    const GdftConfiguration& configuration,
                    GdftRawFrame& output) noexcept;
void analyseGdftRaw(const GdftSampleWindow& samples,
                    const GdftConfiguration& configuration,
                    AudioRateConfiguration rate,
                    GdftRawFrame& output) noexcept;

}  // namespace k1::core::audio
