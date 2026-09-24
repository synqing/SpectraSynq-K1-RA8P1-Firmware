#pragma once

// Spectrum family Liveiness adapters: SPECTRUM_RIVER (14), SPECTRUM_RIVER_V2
// (15), DENSE_FORGE (21), DENSE_FORGE_CHORD (24), CHROMA_CONSTELLATION (25).
//
// Frequency ordering, injection gain, chroma identity and chord evidence are
// never touched: Liveiness changes only how the display moves.

#include <array>

#include "core/visual/modes/liveiness_contract.h"

namespace k1::core::visual::modes {

inline constexpr std::array<LiveinessAdapterDescriptor, 5U>
    kSpectrumLiveinessAdapters{{
        // Hook: renderSpectrumRiver, `drift` (constant 0.6875 px/frame for
        // mode 14; tide-scaled 0.619..1.2375 px/frame for mode 15).
        {14U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kSpectrum,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         0.15F, 2.5F, "spectrum_river.drift", "px_per_nominal_frame",
         "liveiness.spectrum.river.flow",
         LiveinessAcceptance::kHostPass, ""},
        {15U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kSpectrum,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         0.15F, 3.0F, "spectrum_river_v2.drift", "px_per_nominal_frame",
         "liveiness.spectrum.river_v2.flow",
         LiveinessAcceptance::kHostPass, ""},
        // DENSE_FORGE / DENSE_FORGE_CHORD, candidate v4 (final; ORCH: no v5):
        // excursion of the moving interference texture. Hook:
        // renderDenseForge, contrast = effective (base 1.0) multiplies only
        // the 0.48 interference term of the field level; the spectrum term
        // (tone evidence), lattice, carrier, trail transport, activity
        // envelope and chord hold are untouched. Rationale: the moving
        // interference drives ~90% of 21's frame activity at neutral
        // (carrier frozen 183 -> 17), but speeding it only blurs through
        // the 0.9 trail memory; its excursion is not blurred away.
        // Candidates v1 (carrier rate), v2 (lattice + carrier clock) and v3
        // (local time scale) FAILED their registered metric; receipts are in
        // mode_liveiness_v1.json.
        {21U, 4U, LiveinessSupport::kSupported, LiveinessFamily::kSpectrum,
         LiveinessDimension::kExcursion,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         0.5F, 2.0F, "dense_forge.interference_contrast",
         "multiple_of_legacy_contrast",
         "liveiness.spectrum.dense_forge.flow",
         LiveinessAcceptance::kHostPass, ""},
        {24U, 4U, LiveinessSupport::kSupported, LiveinessFamily::kSpectrum,
         LiveinessDimension::kExcursion,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kSpectrum, 1.0F,
         0.5F, 2.0F, "dense_forge_chord.interference_contrast",
         "multiple_of_legacy_contrast",
         "liveiness.spectrum.dense_forge_chord.flow",
         LiveinessAcceptance::kHostPass, ""},
        // CHROMA_CONSTELLATION: outward drift of the star trails. Star
        // positions (circle-of-fifths rank) and the chroma smoothing are
        // untouched. Base range 0.175..0.4375 px/frame.
        {25U, 1U, LiveinessSupport::kSupported, LiveinessFamily::kSpectrum,
         LiveinessDimension::kTransportRate,
         LiveinessObjectPolicy::kFieldContinuous, evidence::kChroma, 1.0F,
         0.05F, 1.0F, "constellation.drift", "px_per_nominal_frame",
         "liveiness.spectrum.constellation.drift",
         LiveinessAcceptance::kHostPass, ""},
    }};

}  // namespace k1::core::visual::modes
