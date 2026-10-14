// Host-only. Feed k1::core::audio::OnsetBeat the in-house pre-AGC magnitudes
// (linear_shape = magnitude_last * tilt, before gain, knee and the unity clamp).
// One row per analysis hop. Not part of the product image.
//   p6_onset_pre_agc in.s16 out.csv offset_s scale
#include "inhouse_audio.h"
#include "core/audio/onset_beat.h"
#include "k1_poly24.h"
#include "pdm_target.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <vector>

namespace {

float band_flux(const float* spectrum, const float* previous, int low, int high) {
  float flux = 0.0F;
  for (int bin = low; bin < high; ++bin) {
    const float difference = std::log(std::fmax(1.0e-6F, spectrum[bin])) -
                             std::log(std::fmax(1.0e-6F, previous[bin]));
    if (difference > 0.0F) flux += difference;
  }
  return flux;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: p6_onset_pre_agc in.s16 out.csv offset_s scale\n");
    return 2;
  }
  const float offset = std::strtof(argv[3], nullptr);
  const float scale = std::strtof(argv[4], nullptr);
  if (!(scale > 0.0F)) return 2;
  std::FILE* in = std::fopen(argv[1], "rb");
  if (!in) return 3;
  std::vector<std::int16_t> pcm;
  for (std::int16_t s; std::fread(&s, 2, 1, in) == 1;) pcm.push_back(s);
  std::fclose(in);
  std::FILE* out = std::fopen(argv[2], "w");
  if (!out) return 3;

  static k1::titan::InhouseAudio audio;
  audio.reset();
  k1::titan::InhouseAudioConfig config{};
  config.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  if (!audio.configure(config)) return 5;
  static k1_poly24_t poly;
  k1_poly24_reset(&poly, 40000U);
  k1::core::audio::OnsetBeat onset;
  k1::titan::InhouseAudioCursor cursor{};
  std::array<float, 80> previous{};
  bool have_previous = false;

  struct Hop { k1_audio_hop_t hop; std::uint64_t capture_us; };
  std::deque<Hop> queue;
  std::uint64_t hop_seq = 0;
  constexpr std::uint64_t kOriginUs = 1000000U;
  constexpr unsigned kSlot = 296U;
  std::fprintf(out, "t_song_s,silence,energy_pre,energy_post,novelty,kick,snare,hat,transient,"
                    "kick_strength,snare_strength,hat_strength,transient_strength,"
                    "kick_flux,snare_flux,hat_flux,full_flux\n");

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
    while (!queue.empty() && queue.front().capture_us <= now) {
      const auto hop = queue.front();
      queue.pop_front();
      (void)audio.consume(hop.hop, hop.capture_us, true);
      audio.stampPublication(std::min(hop.capture_us + 100U, now));
      k1::core::visual::InhouseWaveformAudio frame{};
      (void)audio.acquire(cursor, hop.capture_us, 50000U, frame, 0);
      const bool quiet = frame.silence && frame.raw_rms < frame.quiet_entry_rms;
      k1::contract::AudioFeaturesV1 features{};
      float energy_pre = 0.0F;
      for (unsigned bin = 0; bin < 80U; ++bin) {
        features.spectrum[bin] = frame.linear_shape[bin];
        energy_pre += frame.linear_shape[bin];
      }
      energy_pre /= 80.0F;
      features.spectral_energy = energy_pre;
      features.novelty = frame.novelty;
      features.low_energy = 0.0F;
      for (unsigned bin = 1; bin < 25U; ++bin) features.low_energy += frame.linear_shape[bin];
      features.low_energy /= 24.0F;
      features.peak_scaled = std::clamp(frame.peak_scaled, 0.0F, 1.0F);
      features.nyquist_safe_bin_hi = 80;
      features.source_frame_ms = static_cast<std::uint32_t>(hop.capture_us / 1000U);
      features.event_flags = quiet ? k1::contract::kEventSilence : 0U;
      features.validity_flags = k1::contract::kValidSpectrum;
      const auto event = onset.update(features);
      const float kick_flux = have_previous ? band_flux(features.spectrum, previous.data(), 1, 25) : 0.0F;
      const float snare_flux = have_previous ? band_flux(features.spectrum, previous.data(), 25, 50) : 0.0F;
      const float hat_flux = have_previous ? band_flux(features.spectrum, previous.data(), 70, 80) : 0.0F;
      const float full_flux = have_previous ? band_flux(features.spectrum, previous.data(), 1, 76) : 0.0F;
      for (unsigned bin = 0; bin < 80U; ++bin) previous[bin] = features.spectrum[bin];
      have_previous = !quiet;
      const float t_file = static_cast<float>(hop.capture_us - kOriginUs) / 1.0e6F;
      const float t_song = (t_file - offset) / scale;
      std::fprintf(out, "%.6f,%d,%.8g,%.8g,%.8g,%d,%d,%d,%d,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g\n",
                   t_song, quiet ? 1 : 0, energy_pre, frame.spectral_energy, frame.novelty,
                   event.kick ? 1 : 0, event.snare ? 1 : 0, event.hihat ? 1 : 0, event.transient ? 1 : 0,
                   event.kick_strength, event.snare_strength, event.hihat_strength, event.transient_strength,
                   kick_flux, snare_flux, hat_flux, full_flux);
    }
  }
  std::fclose(out);
  return 0;
}
