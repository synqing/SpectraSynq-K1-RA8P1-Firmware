#pragma once

// Temporal stage primitives for the wide path.
//
// Time: presentation timestamps stay integer microseconds. Only bounded local
// differences are converted to float seconds; a growing absolute timestamp is
// never converted to float. Timestamp 0 is reserved ("no time") and rejected;
// a timestamp earlier than the last accepted one is rejected; neither mutates
// time-dependent state. The first accepted timestamp starts the timeline with
// dt = 0. Long gaps are integrated with their real elapsed time (analytic
// retention), never clamped to a frame-sized step.
//
// Immediate-attack persistence (SuperPixie-style), per component:
//   p(t + dt) = max(input(t + dt), p(t) * 2^(-dt / h))
// The output is p itself (never input + p, which would double the attack).
// wet amount m in [0, 1]: output = input + m (p - input); m = 0 returns the
// dry input exactly while history keeps tracking (enable is independent of
// amount). Disabled: output is the dry input exactly (hard bypass).
// Lifecycle on disable (declared per instance, default reset):
//   kResetOnDisable     history cleared at the disable edge; re-enable seeds
//                       from the current dry input, so no stale light returns
//   kDecayWithoutDrive  history keeps ageing with zero drive while disabled
//   kFreeze             history held untouched (explicit opt-in only: it can
//                       resurrect old light at its former strength)
// The v1 disable ramp is zero seconds. RGB and scalar persistence are
// separately named operators: componentwise RGB max can combine hues from
// different times; scalar persistence holds intensity and is recoloured by
// its owner at read time.

#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

inline constexpr float kPersistenceHalfLifeMinimumS = 0.02F;
inline constexpr float kPersistenceHalfLifeMaximumS = 4.0F;

// 2^(-dt / h); dt = 0 -> exactly 1. Underflow yields exact 0.
[[nodiscard]] float halfLifeRetentionV1(float delta_seconds,
                                        float half_life_s) noexcept;

// First-order follower coefficient -expm1(-dt / tau) for smoothed envelopes.
[[nodiscard]] float smoothingAlphaV1(float delta_seconds, float tau_s) noexcept;

// (to - from) in seconds; 0 when to <= from.
[[nodiscard]] float secondsBetweenV1(std::uint64_t from_us,
                                     std::uint64_t to_us) noexcept;

enum class TimeStepResultV1 : std::uint8_t {
  kFirst = 0U,
  kAdvanced = 1U,
  kRepeated = 2U,
  kRejectedZero = 3U,
  kRejectedBackwards = 4U,
};

struct TimelineV1 final {
  std::uint64_t last_us = 0U;
  bool started = false;
  std::uint32_t rejected_zero = 0U;
  std::uint32_t rejected_backwards = 0U;
};

struct TimeStepV1 final {
  TimeStepResultV1 result = TimeStepResultV1::kFirst;
  float delta_seconds = 0.0F;
  [[nodiscard]] bool accepted() const noexcept {
    return result == TimeStepResultV1::kFirst ||
           result == TimeStepResultV1::kAdvanced ||
           result == TimeStepResultV1::kRepeated;
  }
};

TimeStepV1 advanceTimelineV1(TimelineV1& timeline,
                             std::uint64_t presentation_us) noexcept;

enum class PersistenceLifecycleV1 : std::uint8_t {
  kResetOnDisable = 0U,
  kDecayWithoutDrive = 1U,
  kFreeze = 2U,
};

struct PersistenceControlsV1 final {
  bool enabled = false;
  float amount = 1.0F;       // wet mix, [0, 1]
  float half_life_s = 0.35F; // [0.02, 4] s
  PersistenceLifecycleV1 lifecycle = PersistenceLifecycleV1::kResetOnDisable;
};

enum class PersistenceValidationV1 : std::uint8_t {
  kValid = 0U,
  kAmount = 1U,
  kHalfLife = 2U,
  kLifecycle = 3U,
};

[[nodiscard]] PersistenceValidationV1 validatePersistenceV1(
    const PersistenceControlsV1& controls) noexcept;

struct PersistenceLifecycleCountersV1 final {
  std::uint32_t enables = 0U;
  std::uint32_t disables = 0U;
  std::uint32_t resets = 0U;
};

struct RgbPersistenceStateV1 final {
  WorkingFrame held{};
  bool enabled_last = false;
  PersistenceLifecycleCountersV1 counters{};
};

struct ScalarPersistenceStateV1 final {
  ScalarFrame held{};
  bool enabled_last = false;
  PersistenceLifecycleCountersV1 counters{};
};

// `layer` holds the dry input on entry and the stage output on exit.
void applyRgbPersistenceV1(WorkingFrame& layer, RgbPersistenceStateV1& state,
                           const PersistenceControlsV1& controls,
                           float delta_seconds) noexcept;
void applyScalarPersistenceV1(ScalarFrame& layer,
                              ScalarPersistenceStateV1& state,
                              const PersistenceControlsV1& controls,
                              float delta_seconds) noexcept;

// Single-pixel forms of the same laws (used by fixtures and adapters).
[[nodiscard]] WorkingRgbF32V1 rgbPeakHoldStepV1(const WorkingRgbF32V1& input,
                                                const WorkingRgbF32V1& held,
                                                float retention) noexcept;
[[nodiscard]] float scalarPeakHoldStepV1(float input, float held,
                                         float retention) noexcept;

}  // namespace k1::core::visual::wide
