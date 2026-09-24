// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// K1 control contract v2: profile drafts.
//
// A ProfileDraft is the bounded, allocation-free form a profile editor (the
// console touch UI or a companion tool) produces before admission. It is not
// a wire object: core/control/v2/profile_compiler validates it completely and
// compiles it into a CompiledProfile, or rejects it without touching any
// output. The JSON draft form (contract/control_v2/schema/
// control_profile_v2.schema.json) compiles through
// scripts/compile_control_profile.py to byte-identical CompiledProfile bytes.
//
// Capacity: a draft can hold one page more than the largest codec capacity so
// that an over-capacity draft can be represented and visibly rejected instead
// of being truncated on entry.
#pragma once

#include <cstdint>

#include "contract/control_v2/control_v2_types.h"

namespace k1::control_v2 {

inline constexpr std::size_t kMaxDraftPages = kMaxProfilePages + 1U;

struct BindingDraft final {
  std::uint8_t slot{0U};  // 1..9, each exactly once per page
  bool unused{false};     // explicit unused slot (every other field zero)
  std::uint16_t semantic_id{0U};
  MappingKind mapping_kind{MappingKind::kLinear};
  bool inverted{false};
  bool relative{false};   // relative-delta modality (touch drag, encoder, trim)
  TakeoverMode takeover{TakeoverMode::kPickup};
  std::uint8_t response_policy_id{kResponsePolicyDescriptorDefault};
  float range_min{0.0F};
  float range_max{0.0F};
  float exponent{1.0F};   // used only by MappingKind::kExponent
  std::uint32_t input_steps{0U};
  float hysteresis{0.0F};
  float quant_step{0.0F};
  float pickup_tolerance{0.0F};
  float rel_gain{0.0F};
  float rel_accel_exponent{0.0F};
  float rel_reference_velocity{0.0F};
  float rel_max_multiplier{0.0F};
  float rel_max_rate{0.0F};
  char label[kBindingLabelBytes]{};  // optional override; empty = descriptor label
};

struct PageDraft final {
  std::uint8_t page_id{0U};  // 1..255, unique per channel
  Channel channel{Channel::kPrimary};
  PageKind kind{PageKind::kCustom};
  std::uint8_t slot_count{0U};  // must be 9
  char label[kPageLabelBytes]{};
  BindingDraft slots[kSlotsPerPage]{};
};

struct ProfileDraft final {
  std::uint32_t profile_id{0U};
  std::uint32_t profile_revision{0U};
  char label[kProfileLabelBytes]{};
  std::uint8_t page_count{0U};
  PageDraft pages[kMaxDraftPages]{};
};

}  // namespace k1::control_v2
