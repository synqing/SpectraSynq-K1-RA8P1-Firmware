#pragma once

// MOD implementation of CTL's ModulatorHook seam (core/control/v2). The engine
// hands the hook a target's effective (response-shaped) value together with
// the channel's Liveiness base, Liveiness enable and mode base; the hook
// returns a material value and never sees or writes a base.
//
// Affected target: Motion, for modes whose registered Liveiness dimension is
// motion-like (transport rate, object velocity, pattern flow rate). Modes
// whose dimension is excursion or spawn reach (Snapwave 22, Anticipate 27,
// Dense Forge 21/24 v4) modulate a mode-internal quantity only, so no
// control target is modulated for them. Unsupported/pending modes and a
// disabled input produce no material.
//
// Law: the same resolveLiveiness() law and per-mode registered gain k as the
// renderer hook liveinessEffective(). The renderer-unit bounds of the mode
// descriptor do not apply to control units; the engine validates material
// against the target's own domain. Neutral (and the neutral snap) is exact
// identity: material == effective, bit for bit.

#include <cstdint>
#include <limits>

#include "contract/control_v2/generated/control_registry_v2.generated.h"
#include "core/control/v2/control_engine.h"
#include "core/visual/modes/liveiness_registry.h"

namespace k1::core::visual::modes {

inline constexpr std::uint8_t kLiveinessControlHookVersion =
    static_cast<std::uint8_t>(kLiveinessContractVersion);

[[nodiscard]] constexpr bool liveinessModulatesMotion(
    const LiveinessDimension dimension) noexcept {
  return dimension == LiveinessDimension::kTransportRate ||
         dimension == LiveinessDimension::kObjectVelocity ||
         dimension == LiveinessDimension::kPatternFlowRate;
}

inline bool liveinessModulateControl(
    void* /*context*/, const control::v2::ModulationInput& input,
    control_v2::TypedValue& material) noexcept {
  if (input.target.semantic_id != control_v2::generated::kMotion ||
      input.effective.kind != control_v2::ValueKind::kReal) {
    return false;
  }
  const LiveinessAdapterDescriptor& mode = liveinessAdapter(input.mode_id);
  if (mode.support != LiveinessSupport::kSupported ||
      !liveinessModulatesMotion(mode.dimension)) {
    return false;
  }
  LiveinessAdapterDescriptor law = mode;
  law.minimum = 0.0F;
  law.maximum = std::numeric_limits<float>::max();
  const LiveinessResult result = resolveLiveiness(
      law, input.effective.asReal(),
      LiveinessInput{input.liveiness, input.liveiness_enabled});
  if (result.reason == LiveinessReason::kUnsupportedMode ||
      result.reason == LiveinessReason::kDisabled ||
      result.reason == LiveinessReason::kInvalidAmount ||
      result.reason == LiveinessReason::kInvalidBase) {
    return false;
  }
  if (!(result.factor != 1.0F)) {
    material = input.effective;  // exact identity at neutral
    return true;
  }
  material = control_v2::TypedValue::real(result.effective);
  return true;
}

inline const control::v2::ModulatorHook kLiveinessControlHook{
    kLiveinessControlHookVersion, &liveinessModulateControl, nullptr};

}  // namespace k1::core::visual::modes
