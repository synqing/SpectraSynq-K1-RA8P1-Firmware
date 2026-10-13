#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "core/visual/product_palette.h"
#include "core/visual/ws2816_pack.h"

namespace k1::core::visual {

// Source tracker emits every third 96-sample observation at 12,800 Hz. A held
// event can therefore precede a still-fresh AP observation by this interval.
inline constexpr std::uint64_t kInhouseTempoUpdatePeriodUs = 22500U;
inline constexpr float kInhouseQuietEntryRms = 0.04F;

// One coherent producer observation; spectrum is the source 80-note bank after
// audio gain/tilt, BEFORE visual smoothing. PCM peak is in signed-16 units.
struct InhouseWaveformAudio final {
  std::array<float, 80> spectrum{};
  float raw_peak_pcm = 0.0F;
  float raw_rms = 0.0F;
  float quiet_entry_rms = kInhouseQuietEntryRms;
  float raw_floor_pcm = 350.0F;  // Reconstructed producer drive floor, not room calibration.
  float peak_scaled = 0.0F;
  float observed_peak_scaled = 0.0F;
  bool peak_summary_valid = false;
  float vu_level = 0.0F;
  float vu_average = 0.0F;
  float spectral_energy = 0.0F;
  float novelty = 0.0F;
  std::array<std::array<std::int16_t, 96>, 4> pcm_history{};
  bool pcm_history_valid = false;
  float tempo_bpm = 0.0F;
  float tempo_phase01 = 0.0F;
  float tempo_confidence = 0.0F;
  bool tempo_valid = false;
  std::uint64_t tempo_capture_us = 0;
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
  std::uint8_t mode = 32;
  std::uint16_t palette_id = 40;
  float colour_offset = 0.05F;
  float motion_scale = 1.0F;
  float saturation = 1.0F;
  bool mirror = true;
  std::uint8_t chroma_range = 60;
  float sensitivity = 2.4F;  // Mode 8 displacement; distinct from input response.
  float square_iter = 1.0F;  // Modes 8, 7 and 11 source colour contrast.
  float photons = 1.0F;     // Mode 8 new-material gain; never scales stored history.
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
  // Cumulative nonzero source-pixel writes, before mirror. Mode 11 writes both
  // halves; this counts pixel additions/assignments, never musical events.
  std::uint64_t injection_count = 0;
};

// Source: effects/light_mode_waveform{_hybrid_k1,_tempo,,_fast,_hybrid}.cpp
// and visual/{lightshow_modes,led_utilities}.h in SpectraSynq_K1_Firmware.
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
  std::uint8_t mode_ = 32;
  [[nodiscard]] bool renderFamily(const InhouseWaveformAudio& audio,
      const InhouseWaveformControls& controls, float dt, bool fresh,
      Pixel16* output) noexcept;
  [[nodiscard]] PaletteLinearRgb familyColour(const InhouseWaveformControls& controls,
      float brightness, float offset) noexcept;
};

}  // namespace k1::core::visual
