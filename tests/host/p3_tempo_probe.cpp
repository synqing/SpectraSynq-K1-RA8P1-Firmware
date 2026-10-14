// Lane P3 host probe: what the v8 Tempo board (mode 18) actually receives.
// Replays 40 kHz int16 mono PCM through the product ASRC -> InhouseAudio (its own
// tempo tracker, the source of mode 18's tempo) -> InhouseWaveformRenderer, and
// prints one CSV row per 100 Hz frame for both boards: tempo valid, BPM,
// confidence, phase, the mode-18 beat gate clamp01((confidence-0.30)/0.30)
// (inhouse_waveform.cpp:347), spectral energy, novelty and level inputs.
// Host oracle on the source file, not the room microphone; not M85 execution.
#include "inhouse_audio.h"
#include "core/visual/inhouse_waveform.h"
#include "k1_asrc_24k.h"
#include "pdm_target.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

using k1::core::visual::InhouseWaveformAudio;
using k1::core::visual::InhouseWaveformControls;
using k1::core::visual::InhouseWaveformRenderer;
using k1::core::visual::Pixel16;

int main(int argc, char** argv) {
  if (argc != 2) { std::fprintf(stderr, "usage: p3_tempo_probe in.s16 > out.csv\n"); return 2; }
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);

  constexpr unsigned kSlot = 296U;
  constexpr std::uint64_t kHopUs = 5000U;
  static k1_asrc24_t asrc;
  k1_asrc24_reset(&asrc, 1U, 40000U);
  static k1::titan::InhouseAudio audio;
  audio.reset();
  if (!audio.configure(k1::titan::InhouseAudioConfig{})) return 5;
  audio.setTempoDemand(0, true);
  audio.setTempoDemand(1, true);
  static InhouseWaveformRenderer renderer[2];
  renderer[0].reset();
  renderer[1].reset();
  InhouseWaveformControls controls[2];
  controls[0].mode = 32; controls[0].palette_id = 32;
  controls[1].mode = 18; controls[1].palette_id = 39;
  k1::titan::InhouseAudioCursor cursor[2]{};
  static Pixel16 frame[2][160];

  std::printf("t_s,board,acquired,tempo_valid,tempo_bpm,tempo_confidence,tempo_phase01,beat_gate,"
              "spectral_energy,novelty,peak_scaled,vu_level,silence,mean_rgb16\n");
  std::uint64_t hop_seq = 0;
  for (std::size_t at = 0; at + kSlot <= pcm.size(); at += kSlot) {
    if (k1_asrc24_push(&asrc, &pcm[at], kSlot) != k1_asrc24_ok) return 6;
    for (;;) {
      k1_audio_hop_t hop{};
      if (asrc.filled < k1_asrc24_required_for_pull180(&asrc)) break;
      hop.contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;
      hop.samples = K1_ASRC24_OUT_FRAMES;
      hop.analysis_rate_hz = K1_ASRC24_OUT_HZ;
      hop.stream_epoch = 1U;
      hop.hop_sequence = hop_seq + 1U;
      hop.analysis_begin = hop_seq * K1_ASRC24_OUT_FRAMES;
      hop.analysis_end_exclusive = hop.analysis_begin + K1_ASRC24_OUT_FRAMES;
      if (k1_asrc24_pull180(&asrc, hop.pcm) != k1_asrc24_ok) break;
      ++hop_seq;
      const std::uint64_t now = hop_seq * kHopUs;
      hop.capture_estimate_valid = 1;
      hop.newest_sample_capture_estimate_us = now;
      (void)audio.consume(hop, now, true);
      audio.stampPublication(now + 100U);
      if (hop_seq % 2U) continue;
      for (unsigned b = 0; b < 2; ++b) {
        InhouseWaveformAudio a{};
        const bool got = audio.acquire(cursor[b], now + 200U, 50000U, a, b);
        (void)renderer[b].render(a, controls[b], now + 200U, frame[b], 160);
        const float gate = a.tempo_valid
            ? std::clamp((a.tempo_confidence - 0.30F) / 0.30F, 0.0F, 1.0F) : 0.0F;
        double sum = 0.0;
        for (unsigned p = 0; p < 160; ++p) sum += frame[b][p].red + frame[b][p].green + frame[b][p].blue;
        std::printf("%.3f,%u,%d,%d,%.3f,%.4f,%.4f,%.4f,%.5f,%.5f,%.5f,%.5f,%d,%.2f\n",
                    now / 1e6, b, got ? 1 : 0, a.tempo_valid ? 1 : 0, a.tempo_bpm, a.tempo_confidence,
                    a.tempo_phase01, gate, a.spectral_energy, a.novelty, a.peak_scaled, a.vu_level,
                    a.silence ? 1 : 0, sum / 480.0);
      }
    }
  }
  return 0;
}
