# In-house Waveform migration assessment — 12 October 2026

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Root retains sole live ownership.

## Assignment and disposition

Captain redirected the current recovery work to investigate migration/refactoring
from `/Users/spectrasynq/SpectraSynq_K1_Firmware/SPECTRASYNQ_K1_FIRMWARE`, learning
from known working lightshows with their actual parameters and palettes. Captain
confirmed that Waveform Hybrid K1 is one shortlisted effect; Waveform Tempo and
the other Waveform family effects are also excellent. This assessment preserves
that family scope. It does not nominate Hybrid K1 as the sole desired show.

This is a source/configuration assessment. No firmware edits, deployment, music
playback or new physical comparison were performed for this assessment. Previous
Bloom source changes and failed/inconclusive evidence remain intact. New tuning
stopped at the change of direction. The earlier complete assignments remain in
`CONTINUOUS-BLOOM-PHYSICAL-20261012.md` and the linked historical documents.

Recommendation: migrate the selected family's observable audio, motion and colour
behaviour as a complete unit. First establish its reference configurations. Reuse
Titan's verified capture, independent output ownership, controls and programming
route. Neither the current legacy checkout nor the existing portable mode names
establish fidelity to the valued performances.

## Reference identity and recoverability

Inspected repository root: `/Users/spectrasynq/SpectraSynq_K1_Firmware`.
Current branch `wip/emotiscope-perf`, HEAD
`96e683944f13c64746bd2c54920719b26fcc354e`. Scoped firmware source is clean;
unrelated parent skill/output changes remain untouched. Current source includes
October Emotiscope adaptations. Its default build also enables a Chromatic Score
owner capable of bypassing the legacy path. It is not automatically the remembered
in-house build. Historical lane metadata is not current implementation authority.

The strongest specific recovered colour reference is the August readback:

- Binary: `/Users/spectrasynq/SpectraSynq_K1_Firmware/_scratch/deck16_backend_b1b2_20260808/flash_readback_k1.bin`.
- Size: 973,792 bytes; inspected SHA-256
  `04215afb279af9f63b833a2304d2fda8975db37f9d1cb0cc5d6a2ada814124ad`.
- Reported embedded identity: `db300db`, epoch `1786214500`, environment
  `k1_bench_im69d_ble`; historical target `B489A500`.
- Configuration source: `_scratch/im69d_vs_main_20260805/20260805T201538_im69d_vs_sph_ab/preflight_bench_im69d.log`
  in that repository, lines 30–68.
- Captain's recorded August comparison preferred this binary with era settings;
  the then-main build rendered gold white in modes 18/32 at the same settings.
  Evidence: `docs/forensics/colour-nuance-regression-verdict-2026-08-13.md:19`.
- That document explicitly says the binary included unrecorded dirty work, so
  checking out `db300db` does not reconstruct it. Its quarantine at lines 7–11
  restricts its evidentiary use to visual/configuration reproduction, excluding
  input health, absolute drive and silence qualification.

Recovered settings: mode 32; photons/master 1; chroma/mood .05; mirror on;
chromagram range 60; note offset 12; sensitivity 2.4; response 1;
square iterations 1; saturation/prism 1; incandescent filter .50 but mode off;
base coat off/zero; auto colour on; 12,800 Hz/96 samples. SSL 253 and DC 230 are
historical calibration values, not values to write to Titan.

The dump does **not** establish palette ID/mode, secondary configuration or
EdgeMixer relationship. Current boot palette 40 (Naberius Gold) is not proof of
the historical runtime selection; saved show state can override startup choices.
The July mode-32 7.5–8/10 verdict and its subsequently unaccepted colour polish are
separate evidence, not interchangeable with the August binary.

Named pairing presets in `system/presets.h:48` only select modes and deliberately
retain existing palettes. The actual persistence models are per-channel
`K1EffectPreset` (`control/k1_effect_queue.h:52`, 15 fields, `/PRESETS_V1.BIN`) and
complete `K1ShowState` (`control/k1_show_state.h:34`, both channels, EdgeMixer,
secondary enable and score state, `/SHOW_STATE_V1.BIN`). Reproduction therefore
requires a complete show/configuration record, including knob store and calibration
provenance, rather than a pairing name. Missing values must remain unknown until
recovered or explicitly recorded as a newly authored reference.

## Concrete migration gaps already demonstrated in source

Current persisted ordinals are 7 Waveform Fast, 8 Waveform, 11 Waveform Hybrid,
18 Waveform Tempo and 32 Waveform Hybrid K1 (`system/config_types.h:211–247`).
Ordinal 9 is Bloom Fast. Menu indices can differ; integration must bind names
and persisted IDs explicitly. Preserve the family members' distinct mechanisms:
mode 11's raw-audio-shaped centre seed is not mode 32's amplitude-position dot.

| Boundary | Reference behaviour | Current Titan implication |
|---|---|---|
| Hybrid K1 spatial gesture | `effects/light_mode_waveform_hybrid_k1.cpp:360` positions a two-pixel subpixel dot at centre + 0.7 × smoothed peak × half-width; history travels outward | `src/k1/core/visual/product_effect_renderer.cpp:859` instead calls `depositWaveformSeed`, drawing a centre-local waveform band. This portable implementation is not a faithful replacement |
| Hybrid K1 colour | Chroma contrast/soft knee, peak/VU fallback, amplitude palette walk, held coordinate and RGB smoothing | Portable mode uses generic `injectionColour`; its palette helper additionally attracts high-energy colour toward the brightest stop. Shared names/constants do not establish the same colour occupancy |
| Precision/history | Legacy `CRGB16` contains `SQ15x16`; `finalize_additive_frame` preserves channel ratios and saves history before later display transforms | Portable mode works through `Pixel8`, quantises colour and uses saturating additions. Audit precision and history boundaries before reuse; do not feed display attenuation back into history |
| Geometry | In-house canvas is 160 pixels, centre pair 79/80 | Active donor renders 128 and maps to 160 with nearest-pixel expansion. A 160-pixel reference must retain its own geometry; passing it through the 128-pixel adapter changes width and motion |
| Audio | In-house 80-bin note spectrum, visual pre/post-gate chroma, waveform peak/history, VU and defined silence | Donor publication has 64-bin spectrum, 12 chroma, donor VU and candidate tempo arrays. Equal field names and 0–1 ranges do not mean equal signals |
| State | Reference has explicit per-channel effect fields plus some hidden primary/secondary statics and temporarily swapped globals | Move all required followers, history, palette anchors and event cursors into each renderer's owned state; share only immutable audio observations |
| Runtime selection | Current Titan donor branch admits modes 0–6 and returns before native product rendering (`palette_runtime.cpp:39,300`) | Merely compiling or selecting native mode 32 does not make the active donor path execute it. Integration must reach the current authorised output owner, not switch on the parked native deployment |

The concrete Hybrid K1 temporal constants in the inspected source are 16/23 ms
peak smoothing, 80 ms RGB smoothing, 20/500 ms presence rise/fall, 50/300 ms
silence transition constants, 405 pixels/s wake travel, and decay rate
`0.8 + 3.5*peak` with extra decay during quiet. These describe current source,
not an assertion that the unreconstructed golden binary used every same value.
They illustrate why replacing all audio influence with one brightness envelope
cannot reproduce the effect. The dot reacts, older colour travels, strong sound
shortens the wake, and colour moves on a slower timescale.

### Waveform Tempo has a separate, concrete contract

The compiled source is `effects/light_mode_waveform_tempo.cpp`, not the root
`.wip` file. Its motion consumes BPM, continuous phase (zero at beat) and a
silence-attenuated confidence. It does **not** need beat ticks, a lock Boolean or
beat strength to move. The velocity law is
`24 * BPM/60 * (1 + 0.5*cos(2*pi*phase))`, blended from 30 pixels/s free-running
motion across confidence .30–.60. The unlocked case must remain responsive.

Reference source lines 194–300 retain a fractional shift remainder, execute
integer outward steps (cap 30/frame), fade by `1 - .10*peak` per render with .82
quiet drain, and paint at an amplitude-dependent position. Its presence gate
uses peak/VU plus spectral energy, novelty and silence, and its near-black
colour path has an explicit peak-seeded palette fallback.

The existing portable Titan function (`product_effect_renderer.cpp:1304–1355`)
retains the essential tempo velocity equation but injects at the centre, scales
colour by peak, adds with saturation and uses fractional spatial resampling.
It also converts decay against a nominal 120 Hz and differs in silence/fallback
behaviour. Those changes can alter the visible signature even with identical
BPM. Correct these differences against the selected reference; do not treat
new beat-clock architecture as a prerequisite to this effect.

Surviving mode-18 evidence includes
`docs/forensics/runtime-evidence/2026-06-07-vpab-frame-mode18-1401-music.frames.log`
and its semantic brief in the reference repository. It contains final LED
payloads/timing, but no matched source music or plate footage. The August 9
`_scratch/bench_unit2_waveform_tempo_20260809/RECEIPT.md` records a two-edge mode-18
repair on Unit2; it explicitly leaves glass-character judgement unverified and
its folder contains no binary/video. Neither supplies a complete accepted
palette/parameter configuration. Targeted inspection found no physical video
in the runtime-evidence directory; footage elsewhere has not been ruled out.

## Required audio and colour contract

For each shortlisted effect, trace only fields it actually consumes. Define the
producer, units, frequency coverage, normalisation, time constants, validity and
generation for peak, VU, waveform samples, spectrum, pre-gate chroma, post-gate
chroma, and required rhythm observations. Hybrid K1 needs continuous peak/chroma
behaviour immediately; it must not wait for a beat event or a five-second cycle.

Use one coherent publication per render generation, with explicit timestamps.
The in-house `K1AudioFrame`/sidecar acquisition already describes this contract;
the current donor also publishes a coherent frame. Reuse the mechanism while
making signal semantics explicit. A 64-to-80 array resample does not reproduce
note spacing, window lengths, smoothing or gain. Do not invent missing beat or
chord semantics from donor candidate magnitudes.

Start by comparing the required feature trajectories under identical recorded
input. Where existing Titan observations can reproduce them, use a bounded
adapter. Where they cannot, identify the specific missing producer and port or
replace that producer deliberately. A permanent second full analyser is not the
default solution; nor is forcing every in-house behaviour through donor VU.
No analyser replacement is implemented or authorised by this assessment itself.

Preserve palette stop data, index conventions, interpolation, palette-coordinate
selection, colour smoothing, fallback, reverse/range behaviour and per-edge
relationships together. Named colours alone are insufficient. Test gold/violet
transitions and deliberate dark stops, because retained source history records
white mixing and held single-colour failures. Current Titan palettes are always
enabled; preserve that mandate while reconstructing intended colour behaviour.

Convert frame-count-based smoothing deliberately to the target cadence. The
reference contains both time-correct and per-frame followers. Nominal 12,800/96
audio implies 133⅓ updates/s; donor 12,800/64 implies 200 updates/s. Titan render
is 100 Hz. These are different clocks; do not assume the old observed render
cadence was 120 Hz merely because a helper uses 120 as its reference constant.

## Execution sequence and decisive evidence

1. **Recover and observe the family reference.** Bind each shortlisted mode to
   binary/build, complete primary/secondary settings, palettes and palette
   controls, calibration and audio cue. Use surviving recordings first. If a
   fresh reference run is needed, identify the original compatible device and
   applicable ownership/flash authority; the ESP32 binary cannot run on RA8P1.
   Capture the actual plate and final pixels with the existing tools. Record
   attack movement, wake overlap, colour occupancy and quiet recovery at named
   timestamps. Do not substitute a present default for missing settings.
2. **Freeze the behaviour to reproduce.** Use those observations and traced
   consumers to record the essential motion/colour/audio relationships for each
   family member. Separate preserved signature from demonstrated defects. The
   quarantined old silence implementation is not a required regression.
3. **Implement one member at a time in the authorised Titan path.** Reuse the
   useful state/control/output pieces; repair the demonstrated semantic gaps.
   Preserve 160-pixel centre geometry and per-edge state. Add only controls with
   real consumers and save both boards and their relationship through the
   existing authoring path. Do not port ESP32 tasks, NVS, RMT, Wi-Fi or pins.
4. **Compare before extending.** First check source/reference render behaviour
   with the same frozen feature stream; then check the live audio producer and
   the assembled optical stack with matched music, level, framing, exposure and
   output settings. Feature replay isolates renderer errors; it does not prove
   microphone or physical parity. Test peer independence and complete recall.
   Meet the actual Titan deadlines with capture overhead distinguished. Inspect
   the full recording at playback speed, correct faults, then offer the concrete
   comparison for Captain's preference. Only a successful member enables the
   next family member; no effect catalogue sweep.

Failure criteria remain: missing successive musical changes, uncaused or jerky
motion, colour collapse/white mixing, loss of darker space, continuous room-noise
births, stale replay, cross-board state coupling or timing/output violations.
Do not improve coverage by causing hue twitch, enlarge tails until they erase
attacks, or brighten/dim the whole device to conceal a behavioural mismatch.

## Current leave-state and outstanding prior work

Read-only health on this assessment confirmed UID
`545433931bd25436593630352d068363`, installed build
`037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272`,
autonomous emission ON, approximately 100 frames/s, zero reported LED faults.
The active primary scene is continuous Bloom (scene 3), despite the health
summary's underlying donor selector label Spectrum. Master 255/255, gains .25/.25
and secondary enable remain set. Snapshot:
`_scratch/continuous-bloom-r4/migration-assessment-entry-status.json`.
That host snapshot exposes the selected primary board; its secondary cache is
null and is not an independent fresh secondary-parameter readback.

The last trial left body return strength .25, detail .55 and musical release
160 ms. The .25 trial is **not saved to SQ4**; SQ4/canonical preset retain .55.
Prior physical comparison `return-match-001` is inconclusive for quiet recovery
because received input remained high after the playback stopped. Earlier 60 ms
release trial was rejected. No new aesthetic acceptance is asserted.

Uncommitted repairs and trace instrumentation remain in the source tree. The
later `arm-final2` build completed but was **not deployed**. It includes a quiet
status consistency change beyond the installed image. Preserve this distinction
before any next build. Previous physical work is paused by Captain's direction,
not completed by this report.

## Assessment validation and ownership

Two read-only specialists traced configurations/evidence and audio/platform
dependencies; root inspected the decisive source, historical verdict, era dump,
target render integration and actual runtime status. The binary hash was checked
against the historical reference. No new visual artefact was generated: source
and document inspection, rather than an image, validates this assessment only.
Historical aesthetic verdicts remain attributed to Captain; no new appearance
claim has been made. No generic task registry or new capture framework was made.

Verification command through `workflow_verify` rehashed the surviving binary,
checked the named source/output artefacts and ran scoped `git diff --check`:
exit 0, receipt
`/Users/spectrasynq/.local/state/spectrasynq-agents/verification/beef33f0b99b9d390e40a32d1a72a1d2ce9785a729fb1ed5809c13ce9a46e2d6.json`.
`workflow_accept` returned “Bind an assignment before admission”: this changed
assessment has no applicable bound registry. That is not a firmware or aesthetic
certificate. Original task ledgers were not repurposed. Document/source review
is complete at this scope; reference reconstruction and migration remain work
to execute under the selected reference contract.
