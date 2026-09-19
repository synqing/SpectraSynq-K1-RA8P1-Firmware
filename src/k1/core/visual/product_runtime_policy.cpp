/* Ported from DualMCU pin 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a core/visual/product_runtime_policy.cpp — Titan D3 output composition. */
#include "core/visual/product_runtime_policy.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace k1::core::visual {
using k1::core::Pixel8;
using k1::core::PixelSpan;
namespace {

enum class MusicState : std::uint8_t {
  kSilence = 0U,
  kAmbient,
  kBuild,
  kDrop,
  kBreakdown,
  kDense,
  kSteady,
};

float clamp01(const float value) noexcept {
  if (!std::isfinite(value) || value <= 0.0F) {
    return 0.0F;
  }
  return value >= 1.0F ? 1.0F : value;
}

float finitePositive(const float value) noexcept {
  return std::isfinite(value) && value > 0.0F ? value : 0.0F;
}

std::uint16_t scaleQ0_16(const std::uint16_t value,
                         const float scalar) noexcept {
  const float scaled = static_cast<float>(value) * scalar;
  return static_cast<std::uint16_t>(
      std::clamp(scaled, 0.0F, 65535.0F));
}

void scaleAudioFields(contract::AudioFeaturesV1& audio,
                      const float gain) noexcept {
  audio.peak_scaled = clamp01(audio.peak_scaled * gain);
  audio.vu_level = clamp01(audio.vu_level * gain);
  audio.novelty = clamp01(audio.novelty * gain);
  audio.spectral_energy = finitePositive(audio.spectral_energy) * gain;
  audio.low_energy = finitePositive(audio.low_energy) * gain;
  audio.mid_energy = finitePositive(audio.mid_energy) * gain;
  audio.high_energy = finitePositive(audio.high_energy) * gain;
  audio.chroma_strength = finitePositive(audio.chroma_strength) * gain;
  audio.onset_strength = clamp01(audio.onset_strength * gain);
  audio.bass_onset_strength = clamp01(audio.bass_onset_strength * gain);
  audio.transient_strength = clamp01(audio.transient_strength * gain);
  audio.kick_strength = clamp01(audio.kick_strength * gain);
  audio.snare_strength = clamp01(audio.snare_strength * gain);
  audio.hihat_strength = clamp01(audio.hihat_strength * gain);
  audio.transient_level = clamp01(audio.transient_level * gain);
  audio.kick_level = clamp01(audio.kick_level * gain);
  audio.snare_level = clamp01(audio.snare_level * gain);
  audio.hihat_level = clamp01(audio.hihat_level * gain);
}

float applySoftKnee(const float value) noexcept {
  if (value <= 0.50F) {
    return finitePositive(value);
  }
  const float excess = value - 0.50F;
  return clamp01(0.50F + (excess / (1.0F + excess)));
}

float dtAlpha(const std::uint32_t dt_ms, const float tau_ms) noexcept {
  if (dt_ms == 0U) {
    return 0.0F;
  }
  return clamp01(1.0F - std::exp(-static_cast<float>(dt_ms) / tau_ms));
}

void decayPulse(float& pulse, const std::uint32_t dt_ms,
                const std::uint32_t tau_ms) noexcept {
  const float decay = clamp01(
      static_cast<float>(dt_ms) / static_cast<float>(tau_ms));
  pulse *= 1.0F - decay;
  if (pulse < 0.0001F) {
    pulse = 0.0F;
  }
}

MusicState classifyAudio(const contract::AudioFeaturesV1& audio,
                         const float energy_delta) noexcept {
  if ((audio.event_flags & contract::kEventSilence) != 0U) {
    return MusicState::kSilence;
  }
  if (audio.novelty > 0.45F && audio.peak_scaled > 0.55F) {
    return MusicState::kDrop;
  }
  if (energy_delta > 0.035F && audio.novelty > 0.18F) {
    return MusicState::kBuild;
  }
  if (energy_delta < -0.04F && audio.spectral_energy < 0.22F) {
    return MusicState::kBreakdown;
  }
  if (audio.spectral_energy < 0.08F && audio.novelty < 0.08F) {
    return MusicState::kAmbient;
  }
  if (audio.spectral_energy > 0.42F && audio.low_energy > 0.18F &&
      audio.mid_energy > 0.18F && audio.high_energy > 0.12F) {
    return MusicState::kDense;
  }
  return MusicState::kSteady;
}

void directorScalars(const MusicState music,
                     float& photons, float& chroma,
                     float& mood, float& saturation) noexcept {
  photons = 1.0F;
  chroma = 1.0F;
  mood = 1.0F;
  saturation = 1.0F;
  switch (music) {
    case MusicState::kSilence:
      photons = 0.72F;
      chroma = 0.85F;
      saturation = 0.82F;
      break;
    case MusicState::kAmbient:
      photons = 0.86F;
      chroma = 0.92F;
      saturation = 0.90F;
      break;
    case MusicState::kBuild:
      mood = 1.16F;
      photons = 1.14F;
      chroma = 1.12F;
      saturation = 1.04F;
      break;
    case MusicState::kDrop:
      mood = 1.28F;
      photons = 1.24F;
      chroma = 1.20F;
      saturation = 1.08F;
      break;
    case MusicState::kBreakdown:
      mood = 0.78F;
      photons = 0.82F;
      chroma = 0.92F;
      saturation = 0.88F;
      break;
    case MusicState::kDense:
      mood = 1.06F;
      photons = 1.08F;
      chroma = 0.95F;
      saturation = 0.82F;
      break;
    case MusicState::kSteady:
      break;
  }
}

std::uint16_t autonomyMode(const MusicState music) noexcept {
  switch (music) {
    case MusicState::kSilence:
    case MusicState::kAmbient:
    case MusicState::kBreakdown:
      return 3U;
    case MusicState::kBuild:
      return 11U;
    case MusicState::kDrop:
      return 13U;
    case MusicState::kDense:
      return 21U;
    case MusicState::kSteady:
      return 11U;
  }
  return 3U;
}

std::uint16_t autonomyPalette(const MusicState music) noexcept {
  switch (music) {
    case MusicState::kSilence:
      return 2U;
    case MusicState::kAmbient:
      return 11U;
    case MusicState::kBuild:
      return 29U;
    case MusicState::kDrop:
      return 24U;
    case MusicState::kBreakdown:
      return 22U;
    case MusicState::kDense:
      return 31U;
    case MusicState::kSteady:
      return 22U;
  }
  return 2U;
}

float harmonyAngle(const std::uint8_t mode) noexcept {
  constexpr float kPi = 3.14159265358979323846F;
  switch (mode) {
    case 1U:
      return 33.0F * (kPi / 180.0F);
    case 2U:
      return kPi;
    case 3U:
      return 150.0F * (kPi / 180.0F);
    case 5U:
      return 120.0F * (kPi / 180.0F);
    case 6U:
      return 90.0F * (kPi / 180.0F);
    default:
      return 0.0F;
  }
}

void transformEdge(const PixelSpan frame, const float angle,
                   const float strength, const bool veil) noexcept {
  if (frame.size() <= 1U || strength <= 0.0F) {
    return;
  }
  const float cosine = std::cos(angle);
  const float sine = std::sin(angle);
  const float third = (1.0F - cosine) / 3.0F;
  const float sine_term = 0.57735026919F * sine;
  const float matrix[9] = {
      cosine + third, third - sine_term, third + sine_term,
      third + sine_term, cosine + third, third - sine_term,
      third - sine_term, third + sine_term, cosine + third};
  const float centre = static_cast<float>(frame.size() - 1U) * 0.5F;
  for (std::size_t index = 0U; index < frame.size(); ++index) {
    Pixel8& pixel = frame[index];
    const float red = static_cast<float>(pixel.red);
    const float green = static_cast<float>(pixel.green);
    const float blue = static_cast<float>(pixel.blue);
    const float mask = std::fabs(static_cast<float>(index) - centre) / centre;
    const float mix = clamp01(strength * mask);
    float transformed_red = red;
    float transformed_green = green;
    float transformed_blue = blue;
    if (veil) {
      const float luma = red * 0.299F + green * 0.587F + blue * 0.114F;
      transformed_red = red * 0.50F + luma * 0.50F;
      transformed_green = green * 0.50F + luma * 0.50F;
      transformed_blue = blue * 0.50F + luma * 0.50F;
    } else {
      transformed_red = matrix[0] * red + matrix[1] * green + matrix[2] * blue;
      transformed_green = matrix[3] * red + matrix[4] * green + matrix[5] * blue;
      transformed_blue = matrix[6] * red + matrix[7] * green + matrix[8] * blue;
    }
    pixel.red = static_cast<std::uint8_t>(std::clamp(
        red + (transformed_red - red) * mix, 0.0F, 255.0F));
    pixel.green = static_cast<std::uint8_t>(std::clamp(
        green + (transformed_green - green) * mix, 0.0F, 255.0F));
    pixel.blue = static_cast<std::uint8_t>(std::clamp(
        blue + (transformed_blue - blue) * mix, 0.0F, 255.0F));
  }
}

std::uint64_t currentUnits(const PixelSpan frame) noexcept {
  std::uint64_t units = 0U;
  for (std::size_t index = 0U; index < frame.size(); ++index) {
    units += frame[index].red;
    units += frame[index].green;
    units += frame[index].blue;
  }
  return units;
}

void scaleFrame(const PixelSpan frame, const std::uint32_t q16) noexcept {
  for (std::size_t index = 0U; index < frame.size(); ++index) {
    Pixel8& pixel = frame[index];
    pixel.red = static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(pixel.red) * q16 + 32767U) / 65535U);
    pixel.green = static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(pixel.green) * q16 + 32767U) / 65535U);
    pixel.blue = static_cast<std::uint8_t>(
        (static_cast<std::uint32_t>(pixel.blue) * q16 + 32767U) / 65535U);
  }
}

}  // namespace

void applyProductAudioPolicy(
    const contract::AudioFeaturesV1& source,
    const ProductRuntimePolicy& policy,
    contract::AudioFeaturesV1& output) noexcept {
  output = source;
  const float sensitivity = std::clamp(policy.sensitivity, 0.10F, 20.0F);
  scaleAudioFields(output, sensitivity);
  const std::size_t range = std::clamp<std::size_t>(
      policy.chromagram_range, 1U, contract::kSpectrumBinCount);
  float chroma[contract::kChromaBinCount]{};
  float chroma_peak = 0.0F;
  for (std::size_t bin = 0U; bin < contract::kSpectrumBinCount; ++bin) {
    if (bin >= range) {
      output.spectrum[bin] = 0.0F;
      continue;
    }
    const float scaled = finitePositive(output.spectrum[bin]) * sensitivity;
    output.spectrum[bin] = policy.vp_fix_agc_soft_knee
        ? applySoftKnee(scaled)
        : std::min(1.0F, scaled);
    chroma[bin % contract::kChromaBinCount] += output.spectrum[bin];
  }
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    chroma_peak = std::max(chroma_peak, chroma[bin]);
  }
  for (std::size_t bin = 0U; bin < contract::kChromaBinCount; ++bin) {
    float gate = 1.0F;
    if (policy.vp_fix_chroma_gate) {
      gate = clamp01((chroma_peak - 0.08F) / 0.20F);
    }
    output.chroma_a_origin[bin] = chroma_peak > 0.0F
        ? (chroma[bin] / chroma_peak) * gate
        : 0.0F;
  }
  output.chroma_strength = chroma_peak;
}

void applyProductScenePolicy(
    const ProductRuntimePolicy& policy,
    ProductRuntimePolicyState& state,
    const contract::AudioFeaturesV1& audio,
    const std::uint32_t now_ms,
    ChannelVisualControls& primary,
    ChannelVisualControls& secondary) noexcept {
  const std::uint32_t dt_ms = state.last_update_ms == 0U ||
                                      now_ms < state.last_update_ms
                                  ? 0U
                                  : now_ms - state.last_update_ms;
  state.last_update_ms = now_ms;
  state.previous_energy_smooth = state.energy_smooth;
  const float alpha = dtAlpha(dt_ms, 180.0F);
  state.energy_smooth +=
      (finitePositive(audio.spectral_energy) - state.energy_smooth) * alpha;
  state.novelty_smooth +=
      (clamp01(audio.novelty) - state.novelty_smooth) * alpha;
  const MusicState music = classifyAudio(
      audio, state.energy_smooth - state.previous_energy_smooth);

  if (policy.director_enabled) {
    float photons = 1.0F;
    float chroma = 1.0F;
    float mood = 1.0F;
    float saturation = 1.0F;
    directorScalars(music, photons, chroma, mood, saturation);
    primary.photons_id = scaleQ0_16(primary.photons_id, photons);
    secondary.photons_id = scaleQ0_16(secondary.photons_id, photons);
    primary.chroma = std::clamp(primary.chroma * chroma, 0.0F, 2.0F);
    secondary.chroma = std::clamp(secondary.chroma * chroma, 0.0F, 2.0F);
    primary.mood = std::clamp(primary.mood * mood, 0.0F, 2.0F);
    secondary.mood = std::clamp(secondary.mood * mood, 0.0F, 2.0F);
    primary.saturation = clamp01(primary.saturation * saturation);
    secondary.saturation = clamp01(secondary.saturation * saturation);

    const float confidence = clamp01(
        state.novelty_smooth * 0.65F + state.energy_smooth * 0.35F);
    const bool dwell_complete = state.last_switch_ms == 0U ||
        (now_ms - state.last_switch_ms) >= policy.director_min_dwell_ms;
    if (policy.director_assist && policy.director_autonomy &&
        confidence >= clamp01(policy.director_confidence_floor) &&
        dwell_complete) {
      primary.mode_id = autonomyMode(music);
      primary.effect_id = primary.mode_id;
      primary.palette_id = autonomyPalette(music);
      primary.palette_mode_enabled = true;
      primary.auto_colour_shift = music == MusicState::kBuild ||
                                  music == MusicState::kDrop ||
                                  music == MusicState::kDense;
      state.last_switch_ms = now_ms;
    }
  }

  if (policy.standby_dimming &&
      (audio.event_flags & contract::kEventSilence) != 0U) {
    primary.brightness = static_cast<std::uint8_t>(primary.brightness / 8U);
    secondary.brightness = static_cast<std::uint8_t>(secondary.brightness / 8U);
  }

  decayPulse(state.onset_pulse, dt_ms, 100U);
  decayPulse(state.bass_pulse, dt_ms, 180U);
  decayPulse(state.beat_pulse, dt_ms, 250U);
  if (!policy.hooks_enabled || audio.onset_event_age_ms > 80U) {
    return;
  }
  if ((audio.event_flags & contract::kEventOnset) != 0U &&
      audio.onset_event_id != 0U &&
      audio.onset_event_id != state.last_onset_event_id) {
    state.onset_pulse = std::max(state.onset_pulse, clamp01(audio.onset_strength));
    state.last_onset_event_id = audio.onset_event_id;
  }
  if ((audio.event_flags & contract::kEventBassOnset) != 0U &&
      audio.onset_event_id != 0U &&
      audio.onset_event_id != state.last_bass_event_id) {
    state.bass_pulse = std::max(
        state.bass_pulse, clamp01(audio.bass_onset_strength));
    state.last_bass_event_id = audio.onset_event_id;
  }
  if ((audio.event_flags & contract::kEventBeat) != 0U &&
      audio.onset_event_id != 0U &&
      audio.onset_event_id != state.last_beat_event_id) {
    state.beat_pulse = std::max(state.beat_pulse, clamp01(audio.beat_confidence));
    state.last_beat_event_id = audio.onset_event_id;
  }
  const float photon_scalar = 1.0F + state.onset_pulse * 0.16F;
  const float chroma_scalar = 1.0F + state.beat_pulse * 0.12F;
  primary.photons_id = scaleQ0_16(primary.photons_id, photon_scalar);
  secondary.photons_id = scaleQ0_16(secondary.photons_id, photon_scalar);
  primary.chroma = std::clamp(primary.chroma * chroma_scalar, 0.0F, 2.0F);
  secondary.chroma = std::clamp(secondary.chroma * chroma_scalar, 0.0F, 2.0F);
}

void applyProductEdgePolicy(
    const ProductRuntimePolicy& policy,
    const ProductRuntimePolicyState& state,
    const PixelSpan primary,
    const PixelSpan secondary) noexcept {
  if (!policy.edge_enabled || policy.edge_mode == 0U) {
    return;
  }
  const float hook_scalar = 1.0F + state.bass_pulse * 0.20F;
  const float strength = clamp01(policy.edge_strength * hook_scalar);
  const bool veil = policy.edge_mode == 4U;
  const float angle = harmonyAngle(policy.edge_mode);
  transformEdge(primary, -angle * 0.5F, strength, veil);
  transformEdge(secondary, angle * 0.5F, strength, veil);
}

void advanceAutomaticColourShift(
    ChannelVisualControls& controls,
    const float delta_seconds) noexcept {
  if (!controls.auto_colour_shift || !std::isfinite(delta_seconds) ||
      delta_seconds <= 0.0F) {
    return;
  }
  // Legacy source: 0.0003 turns per render at percentile rank, whose mean is
  // 0.5 at the documented approximately 200 FPS render cadence. Expressing
  // the resulting 0.03 turns/s here removes frame-rate dependence while
  // retaining the established direction and typical sweep period.
  constexpr float kLegacyMeanTurnsPerSecond = 0.03F;
  controls.hue_position -= kLegacyMeanTurnsPerSecond * delta_seconds;
  controls.hue_position -= std::floor(controls.hue_position);
}

void applyProductOutputGain(
    const PixelSpan frame,
    const bool enabled,
    const std::uint16_t master_brightness_q0_16,
    const std::uint8_t policy_brightness) noexcept {
  if (!enabled || master_brightness_q0_16 == 0U || policy_brightness == 0U) {
    for (std::size_t index = 0U; index < frame.size(); ++index) {
      frame[index] = {};
    }
    return;
  }
  const std::uint32_t combined_q0_16 = static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(master_brightness_q0_16) *
           policy_brightness +
       127U) /
      255U);
  scaleFrame(frame, combined_q0_16);
}

void applyProductCurrentLimit(
    const PixelSpan primary,
    const PixelSpan secondary,
    const std::uint16_t maximum_current_ma) noexcept {
  const std::uint64_t units = currentUnits(primary) + currentUnits(secondary);
  const std::uint64_t maximum_units =
      static_cast<std::uint64_t>(std::max<std::uint16_t>(100U, maximum_current_ma)) *
      255U / 20U;
  if (units == 0U || units <= maximum_units) {
    return;
  }
  const std::uint32_t scale_q16 = static_cast<std::uint32_t>(
      std::min<std::uint64_t>(65535U, (maximum_units << 16U) / units));
  scaleFrame(primary, scale_q16);
  scaleFrame(secondary, scale_q16);
}

}  // namespace k1::core::visual
