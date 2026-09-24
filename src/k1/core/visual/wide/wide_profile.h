#pragma once

// First wide appearance profile: "ribbon v1". One fixed stage graph per
// channel; presets choose parameters and enables, never code or order.
//
// Stage order (renderRibbonChannelV1):
//   S0 time        integer presentation timestamp -> dt (wide_temporal.h);
//                  a rejected timestamp renders nothing and mutates nothing
//   S1 palette     transition advance, travel offset, 80-entry colour table
//                  over the palette window (u = d / 79)
//   S2 body        typed history slot advected outward at body motion with
//                  retention 2^(-dt/h_body) and the envelope emitted at a
//                  rate at the centre (advectBodyOutwardV1), then read out
//                  (scalar recoloured now / deposited RGB)
//   S3 body persistence (optional, per layer, before any gain)
//   S4 compose     output = (1 - rhythm_mix) * body
//   S5 accent      analytic pulses: recorded spawns (colour deposited from
//                  the palette window at spawn), retirement, rasterisation
//   S6 accent persistence (optional)
//   S7 compose     output += rhythm_mix * accent
//   S8 atmosphere  bounded base (<= 0.25) coloured by the window, suppressed
//                  while quiet; optional persistence; output += atmosphere
//   S9 bound       accumulator [0, 12], counted
//   S10 colour     saturation -> warmth -> contrast -> knee
// The endpoint (wide_endpoint.h) then applies channel/master intent, the
// single device transfer, the shared current limit and one quantisation.
//
// Layer rules: a disabled layer contributes exact zero and its whole chain
// (generator and its persistence) is bypassed; a layer's persistence is gated
// by the layer enable. Weights come from rhythm_mix alone and are never
// renormalised over enabled layers. Declared lifecycles: body disable clears
// the body history; accent disable clears the pulse pool; persistence follows
// its own PersistenceLifecycleV1. No stage reads final gain, limiter or
// quantisation output, so nothing downstream feeds back into artistic state.
//
// Channel isolation: each channel owns a complete RibbonChannelStateV1; there
// is no shared mutable state. The endpoint's shared current limit is the only
// declared A/B coupling and it touches output words, never artistic state.
//
// Controls arrive as validated transactions applied at a recorded effective
// time; every field of both channels is validated before any mutation, and
// only the owners of changed fields are dispatched (law edit re-anchors
// pulses, palette change starts a transition, meaning change cuts the
// history). Liveiness and other macros are resolved by their owners (MOD)
// into the effective values carried here.

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/visual/pixel_topology.h"
#include "core/visual/wide/wide_colour.h"
#include "core/visual/wide/wide_field.h"
#include "core/visual/wide/wide_material.h"
#include "core/visual/wide/wide_palette.h"
#include "core/visual/wide/wide_temporal.h"
#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr std::uint16_t kRibbonProfileVersion = 1U;
inline constexpr std::size_t kRibbonLayerCount = 3U;
inline constexpr std::size_t kRibbonMaximumSpawnsPerFrame = 4U;
inline constexpr float kRibbonAtmosphereMaximum = 0.25F;
inline constexpr float kRibbonMotionMaximumPxS = 240.0F;

enum class RibbonLayerV1 : std::uint8_t {
  kBody = 0U,
  kAccent = 1U,
  kAtmosphere = 2U,
};

struct RibbonControlsV1 final {
  bool body_enabled = true;
  float body_level = 1.0F;  // [0, 4]
  HistoryMeaningV1 body_meaning = HistoryMeaningV1::kScalarRecolouredNow;
  float body_motion_px_s = 80.0F;  // [0, 240]
  float body_half_life_s = 0.35F;  // [0.02, 4]
  bool accent_enabled = true;
  float rhythm_mix = 0.4F;  // accent weight in the body/accent crossfade
  MaterialLawV1 accent_law{80.0F, 2.0F, 0.35F};
  float accent_width_px = 2.0F;  // sigma at birth
  PulseNormalisationV1 accent_normalisation = PulseNormalisationV1::kPeak;
  bool atmosphere_enabled = false;
  float atmosphere_level = 0.04F;  // [0, 0.25]
  std::array<PersistenceControlsV1, kRibbonLayerCount> persistence{};
  std::uint16_t palette_id = 0U;
  PaletteWindowV1 window{0.3F, 0.7F, 0.2F};
  float palette_transition_s = 0.5F;  // 0 = cut, else [0.02, 5]
  ColourTreatmentControlsV1 colour{};
};

enum class RibbonValidationV1 : std::uint8_t {
  kValid = 0U,
  kBodyLevel = 1U,
  kBodyMeaning = 2U,
  kBodyMotion = 3U,
  kBodyHalfLife = 4U,
  kRhythmMix = 5U,
  kAccentLaw = 6U,
  kAccentWidth = 7U,
  kAccentNormalisation = 8U,
  kAtmosphere = 9U,
  kPersistence = 10U,
  kPalette = 11U,
  kWindow = 12U,
  kTransition = 13U,
  kColour = 14U,
};

[[nodiscard]] RibbonValidationV1 validateRibbonControlsV1(
    const RibbonControlsV1& controls) noexcept;

struct AccentSpawnV1 final {
  std::uint64_t event_us = 0U;
  PulseRoleV1 role = PulseRoleV1::kPrimary;
  float amplitude = 1.0F;
  float palette_u = 0.5F;  // window-relative palette coordinate at spawn
};

struct RibbonFrameInputV1 final {
  std::uint64_t presentation_us = 0U;
  float body_amplitude = 0.0F;  // mode-resolved envelope, >= 0
  float travel_q = 0.0F;        // mode-resolved travel source, [-1, 1]
  std::array<AccentSpawnV1, kRibbonMaximumSpawnsPerFrame> spawns{};
  std::uint8_t spawn_count = 0U;
  bool quiet = false;  // quiet-space policy: atmosphere suppressed
};

enum class WideTapPointV1 : std::uint8_t {
  kBody = 1U,
  kBodyPersisted = 2U,
  kAccent = 3U,
  kAccentPersisted = 4U,
  kAtmosphere = 5U,
  kComposite = 6U,
  kColourTreated = 7U,
};

enum class RibbonStageV1 : std::uint8_t {
  kTime = 0U,
  kPalette = 1U,
  kBody = 2U,
  kBodyPersistence = 3U,
  kAccent = 4U,
  kAccentPersistence = 5U,
  kAtmosphere = 6U,
  kBound = 7U,
  kColour = 8U,
  kCount = 9U,
};

inline constexpr std::size_t kRibbonStageCount =
    static_cast<std::size_t>(RibbonStageV1::kCount);

// Capture/probe hook. `capture` receives read-only stage surfaces for the
// evaluator (VP-05 comparisons, stage traces). `cycles` is an optional
// platform cycle counter (for example DWT CYCCNT on the RT1062) used to time
// each stage on target; a null sink costs nothing.
struct WideTapSinkV1 final {
  void* context = nullptr;
  void (*capture)(void* context, PixelChannelId channel, WideTapPointV1 point,
                  const WorkingFrame& surface) noexcept = nullptr;
  std::uint32_t (*cycles)(void* context) noexcept = nullptr;
};

struct WideCostCountersV1 final {
  std::array<std::uint32_t, kRibbonStageCount> stage_cycles{};
  std::uint32_t exp_calls = 0U;
  std::uint32_t exp2_calls = 0U;
  std::uint32_t pow_calls = 0U;
  std::uint32_t palette_samples = 0U;
  std::uint32_t objects_rasterised = 0U;
  std::uint32_t pixel_passes = 0U;
};

struct RibbonChannelStateV1 final {
  PixelChannelId channel = PixelChannelId::kChannelA;
  RibbonControlsV1 controls{};
  TimelineV1 timeline{};
  PaletteTransitionStateV1 palette{};
  HistoryFieldV1 body_history{};
  PulsePoolV1 accent_pool{};
  std::array<RgbPersistenceStateV1, kRibbonLayerCount> persistence{};
  bool body_enabled_last = false;
  bool accent_enabled_last = false;
  bool body_emission_valid = false;
  float body_previous_amplitude = 0.0F;
  WorkingRgbF32V1 body_previous_emission{};
  std::uint32_t body_resets = 0U;
  std::uint32_t accent_resets = 0U;
  std::uint32_t spawns_ignored = 0U;
  std::uint32_t spawns_rejected_future = 0U;
  std::uint64_t frames = 0U;
  WorkingFrame output{};  // S10 result; the endpoint input
  WorkingFrame layer{};   // reusable layer scratch, one layer live at a time
  DistanceColourTable colour_by_distance{};
  StageBoundaryCountersV1 boundary{};
  WideCostCountersV1 cost{};
};

// Installs validated controls; returns false (state untouched) when invalid.
[[nodiscard]] bool initialiseRibbonChannelV1(RibbonChannelStateV1& state,
                                             PixelChannelId channel,
                                             const RibbonControlsV1& controls) noexcept;

struct RibbonFrameResultV1 final {
  TimeStepV1 step{};
  bool rendered = false;
};

RibbonFrameResultV1 renderRibbonChannelV1(RibbonChannelStateV1& state,
                                          const RibbonFrameInputV1& input,
                                          const WideTapSinkV1* taps) noexcept;

struct RibbonTransactionV1 final {
  bool has_a = false;
  bool has_b = false;
  RibbonControlsV1 a{};
  RibbonControlsV1 b{};
  std::uint64_t effective_us = 0U;
};

enum class RibbonTransactionResultV1 : std::uint8_t {
  kApplied = 0U,
  kRejectedA = 1U,
  kRejectedB = 2U,
  kRejectedTime = 3U,
  kEmpty = 4U,
};

// All-or-nothing across both channels.
RibbonTransactionResultV1 applyRibbonTransactionV1(
    RibbonChannelStateV1& channel_a, RibbonChannelStateV1& channel_b,
    const RibbonTransactionV1& transaction) noexcept;

}  // namespace k1::core::visual::wide
