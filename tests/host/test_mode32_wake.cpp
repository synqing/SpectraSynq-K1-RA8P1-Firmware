#include "palette_runtime.h"
#include "k1_pdm_sensitivity.h"
#include "core/audio/audio_pipeline.h"
#include "core/visual/product_effect_renderer.h"
#include "core/visual/visual_audio_frame.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace k1;
using namespace k1::core::audio;
using namespace k1::core::visual;
using namespace k1::titan;

static unsigned energy(const PaletteRuntime& rt) {
  unsigned sum = 0U;
  for (unsigned i = 0U; i < 160U; ++i) {
    const auto p = rt.channel(0).frame()[i];
    sum += unsigned(p.red) + unsigned(p.green) + unsigned(p.blue);
  }
  return sum;
}

static unsigned max_pixel(const PaletteRuntime& rt) {
  unsigned peak = 0U;
  for (unsigned i = 0U; i < 160U; ++i) {
    const auto p = rt.channel(0).frame()[i];
    peak = std::max(peak, unsigned(p.red));
    peak = std::max(peak, unsigned(p.green));
    peak = std::max(peak, unsigned(p.blue));
  }
  return peak;
}

static void configure_runtime(PaletteRuntime& rt) {
  PaletteConfig cfg;
  cfg.mode_a = 32U;
  cfg.mode_b = 32U;
  cfg.flags = 1U;
  cfg.brightness = 255U;
#ifdef K1_PALETTE_MORPH
  cfg.version = 3U;
#endif
  assert(rt.configure(cfg, 0U));
}

struct FeatureFrame {
  contract::AudioFeaturesV1 audio{};
  VisualWaveformHistory wave{};
  TempoTrackerEvent tempo{};
};

static FeatureFrame make_hit(float peak, bool chroma) {
  FeatureFrame frame{};
  frame.audio.validity_flags = contract::kValidBaseSnapshot |
                               contract::kValidChroma | contract::kValidSpectrum;
  frame.audio.peak_scaled = peak;
  frame.audio.vu_level = peak;
  if (chroma) {
    frame.audio.chroma_a_origin[0] = 0.9F;
    frame.audio.chroma_a_origin[7] = 0.35F;
    frame.audio.chroma_strength = 0.8F;
  }
  frame.wave.sample_count = 96U;
  frame.wave.raw_maximum = 20000.0F;
  frame.wave.peak_scaled = peak;
  for (std::size_t h = 0; h < kVisualWaveformHistoryFrames; ++h)
    for (std::size_t s = 0; s < frame.wave.sample_count; ++s)
      frame.wave.frames[h][s] = static_cast<std::int16_t>((s % 12U) * 2000);
  return frame;
}

static FeatureFrame make_gap() {
  FeatureFrame frame{};
  frame.audio.event_flags = contract::kEventSilence;
  return frame;
}

static FeatureFrame make_noise(float peak) {
  FeatureFrame frame{};
  frame.audio.peak_scaled = peak;
  frame.audio.vu_level = peak;
  frame.wave.sample_count = 96U;
  frame.wave.raw_maximum = peak * 32768.0F;
  frame.wave.peak_scaled = peak;
  return frame;
}

static VisualAudioFrameView view_of(const FeatureFrame& frame) {
  return VisualAudioFrameView{frame.audio, frame.tempo, frame.wave, 0U};
}

static int feature_replay() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto second = make_hit(0.4F, true);
  const auto sparse = make_hit(0.4F, false);
  const auto gap = make_gap();
  unsigned f = 0U;
  auto step = [&](const FeatureFrame& frame) {
    const auto view = view_of(frame);
    assert(rt.step(std::uint64_t(f) * kPalettePeriodUs, &view));
    ++f;
  };
  for (unsigned i = 0U; i < 24U; ++i) step(hit);
  const unsigned after_hit = energy(rt);
  assert(after_hit > 0U);
  unsigned after_100ms = 0U, after_250ms = 0U;
  for (unsigned i = 0U; i < 30U; ++i) {
    step(gap);
    if (i == 11U) after_100ms = energy(rt);
    if (i == 29U) after_250ms = energy(rt);
  }
  const unsigned after_short_gap = energy(rt);
  for (unsigned i = 0U; i < 24U; ++i) step(second);
  const unsigned after_second_hit = energy(rt);
  PaletteRuntime sparse_rt;
  configure_runtime(sparse_rt);
  for (unsigned i = 0U; i < 24U; ++i) {
    const auto view = view_of(sparse);
    assert(sparse_rt.step(std::uint64_t(i) * kPalettePeriodUs, &view));
  }
  std::printf("SPARSE_FALLBACK energy=%u max=%u\n", energy(sparse_rt),
              max_pixel(sparse_rt));
  if (energy(sparse_rt) == 0U) {
    std::fprintf(stderr, "SPARSE_FAIL peak/VU fallback deposited nothing\n");
    return 1;
  }
  unsigned after_500ms = 0U, after_1s = 0U, after_2s = 0U;
  for (unsigned i = 0U; i < 240U; ++i) {
    step(gap);
    if (i == 59U) after_500ms = energy(rt);
    if (i == 119U) after_1s = energy(rt);
    if (i == 239U) after_2s = energy(rt);
  }
  for (unsigned i = 0U; i < 480U; ++i) step(gap);
  const unsigned after_6s = energy(rt);
  std::printf("FEATURE after_hit=%u short_gap_100ms=%u short_gap_250ms=%u "
              "after_second=%u gap_500ms=%u gap_1s=%u gap_2s=%u silence_6s=%u "
              "max_at_2s=%u\n",
              after_hit, after_100ms, after_250ms, after_second_hit,
              after_500ms, after_1s, after_2s, after_6s, max_pixel(rt));
  if (after_short_gap == 0U || after_2s == 0U) {
    std::fprintf(stderr, "WAKE_FAIL feature gap erased history short=%u two_s=%u\n",
                 after_short_gap, after_2s);
    return 1;
  }
  if (after_second_hit == 0U) {
    std::fprintf(stderr, "WAKE_FAIL second hit did not deposit\n");
    return 1;
  }
  if (after_6s >= after_2s && after_2s > 0U) {
    std::fprintf(stderr, "SILENCE_FAIL did not decay after dwell\n");
    return 1;
  }
  return 0;
}

static int noise_present_replay() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto hit_view = view_of(hit);
  const float raw = 200.0F / 32768.0F;
  const float scaled =
      float(k1_pdm_scale_sample(200, K1_PDM_SENSITIVITY_Q8)) / 32768.0F;
  const auto scaled_noise = make_noise(scaled);
  const auto raw_noise = make_noise(raw);
  const auto scaled_view = view_of(scaled_noise);
  const auto raw_view = view_of(raw_noise);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  const unsigned after_hit = energy(rt);
  unsigned scaled_100 = 0U, scaled_250 = 0U, scaled_500 = 0U;
  for (unsigned i = 0U; i < 240U; ++i) {
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &scaled_view));
    if (i == 11U) scaled_100 = energy(rt);
    if (i == 29U) scaled_250 = energy(rt);
    if (i == 59U) scaled_500 = energy(rt);
  }
  const unsigned after_scaled_noise = energy(rt);
  std::printf("SCALED_NOISE_TRACE 100ms=%u 250ms=%u 500ms=%u 2s=%u max=%u\n",
              scaled_100, scaled_250, scaled_500, after_scaled_noise,
              max_pixel(rt));
  PaletteRuntime rt2;
  configure_runtime(rt2);
  f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt2.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  for (unsigned i = 0U; i < 240U; ++i)
    assert(rt2.step(std::uint64_t(f++) * kPalettePeriodUs, &raw_view));
  const unsigned after_raw_noise = energy(rt2);
  std::printf("NOISE raw_peak=%.5f scaled_peak=%.5f after_hit=%u "
              "after_2s_raw_noise=%u after_2s_scaled_noise=%u\n",
              raw, scaled, after_hit, after_raw_noise, after_scaled_noise);
  if (after_scaled_noise == 0U) {
    std::fprintf(stderr, "NOISE_WAKE_FAIL scaled PCM noise ejected history\n");
    return 1;
  }
  return 0;
}

struct PcmRig {
  AudioPipeline ap{};
  GdftSampleWindow window{};
  VisualWaveformHistory waveform{};
  std::uint32_t sequence = 0;
};

static void scale_hop(std::int16_t hop[180]) {
  uint32_t peak = 1u;
  for (unsigned i = 0U; i < 180U; ++i) {
    const int32_t s = hop[i];
    const uint32_t mag = s < 0 ? uint32_t(-s) : uint32_t(s);
    if (mag > peak) peak = mag;
  }
  const uint32_t gain = k1_pdm_loud_guard_q8(peak);
  for (unsigned i = 0U; i < 180U; ++i)
    hop[i] = k1_pdm_scale_sample(hop[i], gain);
}

static void feed_hop(PaletteRuntime& rt, PcmRig& rig, std::uint64_t t_us,
                     const std::int16_t hop[180]) {
  std::memmove(rig.window.data(), rig.window.data() + 180,
               (rig.window.size() - 180) * sizeof(std::int16_t));
  std::memcpy(rig.window.data() + rig.window.size() - 180, hop,
              180 * sizeof(std::int16_t));
  float peak = 0.0F, energy_acc = 0.0F;
  for (unsigned i = 0U; i < 180U; ++i) {
    const float x = hop[i] / 32768.0F;
    peak = std::fmax(peak, std::fabs(x));
    energy_acc += x * x;
  }
  AudioPipelineInput input{};
  input.samples = &rig.window;
  input.sequence = ++rig.sequence;
  input.media_time = {1, static_cast<std::uint64_t>(rig.sequence) * 360};
  input.media_time_valid = true;
  input.frame_ms = static_cast<std::uint32_t>(input.media_time.frame_index / 48);
  input.capture_time_us = t_us;
  input.publish_time_us = t_us;
  input.peak_scaled = peak;
  input.vu_level = std::sqrt(energy_acc / 180.0F);
  input.silence = peak == 0.0F;
  auto output = rig.ap.process(input);
  pushVisualWaveform(rig.waveform, hop, 180, peak * 32768.0F, peak,
                     rig.sequence);
  const VisualAudioFrameView view{output.features, output.tempo, rig.waveform,
                                  t_us};
  assert(output.valid);
  assert(rt.step(t_us, &view));
}

static int pcm_replay() {
  PaletteRuntime rt;
  configure_runtime(rt);
  PcmRig rig{};
  std::int16_t hop[180]{};
  unsigned f = 0U;
  auto click = [&]() {
    for (unsigned i = 0U; i < 180U; ++i) {
      const float phase = 2.0F * 3.14159265F * 6.0F * float(i) / 180.0F;
      hop[i] = static_cast<std::int16_t>(20000.0F * std::sin(phase));
    }
    scale_hop(hop);
    feed_hop(rt, rig, std::uint64_t(f++) * kPalettePeriodUs, hop);
  };
  auto quiet = [&]() {
    std::memset(hop, 0, sizeof(hop));
    feed_hop(rt, rig, std::uint64_t(f++) * kPalettePeriodUs, hop);
  };
  for (unsigned i = 0U; i < 24U; ++i) click();
  const unsigned after_hit = energy(rt);
  for (unsigned i = 0U; i < 30U; ++i) quiet();
  const unsigned after_short = energy(rt);
  for (unsigned i = 0U; i < 24U; ++i) click();
  const unsigned after_second = energy(rt);
  for (unsigned i = 0U; i < 240U; ++i) quiet();
  const unsigned after_2s = energy(rt);
  for (unsigned i = 0U; i < 480U; ++i) quiet();
  const unsigned after_6s = energy(rt);
  std::printf("PCM after_hit=%u short_gap=%u after_second=%u gap_2s=%u "
              "silence_6s=%u\n",
              after_hit, after_short, after_second, after_2s, after_6s);
  if (after_hit == 0U || after_short == 0U || after_2s == 0U) {
    std::fprintf(stderr, "PCM_WAKE_FAIL hit=%u short=%u two_s=%u\n", after_hit,
                 after_short, after_2s);
    return 1;
  }
  return 0;
}

static unsigned nonzero_bytes(const PaletteRuntime& rt) {
  unsigned n = 0U;
  for (unsigned i = 0U; i < 160U; ++i) {
    const auto p = rt.channel(0).frame()[i];
    n += unsigned(p.red != 0) + unsigned(p.green != 0) + unsigned(p.blue != 0);
  }
  return n;
}

static int isolation_zero_input(std::uint32_t period_us, const char* label) {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto gap = make_gap();
  const auto hit_view = view_of(hit);
  const auto gap_view = view_of(gap);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * period_us, &hit_view));
  auto history = rt.channel(0).previousFrame();
  auto dest = rt.channel(0).frame();
  for (unsigned i = 0U; i < 160U; ++i) {
    history[i] = k1::core::Pixel8{0U, 0U, 0U};
    dest[i] = k1::core::Pixel8{0U, 0U, 0U};
  }
  for (unsigned i = 80U; i < 88U; ++i)
    history[i] = k1::core::Pixel8{72U, 0U, 0U};
  float t_one = -1.0F, t_zero = -1.0F;
  float one_span = 0.0F, one_run = 0.0F;
  const unsigned limit = unsigned(6.0F * 1000000.0F / float(period_us)) + 8U;
  for (unsigned i = 0U; i < limit; ++i) {
    assert(rt.step(std::uint64_t(f++) * period_us, &gap_view));
    const float t = float(i + 1U) * float(period_us) / 1000000.0F;
    const unsigned mx = max_pixel(rt);
    const unsigned nz = nonzero_bytes(rt);
    if (mx == 1U && t_one < 0.0F) t_one = t;
    if (mx <= 1U) one_run += float(period_us) / 1000000.0F;
    else one_run = 0.0F;
    if (one_run > one_span) one_span = one_run;
    if (nz == 0U && t_zero < 0.0F) {
      t_zero = t;
      break;
    }
  }
  std::printf("ISOLATION %s t_one=%.3f t_zero=%.3f one_span=%.3f max=%u nz=%u\n",
              label, double(t_one), double(t_zero), double(one_span),
              max_pixel(rt), nonzero_bytes(rt));
  if (t_zero < 0.0F || t_zero > 5.20F) {
    std::fprintf(stderr, "ISOLATION_FAIL %s never reached true black in 5.2s\n",
                 label);
    return 1;
  }
  if (t_zero < 3.20F) {
    std::fprintf(stderr, "ISOLATION_FAIL %s collapsed wake t_zero=%.3f\n",
                 label, double(t_zero));
    return 1;
  }
  if (t_one >= 0.0F && t_zero - t_one > 2.00F) {
    std::fprintf(stderr, "ISOLATION_FAIL %s one-count floor lasted %.3fs\n",
                 label, double(t_zero - t_one));
    return 1;
  }
  return 0;
}

static int isolation_second_hit() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto gap = make_gap();
  const auto hit_view = view_of(hit);
  const auto gap_view = view_of(gap);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  for (unsigned i = 0U; i < 120U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &gap_view));
  const unsigned during_release = energy(rt);
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  const unsigned after_second = energy(rt);
  std::printf("ISOLATION_SECOND during_release=%u after_second=%u\n",
              during_release, after_second);
  if (after_second <= during_release) {
    std::fprintf(stderr, "ISOLATION_FAIL second hit did not draw during release\n");
    return 1;
  }
  return 0;
}

static bool status_has(const PaletteRuntime& rt, const char* token) {
  char text[8192];
  const auto n = rt.statusJson(text, sizeof(text));
  return n > 0 && std::strstr(text, token) != nullptr;
}

static int isolation_path_tap() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto gap = make_gap();
  const auto hit_view = view_of(hit);
  const auto gap_view = view_of(gap);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  if (!status_has(rt, "\"visual_path\":\"effect\"") ||
      !status_has(rt, "\"musical\":true")) {
    std::fprintf(stderr, "ISOLATION_FAIL hit path was not effect/musical\n");
    return 1;
  }
  assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &gap_view));
  if (!status_has(rt, "\"visual_path\":\"dwell\"") ||
      !status_has(rt, "\"dwell_reinit\":true") ||
      !status_has(rt, "\"musical\":false")) {
    std::fprintf(stderr, "ISOLATION_FAIL first gap did not reinit dwell\n");
    return 1;
  }
  assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &gap_view));
  if (!status_has(rt, "\"dwell_reinit\":false") ||
      !status_has(rt, "\"visual_path\":\"dwell\"")) {
    std::fprintf(stderr, "ISOLATION_FAIL dwell reinit repeated\n");
    return 1;
  }
  std::printf("ISOLATION_PATH hit=effect gap=dwell reinit-once\n");
  return 0;
}

static int isolation_run26_hops_do_not_brighten() {
  // Sampled last_hop_peak from run-26 AFTER the click hop 22585.
  // peak_scaled = hop/32767. None of these reach the 0.25 musical gate.
  static const int hops[] = {
      368, 225, 199, 249, 233, 269, 290, 373, 265, 227, 288, 215, 243, 285,
      338, 233, 228, 229, 240, 261, 215, 252, 267, 214, 1197, 242, 489, 240,
      273, 226, 297, 210, 194, 1409, 260, 216, 158, 251, 245, 163, 225, 229};
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto hit_view = view_of(hit);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  unsigned prev = max_pixel(rt);
  unsigned rises = 0U;
  unsigned effect = 0U;
  for (int hop : hops) {
    FeatureFrame gap = make_gap();
    const float peak = float(hop) / 32767.0F;
    gap.audio.peak_scaled = peak;
    gap.audio.vu_level = peak;
    gap.wave.peak_scaled = peak;
    gap.wave.raw_maximum = float(hop);
    const auto view = view_of(gap);
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &view));
    char text[8192];
    rt.statusJson(text, sizeof(text));
    if (std::strstr(text, "\"visual_path\":\"effect\"")) ++effect;
    const unsigned mx = max_pixel(rt);
    if (mx > prev) ++rises;
    prev = mx;
  }
  std::printf("ISOLATION_HOP_REPLAY rises=%u effect_frames_in_tail=%u last_max=%u\n",
              rises, effect, max_pixel(rt));
  if (effect != 0U) {
    std::fprintf(stderr, "ISOLATION_FAIL hop-as-peak selected effect path\n");
    return 1;
  }
  if (rises != 0U) {
    std::fprintf(stderr, "ISOLATION_FAIL hop-as-peak brightened %u times\n",
                 rises);
    return 1;
  }
  return 0;
}

static int isolation_quiet_chroma_stays_dwell() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto hit_view = view_of(hit);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  FeatureFrame quiet = make_gap();
  quiet.audio.peak_scaled = 0.04F;
  quiet.audio.vu_level = 0.04F;
  quiet.audio.chroma_strength = 0.09F;
  quiet.wave.peak_scaled = 0.04F;
  const auto view = view_of(quiet);
  assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &view));
  if (!status_has(rt, "\"visual_path\":\"dwell\"") ||
      !status_has(rt, "\"musical\":false")) {
    std::fprintf(stderr, "ISOLATION_FAIL quiet chroma opened effect path\n");
    return 1;
  }
  std::printf("ISOLATION_CHROMA peak=0.04 chroma=0.09 path=dwell\n");
  return 0;
}

static int isolation_loud_peak_opens_effect() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto hit = make_hit(0.4F, true);
  const auto hit_view = view_of(hit);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 24U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &hit_view));
  FeatureFrame loud = make_gap();
  loud.audio.peak_scaled = 0.40F;
  loud.audio.vu_level = 0.40F;
  loud.audio.chroma_strength = 0.0F;
  loud.wave.peak_scaled = 0.40F;
  const auto view = view_of(loud);
  assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &view));
  if (!status_has(rt, "\"visual_path\":\"effect\"") ||
      !status_has(rt, "\"musical\":true")) {
    std::fprintf(stderr, "ISOLATION_FAIL loud peak stayed on dwell\n");
    return 1;
  }
  std::printf("ISOLATION_LOUD_PEAK path=effect\n");
  return 0;
}

static int isolation_zero_history() {
  PaletteRuntime rt;
  configure_runtime(rt);
  const auto quiet = make_noise(0.01F);
  const auto quiet_view = view_of(quiet);
  unsigned f = 0U;
  for (unsigned i = 0U; i < 48U; ++i)
    assert(rt.step(std::uint64_t(f++) * kPalettePeriodUs, &quiet_view));
  const unsigned nz = nonzero_bytes(rt);
  std::printf("ISOLATION_ZERO_HISTORY nz=%u energy=%u\n", nz, energy(rt));
  if (nz != 0U) {
    std::fprintf(stderr, "ISOLATION_FAIL subthreshold features drew nz=%u\n", nz);
    return 1;
  }
  return 0;
}

int main() {
  const int feature = feature_replay();
  const int noise = noise_present_replay();
  const int pcm = pcm_replay();
  const int iso120 = isolation_zero_input(8333U, "120fps");
  const int iso60 = isolation_zero_input(16667U, "60fps");
  const int second = isolation_second_hit();
  const int zero = isolation_zero_history();
  const int path = isolation_path_tap();
  const int hops = isolation_run26_hops_do_not_brighten();
  const int chroma = isolation_quiet_chroma_stays_dwell();
  const int loud = isolation_loud_peak_opens_effect();
  if (feature || noise || pcm || iso120 || iso60 || second || zero || path ||
      hops || chroma || loud)
    return 1;
  return 0;
}
