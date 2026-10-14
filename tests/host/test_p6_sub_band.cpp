// Lane P6: SubBandDetector (src/k1/core/audio/sub_band.h) host test. Exit 0 only if the 40-110 Hz
// band passes 66 Hz near unity and rejects 1 kHz and 220 Hz at both analysis rates. Built with
// K1_SUBBAND_PLANT_WRONG_CENTRE it must fail (tests/host/test_p6_sub_band.py checks both).
#include "core/audio/sub_band.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
float rms_of_tone(std::uint32_t rate, float hz, float amplitude) {
  k1::core::audio::SubBandDetector d;
  if (!d.configure(rate)) return -1.0F;
  std::vector<std::int16_t> pcm(rate * 2U);
  for (std::size_t i = 0; i < pcm.size(); ++i)
    pcm[i] = static_cast<std::int16_t>(std::lround(32767.0 * amplitude * std::sin(6.283185307179586 * hz * i / rate)));
  const std::size_t hop = rate == 24000U ? 180U : 96U;
  float last = 0.0F;
  for (std::size_t at = 0; at + hop <= pcm.size(); at += hop) last = d.process(&pcm[at], hop);  // settled by 2 s
  return last;
}
}  // namespace

int main() {
  int failures = 0;
  for (std::uint32_t rate : {24000U, 12800U}) {
    const float in = rms_of_tone(rate, 66.3F, 0.5F), ref = 0.5F / std::sqrt(2.0F);
    const float hi = rms_of_tone(rate, 1000.0F, 0.5F), mid = rms_of_tone(rate, 220.0F, 0.5F);
    // Fourth order (two sections): 66 Hz within 10%, 220 Hz at least 12 dB down, 1 kHz at least 40 dB down.
    const bool ok = in > 0.9F * ref && in < 1.1F * ref && hi < 0.01F * ref && mid < 0.25F * ref;
    std::printf("rate %u: 66 Hz %.4f (ref %.4f) 220 Hz %.4f 1 kHz %.4f -> %s\n", rate, in, ref, mid, hi, ok ? "ok" : "FAIL");
    failures += ok ? 0 : 1;
  }
  k1::core::audio::SubBandDetector d;
  if (d.configure(500U)) { std::printf("accepted an unusable rate\n"); ++failures; }
  if (d.process(nullptr, 10U) != 0.0F) { std::printf("null input not zero\n"); ++failures; }
  return failures == 0 ? 0 : 1;
}
