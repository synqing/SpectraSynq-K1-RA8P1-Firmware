#pragma once

#include "core/audio/tempo_acf.h"

#include <array>
#include <cstdint>

namespace k1::core::audio {

inline constexpr std::size_t kTempoAcfLagCap = 200;

// Same MAC order as computeTempoAcfAtRate. Work is split by lag row so a hop
// can publish the last tempo without waiting for the full nested multiply.
struct TempoAcfSliceJob final {
    bool active = false;
    bool lags_done = false;
    std::uint16_t row = 0;
    int lag_count = 0;
    int minimum_lag = 0;
    float novelty_rate_hz = 0.0F;
    float delta_seconds = 0.0F;
    float sample = 0.0F;
    TempoNoveltyRing work{};
    std::array<float, kTempoAcfLagCap> acf{};
};

void tempoAcfSliceBegin(TempoAcfSliceJob& job,
                        const TempoNoveltyRing& novelty_ring,
                        std::uint16_t oldest_index,
                        float novelty_scale,
                        float novelty_rate_hz,
                        float delta_seconds,
                        float sample) noexcept;

// Returns true when every lag row is filled (comb/normalise not yet applied).
bool tempoAcfSlicePump(TempoAcfSliceJob& job, unsigned max_rows) noexcept;

void tempoAcfSliceFinish(TempoAcfSliceJob& job, TempoAcfOutput& output) noexcept;

}  // namespace k1::core::audio
