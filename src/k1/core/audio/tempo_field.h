#pragma once

// TempoFieldV1 producer: a read-only sidecar of the incumbent tempo tracker.
//
// PARITY BY CONSTRUCTION. The publisher receives the incumbent state only as
// `const TempoTrackerState&` after the incumbent update has run. It never
// feeds back: winner/flywheel, onset, feature and musical-time trajectories
// are those of the incumbent alone, whether the sidecar is disabled, enabled,
// or absent (proved by test/test_ap_incumbent_parity).
//
// DECLARED SWITCH. TempoFieldConfig::publication_enabled defaults to false.
// Disabled, observe() does O(1) bookkeeping per hop (epoch, media-time
// monotonicity, the incumbent's two recomputed bank bins) so ages stay truthful
// on re-enable; no selection, correlation or publication work runs.
//
// CALL PATTERN (one AP owner thread):
//   pipeline.process(input);                        // incumbent, unchanged
//   publisher.observe(pipeline.tempoTrackerState(), hop);
//   copy publisher.latest() into the platform's committed publication.
// Call noteIncumbentReset() whenever the incumbent tracker is reset.
//
// RATE TUPLES. The analysis rate and hop are configuration: 12.8 kHz / 96 and
// 24 kHz / 180 both map to 360 media frames per hop and 1080 per novelty
// interval, but the tuple is published in every field so the two kinds of
// evidence can never be conflated.

#include <array>
#include <cstdint>

#include "contract/tempo_field_v1.h"
#include "core/audio/audio_rate_config.h"
#include "core/audio/media_time.h"
#include "core/audio/tempo_phase_tracks.h"
#include "core/audio/tempo_tracker.h"

namespace k1::core::audio {

struct TempoFieldConfig final {
  AudioRateConfiguration rate = kProductionAudioRate;
  std::uint32_t configuration_revision = 1U;
  bool publication_enabled = false;  // declared switch; off is the incumbent
  bool phase_enabled = true;         // optional phase computation
  contract::TempoCanonicalTimeBase canonical_time_base =
      contract::TempoCanonicalTimeBase::kTrackerWallMs;
  std::uint16_t activity_window_updates = 64U;  // 1.44 s
  float activity_floor = 0.005F;                // raw novelty units
  std::uint32_t warmup_updates = 128U;          // 2.88 s of in-epoch support
  // Candidate selection over the incumbent comb salience.
  float admission_salience = 0.40F;
  float retention_salience = 0.25F;
  float replacement_margin = 0.10F;
  float separation_cents = 100.0F;
  std::uint8_t max_selected = 4U;  // <= kTempoFieldMaxSelected
  TempoPhaseTrackConfig phase{};
  TempoTrackLimits limits{};
};

// One AP hop, supplied after the incumbent update of that hop.
struct TempoFieldHop final {
  AudioTime media_time{};  // this hop's end in MEDIA_TIME_48K
  bool media_time_valid = false;
  bool input_silence = false;              // AudioPipelineInput::silence
  std::uint64_t result_available_us = 0U;  // platform monotonic time
  // Sliced or delayed completion (for example Titan's ACF slice overlay): the
  // support end captured when the job was created. Unset for the unsliced
  // path, where the completing hop's own end is the support end.
  bool evidence_stamp_valid = false;
  AudioTime evidence_end{};
};

struct TempoFieldWorkCounters final {
  std::uint64_t hops_observed = 0U;
  std::uint64_t updates_processed = 0U;  // completed incumbent updates seen
  std::uint64_t publications = 0U;       // generations committed
  std::uint32_t fresh_evaluations_last = 0U;
  std::uint32_t fresh_evaluations_max = 0U;
  std::uint32_t observer_samples_last = 0U;
  std::uint32_t observer_samples_max = 0U;
  std::uint32_t selection_steps_last = 0U;
  std::uint32_t selection_steps_max = 0U;
  std::uint32_t stage_ticks_last = 0U;
  std::uint32_t stage_ticks_max = 0U;
  std::uint32_t discarded_jobs = 0U;
  std::uint32_t invalid_time_events = 0U;
  std::uint32_t invalid_evidence_events = 0U;
  std::uint32_t overflow_drops = 0U;
  std::uint32_t evictions = 0U;
};

// Optional target instrumentation: a platform tick source (for example the
// Cortex-M7 DWT cycle counter). Unset on host; never required for correctness.
struct TempoFieldTimingHook final {
  std::uint32_t (*now_ticks)(void* context) = nullptr;
  void* context = nullptr;
};

// Source-derived arithmetic for a configuration. Not a cycle measurement.
struct TempoFieldCostModel final {
  std::uint32_t max_fresh_evaluations_per_update = 0U;
  std::uint32_t max_observer_samples_per_update = 0U;
  std::uint32_t max_selection_steps_per_update = 0U;
  std::uint32_t novelty_updates_per_second_x1000 = 0U;
  std::uint32_t max_observer_samples_per_second = 0U;
  std::uint32_t publication_bytes = 0U;
  std::uint32_t diagnostics_bytes = 0U;
  std::uint32_t publisher_state_bytes = 0U;
};

[[nodiscard]] TempoFieldCostModel tempoFieldCostModel(
    const TempoFieldConfig& config) noexcept;

// ---------------------------------------------------------------------------
// Candidate selection (pure). Identity is the bank bin; ties break by lower
// bin; previously selected bins are retained with hysteresis.
// ---------------------------------------------------------------------------

struct TempoSelectionConfig final {
  float admission_salience = 0.40F;
  float retention_salience = 0.25F;
  float replacement_margin = 0.10F;
  std::uint8_t max_selected = 4U;
  // Minimum upward bin distance from bin b that is at least the declared
  // separation in cents (precomputed; never evaluated in the hot path).
  std::array<std::uint8_t, contract::kTempoFieldBankBinCount> separation_bins{};
};

[[nodiscard]] TempoSelectionConfig makeTempoSelectionConfig(
    float admission_salience, float retention_salience,
    float replacement_margin, float separation_cents,
    std::uint8_t max_selected) noexcept;

struct TempoSelectionResult final {
  std::array<std::uint8_t, contract::kTempoFieldMaxSelected> bins{};
  std::uint8_t count = 0U;
  std::uint32_t steps = 0U;  // bounded work counter
  bool invalid_input = false;
};

// `comb` holds kTempoFieldBankBinCount values. Non-finite or negative values
// count as zero evidence and set invalid_input.
[[nodiscard]] TempoSelectionResult selectTempoCandidates(
    const float* comb, const std::uint8_t* previous, std::uint8_t previous_count,
    bool allow_admission, const TempoSelectionConfig& config) noexcept;

class TempoFieldPublisher final {
 public:
  TempoFieldPublisher() noexcept;

  void configure(const TempoFieldConfig& config) noexcept;
  void setEnabled(bool enabled) noexcept;
  void setPhaseEnabled(bool enabled) noexcept;
  void noteIncumbentReset() noexcept;
  void setTimingHook(TempoFieldTimingHook hook) noexcept { hook_ = hook; }

  // Returns true only when a new evidence generation was committed.
  bool observe(const TempoTrackerState& incumbent,
               const TempoFieldHop& hop) noexcept;

  [[nodiscard]] const contract::TempoFieldV1& latest() const noexcept {
    return published_;
  }
  [[nodiscard]] const TempoFieldConfig& config() const noexcept {
    return config_;
  }
  [[nodiscard]] const TempoFieldWorkCounters& counters() const noexcept {
    return counters_;
  }
  [[nodiscard]] const TempoPhaseTracks& tracks() const noexcept {
    return tracks_;
  }
  [[nodiscard]] bool bankObserved(std::uint8_t bin,
                                  std::uint64_t& media_frame) const noexcept;
  // Full 96-bin snapshot for diagnostics, sampled from the same incumbent
  // state as the latest publication. Returns false when no evidence exists.
  bool captureDiagnostics(const TempoTrackerState& incumbent,
                          contract::TempoFieldDiagnosticsV1& out) const noexcept;
  // Test seam for generation-wrap vectors; not used by production code.
  void seedTrackGenerationForTest(std::uint32_t value) noexcept {
    tracks_.seedGenerationForTest(value);
  }

 private:
  void invalidateEvidence(contract::TempoFieldReason reason) noexcept;
  void publishUnavailable(contract::TempoFieldReason reason) noexcept;
  void fillStaticHeader(contract::TempoFieldV1& field) const noexcept;
  bool buildPublication(const TempoTrackerState& incumbent,
                        const TempoFieldHop& hop,
                        const AudioTime& evidence_end) noexcept;

  TempoFieldConfig config_{};
  TempoSelectionConfig selection_{};
  TempoPhaseTracks tracks_{};
  TempoFieldTimingHook hook_{};
  TempoFieldWorkCounters counters_{};
  contract::TempoFieldV1 published_{};
  contract::TempoFieldV1 building_{};
  std::array<std::uint64_t, contract::kTempoFieldBankBinCount> bank_observed_{};
  std::array<bool, contract::kTempoFieldBankBinCount> bank_valid_{};
  std::uint64_t epoch_ = 0U;
  std::uint64_t last_hop_frame_ = 0U;
  std::uint64_t last_update_end_ = 0U;
  std::uint64_t generation_ = 0U;
  std::uint32_t updates_in_epoch_ = 0U;
  std::uint16_t regular_samples_ = 0U;
  std::uint16_t last_calculation_bin_ = 0U;
  bool have_epoch_ = false;
  bool have_last_update_ = false;
  bool have_calculation_bin_ = false;
  bool history_predates_epoch_ = false;
};

}  // namespace k1::core::audio
