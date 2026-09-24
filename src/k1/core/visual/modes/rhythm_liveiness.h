#pragma once

// Rhythm family Liveiness adapters: WAVEFORM_TEMPO (18), TEMPO_RIVER (19),
// TEMPO_COMET (20), PULSE_PRISM (23), PERCUSSION_BURST (26),
// TEMPO_COMET_ANTICIPATE (27), RIVER_SURGE (28), TEMPO_RIVER_WALK (29).
//
// Tempo evidence is read-only input. No adapter touches the AP BPM, phase,
// confidence, event IDs or timestamps, nor the renderer's flywheel, beat
// counting or spawn schedule: Liveiness scales visual excursion per observed
// or derived event (pixels per beat, object velocity, spawn reach) only.

#include <array>

#include "core/visual/modes/liveiness_contract.h"

namespace k1::core::visual::modes {

inline constexpr std::array<LiveinessAdapterDescriptor, 8U>
    kRhythmLiveinessAdapters{{
        // Hook: renderWaveformTempo, transport velocity (pixels per beat x
        // BPM/60 x phase envelope, gated by confidence). A stopped idle in
        // silence is a zero base (kBaseZero).
        {18U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         4.0F, 240.0F, "waveform_tempo.velocity", "px_per_second",
         "liveiness.rhythm.waveform_tempo.excursion",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderTempoRiver, velocity before the rhythm law's own
        // floor/ceiling (45..450 px/s), which are also the adapter bounds.
        {19U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         45.0F, 450.0F, "tempo_river.velocity", "px_per_second",
         "liveiness.rhythm.tempo_river.excursion",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: drawTempoComet, velocity at the integration step, so comets
        // in flight re-anchor. Spawn timing stays on the flywheel beat.
        {20U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kObjectVelocity,
         LiveinessObjectPolicy::kCurrentObjectsReanchored,
         evidence::kTempoLock, 1.0F, 10.0F, 400.0F, "tempo_comet.velocity",
         "px_per_second", "liveiness.rhythm.tempo_comet.reach",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderPulsePrism, ring expansion velocity at integration.
        // Ring life, bed glow and spawn on kick/transient are untouched.
        {23U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kObjectVelocity,
         LiveinessObjectPolicy::kCurrentObjectsReanchored,
         evidence::kOnsetEvents, 1.0F, 15.0F, 160.0F,
         "pulse_prism.ring_velocity", "px_per_second",
         "liveiness.rhythm.pulse_prism.excursion",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderPercussionBurst, signed particle velocity at
        // integration (the snare's negative direction is preserved). Pool
        // size, class, life and timestamps are untouched.
        {26U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kObjectVelocity,
         LiveinessObjectPolicy::kCurrentObjectsReanchored,
         evidence::kOnsetEvents, 1.0F, 0.2F, 4.5F, "percussion.velocity",
         "half_strips_per_second", "liveiness.rhythm.percussion.spread",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderTempoCometAnticipate, spawn target (reach). Position is
        // a closed-form ease of the stored target, so live comets keep their
        // spawn target and only future comets use the current amount.
        {27U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kObjectReach,
         LiveinessObjectPolicy::kFutureObjectsOnly, evidence::kTempoLock,
         1.0F, 12.0F, 80.0F, "anticipate.target", "px",
         "liveiness.rhythm.anticipate.reach_next",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderSpectrumRiver, `drift` for mode 28; the surge wavefront
        // speed follows the same drift. The build/drop envelopes, refractory
        // period and quiet recovery are untouched.
        {28U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         0.15F, 4.0F, "river_surge.drift", "px_per_nominal_frame",
         "liveiness.rhythm.river_surge.excursion",
         LiveinessAcceptance::kHostPass, ""},
        // Hook: renderTempoRiverWalk, velocity before the law's 45..450 px/s
        // clamp. Beat counting, parity and the palette walk are untouched.
        {29U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kRhythm,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         45.0F, 450.0F, "tempo_river_walk.velocity", "px_per_second",
         "liveiness.rhythm.tempo_river_walk.excursion",
         LiveinessAcceptance::kHostPass, ""},
    }};

}  // namespace k1::core::visual::modes
