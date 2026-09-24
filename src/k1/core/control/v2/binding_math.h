// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Pure, allocation-free binding transforms for the K1 control contract v2.
//
// Absolute law: calibration -> effective input limit -> movement
// qualification (hook; INP owns the session state machine) -> captured
// binding / takeover -> monotonic range mapping (inversion, legal positive log
// domains) -> canonical stepping (exact configured endpoints) -> typed intent.
// Relative law (touch drag, encoder, trim): bounded delta integration with a
// gain curve against velocity, a maximum rate and an endpoint clamp. Absolute
// mappings carry no velocity input at all: acceleration cannot reach them.
//
// Arithmetic is double precision; outputs are canonical binary32/int32 values.
#pragma once

#include <cstdint>

#include "contract/control_v2/control_v2_objects.h"

namespace k1::core::control::v2 {

namespace cv2 = ::k1::control_v2;

// --- calibration and input limit ------------------------------------------

struct Calibration final {
  std::uint16_t raw_min{0U};
  std::uint16_t raw_max{4095U};
};
// Normalised physical position in [0, 1]; false if the calibration is invalid.
[[nodiscard]] bool calibrate(std::uint16_t raw, const Calibration& calibration, double& out) noexcept;
// Effective input resolution: round(x * (steps - 1)) / (steps - 1).
[[nodiscard]] double limitInput(double x01, std::uint32_t steps) noexcept;

// --- movement qualification hook -------------------------------------------

// INP supplies the qualifier (and its quiet/rearm session boundary). The
// default is the binding's hysteresis: endpoints always qualify, otherwise
// the move must reach the hysteresis from the last accepted position.
using MovementQualifier = bool (*)(void* context, double last_accepted01, double x01,
                                   const cv2::CompiledBinding& binding);
[[nodiscard]] bool hysteresisQualifier(void* context, double last_accepted01, double x01,
                                       const cv2::CompiledBinding& binding) noexcept;

// --- takeover -----------------------------------------------------------------

enum class PickupState : std::uint8_t {
  kLatched = 0U,             // physical path crossed (or is within tolerance of) the base
  kWaiting = 1U,             // not yet crossed: no value is produced
  kTargetOutsideRange = 2U,  // base lies outside the binding range: never clamped silently
};
// Segment-crossing test so a fast move that skips over the base still latches.
[[nodiscard]] PickupState pickupCheck(double previous01, double current01, double base01,
                                      float tolerance, bool base_in_range) noexcept;

struct RangeTakeover final {
  bool pickup_fallback{false};  // anchored at a physical endpoint: degenerate side
  double anchor_physical{0.0};
  double anchor_target01{0.0};
};
[[nodiscard]] RangeTakeover rangeTakeoverArm(double physical01, double base01) noexcept;
// Piecewise map anchoring the accepted base at the current position; reaches 0 and 1.
[[nodiscard]] double rangeTakeoverMap(const RangeTakeover& takeover, double x01) noexcept;

// --- mapping and stepping -----------------------------------------------------

// Normalised travel (after inversion) to canonical units; exact endpoints.
[[nodiscard]] double mapForward(const cv2::CompiledBinding& binding, double x01) noexcept;
// Canonical value (clamped to the binding range) back to travel in [0, 1].
[[nodiscard]] double mapInverse(const cv2::CompiledBinding& binding, double value) noexcept;
// Nearest point of {min, min + k*step, ..., max}: max is always a grid point,
// so both configured endpoints stay reachable when the span is not a multiple
// of the step. Monotonic; step 0 keeps the continuous value.
[[nodiscard]] cv2::TypedValue quantise(const cv2::CompiledBinding& binding, double value) noexcept;

struct AbsoluteResult final {
  bool ok{false};
  cv2::TypedValue value{};
};
// Full absolute law from a normalised, already qualified position.
[[nodiscard]] AbsoluteResult evaluateAbsolute(const cv2::CompiledBinding& binding, double x01) noexcept;

// --- relative deltas ------------------------------------------------------------

struct RelativeResult final {
  bool ok{false};
  double new_estimate{0.0};    // clamped, stepped canonical value after this delta
  cv2::TypedValue delta{};     // exact delta to send (new_estimate - current)
  double multiplier{1.0};      // acceleration multiplier actually applied
};
// input_delta: signed normalised travel since the previous event; dt_s: event
// spacing (> 0). gain * span per unit travel, multiplier = clamp((v/v_ref)^k,
// 1, max_multiplier), |delta| <= max_rate * dt, result clamped to the range.
[[nodiscard]] RelativeResult evaluateRelative(const cv2::CompiledBinding& binding, double input_delta,
                                              double dt_s, double current_estimate) noexcept;

}  // namespace k1::core::control::v2
