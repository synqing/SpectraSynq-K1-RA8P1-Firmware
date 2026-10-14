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

#if defined(K1_INHOUSE_MAPPING_V9)
// Boot composition v9 (lane P6, Captain ruling 1): level drives deposit amplitude and
// bounded width, release is constant, colour is a slow layer that holds when tonality
// drops. Modes 32 and 18 only; the other family modes keep the v8 path. Without this
// define the renderer is the unchanged v8 reference profile.
enum class InhouseV9Variant : std::uint8_t {
  kConstantPeak = 0,     // peak fixed; level drives width only
  kConstantEnergy = 1,   // integrated deposit energy fixed; level drives spread only
  kLevelEmphasis = 2,    // peak = level^1.5 (bounded) and level-driven width
};
// Planted faults for the frozen P6 gates; compiled only into the host probe.
enum class InhouseV9Fault : std::uint8_t {
  kNone = 0, kPerChannelMax = 1, kBrightnessFloor = 2, kRgbBlend = 3,
  kRgbMixPalette = 4, kSlot16 = 5, kFrozenPosition = 6,  // PAL planted faults (path P)
};
// Chosen by the frozen P6 rule at matched optical energy (TOLERANCE-FROZEN-P6.json,
// iteration 4, palette variant B). Gains match v8's mean light on the BOOM file/room +
// Sonic corpus per mode.
inline constexpr InhouseV9Variant kInhouseV9Variant = InhouseV9Variant::kLevelEmphasis;
// P6 path P matched-average-light calibration (calibration-P-fix.json); test parameter under the Captain's testing ruling.
// Scored replay-pfix/P passed these as "%.6f" argv ("0.580092", "0.830949"). That is the float32 the probe executed.
// 0.58009236F / 0.83094886F are a different rounding of the same JSON doubles and were not the executed gains.
inline constexpr float kInhouseV9Gain32 = 0.580092F;  // mode 32 deposit gain
inline constexpr float kInhouseV9Gain18 = 0.830949F;  // mode 18 deposit gain
// P6 iteration 4 test variants (P6-IT4-PALETTE-VARIANTS-FROZEN.json), v9 only; product
// palettes and v8 are untouched. A lifts the dark low end of palettes 32/39 (ids 31/38)
// keeping each stop's hue; B folds the colour circle onto a sub-range without the ends.
// kPerPixelMapped (iteration P, TOLERANCE-FROZEN-P6-PAL.json): the K1 per-bin HD mapping; trails
// carry intensity and the deposited palette offset, colour is sampled per pixel at output.
enum class InhouseV9Palette : std::uint8_t {
  kAsIs = 0, kLiftedLowEnd = 1, kRestrictedRange = 2, kPerPixelMapped = 3,
};
// TEST VARIANT under the Captain's testing ruling (14 Oct 2026); not a palette change.
// Path P: K1 per-bin HD mapping, TOLERANCE-FROZEN-P6-PAL / PAL-HD-v2.
inline constexpr InhouseV9Palette kInhouseV9Palette = InhouseV9Palette::kPerPixelMapped;
// Level transfer ceiling (dBFS at l = 1; floor -33 dBFS) and emphasis exponent (peak = g l^gamma).
inline constexpr float kInhouseV9LevelCeilingDb = -9.0F;
inline constexpr float kInhouseV9Emphasis = 1.5F;
// Iteration 6 (P6-IT6-DESIGN-CANDIDATES-FROZEN.json). Drop accent keyed to low-band onset
// density against a several-second baseline (the cut lets the baseline fall); 0 = off.
struct InhouseV9Accent final {
  float boost = 0.0F;         // extra deposit amplitude at full accent (x(1 + boost))
  float widen_px = 0.0F;      // extra half-width at full accent
  float ratio_full = 3.0F;    // fast/slow onset density ratio giving full accent
  float epsilon = 0.0F;       // density floor in both terms of the ratio
  float fast_s = 0.4F;        // fast onset-density time constant
  float slow_s = 4.0F;        // slow baseline time constant
};
inline constexpr InhouseV9Accent kInhouseV9AccentConfig{};
inline constexpr bool kInhouseV9LumaHold = false;  // colour luma hold through low confidence
// Extent release (P6-EXT-DESIGN-CANDIDATES-FROZEN.json): a pre-AGC level surge against a slow
// baseline pulls the deposit inward so the waveform spans edge to centre; off by default.
struct InhouseV9Release final {
  bool enabled = false;
  float fast_s = 0.03F;          // fast power follower
  float slow_s = 2.0F;           // slow power baseline
  float lo_db = 3.0F;            // surge below this gives no release
  float span_db = 6.0F;          // surge range to full release
  float release_s = 0.6F;        // release envelope decay
  float newpeak_db_s = 0.0F;     // > 0: new-peak gate with this peak-holder decay
  bool trigger_sub = false;      // true: surge of the 40-110 Hz sub_rms instead of vu_level
  float dip_db = 0.0F;           // > 0: broadband span over [t - 6 s, t - 0.5 s] must reach this
};
inline constexpr InhouseV9Release kInhouseV9ReleaseConfig{};
// Experiment 1 (host only). Default off. The product image does not define
// K1_INHOUSE_V9_HOST_VARIANTS, so this type is not part of that build.
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
struct InhouseV9Front final {
  bool enabled = false;
  bool hold_contraction = false;  // R03: keep the outer-band contraction until the drop
  float travel_s = 0.12F;         // R01 travel, inside 0.100..0.180 s (donor floor is 0.200 s)
  float hold_lead_s = 6.0F;       // riser hold, ending at the labelled drop
  std::array<float, 8> drop_s{};  // probe clock, file time in seconds
  unsigned drop_count = 0;
};
#endif
#endif

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
#if defined(K1_INHOUSE_MAPPING_V9)
  float sub_rms = 0.0F;  // v9: 40-110 Hz band RMS of the hop PCM (core/audio/sub_band.h)
  // Path P only: pre-knee bin shape (magnitude_last * tilt). Unused by v8 and by other v9 palettes.
  std::array<float, 80> linear_shape{};
#endif
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
#if defined(K1_INHOUSE_MAPPING_V9)
  float level = 0.0F;          // v9 fixed dB transfer of the pre-AGC RMS
  float tempo_weight = 0.0F;   // v9 Tempo board: share of motion from the tempo winner
  float tonality_gate = 0.0F;  // v9 colour-layer inputs (stage-4 outputs, read only)
  float centroid_strength = 0.0F;
  float intensity_mean = 0.0F; // v9 mean intensity field before colour (light without palette)
  float low_flux = 0.0F;       // v9 iteration 6: low-band (bins 0-11) positive spectral flux
  float accent = 0.0F;         // v9 iteration 6: drop accent 0..1
  float luma_ref = 0.0F;       // v9 iteration 6: colour luma reference (luma hold)
  float release = 0.0F;        // v9 extent release envelope 0..1
#endif
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
#if defined(K1_INHOUSE_MAPPING_V9) && defined(K1_INHOUSE_V9_HOST_VARIANTS)
  // Host comparison only: select a P6 variant, a planted fault and per-mode gains.
  [[nodiscard]] bool setV9HostConfig(InhouseV9Variant variant, InhouseV9Fault fault,
                                     float gain32, float gain18) noexcept {
    if (static_cast<unsigned>(variant) > 2U || static_cast<unsigned>(fault) > 6U ||
        !(gain32 > 0.0F && gain32 <= 64.0F) || !(gain18 > 0.0F && gain18 <= 64.0F)) return false;
    v9_variant_ = variant; v9_fault_ = fault; v9_gain_[0] = gain32; v9_gain_[1] = gain18;
    return true;
  }
  // Iteration 5 candidates (P6-IT5-DESIGN-CANDIDATES-FROZEN.json): level ceiling and exponent.
  [[nodiscard]] bool setV9HostAccent(const InhouseV9Accent& accent, bool luma_hold) noexcept {
    if (!(accent.boost >= 0.0F && accent.boost <= 4.0F) || !(accent.widen_px >= 0.0F && accent.widen_px <= 4.0F) ||
        !(accent.ratio_full > 1.0F) || !(accent.epsilon >= 0.0F) || !(accent.fast_s > 0.0F) ||
        !(accent.slow_s > accent.fast_s)) return false;
    v9_accent_ = accent; v9_luma_hold_ = luma_hold;
    return true;
  }
  [[nodiscard]] bool setV9HostRelease(const InhouseV9Release& release) noexcept {
    if (!(release.fast_s > 0.0F) || !(release.slow_s > release.fast_s) || !(release.span_db > 0.0F) ||
        !(release.release_s > 0.0F) || !(release.newpeak_db_s >= 0.0F)) return false;
    v9_release_ = release;
    return true;
  }
  [[nodiscard]] bool setV9HostLevel(float ceiling_db, float emphasis) noexcept {
    if (!(ceiling_db > -30.0F && ceiling_db <= 0.0F) || !(emphasis >= 1.0F && emphasis <= 6.0F)) return false;
    v9_ceiling_db_ = ceiling_db; v9_emphasis_ = emphasis;
    return true;
  }
  [[nodiscard]] bool setV9HostPalette(InhouseV9Palette palette) noexcept {
    if (static_cast<unsigned>(palette) > 3U) return false;
    v9_palette_ = palette;
    return true;
  }
  // R01 inward release, optional R03 hold. Oracle drop times only; default off.
  [[nodiscard]] bool setV9HostFront(const InhouseV9Front& front) noexcept;
  void setV9HostClock(float file_time_s) noexcept { v9_clock_s_ = file_time_s; }
  // C5b hue probe: one deposit of an explicit palette position through the v9 release,
  // deposit and output code (dt > 0 applies one release step first).
  [[nodiscard]] bool probeV9Deposit(const InhouseWaveformControls& controls, float palette_position,
                                    float level, float position, float dt, Pixel16* output) noexcept;
#endif
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
#if defined(K1_INHOUSE_MAPPING_V9)
  // Intensity field (Q16) and the palette colour each pixel was deposited with.
  std::array<std::int32_t, 160> intensity_{};
  std::array<FixedRgb, 160> colour_{};
  std::array<float, 160> offset_{};   // path P: palette offset deposited into each pixel
  float deposit_offset_ = 0.0F;        // path P: offset the next deposit stores
  float colour_position_ = 0.0F;
  float colour_velocity_ = 0.0F;
  float colour_anchor_ = 0.0F;  // long-term confident colour (iteration 3)
  std::array<float, 12> low_previous_{};  // iteration 6 onset density state
  float density_fast_ = 0.0F;
  float density_slow_ = 0.0F;
  float accent_ = 0.0F;
  float luma_ref_ = -1.0F;      // iteration 6: luma of the last strongly confident colour
  float release_fast_ = 0.0F;   // extent release state (power followers, peak holder, envelope)
  float release_slow_ = 0.0F;
  float release_peak_db_ = -120.0F;
  float release_ = 0.0F;
  std::array<float, 60> dip_ring_{};  // 0.1 s broadband dB means for the dip gate (6 s)
  unsigned dip_count_ = 0, dip_index_ = 0;
  float dip_acc_ = 0.0F, dip_acc_s_ = 0.0F;
  bool colour_valid_ = false;
  std::int32_t tonality_gate_ = 0;
  float centroid_ = 0.0F;
  float centroid_strength_ = 0.0F;
  std::array<float, 80> shape_{};  // path P: smoothed max-normalised pre-knee shape
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  InhouseV9Variant v9_variant_ = kInhouseV9Variant;
  InhouseV9Fault v9_fault_ = InhouseV9Fault::kNone;
  float v9_gain_[2] = {kInhouseV9Gain32, kInhouseV9Gain18};
  InhouseV9Palette v9_palette_ = kInhouseV9Palette;
  float v9_ceiling_db_ = kInhouseV9LevelCeilingDb;
  float v9_emphasis_ = kInhouseV9Emphasis;
  InhouseV9Accent v9_accent_ = kInhouseV9AccentConfig;
  bool v9_luma_hold_ = kInhouseV9LumaHold;
  InhouseV9Release v9_release_ = kInhouseV9ReleaseConfig;
  InhouseV9Front v9_front_{};
  float v9_clock_s_ = 0.0F;
#endif
  [[nodiscard]] bool renderV9(const InhouseWaveformAudio& audio,
      const InhouseWaveformControls& controls, float dt, bool fresh, bool silent,
      Pixel16* output) noexcept;
  void updateOffsetP(const InhouseWaveformControls& controls, bool silent) noexcept;
  void applyShapeCentroidP(const InhouseWaveformAudio& audio, const InhouseWaveformControls& controls,
                           float dt, bool fresh) noexcept;
  [[nodiscard]] bool perPixelMapped() const noexcept;
  void updateColourV9(const InhouseWaveformControls& controls, float dt, bool silent,
                      float level) noexcept;
  void fadeV9(bool mode32, bool silent, float dt) noexcept;
  void updateAccentV9(const InhouseWaveformAudio& audio, bool fresh, bool silent, float dt) noexcept;
  void updateReleaseV9(const InhouseWaveformAudio& audio, bool fresh, bool silent, float dt) noexcept;
  [[nodiscard]] PaletteLinearRgb colourV9(std::uint16_t palette_id, float coordinate) const noexcept;
  void depositV9(bool mode32, unsigned start, float position, float level,
                 const FixedRgb& colour) noexcept;
#if defined(K1_INHOUSE_V9_HOST_VARIANTS)
  void applyOracleFront(bool mode32, unsigned start) noexcept;
#endif
  void outputV9(const InhouseWaveformControls& controls, const FixedRgb& colour,
                Pixel16* output) noexcept;
#endif
};

}  // namespace k1::core::visual
