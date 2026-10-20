#pragma once

#include <array>
#include <cstdint>
#include "core/visual/pixel_topology.h"
#include "core/visual/product_palette.h"
#include "core/visual/visual_audio_frame.h"
#include "core/visual/ws2816_pack.h"

namespace k1::core::visual {

// Reference light intent, not calibrated radiometric RGB. No Pixel8 round trip.
using RibbonRgb = PaletteLinearRgb;
using RibbonPixel16 = Pixel16;
using RibbonFrame = std::array<RibbonRgb, kPixelsPerChannel>;
using RibbonFrame16 = std::array<RibbonPixel16, kPixelsPerChannel>;

struct RibbonSettings {
  float velocity_px_s = 64.0F;
  float spread_px2_s = 0.12F;
  float half_life_s = 1.6F;
  float width_px = 2.0F;
  float tail_ratio = 1.8F;
  float launch_spacing_s = 0.70F;
  float gap_s = 0.32F;
  float return_strength = 0.24F;
  float injection_response = 1.6F;
  float persistence_s = 0.0F; // Per-layer immediate attack / exponential release.
  float saturation = 1.0F;
  float warmth = 0.0F;
  float contrast = 1.0F;
  std::uint16_t palette_id = 33U;
  bool recolour_history = false;
  bool knee = false;
};

struct RibbonObject {
  static constexpr unsigned kSamples=24;
  bool active = false;
  bool returning = false;
  bool waiting = false;
  float radius = 0.0F;
  float variance = 9.0F;
  float amplitude = 0.0F;
  float palette_position = 0.5F;
  float quiet_remaining_s = 0.0F;
  RibbonRgb colour{};
  std::uint64_t born_us = 0;
  // Deposited ribbon cross-section, ordered from its oldest front backwards.
  // Offsets are physical distances at deposit time, retained across speed edits.
  std::array<RibbonRgb,kSamples> samples{};
  std::array<float,kSamples> offsets{};
  unsigned sample_count = 0;
};

struct RibbonTelemetry {
  std::uint32_t launches{}, merged{}, omitted{};
  std::uint32_t active{}, outgoing{}, returning{};
  bool audio_available = false;
  bool frame_valid = true;
  float pre_limit_peak = 0.0F;
  float limiter_gain = 1.0F;
};

class RibbonMaterial final {
 public:
  static constexpr std::size_t kCapacity = 16;
  bool configure(const RibbonSettings& settings, std::uint64_t time_us) noexcept;
  void reset(std::uint64_t time_us = 0) noexcept;
  bool advance(std::uint64_t time_us) noexcept;
  bool deposit(float strength, float palette_position, std::uint64_t time_us) noexcept;
  void consume(const VisualAudioFrameView& audio, std::uint64_t time_us) noexcept;
  const RibbonFrame16& render(std::uint64_t time_us, float master = 24.0F/255.0F) noexcept;
  const RibbonFrame& intent() const noexcept { return frame_; }
  const auto& objects() const noexcept { return objects_; }
  const auto& settings() const noexcept { return settings_; }
  const auto& telemetry() const noexcept { return telemetry_; }
 private:
  RibbonSettings settings_{};
  std::array<RibbonObject, kCapacity> objects_{};
  RibbonFrame frame_{}, body_history_{}, return_history_{};
  RibbonFrame16 output_{};
  RibbonTelemetry telemetry_{};
  std::uint64_t time_us_ = 0, render_us_ = 0, last_launch_us_ = 0, last_audio_us_ = 0;
  std::uint32_t last_onset_ = 0, last_sequence_ = 0;
  float colour_position_ = 0.5F, prior_energy_ = 0.0F, material_energy_ = 0.0F;
  bool initialised_ = false, have_launch_ = false, have_sequence_ = false;
};

// Clamped palette semantics: unlike the compatibility sampler, position 1 is
// the final stop. No automatic hue-wheel travel.
RibbonRgb ribbonPalette(std::uint16_t id, float position) noexcept;
bool validRibbonSettings(const RibbonSettings& settings) noexcept;
} // namespace k1::core::visual
