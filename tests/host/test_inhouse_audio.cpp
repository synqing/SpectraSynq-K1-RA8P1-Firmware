#include "inhouse_audio.h"
#include "inhouse_source_oracle.h"
#include "k1_tempo.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <chrono>

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
  std::array<std::array<std::int16_t,96>,4> history{};
  unsigned history_row=0;
  float previous_vu=0;
  std::size_t pending = 0;
  unsigned comparisons = 0;
  std::uint32_t random = 0x41554449U;
  for (std::uint64_t seq = 1; seq <= 360U; ++seq) {
    // Quiet, weak, high sustained resonance, isolated attack, then quiet.
    const float amplitude = seq < 15U ? 0.0F : seq < 35U ? 500.0F :
        seq < 100U ? 30000.0F : seq == 110U ? 20000.0F : 0.0F;
    auto h = hop(seq, amplitude);
    // Retain the original 120 full-chain frames, then exercise boundary PCM,
    // seeded full-range random PCM, and sustained positive/negative rails.
    if (seq > 180U) {
      for (unsigned i = 0; i < 64; ++i) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        if (seq <= 240U) h.pcm[i] = i % 2U == 0U ? INT16_MIN : INT16_MAX;
        else if (seq <= 300U) h.pcm[i] = static_cast<std::int16_t>(
            static_cast<std::int32_t>(random & 65535U) - 32768);
        else h.pcm[i] = seq <= 330U ? INT16_MAX : INT16_MIN;
      }
    }
    for (unsigned i = 0; i < 64; ++i) {
      block[pending++] = h.pcm[i];
      if (pending == 96U) {
        std::move(oracle_window.begin() + 96, oracle_window.end(), oracle_window.begin());
        std::copy(block.begin(), block.end(), oracle_window.end() - 96);
        history_row=(history_row+1)%4;history[history_row]=block;
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
      const auto magnitude = static_cast<float>(static_cast<std::int32_t>(std::sqrt(static_cast<float>(mag2))));
      assert(raw.bins[i].magnitude == magnitude);
      assert(raw.bins[i].normalised_magnitude == magnitude * config.inverse_block_size_half);
    }
    for (unsigned i = 71; i < 80; ++i) {
      assert(raw.bins[i].q1 == 0 && raw.bins[i].q2 == 0 && raw.bins[i].magnitude_squared == 0);
      assert(raw.bins[i].magnitude == 0.0F && raw.bins[i].normalised_magnitude == 0.0F);
    }
    // The test runner extracts the actual broadband branch and time constants
    // from the pinned S3 source. FixedPoints is the independent arithmetic oracle.
    const auto expected = source_broadband(raw, audio.configuration(), comparisons == 1U);
    for (unsigned i = 0; i < 80; ++i) assert(audio.latest().spectrum[i] == expected[i]);
    float spectral=0,novelty=0;source_features(expected,&spectral,&novelty);
    assert(audio.latest().spectral_energy==spectral&&audio.latest().novelty==novelty);
    assert(audio.latest().vu_average==source_vu_average(audio.latest().vu_level,previous_vu));
    previous_vu=audio.latest().vu_level;
    assert(audio.latest().pcm_history==history);
    assert(audio.latest().pcm_history_valid==(comparisons>=4));
    assert(audio.latest().raw_floor_pcm==audio.config().drive_floor_pcm);
    assert(std::isfinite(audio.latest().peak_scaled));
    if (seq == 2U) {
      assert(audio.latest().analysis_begin_sample == 0U);
      assert(audio.latest().analysis_end_sample_exclusive == 96U);
      assert(audio.latest().capture_us == 7400U);
      assert(audio.latest().publish_us == 10000U);
      assert(!audio.latest().valid); // no invented full spectral history at boot
    }
  }
  assert(comparisons == 240U);
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
  const auto capture = audio.latest().capture_us;
  assert(audio.acquire(a, capture + 30000U, 30000U, af));
  assert(!audio.acquire(a, capture + 30001U, 30000U, af));
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

static void source_tempo_and_peer() {
  InhouseAudio audio;audio.reset();audio.setTempoDemand(0,true);audio.setTempoDemand(1,true);
  k1_tempo_init();InhouseAudioCursor cursors[2]{};
  unsigned compared=0,emits=0;bool nonzero_phase=false,nonzero_confidence=false;
  for(std::uint64_t seq=1;seq<=6000;++seq){
    // Real admitted PCM: weak carrier + 120 BPM attacks, then six seconds quiet.
    const bool beat=((seq*64)%6400)<768;
    auto h=hop(seq,seq<4800?(beat?20000.0F:2000.0F):0.0F);
    if(seq==2700)assert(audio.setMood(1,.9f));
    if(seq==3000)audio.setTempoDemand(1,false);
    if(seq==3100)audio.setTempoDemand(1,true);
    if(!push(audio,h))continue;
    const auto& latest=audio.latest();
    K1AudioSnapshot source{};source.frame_ms=std::uint32_t(latest.capture_us/1000);
    source.novelty=latest.novelty;source.silence=latest.silence;k1_tempo_update(source);
    const auto expected=k1_tempo_read();
    Frame a{},b{};const bool valid=audio.acquire(cursors[0],seq*5000,30000,a,0);
    (void)audio.acquire(cursors[1],seq*5000,30000,b,1);
    assert(a.tempo_bpm==expected.bpm);
    assert(a.tempo_phase01==expected.phase01);
    assert(a.tempo_confidence==expected.confidence);
    if(valid&&a.tempo_capture_us){assert(a.tempo_valid);++emits;}
    nonzero_phase|=a.tempo_phase01>0;nonzero_confidence|=a.tempo_confidence>0;
    ++compared;
  }
  assert(compared==4000&&emits>3500&&nonzero_phase&&nonzero_confidence);
  Frame final{};assert(audio.acquire(cursors[0],30000000,30000,final,0));
  const auto peer_bpm=final.tempo_bpm,peer_phase=final.tempo_phase01,peer_conf=final.tempo_confidence;
  audio.setTempoDemand(1,false);
  assert(audio.acquire(cursors[0],30000000,30000,final,0));
  assert(final.tempo_bpm==peer_bpm&&final.tempo_phase01==peer_phase&&final.tempo_confidence==peer_conf);
  assert(audio.acquire(cursors[1],30000000,30000,final,1)&&!final.tempo_valid);
  assert(!audio.acquire(cursors[0],30040000,30000,final,0)&&!final.tempo_valid);
  // A new capture epoch discards all PCM and tempo continuity. Demand survives;
  // new observations must earn validity and confidence again.
  auto next=hop(1,8000,8);push(audio,next);
  assert(!audio.acquire(cursors[0],5000,30000,final,0));
  assert(!final.pcm_history_valid&&!final.tempo_valid);
  next=hop(2,8000,8);(void)audio.consume(next,0,false);audio.stampPublication(10000);
  assert(!audio.acquire(cursors[0],10000,30000,final,0));
  assert(!final.tempo_valid&&final.tempo_capture_us==0&&final.tempo_confidence==0);
  next=hop(3,8000,8);(void)audio.consume(next,0,false);audio.stampPublication(15000);
  assert(!audio.acquire(cursors[0],15000,30000,final,0)&&!final.tempo_valid);
  std::printf("4000 completed PCM generations: exact original V2 BPM/phase/confidence; independent Mood/demand, stale and epoch checks\n");
}

static void tempo_freshness() {
  InhouseAudio audio; audio.reset(); audio.setTempoDemand(0, true);
  InhouseAudioCursor cursor{}; Frame frame{}; unsigned held_checks = 0;
  for (std::uint64_t seq = 1; seq <= 300; ++seq) {
    const auto input = hop(seq);
    if (!audio.consume(input, input.newest_sample_capture_estimate_us, true)) continue;
    const auto capture = audio.latest().capture_us;
    // Real production publication delay: retain the actual tracker timestamp,
    // then render a fresh AP after the held event has reached 34 ms of age.
    audio.stampPublication(capture + 7500U);
    if (!audio.acquire(cursor, capture + 7500U, 30000U, frame)) continue;
    if (!frame.tempo_capture_us || capture - frame.tempo_capture_us != 15000U) continue;
    const auto now = frame.tempo_capture_us + 34000U;
    assert(audio.acquire(cursor, now, 30000U, frame));
    assert(now - frame.capture_us == 19000U && frame.tempo_valid);
    // Fresh publication time cannot disguise a capture older than 30 ms.
    assert(!audio.acquire(cursor, capture + 30001U, 30000U, frame));
    assert(!frame.valid && !frame.tempo_valid);
    ++held_checks;
  }
  assert(held_checks > 20U);
  std::printf("tempo freshness: %u actual held events valid at 34 ms; stale capture rejected despite fresh publication\n", held_checks);
}

static void host_cost_observation() {
  // Sanitised host relative cost only; root must measure the assembled Titan.
  std::printf("static InhouseAudio bytes=%zu; each TempoTrackerState bytes=%zu\n",sizeof(InhouseAudio),sizeof(k1::core::audio::TempoTrackerState));
  for(unsigned demand=0;demand<=2;++demand){
    InhouseAudio audio;audio.reset();for(unsigned board=0;board<demand;++board)audio.setTempoDemand(board,true);
    double total=0,maximum=0;unsigned completed=0;
    for(std::uint64_t seq=1;seq<=900;++seq){
      const auto input=hop(seq,seq%100<12?22000:2000);
      const auto begin=std::chrono::steady_clock::now();const bool ready=push(audio,input);
      const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
      if(ready){total+=us;maximum=std::max(maximum,us);++completed;}
    }
    std::printf("sanitised host demand=%u completed=%u mean_us=%.3f maximum_us=%.3f (not target timing)\n",demand,completed,total/completed,maximum);
  }
}

int main() {
  source_parity();
  chronology_and_levels();
  independent_mood();
  source_tempo_and_peer();
  tempo_freshness();
  host_cost_observation();
  std::puts("inhouse audio: exact source recurrence/LPF/broadband arithmetic, chronology, weak/sustain/quiet, independent Mood and peak cursors");
}
