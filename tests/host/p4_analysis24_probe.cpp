// Lane P4 step 3 host probe: the in-house analysis at 12.8 kHz / 96 (product ASRC,
// Emotiscope profile, 64-sample descriptors) against 24 kHz / 180 (k1_poly24), on the
// same 40 kHz int16 PCM. One CSV row per published generation per path, with the
// level streams, board-1 tempo and every raw GDFT bin. Planted faults (frozen in
// TOLERANCE-FROZEN-analysis24.json) alter only the 24 kHz path:
//   rate     : k1_poly24 told the source is 5% faster than it is
//   upsample : the 24 kHz path is fed a linear upsample of the 12.8 kHz output
//   gain     : the 24 kHz path input is scaled x1.25
// Host oracle (Apple clang), not the M85 binary.
#include "inhouse_audio.h"
#include "k1_asrc_24k.h"
#include "k1_poly24.h"
#include "pdm_target.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
struct Path final {
  k1::titan::InhouseAudio audio;
  k1::titan::InhouseAudioCursor cursor{};
  std::uint64_t last_generation = 0;
  std::uint64_t hop_seq = 0;
  std::FILE* out = nullptr;
};

void header(std::FILE* f) {
  std::fprintf(f, "generation,t_end_s,valid,raw_rms,peak_scaled,vu_level,silence,novelty,spectral_energy,"
                  "tempo_valid,tempo_bpm,nyquist_bins");
  for (int b = 0; b < 80; ++b) std::fprintf(f, ",m%d", b);
  std::fprintf(f, "\n");
}

void feed(Path& p, const std::int16_t* pcm, std::uint32_t samples, std::uint32_t rate) {
  k1_audio_hop_t hop{};
  hop.contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;
  hop.samples = samples;
  hop.analysis_rate_hz = rate;
  hop.stream_epoch = 1U;
  hop.hop_sequence = p.hop_seq + 1U;
  hop.analysis_begin = p.hop_seq * samples;
  hop.analysis_end_exclusive = hop.analysis_begin + samples;
  std::memcpy(hop.pcm, pcm, samples * sizeof(std::int16_t));
  ++p.hop_seq;
  const std::uint64_t now = hop.analysis_end_exclusive * 1000000ULL / rate + 1000000ULL;
  hop.capture_estimate_valid = 1;
  hop.newest_sample_capture_estimate_us = now;
  (void)p.audio.consume(hop, now, true);
  p.audio.stampPublication(now + 100U);
  const auto& pub = p.audio.latest();
  if (pub.generation == p.last_generation) return;
  p.last_generation = pub.generation;
  k1::core::visual::InhouseWaveformAudio a{};
  (void)p.audio.acquire(p.cursor, now + 200U, 1000000U, a, 1U);
  const auto& raw = p.audio.rawSpectrum();
  std::fprintf(p.out, "%llu,%.6f,%d,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%d,%.3f,%u",
               static_cast<unsigned long long>(pub.generation),
               static_cast<double>(pub.analysis_end_sample_exclusive) / rate, pub.valid ? 1 : 0, pub.raw_rms,
               pub.peak_scaled, pub.vu_level, pub.silence ? 1 : 0, a.novelty, a.spectral_energy,
               a.tempo_valid ? 1 : 0, a.tempo_bpm, static_cast<unsigned>(raw.nyquist_safe_bin_high));
  for (int b = 0; b < 80; ++b) std::fprintf(p.out, ",%.4f", raw.bins[b].normalised_magnitude);
  std::fprintf(p.out, "\n");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: p4_analysis24_probe in.s16 out12800.csv out24000.csv none|rate|upsample|gain\n");
    return 2;
  }
  const std::string fault = argv[4];
  if (fault != "none" && fault != "rate" && fault != "upsample" && fault != "gain") return 2;
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);

  static Path legacy, a24;
  legacy.out = std::fopen(argv[2], "w");
  a24.out = std::fopen(argv[3], "w");
  if (!legacy.out || !a24.out) return 3;
  header(legacy.out);
  header(a24.out);
  k1::titan::InhouseAudioConfig c12{}, c24{};
  c24.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  legacy.audio.reset();
  a24.audio.reset();
  if (!legacy.audio.configure(c12) || !a24.audio.configure(c24)) return 5;
  legacy.audio.setTempoDemand(1, true);
  a24.audio.setTempoDemand(1, true);

  static k1_asrc24_t asrc;
  k1_asrc24_reset(&asrc, 1U, 40000U);
  static k1_poly24_t poly;
  k1_poly24_reset(&poly, fault == "rate" ? 42000U : 40000U);
  // Upsample fault: 12.8 kHz -> 24 kHz by linear interpolation (15/8), no content above 6.4 kHz.
  std::vector<std::int16_t> up_out;
  double up_pos = 0.0;
  std::int16_t up_prev = 0;
  bool up_primed = false;
  std::vector<std::int16_t> pending24;

  constexpr unsigned kSlot = 296U;
  std::int16_t scaled[kSlot];
  for (std::size_t at = 0; at + kSlot <= pcm.size(); at += kSlot) {
    if (k1_asrc24_push(&asrc, &pcm[at], kSlot) != k1_asrc24_ok) return 6;
    for (;;) {
      if (asrc.filled < k1_asrc24_required_for_pull180(&asrc)) break;
      std::int16_t hop64[K1_ASRC24_OUT_FRAMES];
      if (k1_asrc24_pull180(&asrc, hop64) != k1_asrc24_ok) break;
      feed(legacy, hop64, K1_ASRC24_OUT_FRAMES, K1_ASRC24_OUT_HZ);
      if (fault == "upsample") {
        for (unsigned i = 0; i < K1_ASRC24_OUT_FRAMES; ++i) {
          const std::int16_t cur = hop64[i];
          if (up_primed) {
            while (up_pos < 1.0) {
              pending24.push_back(static_cast<std::int16_t>(std::lrint(up_prev + (cur - up_prev) * up_pos)));
              up_pos += 12800.0 / 24000.0;
            }
            up_pos -= 1.0;
          }
          up_prev = cur;
          up_primed = true;
        }
      }
    }
    if (fault != "upsample") {
      const std::int16_t* src = &pcm[at];
      if (fault == "gain") {
        for (unsigned i = 0; i < kSlot; ++i)
          scaled[i] = static_cast<std::int16_t>(std::clamp<long>(std::lrint(pcm[at + i] * 1.25), -32768, 32767));
        src = scaled;
      }
      if (k1_poly24_push(&poly, src, kSlot) != k1_poly24_ok) return 7;
      std::int16_t hop180[K1_POLY24_OUT_FRAMES];
      while (k1_poly24_pull180(&poly, hop180) == k1_poly24_ok)
        feed(a24, hop180, K1_POLY24_OUT_FRAMES, K1_POLY24_OUT_HZ);
    } else {
      while (pending24.size() >= K1_POLY24_OUT_FRAMES) {
        feed(a24, pending24.data(), K1_POLY24_OUT_FRAMES, K1_POLY24_OUT_HZ);
        pending24.erase(pending24.begin(), pending24.begin() + K1_POLY24_OUT_FRAMES);
      }
    }
  }
  std::fclose(legacy.out);
  std::fclose(a24.out);
  std::printf("{\"poly24_produced\":%u,\"poly24_rejected\":%u,\"legacy_generations\":%llu,\"a24_generations\":%llu}\n",
              poly.produced, poly.rejected, static_cast<unsigned long long>(legacy.last_generation),
              static_cast<unsigned long long>(a24.last_generation));
  return 0;
}
