#include "core/visual/wide/wide_temporal.h"

#include <cmath>

namespace k1::core::visual::wide {
namespace {

bool finiteIn(const float value, const float low, const float high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

float maxOf(const float left, const float right) noexcept {
  return left > right ? left : right;
}

float wet(const float dry, const float held, const float amount) noexcept {
  return dry + amount * (held - dry);
}

// Disable edge: apply the declared lifecycle to the owned history only. A
// cleared history makes the next enable seed from the current dry input.
template <typename Frame>
void handleDisableEdge(Frame& held, PersistenceLifecycleCountersV1& counters,
                       const PersistenceControlsV1& controls) noexcept {
  ++counters.disables;
  if (controls.lifecycle == PersistenceLifecycleV1::kResetOnDisable) {
    held = Frame{};
    ++counters.resets;
  }
}

}  // namespace

float halfLifeRetentionV1(const float delta_seconds,
                          const float half_life_s) noexcept {
  if (!(delta_seconds > 0.0F)) {
    return 1.0F;
  }
  if (!(half_life_s > 0.0F) || !std::isfinite(delta_seconds)) {
    return 0.0F;
  }
  return std::exp2(-delta_seconds / half_life_s);
}

float smoothingAlphaV1(const float delta_seconds, const float tau_s) noexcept {
  if (!(delta_seconds > 0.0F)) {
    return 0.0F;
  }
  if (!(tau_s > 0.0F) || !std::isfinite(delta_seconds)) {
    return 1.0F;
  }
  return -std::expm1(-delta_seconds / tau_s);
}

float secondsBetweenV1(const std::uint64_t from_us,
                       const std::uint64_t to_us) noexcept {
  if (to_us <= from_us) {
    return 0.0F;
  }
  return static_cast<float>(to_us - from_us) / 1000000.0F;
}

TimeStepV1 advanceTimelineV1(TimelineV1& timeline,
                             const std::uint64_t presentation_us) noexcept {
  TimeStepV1 step{};
  if (presentation_us == 0U) {
    ++timeline.rejected_zero;
    step.result = TimeStepResultV1::kRejectedZero;
    return step;
  }
  if (!timeline.started) {
    timeline.started = true;
    timeline.last_us = presentation_us;
    step.result = TimeStepResultV1::kFirst;
    return step;
  }
  if (presentation_us < timeline.last_us) {
    ++timeline.rejected_backwards;
    step.result = TimeStepResultV1::kRejectedBackwards;
    return step;
  }
  if (presentation_us == timeline.last_us) {
    step.result = TimeStepResultV1::kRepeated;
    return step;
  }
  step.result = TimeStepResultV1::kAdvanced;
  step.delta_seconds = secondsBetweenV1(timeline.last_us, presentation_us);
  timeline.last_us = presentation_us;
  return step;
}

PersistenceValidationV1 validatePersistenceV1(
    const PersistenceControlsV1& controls) noexcept {
  if (!finiteIn(controls.amount, 0.0F, 1.0F)) {
    return PersistenceValidationV1::kAmount;
  }
  if (!finiteIn(controls.half_life_s, kPersistenceHalfLifeMinimumS,
                kPersistenceHalfLifeMaximumS)) {
    return PersistenceValidationV1::kHalfLife;
  }
  if (controls.lifecycle != PersistenceLifecycleV1::kResetOnDisable &&
      controls.lifecycle != PersistenceLifecycleV1::kDecayWithoutDrive &&
      controls.lifecycle != PersistenceLifecycleV1::kFreeze) {
    return PersistenceValidationV1::kLifecycle;
  }
  return PersistenceValidationV1::kValid;
}

WorkingRgbF32V1 rgbPeakHoldStepV1(const WorkingRgbF32V1& input,
                                  const WorkingRgbF32V1& held,
                                  const float retention) noexcept {
  return {maxOf(input.red, held.red * retention),
          maxOf(input.green, held.green * retention),
          maxOf(input.blue, held.blue * retention)};
}

float scalarPeakHoldStepV1(const float input, const float held,
                           const float retention) noexcept {
  return maxOf(input, held * retention);
}

void applyRgbPersistenceV1(WorkingFrame& layer, RgbPersistenceStateV1& state,
                           const PersistenceControlsV1& controls,
                           const float delta_seconds) noexcept {
  const float retention =
      halfLifeRetentionV1(delta_seconds, controls.half_life_s);
  if (!controls.enabled) {
    if (state.enabled_last) {
      handleDisableEdge(state.held, state.counters, controls);
    }
    state.enabled_last = false;
    if (controls.lifecycle == PersistenceLifecycleV1::kDecayWithoutDrive) {
      for (WorkingRgbF32V1& held : state.held) {
        held = {held.red * retention, held.green * retention,
                held.blue * retention};
      }
    }
    return;  // exact dry bypass: `layer` untouched
  }
  if (!state.enabled_last) {
    ++state.counters.enables;
  }
  state.enabled_last = true;
  const bool dry_only = controls.amount == 0.0F;
  const bool full_wet = controls.amount == 1.0F;
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const WorkingRgbF32V1 dry = layer[index];
    const WorkingRgbF32V1 held =
        rgbPeakHoldStepV1(dry, state.held[index], retention);
    state.held[index] = held;
    if (dry_only) {
      continue;
    }
    layer[index] = full_wet
                       ? held
                       : WorkingRgbF32V1{wet(dry.red, held.red, controls.amount),
                                         wet(dry.green, held.green,
                                             controls.amount),
                                         wet(dry.blue, held.blue,
                                             controls.amount)};
  }
}

void applyScalarPersistenceV1(ScalarFrame& layer,
                              ScalarPersistenceStateV1& state,
                              const PersistenceControlsV1& controls,
                              const float delta_seconds) noexcept {
  const float retention =
      halfLifeRetentionV1(delta_seconds, controls.half_life_s);
  if (!controls.enabled) {
    if (state.enabled_last) {
      handleDisableEdge(state.held, state.counters, controls);
    }
    state.enabled_last = false;
    if (controls.lifecycle == PersistenceLifecycleV1::kDecayWithoutDrive) {
      for (float& held : state.held) {
        held *= retention;
      }
    }
    return;
  }
  if (!state.enabled_last) {
    ++state.counters.enables;
  }
  state.enabled_last = true;
  const bool dry_only = controls.amount == 0.0F;
  const bool full_wet = controls.amount == 1.0F;
  for (std::size_t index = 0U; index < kPixelsPerChannel; ++index) {
    const float dry = layer[index];
    const float held = scalarPeakHoldStepV1(dry, state.held[index], retention);
    state.held[index] = held;
    if (!dry_only) {
      layer[index] = full_wet ? held : wet(dry, held, controls.amount);
    }
  }
}

}  // namespace k1::core::visual::wide
