#include "core/visual/product_effect_renderer.h"

#include <algorithm>

#include "core/visual/fastled_colour_compat.h"
#include "core/visual/modes/liveiness_registry.h"
#include "core/visual/pixel_topology.h"
#include "core/visual/product_catalogue.h"
#include "core/visual/product_palette.h"

#include <cmath>
#include <cstddef>

namespace k1::core::visual {
namespace {

constexpr float kNominalFramesPerSecond = 120.0F;
constexpr float kAuroraAlpha = 0.98F;
constexpr float kRiverAlpha = 0.90F;
constexpr float kRiverFloor = 0.015F;
constexpr float kRiverInjectionGain = 0.90F;

Pixel8 forcePixelSaturation(const Pixel8 colour) noexcept {
  const std::uint8_t minimum = std::min({colour.red, colour.green, colour.blue});
  return {
      static_cast<std::uint8_t>(colour.red - minimum),
      static_cast<std::uint8_t>(colour.green - minimum),
      static_cast<std::uint8_t>(colour.blue - minimum)};
}
constexpr float kRiverDriftBase = 0.55F;
constexpr float kRiverTideFloor = 0.90F;
constexpr float kRiverTideSurge = 0.90F;
constexpr float kRiverTideNominalAlpha = 0.05F;
constexpr std::uint8_t kRiverMaximumSquareIterations = 4U;

constexpr float kEmberDriftBase = 0.45F;
constexpr float kEmberDriftSurge = 0.60F;
constexpr float kEmberDriftFloor = 0.70F;
constexpr float kEmberAlpha = 0.88F;
constexpr float kEmberFloor = 0.015F;
constexpr float kEmberGain = 0.95F;
constexpr float kEmberHueSpread = 0.18F;
constexpr float kEmberMinimumReach = 4.0F;

constexpr float kSurgeFastTauSeconds = 0.50F;
constexpr float kSurgeSlowTauSeconds = 20.0F;
constexpr float kSurgeSlowMinimum = 0.05F;
constexpr float kSurgePeakTauSeconds = 4.0F;
constexpr float kSurgeBuildHot = 1.30F;
constexpr float kSurgeBuildMemorySeconds = 2.0F;
constexpr float kSurgeDropFloor = 0.45F;
constexpr float kSurgeRefractorySeconds = 4.0F;
constexpr float kSurgeWavefrontSpeedMultiplier = 2.50F;
constexpr float kSurgeWavefrontLifeSeconds = 0.80F;
constexpr float kSurgeWavefrontHalfWidth = 3.0F;
constexpr float kSurgeWavefrontGain = 0.95F;

constexpr float kWaveformFastScrollPixelsPerSecond = 120.0F;
constexpr float kWaveformScrollPixelsPerSecond = 120.0F;
constexpr float kWaveformHybridScrollPixelsPerSecond = 120.0F;
constexpr float kWaveformK1ScrollPixelsPerSecond = 405.0F;
constexpr float kWaveformK1MinimumDecayRate = 0.80F;
constexpr float kWaveformK1DecayScale = 3.50F;
constexpr float kWaveformK1SilenceDecay = 10.0F;
constexpr float kWaveformK1PeakTauOne = 0.016F;
constexpr float kWaveformK1PeakTauTwo = 0.023F;
constexpr float kWaveformK1ColourTau = 0.080F;

constexpr float kCometTrailAlpha = 0.95F;
constexpr float kCometLifeAlpha = 0.982F;
constexpr float kCometMinimumStrength = 0.06F;
constexpr float kCometSpeedMinimum = 0.60F;
constexpr float kCometSpeedMood = 2.80F;
constexpr float kCometSize = 3.50F;
constexpr float kCometHeadGain = 0.85F;
constexpr float kCometWakeStretch = 2.0F;
constexpr int kCometGlow = 2;

constexpr float kPercussionTrailAlpha = 0.94F;
constexpr float kPercussionKickLife = 0.45F;
constexpr float kPercussionSnareLife = 0.35F;
constexpr float kPercussionHatLife = 0.15F;
constexpr float kPercussionKickSpeed = 2.20F;
constexpr float kPercussionSnareSpeed = 1.10F;
constexpr float kPercussionHatSpeed = 0.50F;
constexpr float kPercussionSnarePosition = 0.40F;
constexpr float kPercussionHatPositionA = 0.84F;
constexpr float kPercussionHatPositionB = 0.93F;

constexpr float kTempoRiverPixelsPerBeat = 22.0F;
constexpr float kTempoRiverDepth = 0.60F;
constexpr float kTempoRiverIdlePixelsPerFrame = 0.55F;
constexpr float kTempoRiverFloorPixelsPerFrame = 0.30F;
constexpr float kTempoRiverMaximumPixelsPerFrame = 3.0F;
constexpr float kTempoConfidenceLow = 0.30F;
constexpr float kTempoConfidenceHigh = 0.60F;

constexpr float kTempoCometResynchroniseHz = 2.0F;
constexpr std::uint16_t kTempoCometCoastBeats = 8U;
constexpr float kTempoCometNominalBpmAlpha = 0.05F;
constexpr float kTempoCometReachMaximum = 0.92F;
constexpr float kTempoCometReachMinimum = 0.45F;
constexpr float kTempoCometStrengthFloor = 0.35F;
constexpr float kTempoCometPaletteSpread = 0.32F;

float clamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

float wrap01(float value) noexcept {
  if (!std::isfinite(value)) {
    return 0.0F;
  }
  value -= ::floorf(value);
  return value < 0.0F ? value + 1.0F : value;
}

float sanitiseDeltaSeconds(const float delta_seconds) noexcept {
  if (!std::isfinite(delta_seconds) || delta_seconds < 0.0F) {
    return 1.0F / kNominalFramesPerSecond;
  }
  return delta_seconds;
}

float nominalFrames(const float delta_seconds) noexcept {
  const float frames = sanitiseDeltaSeconds(delta_seconds) *
                       kNominalFramesPerSecond;
  return frames > 30.0F ? 30.0F : frames;
}

float dtCorrectAlpha(const float nominal_alpha,
                     const float delta_seconds) noexcept {
  return 1.0F - ::powf(1.0F - clamp01(nominal_alpha),
                       nominalFrames(delta_seconds));
}

PaletteLinearRgb hsv(const float hue, const float saturation,
                     const float value) noexcept {
  const float h = wrap01(hue);
  const float s = clamp01(saturation);
  const float v = clamp01(value);
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

Pixel8 chromaticColour(const ChannelVisualControls& controls,
                       const contract::AudioFeaturesV1& audio) noexcept {
  PaletteLinearRgb sum{};
  constexpr float kShare = 1.0F / 6.0F;
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = clamp01(audio.chroma_a_origin[bin]);
    const PaletteLinearRgb addition =
        hsv(static_cast<float>(bin) / 12.0F, controls.saturation,
            level * level * kShare);
    sum.red += addition.red;
    sum.green += addition.green;
    sum.blue += addition.blue;
  }
  sum.red = clamp01(sum.red);
  sum.green = clamp01(sum.green);
  sum.blue = clamp01(sum.blue);
  const std::uint8_t iterations = static_cast<std::uint8_t>(
      controls.square_iterations < 0.0F
          ? 0.0F
          : (controls.square_iterations > 10.0F
                 ? 10.0F
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
        static_cast<std::uint8_t>(clamp01(controls.saturation) * 255.0F));
  }
  if (!controls.chromatic_mode) {
    colour = forceFastLedHue(
        colour,
        static_cast<std::uint8_t>(
            wrap01(controls.chroma_value + controls.hue_position + 0.05F) *
            255.0F));
  }
  return colour;
}

float brightestStopPhase(const ProductPaletteDescriptor& palette) noexcept {
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

Pixel8 paletteColour(const ChannelVisualControls& controls,
                     const contract::AudioFeaturesV1& audio,
                     ChannelEffectState& state) noexcept {
  float total = 0.0F;
  float maximum = 0.0F;
  float x = 0.0F;
  float y = 0.0F;
  std::uint8_t dominant = 0U;
  constexpr float kTwoPi = 6.2831853071795864769F;
  for (std::uint8_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = clamp01(audio.chroma_a_origin[bin]);
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
    const float live = wrap01(::atan2f(y, x) / kTwoPi);
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
        phase = wrap01(state.palette_held_position +
                       delta * clamp01(centroid_strength / 0.08F));
      } else {
        phase = dominant_phase;
      }
    } else {
      phase = live;
      state.palette_held_position = phase;
      state.palette_held_position_valid = true;
    }
    energy = clamp01(maximum * 0.62F + contrast * 0.30F + average * 0.08F);
  }
  const ProductPaletteDescriptor& palette = productPalette(controls.palette_id);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  if (!silent && energy > 0.35F) {
    float delta = brightestStopPhase(palette) - phase;
    if (delta > 0.5F) {
      delta -= 1.0F;
    } else if (delta < -0.5F) {
      delta += 1.0F;
    }
    phase = wrap01(phase + delta * energy * energy * 0.85F);
  }
  if (controls.auto_colour_shift) {
    phase = wrap01(phase + controls.hue_position);
  }
  return quantiseLinearRgb(sampleProductPaletteHd(
      controls.palette_id, phase, energy > controls.chroma ? energy
                                                           : controls.chroma));
}

Pixel8 injectionColour(ChannelRenderState& channel) noexcept {
  const ChannelVisualControls& controls = channel.controls();
  Pixel8 colour = controls.palette_mode_enabled
                      ? paletteColour(controls, channel.focusedAudio(),
                                      channel.effectState())
                      : chromaticColour(controls, channel.focusedAudio());
  const float output_level =
      static_cast<float>(controls.brightness) / 255.0F *
      static_cast<float>(controls.photons_id) / 65535.0F;
  return scalePixel(colour, output_level);
}

float channelOutputLevel(const ChannelVisualControls& controls) noexcept {
  return static_cast<float>(controls.brightness) / 255.0F *
         static_cast<float>(controls.photons_id) / 65535.0F;
}

Pixel8 explicitPaletteColour(const ChannelVisualControls& controls,
                             float phase, const float level) noexcept {
  if (controls.auto_colour_shift) {
    phase += controls.hue_position;
  }
  return scalePixel(
      quantiseLinearRgb(
          sampleProductPaletteHd(controls.palette_id, wrap01(phase), level)),
      channelOutputLevel(controls));
}

void addWeighted(Pixel8& destination, const Pixel8 source,
                 const float weight) noexcept {
  destination = addSaturating(destination, scalePixel(source, weight));
}

void transportOutward(const PixelSpan destination,
                      const ConstPixelSpan history,
                      const float pixels_per_frame, const float alpha,
                      const float delta_seconds) noexcept {
  const float frames = nominalFrames(delta_seconds);
  const float displacement = pixels_per_frame * frames;
  const float retention = ::powf(alpha, frames);
  for (std::size_t index = 0U; index < history.size(); ++index) {
    const Pixel8 source = history[index];
    if (source.red == 0U && source.green == 0U && source.blue == 0U) {
      continue;
    }
    const float target = index <= kCentreLeft
                             ? static_cast<float>(index) - displacement
                             : static_cast<float>(index) + displacement;
    const int lower = static_cast<int>(::floorf(target));
    const float fraction = target - static_cast<float>(lower);
    if (lower >= 0 && lower < static_cast<int>(destination.size())) {
      addWeighted(destination[static_cast<std::size_t>(lower)], source,
                  (1.0F - fraction) * retention);
    }
    const int upper = lower + 1;
    if (upper >= 0 && upper < static_cast<int>(destination.size())) {
      addWeighted(destination[static_cast<std::size_t>(upper)], source,
                  fraction * retention);
    }
  }
}

void snapshotHistory(ChannelRenderState& channel) noexcept {
  const ConstPixelSpan frame = channel.frame();
  const PixelSpan history = channel.previousFrame();
  for (std::size_t index = 0U; index < frame.size(); ++index) {
    history[index] = frame[index];
  }
}

void mirrorRightHalf(const PixelSpan frame) noexcept {
  for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
    frame[kCentreLeft - distance] = frame[kCentreRight + distance];
  }
}

void clearLeftHalf(const PixelSpan frame) noexcept {
  for (std::size_t index = 0U; index < kPixelsPerHalf; ++index) {
    frame[index] = Pixel8{};
  }
}

std::size_t spectrumBinCount(
    const contract::AudioFeaturesV1& audio) noexcept {
  if ((audio.validity_flags & contract::kValidSpectrum) == 0U) {
    return 0U;
  }
  const std::size_t count = audio.nyquist_safe_bin_hi;
  return count < kPixelsPerHalf ? count : kPixelsPerHalf;
}

std::uint8_t riverSquareIterations(
    const ChannelVisualControls& controls) noexcept {
  if (!std::isfinite(controls.square_iterations) ||
      controls.square_iterations <= 0.0F) {
    return 0U;
  }
  const auto iterations =
      static_cast<std::uint8_t>(controls.square_iterations);
  return iterations < kRiverMaximumSquareIterations
             ? iterations
             : kRiverMaximumSquareIterations;
}

void injectSpectrumRiverAtOffset(ChannelRenderState& channel,
                                 const float phase_offset) noexcept {
  const ChannelVisualControls& controls = channel.controls();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const std::size_t bin_count = spectrumBinCount(audio);
  const float denominator =
      static_cast<float>(bin_count > 1U ? bin_count - 1U : 1U);
  const std::uint8_t iterations = riverSquareIterations(controls);
  for (std::size_t bin = 0U; bin < bin_count; ++bin) {
    float energy = clamp01(audio.spectrum[bin]);
    for (std::uint8_t iteration = 0U; iteration < iterations; ++iteration) {
      energy *= energy;
    }
    if (energy < kRiverFloor) {
      continue;
    }
    const Pixel8 colour = explicitPaletteColour(
        controls, static_cast<float>(bin) / denominator + phase_offset,
        energy * kRiverInjectionGain);
    Pixel8& destination = channel.frame()[kCentreRight + bin];
    destination = addSaturating(destination, colour);
  }
}

void injectSpectrumRiver(ChannelRenderState& channel) noexcept {
  injectSpectrumRiverAtOffset(channel, 0.0F);
}

void updateTide(float& tide, const float low_energy,
                const float delta_seconds) noexcept {
  tide += (clamp01(low_energy) - tide) *
          dtCorrectAlpha(kRiverTideNominalAlpha, delta_seconds);
  tide = clamp01(tide);
}

float chromagramCentroidHue(ChannelEffectState& state,
                            const contract::AudioFeaturesV1& audio) noexcept {
  constexpr float kTwoPi = 6.2831853071795864769F;
  float x = 0.0F;
  float y = 0.0F;
  float total = 0.0F;
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = clamp01(audio.chroma_a_origin[bin]);
    const float angle = static_cast<float>(bin) / 12.0F * kTwoPi;
    x += ::cosf(angle) * level;
    y += ::sinf(angle) * level;
    total += level;
  }
  if ((audio.validity_flags & contract::kValidChroma) != 0U &&
      total > 0.0001F) {
    state.palette_held_position = wrap01(::atan2f(y, x) / kTwoPi);
    state.palette_held_position_valid = true;
  }
  return state.palette_held_position_valid ? state.palette_held_position : 0.0F;
}

float updateRiverSurge(ChannelEffectState& state,
                       const contract::AudioFeaturesV1& audio,
                       const float delta_seconds) noexcept {
  const float dt = delta_seconds < 0.001F
                       ? 0.001F
                       : (delta_seconds > 0.05F ? 0.05F : delta_seconds);
  const float energy = clamp01(audio.spectral_energy);
  if (!state.surge_initialised) {
    state.surge_fast_env = energy;
    state.surge_slow_env = energy;
    state.surge_peak_env = energy;
    state.surge_initialised = true;
  }
  state.surge_fast_env +=
      (energy - state.surge_fast_env) * clamp01(dt / kSurgeFastTauSeconds);
  state.surge_slow_env +=
      (energy - state.surge_slow_env) * clamp01(dt / kSurgeSlowTauSeconds);

  const float slow = state.surge_slow_env < kSurgeSlowMinimum
                         ? kSurgeSlowMinimum
                         : state.surge_slow_env;
  float build_ratio = state.surge_fast_env / slow;
  if (build_ratio < 0.50F) {
    build_ratio = 0.50F;
  } else if (build_ratio > 2.0F) {
    build_ratio = 2.0F;
  }
  float multiplier = 0.70F + 0.55F * (build_ratio - 0.50F);
  if (multiplier < 0.70F) {
    multiplier = 0.70F;
  } else if (multiplier > 1.75F) {
    multiplier = 1.75F;
  }

  state.surge_peak_env *=
      1.0F - clamp01(dt / kSurgePeakTauSeconds);
  if (state.surge_fast_env > state.surge_peak_env) {
    state.surge_peak_env = state.surge_fast_env;
  }
  if (build_ratio > kSurgeBuildHot) {
    state.surge_build_recent_seconds = kSurgeBuildMemorySeconds;
  } else {
    state.surge_build_recent_seconds =
        state.surge_build_recent_seconds > dt
            ? state.surge_build_recent_seconds - dt
            : 0.0F;
  }
  state.surge_refractory_seconds =
      state.surge_refractory_seconds > dt
          ? state.surge_refractory_seconds - dt
          : 0.0F;

  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  if (!state.surge_wavefront_active && !silent &&
      state.surge_refractory_seconds <= 0.0F &&
      state.surge_build_recent_seconds > 0.0F &&
      state.surge_fast_env < kSurgeDropFloor * state.surge_peak_env) {
    state.surge_wavefront_active = true;
    state.surge_wavefront_pos = static_cast<float>(kCentreRight);
    state.surge_wavefront_life = 1.0F;
    state.surge_refractory_seconds = kSurgeRefractorySeconds;
    state.surge_build_recent_seconds = 0.0F;
  }
  return multiplier;
}

void injectRiverSurgeWavefront(ChannelRenderState& channel,
                               const float drift,
                               const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  if (!state.surge_wavefront_active) {
    return;
  }
  state.surge_wavefront_pos += kSurgeWavefrontSpeedMultiplier * drift *
                               nominalFrames(delta_seconds);
  state.surge_wavefront_life -=
      delta_seconds / kSurgeWavefrontLifeSeconds;
  if (state.surge_wavefront_life <= 0.0F ||
      state.surge_wavefront_pos >=
          static_cast<float>(kPixelsPerChannel - 1U)) {
    state.surge_wavefront_active = false;
    state.surge_wavefront_life = 0.0F;
    return;
  }

  const std::size_t bin_count = spectrumBinCount(channel.focusedAudio());
  const float denominator =
      static_cast<float>(bin_count > 1U ? bin_count - 1U : 1U);
  const float phase = clamp01(
      (state.surge_wavefront_pos - static_cast<float>(kCentreRight)) /
      denominator);
  const int centre = static_cast<int>(state.surge_wavefront_pos + 0.5F);
  const int half_width = static_cast<int>(kSurgeWavefrontHalfWidth);
  for (int offset = -half_width; offset <= half_width; ++offset) {
    const int index = centre + offset;
    if (index < static_cast<int>(kCentreRight) ||
        index >= static_cast<int>(kPixelsPerChannel)) {
      continue;
    }
    const int absolute_offset = offset < 0 ? -offset : offset;
    const float profile =
        1.0F - static_cast<float>(absolute_offset) /
                   (kSurgeWavefrontHalfWidth + 1.0F);
    const Pixel8 colour = explicitPaletteColour(
        channel.controls(), phase,
        state.surge_wavefront_life * profile * kSurgeWavefrontGain);
    Pixel8& destination = channel.frame()[static_cast<std::size_t>(index)];
    destination = addSaturating(destination, colour);
  }
}

void renderSpectrumRiver(ChannelRenderState& channel, const std::uint16_t mode,
                         const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  float drift_multiplier = 1.0F;
  if (mode == 15U) {
    updateTide(state.river_tide_env, channel.focusedAudio().low_energy,
               delta_seconds);
    drift_multiplier =
        kRiverTideFloor + kRiverTideSurge * state.river_tide_env;
  } else if (mode == 28U) {
    updateTide(state.surge_tide_env, channel.focusedAudio().low_energy,
               delta_seconds);
    drift_multiplier =
        (kRiverTideFloor + kRiverTideSurge * state.surge_tide_env) *
        updateRiverSurge(state, channel.focusedAudio(), delta_seconds);
  }
  const float drift = modes::liveinessEffective(
      mode,
      kRiverDriftBase * drift_multiplier *
          static_cast<float>(kPixelsPerChannel) / 128.0F,
      channel.controls().liveiness);
  transportOutward(channel.frame(), channel.previousFrame(), drift,
                   kRiverAlpha, delta_seconds);
  clearLeftHalf(channel.frame());
  injectSpectrumRiver(channel);
  if (mode == 28U) {
    injectRiverSurgeWavefront(channel, drift, delta_seconds);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

void renderEmber(ChannelRenderState& channel,
                 const float delta_seconds) noexcept {
  const float energy = clamp01(channel.focusedAudio().spectral_energy);
  const float drift = modes::liveinessEffective(
      16U,
      kEmberDriftBase * (kEmberDriftFloor + kEmberDriftSurge * energy) *
          static_cast<float>(kPixelsPerChannel) / 128.0F,
      channel.controls().liveiness);
  transportOutward(channel.frame(), channel.previousFrame(), drift,
                   kEmberAlpha, delta_seconds);
  clearLeftHalf(channel.frame());
  const float centroid = chromagramCentroidHue(
      channel.effectState(), channel.focusedAudio());
  const float reach = kEmberMinimumReach +
                      energy * (static_cast<float>(kPixelsPerHalf) -
                                kEmberMinimumReach);
  for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
    if (static_cast<float>(distance) > reach) {
      break;
    }
    const float normalised = static_cast<float>(distance) / reach;
    const float brightness =
        energy * (1.0F - normalised * normalised) * kEmberGain;
    if (brightness < kEmberFloor) {
      continue;
    }
    Pixel8& destination = channel.frame()[kCentreRight + distance];
    destination = addSaturating(
        destination,
        explicitPaletteColour(channel.controls(),
                              centroid + normalised * kEmberHueSpread,
                              brightness));
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

float exponentialAlpha(const float delta_seconds,
                       const float tau_seconds) noexcept {
  return clamp01(1.0F - ::expf(-sanitiseDeltaSeconds(delta_seconds) /
                               tau_seconds));
}

Pixel8 waveformLastColour(ChannelRenderState& channel,
                          WaveformEffectState& state,
                          const float peak,
                          const float delta_seconds,
                          const float nominal_colour_alpha) noexcept {
  const Pixel8 target = injectionColour(channel);
  float chroma_energy = 0.0F;
  for (const float value : channel.focusedAudio().chroma_a_origin) {
    chroma_energy += clamp01(value);
  }
  chroma_energy /= static_cast<float>(contract::kChromaBinCount);
  const float blend = clamp01(
      chroma_energy * channel.controls().vp_waveform_chroma_blend_gain);
  const float alpha = dtCorrectAlpha(
      nominal_colour_alpha * (0.50F + blend), delta_seconds);
  const PaletteLinearRgb target_linear{
      static_cast<float>(target.red) / 255.0F,
      static_cast<float>(target.green) / 255.0F,
      static_cast<float>(target.blue) / 255.0F};
  state.last_colour.red +=
      (target_linear.red - state.last_colour.red) * alpha;
  state.last_colour.green +=
      (target_linear.green - state.last_colour.green) * alpha;
  state.last_colour.blue +=
      (target_linear.blue - state.last_colour.blue) * alpha;
  const float fallback = clamp01(
      peak * channel.controls().vp_waveform_fallback_brightness);
  const float level = clamp01((peak * blend) + (fallback * (1.0F - blend)));
  return quantiseLinearRgb(
      {state.last_colour.red * level, state.last_colour.green * level,
       state.last_colour.blue * level});
}

void depositWaveformSeed(ChannelRenderState& channel,
                         const VisualWaveformHistory& waveform,
                         const Pixel8 colour, const float peak,
                         const bool use_history) noexcept {
  const std::size_t requested = channel.controls().samples_per_chunk;
  const std::size_t sample_count =
      waveform.sample_count < requested ? waveform.sample_count : requested;
  const bool valid_history =
      use_history && sample_count > 1U &&
      sample_count <= kVisualWaveformMaximumSamples && waveform.raw_maximum > 0.0F;
  const std::size_t radius = valid_history
                                 ? 3U + static_cast<std::size_t>(peak * 7.0F)
                                 : 0U;
  for (std::size_t distance = 0U; distance <= radius; ++distance) {
    float level = peak;
    if (valid_history) {
      const std::size_t sample =
          distance * (sample_count - 1U) / (radius > 0U ? radius : 1U);
      std::uint32_t total = 0U;
      std::size_t count = 0U;
      for (std::size_t frame = 0U; frame < kVisualWaveformHistoryFrames;
           ++frame) {
        for (int tap = -2; tap <= 2; ++tap) {
          int index = static_cast<int>(sample) + tap;
          if (index < 0) {
            index = 0;
          } else if (index >= static_cast<int>(sample_count)) {
            index = static_cast<int>(sample_count) - 1;
          }
          const std::int32_t value = waveform.frames[frame]
                                                    [static_cast<std::size_t>(index)];
          total += static_cast<std::uint32_t>(value < 0 ? -value : value);
          ++count;
        }
      }
      level = count > 0U
                  ? clamp01(static_cast<float>(total) /
                            static_cast<float>(count) / waveform.raw_maximum)
                  : 0.0F;
      level *= 0.30F + 0.70F * peak;
    }
    Pixel8& destination = channel.frame()[kCentreRight + distance];
    destination = addSaturating(destination, scalePixel(colour, level));
  }
}

void renderWaveformFamily(ChannelRenderState& channel,
                          const VisualAudioFrameView& visual,
                          const std::uint16_t mode,
                          const float delta_seconds) noexcept {
  ChannelEffectState& effect = channel.effectState();
  WaveformEffectState* waveform_state = &effect.waveform;
  float scroll_pixels_per_second = kWaveformScrollPixelsPerSecond;
  float nominal_peak_alpha = 0.08F;
  float nominal_colour_alpha = 0.08F;
  bool use_history = false;
  if (mode == 7U) {
    waveform_state = &effect.waveform_fast;
    scroll_pixels_per_second = kWaveformFastScrollPixelsPerSecond;
    nominal_peak_alpha = 0.05F;
    nominal_colour_alpha = 0.05F;
  } else if (mode == 11U) {
    waveform_state = &effect.waveform_hybrid;
    scroll_pixels_per_second = kWaveformHybridScrollPixelsPerSecond;
    use_history = true;
  }
  scroll_pixels_per_second *= std::clamp(
      channel.controls().vp_waveform_shift_rate / 120.0F, 0.0F, 2.0F);

  const float peak = clamp01(
      visual.waveform.peak_scaled > channel.focusedAudio().peak_scaled
          ? visual.waveform.peak_scaled
          : channel.focusedAudio().peak_scaled);
  waveform_state->peak_scaled_last +=
      (peak - waveform_state->peak_scaled_last) *
      dtCorrectAlpha(nominal_peak_alpha, delta_seconds);
  const float smoothed_peak = clamp01(waveform_state->peak_scaled_last);
  const bool reactive =
      visual.waveform.raw_maximum >
          static_cast<float>(channel.controls().sweet_spot_min_level) *
              std::clamp(channel.controls().vp_waveform_raw_margin, 1.0F, 3.0F) &&
      (peak >= clamp01(channel.controls().vp_waveform_peak_floor) ||
       channel.focusedAudio().vu_level >=
           clamp01(channel.controls().vp_waveform_vu_floor));

  float target_retention = clamp01(channel.controls().vp_waveform_idle_fade);
  if (reactive) {
    const float reduction = std::clamp(
        channel.controls().vp_waveform_active_fade, 0.0F, 0.50F);
    target_retention = mode == 11U
        ? 1.0F - reduction * (1.0F - smoothed_peak)
        : 1.0F - reduction * peak;
    target_retention = std::clamp(
        target_retention,
        clamp01(channel.controls().vp_waveform_idle_fade), 0.999F);
  }
  const float pixels_per_frame = modes::liveinessEffective(
      mode, scroll_pixels_per_second / kNominalFramesPerSecond,
      channel.controls().liveiness);
  transportOutward(channel.frame(), channel.previousFrame(), pixels_per_frame,
                   clamp01(target_retention), delta_seconds);
  clearLeftHalf(channel.frame());

  if (mode != 7U || reactive) {
    const Pixel8 colour = waveformLastColour(
        channel, *waveform_state, smoothed_peak, delta_seconds,
        nominal_colour_alpha);
    depositWaveformSeed(channel, visual.waveform, colour, smoothed_peak,
                        use_history);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

void renderWaveformK1(ChannelRenderState& channel,
                      const VisualAudioFrameView& visual,
                      const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const float dt = sanitiseDeltaSeconds(delta_seconds);
  const float peak = clamp01(
      visual.waveform.peak_scaled > channel.focusedAudio().peak_scaled
          ? visual.waveform.peak_scaled
          : channel.focusedAudio().peak_scaled);
  const float vu = clamp01(channel.focusedAudio().vu_level);
  const bool present = peak > 0.02F || vu > 0.02F;
  const bool silent =
      (channel.focusedAudio().event_flags & contract::kEventSilence) != 0U &&
      !present;

  const float hold_tau = present ? 0.02F : 0.50F;
  state.waveform_k1_hold_env +=
      ((present ? 1.0F : 0.0F) - state.waveform_k1_hold_env) *
      exponentialAlpha(dt, hold_tau);
  const float silence_tau = silent ? 0.05F : 0.30F;
  state.waveform_k1_silence_scale +=
      ((silent ? 0.0F : 1.0F) - state.waveform_k1_silence_scale) *
      exponentialAlpha(dt, silence_tau);
  state.waveform_k1_peak_ema1 +=
      (peak - state.waveform_k1_peak_ema1) *
      exponentialAlpha(dt, kWaveformK1PeakTauOne);
  state.waveform_k1_peak_last +=
      (state.waveform_k1_peak_ema1 - state.waveform_k1_peak_last) *
      exponentialAlpha(dt, kWaveformK1PeakTauTwo);

  const Pixel8 raw_colour = injectionColour(channel);
  const float colour_alpha = exponentialAlpha(dt, kWaveformK1ColourTau);
  state.waveform_k1_dot.red +=
      (static_cast<float>(raw_colour.red) / 255.0F -
       state.waveform_k1_dot.red) * colour_alpha;
  state.waveform_k1_dot.green +=
      (static_cast<float>(raw_colour.green) / 255.0F -
       state.waveform_k1_dot.green) * colour_alpha;
  state.waveform_k1_dot.blue +=
      (static_cast<float>(raw_colour.blue) / 255.0F -
       state.waveform_k1_dot.blue) * colour_alpha;
  float gain = clamp01(state.waveform_k1_hold_env) *
               clamp01(state.waveform_k1_silence_scale);
  if (present && gain < peak) {
    gain = peak;
  }
  const Pixel8 colour = quantiseLinearRgb(
      {state.waveform_k1_dot.red * gain,
       state.waveform_k1_dot.green * gain,
       state.waveform_k1_dot.blue * gain});

  float decay_rate =
      kWaveformK1MinimumDecayRate + kWaveformK1DecayScale * peak;
  const float quiet = clamp01(state.waveform_k1_hold_env <
                                      state.waveform_k1_silence_scale
                                  ? state.waveform_k1_hold_env
                                  : state.waveform_k1_silence_scale);
  if (quiet < 0.90F) {
    decay_rate += kWaveformK1SilenceDecay * (1.0F - quiet);
  }
  const float nominal_retention =
      ::expf(-decay_rate / kNominalFramesPerSecond);
  transportOutward(
      channel.frame(), channel.previousFrame(),
      modes::liveinessEffective(
          32U, kWaveformK1ScrollPixelsPerSecond / kNominalFramesPerSecond,
          channel.controls().liveiness),
      nominal_retention, dt);
  clearLeftHalf(channel.frame());
  if (present) {
    depositWaveformSeed(channel, visual.waveform, colour,
                        clamp01(state.waveform_k1_peak_last), true);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

Pixel8 classColour(ChannelRenderState& channel, const float phase,
                   const float level) noexcept {
  if (channel.controls().palette_mode_enabled) {
    return explicitPaletteColour(channel.controls(), phase, level);
  }
  return scalePixel(injectionColour(channel), level);
}

void renderComet(ChannelRenderState& channel,
                 const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float frames = nominalFrames(delta_seconds);
  transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                   kCometTrailAlpha, delta_seconds);
  clearLeftHalf(channel.frame());

  const bool fresh = audio.onset_event_id != state.comet_last_event_id;
  state.comet_last_event_id = audio.onset_event_id;
  if (fresh && (audio.event_flags & contract::kEventBassOnset) != 0U) {
    const float strength = clamp01(audio.bass_onset_strength);
    if (strength >= kCometMinimumStrength) {
      std::size_t slot = 0U;
      for (std::size_t index = 1U; index < kCometPoolSize; ++index) {
        if (state.comet_life[index] < state.comet_life[slot]) {
          slot = index;
        }
      }
      state.comet_pos[slot] = static_cast<float>(kCentreRight);
      state.comet_vel[slot] =
          (kCometSpeedMinimum +
           kCometSpeedMood * clamp01(channel.controls().mood)) *
          static_cast<float>(kPixelsPerChannel) / 128.0F;
      state.comet_hue[slot] = 0.04F;
      state.comet_size[slot] = kCometSize * (0.80F + 0.40F * strength);
      state.comet_life[slot] = 1.0F;
    }
  }

  for (std::size_t index = 0U; index < kCometPoolSize; ++index) {
    if (state.comet_life[index] <= 0.01F) {
      continue;
    }
    state.comet_pos[index] +=
        modes::liveinessEffective(13U, state.comet_vel[index],
                                  channel.controls().liveiness) *
        frames;
    if (state.comet_pos[index] >= static_cast<float>(kPixelsPerChannel)) {
      state.comet_life[index] = 0.0F;
      continue;
    }
    const Pixel8 colour = classColour(channel, state.comet_hue[index], 1.0F);
    const float life = state.comet_life[index];
    const float radius = state.comet_size[index];
    const int centre = static_cast<int>(state.comet_pos[index] + 0.5F);
    const int reach = static_cast<int>(radius) + kCometGlow;
    for (int offset = -reach; offset <= reach; ++offset) {
      const int pixel = centre + offset;
      if (pixel < static_cast<int>(kCentreRight) ||
          pixel >= static_cast<int>(kPixelsPerChannel)) {
        continue;
      }
      const float distance =
          static_cast<float>(offset < 0 ? -offset : offset);
      const bool trailing = offset < 0;
      const float span =
          trailing ? radius * kCometWakeStretch : radius * 0.70F;
      float weight = 1.0F - distance / (span + 1.0F);
      if (weight < 0.0F) {
        weight = 0.0F;
      }
      if (offset == 0) {
        weight = 1.0F;
      }
      Pixel8& destination =
          channel.frame()[static_cast<std::size_t>(pixel)];
      destination = addSaturating(
          destination, scalePixel(colour, life * weight * kCometHeadGain));
    }
    state.comet_life[index] *= ::powf(kCometLifeAlpha, frames);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

std::size_t claimPercussionSlot(ChannelEffectState& state) noexcept {
  for (std::size_t index = 0U; index < kPercussionPoolSize; ++index) {
    if (state.percussion_active[index] == 0U) {
      return index;
    }
  }
  return kPercussionPoolSize;
}

void spawnPercussion(ChannelEffectState& state, const std::size_t slot,
                     const float position, const float velocity,
                     const float life, const float intensity,
                     const float hue) noexcept {
  state.percussion_pos[slot] = position;
  state.percussion_last_pos[slot] = position;
  state.percussion_velocity[slot] = velocity;
  state.percussion_life[slot] = life;
  state.percussion_life_max[slot] = life;
  state.percussion_intensity[slot] = intensity;
  state.percussion_hue[slot] = hue;
  state.percussion_active[slot] = 1U;
}

std::size_t percussionPixel(const float position) noexcept {
  const float bounded = clamp01(position);
  return kCentreRight + static_cast<std::size_t>(
                            bounded * static_cast<float>(kPixelsPerHalf - 1U));
}

void drawPercussionStreak(ChannelRenderState& channel, const float from,
                          const float to, const Pixel8 colour) noexcept {
  const std::size_t first = percussionPixel(from);
  const std::size_t last = percussionPixel(to);
  const std::size_t low = first < last ? first : last;
  const std::size_t high = first < last ? last : first;
  for (std::size_t pixel = low; pixel <= high; ++pixel) {
    channel.frame()[pixel] = addSaturating(channel.frame()[pixel], colour);
  }
}

void renderPercussionBurst(ChannelRenderState& channel,
                           const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float dt = delta_seconds < 0.001F
                       ? 0.001F
                       : (delta_seconds > 0.05F ? 0.05F : delta_seconds);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  if (silent) {
    for (std::size_t index = 0U; index < kPercussionPoolSize; ++index) {
      state.percussion_active[index] = 0U;
      state.percussion_life[index] = 0.0F;
    }
  } else {
    transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                     kPercussionTrailAlpha, delta_seconds);
  }
  clearLeftHalf(channel.frame());

  const bool kick_fresh =
      (audio.event_flags & contract::kEventKick) != 0U &&
      audio.kick_event_id != state.percussion_kick_id;
  const bool snare_fresh =
      (audio.event_flags & contract::kEventSnare) != 0U &&
      audio.snare_event_id != state.percussion_snare_id;
  const bool hat_fresh =
      (audio.event_flags & contract::kEventHihat) != 0U &&
      audio.hihat_event_id != state.percussion_hihat_id;
  state.percussion_kick_id = audio.kick_event_id;
  state.percussion_snare_id = audio.snare_event_id;
  state.percussion_hihat_id = audio.hihat_event_id;

  const bool presence =
      audio.spectral_energy >= 0.08F || audio.novelty >= 0.08F;
  if (!silent && presence) {
    std::size_t free_slots = 0U;
    for (std::uint8_t active : state.percussion_active) {
      free_slots += active == 0U ? 1U : 0U;
    }
    const bool starved = free_slots < 2U;
    if (kick_fresh) {
      const std::size_t slot = claimPercussionSlot(state);
      if (slot < kPercussionPoolSize) {
        spawnPercussion(state, slot, 0.0F, kPercussionKickSpeed,
                        kPercussionKickLife,
                        0.45F + 0.55F * clamp01(audio.kick_strength), 0.02F);
      }
    }
    if (snare_fresh && !starved) {
      const float intensity =
          0.35F + 0.45F * clamp01(audio.snare_strength);
      for (int direction : {1, -1}) {
        const std::size_t slot = claimPercussionSlot(state);
        if (slot >= kPercussionPoolSize) {
          break;
        }
        spawnPercussion(state, slot, kPercussionSnarePosition,
                        static_cast<float>(direction) * kPercussionSnareSpeed,
                        kPercussionSnareLife, intensity, 0.33F);
      }
    }
    if (hat_fresh && !starved) {
      const std::size_t slot = claimPercussionSlot(state);
      if (slot < kPercussionPoolSize) {
        const float position = state.percussion_hat_left
                                   ? kPercussionHatPositionA
                                   : kPercussionHatPositionB;
        state.percussion_hat_left = !state.percussion_hat_left;
        float intensity = 0.15F + 0.35F * clamp01(audio.hihat_strength);
        if (intensity > 0.40F) {
          intensity = 0.40F;
        }
        spawnPercussion(state, slot, position, kPercussionHatSpeed,
                        kPercussionHatLife, intensity, 0.66F);
      }
    }
  }

  if (!silent) {
    for (std::size_t index = 0U; index < kPercussionPoolSize; ++index) {
      if (state.percussion_active[index] == 0U) {
        continue;
      }
      state.percussion_last_pos[index] = state.percussion_pos[index];
      state.percussion_pos[index] +=
          modes::liveinessEffective(26U, state.percussion_velocity[index],
                                    channel.controls().liveiness) *
          dt;
      state.percussion_life[index] -= dt;
      if (state.percussion_life[index] <= 0.0F ||
          state.percussion_pos[index] < 0.0F ||
          state.percussion_pos[index] > 1.0F) {
        state.percussion_active[index] = 0U;
        state.percussion_life[index] = 0.0F;
        continue;
      }
      const float envelope = state.percussion_life_max[index] > 0.0F
                                 ? state.percussion_life[index] /
                                       state.percussion_life_max[index]
                                 : 0.0F;
      const float weight =
          clamp01(state.percussion_intensity[index] * envelope);
      if (weight <= 0.008F) {
        continue;
      }
      drawPercussionStreak(
          channel, state.percussion_last_pos[index],
          state.percussion_pos[index],
          classColour(channel, state.percussion_hue[index], weight));
    }
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

float smoothstep(const float low, const float high,
                 const float value) noexcept {
  if (high <= low) {
    return value >= high ? 1.0F : 0.0F;
  }
  const float position = clamp01((value - low) / (high - low));
  return position * position * (3.0F - 2.0F * position);
}

float boundedBpm(const float bpm) noexcept {
  if (!std::isfinite(bpm) || bpm < 40.0F) {
    return 40.0F;
  }
  return bpm > 200.0F ? 200.0F : bpm;
}

void renderTempoRiver(ChannelRenderState& channel,
                      const VisualAudioFrameView& visual,
                      const float delta_seconds) noexcept {
  constexpr float kTwoPi = 6.2831853071795864769F;
  const float scale = static_cast<float>(kPixelsPerChannel) / 128.0F;
  const float bpm = boundedBpm(visual.tempo.bpm);
  float phase = visual.tempo.phase01;
  phase = wrap01(phase);
  float envelope = 1.0F + kTempoRiverDepth * ::cosf(kTwoPi * phase);
  if (envelope < 0.0F) {
    envelope = 0.0F;
  }
  const float tempo_pixels_per_second =
      kTempoRiverPixelsPerBeat * (bpm / 60.0F) * scale * envelope;
  const float idle_pixels_per_second =
      kTempoRiverIdlePixelsPerFrame * scale * kNominalFramesPerSecond;
  const float gate = smoothstep(kTempoConfidenceLow, kTempoConfidenceHigh,
                                visual.tempo.confidence);
  float velocity = idle_pixels_per_second +
                   (tempo_pixels_per_second - idle_pixels_per_second) * gate;
  velocity = modes::liveinessEffective(19U, velocity,
                                       channel.controls().liveiness);
  const float floor_velocity = kTempoRiverFloorPixelsPerFrame * scale *
                               kNominalFramesPerSecond;
  const float maximum_velocity = kTempoRiverMaximumPixelsPerFrame * scale *
                                 kNominalFramesPerSecond;
  if (velocity < floor_velocity) {
    velocity = floor_velocity;
  } else if (velocity > maximum_velocity) {
    velocity = maximum_velocity;
  }
  transportOutward(channel.frame(), channel.previousFrame(),
                   velocity / kNominalFramesPerSecond, kRiverAlpha,
                   delta_seconds);
  clearLeftHalf(channel.frame());
  injectSpectrumRiver(channel);
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

void drawTempoComet(ChannelRenderState& channel, const std::size_t index,
                    const float frames) noexcept {
  ChannelEffectState& state = channel.effectState();
  state.tempo_comet_pos[index] +=
      modes::liveinessEffective(20U, state.tempo_comet_vel[index],
                                channel.controls().liveiness) *
      frames / kNominalFramesPerSecond;
  if (state.tempo_comet_pos[index] >=
      static_cast<float>(kPixelsPerChannel)) {
    state.tempo_comet_life[index] = 0.0F;
    return;
  }
  const float life = state.tempo_comet_life[index];
  const float radius = state.tempo_comet_size[index];
  const int centre =
      static_cast<int>(state.tempo_comet_pos[index] + 0.5F);
  const int reach = static_cast<int>(radius) + kCometGlow;
  const float travel = clamp01(
      (state.tempo_comet_pos[index] - static_cast<float>(kCentreRight)) /
      static_cast<float>(kPixelsPerHalf));
  const Pixel8 colour =
      classColour(channel, travel * kTempoCometPaletteSpread, 1.0F);
  for (int offset = -reach; offset <= reach; ++offset) {
    const int pixel = centre + offset;
    if (pixel < static_cast<int>(kCentreRight) ||
        pixel >= static_cast<int>(kPixelsPerChannel)) {
      continue;
    }
    const float distance =
        static_cast<float>(offset < 0 ? -offset : offset);
    const float span = offset < 0 ? radius * kCometWakeStretch
                                  : radius * 0.70F;
    float weight = 1.0F - distance / (span + 1.0F);
    if (weight < 0.0F) {
      weight = 0.0F;
    }
    if (offset == 0) {
      weight = 1.0F;
    }
    Pixel8& destination =
        channel.frame()[static_cast<std::size_t>(pixel)];
    destination = addSaturating(
        destination, scalePixel(colour, life * weight * kCometHeadGain));
  }
  state.tempo_comet_life[index] *= ::powf(kCometLifeAlpha, frames);
}

void renderTempoComet(ChannelRenderState& channel,
                      const VisualAudioFrameView& visual,
                      const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const float dt = delta_seconds < 0.001F
                       ? 0.001F
                       : (delta_seconds > 0.05F ? 0.05F : delta_seconds);
  const float frames = dt * kNominalFramesPerSecond;
  transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                   kCometTrailAlpha, dt);
  clearLeftHalf(channel.frame());

  const float gate = smoothstep(kTempoConfidenceLow, kTempoConfidenceHigh,
                                visual.tempo.confidence);
  const bool confident = gate >= 0.50F && visual.tempo.locked;
  const float bpm = boundedBpm(visual.tempo.bpm);
  if (confident) {
    if (state.tempo_comet_locked_bpm <= 0.0F) {
      state.tempo_comet_locked_bpm = bpm;
    } else {
      const float ratio = bpm / state.tempo_comet_locked_bpm;
      if (ratio > 0.70F && ratio < 1.40F) {
        state.tempo_comet_locked_bpm +=
            (bpm - state.tempo_comet_locked_bpm) *
            dtCorrectAlpha(kTempoCometNominalBpmAlpha, dt);
      }
    }
    state.tempo_comet_coast_beats = 0U;
  }
  const float run_bpm = state.tempo_comet_locked_bpm > 0.0F
                            ? state.tempo_comet_locked_bpm
                            : bpm;
  state.tempo_comet_beat_phase += run_bpm / 60.0F * dt;
  if (confident) {
    float error = wrap01(visual.tempo.phase01) -
                  state.tempo_comet_beat_phase;
    error -= ::floorf(error);
    if (error > 0.50F) {
      error -= 1.0F;
    }
    state.tempo_comet_beat_phase +=
        kTempoCometResynchroniseHz * dt * error;
  }
  bool internal_beat = false;
  if (state.tempo_comet_beat_phase >= 1.0F) {
    state.tempo_comet_beat_phase -=
        ::floorf(state.tempo_comet_beat_phase);
    internal_beat = true;
  } else if (state.tempo_comet_beat_phase < 0.0F) {
    state.tempo_comet_beat_phase = 0.0F;
  }

  if (internal_beat) {
    if (!confident) {
      ++state.tempo_comet_coast_beats;
    }
    const bool have_lock = state.tempo_comet_locked_bpm > 0.0F &&
                           state.tempo_comet_coast_beats <=
                               kTempoCometCoastBeats;
    if (have_lock) {
      float strength = clamp01(visual.tempo.beat_strength);
      if (strength < kTempoCometStrengthFloor) {
        strength = kTempoCometStrengthFloor;
      }
      const float beat_seconds = 60.0F / run_bpm;
      const float reach_fraction =
          kTempoCometReachMinimum +
          (kTempoCometReachMaximum - kTempoCometReachMinimum) * strength;
      const float velocity =
          reach_fraction * static_cast<float>(kPixelsPerHalf) / beat_seconds;
      std::size_t slot = 0U;
      for (std::size_t index = 1U; index < kCometPoolSize; ++index) {
        if (state.tempo_comet_life[index] <
            state.tempo_comet_life[slot]) {
          slot = index;
        }
      }
      state.tempo_comet_pos[slot] = static_cast<float>(kCentreRight);
      state.tempo_comet_vel[slot] = velocity;
      state.tempo_comet_size[slot] =
          kCometSize * (0.80F + 0.40F * strength);
      state.tempo_comet_life[slot] = 1.0F;
    } else {
      state.tempo_comet_locked_bpm = 0.0F;
    }
  }
  for (std::size_t index = 0U; index < kCometPoolSize; ++index) {
    if (state.tempo_comet_life[index] > 0.01F) {
      drawTempoComet(channel, index, frames);
    }
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

float tempoVelocityPixelsPerSecond(
    const audio::TempoTrackerEvent& tempo, const float pixels_per_beat,
    const float depth, const float idle_pixels_per_second,
    const bool stop_idle) noexcept {
  constexpr float kTwoPi = 6.2831853071795864769F;
  const float bpm = boundedBpm(tempo.bpm);
  float envelope =
      1.0F + depth * ::cosf(kTwoPi * wrap01(tempo.phase01));
  if (envelope < 0.0F) {
    envelope = 0.0F;
  }
  const float tempo_velocity = pixels_per_beat * bpm / 60.0F * envelope;
  const float gate = smoothstep(kTempoConfidenceLow, kTempoConfidenceHigh,
                                tempo.confidence);
  const float idle = stop_idle ? 0.0F : idle_pixels_per_second;
  return idle + (tempo_velocity - idle) * gate;
}

void renderWaveformTempo(ChannelRenderState& channel,
                         const VisualAudioFrameView& visual,
                         const float delta_seconds) noexcept {
  const float peak = clamp01(
      visual.waveform.peak_scaled > channel.focusedAudio().peak_scaled
          ? visual.waveform.peak_scaled
          : channel.focusedAudio().peak_scaled);
  const float level = peak > channel.focusedAudio().vu_level
                          ? peak
                          : clamp01(channel.focusedAudio().vu_level);
  const bool present =
      level >= 0.02F &&
      !(channel.focusedAudio().spectral_energy < 0.08F &&
        channel.focusedAudio().novelty < 0.08F && level < 0.08F);
  const bool silent =
      (channel.focusedAudio().event_flags & contract::kEventSilence) != 0U &&
      peak < 0.02F;
  const float velocity = modes::liveinessEffective(
      18U, tempoVelocityPixelsPerSecond(visual.tempo, 24.0F, 0.50F, 30.0F,
                                        silent),
      channel.controls().liveiness);
  const float retention = present ? 1.0F - 0.10F * peak : 0.82F;
  transportOutward(channel.frame(), channel.previousFrame(),
                   velocity / kNominalFramesPerSecond, retention,
                   delta_seconds);
  clearLeftHalf(channel.frame());
  if (present) {
    Pixel8 colour = injectionColour(channel);
    colour = scalePixel(colour, peak);
    channel.frame()[kCentreRight] =
        addSaturating(channel.frame()[kCentreRight], colour);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

void renderTempoCometAnticipate(ChannelRenderState& channel,
                                const VisualAudioFrameView& visual,
                                const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const float dt = delta_seconds < 0.001F
                       ? 0.001F
                       : (delta_seconds > 0.05F ? 0.05F : delta_seconds);
  const float frames = dt * kNominalFramesPerSecond;
  transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                   kCometTrailAlpha, dt);
  clearLeftHalf(channel.frame());

  const bool confident =
      smoothstep(kTempoConfidenceLow, kTempoConfidenceHigh,
                 visual.tempo.confidence) >= 0.50F &&
      visual.tempo.locked;
  const float bpm = boundedBpm(visual.tempo.bpm);
  if (confident) {
    if (state.anticipate_locked_bpm <= 0.0F) {
      state.anticipate_locked_bpm = bpm;
    } else {
      const float ratio = bpm / state.anticipate_locked_bpm;
      if (ratio > 0.70F && ratio < 1.40F) {
        state.anticipate_locked_bpm +=
            (bpm - state.anticipate_locked_bpm) *
            dtCorrectAlpha(kTempoCometNominalBpmAlpha, dt);
      }
    }
    state.anticipate_coast_beats = 0U;
  }
  const float run_bpm = state.anticipate_locked_bpm > 0.0F
                            ? state.anticipate_locked_bpm
                            : bpm;
  state.anticipate_beat_phase += run_bpm / 60.0F * dt;
  if (confident) {
    float error = wrap01(visual.tempo.phase01) - state.anticipate_beat_phase;
    error -= ::floorf(error);
    if (error > 0.50F) {
      error -= 1.0F;
    }
    state.anticipate_beat_phase +=
        kTempoCometResynchroniseHz * dt * error;
  }
  bool internal_beat = false;
  if (state.anticipate_beat_phase >= 1.0F) {
    state.anticipate_beat_phase -= ::floorf(state.anticipate_beat_phase);
    internal_beat = true;
  } else if (state.anticipate_beat_phase < 0.0F) {
    state.anticipate_beat_phase = 0.0F;
  }
  if (internal_beat) {
    if (!confident) {
      ++state.anticipate_coast_beats;
    }
    const bool have_lock = state.anticipate_locked_bpm > 0.0F &&
                           state.anticipate_coast_beats <=
                               kTempoCometCoastBeats;
    if (have_lock) {
      float strength = clamp01(visual.tempo.beat_strength);
      if (strength < kTempoCometStrengthFloor) {
        strength = kTempoCometStrengthFloor;
      }
      std::size_t slot = 0U;
      for (std::size_t index = 1U; index < kCometPoolSize; ++index) {
        if (state.anticipate_life[index] < state.anticipate_life[slot]) {
          slot = index;
        }
      }
      state.anticipate_launch[slot] = static_cast<float>(kCentreRight);
      state.anticipate_target[slot] = modes::liveinessEffective(
          27U,
          (kTempoCometReachMinimum +
           (kTempoCometReachMaximum - kTempoCometReachMinimum) * strength) *
              static_cast<float>(kPixelsPerHalf),
          channel.controls().liveiness);
      state.anticipate_period[slot] = 60.0F / run_bpm;
      state.anticipate_time[slot] = 0.0F;
      state.anticipate_size[slot] =
          kCometSize * (0.80F + 0.40F * strength);
      state.anticipate_life[slot] = 1.0F;
    } else {
      state.anticipate_locked_bpm = 0.0F;
    }
  }

  for (std::size_t index = 0U; index < kCometPoolSize; ++index) {
    if (state.anticipate_life[index] <= 0.01F) {
      continue;
    }
    const float period = state.anticipate_period[index];
    if (period <= 0.0F) {
      state.anticipate_life[index] = 0.0F;
      continue;
    }
    if (state.anticipate_time[index] < period) {
      state.anticipate_time[index] += dt;
      const float ideal = state.anticipate_beat_phase * period;
      float error = ideal - state.anticipate_time[index];
      error -= period * ::floorf(error / period + 0.50F);
      float slew = 0.15F * frames;
      if (slew > 1.0F) {
        slew = 1.0F;
      }
      state.anticipate_time[index] += error * slew;
      if (state.anticipate_time[index] < 0.0F) {
        state.anticipate_time[index] = 0.0F;
      } else if (state.anticipate_time[index] > period) {
        state.anticipate_time[index] = period;
      }
    }
    const float progress = clamp01(state.anticipate_time[index] / period);
    const float inverse = 1.0F - progress;
    const float position =
        state.anticipate_launch[index] +
        state.anticipate_target[index] * (1.0F - inverse * inverse);
    if (position >= static_cast<float>(kPixelsPerChannel)) {
      state.anticipate_life[index] = 0.0F;
      continue;
    }
    const Pixel8 colour = classColour(
        channel, progress * kTempoCometPaletteSpread, 1.0F);
    const float radius = state.anticipate_size[index];
    const int centre = static_cast<int>(position + 0.5F);
    const int reach = static_cast<int>(radius) + kCometGlow;
    for (int offset = -reach; offset <= reach; ++offset) {
      const int pixel = centre + offset;
      if (pixel < static_cast<int>(kCentreRight) ||
          pixel >= static_cast<int>(kPixelsPerChannel)) {
        continue;
      }
      const float distance =
          static_cast<float>(offset < 0 ? -offset : offset);
      const float span = offset < 0 ? radius * kCometWakeStretch
                                    : radius * 0.70F;
      float weight = 1.0F - distance / (span + 1.0F);
      if (weight < 0.0F) {
        weight = 0.0F;
      }
      Pixel8& destination =
          channel.frame()[static_cast<std::size_t>(pixel)];
      destination = addSaturating(
          destination,
          scalePixel(colour, state.anticipate_life[index] * weight *
                                 kCometHeadGain));
    }
    state.anticipate_life[index] *= ::powf(kCometLifeAlpha, frames);
  }
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

void renderTempoRiverWalk(ChannelRenderState& channel,
                          const VisualAudioFrameView& visual,
                          const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const float dt = delta_seconds < 0.001F
                       ? 0.001F
                       : (delta_seconds > 0.05F ? 0.05F : delta_seconds);
  const float gate = smoothstep(kTempoConfidenceLow, kTempoConfidenceHigh,
                                visual.tempo.confidence);
  const float phase = wrap01(visual.tempo.phase01);
  const bool beat_wrap = phase + 0.50F < state.river_walk_last_phase;
  state.river_walk_last_phase = phase;
  if (gate > 0.0F && beat_wrap) {
    ++state.river_walk_beats;
    if (state.river_walk_beats >= 4U) {
      state.river_walk_beats = 0U;
      state.river_walk_target = wrap01(state.river_walk_target + 0.125F);
    }
  }
  float offset_error = state.river_walk_target - state.river_walk_offset;
  offset_error -= ::floorf(offset_error + 0.50F);
  const float maximum_step = 0.25F * dt;
  if (offset_error > maximum_step) {
    offset_error = maximum_step;
  } else if (offset_error < -maximum_step) {
    offset_error = -maximum_step;
  }
  state.river_walk_offset =
      wrap01(state.river_walk_offset + offset_error);

  const float scale = static_cast<float>(kPixelsPerChannel) / 128.0F;
  float velocity = tempoVelocityPixelsPerSecond(
      visual.tempo, kTempoRiverPixelsPerBeat * scale, kTempoRiverDepth,
      kTempoRiverIdlePixelsPerFrame * scale * kNominalFramesPerSecond,
      false);
  velocity = modes::liveinessEffective(29U, velocity,
                                       channel.controls().liveiness);
  const float floor_velocity = kTempoRiverFloorPixelsPerFrame * scale *
                               kNominalFramesPerSecond;
  const float maximum_velocity = kTempoRiverMaximumPixelsPerFrame * scale *
                                 kNominalFramesPerSecond;
  if (velocity < floor_velocity) {
    velocity = floor_velocity;
  } else if (velocity > maximum_velocity) {
    velocity = maximum_velocity;
  }
  transportOutward(channel.frame(), channel.previousFrame(),
                   velocity / kNominalFramesPerSecond, kRiverAlpha, dt);
  clearLeftHalf(channel.frame());
  injectSpectrumRiverAtOffset(channel, state.river_walk_offset);
  snapshotHistory(channel);
  if (channel.controls().mirror_enabled) {
    mirrorRightHalf(channel.frame());
  }
}

float tauAlpha(const float tau_seconds, const float delta_seconds) noexcept {
  if (tau_seconds <= 0.0F) {
    return 1.0F;
  }
  return clamp01(1.0F - ::expf(-sanitiseDeltaSeconds(delta_seconds) /
                               tau_seconds));
}

float asymmetricFollower(const float current, const float target,
                         const float attack_tau, const float release_tau,
                         const float delta_seconds) noexcept {
  const float tau = target > current ? attack_tau : release_tau;
  return current + (target - current) * tauAlpha(tau, delta_seconds);
}

std::size_t rightPixelFromUnit(const float position) noexcept {
  return kCentreRight + static_cast<std::size_t>(
                            clamp01(position) *
                            static_cast<float>(kPixelsPerHalf - 1U));
}

void drawRightStreak(ChannelRenderState& channel, const float from,
                     const float to, const Pixel8 colour) noexcept {
  const std::size_t first = rightPixelFromUnit(from);
  const std::size_t last = rightPixelFromUnit(to);
  const std::size_t low = first < last ? first : last;
  const std::size_t high = first < last ? last : first;
  for (std::size_t pixel = low; pixel <= high; ++pixel) {
    channel.frame()[pixel] =
        addSaturating(channel.frame()[pixel], colour);
  }
}

float heldChordHue(ChannelEffectState& state,
                   const contract::AudioFeaturesV1& audio,
                   const float centroid_hue,
                   const float delta_seconds) noexcept {
  const bool chord_valid =
      audio.chord_type != contract::ChordTypeV1::kNone &&
      audio.chord_confidence >= 0.625F;
  const std::uint8_t root =
      static_cast<std::uint8_t>(audio.chord_root_a_origin % 12U);
  if (chord_valid) {
    if (root != state.dense_chord_candidate_root) {
      state.dense_chord_candidate_root = root;
      state.dense_chord_candidate_ms = 0.0F;
    } else {
      state.dense_chord_candidate_ms +=
          sanitiseDeltaSeconds(delta_seconds) * 1000.0F;
      if (state.dense_chord_candidate_ms >= 250.0F) {
        state.dense_chord_held_root = root;
      }
    }
  } else {
    state.dense_chord_candidate_ms = 0.0F;
  }

  float target = centroid_hue;
  if (chord_valid) {
    float confidence = clamp01((audio.chord_confidence - 0.625F) / 0.375F);
    confidence *= 0.60F;
    const float root_hue =
        static_cast<float>(state.dense_chord_held_root) / 12.0F;
    float pull = root_hue - target;
    pull -= ::floorf(pull + 0.50F);
    target = wrap01(target + pull * confidence);
  }
  float difference = target - state.dense_chord_hue;
  difference -= ::floorf(difference + 0.50F);
  const float maximum_step = 0.50F * sanitiseDeltaSeconds(delta_seconds);
  if (difference > maximum_step) {
    difference = maximum_step;
  } else if (difference < -maximum_step) {
    difference = -maximum_step;
  }
  state.dense_chord_hue = wrap01(state.dense_chord_hue + difference);
  return state.dense_chord_hue;
}

void renderDenseForge(ChannelRenderState& channel, const bool chord_aware,
                      const float delta_seconds) noexcept {
  constexpr float kOmega[kDenseForgeLatticeSize] = {
      1.00F, 1.12F, 1.25F, 1.33F, 1.50F, 1.67F, 1.85F, 2.00F};
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float dt = sanitiseDeltaSeconds(delta_seconds);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  const bool present =
      audio.spectral_energy >= 0.08F || audio.novelty >= 0.08F;
  const float transient = clamp01(audio.transient_level);
  const float activity_target =
      clamp01(0.30F * audio.novelty + 0.35F * audio.spectral_energy +
              0.35F * transient);
  state.dense_activity_env +=
      (activity_target - state.dense_activity_env) *
      dtCorrectAlpha(0.05F, dt);
  if (!state.dense_initialised) {
    for (std::size_t index = 0U; index < kDenseForgeLatticeSize; ++index) {
      const float position =
          static_cast<float>(index + 1U) /
          static_cast<float>(kDenseForgeLatticeSize + 1U);
      state.dense_lattice_pos[index] = position;
      state.dense_lattice_last[index] = position;
    }
    state.dense_initialised = true;
  }

  state.dense_carrier = wrap01(
      state.dense_carrier +
      dt * (1.20F + 7.0F * clamp01(audio.novelty) + 3.0F * transient));
  const std::size_t bin_count = spectrumBinCount(audio);
  for (std::size_t index = 0U; index < kDenseForgeLatticeSize; ++index) {
    state.dense_lattice_last[index] = state.dense_lattice_pos[index];
    float band = 0.0F;
    std::size_t samples = 0U;
    if (bin_count > 0U) {
      const std::size_t begin = index * bin_count / kDenseForgeLatticeSize;
      const std::size_t end = (index + 1U) * bin_count /
                              kDenseForgeLatticeSize;
      for (std::size_t bin = begin; bin < end; ++bin) {
        band += clamp01(audio.spectrum[bin]);
        ++samples;
      }
    }
    band = samples > 0U ? band / static_cast<float>(samples) : 0.0F;
    const float rest =
        static_cast<float>(index + 1U) /
        static_cast<float>(kDenseForgeLatticeSize + 1U);
    const float target = clamp01(rest + (band - 0.50F) * 0.20F);
    const float detune = ::sinf(state.dense_carrier * 6.28318530718F *
                                kOmega[index]);
    float force = 22.0F * (target - state.dense_lattice_pos[index]) -
                  7.50F * state.dense_lattice_vel[index] +
                  2.20F * detune * state.dense_activity_env;
    force += 5.0F * transient * (index % 2U == 0U ? 1.0F : -1.0F);
    state.dense_lattice_vel[index] += force * dt;
    if (state.dense_lattice_vel[index] > 0.32F) {
      state.dense_lattice_vel[index] = 0.32F;
    } else if (state.dense_lattice_vel[index] < -0.32F) {
      state.dense_lattice_vel[index] = -0.32F;
    }
    state.dense_lattice_pos[index] += state.dense_lattice_vel[index] * dt;
    if (state.dense_lattice_pos[index] < 0.02F) {
      state.dense_lattice_pos[index] = 0.02F;
      state.dense_lattice_vel[index] *= -0.35F;
    } else if (state.dense_lattice_pos[index] > 0.98F) {
      state.dense_lattice_pos[index] = 0.98F;
      state.dense_lattice_vel[index] *= -0.35F;
    }
  }

  if (!silent) {
    const float drift = 0.50F *
                        (0.88F + 0.80F * state.dense_activity_env) *
                        (0.85F + 0.30F * clamp01(channel.controls().mood)) *
                        (static_cast<float>(kPixelsPerChannel) / 128.0F);
    transportOutward(channel.frame(), channel.previousFrame(), drift,
                     present ? 0.90F : 0.82F, dt);
  }
  clearLeftHalf(channel.frame());
  if (!silent) {
    float base_hue = chromagramCentroidHue(state, audio);
    if (chord_aware) {
      base_hue = heldChordHue(state, audio, base_hue, dt);
    }
    const float contrast = modes::liveinessEffective(
        chord_aware ? 24U : 21U, 1.0F, channel.controls().liveiness);
    for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
      const float unit = static_cast<float>(distance) /
                         static_cast<float>(kPixelsPerHalf - 1U);
      float interference = 0.0F;
      for (std::size_t node = 0U; node < kDenseForgeLatticeSize; ++node) {
        interference += ::cosf(6.28318530718F *
                               (unit * kOmega[node] -
                                state.dense_lattice_pos[node] +
                                state.dense_carrier));
      }
      interference = 0.50F + 0.50F *
                                  interference /
                                  static_cast<float>(kDenseForgeLatticeSize);
      const std::size_t bin = bin_count > 0U
                                  ? distance * bin_count / kPixelsPerHalf
                                  : 0U;
      const float spectrum =
          bin_count > 0U ? clamp01(audio.spectrum[bin]) : 0.0F;
      const float level = clamp01(
          (0.48F * interference * contrast + 0.52F * spectrum) *
          state.dense_activity_env);
      if (level >= 0.008F) {
        addWeighted(channel.frame()[kCentreRight + distance],
                    explicitPaletteColour(channel.controls(),
                                          base_hue + unit * 0.30F, level),
                    level);
      }
    }
    for (std::size_t node = 0U; node < kDenseForgeLatticeSize; ++node) {
      const float level = clamp01(0.20F + 0.55F *
                                              state.dense_activity_env);
      drawRightStreak(
          channel, state.dense_lattice_last[node],
          state.dense_lattice_pos[node],
          explicitPaletteColour(channel.controls(),
                                base_hue + static_cast<float>(node) /
                                               static_cast<float>(
                                                   kDenseForgeLatticeSize) *
                                               0.35F,
                                level));
    }
  }
  snapshotHistory(channel);
  mirrorRightHalf(channel.frame());
}

void renderSnapwave(ChannelRenderState& channel,
                    const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float dt = sanitiseDeltaSeconds(delta_seconds);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  const float peak = clamp01(audio.peak_scaled);
  state.snap_peak_env = asymmetricFollower(
      state.snap_peak_env, peak, 0.05F, 0.28F, dt);
  const bool present = !silent &&
                       (audio.spectral_energy >= 0.08F ||
                        audio.novelty >= 0.08F || state.snap_peak_env >= 0.05F);
  if (present) {
    state.snap_phase = wrap01(
        state.snap_phase + dt * (0.159154943F +
                                 0.32F * clamp01(audio.novelty)));
  }
  float oscillator = 0.0F;
  float chroma_peak = 0.0F;
  std::size_t notes = 0U;
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    const float level = clamp01(audio.chroma_a_origin[bin]);
    if (level < 0.10F) {
      continue;
    }
    oscillator += level *
                  ::sinf(6.28318530718F * state.snap_phase *
                         (1.0F + static_cast<float>(bin) * 0.50F));
    chroma_peak = level > chroma_peak ? level : chroma_peak;
    ++notes;
  }
  if (notes > 0U) {
    oscillator /= static_cast<float>(notes);
  }
  oscillator = ::tanhf(2.0F * oscillator);
  float amplitude = modes::liveinessEffective(
      22U, oscillator * state.snap_peak_env * 0.97F,
      channel.controls().liveiness);
  if (audio.kick_strength > 0.0F) {
    amplitude += (amplitude < 0.0F ? -1.0F : 1.0F) *
                 0.05F * clamp01(audio.kick_strength);
  }
  if (amplitude > -0.05F && amplitude < 0.05F) {
    amplitude = 0.0F;
  }
  state.snap_amp_smooth +=
      (amplitude - state.snap_amp_smooth) * tauAlpha(0.08F, dt);
  state.snap_hue_ema +=
      (chroma_peak - state.snap_hue_ema) * dtCorrectAlpha(0.10F, dt);

  const float retention = present
                              ? 0.965F - 0.105F *
                                             clamp01(state.snap_peak_env)
                              : 0.86F;
  transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                   retention, dt);
  clearLeftHalf(channel.frame());
  if (present) {
    const float reach = clamp01(
        (state.snap_amp_smooth < 0.0F ? -state.snap_amp_smooth
                                      : state.snap_amp_smooth));
    const float brightness = clamp01(
        0.18F + 0.52F * chroma_peak + 0.26F * state.snap_peak_env +
        0.20F * clamp01(audio.kick_strength));
    const float phase = chromagramCentroidHue(state, audio) +
                        reach * 0.30F +
                        (state.snap_amp_smooth < 0.0F ? 0.50F : 0.0F);
    const Pixel8 colour =
        explicitPaletteColour(channel.controls(), phase,
                              notes > 0U ? brightness : brightness * 0.35F);
    drawRightStreak(channel, 0.0F, reach, scalePixel(colour, 0.22F));
    const int centre = static_cast<int>(rightPixelFromUnit(reach));
    for (int offset = -5; offset <= 5; ++offset) {
      const int pixel = centre + offset;
      if (pixel < static_cast<int>(kCentreRight) ||
          pixel >= static_cast<int>(kPixelsPerChannel)) {
        continue;
      }
      const float distance =
          static_cast<float>(offset < 0 ? -offset : offset);
      const float weight = ::expf(-0.085F * distance * distance);
      if (weight >= 0.012F) {
        addWeighted(channel.frame()[static_cast<std::size_t>(pixel)],
                    colour, weight);
      }
    }
  }
  snapshotHistory(channel);
  mirrorRightHalf(channel.frame());
}

std::size_t claimPrismRing(ChannelEffectState& state) noexcept {
  std::size_t slot = 0U;
  for (std::size_t index = 1U; index < kPrismRingPoolSize; ++index) {
    if (state.prism_life[index] < state.prism_life[slot]) {
      slot = index;
    }
  }
  return slot;
}

void renderPulsePrism(ChannelRenderState& channel,
                      const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float dt = sanitiseDeltaSeconds(delta_seconds);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  const float bed_target =
      clamp01(0.55F * audio.vu_level + 0.45F * audio.low_energy);
  state.prism_bed_env = asymmetricFollower(
      state.prism_bed_env, bed_target, 0.035F, 0.32F, dt);
  const float beat_mod = clamp01(audio.kick_strength);
  float retention = 1.0F - 0.075F * nominalFrames(dt) *
                               (0.75F + 0.25F * beat_mod);
  if (retention < 0.80F) {
    retention = 0.80F;
  } else if (retention > 0.965F) {
    retention = 0.965F;
  }
  transportOutward(channel.frame(), channel.previousFrame(), 0.0F,
                   retention, dt);
  clearLeftHalf(channel.frame());

  const bool kick_fresh =
      !silent && audio.kick_event_id != 0U &&
      audio.kick_event_id != state.prism_last_kick_id;
  const bool transient_fresh =
      !silent && !kick_fresh && audio.transient_strength > 0.72F &&
      audio.transient_event_id != 0U &&
      audio.transient_event_id != state.prism_last_transient_id;
  if (kick_fresh || transient_fresh) {
    const float strength = clamp01(kick_fresh ? audio.kick_strength
                                              : audio.transient_strength);
    const std::size_t slot = claimPrismRing(state);
    state.prism_radius[slot] = 0.0F;
    state.prism_velocity[slot] = 36.0F + 42.0F * strength;
    state.prism_life[slot] = 0.28F + 0.72F * strength;
    const std::uint32_t event_id =
        kick_fresh ? audio.kick_event_id : audio.transient_event_id;
    state.prism_hue[slot] = wrap01(
        chromagramCentroidHue(state, audio) +
        static_cast<float>(event_id % 12U) * 0.055F);
  }
  state.prism_last_kick_id = audio.kick_event_id;
  state.prism_last_transient_id = audio.transient_event_id;

  if (!silent) {
    const float bed_reach = 5.0F + 13.0F * state.prism_bed_env;
    const Pixel8 bed_colour = explicitPaletteColour(
        channel.controls(), chromagramCentroidHue(state, audio),
        0.36F * state.prism_bed_env);
    for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
      if (static_cast<float>(distance) > bed_reach) {
        break;
      }
      const float unit = static_cast<float>(distance) / bed_reach;
      addWeighted(channel.frame()[kCentreRight + distance], bed_colour,
                  (1.0F - unit) * (1.0F - unit));
    }
  }
  for (std::size_t ring = 0U; ring < kPrismRingPoolSize; ++ring) {
    if (state.prism_life[ring] <= 0.0F) {
      continue;
    }
    state.prism_radius[ring] +=
        modes::liveinessEffective(23U, state.prism_velocity[ring],
                                  channel.controls().liveiness) *
        dt;
    state.prism_life[ring] -= 0.58F * dt;
    if (state.prism_life[ring] <= 0.0F ||
        state.prism_radius[ring] >= static_cast<float>(kPixelsPerHalf)) {
      state.prism_life[ring] = 0.0F;
      continue;
    }
    const Pixel8 ring_colour = explicitPaletteColour(
        channel.controls(), state.prism_hue[ring],
        clamp01(0.28F + 0.72F * state.prism_life[ring]));
    for (std::size_t distance = 0U; distance < kPixelsPerHalf; ++distance) {
      float delta = static_cast<float>(distance) -
                    state.prism_radius[ring];
      delta = delta < 0.0F ? -delta : delta;
      if (delta > 3.60F) {
        continue;
      }
      const float weight =
          0.70F * (1.0F - delta / 3.60F) * state.prism_life[ring];
      addWeighted(channel.frame()[kCentreRight + distance], ring_colour,
                  weight);
    }
  }
  snapshotHistory(channel);
  mirrorRightHalf(channel.frame());
}

void renderChromaConstellation(ChannelRenderState& channel,
                               const float delta_seconds) noexcept {
  ChannelEffectState& state = channel.effectState();
  const contract::AudioFeaturesV1& audio = channel.focusedAudio();
  const float dt = sanitiseDeltaSeconds(delta_seconds);
  const bool silent = (audio.event_flags & contract::kEventSilence) != 0U;
  const bool present =
      audio.spectral_energy >= 0.08F || audio.novelty >= 0.08F;
  if (!silent) {
    const float drift = modes::liveinessEffective(
        25U,
        0.35F * (0.40F + 0.60F * clamp01(audio.spectral_energy)) *
            (static_cast<float>(kPixelsPerChannel) / 128.0F),
        channel.controls().liveiness);
    transportOutward(channel.frame(), channel.previousFrame(), drift,
                     present ? 0.90F : 0.82F, dt);
  }
  clearLeftHalf(channel.frame());
  for (std::size_t pitch = 0U; pitch < contract::kChromaBinCount; ++pitch) {
    const float target = silent ? 0.0F : clamp01(audio.chroma_a_origin[pitch]);
    state.constellation_chroma_smooth[pitch] +=
        (target - state.constellation_chroma_smooth[pitch]) *
        dtCorrectAlpha(0.15F, dt);
    const float level = state.constellation_chroma_smooth[pitch] * 0.90F;
    if (level < 0.06F) {
      continue;
    }
    const std::size_t rank = (pitch * 7U) % 12U;
    const float position =
        (static_cast<float>(rank) + 0.50F) / 12.0F;
    const std::size_t pixel = rightPixelFromUnit(position);
    const Pixel8 colour = explicitPaletteColour(
        channel.controls(), static_cast<float>(pitch) / 12.0F, level);
    addWeighted(channel.frame()[pixel], colour, level);
    if (pixel > kCentreRight) {
      addWeighted(channel.frame()[pixel - 1U], colour, level * 0.35F);
    }
    if (pixel + 1U < kPixelsPerChannel) {
      addWeighted(channel.frame()[pixel + 1U], colour, level * 0.35F);
    }
  }
  snapshotHistory(channel);
  mirrorRightHalf(channel.frame());
}

void fadeBloomEdges(const PixelSpan frame) noexcept {
  constexpr std::size_t kFadeWidth = kPixelsPerChannel / 4U;
  for (std::size_t index = 0U; index < kFadeWidth; ++index) {
    const float progress = static_cast<float>(index) /
                           static_cast<float>(kFadeWidth - 1U);
    const float amount = progress * progress;
    frame[index] = scalePixel(frame[index], amount);
    frame[kPixelsPerChannel - 1U - index] =
        scalePixel(frame[kPixelsPerChannel - 1U - index], amount);
  }
}

void fadeAuroraEdges(const PixelSpan frame) noexcept {
  constexpr std::size_t kFadeWidth = 6U;
  for (std::size_t index = 0U; index < kFadeWidth; ++index) {
    const float amount =
        static_cast<float>(index + 1U) / static_cast<float>(kFadeWidth + 1U);
    frame[index] = scalePixel(frame[index], amount);
    frame[kPixelsPerChannel - 1U - index] =
        scalePixel(frame[kPixelsPerChannel - 1U - index], amount);
  }
}

void renderBloomFamily(ChannelRenderState& channel, const std::uint16_t mode,
                       const float delta_seconds) noexcept {
  const ChannelVisualControls& controls = channel.controls();
  const float resolution_scale =
      static_cast<float>(kPixelsPerChannel) / 128.0F;
  float propagation = 0.0F;
  float alpha = std::clamp(controls.vp_bloom_alpha, 0.80F, 1.00F);
  if (controls.vp_fix_bloom_decay) {
    alpha = 0.88F;
  }
  if (mode == 12U) {
    propagation = (0.80F + 1.20F * clamp01(controls.mood)) * resolution_scale;
    alpha = kAuroraAlpha;
  } else {
    const float multiplier = mode == 9U ? 2.0F : 1.0F;
    propagation = (0.25F + 1.75F * clamp01(controls.mood)) *
                  resolution_scale * multiplier;
  }
  propagation *= std::clamp(controls.vp_bloom_shift_scale, 0.25F, 2.00F);
  propagation = modes::liveinessEffective(mode, propagation,
                                          controls.liveiness);
  transportOutward(channel.frame(), channel.previousFrame(), propagation,
                   alpha, delta_seconds);
  Pixel8 injection = injectionColour(channel);
  if (controls.vp_bloom_force_saturation &&
      !controls.vp_fix_bloom_decay) {
    injection = forcePixelSaturation(injection);
  }
  channel.frame()[kCentreLeft] = injection;
  channel.frame()[kCentreRight] = injection;
  snapshotHistory(channel);
  if (mode == 12U) {
    fadeAuroraEdges(channel.frame());
    if (controls.mirror_enabled) {
      mirrorRightHalf(channel.frame());
    }
  } else {
    fadeBloomEdges(channel.frame());
    if (controls.mirror_enabled) {
      mirrorRightHalf(channel.frame());
    }
  }
}

}  // namespace

ProductRenderResult renderProductChannel(
    ChannelRenderState& channel, const VisualAudioFrameView& visual,
    const float delta_seconds) noexcept {
  const std::uint16_t mode = sanitiseProductMode(channel.controls().mode_id);
  switch (mode) {
    case 3U:
    case 9U:
    case 12U:
      renderBloomFamily(channel, mode,
                        std::isfinite(delta_seconds) && delta_seconds >= 0.0F
                            ? delta_seconds
                            : 1.0F / kNominalFramesPerSecond);
      return {true, mode};
    case 14U:
    case 15U:
    case 28U:
      renderSpectrumRiver(channel, mode, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 16U:
      renderEmber(channel, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 7U:
    case 8U:
    case 11U:
      renderWaveformFamily(channel, visual, mode,
                           sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 32U:
      renderWaveformK1(channel, visual,
                       sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 13U:
      renderComet(channel, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 26U:
      renderPercussionBurst(channel, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 19U:
      renderTempoRiver(channel, visual,
                       sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 20U:
      renderTempoComet(channel, visual,
                       sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 18U:
      renderWaveformTempo(channel, visual,
                          sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 27U:
      renderTempoCometAnticipate(channel, visual,
                                 sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 29U:
      renderTempoRiverWalk(channel, visual,
                           sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 21U:
      renderDenseForge(channel, false,
                       sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 22U:
      renderSnapwave(channel, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 23U:
      renderPulsePrism(channel, sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 24U:
      renderDenseForge(channel, true,
                       sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    case 25U:
      renderChromaConstellation(channel,
                                sanitiseDeltaSeconds(delta_seconds));
      return {true, mode};
    default:
      return {false, mode};
  }
}

}  // namespace k1::core::visual
