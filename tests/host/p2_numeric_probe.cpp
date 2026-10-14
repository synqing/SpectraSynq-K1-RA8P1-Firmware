// Lane P2 host numeric probe: the v8 output path (ASRC -> InhouseAudio ->
// InhouseWaveformRenderer, lower mode 32/palette 32, upper mode 18/palette 39)
// on frozen 40 kHz int16 mono PCM. Built twice with different float semantics
// (resident: contraction off, no vectorisation; product: contraction fast,
// vectorisation on) and compared by scripts/p2_numeric_compare.py against the
// frozen tolerance. Host oracle only; not M85 execution.
#include "inhouse_audio.h"
#include "core/visual/inhouse_waveform.h"
#include "k1_asrc_24k.h"
#include "pdm_target.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using k1::core::visual::InhouseWaveformAudio;
using k1::core::visual::InhouseWaveformControls;
using k1::core::visual::InhouseWaveformRenderer;
using k1::core::visual::Pixel16;

namespace {
constexpr unsigned kSlot = 296U;  // K1_PDM_TARGET_SLOT_ELEMENTS
constexpr std::uint64_t kHopUs = 5000U;  // 64 samples at 12.8 kHz

void put_f32(std::FILE* f, float v) { std::fwrite(&v, 4, 1, f); }
void put_u16(std::FILE* f, std::uint16_t v) { std::fwrite(&v, 2, 1, f); }
void put_u32(std::FILE* f, std::uint32_t v) { std::fwrite(&v, 4, 1, f); }
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) { std::fprintf(stderr, "usage: probe in.s16 out.bin\n"); return 2; }
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);
  std::FILE* out = std::fopen(argv[2], "wb");
  if (!out) return 4;

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

  std::uint64_t hop_seq = 0, hops = 0, frames = 0, invalid = 0;
  for (std::size_t at = 0; at + kSlot <= pcm.size(); at += kSlot) {
    if (k1_asrc24_push(&asrc, &pcm[at], kSlot) != k1_asrc24_ok) return 6;
    for (;;) {
      k1_audio_hop_t hop{};
      if (asrc.filled < k1_asrc24_required_for_pull180(&asrc)) break;
      hop.contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;  // as pdm_target.c sets it
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
      for (unsigned i = 0; i < K1_ASRC24_OUT_FRAMES; ++i) put_u16(out, std::uint16_t(hop.pcm[i]));
      const bool done = audio.consume(hop, now, true);
      audio.stampPublication(now + 100U);
      ++hops;
      put_u32(out, done ? 1U : 0U);
      if (hop_seq % 2U) continue;  // 100 Hz presentation, 200 Hz hops
      for (unsigned b = 0; b < 2; ++b) {
        InhouseWaveformAudio a{};
        const bool got = audio.acquire(cursor[b], now + 200U, 50000U, a, b);
        if (!renderer[b].render(a, controls[b], now + 200U, frame[b], 160)) ++invalid;
        const auto& o = renderer[b].observation();
        put_u32(out, got ? 1U : 0U);
        for (float v : o.chroma) put_f32(out, v);
        put_f32(out, o.peak_stage1); put_f32(out, o.peak_stage2); put_f32(out, o.position);
        put_f32(out, o.palette_coordinate); put_f32(out, o.decay_rate); put_f32(out, o.deposit_strength);
        put_f32(out, audio.rawRms());
        for (unsigned p = 0; p < 160; ++p) {
          put_u16(out, frame[b][p].red); put_u16(out, frame[b][p].green); put_u16(out, frame[b][p].blue);
        }
      }
      ++frames;
    }
  }
  std::fclose(out);
  std::printf("{\"hops\":%llu,\"frames\":%llu,\"render_invalid\":%llu}\n",
              (unsigned long long)hops, (unsigned long long)frames, (unsigned long long)invalid);
  return 0;
}
