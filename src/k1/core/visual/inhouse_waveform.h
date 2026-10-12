#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"

namespace k1::core::visual {

// One coherent producer observation; spectrum is the source 80-note bank after
// audio gain/tilt, BEFORE visual smoothing. PCM peak is in signed-16 units.
struct InhouseWaveformAudio final {
  std::array<float, 80> spectrum{};
  float raw_peak_pcm = 0.0F;
  float peak_scaled = 0.0F;
  float observed_peak_scaled = 0.0F;
  bool peak_summary_valid = false;
  float vu_level = 0.0F;
  bool silence = true;
  std::uint64_t epoch = 0;
  std::uint64_t generation = 0;
  std::uint64_t analysis_begin_sample = 0;
  std::uint64_t analysis_end_sample_exclusive = 0;
  std::uint64_t capture_us = 0;
  std::uint64_t publish_us = 0;
  bool valid = false;
};

struct InhouseWaveformControls final {
  std::uint16_t palette_id = 40;
  float colour_offset = 0.05F;
  float motion_scale = 1.0F;
  float saturation = 1.0F;
  bool mirror = true;
  std::uint8_t chroma_range = 60;
};

// Read-only feature witness for source-vector comparisons, never output control.
struct InhouseWaveformObservation final {
  std::array<float, 12> chroma_pregate{};
  std::array<float, 12> chroma{};
  float peak_stage1 = 0.0F;
  float peak_stage2 = 0.0F;
  float position = 80.0F;
  float palette_coordinate = 0.0F;
  float held_coordinate = 0.0F;
  bool held_coordinate_valid = false;
  float decay_rate = 0.0F;
  float scroll_remainder = 0.0F;
  unsigned scroll_steps = 0;
  bool input_fresh = false;
  PaletteLinearRgb smoothed_colour{};
  float deposit_strength = 0.0F;
  std::uint64_t injection_count = 0;  // Cumulative nonzero upper-half pixel writes.
};

// Source: effects/light_mode_waveform_hybrid_k1.cpp and visual/{lightshow_modes,
// led_utilities}.h in the read-only SpectraSynq_K1_Firmware reference.
// Every mutable follower/history belongs to this object, independently per board.
class InhouseWaveformRenderer final {
 public:
  void reset() noexcept;
  [[nodiscard]] bool render(const InhouseWaveformAudio& audio,
      const InhouseWaveformControls& controls, std::uint64_t render_us,
      Pixel16* output, std::size_t count) noexcept;
  [[nodiscard]] const InhouseWaveformObservation& observation() const noexcept {
    return observation_;
  }
 private:
  struct FixedRgb { std::int32_t red = 0, green = 0, blue = 0; };
  std::array<FixedRgb, 160> history_{};
  std::array<std::int32_t, 80> spectrum_{};
  std::int32_t chroma_max_ = 65;
  float hold_ = 0.0F;
  float silence_scale_ = 0.0F;
  PaletteLinearRgb dot_{};
  float saturation_ema_ = 0.0F;
  bool held_valid_ = false;
  bool started_ = false;
  std::uint64_t last_render_us_ = 0;
  std::uint64_t newest_capture_us_ = 0;
  std::uint64_t epoch_ = 0;
  std::uint64_t generation_ = 0;
  InhouseWaveformObservation observation_{};
};

}  // namespace k1::core::visual
