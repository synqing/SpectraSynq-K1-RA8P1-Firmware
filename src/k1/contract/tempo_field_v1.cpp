#include "contract/tempo_field_v1.h"

#include <cmath>

namespace k1::contract {
namespace {

constexpr std::uint64_t kLow32 = 0xFFFFFFFFULL;
// Periods from one media frame to 2^31 frames per beat keep every intermediate
// below 2^64 in the long division below.
constexpr std::uint64_t kMinimumPeriodQ32 = kTempoQ32One;
constexpr std::uint64_t kMaximumPeriodQ32 = 1ULL << 63U;
constexpr std::uint64_t kProjectionHorizonFrames = 1ULL << 32U;

// floor(delta * 2^64 / period) as Q32.32 beats, exact. `delta` < 2^32 and
// kMinimumPeriodQ32 <= period < kMaximumPeriodQ32 are guaranteed by callers.
void beatsFromFrames(const std::uint64_t delta_frames,
                     const std::uint64_t period_q32, std::uint64_t& beats_q32,
                     bool& inexact) noexcept {
  const std::uint64_t numerator = delta_frames << 32U;
  const std::uint64_t whole = numerator / period_q32;  // <= delta < 2^32
  std::uint64_t remainder = numerator % period_q32;
  std::uint64_t fraction = 0U;
  // Bounded binary long division: 32 quotient bits, never a data-dependent
  // loop. remainder < period < 2^63, so the shift cannot overflow.
  for (unsigned bit = 0U; bit < 32U; ++bit) {
    remainder <<= 1U;
    fraction <<= 1U;
    if (remainder >= period_q32) {
      remainder -= period_q32;
      fraction |= 1U;
    }
  }
  beats_q32 = (whole << 32U) | fraction;
  inexact = remainder != 0U;
}

bool validPeriod(const std::uint64_t period_q32) noexcept {
  return period_q32 >= kMinimumPeriodQ32 && period_q32 < kMaximumPeriodQ32;
}

bool finite(const float value) noexcept { return std::isfinite(value); }

bool finiteNonNegative(const float value) noexcept {
  return std::isfinite(value) && value >= 0.0F;
}

// Weaker of two authorities: observed/derived > coasting > stale > invalid.
int authorityRank(const TempoTrackState state) noexcept {
  switch (state) {
    case TempoTrackState::kObserved:
    case TempoTrackState::kDerived:
      return 3;
    case TempoTrackState::kCoasting:
      return 2;
    case TempoTrackState::kStale:
      return 1;
    case TempoTrackState::kInvalid:
    default:
      return 0;
  }
}

bool candidateFloatsFinite(const TempoCandidateV1& candidate) noexcept {
  const float values[] = {
      candidate.comb_salience,         candidate.point_salience,
      candidate.prominence,            candidate.bank_smoothed,
      candidate.phase_coherence,       candidate.phase_innovation_cycles,
      candidate.applied_correction_cycles,
      candidate.phase_uncertainty_cycles, candidate.running_bpm};
  for (const float value : values) {
    if (!finite(value)) return false;
  }
  return true;
}

bool candidateEvidenceNonNegative(const TempoCandidateV1& candidate) noexcept {
  return finiteNonNegative(candidate.comb_salience) &&
         finiteNonNegative(candidate.point_salience) &&
         finiteNonNegative(candidate.prominence) &&
         finiteNonNegative(candidate.bank_smoothed) &&
         finiteNonNegative(candidate.phase_uncertainty_cycles) &&
         finiteNonNegative(candidate.running_bpm) &&
         candidate.phase_coherence >= 0.0F &&
         candidate.phase_coherence <= 1.0F;
}

bool isZeroCandidate(const TempoCandidateV1& candidate) noexcept {
  return candidate.admission ==
             static_cast<std::uint8_t>(TempoCandidateAdmission::kEmpty) &&
         candidate.track_generation == 0U && candidate.flags == 0U;
}

}  // namespace

TempoProjectionStatus projectTempoAnchor(const TempoPhaseAnchorV1& anchor,
                                         const std::uint64_t epoch_id,
                                         const std::uint64_t media_frame,
                                         const std::uint8_t ratio_numerator,
                                         const std::uint8_t ratio_denominator,
                                         TempoBeatPositionV1& out) noexcept {
  if (ratio_numerator == 0U || ratio_denominator == 0U) {
    return TempoProjectionStatus::kZeroRatio;
  }
  if (anchor.epoch_id != epoch_id) return TempoProjectionStatus::kEpochMismatch;
  if (!validPeriod(anchor.beat_period_q32)) {
    return TempoProjectionStatus::kInvalidAnchor;
  }
  const bool forward = media_frame >= anchor.reference_media_frame;
  const std::uint64_t delta = forward
                                  ? media_frame - anchor.reference_media_frame
                                  : anchor.reference_media_frame - media_frame;
  if (delta >= kProjectionHorizonFrames) {
    return TempoProjectionStatus::kOutOfHorizon;
  }
  std::uint64_t beats = 0U;
  bool inexact = false;
  beatsFromFrames(delta, anchor.beat_period_q32, beats, inexact);
  std::uint64_t position = 0U;
  if (forward) {
    if (beats > UINT64_MAX - anchor.beat_position_q32) {
      return TempoProjectionStatus::kOverflow;
    }
    position = anchor.beat_position_q32 + beats;
  } else {
    // floor(B - x) = B - ceil(x) for the exact real x.
    const std::uint64_t ceiling = beats + (inexact ? 1U : 0U);
    if (ceiling > anchor.beat_position_q32) {
      return TempoProjectionStatus::kBeforeOrigin;
    }
    position = anchor.beat_position_q32 - ceiling;
  }
  const std::uint64_t whole_beats = position >> 32U;
  const std::uint64_t fraction = position & kLow32;
  // Exact floor((p / q) * (whole + fraction / 2^32)). whole < 2^32 and p, q
  // are eight-bit, so every product stays below 2^41.
  const std::uint64_t scaled_whole =
      static_cast<std::uint64_t>(ratio_numerator) * whole_beats;
  std::uint64_t result_whole = scaled_whole / ratio_denominator;
  const std::uint64_t whole_remainder = scaled_whole % ratio_denominator;
  const std::uint64_t fraction_numerator =
      (whole_remainder << 32U) +
      static_cast<std::uint64_t>(ratio_numerator) * fraction;
  const std::uint64_t scaled_fraction = fraction_numerator / ratio_denominator;
  result_whole += scaled_fraction >> 32U;
  const std::uint32_t phase_q32 =
      static_cast<std::uint32_t>(scaled_fraction & kLow32);
  out.beat_index = result_whole;
  out.phase_q32 = phase_q32;
  // Correctly rounded conversion can reach 1.0F for phases within 2^-25 of a
  // wrap; publish the largest float below one instead of a false wrap.
  float phase01 = static_cast<float>(phase_q32) * (1.0F / 4294967296.0F);
  if (phase01 >= 1.0F) phase01 = 0.99999994F;
  out.phase01 = phase01;
  return TempoProjectionStatus::kOk;
}

TempoProjectionStatus projectTempoCandidate(const TempoFieldV1& field,
                                            const TempoCandidateV1& candidate,
                                            const std::uint64_t epoch_id,
                                            const std::uint64_t media_frame,
                                            TempoBeatPositionV1& out) noexcept {
  if (field.stream_epoch != epoch_id) {
    return TempoProjectionStatus::kEpochMismatch;
  }
  if ((candidate.flags & kCandidatePhaseValid) == 0U) {
    return TempoProjectionStatus::kInvalidAnchor;
  }
  return projectTempoAnchor(candidate.anchor, epoch_id, media_frame,
                            candidate.projection_ratio_numerator,
                            candidate.projection_ratio_denominator, out);
}

TempoTrackState classifyTempoCandidateAt(const TempoFieldV1& field,
                                         const TempoCandidateV1& candidate,
                                         const std::uint64_t epoch_id,
                                         const std::uint64_t media_frame) noexcept {
  if ((field.flags & kTempoFieldEnabled) == 0U ||
      (field.flags & kTempoFieldValid) == 0U ||
      field.stream_epoch != epoch_id ||
      candidate.admission ==
          static_cast<std::uint8_t>(TempoCandidateAdmission::kEmpty) ||
      (candidate.flags & kCandidatePhaseValid) == 0U ||
      candidate.anchor.epoch_id != epoch_id) {
    return TempoTrackState::kInvalid;
  }
  const auto published = static_cast<TempoTrackState>(candidate.state);
  if (authorityRank(published) <= 1) return published;
  const std::uint64_t age =
      media_frame >= candidate.phase_observed_media_frame
          ? media_frame - candidate.phase_observed_media_frame
          : 0U;
  TempoTrackState by_age = published;
  if (age > field.coast_limit_media_frames) {
    by_age = TempoTrackState::kStale;
  } else if (age > field.fresh_limit_media_frames) {
    by_age = TempoTrackState::kCoasting;
  }
  return authorityRank(by_age) < authorityRank(published) ? by_age : published;
}

TempoFieldValidation validateTempoFieldV1(const TempoFieldV1& field) noexcept {
  if (field.schema_major != kTempoFieldSchemaMajor) {
    return TempoFieldValidation::kUnknownSchema;
  }
  if ((field.flags & ~kTempoFieldKnownFlags) != 0U) {
    return TempoFieldValidation::kUnknownFlags;
  }
  if (field.candidate_count > kTempoFieldCandidateCapacity ||
      field.selected_count > kTempoFieldMaxSelected ||
      field.selected_count > field.candidate_count ||
      (field.primary_index != kTempoFieldNoIndex &&
       field.primary_index >= field.candidate_count)) {
    return TempoFieldValidation::kCountOutOfRange;
  }
  if (field.reason > static_cast<std::uint8_t>(
                         TempoFieldReason::kDiscardedJob) ||
      field.coast_reason >
          static_cast<std::uint8_t>(TempoCoastReason::kWarmup) ||
      field.canonical.time_base >
          static_cast<std::uint8_t>(TempoCanonicalTimeBase::kTrackerMediaMs)) {
    return TempoFieldValidation::kBadState;
  }
  const float header_values[] = {
      field.activity,           field.activity_floor,
      field.latest_novelty,     field.concentration,
      field.canonical.winner_target_bpm, field.canonical.running_bpm,
      field.canonical.phase01,  field.canonical.confidence,
      field.canonical.beat_strength};
  for (const float value : header_values) {
    if (!finite(value)) return TempoFieldValidation::kNonFinite;
  }
  for (const TempoCandidateV1& candidate : field.candidates) {
    if (!candidateFloatsFinite(candidate)) {
      return TempoFieldValidation::kNonFinite;
    }
  }
  if (field.activity < 0.0F || field.activity_floor < 0.0F ||
      field.latest_novelty < 0.0F || field.concentration < 0.0F ||
      field.concentration > 1.0F || field.canonical.confidence < 0.0F ||
      field.canonical.beat_strength < 0.0F) {
    return TempoFieldValidation::kNegativeEvidence;
  }
  if (field.support_start_media_frame > field.support_end_media_frame ||
      field.fresh_limit_media_frames > field.coast_limit_media_frames ||
      field.coast_limit_media_frames > field.stale_limit_media_frames) {
    return TempoFieldValidation::kTimeOrder;
  }
  std::uint8_t selected = 0U;
  for (std::size_t index = 0U; index < kTempoFieldCandidateCapacity; ++index) {
    const TempoCandidateV1& candidate = field.candidates[index];
    if (index >= field.candidate_count) {
      if (!isZeroCandidate(candidate)) {
        return TempoFieldValidation::kCountOutOfRange;
      }
      continue;
    }
    if (!candidateEvidenceNonNegative(candidate)) {
      return TempoFieldValidation::kNegativeEvidence;
    }
    if ((candidate.flags & ~kTempoCandidateKnownFlags) != 0U ||
        candidate.state > static_cast<std::uint8_t>(TempoTrackState::kStale)) {
      return TempoFieldValidation::kBadState;
    }
    const auto admission =
        static_cast<TempoCandidateAdmission>(candidate.admission);
    if (candidate.family_ratio_numerator == 0U ||
        candidate.family_ratio_denominator == 0U ||
        candidate.projection_ratio_numerator == 0U ||
        candidate.projection_ratio_denominator == 0U) {
      return TempoFieldValidation::kBadRatio;
    }
    if (admission == TempoCandidateAdmission::kSelected ||
        admission == TempoCandidateAdmission::kRetiring) {
      if (candidate.bin_id >= kTempoFieldBankBinCount ||
          candidate.track_generation == 0U) {
        return TempoFieldValidation::kBadIdentity;
      }
      if (candidate.projection_ratio_numerator != 1U ||
          candidate.projection_ratio_denominator != 1U) {
        return TempoFieldValidation::kBadRatio;
      }
      if (candidate.state ==
          static_cast<std::uint8_t>(TempoTrackState::kDerived)) {
        return TempoFieldValidation::kBadState;
      }
      if (admission == TempoCandidateAdmission::kSelected) ++selected;
    } else if (admission == TempoCandidateAdmission::kDerived) {
      if (candidate.bin_id != kTempoFieldNoBin ||
          candidate.family_root_bin >= kTempoFieldBankBinCount ||
          candidate.family_root_generation == 0U ||
          candidate.track_generation == 0U) {
        return TempoFieldValidation::kBadIdentity;
      }
      // A derived entry is never an observation.
      if (candidate.state ==
          static_cast<std::uint8_t>(TempoTrackState::kObserved)) {
        return TempoFieldValidation::kBadState;
      }
    } else {
      return TempoFieldValidation::kBadIdentity;
    }
    if ((candidate.flags & kCandidatePhaseValid) != 0U) {
      if (candidate.anchor.epoch_id != field.stream_epoch) {
        return TempoFieldValidation::kEpochMismatch;
      }
      if (!validPeriod(candidate.anchor.beat_period_q32)) {
        return TempoFieldValidation::kBadAnchor;
      }
      if (candidate.phase_observed_media_frame > field.support_end_media_frame) {
        return TempoFieldValidation::kTimeOrder;
      }
    } else if (candidate.state ==
                   static_cast<std::uint8_t>(TempoTrackState::kObserved) ||
               candidate.state ==
                   static_cast<std::uint8_t>(TempoTrackState::kDerived)) {
      return TempoFieldValidation::kBadState;
    }
    for (std::size_t other = 0U; other < index; ++other) {
      const TempoCandidateV1& previous = field.candidates[other];
      const bool both_derived =
          previous.admission == candidate.admission &&
          admission == TempoCandidateAdmission::kDerived;
      if (both_derived) {
        if (previous.family_root_bin == candidate.family_root_bin &&
            previous.projection_ratio_numerator ==
                candidate.projection_ratio_numerator &&
            previous.projection_ratio_denominator ==
                candidate.projection_ratio_denominator) {
          return TempoFieldValidation::kDuplicateIdentity;
        }
      } else if (admission != TempoCandidateAdmission::kDerived &&
                 previous.admission !=
                     static_cast<std::uint8_t>(
                         TempoCandidateAdmission::kDerived) &&
                 previous.bin_id == candidate.bin_id) {
        return TempoFieldValidation::kDuplicateIdentity;
      }
    }
  }
  if (selected != field.selected_count) {
    return TempoFieldValidation::kCountOutOfRange;
  }
  return TempoFieldValidation::kValid;
}

}  // namespace k1::contract
