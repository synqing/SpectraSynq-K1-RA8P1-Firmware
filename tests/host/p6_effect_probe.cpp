// Lane P6 host replay of the two boot-composition effects (v8 reference, v9 candidate).
// 40 kHz int16 mono PCM -> k1_poly24 (24 kHz / 180, the installed product path) ->
// InhouseAudio (analysis24; AGC and levelling stages unchanged) -> one
// InhouseWaveformRenderer per board with the v8 composition controls (lower mode 32
// palette 32, upper mode 18 palette 39; renderInhouse passes palette-1, speed 0.5 ->
// motion 1.0, mirror on, response gain 1.0) at the 10 ms render period -> renderInhouse's
// display stage (brightness 1.0, shoulder, warmth .591 / 0, white balance) -> output
// intensity 0.25, the P3 payload metric. Not modelled: the donor admission gate, the wire
// packer, gamma, dither and the 24/255 hardware cap. Host oracle (Apple clang), not M85.
//
// Build without K1_INHOUSE_MAPPING_V9 for v8. The product v9 image defines
// K1_INHOUSE_MAPPING_V9 and not K1_INHOUSE_V9_HOST_VARIANTS: argc is 4 and the
// palette, gains and variant are the compile-time firmware constants. The scored
// host build defines both and selects the palette with the 9th argument (mapped = path P).
// Per 10 ms frame per board one CSV row: inputs, renderer observation, channel
// sum S and Rec.709 luma L (both means over 160 pixels), and the palette-hue check on the
// renderer output. frames.bin receives every renderer Pixel16 frame (board 0 then 1) for
// the byte-identity hash.
//   v8: p6_effect_probe in.s16 out.csv frames.bin
//   v9: p6_effect_probe in.s16 out.csv frames.bin variant fault gain32 gain18
//       variant: peak|energy|emphasis|default (firmware constants) ; fault: none|channel_max|floor
//       optional 9th argument (iteration 4): palette test variant asis|lifted|range
//       optional 10th/11th (iteration 5): level ceiling dBFS, emphasis exponent
//       optional 12th-18th (iteration 6): boost widen ratio_full epsilon fast_s slow_s luma_hold
//       optional 19th-25th (extent release): enabled fast_s slow_s lo_db span_db release_s newpeak_db_s
//       optional 26th-27th (kick trigger): trigger_sub(0|1) dip_db
//       optional final token (Experiment 1, default off): r01@t1,t2 or r01+r03@t1,t2
//       Drop times are probe file-time seconds. Absent token leaves the front off.
#include "inhouse_audio.h"
#include "core/visual/inhouse_waveform.h"
#include "core/visual/product_palette.h"
#include "k1_poly24.h"
#include "pdm_target.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

using k1::core::visual::InhouseWaveformAudio;
using k1::core::visual::InhouseWaveformControls;
using k1::core::visual::InhouseWaveformRenderer;
using k1::core::visual::Pixel16;

namespace {
struct Hop final { k1_audio_hop_t hop; std::uint64_t capture_us; };

// Normalised (max channel = 1) colours along the palette's stop interpolation.
// Element [3] is the palette position, used to recover each lit pixel's position (PAL b).
std::vector<std::array<float, 4>> paletteCurve(std::uint16_t id) {
  std::vector<std::array<float, 4>> curve;
  const auto& p = k1::core::visual::productPalette(id);
  for (unsigned k = 0; k <= 4096U; ++k) {
    const float x = static_cast<float>(k) / 4096.0F;
    unsigned i = 0;
    while (i + 1U < p.stop_count && static_cast<float>(p.stops[i + 1U].position) / 255.0F < x) ++i;
    float c[3];
    const auto& a = p.stops[i].colour;
    if (i + 1U >= p.stop_count || x <= p.stops[0].position / 255.0F) {
      const auto& s = p.stops[x <= p.stops[0].position / 255.0F ? 0U : p.stop_count - 1U].colour;
      c[0] = s.red; c[1] = s.green; c[2] = s.blue;
    } else {
      const auto& b = p.stops[i + 1U].colour;
      const float pos = p.stops[i].position / 255.0F, span = p.stops[i + 1U].position / 255.0F - pos;
      const float t = span > 1e-6F ? std::clamp((x - pos) / span, 0.0F, 1.0F) : 0.0F;
      c[0] = a.red + (b.red - a.red) * t; c[1] = a.green + (b.green - a.green) * t; c[2] = a.blue + (b.blue - a.blue) * t;
    }
    const float m = std::max(c[0], std::max(c[1], c[2]));
    if (m > 0.5F) curve.push_back({c[0] / m, c[1] / m, c[2] / m, x});
  }
  return curve;
}
}  // namespace

int main(int argc, char** argv) {
#if defined(K1_INHOUSE_MAPPING_V9) && defined(K1_INHOUSE_V9_HOST_VARIANTS)
  k1::core::visual::InhouseV9Front front{};
  if (argc > 4) {
    const std::string tail = argv[argc - 1];
    if (tail.compare(0, 3, "r01") == 0) {
      --argc;
      front.enabled = true;
      front.hold_contraction = tail.compare(0, 7, "r01+r03") == 0;
      front.travel_s = 0.12F;
      front.hold_lead_s = 6.0F;
      const auto at = tail.find('@');
      if (at != std::string::npos) {
        const std::string list = tail.substr(at + 1);
        unsigned n = 0;
        std::size_t pos = 0;
        while (pos < list.size() && n < front.drop_s.size()) {
          char* end = nullptr;
          const float value = std::strtof(list.c_str() + pos, &end);
          if (end == list.c_str() + pos) return 2;
          front.drop_s[n++] = value;
          pos = static_cast<std::size_t>(end - list.c_str());
          if (pos < list.size() && list[pos] == ',') ++pos;
        }
        front.drop_count = n;
      }
    }
  }
  if (argc != 8 && argc != 9 && argc != 11 && argc != 18 && argc != 25 && argc != 27) {
    std::fprintf(stderr, "usage: p6_effect_probe in.s16 out.csv frames.bin variant fault gain32 gain18 [asis|lifted|range|mapped] [r01@t | r01+r03@t]\n");
    return 2;
  }
#else
  // v8, and the product v9 image (mapping on, host-variant switch off).
  if (argc != 4) { std::fprintf(stderr, "usage: p6_effect_probe in.s16 out.csv frames.bin\n"); return 2; }
#endif
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);
  std::FILE* csv = std::fopen(argv[2], "w");
  std::FILE* bin = std::fopen(argv[3], "wb");
  if (!csv || !bin) return 3;

  static k1::titan::InhouseAudio audio;
  audio.reset();
  k1::titan::InhouseAudioConfig config{};
  config.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  if (!audio.configure(config)) return 5;
  audio.setTempoDemand(0, true);
  audio.setTempoDemand(1, true);
  static k1_poly24_t poly;
  k1_poly24_reset(&poly, 40000U);
  static InhouseWaveformRenderer renderer[2];
  renderer[0].reset();
  renderer[1].reset();
#if defined(K1_INHOUSE_MAPPING_V9) && defined(K1_INHOUSE_V9_HOST_VARIANTS)
  using k1::core::visual::InhouseV9Fault;
  using k1::core::visual::InhouseV9Variant;
  const std::string vname = argv[4], fname = argv[5];
  const InhouseV9Variant variant = vname == "peak" ? InhouseV9Variant::kConstantPeak
      : vname == "energy" ? InhouseV9Variant::kConstantEnergy
      : vname == "emphasis" ? InhouseV9Variant::kLevelEmphasis : static_cast<InhouseV9Variant>(255);
  const InhouseV9Fault fault = fname == "none" ? InhouseV9Fault::kNone
      : fname == "channel_max" ? InhouseV9Fault::kPerChannelMax
      : fname == "floor" ? InhouseV9Fault::kBrightnessFloor
      : fname == "rgb_mix" ? InhouseV9Fault::kRgbMixPalette : fname == "slot16" ? InhouseV9Fault::kSlot16
      : fname == "frozen" ? InhouseV9Fault::kFrozenPosition : static_cast<InhouseV9Fault>(255);
  // "default" keeps the firmware constants (kInhouseV9Variant, kInhouseV9Gain32/18).
  if (vname != "default") {
    if (static_cast<unsigned>(variant) > 2U || static_cast<unsigned>(fault) > 6U) return 2;
    const float gains[2] = {std::strtof(argv[6], nullptr), std::strtof(argv[7], nullptr)};
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostConfig(variant, fault, gains[0], gains[1])) return 2;
  }
  if (argc == 9) {  // iteration 4 palette test variant (P6-IT4-PALETTE-VARIANTS-FROZEN.json)
    using k1::core::visual::InhouseV9Palette;
    const std::string pname = argv[8];
    const InhouseV9Palette palette = pname == "asis" ? InhouseV9Palette::kAsIs
        : pname == "lifted" ? InhouseV9Palette::kLiftedLowEnd
        : pname == "range" ? InhouseV9Palette::kRestrictedRange
        : pname == "mapped" ? InhouseV9Palette::kPerPixelMapped : static_cast<InhouseV9Palette>(255);
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostPalette(palette)) return 2;
  }
  if (argc >= 25) {  // extent release: enabled fast_s slow_s lo_db span_db release_s newpeak_db_s [sub dip_db]
    k1::core::visual::InhouseV9Release release{};
    release.enabled = std::string(argv[18]) == "1";
    release.fast_s = std::strtof(argv[19], nullptr); release.slow_s = std::strtof(argv[20], nullptr);
    release.lo_db = std::strtof(argv[21], nullptr); release.span_db = std::strtof(argv[22], nullptr);
    release.release_s = std::strtof(argv[23], nullptr); release.newpeak_db_s = std::strtof(argv[24], nullptr);
    if (argc == 27) { release.trigger_sub = std::string(argv[25]) == "1"; release.dip_db = std::strtof(argv[26], nullptr); }
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostRelease(release)) return 2;
  }
  if (argc >= 18) {  // iteration 6: boost widen ratio_full epsilon fast_s slow_s luma_hold(0|1)
    k1::core::visual::InhouseV9Accent accent{};
    accent.boost = std::strtof(argv[11], nullptr); accent.widen_px = std::strtof(argv[12], nullptr);
    accent.ratio_full = std::strtof(argv[13], nullptr); accent.epsilon = std::strtof(argv[14], nullptr);
    accent.fast_s = std::strtof(argv[15], nullptr); accent.slow_s = std::strtof(argv[16], nullptr);
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostAccent(accent, std::string(argv[17]) == "1")) return 2;
  }
  if (argc >= 11) {  // iteration 5 candidate: level ceiling (dBFS) and emphasis exponent
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostLevel(std::strtof(argv[9], nullptr), std::strtof(argv[10], nullptr))) return 2;
  }
  if (front.enabled) {
    for (unsigned b = 0; b < 2; ++b)
      if (!renderer[b].setV9HostFront(front)) return 2;
  }
#endif
  InhouseWaveformControls controls[2];
  const float warmth[2] = {0.5905512F, 0.0F};
  for (unsigned b = 0; b < 2; ++b) { controls[b].motion_scale = 0.5F * 2.0F; controls[b].mirror = true; }
  controls[0].mode = 32; controls[0].palette_id = 32 - 1;
  controls[1].mode = 18; controls[1].palette_id = 39 - 1;
  const std::vector<std::array<float, 4>> curve[2] = {paletteCurve(controls[0].palette_id),
                                                      paletteCurve(controls[1].palette_id)};
  k1::titan::InhouseAudioCursor cursor[2]{};
  static Pixel16 frame[2][160];

  std::fprintf(csv, "t_s,board,fresh,silence,raw_rms,vu_level,peak_scaled,tempo_valid,tempo_bpm,tempo_confidence,"
                    "novelty,palette_coordinate,decay_rate,deposit_strength,position,scroll_steps,"
                    "S,L,lit_px,offpal_px,max_channel,level,tonality_gate,centroid_strength,intensity_mean,tempo_weight,"
                    "Lstar,Lstar_x3,extent,low_flux,accent,luma_ref,extent_edge,release,sub_rms,pal_bins,pal_span\n");
  constexpr unsigned kSlot = 296U;                   // device PDM block at 40 kHz
  constexpr std::uint64_t kOriginUs = 1000000U;      // keep every timestamp positive
  constexpr std::uint64_t kRenderUs = 10000U;        // palette period (composition period_us)
  std::deque<Hop> queue;
  std::uint64_t hop_seq = 0, next_tick = kOriginUs + kRenderUs;
  for (std::size_t at = 0; at + kSlot <= pcm.size(); at += kSlot) {
    if (k1_poly24_push(&poly, &pcm[at], kSlot) != k1_poly24_ok) return 6;
    Hop h{};
    while (k1_poly24_pull180(&poly, h.hop.pcm) == k1_poly24_ok) {
      h.hop.contract_version = K1_PDM_TARGET_AUDIO_CONTRACT_VERSION;
      h.hop.samples = K1_POLY24_OUT_FRAMES;
      h.hop.analysis_rate_hz = K1_POLY24_OUT_HZ;
      h.hop.stream_epoch = 1U;
      h.hop.hop_sequence = hop_seq + 1U;
      h.hop.analysis_begin = hop_seq * K1_POLY24_OUT_FRAMES;
      h.hop.analysis_end_exclusive = h.hop.analysis_begin + K1_POLY24_OUT_FRAMES;
      ++hop_seq;
      h.capture_us = kOriginUs + h.hop.analysis_end_exclusive * 1000000ULL / K1_POLY24_OUT_HZ;
      h.hop.capture_estimate_valid = 1;
      h.hop.newest_sample_capture_estimate_us = h.capture_us;
      queue.push_back(h);
    }
    const std::uint64_t now = kOriginUs + (at + kSlot) * 1000000ULL / 40000ULL;
    while (next_tick <= now) {
      const std::uint64_t tick = next_tick;
      next_tick += kRenderUs;
      while (!queue.empty() && queue.front().capture_us <= tick) {
        (void)audio.consume(queue.front().hop, queue.front().capture_us, true);
        audio.stampPublication(std::min(queue.front().capture_us + 100U, tick));
        queue.pop_front();
      }
      for (unsigned b = 0; b < 2; ++b) {
        InhouseWaveformAudio a{};
        (void)audio.acquire(cursor[b], tick, 50000U, a, b);
        // renderInhouse: confirmed quiet; response gain 1.0 clamps (no donor admission here).
        const bool confirmed_quiet = a.silence && a.raw_rms < a.quiet_entry_rms;
        a.silence = confirmed_quiet;
        a.peak_scaled = std::clamp(a.peak_scaled, 0.0F, 1.0F);
        a.vu_level = std::clamp(a.vu_level, 0.0F, 1.0F);
        a.vu_average = std::clamp(a.vu_average, 0.0F, 1.0F);
        a.observed_peak_scaled = std::clamp(a.observed_peak_scaled, 0.0F, 1.0F);
#if defined(K1_INHOUSE_MAPPING_V9) && defined(K1_INHOUSE_V9_HOST_VARIANTS)
        renderer[b].setV9HostClock(static_cast<float>(tick - kOriginUs) / 1.0e6F);
#endif
        if (!renderer[b].render(a, controls[b], tick, frame[b], 160)) std::memset(frame[b], 0, sizeof frame[b]);
        std::fwrite(frame[b], sizeof(Pixel16), 160, bin);
        const auto& o = renderer[b].observation();
        const float warm[3] = {1.0F, 1.0F - warmth[b] + warmth[b] * std::sqrt(.1982F),
                               1.0F - warmth[b] + warmth[b] * std::sqrt(.0244F)};
        const float white[3] = {1.0F, .9375F, .84F};
        double sum = 0, luma = 0;
        double ys[160];  // per-pixel Rec.709 luma for iteration-5 DI (TOLERANCE-FROZEN-P6-DI.json)
        unsigned lit = 0, off = 0;
        float maxc = 0.0F;
        std::vector<float> positions;  // recovered palette positions of lit right-half pixels (PAL b)
        for (unsigned p = 0; p < 160; ++p) {
          float c[3] = {frame[b][p].red / 65535.0F, frame[b][p].green / 65535.0F, frame[b][p].blue / 65535.0F};
          const float m = std::max(c[0], std::max(c[1], c[2]));
          maxc = std::max(maxc, m);
          if (m >= 0.01F) {
            ++lit;
            float best = 9.0F, best_pos = 0.0F;
            for (const auto& k : curve[b]) {
              const float d = std::max(std::fabs(c[0] / m - k[0]), std::max(std::fabs(c[1] / m - k[1]), std::fabs(c[2] / m - k[2])));
              if (d < best) { best = d; best_pos = k[3]; }
            }
            off += best > 0.03F ? 1U : 0U;
            if (p >= 80U) positions.push_back(best_pos);
          }
          const float sh = m > .75F ? (.75F + .25F * std::tanh((m - .75F) * 4.0F)) / m : 1.0F;
          double px[3];
          for (unsigned k = 0; k < 3; ++k) px[k] = std::clamp(c[k] * sh * warm[k] * white[k], 0.0F, 1.0F) * 0.25;
          sum += (px[0] + px[1] + px[2]) / 3.0;
          ys[p] = 0.2126 * px[0] + 0.7152 * px[1] + 0.0722 * px[2];
          luma += ys[p];
        }
        // PAL (b): distinct 1/32 bins and circular span of the recovered positions.
        unsigned n_bins = 0;
        double pos_span = 0.0;
        if (!positions.empty()) {
          bool seen[32] = {};
          for (float q : positions) seen[std::min(31, static_cast<int>(q * 32.0F))] = true;
          for (bool v : seen) n_bins += v ? 1U : 0U;
          std::sort(positions.begin(), positions.end());
          double gap = 1.0 - positions.back() + positions.front();
          for (std::size_t k = 1; k < positions.size(); ++k) gap = std::max<double>(gap, positions[k] - positions[k - 1]);
          pos_span = 1.0 - gap;
        }
        // CIE L* with Y_ref = 0.25 (full scale after the x0.25 output intensity).
        const auto lstar = [](double y) {
          const double t = std::min(1.0, y / 0.25);
          return t > 0.008856 ? 116.0 * std::cbrt(t) - 16.0 : 903.3 * t;
        };
        double lsum = 0, lsum3 = 0, ymax = 0;
        for (double y : ys) { lsum += lstar(y); lsum3 += lstar(3.0 * y); ymax = std::max(ymax, y); }
        unsigned wide = 0;
        for (double y : ys) wide += ymax > 0 && y >= 0.25 * ymax ? 1U : 0U;
        // EXT (TOLERANCE-FROZEN-P6-EXT.json): right half, innermost pixel >= 10% of the half's peak.
        double half_max = 0;
        for (unsigned p = 80; p < 160; ++p) half_max = std::max(half_max, ys[p]);
        double extent_edge = 0;
        if (half_max >= 1e-5)
          for (unsigned p = 80; p < 160; ++p)
            if (ys[p] >= 0.10 * half_max) { extent_edge = (160.0 - p) / 80.0; break; }
        std::fprintf(csv, "%.3f,%u,%d,%d,%.6f,%.6f,%.5f,%d,%.2f,%.4f,%.5f,%.5f,%.4f,%.5f,%.2f,%u,%.8f,%.8f,%u,%u,%.6f,",
                     (tick - kOriginUs) / 1e6, b, o.input_fresh ? 1 : 0, a.silence ? 1 : 0, a.raw_rms, a.vu_level,
                     a.peak_scaled, a.tempo_valid ? 1 : 0, a.tempo_bpm, a.tempo_confidence, a.novelty,
                     o.palette_coordinate, o.decay_rate, o.deposit_strength, o.position, o.scroll_steps,
                     sum / 160.0, luma / 160.0, lit, off, maxc);
#ifdef K1_INHOUSE_MAPPING_V9
        std::fprintf(csv, "%.4f,%.4f,%.4f,%.6f,%.4f,", o.level, o.tonality_gate, o.centroid_strength, o.intensity_mean,
                     o.tempo_weight);
#else
        std::fprintf(csv, ",,,,,");
#endif
        std::fprintf(csv, "%.4f,%.4f,%.4f,", lsum / 160.0, lsum3 / 160.0, wide / 160.0);
#ifdef K1_INHOUSE_MAPPING_V9
        std::fprintf(csv, "%.6f,%.4f,%.4f,", o.low_flux, o.accent, o.luma_ref);
#else
        std::fprintf(csv, ",,,");
#endif
#ifdef K1_INHOUSE_MAPPING_V9
        std::fprintf(csv, "%.4f,%.4f,%.6f,", extent_edge, o.release, a.sub_rms);
#else
        std::fprintf(csv, "%.4f,,,", extent_edge);
#endif
        std::fprintf(csv, "%u,%.4f\n", n_bins, pos_span);
      }
    }
  }
  std::fclose(csv);
  std::fclose(bin);
  return 0;
}
