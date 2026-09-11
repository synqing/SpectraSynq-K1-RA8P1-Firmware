#pragma once

// MEDIA_TIME_48K — the canonical media coordinate for SpectraSynq K1.
//
// Authority: research lane K1-AUDIO-TIMEBASE-R0 (K1-CORE-VAL-REV2,
// research/lanes/K1-AUDIO-TIMEBASE-R0/), rulings REV2-D-018..023.
//
// THREE COORDINATES, KEPT SEPARATE. Conflating them is the defect this module
// exists to prevent:
//
//   MEDIA_TIME_48K        uint64 count of 48 kHz TDM capture frames. CANONICAL.
//                         One frame is ONE media instant shared by all four TDM
//                         slots (AUX-L, AUX-R, ROOM_MIC, reserved).
//   ANALYSIS_TIME         uint64 index in the analysis domain (24 kHz today,
//                         12.8 kHz on the legacy reference path).
//   RESULT_MONOTONIC_TIME wall-clock microseconds. Availability ONLY. It never
//                         expresses a musical or media position.
//
// SCOPE HONESTY. The DualMCU capture path today receives samples that are
// already at the analysis rate (CaptureWindowAssembler consumes 32-sample
// Teensy Audio blocks at 24 kHz). A real 48 kHz four-slot TDM ingest is future
// CORE-VAL hardware. This module therefore DEFINES the media coordinate and
// derives it exactly from the analysis stream today. When SAI/TDM ingest lands,
// only the SOURCE of frame_index changes — every consumer stays as written.
// Nothing here claims 48 kHz capture exists.
//
// WHAT ADVANCES THE COUNTER. Only frames that reached memory. Not MCLK edges,
// not BCLK, not FSYNC transitions: a frame that was clocked but lost to a DMA
// overrun must never advance a counter used to timestamp data you hold.

#include <atomic>
#include <cstdint>

namespace k1::core::audio {

inline constexpr std::uint32_t kMediaRateHz = 48000U;

// Exact rational media-frames-per-analysis-sample. Both supported analysis
// rates are exact rationals of 48 kHz, so no timestamp drift is introduced by
// the conversion itself:
//   48000 -> 24000   = 2/1   (each analysis sample is exactly 2 media frames)
//   48000 -> 12800   = 15/4  (15 media frames per 4 analysis samples)
struct MediaRatio final {
  std::uint32_t numerator = 0U;
  std::uint32_t denominator = 1U;
};

[[nodiscard]] constexpr MediaRatio mediaRatioFor(
    const std::uint32_t analysis_rate_hz) noexcept {
  return analysis_rate_hz == 24000U   ? MediaRatio{2U, 1U}
         : analysis_rate_hz == 12800U ? MediaRatio{15U, 4U}
                                      : MediaRatio{0U, 1U};  // unsupported
}

// A media position that may fall between whole frames. `quarter` is in units of
// a quarter media frame and is always 0 for the 24 kHz path; the 12.8 kHz path
// needs it because 15/4 is not integral. Kept exact — never a float.
struct MediaPosition final {
  std::uint64_t frame = 0U;
  std::uint8_t quarter = 0U;  // 0..3
};

// analysis sample index -> media position, EXACT for both supported rates.
[[nodiscard]] constexpr MediaPosition analysisToMedia(
    const std::uint64_t analysis_index,
    const std::uint32_t analysis_rate_hz) noexcept {
  const MediaRatio ratio = mediaRatioFor(analysis_rate_hz);
  if (ratio.numerator == 0U) {
    return MediaPosition{0U, 0U};
  }
  // quarters = analysis_index * numerator * 4 / denominator, exact because
  // denominator is 1 or 4.
  const std::uint64_t quarters =
      (analysis_index * static_cast<std::uint64_t>(ratio.numerator) * 4U) /
      static_cast<std::uint64_t>(ratio.denominator);
  return MediaPosition{quarters / 4U, static_cast<std::uint8_t>(quarters % 4U)};
}

// media frame -> analysis sample index, truncating. Exact only where the media
// frame lands on an analysis sample; callers that need the remainder use
// analysisToMedia in the other direction.
[[nodiscard]] constexpr std::uint64_t mediaToAnalysis(
    const std::uint64_t media_frame,
    const std::uint32_t analysis_rate_hz) noexcept {
  const MediaRatio ratio = mediaRatioFor(analysis_rate_hz);
  if (ratio.numerator == 0U) {
    return 0U;
  }
  return (media_frame * static_cast<std::uint64_t>(ratio.denominator)) /
         static_cast<std::uint64_t>(ratio.numerator);
}

// Whole media frames spanned by a run of analysis samples (e.g. a GDFT block).
[[nodiscard]] constexpr std::uint64_t analysisSpanToMediaFrames(
    const std::uint64_t analysis_samples,
    const std::uint32_t analysis_rate_hz) noexcept {
  const MediaRatio ratio = mediaRatioFor(analysis_rate_hz);
  if (ratio.numerator == 0U) {
    return 0U;
  }
  return (analysis_samples * static_cast<std::uint64_t>(ratio.numerator)) /
         static_cast<std::uint64_t>(ratio.denominator);
}

// Why a stream became discontinuous. Recorded so a consumer can tell an
// honest gap from a silent one.
enum class Discontinuity : std::uint8_t {
  kNone = 0U,
  kDmaOverrun = 1U,
  kClockError = 2U,
  kClockSourceSwitch = 3U,
  kSourceChange = 4U,
  kResamplerReset = 5U,
  kReset = 6U,
};

// THE CANONICAL COORDINATE. Deliberately pure: no flags, no metadata. Event
// metadata belongs to the frame descriptor or the feature record, never to a
// timestamp.
struct AudioTime final {
  std::uint64_t epoch_id = 0U;
  std::uint64_t frame_index = 0U;  // 48 kHz media frames, monotonic within epoch
};

[[nodiscard]] constexpr bool comparable(const AudioTime& a,
                                        const AudioTime& b) noexcept {
  return a.epoch_id == b.epoch_id;
}

inline constexpr std::uint64_t kNoEventEstimate =
    static_cast<std::uint64_t>(-1);

// Every derived analysis result carries its SOURCE WINDOW and, where the
// detector defines one, an EVENT ESTIMATE — plus when the CPU finished.
//
// There is deliberately no single "feature time". A universal fixed
// window-delay subtraction is PROHIBITED (REV2-D-021): the longest GDFT bin
// integrates over a 76.4 ms causal window whose 38.2 ms centroid is the
// effective time for STATIONARY content only, while a transient is detected on
// the newest data. Applying the centroid to an onset would be an error far
// larger than every clock term in the timebase lane combined.
struct AnalysisStamp final {
  std::uint64_t source_epoch = 0U;
  std::uint64_t window_start_frame = 0U;
  std::uint64_t window_centre_frame = 0U;
  std::uint64_t window_end_frame = 0U;
  std::uint64_t event_estimate_frame = kNoEventEstimate;
  std::uint64_t result_available_monotonic_us = 0U;
};

// Build a stamp for a causal analysis window that ENDS at `window_end` (the
// newest media frame in the window) and spans `block_size` analysis samples.
// Per-bin by construction: callers pass that bin's own block size.
[[nodiscard]] AnalysisStamp makeAnalysisStamp(
    const AudioTime& now, std::uint64_t window_end_media_frame,
    std::uint32_t block_size_analysis_samples, std::uint32_t analysis_rate_hz,
    std::uint64_t result_available_monotonic_us) noexcept;

// Single-writer media clock with a seqlock for 64-bit reads.
//
// Cortex-M7 has no 64-bit atomic load, so a torn read of a sample counter is a
// ~4.3-billion-frame timestamp error (24.9 hours of media time at 48 kHz) that
// would surface as a wild bug elsewhere. Readers retry on an odd or changed
// sequence.
//
// OWNERSHIP: advance() and the epoch calls are for the capture completion path
// ONLY. Nothing else writes.
class MediaClock final {
 public:
  void advance(std::uint64_t media_frames) noexcept;

  // A gap whose length was recovered: time really passed, the stream is intact,
  // so the epoch survives and the counter steps over the gap.
  void insertMeasuredGap(std::uint64_t media_frames) noexcept;

  // Continuity could not be proven. Opens a new epoch and restarts frame_index,
  // so a cross-epoch comparison is invalid by construction rather than
  // subtly wrong.
  void breakEpoch(Discontinuity reason) noexcept;

  [[nodiscard]] AudioTime load() const noexcept;
  [[nodiscard]] Discontinuity lastDiscontinuity() const noexcept {
    return last_discontinuity_;
  }
  [[nodiscard]] std::uint64_t measuredGapFrames() const noexcept {
    return measured_gap_frames_;
  }
  [[nodiscard]] bool gapSinceEpochStart() const noexcept {
    return measured_gap_frames_ != 0U;
  }

  // `first_epoch_id` lets a device continue epoch numbering across a reset from
  // non-volatile storage, so post-reset indices never collide with pre-reset
  // ones.
  void reset(std::uint64_t first_epoch_id = 0U) noexcept;

 private:
  std::atomic<std::uint32_t> sequence_{0U};
  std::uint64_t frame_index_ = 0U;
  std::uint64_t epoch_id_ = 0U;
  std::uint64_t measured_gap_frames_ = 0U;
  Discontinuity last_discontinuity_ = Discontinuity::kNone;
};

}  // namespace k1::core::audio
