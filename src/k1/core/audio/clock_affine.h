#pragma once

// Affine mapping between two K1 media clocks.
//
//     N_peer  ~=  a * N_local + b
//
//         b  media-clock OFFSET, in frames
//         a  relative RATE, i.e. oscillator skew (a = 1 + ppm x 1e-6)
//
// Authority: research lane K1-AUDIO-TIMEBASE-R0 (REV2-D-023). That lane ruled a
// shared coordinate NECESSARY BUT NOT SUFFICIENT: offset estimation, rate/skew
// estimation and periodic resynchronisation are all required, and it measured
// two +/-20 ppm units diverging 14.4 ms over a six-minute track.
//
// TWO K1s AT "48 kHz" DO NOT ADVANCE AT THE SAME PHYSICAL RATE. Assuming a = 1
// is the error this class exists to prevent.
//
// TRANSPORT AGNOSTIC. This layer consumes paired observations and knows nothing
// about K1BR, BLE, a wired link or a bench harness. The exchange layer measures
// (local_frame, peer_frame, delay); the mathematics lives here.

#include <cstddef>
#include <cstdint>

namespace k1::core::audio {

inline constexpr std::size_t kClockObservationCapacity = 32U;

// One paired observation of the two clocks at a shared instant.
struct ClockObservation final {
  std::uint64_t local_epoch = 0U;
  std::uint64_t peer_epoch = 0U;
  std::uint64_t local_frame = 0U;   // local media frame at the shared instant
  std::uint64_t peer_frame = 0U;    // peer media frame at the same instant
  std::uint64_t delay_frames = 0U;  // transport delay estimate for this exchange
};

struct AffineMapping final {
  bool mapping_valid = false;
  double rate_ratio = 1.0;         // a
  double offset_frames = 0.0;      // b
  double skew_ppm = 0.0;           // (a - 1) x 1e6
  double fit_error_frames = 0.0;   // RMS residual over accepted observations
  std::size_t sample_count = 0U;   // observations accepted by the fit
  std::uint64_t last_update_local_frame = 0U;
  std::uint64_t span_frames = 0U;  // local-frame span the fit covers
  float confidence = 0.0F;
  std::uint64_t local_epoch = 0U;
  std::uint64_t peer_epoch = 0U;
};

class AffineClockEstimator final {
 public:
  // Minimum accepted observations before a mapping is offered at all. A single
  // pair cannot separate offset from skew, so refusing is the correct answer.
  static constexpr std::size_t kMinSamples = 4U;

  void reset() noexcept;

  // Returns false if the observation is rejected (epoch change resets the
  // estimator; a stale/!valid pair is dropped).
  bool addObservation(const ClockObservation& obs) noexcept;

  [[nodiscard]] const AffineMapping& mapping() const noexcept { return mapping_; }

  // peer -> local and local -> peer, with explicit epoch checks. Both return
  // false rather than guessing when the mapping is invalid or the epochs differ.
  [[nodiscard]] bool peerToLocal(std::uint64_t peer_epoch, double peer_frame,
                                 std::uint64_t local_epoch,
                                 double& local_frame_out) const noexcept;
  [[nodiscard]] bool localToPeer(std::uint64_t local_epoch, double local_frame,
                                 std::uint64_t peer_epoch,
                                 double& peer_frame_out) const noexcept;

  // A mapping ages: skew makes an old fit progressively wrong. The caller
  // decides the tolerance; this reports the elapsed local frames.
  [[nodiscard]] bool stale(std::uint64_t now_local_frame,
                           std::uint64_t max_age_frames) const noexcept;

 private:
  void refit() noexcept;

  ClockObservation ring_[kClockObservationCapacity]{};
  std::size_t count_ = 0U;
  std::size_t write_ = 0U;
  AffineMapping mapping_{};
};

}  // namespace k1::core::audio
