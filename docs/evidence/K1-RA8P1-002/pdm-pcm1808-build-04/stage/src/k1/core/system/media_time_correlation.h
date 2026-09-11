#pragma once

#include <cstddef>
#include <cstdint>

#include "core/audio/media_time.h"

namespace k1::core::system {

inline constexpr std::size_t kMediaCorrelationCapacity = 32U;
inline constexpr std::size_t kMediaCorrelationMinSamples = 4U;
inline constexpr double kNominalMediaFrameUs = 1000000.0 / 48000.0;

// One observation joining the media/sample coordinate to a platform monotonic
// clock. `uncertainty_us` describes the quality of that observation; it is NOT
// silently subtracted as latency compensation.
struct MediaMonotonicObservation final {
  core::audio::AudioTime media{};
  std::uint64_t monotonic_us = 0U;
  std::uint32_t uncertainty_us = 0U;
};

struct MediaMonotonicMapping final {
  bool valid = false;
  std::uint64_t epoch_id = 0U;
  double us_per_media_frame = kNominalMediaFrameUs;
  double offset_us = 0.0;
  double rate_error_ppm = 0.0;
  double fit_rms_us = 0.0;
  std::uint32_t observation_uncertainty_us = 0U;
  std::size_t sample_count = 0U;
  std::uint64_t span_frames = 0U;
  std::uint64_t last_media_frame = 0U;
  float confidence = 0.0F;
};

// Platform-neutral local clock correlation:
//
//   monotonic_us ~= a * MEDIA_TIME_48K_frame + b
//
// RT1062 currently feeds hop publication timestamps; RA8P1 can later feed a
// DMA/SSIE hardware timestamp without changing any consumer.
class MediaTimeCorrelation final {
 public:
  void reset() noexcept;
  [[nodiscard]] bool addObservation(
      const MediaMonotonicObservation& observation) noexcept;

  [[nodiscard]] const MediaMonotonicMapping& mapping() const noexcept {
    return mapping_;
  }

  [[nodiscard]] bool mediaToMonotonic(
      std::uint64_t epoch_id, double media_frame,
      double& monotonic_us_out) const noexcept;
  [[nodiscard]] bool monotonicToMedia(
      std::uint64_t epoch_id, double monotonic_us,
      double& media_frame_out) const noexcept;
  [[nodiscard]] bool stale(std::uint64_t current_media_frame,
                           std::uint64_t max_age_frames) const noexcept;

 private:
  void refit() noexcept;

  MediaMonotonicObservation ring_[kMediaCorrelationCapacity]{};
  std::size_t count_ = 0U;
  std::size_t write_ = 0U;
  MediaMonotonicMapping mapping_{};
};

}  // namespace k1::core::system
