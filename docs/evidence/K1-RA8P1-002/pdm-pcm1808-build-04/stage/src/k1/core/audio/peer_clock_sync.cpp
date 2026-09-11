#include "core/audio/peer_clock_sync.h"

namespace k1::core::audio {

void PeerClockSync::reset() noexcept {
  estimator_.reset();
  pending_request_ = {};
  pending_ = false;
}

PeerClockSyncStatus PeerClockSync::beginExchange(
    const std::uint32_t sequence, const AudioTime& local_send,
    ClockSyncRequest& request) noexcept {
  if (pending_) return PeerClockSyncStatus::kAlreadyPending;
  pending_request_.sequence = sequence;
  pending_request_.requester_send = local_send;
  pending_ = true;
  request = pending_request_;
  return PeerClockSyncStatus::kAccepted;
}

PeerClockSyncStatus PeerClockSync::makeResponse(
    const ClockSyncRequest& request, const AudioTime& peer_receive,
    const AudioTime& peer_send, ClockSyncResponse& response) noexcept {
  if (peer_receive.epoch_id != peer_send.epoch_id) {
    return PeerClockSyncStatus::kPeerEpochMismatch;
  }
  if (peer_send.frame_index < peer_receive.frame_index) {
    return PeerClockSyncStatus::kPeerTimeReversed;
  }
  response.sequence = request.sequence;
  response.requester_send = request.requester_send;
  response.responder_receive = peer_receive;
  response.responder_send = peer_send;
  return PeerClockSyncStatus::kAccepted;
}

PeerClockSyncStatus PeerClockSync::acceptResponse(
    const ClockSyncResponse& response,
    const AudioTime& local_receive) noexcept {
  if (!pending_) return PeerClockSyncStatus::kNoPendingRequest;
  if (response.sequence != pending_request_.sequence) {
    return PeerClockSyncStatus::kSequenceMismatch;
  }
  if (response.requester_send.epoch_id != pending_request_.requester_send.epoch_id ||
      response.requester_send.frame_index != pending_request_.requester_send.frame_index) {
    return PeerClockSyncStatus::kRequestEchoMismatch;
  }
  if (local_receive.epoch_id != pending_request_.requester_send.epoch_id) {
    pending_ = false;
    estimator_.reset();
    return PeerClockSyncStatus::kLocalEpochChanged;
  }
  if (local_receive.frame_index < pending_request_.requester_send.frame_index) {
    pending_ = false;
    return PeerClockSyncStatus::kLocalTimeReversed;
  }

  ClockExchangeFourStamp exchange{};
  exchange.local_send = pending_request_.requester_send;
  exchange.peer_receive = response.responder_receive;
  exchange.peer_send = response.responder_send;
  exchange.local_receive = local_receive;
  pending_ = false;

  ClockObservation observation{};
  switch (makeClockObservation(exchange, observation)) {
    case ClockExchangeStatus::kAccepted:
      static_cast<void>(estimator_.addObservation(observation));
      return PeerClockSyncStatus::kAccepted;
    case ClockExchangeStatus::kLocalEpochMismatch:
      estimator_.reset();
      return PeerClockSyncStatus::kLocalEpochChanged;
    case ClockExchangeStatus::kPeerEpochMismatch:
      return PeerClockSyncStatus::kPeerEpochMismatch;
    case ClockExchangeStatus::kLocalTimeReversed:
      return PeerClockSyncStatus::kLocalTimeReversed;
    case ClockExchangeStatus::kPeerTimeReversed:
      return PeerClockSyncStatus::kPeerTimeReversed;
  }
  return PeerClockSyncStatus::kPeerTimeReversed;
}

}  // namespace k1::core::audio
