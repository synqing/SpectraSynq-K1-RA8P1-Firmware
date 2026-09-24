// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/response_dynamics.h"

#include <cmath>

#include "core/control/v2/registry_view.h"

namespace k1::core::control::v2 {

ResponseLaw resolveResponse(const std::uint8_t policy_id, const cv2::ParameterDescriptor& d) noexcept {
  ResponseLaw law{};
  const double span = d.legal_max.asNumber() - d.legal_min.asNumber();
  law.snap_epsilon = static_cast<double>(d.settle_epsilon);
  const gen::ResponsePolicyEntry* p = responsePolicy(policy_id);
  if (p == nullptr || p->descriptor_default || d.response_owner != cv2::ResponseOwner::kEngine ||
      d.value_kind != cv2::ValueKind::kReal) {
    return law;  // direct
  }
  law.kind = p->kind;
  law.tau_s = static_cast<double>(p->tau_ms) / 1000.0;
  law.rate_per_s = static_cast<double>(p->span_per_second) * span;
  return law;
}

double stepExponential(const double y, const double target, const double dt_s, const double tau_s) noexcept {
  if (!(tau_s > 0.0) || !(dt_s > 0.0)) {
    return dt_s > 0.0 ? target : y;
  }
  return y + (-std::expm1(-dt_s / tau_s)) * (target - y);
}

double stepSlew(const double y, const double target, const double dt_s, const double rate_per_s) noexcept {
  if (!(dt_s > 0.0)) {
    return y;
  }
  const double reach = rate_per_s * dt_s;
  const double diff = target - y;
  if (std::fabs(diff) <= reach) {
    return target;
  }
  return y + (diff > 0.0 ? reach : -reach);
}

void responseReset(ResponseState& state, const double value, const std::uint64_t now_us) noexcept {
  state.anchor_value = value;
  state.target = value;
  state.anchor_time_us = now_us;
}

double responseEvaluate(const ResponseState& s, const std::uint64_t now_us) noexcept {
  const double dt = now_us > s.anchor_time_us
                        ? static_cast<double>(now_us - s.anchor_time_us) / 1.0e6
                        : 0.0;
  double y = s.target;
  switch (s.law.kind) {
    case cv2::ResponseKind::kExponential:
      y = stepExponential(s.anchor_value, s.target, dt, s.law.tau_s);
      break;
    case cv2::ResponseKind::kSlew:
      y = stepSlew(s.anchor_value, s.target, dt, s.law.rate_per_s);
      break;
    case cv2::ResponseKind::kDirect:
      break;
  }
  return std::fabs(s.target - y) <= s.law.snap_epsilon ? s.target : y;
}

bool responseSettled(const ResponseState& s, const std::uint64_t now_us) noexcept {
  return responseEvaluate(s, now_us) == s.target;
}

void responseRetarget(ResponseState& state, const double target, const ResponseLaw& law,
                      const std::uint64_t now_us) noexcept {
  const double current = responseEvaluate(state, now_us);
  state.anchor_value = law.kind == cv2::ResponseKind::kDirect ? target : current;
  state.anchor_time_us = now_us > state.anchor_time_us ? now_us : state.anchor_time_us;
  state.target = target;
  state.law = law;
}

}  // namespace k1::core::control::v2
