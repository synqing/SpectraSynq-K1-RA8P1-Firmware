// Host-only. Feed k1::core::audio::OnsetBeat the in-house pre-AGC magnitudes
// (linear_shape = magnitude_last * tilt, before gain, knee and the unity clamp).
// One row per analysis hop. Not part of the product image.
//   p6_onset_pre_agc in.s16 out.csv offset_s scale [stages.f32]
//
// Chronology (DBS pilot step 1, Titan_DBS_Translation.html L5 repair). Each hop is
// consumed when the simulated clock passes its capture time; its publication is
// stamped at min(capture + 100 us, now) and read at `now`, which is at or after that
// publication. capture_us and the analysis sample interval remain the event times;
// the read time is recorded separately and never replaces them. consume(), acquire()
// and the frame's own valid flag are all checked. A rejected or unavailable read
// writes an explicit row (status != accepted) and is not given to the onset consumer.
//
// Optional stages.f32: per accepted hop, 3 x 80 float32 in this order:
// raw normalised magnitude (rawSpectrum, before the 0.3/0.7 average and the board
// Mood LPF), board-0 linear_shape (pre-gain, after that smoothing), board-0 spectrum
// (post gain/knee/clamp). Bins at or above nyquist_safe_bin_high are zero.
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

constexpr float kEpsilon = 1.0e-6F;  // numerical floor, not calibrated microphone noise

float band_flux(const float* spectrum, const float* previous, int low, int high) {
  float flux = 0.0F;
  for (int bin = low; bin < high; ++bin) {
    const float difference = std::log(std::fmax(kEpsilon, spectrum[bin])) -
                             std::log(std::fmax(kEpsilon, previous[bin]));
    if (difference > 0.0F) flux += difference;
  }
  return flux;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5 && argc != 6) {
    std::fprintf(stderr, "usage: p6_onset_pre_agc in.s16 out.csv offset_s scale [stages.f32]\n");
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
  std::FILE* stages = argc == 6 ? std::fopen(argv[5], "wb") : nullptr;
  if (argc == 6 && !stages) return 3;

  static k1::titan::InhouseAudio audio;
  audio.reset();
  k1::titan::InhouseAudioConfig config{};
  config.analysis_rate_hz = k1::titan::kInhouseAnalysis24Rate;
  if (!audio.configure(config)) return 5;
  static k1_poly24_t poly;
  k1_poly24_reset(&poly, 40000U);
  k1::core::audio::OnsetBeat onset;
  k1::titan::InhouseAudioCursor cursor{};
  std::array<float, 80> previous{}, previous_raw{}, previous_linear{};
  bool have_previous = false;

  struct Hop { k1_audio_hop_t hop; std::uint64_t capture_us; };
  std::deque<Hop> queue;
  std::uint64_t hop_seq = 0;
  std::uint64_t accepted = 0, rejected = 0;
  constexpr std::uint64_t kOriginUs = 1000000U;
  constexpr unsigned kSlot = 296U;
  std::fprintf(out, "t_song_s,silence,energy_pre,energy_post,novelty,kick,snare,hat,transient,"
                    "kick_strength,snare_strength,hat_strength,transient_strength,"
                    "kick_flux,snare_flux,hat_flux,full_flux,"
                    "status,generation,capture_us,publish_us,read_us,support_bins,"
                    "raw_full_flux,linear_full_flux,spectrum_full_flux,vu_level,raw_rms\n");

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
      const float t_file = static_cast<float>(hop.capture_us - kOriginUs) / 1.0e6F;
      const float t_song = (t_file - offset) / scale;
      const char* status = "accepted";
      const bool consumed = audio.consume(hop.hop, hop.capture_us, true);
      const std::uint64_t publish_us = std::min(hop.capture_us + 100U, now);
      k1::core::visual::InhouseWaveformAudio frame{};
      bool read_ok = false;
      if (!consumed) {
        status = "rejected_consume";
      } else {
        audio.stampPublication(publish_us);
        // Read at the simulated time now >= publish_us, never at the earlier capture time.
        read_ok = audio.acquire(cursor, now, 50000U, frame, 0);
        if (!read_ok) status = "rejected_acquire";
        else if (!frame.valid) status = "rejected_invalid";
      }
      const auto& raw = audio.rawSpectrum();
      const unsigned support = std::min<unsigned>(80U, static_cast<unsigned>(raw.nyquist_safe_bin_high));
      if (!consumed || !read_ok || !frame.valid) {
        ++rejected;
        // Explicit unavailable row: no feature values, no onset update, history kept.
        std::fprintf(out, "%.6f,,,,,,,,,,,,,,,,,%s,%llu,%llu,%llu,%llu,%u,,,,,\n", t_song, status,
                     static_cast<unsigned long long>(frame.generation),
                     static_cast<unsigned long long>(hop.capture_us),
                     static_cast<unsigned long long>(publish_us), static_cast<unsigned long long>(now),
                     support);
        continue;
      }
      ++accepted;
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
      features.nyquist_safe_bin_hi = static_cast<std::uint8_t>(support);
      features.source_frame_ms = static_cast<std::uint32_t>(hop.capture_us / 1000U);
      features.event_flags = quiet ? k1::contract::kEventSilence : 0U;
      features.validity_flags = k1::contract::kValidSpectrum;  // only after an accepted, valid read
      const auto event = onset.update(features);
      std::array<float, 80> raw_now{};
      for (unsigned bin = 0; bin < support; ++bin) raw_now[bin] = raw.bins[bin].normalised_magnitude;
      const float kick_flux = have_previous ? band_flux(features.spectrum, previous.data(), 1, 25) : 0.0F;
      const float snare_flux = have_previous ? band_flux(features.spectrum, previous.data(), 25, 50) : 0.0F;
      const float hat_flux = have_previous ? band_flux(features.spectrum, previous.data(), 70, 80) : 0.0F;
      const float full_flux = have_previous ? band_flux(features.spectrum, previous.data(), 1, 76) : 0.0F;
      const int top = static_cast<int>(support);
      const float raw_flux = have_previous ? band_flux(raw_now.data(), previous_raw.data(), 0, top) : 0.0F;
      const float linear_flux = have_previous ? band_flux(frame.linear_shape.data(), previous_linear.data(), 0, top) : 0.0F;
      static std::array<float, 80> previous_post{};
      const float post_flux = have_previous ? band_flux(frame.spectrum.data(), previous_post.data(), 0, top) : 0.0F;
      for (unsigned bin = 0; bin < 80U; ++bin) {
        previous[bin] = features.spectrum[bin];
        previous_raw[bin] = raw_now[bin];
        previous_linear[bin] = frame.linear_shape[bin];
        previous_post[bin] = frame.spectrum[bin];
      }
      have_previous = !quiet;
      if (stages) {
        std::fwrite(raw_now.data(), sizeof(float), 80, stages);
        std::fwrite(frame.linear_shape.data(), sizeof(float), 80, stages);
        std::fwrite(frame.spectrum.data(), sizeof(float), 80, stages);
      }
      std::fprintf(out, "%.6f,%d,%.8g,%.8g,%.8g,%d,%d,%d,%d,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g,%.8g,"
                        "%s,%llu,%llu,%llu,%llu,%u,%.8g,%.8g,%.8g,%.8g,%.8g\n",
                   t_song, quiet ? 1 : 0, energy_pre, frame.spectral_energy, frame.novelty,
                   event.kick ? 1 : 0, event.snare ? 1 : 0, event.hihat ? 1 : 0, event.transient ? 1 : 0,
                   event.kick_strength, event.snare_strength, event.hihat_strength, event.transient_strength,
                   kick_flux, snare_flux, hat_flux, full_flux, status,
                   static_cast<unsigned long long>(frame.generation),
                   static_cast<unsigned long long>(frame.capture_us),
                   static_cast<unsigned long long>(frame.publish_us), static_cast<unsigned long long>(now),
                   support, raw_flux, linear_flux, post_flux, frame.vu_level, frame.raw_rms);
    }
  }
  std::fclose(out);
  if (stages) std::fclose(stages);
  std::fprintf(stderr, "accepted %llu rejected %llu\n", static_cast<unsigned long long>(accepted),
               static_cast<unsigned long long>(rejected));
  return accepted > 0 ? 0 : 7;
}
