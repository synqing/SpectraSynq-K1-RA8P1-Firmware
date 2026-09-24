#pragma once

// Sampled-field stage for the wide path: centre-origin transforms, typed
// history and declared composition operators.
//
// Geometry: 160 logical pixels per channel; distance d from the centre maps
// to pixels 80 + d (right) and 79 - d (left). Outward shift moves right-half
// content towards 159 and left-half content towards 0; nothing crosses the
// centre, and content pushed past either end is lost (declared edge loss).
//
// Outward transport law v1 (the legacy Bloom splat law in float): a value at
// pixel i moves to i -/+ shift and is split linearly between the two
// neighbouring pixels, scaled by `retention`. Mass is conserved apart from
// retention and edge loss. Repeated linear splatting adds numerical
// diffusion that depends on render cadence; this sampled field is therefore
// a reference-coupled material, not the cadence-independent analytic
// material of wide_material.h. Its diffusion is measured, not hidden.
//
// History meanings (typed; never reinterpreted):
//   kScalarRecolouredNow  intensity is stored and coloured by its owner at
//                         read time with the current palette window, so a
//                         palette change recolours the whole trail
//   kDepositedRgb         colour is stored when deposited, so old trails
//                         keep their original palette
// Changing the meaning is an owner-local cut: the slot is cleared and its
// generation increments. The slot is sized for RGB (three floats/pixel); the
// scalar meaning uses the first third for data and the second third as its
// own transport scratch.
//
// Composition operators (each maps all-black inputs to exact black):
//   additive          acc += gain * layer
//   weighted mix      out = (1 - w) a + w b         (a declared crossfade)
//   premultiplied over dst = src + (1 - alpha) dst (coverage applied once)
//   componentwise max acc = max(acc, layer)         (may mix hues)
// Weights are never renormalised over enabled layers: enabling a black layer
// cannot dim another layer.

#include <array>
#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

enum class ComposeOperatorV1 : std::uint8_t {
  kAdditive = 1U,
  kWeightedMix = 2U,
  kPremultipliedOver = 3U,
  kComponentMax = 4U,
};

void composeAddV1(WorkingFrame& accumulator, const WorkingFrame& layer,
                  float gain) noexcept;
void composeWeightedMixV1(WorkingFrame& out, const WorkingFrame& a,
                          const WorkingFrame& b, float weight_b) noexcept;
void composePremultipliedOverV1(WorkingFrame& destination,
                                const WorkingFrame& source_premultiplied,
                                const ScalarFrame& source_alpha) noexcept;
void composeMaxV1(WorkingFrame& accumulator, const WorkingFrame& layer) noexcept;

// Stage boundary for the pre-tone accumulator: [0, kAccumulatorComponentMax].
void boundAccumulatorV1(WorkingFrame& accumulator,
                        StageBoundaryCountersV1& counters) noexcept;

void clearFrameV1(WorkingFrame& frame) noexcept;

// Outward centre-origin shift of `source` into `destination` (cleared first).
// shift_px >= 0 finite; retention in [0, 1]. Invalid arguments leave the
// destination cleared (no contribution) and return false.
[[nodiscard]] bool shiftOutwardV1(const WorkingFrame& source,
                                  WorkingFrame& destination, float shift_px,
                                  float retention) noexcept;

enum class MirrorPolicyV1 : std::uint8_t {
  kNone = 0U,
  kRightToLeft = 1U,  // pixel 79 - d takes pixel 80 + d
  kLeftToRight = 2U,  // pixel 80 + d takes pixel 79 - d
};

void applyMirrorV1(WorkingFrame& frame, MirrorPolicyV1 policy) noexcept;

enum class HistoryMeaningV1 : std::uint8_t {
  kScalarRecolouredNow = 1U,
  kDepositedRgb = 2U,
};

struct HistoryFieldV1 final {
  HistoryMeaningV1 meaning = HistoryMeaningV1::kDepositedRgb;
  std::array<float, 3U * kPixelsPerChannel> storage{};
  std::uint32_t generation = 0U;
  std::uint32_t cuts = 0U;
};

using DistanceColourTable = std::array<WorkingRgbF32V1, kPixelsPerHalf>;

void clearHistoryV1(HistoryFieldV1& field) noexcept;

// Same meaning: no effect. Different meaning: cut (clear, generation + 1).
void setHistoryMeaningV1(HistoryFieldV1& field,
                         HistoryMeaningV1 meaning) noexcept;

// Transports the retained field outward. The RGB meaning uses `scratch`; the
// scalar meaning uses its own spare storage and leaves `scratch` untouched.
[[nodiscard]] bool transportHistoryOutwardV1(HistoryFieldV1& field,
                                             float shift_px, float retention,
                                             WorkingFrame& scratch) noexcept;

// Ribbon body law v1 (cadence-consistent emission). Discretises
//   dI/dt + v dI/dd = -lambda I,  I(0, t) = E(t)
// on each half with backward semi-Lagrangian advection: a pixel at distance
// d >= s (s = v dt) takes the previous field linearly interpolated at d - s,
// times the frame retention; the vacated region d < s is filled with the
// emission interpolated across the frame interval and decayed by its age:
//   I(d) = lerp(E_now, E_previous, d / s) * retention^(d / s).
// Emission therefore enters at a rate, not as one stamp per frame, so the
// steady profile E * 2^(-d / (v h)) and the speed of a front do not depend
// on render cadence; linear interpolation still adds a small cadence-
// dependent numerical diffusion, which is measured. With s = 0 the centre
// pair takes E_now and the rest decays. Material leaving the strip end is
// lost. The RGB meaning uses `scratch`; the scalar meaning uses its spare
// storage.
[[nodiscard]] bool advectBodyOutwardV1(HistoryFieldV1& field, float shift_px,
                                       float retention,
                                       const WorkingRgbF32V1& emission_now,
                                       float amplitude_now,
                                       const WorkingRgbF32V1& emission_previous,
                                       float amplitude_previous,
                                       WorkingFrame& scratch) noexcept;

// Writes the centre pair (assignment, as the legacy injection does). The RGB
// meaning stores `colour`; the scalar meaning stores `amplitude`.
void injectHistoryCentreV1(HistoryFieldV1& field, const WorkingRgbF32V1& colour,
                           float amplitude) noexcept;

// Read-out into a layer surface. RGB meaning: stored colour. Scalar meaning:
// stored intensity x colour_by_distance[d] (recoloured now).
void renderHistoryLayerV1(const HistoryFieldV1& field,
                          const DistanceColourTable& colour_by_distance,
                          WorkingFrame& layer) noexcept;

[[nodiscard]] float historyScalarAtV1(const HistoryFieldV1& field,
                                      std::size_t pixel) noexcept;
[[nodiscard]] WorkingRgbF32V1 historyRgbAtV1(const HistoryFieldV1& field,
                                             std::size_t pixel) noexcept;

}  // namespace k1::core::visual::wide
