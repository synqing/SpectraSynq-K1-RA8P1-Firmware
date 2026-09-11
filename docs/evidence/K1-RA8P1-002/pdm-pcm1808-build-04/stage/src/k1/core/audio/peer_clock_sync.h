#pragma once

// K1-DM-141 transport-neutral peer clock synchronisation session.
//
// The transport only carries request/response fields. This class owns the
// exchange lifecycle, produces four timestamps, feeds AffineClockEstimator, and
// maps leader beat predictions into the follower's local MEDIA_TIME domain.

#include <cstdint>

#include "core/audio/clock_exchange.h"

namespace k1::core::audio {

struct ClockSyncRequest final {
  std::uint32_t sequence = 0U;
  AudioTime requester_send{};  // t0, echoed by the responder
};

struct ClockSyncResponse final {
  std::uint32_t sequence = 0U;
  AudioTime requester_send{};  // echoed t0 binds response to one request
  AudioTime responder_receive{};  // t1
  AudioTime responder_send{};     // t2
};

enum class PeerClockSyncStatus : std::uint8_t {
  kAccepted = 0U,
  kAlreadyPending,
  kNoPendingRequest,
  kSequenceMismatch,
  kRequestEchoMismatch,
  kLocalEpochChanged,
  kLocalTimeReversed,
  kPeerEpochMismatch,
  kPeerTimeReversed,
};

class PeerClockSync final {
 public:
  void reset() noexcept;

  // One exchange at a time. `local_send` must be captured immediately before
  // handing the request to the transport; overlapping requests are refused so
  // a late response can never be paired with the wrong t0.
  [[nodiscard]] PeerClockSyncStatus beginExchange(
      std::uint32_t sequence, const AudioTime& local_send,
      ClockSyncRequest& request) noexcept;

  // Responder-side helper. receive/send are captured in the responder's own
  // MEDIA_TIME epoch. This is pure construction; no estimator state is touched.
  [[nodiscard]] static PeerClockSyncStatus makeResponse(
      const ClockSyncRequest& request, const AudioTime& peer_receive,
      const AudioTime& peer_send, ClockSyncResponse& response) noexcept;

  // Requester records t3 on receipt, validates the response against the pending
  // request, then contributes one observation to the rolling affine fit.
  [[nodiscard]] PeerClockSyncStatus acceptResponse(
      const ClockSyncResponse& response, const AudioTime& local_receive) noexcept;

  [[nodiscard]] bool requestPending() const noexcept { return pending_; }
  [[nodiscard]] const AffineMapping& mapping() const noexcept {
    return estimator_.mapping();
  }
  [[nodiscard]] const AffineClockEstimator& estimator() const noexcept {
    return estimator_;
  }

  [[nodiscard]] bool mapPeerBeat(const BeatEvent& peer_beat,
                                 std::uint64_t local_epoch,
                                 LocalBeatTarget& target) const noexcept {
    return mapPeerBeatToLocal(estimator_, peer_beat, local_epoch, target);
  }

 private:
  AffineClockEstimator estimator_{};
  ClockSyncRequest pending_request_{};
  bool pending_ = false;
};

}  // namespace k1::core::audio
