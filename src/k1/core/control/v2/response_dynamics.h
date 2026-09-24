// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Engine-owned response dynamics (accepted base -> effective control value).
//
// Laws, all in elapsed engine time (never frame or packet counts):
//   direct       y = target
//   exponential  y += -expm1(-dt / tau) * (target - y)
//   slew         y moves toward target by at most rate * dt (canonical
//                units per second); never overshoots
// The state is evaluated in closed form from the last retarget anchor
// (y0, t0): y(t) = y0 + (-expm1(-(t - t0) / tau)) * (target - y0), which is the
// incremental law applied over dt = t - t0. Evaluation cadence therefore
// cannot change the trajectory, rounding never accumulates, and a long
// scheduling gap needs no dt clamp. A retarget re-anchors at the value the
// old trajectory has reached. Within the terminal snap epsilon the value
// equals the target exactly. Finite-duration easing is not offered.
#pragma once

#include <cstdint>

#include "contract/control_v2/control_v2_objects.h"

namespace k1::core::control::v2 {

namespace cv2 = ::k1::control_v2;

struct ResponseLaw final {
  cv2::ResponseKind kind{cv2::ResponseKind::kDirect};
  double tau_s{0.0};             // exponential time constant
  double rate_per_s{0.0};        // slew rate, canonical units per second
  double snap_epsilon{0.0};      // terminal snap, canonical units
};

// Resolves an admitted policy id for a descriptor (0 = descriptor default =
// direct; immediate descriptors are always direct). Slew rates in the
// catalogue are fractions of the legal span per second.
[[nodiscard]] ResponseLaw resolveResponse(std::uint8_t policy_id,
                                          const cv2::ParameterDescriptor& descriptor) noexcept;

struct ResponseState final {
  double anchor_value{0.0};
  double target{0.0};
  std::uint64_t anchor_time_us{0U};
  ResponseLaw law{};
};

void responseReset(ResponseState& state, double value, std::uint64_t now_us) noexcept;
// New target at now_us: re-anchors at the current trajectory value.
void responseRetarget(ResponseState& state, double target, const ResponseLaw& law,
                      std::uint64_t now_us) noexcept;
// Effective value at now_us (times before the anchor evaluate at the anchor).
[[nodiscard]] double responseEvaluate(const ResponseState& state, std::uint64_t now_us) noexcept;
[[nodiscard]] bool responseSettled(const ResponseState& state, std::uint64_t now_us) noexcept;

// The incremental primitive, for consumers that step explicitly.
[[nodiscard]] double stepExponential(double y, double target, double dt_s, double tau_s) noexcept;
[[nodiscard]] double stepSlew(double y, double target, double dt_s, double rate_per_s) noexcept;

}  // namespace k1::core::control::v2
