#pragma once

// Feature and parameter descriptors of the wide path, for CTL (schema,
// validation ownership, inactive reasons) and UI (context data). Stable ids
// are never reused with a different meaning; a changed law gets a fresh
// semantic version. State sizes are computed from the real types with sizeof.
//
// Infrastructure rows (transfer, current limit, blackout, quantiser) are not
// artistic toggles: they cannot be bypassed from a fader. Evaluation outputs
// (RGB8 adapters) never feed the native path.

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/visual/wide/wide_types.h"

namespace k1::core::visual::wide {

enum class FeatureScopeV1 : std::uint8_t {
  kLayer = 1U,           // one layer of one channel's graph
  kChannel = 2U,         // one channel's graph
  kPhysicalOutput = 3U,  // endpoint, both channels
  kEvaluationOutput = 4U,
};

enum class BypassKindV1 : std::uint8_t {
  kIdentity = 1U,          // disabled/neutral returns its input exactly
  kZeroContribution = 2U,  // disabled generator contributes exact zero
  kExplicitCut = 3U,       // disabled transition is an immediate cut
  kNotBypassable = 4U,     // infrastructure
};

enum class LifecycleKindV1 : std::uint8_t {
  kStateless = 0U,
  kResetOnDisable = 1U,
  kDeclaredPerInstance = 2U,       // persistence: reset / decay / freeze
  kCutOnMeaningChange = 3U,        // typed history
  kTransitionFromResolved = 4U,    // palette transition
  kAdvanceOnPresentation = 5U,     // RGB8 dither phase
  kReanchorOnEdit = 6U,            // analytic material
};

namespace reset_cause {
inline constexpr std::uint32_t kNone = 0U;
inline constexpr std::uint32_t kDisable = 1U << 0U;
inline constexpr std::uint32_t kMeaningChange = 1U << 1U;
inline constexpr std::uint32_t kInitialise = 1U << 2U;
inline constexpr std::uint32_t kRouteEntrySeed = 1U << 3U;
}  // namespace reset_cause

struct WideFeatureDescriptorV1 final {
  std::uint16_t feature_id;
  std::uint8_t semantic_version;
  const char* name;
  FeatureScopeV1 scope;
  std::uint8_t stage_order;  // graph position; endpoint stages from 100
  ColourDomain input_domain;
  ColourDomain output_domain;
  BypassKindV1 bypass;
  LifecycleKindV1 lifecycle;
  bool artistic_toggle;
  std::uint32_t state_bytes_per_channel;
  std::uint32_t reset_causes;
  // Worst-case transcendental calls (exp/exp2/pow) per channel per frame at
  // the admitted capacity; the host cost fixture checks the counters.
  std::uint32_t worst_transcendentals_per_frame;
};

enum class ParameterBehaviourV1 : std::uint8_t {
  kImmediate = 0U,       // takes effect at the next frame
  kReanchored = 1U,      // live objects re-anchor at the effective time
  kFutureBirths = 2U,    // affects objects born after the edit only
  kTransition = 3U,      // starts a bounded transition
  kCutSelector = 4U,     // selector whose change is a declared cut
};

struct WideParameterDescriptorV1 final {
  std::uint16_t parameter_id;
  std::uint16_t feature_id;
  const char* name;
  const char* unit;
  float minimum;
  float maximum;
  float default_value;
  bool has_neutral;
  float neutral;  // exact-identity value when has_neutral
  ParameterBehaviourV1 behaviour;
};

inline constexpr std::size_t kWideFeatureCount = 19U;
inline constexpr std::size_t kWideParameterCount = 28U;

[[nodiscard]] const std::array<WideFeatureDescriptorV1, kWideFeatureCount>&
wideFeatureDescriptorsV1() noexcept;

[[nodiscard]] const std::array<WideParameterDescriptorV1, kWideParameterCount>&
wideParameterDescriptorsV1() noexcept;

[[nodiscard]] const WideFeatureDescriptorV1* findWideFeatureV1(
    std::uint16_t feature_id) noexcept;

[[nodiscard]] const WideParameterDescriptorV1* findWideParameterV1(
    std::uint16_t parameter_id) noexcept;

}  // namespace k1::core::visual::wide
