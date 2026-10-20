# Phase 6: WP5 live image admission - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §5 WP5)

<domain>
## Phase Boundary

Build and admit one named integration candidate containing the accepted driver and current intentional live changes. Prerequisite: repaired transmitter admitted at bench scope.

</domain>

<decisions>
## Implementation Decisions

### Boot
- Mode 32 both logical channels, active+emit, no four-second carousel.
- Record actual brightness/palettes/direction. Do not silently substitute historical brightness 255.

### PDM
- SINCDEC 49 / SINCRNG 10, nominal ×16 with loud guard. Need matching target evidence.
- Changing gain is not the default next task.
- Preserve reserve-before-rearm/FILLING and programme/measurement separation.
- Do not silently feed 16 kHz diagnostic to 24 kHz AP or fall back to fixture audio.
- Microphones are LinkMems LMD2718T261-OA1, not IM69D130. U14 LOW/ch2 RISE programme; U13 HIGH/ch0 FALL measurement. Acoustic identity unresolved unless measured.

### ASRC
- Two-sample linear interpolation in `k1_asrc_24k.c`; `pdm_target.c` locks initial measured rate after about two seconds. That is not anti-alias, fidelity, or later-drift proof.
- Historical probes: no bins below 110 Hz; low-bin tone/label disagreement; 160→80 half-time tempo. Reproduce silently against current contract before calling defects.

### Motion / dwell
- `last_live_valid_` at zero time and wrap.
- Five-second dwell rendering uninterrupted.
- Five-second quiet/black tail without history reinit or min-one residuals.
- Travelling structure ≠ centroid of a filling half-strip.
- Centre-origin: 160 px centres 79/80; 128 bench 63/64.

### Cadence
- 45 s combined runner is preflight, not G4 240,000-hop qualification.
- Observer-off/on: dense 20 Hz USB froze emission.

### Claude's Discretion
- Named candidate string, provided all sources/config are bound.
- Whether acoustic identity stays `ACOUSTIC_IDENTITY_UNRESOLVED` if no controlled observation.

</decisions>

<specifics>
## Specific Ideas

Use repaired `run_mode32_real_audio.py --audio ... --build-id ...`. `run_music_baseline.py` must still refuse synthetic room playback. No synthetic computer tones or white noise. No repeated same-song loop by default.

</specifics>

<code_context>
## Existing Code Insights

- `pdm_target.c`, `k1_pdm_sensitivity.h`, `k1_asrc_24k.*`, `fixture_app.cpp`, `palette_runtime.cpp`
- Current AP contract: 24 kHz, 180, 7,500 µs hop, 80 bins, crossover 40

</code_context>

<deferred>
## Deferred Ideas

Product four-lane, K1-L physical latency stamp, NPU/MVE.

</deferred>
