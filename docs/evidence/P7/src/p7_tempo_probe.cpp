// Lane P7 steps 1-2 host probe (tempo half-time / 3:2 lock diagnosis and gate baseline).
//
// Device path reproduced: int16 mono PCM at 36-44 kHz -> k1_poly24 (24 kHz, 180-sample hops,
// as installed f2f1983a) -> InhouseAudio (analysis_rate_hz 24000) with tempo demand on board 1.
// A 24 kHz input bypasses k1_poly24 and is cut into 180-sample hops directly.
//
// Besides the product tracker inside InhouseAudio, the probe runs EXTERNAL TempoTrackerState
// copies (the unmodified product tempo_tracker.cpp / tempo_acf.cpp) fed with the same per-frame
// novelty and silence the product passes. Variants are HOST-ONLY probe switches; nothing in the
// product source is changed:
//   baseline   : product prior (log2-Gaussian 88 BPM, sigma 0.75 oct), product novelty
//   prior_flat : tempo_prior = 1 for every bin
//   prior_120  : log2-Gaussian centred on 120 BPM, sigma 0.75 oct (same shape as product)
//   gdft_low   : product prior; novelty = log-compressed positive flux of the raw GDFT bins
//                below 200 Hz (bins 0..10, 110-196 Hz; the bank has nothing below 110 Hz)
//   pcm_kick   : product prior; novelty = half-wave-rectified rise of log energy of the PCM
//                after a 2nd-order 150 Hz low-pass (kick-weighted; outside the GDFT bank)
//   flat_kick  : prior_flat + pcm_kick
// Every external baseline update is checked against the product board-1 tracker (bpm and
// confidence bit-identical); any divergence aborts with exit 9 (the external copy IS the product).
//
// Output: one CSV row per tracker update (44.44 Hz) per variant.
// Host oracle (Apple clang), not the M85 binary.
#include "inhouse_audio.h"
#include "core/audio/tempo_tracker.h"
#include "k1_poly24.h"
#include "pdm_target.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace ca = k1::core::audio;

namespace {

enum class Novelty { kProduct, kGdftLow, kPcmKick };

struct Variant final {
  const char* name;
  Novelty novelty;
  int prior;  // 0 product, 1 flat, 2 centred 120
  ca::TempoTrackerState state{};
};

constexpr std::uint16_t kBin69 = 69 - 60, kBin92 = 92 - 60, kBin138 = 138 - 60;

struct Biquad final {
  double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void lowpass(double fc, double fs, double q) {
    const double w = 2.0 * M_PI * fc / fs, c = std::cos(w), al = std::sin(w) / (2.0 * q), a0 = 1.0 + al;
    b0 = (1.0 - c) / 2.0 / a0; b1 = (1.0 - c) / a0; b2 = b0; a1 = -2.0 * c / a0; a2 = (1.0 - al) / a0;
  }
  double step(double x) {
    const double y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }
};

void applyPrior(ca::TempoTrackerState& s, int prior) {
  if (prior == 0) return;
  for (std::size_t i = 0; i < ca::kTempoTrackerBinCount; ++i) {
    const float bpm = 60.0F + static_cast<float>(i);
    const float z = std::log2(bpm / 120.0F) / 0.75F;
    s.tempo_prior[i] = prior == 1 ? 1.0F : std::exp(-0.5F * z * z);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: p7_tempo_probe in.s16 in_rate_hz out.csv [all|baseline]\n");
    return 2;
  }
  const unsigned in_rate = static_cast<unsigned>(std::strtoul(argv[2], nullptr, 10));
  const bool all = argc < 5 || std::string(argv[4]) == "all";
  if (in_rate != 24000U && (in_rate < K1_POLY24_MIN_IN_HZ || in_rate > K1_POLY24_MAX_IN_HZ)) return 2;
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);
  std::FILE* out = std::fopen(argv[3], "w");
  if (!out) return 3;

  static k1::titan::InhouseAudio audio;
  k1::titan::InhouseAudioConfig cfg{};
  cfg.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  audio.reset();
  if (!audio.configure(cfg)) return 5;
  audio.setTempoDemand(1, true);

  static std::array<Variant, 6> variants{{{"baseline", Novelty::kProduct, 0},
                                          {"prior_flat", Novelty::kProduct, 1},
                                          {"prior_120", Novelty::kProduct, 2},
                                          {"gdft_low", Novelty::kGdftLow, 0},
                                          {"pcm_kick", Novelty::kPcmKick, 0},
                                          {"flat_kick", Novelty::kPcmKick, 1}}};
  const std::size_t n_variants = all ? variants.size() : 1U;
  for (std::size_t v = 0; v < n_variants; ++v) {
    ca::initialiseTempoTracker(variants[v].state, ca::kProductionAudioRate);
    applyPrior(variants[v].state, variants[v].prior);
  }

  std::fprintf(out, "t_s,variant,bpm,confidence,locked,phase01,beat_tick,silence,novelty_in,hist_sample,"
                    "novelty_scale,acf_valid,comb69,comb92,comb138,score69,score92,score138,"
                    "argmax_bpm,product_bpm,product_conf\n");

  // Kick-weighted PCM envelope state (24 kHz domain, per 180-sample hop).
  Biquad lp1, lp2;
  lp1.lowpass(150.0, 24000.0, 0.7071);
  lp2.lowpass(150.0, 24000.0, 0.7071);
  double prev_log_e = -20.0;
  std::array<float, 80> prev_low{};

  static k1_poly24_t poly;
  k1_poly24_reset(&poly, in_rate == 24000U ? 40000U : in_rate);
  std::uint64_t hop_seq = 0;
  k1::titan::InhouseAudioCursor cursor{};
  std::vector<std::int16_t> pending;
  std::uint64_t mismatches = 0;

  auto feed = [&](const std::int16_t* hop180) {
    k1_audio_hop_t hop{};
    hop.contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;
    hop.samples = K1_POLY24_OUT_FRAMES;
    hop.analysis_rate_hz = K1_POLY24_OUT_HZ;
    hop.stream_epoch = 1U;
    hop.hop_sequence = hop_seq + 1U;
    hop.analysis_begin = hop_seq * K1_POLY24_OUT_FRAMES;
    hop.analysis_end_exclusive = hop.analysis_begin + K1_POLY24_OUT_FRAMES;
    std::memcpy(hop.pcm, hop180, K1_POLY24_OUT_FRAMES * sizeof(std::int16_t));
    ++hop_seq;
    const std::uint64_t now = hop.analysis_end_exclusive * 1000000ULL / K1_POLY24_OUT_HZ + 1000000ULL;
    hop.capture_estimate_valid = 1;
    hop.newest_sample_capture_estimate_us = now;
    (void)audio.consume(hop, now, true);
    audio.stampPublication(now + 100U);

    // Kick envelope from this hop's PCM.
    double e = 0.0;
    for (unsigned i = 0; i < K1_POLY24_OUT_FRAMES; ++i) {
      const double y = lp2.step(lp1.step(hop180[i] / 32768.0));
      e += y * y;
    }
    const double log_e = std::log10(e / K1_POLY24_OUT_FRAMES + 1e-12);
    const float pcm_kick = static_cast<float>(std::clamp((log_e - prev_log_e) / 3.0, 0.0, 1.0));
    prev_log_e = std::max(log_e, -12.0);
    // GDFT low-band flux (raw normalised magnitudes, bins below 200 Hz).
    const auto& raw = audio.rawSpectrum();
    const auto& freq = audio.configuration();
    double flux = 0.0;
    int nlow = 0;
    for (std::size_t b = 0; b < 80; ++b) {
      if (freq[b].target_frequency_hz >= 200.0F) break;
      const float m = std::log1p(100.0F * raw.bins[b].normalised_magnitude) / std::log1p(100.0F);
      flux += std::max(0.0F, m - prev_low[b]);
      prev_low[b] = m;
      ++nlow;
    }
    const float gdft_low = static_cast<float>(std::sqrt(flux / std::max(nlow, 1)));

    const auto& pub = audio.latest();
    const bool tempo_feed = pub.generation == hop_seq && audio.rawSpectrum().q0_overflow_count == 0U;
    if (!tempo_feed) return;
    k1::core::visual::InhouseWaveformAudio a{};
    (void)audio.acquire(cursor, now + 200U, 1000000U, a, 1U);
    const float product_novelty = a.novelty;
    const bool silence = pub.silence;
    for (std::size_t v = 0; v < n_variants; ++v) {
      auto& var = variants[v];
      const float nov = var.novelty == Novelty::kProduct ? product_novelty
                        : var.novelty == Novelty::kGdftLow ? gdft_low : pcm_kick;
      ca::updateTempoTracker(var.state, {static_cast<std::uint32_t>(pub.capture_us / 1000U), nov, silence});
      const auto ev = ca::readTempoTracker(var.state);
      if (!ev.updated) continue;
      if (v == 0 && (ev.bpm != a.tempo_bpm || ev.confidence != a.tempo_confidence)) ++mismatches;
      const auto& s = var.state;
      float best = -1.0F;
      std::size_t best_bin = 0;
      for (std::size_t b = 0; b < ca::kTempoTrackerBinCount; ++b) {
        const float sc = s.acf.comb_salience[b] * s.tempo_prior[b];
        if (sc > best) { best = sc; best_bin = b; }
      }
      const auto newest = s.novelty_history[(s.history_index + ca::kTempoAcfHistoryLength - 1U) %
                                            ca::kTempoAcfHistoryLength];
      std::fprintf(out, "%.6f,%s,%.1f,%.6f,%d,%.5f,%d,%d,%.6f,%.6f,%.6f,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.0f,%.1f,%.6f\n",
                   pub.capture_us / 1e6 - 1.0, var.name, ev.bpm, ev.confidence, ev.locked ? 1 : 0, ev.phase01,
                   ev.beat_tick ? 1 : 0, silence ? 1 : 0, nov, newest, s.novelty_scale, s.acf.valid ? 1 : 0,
                   s.acf.comb_salience[kBin69], s.acf.comb_salience[kBin92], s.acf.comb_salience[kBin138],
                   s.acf.comb_salience[kBin69] * s.tempo_prior[kBin69],
                   s.acf.comb_salience[kBin92] * s.tempo_prior[kBin92],
                   s.acf.comb_salience[kBin138] * s.tempo_prior[kBin138], 60.0F + best_bin, a.tempo_bpm,
                   a.tempo_confidence);
    }
  };

  constexpr unsigned kSlot = 296U;
  std::int16_t hop180[K1_POLY24_OUT_FRAMES];
  if (in_rate == 24000U) {
    for (std::size_t at = 0; at + K1_POLY24_OUT_FRAMES <= pcm.size(); at += K1_POLY24_OUT_FRAMES)
      feed(&pcm[at]);
  } else {
    for (std::size_t at = 0; at + kSlot <= pcm.size(); at += kSlot) {
      if (k1_poly24_push(&poly, &pcm[at], kSlot) != k1_poly24_ok) return 7;
      while (k1_poly24_pull180(&poly, hop180) == k1_poly24_ok) feed(hop180);
    }
  }
  std::fclose(out);
  std::printf("{\"hops\":%llu,\"parity_mismatches\":%llu,\"poly24_rejected\":%u}\n",
              static_cast<unsigned long long>(hop_seq), static_cast<unsigned long long>(mismatches), poly.rejected);
  return mismatches == 0U ? 0 : 9;
}
