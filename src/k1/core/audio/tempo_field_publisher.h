#pragma once
#include "core/audio/tempo_tracker.h"
#include "core/audio/tempo_phase_tracks.h"
namespace k1::core::audio {
struct TempoFieldPublisher final {
    contract::TempoFieldV1 field{};
    TempoPhaseTracks tracks{};
    std::uint64_t seen_generation = 0;
    bool was_enabled = false;
    bool was_phase_enabled = false;
};
// Single AP owner, bounded copy out. No pointer into mutable detector state.
[[nodiscard]] contract::TempoFieldV1 publishTempoField(TempoFieldPublisher& publisher,
    const TempoTrackerState& tracker, AudioTime now, bool media_valid,
    std::uint64_t available_us, bool enabled=true, bool phase_enabled=true) noexcept;
} // namespace k1::core::audio
