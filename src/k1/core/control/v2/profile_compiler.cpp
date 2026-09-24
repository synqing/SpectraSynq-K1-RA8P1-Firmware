// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
#include "core/control/v2/profile_compiler.h"

#include <cstring>

#include "core/control/v2/registry_view.h"

namespace k1::core::control::v2 {
namespace {

using cv2::BindingDraft;
using cv2::CompiledBinding;
using cv2::CompiledPage;
using cv2::CompiledProfile;
using cv2::PageDraft;
using cv2::ProfileDraft;

bool finiteFloat(const float v) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &v, sizeof(bits));
  return (bits & 0x7F800000U) != 0x7F800000U;
}

bool isZero(const float v) noexcept { return cv2::canonicalRealBits(v) == 0U; }

bool integral(const float v) noexcept {
  return finiteFloat(v) && v >= -2147483648.0F && v <= 2147483520.0F &&
         static_cast<float>(static_cast<std::int64_t>(v)) == v;
}

std::size_t labelLength(const char* field, const std::size_t width) noexcept {
  std::size_t n = 0U;
  while (n < width && field[n] != '\0') {
    ++n;
  }
  return n;
}

// Bounded UTF-8 in a field of `field_width`, whose text must also fit the
// platform's label width (text bytes < width).
bool labelOk(const char* field, const std::size_t field_width, const std::size_t width,
             const bool allow_empty) noexcept {
  return cv2::validLabelField(field, field_width, allow_empty) &&
         labelLength(field, field_width) < width;
}

std::size_t minSize(const std::size_t a, const std::size_t b) noexcept { return a < b ? a : b; }

CompileResult fail(const CompileError error, const std::size_t page = 0xFFU,
                   const std::uint8_t slot = 0U) noexcept {
  return {error, static_cast<std::uint8_t>(page), slot};
}

bool unusedEmpty(const BindingDraft& b) noexcept {
  const float zeros[] = {b.range_min, b.range_max, b.hysteresis, b.quant_step, b.pickup_tolerance,
                         b.rel_gain, b.rel_accel_exponent, b.rel_reference_velocity,
                         b.rel_max_multiplier, b.rel_max_rate};
  bool empty = b.semantic_id == 0U && b.mapping_kind == cv2::MappingKind::kLinear && !b.inverted &&
               !b.relative && b.takeover == cv2::TakeoverMode::kPickup &&
               b.response_policy_id == 0U && b.input_steps == 0U &&
               cv2::canonicalRealBits(b.exponent) == cv2::canonicalRealBits(1.0F) &&
               b.label[0] == '\0';
  for (const float z : zeros) {
    empty = empty && isZero(z);
  }
  return empty;
}

CompileError checkBinding(const BindingDraft& b, const CompileCapacity& capacity,
                          std::uint16_t (&seen)[cv2::kSlotsPerPage], std::size_t& seen_count) noexcept {
  if (b.unused) {
    return unusedEmpty(b) ? CompileError::kNone : CompileError::kUnusedSlotNotEmpty;
  }
  const cv2::ParameterDescriptor* d = findParameter(b.semantic_id);
  if (d == nullptr) {
    return CompileError::kUnknownParameter;
  }
  if (d->lifecycle == cv2::Lifecycle::kUnadvertised) {
    return CompileError::kUnadvertisedParameter;
  }
  if (d->scope != cv2::Scope::kChannel) {
    return CompileError::kScopeNotBindable;
  }
  if (d->value_kind != cv2::ValueKind::kReal && d->value_kind != cv2::ValueKind::kInteger) {
    return CompileError::kDiscreteNotBindable;
  }
  for (std::size_t i = 0U; i < seen_count; ++i) {
    if (seen[i] == b.semantic_id) {
      return CompileError::kDuplicateParameterOnPage;
    }
  }
  seen[seen_count++] = b.semantic_id;
  const float numbers[] = {b.range_min, b.range_max, b.exponent, b.hysteresis, b.quant_step,
                           b.pickup_tolerance, b.rel_gain, b.rel_accel_exponent,
                           b.rel_reference_velocity, b.rel_max_multiplier, b.rel_max_rate};
  for (const float n : numbers) {
    if (!finiteFloat(n)) {
      return CompileError::kNonFinite;
    }
  }
  const float lo = cv2::canonicalReal(b.range_min);
  const float hi = cv2::canonicalReal(b.range_max);
  if (!(lo < hi)) {
    return CompileError::kRangeOrder;
  }
  if (static_cast<double>(lo) < d->legal_min.asNumber() ||
      static_cast<double>(hi) > d->legal_max.asNumber()) {
    return CompileError::kRangeOutsideDomain;
  }
  if (b.mapping_kind == cv2::MappingKind::kLog && !(lo > 0.0F)) {
    return CompileError::kLogDomain;
  }
  if (static_cast<std::uint8_t>(b.mapping_kind) >= cv2::kMappingKindCount) {
    return CompileError::kExponentRange;
  }
  if (b.mapping_kind == cv2::MappingKind::kExponent) {
    if (!(b.exponent >= static_cast<float>(0.1) && b.exponent <= 10.0F)) {
      return CompileError::kExponentRange;
    }
  } else if (cv2::canonicalRealBits(b.exponent) != cv2::canonicalRealBits(1.0F)) {
    return CompileError::kExponentRange;
  }
  if (b.input_steps < cv2::kInputStepsMin || b.input_steps > cv2::kInputStepsMax) {
    return CompileError::kInputSteps;
  }
  if (!(b.hysteresis >= 0.0F && b.hysteresis <= static_cast<float>(0.1))) {
    return CompileError::kHysteresis;
  }
  const auto span = static_cast<float>(static_cast<double>(hi) - static_cast<double>(lo));
  if (!(b.quant_step >= 0.0F && b.quant_step <= span)) {
    return CompileError::kQuantStep;
  }
  if (d->value_kind == cv2::ValueKind::kInteger &&
      (!integral(lo) || !integral(hi) || !integral(b.quant_step) || b.quant_step < 1.0F)) {
    return CompileError::kIntegerStep;
  }
  if (responsePolicy(b.response_policy_id) == nullptr) {
    return CompileError::kResponsePolicyUnknown;
  }
  if (!policyAdmitted(*d, b.response_policy_id, capacity.response_policy_mask)) {
    return CompileError::kResponsePolicyNotAdmitted;
  }
  if (!(b.pickup_tolerance >= 0.0F && b.pickup_tolerance <= static_cast<float>(0.05))) {
    return CompileError::kTakeoverTolerance;
  }
  if (b.relative) {
    if ((d->flags & cv2::kParamFlagRelativeAdmitted) == 0U || !capacity.relative_admitted) {
      return CompileError::kRelativeNotAdmitted;
    }
    if (!(b.rel_gain > 0.0F && b.rel_gain <= cv2::kRelativeGainMax &&
          b.rel_accel_exponent >= 0.0F && b.rel_accel_exponent <= cv2::kRelativeAccelExponentMax &&
          b.rel_reference_velocity > 0.0F &&
          b.rel_reference_velocity <= cv2::kRelativeReferenceVelocityMax &&
          b.rel_max_multiplier >= 1.0F && b.rel_max_multiplier <= cv2::kRelativeMaxMultiplierMax &&
          b.rel_max_rate > 0.0F && b.rel_max_rate <= cv2::kSlewRateMax)) {
      return CompileError::kRelativeParams;
    }
    if (b.takeover != cv2::TakeoverMode::kJump || !isZero(b.pickup_tolerance)) {
      return CompileError::kRelativeTakeover;
    }
  } else if (!isZero(b.rel_gain) || !isZero(b.rel_accel_exponent) ||
             !isZero(b.rel_reference_velocity) || !isZero(b.rel_max_multiplier) ||
             !isZero(b.rel_max_rate)) {
    return CompileError::kAbsoluteAcceleration;
  }
  if (static_cast<std::uint8_t>(b.takeover) >= cv2::kTakeoverModeCount) {
    return CompileError::kTakeoverTolerance;
  }
  const std::size_t width = minSize(cv2::kBindingLabelBytes, capacity.max_label_bytes);
  if (!labelOk(b.label, cv2::kBindingLabelBytes, width, true)) {
    return CompileError::kLabel;
  }
  return CompileError::kNone;
}

CompiledBinding emitBinding(const BindingDraft& b, const std::uint8_t page_index,
                            const cv2::Channel channel) noexcept {
  CompiledBinding out{};
  out.page_index = page_index;
  out.slot = b.slot;
  out.channel = channel;
  if (b.unused) {
    out.flags = cv2::kBindingFlagUnused;
    out.target_index = 0xFFFFU;
    return out;
  }
  const cv2::ParameterDescriptor* d = findParameter(b.semantic_id);
  out.flags = static_cast<std::uint8_t>((b.inverted ? cv2::kBindingFlagInverted : 0U) |
                                        (b.relative ? cv2::kBindingFlagRelative : 0U));
  out.mapping_kind = b.mapping_kind;
  out.semantic_id = b.semantic_id;
  out.takeover = b.takeover;
  out.response_policy_id = b.response_policy_id;
  out.value_kind = d->value_kind;
  out.target_index = static_cast<std::uint16_t>(targetIndex({b.semantic_id, channel}));
  out.range_min = cv2::canonicalReal(b.range_min);
  out.range_max = cv2::canonicalReal(b.range_max);
  out.exponent = b.mapping_kind == cv2::MappingKind::kExponent ? cv2::canonicalReal(b.exponent) : 1.0F;
  out.input_steps = b.input_steps;
  out.hysteresis = cv2::canonicalReal(b.hysteresis);
  out.quant_step = cv2::canonicalReal(b.quant_step);
  out.pickup_tolerance = b.relative ? 0.0F : cv2::canonicalReal(b.pickup_tolerance);
  if (b.relative) {
    out.rel_gain = cv2::canonicalReal(b.rel_gain);
    out.rel_accel_exponent = cv2::canonicalReal(b.rel_accel_exponent);
    out.rel_reference_velocity = cv2::canonicalReal(b.rel_reference_velocity);
    out.rel_max_multiplier = cv2::canonicalReal(b.rel_max_multiplier);
    out.rel_max_rate = cv2::canonicalReal(b.rel_max_rate);
  }
  std::memcpy(out.label, b.label, cv2::kBindingLabelBytes);
  return out;
}

}  // namespace

const char* compileErrorName(const CompileError error) noexcept {
  static constexpr const char* kNames[] = {
      "none", "profile_identity", "label", "capacity_pages", "page_channel", "page_id",
      "slot_layout", "page_kind_duplicate", "channel_without_page", "unused_slot_not_empty",
      "unknown_parameter", "unadvertised_parameter", "scope_not_bindable", "discrete_not_bindable",
      "duplicate_parameter_on_page", "non_finite", "range_order", "range_outside_domain",
      "log_domain", "exponent_range", "input_steps", "hysteresis", "quant_step", "integer_step",
      "response_policy_unknown", "response_policy_not_admitted", "takeover_tolerance",
      "relative_not_admitted", "relative_params", "relative_takeover", "absolute_acceleration",
      "capacity_profile_bytes", "registry_mismatch"};
  const auto index = static_cast<std::size_t>(error);
  return index < sizeof(kNames) / sizeof(kNames[0]) ? kNames[index] : "unknown";
}

CompileCapacity compileCapacity(const cv2::Platform platform) noexcept {
  const gen::PlatformProfile* p = platformProfile(platform);
  if (p == nullptr) {
    return {};
  }
  return {p->max_pages, p->max_label_bytes, p->max_profile_bytes, p->response_policy_mask,
          (p->input_modality_mask & cv2::kModalityMaskRelative) != 0U};
}

CompileResult compileProfile(const ProfileDraft& draft, const CompileCapacity& capacity,
                             CompiledProfile& out) noexcept {
  if (draft.profile_id == 0U || draft.profile_revision == 0U) {
    return fail(CompileError::kProfileIdentity);
  }
  if (!labelOk(draft.label, cv2::kProfileLabelBytes, cv2::kProfileLabelBytes, false)) {
    return fail(CompileError::kLabel);
  }
  const std::size_t limit = minSize(capacity.max_pages, cv2::kMaxProfilePages);
  if (draft.page_count == 0U || draft.page_count > limit || draft.page_count > cv2::kMaxDraftPages) {
    return fail(CompileError::kCapacityPages);
  }
  const std::size_t page_label_width = minSize(cv2::kPageLabelBytes, capacity.max_label_bytes);
  for (std::size_t i = 0U; i < draft.page_count; ++i) {
    const PageDraft& page = draft.pages[i];
    if (page.channel != cv2::Channel::kPrimary && page.channel != cv2::Channel::kSecondary) {
      return fail(CompileError::kPageChannel, i);
    }
    if (page.page_id == 0U) {
      return fail(CompileError::kPageId, i);
    }
    if (!labelOk(page.label, cv2::kPageLabelBytes, page_label_width, false)) {
      return fail(CompileError::kLabel, i);
    }
    bool slots_ok = page.slot_count == cv2::kSlotsPerPage &&
                    static_cast<std::uint8_t>(page.kind) < cv2::kPageKindCount;
    std::uint16_t mask = 0U;
    for (const BindingDraft& b : page.slots) {
      slots_ok = slots_ok && b.slot >= 1U && b.slot <= cv2::kSlotsPerPage &&
                 (mask & (1U << b.slot)) == 0U;
      mask = static_cast<std::uint16_t>(mask | (1U << (b.slot & 0x0FU)));
    }
    if (!slots_ok) {
      return fail(CompileError::kSlotLayout, i);
    }
  }
  // Canonical order: (channel, page_id); stable insertion sort of indices.
  std::uint8_t order[cv2::kMaxDraftPages];
  for (std::size_t i = 0U; i < draft.page_count; ++i) {
    std::size_t j = i;
    const PageDraft& page = draft.pages[i];
    while (j > 0U) {
      const PageDraft& prev = draft.pages[order[j - 1U]];
      const bool greater = static_cast<std::uint8_t>(prev.channel) > static_cast<std::uint8_t>(page.channel) ||
                           (prev.channel == page.channel && prev.page_id > page.page_id);
      if (!greater) {
        break;
      }
      order[j] = order[j - 1U];
      --j;
    }
    order[j] = static_cast<std::uint8_t>(i);
  }
  for (std::size_t k = 1U; k < draft.page_count; ++k) {
    const PageDraft& a = draft.pages[order[k - 1U]];
    const PageDraft& b = draft.pages[order[k]];
    if (a.channel == b.channel && a.page_id == b.page_id) {
      return fail(CompileError::kPageId, order[k]);
    }
  }
  for (std::uint8_t ch = 0U; ch < 2U; ++ch) {
    std::size_t perform = 0U;
    std::size_t colour = 0U;
    for (std::size_t i = 0U; i < draft.page_count; ++i) {
      const PageDraft& page = draft.pages[i];
      if (static_cast<std::uint8_t>(page.channel) == ch) {
        perform += page.kind == cv2::PageKind::kPerform ? 1U : 0U;
        colour += page.kind == cv2::PageKind::kColour ? 1U : 0U;
      }
    }
    if (perform > 1U || colour > 1U) {
      return fail(CompileError::kPageKindDuplicate);
    }
  }
  for (std::uint8_t ch = 0U; ch < 2U; ++ch) {
    bool present = false;
    for (std::size_t i = 0U; i < draft.page_count; ++i) {
      present = present || static_cast<std::uint8_t>(draft.pages[i].channel) == ch;
    }
    if (!present) {
      return fail(CompileError::kChannelWithoutPage);
    }
  }
  for (std::size_t k = 0U; k < draft.page_count; ++k) {
    const PageDraft& page = draft.pages[order[k]];
    std::uint16_t seen[cv2::kSlotsPerPage] = {};
    std::size_t seen_count = 0U;
    for (std::uint8_t slot = 1U; slot <= cv2::kSlotsPerPage; ++slot) {
      for (const BindingDraft& b : page.slots) {
        if (b.slot == slot) {
          const CompileError e = checkBinding(b, capacity, seen, seen_count);
          if (e != CompileError::kNone) {
            return fail(e, order[k], slot);
          }
        }
      }
    }
  }
  if (cv2::compiledProfileBytes(draft.page_count) > capacity.max_profile_bytes) {
    return fail(CompileError::kCapacityProfileBytes);
  }
  // Every check passed: emit (the only point where `out` changes).
  out = CompiledProfile{};
  out.profile_schema_major = cv2::kProfileSchemaMajor;
  out.compiler_version = kCompilerVersion;
  out.profile_id = draft.profile_id;
  out.profile_revision = draft.profile_revision;
  std::memcpy(out.registry_sha256, gen::kRegistrySha256, cv2::kDigestBytes);
  out.channel_count = static_cast<std::uint8_t>(cv2::kChannelCount);
  out.page_count = draft.page_count;
  out.binding_count = static_cast<std::uint16_t>(draft.page_count * cv2::kSlotsPerPage);
  std::memcpy(out.profile_label, draft.label, cv2::kProfileLabelBytes);
  for (std::size_t k = 0U; k < draft.page_count; ++k) {
    const PageDraft& page = draft.pages[order[k]];
    CompiledPage& cp = out.pages[k];
    cp.page_id = page.page_id;
    cp.channel = page.channel;
    cp.kind = page.kind;
    cp.slot_count = static_cast<std::uint8_t>(cv2::kSlotsPerPage);
    std::memcpy(cp.label, page.label, cv2::kPageLabelBytes);
    for (std::uint8_t slot = 1U; slot <= cv2::kSlotsPerPage; ++slot) {
      for (const BindingDraft& b : page.slots) {
        if (b.slot == slot) {
          out.bindings[k * cv2::kSlotsPerPage + slot - 1U] =
              emitBinding(b, static_cast<std::uint8_t>(k), page.channel);
        }
      }
    }
  }
  return {};
}

void draftFromCompiled(const CompiledProfile& p, ProfileDraft& out) noexcept {
  out = ProfileDraft{};
  out.profile_id = p.profile_id;
  out.profile_revision = p.profile_revision;
  std::memcpy(out.label, p.profile_label, cv2::kProfileLabelBytes);
  out.page_count = p.page_count;
  for (std::size_t k = 0U; k < p.page_count && k < cv2::kMaxProfilePages; ++k) {
    PageDraft& page = out.pages[k];
    page.page_id = p.pages[k].page_id;
    page.channel = p.pages[k].channel;
    page.kind = p.pages[k].kind;
    page.slot_count = p.pages[k].slot_count;
    std::memcpy(page.label, p.pages[k].label, cv2::kPageLabelBytes);
    for (std::size_t s = 0U; s < cv2::kSlotsPerPage; ++s) {
      const CompiledBinding& b = p.bindings[k * cv2::kSlotsPerPage + s];
      BindingDraft& d = page.slots[s];
      d.slot = b.slot;
      d.unused = (b.flags & cv2::kBindingFlagUnused) != 0U;
      if (d.unused) {
        continue;
      }
      d.semantic_id = b.semantic_id;
      d.mapping_kind = b.mapping_kind;
      d.inverted = (b.flags & cv2::kBindingFlagInverted) != 0U;
      d.relative = (b.flags & cv2::kBindingFlagRelative) != 0U;
      d.takeover = b.takeover;
      d.response_policy_id = b.response_policy_id;
      d.range_min = b.range_min;
      d.range_max = b.range_max;
      d.exponent = b.exponent;
      d.input_steps = b.input_steps;
      d.hysteresis = b.hysteresis;
      d.quant_step = b.quant_step;
      d.pickup_tolerance = b.pickup_tolerance;
      d.rel_gain = b.rel_gain;
      d.rel_accel_exponent = b.rel_accel_exponent;
      d.rel_reference_velocity = b.rel_reference_velocity;
      d.rel_max_multiplier = b.rel_max_multiplier;
      d.rel_max_rate = b.rel_max_rate;
      std::memcpy(d.label, b.label, cv2::kBindingLabelBytes);
    }
  }
}

CompileResult validateCompiledProfile(const CompiledProfile& profile, const CompileCapacity& capacity,
                                      ProfileDraft& scratch_draft,
                                      CompiledProfile& scratch_profile) noexcept {
  if (cv2::validate(profile) != cv2::CodecStatus::kOk) {
    return fail(CompileError::kSlotLayout);
  }
  if (std::memcmp(profile.registry_sha256, gen::kRegistrySha256, cv2::kDigestBytes) != 0 ||
      profile.compiler_version != kCompilerVersion) {
    return fail(CompileError::kRegistryMismatch);
  }
  draftFromCompiled(profile, scratch_draft);
  const CompileResult result = compileProfile(scratch_draft, capacity, scratch_profile);
  if (!result.ok()) {
    return result;
  }
  static std::uint8_t a[cv2::kMaxCompiledProfileBytes];
  static std::uint8_t b[cv2::kMaxCompiledProfileBytes];
  const cv2::EncodeResult ea = cv2::encode(profile, a, sizeof(a));
  const cv2::EncodeResult eb = cv2::encode(scratch_profile, b, sizeof(b));
  if (ea.status != cv2::CodecStatus::kOk || eb.status != cv2::CodecStatus::kOk ||
      ea.size != eb.size || std::memcmp(a, b, ea.size) != 0) {
    return fail(CompileError::kRegistryMismatch);
  }
  return {};
}

}  // namespace k1::core::control::v2
