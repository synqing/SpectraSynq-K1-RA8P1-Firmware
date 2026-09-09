#include "core/audio/musical_time.h"

#include <cmath>

namespace k1::core::audio {
namespace {

// Exact Q32.32 period x uint64 beat count using 32-bit limbs. The result is
// returned as whole frames + Q32 fraction; false means the whole-frame portion
// itself exceeds uint64_t.
bool multiplyPeriod(const BeatPeriodQ32 period, const std::uint64_t beats,
                    std::uint64_t& frames, std::uint32_t& frac_q32) noexcept {
  if (!beatOffsetRepresentable(period, beats)) return false;
  const std::uint64_t period_hi = period >> 32U;
  const std::uint64_t period_lo = period & 0xFFFFFFFFULL;
  const std::uint64_t beats_hi = beats >> 32U;
  const std::uint64_t beats_lo = beats & 0xFFFFFFFFULL;

  const std::uint64_t low_product = period_lo * beats_lo;
  frac_q32 = static_cast<std::uint32_t>(low_product & 0xFFFFFFFFULL);
  const std::uint64_t fractional_frames =
      period_lo * beats_hi + (low_product >> 32U);
  frames = period_hi * beats + fractional_frames;
  return true;
}

// anchor + delta_beats x period, as ONE exact fixed-point multiply.
bool addBeats(const MusicalTime& mt, const std::uint64_t delta_beats,
              FramePositionQ32& out) noexcept {
  std::uint64_t offset_frames = 0U;
  std::uint32_t offset_frac = 0U;
  if (!multiplyPeriod(mt.beat_period_q32, delta_beats, offset_frames, offset_frac)) {
    return false;
  }
  if (offset_frames > UINT64_MAX - mt.anchor.frame_index) return false;

  std::uint64_t position_frame = mt.anchor.frame_index + offset_frames;
  std::uint32_t position_frac = offset_frac;

  // Apply the signed residual in Q32.32 without converting an unsigned product
  // through int64_t. This is constant-time even across the old signed boundary.
  const bool negative = mt.phase_error_q32 < 0;
  const std::uint64_t magnitude = negative
      ? (mt.phase_error_q32 == INT64_MIN
             ? (1ULL << 63U)
             : static_cast<std::uint64_t>(-mt.phase_error_q32))
      : static_cast<std::uint64_t>(mt.phase_error_q32);
  const std::uint64_t phase_frames = magnitude >> 32U;
  const std::uint32_t phase_frac =
      static_cast<std::uint32_t>(magnitude & 0xFFFFFFFFULL);

  if (!negative) {
    const std::uint64_t frac_sum =
        static_cast<std::uint64_t>(position_frac) + phase_frac;
    const std::uint64_t carry = frac_sum >> 32U;
    if (phase_frames > UINT64_MAX - position_frame) return false;
    position_frame += phase_frames;
    if (carry > UINT64_MAX - position_frame) return false;
    position_frame += carry;
    position_frac = static_cast<std::uint32_t>(frac_sum & 0xFFFFFFFFULL);
  } else {
    // Compare the absolute position pair against the magnitude before subtracting.
    if (position_frame < phase_frames ||
        (position_frame == phase_frames && position_frac < phase_frac)) {
      return false;
    }
    position_frame -= phase_frames;
    if (position_frac < phase_frac) {
      if (position_frame == 0U) return false;
      --position_frame;
      position_frac = static_cast<std::uint32_t>(
          (static_cast<std::uint64_t>(1ULL << 32U) + position_frac) - phase_frac);
    } else {
      position_frac = static_cast<std::uint32_t>(position_frac - phase_frac);
    }
  }

  out.frame = position_frame;
  out.frac_q32 = position_frac;
  return true;
}


}  // namespace

bool predictBeatFrame(const MusicalTime& mt, const std::uint64_t beat_index_target,
                      FramePositionQ32& out) noexcept {
  if (!mt.valid() || beat_index_target < mt.beat_index) {
    return false;
  }
  return addBeats(mt, beat_index_target - mt.beat_index, out);
}

bool nextBeatFrame(const MusicalTime& mt, const std::uint64_t now_frame,
                   FramePositionQ32& out, std::uint64_t* const out_beat_index) noexcept {
  if (!mt.valid()) {
    return false;
  }
  // Closed-form estimate of how many beats past the anchor `now` sits, then a
  // bounded correction. No unbounded loop.
  const double period_frames =
      static_cast<double>(mt.beat_period_q32) / static_cast<double>(kQ32One);
  if (period_frames <= 0.0) {
    return false;
  }
  double elapsed = static_cast<double>(now_frame) -
                   static_cast<double>(mt.anchor.frame_index);
  if (elapsed < 0.0) {
    elapsed = 0.0;
  }
  std::uint64_t k = static_cast<std::uint64_t>(elapsed / period_frames);

  FramePositionQ32 candidate{};
  for (int guard = 0; guard < 4; ++guard) {
    if (!addBeats(mt, k, candidate)) {
      return false;
    }
    if (candidate.frame > now_frame ||
        (candidate.frame == now_frame && candidate.frac_q32 > 0U)) {
      if (k > 0U) {
        FramePositionQ32 prev{};
        if (addBeats(mt, k - 1U, prev) &&
            (prev.frame > now_frame ||
             (prev.frame == now_frame && prev.frac_q32 > 0U))) {
          --k;
          continue;  // stepped too far forward
        }
      }
      out = candidate;
      if (out_beat_index != nullptr) {
        *out_beat_index = mt.beat_index + k;
      }
      return true;
    }
    ++k;
  }
  return false;
}

bool beatPhaseAt(const MusicalTime& mt, const std::uint64_t now_frame,
                 double& phase_out) noexcept {
  if (!mt.valid()) {
    return false;
  }
  const double period_frames =
      static_cast<double>(mt.beat_period_q32) / static_cast<double>(kQ32One);
  if (period_frames <= 0.0) {
    return false;
  }
  const double anchor =
      static_cast<double>(mt.anchor.frame_index) +
      static_cast<double>(mt.phase_error_q32) / static_cast<double>(kQ32One);
  double phase = (static_cast<double>(now_frame) - anchor) / period_frames;
  phase -= static_cast<double>(static_cast<long long>(phase));
  if (phase < 0.0) {
    phase += 1.0;
  }
  phase_out = phase;
  return true;
}

void reanchor(MusicalTime& mt, const AudioTime& observed,
              const std::uint64_t observed_beat_index, const float confidence) noexcept {
  if (mt.anchor.epoch_id != observed.epoch_id) {
    // Different epoch: nothing carries over. Re-acquire from scratch.
    mt.anchor = observed;
    mt.beat_index = observed_beat_index;
    mt.phase_error_q32 = 0;
    mt.confidence = confidence;
    mt.locked = mt.beat_period_q32 != 0U;
    return;
  }
  FramePositionQ32 predicted{};
  if (mt.valid() && predictBeatFrame(mt, observed_beat_index, predicted)) {
    const double err =
        (static_cast<double>(observed.frame_index) - predicted.toDouble()) *
        static_cast<double>(kQ32One);
    mt.phase_error_q32 = static_cast<std::int64_t>(err);
  } else {
    mt.phase_error_q32 = 0;
  }
  mt.anchor = observed;
  mt.beat_index = observed_beat_index;
  mt.confidence = confidence;
  mt.locked = mt.beat_period_q32 != 0U;
  // The anchor now IS the observation, so its residual is consumed.
  mt.phase_error_q32 = 0;
}

bool bindTempoPhase(MusicalTime& mt, const AudioTime& now, const double bpm,
                    const double phase01, const float confidence,
                    const bool beat_tick) noexcept {
  if (!std::isfinite(bpm) || bpm <= 0.0 || !std::isfinite(phase01) ||
      !std::isfinite(confidence)) {
    return false;
  }
  const BeatPeriodQ32 period = beatPeriodFromBpm(bpm);
  if (period == 0U) return false;

  double phase = phase01 - std::floor(phase01);
  if (phase < 0.0) phase += 1.0;
  const double elapsed_raw = static_cast<double>(period) * phase;
  if (elapsed_raw < 0.0 || elapsed_raw > static_cast<double>(UINT64_MAX)) {
    return false;
  }
  const std::uint64_t elapsed_q32 =
      static_cast<std::uint64_t>(elapsed_raw + 0.5);
  const std::uint64_t elapsed_frames = elapsed_q32 >> 32U;
  const std::uint32_t elapsed_frac =
      static_cast<std::uint32_t>(elapsed_q32 & 0xFFFFFFFFULL);
  const std::uint64_t borrow = elapsed_frac == 0U ? 0U : 1U;
  if (elapsed_frames > now.frame_index ||
      borrow > now.frame_index - elapsed_frames) {
    return false;
  }

  std::uint64_t beat_index = 0U;
  if (mt.valid() && mt.anchor.epoch_id == now.epoch_id) {
    beat_index = mt.beat_index;
    if (beat_tick) {
      if (beat_index == UINT64_MAX) return false;
      ++beat_index;
    }
  }

  mt.anchor.epoch_id = now.epoch_id;
  mt.anchor.frame_index = now.frame_index - elapsed_frames - borrow;
  mt.phase_error_q32 =
      elapsed_frac == 0U
          ? 0
          : static_cast<std::int64_t>(kQ32One - elapsed_frac);
  mt.beat_period_q32 = period;
  mt.beat_index = beat_index;
  mt.tempo_bpm = static_cast<float>(bpm);
  mt.confidence = confidence < 0.0F ? 0.0F : (confidence > 1.0F ? 1.0F : confidence);
  mt.locked = true;
  return true;
}

void invalidateForEpochChange(MusicalTime& mt, const std::uint64_t new_epoch_id) noexcept {
  mt.locked = false;
  mt.confidence = 0.0F;
  mt.phase_error_q32 = 0;
  mt.beat_index = 0U;
  mt.anchor.epoch_id = new_epoch_id;
  mt.anchor.frame_index = 0U;
  // beat_period_q32 is deliberately RETAINED: tempo is a property of the music,
  // not of the capture stream, so relock can start from the last known tempo.
  // `locked` stays false until an observed beat re-anchors it.
}

}  // namespace k1::core::audio
