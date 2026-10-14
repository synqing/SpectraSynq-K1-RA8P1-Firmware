// DBS pilot step 1 regression (Titan_DBS_Translation.html L5). The old pre-AGC probe stamped a
// publication up to 100 us after capture, then read at the earlier capture time and forced a valid
// spectrum. This keeps one valid read and that future-publication failure.
#include "inhouse_audio.h"

#include <cmath>
#include <cstdio>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
  if (!ok) { std::printf("FAIL %s\n", what); ++failures; }
}
}  // namespace

int main() {
  static k1::titan::InhouseAudio audio;
  k1::titan::InhouseAudioConfig config{};
  config.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  check(audio.configure(config), "configure analysis24");
  k1::titan::InhouseAudioCursor cursor{};
  constexpr std::uint64_t kOriginUs = 1000000U;
  bool saw_valid = false, saw_future_reject = false;
  for (std::uint64_t seq = 1; seq <= 40U; ++seq) {
    k1_audio_hop_t hop{};
    hop.contract_version = 2U;
    hop.samples = k1::titan::kInhouseAnalysis24Hop;
    hop.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
    hop.stream_epoch = 1U;
    hop.hop_sequence = seq;
    hop.analysis_begin = (seq - 1U) * hop.samples;
    hop.analysis_end_exclusive = seq * hop.samples;
    for (std::size_t i = 0; i < hop.samples; ++i) {
      const double t = static_cast<double>(hop.analysis_begin + i) / 24000.0;
      hop.pcm[i] = static_cast<std::int16_t>(6000.0 * std::sin(2.0 * 3.14159265358979 * 440.0 * t));
    }
    const std::uint64_t capture_us = kOriginUs + hop.analysis_end_exclusive * 1000000ULL / 24000ULL;
    check(audio.consume(hop, capture_us, true), "consume accepted hop");
    const std::uint64_t publish_us = capture_us + 100U;
    audio.stampPublication(publish_us);
    k1::core::visual::InhouseWaveformAudio frame{};
    if (audio.latest().valid) {
      // The earlier failure: a read at capture time requests a future publication.
      k1::titan::InhouseAudioCursor early = cursor;
      const bool early_ok = audio.acquire(early, capture_us, 50000U, frame, 0);
      check(!early_ok && !frame.valid, "read before publication is rejected");
      saw_future_reject = saw_future_reject || (!early_ok && !frame.valid);
      // The repair: read at a simulated time at or after publication.
      const bool ok = audio.acquire(cursor, publish_us, 50000U, frame, 0);
      check(ok && frame.valid, "read at publication is accepted");
      check(frame.capture_us == capture_us, "capture time stays the event time");
      check(frame.publish_us == publish_us, "publication time recorded");
      saw_valid = saw_valid || (ok && frame.valid);
    } else {
      const bool ok = audio.acquire(cursor, publish_us, 50000U, frame, 0);
      check(!ok && !frame.valid, "warm-up publication is not valid");
    }
  }
  check(saw_valid, "at least one valid read");
  check(saw_future_reject, "future-publication read observed and rejected");
  std::printf(failures == 0 ? "PASS p6 onset chronology\n" : "FAIL p6 onset chronology (%d)\n", failures);
  return failures == 0 ? 0 : 1;
}
