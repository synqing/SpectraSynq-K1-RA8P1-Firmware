#include "core/audio/clock_affine.h"

#include <cmath>

namespace k1::core::audio {

void AffineClockEstimator::reset() noexcept {
  count_ = 0U;
  write_ = 0U;
  mapping_ = AffineMapping{};
}

bool AffineClockEstimator::addObservation(const ClockObservation& obs) noexcept {
  // An epoch change on either side invalidates every stored pair: frame indices
  // restart, so old observations describe a coordinate system that no longer
  // exists. Reset rather than fit across the discontinuity.
  if (count_ > 0U) {
    const ClockObservation& any = ring_[0];
    if (any.local_epoch != obs.local_epoch || any.peer_epoch != obs.peer_epoch) {
      reset();
    }
  }
  ring_[write_] = obs;
  write_ = (write_ + 1U) % kClockObservationCapacity;
  if (count_ < kClockObservationCapacity) {
    ++count_;
  }
  refit();
  return true;
}

void AffineClockEstimator::refit() noexcept {
  mapping_.mapping_valid = false;
  mapping_.sample_count = 0U;
  if (count_ < kMinSamples) {
    mapping_.confidence = 0.0F;
    return;
  }

  // Delay filtering by PERCENTILE, not by a multiple of the minimum.
  //
  // A "2x the best delay" rule is scale-dependent: on a link whose best exchange
  // is near zero it rejects almost everything and starves the fit. Measured on
  // the integrated test with 0-4 ms of transport jitter it kept 4 of 24
  // observations - exactly the minimum - which is one unlucky sample away from
  // refusing to map at all.
  //
  // Keeping the best PERCENTILE_KEEP fraction by delay is scale-free: the least
  // contaminated exchanges are still preferred, but the survivor count is
  // bounded below by construction.
  std::uint64_t sorted[kClockObservationCapacity];
  for (std::size_t i = 0U; i < count_; ++i) {
    sorted[i] = ring_[i].delay_frames;
  }
  for (std::size_t i = 1U; i < count_; ++i) {  // insertion sort, count_ <= 32
    const std::uint64_t key = sorted[i];
    std::size_t j = i;
    while (j > 0U && sorted[j - 1U] > key) {
      sorted[j] = sorted[j - 1U];
      --j;
    }
    sorted[j] = key;
  }
  // Keep the best ~60 %, never fewer than kMinSamples.
  std::size_t keep = (count_ * 3U) / 5U;
  if (keep < kMinSamples) {
    keep = kMinSamples;
  }
  if (keep > count_) {
    keep = count_;
  }
  const std::uint64_t threshold = sorted[keep - 1U];

  double sx = 0.0, sy = 0.0;
  std::size_t n = 0U;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].delay_frames <= threshold) {
      sx += static_cast<double>(ring_[i].local_frame);
      sy += static_cast<double>(ring_[i].peer_frame);
      ++n;
    }
  }
  if (n < kMinSamples) {
    mapping_.confidence = 0.0F;
    return;
  }
  const double mx = sx / static_cast<double>(n);
  const double my = sy / static_cast<double>(n);

  // Centred least squares. Frame indices reach 1e8+, so an uncentred normal
  // equation loses the skew term entirely in double rounding.
  double sxx = 0.0, sxy = 0.0;
  std::uint64_t lo = UINT64_MAX, hi = 0U, last = 0U;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].delay_frames > threshold) {
      continue;
    }
    const double dx = static_cast<double>(ring_[i].local_frame) - mx;
    const double dy = static_cast<double>(ring_[i].peer_frame) - my;
    sxx += dx * dx;
    sxy += dx * dy;
    if (ring_[i].local_frame < lo) lo = ring_[i].local_frame;
    if (ring_[i].local_frame > hi) hi = ring_[i].local_frame;
    if (ring_[i].local_frame > last) last = ring_[i].local_frame;
  }
  if (sxx <= 0.0) {
    // All observations at one instant: offset is knowable, skew is not.
    mapping_.confidence = 0.0F;
    return;
  }
  const double a = sxy / sxx;
  const double b = my - a * mx;

  double sse = 0.0;
  for (std::size_t i = 0U; i < count_; ++i) {
    if (ring_[i].delay_frames > threshold) {
      continue;
    }
    const double pred = a * static_cast<double>(ring_[i].local_frame) + b;
    const double r = static_cast<double>(ring_[i].peer_frame) - pred;
    sse += r * r;
  }
  const double rms = std::sqrt(sse / static_cast<double>(n));

  mapping_.mapping_valid = true;
  mapping_.rate_ratio = a;
  mapping_.offset_frames = b;
  mapping_.skew_ppm = (a - 1.0) * 1.0e6;
  mapping_.fit_error_frames = rms;
  mapping_.sample_count = n;
  mapping_.last_update_local_frame = last;
  mapping_.span_frames = (hi > lo) ? (hi - lo) : 0U;
  mapping_.local_epoch = ring_[0].local_epoch;
  mapping_.peer_epoch = ring_[0].peer_epoch;

  // Confidence rises with sample count and observation span, and falls with fit
  // error. A long span is what separates skew from offset.
  const double n_term = static_cast<double>(n) / static_cast<double>(kClockObservationCapacity);
  const double span_term =
      (mapping_.span_frames > 0U)
          ? (static_cast<double>(mapping_.span_frames) / (48000.0 * 10.0))
          : 0.0;
  const double err_term = 1.0 / (1.0 + rms);
  double c = n_term * 0.3 + (span_term > 1.0 ? 1.0 : span_term) * 0.4 + err_term * 0.3;
  if (c < 0.0) c = 0.0;
  if (c > 1.0) c = 1.0;
  mapping_.confidence = static_cast<float>(c);
}

bool AffineClockEstimator::peerToLocal(const std::uint64_t peer_epoch,
                                       const double peer_frame,
                                       const std::uint64_t local_epoch,
                                       double& local_frame_out) const noexcept {
  if (!mapping_.mapping_valid || mapping_.rate_ratio == 0.0) {
    return false;
  }
  if (peer_epoch != mapping_.peer_epoch || local_epoch != mapping_.local_epoch) {
    return false;
  }
  local_frame_out = (peer_frame - mapping_.offset_frames) / mapping_.rate_ratio;
  return true;
}

bool AffineClockEstimator::localToPeer(const std::uint64_t local_epoch,
                                       const double local_frame,
                                       const std::uint64_t peer_epoch,
                                       double& peer_frame_out) const noexcept {
  if (!mapping_.mapping_valid) {
    return false;
  }
  if (peer_epoch != mapping_.peer_epoch || local_epoch != mapping_.local_epoch) {
    return false;
  }
  peer_frame_out = mapping_.rate_ratio * local_frame + mapping_.offset_frames;
  return true;
}

bool AffineClockEstimator::stale(const std::uint64_t now_local_frame,
                                 const std::uint64_t max_age_frames) const noexcept {
  if (!mapping_.mapping_valid) {
    return true;
  }
  if (now_local_frame <= mapping_.last_update_local_frame) {
    return false;
  }
  return (now_local_frame - mapping_.last_update_local_frame) > max_age_frames;
}

}  // namespace k1::core::audio
