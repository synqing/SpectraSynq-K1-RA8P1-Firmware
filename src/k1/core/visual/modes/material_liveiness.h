#pragma once

// Material family Liveiness adapters: BLOOM (3), BLOOM_FAST (9), AURORA (12),
// COMET (13), EMBER (16).
//
// Every row routes one renderer quantity through resolveLiveiness(). The
// legacy Mood law still computes the base where the renderer already uses it;
// Liveiness multiplies that base and never replaces it. Bounds are in the
// renderer's own units. Rows marked kPending are registered candidates whose
// adapters are not admitted yet: they resolve to exact identity and report
// kUnsupportedMode until their fixtures pass.

#include <array>

#include "core/visual/modes/liveiness_contract.h"

namespace k1::core::visual::modes {

inline constexpr std::array<LiveinessAdapterDescriptor, 5U>
    kMaterialLiveinessAdapters{{
        // BLOOM: outward transport of the injected RGB history.
        // Hook: renderBloomFamily, `propagation` after the shift-scale clamp.
        // Base range (Mood 0..1, shift 0.25..2): 0.078..5.0 px/frame.
        // Preserves centre injection at 79/80, authored history, Mood law.
        // Acceptance (ORCH): wired, NOT accepted - the registered 10% cadence
        // ratio fails on the legacy Pixel8 transport (reference-bound: the
        // output is byte-identical to the equivalent legacy base change).
        // The same criterion is re-run on the wide Bloom route.
        {3U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kMaterial,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kChroma, 1.0F,
         0.05F, 6.0F, "bloom.propagation", "px_per_nominal_frame",
         "liveiness.material.bloom.transport",
         LiveinessAcceptance::kFailReferenceBound,
         "cadence_legacy_transport_reference_bound"},
        // BLOOM_FAST: same hook; its 2x identity stays inside the base.
        // Base range 0.156..10.0 px/frame.
        {9U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kMaterial,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kChroma, 1.0F,
         0.10F, 12.0F, "bloom_fast.propagation", "px_per_nominal_frame",
         "liveiness.material.bloom_fast.transport",
         LiveinessAcceptance::kHostPass, ""},
        // AURORA: flow rate inside the auroral law (alpha 0.98, 6 px fade).
        // Base range 0.25..5.0 px/frame.
        {12U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kMaterial,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kChroma, 1.0F,
         0.10F, 6.0F, "aurora.propagation", "px_per_nominal_frame",
         "liveiness.material.aurora.flow",
         LiveinessAcceptance::kHostPass, ""},
        // COMET: velocity of every live comet, applied where the renderer
        // integrates position, so a comet already in flight responds from its
        // current position (no teleport, no extra onset). The spawn-time Mood
        // law still fixes each comet's stored base velocity.
        // Base range 0.75..4.25 px/frame.
        {13U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kMaterial,
         LiveinessDimension::kObjectVelocity,
         LiveinessObjectPolicy::kCurrentObjectsReanchored,
         evidence::kOnsetEvents, 1.0F, 0.30F, 8.0F, "comet.velocity",
         "px_per_nominal_frame", "liveiness.material.comet.velocity",
         LiveinessAcceptance::kHostPass, ""},
        // EMBER: ember drift (history transport); reach, gain and the sparse
        // decay (alpha 0.88) are untouched. Base range 0.394..0.731 px/frame.
        {16U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kMaterial,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         0.15F, 1.5F, "ember.drift", "px_per_nominal_frame",
         "liveiness.material.ember.drift",
         LiveinessAcceptance::kHostPass, ""},
    }};

}  // namespace k1::core::visual::modes
