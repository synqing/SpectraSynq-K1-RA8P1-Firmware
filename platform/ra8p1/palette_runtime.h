#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "core/visual/channel_render_state.h"
#include "core/visual/visual_audio_frame.h"
#include "core/visual/wide/wide_bloom.h"
#include "core/visual/wide/wide_endpoint.h"
#include "core/visual/wide/wide_native_output.h"
#include "core/visual/wide/wide_types.h"
#ifdef K1_PALETTE_MORPH
#include "palette_transition.h"
#include "centre_palette_engine.h"
#endif

namespace k1::titan {
inline constexpr std::uint32_t kPaletteCatalogueOpcode = 15U;
inline constexpr std::uint32_t kPaletteConfigureOpcode = 16U;
inline constexpr std::uint32_t kPaletteStatusOpcode = 17U;
inline constexpr std::uint32_t kPaletteFrameOpcode = 18U;
inline constexpr std::uint32_t kPaletteWireSnapshotOpcode = 19U; // WS2816 only; last submission, not photons.
// SET_CONFIG (opcode 16) wire version 4 (INT, round 4): the version-3 layout
// (10 x u32, little-endian, 40 bytes) followed by one u32 little-endian
// switch mask at payload offset 40 (44 bytes). Bit 0 = use_wide_route,
// bit 1 = use_wide_native16; any other bit is refused. Versions 1..3 carry
// no mask and decode with both switches off (the incumbent path).
inline constexpr std::uint32_t kPaletteConfigV4Bytes = 44U;
inline constexpr std::uint32_t kPaletteConfigSwitchOffset = 40U;
inline constexpr std::uint32_t kPaletteSwitchWideRoute = 1U << 0U;
inline constexpr std::uint32_t kPaletteSwitchWideNative16 = 1U << 1U;
inline constexpr std::uint32_t kPaletteSwitchMask =
    kPaletteSwitchWideRoute | kPaletteSwitchWideNative16;
static_assert(kPaletteConfigV4Bytes == 11U * 4U &&
              kPaletteConfigSwitchOffset + 4U == kPaletteConfigV4Bytes,
              "SET_CONFIG v4 is the v3 layout plus one u32 switch mask");
#ifdef K1_PALETTE_WS2816
// Two sequential 80-pixel GRB48 transfers need more than an 8.333 ms slot.
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
inline constexpr std::uint32_t kDiagnosticBounceFrameUs = 33333U;
inline constexpr std::uint32_t kDiagnosticBounceSteps = 127U;
// Flags: active, automatic catalogue cycle, physical bench output.
struct PaletteConfig {
  std::uint32_t version = 1U;
  std::uint32_t palette_a = 0U, palette_b = 1U;
  std::uint32_t mode_a = 0U, mode_b = 0U; // 0 = preview; 64 = palette bounce.
  std::uint32_t flags = 0U, brightness = 24U, output_channel = 0U;
  std::uint32_t transition_ms = 0U; // Versions 2/3; 0 is an immediate cut.
  std::uint32_t travel_ms = 4000U; // Version 3: centre-to-edge travel duration.
  // TIT-2: select the wide endpoint's genuine 16-bit-per-channel words over
  // the legacy Pixel8 lift-to-16 (value*257) in packNative16Lane(). Default
  // OFF is bit-identical to today's packBenchGrb48Lane (functional-scope
  // freeze); no current wire decoder sets this field, so every live SET_CONFIG
  // still lands on the legacy path until a decoder explicitly opts in.
  bool use_wide_native16 = false;
  // Round 4 (ORCH): maps to core::visual::wide::WideRoutedChannelV1::route_enabled,
  // INT's declared switch at the renderProductChannel call site itself
  // (core/visual/wide/wide_bloom.h). Default OFF: renderRoutedProductChannelV1
  // with route_enabled=false, or any mode outside kWideRouteAdmittedModesV1,
  // is documented and host-proven to be exactly the legacy renderProductChannel
  // call (bit-identical output) -- see docs/reference-import-receipt-tit2.md
  // and tests/host/test_palette_runtime.cpp's WIDE_ROUTE_* assertions.
  bool use_wide_route = false;
};
class PaletteRuntime {
 public:
  PaletteRuntime() = default;
  PaletteRuntime(const PaletteRuntime&) = delete;
  PaletteRuntime& operator=(const PaletteRuntime&) = delete;
  bool configure(const PaletteConfig& config, std::uint64_t now_us) noexcept;
  bool step(std::uint64_t now_us,
            const core::visual::VisualAudioFrameView* audio) noexcept;
  void stop() noexcept { config_.flags = 0U; }
  bool active() const noexcept { return (config_.flags & 1U) != 0U; }
  bool emitEnabled() const noexcept { return (config_.flags & 4U) != 0U; }
  const PaletteConfig& config() const noexcept { return config_; }
  core::visual::ChannelRenderState& channel(unsigned i) noexcept {
    return i ? b_ : a_;
  }
  const core::visual::ChannelRenderState& channel(unsigned i) const noexcept {
    return i ? b_ : a_;
  }
  std::size_t packBenchGrb(std::uint8_t* destination, std::size_t capacity,
                         unsigned pixels = 128U) const noexcept;
  std::size_t packBenchGrb48Lane(std::uint8_t* destination, std::size_t capacity,
                               unsigned lane) const noexcept;
  // Wide endpoint's native DeviceRgb16V1 words, packed to the identical
  // GRB48 wire layout packBenchGrb48Lane already uses (G_hi,G_lo,R_hi,R_lo,
  // B_hi,B_lo -- see src/k1/core/visual/ws2816_pack.h and DualMCU's
  // packWs2816PixelV1, which this mirrors byte for byte -- proof in
  // tests/host/test_palette_runtime.cpp). Round 4 (ORCH): does NOT scale by
  // brightness any more -- resolveWideEndpointV1 already applies
  // EndpointConfigV1::master (== brightness/255) once, in the FP32 drive
  // domain, before quantising; scaling the already-quantised 16-bit words
  // again here would apply brightness twice.
  std::size_t packWideNative16Lane(std::uint8_t* destination, std::size_t capacity,
                                  unsigned lane,
                                  const core::visual::wide::DeviceRgb16Frame& frame) const noexcept;
  // Selector: config_.use_wide_native16 (default false) picks
  // packWideNative16Lane over packBenchGrb48Lane when a wide frame is
  // supplied. wide_frame == nullptr always falls back to the legacy path
  // regardless of the flag, so a caller that has not produced a wide frame
  // this cycle never silently emits stale/uninitialised wide data.
  std::size_t packNative16Lane(std::uint8_t* destination, std::size_t capacity,
                              unsigned lane,
                              const core::visual::wide::DeviceRgb16Frame* wide_frame) const noexcept;
  // step()'s wide-native16 output for channel i (0=A, 1=B): the genuine
  // resolveNativeOutputV1() result (capture -> FP32 output treatment ->
  // resolveWideEndpointV1, DUR-011). Produced only when
  // config_.use_wide_native16 is true; holds its default-constructed
  // (all-black) value otherwise, so a caller must still gate on
  // config().use_wide_native16 itself, exactly as packNative16Lane does.
  const core::visual::wide::DeviceRgb16Frame& wideFrame(unsigned i) const noexcept {
    return i ? wide_b_ : wide_a_;
  }
  // This cycle's resolveNativeOutputV1() report (per-channel finite/negative/
  // overrange counts, requested current, limiter_scale). Only meaningful
  // when config_.use_wide_native16 is true; default-constructed otherwise.
  // limiter_scale stays 1.0 by construction of endpointConfig() -- see its
  // definition -- because Titan has no incumbent current limiter to
  // reproduce; this accessor is how a test proves that stays true.
  const core::visual::wide::EndpointReportV1& endpointReport() const noexcept {
    return endpoint_report_;
  }
  // This cycle's per-channel capture provenance (kWideRenderer vs kLiftedPixel8). Only meaningful when config_.use_wide_native16 is true.
  core::visual::wide::WideCaptureProvenanceV1 provenance(unsigned i) const noexcept {
    return i ? provenance_[1] : provenance_[0];
  }
  std::size_t catalogueJson(char* out, std::size_t capacity) const noexcept;
  std::size_t statusJson(char* out, std::size_t capacity) const noexcept;
  void recordEmit(int result, std::uint32_t cycles) noexcept;
 private:
  core::visual::ChannelRenderState a_{core::visual::PixelChannelId::kChannelA};
  core::visual::ChannelRenderState b_{core::visual::PixelChannelId::kChannelB};
  PaletteConfig config_{};
#ifdef K1_PALETTE_MORPH
  core::visual::PaletteTransition transitions_[2]{};
#endif
  std::uint64_t next_us_ = 0U, last_us_ = 0U, cycle_start_us_ = 0U,
      last_live_us_ = 0U;
  bool last_live_valid_ = false;
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
  core::visual::wide::DeviceRgb16Frame wide_a_{}, wide_b_{};
  // Persistent per-channel wide-route state (bloom history, frame counters,
  // route_enabled) for renderRoutedProductChannelV1 -- must outlive a
  // single step() call so the wide Bloom adapter's own history survives
  // frame to frame exactly like the legacy renderer's channel state does.
  core::visual::wide::WideRoutedChannelV1 wide_route_a_{}, wide_route_b_{};
  // Caller-owned scratch for resolveNativeOutputV1 (7680 + 3840 B) --
  // members, not step()-local, so producing a native frame allocates
  // nothing (A5).
  core::visual::wide::WideNativeOutputWorkspaceV1 wide_workspace_{};
  core::visual::wide::EndpointReportV1 endpoint_report_{};
  std::array<core::visual::wide::WideCaptureProvenanceV1, 2> provenance_{
      core::visual::wide::WideCaptureProvenanceV1::kLiftedPixel8,
      core::visual::wide::WideCaptureProvenanceV1::kLiftedPixel8};
  // Titan's incumbent current limiting: there is none (grep-confirmed --
  // no "current_ma"/"max_current"/"current_limit" anywhere under
  // platform/ra8p1/). max_current_ma is set to 1e6 (the top of
  // EndpointConfigV1's declared valid range [100, 1e6]) so
  // resolveWideEndpointV1's limiter is mathematically inert: the largest
  // possible two-channel draw (160 px x 3 components x 20 mA/component x 2
  // channels = 19200 mA) is far below it, so limiter_scale stays 1.0. This
  // reproduces "no limiting" using a valid config value, not an
  // out-of-range hack.
  static core::visual::wide::EndpointConfigV1 endpointConfig(std::uint32_t brightness) noexcept {
    core::visual::wide::EndpointConfigV1 config{};
    config.master = static_cast<float>(brightness) / 255.0F;
    config.max_current_ma = 1000000.0F;
    return config;
  }
};
} // namespace k1::titan
