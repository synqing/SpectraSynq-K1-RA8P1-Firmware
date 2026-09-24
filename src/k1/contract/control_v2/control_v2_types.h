// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 SpectraSynq
//
// K1 control contract v2: shared vocabulary.
//
// This header is portable (no board, transport, filesystem or rendering
// dependency). It defines the enumerations, identity types and fixed
// capacities that every v2 control object uses. Byte layouts of the objects
// themselves live in control_v2_objects.h.
//
// Identity rules:
// - v2 semantic identifiers live in [0x0100, 0x0FFF]: they fit 12 bits, so a
//   target (semantic plus channel) packs into the 16-bit target key the
//   transport lane header carries (semantic_id | channel << 12). The v1
//   control map uses 8-bit identifiers 0..70, so a v2 identifier can never
//   alias a v1 identifier. A v2 semantic that is exactly a v1 meaning carries
//   an explicit v1 alias in its ParameterDescriptor instead.
// - Feature identifiers live in [0x1000, 0x1FFF]; visual-context identifiers
//   in [0x2000, 0x2FFF].
// - Every canonical encoding is little-endian with explicit offsets. Native
//   struct memory is never used as wire or storage bytes.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace k1::control_v2 {

// ---------------------------------------------------------------------------
// Versions and identifier spaces
// ---------------------------------------------------------------------------

inline constexpr std::uint8_t kObjectLayoutVersion = 1U;
inline constexpr std::uint16_t kRegistryMajor = 2U;
inline constexpr std::uint16_t kProfileSchemaMajor = 2U;

inline constexpr std::uint16_t kParameterIdMin = 0x0100U;
inline constexpr std::uint16_t kParameterIdMax = 0x0FFFU;
inline constexpr std::uint16_t kFeatureIdMin = 0x1000U;
inline constexpr std::uint16_t kFeatureIdMax = 0x1FFFU;
inline constexpr std::uint16_t kContextIdMin = 0x2000U;
inline constexpr std::uint16_t kContextIdMax = 0x2FFFU;

// v1 identifiers are 0..70 (contract/bridge_protocol.h kControlMapEntries).
inline constexpr std::uint8_t kV1ControlCount = 71U;
inline constexpr std::uint8_t kNoV1Alias = 0xFFU;

// Largest time value accepted in any uint64 microsecond field. The top bit is
// reserved so every time fits a signed 64-bit difference without overflow.
inline constexpr std::uint64_t kMaxTimeUs = 0x7FFFFFFFFFFFFFFFULL;

// ---------------------------------------------------------------------------
// Fixed capacities of the canonical objects (layout version 1). Platform
// capacities (ControlCapabilities) are always at or below these.
// ---------------------------------------------------------------------------

inline constexpr std::size_t kSlotsPerPage = 9U;
inline constexpr std::size_t kMaxProfilePages = 16U;
inline constexpr std::size_t kMaxProfileBindings = kMaxProfilePages * kSlotsPerPage;
inline constexpr std::size_t kMaxTransactionOps = 16U;
inline constexpr std::size_t kMaxSnapshotEntries = 64U;
// StateDeltaV2 entry cap: one transaction's ops (16) plus as many dependent
// state changes (enable -> dependant inactive reason, blackout -> master,
// mode -> Liveiness). 36 + 20 * 32 + 4 = 680 bytes, one message.
inline constexpr std::size_t kMaxDeltaEntries = 32U;
inline constexpr std::size_t kMaxCapabilityParameters = 64U;
inline constexpr std::size_t kMaxCapabilityModes = 64U;
inline constexpr std::size_t kMaxFeatureAmounts = 4U;
inline constexpr std::size_t kMaxFeatureDependencies = 4U;
inline constexpr std::size_t kMaxContextParameters = 8U;
inline constexpr std::size_t kMaxContextDependencies = 8U;
inline constexpr std::size_t kChannelCount = 2U;

// Bounded strings: fixed byte width, NUL padded, at least one NUL terminator.
// Keys are ASCII [a-z0-9_]; labels are well-formed UTF-8 without control
// characters. A field of width W holds at most W - 1 bytes of text.
inline constexpr std::size_t kKeyBytes = 24U;
inline constexpr std::size_t kDescriptorLabelBytes = 24U;
// Slot text for the 66 px console slot: at most two rendered lines; line
// breaks only at spaces or at a declared soft hyphen U+00AD (rendered as "-"
// when the break is used, removed otherwise). The UI never re-abbreviates.
inline constexpr std::size_t kShortLabelBytes = 24U;
inline constexpr std::size_t kProfileLabelBytes = 24U;
inline constexpr std::size_t kBindingLabelBytes = 16U;
inline constexpr std::size_t kPageLabelBytes = 16U;

inline constexpr std::size_t kDigestBytes = 32U;  // SHA-256

// ---------------------------------------------------------------------------
// Object kinds (first byte of every canonical encoding)
// ---------------------------------------------------------------------------

enum class ObjectKind : std::uint8_t {
  kParameterDescriptor = 0x01U,
  kFeatureDescriptor = 0x02U,
  kControlCapabilities = 0x03U,
  kCompiledProfile = 0x04U,
  kGestureIntent = 0x05U,
  kControlTransaction = 0x06U,
  kApplyReceipt = 0x07U,
  kStateSnapshot = 0x08U,
  kEffectiveObservation = 0x09U,
  kVisualContextDescriptor = 0x0AU,
  kProfileIdentity = 0x0BU,
  kStateDelta = 0x0CU,
};

// Compact lane forms carry a single tag byte (no 4-byte header) so they fit
// the transport's continuous/ACK/notice budgets. The tag identifies both the
// form and its layout; a changed layout gets a new tag.
enum class CompactTag : std::uint8_t {
  kContinuousSample = 0x21U,        // ContinuousSampleV2, <= 29 bytes
  kReceiptSummary = 0x22U,          // ReceiptSummaryV2, <= 29 bytes
  kEffectiveObservationCompact = 0x23U,  // EffectiveObservationCompactV2, <= 35 bytes
};

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

enum class ValueKind : std::uint8_t {
  kNone = 0U,     // absent; payload must be zero
  kReal = 1U,     // IEEE-754 binary32, finite
  kInteger = 2U,  // two's-complement int32
  kBoolean = 3U,  // 0 or 1
  kEnum = 4U,     // unsigned index 0..0xFFFF
};
inline constexpr std::uint8_t kValueKindCount = 5U;

// Canonical binary32: negative zero is not a canonical value. The build flags
// (-ffast-math) let the compiler fold float comparisons that would tell -0.0
// from +0.0, so canonicalisation uses integer bit operations only.
[[nodiscard]] inline std::uint32_t canonicalRealBits(const float value) noexcept {
  std::uint32_t bits = 0U;
  std::memcpy(&bits, &value, sizeof(bits));
  return (bits & 0x7FFFFFFFU) == 0U ? 0U : bits;
}
[[nodiscard]] inline float canonicalReal(const float value) noexcept {
  const std::uint32_t bits = canonicalRealBits(value);
  float out = 0.0F;
  std::memcpy(&out, &bits, sizeof(out));
  return out;
}

// A typed canonical value. `bits` holds the binary32 bit pattern for kReal,
// the two's-complement pattern for kInteger and the unsigned value otherwise.
// Canonical encoding (8 bytes):
//   [0] u8 kind  [1..3] reserved, zero  [4..7] u32 bits (LE)
struct TypedValue final {
  ValueKind kind{ValueKind::kNone};
  std::uint32_t bits{0U};

  [[nodiscard]] static constexpr TypedValue none() noexcept { return {}; }
  [[nodiscard]] static constexpr TypedValue integer(const std::int32_t v) noexcept {
    return {ValueKind::kInteger, static_cast<std::uint32_t>(v)};
  }
  [[nodiscard]] static constexpr TypedValue boolean(const bool v) noexcept {
    return {ValueKind::kBoolean, v ? 1U : 0U};
  }
  [[nodiscard]] static constexpr TypedValue enumeration(const std::uint16_t v) noexcept {
    return {ValueKind::kEnum, v};
  }
  // Canonicalises -0.0 to +0.0 (integer bit operations).
  [[nodiscard]] static TypedValue real(const float v) noexcept {
    return {ValueKind::kReal, canonicalRealBits(v)};
  }
  [[nodiscard]] float asReal() const noexcept {
    float v = 0.0F;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
  }
  [[nodiscard]] std::int32_t asInteger() const noexcept {
    return static_cast<std::int32_t>(bits);
  }
  // Numeric view for Real/Integer/Boolean/Enum (used by binding maths and
  // response dynamics). kNone yields 0.
  [[nodiscard]] double asNumber() const noexcept {
    switch (kind) {
      case ValueKind::kReal:
        return static_cast<double>(asReal());
      case ValueKind::kInteger:
        return static_cast<double>(asInteger());
      case ValueKind::kBoolean:
      case ValueKind::kEnum:
        return static_cast<double>(bits);
      case ValueKind::kNone:
        break;
    }
    return 0.0;
  }
};

[[nodiscard]] constexpr bool operator==(const TypedValue& a,
                                        const TypedValue& b) noexcept {
  return a.kind == b.kind && a.bits == b.bits;
}
[[nodiscard]] constexpr bool operator!=(const TypedValue& a,
                                        const TypedValue& b) noexcept {
  return !(a == b);
}

// ---------------------------------------------------------------------------
// Scope and targets
// ---------------------------------------------------------------------------

enum class Channel : std::uint8_t {
  kPrimary = 0U,    // Primary / Top (v1 Target::kPrimary)
  kSecondary = 1U,  // Secondary / Bottom (v1 Target::kSecondary)
  kGlobal = 2U,     // global scope (master, blackout, electrical limit)
};
inline constexpr std::uint8_t kChannelEnumCount = 3U;

enum class Scope : std::uint8_t { kChannel = 0U, kGlobal = 1U };

// A canonical target: stable semantic plus channel (or global). Encoding
// (4 bytes): [0..1] u16 semantic_id (LE)  [2] u8 channel  [3] reserved, zero.
struct TargetRef final {
  std::uint16_t semantic_id{0U};
  Channel channel{Channel::kPrimary};
};
[[nodiscard]] constexpr bool operator==(const TargetRef& a,
                                        const TargetRef& b) noexcept {
  return a.semantic_id == b.semantic_id && a.channel == b.channel;
}
[[nodiscard]] constexpr bool operator!=(const TargetRef& a,
                                        const TargetRef& b) noexcept {
  return !(a == b);
}
// 16-bit target key used by transport lane headers:
//   bits 0..11 semantic_id, bits 12..13 channel, bits 14..15 zero.
[[nodiscard]] constexpr std::uint16_t targetKey(const TargetRef& t) noexcept {
  return static_cast<std::uint16_t>(
      (t.semantic_id & 0x0FFFU) |
      (static_cast<std::uint16_t>(static_cast<std::uint8_t>(t.channel) & 0x03U) << 12U));
}

// Canonical ordering used by snapshots and capability lists.
[[nodiscard]] constexpr bool targetLess(const TargetRef& a,
                                        const TargetRef& b) noexcept {
  return a.semantic_id != b.semantic_id
             ? a.semantic_id < b.semantic_id
             : static_cast<std::uint8_t>(a.channel) <
                   static_cast<std::uint8_t>(b.channel);
}

// ---------------------------------------------------------------------------
// Descriptor vocabularies
// ---------------------------------------------------------------------------

enum class Unit : std::uint8_t {
  kNone = 0U,
  kFraction = 1U,         // 0..1 of a declared span
  kBipolarFraction = 2U,  // -1..1
  kRatio = 3U,
  kPixels = 4U,
  kPixelsPerSecond = 5U,
  kSeconds = 6U,
  kMilliseconds = 7U,
  kQ0_16 = 8U,     // unsigned 16-bit fraction of full scale
  kCc14 = 9U,      // legacy 14-bit controller value 0..16383
  kMilliamps = 10U,
  kEnumIndex = 11U,
  kBoolean = 12U,
};
inline constexpr std::uint8_t kUnitCount = 13U;

enum class Formatter : std::uint8_t {
  kRaw = 0U,
  kPercent = 1U,
  kSignedPercent = 2U,
  kDecimal2 = 3U,
  kSeconds = 4U,
  kPixels = 5U,
  kPixelsPerSecond = 6U,
  kOnOff = 7U,
  kEnumLabel = 8U,
  kQ0_16Percent = 9U,
  kCc14Percent = 10U,
  kMilliamps = 11U,
  kRatio = 12U,
};
inline constexpr std::uint8_t kFormatterCount = 13U;

// Who owns the time response of a parameter's effective value.
enum class ResponseOwner : std::uint8_t {
  kImmediate = 0U,   // discrete or authority value; effective == base at once
  kEngine = 1U,      // engine response dynamics (direct/exponential/slew)
  kModeAdapter = 2U, // a versioned mode adapter owns a declared dynamic
};
inline constexpr std::uint8_t kResponseOwnerCount = 3U;

// Numeric encoding admitted for a parameter's canonical value, with its
// declared error bound carried in the descriptor.
enum class NumericEncoding : std::uint8_t {
  kFloat32 = 0U,
  kInt32 = 1U,
  kUInt16Q0_16 = 2U,
  kCc14 = 3U,
  kBoolean = 4U,
  kEnumU16 = 5U,
};
inline constexpr std::uint8_t kNumericEncodingCount = 6U;

enum class Lifecycle : std::uint8_t {
  kProposed = 0U,
  kFrozen = 1U,
  kDeprecated = 2U,
  kUnadvertised = 3U,  // semantic reserved to keep meanings distinct; not offered
};
inline constexpr std::uint8_t kLifecycleCount = 4U;

enum class MigrationPolicy : std::uint8_t {
  kNone = 0U,
  kLegacyPreserved = 1U,         // legacy value keeps legacy meaning
  kNamedMigrationRequired = 2U,  // only an explicit named migration may map it
  kDefaultForNewField = 3U,      // absent in older data: documented default
};
inline constexpr std::uint8_t kMigrationPolicyCount = 4U;

// ParameterDescriptor.flags
inline constexpr std::uint16_t kParamFlagEnable = 1U << 0U;          // boolean feature enable
inline constexpr std::uint16_t kParamFlagLegacyAlias = 1U << 1U;     // exact v1 meaning
inline constexpr std::uint16_t kParamFlagLogDomainLegal = 1U << 2U;  // legal min > 0
inline constexpr std::uint16_t kParamFlagRelativeAdmitted = 1U << 3U;
inline constexpr std::uint16_t kParamFlagSparseEnum = 1U << 4U;      // legal subset table
inline constexpr std::uint16_t kParamFlagOutputAuthority = 1U << 5U; // master/blackout/limit
inline constexpr std::uint16_t kParamFlagsDefined = 0x003FU;

enum class FeatureOwner : std::uint8_t {
  kVp = 1U,
  kAp = 2U,
  kMod = 3U,
  kCtl = 4U,
};
inline constexpr std::uint8_t kFeatureOwnerMin = 1U;
inline constexpr std::uint8_t kFeatureOwnerMax = 4U;

enum class BypassPolicy : std::uint8_t {
  kIdentity = 0U,       // bypassed stage passes its input unchanged
  kHoldLast = 1U,       // bypassed stage freezes its last output
  kReleaseToBase = 2U,  // bypassed stage releases to base over its lifecycle
};
inline constexpr std::uint8_t kBypassPolicyCount = 3U;

enum class ReenablePolicy : std::uint8_t {
  kResumeFromBase = 0U,
  kReinitialise = 1U,
};
inline constexpr std::uint8_t kReenablePolicyCount = 2U;

// FeatureDescriptor.reset_causes
inline constexpr std::uint16_t kResetOnModeChange = 1U << 0U;
inline constexpr std::uint16_t kResetOnPaletteChange = 1U << 1U;
inline constexpr std::uint16_t kResetOnEngineEpoch = 1U << 2U;
inline constexpr std::uint16_t kResetOnProfileCommit = 1U << 3U;
inline constexpr std::uint16_t kResetOnBlackout = 1U << 4U;
inline constexpr std::uint16_t kResetOnExplicitReset = 1U << 5U;
inline constexpr std::uint16_t kResetCausesDefined = 0x003FU;

enum class Visualiser : std::uint8_t {
  kPalettePath = 1U,
  kColourChange = 2U,
  kTransferCurve = 3U,
  kTrajectory = 4U,
  kDecayEnvelope = 5U,
  kRhythmEvidence = 6U,
  kResponseTrace = 7U,
  kLiveinessModulation = 8U,
  kOutputPower = 9U,
};
inline constexpr std::uint8_t kVisualiserMin = 1U;
inline constexpr std::uint8_t kVisualiserMax = 9U;

enum class PreviewBasis : std::uint8_t { kIllustrative = 0U, kMeasured = 1U };

// ---------------------------------------------------------------------------
// Platforms and capability vocabularies
// ---------------------------------------------------------------------------

enum class Platform : std::uint8_t {
  kRt1062 = 1U,       // DualMCU real-time engine (AP+VP) behind the S3 radio bridge
  kTitan = 2U,        // RA8P1 Titan Mini
  kStandaloneS3 = 3U, // standalone S3 engine
  kConsole = 4U,      // K1.Sliders console (profile owner, not an engine)
};
inline constexpr std::uint8_t kPlatformMin = 1U;
inline constexpr std::uint8_t kPlatformMax = 4U;

enum class CapacityStatus : std::uint8_t {
  kProposed = 0U,
  kDerived = 1U,
  kMeasured = 2U,
};
inline constexpr std::uint8_t kCapacityStatusCount = 3U;

enum class Support : std::uint8_t {
  kImplemented = 0U,  // a consumer exists on this platform
  kPlanned = 1U,      // declared; no consumer yet (visible inactive reason)
  kUnsupported = 2U,  // this platform cannot offer it
  kUnadvertised = 3U, // semantic reserved, not offered anywhere
  kNotApplicable = 4U,// platform is not an engine for this semantic
};
inline constexpr std::uint8_t kSupportCount = 5U;

enum class InactiveReason : std::uint8_t {
  kNone = 0U,
  kFeatureDisabled = 1U,
  kModeUnsupported = 2U,
  kPlatformPlanned = 3U,
  kPlatformUnsupported = 4U,
  kUnadvertised = 5U,
  kNoConsumer = 6U,
  kBlackoutActive = 7U,
  kDependencyInactive = 8U,
  kNotApplicable = 9U,
};
inline constexpr std::uint8_t kInactiveReasonCount = 10U;

// ControlCapabilities bitmasks
inline constexpr std::uint8_t kEncodingMaskDefined = 0x3FU;  // one bit per NumericEncoding
inline constexpr std::uint8_t kModalityMaskAbsolute = 1U << 0U;
inline constexpr std::uint8_t kModalityMaskRelative = 1U << 1U;
inline constexpr std::uint8_t kModalityMaskDefined = 0x03U;
inline constexpr std::uint8_t kTelemetryMaskEffective = 1U << 0U;
inline constexpr std::uint8_t kTelemetryMaskMaterial = 1U << 1U;
inline constexpr std::uint8_t kTelemetryMaskDefined = 0x03U;
// Declared services and routes. Unsupported ones are explicit zeros.
inline constexpr std::uint8_t kServiceProfileTransfer = 1U << 0U;  // engine accepts a staged CompiledProfile
inline constexpr std::uint8_t kServicePersistence = 1U << 1U;      // a persistence store is bound
inline constexpr std::uint8_t kServiceRouteBle = 1U << 2U;         // BLE route to the console
inline constexpr std::uint8_t kServiceRouteFixture = 1U << 3U;     // host USB-CDC fixture route
inline constexpr std::uint8_t kServiceRouteLocal = 1U << 4U;       // in-process local control
inline constexpr std::uint8_t kServiceMaskDefined = 0x1FU;

// ---------------------------------------------------------------------------
// Profile vocabularies
// ---------------------------------------------------------------------------

enum class PageKind : std::uint8_t { kPerform = 0U, kColour = 1U, kCustom = 2U };
inline constexpr std::uint8_t kPageKindCount = 3U;

enum class MappingKind : std::uint8_t { kLinear = 0U, kLog = 1U, kExponent = 2U };
inline constexpr std::uint8_t kMappingKindCount = 3U;

enum class TakeoverMode : std::uint8_t { kPickup = 0U, kJump = 1U, kRange = 2U };
inline constexpr std::uint8_t kTakeoverModeCount = 3U;

enum class ResponseKind : std::uint8_t { kDirect = 0U, kExponential = 1U, kSlew = 2U };
inline constexpr std::uint8_t kResponseKindCount = 3U;

// Response dynamics are engine-owned. Bindings and operations name one
// admitted policy from the registry's response-policy catalogue by id; they
// never carry free-form smoothing. Id 0 asks for the descriptor default
// (direct); capability masks admit policies by id (bit n = policy id n).
inline constexpr std::uint8_t kResponsePolicyDescriptorDefault = 0U;
inline constexpr std::uint8_t kResponsePolicyDirect = 1U;
inline constexpr std::uint8_t kMaxResponsePolicies = 32U;

// CompiledBinding.flags
inline constexpr std::uint8_t kBindingFlagUnused = 1U << 0U;
inline constexpr std::uint8_t kBindingFlagInverted = 1U << 1U;
inline constexpr std::uint8_t kBindingFlagRelative = 1U << 2U;
inline constexpr std::uint8_t kBindingFlagsDefined = 0x07U;

// Declared bounds of binding parameters (profile schema v2).
inline constexpr float kExponentMin = 0.1F;
inline constexpr float kExponentMax = 10.0F;
inline constexpr std::uint32_t kInputStepsMin = 2U;
inline constexpr std::uint32_t kInputStepsMax = 65536U;
inline constexpr float kHysteresisMax = 0.1F;
inline constexpr float kPickupToleranceMax = 0.05F;
inline constexpr float kSlewRateMax = 100000.0F;  // relative max rate bound
inline constexpr float kRelativeGainMax = 16.0F;
inline constexpr float kRelativeAccelExponentMax = 4.0F;
inline constexpr float kRelativeReferenceVelocityMax = 1000.0F;
inline constexpr float kRelativeMaxMultiplierMax = 64.0F;

// ---------------------------------------------------------------------------
// Intents, transactions and receipts
// ---------------------------------------------------------------------------

enum class IntentKind : std::uint8_t { kAbsolute = 0U, kRelativeDelta = 1U };
inline constexpr std::uint8_t kIntentFlagTerminal = 1U << 0U;
inline constexpr std::uint8_t kIntentFlagsDefined = 0x01U;

enum class TransactionSource : std::uint8_t {
  kGesture = 0U,
  kEditor = 1U,
  kPresetRecall = 2U,
  kAutomation = 3U,
  kBlackout = 4U,
  kExternalSync = 5U,
};
inline constexpr std::uint8_t kTransactionSourceCount = 6U;
inline constexpr std::uint8_t kTxnFlagOverrideLeases = 1U << 0U;
inline constexpr std::uint8_t kTxnFlagsDefined = 0x01U;

enum class Atomicity : std::uint8_t {
  kSingleTarget = 0U,
  kChannel = 1U,         // every op on the same channel
  kLinkedChannels = 2U,  // explicit A+B linked edit
  kGlobal = 3U,          // global targets only
};
inline constexpr std::uint8_t kAtomicityCount = 4U;

enum class ApplyTiming : std::uint8_t { kNextSafeBoundary = 0U };

enum class OpKind : std::uint8_t { kSetAbsolute = 0U, kApplyDelta = 1U };
inline constexpr std::uint8_t kOpKindCount = 2U;
inline constexpr std::uint8_t kOpFlagTerminal = 1U << 0U;
inline constexpr std::uint8_t kOpFlagCheckExpectedRevision = 1U << 1U;
inline constexpr std::uint8_t kOpFlagsDefined = 0x03U;

enum class Outcome : std::uint8_t {
  kAccepted = 0U,
  kRejected = 1U,
  kSuperseded = 2U,
  kExpired = 3U,
  kUnavailable = 4U,
};
inline constexpr std::uint8_t kOutcomeCount = 5U;

enum class Reason : std::uint8_t {
  kNone = 0U,
  kMalformed = 1U,
  kEpochMismatch = 2U,
  kStaleSession = 3U,
  kStaleSequence = 4U,
  kIdentityReuse = 5U,
  kProfileRevisionMismatch = 6U,
  kUnknownTarget = 7U,
  kScopeMismatch = 8U,
  kTypeMismatch = 9U,
  kOutOfDomain = 10U,
  kNonFinite = 11U,
  kRevisionConflict = 12U,
  kLeaseHeld = 13U,
  kLeaseSuperseded = 14U,
  kDeadlineExpired = 15U,
  kDuplicateWindowExpired = 16U,
  kUnsupportedOnPlatform = 17U,
  kBindingContract = 18U,
  kAtomicityViolation = 19U,
  kCapacityExceeded = 20U,
  kEpochExhausted = 21U,
  kResponsePolicyNotAdmitted = 22U,
  kDeltaNotAdmitted = 23U,
  kDuplicateTarget = 24U,
  kExpectedRevisionRequired = 25U,
  kTransactionAborted = 26U,
  kProfileRetired = 27U,
  kUnadvertisedTarget = 28U,
};
inline constexpr std::uint8_t kReasonCount = 29U;

// ReceiptOpResult.flags / SnapshotEntry.flags
inline constexpr std::uint8_t kStateFlagActive = 1U << 0U;
inline constexpr std::uint8_t kStateFlagLeaseHeld = 1U << 1U;
inline constexpr std::uint8_t kStateFlagsDefined = 0x03U;

// ---------------------------------------------------------------------------
// Observations
// ---------------------------------------------------------------------------

enum class TimeDomain : std::uint8_t { kEngineMonotonicUs = 1U };

enum class Availability : std::uint8_t {
  kAvailable = 0U,
  kStale = 1U,
  kUnavailable = 2U,
  kUnsupported = 3U,
};
inline constexpr std::uint8_t kAvailabilityCount = 4U;

inline constexpr std::uint16_t kModulationResponse = 1U << 0U;
inline constexpr std::uint16_t kModulationLiveiness = 1U << 1U;
inline constexpr std::uint16_t kModulationAudio = 1U << 2U;
inline constexpr std::uint16_t kModulationOutputLimit = 1U << 3U;
inline constexpr std::uint16_t kModulationBlackout = 1U << 4U;
inline constexpr std::uint16_t kModulationDefined = 0x001FU;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

[[nodiscard]] constexpr bool isParameterId(const std::uint16_t id) noexcept {
  return id >= kParameterIdMin && id <= kParameterIdMax;
}
[[nodiscard]] constexpr bool isFeatureId(const std::uint16_t id) noexcept {
  return id >= kFeatureIdMin && id <= kFeatureIdMax;
}
[[nodiscard]] constexpr bool isContextId(const std::uint16_t id) noexcept {
  return id >= kContextIdMin && id <= kContextIdMax;
}

}  // namespace k1::control_v2
