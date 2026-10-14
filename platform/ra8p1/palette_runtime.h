#pragma once
#include <cstddef>
#include <cstdint>
#include "core/visual/channel_render_state.h"
#include "core/visual/visual_audio_frame.h"
#include "core/visual/ws2816_pack.h"
#if defined(K1_TITAN_TOP_OUTPUT) && defined(K1_EMOTISCOPE_PORT)
#include "emotiscope_port.h"
#include "composed_scene_runtime.h"
#include "core/visual/inhouse_waveform.h"
#if defined(K1_V9_RELEASE_PUPPET)
#include "core/visual/v9_release_puppet.h"
#endif
#endif
#ifdef K1_PALETTE_MORPH
#include "palette_transition.h"
#include "centre_palette_engine.h"
#endif

namespace k1::titan {
#ifndef K1_LIVE_BOOT_BRIGHTNESS
#define K1_LIVE_BOOT_BRIGHTNESS 100U
#endif
// User directive 29 September 2026: normal boot default is at least 100/255.
inline constexpr std::uint32_t kDefaultPaletteBrightness = K1_LIVE_BOOT_BRIGHTNESS;
static_assert(kDefaultPaletteBrightness >= 100U && kDefaultPaletteBrightness <= 255U);

inline constexpr std::uint32_t kPaletteCatalogueOpcode = 15U;
inline constexpr std::uint32_t kPaletteConfigureOpcode = 16U;
inline constexpr std::uint32_t kPaletteStatusOpcode = 17U;
inline constexpr std::uint32_t kPaletteFrameOpcode = 18U;
inline constexpr std::uint32_t kPaletteWireSnapshotOpcode = 19U; // WS2816 only; last submission, not photons.
#if defined(K1_PALETTE_GPT_DMA) && defined(K1_PALETTE_WS2816)
#error "K1_PALETTE_GPT_DMA and K1_PALETTE_WS2816 are mutually exclusive"
#endif
#if defined(K1_PALETTE_WS2816_GPT_PAIR) && defined(K1_PALETTE_GPT_DMA)
#error "K1_PALETTE_WS2816_GPT_PAIR and K1_PALETTE_GPT_DMA are mutually exclusive"
#endif
#if defined(K1_PALETTE_WS2816_GPT_PAIR) && defined(K1_PALETTE_WS2816)
#error "K1_PALETTE_WS2816_GPT_PAIR and K1_PALETTE_WS2816 are mutually exclusive"
#endif
#if defined(K1_EMOTISCOPE_PORT)
inline constexpr std::uint32_t kPalettePeriodUs = 10000U;
#elif defined(K1_DUAL_EDGE_DONOR) || defined(K1_PALETTE_WS2816)
// Keep the donor audition at 60 Hz. Its two DINs now emit 80 pixels each
// concurrently; donor selection does not imply two physical strips. The
// legacy GPIO backend sends two 80-pixel segments sequentially.
inline constexpr std::uint32_t kPalettePeriodUs = 16667U;
#else
inline constexpr std::uint32_t kPalettePeriodUs = 8333U;
#endif
// K1 SILENCE_DWELL_MS (5 s). PCM SENSITIVITY lives in k1_pdm_sensitivity.h;
// do not also scale peak here.
inline constexpr std::uint64_t kTitanSilenceDwellUs = 5000000U;
// Elapsed-time dwell aging. Pixel8×0.99875 with a min-1 floor stuck at 1.
inline constexpr float kTitanDwellTauS = 1.17F;
inline bool k1MusicalPresence(const k1::contract::AudioFeaturesV1& features) noexcept {
  // Quiet-room chroma sits near 0.1. Do not treat it as music.
  // This arms the 5 s keep-alive window. It must not chop the renderer.
  return features.peak_scaled > 0.25F || features.vu_level > 0.25F;
}
inline void applyK1PresencePolicy(k1::contract::AudioFeaturesV1& features,
                                  std::uint64_t now_us,
                                  std::uint64_t& last_live_us,
                                  bool& last_live_valid) noexcept {
  if (k1MusicalPresence(features)) {
    last_live_us = now_us;
    last_live_valid = true;
  }
  if (last_live_valid && now_us - last_live_us < kTitanSilenceDwellUs) {
    features.event_flags &= ~k1::contract::kEventSilence;
  }
}
inline constexpr std::uint32_t kDiagnosticBounceMode = 64U;
inline constexpr std::uint32_t kLiveAudioBootMode = 32U; /* WAVEFORM_HYBRID_K1 */
// Compile-time backend only. DMA/emit counters are observed activity, not wire
// admission. last_emit_cycles is emitter DWT, not AP hop_max_us.
inline const char* configuredPaletteBackend() noexcept {
#if defined(K1_PALETTE_WS2816_GPT_PAIR)
  return "ws2816_gpt_pair";
#elif defined(K1_PALETTE_GPT_DMA)
  return "gpt_dma";
#elif defined(K1_PALETTE_WS2816)
  return "ws2816_gpio";
#elif defined(K1_PALETTE_RUNTIME)
  return "gpio_diagnostic";
#else
  return "unknown";
#endif
}
inline const char* runtimePaletteOutputBackend(bool emit_enabled) noexcept {
  return emit_enabled ? configuredPaletteBackend() : "disabled";
}
inline constexpr std::uint32_t kDiagnosticBounceFrameUs = 33333U;
inline constexpr std::uint32_t kDiagnosticBounceSteps = 127U;
// Flags: active, automatic catalogue cycle, physical bench output.
struct PaletteConfig {
  std::uint32_t version = 1U;
  std::uint32_t palette_a = 0U, palette_b = 1U;
  std::uint32_t mode_a = 0U, mode_b = 0U; // 0 = preview; 64 = palette bounce.
  std::uint32_t flags = 0U, brightness = kDefaultPaletteBrightness, output_channel = 0U;
  std::uint32_t transition_ms = 0U; // Versions 2/3; 0 is an immediate cut.
  std::uint32_t travel_ms = 4000U; // Version 3: centre-to-edge travel duration.
};
class PaletteRuntime {
 public:
  PaletteRuntime() = default;
  PaletteRuntime(const PaletteRuntime&) = delete;
  PaletteRuntime& operator=(const PaletteRuntime&) = delete;
  bool configure(const PaletteConfig& config, std::uint64_t now_us) noexcept;
  void setHardwareBrightness(std::uint32_t brightness) noexcept;
  // Physical DIN attenuation after global brightness; owner applies before packing.
  bool setOutputIntensity(unsigned lane, float intensity) noexcept;
  float outputIntensity(unsigned lane) const noexcept;
#ifdef K1_TITAN_TOP_OUTPUT
#ifdef K1_EMOTISCOPE_PORT
  void setInhouseAudio(unsigned board,const core::visual::InhouseWaveformAudio& audio) noexcept {
    if(board<2U)inhouse_audio_[board]=audio;
  }
  bool setInhouseParameters(unsigned board, float sensitivity, float square_iter, float photons) noexcept;
  core::visual::InhouseWaveformControls inhouseParameters(unsigned board) const noexcept {
    return board<2U?inhouse_controls_[board]:core::visual::InhouseWaveformControls{};
  }
  bool inhouseAdmissionOpen(unsigned board) const noexcept {
    return board<2U && k1_emotiscope_is_inhouse_mode(board?config_.mode_b:config_.mode_a) &&
        inhouse_renderer_[board].observation().input_fresh && !inhouse_observed_audio_[board].silence;
  }
  bool inhouseSourceSilence(unsigned board) const noexcept {
    return board<2U?inhouse_source_silence_[board]:true;
  }
  std::uint32_t inhouseDonorSequence() const noexcept { return donor_publication_.sequence; }
  float inhouseDonorPeakSquared() const noexcept { return donor_publication_.pcm_peak_squared; }
#endif
  bool setBoardIntensity(unsigned board, float intensity) noexcept;
  float boardIntensity(unsigned board) const noexcept;
  void setSecondaryOutputEnabled(bool enabled) noexcept { secondary_output_enabled_ = enabled; }
  bool secondaryOutputEnabled() const noexcept { return secondary_output_enabled_; }
  std::uint64_t lastBoardFrameUs(unsigned board) const noexcept {return board<2U?board_frame_us_[board]:0U;}
  bool lastBoardRenderValid(unsigned board) const noexcept {
    return board<2U && board_frame_valid_[board];
  }
  std::size_t packBoardGrb48Lane(std::uint8_t* destination, std::size_t capacity,
                               unsigned board, unsigned lane) const noexcept;
#endif
  bool setEmissionEnabled(bool enabled) noexcept;
  void noteDonorMode(std::uint32_t mode, unsigned board = 0U) noexcept;
  bool frameDue(std::uint64_t now_us) const noexcept { return active() && now_us>=next_us_; }
  bool step(std::uint64_t now_us,
            const core::visual::VisualAudioFrameView* audio) noexcept;
  void selectPerformanceMode(std::uint32_t mode) noexcept;
  void resetAudioState() noexcept;
  void stop() noexcept { config_.flags = 0U; }
  bool active() const noexcept { return (config_.flags & 1U) != 0U; }
  bool emitEnabled() const noexcept { return (config_.flags & 4U) != 0U; }
  // Owner-only A/B output precision: 0 = reference RGB8 (boot), 1 = 16-bit
  // candidate. Takes effect at the next rendered frame. Not part of any scene
  // or defaults restore; a reboot returns to 0.
  void setOutputPrecision(std::uint32_t precision) noexcept { output_precision_ = precision ? 1U : 0U; }
  std::uint32_t outputPrecision() const noexcept { return output_precision_; }
  const PaletteConfig& config() const noexcept { return config_; }
  core::visual::ChannelRenderState& channel(unsigned i) noexcept {
    return i ? b_ : a_;
  }
  const core::visual::ChannelRenderState& channel(unsigned i) const noexcept {
    return i ? b_ : a_;
  }
  std::size_t packBenchGrb(std::uint8_t* destination, std::size_t capacity,
                         unsigned pixels = 128U) const noexcept;
  // Stage a 160-pixel TRUE16 channel for packBenchGrb48Lane. RGB8×257 is not
  // TRUE16 and must not be used to synthesise these values.
  bool stageWideFrame(unsigned channel_index, const core::visual::Pixel16* pixels,
                      std::size_t count) noexcept;
  // Pack one DIN lane (80 pixels) from the staged Pixel16 channel. Preserves
  // low bytes (0x12AB / 0x12AC / 0x1200). Returns 0 if wide frame is unset.
  std::size_t packBenchGrb48Lane(std::uint8_t* destination, std::size_t capacity,
                               unsigned lane) const noexcept;
  std::size_t catalogueJson(char* out, std::size_t capacity) const noexcept;
  std::size_t statusJson(char* out, std::size_t capacity) const noexcept;
  // Owner-only same-mode palette selection; retain audio/presence/history clocks.
  void selectPerformancePalette(std::uint32_t id, std::uint64_t now_us) noexcept;
  void recordEmit(int result, std::uint32_t cycles) noexcept;
  const char* visualPath() const noexcept { return visual_path_ ? visual_path_ : "none"; }
  std::uint64_t lastPublicationUs() const noexcept { return last_publication_us_; }
  std::uint64_t skippedReleases() const noexcept { return skipped_; }
  std::uint64_t frames() const noexcept { return frames_; }
 private:
  core::visual::ChannelRenderState a_{core::visual::PixelChannelId::kChannelA};
  core::visual::ChannelRenderState b_{core::visual::PixelChannelId::kChannelB};
  PaletteConfig config_{};
#ifdef K1_PALETTE_MORPH
  core::visual::PaletteTransition transitions_[2]{};
#endif
  std::uint64_t next_us_ = 0U, last_us_ = 0U, cycle_start_us_ = 0U,
      last_live_us_ = 0U, last_publication_us_ = 0U;
  bool last_live_valid_ = false;
  std::uint32_t last_presence_sequence_ = 0U;
  std::uint64_t frames_ = 0U, skipped_ = 0U, emitted_ = 0U, emit_errors_ = 0U;
  std::uint32_t last_emit_cycles_ = 0U, maximum_emit_cycles_ = 0U;
  bool waiting_for_audio_ = false;
  std::uint16_t dwell_q8_[2][160][3]{};
  bool dwell_armed_[2]{};
  const char* visual_path_ = "none";
  bool last_musical_ = false;
  bool last_in_dwell_ = false;
  bool last_dwell_reinit_ = false;
  std::uint64_t last_live_age_us_ = 0U;
  std::uint32_t last_peak_milli_ = 0U;
  std::uint32_t last_vu_milli_ = 0U;
  std::uint32_t last_chroma_milli_ = 0U;
  std::uint32_t last_wave_milli_ = 0U;
  std::uint64_t effect_frames_ = 0U;
  std::uint64_t dwell_frames_ = 0U;
  std::uint64_t dwell_reinits_ = 0U;
  core::visual::Pixel16 wide_[2][core::visual::kPixelsPerChannel]{};
  bool wide_valid_[2]{};
  std::uint32_t output_precision_ = 0U;
  float output_intensity_[2]{1.0F, 1.0F};
  std::uint16_t output_intensity_q16_[2]{65535U, 65535U};
#ifdef K1_TITAN_TOP_OUTPUT
#ifdef K1_EMOTISCOPE_PORT
  core::visual::InhouseWaveformRenderer inhouse_renderer_[2]{};
  core::visual::InhouseWaveformControls inhouse_controls_[2]{};
  composed_scene::BloomAdmission inhouse_admission_[2]{};
  core::visual::InhouseWaveformAudio inhouse_audio_[2]{};
  core::visual::InhouseWaveformAudio inhouse_observed_audio_[2]{};
  bool inhouse_source_silence_[2]{true,true};
  std::uint64_t inhouse_render_us_[2]{};
#if defined(K1_V9_RELEASE_PUPPET)
  core::visual::V9PuppetState puppet_{};  // TEST IMAGE ONLY
#endif
  bool renderInhouse(unsigned board,std::uint64_t now_us) noexcept;
#endif
  float board_intensity_[2]{1.0F, 1.0F};
  std::uint16_t board_intensity_q16_[2]{65535U, 65535U};
  bool secondary_output_enabled_ = false;
  bool board_frame_valid_[2]{};
  std::uint64_t board_frame_us_[2]{};
#ifdef K1_EMOTISCOPE_PORT
  K1EmotiscopeAudioPublication donor_publication_{};
#endif
#endif
};
} // namespace k1::titan
