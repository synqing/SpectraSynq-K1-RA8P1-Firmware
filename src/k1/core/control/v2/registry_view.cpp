// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/registry_view.h"

#include <cstring>

namespace k1::core::control::v2 {

int parameterIndex(const std::uint16_t semantic_id) noexcept {
  std::size_t lo = 0U;
  std::size_t hi = gen::kParameterCount;
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2U;
    const std::uint16_t id = gen::kParameters[mid].semantic_id;
    if (id == semantic_id) {
      return static_cast<int>(mid);
    }
    if (id < semantic_id) {
      lo = mid + 1U;
    } else {
      hi = mid;
    }
  }
  return kNoIndex;
}

const cv2::ParameterDescriptor* findParameter(const std::uint16_t semantic_id) noexcept {
  const int index = parameterIndex(semantic_id);
  return index == kNoIndex ? nullptr : &gen::kParameters[index];
}

const cv2::ParameterDescriptor* findParameterByKey(const char* key) noexcept {
  if (key == nullptr) {
    return nullptr;
  }
  for (const cv2::ParameterDescriptor& d : gen::kParameters) {
    if (std::strncmp(d.key, key, cv2::kKeyBytes) == 0) {
      return &d;
    }
  }
  return nullptr;
}

int targetIndex(const cv2::TargetRef& target) noexcept {
  std::size_t lo = 0U;
  std::size_t hi = gen::kTargetCount;
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2U;
    const cv2::TargetRef candidate{gen::kTargets[mid].semantic_id, gen::kTargets[mid].channel};
    if (candidate == target) {
      return static_cast<int>(mid);
    }
    if (cv2::targetLess(candidate, target)) {
      lo = mid + 1U;
    } else {
      hi = mid;
    }
  }
  return kNoIndex;
}

cv2::TargetRef targetAt(const std::size_t index) noexcept {
  return index < gen::kTargetCount
             ? cv2::TargetRef{gen::kTargets[index].semantic_id, gen::kTargets[index].channel}
             : cv2::TargetRef{};
}

const cv2::ParameterDescriptor& targetDescriptor(const std::size_t index) noexcept {
  return gen::kParameters[gen::kTargets[index < gen::kTargetCount ? index : 0U].parameter_index];
}

bool modeEnabled(const std::uint16_t mode_id) noexcept {
  for (const std::uint16_t mode : gen::kEnabledModes) {
    if (mode == mode_id) {
      return true;
    }
  }
  return false;
}

bool legalValue(const cv2::ParameterDescriptor& d, const cv2::TypedValue& value) noexcept {
  if (value.kind != d.value_kind) {
    return false;
  }
  switch (value.kind) {
    case cv2::ValueKind::kReal: {
      const float v = value.asReal();
      std::uint32_t bits = 0U;
      std::memcpy(&bits, &v, sizeof(bits));
      if ((bits & 0x7F800000U) == 0x7F800000U) {
        return false;
      }
      return v >= d.legal_min.asReal() && v <= d.legal_max.asReal();
    }
    case cv2::ValueKind::kInteger: {
      const std::int64_t v = value.asInteger();
      const std::int64_t lo = d.legal_min.asInteger();
      const std::int64_t hi = d.legal_max.asInteger();
      const std::int64_t step = d.canonical_step.asInteger();
      return v >= lo && v <= hi && step >= 1 && (v - lo) % step == 0;
    }
    case cv2::ValueKind::kBoolean:
      return value.bits <= 1U;
    case cv2::ValueKind::kEnum:
      if (value.bits > d.legal_max.bits) {
        return false;
      }
      return (d.flags & cv2::kParamFlagSparseEnum) == 0U ||
             modeEnabled(static_cast<std::uint16_t>(value.bits));
    case cv2::ValueKind::kNone:
      break;
  }
  return false;
}

const gen::ResponsePolicyEntry* responsePolicy(const std::uint8_t id) noexcept {
  return id < gen::kResponsePolicyCount ? &gen::kResponsePolicies[id] : nullptr;
}

bool policyAdmitted(const cv2::ParameterDescriptor& d, const std::uint8_t policy_id,
                    const std::uint32_t platform_policy_mask) noexcept {
  if (responsePolicy(policy_id) == nullptr || policy_id >= 32U ||
      ((platform_policy_mask >> policy_id) & 1U) == 0U) {
    return false;
  }
  // Immediate (discrete or authority) parameters admit only the default or
  // direct policy; engine-owned parameters admit the whole catalogue.
  return d.response_owner == cv2::ResponseOwner::kEngine ||
         policy_id == cv2::kResponsePolicyDescriptorDefault ||
         policy_id == cv2::kResponsePolicyDirect;
}

const gen::PlatformProfile* platformProfile(const cv2::Platform platform) noexcept {
  for (const gen::PlatformProfile& p : gen::kPlatforms) {
    if (p.platform == platform) {
      return &p;
    }
  }
  return nullptr;
}

cv2::CapabilityParameterEntry parameterSupport(const cv2::Platform platform,
                                               const std::uint16_t semantic_id) noexcept {
  const gen::PlatformProfile* p = platformProfile(platform);
  const int index = parameterIndex(semantic_id);
  if (p == nullptr || index == kNoIndex) {
    return {semantic_id, cv2::Support::kUnsupported, cv2::InactiveReason::kPlatformUnsupported};
  }
  return p->parameter_support[index];
}

bool buildCapabilities(const cv2::Platform platform, cv2::ControlCapabilities& out) noexcept {
  const gen::PlatformProfile* p = platformProfile(platform);
  if (p == nullptr) {
    return false;
  }
  cv2::ControlCapabilities c{};
  c.product_id = p->product_id;
  c.platform = p->platform;
  c.capacity_status = p->capacity_status;
  c.layout_version_min = cv2::kObjectLayoutVersion;
  c.layout_version_max = cv2::kObjectLayoutVersion;
  c.registry_major = cv2::kRegistryMajor;
  c.registry_minor = gen::kRegistryMinor;
  std::memcpy(c.registry_sha256, gen::kRegistrySha256, cv2::kDigestBytes);
  c.engine_identity = 0U;
  c.channel_count = p->channel_count;
  c.max_transaction_ops = p->max_transaction_ops;
  c.max_concurrent_leases = p->max_concurrent_leases;
  c.max_writers = p->max_writers;
  c.max_targets = p->max_targets;
  c.max_pages = p->max_pages;
  c.max_bindings = static_cast<std::uint16_t>(p->max_pages * cv2::kSlotsPerPage);
  c.max_snapshot_entries = p->max_snapshot_entries;
  c.max_label_bytes = p->max_label_bytes;
  c.duplicate_window = p->duplicate_window;
  c.max_profile_bytes = p->max_profile_bytes;
  c.persistence_slot_bytes = p->persistence_slot_bytes;
  c.lease_timeout_us = p->lease_timeout_us;
  c.response_policy_mask = p->response_policy_mask;
  c.numeric_encoding_mask = p->numeric_encoding_mask;
  c.input_modality_mask = p->input_modality_mask;
  c.telemetry_mask = p->telemetry_mask;
  c.service_mask = p->service_mask;
  c.parameter_count = static_cast<std::uint16_t>(gen::kParameterCount);
  for (std::size_t i = 0U; i < gen::kParameterCount; ++i) {
    c.parameters[i] = p->parameter_support[i];
  }
  c.mode_count = p->mode_count;
  for (std::size_t i = 0U; i < p->mode_count && i < cv2::kMaxCapabilityModes; ++i) {
    c.modes[i] = p->modes[i];
  }
  if (cv2::validate(c) != cv2::CodecStatus::kOk) {
    return false;
  }
  out = c;
  return true;
}

const gen::V1AliasEntry* v1Alias(const std::uint8_t v1_id) noexcept {
  for (const gen::V1AliasEntry& alias : gen::kV1Aliases) {
    if (alias.v1_id == v1_id) {
      return &alias;
    }
  }
  return nullptr;
}

}  // namespace k1::core::control::v2
