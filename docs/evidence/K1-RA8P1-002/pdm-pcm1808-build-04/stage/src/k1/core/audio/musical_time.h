#pragma once

// MUSICAL_TIME — tempo, beat and phase expressed in MEDIA_TIME_48K coordinates.
//
// Authority: research lane K1-AUDIO-TIMEBASE-R0 (REV2-D-020), built on the
// K1-DM-140 media coordinate.
//
// WHY THE PERIOD IS FRACTIONAL
// 120 BPM at 48 kHz happens to be exactly 24 000 frames per beat. Almost nothing
// else is. 127.43 BPM is 22 601.42... frames. An integer period would shed that
// remainder every beat and accumulate phase error without bound, so the period is
// carried as unsigned Q32.32 — 32 integer bits of frames, 32 fractional.
//
// WHY PREDICTION MULTIPLIES ONCE
// A predicted beat is anchor + k x period, computed as a SINGLE multiply in Q32.32
// and shifted once. It is never accumulated beat by beat, because accumulation
// re-rounds every step and reintroduces exactly the drift the fractional period
// exists to prevent.
//
// WHAT IS NOT HERE
// No fixed latency-compensation constant. The detector cadences (7.5 ms AP,
// 22.5 ms tempo, one-frame-late band triggers) are recorded as the source event's
// own frame estimate by the caller; this module never invents an offset.

#include <cstdint>

#include "core/audio/media_time.h"

namespace k1::core::audio {

// Frames per beat in unsigned Q32.32.
using BeatPeriodQ32 = std::uint64_t;

inline constexpr std::uint64_t kQ32One = 0x1'0000'0000ULL;

[[nodiscard]] constexpr BeatPeriodQ32 beatPeriodFromBpm(
    const double bpm, const std::uint32_t media_rate_hz = kMediaRateHz) noexcept {
  return (bpm > 0.0)
             ? static_cast<BeatPeriodQ32>((static_cast<double>(media_rate_hz) * 60.0 / bpm) *
                                          static_cast<double>(kQ32One))
             : 0ULL;
}

[[nodiscard]] constexpr double bpmFromBeatPeriod(
    const BeatPeriodQ32 period, const std::uint32_t media_rate_hz = kMediaRateHz) noexcept {
  return (period != 0ULL)
             ? (static_cast<double>(media_rate_hz) * 60.0 /
                (static_cast<double>(period) / static_cast<double>(kQ32One)))
             : 0.0;
}

// A media-frame position carrying its Q32 fraction, so a predicted beat is not
// silently truncated to a whole frame before the caller can see the remainder.
struct FramePositionQ32 final {
  std::uint64_t frame = 0U;
  std::uint32_t frac_q32 = 0U;  // 0 .. 2^32-1

  [[nodiscard]] double toDouble() const noexcept {
    return static_cast<double>(frame) +
           static_cast<double>(frac_q32) / static_cast<double>(kQ32One);
  }
};

// The musical model. `anchor` inherits AudioTime's epoch semantics: a beat
// anchored in epoch A is meaningless in epoch B and must be re-acquired.
struct MusicalTime final {
  AudioTime anchor{};                 // epoch + media frame of the anchor beat
  BeatPeriodQ32 beat_period_q32 = 0U; // frames per beat, fractional
  // Signed Q32.32 residual added to the integer anchor. bindTempoPhase() uses
  // it to preserve a sub-frame anchor; reanchor() consumes an observed correction.
  std::int64_t phase_error_q32 = 0;
  std::uint64_t beat_index = 0U;      // index of the anchor beat
  float tempo_bpm = 0.0F;
  float confidence = 0.0F;
  bool locked = false;

  [[nodiscard]] bool valid() const noexcept {
    return locked && beat_period_q32 != 0U;
  }
};

// Musical comparability inherits epoch comparability. Nothing about a beat
// survives an epoch break.
[[nodiscard]] constexpr bool comparable(const MusicalTime& a,
                                        const MusicalTime& b) noexcept {
  return a.anchor.epoch_id == b.anchor.epoch_id;
}
[[nodiscard]] constexpr bool comparable(const MusicalTime& m,
                                        const AudioTime& t) noexcept {
  return m.anchor.epoch_id == t.epoch_id;
}

// A beat event: where it BELONGS in media time, and separately when firmware
// learned of it. Never the same number.
struct BeatEvent final {
  std::uint64_t epoch_id = 0U;
  FramePositionQ32 beat_event_frame{};       // where the beat belongs
  std::uint64_t beat_result_available_frame = 0U;  // when firmware knew
  std::uint64_t beat_index = 0U;
  float confidence = 0.0F;
  bool predicted = false;  // true = forecast, false = observed

  // How late the knowledge was, in frames. Diagnostic only; never subtracted
  // from beat_event_frame by this module.
  [[nodiscard]] std::int64_t knowledgeLatencyFrames() const noexcept {
    return static_cast<std::int64_t>(beat_result_available_frame) -
           static_cast<std::int64_t>(beat_event_frame.frame);
  }
};

// Overflow guard for the single-multiply prediction. The product is Q32.32, but
// the RT1062 Arm GCC does not provide a 128-bit integer type. Split the 64x64
// multiply into 32-bit limbs and ask whether the INTEGER-frame portion fits in
// uint64_t. This preserves the full media-frame horizon without a host-only type.
[[nodiscard]] constexpr bool beatOffsetRepresentable(
    const BeatPeriodQ32 period, const std::uint64_t beats) noexcept {
  if (period == 0ULL || beats == 0ULL) return true;
  const std::uint64_t period_hi = period >> 32U;
  const std::uint64_t period_lo = period & 0xFFFFFFFFULL;
  const std::uint64_t beats_hi = beats >> 32U;
  const std::uint64_t beats_lo = beats & 0xFFFFFFFFULL;

  const std::uint64_t low_product = period_lo * beats_lo;
  const std::uint64_t fractional_frames =
      period_lo * beats_hi + (low_product >> 32U);
  if (period_hi == 0ULL) return true;
  return beats <= (UINT64_MAX - fractional_frames) / period_hi;
}

// Predicted position of `beat_index_target`. Exact: one multiply, one shift.
[[nodiscard]] bool predictBeatFrame(const MusicalTime& mt,
                                    std::uint64_t beat_index_target,
                                    FramePositionQ32& out) noexcept;

// The next beat strictly after `now_frame`.
[[nodiscard]] bool nextBeatFrame(const MusicalTime& mt, std::uint64_t now_frame,
                                 FramePositionQ32& out,
                                 std::uint64_t* out_beat_index = nullptr) noexcept;

// Phase within the current beat, 0..1, at `now_frame`.
[[nodiscard]] bool beatPhaseAt(const MusicalTime& mt, std::uint64_t now_frame,
                               double& phase_out) noexcept;

// Re-anchor onto an observed beat, recording the phase error against what the
// model predicted. This is the only way phase_error_q32 is set.
void reanchor(MusicalTime& mt, const AudioTime& observed_epoch_and_frame,
              std::uint64_t observed_beat_index, float confidence) noexcept;

// Epoch break: a beat anchored in the old epoch cannot project into the new one.
void invalidateForEpochChange(MusicalTime& mt,
                              std::uint64_t new_epoch_id) noexcept;


// Bind the existing tempo/flywheel estimate to MEDIA_TIME_48K without changing
// the detector. `now` is the media frame corresponding to the tempo update;
// phase01 says how far the flywheel is past its most recent beat. The function
// back-projects that beat into a sub-frame Q32.32 anchor. Call only when the
// tempo tracker reports an actual update edge (22.5 ms today), not on cached AP
// frames between updates.
[[nodiscard]] bool bindTempoPhase(MusicalTime& mt, const AudioTime& now,
                                  double bpm, double phase01, float confidence,
                                  bool beat_tick) noexcept;

// ---------------------------------------------------------------------------
// OUTPUT BOUNDARY ONLY
// ---------------------------------------------------------------------------
// Media frames are the coordinate. Microseconds are a presentation unit, and
// this is the single place the conversion is permitted to happen. A musical
// position is never stored, transported or compared in microseconds.
//
// 1 media frame = 1/48000 s = 125/6 us exactly. Do division first so the
// RT1062's 64-bit-only compiler never needs a 128-bit intermediate. The return
// type itself cannot represent the full uint64 frame horizon in microseconds, so
// the conversion saturates instead of wrapping at that presentation boundary.
[[nodiscard]] constexpr std::uint64_t mediaFramesToMicros(
    const std::uint64_t frames) noexcept {
  const std::uint64_t quotient = frames / 6ULL;
  const std::uint64_t remainder = frames % 6ULL;
  if (quotient > UINT64_MAX / 125ULL) return UINT64_MAX;
  const std::uint64_t whole = quotient * 125ULL;
  const std::uint64_t tail = (remainder * 125ULL) / 6ULL;
  return whole > UINT64_MAX - tail ? UINT64_MAX : whole + tail;
}

}  // namespace k1::core::audio
