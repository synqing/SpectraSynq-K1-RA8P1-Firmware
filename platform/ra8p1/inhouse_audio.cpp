#include "inhouse_audio.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace k1::titan {
namespace {
constexpr std::int32_t kOne = 65536;
constexpr float kDt = static_cast<float>(kInhouseHopSamples) / kInhouseSampleRate;
// The source tracker emits after three admitted 96-sample generations.
static_assert(core::visual::kInhouseTempoUpdatePeriodUs ==
              kInhouseHopSamples * 3U * 1000000U / kInhouseSampleRate);
std::int32_t q(float value) noexcept { return static_cast<std::int32_t>(value * 65536.0F); }
float f(std::int32_t value) noexcept { return static_cast<float>(value) / 65536.0F; }
std::int32_t mul(std::int32_t a, std::int32_t b) noexcept {
  return static_cast<std::int32_t>((static_cast<std::int64_t>(a) * b) >> 16);
}
std::int32_t divq(std::int32_t a, std::int32_t b) noexcept {
  return b == 0 ? 0 : static_cast<std::int32_t>(static_cast<std::int64_t>(a) * kOne / b);
}
float alpha(float reference, float dt) noexcept {
  // Source k1_agc_dt_clock.h: rational, 100 Hz reference, not exponential EMA.
  const float tau = 0.010F * (1.0F - reference) / reference;
  return dt / (tau + dt);
}

struct InhouseLane final {
  std::int32_t coefficient_q14 = 0;
  std::uint16_t block_size = 0;
  std::int32_t q1 = 0;
  std::int32_t q2 = 0;
};

inline void stepInhouseLane(InhouseLane& lane, std::int32_t sample,
                            std::uint32_t& overflows) noexcept {
  const auto product = static_cast<std::int64_t>(lane.coefficient_q14) * lane.q1;
  const auto q0 = (static_cast<std::int64_t>(sample) >> 6) + (product >> 14) - lane.q2;
  if (q0 > INT32_MAX || q0 < INT32_MIN) ++overflows;
  lane.q2 = lane.q1;
  lane.q1 = core::audio::wrapToInt32(q0);
}

void analyseInhouseScalar(const core::audio::GdftSampleWindow& window,
                          const core::audio::GdftConfiguration& frequencies,
                          core::audio::GdftRawFrame& output) noexcept {
  // Source audio/k1_gdft_core.cpp:151-242, unchanged per-bin recurrence order.
  // On this ARM target the scalar state stays in registers. Four-lane grouping
  // introduced uncached stack traffic despite sharing its PCM sample loads.
  output = {};
  for (std::size_t bin = 0; bin < frequencies.size(); ++bin) {
    const auto& frequency = frequencies[bin];
    if (frequency.target_frequency_hz > static_cast<float>(kInhouseSampleRate) * 0.5F)
      continue;
    ++output.nyquist_safe_bin_high;
    InhouseLane lane{frequency.coefficient_q14, frequency.block_size, 0, 0};
    for (std::size_t age = 0; age < lane.block_size; ++age)
      stepInhouseLane(lane, window[window.size() - 1U - age], output.q0_overflow_count);
    const auto term = (static_cast<std::int64_t>(lane.coefficient_q14) * lane.q1) >> 14;
    auto& result = output.bins[bin];
    result.q1 = lane.q1;
    result.q2 = lane.q2;
    result.magnitude_squared = std::max<std::int64_t>(0,
        static_cast<std::int64_t>(lane.q2) * lane.q2 +
        static_cast<std::int64_t>(lane.q1) * lane.q1 - term * lane.q2);
    result.magnitude = static_cast<float>(static_cast<std::int32_t>(
        std::sqrt(static_cast<float>(result.magnitude_squared))));
    result.normalised_magnitude = result.magnitude * frequency.inverse_block_size_half;
  }
}
}  // namespace

bool InhouseAudio::configure(const InhouseAudioConfig& config) noexcept {
  if (!std::isfinite(config.drive_floor_pcm) || config.drive_floor_pcm < 0.0F ||
      config.drive_floor_pcm > 32767.0F ||
      !std::isfinite(config.follower_floor_pcm) || config.follower_floor_pcm < 1.0F ||
      config.follower_floor_pcm > 32767.0F ||
      !std::isfinite(config.vu_floor) || config.vu_floor < 0.0F || config.vu_floor > 0.99F ||
      !std::isfinite(config.mood) || config.mood < 0.0F || config.mood > 1.0F ||
      !std::isfinite(config.silence_rms_enter) || config.silence_rms_enter < 0.0F ||
      !std::isfinite(config.silence_rms_exit) ||
      config.silence_rms_exit < config.silence_rms_enter || config.silence_rms_exit > 1.0F ||
      config.reference_presentation_hz == 0U) return false;
  config_ = config;
  for (auto& state : conditioning_) state.mood = config.mood;
  reset();
  return true;
}

bool InhouseAudio::setMood(unsigned board, float mood) noexcept {
  if (board >= conditioning_.size() || !std::isfinite(mood) || mood < 0.0F || mood > 1.0F)
    return false;
  auto& state = conditioning_[board];
  if (state.mood == mood && state.low_pass_alpha != 0.0F) return true;
  state.mood = mood;
  // Source utilities.h low_pass_filter accepts a float cutoff but evaluates
  // its exponent with double PI. Preserve that coefficient at CONTROL rate;
  // The per-sample recurrence remains integer. The filter mix below preserves
  // its separate inherited rounding boundary and is covered by the oracle.
  constexpr double pi = 3.14159265358979323846;
  const float cutoff = static_cast<float>(1.0 + 10.0 * static_cast<double>(mood));
  const float exponent = static_cast<float>(-2.0 * pi * static_cast<double>(cutoff) /
      static_cast<double>(config_.reference_presentation_hz));
  state.low_pass_alpha = 1.0F - std::exp(exponent);
  return true;
}

void InhouseAudio::setTempoDemand(unsigned board, bool enabled) noexcept {
  if (board >= tempo_.size() || tempo_demand_[board] == enabled) return;
  tempo_demand_[board] = enabled;
  tempo_capture_us_[board] = 0;
  if (enabled)
    core::audio::initialiseTempoTracker(tempo_[board], core::audio::kLegacy12800AudioRate);
  else core::audio::resetTempoTracker(tempo_[board]);
}

void InhouseAudio::reset() noexcept {
  frequencies_ = core::audio::makeGdftConfiguration(core::audio::kLegacy12800AudioRate);
  window_ = {};
  raw_ = {};
  pending_ = {};
  magnitude_average_ = {};
  for (unsigned board = 0; board < conditioning_.size(); ++board) {
    const float mood = conditioning_[board].mood;
    conditioning_[board] = {};
    (void)setMood(board, mood);
    core::audio::resetTempoTracker(tempo_[board]);
  }
  tempo_capture_us_ = {};
  pcm_history_ = {};
  pcm_history_index_ = pcm_history_count_ = 0;
  previous_vu_ = 0.0F;
  peaks_ = {};
  publication_ = {};
  // Publication generations never repeat across a gap within one capture epoch.
  epoch_ = expected_begin_ = admitted_samples_ = quiet_samples_ = 0;
  pending_count_ = support_samples_ = 0;
  for (const auto& bin : frequencies_) {
    if (bin.target_frequency_hz <= static_cast<float>(kInhouseSampleRate) * 0.5F)
      support_samples_ = std::max(support_samples_, static_cast<std::size_t>(bin.block_size));
  }
  follower_ = config_.follower_floor_pcm;
  peak_scaled_ = previous_rms_ = 0.0F;
  rms_silent_ = silence_ = unstamped_ = false;
}

bool InhouseAudio::consume(const k1_audio_hop_t& hop, std::uint64_t capture_us,
                           bool capture_valid) noexcept {
  // Defence in depth for host replay; live admission still has one owner.
  if (hop.contract_version != 2U || hop.samples != 64U ||
      hop.analysis_rate_hz != kInhouseSampleRate || hop.stream_epoch == 0U ||
      hop.hop_sequence == 0U || hop.hop_sequence > UINT64_MAX / 64U ||
      hop.analysis_begin != (hop.hop_sequence - 1U) * 64U ||
      hop.analysis_end_exclusive != hop.hop_sequence * 64U || hop.discontinuity_reason != 0U) {
    reset();
    return false;
  }
  if (epoch_ == hop.stream_epoch && hop.analysis_begin < expected_begin_) return false;
  if (epoch_ == hop.stream_epoch && expected_begin_ != hop.analysis_begin) {
    reset();
    return false;
  }
  if (epoch_ != 0U && epoch_ != hop.stream_epoch) reset();
  if (epoch_ == 0U) {
    epoch_ = hop.stream_epoch;
    expected_begin_ = hop.analysis_begin;
  }
  bool completed = false;
  for (std::size_t i = 0; i < hop.samples; ++i) {
    pending_[pending_count_++] = hop.pcm[i];
    if (pending_count_ == kInhouseHopSamples) {
      const std::uint64_t tail = hop.samples - 1U - i;
      const std::uint64_t tail_us = tail * 1000000U / kInhouseSampleRate;
      const bool stamp_valid = capture_valid && capture_us >= tail_us;
      analyse(hop.analysis_begin + i + 1U,
              stamp_valid ? capture_us - tail_us : 0U, stamp_valid);
      pending_count_ = 0;
      completed = true;
    }
  }
  expected_begin_ = hop.analysis_end_exclusive;
  return completed;
}

void InhouseAudio::analyse(std::uint64_t end_sample, std::uint64_t capture_us,
                           bool capture_valid) noexcept {
  std::memmove(window_.data(), window_.data() + kInhouseHopSamples,
      (window_.size() - kInhouseHopSamples) * sizeof(window_[0]));
  std::memcpy(window_.data() + window_.size() - kInhouseHopSamples,
      pending_.data(), kInhouseHopSamples * sizeof(pending_[0]));
  admitted_samples_ += kInhouseHopSamples;
  ++generation_;
  publication_ = {};
  publication_.epoch = epoch_;
  publication_.generation = generation_;
  publication_.analysis_begin_sample = end_sample - kInhouseHopSamples;
  publication_.analysis_end_sample_exclusive = end_sample;
  publication_.capture_us = capture_us;
  publication_.valid = capture_valid && admitted_samples_ >= support_samples_;
  publication_.raw_floor_pcm = config_.drive_floor_pcm;
  // Source i2s_audio.h increments the four-row physical ring before copying
  // each completed 96-sample frame. Mode 11 sums all four rows, not a waveform
  // invented from spectrum bins. Never join partial frames across an epoch.
  pcm_history_index_ = (pcm_history_index_ + 1U) % pcm_history_.size();
  pcm_history_[pcm_history_index_] = pending_;
  pcm_history_count_ = std::min(pcm_history_count_ + 1U, 4U);
  publication_.pcm_history = pcm_history_;
  publication_.pcm_history_valid = pcm_history_count_ == 4U;

  float square_sum = 0.0F;
  for (const auto sample : pending_) {
    publication_.raw_peak_pcm = std::max(publication_.raw_peak_pcm,
        std::fabs(static_cast<float>(sample)));
    // Source waveform_fixed_point = SQ15x16(sample) * SQ15x16(1/32768).
    const auto normalised_q16 = static_cast<std::int32_t>(sample) * 2;
    square_sum += f(mul(normalised_q16, normalised_q16));
  }
  const float rms = f(q(std::sqrt(square_sum / static_cast<float>(kInhouseHopSamples))));
  publication_.raw_rms = rms;
  publication_.quiet_entry_rms = config_.silence_rms_enter;
  publication_.vu_level = f(divq(std::max<std::int32_t>(0, q(rms) - q(config_.vu_floor)),
      q(1.0F - config_.vu_floor)));
  publication_.vu_average = f((q(publication_.vu_level) + q(previous_vu_)) / 2);
  previous_vu_ = publication_.vu_level;
  const float drive = std::max(0.0F, publication_.raw_peak_pcm - config_.drive_floor_pcm);
  // Source's first dt-clock call uses 10 ms; later calls use sample chronology.
  const float dt = admitted_samples_ == kInhouseHopSamples ? 0.010F : kDt;
  follower_ += (drive - follower_) * alpha(drive > follower_ ? 0.25F : 0.005F, dt);
  follower_ = std::max(config_.follower_floor_pcm, follower_);
  const float ratio = drive / follower_;
  peak_scaled_ += (ratio - peak_scaled_) * alpha(ratio > peak_scaled_ ? 0.65F : 0.15F, dt);
  publication_.peak_scaled = peak_scaled_;
  // Production SPH source uses the previous calculate_vu() RMS at this stage.
  rms_silent_ = previous_rms_ < (rms_silent_ ? config_.silence_rms_exit : config_.silence_rms_enter);
  if (rms_silent_) {
    quiet_samples_ += kInhouseHopSamples;
    if (quiet_samples_ * 1000U >= static_cast<std::uint64_t>(config_.silence_dwell_ms) * kInhouseSampleRate)
      silence_ = true;
  } else {
    quiet_samples_ = 0;
    silence_ = false;
  }
  previous_rms_ = rms;
  publication_.silence = silence_;

  // Source k1_gdft_core.cpp INT64_RECURRENCE + INT64_MAGNITUDE, Cross40,
  // rectangular windows. The parked native kernel deliberately wraps int32
  // products, so only its configuration generator is reused here.
  analyseInhouseScalar(window_, frequencies_, raw_);
  for (std::size_t bin = 0; bin < frequencies_.size(); ++bin) {
    if (bin < raw_.nyquist_safe_bin_high) {
      magnitude_average_[bin] = raw_.bins[bin].normalised_magnitude * 0.3F + magnitude_average_[bin] * 0.7F;
    } else {
      magnitude_average_[bin] = 0.0F;
    }
  }
  for (unsigned board = 0; board < conditioning_.size(); ++board) {
    condition(conditioning_[board], dt);
    if (tempo_demand_[board]) {
      if (!capture_valid || raw_.q0_overflow_count != 0U) {
        // No phase continuity through an unclocked/rejected analysis. Repeated
        // invalid input has no live tempo state to reset a second time.
        if (tempo_[board].input_primed) core::audio::resetTempoTracker(tempo_[board]);
        tempo_capture_us_[board] = 0;
        continue;
      }
      // k1_tempo_update receives one source novelty/silence observation per
      // 96-sample AP frame. The existing V2 tracker owns its 3-frame peak hold,
      // actual BPM/phase and silence-attenuated confidence; no synthetic beat.
      // Do not mark the optional native TempoField evidence valid: that API
      // uses canonical 48 kHz frames. Our 12.8 kHz chronology stays in this
      // publication's epoch/sample interval and actual tempo capture stamp.
      core::audio::updateTempoTracker(tempo_[board], {
          static_cast<std::uint32_t>(capture_us / 1000U), conditioning_[board].novelty,
          silence_});
      if (core::audio::readTempoTracker(tempo_[board]).updated)
        tempo_capture_us_[board] = capture_us;
    }
  }
  publication_.spectrum = conditioning_[0].spectrum;
  publication_.spectral_energy = conditioning_[0].spectral_energy;
  publication_.novelty = conditioning_[0].novelty;
  if (raw_.q0_overflow_count != 0U) publication_.valid = false;
  peaks_[generation_ % peaks_.size()] = Peak{generation_, peak_scaled_};
  unstamped_ = true;
}

void InhouseAudio::condition(Conditioning& state, float dt) noexcept {
  // This source LPF lives in process_GDFT, so it advances on every 96-sample
  // generation even though its coefficient uses reconstructed SYSTEM_FPS=100.
  // Keep that source quirk explicit. Each edge owns its Mood and downstream
  // AGC history; changing the peer cannot recondition already emitted colour.
  float signal_sum = 0.0F;
  for (std::size_t bin = 0; bin < frequencies_.size(); ++bin) {
    // Source utilities.h promotes only the first term through its 1.0 literal.
    // Keep its final float rounding; replacing this with an all-float mix is a
    // separately measured optimisation, not silently "identical" arithmetic.
    state.magnitude_last[bin] = static_cast<float>(
        (1.0 - static_cast<double>(state.low_pass_alpha)) *
        static_cast<double>(state.magnitude_last[bin]) +
        static_cast<double>(state.low_pass_alpha * magnitude_average_[bin]));
    signal_sum += state.magnitude_last[bin];
  }
  const auto signal_q16 = q(signal_sum / 80.0F);
  state.envelope_q16 += mul(signal_q16 - state.envelope_q16,
      q(alpha(signal_q16 > state.envelope_q16 ? 0.28F : 0.02F, dt)));
  if (state.envelope_q16 < mul(state.noise_q16, q(2.0F)))
    state.noise_q16 += mul(state.envelope_q16 - state.noise_q16, q(alpha(0.001F, dt)));
  state.noise_q16 = std::max(state.noise_q16, q(0.001F));
  if (state.gated && state.envelope_q16 > mul(state.noise_q16, q(4.0F))) state.gated = false;
  if (!state.gated && state.envelope_q16 < mul(state.noise_q16, q(2.5F))) state.gated = true;
  if (!state.gated) {
    const auto target = std::clamp(divq(q(0.25F), state.envelope_q16 + q(0.001F)), q(0.1F), q(10.0F));
    state.gain_q16 += mul(target - state.gain_q16, q(alpha(0.05F, dt)));
  }
  std::int32_t novelty_sum = 0;
  float low_sum = 0.0F, mid_sum = 0.0F, high_sum = 0.0F;
  for (std::size_t bin = 0; bin < frequencies_.size(); ++bin) {
    const float hz = frequencies_[bin].target_frequency_hz;
    const auto tilt = q(hz < 200.0F ? 1.30F : (hz > 3000.0F ? 0.85F : 1.0F));
    auto value = mul(mul(q(state.magnitude_last[bin]), state.gain_q16), tilt);
    if (value > q(0.5F)) {
      const auto excess = value - q(0.5F);
      value = q(0.5F) + divq(excess, kOne + excess);
    }
    const auto current = std::clamp<std::int32_t>(value, 0, kOne);
    novelty_sum += std::max<std::int32_t>(0, current - q(state.spectrum[bin]));
    state.spectrum[bin] = f(current);
    // k1_audio_snapshot.cpp retains three separate sums before their final add.
    if (bin < 80U / 3U) low_sum += state.spectrum[bin];
    else if (bin < 80U * 2U / 3U) mid_sum += state.spectrum[bin];
    else high_sum += state.spectrum[bin];
  }
  state.spectral_energy = (low_sum + mid_sum + high_sum) / 80.0F;
  // Source calculate_novelty: positive SQ15x16 difference, average then sqrt,
  // stored back into SQ15x16. Only the preceding row is consumed here.
  state.novelty = f(q(std::sqrt(f(novelty_sum / 80))));
}

void InhouseAudio::stampPublication(std::uint64_t publish_us) noexcept {
  if (!unstamped_) return;
  publication_.publish_us = publish_us;
  if (publication_.capture_us > publish_us) publication_.valid = false;
  unstamped_ = false;
}

bool InhouseAudio::acquire(InhouseAudioCursor& cursor, std::uint64_t now_us,
                          std::uint64_t stale_us, core::visual::InhouseWaveformAudio& out,
                          unsigned board) const noexcept {
  out = publication_;
  if (board >= conditioning_.size()) { out.valid = false; return false; }
  out.spectrum = conditioning_[board].spectrum;
  out.spectral_energy = conditioning_[board].spectral_energy;
  out.novelty = conditioning_[board].novelty;
  const auto tempo = core::audio::readTempoTracker(tempo_[board]);
  out.tempo_bpm = tempo.bpm;
  out.tempo_phase01 = tempo.phase01;
  out.tempo_confidence = tempo.confidence;
  out.tempo_capture_us = tempo_capture_us_[board];
  // A held tracker event may precede the latest fresh audio publication by one
  // 22.5 ms update period. Subtract the period rather than adding to stale_us
  // so even an unrestricted caller allowance cannot overflow.
  const auto tempo_age = now_us >= out.tempo_capture_us ? now_us - out.tempo_capture_us : 0U;
  out.tempo_valid = out.valid && !unstamped_ && tempo_demand_[board] && out.tempo_capture_us != 0U &&
      now_us >= out.tempo_capture_us &&
      (tempo_age <= core::visual::kInhouseTempoUpdatePeriodUs ||
       tempo_age - core::visual::kInhouseTempoUpdatePeriodUs <= stale_us);
  out.observed_peak_scaled = 0.0F;
  out.peak_summary_valid = false;
  if (unstamped_ || !out.valid || now_us < out.publish_us || now_us - out.publish_us > stale_us ||
      now_us < out.capture_us || now_us - out.capture_us > stale_us) {
    out.valid = false;
    out.tempo_valid = false;
    cursor = {epoch_, generation_};
    return false;
  }
  if (cursor.epoch != epoch_) cursor = {epoch_, generation_ > 0U ? generation_ - 1U : 0U};
  if (cursor.generation > generation_ || generation_ - cursor.generation > peaks_.size()) {
    // Reader missed the bounded summary: one invalid read resynchronises it,
    // rather than falsely announcing a complete attack history.
    out.valid = false;
    out.tempo_valid = false;
    cursor = {epoch_, generation_};
    return false;
  }
  if (cursor.generation < generation_) {
    out.peak_summary_valid = true;
    for (std::uint64_t gen = cursor.generation + 1U; gen <= generation_; ++gen) {
      const auto& item = peaks_[gen % peaks_.size()];
      if (item.generation != gen) { out.valid = false; out.peak_summary_valid = false; break; }
      out.observed_peak_scaled = std::max(out.observed_peak_scaled, item.peak);
    }
  }
  cursor = {epoch_, generation_};
  out.tempo_valid = out.tempo_valid && out.valid;
  return out.valid;
}

}  // namespace k1::titan
