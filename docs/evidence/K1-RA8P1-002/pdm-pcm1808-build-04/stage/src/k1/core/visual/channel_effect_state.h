#pragma once

#include <array>
#include <cstdint>

namespace k1::core::visual {

inline constexpr std::size_t kCometPoolSize = 6U;
inline constexpr std::size_t kPercussionPoolSize = 8U;
inline constexpr std::size_t kDenseForgeLatticeSize = 8U;
inline constexpr std::size_t kPrismRingPoolSize = 6U;

struct LinearRgb final {
  float red = 0.0F;
  float green = 0.0F;
  float blue = 0.0F;
};

struct WaveformEffectState final {
  LinearRgb last_colour{};
  float peak_scaled_last = 0.0F;
  float shift_accumulator = 0.0F;
  std::uint32_t last_frame_ms = 0U;
};

// Explicit, fixed-size per-channel state for every production-enabled legacy
// mode. Source authority: visual/channel_effect_state.h plus the previously
// global waveform state at legacy commit 71520ebd312558ae99c48a521e144a268f777192.
struct ChannelEffectState final {
  WaveformEffectState waveform_fast{};
  WaveformEffectState waveform{};
  WaveformEffectState waveform_hybrid{};

  std::array<float, kCometPoolSize> comet_pos{};
  std::array<float, kCometPoolSize> comet_vel{};
  std::array<float, kCometPoolSize> comet_hue{};
  std::array<float, kCometPoolSize> comet_size{};
  std::array<float, kCometPoolSize> comet_life{};
  std::uint32_t comet_last_ms = 0U;
  std::uint32_t comet_last_event_id = 0U;
  float comet_strength_max = 0.0F;

  float river_tide_env = 0.0F;
  float ember_shimmer_phase = 0.0F;
  float tempo_scroll_accum = 0.0F;
  std::uint32_t tempo_last_ms = 0U;
  std::uint32_t tempo_river_last_ms = 0U;

  std::array<float, kCometPoolSize> tempo_comet_pos{};
  std::array<float, kCometPoolSize> tempo_comet_vel{};
  std::array<float, kCometPoolSize> tempo_comet_size{};
  std::array<float, kCometPoolSize> tempo_comet_life{};
  std::uint32_t tempo_comet_last_ms = 0U;
  std::uint32_t tempo_comet_last_event_id = 0U;
  float tempo_comet_last_phase = 0.0F;
  float tempo_comet_beat_phase = 0.0F;
  float tempo_comet_locked_bpm = 0.0F;
  std::uint16_t tempo_comet_coast_beats = 0U;

  std::array<float, kDenseForgeLatticeSize> dense_lattice_pos{};
  std::array<float, kDenseForgeLatticeSize> dense_lattice_vel{};
  std::array<float, kDenseForgeLatticeSize> dense_lattice_last{};
  float dense_carrier = 0.0F;
  float dense_activity_env = 0.0F;
  std::uint32_t dense_last_ms = 0U;
  bool dense_initialised = false;
  std::uint8_t dense_chord_held_root = 0U;
  std::uint8_t dense_chord_candidate_root = 0U;
  float dense_chord_candidate_ms = 0.0F;
  float dense_chord_hue = 0.0F;

  float snap_peak_env = 0.0F;
  float snap_amp_smooth = 0.0F;
  float snap_hue_ema = 0.0F;
  float snap_phase = 0.0F;
  std::uint32_t snap_last_ms = 0U;

  std::array<float, kPrismRingPoolSize> prism_radius{};
  std::array<float, kPrismRingPoolSize> prism_velocity{};
  std::array<float, kPrismRingPoolSize> prism_life{};
  std::array<float, kPrismRingPoolSize> prism_hue{};
  float prism_bed_env = 0.0F;
  std::uint32_t prism_last_ms = 0U;
  std::uint32_t prism_last_kick_id = 0U;
  std::uint32_t prism_last_transient_id = 0U;

  std::array<float, 12U> constellation_chroma_smooth{};
  std::uint32_t constellation_last_ms = 0U;

  std::array<float, kPercussionPoolSize> percussion_pos{};
  std::array<float, kPercussionPoolSize> percussion_last_pos{};
  std::array<float, kPercussionPoolSize> percussion_velocity{};
  std::array<float, kPercussionPoolSize> percussion_life{};
  std::array<float, kPercussionPoolSize> percussion_life_max{};
  std::array<float, kPercussionPoolSize> percussion_intensity{};
  std::array<float, kPercussionPoolSize> percussion_hue{};
  std::array<std::uint8_t, kPercussionPoolSize> percussion_active{};
  std::uint32_t percussion_last_ms = 0U;
  std::uint32_t percussion_kick_id = 0U;
  std::uint32_t percussion_snare_id = 0U;
  std::uint32_t percussion_hihat_id = 0U;
  bool percussion_hat_left = false;

  std::array<float, kCometPoolSize> anticipate_launch{};
  std::array<float, kCometPoolSize> anticipate_target{};
  std::array<float, kCometPoolSize> anticipate_period{};
  std::array<float, kCometPoolSize> anticipate_time{};
  std::array<float, kCometPoolSize> anticipate_size{};
  std::array<float, kCometPoolSize> anticipate_life{};
  std::uint32_t anticipate_last_ms = 0U;
  float anticipate_beat_phase = 0.0F;
  float anticipate_locked_bpm = 0.0F;
  std::uint16_t anticipate_coast_beats = 0U;

  float surge_tide_env = 0.0F;
  float surge_fast_env = 0.0F;
  float surge_slow_env = 0.0F;
  float surge_peak_env = 0.0F;
  float surge_build_recent_seconds = 0.0F;
  float surge_refractory_seconds = 0.0F;
  bool surge_wavefront_active = false;
  float surge_wavefront_pos = 0.0F;
  float surge_wavefront_life = 0.0F;
  std::uint32_t surge_last_ms = 0U;
  bool surge_initialised = false;

  std::uint32_t river_walk_last_ms = 0U;
  float river_walk_last_phase = 0.0F;
  std::uint8_t river_walk_beats = 0U;
  float river_walk_target = 0.0F;
  float river_walk_offset = 0.0F;

  std::uint32_t waveform_k1_last_ms = 0U;
  float waveform_k1_scroll_accum = 0.0F;
  float waveform_k1_peak_ema1 = 0.0F;
  float waveform_k1_peak_last = 0.0F;
  LinearRgb waveform_k1_dot{};
  float waveform_k1_hold_env = 0.0F;
  float waveform_k1_silence_scale = 0.0F;

  float palette_held_position = 0.0F;
  bool palette_held_position_valid = false;
};

}  // namespace k1::core::visual
