#pragma once

#include <cstdint>

#include "contract/audio_features_v1.h"
#include "core/pixel.h"
#include "core/visual/audio_focus.h"
#include "core/visual/channel_effect_state.h"
#include "core/visual/pixel_topology.h"
#include "core/visual/product_output_treatment.h"

namespace k1::core::visual {

struct ChannelVisualControls final {
  std::uint16_t effect_id = 0U;
  std::uint16_t mode_id = 0U;
  std::uint16_t palette_id = 0U;
  std::uint16_t photons_id = 0U;
  std::uint8_t brightness = 255U;
  bool enabled = true;
  float chroma = 0.0F;
  float mood = 0.0F;
  float saturation = 1.0F;
  float square_iterations = 0.0F;
  float sensitivity = 1.0F;
  float incandescent_filter = 0.0F;
  float bulb_opacity = 0.0F;
  float base_coat_intensity = 0.0F;
  float prism_count = 0.0F;
  float hue_position = 0.0F;
  float chroma_value = 0.0F;
  float hue_shifting_mix = 0.0F;
  std::uint32_t sweet_spot_min_level = 350U;
  std::uint16_t samples_per_chunk = 180U;
  bool mirror_enabled = true;
  bool auto_colour_shift = false;
  bool reverse_order = false;
  bool incandescent_mode = false;
  bool temporal_dithering = false;
  bool base_coat = false;
  bool palette_mode_enabled = false;
  bool chromatic_mode = true;
  bool vp_fix_agc_soft_knee = false;
  bool vp_fix_chroma_gate = false;
  bool vp_fix_prism_off = false;
  bool vp_fix_bloom_decay = false;
  bool vp_fix_hsv_source_sat = false;
  bool vp_fix_secondary_clean = false;
  float vp_bloom_alpha = 0.99F;
  float vp_bloom_shift_scale = 1.0F;
  bool vp_bloom_force_saturation = true;
  float vp_waveform_shift_rate = 120.0F;
  float vp_waveform_idle_fade = 0.985F;
  float vp_waveform_raw_margin = 1.10F;
  float vp_waveform_peak_floor = 0.08F;
  float vp_waveform_active_fade = 0.04F;
  float vp_waveform_chroma_blend_gain = 2.0F;
  float vp_waveform_fallback_brightness = 1.0F;
  float vp_waveform_vu_floor = 0.02F;
};

class ChannelRenderState final {
 public:
  explicit ChannelRenderState(PixelChannelId channel) noexcept;

  [[nodiscard]] PixelChannelId channel() const noexcept { return channel_; }
  [[nodiscard]] PixelSpan frame() noexcept {
    return PixelSpan{frame_, kPixelsPerChannel};
  }
  [[nodiscard]] PixelSpan previousFrame() noexcept {
    return PixelSpan{previous_, kPixelsPerChannel};
  }
  [[nodiscard]] PixelSpan scratch() noexcept {
    return PixelSpan{scratch_, kPixelsPerChannel};
  }
  [[nodiscard]] ConstPixelSpan frame() const noexcept {
    return ConstPixelSpan{frame_, kPixelsPerChannel};
  }
  [[nodiscard]] ChannelVisualControls& controls() noexcept { return controls_; }
  [[nodiscard]] const ChannelVisualControls& controls() const noexcept {
    return controls_;
  }
  [[nodiscard]] AudioFocusProfile& audioFocus() noexcept { return audio_focus_; }
  [[nodiscard]] const AudioFocusProfile& audioFocus() const noexcept {
    return audio_focus_;
  }
  [[nodiscard]] ChannelEffectState& effectState() noexcept {
    return effect_state_;
  }
  [[nodiscard]] const ChannelEffectState& effectState() const noexcept {
    return effect_state_;
  }
  [[nodiscard]] ProductOutputTreatmentState& outputTreatmentState() noexcept {
    return output_treatment_state_;
  }
  [[nodiscard]] const contract::AudioFeaturesV1& focusedAudio() const noexcept {
    return focused_audio_;
  }

  void prepareAudio(const contract::AudioFeaturesV1& shared) noexcept;
  void clearFrame() noexcept;

 private:
  PixelChannelId channel_;
  Pixel8 frame_[kPixelsPerChannel]{};
  Pixel8 previous_[kPixelsPerChannel]{};
  Pixel8 scratch_[kPixelsPerChannel]{};
  ChannelVisualControls controls_{};
  AudioFocusProfile audio_focus_{};
  ChannelEffectState effect_state_{};
  ProductOutputTreatmentState output_treatment_state_{};
  contract::AudioFeaturesV1 focused_audio_{};
};

}  // namespace k1::core::visual
