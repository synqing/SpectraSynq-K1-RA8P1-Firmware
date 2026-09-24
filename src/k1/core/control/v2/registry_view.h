// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// Portable, allocation-free lookups over the generated v2 registry tables
// (contract/control_v2/generated/control_registry_v2.generated.h).
#pragma once

#include <cstddef>
#include <cstdint>

#include "contract/control_v2/control_v2_objects.h"
#include "contract/control_v2/generated/control_registry_v2.generated.h"

namespace k1::core::control::v2 {

namespace cv2 = ::k1::control_v2;
namespace gen = ::k1::control_v2::generated;

inline constexpr std::size_t kTargetCount = gen::kTargetCount;
inline constexpr int kNoIndex = -1;

// Parameter descriptors.
[[nodiscard]] int parameterIndex(std::uint16_t semantic_id) noexcept;
[[nodiscard]] const cv2::ParameterDescriptor* findParameter(std::uint16_t semantic_id) noexcept;
[[nodiscard]] const cv2::ParameterDescriptor* findParameterByKey(const char* key) noexcept;

// Dense targets in canonical (semantic_id, channel) order. A target is valid
// only when its channel matches the parameter's scope (channel parameters on
// primary/secondary, global parameters on kGlobal).
[[nodiscard]] int targetIndex(const cv2::TargetRef& target) noexcept;
[[nodiscard]] cv2::TargetRef targetAt(std::size_t index) noexcept;
[[nodiscard]] const cv2::ParameterDescriptor& targetDescriptor(std::size_t index) noexcept;

// Value legality against a descriptor: kind, finite legal domain, integer grid
// and the sparse mode enumeration.
[[nodiscard]] bool legalValue(const cv2::ParameterDescriptor& descriptor,
                              const cv2::TypedValue& value) noexcept;
[[nodiscard]] bool modeEnabled(std::uint16_t mode_id) noexcept;

// Response policies (engine-owned catalogue).
[[nodiscard]] const gen::ResponsePolicyEntry* responsePolicy(std::uint8_t id) noexcept;
// True when the policy may be requested for this descriptor on this platform.
[[nodiscard]] bool policyAdmitted(const cv2::ParameterDescriptor& descriptor,
                                  std::uint8_t policy_id,
                                  std::uint32_t platform_policy_mask) noexcept;

// Platforms.
[[nodiscard]] const gen::PlatformProfile* platformProfile(cv2::Platform platform) noexcept;
[[nodiscard]] cv2::CapabilityParameterEntry parameterSupport(cv2::Platform platform,
                                                             std::uint16_t semantic_id) noexcept;
// Builds the platform's ControlCapabilities from the generated tables.
[[nodiscard]] bool buildCapabilities(cv2::Platform platform,
                                     cv2::ControlCapabilities& out) noexcept;

// Exact v1 meanings.
[[nodiscard]] const gen::V1AliasEntry* v1Alias(std::uint8_t v1_id) noexcept;

}  // namespace k1::core::control::v2
