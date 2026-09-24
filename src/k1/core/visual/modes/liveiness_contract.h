#pragma once

// Liveiness contract, version 1 (MOD lane).
//
// Liveiness is the user-facing expressive intent L in [0, 1] with a neutral
// identity at 0.5. It is a separate semantic from the legacy Mood controls
// (wire map IDs 5 and 25), which keep their existing per-mode laws; nothing in
// this module reads, writes or infers Liveiness from those IDs.
//
// Each admitted mode routes exactly one renderer quantity through its adapter.
// The adapter computes an effective value from an immutable base:
//
//   f(L)  = 2^(k * (2L - 1))                    (0.5 / 1 / 2 when k = 1)
//   v_eff = sign(v_base) * clamp(|v_base| * f(L), lo, hi)
//   lo    = min(v_min, |v_base|),  hi = max(v_max, |v_base|)
//
// The base-preserving limits mean the registered bounds restrict only the
// macro's excursion: an adapter never moves a base that already sits outside
// them, and the neutral point is an exact identity. The sign of a base is never
// changed, so a direction encoded in a signed velocity survives every amount.
//
// Response dynamics belong to CTL. This module is stateless: it keeps no
// smoothing, history or clock, so the effective value depends only on
// (base, L, enable) and is identical at every render cadence. It performs no
// allocation and touches no audio-processor state.

#include <cmath>
#include <cstdint>

namespace k1::core::visual::modes {

inline constexpr std::uint16_t kLiveinessContractVersion = 1U;
inline constexpr float kLiveinessMinimum = 0.0F;
inline constexpr float kLiveinessMaximum = 1.0F;
inline constexpr float kLiveinessNeutral = 0.5F;
// Amounts within this distance of neutral resolve to exact identity. It is
// below one CC14 step (1/16383), so it is not a perceptible dead band; it keeps
// a CTL response that settles asymptotically from leaving 1-ulp residue.
inline constexpr float kLiveinessNeutralSnap = 1.0F / 65536.0F;
// ORCH starting candidate: k = 1 gives factors 0.5 / 1 / 2 at L = 0 / 0.5 / 1.
inline constexpr float kLiveinessCandidateExponentGain = 1.0F;

// Per-channel applied Liveiness input. The enable is independent of the
// amount: disabling restores the unmodulated bases while the remembered
// amount survives. The default is the legacy profile (disabled, neutral).
struct LiveinessInput final {
  float amount = kLiveinessNeutral;
  bool enabled = false;
};

enum class LiveinessSupport : std::uint8_t {
  kUnsupported = 0U,  // disabled catalogue ordinal or unknown mode
  kPending = 1U,      // enabled mode whose adapter is not yet admitted
  kSupported = 2U,    // admitted adapter; the only state that modulates
};

enum class LiveinessFamily : std::uint8_t {
  kNone = 0U,
  kMaterial = 1U,
  kWaveform = 2U,
  kSpectrum = 3U,
  kRhythm = 4U,
};

enum class LiveinessDimension : std::uint8_t {
  kNone = 0U,
  kTransportRate = 1U,     // outward history/field transport rate
  kObjectVelocity = 2U,    // velocity of live objects, integrated each frame
  kObjectReach = 3U,       // spawn-time reach of future objects
  kPatternFlowRate = 4U,   // mode-local pattern/carrier rate (not an AP clock)
  kExcursion = 5U,         // gesture excursion amplitude
};

enum class LiveinessObjectPolicy : std::uint8_t {
  kNotApplicable = 0U,
  kFieldContinuous = 1U,          // history field: an edit changes only future
                                  // displacement, so nothing can teleport
  kCurrentObjectsReanchored = 2U, // live objects integrate the effective rate
                                  // from the edit onward from their position
  kFutureObjectsOnly = 3U,        // sampled at spawn; live objects keep their
                                  // spawn value (UI must say so)
  kModeSmoothed = 4U,             // enters before the mode's own authored
                                  // smoother, which provides continuity
};

// Audio evidence an admitted adapter needs before its status reads active.
namespace evidence {
inline constexpr std::uint8_t kNone = 0U;
inline constexpr std::uint8_t kAmplitude = 1U << 0U;    // not silence-flagged
inline constexpr std::uint8_t kChroma = 1U << 1U;       // chroma valid, audible
inline constexpr std::uint8_t kSpectrum = 1U << 2U;     // spectrum valid, audible
inline constexpr std::uint8_t kOnsetEvents = 1U << 3U;  // onset/percussion valid
inline constexpr std::uint8_t kTempoLock = 1U << 4U;    // TempoFieldV1 lock
inline constexpr std::uint8_t kAll = 0x1FU;
}  // namespace evidence

enum class LiveinessReason : std::uint8_t {
  kNone = 0U,             // active (including an amount at neutral)
  kUnsupportedMode = 1U,  // no admitted adapter for this mode
  kDisabled = 2U,         // enable off; bases pass through unmodulated
  kInvalidAmount = 3U,    // non-finite amount; treated as neutral
  kInvalidBase = 4U,      // non-finite base; passed through untouched
  kBaseZero = 5U,         // a zero base masks a multiplicative macro
  kLimitMasked = 6U,      // the registered limit masks the requested direction
  kLimitClamped = 7U,     // partly clamped at the registered limit; active
  kNoEvidence = 8U,       // required audio evidence absent (status only)
};

// Acceptance of an admitted adapter against its registered criteria. A wired
// adapter whose registered criterion failed stays wired (its hook is live)
// but is never counted as accepted; the failing criterion and root cause are
// published so capability feedback can say so.
enum class LiveinessAcceptance : std::uint8_t {
  kNotScored = 0U,           // pending rows
  kHostPass = 1U,            // every registered host criterion passed
  kFailReferenceBound = 2U,  // a registered criterion failed; root cause is
                             // the legacy path, proven by exact equivalence
};

struct LiveinessAdapterDescriptor final {
  std::uint16_t mode_id = 0U;
  std::uint16_t adapter_version = 0U;
  LiveinessSupport support = LiveinessSupport::kUnsupported;
  LiveinessFamily family = LiveinessFamily::kNone;
  LiveinessDimension dimension = LiveinessDimension::kNone;
  LiveinessObjectPolicy object_policy = LiveinessObjectPolicy::kNotApplicable;
  std::uint8_t evidence = evidence::kNone;
  float exponent_gain = 0.0F;  // k in f(L) = 2^(k(2L-1))
  float minimum = 0.0F;        // registered lower bound on |effective|
  float maximum = 0.0F;        // registered upper bound on |effective|
  const char* parameter = "";  // renderer quantity routed by the hook
  const char* units = "";
  const char* ui_key = "";     // UI explanation key
  LiveinessAcceptance acceptance = LiveinessAcceptance::kNotScored;
  const char* acceptance_note = "";  // failing criterion and cause, if any
};

struct LiveinessResult final {
  float effective = 0.0F;
  float base = 0.0F;
  float factor = 1.0F;  // requested f(L); 1 when bypassed
  bool active = false;
  bool clamped = false;
  LiveinessReason reason = LiveinessReason::kUnsupportedMode;
};

[[nodiscard]] inline float sanitiseLiveinessAmount(const float amount) noexcept {
  if (!std::isfinite(amount)) {
    return kLiveinessNeutral;
  }
  if (amount <= kLiveinessMinimum) {
    return kLiveinessMinimum;
  }
  return amount >= kLiveinessMaximum ? kLiveinessMaximum : amount;
}

[[nodiscard]] inline bool liveinessAtNeutral(const float amount) noexcept {
  const float distance = amount - kLiveinessNeutral;
  return distance <= kLiveinessNeutralSnap &&
         distance >= -kLiveinessNeutralSnap;
}

// f(L) = 2^(k(2L - 1)); exactly 1 at (and within the snap of) neutral.
[[nodiscard]] inline float liveinessFactor(const float amount,
                                           const float exponent_gain) noexcept {
  const float sanitised = sanitiseLiveinessAmount(amount);
  if (liveinessAtNeutral(sanitised) || !std::isfinite(exponent_gain) ||
      exponent_gain <= 0.0F) {
    return 1.0F;
  }
  // Written about the neutral constant so the law and the snap share one
  // neutral: 2^(2k(L - 0.5)) == 2^(k(2L - 1)).
  return std::exp2(2.0F * exponent_gain * (sanitised - kLiveinessNeutral));
}

// Inverse of liveinessFactor: the amount that requests `factor` (clamped to
// the legal domain). Used by migration and UI annotation, never by render.
[[nodiscard]] inline float liveinessForFactor(
    const float factor, const float exponent_gain) noexcept {
  if (!std::isfinite(factor) || factor <= 0.0F ||
      !std::isfinite(exponent_gain) || exponent_gain <= 0.0F) {
    return kLiveinessNeutral;
  }
  return sanitiseLiveinessAmount(kLiveinessNeutral +
                                 std::log2(factor) / (2.0F * exponent_gain));
}

[[nodiscard]] inline LiveinessResult resolveLiveiness(
    const LiveinessAdapterDescriptor& adapter, const float base,
    const LiveinessInput& input) noexcept {
  LiveinessResult result{};
  result.base = base;
  result.effective = base;
  result.factor = 1.0F;
  if (adapter.support != LiveinessSupport::kSupported) {
    result.reason = LiveinessReason::kUnsupportedMode;
    return result;
  }
  if (!input.enabled) {
    result.reason = LiveinessReason::kDisabled;
    return result;
  }
  if (!std::isfinite(input.amount)) {
    result.reason = LiveinessReason::kInvalidAmount;
    return result;
  }
  if (!std::isfinite(base)) {
    result.reason = LiveinessReason::kInvalidBase;
    return result;
  }
  const float factor = liveinessFactor(input.amount, adapter.exponent_gain);
  result.factor = factor;
  if (base == 0.0F) {
    result.reason = LiveinessReason::kBaseZero;
    return result;
  }
  if (factor == 1.0F) {
    result.active = true;
    result.reason = LiveinessReason::kNone;
    return result;
  }
  const float magnitude = base < 0.0F ? -base : base;
  const float lower = adapter.minimum < magnitude ? adapter.minimum : magnitude;
  const float upper = adapter.maximum > magnitude ? adapter.maximum : magnitude;
  const float requested = magnitude * factor;
  const float limited =
      requested < lower ? lower : (requested > upper ? upper : requested);
  result.clamped = limited != requested;
  if (limited == magnitude) {
    result.reason = LiveinessReason::kLimitMasked;
    return result;
  }
  result.effective = base < 0.0F ? -limited : limited;
  result.active = true;
  result.reason = result.clamped ? LiveinessReason::kLimitClamped
                                 : LiveinessReason::kNone;
  return result;
}

}  // namespace k1::core::visual::modes
