#include "core/visual/wide/wide_descriptor.h"

#include "core/visual/product_catalogue.h"
#include "core/visual/wide/wide_bloom.h"
#include "core/visual/wide/wide_endpoint.h"
#include "core/visual/wide/wide_profile.h"

namespace k1::core::visual::wide {
namespace {

using D = ColourDomain;
using S = FeatureScopeV1;
using B = BypassKindV1;
using L = LifecycleKindV1;
using P = ParameterBehaviourV1;

constexpr std::uint32_t kPersistenceBytes =
    static_cast<std::uint32_t>(sizeof(RgbPersistenceStateV1));
constexpr std::uint32_t kPersistenceResets =
    reset_cause::kDisable | reset_cause::kInitialise;
// 32 objects x 80 distances of exp, plus exp2 for retire, raster, and up to
// four spawn-time retire and eviction scans over 32 objects.
constexpr std::uint32_t kAccentTranscendentals =
    static_cast<std::uint32_t>(kPulseCapacity * kPixelsPerHalf) +
    static_cast<std::uint32_t>(kPulseCapacity) * (2U + 2U * 4U);

constexpr std::array<WideFeatureDescriptorV1, kWideFeatureCount> kFeatures{{
    {0x0001U, 1U, "route.bloom_wide", S::kChannel, 1U, D::kLegacyPaletteCodeV1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kResetOnDisable, true,
     static_cast<std::uint32_t>(sizeof(WideRoutedChannelV1)),
     reset_cause::kRouteEntrySeed | reset_cause::kInitialise, 30U},
    {0x0301U, 1U, "palette.window", S::kChannel, 10U, D::kLegacyPaletteCodeV1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kStateless, true, 0U,
     reset_cause::kNone, 0U},
    {0x0302U, 1U, "palette.transition", S::kChannel, 11U,
     D::kLegacyPaletteCodeV1, D::kLegacyPaletteCodeV1, B::kExplicitCut,
     L::kTransitionFromResolved, true,
     static_cast<std::uint32_t>(sizeof(PaletteTransitionStateV1)),
     reset_cause::kInitialise, 0U},
    {0x0101U, 1U, "ribbon.body", S::kLayer, 20U, D::kNone, D::kWorkingRgbF32V1,
     B::kZeroContribution, L::kCutOnMeaningChange, true,
     static_cast<std::uint32_t>(sizeof(HistoryFieldV1)),
     reset_cause::kDisable | reset_cause::kMeaningChange |
         reset_cause::kInitialise,
     1U},
    {0x0201U, 1U, "persistence.body", S::kLayer, 30U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kDeclaredPerInstance, true,
     kPersistenceBytes, kPersistenceResets, 1U},
    {0x0102U, 1U, "ribbon.accent", S::kLayer, 50U, D::kLegacyPaletteCodeV1,
     D::kWorkingRgbF32V1, B::kZeroContribution, L::kReanchorOnEdit, true,
     static_cast<std::uint32_t>(sizeof(PulsePoolV1)),
     reset_cause::kDisable | reset_cause::kInitialise, kAccentTranscendentals},
    {0x0202U, 1U, "persistence.accent", S::kLayer, 60U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kDeclaredPerInstance, true,
     kPersistenceBytes, kPersistenceResets, 1U},
    {0x0103U, 1U, "ribbon.atmosphere", S::kLayer, 80U, D::kLegacyPaletteCodeV1,
     D::kWorkingRgbF32V1, B::kZeroContribution, L::kStateless, true, 0U,
     reset_cause::kNone, 0U},
    {0x0203U, 1U, "persistence.atmosphere", S::kLayer, 81U,
     D::kWorkingRgbF32V1, D::kWorkingRgbF32V1, B::kIdentity,
     L::kDeclaredPerInstance, true, kPersistenceBytes, kPersistenceResets, 1U},
    {0x0401U, 1U, "colour.saturation", S::kChannel, 100U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kStateless, true, 0U,
     reset_cause::kNone, 0U},
    {0x0402U, 1U, "colour.warmth", S::kChannel, 101U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kStateless, true, 0U,
     reset_cause::kNone, 0U},
    {0x0403U, 1U, "colour.contrast", S::kChannel, 102U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kStateless, true, 0U,
     reset_cause::kNone, static_cast<std::uint32_t>(kPixelsPerChannel)},
    {0x0404U, 1U, "colour.knee", S::kChannel, 103U, D::kWorkingRgbF32V1,
     D::kWorkingRgbF32V1, B::kIdentity, L::kStateless, true, 0U,
     reset_cause::kNone, 0U},
    {0x0501U, 1U, "endpoint.blackout", S::kPhysicalOutput, 109U,
     D::kWorkingRgbF32V1, D::kDeviceRgb16V1, B::kNotBypassable, L::kStateless,
     false, 0U, reset_cause::kNone, 0U},
    {0x0502U, 1U, "endpoint.transfer", S::kPhysicalOutput, 110U,
     D::kWorkingRgbF32V1, D::kDeviceDriveRgbF32V1, B::kNotBypassable,
     L::kStateless, false, 0U, reset_cause::kNone, 0U},
    {0x0503U, 1U, "endpoint.current_limit", S::kPhysicalOutput, 111U,
     D::kDeviceDriveRgbF32V1, D::kDeviceDriveRgbF32V1, B::kNotBypassable,
     L::kStateless, false, 0U, reset_cause::kNone, 0U},
    {0x0504U, 1U, "endpoint.quantise16", S::kPhysicalOutput, 112U,
     D::kDeviceDriveRgbF32V1, D::kDeviceRgb16V1, B::kNotBypassable,
     L::kStateless, false, 0U, reset_cause::kNone, 0U},
    {0x0601U, 1U, "compat.rgb8_rounded", S::kEvaluationOutput, 120U,
     D::kDeviceRgb16V1, D::kCompatRgb8V1, B::kNotBypassable, L::kStateless,
     false, 0U, reset_cause::kNone, 0U},
    {0x0602U, 1U, "compat.rgb8_dithered", S::kEvaluationOutput, 121U,
     D::kDeviceRgb16V1, D::kCompatRgb8V1, B::kNotBypassable,
     L::kAdvanceOnPresentation, false,
     static_cast<std::uint32_t>(sizeof(Rgb8DitherStateV1)),
     reset_cause::kInitialise, 0U},
}};

constexpr float kLastPalette =
    static_cast<float>(kProductPaletteCount - 1U);

constexpr std::array<WideParameterDescriptorV1, kWideParameterCount> kParameters{{
    {0x1001U, 0x0101U, "body.level", "", 0.0F, 4.0F, 1.0F, false, 0.0F,
     P::kImmediate},
    {0x1002U, 0x0101U, "body.motion", "px/s", 0.0F, 240.0F, 80.0F, false, 0.0F,
     P::kImmediate},
    {0x1003U, 0x0101U, "body.half_life", "s", 0.02F, 4.0F, 0.35F, false, 0.0F,
     P::kImmediate},
    {0x1004U, 0x0101U, "body.meaning", "enum", 1.0F, 2.0F, 1.0F, false, 0.0F,
     P::kCutSelector},
    {0x1101U, 0x0102U, "accent.rhythm_mix", "", 0.0F, 1.0F, 0.4F, false, 0.0F,
     P::kImmediate},
    {0x1102U, 0x0102U, "accent.velocity", "px/s", 0.0F, 480.0F, 80.0F, false,
     0.0F, P::kReanchored},
    {0x1103U, 0x0102U, "accent.diffusion", "px^2/s", 0.0F, 1024.0F, 2.0F,
     false, 0.0F, P::kReanchored},
    {0x1104U, 0x0102U, "accent.half_life", "s", 0.02F, 60.0F, 0.35F, false,
     0.0F, P::kReanchored},
    {0x1105U, 0x0102U, "accent.width", "px", 0.25F, 24.0F, 2.0F, false, 0.0F,
     P::kFutureBirths},
    {0x1201U, 0x0103U, "atmosphere.level", "", 0.0F, 0.25F, 0.04F, false, 0.0F,
     P::kImmediate},
    {0x2001U, 0x0201U, "persistence.body.amount", "", 0.0F, 1.0F, 1.0F, true,
     0.0F, P::kImmediate},
    {0x2002U, 0x0201U, "persistence.body.half_life", "s", 0.02F, 4.0F, 0.35F,
     false, 0.0F, P::kImmediate},
    {0x2011U, 0x0202U, "persistence.accent.amount", "", 0.0F, 1.0F, 1.0F, true,
     0.0F, P::kImmediate},
    {0x2012U, 0x0202U, "persistence.accent.half_life", "s", 0.02F, 4.0F, 0.35F,
     false, 0.0F, P::kImmediate},
    {0x2021U, 0x0203U, "persistence.atmosphere.amount", "", 0.0F, 1.0F, 1.0F,
     true, 0.0F, P::kImmediate},
    {0x2022U, 0x0203U, "persistence.atmosphere.half_life", "s", 0.02F, 4.0F,
     0.35F, false, 0.0F, P::kImmediate},
    {0x3001U, 0x0301U, "palette.position", "", 0.0F, 1.0F, 0.3F, false, 0.0F,
     P::kImmediate},
    {0x3002U, 0x0301U, "palette.span", "", 0.0F, 1.0F, 0.7F, false, 0.0F,
     P::kImmediate},
    {0x3003U, 0x0301U, "palette.travel_depth", "", 0.0F, 1.0F, 0.2F, true,
     0.0F, P::kImmediate},
    {0x3004U, 0x0302U, "palette.transition", "s", 0.0F, 5.0F, 0.5F, false,
     0.0F, P::kTransition},
    {0x3005U, 0x0302U, "palette.id", "enum", 0.0F, kLastPalette, 0.0F, false,
     0.0F, P::kTransition},
    {0x4001U, 0x0401U, "colour.saturation", "", 0.0F, 1.0F, 1.0F, true, 1.0F,
     P::kImmediate},
    {0x4002U, 0x0402U, "colour.warmth", "", -1.0F, 1.0F, 0.0F, true, 0.0F,
     P::kImmediate},
    {0x4003U, 0x0403U, "colour.contrast", "", 0.25F, 3.0F, 1.0F, true, 1.0F,
     P::kImmediate},
    {0x5001U, 0x0502U, "endpoint.master", "", 0.0F, 1.0F, 1.0F, false, 0.0F,
     P::kImmediate},
    {0x5002U, 0x0502U, "endpoint.intensity", "", 0.0F, 1.0F, 1.0F, false,
     0.0F, P::kImmediate},
    {0x5003U, 0x0503U, "endpoint.max_current", "mA", 100.0F, 1.0e6F, 5000.0F,
     false, 0.0F, P::kImmediate},
    {0x5004U, 0x0503U, "endpoint.full_component_current", "mA", 0.001F,
     1000.0F, 20.0F, false, 0.0F, P::kImmediate},
}};

}  // namespace

const std::array<WideFeatureDescriptorV1, kWideFeatureCount>&
wideFeatureDescriptorsV1() noexcept {
  return kFeatures;
}

const std::array<WideParameterDescriptorV1, kWideParameterCount>&
wideParameterDescriptorsV1() noexcept {
  return kParameters;
}

const WideFeatureDescriptorV1* findWideFeatureV1(
    const std::uint16_t feature_id) noexcept {
  for (const WideFeatureDescriptorV1& feature : kFeatures) {
    if (feature.feature_id == feature_id) {
      return &feature;
    }
  }
  return nullptr;
}

const WideParameterDescriptorV1* findWideParameterV1(
    const std::uint16_t parameter_id) noexcept {
  for (const WideParameterDescriptorV1& parameter : kParameters) {
    if (parameter.parameter_id == parameter_id) {
      return &parameter;
    }
  }
  return nullptr;
}

}  // namespace k1::core::visual::wide
