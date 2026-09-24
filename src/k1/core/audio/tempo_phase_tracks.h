#pragma once

// Bounded tempo-candidate identity and expressive phase tracks for TempoFieldV1.
//
// This module owns candidate IDENTITY (bank bin + track generation), the
// selected/retiring lifecycle and, when phase computation is enabled, one
// media-anchored phase track per identity. It never reads or writes incumbent
// tracker state: the caller supplies the selected set, the completed update's
// media reference and an observer that reads one stable novelty snapshot.
//
// Work bound per completed update: at most kTempoFieldMaxFreshPhasePerUpdate
// observer calls (one per selected slot), each over at most
// kTempoFieldMaxPhaseWindow samples. Retiring slots coast without evaluation.
// Fixed storage, no allocation, no data-dependent loops beyond these bounds.

#include <array>
#include <cstddef>
#include <cstdint>

#include "contract/tempo_field_v1.h"

namespace k1::core::audio {

// ---------------------------------------------------------------------------
// Phase observer (pure).
//
// PHASE CONVENTION (frozen for TempoFieldV1 schema 1.x):
//   Sample order  x_k is the ring sample k completed updates before the newest
//                 (k = 0 is the newest). The newest sample's support end is the
//                 reference time R: the media end of the completing update hop.
//   Window        Hann over the L newest usable samples,
//                 w_k = 0.5 - 0.5 cos(2 pi (k + 0.5) / L), k = 0 .. L-1.
//   Mean          m = sum(w x) / sum(w) is removed, so a constant input has no
//                 response at any frequency.
//   Correlation   C = sum_k w_k (x_k - m) exp(+i 2 pi k T / P), with T the
//                 media frames per novelty interval and P the beat period in
//                 media frames.
//   Sign, offset  frac(arg(C) / 2 pi) is the phase at R: a pulse train whose
//                 most recent pulse is d frames before R yields frac(d / P);
//                 zero is the beat instant and phase rises with time. No donor
//                 offset (for example the incumbent bank's pi x 0.08 shift) is
//                 applied.
//   Reference     The published observation refers to the window centroid
//                 t_c = R - floor((L - 1) T / 2), phase_c = frac(phase_R -
//                 (R - t_c) / P). The window is symmetric about t_c, so the
//                 phase there is unbiased to first order in the detune between
//                 P and the true tempo, whereas the phase at R carries a bias of
//                 (detune x beats between t_c and R). The observation is always
//                 made at the bank bin's centre period, never at a track's own
//                 running period, so an observation cannot depend on the
//                 estimate it corrects.
//   Support delay R is the newest pooled novelty sample's hop end. Three-hop
//                 peak pooling can place the true attack up to two hops (720
//                 frames) earlier and spectral analysis adds its own delay.
//                 Neither is subtracted: there is no universal latency constant
//                 (REV2-D-021). The phase is continuous expressive evidence, not
//                 a calibrated beat-event timestamp.
//   Coherence     |C| / sum(w x), clamped to [0, 1]; 0 with no energy.
//   Arithmetic    double precision with rotating phasors: error grows as
//                 O(L eps_double), far below the float publication resolution.
// ---------------------------------------------------------------------------

struct NoveltyHistoryView final {
  const float* ring = nullptr;
  std::uint16_t ring_length = 0U;
  std::uint16_t newest_index = 0U;
  // Newest samples that are in this epoch and regularly spaced in media time.
  std::uint16_t usable_samples = 0U;
  std::uint32_t interval_media_frames = 0U;
};

struct TempoPhaseObservation final {
  bool valid = false;
  double phase_cycles = 0.0;  // [0, 1) at the reference t_c (see above)
  float coherence = 0.0F;     // [0, 1]
  std::uint16_t window_samples = 0U;
  // R - t_c in media frames: how far before the newest sample's support end
  // the phase refers.
  std::uint64_t reference_offset_frames = 0U;
};

[[nodiscard]] TempoPhaseObservation observeNoveltyPhase(
    const NoveltyHistoryView& history, double beat_period_frames,
    std::uint16_t window_samples) noexcept;

// Exact bank-centre period: floor(48000 * 60 * 2^32 / (60 + bin)).
[[nodiscard]] std::uint64_t tempoBinPeriodQ32(std::uint8_t bin) noexcept;

// ---------------------------------------------------------------------------
// Track table.
// ---------------------------------------------------------------------------

inline constexpr std::size_t kTempoTrackSlotCount =
    contract::kTempoFieldCandidateCapacity;
inline constexpr std::size_t kTempoTrackInnovationHistory = 8U;
inline constexpr std::uint8_t kTempoTrackUncertaintyMinimum = 4U;

struct TempoPhaseTrackConfig final {
  std::uint16_t window_samples = 256U;          // 5.76 s at 44.444 Hz
  std::uint16_t minimum_window_samples = 128U;  // 2.88 s before any phase
  float minimum_coherence = 0.20F;
  float correction_gain = 0.5F;           // applied fraction of innovation
  float max_correction_cycles = 0.0625F;  // clamp, cycles per update
  float demote_threshold_cycles = 0.25F;  // larger innovations are rejected
  std::uint8_t reinitialise_after = 3U;   // consecutive rejections
  // Fraction of the phase-rate error applied per update. Clicks quantised to
  // the 22.5 ms novelty grid give the bin-centre observation a slow periodic
  // phase jitter (about 6.6 s period at 127.43 BPM); 0.0025 keeps the running
  // frequency within 0.13 BPM of the true tempo through it (0.02 followed the
  // jitter to +/-0.48 BPM) while converging in about 200 updates (4.5 s).
  float frequency_gain = 0.0025F;
  float max_frequency_deviation = 0.03F;  // |f / f_bin - 1| bound
  std::uint8_t derived_ratio_count = 2U;  // <= kTempoFieldMaxDerived
  std::uint8_t derived_numerator[contract::kTempoFieldMaxDerived] = {2U, 1U};
  std::uint8_t derived_denominator[contract::kTempoFieldMaxDerived] = {1U, 2U};
};

// Lifecycle limits in MEDIA_TIME_48K frames, measured from the last accepted
// observation (or, without one, from the last selection).
struct TempoTrackLimits final {
  std::uint32_t fresh_limit_media_frames = 3240U;    // 3 novelty intervals
  std::uint32_t coast_limit_media_frames = 192000U;  // 4.0 s
  std::uint32_t stale_limit_media_frames = 384000U;  // 8.0 s
};

struct TempoTrackSlot final {
  contract::TempoPhaseAnchorV1 anchor{};
  std::uint64_t admitted_media_frame = 0U;
  std::uint64_t last_selected_media_frame = 0U;
  std::uint64_t phase_observed_media_frame = 0U;
  std::uint64_t bin_period_q32 = 0U;
  std::array<float, kTempoTrackInnovationHistory> innovations{};
  std::uint32_t generation = 0U;
  float coherence = 0.0F;
  float innovation = 0.0F;
  float applied_correction = 0.0F;
  std::uint8_t bin = contract::kTempoFieldNoBin;
  contract::TempoCandidateAdmission admission =
      contract::TempoCandidateAdmission::kEmpty;
  contract::TempoTrackState state = contract::TempoTrackState::kInvalid;
  std::uint8_t rejections = 0U;
  std::uint8_t innovation_count = 0U;
  std::uint8_t innovation_write = 0U;
  bool occupied = false;
  bool phase_valid = false;
  bool fresh_this_update = false;
  bool correction_clamped = false;
  bool frequency_clamped = false;
  bool reinitialised = false;
};

using TempoPhaseObserverFn = TempoPhaseObservation (*)(
    void* context, std::uint8_t bin, double beat_period_frames);

struct TempoTrackUpdate final {
  std::uint64_t epoch_id = 0U;
  std::uint64_t reference_media_frame = 0U;
  const std::uint8_t* selected_bins = nullptr;
  std::uint8_t selected_count = 0U;  // <= kTempoFieldMaxSelected
  bool observe_phase = false;
  TempoPhaseObserverFn observer = nullptr;
  void* observer_context = nullptr;
};

struct TempoTrackUpdateResult final {
  std::uint8_t fresh_evaluations = 0U;
  std::uint8_t accepted = 0U;
  std::uint8_t admissions = 0U;
  std::uint8_t retirements = 0U;
  std::uint8_t removals = 0U;
  std::uint8_t evictions = 0U;
  std::uint8_t reinitialisations = 0U;
  std::uint8_t rejected = 0U;
  std::uint8_t low_coherence = 0U;
  bool input_rejected = false;  // malformed update (count/bin) refused
};

class TempoPhaseTracks final {
 public:
  void configure(const TempoPhaseTrackConfig& config,
                 const TempoTrackLimits& limits) noexcept;
  // Clears every slot: a new identity namespace. Generations never repeat
  // within this object's lifetime.
  void reset() noexcept;
  [[nodiscard]] TempoTrackUpdateResult update(
      const TempoTrackUpdate& update) noexcept;

  [[nodiscard]] const std::array<TempoTrackSlot, kTempoTrackSlotCount>& slots()
      const noexcept {
    return slots_;
  }
  [[nodiscard]] std::uint8_t selectedBins(
      std::array<std::uint8_t, contract::kTempoFieldMaxSelected>& bins)
      const noexcept;
  [[nodiscard]] const TempoPhaseTrackConfig& config() const noexcept {
    return config_;
  }
  [[nodiscard]] const TempoTrackLimits& limits() const noexcept {
    return limits_;
  }
  [[nodiscard]] std::uint32_t lastGeneration() const noexcept {
    return last_generation_;
  }
  // Test seam for generation-wrap vectors: the next admission uses value + 1,
  // skipping zero. Not called by production code.
  void seedGenerationForTest(std::uint32_t value) noexcept {
    last_generation_ = value;
  }

 private:
  std::uint32_t nextGeneration() noexcept;
  void initialiseAnchor(TempoTrackSlot& slot, std::uint64_t epoch_id,
                        std::uint64_t reference, std::uint64_t now,
                        double phase_cycles,
                        float coherence) noexcept;
  void acceptObservation(TempoTrackSlot& slot, std::uint64_t epoch_id,
                         std::uint64_t now,
                         const TempoPhaseObservation& observation,
                         TempoTrackUpdateResult& result) noexcept;

  std::array<TempoTrackSlot, kTempoTrackSlotCount> slots_{};
  TempoPhaseTrackConfig config_{};
  TempoTrackLimits limits_{};
  std::uint32_t last_generation_ = 0U;
};

}  // namespace k1::core::audio
