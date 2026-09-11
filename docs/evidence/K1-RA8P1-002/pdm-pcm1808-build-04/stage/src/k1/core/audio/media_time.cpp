#include "core/audio/media_time.h"

namespace k1::core::audio {

AnalysisStamp makeAnalysisStamp(
    const AudioTime& now, const std::uint64_t window_end_media_frame,
    const std::uint32_t block_size_analysis_samples,
    const std::uint32_t analysis_rate_hz,
    const std::uint64_t result_available_monotonic_us) noexcept {
  AnalysisStamp stamp{};
  stamp.source_epoch = now.epoch_id;
  stamp.window_end_frame = window_end_media_frame;
  stamp.result_available_monotonic_us = result_available_monotonic_us;
  stamp.event_estimate_frame = kNoEventEstimate;

  const std::uint64_t span = analysisSpanToMediaFrames(
      block_size_analysis_samples, analysis_rate_hz);
  stamp.window_start_frame =
      span > window_end_media_frame ? 0U : window_end_media_frame - span;
  const std::uint64_t half = span / 2U;
  stamp.window_centre_frame =
      half > window_end_media_frame ? 0U : window_end_media_frame - half;
  return stamp;
}

void MediaClock::advance(const std::uint64_t media_frames) noexcept {
  if (media_frames == 0U) {
    return;
  }
  const std::uint32_t start = sequence_.load(std::memory_order_relaxed);
  sequence_.store(start + 1U, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
  frame_index_ += media_frames;
  std::atomic_thread_fence(std::memory_order_release);
  sequence_.store(start + 2U, std::memory_order_relaxed);
}

void MediaClock::insertMeasuredGap(const std::uint64_t media_frames) noexcept {
  if (media_frames == 0U) {
    return;
  }
  const std::uint32_t start = sequence_.load(std::memory_order_relaxed);
  sequence_.store(start + 1U, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
  frame_index_ += media_frames;
  measured_gap_frames_ += media_frames;
  std::atomic_thread_fence(std::memory_order_release);
  sequence_.store(start + 2U, std::memory_order_relaxed);
}

void MediaClock::breakEpoch(const Discontinuity reason) noexcept {
  const std::uint32_t start = sequence_.load(std::memory_order_relaxed);
  sequence_.store(start + 1U, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
  ++epoch_id_;
  frame_index_ = 0U;
  measured_gap_frames_ = 0U;
  last_discontinuity_ = reason;
  std::atomic_thread_fence(std::memory_order_release);
  sequence_.store(start + 2U, std::memory_order_relaxed);
}

AudioTime MediaClock::load() const noexcept {
  AudioTime value{};
  for (;;) {
    const std::uint32_t before = sequence_.load(std::memory_order_relaxed);
    if ((before & 1U) != 0U) {
      continue;  // writer in progress
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    value.epoch_id = epoch_id_;
    value.frame_index = frame_index_;
    std::atomic_thread_fence(std::memory_order_acquire);
    if (sequence_.load(std::memory_order_relaxed) == before) {
      return value;
    }
  }
}

void MediaClock::reset(const std::uint64_t first_epoch_id) noexcept {
  const std::uint32_t start = sequence_.load(std::memory_order_relaxed);
  sequence_.store(start + 1U, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
  frame_index_ = 0U;
  epoch_id_ = first_epoch_id;
  measured_gap_frames_ = 0U;
  last_discontinuity_ = Discontinuity::kReset;
  std::atomic_thread_fence(std::memory_order_release);
  sequence_.store(start + 2U, std::memory_order_relaxed);
}

}  // namespace k1::core::audio
