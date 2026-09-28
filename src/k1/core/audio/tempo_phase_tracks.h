#pragma once
#include "contract/tempo_field_v1.h"
#include "core/audio/tempo_acf.h"
#include "core/audio/media_time.h"
namespace k1::core::audio {
inline constexpr std::uint64_t kTempoPhaseCoastFrames = 48000;
inline constexpr std::uint64_t kTempoPhaseStaleFrames = 96000;
inline constexpr double kTempoPhaseCorrectionLimit = 0.08;
inline constexpr float kTempoPeakSeparation = 0.06F;
struct TempoExtendedPhase final { std::uint64_t beat_index = 0; double phase01 = 0.0; };
// Chronological stored novelty (including incumbent decay), rectangular extra
// window. Newest sample is support_end, NOT a calibrated attack timestamp.
// C+iS = sum x[i] exp(+i*2*pi*bpm/60*(511-i)*interval/48000).
[[nodiscard]] bool observeTempoPhase(const TempoNoveltyRing& chronological,
    float bpm, std::uint64_t interval_frames, double& phase_out) noexcept;
[[nodiscard]] bool projectTempoTrack(const contract::TempoFieldCandidateV1& track,
    AudioTime now, TempoExtendedPhase& out) noexcept;
[[nodiscard]] bool deriveTempoPhase(const TempoExtendedPhase& root,
    std::uint32_t numerator, std::uint32_t denominator,
    std::uint64_t root_epoch, std::uint64_t requested_epoch,
    TempoExtendedPhase& out) noexcept;
struct TempoPhaseTracks final {
    std::array<contract::TempoFieldCandidateV1, contract::kTempoFieldCapacity> slots{};
    std::uint32_t next_generation = 1;
};
void updateTempoPhaseTracks(TempoPhaseTracks& state,
    contract::TempoFieldV1& field, const TempoNoveltyRing& chronological,
    const TempoAcfOutput& acf, std::uint64_t interval_frames,
    bool phase_enabled) noexcept;
void ageTempoPhaseTracks(contract::TempoFieldV1& field, std::uint64_t now_frame) noexcept;
} // namespace k1::core::audio
