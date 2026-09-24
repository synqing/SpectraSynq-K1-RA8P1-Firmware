#pragma once

// TempoFieldV1 — bounded, engine-local rhythmic evidence published BESIDE the
// incumbent TempoTrackerEvent, never instead of it.
//
// SCOPE. A local portable seam between an engine's own audio pipeline and its
// own visual pipeline (RT1062 on DualMCU, M85 on Titan), exactly like
// AudioFeaturesV1. It is NOT an inter-MCU protocol: it never crosses the S3
// bridge, BLE, SPI or UART, and the native struct layout is not a wire format.
// A diagnostic exporter that needs bytes defines its own field-by-field
// serialisation; it never copies this struct.
//
// WHAT IT DESCRIBES. An evidence field, not a list of musicians. Several strong
// tempo bins can come from one rhythm (broad peaks, metrical levels,
// harmonics); nothing here claims verified polytempo. Saliences are relative
// measurements, not probabilities. `activity` is the only absolute evidence
// level: consumers scale any contribution by it, so zero evidence is zero.
//
// TIME. Three coordinates are kept apart, as in core/audio/media_time.h:
//   * MEDIA_TIME_48K frames (uint64, per stream epoch) for every analysed
//     support boundary, observation time and phase reference;
//   * result_available_us, a local monotonic availability time only;
//   * the incumbent tracker's own millisecond clock, reported verbatim in the
//     canonical descriptor together with the time base it was advanced in.
// A phase advanced in wall time is never relabelled as media time: the
// canonical media projection is published only when the platform declares a
// media-derived tracker clock, and then with its exact truncation interval.
//
// IDENTITY. `bin_id` is the incumbent bank bin (BPM = 60 + bin). Array order is
// presentation order only. `track_generation` distinguishes a genuinely new
// track from state that previously held the same bin: a retained or coasting
// track that is re-selected keeps its generation; a stale, removed, epoch-reset
// or re-initialised track returns with a new one.
//
// DERIVED FAMILY. A derived entry (admission kDerived) is arithmetic on an
// observed root; it is never reported as observed. Its anchor is the root's
// anchor and its projection ratio is applied by projectTempoCandidate(), so a
// half-time relative keeps the root's beat parity. `family_*` fields on an
// observed entry only annotate an interpretation (e.g. 2:1 of the root); they
// never alter its own measured anchor.
//
// CAPACITY (fixed; justified from the packet's source-derived AP costs):
//   * kTempoFieldMaxFreshPhasePerUpdate = 4 fresh phase observations per
//     completed field update. Each is one windowed correlation over at most
//     kTempoFieldMaxPhaseWindow novelty samples, so the fresh work is bounded
//     at 4 x 512 = 2,048 sample terms per update (about 91,000 per second at
//     44.444 Hz), under 3% of the incumbent ACF's 73,667 inner products per
//     update. Capacity eight never authorises eight analyses.
//   * kTempoFieldCandidateCapacity = 8 publication slots: at most four
//     selected (fresh) entries, at most two derived entries and the remainder
//     for retiring entries that coast while a consumer crossfades them out.
//   * sizeof(TempoFieldV1) == 1112 bytes; a double-buffered publication is
//     2224 bytes. The optional 96-bin diagnostic snapshot is a separate
//     2048-byte record sampled on field updates, never per render.
//
// FAILURE SEMANTICS. An unavailable or invalid measurement is signalled by a
// flag or reason, never by a convincing numeric zero on its own. Every float in
// a published field is finite; validateTempoFieldV1() rejects anything else.

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace k1::contract {

inline constexpr std::uint16_t kTempoFieldSchemaMajor = 1U;
inline constexpr std::uint16_t kTempoFieldSchemaMinor = 0U;

inline constexpr std::size_t kTempoFieldCandidateCapacity = 8U;
inline constexpr std::size_t kTempoFieldMaxSelected = 4U;
inline constexpr std::size_t kTempoFieldMaxFreshPhasePerUpdate = 4U;
inline constexpr std::size_t kTempoFieldMaxDerived = 2U;
inline constexpr std::size_t kTempoFieldMaxPhaseWindow = 512U;

// Present incumbent bank: 96 bins, one BPM apart, from 60 BPM.
inline constexpr std::uint8_t kTempoFieldBankBinCount = 96U;
inline constexpr std::uint16_t kTempoFieldBankLowBpm = 60U;
inline constexpr std::uint8_t kTempoFieldNoBin = 0xFFU;
inline constexpr std::uint8_t kTempoFieldNoIndex = 0xFFU;

inline constexpr std::uint32_t kTempoFieldMediaRateHz = 48000U;
inline constexpr std::uint32_t kTempoFieldUnknownFrames = 0xFFFFFFFFU;
inline constexpr std::uint64_t kTempoQ32One = 0x1'0000'0000ULL;

// Extended-beat origin of every new track generation. Beat indices are counted
// from this even origin so that small negative corrections and projections a
// little before the reference stay representable; parity is preserved.
inline constexpr std::uint64_t kTempoTrackBeatOrigin = 64U;

enum TempoFieldFlag : std::uint32_t {
  kTempoFieldEnabled = 1U << 0U,            // publication switch is on
  kTempoFieldValid = 1U << 1U,              // header describes usable evidence
  kTempoFieldWarmup = 1U << 2U,             // fewer than warmup_updates in epoch
  kTempoFieldSilence = 1U << 3U,            // incumbent tempo silence detector
  kTempoFieldInputSilence = 1U << 4U,       // capture silence gate on the hop
  kTempoFieldActivityValid = 1U << 5U,      // activity finite and >= floor
  kTempoFieldAcfValid = 1U << 6U,           // incumbent acf.valid for snapshot
  kTempoFieldPhaseEnabled = 1U << 7U,       // optional phase computation on
  kTempoFieldPhaseAuthority = 1U << 8U,     // >= one entry directly observed
  kTempoFieldCoasting = 1U << 9U,           // >= one selected entry coasting
  kTempoFieldSupportPreEpoch = 1U << 10U,   // incumbent history predates epoch
  kTempoFieldIrregularSupport = 1U << 11U,  // media spacing broke in window
  kTempoFieldInvalidEvidence = 1U << 12U,   // non-finite/negative input seen
  kTempoFieldCanonicalLocked = 1U << 13U,
  kTempoFieldCanonicalCoasting = 1U << 14U,
  kTempoFieldCanonicalBeatTick = 1U << 15U,
  kTempoFieldCanonicalMediaProjection = 1U << 16U,
  kTempoFieldSlicedCompletion = 1U << 17U,  // completed after its support hop
};

inline constexpr std::uint32_t kTempoFieldKnownFlags = (1U << 18U) - 1U;

enum class TempoFieldReason : std::uint8_t {
  kAvailable = 0U,
  kDisabled = 1U,
  kNoEvidence = 2U,            // enabled, no completed update yet
  kWarmup = 3U,
  kEpochChanged = 4U,          // old evidence invalidated by a new epoch
  kInvalidTime = 5U,           // missing or non-monotonic media time
  kInvalidEvidence = 6U,       // incumbent snapshot failed validation
  kReset = 7U,                 // incumbent reset observed
  kConfigurationChanged = 8U,  // detector/sidecar revision transition
  kDiscardedJob = 9U,          // completed job crossed an epoch boundary
};

enum class TempoCoastReason : std::uint8_t {
  kNone = 0U,
  kSilence = 1U,
  kLowActivity = 2U,
  kPhaseSuspended = 3U,
  kLowCoherence = 4U,
  kCorrectionExceeded = 5U,
  kWarmup = 6U,
};

enum class TempoCandidateAdmission : std::uint8_t {
  kEmpty = 0U,
  kSelected = 1U,  // selected this update; at most four; fresh evaluation
  kRetiring = 2U,  // deselected; coasts out with explicit ages
  kDerived = 3U,   // arithmetic relative of the primary observed entry
};

// Authority of an entry's phase. kObserved is reserved for a direct
// observation; a derived entry reports kDerived while its root is fresh and is
// never promoted to kObserved.
enum class TempoTrackState : std::uint8_t {
  kInvalid = 0U,
  kObserved = 1U,
  kDerived = 2U,
  kCoasting = 3U,
  kStale = 4U,
};

// Clock in which the incumbent flywheel phase was actually advanced.
enum class TempoCanonicalTimeBase : std::uint8_t {
  kTrackerWallMs = 0U,   // hop-ready monotonic ms (DualMCU production today)
  kTrackerMediaMs = 1U,  // floor(MEDIA_TIME_48K / 48) (Titan live runtime)
};

enum TempoCandidateFlag : std::uint16_t {
  kCandidatePhaseValid = 1U << 0U,         // anchor may be projected
  kCandidateBankObserved = 1U << 1U,       // bank_observed_media_frame valid
  kCandidateCanonicalWinner = 1U << 2U,    // bin is the incumbent winner
  kCandidateCorrectionClamped = 1U << 3U,  // innovation exceeded the clamp
  kCandidateReinitialised = 1U << 4U,      // new generation this update
  kCandidateUncertaintyKnown = 1U << 5U,   // phase_uncertainty_cycles valid
  kCandidateFamilyRelated = 1U << 6U,      // family_* fields annotate a ratio
  kCandidateFrequencyClamped = 1U << 7U,   // running frequency at its bound
  kCandidateFreshThisUpdate = 1U << 8U,    // observation accepted this update
};

inline constexpr std::uint16_t kTempoCandidateKnownFlags = (1U << 9U) - 1U;

// A time-referenced oscillator. At media frame t of `epoch_id` the extended
// beat coordinate is beat_position_q32 + (t - reference) / period, evaluated
// exactly by projectTempoAnchor(). Integer part = beat index (parity), fraction
// = phase (0 = beat instant, rising with time).
struct TempoPhaseAnchorV1 final {
  std::uint64_t epoch_id = 0U;
  std::uint64_t reference_media_frame = 0U;
  std::uint64_t beat_position_q32 = 0U;  // unsigned Q32.32 beats
  std::uint64_t beat_period_q32 = 0U;    // unsigned Q32.32 media frames/beat
};

struct TempoCandidateV1 final {
  TempoPhaseAnchorV1 anchor{};
  // Support end of the incumbent ACF snapshot these saliences came from.
  std::uint64_t salience_media_frame = 0U;
  // Last time the incumbent interlaced bank recomputed this bin (valid only
  // with kCandidateBankObserved; cached bins keep their old time).
  std::uint64_t bank_observed_media_frame = 0U;
  // Support end of the last ACCEPTED direct phase observation. Projection and
  // coasting never refresh it.
  std::uint64_t phase_observed_media_frame = 0U;
  std::uint64_t admitted_media_frame = 0U;
  std::uint32_t track_generation = 0U;  // 0 = none
  std::uint32_t family_root_generation = 0U;
  float comb_salience = 0.0F;   // incumbent comb salience, peak-normalised
  float point_salience = 0.0F;  // incumbent first-tooth salience, normalised
  float prominence = 0.0F;      // comb minus mean comb outside +/-6 bins, >= 0
  float bank_smoothed = 0.0F;   // incumbent smoothed bank value (legacy scale)
  float phase_coherence = 0.0F;          // |C| / sum(w x) of observation, 0..1
  float phase_innovation_cycles = 0.0F;  // observed - predicted, (-0.5, 0.5]
  float applied_correction_cycles = 0.0F;  // |value| <= configured clamp
  float phase_uncertainty_cycles = 0.0F;   // RMS innovation (see flag)
  float running_bpm = 0.0F;  // informational: from beat_period_q32
  std::uint16_t flags = 0U;  // TempoCandidateFlag
  std::uint8_t bin_id = kTempoFieldNoBin;
  std::uint8_t admission = 0U;  // TempoCandidateAdmission
  std::uint8_t state = 0U;      // TempoTrackState at publication
  std::uint8_t family_root_bin = kTempoFieldNoBin;
  std::uint8_t family_ratio_numerator = 1U;
  std::uint8_t family_ratio_denominator = 1U;
  std::uint8_t projection_ratio_numerator = 1U;
  std::uint8_t projection_ratio_denominator = 1U;
  std::uint8_t reserved[2] = {0U, 0U};
};

// The incumbent clock, described without changing it. winner_target_bpm is
// exactly TempoTrackerEvent::bpm (the winner bin's target); running_bpm is the
// flywheel frequency that actually advanced phase01. They differ during pulls.
struct TempoCanonicalClockV1 final {
  // Lower bound of the media interval holding the tracker reference; valid
  // only with kTempoFieldCanonicalMediaProjection.
  std::uint64_t media_reference_frame = 0U;
  std::uint32_t tracker_reference_ms = 0U;  // tracker clock at its update
  // Width of [media_reference_frame, +width]; kTempoFieldUnknownFrames when
  // no qualified mapping exists.
  std::uint32_t media_uncertainty_frames = kTempoFieldUnknownFrames;
  float winner_target_bpm = 0.0F;
  float running_bpm = 0.0F;
  float phase01 = 0.0F;
  float confidence = 0.0F;  // incumbent heuristic, not a probability
  float beat_strength = 0.0F;
  std::uint16_t coast_updates_remaining = 0U;
  std::uint8_t winner_bin = kTempoFieldNoBin;
  std::uint8_t time_base = 0U;  // TempoCanonicalTimeBase
};

struct TempoFieldV1 final {
  std::uint64_t stream_epoch = 0U;
  // Completed-evidence generation. Advances once per completed incumbent
  // update processed by the sidecar; cached copies never advance it.
  std::uint64_t generation = 0U;
  // Analysed in-epoch support, (start, end] in MEDIA_TIME_48K frames.
  std::uint64_t support_start_media_frame = 0U;
  std::uint64_t support_end_media_frame = 0U;
  std::uint64_t result_available_us = 0U;  // availability only
  std::uint32_t configuration_revision = 0U;
  std::uint32_t flags = 0U;  // TempoFieldFlag
  std::uint32_t analysis_rate_hz = 0U;
  std::uint32_t novelty_interval_media_frames = 0U;
  std::uint32_t fresh_limit_media_frames = 0U;
  std::uint32_t coast_limit_media_frames = 0U;
  std::uint32_t stale_limit_media_frames = 0U;
  std::uint32_t updates_in_epoch = 0U;  // saturating
  std::uint32_t warmup_updates = 0U;
  std::uint32_t overflow_drops = 0U;    // saturating
  float activity = 0.0F;        // mean raw novelty over the activity window
  float activity_floor = 0.0F;
  float latest_novelty = 0.0F;  // newest raw pooled novelty sample
  float concentration = 0.0F;   // max(comb) / sum(comb); 0 when sum is 0
  std::uint16_t schema_major = kTempoFieldSchemaMajor;
  std::uint16_t schema_minor = kTempoFieldSchemaMinor;
  std::uint16_t hop_samples = 0U;
  std::uint16_t hop_media_frames = 0U;
  std::uint8_t reason = 0U;        // TempoFieldReason
  std::uint8_t coast_reason = 0U;  // TempoCoastReason
  std::uint8_t candidate_count = 0U;
  std::uint8_t selected_count = 0U;
  std::uint8_t primary_index = kTempoFieldNoIndex;  // family root, if any
  std::uint8_t reserved[3] = {0U, 0U, 0U};
  TempoCanonicalClockV1 canonical{};
  TempoCandidateV1 candidates[kTempoFieldCandidateCapacity]{};
};

// Optional full-bank diagnostic snapshot. Sampled on field updates by the
// producer on request; never copied per render and never streamed over radio.
struct TempoFieldDiagnosticsV1 final {
  std::uint64_t stream_epoch = 0U;
  std::uint64_t generation = 0U;
  std::uint64_t support_end_media_frame = 0U;
  std::uint64_t bank_observed_media_frame[kTempoFieldBankBinCount]{};
  float comb_salience[kTempoFieldBankBinCount]{};
  float point_salience[kTempoFieldBankBinCount]{};
  float bank_smoothed[kTempoFieldBankBinCount]{};
  std::uint8_t bank_observed_valid[kTempoFieldBankBinCount]{};
  std::uint16_t schema_major = kTempoFieldSchemaMajor;
  std::uint16_t schema_minor = kTempoFieldSchemaMinor;
  std::uint32_t flags = 0U;
};

static_assert(std::is_trivially_copyable_v<TempoPhaseAnchorV1>);
static_assert(std::is_trivially_copyable_v<TempoCandidateV1>);
static_assert(std::is_trivially_copyable_v<TempoCanonicalClockV1>);
static_assert(std::is_trivially_copyable_v<TempoFieldV1>);
static_assert(std::is_trivially_copyable_v<TempoFieldDiagnosticsV1>);
static_assert(std::is_standard_layout_v<TempoFieldV1>);
static_assert(std::is_standard_layout_v<TempoFieldDiagnosticsV1>);
static_assert(sizeof(TempoPhaseAnchorV1) == 32U);
static_assert(sizeof(TempoCandidateV1) == 120U);
static_assert(sizeof(TempoCanonicalClockV1) == 40U);
static_assert(sizeof(TempoFieldV1) == 1112U);
static_assert(sizeof(TempoFieldDiagnosticsV1) == 2048U);
static_assert(alignof(TempoFieldV1) == 8U);
static_assert(offsetof(TempoFieldV1, canonical) == 112U);
static_assert(offsetof(TempoFieldV1, candidates) == 152U);
static_assert(offsetof(TempoCandidateV1, track_generation) == 64U);
static_assert(offsetof(TempoCandidateV1, flags) == 108U);

// ---------------------------------------------------------------------------
// Consumer arithmetic. Pure, allocation-free, integer-exact; safe in render.
// ---------------------------------------------------------------------------

enum class TempoProjectionStatus : std::uint8_t {
  kOk = 0U,
  kInvalidAnchor = 1U,      // zero or out-of-range period
  kEpochMismatch = 2U,      // never project across epochs
  kZeroRatio = 3U,          // numerator or denominator zero
  kOutOfHorizon = 4U,       // |t - reference| >= 2^32 frames
  kOverflow = 5U,           // beat coordinate not representable
  kBeforeOrigin = 6U,       // projection precedes beat coordinate zero
};

struct TempoBeatPositionV1 final {
  std::uint64_t beat_index = 0U;  // floor of the (ratio-applied) coordinate
  std::uint32_t phase_q32 = 0U;   // fractional phase, unsigned Q0.32
  float phase01 = 0.0F;           // phase_q32 / 2^32, strictly below 1.0F
};

// Integer-exact projection in two floors, both frozen: first the anchor's own
// coordinate B(t) = beat_position + floor/ceil((t - reference) * 2^64 / period)
// (floor forwards, ceiling backwards, i.e. floor of the real value), then
// floor((p/q) * B(t)) exactly in Q32.32. Deterministic on every IEEE or
// non-IEEE target: no floating point participates except phase01.
[[nodiscard]] TempoProjectionStatus projectTempoAnchor(
    const TempoPhaseAnchorV1& anchor, std::uint64_t epoch_id,
    std::uint64_t media_frame, std::uint8_t ratio_numerator,
    std::uint8_t ratio_denominator, TempoBeatPositionV1& out) noexcept;

// Projects a published entry with its projection ratio (1/1 for observed
// entries, p/q for derived entries).
[[nodiscard]] TempoProjectionStatus projectTempoCandidate(
    const TempoFieldV1& field, const TempoCandidateV1& candidate,
    std::uint64_t epoch_id, std::uint64_t media_frame,
    TempoBeatPositionV1& out) noexcept;

// Phase authority of an entry at a presentation time, using the thresholds
// echoed in the field header. Boundaries are inclusive: age == fresh limit is
// still fresh; age == coast limit is still coasting; beyond it is stale.
[[nodiscard]] TempoTrackState classifyTempoCandidateAt(
    const TempoFieldV1& field, const TempoCandidateV1& candidate,
    std::uint64_t epoch_id, std::uint64_t media_frame) noexcept;

enum class TempoFieldValidation : std::uint8_t {
  kValid = 0U,
  kUnknownSchema = 1U,
  kUnknownFlags = 2U,
  kCountOutOfRange = 3U,
  kNonFinite = 4U,
  kNegativeEvidence = 5U,
  kBadIdentity = 6U,
  kDuplicateIdentity = 7U,
  kBadRatio = 8U,
  kEpochMismatch = 9U,
  kTimeOrder = 10U,
  kBadAnchor = 11U,
  kBadState = 12U,
};

// Full semantic validation a consumer applies before trusting a field it did
// not produce (for example a copy received through a platform publication).
[[nodiscard]] TempoFieldValidation validateTempoFieldV1(
    const TempoFieldV1& field) noexcept;

}  // namespace k1::contract
