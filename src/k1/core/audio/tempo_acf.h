#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/audio/audio_rate_config.h"

namespace k1::core::audio {

inline constexpr std::size_t kTempoAcfHistoryLength = 512;
inline constexpr std::size_t kTempoAcfBinCount = 96;

using TempoNoveltyRing = std::array<float, kTempoAcfHistoryLength>;

struct TempoAcfOutput final {
    std::array<float, kTempoAcfBinCount> comb_salience{};
    std::array<float, kTempoAcfBinCount> point_salience{};
    bool valid = false;
};

// Portable extraction of the legacy K1 harmonic-comb ACF. The ring index must
// identify the oldest sample, matching k1_tempo.cpp after its write-index
// advance. This function owns no state and performs no allocation.
void computeTempoAcf(const TempoNoveltyRing& novelty_ring,
                     std::uint16_t oldest_index,
                     float novelty_scale,
                     TempoAcfOutput& output) noexcept;
void computeTempoAcfAtRate(const TempoNoveltyRing& novelty_ring,
                           std::uint16_t oldest_index,
                           float novelty_scale,
                           float novelty_rate_hz,
                           TempoAcfOutput& output) noexcept;
void computeTempoAcf(const TempoNoveltyRing& novelty_ring,
                     std::uint16_t oldest_index,
                     float novelty_scale,
                     AudioRateConfiguration rate,
                     TempoAcfOutput& output) noexcept;

}  // namespace k1::core::audio
