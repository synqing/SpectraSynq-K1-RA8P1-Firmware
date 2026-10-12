#include "inhouse_audio.h"
#include "inhouse_source_oracle.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace k1::titan;
using Frame = k1::core::visual::InhouseWaveformAudio;

static k1_audio_hop_t hop(std::uint64_t seq, float amplitude = 8000.0F,
                         std::uint64_t epoch = 7U) {
  k1_audio_hop_t h{};
  h.contract_version = 2;
  h.samples = 64;
  h.analysis_rate_hz = 12800;
  h.stream_epoch = epoch;
  h.hop_sequence = seq;
  h.analysis_begin = (seq - 1U) * 64U;
  h.analysis_end_exclusive = seq * 64U;
  h.capture_estimate_valid = 1;
  h.newest_sample_capture_estimate_us = seq * 5000U - 100U;
  for (unsigned i = 0; i < 64U; ++i)
    h.pcm[i] = static_cast<std::int16_t>(amplitude * std::sin(
        static_cast<float>(h.analysis_begin + i) * 6.283185307179586F * 440.0F / 12800.0F));
  return h;
}

static bool push(InhouseAudio& audio, const k1_audio_hop_t& h) {
  const bool complete = audio.consume(h, h.newest_sample_capture_estimate_us, true);
  audio.stampPublication(h.hop_sequence * 5000U);
  return complete;
}

static void source_parity() {
  InhouseAudio audio;
  audio.reset();
  assert(audio.configuration()[0].target_frequency_hz == 110.0F);
  assert(audio.configuration()[0].block_size == 978U);
  assert(audio.configuration()[24].target_frequency_hz == 440.0F);
  std::array<std::int16_t, 4096> oracle_window{};
  std::array<std::int16_t, 96> block{};
  std::size_t pending = 0;
  unsigned comparisons = 0;
  for (std::uint64_t seq = 1; seq <= 180U; ++seq) {
    // Quiet, weak, high sustained resonance, isolated attack, then quiet.
    const float amplitude = seq < 15U ? 0.0F : seq < 35U ? 500.0F :
        seq < 100U ? 30000.0F : seq == 110U ? 20000.0F : 0.0F;
    const auto h = hop(seq, amplitude);
    for (unsigned i = 0; i < 64; ++i) {
      block[pending++] = h.pcm[i];
      if (pending == 96U) {
        std::move(oracle_window.begin() + 96, oracle_window.end(), oracle_window.begin());
        std::copy(block.begin(), block.end(), oracle_window.end() - 96);
        pending = 0;
      }
    }
    if (!push(audio, h)) continue;
    ++comparisons;
    const auto& raw = audio.rawSpectrum();
    assert(raw.nyquist_safe_bin_high == 71U && raw.q0_overflow_count == 0U);
    for (unsigned i = 0; i < 71U; ++i) {
      const auto& config = audio.configuration()[i];
      K1GdftLane4ExactState state{config.coefficient_q14, config.block_size, 0, 0};
      k1_gdft_lane4_exact_tail(state, oracle_window.data(), 4096, 0);
      assert(raw.bins[i].q1 == state.q1 && raw.bins[i].q2 == state.q2);
      const std::int64_t coeff_term = (static_cast<std::int64_t>(state.coeff_q14) * state.q1) >> 14;
      const std::int64_t mag2 = std::max<std::int64_t>(0,
          static_cast<std::int64_t>(state.q2) * state.q2 +
          static_cast<std::int64_t>(state.q1) * state.q1 - coeff_term * state.q2);
      assert(raw.bins[i].magnitude_squared == mag2);
    }
    for (unsigned i = 71; i < 80; ++i) assert(raw.bins[i].normalised_magnitude == 0.0F);
    // The test runner extracts the actual broadband branch and time constants
    // from the pinned S3 source. FixedPoints is the independent arithmetic oracle.
    const auto expected = source_broadband(raw, audio.configuration(), comparisons == 1U);
    for (unsigned i = 0; i < 80; ++i) assert(audio.latest().spectrum[i] == expected[i]);
    assert(std::isfinite(audio.latest().peak_scaled));
    if (seq == 2U) {
      assert(audio.latest().analysis_begin_sample == 0U);
      assert(audio.latest().analysis_end_sample_exclusive == 96U);
      assert(audio.latest().capture_us == 7400U);
      assert(audio.latest().publish_us == 10000U);
      assert(!audio.latest().valid); // no invented full spectral history at boot
    }
  }
  assert(comparisons == 120U);
}

static void chronology_and_levels() {
  InhouseAudio audio;
  audio.reset();
  InhouseAudioConfig invalid;
  invalid.drive_floor_pcm = std::numeric_limits<float>::quiet_NaN();
  assert(!audio.configure(invalid));
  assert(!push(audio, hop(1)));
  assert(push(audio, hop(2)));
  const auto gen = audio.latest().generation;
  assert(!push(audio, hop(2))); // duplicate cannot advance state
  assert(audio.latest().generation == gen);
  assert(!push(audio, hop(4))); // gap discards partial state and descriptor
  assert(!audio.latest().valid);
  assert(!push(audio, hop(5, 0.0F)));
  assert(push(audio, hop(6, 0.0F)));
  assert(audio.latest().raw_peak_pcm == 0.0F);
  assert(audio.latest().peak_scaled == 0.0F);
  assert(audio.latest().analysis_begin_sample == 256U);
  assert(audio.latest().analysis_end_sample_exclusive == 352U);
  for (std::uint64_t seq = 7U; seq <= 45U; ++seq) push(audio, hop(seq, 600.0F));
  assert(audio.latest().valid && audio.latest().peak_scaled > 0.02F);
  assert(audio.latest().vu_level > 0.0F); // weak music survives source floor
  InhouseAudioCursor a{}, b{};
  Frame af{}, bf{};
  auto now = audio.latest().publish_us;
  assert(audio.acquire(a, now, 30000U, af));
  assert(audio.acquire(b, now, 30000U, bf));
  assert(af.peak_summary_valid && bf.peak_summary_valid);
  assert(af.observed_peak_scaled == bf.observed_peak_scaled);
  assert(audio.acquire(a, now, 30000U, af) && !af.peak_summary_valid); // sustained use, no replayed attack
  for (std::uint64_t seq = 46; seq <= 48; ++seq) push(audio, hop(seq, seq == 46 ? 30000.0F : 0.0F));
  now = audio.latest().publish_us;
  // A skipped output may inspect a temporary cursor but must not commit it.
  auto tentative = a;
  assert(audio.acquire(tentative, now, 30000U, af));
  const float skipped_peak = af.observed_peak_scaled;
  assert(audio.acquire(a, now, 30000U, af));
  assert(audio.acquire(b, now, 30000U, bf));
  assert(af.peak_summary_valid && af.observed_peak_scaled > af.peak_scaled);
  assert(af.observed_peak_scaled == skipped_peak);
  assert(af.observed_peak_scaled == bf.observed_peak_scaled); // independent readers
  assert(audio.acquire(a, now + 30000U, 30000U, af));
  assert(!audio.acquire(a, now + 30001U, 30000U, af));
  assert(!af.valid && !af.peak_summary_valid);
  assert(!audio.acquire(a, now - 1U, 30000U, af)); // reversed clock is invalid
  for (std::uint64_t seq = 49; seq <= 70; ++seq) push(audio, hop(seq));
  assert(!audio.acquire(b, audio.latest().publish_us, 30000U, bf)); // eight-entry retention overflow
  assert(audio.acquire(b, audio.latest().publish_us, 30000U, bf));
  assert(!bf.peak_summary_valid); // no fabricated retrospective attack
  for (std::uint64_t seq = 71; seq <= 1400; ++seq) push(audio, hop(seq, 0.0F));
  assert(audio.latest().silence && audio.latest().vu_level == 0.0F);
  assert(audio.latest().peak_scaled < 0.00001F);
  assert(!push(audio, hop(1, 0.0F, 8U)));
  assert(!audio.latest().valid); // epoch transition flushes all state
  auto malformed = hop(2, 0.0F, 8U);
  malformed.analysis_rate_hz = 24000;
  assert(!push(audio, malformed));
  assert(!audio.latest().valid);
}

static void independent_mood() {
  InhouseAudio changed, baseline;
  changed.reset(); baseline.reset();
  assert(!changed.setMood(2U, 0.5F));
  assert(!changed.setMood(0U, std::numeric_limits<float>::quiet_NaN()));
  InhouseAudioCursor cc0{}, cc1{}, bc0{}, bc1{};
  Frame c0{}, c1{}, b0{}, b1{};
  bool changed_spectrum = false;
  for (std::uint64_t seq = 1; seq <= 90; ++seq) {
    if (seq == 30U) assert(changed.setMood(1U, 0.95F));
    const auto h = hop(seq, seq < 50U ? 8000.0F : 500.0F);
    push(changed, h); push(baseline, h);
    const auto now = seq * 5000U;
    const bool a = changed.acquire(cc0, now, 30000U, c0, 0U);
    const bool b = baseline.acquire(bc0, now, 30000U, b0, 0U);
    const bool c = changed.acquire(cc1, now, 30000U, c1, 1U);
    const bool d = baseline.acquire(bc1, now, 30000U, b1, 1U);
    assert(a == b && c == d);
    assert(c0.spectrum == b0.spectrum); // changing top Mood cannot touch bottom
    if (seq > 30U && c1.spectrum != b1.spectrum) changed_spectrum = true;
  }
  assert(changed_spectrum);
  assert(changed.mood(0U) == 0.05F && changed.mood(1U) == 0.95F);
}

int main() {
  source_parity();
  chronology_and_levels();
  independent_mood();
  std::puts("inhouse audio: exact source recurrence/LPF/broadband arithmetic, chronology, weak/sustain/quiet, independent Mood and peak cursors");
}
