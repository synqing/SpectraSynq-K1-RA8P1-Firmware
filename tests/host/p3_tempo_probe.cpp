// Lane P3 host replay of the two v8 effects (the existing v8 replay entry point;
// used for the P3 tempo replay and the AGC trace).
// 40 kHz int16 mono PCM -> product ASRC -> InhouseAudio (in-house AGC, GDFT, tempo)
// -> InhouseWaveformRenderer with the v8 controls (lower mode 32 palette 32,
// upper mode 18 palette 39; renderInhouse passes palette-1 as the id, speed 0.5
// -> motion 1.0, mirror on) -> renderInhouse's display stage (brightness 1.0,
// shoulder, warmth 0.591 / 0.0, white balance) -> output intensity 0.25.
// Prints one CSV row per 100 Hz frame per board with every input the effect
// reads, its internal state, and the payload luma. Host oracle: no donor
// admission gate, no response-gain changes (v8 response_gain = 1.0), not M85.
#include "inhouse_audio.h"
#include "core/visual/inhouse_waveform.h"
#include "k1_asrc_24k.h"
#include "pdm_target.h"

#include <algorithm>
#include <cmath>
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
  const float warmth[2] = {0.5905512F, 0.0F};
  for (unsigned b = 0; b < 2; ++b) { controls[b].motion_scale = 0.5F * 2.0F; controls[b].mirror = true; }
  controls[0].mode = 32; controls[0].palette_id = 32 - 1;
  controls[1].mode = 18; controls[1].palette_id = 39 - 1;
  k1::titan::InhouseAudioCursor cursor[2]{};
  static Pixel16 frame[2][160];

  std::printf("t_s,board,acquired,fresh,peak_scaled,observed_peak_scaled,peak_summary_valid,vu_level,vu_average,"
              "raw_rms,raw_peak_pcm,silence,spectral_energy,novelty,tempo_valid,tempo_bpm,tempo_confidence,"
              "tempo_phase01,beat_gate");
  for (int i = 0; i < 12; ++i) std::printf(",chroma%d", i);
  std::printf(",chroma_pregate_max,palette_coordinate,held_coordinate,peak_stage1,peak_stage2,position,"
              "decay_rate,deposit_strength,dot_r,dot_g,dot_b,out_r,out_g,out_b,luma,lit_fraction\n");
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
        const auto& o = renderer[b].observation();
        float gate = a.tempo_valid ? std::clamp((a.tempo_confidence - 0.30F) / 0.30F, 0.0F, 1.0F) : 0.0F;
        gate = gate * gate * (3.0F - 2.0F * gate);
        // renderInhouse display stage (palette_runtime.cpp), then output intensity 0.25.
        const float warm[3] = {1.0F, 1.0F - warmth[b] + warmth[b] * std::sqrt(.1982F),
                               1.0F - warmth[b] + warmth[b] * std::sqrt(.0244F)};
        const float white[3] = {1.0F, .9375F, .84F};
        double sum[3] = {0, 0, 0}, luma = 0;
        unsigned lit = 0;
        for (unsigned p = 0; p < 160; ++p) {
          float c[3] = {frame[b][p].red / 65535.0F, frame[b][p].green / 65535.0F, frame[b][p].blue / 65535.0F};
          const float m = std::max(c[0], std::max(c[1], c[2]));
          const float sh = m > .75F ? (.75F + .25F * std::tanh((m - .75F) * 4.0F)) / m : 1.0F;
          double px[3];
          for (unsigned k = 0; k < 3; ++k) {
            px[k] = std::clamp(c[k] * sh * warm[k] * white[k], 0.0F, 1.0F) * 0.25;
            sum[k] += px[k];
          }
          const double l = 0.2126 * px[0] + 0.7152 * px[1] + 0.0722 * px[2];
          luma += l;
          lit += l > 0.01 ? 1U : 0U;
        }
        float pre = 0.0F;
        for (float v : o.chroma_pregate) pre = std::max(pre, v);
        std::printf("%.3f,%u,%d,%d,%.5f,%.5f,%d,%.5f,%.5f,%.5f,%.1f,%d,%.5f,%.5f,%d,%.2f,%.4f,%.4f,%.4f",
                    now / 1e6, b, got ? 1 : 0, o.input_fresh ? 1 : 0, a.peak_scaled, a.observed_peak_scaled,
                    a.peak_summary_valid ? 1 : 0, a.vu_level, a.vu_average, a.raw_rms, a.raw_peak_pcm,
                    a.silence ? 1 : 0, a.spectral_energy, a.novelty, a.tempo_valid ? 1 : 0, a.tempo_bpm,
                    a.tempo_confidence, a.tempo_phase01, gate);
        for (float v : o.chroma) std::printf(",%.4f", v);
        std::printf(",%.4f,%.4f,%.4f,%.5f,%.5f,%.2f,%.4f,%.5f,%.4f,%.4f,%.4f,%.6f,%.6f,%.6f,%.6f,%.4f\n",
                    pre, o.palette_coordinate, o.held_coordinate, o.peak_stage1, o.peak_stage2, o.position,
                    o.decay_rate, o.deposit_strength, o.smoothed_colour.red, o.smoothed_colour.green,
                    o.smoothed_colour.blue, sum[0] / 160, sum[1] / 160, sum[2] / 160, luma / 160, lit / 160.0);
      }
    }
  }
  return 0;
}
