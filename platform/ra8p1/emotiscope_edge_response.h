#ifndef K1_EMOTISCOPE_EDGE_RESPONSE_H
#define K1_EMOTISCOPE_EDGE_RESPONSE_H

#include "emotiscope_port.h"
#include <cmath>
#include <cstdint>

// EdgeMixer VALUE formula adapted to donor novelty/spectrum, not legacy STM
// parity. One instance per visual context; shared publication is never mutated.
// Caller owns publication freshness and sets valid=0 on unavailable/rewarming
// audio. Include outside the donor namespace. No allocation or hardware access.
class EmotiscopeEdgeResponse {
 public:
  EmotiscopeEdgeResponse() noexcept { initialise_bins(); }

  bool configure(std::uint32_t mode, float depth) noexcept {
    if (mode > 2u || !std::isfinite(depth) || depth < 0.0f || depth > 1.0f)
      return false;
    if (mode_ != mode) reset();
    mode_ = mode;
    depth_ = depth;
    return true;
  }
  std::uint32_t mode() const noexcept { return mode_; }
  float depth() const noexcept { return depth_; }
  void reset() noexcept {
    for (float& value : envelope_) value = 0.0f;
  }
  void initialise_bins() noexcept {
    static_assert(K1_EMOTISCOPE_PIXELS == 128 && K1_EMOTISCOPE_FREQS == 64,
                  "Edge response geometry must match the donor");
    for (unsigned r = 0; r < 64; ++r)
      bins_[r] = static_cast<std::uint8_t>(
          63.0f * std::pow((static_cast<float>(r) + 0.5f) / 63.5f, 0.7f));
  }

  // RGB is interleaved linear float data for all128 native donor pixels.
  // False means invalid observation/time or null output: RGB is untouched and
  // response history resets. Valid OFF/depth0 is byte-identical pass-through.
  bool apply(float rgb[384], const K1EmotiscopeAudioPublication& audio,
             float dt_seconds) noexcept {
    if (!rgb || !audio.valid || !std::isfinite(dt_seconds) || dt_seconds < 0.0f) {
      reset();
      return false;
    }
    if (mode_ == 0u || depth_ == 0.0f) return true;
    if (!std::isfinite(audio.vu_level) ||
        (mode_ == 1u && !std::isfinite(audio.novelty_normalised))) {
      reset();
      return false;
    }
    // Preflight every consumed bin before changing any history or pixel.
    if (mode_ == 2u) {
      for (unsigned r = 0; r < 64; ++r) {
        if (!std::isfinite(audio.spectrum_smooth[bins_[r]])) {
          reset();
          return false;
        }
      }
    }
    const float alpha = dt_seconds > 0.0f
        ? 1.0f - std::exp(-dt_seconds / 0.040f) : 0.0f;
    const float loud = clamp01(audio.vu_level);
    for (unsigned r = 0; r < 64; ++r) {
      const float energy = clamp01(mode_ == 1u ? audio.novelty_normalised
                                             : audio.spectrum_smooth[bins_[r]]);
      envelope_[r] += (energy - envelope_[r]) * alpha;
      const float gain = 1.0f - depth_ * loud * (1.0f - envelope_[r]);
      const unsigned left = 3u * (63u - r), right = 3u * (64u + r);
      for (unsigned colour = 0; colour < 3; ++colour) {
        rgb[left + colour] *= gain;
        rgb[right + colour] *= gain;
      }
    }
    return true;
  }

 private:
  static float clamp01(float value) noexcept {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
  }
  std::uint32_t mode_ = 0;
  float depth_ = 0.0f;
  float envelope_[64]{};
  std::uint8_t bins_[64]{};
};

#endif
