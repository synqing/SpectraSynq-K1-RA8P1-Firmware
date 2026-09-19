#include "core/audio/audio_pipeline.h"
#include "core/audio/gdft_goertzel.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace k1::core::audio;

static void fill_sine(GdftSampleWindow& window, float hz, float amp) {
  window.fill(0);
  const float sr = 24000.0F;
  for (std::size_t i = 0; i < window.size(); ++i) {
    const float s = amp * std::sin(2.0F * 3.14159265F * hz * float(i) / sr);
    int v = int(s * 32767.0F);
    if (v > 32767) v = 32767;
    if (v < -32767) v = -32767;
    window[i] = static_cast<std::int16_t>(v);
  }
}

static int gdft_centres() {
  const auto cfg = makeProductionGdftConfiguration();
  std::printf("GDFT bins=80 f0=%.3f f79=%.3f bass_below_hz=%.3f\n",
              cfg[0].target_frequency_hz, cfg[79].target_frequency_hz,
              cfg[0].target_frequency_hz);
  int miss = 0;
  const int probes[] = {0, 12, 24, 36, 40, 52, 64, 79};
  for (int p : probes) {
    GdftSampleWindow window{};
    fill_sine(window, cfg[static_cast<std::size_t>(p)].target_frequency_hz, 0.4F);
    GdftRawFrame raw{};
    analyseGdftRaw(window, cfg, raw);
    int best = 0;
    float best_m = -1.0F;
    for (int b = 0; b < 80; ++b) {
      if (raw.bins[static_cast<std::size_t>(b)].magnitude > best_m) {
        best_m = raw.bins[static_cast<std::size_t>(b)].magnitude;
        best = b;
      }
    }
    const int err = best > p ? best - p : p - best;
    std::printf("GDFT_TONE label_bin=%d peak_bin=%d err=%d mag=%.3f\n", p, best,
                err, best_m);
    if (err > 1) ++miss;
  }
  std::printf("GDFT_CENTRES misses=%d\n", miss);
  return 0;
}

static std::vector<std::int16_t> click_hops(float bpm, float amp, float seconds) {
  const int hops = int(seconds * 24000.0F / 180.0F);
  const float hop_hz = 24000.0F / 180.0F;
  const float period = hop_hz * 60.0F / bpm;
  std::vector<std::int16_t> out(static_cast<std::size_t>(hops) * 180U, 0);
  int next = 8;
  for (int h = 0; h < hops; ++h) {
    if (std::fabs(float(h) - float(next)) < 0.51F) {
      out[static_cast<std::size_t>(h) * 180U + 40U] =
          static_cast<std::int16_t>(amp * 20000.0F);
      next = int(std::lround(float(next) + period));
    }
  }
  return out;
}

struct BpmScore {
  float labelled;
  float reported;
  int labelled_hits;
  int onset_flags;
  int tempo_updates;
  int abs_err_ok;
};

static BpmScore run_bpm(float bpm, float amp) {
  AudioPipeline ap;
  GdftSampleWindow window{};
  auto hops = click_hops(bpm, amp, 8.0F);
  const int hop_n = int(hops.size() / 180);
  std::uint32_t last_onset = 0;
  BpmScore s{};
  s.labelled = bpm;
  s.labelled_hits = 0;
  for (int h = 0; h < hop_n; ++h) {
    if (hops[static_cast<std::size_t>(h) * 180U + 40U] != 0) ++s.labelled_hits;
  }
  float last_bpm = 0.0F;
  for (int h = 0; h < hop_n; ++h) {
    std::memmove(window.data(), window.data() + 180,
                 (window.size() - 180) * sizeof(std::int16_t));
    std::memcpy(window.data() + window.size() - 180,
                hops.data() + static_cast<std::size_t>(h) * 180U,
                180 * sizeof(std::int16_t));
    float peak = 0.0F, energy = 0.0F;
    for (int i = 0; i < 180; ++i) {
      const float x = hops[static_cast<std::size_t>(h) * 180U + static_cast<std::size_t>(i)] /
                      32768.0F;
      peak = std::fmax(peak, std::fabs(x));
      energy += x * x;
    }
    AudioPipelineInput in{};
    in.samples = &window;
    in.sequence = static_cast<std::uint32_t>(h + 1);
    in.peak_scaled = peak;
    in.vu_level = std::sqrt(energy / 180.0F);
    in.silence = peak == 0.0F;
    in.frame_ms = static_cast<std::uint32_t>(float(h) * 7.5F);
    in.media_time = {1, static_cast<std::uint64_t>(h + 1) * 360};
    in.media_time_valid = true;
    auto out = ap.process(in);
    if ((out.features.event_flags & k1::contract::kEventOnset) != 0U &&
        out.features.onset_event_id != last_onset) {
      ++s.onset_flags;
      last_onset = out.features.onset_event_id;
    }
    if (out.tempo.updated) {
      ++s.tempo_updates;
      last_bpm = out.tempo.bpm;
    }
  }
  s.reported = last_bpm;
  const float err = std::fabs(last_bpm - bpm);
  const float half = std::fabs(last_bpm - bpm * 0.5F);
  const float dbl = std::fabs(last_bpm - bpm * 2.0F);
  s.abs_err_ok = (err < 12.0F || half < 12.0F || dbl < 12.0F) ? 1 : 0;
  std::printf("BPM labelled=%.0f reported=%.2f onsets=%d labelled_hits=%d "
              "tempo_updates=%d family_ok=%d\n",
              s.labelled, s.reported, s.onset_flags, s.labelled_hits,
              s.tempo_updates, s.abs_err_ok);
  return s;
}

static int novelty_gain() {
  auto a = run_bpm(120.0F, 0.25F);
  auto b = run_bpm(120.0F, 0.90F);
  std::printf("NOVELTY_GAIN quiet_onsets=%d loud_onsets=%d quiet_bpm=%.2f "
              "loud_bpm=%.2f\n",
              a.onset_flags, b.onset_flags, a.reported, b.reported);
  return 0;
}

int main() {
  int rc = 0;
  rc |= gdft_centres();
  run_bpm(80.0F, 0.7F);
  run_bpm(120.0F, 0.7F);
  run_bpm(160.0F, 0.7F);
  rc |= novelty_gain();
  return rc;
}
