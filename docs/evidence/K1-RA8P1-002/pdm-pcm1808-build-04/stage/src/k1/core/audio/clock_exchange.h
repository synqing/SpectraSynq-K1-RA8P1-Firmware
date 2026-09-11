#pragma once

// Transport-neutral exchange seam for multi-K1 AUDIO_TIME synchronisation.
//
// A transport (BLE, wired K1 link, bench harness, future network) captures four
// media-clock timestamps around one request/response exchange. This layer turns
// them into the paired observation consumed by AffineClockEstimator. It does not
// know packet formats, radios, queues, or bridge semantics.

#include <cstdint>

#include "core/audio/clock_affine.h"
#include "core/audio/media_time.h"
#include "core/audio/musical_time.h"

namespace k1::core::audio {

struct ClockExchangeFourStamp final {
  AudioTime local_send{};     // t0: request leaves local K1
  AudioTime peer_receive{};   // t1: request reaches peer K1
  AudioTime peer_send{};      // t2: response leaves peer K1
  AudioTime local_receive{};  // t3: response reaches local K1
};

enum class ClockExchangeStatus : std::uint8_t {
  kAccepted = 0U,
  kLocalEpochMismatch,
  kPeerEpochMismatch,
  kLocalTimeReversed,
  kPeerTimeReversed,
};

// Convert a four-stamp exchange into one approximate shared-instant pair. The
// two midpoints are the standard symmetric-path estimate. Integer frame
// coordinates introduce at most 0.5 frame of midpoint quantisation; path
// asymmetry remains measurement noise and is handled by repeated observations
// plus the affine estimator's low-delay selection.
[[nodiscard]] ClockExchangeStatus makeClockObservation(
    const ClockExchangeFourStamp& exchange,
    ClockObservation& observation) noexcept;

struct LocalBeatTarget final {
  std::uint64_t epoch_id = 0U;
  FramePositionQ32 event_frame{};
  std::uint64_t beat_index = 0U;
  float beat_confidence = 0.0F;
  float clock_confidence = 0.0F;
  double mapping_error_frames = 0.0;
};

// Convert a peer/leader beat prediction directly into the follower's local
// MEDIA_TIME coordinate. The output is a target instant, not a transported
// "beat happened" event; the renderer can schedule ahead against event_frame.
[[nodiscard]] bool mapPeerBeatToLocal(
    const AffineClockEstimator& estimator,
    const BeatEvent& peer_beat,
    std::uint64_t local_epoch,
    LocalBeatTarget& target) noexcept;

}  // namespace k1::core::audio
