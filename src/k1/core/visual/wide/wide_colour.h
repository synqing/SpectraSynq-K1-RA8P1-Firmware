#pragma once

// Common artistic colour treatment for the wide path. Fixed order:
// saturation -> warmth -> contrast -> knee. Every operator works in
// WorkingRgbF32V1 (reference light intent), is stateless, has an explicit
// enable independent of its amount, and is an exact identity when disabled
// or at its neutral value (an early return, not arithmetic that happens to
// cancel). Palette sampling is not repeated here.
//
// Reference luma uses the Rec. 709 weights .2126/.7152/.0722 applied to the
// working intent. They are reference weights, not measured LED luminance.
//
// Semantic versions (v1 laws; a replacement law gets a fresh version):
//   Saturation v1  C' = Y + s (C - Y), s in [0, 1], neutral 1. Luma is kept.
//                  For non-negative input C' is a convex combination of C and
//                  grey: non-negative and never above max(C), so no gamut
//                  excursion. Above-unity saturation is not admitted.
//   Warmth v1      (R (1 + .2 w), G, B (1 - .2 w)), w in [-1, 1], neutral 0.
//                  A modest primary-gain law, not white balance or colour
//                  temperature; it may change overall brightness.
//   Contrast v1    Y' = Y^k, RGB *= Y'/Y for Y > 0, exact black for Y <= 0,
//                  k in [0.25, 3], neutral 1. Hue-preserving luma power; not
//                  device gamma. Output bounded to kAccumulatorComponentMax
//                  per component (counted as overrange).
//   Knee v1        T(x) = x / (1 + x) per component, x >= 0. Off by default;
//                  never enabled automatically to hide an overbright scene.
//                  The endpoint clamp still applies after it.

#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr float kReferenceLumaRed = 0.2126F;
inline constexpr float kReferenceLumaGreen = 0.7152F;
inline constexpr float kReferenceLumaBlue = 0.0722F;

inline constexpr float kSaturationNeutral = 1.0F;
inline constexpr float kWarmthNeutral = 0.0F;
inline constexpr float kContrastNeutral = 1.0F;
inline constexpr float kContrastMinimum = 0.25F;
inline constexpr float kContrastMaximum = 3.0F;
inline constexpr float kWarmthGain = 0.2F;

[[nodiscard]] float referenceLumaV1(const WorkingRgbF32V1& colour) noexcept;

[[nodiscard]] WorkingRgbF32V1 applySaturationV1(const WorkingRgbF32V1& colour,
                                                float saturation) noexcept;
[[nodiscard]] WorkingRgbF32V1 applyWarmthV1(const WorkingRgbF32V1& colour,
                                            float warmth) noexcept;
[[nodiscard]] WorkingRgbF32V1 applyContrastV1(const WorkingRgbF32V1& colour,
                                              float contrast,
                                              StageBoundaryCountersV1& counters) noexcept;
[[nodiscard]] float kneeV1(float value) noexcept;

struct ColourTreatmentControlsV1 final {
  bool saturation_enabled = false;
  float saturation = kSaturationNeutral;
  bool warmth_enabled = false;
  float warmth = kWarmthNeutral;
  bool contrast_enabled = false;
  float contrast = kContrastNeutral;
  bool knee_enabled = false;
};

enum class ColourValidationV1 : std::uint8_t {
  kValid = 0U,
  kSaturation = 1U,
  kWarmth = 2U,
  kContrast = 3U,
};

// Amounts are validated even while their stage is disabled, so enabling a
// stage can never admit a value that was not validated.
[[nodiscard]] ColourValidationV1 validateColourTreatmentV1(
    const ColourTreatmentControlsV1& controls) noexcept;

// True when every stage is disabled or neutral: the whole treatment is then
// an exact identity and the frame is not touched.
[[nodiscard]] bool colourTreatmentIsIdentityV1(
    const ColourTreatmentControlsV1& controls) noexcept;

[[nodiscard]] WorkingRgbF32V1 applyColourTreatmentPixelV1(
    const WorkingRgbF32V1& colour, const ColourTreatmentControlsV1& controls,
    StageBoundaryCountersV1& counters) noexcept;

void applyColourTreatmentV1(WorkingFrame& frame,
                            const ColourTreatmentControlsV1& controls,
                            StageBoundaryCountersV1& counters) noexcept;

}  // namespace k1::core::visual::wide
