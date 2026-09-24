#pragma once

// Wide Bloom family adapter (modes 3, 9, 12) and the opt-in INT route hook.
//
// The adapter reproduces the legacy renderBloomFamily law with FP32 working
// values (reference-coupled sampled field, deposited-RGB history):
//   frames      = min(120 * dt, 30)          legacy nominal-frame law, kept
//   displacement= propagation * frames       px, outward splat transport
//   retention   = alpha ^ frames
//   injection   = colour at the centre pair 79/80 (assignment)
//   history     = snapshot BEFORE the display edge fade and mirror
// Only the numeric representation changes: transport, retention, injection
// scaling, forced saturation and the edge fade no longer truncate to bytes.
// The colour law is the legacy one: the palette path ports paletteColour()
// operation for operation (differential test: applying the legacy byte chain
// to the adapter's trace reproduces the legacy injection bytes); the
// chromatic path keeps the legacy 8-bit colour law (its byte-domain HSV
// saturation has no float definition) and is a declared narrowing adapter,
// scaled afterwards in float. The palette-hold state is owned by the adapter.
//
// Route hook: renderRoutedProductChannelV1 is the single call INT places
// where renderProductChannel is called today. With route_enabled false, or
// for any mode outside kWideRouteAdmittedModesV1, it is exactly that legacy
// call (bit-identical output). When routed, the wide display is emitted into
// the unchanged Pixel8 treatment/gain/limit chain through the declared
// rounding adapter (emitWorkingCompatRgb8RoundedV1). On route entry the wide
// history is seeded from the legacy history (code / 255); while routed, the
// legacy history is kept equal to the rounded wide history, so leaving the
// route continues the current trail and never resurrects stale light.
//
// Liveiness: the route resolves the effective propagation with MOD's single
// law, modes::liveinessEffective(mode, wideBloomBasePropagationV1(), input),
// the same px-per-nominal-frame quantity the legacy hook modulates. VP does
// not re-implement the law. The adapter core takes the resolved value.

#include <array>
#include <cstdint>

#include "contract/audio_features_v1.h"
#include "core/pixel.h"
#include "core/visual/channel_render_state.h"
#include "core/visual/modes/liveiness_contract.h"
#include "core/visual/product_effect_renderer.h"
#include "core/visual/visual_audio_frame.h"
#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr std::uint16_t kWideBloomAdapterVersion = 1U;

struct WideBloomStateV1 final {
  WorkingFrame history{};
  float palette_held_position = 0.0F;
  bool palette_held_position_valid = false;
  std::uint32_t frames = 0U;
};

// Pre-quantisation intermediates of one frame (parity oracle and taps).
struct WideBloomTraceV1 final {
  bool palette_path = false;
  float phase = 0.0F;
  float level = 0.0F;
  LegacyPaletteCodeV1 palette_code{};
  Pixel8 chromatic_code8{};
  float output_level = 0.0F;
  bool forced_saturation = false;
  float propagation = 0.0F;
  float frames = 0.0F;
  float retention = 0.0F;
  WorkingRgbF32V1 injection{};
};

struct WideBloomModulationV1 final {
  bool has_effective_propagation = false;
  float effective_propagation = 0.0F;  // px per nominal 120 Hz frame
};

[[nodiscard]] bool wideBloomSupportsModeV1(std::uint16_t mode) noexcept;

[[nodiscard]] float wideBloomBasePropagationV1(
    std::uint16_t mode, const ChannelVisualControls& controls) noexcept;

void resetWideBloomV1(WideBloomStateV1& state) noexcept;

// `display` receives the full 160-pixel frame (after fade and mirror).
WideBloomTraceV1 renderWideBloomV1(WideBloomStateV1& state, std::uint16_t mode,
                                   const ChannelVisualControls& controls,
                                   const contract::AudioFeaturesV1& focused_audio,
                                   float delta_seconds,
                                   const WideBloomModulationV1& modulation,
                                   WorkingFrame& display) noexcept;

inline constexpr std::array<std::uint16_t, 1> kWideRouteAdmittedModesV1{{3U}};

[[nodiscard]] bool wideRouteAdmitsModeV1(std::uint16_t mode) noexcept;

struct WideRoutedChannelV1 final {
  bool route_enabled = false;  // declared switch; false = legacy path
  WideBloomStateV1 bloom{};
  WorkingFrame display{};
  bool previous_frame_wide = false;
  std::uint32_t wide_frames = 0U;
  std::uint32_t legacy_frames = 0U;
  std::uint32_t route_entries = 0U;
  std::uint32_t route_exits = 0U;
  StageBoundaryCountersV1 emit_counters{};
  WideBloomTraceV1 last_trace{};
};

// `liveiness` is the channel's applied Liveiness input (after INT applies
// MOD's renderer hook patch this is channel.controls().liveiness). The
// default is the legacy profile: disabled, neutral, exact identity.
ProductRenderResult renderRoutedProductChannelV1(
    ChannelRenderState& channel, WideRoutedChannelV1& wide,
    const VisualAudioFrameView& visual, float delta_seconds,
    const modes::LiveinessInput& liveiness = modes::LiveinessInput{}) noexcept;

}  // namespace k1::core::visual::wide
