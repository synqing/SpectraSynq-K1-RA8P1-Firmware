#pragma once

// Palette stage for the wide path.
//
// Palette window v1 (clamped): for a source coordinate u in [0, 1]
//   s = clamp(position + span * (u - 0.5) + travel_offset, 0, 1)
//   travel_offset = 0.28 * travel_depth * q
// Palette travel is excursion DEPTH, not speed: q in [-1, 1] is a bounded
// source declared and timed by the mode adapter (MOD); travel_depth scales
// how far the window swings. Travel never rewrites position. A separate
// travel-rate control would need its own descriptor. Position 0.5 with span
// 1 and depth 0 exposes the whole palette unchanged. Legal UI span is
// 0.02..1; the primitive also admits span 0 (single-colour limit). Sampling
// clamps at both ends; this v1 sampler never wraps.
//
// The sampler reproduces the legacy stop interpolation of
// sampleProductPaletteHd() bit for bit on [0, 1) (differential test in
// test_visual_wide_stages) and differs only at s = 1, where the legacy
// sampler wraps to the first stop and this one returns the last stop.
//
// Palette transition v1: selecting a palette starts a linear crossfade of
// SAMPLED COLOURS at the same coordinate (the effect source is evaluated
// once; no second advance). An interruption snapshots the currently resolved
// mix as the starting point, so it never jumps back to an old target.
// Re-selecting the current target does not restart. Capacity: four
// contributors; overflow drops the lightest contributor, renormalises and is
// counted in `merges`. At rest the target palette is sampled directly.

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr float kPaletteTravelExcursionV1 = 0.28F;
inline constexpr float kPaletteTransitionMinimumSeconds = 0.02F;
inline constexpr float kPaletteTransitionMaximumSeconds = 5.0F;

struct PaletteWindowV1 final {
  float position = 0.5F;
  float span = 1.0F;
  float travel_depth = 0.0F;
};

enum class PaletteWindowValidationV1 : std::uint8_t {
  kValid = 0U,
  kPosition = 1U,
  kSpan = 2U,
  kTravelDepth = 3U,
};

[[nodiscard]] PaletteWindowValidationV1 validatePaletteWindowV1(
    const PaletteWindowV1& window) noexcept;

// Bounded travel source: non-finite -> 0, outside [-1, 1] -> clamped; both
// counted.
[[nodiscard]] float paletteTravelOffsetV1(float travel_depth, float travel_q,
                                          StageBoundaryCountersV1& counters) noexcept;

[[nodiscard]] float paletteWindowCoordinateV1(const PaletteWindowV1& window,
                                              float u,
                                              float travel_offset) noexcept;

// Stop interpolation on normalised legacy codes, clamped to [0, 1]. Unknown
// palette ids use the catalogue's own sanitisation (id 0).
[[nodiscard]] LegacyPaletteCodeV1 samplePaletteCodeClampedV1(
    std::uint16_t palette_id, float coordinate) noexcept;

inline constexpr std::size_t kPaletteContributorCapacity = 4U;

struct PaletteContributorV1 final {
  std::uint16_t palette_id = 0U;
  float weight = 0.0F;
};

struct PaletteTransitionStateV1 final {
  std::uint16_t target = 0U;
  std::array<PaletteContributorV1, kPaletteContributorCapacity> from{};
  std::uint8_t from_count = 0U;
  float progress = 1.0F;  // 1 means at rest on `target`
  float duration_s = 0.5F;
  std::uint32_t selections = 0U;
  std::uint32_t repeated_selections = 0U;
  std::uint32_t interruptions = 0U;
  std::uint32_t merges = 0U;
};

void initialisePaletteTransitionV1(PaletteTransitionStateV1& state,
                                   std::uint16_t palette_id) noexcept;

enum class PaletteSelectResultV1 : std::uint8_t {
  kStarted = 0U,
  kCut = 1U,
  kUnchanged = 2U,
  kRejectedDuration = 3U,
};

// duration_s 0 is an explicit cut; otherwise it must lie in
// [kPaletteTransitionMinimumSeconds, kPaletteTransitionMaximumSeconds].
PaletteSelectResultV1 selectPaletteV1(PaletteTransitionStateV1& state,
                                      std::uint16_t palette_id,
                                      float duration_s) noexcept;

void advancePaletteTransitionV1(PaletteTransitionStateV1& state,
                                float delta_seconds) noexcept;

[[nodiscard]] bool paletteTransitionActiveV1(
    const PaletteTransitionStateV1& state) noexcept;

// Resolved weight of one palette id at the current progress (0 if absent).
[[nodiscard]] float paletteResolvedWeightV1(
    const PaletteTransitionStateV1& state, std::uint16_t palette_id) noexcept;

[[nodiscard]] LegacyPaletteCodeV1 samplePaletteTransitionV1(
    const PaletteTransitionStateV1& state, float coordinate) noexcept;

}  // namespace k1::core::visual::wide
