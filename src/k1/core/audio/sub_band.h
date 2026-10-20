#pragma once

// Lane P6 (EXT release trigger): 40-110 Hz sub-band level of the analysis PCM. The in-house
// note bank starts at 110 Hz, so the kick/sub-bass of a drop is otherwise invisible. RBJ
// band-pass biquad (constant 0 dB peak gain) centred on sqrt(40 x 110) Hz with Q = f0 / 70 Hz.
// Independent of the AGC and every levelling stage: it reads the same hop PCM, writes nothing
// back. Two identical sections are cascaded (4th order) so 110-220 Hz bass harmonics do not
// leak in. Portable (float only, no heap).
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace k1::core::audio {

class SubBandDetector final {
 public:
  static constexpr float kLowHz = 40.0F;
  static constexpr float kHighHz = 110.0F;

  // Returns false for an unusable rate. Coefficients are computed once, here, never per hop.
  [[nodiscard]] bool configure(std::uint32_t rate_hz) noexcept {
    if (rate_hz < 1000U) return false;
#if defined(K1_SUBBAND_PLANT_WRONG_CENTRE)
    const float f0 = 660.0F;  // planted fault for the host test: band moved a decade up
#else
    const float f0 = std::sqrt(kLowHz * kHighHz);
#endif
    const float q = f0 / (kHighHz - kLowHz);
    const float w0 = 6.2831853071795864769F * f0 / static_cast<float>(rate_hz);
    const float alpha = std::sin(w0) / (2.0F * q);
    const float a0 = 1.0F + alpha;
    b0_ = alpha / a0;
    b2_ = -alpha / a0;
    a1_ = -2.0F * std::cos(w0) / a0;
    a2_ = (1.0F - alpha) / a0;
    reset();
    return true;
  }
  void reset() noexcept { z_[0] = z_[1] = z_[2] = z_[3] = 0.0F; }
  // RMS of the band-passed hop (full scale 1.0 = int16 32768). Non-finite state resets.
  [[nodiscard]] float process(const std::int16_t* pcm, std::size_t count) noexcept {
    if (pcm == nullptr || count == 0U) return 0.0F;
    float sum = 0.0F;
    for (std::size_t i = 0; i < count; ++i) {
      float y = static_cast<float>(pcm[i]) * (1.0F / 32768.0F);
      for (unsigned s = 0; s < 2U; ++s) {  // transposed direct form II, b1 = 0 for this band-pass
        const float x = y;
        y = b0_ * x + z_[2U * s];
        z_[2U * s] = -a1_ * y + z_[2U * s + 1U];
        z_[2U * s + 1U] = b2_ * x - a2_ * y;
      }
      sum += y * y;
    }
    if (!std::isfinite(sum) || !std::isfinite(z_[0]) || !std::isfinite(z_[2])) { reset(); return 0.0F; }
    return std::sqrt(sum / static_cast<float>(count));
  }

 private:
  float b0_ = 0.0F, b2_ = 0.0F, a1_ = 0.0F, a2_ = 0.0F;
  float z_[4] = {0.0F, 0.0F, 0.0F, 0.0F};
};

}  // namespace k1::core::audio
