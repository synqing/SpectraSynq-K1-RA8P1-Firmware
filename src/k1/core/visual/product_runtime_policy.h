/* Ported from DualMCU pin 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a core/visual/product_runtime_policy.h — Titan D3 output composition. */
#pragma once

#include <cstdint>

#include "contract/audio_features_v1.h"
#include "core/pixel.h"
#include "core/visual/channel_render_state.h"

namespace k1::core::visual {
using k1::core::PixelSpan;

struct ProductRuntimePolicy final {
  float sensitivity = 1.0F;
  bool standby_dimming = false;
  std::uint8_t chromagram_range = 60U;
  std::uint16_t max_current_ma = 5000U;
  bool director_enabled = false;
  bool director_assist = false;
  bool director_autonomy = false;
  float director_confidence_floor = 0.08F;
  bool hooks_enabled = false;
  bool edge_enabled = false;
  std::uint8_t edge_mode = 0U;
  float edge_strength = 0.0F;
  std::uint32_t director_min_dwell_ms = 8000U;
  bool vp_fix_agc_soft_knee = false;
  bool vp_fix_chroma_gate = false;
};

struct ProductRuntimePolicyState final {
  float energy_smooth = 0.0F;
  float previous_energy_smooth = 0.0F;
  float novelty_smooth = 0.0F;
  float onset_pulse = 0.0F;
  float bass_pulse = 0.0F;
  float beat_pulse = 0.0F;
  std::uint32_t last_update_ms = 0U;
  std::uint32_t last_switch_ms = 0U;
  std::uint32_t last_onset_event_id = 0U;
  std::uint32_t last_bass_event_id = 0U;
  std::uint32_t last_beat_event_id = 0U;
};

void applyProductAudioPolicy(
    const contract::AudioFeaturesV1& source,
    const ProductRuntimePolicy& policy,
    contract::AudioFeaturesV1& output) noexcept;

void applyProductScenePolicy(
    const ProductRuntimePolicy& policy,
    ProductRuntimePolicyState& state,
    const contract::AudioFeaturesV1& audio,
    std::uint32_t now_ms,
    ChannelVisualControls& primary,
    ChannelVisualControls& secondary) noexcept;

void applyProductEdgePolicy(
    const ProductRuntimePolicy& policy,
    const ProductRuntimePolicyState& state,
    PixelSpan primary,
    PixelSpan secondary) noexcept;

void advanceAutomaticColourShift(
    ChannelVisualControls& controls,
    float delta_seconds) noexcept;

void applyProductOutputGain(
    PixelSpan frame,
    bool enabled,
    std::uint16_t master_brightness_q0_16,
    std::uint8_t policy_brightness) noexcept;

void applyProductCurrentLimit(
    PixelSpan primary,
    PixelSpan secondary,
    std::uint16_t maximum_current_ma) noexcept;

}  // namespace k1::core::visual
