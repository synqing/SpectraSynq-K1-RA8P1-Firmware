#include "core/audio/clock_exchange.h"

#include <cmath>
#include <limits>

namespace k1::core::audio {
namespace {

std::uint64_t midpoint(const std::uint64_t a,
                       const std::uint64_t b) noexcept {
  return a + (b - a) / 2U;
}

bool doubleToFramePosition(const double value,
                           FramePositionQ32& out) noexcept {
  if (!std::isfinite(value) || value < 0.0 ||
      value > static_cast<double>(UINT64_MAX)) {
    return false;
  }
  double whole = 0.0;
  const double fraction = std::modf(value, &whole);
  std::uint64_t frame = static_cast<std::uint64_t>(whole);
  const double scaled = std::ldexp(fraction, 32);
  std::uint64_t frac = static_cast<std::uint64_t>(scaled + 0.5);
  if (frac >= (1ULL << 32U)) {
    if (frame == UINT64_MAX) return false;
    ++frame;
    frac = 0U;
  }
  out.frame = frame;
  out.frac_q32 = static_cast<std::uint32_t>(frac);
  return true;
}

}  // namespace

ClockExchangeStatus makeClockObservation(
    const ClockExchangeFourStamp& exchange,
    ClockObservation& observation) noexcept {
  if (exchange.local_send.epoch_id != exchange.local_receive.epoch_id) {
    return ClockExchangeStatus::kLocalEpochMismatch;
  }
  if (exchange.peer_receive.epoch_id != exchange.peer_send.epoch_id) {
    return ClockExchangeStatus::kPeerEpochMismatch;
  }
  if (exchange.local_receive.frame_index < exchange.local_send.frame_index) {
    return ClockExchangeStatus::kLocalTimeReversed;
  }
  if (exchange.peer_send.frame_index < exchange.peer_receive.frame_index) {
    return ClockExchangeStatus::kPeerTimeReversed;
  }

  const std::uint64_t local_rtt =
      exchange.local_receive.frame_index - exchange.local_send.frame_index;

  observation = {};
  observation.local_epoch = exchange.local_send.epoch_id;
  observation.peer_epoch = exchange.peer_receive.epoch_id;
  observation.local_frame = midpoint(exchange.local_send.frame_index,
                                     exchange.local_receive.frame_index);
  observation.peer_frame = midpoint(exchange.peer_receive.frame_index,
                                    exchange.peer_send.frame_index);
  // Use the measured local round trip as the contamination score. Peer service
  // time is intentionally not subtracted: clocks have different rates, and the
  // percentile filter only needs a monotonic quality proxy, not one-way delay.
  observation.delay_frames = local_rtt;
  return ClockExchangeStatus::kAccepted;
}

bool mapPeerBeatToLocal(const AffineClockEstimator& estimator,
                        const BeatEvent& peer_beat,
                        const std::uint64_t local_epoch,
                        LocalBeatTarget& target) noexcept {
  double local_frame = 0.0;
  if (!estimator.peerToLocal(peer_beat.epoch_id,
                             peer_beat.beat_event_frame.toDouble(),
                             local_epoch, local_frame)) {
    return false;
  }

  FramePositionQ32 mapped{};
  if (!doubleToFramePosition(local_frame, mapped)) return false;

  target = {};
  target.epoch_id = local_epoch;
  target.event_frame = mapped;
  target.beat_index = peer_beat.beat_index;
  target.beat_confidence = peer_beat.confidence;
  target.clock_confidence = estimator.mapping().confidence;
  target.mapping_error_frames = estimator.mapping().fit_error_frames;
  return true;
}

}  // namespace k1::core::audio
