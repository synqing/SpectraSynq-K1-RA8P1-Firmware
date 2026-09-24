// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// On-device profile compiler (console) for the K1 control contract v2.
//
// compileProfile() validates a complete ProfileDraft against the generated
// registry and a platform capacity, then writes a CompiledProfile. It is
// atomic: on any error `out` is left untouched. The checks run in the same
// order as scripts/compile_control_profile.py, so both compilers report the
// same first error, and a valid draft compiles to byte-identical tables.
// Runs outside audio/render paths; bounded, no allocation.
#pragma once

#include <cstdint>

#include "contract/control_v2/control_v2_draft.h"
#include "contract/control_v2/control_v2_objects.h"

namespace k1::core::control::v2 {

inline constexpr std::uint16_t kCompilerVersion = 1U;

// Same names and order as ERRORS in scripts/compile_control_profile.py.
enum class CompileError : std::uint8_t {
  kNone = 0U,
  kProfileIdentity,
  kLabel,
  kCapacityPages,
  kPageChannel,
  kPageId,
  kSlotLayout,
  kPageKindDuplicate,
  kChannelWithoutPage,
  kUnusedSlotNotEmpty,
  kUnknownParameter,
  kUnadvertisedParameter,
  kScopeNotBindable,
  kDiscreteNotBindable,
  kDuplicateParameterOnPage,
  kNonFinite,
  kRangeOrder,
  kRangeOutsideDomain,
  kLogDomain,
  kExponentRange,
  kInputSteps,
  kHysteresis,
  kQuantStep,
  kIntegerStep,
  kResponsePolicyUnknown,
  kResponsePolicyNotAdmitted,
  kTakeoverTolerance,
  kRelativeNotAdmitted,
  kRelativeParams,
  kRelativeTakeover,
  kAbsoluteAcceleration,
  kCapacityProfileBytes,
  kRegistryMismatch,
};
[[nodiscard]] const char* compileErrorName(CompileError error) noexcept;

struct CompileResult final {
  CompileError error{CompileError::kNone};
  std::uint8_t page_index{0xFFU};  // draft page index (0xFF = profile level)
  std::uint8_t slot{0U};           // 1..9 when a binding failed
  [[nodiscard]] bool ok() const noexcept { return error == CompileError::kNone; }
};

struct CompileCapacity final {
  std::uint16_t max_pages{0U};
  std::uint16_t max_label_bytes{0U};
  std::uint32_t max_profile_bytes{0U};
  std::uint32_t response_policy_mask{0U};
  bool relative_admitted{false};
};

// Capacity from the generated platform profile (zero capacity if unknown).
[[nodiscard]] CompileCapacity compileCapacity(::k1::control_v2::Platform platform) noexcept;

[[nodiscard]] CompileResult compileProfile(const ::k1::control_v2::ProfileDraft& draft,
                                           const CompileCapacity& capacity,
                                           ::k1::control_v2::CompiledProfile& out) noexcept;

// Rebuilds the draft a compiled profile was compiled from (inverse of compile).
void draftFromCompiled(const ::k1::control_v2::CompiledProfile& profile,
                       ::k1::control_v2::ProfileDraft& out) noexcept;

// Registry-level validation of a profile loaded from storage or received:
// structurally valid, compiled against this registry, and recompiling its
// draft reproduces it exactly. Scratch objects are caller-owned (no stack
// blow-up on small tasks).
[[nodiscard]] CompileResult validateCompiledProfile(
    const ::k1::control_v2::CompiledProfile& profile, const CompileCapacity& capacity,
    ::k1::control_v2::ProfileDraft& scratch_draft,
    ::k1::control_v2::CompiledProfile& scratch_profile) noexcept;

}  // namespace k1::core::control::v2
