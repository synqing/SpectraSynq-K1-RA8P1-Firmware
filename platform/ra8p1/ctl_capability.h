#pragma once
// CTL_CAPABILITY (fixture opcode 21): a read-only report of CTL v2's own
// generated Titan platform profile (contract/control_v2/generated/
// control_registry_v2.generated.h, imported at PIN_TIT2). No control value
// is read or written here -- this is capability discovery only, so a host
// tool can ask "what can this board actually do" before attempting a
// control transaction over a route this repository does not implement yet.
//
// Every field below is read directly from the imported, generated
// PlatformProfile{Platform::kTitan, ...} row, not asserted by this file:
// max_pages == 0 (no pages), persistence_slot_bytes == 0 (no persistence),
// service_mask has only kServiceRouteFixture set (no BLE, no profile
// transfer) -- see tests/host/test_ctl_capability.cpp, which fails loudly if
// the generated data ever stops saying that.
#include <cstddef>
#include <cstdio>

#include "contract/control_v2/control_v2_types.h"
#include "core/control/v2/registry_view.h"

namespace k1::titan {

inline std::size_t ctlCapabilityJson(char* out, std::size_t capacity) noexcept {
  if (!out || capacity == 0U) return 0U;
  const auto* profile = core::control::v2::platformProfile(control_v2::Platform::kTitan);
  if (!profile) return 0U;
  namespace cv2 = k1::control_v2;
  const bool pages = profile->max_pages > 0U;
  const bool persistence = profile->persistence_slot_bytes > 0U;
  const bool ble = (profile->service_mask & cv2::kServiceRouteBle) != 0U;
  const bool fixture_route = (profile->service_mask & cv2::kServiceRouteFixture) != 0U;
  const int n = std::snprintf(out, capacity,
      "{\"opcode\":21,\"version\":1,\"platform\":\"titan\",\"engine\":%s,"
      "\"channel_count\":%u,\"max_targets\":%u,\"max_transaction_ops\":%u,"
      "\"max_concurrent_leases\":%u,\"max_writers\":%u,"
      "\"pages_supported\":%s,\"max_pages\":%u,"
      "\"persistence_supported\":%s,\"persistence_slot_bytes\":%u,"
      "\"ble_route_supported\":%s,\"fixture_route_supported\":%s,"
      "\"response_policy_mask\":%lu,\"mode_count\":%u,"
      "\"parameter_count\":%u}",
      profile->engine ? "true" : "false",
      unsigned(profile->channel_count), unsigned(profile->max_targets),
      unsigned(profile->max_transaction_ops), unsigned(profile->max_concurrent_leases),
      unsigned(profile->max_writers),
      pages ? "true" : "false", unsigned(profile->max_pages),
      persistence ? "true" : "false", unsigned(profile->persistence_slot_bytes),
      ble ? "true" : "false", fixture_route ? "true" : "false",
      static_cast<unsigned long>(profile->response_policy_mask),
      unsigned(profile->mode_count),
      unsigned(k1::control_v2::generated::kParameterCount));
  if (n < 0 || static_cast<std::size_t>(n) >= capacity) return 0U;
  return static_cast<std::size_t>(n);
}

}  // namespace k1::titan
