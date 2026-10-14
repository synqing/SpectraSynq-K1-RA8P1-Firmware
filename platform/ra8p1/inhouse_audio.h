#pragma once

#include <array>
#include <cstdint>
#include "core/audio/gdft_goertzel.h"
#include "core/audio/tempo_tracker.h"
#include "core/visual/inhouse_waveform.h"
#include "k1/core/audio/k1_audio_hop.h"

namespace k1::titan {

inline constexpr std::uint32_t kInhouseSampleRate = 12800U;
inline constexpr std::size_t kInhouseHopSamples = 96U;
inline constexpr std::size_t kInhousePeakHistory = 8U;
static_assert(kInhouseSampleRate == core::audio::kLegacy12800AudioRate.sample_rate_hz);
static_assert(kInhouseHopSamples == core::audio::kLegacy12800AudioRate.hop_samples);
// Lane P4: optional 24 kHz / 180 analysis input (k1_poly24 output). Same 7.5 ms
// hop, so every time constant, the tempo cadence and the publication rate hold.
inline constexpr std::uint32_t kInhouseAnalysis24Rate = 24000U;
inline constexpr std::size_t kInhouseAnalysis24Hop = 180U;
inline constexpr std::size_t kInhouseMaxHopSamples = kInhouseAnalysis24Hop;
static_assert(kInhouseAnalysis24Rate == core::audio::kProductionAudioRate.sample_rate_hz);
static_assert(kInhouseAnalysis24Hop == core::audio::kProductionAudioRate.hop_samples);
static_assert(kInhouseAnalysis24Hop * kInhouseSampleRate == kInhouseHopSamples * kInhouseAnalysis24Rate);

// Explicit reconstruction settings, NOT a transferred microphone calibration.
// Source HEAD 96e6839: globals/config_types, i2s_audio, k1_agc_dt_clock,
// k1_gdft_core. Static spectral-noise subtraction is disabled. Loud trim is
// unity; the source's separate loud-room controller has not been transplanted.
struct InhouseAudioConfig final {
  float drive_floor_pcm = 350.0F;
  float follower_floor_pcm = 350.0F;
  float vu_floor = 0.0F;
  float mood = 0.05F;
  float silence_rms_enter = core::visual::kInhouseQuietEntryRms;
  float silence_rms_exit = 0.08F;
  std::uint32_t silence_dwell_ms = 5000U;
  // SYSTEM_FPS was a visual clock in the source. Explicit 100 Hz reconstructed
  // presentation profile, not the 133.333 Hz audio cadence.
  std::uint32_t reference_presentation_hz = 100U;
  // 12800: consumes the donor's 64-sample 12.8 kHz descriptors (96-sample frames).
  // 24000: consumes 180-sample 24 kHz descriptors from k1_poly24 (one frame each).
  std::uint32_t analysis_rate_hz = kInhouseSampleRate;
};

struct InhouseAudioCursor final {
  std::uint64_t epoch = 0;
  std::uint64_t generation = 0;
};

class InhouseAudio final {
 public:
  void reset() noexcept;
  [[nodiscard]] bool configure(const InhouseAudioConfig& config) noexcept;
  [[nodiscard]] bool setMood(unsigned board, float mood) noexcept;
  void setTempoDemand(unsigned board, bool enabled) noexcept;
  [[nodiscard]] float mood(unsigned board) const noexcept {
    return board < conditioning_.size() ? conditioning_[board].mood : 0.0F;
  }
  // Called ONLY after LiveAudioRuntime admits this same descriptor to the donor.
  // capture_us is the descriptor's newest-sample estimate in the live clock.
  // A 96 boundary inside a 64 descriptor subtracts its exact 32-sample tail.
  [[nodiscard]] bool consume(const k1_audio_hop_t& hop,
      std::uint64_t capture_us, bool capture_valid) noexcept;
  void stampPublication(std::uint64_t publish_us) noexcept;
  [[nodiscard]] bool acquire(InhouseAudioCursor& cursor, std::uint64_t now_us,
      std::uint64_t stale_us, core::visual::InhouseWaveformAudio& out, unsigned board = 0U) const noexcept;
  [[nodiscard]] const core::visual::InhouseWaveformAudio& latest() const noexcept {
    return publication_;
  }
  [[nodiscard]] const core::audio::GdftConfiguration& configuration() const noexcept {
    return frequencies_;
  }
  [[nodiscard]] const core::audio::GdftRawFrame& rawSpectrum() const noexcept {
    return raw_;
  }
  [[nodiscard]] const InhouseAudioConfig& config() const noexcept { return config_; }
  [[nodiscard]] float rawRms() const noexcept { return previous_rms_; }
  [[nodiscard]] const std::array<float, 80>& magnitudeFinal(unsigned board = 0U) const noexcept {
    return conditioning_[board < conditioning_.size() ? board : 0U].magnitude_last;
  }

 private:
  void analyse(std::uint64_t end_sample, std::uint64_t capture_us,
      bool capture_valid) noexcept;
  struct Peak final { std::uint64_t generation = 0; float peak = 0.0F; };
  struct Conditioning final {
    std::array<float, 80> magnitude_last{};
    std::array<float, 80> spectrum{};
    float spectral_energy = 0.0F;
    float novelty = 0.0F;
    float mood = 0.05F;
    float low_pass_alpha = 0.0F;
    std::int32_t envelope_q16 = 0;
    std::int32_t noise_q16 = 65;
    std::int32_t gain_q16 = 65536;
    bool gated = true;
  };
  void condition(Conditioning& state, float dt) noexcept;
  InhouseAudioConfig config_{};
  core::audio::GdftConfiguration frequencies_{};
  core::audio::GdftSampleWindow window_{};
  core::audio::GdftRawFrame raw_{};
  std::array<std::int16_t, kInhouseMaxHopSamples> pending_{};
  std::array<float, 80> magnitude_average_{};
  std::array<Conditioning, 2> conditioning_{};
  std::array<core::audio::TempoTrackerState, 2> tempo_{};
  std::array<bool, 2> tempo_demand_{};
  std::array<std::uint64_t, 2> tempo_capture_us_{};
  std::array<std::array<std::int16_t, kInhouseHopSamples>, 4> pcm_history_{};
  unsigned pcm_history_index_ = 0;
  unsigned pcm_history_count_ = 0;
  std::array<Peak, kInhousePeakHistory> peaks_{};
  core::visual::InhouseWaveformAudio publication_{};
  std::uint64_t epoch_ = 0;
  std::uint64_t expected_begin_ = 0;
  std::uint64_t generation_ = 0;
  std::uint64_t admitted_samples_ = 0;
  std::uint64_t quiet_samples_ = 0;
  std::size_t pending_count_ = 0;
  std::size_t support_samples_ = 0;
  std::uint32_t rate_hz_ = kInhouseSampleRate;
  std::size_t hop_ = kInhouseHopSamples;
  std::uint32_t input_samples_ = 64U;
  float follower_ = 350.0F;
  float peak_scaled_ = 0.0F;
  float previous_rms_ = 0.0F;
  float previous_vu_ = 0.0F;
  bool rms_silent_ = false;
  bool silence_ = false;
  bool unstamped_ = false;
};

}  // namespace k1::titan
