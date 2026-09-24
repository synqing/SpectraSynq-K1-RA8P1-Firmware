#include "core/visual/wide/wide_bloom.h"

#include <algorithm>
#include <cmath>

#include "core/visual/fastled_colour_compat.h"
#include "core/visual/modes/liveiness_registry.h"
#include "core/visual/pixel_topology.h"
#include "core/visual/product_catalogue.h"
#include "core/visual/product_palette.h"
#include "core/visual/wide/wide_endpoint.h"
#include "core/visual/wide/wide_field.h"

namespace k1::core::visual::wide {
namespace {

// Provenance: the helpers below reproduce the legacy anonymous-namespace
// helpers of core/visual/product_effect_renderer.cpp (DualMCU 9792248, blob
// of that file at this commit) operation for operation, so that the wide
// adapter keeps the legacy Bloom colour and transport laws. Parity is proven
// by test_visual_wide_bloom, not assumed from the port.
constexpr float kNominalFramesPerSecond = 120.0F;
constexpr float kAuroraAlpha = 0.98F;

float legacyClamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

float legacyWrap01(float value) noexcept {
  if (!std::isfinite(value)) {
    return 0.0F;
  }
  value -= ::floorf(value);
  return value < 0.0F ? value + 1.0F : value;
}

float legacySanitiseDeltaSeconds(const float delta_seconds) noexcept {
  if (!std::isfinite(delta_seconds) || delta_seconds < 0.0F) {
    return 1.0F / kNominalFramesPerSecond;
  }
  return delta_seconds;
}

float legacyNominalFrames(const float delta_seconds) noexcept {
  const float frames =
      legacySanitiseDeltaSeconds(delta_seconds) * kNominalFramesPerSecond;
  return frames > 30.0F ? 30.0F : frames;
}

PaletteLinearRgb legacyHsv(const float hue, const float saturation,
                           const float value) noexcept {
  const float h = legacyWrap01(hue);
  const float s = legacyClamp01(saturation);
  const float v = legacyClamp01(value);
  if (s <= 0.0F) {
    return {v, v, v};
  }
  const float h6 = h * 6.0F;
  int sector = static_cast<int>(h6);
  if (sector >= 6) {
    sector = 0;
  }
  const float fraction = h6 - static_cast<float>(sector);
  const float p = v * (1.0F - s);
  const float q = v * (1.0F - s * fraction);
  const float t = v * (1.0F - s * (1.0F - fraction));
  switch (sector) {
    case 0:
      return {v, t, p};
    case 1:
      return {q, v, p};
    case 2:
      return {p, v, t};
    case 3:
      return {p, q, v};
    case 4:
      return {t, p, v};
    default:
      return {v, p, q};
  }
}

Pixel8 legacyChromaticColour(const ChannelVisualControls& controls,
                             const contract::AudioFeaturesV1& audio) noexcept {
  PaletteLinearRgb sum{};
  constexpr float kShare = 1.0F / 6.0F;
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = legacyClamp01(audio.chroma_a_origin[bin]);
    const PaletteLinearRgb addition =
        legacyHsv(static_cast<float>(bin) / 12.0F, controls.saturation,
                  level * level * kShare);
    sum.red += addition.red;
    sum.green += addition.green;
    sum.blue += addition.blue;
  }
  sum.red = legacyClamp01(sum.red);
  sum.green = legacyClamp01(sum.green);
  sum.blue = legacyClamp01(sum.blue);
  const std::uint8_t iterations = static_cast<std::uint8_t>(
      controls.square_iterations < 0.0F
          ? 0.0F
          : (controls.square_iterations > 10.0F ? 10.0F
                                                : controls.square_iterations));
  for (std::uint8_t iteration = 0U; iteration < iterations; ++iteration) {
    sum.red *= sum.red;
    sum.green *= sum.green;
    sum.blue *= sum.blue;
  }
  Pixel8 colour = quantiseLinearRgb(sum);
  if (!controls.vp_fix_hsv_source_sat) {
    colour = forceFastLedSaturation(
        colour,
        static_cast<std::uint8_t>(legacyClamp01(controls.saturation) * 255.0F));
  }
  if (!controls.chromatic_mode) {
    colour = forceFastLedHue(
        colour,
        static_cast<std::uint8_t>(
            legacyWrap01(controls.chroma_value + controls.hue_position + 0.05F) *
            255.0F));
  }
  return colour;
}

float legacyBrightestStopPhase(const ProductPaletteDescriptor& palette) noexcept {
  float brightest = -1.0F;
  float phase = 0.0F;
  for (std::uint8_t index = 0U; index < palette.stop_count; ++index) {
    const Pixel8 colour = palette.stops[index].colour;
    const float luminance = 0.30F * static_cast<float>(colour.red) +
                            0.59F * static_cast<float>(colour.green) +
                            0.11F * static_cast<float>(colour.blue);
    if (luminance > brightest) {
      brightest = luminance;
      phase = static_cast<float>(palette.stops[index].position) / 255.0F;
    }
  }
  return phase;
}

struct PaletteChoice final {
  float phase;
  float level;
};

// Port of paletteColour() up to (not including) its byte quantisation; the
// palette-hold state lives in the adapter's own state.
PaletteChoice legacyPaletteChoice(const ChannelVisualControls& controls,
                                  const contract::AudioFeaturesV1& audio,
                                  WideBloomStateV1& state) noexcept {
  float total = 0.0F;
  float maximum = 0.0F;
  float x = 0.0F;
  float y = 0.0F;
  std::uint8_t dominant = 0U;
  constexpr float kTwoPi = 6.2831853071795864769F;
  for (std::uint8_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = legacyClamp01(audio.chroma_a_origin[bin]);
    const float angle = static_cast<float>(bin) / 12.0F * kTwoPi;
    x += ::cosf(angle) * level;
    y += ::sinf(angle) * level;
    total += level;
    if (level > maximum) {
      maximum = level;
      dominant = bin;
    }
  }
  float phase = state.palette_held_position;
  float energy = 0.0F;
  if (total > 0.0001F) {
    const float live = legacyWrap01(::atan2f(y, x) / kTwoPi);
    const float centroid_strength = ::sqrtf(x * x + y * y) / total;
    const float average = total / 12.0F;
    const float contrast = maximum > average ? maximum - average : 0.0F;
    if (!std::isfinite(centroid_strength) || centroid_strength < 0.08F) {
      const float dominant_phase = static_cast<float>(dominant) / 12.0F;
      if (state.palette_held_position_valid) {
        float delta = live - state.palette_held_position;
        if (delta > 0.5F) {
          delta -= 1.0F;
        } else if (delta < -0.5F) {
          delta += 1.0F;
        }
        phase = legacyWrap01(state.palette_held_position +
                             delta * legacyClamp01(centroid_strength / 0.08F));
      } else {
        phase = dominant_phase;
      }
    } else {
      phase = live;
      state.palette_held_position = phase;
      state.palette_held_position_valid = true;
    }
    energy = legacyClamp01(maximum * 0.62F + contrast * 0.30F + average * 0.08F);
  }
  const ProductPaletteDescriptor& palette = productPalette(controls.palette_id);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  if (!silent && energy > 0.35F) {
    float delta = legacyBrightestStopPhase(palette) - phase;
    if (delta > 0.5F) {
      delta -= 1.0F;
    } else if (delta < -0.5F) {
      delta += 1.0F;
    }
    phase = legacyWrap01(phase + delta * energy * energy * 0.85F);
  }
  if (controls.auto_colour_shift) {
    phase = legacyWrap01(phase + controls.hue_position);
  }
  return {phase, energy > controls.chroma ? energy : controls.chroma};
}

WorkingRgbF32V1 forceSaturationWide(const WorkingRgbF32V1& colour) noexcept {
  const float minimum = std::min({colour.red, colour.green, colour.blue});
  return {colour.red - minimum, colour.green - minimum, colour.blue - minimum};
}

void fadeBloomEdgesWide(WorkingFrame& frame) noexcept {
  constexpr std::size_t kFadeWidth = kPixelsPerChannel / 4U;
  for (std::size_t index = 0U; index < kFadeWidth; ++index) {
    const float progress =
        static_cast<float>(index) / static_cast<float>(kFadeWidth - 1U);
    const float amount = progress * progress;
    WorkingRgbF32V1& low = frame[index];
    WorkingRgbF32V1& high = frame[kPixelsPerChannel - 1U - index];
    low = {low.red * amount, low.green * amount, low.blue * amount};
    high = {high.red * amount, high.green * amount, high.blue * amount};
  }
}

void fadeAuroraEdgesWide(WorkingFrame& frame) noexcept {
  constexpr std::size_t kFadeWidth = 6U;
  for (std::size_t index = 0U; index < kFadeWidth; ++index) {
    const float amount =
        static_cast<float>(index + 1U) / static_cast<float>(kFadeWidth + 1U);
    WorkingRgbF32V1& low = frame[index];
    WorkingRgbF32V1& high = frame[kPixelsPerChannel - 1U - index];
    low = {low.red * amount, low.green * amount, low.blue * amount};
    high = {high.red * amount, high.green * amount, high.blue * amount};
  }
}

void seedFromLegacyHistory(WideBloomStateV1& state,
                           const ConstPixelSpan legacy) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    state.history[index] = {static_cast<float>(legacy[index].red) / 255.0F,
                            static_cast<float>(legacy[index].green) / 255.0F,
                            static_cast<float>(legacy[index].blue) / 255.0F};
  }
}

}  // namespace

bool wideBloomSupportsModeV1(const std::uint16_t mode) noexcept {
  return mode == 3U || mode == 9U || mode == 12U;
}

float wideBloomBasePropagationV1(const std::uint16_t mode,
                                 const ChannelVisualControls& controls) noexcept {
  const float resolution_scale =
      static_cast<float>(kPixelsPerChannel) / 128.0F;
  float propagation = 0.0F;
  if (mode == 12U) {
    propagation = (0.80F + 1.20F * legacyClamp01(controls.mood)) *
                  resolution_scale;
  } else {
    const float multiplier = mode == 9U ? 2.0F : 1.0F;
    propagation = (0.25F + 1.75F * legacyClamp01(controls.mood)) *
                  resolution_scale * multiplier;
  }
  return propagation * std::clamp(controls.vp_bloom_shift_scale, 0.25F, 2.00F);
}

void resetWideBloomV1(WideBloomStateV1& state) noexcept {
  state = WideBloomStateV1{};
}

WideBloomTraceV1 renderWideBloomV1(WideBloomStateV1& state,
                                   const std::uint16_t mode,
                                   const ChannelVisualControls& controls,
                                   const contract::AudioFeaturesV1& focused_audio,
                                   const float delta_seconds,
                                   const WideBloomModulationV1& modulation,
                                   WorkingFrame& display) noexcept {
  WideBloomTraceV1 trace{};
  float alpha = std::clamp(controls.vp_bloom_alpha, 0.80F, 1.00F);
  if (controls.vp_fix_bloom_decay) {
    alpha = 0.88F;
  }
  if (mode == 12U) {
    alpha = kAuroraAlpha;
  }
  trace.propagation = wideBloomBasePropagationV1(mode, controls);
  if (modulation.has_effective_propagation &&
      std::isfinite(modulation.effective_propagation) &&
      modulation.effective_propagation >= 0.0F) {
    trace.propagation = modulation.effective_propagation;
  }
  trace.frames = legacyNominalFrames(delta_seconds);
  trace.retention = ::powf(alpha, trace.frames);
  static_cast<void>(shiftOutwardV1(state.history, display,
                                   trace.propagation * trace.frames,
                                   trace.retention));

  trace.palette_path = controls.palette_mode_enabled;
  trace.output_level = static_cast<float>(controls.brightness) / 255.0F *
                       static_cast<float>(controls.photons_id) / 65535.0F;
  WorkingRgbF32V1 colour{};
  if (trace.palette_path) {
    const PaletteChoice choice =
        legacyPaletteChoice(controls, focused_audio, state);
    trace.phase = choice.phase;
    trace.level = choice.level;
    const PaletteLinearRgb sample =
        sampleProductPaletteHd(controls.palette_id, choice.phase, choice.level);
    trace.palette_code = {sample.red, sample.green, sample.blue};
    colour = legacyCodeAsIntentV1(trace.palette_code);
  } else {
    trace.chromatic_code8 = legacyChromaticColour(controls, focused_audio);
    colour = {static_cast<float>(trace.chromatic_code8.red) / 255.0F,
              static_cast<float>(trace.chromatic_code8.green) / 255.0F,
              static_cast<float>(trace.chromatic_code8.blue) / 255.0F};
  }
  colour = {colour.red * trace.output_level, colour.green * trace.output_level,
            colour.blue * trace.output_level};
  trace.forced_saturation =
      controls.vp_bloom_force_saturation && !controls.vp_fix_bloom_decay;
  if (trace.forced_saturation) {
    colour = forceSaturationWide(colour);
  }
  trace.injection = colour;
  display[kCentreLeft] = colour;
  display[kCentreRight] = colour;
  state.history = display;  // snapshot before display-only fade and mirror
  if (mode == 12U) {
    fadeAuroraEdgesWide(display);
  } else {
    fadeBloomEdgesWide(display);
  }
  if (controls.mirror_enabled) {
    applyMirrorV1(display, MirrorPolicyV1::kRightToLeft);
  }
  ++state.frames;
  return trace;
}

bool wideRouteAdmitsModeV1(const std::uint16_t mode) noexcept {
  for (const std::uint16_t admitted : kWideRouteAdmittedModesV1) {
    if (admitted == mode) {
      return true;
    }
  }
  return false;
}

ProductRenderResult renderRoutedProductChannelV1(
    ChannelRenderState& channel, WideRoutedChannelV1& wide,
    const VisualAudioFrameView& visual, const float delta_seconds,
    const modes::LiveinessInput& liveiness) noexcept {
  const std::uint16_t mode = sanitiseProductMode(channel.controls().mode_id);
  if (!wide.route_enabled || !wideRouteAdmitsModeV1(mode)) {
    if (wide.previous_frame_wide) {
      ++wide.route_exits;
      wide.previous_frame_wide = false;
    }
    ++wide.legacy_frames;
    return renderProductChannel(channel, visual, delta_seconds);
  }
  if (!wide.previous_frame_wide) {
    // Route entry: continue the current trail rather than cutting it.
    seedFromLegacyHistory(wide.bloom, channel.previousFrame());
    ++wide.route_entries;
  }
  const float dt = std::isfinite(delta_seconds) && delta_seconds >= 0.0F
                       ? delta_seconds
                       : 1.0F / kNominalFramesPerSecond;
  WideBloomModulationV1 modulation{};
  modulation.has_effective_propagation = true;
  modulation.effective_propagation = modes::liveinessEffective(
      mode, wideBloomBasePropagationV1(mode, channel.controls()), liveiness);
  wide.last_trace = renderWideBloomV1(wide.bloom, mode, channel.controls(),
                                      channel.focusedAudio(), dt, modulation,
                                      wide.display);
  static_cast<void>(emitWorkingCompatRgb8RoundedV1(wide.display, channel.frame(),
                                                   wide.emit_counters));
  // Keep the legacy history coherent so leaving the route shows no stale
  // light: it always holds the rounded current wide history.
  static_cast<void>(emitWorkingCompatRgb8RoundedV1(
      wide.bloom.history, channel.previousFrame(), wide.emit_counters));
  wide.previous_frame_wide = true;
  ++wide.wide_frames;
  return {true, mode};
}

}  // namespace k1::core::visual::wide
