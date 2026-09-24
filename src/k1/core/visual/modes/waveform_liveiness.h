#pragma once

// Waveform family Liveiness adapters: WAVEFORM_FAST (7), WAVEFORM (8),
// WAVEFORM_HYBRID (11), SNAPWAVE (22), WAVEFORM_HYBRID_K1 (32).
//
// The waveform deposit (sample order, zero crossings, prompt amplitude) is
// never touched. Liveiness moves the presentation: how fast deposited history
// travels outward from the centre, or (Snapwave) how far the snap reaches.

#include <array>

#include "core/visual/modes/liveiness_contract.h"

namespace k1::core::visual::modes {

inline constexpr std::array<LiveinessAdapterDescriptor, 5U>
    kWaveformLiveinessAdapters{{
        // Hook: renderWaveformFamily, `pixels_per_frame` after the legacy
        // shift-rate clamp. Base 0..2 px/frame (shift rate 0..240 px/s);
        // a zero shift rate masks the macro (kBaseZero).
        {7U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kWaveform,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         0.10F, 4.0F, "waveform_fast.scroll", "px_per_nominal_frame",
         "liveiness.waveform.fast.scroll",
         LiveinessAcceptance::kHostPass, ""},
        {8U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kWaveform,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         0.10F, 4.0F, "waveform.scroll", "px_per_nominal_frame",
         "liveiness.waveform.classic.scroll",
         LiveinessAcceptance::kHostPass, ""},
        {11U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kWaveform,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         0.10F, 4.0F, "waveform_hybrid.scroll", "px_per_nominal_frame",
         "liveiness.waveform.hybrid.scroll",
         LiveinessAcceptance::kHostPass, ""},
        // SNAPWAVE: snap excursion. Hook: renderSnapwave, `amplitude` before
        // the dead zone and the authored 80 ms amplitude smoother (which then
        // supplies continuity). The attack follower and kick nudge are
        // untouched. Units: fraction of the half-strip; limit 1.0.
        {22U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kWaveform,
         LiveinessDimension::kExcursion, LiveinessObjectPolicy::kModeSmoothed,
         static_cast<std::uint8_t>(evidence::kAmplitude | evidence::kChroma),
         1.0F, 0.0F, 1.0F, "snapwave.amplitude", "half_strip_fraction",
         "liveiness.waveform.snapwave.excursion",
         LiveinessAcceptance::kHostPass, ""},
        // WAVEFORM_HYBRID_K1: outward scroll of the K1 gesture history.
        // Hook: renderWaveformK1, scroll per nominal frame (405 px/s base);
        // its decay law, dot colour EMA and silence envelope are untouched.
        {32U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kWaveform,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kAmplitude, 1.0F,
         1.0F, 7.0F, "waveform_k1.scroll", "px_per_nominal_frame",
         "liveiness.waveform.k1.scroll",
         LiveinessAcceptance::kHostPass, ""},
    }};

}  // namespace k1::core::visual::modes
