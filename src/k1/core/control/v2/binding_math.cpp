// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/binding_math.h"

#include <cmath>
#include <cstring>

namespace k1::core::control::v2 {
namespace {

bool finiteDouble(const double v) noexcept {
  std::uint64_t bits = 0U;
  std::memcpy(&bits, &v, sizeof(bits));
  return (bits & 0x7FF0000000000000ULL) != 0x7FF0000000000000ULL;
}

double clamp(const double x, const double lo, const double hi) noexcept {
  return x < lo ? lo : (x > hi ? hi : x);
}

bool inverted(const cv2::CompiledBinding& b) noexcept {
  return (b.flags & cv2::kBindingFlagInverted) != 0U;
}

}  // namespace

bool calibrate(const std::uint16_t raw, const Calibration& c, double& out) noexcept {
  if (c.raw_max <= c.raw_min) {
    return false;
  }
  const double x = (static_cast<double>(raw) - c.raw_min) / (static_cast<double>(c.raw_max) - c.raw_min);
  out = clamp(x, 0.0, 1.0);
  return true;
}

double limitInput(const double x01, const std::uint32_t steps) noexcept {
  if (!finiteDouble(x01) || steps < 2U) {
    return x01;
  }
  const double top = static_cast<double>(steps - 1U);
  return std::round(clamp(x01, 0.0, 1.0) * top) / top;
}

bool hysteresisQualifier(void*, const double last, const double x,
                         const cv2::CompiledBinding& binding) noexcept {
  if (!finiteDouble(x)) {
    return false;
  }
  if (x <= 0.0 || x >= 1.0) {
    return true;  // endpoints always deliver, so exact limits stay reachable
  }
  return std::fabs(x - last) >= static_cast<double>(binding.hysteresis);
}

PickupState pickupCheck(const double previous, const double current, const double base,
                        const float tolerance, const bool base_in_range) noexcept {
  if (!base_in_range) {
    return PickupState::kTargetOutsideRange;
  }
  const bool crossed = (previous - base) * (current - base) <= 0.0;
  if (crossed || std::fabs(current - base) <= static_cast<double>(tolerance)) {
    return PickupState::kLatched;
  }
  return PickupState::kWaiting;
}

RangeTakeover rangeTakeoverArm(const double physical, const double base01) noexcept {
  RangeTakeover t{};
  t.anchor_physical = clamp(physical, 0.0, 1.0);
  t.anchor_target01 = clamp(base01, 0.0, 1.0);
  t.pickup_fallback = t.anchor_physical <= 0.0 || t.anchor_physical >= 1.0;
  return t;
}

double rangeTakeoverMap(const RangeTakeover& t, const double x01) noexcept {
  const double x = clamp(x01, 0.0, 1.0);
  const double a = t.anchor_physical;
  const double b = t.anchor_target01;
  if (x <= a) {
    return a > 0.0 ? b * x / a : b;
  }
  return b + (1.0 - b) * (x - a) / (1.0 - a);
}

double mapForward(const cv2::CompiledBinding& b, const double x01) noexcept {
  double x = clamp(x01, 0.0, 1.0);
  if (inverted(b)) {
    x = 1.0 - x;
  }
  const double lo = b.range_min;
  const double hi = b.range_max;
  if (x <= 0.0) {
    return lo;
  }
  if (x >= 1.0) {
    return hi;
  }
  switch (b.mapping_kind) {
    case cv2::MappingKind::kLog:
      return lo * std::exp(x * std::log(hi / lo));
    case cv2::MappingKind::kExponent:
      return lo + (hi - lo) * std::pow(x, static_cast<double>(b.exponent));
    case cv2::MappingKind::kLinear:
      break;
  }
  return lo + (hi - lo) * x;
}

double mapInverse(const cv2::CompiledBinding& b, const double value) noexcept {
  const double lo = b.range_min;
  const double hi = b.range_max;
  const double v = clamp(value, lo, hi);
  double x = 0.0;
  if (v >= hi) {
    x = 1.0;
  } else if (v > lo) {
    switch (b.mapping_kind) {
      case cv2::MappingKind::kLog:
        x = std::log(v / lo) / std::log(hi / lo);
        break;
      case cv2::MappingKind::kExponent:
        x = std::pow((v - lo) / (hi - lo), 1.0 / static_cast<double>(b.exponent));
        break;
      case cv2::MappingKind::kLinear:
        x = (v - lo) / (hi - lo);
        break;
    }
  }
  x = clamp(x, 0.0, 1.0);
  return inverted(b) ? 1.0 - x : x;
}

cv2::TypedValue quantise(const cv2::CompiledBinding& b, const double value) noexcept {
  const double lo = b.range_min;
  const double hi = b.range_max;
  const double step = b.quant_step;
  double out = value;
  if (!finiteDouble(value)) {
    return {};
  }
  if (value <= lo) {
    out = lo;
  } else if (value >= hi) {
    out = hi;
  } else if (step > 0.0) {
    const double k_max = std::floor((hi - lo) / step + 1e-9);
    const double k = clamp(std::round((value - lo) / step), 0.0, k_max);
    const double grid = lo + k * step;
    const double grid_capped = grid > hi ? hi : grid;
    // Max is always a grid point: pick the nearer of the nearest step and max
    // (ties go to the lower point, keeping the map monotonic).
    out = (hi - value) < std::fabs(value - grid_capped) ? hi : grid_capped;
  }
  if (b.value_kind == cv2::ValueKind::kInteger) {
    return cv2::TypedValue::integer(static_cast<std::int32_t>(std::lround(out)));
  }
  return cv2::TypedValue::real(static_cast<float>(out));
}

AbsoluteResult evaluateAbsolute(const cv2::CompiledBinding& b, const double x01) noexcept {
  if ((b.flags & (cv2::kBindingFlagUnused | cv2::kBindingFlagRelative)) != 0U || !finiteDouble(x01)) {
    return {};
  }
  const double limited = limitInput(x01, b.input_steps);
  const cv2::TypedValue v = quantise(b, mapForward(b, limited));
  return {v.kind != cv2::ValueKind::kNone, v};
}

RelativeResult evaluateRelative(const cv2::CompiledBinding& b, const double input_delta,
                                const double dt_s, const double current) noexcept {
  RelativeResult r{};
  if ((b.flags & cv2::kBindingFlagRelative) == 0U || !finiteDouble(input_delta) ||
      !finiteDouble(dt_s) || !(dt_s > 0.0) || !finiteDouble(current)) {
    return r;
  }
  const double lo = b.range_min;
  const double hi = b.range_max;
  const double velocity = std::fabs(input_delta) / dt_s;
  double multiplier = 1.0;
  if (b.rel_accel_exponent > 0.0F) {
    multiplier = clamp(std::pow(velocity / static_cast<double>(b.rel_reference_velocity),
                                static_cast<double>(b.rel_accel_exponent)),
                       1.0, static_cast<double>(b.rel_max_multiplier));
  }
  const double direction = inverted(b) ? -1.0 : 1.0;
  const double raw = direction * input_delta * static_cast<double>(b.rel_gain) * multiplier * (hi - lo);
  const double limit = static_cast<double>(b.rel_max_rate) * dt_s;
  const double delta = clamp(raw, -limit, limit);
  const cv2::TypedValue next = quantise(b, clamp(current + delta, lo, hi));
  if (next.kind == cv2::ValueKind::kNone) {
    return r;
  }
  r.ok = true;
  r.multiplier = multiplier;
  r.new_estimate = next.asNumber();
  const double exact = r.new_estimate - current;
  r.delta = b.value_kind == cv2::ValueKind::kInteger
                ? cv2::TypedValue::integer(static_cast<std::int32_t>(std::lround(exact)))
                : cv2::TypedValue::real(static_cast<float>(exact));
  return r;
}

}  // namespace k1::core::control::v2
