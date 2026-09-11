#include "core/system/media_time_correlation.h"

#include <cmath>

namespace k1::core::system {

void MediaTimeCorrelation::reset() noexcept {
  count_ = 0U;
  write_ = 0U;
  mapping_ = MediaMonotonicMapping{};
}

bool MediaTimeCorrelation::addObservation(
    const MediaMonotonicObservation& observation) noexcept {
  if (count_ > 0U && ring_[0].media.epoch_id != observation.media.epoch_id) {
    reset();
  }
  ring_[write_] = observation;
  write_ = (write_ + 1U) % kMediaCorrelationCapacity;
  if (count_ < kMediaCorrelationCapacity) ++count_;
  refit();
  return true;
}

void MediaTimeCorrelation::refit() noexcept {
  mapping_.valid = false;
  mapping_.sample_count = 0U;
  mapping_.confidence = 0.0F;
  if (count_ < kMediaCorrelationMinSamples) return;

  // Keep the best ~75% by declared timestamp uncertainty. Hardware capture
  // timestamps naturally win over foreground/dequeue proxies when both exist.
  std::uint32_t sorted[kMediaCorrelationCapacity]{};
  for (std::size_t i = 0U; i < count_; ++i) {
    sorted[i] = ring_[i].uncertainty_us;
  }
  for (std::size_t i = 1U; i < count_; ++i) {
    const std::uint32_t key = sorted[i];
    std::size_t j = i;
    while (j > 0U && sorted[j - 1U] > key) {
      sorted[j] = sorted[j - 1U];
      --j;
    }
    sorted[j] = key;
  }
  std::size_t keep = (count_ * 3U) / 4U;
  if (keep < kMediaCorrelationMinSamples) keep = kMediaCorrelationMinSamples;
  if (keep > count_) keep = count_;
  const std::uint32_t threshold = sorted[keep - 1U];

  double sx = 0.0;
  double sy = 0.0;
  std::size_t n = 0U;
  std::uint32_t worst_uncertainty = 0U;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].uncertainty_us > threshold) continue;
    sx += static_cast<double>(ring_[i].media.frame_index);
    sy += static_cast<double>(ring_[i].monotonic_us);
    if (ring_[i].uncertainty_us > worst_uncertainty) {
      worst_uncertainty = ring_[i].uncertainty_us;
    }
    ++n;
  }
  if (n < kMediaCorrelationMinSamples) return;
  const double mx = sx / static_cast<double>(n);
  const double my = sy / static_cast<double>(n);

  double sxx = 0.0;
  double sxy = 0.0;
  std::uint64_t lo = UINT64_MAX;
  std::uint64_t hi = 0U;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].uncertainty_us > threshold) continue;
    const double dx = static_cast<double>(ring_[i].media.frame_index) - mx;
    const double dy = static_cast<double>(ring_[i].monotonic_us) - my;
    sxx += dx * dx;
    sxy += dx * dy;
    if (ring_[i].media.frame_index < lo) lo = ring_[i].media.frame_index;
    if (ring_[i].media.frame_index > hi) hi = ring_[i].media.frame_index;
  }
  if (sxx <= 0.0) return;
  const double a = sxy / sxx;
  const double b = my - a * mx;

  // A local audio clock outside +/-2% of nominal means the observations are not
  // describing the claimed MEDIA_TIME_48K coordinate. Fail closed.
  const double ratio = a / kNominalMediaFrameUs;
  if (!std::isfinite(a) || !std::isfinite(b) || a <= 0.0 ||
      ratio < 0.98 || ratio > 1.02) {
    return;
  }

  double sse = 0.0;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].uncertainty_us > threshold) continue;
    const double predicted =
        a * static_cast<double>(ring_[i].media.frame_index) + b;
    const double residual = static_cast<double>(ring_[i].monotonic_us) - predicted;
    sse += residual * residual;
  }
  const double rms = std::sqrt(sse / static_cast<double>(n));
  const std::uint64_t span = hi > lo ? hi - lo : 0U;

  mapping_.valid = true;
  mapping_.epoch_id = ring_[0].media.epoch_id;
  mapping_.us_per_media_frame = a;
  mapping_.offset_us = b;
  mapping_.rate_error_ppm = (ratio - 1.0) * 1.0e6;
  mapping_.fit_rms_us = rms;
  mapping_.observation_uncertainty_us = worst_uncertainty;
  mapping_.sample_count = n;
  mapping_.span_frames = span;
  mapping_.last_media_frame = hi;

  const double sample_term =
      static_cast<double>(n) / static_cast<double>(kMediaCorrelationCapacity);
  const double span_term =
      static_cast<double>(span) / (48000.0 * 1.0);  // saturates after 1 s
  const double error_term = 1.0 / (1.0 + rms / 250.0);
  const double uncertainty_term =
      1.0 / (1.0 + static_cast<double>(worst_uncertainty) / 1000.0);
  double confidence = 0.20 * sample_term + 0.30 * (span_term > 1.0 ? 1.0 : span_term) +
                      0.25 * error_term + 0.25 * uncertainty_term;
  if (confidence < 0.0) confidence = 0.0;
  if (confidence > 1.0) confidence = 1.0;
  mapping_.confidence = static_cast<float>(confidence);
}

bool MediaTimeCorrelation::mediaToMonotonic(
    const std::uint64_t epoch_id, const double media_frame,
    double& monotonic_us_out) const noexcept {
  if (!mapping_.valid || mapping_.epoch_id != epoch_id ||
      !std::isfinite(media_frame) || media_frame < 0.0) {
    return false;
  }
  monotonic_us_out =
      mapping_.us_per_media_frame * media_frame + mapping_.offset_us;
  return std::isfinite(monotonic_us_out);
}

bool MediaTimeCorrelation::monotonicToMedia(
    const std::uint64_t epoch_id, const double monotonic_us,
    double& media_frame_out) const noexcept {
  if (!mapping_.valid || mapping_.epoch_id != epoch_id ||
      !std::isfinite(monotonic_us) || mapping_.us_per_media_frame <= 0.0) {
    return false;
  }
  media_frame_out =
      (monotonic_us - mapping_.offset_us) / mapping_.us_per_media_frame;
  return std::isfinite(media_frame_out);
}

bool MediaTimeCorrelation::stale(const std::uint64_t current_media_frame,
                                 const std::uint64_t max_age_frames) const noexcept {
  if (!mapping_.valid) return true;
  if (current_media_frame <= mapping_.last_media_frame) return false;
  return current_media_frame - mapping_.last_media_frame > max_age_frames;
}

}  // namespace k1::core::system
