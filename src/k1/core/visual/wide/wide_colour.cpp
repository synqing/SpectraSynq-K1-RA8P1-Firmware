#include "core/visual/wide/wide_colour.h"

#include <cmath>

namespace k1::core::visual::wide {
namespace {

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

bool saturationActive(const ColourTreatmentControlsV1& controls) noexcept {
  return controls.saturation_enabled &&
         controls.saturation != kSaturationNeutral;
}

bool warmthActive(const ColourTreatmentControlsV1& controls) noexcept {
  return controls.warmth_enabled && controls.warmth != kWarmthNeutral;
}

bool contrastActive(const ColourTreatmentControlsV1& controls) noexcept {
  return controls.contrast_enabled && controls.contrast != kContrastNeutral;
}

float boundedComponent(const float value,
                       StageBoundaryCountersV1& counters) noexcept {
  return sanitiseComponent(value, kAccumulatorComponentMax, counters);
}

}  // namespace

float referenceLumaV1(const WorkingRgbF32V1& colour) noexcept {
  return kReferenceLumaRed * colour.red + kReferenceLumaGreen * colour.green +
         kReferenceLumaBlue * colour.blue;
}

WorkingRgbF32V1 applySaturationV1(const WorkingRgbF32V1& colour,
                                  const float saturation) noexcept {
  if (saturation == kSaturationNeutral) {
    return colour;
  }
  const float luma = referenceLumaV1(colour);
  return {luma + saturation * (colour.red - luma),
          luma + saturation * (colour.green - luma),
          luma + saturation * (colour.blue - luma)};
}

WorkingRgbF32V1 applyWarmthV1(const WorkingRgbF32V1& colour,
                              const float warmth) noexcept {
  if (warmth == kWarmthNeutral) {
    return colour;
  }
  return {colour.red * (1.0F + kWarmthGain * warmth), colour.green,
          colour.blue * (1.0F - kWarmthGain * warmth)};
}

WorkingRgbF32V1 applyContrastV1(const WorkingRgbF32V1& colour,
                                const float contrast,
                                StageBoundaryCountersV1& counters) noexcept {
  if (contrast == kContrastNeutral) {
    return colour;
  }
  const float luma = referenceLumaV1(colour);
  if (!(luma > 0.0F)) {
    return {};
  }
  // Y' / Y = Y^(k - 1). With non-negative components each is at most
  // Y / .0722, so every product is bounded by Y^k / .0722 and stays finite.
  const float scale = std::pow(luma, contrast - 1.0F);
  return {boundedComponent(colour.red * scale, counters),
          boundedComponent(colour.green * scale, counters),
          boundedComponent(colour.blue * scale, counters)};
}

float kneeV1(const float value) noexcept {
  if (!(value > 0.0F)) {
    return 0.0F;
  }
  return value / (1.0F + value);
}

ColourValidationV1 validateColourTreatmentV1(
    const ColourTreatmentControlsV1& controls) noexcept {
  if (!finiteIn(controls.saturation, 0.0F, 1.0F)) {
    return ColourValidationV1::kSaturation;
  }
  if (!finiteIn(controls.warmth, -1.0F, 1.0F)) {
    return ColourValidationV1::kWarmth;
  }
  if (!finiteIn(controls.contrast, kContrastMinimum, kContrastMaximum)) {
    return ColourValidationV1::kContrast;
  }
  return ColourValidationV1::kValid;
}

bool colourTreatmentIsIdentityV1(
    const ColourTreatmentControlsV1& controls) noexcept {
  return !saturationActive(controls) && !warmthActive(controls) &&
         !contrastActive(controls) && !controls.knee_enabled;
}

WorkingRgbF32V1 applyColourTreatmentPixelV1(
    const WorkingRgbF32V1& colour, const ColourTreatmentControlsV1& controls,
    StageBoundaryCountersV1& counters) noexcept {
  WorkingRgbF32V1 result = colour;
  if (saturationActive(controls)) {
    result = applySaturationV1(result, controls.saturation);
  }
  if (warmthActive(controls)) {
    result = applyWarmthV1(result, controls.warmth);
  }
  if (contrastActive(controls)) {
    result = applyContrastV1(result, controls.contrast, counters);
  }
  if (controls.knee_enabled) {
    result = {kneeV1(result.red), kneeV1(result.green), kneeV1(result.blue)};
  }
  return result;
}

void applyColourTreatmentV1(WorkingFrame& frame,
                            const ColourTreatmentControlsV1& controls,
                            StageBoundaryCountersV1& counters) noexcept {
  if (colourTreatmentIsIdentityV1(controls)) {
    return;
  }
  for (WorkingRgbF32V1& pixel : frame) {
    pixel = applyColourTreatmentPixelV1(pixel, controls, counters);
  }
}

}  // namespace k1::core::visual::wide
