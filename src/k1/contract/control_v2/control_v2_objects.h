// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// K1 control contract v2: wire-independent objects and their canonical
// little-endian encodings.
//
// Every object has a canonical byte form. It is NOT the native struct memory:
// encode()/decode() write and read each field at the offset documented below.
// These encodings are the payloads that transport codecs (TRN), persistence
// (CTL-4) and fixtures carry; framing, fragmentation and integrity of the
// carrier belong to the transport.
//
// Common object header (4 bytes, every object):
//   [0]    u8  object kind (ObjectKind)
//   [1]    u8  layout version (kObjectLayoutVersion = 1)
//   [2..3] u16 encoded length in bytes, header included
//
// Common field encodings:
//   TypedValue (8 bytes): [0] u8 kind  [1..3] zero  [4..7] u32 bits
//   TargetRef  (4 bytes): [0..1] u16 semantic_id  [2] u8 channel  [3] zero
//   f32 = IEEE-754 binary32 bit pattern as u32; every f32 must be finite and
//   canonical: negative zero (0x80000000) is rejected as kNonCanonical.
//   Keys are fixed-width [a-z0-9_] ASCII. Labels are fixed-width bounded
//   UTF-8: well formed, no control characters, NUL padded and NUL terminated,
//   so a label's byte length is at most its field width minus one. Labels
//   live in descriptors and profiles; they never ride the K1BR hot control
//   path (TRN fragments them inside the v2 envelope).
//
// decode() performs full validation: header kind/version/length, every
// integer against its declared width and legal range (including u64 time
// fields, which must be <= kMaxTimeUs), every count against its capacity and
// against the list length the encoded length implies, enum ranges, finite
// floats, zero reserved bytes, string charset and padding, ordering and
// uniqueness where declared, and trailing CRC-32 where present. On failure
// the output object is reset to its default value.
#pragma once

#include <cstddef>
#include <cstdint>

#include "contract/control_v2/control_v2_types.h"

namespace k1::control_v2 {

// ---------------------------------------------------------------------------
// ParameterDescriptor — fixed 152 bytes
//   0  header (kind 0x01, length 152)
//   4  u16 semantic_id                [0x0100, 0x0FFF]
//   6  u16 semantic_version           >= 1
//   8  u8  value_kind                 Real | Integer | Boolean | Enum
//   9  u8  scope                      Scope
//  10  u8  unit                       Unit
//  11  u8  formatter                  Formatter
//  12  u8  response_owner             ResponseOwner
//  13  u8  numeric_encoding           NumericEncoding
//  14  u8  lifecycle                  Lifecycle
//  15  u8  migration_policy           MigrationPolicy
//  16  u16 flags                      kParamFlag* (undefined bits zero)
//  18  u16 feature_id                 0 or [0x1000, 0x1FFF]
//  20  u16 enable_semantic_id         0 or parameter id (not itself)
//  22  u16 dependency_semantic_id     0 or parameter id (not itself)
//  24  u16 context_id                 0 or [0x2000, 0x2FFF]
//  26  u8  v1_alias[0]                0xFF or v1 id < 71 (primary, or global)
//  27  u8  v1_alias[1]                0xFF or v1 id < 71 (secondary; 0xFF if global)
//  28  TypedValue legal_min
//  36  TypedValue legal_max
//  44  TypedValue default_value
//  52  TypedValue neutral             kNone or value_kind
//  60  TypedValue canonical_step      Real/Integer: value_kind; Boolean/Enum: kNone
//  68  f32 encoding_error_bound       finite, >= 0 (canonical units)
//  72  f32 settle_epsilon             finite, >= 0 (terminal snap, canonical units)
//  76  u16 enum_count                 Enum: legal_max + 1; otherwise 0
//  78  u16 reserved                   zero
//  80  char key[24]                   [a-z0-9_], NUL padded
// 104  char label[24]                 full label (lens, editor): bounded UTF-8, <= 23 bytes
// 128  char short_label[24]           slot label (two lines max; see kShortLabelBytes)
// ---------------------------------------------------------------------------
struct ParameterDescriptor final {
  std::uint16_t semantic_id{0U};
  std::uint16_t semantic_version{0U};
  ValueKind value_kind{ValueKind::kNone};
  Scope scope{Scope::kChannel};
  Unit unit{Unit::kNone};
  Formatter formatter{Formatter::kRaw};
  ResponseOwner response_owner{ResponseOwner::kImmediate};
  NumericEncoding numeric_encoding{NumericEncoding::kFloat32};
  Lifecycle lifecycle{Lifecycle::kProposed};
  MigrationPolicy migration_policy{MigrationPolicy::kNone};
  std::uint16_t flags{0U};
  std::uint16_t feature_id{0U};
  std::uint16_t enable_semantic_id{0U};
  std::uint16_t dependency_semantic_id{0U};
  std::uint16_t context_id{0U};
  std::uint8_t v1_alias[2]{kNoV1Alias, kNoV1Alias};
  TypedValue legal_min{};
  TypedValue legal_max{};
  TypedValue default_value{};
  TypedValue neutral{};
  TypedValue canonical_step{};
  float encoding_error_bound{0.0F};
  float settle_epsilon{0.0F};
  std::uint16_t enum_count{0U};
  char key[kKeyBytes]{};
  char label[kDescriptorLabelBytes]{};
  char short_label[kShortLabelBytes]{};
};
inline constexpr std::size_t kParameterDescriptorBytes = 152U;

// ---------------------------------------------------------------------------
// FeatureDescriptor — fixed 88 bytes
//   0  header (kind 0x02, length 88)
//   4  u16 feature_id                 [0x1000, 0x1FFF]
//   6  u16 feature_version            >= 1
//   8  u16 enable_semantic_id         parameter id (the independent enable)
//  10  u8  owner                      FeatureOwner 1..4
//  11  u8  stage_order                0..255
//  12  u8  bypass_policy              BypassPolicy
//  13  u8  reenable_policy            ReenablePolicy
//  14  u16 reset_causes               kResetOn* (undefined bits zero)
//  16  u32 state_bytes_per_channel
//  20  u8  max_instances_per_channel  >= 1
//  21  u8  amount_count               0..4
//  22  u8  dependency_count           0..4
//  23  u8  reserved                   zero
//  24  u16 amount_semantic_ids[4]     first amount_count non-zero, unique; rest zero
//  32  u16 dependency_feature_ids[4]  first dependency_count non-zero, unique; rest zero
//  40  char key[24]
//  64  char label[24]
// ---------------------------------------------------------------------------
struct FeatureDescriptor final {
  std::uint16_t feature_id{0U};
  std::uint16_t feature_version{0U};
  std::uint16_t enable_semantic_id{0U};
  FeatureOwner owner{FeatureOwner::kVp};
  std::uint8_t stage_order{0U};
  BypassPolicy bypass_policy{BypassPolicy::kIdentity};
  ReenablePolicy reenable_policy{ReenablePolicy::kResumeFromBase};
  std::uint16_t reset_causes{0U};
  std::uint32_t state_bytes_per_channel{0U};
  std::uint8_t max_instances_per_channel{0U};
  std::uint8_t amount_count{0U};
  std::uint8_t dependency_count{0U};
  std::uint16_t amount_semantic_ids[kMaxFeatureAmounts]{};
  std::uint16_t dependency_feature_ids[kMaxFeatureDependencies]{};
  char key[kKeyBytes]{};
  char label[kDescriptorLabelBytes]{};
};
inline constexpr std::size_t kFeatureDescriptorBytes = 88U;

// ---------------------------------------------------------------------------
// VisualContextDescriptor — fixed 96 bytes. Typed visualiser metadata only;
// never executable UI code.
//   0  header (kind 0x0A, length 96)
//   4  u16 context_id                 [0x2000, 0x2FFF]
//   6  u16 context_version            >= 1
//   8  u8  visualiser                 Visualiser 1..9
//   9  u8  scope                      Scope
//  10  u8  primary_unit               Unit
//  11  u8  preview_basis              PreviewBasis
//  12  u8  parameter_count            1..8
//  13  u8  dependency_count           0..8
//  14  u16 reserved                   zero
//  16  u16 parameter_ids[8]           first parameter_count valid, unique; rest zero
//  32  u16 dependency_ids[8]          first dependency_count valid, unique; rest zero
//  48  char key[24]
//  72  char label[24]
// ---------------------------------------------------------------------------
struct VisualContextDescriptor final {
  std::uint16_t context_id{0U};
  std::uint16_t context_version{0U};
  Visualiser visualiser{Visualiser::kPalettePath};
  Scope scope{Scope::kChannel};
  Unit primary_unit{Unit::kNone};
  PreviewBasis preview_basis{PreviewBasis::kIllustrative};
  std::uint8_t parameter_count{0U};
  std::uint8_t dependency_count{0U};
  std::uint16_t parameter_ids[kMaxContextParameters]{};
  std::uint16_t dependency_ids[kMaxContextDependencies]{};
  char key[kKeyBytes]{};
  char label[kDescriptorLabelBytes]{};
};
inline constexpr std::size_t kVisualContextDescriptorBytes = 96U;

// ---------------------------------------------------------------------------
// ControlCapabilities — 92 + 4 * parameter_count + 4 * mode_count bytes
//   0  header (kind 0x03)
//   4  u32 product_id                 non-zero
//   8  u8  platform                   Platform 1..4
//   9  u8  capacity_status            CapacityStatus
//  10  u8  layout_version_min         == 1
//  11  u8  layout_version_max         >= layout_version_min
//  12  u16 registry_major             == kRegistryMajor
//  14  u16 registry_minor
//  16  u8  registry_sha256[32]
//  48  u32 engine_identity            0 = unknown
//  52  u8  channel_count              1..2
//  53  u8  max_transaction_ops        0..16
//  54  u8  max_concurrent_leases
//  55  u8  max_writers
//  56  u16 max_targets                0..64
//  58  u16 max_pages                  0..16
//  60  u16 max_bindings               == max_pages * 9
//  62  u16 max_snapshot_entries       0..64
//  64  u16 max_label_bytes            0..24 (label field width, NUL included)
//  66  u16 duplicate_window
//  68  u32 max_profile_bytes          >= compiled size of max_pages when max_pages > 0
//  72  u32 persistence_slot_bytes     0 = no store on this platform
//  76  u32 lease_timeout_us
//  80  u32 response_policy_mask       bit n = response policy id n admitted
//  84  u8  numeric_encoding_mask      bit n = NumericEncoding n
//  85  u8  input_modality_mask        kModalityMask*
//  86  u8  telemetry_mask             kTelemetryMask*
//  87  u8  service_mask               kService* (profile transfer, persistence, routes)
//  88  u16 parameter_count            <= 64
//  90  u16 mode_count                 <= 64
//  92  parameter entries, 4 bytes each, ascending unique semantic_id:
//        [0..1] u16 semantic_id  [2] u8 support  [3] u8 inactive_reason
//      (inactive_reason == kNone exactly when support == kImplemented)
//  ..  mode entries, 4 bytes each, ascending unique mode_id:
//        [0..1] u16 mode_id (<= 255)  [2] u8 liveiness_adapter_version
//        [3] u8 support  (adapter version >= 1 exactly when implemented)
// ---------------------------------------------------------------------------
struct CapabilityParameterEntry final {
  std::uint16_t semantic_id{0U};
  Support support{Support::kPlanned};
  InactiveReason inactive_reason{InactiveReason::kNone};
};

struct CapabilityModeEntry final {
  std::uint16_t mode_id{0U};
  std::uint8_t liveiness_adapter_version{0U};
  Support support{Support::kUnsupported};
};

struct ControlCapabilities final {
  std::uint32_t product_id{0U};
  Platform platform{Platform::kRt1062};
  CapacityStatus capacity_status{CapacityStatus::kProposed};
  std::uint8_t layout_version_min{kObjectLayoutVersion};
  std::uint8_t layout_version_max{kObjectLayoutVersion};
  std::uint16_t registry_major{kRegistryMajor};
  std::uint16_t registry_minor{0U};
  std::uint8_t registry_sha256[kDigestBytes]{};
  std::uint32_t engine_identity{0U};
  std::uint8_t channel_count{0U};
  std::uint8_t max_transaction_ops{0U};
  std::uint8_t max_concurrent_leases{0U};
  std::uint8_t max_writers{0U};
  std::uint16_t max_targets{0U};
  std::uint16_t max_pages{0U};
  std::uint16_t max_bindings{0U};
  std::uint16_t max_snapshot_entries{0U};
  std::uint16_t max_label_bytes{0U};
  std::uint16_t duplicate_window{0U};
  std::uint32_t max_profile_bytes{0U};
  std::uint32_t persistence_slot_bytes{0U};
  std::uint32_t lease_timeout_us{0U};
  std::uint32_t response_policy_mask{0U};
  std::uint8_t numeric_encoding_mask{0U};
  std::uint8_t input_modality_mask{0U};
  std::uint8_t telemetry_mask{0U};
  std::uint8_t service_mask{0U};
  std::uint16_t parameter_count{0U};
  std::uint16_t mode_count{0U};
  CapabilityParameterEntry parameters[kMaxCapabilityParameters]{};
  CapabilityModeEntry modes[kMaxCapabilityModes]{};
};
inline constexpr std::size_t kControlCapabilitiesFixedBytes = 92U;
inline constexpr std::size_t kCapabilityEntryBytes = 4U;

// ---------------------------------------------------------------------------
// CompiledProfile — 76 + 20 * page_count + 76 * binding_count + 4 bytes
// Console-side: compiled and persisted on the console. The engine receives
// only a ProfileIdentity; staged transfer of these tables to an engine is not
// advertised in this release (kServiceProfileTransfer clear everywhere).
//   0  header (kind 0x04)
//   4  u16 profile_schema_major       == kProfileSchemaMajor
//   6  u16 compiler_version           >= 1
//   8  u32 profile_id                 non-zero
//  12  u32 profile_revision           >= 1
//  16  u8  registry_sha256[32]
//  48  u8  channel_count              == 2
//  49  u8  page_count                 1..16
//  50  u16 binding_count              == page_count * 9
//  52  char profile_label[24]         bounded UTF-8, non-empty
//  76  pages, 20 bytes each, ordered by (channel, page_id), page_id unique per channel:
//        [0] u8 page_id (>= 1)  [1] u8 channel (0/1)  [2] u8 kind (PageKind)
//        [3] u8 slot_count (== 9)  [4..19] char label[16] (bounded UTF-8, non-empty)
//      at most one Perform and one Colour page per channel; each channel >= 1 page
//  ..  bindings, 76 bytes each, page-major then slot 1..9:
//        [0] u8 page_index  [1] u8 slot  [2] u8 flags  [3] u8 mapping_kind
//        [4..5] u16 semantic_id  [6] u8 channel  [7] u8 takeover
//        [8] u8 response_policy_id  [9] u8 value_kind  [10..11] u16 target_index
//        [12] f32 range_min  [16] f32 range_max  [20] f32 exponent
//        [24] u32 input_steps  [28] f32 hysteresis  [32] f32 quant_step
//        [36] f32 pickup_tolerance  [40] f32 rel_gain  [44] f32 rel_accel_exponent
//        [48] f32 rel_reference_velocity  [52] f32 rel_max_multiplier
//        [56] f32 rel_max_rate
//        [60..75] char label[16] (optional slot-label override; empty = the
//                 descriptor's short_label; same two-line rendering rule)
//  end u32 crc32 (IEEE 802.3, reflected) over every preceding byte
//
// Binding rules checked by decode (registry-independent):
//   used binding: semantic_id in parameter range; channel == page channel;
//     value_kind Real or Integer (discrete selections and enables are touch
//     controls, never fader bindings); target_index != 0xFFFF;
//     response_policy_id < kMaxResponsePolicies; range_min < range_max; log
//     mapping needs range_min > 0; exponent in [0.1, 10] for kExponent and
//     exactly 1 otherwise; input_steps 2..65536; hysteresis 0..0.1;
//     quant_step 0..span (integer >= 1 for Integer); pickup_tolerance 0..0.05.
//     Relative bindings require takeover kJump, zero pickup tolerance and
//     bounded relative fields; absolute bindings carry every relative field
//     as zero (absolute mappings reject acceleration).
//   unused binding: flags == kBindingFlagUnused and every other field zero
//     except page_index/slot/channel; target_index 0xFFFF; label empty.
// Registry-dependent checks (target exists, scope, legal domain, value kind,
// dense target index, admitted policy) are core/control/v2/profile_compiler's.
// ---------------------------------------------------------------------------
struct CompiledPage final {
  std::uint8_t page_id{0U};
  Channel channel{Channel::kPrimary};
  PageKind kind{PageKind::kPerform};
  std::uint8_t slot_count{0U};
  char label[kPageLabelBytes]{};
};

struct CompiledBinding final {
  std::uint8_t page_index{0U};
  std::uint8_t slot{0U};
  std::uint8_t flags{0U};
  MappingKind mapping_kind{MappingKind::kLinear};
  std::uint16_t semantic_id{0U};
  Channel channel{Channel::kPrimary};
  TakeoverMode takeover{TakeoverMode::kPickup};
  std::uint8_t response_policy_id{0U};
  ValueKind value_kind{ValueKind::kNone};
  std::uint16_t target_index{0xFFFFU};
  float range_min{0.0F};
  float range_max{0.0F};
  float exponent{0.0F};
  std::uint32_t input_steps{0U};
  float hysteresis{0.0F};
  float quant_step{0.0F};
  float pickup_tolerance{0.0F};
  float rel_gain{0.0F};
  float rel_accel_exponent{0.0F};
  float rel_reference_velocity{0.0F};
  float rel_max_multiplier{0.0F};
  float rel_max_rate{0.0F};
  char label[kBindingLabelBytes]{};
};

struct CompiledProfile final {
  std::uint16_t profile_schema_major{kProfileSchemaMajor};
  std::uint16_t compiler_version{0U};
  std::uint32_t profile_id{0U};
  std::uint32_t profile_revision{0U};
  std::uint8_t registry_sha256[kDigestBytes]{};
  std::uint8_t channel_count{0U};
  std::uint8_t page_count{0U};
  std::uint16_t binding_count{0U};
  char profile_label[kProfileLabelBytes]{};
  CompiledPage pages[kMaxProfilePages]{};
  CompiledBinding bindings[kMaxProfileBindings]{};
};
inline constexpr std::size_t kCompiledProfileFixedBytes = 76U;
inline constexpr std::size_t kCompiledPageBytes = 20U;
inline constexpr std::size_t kCompiledBindingBytes = 76U;
inline constexpr std::size_t kCompiledProfileTrailerBytes = 4U;
[[nodiscard]] constexpr std::size_t compiledProfileBytes(
    const std::size_t page_count) noexcept {
  return kCompiledProfileFixedBytes + kCompiledPageBytes * page_count +
         kCompiledBindingBytes * page_count * kSlotsPerPage +
         kCompiledProfileTrailerBytes;
}
inline constexpr std::size_t kMaxCompiledProfileBytes =
    compiledProfileBytes(kMaxProfilePages);
static_assert(kMaxCompiledProfileBytes <= 0xFFFFU,
              "compiled profile length must fit the u16 header field");

// ---------------------------------------------------------------------------
// ProfileIdentity — fixed 44 bytes (what the engine knows about a profile)
//   0  header (kind 0x0B, length 44)
//   4  u32 profile_id                 non-zero
//   8  u32 profile_revision           >= 1 (strictly increases per activation)
//  12  u8  profile_sha256[32]         SHA-256 of the canonical CompiledProfile
//                                     bytes; never all zero
// ---------------------------------------------------------------------------
struct ProfileIdentity final {
  std::uint32_t profile_id{0U};
  std::uint32_t profile_revision{0U};
  std::uint8_t profile_sha256[kDigestBytes]{};
};
inline constexpr std::size_t kProfileIdentityBytes = 44U;

// ---------------------------------------------------------------------------
// GestureIntent — fixed 64 bytes (INP producer; captured at gesture start)
//   0  header (kind 0x05, length 64)
//   4  u16 writer_id                  >= 1 (controller/client identity)
//   6  u8  intent_kind                IntentKind
//   7  u8  flags                      kIntentFlag*
//   8  u32 session_epoch              >= 1
//  12  u32 engine_epoch               >= 1 (epoch the producer is bound to)
//  16  u32 gesture_id                 >= 1
//  20  u32 sequence                   >= 1 (per session, strictly increasing)
//  24  u32 profile_revision           >= 1
//  28  u32 binding_revision           console view revision, trace only (0 = not carried)
//  32  TargetRef target               captured canonical target; channel 0/1
//  36  u8  page_id                    >= 1 (captured page)
//  37  u8  slot                       1..9 (captured physical fader)
//  38  u8  response_policy_id         admitted policy the binding requests
//  39  u8  reserved                   zero
//  40  TypedValue value               Real or Integer (absolute value or delta)
//  48  u64 observed_time_us           producer monotonic time, <= kMaxTimeUs
//  56  u32 expected_target_revision   target revision the gesture captured
//  60  u16 validity_ms                producer validity policy (0 = none)
//  62  u16 reserved                   zero
// ---------------------------------------------------------------------------
struct GestureIntent final {
  std::uint16_t writer_id{0U};
  IntentKind intent_kind{IntentKind::kAbsolute};
  std::uint8_t flags{0U};
  std::uint32_t session_epoch{0U};
  std::uint32_t engine_epoch{0U};
  std::uint32_t gesture_id{0U};
  std::uint32_t sequence{0U};
  std::uint32_t profile_revision{0U};
  std::uint32_t binding_revision{0U};
  TargetRef target{};
  std::uint8_t page_id{0U};
  std::uint8_t slot{0U};
  std::uint8_t response_policy_id{kResponsePolicyDescriptorDefault};
  TypedValue value{};
  std::uint64_t observed_time_us{0U};
  std::uint32_t expected_target_revision{0U};
  std::uint16_t validity_ms{0U};
};
inline constexpr std::size_t kGestureIntentBytes = 64U;

// ---------------------------------------------------------------------------
// ControlTransaction — 36 + 24 * op_count bytes (engine input)
//   0  header (kind 0x06)
//   4  u16 writer_id                  >= 1
//   6  u8  source                     TransactionSource
//   7  u8  flags                      kTxnFlag*; override only for preset,
//                                     blackout or external-sync sources
//   8  u32 session_epoch              >= 1
//  12  u32 transaction_id             >= 1 (idempotency identity with writer+session)
//  16  u32 engine_epoch               >= 1
//  20  u32 profile_revision           0 = not bound to a profile
//  24  u64 deadline_us                0 = none; else engine time <= kMaxTimeUs
//  32  u8  atomicity                  Atomicity (checked against the ops)
//  33  u8  apply_timing               == kNextSafeBoundary
//  34  u8  op_count                   1..16
//  35  u8  reserved                   zero
//  36  ops, 24 bytes each:
//        [0..3]   TargetRef target (unique within the transaction)
//        [4]      u8 op_kind (OpKind)
//        [5]      u8 op_flags (kOpFlag*; terminal needs gesture_id)
//        [6]      u8 response_policy_id (< kMaxResponsePolicies; 0 = descriptor default)
//        [7]      u8 reserved, zero
//        [8..11]  u32 expected_target_revision
//        [12..15] u32 gesture_id (0 = none)
//        [16..23] TypedValue value (never kNone; delta ops Real or Integer)
// ---------------------------------------------------------------------------
struct ControlOp final {
  TargetRef target{};
  OpKind op_kind{OpKind::kSetAbsolute};
  std::uint8_t op_flags{0U};
  std::uint8_t response_policy_id{kResponsePolicyDescriptorDefault};
  std::uint32_t expected_target_revision{0U};
  std::uint32_t gesture_id{0U};
  TypedValue value{};
};

struct ControlTransaction final {
  std::uint16_t writer_id{0U};
  TransactionSource source{TransactionSource::kGesture};
  std::uint8_t flags{0U};
  std::uint32_t session_epoch{0U};
  std::uint32_t transaction_id{0U};
  std::uint32_t engine_epoch{0U};
  std::uint32_t profile_revision{0U};
  std::uint64_t deadline_us{0U};
  Atomicity atomicity{Atomicity::kSingleTarget};
  ApplyTiming apply_timing{ApplyTiming::kNextSafeBoundary};
  std::uint8_t op_count{0U};
  ControlOp ops[kMaxTransactionOps]{};
};
inline constexpr std::size_t kControlTransactionFixedBytes = 36U;
inline constexpr std::size_t kControlOpBytes = 24U;
inline constexpr std::size_t kMaxControlTransactionBytes =
    kControlTransactionFixedBytes + kControlOpBytes * kMaxTransactionOps;

// ---------------------------------------------------------------------------
// ApplyReceipt — 36 + 20 * op_count bytes (engine output; not a transport ACK)
//   0  header (kind 0x07)
//   4  u16 writer_id                  >= 1 (echo)
//   6  u8  outcome                    Outcome
//   7  u8  reason                     Reason (kNone exactly when accepted)
//   8  u32 session_epoch              >= 1 (echo)
//  12  u32 transaction_id             >= 1 (echo)
//  16  u32 engine_epoch               >= 1
//  20  u32 commit_revision            revision at which the accepted state is visible
//  24  u64 engine_time_us             <= kMaxTimeUs
//  32  u8  failing_op_index           0xFF = none; else < op_count
//  33  u8  op_count                   0..16 (0 only for a non-accepted outcome)
//  34  u16 reserved                   zero
//  36  per-op results, 20 bytes each:
//        [0..3]   TargetRef
//        [4]      u8 outcome   [5] u8 reason   [6] u8 inactive_reason
//        [7]      u8 flags (kStateFlag*; active exactly when inactive_reason == kNone)
//        [8..11]  u32 target_revision (<= commit_revision)
//        [12..19] TypedValue accepted_base (the engine's actual accepted base)
// ---------------------------------------------------------------------------
struct ReceiptOpResult final {
  TargetRef target{};
  Outcome outcome{Outcome::kRejected};
  Reason reason{Reason::kNone};
  InactiveReason inactive_reason{InactiveReason::kNone};
  std::uint8_t flags{0U};
  std::uint32_t target_revision{0U};
  TypedValue accepted_base{};
};

struct ApplyReceipt final {
  std::uint16_t writer_id{0U};
  Outcome outcome{Outcome::kRejected};
  Reason reason{Reason::kNone};
  std::uint32_t session_epoch{0U};
  std::uint32_t transaction_id{0U};
  std::uint32_t engine_epoch{0U};
  std::uint32_t commit_revision{0U};
  std::uint64_t engine_time_us{0U};
  std::uint8_t failing_op_index{0xFFU};
  std::uint8_t op_count{0U};
  ReceiptOpResult results[kMaxTransactionOps]{};
};
inline constexpr std::size_t kApplyReceiptFixedBytes = 36U;
inline constexpr std::size_t kReceiptOpBytes = 20U;

// ---------------------------------------------------------------------------
// StateSnapshot — 92 + 20 * entry_count + 4 bytes (coherent authoritative base)
//   0  header (kind 0x08)
//   4  u32 engine_epoch               >= 1
//   8  u32 snapshot_revision          commit revision the snapshot represents
//  12  u32 profile_revision           0 = no active profile
//  16  u8  registry_sha256[32]
//  48  u8  profile_sha256[32]         all zero exactly when profile_revision == 0
//  80  u64 engine_time_us             <= kMaxTimeUs
//  88  u16 entry_count                <= 64
//  90  u16 reserved                   zero
//  92  entries, 20 bytes each, strictly ascending by (semantic_id, channel):
//        [0..3]   TargetRef
//        [4..7]   u32 target_revision (<= snapshot_revision)
//        [8..15]  TypedValue base (never kNone)
//        [16]     u8 flags  [17] u8 inactive_reason  [18..19] reserved zero
//  end u32 crc32 over every preceding byte (begin = header, end = CRC)
// ---------------------------------------------------------------------------
struct SnapshotEntry final {
  TargetRef target{};
  std::uint32_t target_revision{0U};
  TypedValue base{};
  std::uint8_t flags{0U};
  InactiveReason inactive_reason{InactiveReason::kNone};
};

struct StateSnapshot final {
  std::uint32_t engine_epoch{0U};
  std::uint32_t snapshot_revision{0U};
  std::uint32_t profile_revision{0U};
  std::uint8_t registry_sha256[kDigestBytes]{};
  std::uint8_t profile_sha256[kDigestBytes]{};
  std::uint64_t engine_time_us{0U};
  std::uint16_t entry_count{0U};
  SnapshotEntry entries[kMaxSnapshotEntries]{};
};
inline constexpr std::size_t kStateSnapshotFixedBytes = 92U;
inline constexpr std::size_t kSnapshotEntryBytes = 20U;
inline constexpr std::size_t kMaxStateSnapshotBytes =
    kStateSnapshotFixedBytes + kSnapshotEntryBytes * kMaxSnapshotEntries + 4U;

// ---------------------------------------------------------------------------
// StateDeltaV2 — 36 + 20 * entry_count + 4 bytes (incremental authoritative base)
//   0  header (kind 0x0C)
//   4  u32 engine_epoch               >= 1
//   8  u32 base_revision              revision the receiver must hold exactly
//  12  u32 commit_revision            > base_revision
//  16  u32 profile_revision           unchanged across the delta (0 = none)
//  20  u64 engine_time_us             <= kMaxTimeUs
//  28  u16 entry_count                <= kMaxDeltaEntries (32)
//  30  u16 reserved                   zero
//  32  u32 reserved                   zero
//  36  entries, 20 bytes each, same layout and ordering rule as SnapshotEntry;
//      target_revision <= commit_revision. An entry is every target whose
//      base, target revision, flags or inactive reason changed between the
//      two revisions (feature enables are targets, so enable changes and the
//      inactive reasons they cause travel here too).
//  end u32 crc32 over every preceding byte
// Receiver rule: apply only when (engine_epoch, base_revision,
// profile_revision) equal the receiver's (epoch, snapshot_revision,
// profile_revision) and every entry names a target the receiver holds; any
// other case never applies and requests a full StateSnapshot. A sender whose
// change set exceeds kMaxDeltaEntries, or that crossed an epoch or profile
// change, sends a full StateSnapshot instead.
// ---------------------------------------------------------------------------
struct StateDelta final {
  std::uint32_t engine_epoch{0U};
  std::uint32_t base_revision{0U};
  std::uint32_t commit_revision{0U};
  std::uint32_t profile_revision{0U};
  std::uint64_t engine_time_us{0U};
  std::uint16_t entry_count{0U};
  SnapshotEntry entries[kMaxDeltaEntries]{};
};
inline constexpr std::size_t kStateDeltaFixedBytes = 36U;
inline constexpr std::size_t kMaxStateDeltaBytes =
    kStateDeltaFixedBytes + kSnapshotEntryBytes * kMaxDeltaEntries + 4U;
static_assert(kMaxStateDeltaBytes == 680U, "StateDeltaV2 maximum is one 680-byte message");
static_assert(kMaxStateDeltaBytes <= 2496U, "StateDeltaV2 fits the transport's single-message ceiling");

enum class DeltaBuild : std::uint8_t {
  kBuilt = 0U,
  kNeedSnapshot = 1U,  // epoch/profile crossed, or too many changes: send a snapshot
  kInvalid = 2U,       // inputs are not a valid ordered snapshot pair
};
enum class DeltaApply : std::uint8_t {
  kApplied = 0U,
  kNeedSnapshot = 1U,  // identity mismatch or unknown target: request a snapshot, apply nothing
  kInvalid = 2U,       // the delta itself fails validation
};
// Pure helpers. build diffs two snapshots of the same engine (same entry
// target set); apply mutates `receiver` only on kApplied (all-or-nothing).
[[nodiscard]] DeltaBuild buildStateDelta(const StateSnapshot& from, const StateSnapshot& to,
                                         StateDelta& out) noexcept;
[[nodiscard]] DeltaApply applyStateDelta(StateSnapshot& receiver, const StateDelta& delta) noexcept;

// ---------------------------------------------------------------------------
// EffectiveObservation — fixed 48 bytes (separate from accepted base)
//   0  header (kind 0x09, length 48)
//   4  TargetRef target
//   8  u32 engine_epoch               >= 1
//  12  u32 base_revision              target revision of the base it derives from
//  16  TypedValue effective           kNone exactly when unavailable/unsupported
//  24  u64 observed_time_us           <= kMaxTimeUs
//  32  u32 valid_for_us               freshness window
//  36  u8  time_domain                == kEngineMonotonicUs
//  37  u8  availability               Availability
//  38  u16 modulation_sources         kModulation* (zero when unavailable)
//  40  TypedValue material            optional material-level value (kNone if absent)
// ---------------------------------------------------------------------------
struct EffectiveObservation final {
  TargetRef target{};
  std::uint32_t engine_epoch{0U};
  std::uint32_t base_revision{0U};
  TypedValue effective{};
  std::uint64_t observed_time_us{0U};
  std::uint32_t valid_for_us{0U};
  TimeDomain time_domain{TimeDomain::kEngineMonotonicUs};
  Availability availability{Availability::kUnavailable};
  std::uint16_t modulation_sources{0U};
  TypedValue material{};
};
inline constexpr std::size_t kEffectiveObservationBytes = 48U;

// ---------------------------------------------------------------------------
// Compact lane forms. They carry one tag byte instead of the 4-byte header
// and omit what the transport lane header already carries. TRN's lane header
// must supply the fields listed in ContinuousLaneContext.
// ---------------------------------------------------------------------------

// ContinuousSampleV2 — fixed 26 bytes (continuous fader/touch updates; the
// 12-byte continuous-lane header leaves at most 27 bytes)
//   0  u8  tag                        CompactTag::kContinuousSample
//   1  u8  intent_kind                IntentKind
//   2  u8  flags                      no bits defined in this layout: zero
//   3  u8  page_id                    >= 1
//   4  u8  slot                       1..9
//   5  u8  response_policy_id         < kMaxResponsePolicies
//   6  u32 profile_revision           >= 1
//  10  TypedValue value               Real or Integer
//  18  u32 expected_target_revision
//  22  u32 observed_time_us_lo32      producer monotonic microseconds modulo 2^32
// The transport supplies the rest (ContinuousLaneContext): the frame header's
// session_epoch and msg_seq (becomes sequence / transaction_id) and TERMINAL
// flag; the lane header's target_key, gesture_id and lifetime_ms; writer_id
// from the session HELLO; engine_epoch filled by the receiving engine.
struct ContinuousSampleV2 final {
  IntentKind intent_kind{IntentKind::kAbsolute};
  std::uint8_t flags{0U};
  std::uint8_t page_id{0U};
  std::uint8_t slot{0U};
  std::uint8_t response_policy_id{kResponsePolicyDescriptorDefault};
  std::uint32_t profile_revision{0U};
  TypedValue value{};
  std::uint32_t expected_target_revision{0U};
  std::uint32_t observed_time_us_lo32{0U};
};
inline constexpr std::size_t kContinuousSampleBytes = 26U;

// Fields the transport supplies around a sample (TRN owns their byte layout).
// The rebuilt intent's observed_time_us holds the producer clock modulo 2^32;
// engine-domain age comes from the lane header's age_ms, never from mixing
// clock domains.
struct ContinuousLaneContext final {
  std::uint16_t writer_id{0U};         // session HELLO
  std::uint32_t session_epoch{0U};     // frame header (engine-issued)
  std::uint32_t engine_epoch{0U};      // filled by the receiving engine
  std::uint32_t gesture_id{0U};        // lane header
  std::uint32_t sequence{0U};          // frame header msg_seq
  bool terminal{false};                // frame TERMINAL flag
  std::uint16_t target_key{0U};        // lane header
  std::uint16_t lifetime_ms{0U};       // lane header (becomes validity_ms)
  std::uint32_t binding_revision{0U};  // not carried by the lane: 0
};

// ReceiptSummaryV2 — fixed 24 bytes (rides the transport ACK). The full
// ApplyReceipt with accepted bases rides the ordered state lane.
//   0  u8  tag                        CompactTag::kReceiptSummary
//   1  u8  outcome                    Outcome
//   2  u8  reason                     Reason (kNone exactly when accepted)
//   3  u8  failing_op_index           0xFF = none; else < op_count
//   4  u32 transaction_id             >= 1 (sequence for continuous samples)
//   8  u16 target_key                 first op's target key
//  10  u8  op_count                   1..16
//  11  u8  reserved                   zero
//  12  u32 engine_epoch               >= 1
//  16  u32 commit_revision
//  20  u32 target_revision            first op's target revision (<= commit_revision)
struct ReceiptSummaryV2 final {
  Outcome outcome{Outcome::kRejected};
  Reason reason{Reason::kNone};
  std::uint8_t failing_op_index{0xFFU};
  std::uint32_t transaction_id{0U};
  std::uint16_t target_key{0U};
  std::uint8_t op_count{0U};
  std::uint32_t engine_epoch{0U};
  std::uint32_t commit_revision{0U};
  std::uint32_t target_revision{0U};
};
inline constexpr std::size_t kReceiptSummaryBytes = 24U;

// EffectiveObservationCompactV2 — fixed 32 bytes (optional telemetry notice)
//   0  u8  tag                        CompactTag::kEffectiveObservationCompact
//   1  u8  availability               Availability
//   2  u16 target_key
//   4  TypedValue effective           kNone exactly when unavailable/unsupported
//  12  u32 base_revision
//  16  u32 engine_epoch               >= 1
//  20  u32 observed_time_us_lo32      engine monotonic microseconds modulo 2^32
//  24  u32 valid_for_us               freshness window
//  28  u16 modulation_sources         kModulation* (zero when unavailable)
//  30  u16 reserved                   zero
struct EffectiveObservationCompactV2 final {
  Availability availability{Availability::kUnavailable};
  std::uint16_t target_key{0U};
  TypedValue effective{};
  std::uint32_t base_revision{0U};
  std::uint32_t engine_epoch{0U};
  std::uint32_t observed_time_us_lo32{0U};
  std::uint32_t valid_for_us{0U};
  std::uint16_t modulation_sources{0U};
};
inline constexpr std::size_t kEffectiveObservationCompactBytes = 32U;

// Transport budgets these forms are held to (TRN measured carriage budget).
inline constexpr std::size_t kContinuousLaneBudgetBytes = 27U;
inline constexpr std::size_t kAckBudgetBytes = 29U;
inline constexpr std::size_t kNoticeBudgetBytes = 35U;
static_assert(kContinuousSampleBytes <= kContinuousLaneBudgetBytes,
              "continuous sample must fit the continuous-lane budget");
static_assert(kReceiptSummaryBytes <= kAckBudgetBytes,
              "receipt summary must fit the ACK budget");
static_assert(kEffectiveObservationCompactBytes <= kNoticeBudgetBytes,
              "compact observation must fit the notice budget");

// ---------------------------------------------------------------------------
// Codec API
// ---------------------------------------------------------------------------

enum class CodecStatus : std::uint8_t {
  kOk = 0U,
  kNullBuffer,
  kBufferTooSmall,
  kTruncated,
  kLengthMismatch,
  kWrongKind,
  kWrongVersion,
  kReservedNonZero,
  kEnumOutOfRange,
  kCountExceedsCapacity,
  kCountMismatch,
  kNonFinite,
  kNonCanonical,  // negative zero in an f32 field (not the canonical zero)
  kIntegerOutOfRange,
  kInvalidValue,
  kInvalidString,
  kCrcMismatch,
  kOrdering,
  kDuplicate,
  kInconsistent,
};
[[nodiscard]] const char* codecStatusName(CodecStatus status) noexcept;

struct EncodeResult final {
  CodecStatus status{CodecStatus::kOk};
  std::size_t size{0U};
};

// Structural validation of an in-memory object (the same rules decode applies).
[[nodiscard]] CodecStatus validate(const ParameterDescriptor& value) noexcept;
[[nodiscard]] CodecStatus validate(const FeatureDescriptor& value) noexcept;
[[nodiscard]] CodecStatus validate(const VisualContextDescriptor& value) noexcept;
[[nodiscard]] CodecStatus validate(const ControlCapabilities& value) noexcept;
[[nodiscard]] CodecStatus validate(const CompiledProfile& value) noexcept;
[[nodiscard]] CodecStatus validate(const GestureIntent& value) noexcept;
[[nodiscard]] CodecStatus validate(const ControlTransaction& value) noexcept;
[[nodiscard]] CodecStatus validate(const ApplyReceipt& value) noexcept;
[[nodiscard]] CodecStatus validate(const StateSnapshot& value) noexcept;
[[nodiscard]] CodecStatus validate(const StateDelta& value) noexcept;
[[nodiscard]] CodecStatus validate(const EffectiveObservation& value) noexcept;
[[nodiscard]] CodecStatus validate(const ProfileIdentity& value) noexcept;
[[nodiscard]] CodecStatus validate(const ContinuousSampleV2& value) noexcept;
[[nodiscard]] CodecStatus validate(const ReceiptSummaryV2& value) noexcept;
[[nodiscard]] CodecStatus validate(const EffectiveObservationCompactV2& value) noexcept;

[[nodiscard]] std::size_t encodedSize(const ParameterDescriptor& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const FeatureDescriptor& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const VisualContextDescriptor& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ControlCapabilities& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const CompiledProfile& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const GestureIntent& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ControlTransaction& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ApplyReceipt& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const StateSnapshot& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const StateDelta& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const EffectiveObservation& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ProfileIdentity& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ContinuousSampleV2& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const ReceiptSummaryV2& value) noexcept;
[[nodiscard]] std::size_t encodedSize(const EffectiveObservationCompactV2& value) noexcept;

// encode() refuses (without writing) an object that fails validate().
[[nodiscard]] EncodeResult encode(const ParameterDescriptor& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const FeatureDescriptor& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const VisualContextDescriptor& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ControlCapabilities& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const CompiledProfile& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const GestureIntent& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ControlTransaction& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ApplyReceipt& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const StateSnapshot& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const StateDelta& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const EffectiveObservation& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ProfileIdentity& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ContinuousSampleV2& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const ReceiptSummaryV2& value, std::uint8_t* out, std::size_t capacity) noexcept;
[[nodiscard]] EncodeResult encode(const EffectiveObservationCompactV2& value, std::uint8_t* out, std::size_t capacity) noexcept;

[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ParameterDescriptor& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, FeatureDescriptor& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, VisualContextDescriptor& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ControlCapabilities& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, CompiledProfile& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, GestureIntent& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ControlTransaction& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ApplyReceipt& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, StateSnapshot& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, StateDelta& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, EffectiveObservation& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ProfileIdentity& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ContinuousSampleV2& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, ReceiptSummaryV2& out) noexcept;
[[nodiscard]] CodecStatus decode(const std::uint8_t* in, std::size_t size, EffectiveObservationCompactV2& out) noexcept;

// Target keys (transport lane headers): validates range and reserved bits.
[[nodiscard]] CodecStatus decodeTargetKey(std::uint16_t key, TargetRef& out) noexcept;

// Projections between full and compact forms. The compact projections are
// lossless for every field they carry; toGestureIntent() rebuilds the full
// intent from a sample plus the lane-header context and validates it.
[[nodiscard]] ContinuousSampleV2 toContinuousSample(const GestureIntent& intent) noexcept;
[[nodiscard]] CodecStatus toGestureIntent(const ContinuousSampleV2& sample,
                                          const ContinuousLaneContext& lane,
                                          GestureIntent& out) noexcept;
[[nodiscard]] ReceiptSummaryV2 toReceiptSummary(const ApplyReceipt& receipt) noexcept;
[[nodiscard]] EffectiveObservationCompactV2 toCompactObservation(
    const EffectiveObservation& observation) noexcept;

// Integrity and identity helpers (portable, allocation-free).
[[nodiscard]] std::uint32_t crc32Ieee(const std::uint8_t* data, std::size_t size) noexcept;
[[nodiscard]] std::uint32_t crc32IeeeUpdate(std::uint32_t crc, const std::uint8_t* data,
                                            std::size_t size) noexcept;
void sha256(const std::uint8_t* data, std::size_t size, std::uint8_t out[kDigestBytes]) noexcept;

// Bounded-string helpers shared with the compiler and generators.
[[nodiscard]] bool validKeyField(const char* field, std::size_t width) noexcept;
[[nodiscard]] bool validLabelField(const char* field, std::size_t width,
                                   bool allow_empty) noexcept;
// Copies a NUL-terminated ASCII source into a fixed field; false if it does
// not fit (at least one terminating NUL must remain) — never truncates.
[[nodiscard]] bool copyLabel(char* field, std::size_t width, const char* source) noexcept;

}  // namespace k1::control_v2
