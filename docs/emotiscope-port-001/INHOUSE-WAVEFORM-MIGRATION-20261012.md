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


---

## Execution adoption — Captain mandate, 13 October 2026

Captain approved the complete plan below with “Implement the plan.” This section
retains that approved execution brief verbatim; the assessment above remains
historical evidence. In particular, ESP32-S3 hardware is an **absolute last
resort**, not a prerequisite or first reference step. The approved plan below
supersedes conflicting recommendations in the original assessment.

Entry preservation: [source manifest](evidence/waveform-migration-001/entry-manifest.json),
[pre-existing tracked patch](evidence/waveform-migration-001/entry-tracked.patch),
and [reference/recovery manifest](evidence/waveform-migration-001/reference-recovery-manifest.json).
The snapshots preserve pre-existing work, not a firmware acceptance or an
installed-image claim. All migration physical acceptance remains **OPEN**.
Root retains sole live-control/deployment ownership.

<!-- BEGIN APPROVED WAVEFORM EXECUTION BRIEF 20261013 — RETAIN VERBATIM -->
<proposed_plan>
# K1 Waveform migration — complete execution brief

**THE TITAN MUST CONTINUE AUTONOMOUS MUSIC VISUALISATION.** Programming and reset interruptions must be disclosed and kept bounded. Restore autonomous operation when the application returns. One integrating owner controls the physical target.

## 1. Outcome, scope and settled decisions

Deliver the five valued in-house Waveform effects on the existing assembled Titan K1, preserving their recognisable musical response, movement, colour distribution and temporal character. Retain the existing Emotiscope modes and support independent selection on either physical edge, including mixed Waveform/Emotiscope pairs.

The implementation order is fixed:

| Order | Source ID | Effect | Essential distinction |
|---|---:|---|---|
| 1 | 32 | Waveform Hybrid K1 | Smoothed amplitude-position deposit, coloured travelling history, separately smoothed colour and presence |
| 2 | 18 | Waveform Tempo | Amplitude-position deposits with confidence-weighted, tempo-shaped transport |
| 3 | 8 | Waveform | Smoothed amplitude-position dot with its characteristic transport, decay and colour transfer |
| 4 | 7 | Waveform Fast | Raw-peak placement and faster response, with its own admission and decay behaviour |
| 5 | 11 | Waveform Hybrid | PCM-shaped centre seed and sustained travelling material |

**Source ID 9 is Bloom Fast and is not part of this migration.**

The following decisions govern every phase:

- Establish faithful behaviour first. Correct demonstrated faults; assess enhancements separately.
- Existing Titan firmware is evaluation code. Reuse useful mechanisms, but do not preserve an inferior implementation merely because it already runs.
- Compare in-house and Emotiscope audio mechanisms. Pursue responsiveness, colour precision and useful musical analysis together.
- Increased response delay requires a demonstrated overall physical improvement and Captain’s explicit acceptance of that trade-off.
- Preserve every relevant control with an actual consumer. Do not expose stored-but-inert settings as working controls.
- Use the existing Surface and host persistence. Do not introduce another dashboard.
- Support a complete two-edge composition, including palettes, controls and relationships.
- Embed a fixed boot composition selected after physical comparison. Other compositions remain host-persisted; host persistence must not be described as device NVM.
- Measured software gamma/correction changes are permitted as separate experiments with exact rollback. Electrical limits, wiring and operating brightness remain protected.
- Preserve original assignments, failed evidence and historical verdicts. The migration does not retrospectively complete the earlier Bloom work.
- New choreography, effect catalogues, ML targets, model acquisition and unrelated platform work remain outside this assignment.

**The ESP32-S3 K1 is an absolute last resort.** It is not required to begin implementation, establish a reconstructed reference, or perform Titan physical comparisons.

The brief defines how evidence-dependent decisions are resolved. It does not claim that unknown historical settings, future timing results or physical preference have already been established.

## 2. Working locations, ownership and protected state

### M00 — Bind the execution to the correct sources

**Purpose:** prevent work on the wrong renderer, checkout or target.

**Implementation destination**

`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`

Branch: `feat/titan-emotiscope-port-001`

**Reference repository**

`/Users/spectrasynq/SpectraSynq_K1_Firmware`

Reference firmware subdirectory: `SPECTRASYNQ_K1_FIRMWARE`

**Existing authoring application**

`/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001`

**Actions**

1. Read current global/local instructions, product charter and the complete applicable handovers.
2. Reconcile the migration assessment with this brief by appending the complete execution assignment; preserve its original assessment and the earlier Bloom assignments.
3. Re-read HEAD, branch, tracked changes and relevant untracked source in each affected repository.
4. Record which existing changes are installed, which are uninstalled repairs, and which belong to unrelated work.
5. Use the existing worktrees. Do not create sibling repositories, reset user work or silently repin shared Core sources.
6. Root remains the sole live-control and deployment owner. Any specialists receive bounded source/test ownership under this same brief.

**Required output:** a source manifest containing repository identities, relevant dirty patches, imported-source identities and ownership.

**Pass:** every compiled source input can be traced to its exact file content.

**Fail/repair:** an unexplained changed input blocks building a release from that input, not unrelated source inspection. Resolve its ownership or preserve it explicitly in the candidate manifest.

### M01 — Preserve the actual operating baseline

**Purpose:** make comparisons and rollback refer to the running system rather than an older prompt.

The previously observed state is a starting lead, to be freshly verified:

| Item | Previously observed identity |
|---|---|
| Titan UID | `545433931bd25436593630352d068363` |
| Titan source HEAD | `f4ce313fd50f8cfc7b7858adbc887d9420a989a0` plus existing changes |
| Installed build | `037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272` |
| Installed image directory | `_scratch/continuous-bloom-r4/arm-final` |
| Later build | `arm-final2`; previously built but not installed |
| Operating output | Master 255/255; board gains 0.25/0.25 |
| Unsaved trial | Body return 0.25; detail return 0.55 |
| Saved SQ4 | Body return remained 0.55 |

The first live command is:

```sh
python3 /Users/spectrasynq/.claude/skills/smc-titan-mixer/scripts/smc_titan.py health
```

**Actions**

1. Verify UID, installed build, schema, broker ownership and active output.
2. Read both boards directly through the established control owner. A missing secondary host cache is not secondary readback.
3. Save separate records of:
   - actual active settings;
   - host-saved composition;
   - fixed boot settings;
   - current firmware and broker admission.
4. Preserve the existing usable image and its exact HEX/ELF, receipt and admission.
5. Capture the current assembled-plate before-state using the approved music passage and existing camera method.
6. Record camera framing, exposure, white balance, playback level and physical output settings.

**Pass:** the before-state can be restored without relying on remembered widget values.

**Fail/repair:** reconcile contradictory identities before affected writes. Do not overwrite an unsaved operating choice by recalling SQ4 merely for convenience.

### M02 — Establish two explicit recovery routes

**Purpose:** recover both from a bad but reachable candidate and from loss of normal USB communication.

The following images have been located and hash-checked during planning:

| Role | Location | Build |
|---|---|---|
| Immediate working rollback | `_scratch/continuous-bloom-r4/arm-final` | `037dd22f…` |
| Allowlisted absent-CDC recovery | `_scratch/visible-dual-20261011/arm-001` | `d20b56cd…` |

Recheck their complete hashes and matching admissions before the first deployment.

**Actions**

1. Preserve the corresponding settings and broker admission for each recovery image.
2. Verify the current deployment helper’s healthy-CDC and `--recover-known-image` contracts.
3. Confirm E2 Lite `5AS079228B`, probe target-power **OFF**, SWD **1 MHz**, and the expected full target UID.
4. Account for the recovery image’s own defaults. After recovery, explicitly restore both-edge operation and verified compatible settings.
5. Record the subsequent healthy-CDC route back to the immediate working image.
6. Do not add a new candidate to the absent-CDC allowlist merely to bypass recovery restrictions.

**Critical distinction:** the deployment helper restarting the broker with its old admission is **not firmware rollback**. Restoring firmware requires a separate verified programming operation.

**Pass:** both recovery routes have available, hash-matching inputs and executable procedures.

**Fail:** missing recovery artefacts or failed identity prerequisites block flashing. Source work continues.

## 3. Establish the reference without requiring the ESP32-S3 K1

### M03 — Recover reference evidence in a fixed order

**Purpose:** learn from the valued performances rather than nominate current defaults as historical truth.

Use this order:

1. **Archives:** existing binary readbacks, complete settings, palette data, logs, payload captures and physical recordings.
2. **Source and host execution:** trace the active implementations and reuse existing host/replay tools.
3. **Reconstruction on the assembled Titan:** reproduce the strongest supported configuration, label unresolved values, inspect it physically and obtain Captain’s confirmation of the reconstructed reference.
4. **ESP32-S3 K1, only as last resort:** use it only if a named, consequential uncertainty survives the first three steps and an original-hardware observation can resolve that uncertainty.

Missing old prose, an incomplete palette record or a desire for a more comfortable reference does not itself justify step 4.

The surviving August binary is:

`_scratch/deck16_backend_b1b2_20260808/flash_readback_k1.bin`

SHA-256:

`04215afb279af9f63b833a2304d2fda8975db37f9d1cb0cc5d6a2ada814124ad`

Its reported source includes unrecorded dirty changes. Checking out its embedded commit does not reproduce that binary. Its historical evidentiary scope excludes input-health, absolute-drive and silence qualification.

**Required output:** one reference record per effect containing:

- source/build identity and evidence date;
- parameters and effective feature flags;
- palette stops, palette mode and sampling behaviour;
- primary/secondary configuration where known;
- audio and render cadence;
- audio cue and output settings where available;
- explicit unknowns;
- classification as historical, source-derived or reconstructed.

**Pass:** every reference claim has a source; reconstructed values are visibly distinguished.

**Fail/repair:** remove unsupported historical claims. Continue reconstruction; do not begin an indefinite history search.

### M04 — Freeze each effect’s behavioural contract

**Purpose:** prevent five different effects becoming one generic waveform with different gains.

The contract must cover:

| Effect | Behaviour to preserve |
|---|---|
| Hybrid K1 | Amplitude-position, two-pixel subpixel deposit; 16/23 ms peak followers; 80 ms colour smoothing; presence/silence followers; outward history; its peak-dependent decay and palette fallback |
| Tempo | Amplitude-position painting; integer transport with fractional remainder; `24 × BPM/60 × (1 + 0.5 cos(2π phase))`; confidence blend across 0.30–0.60; free-running fallback; source colour fallback |
| Waveform | Peak follower, deadband, sensitivity-dependent displacement, source injection scaling, one-pixel-per-reference-render transport and its decay |
| Fast | Raw-peak dot placement, source admission floors, elapsed-time transport, distinct per-render decay; do not substitute its otherwise unused peak follower |
| Hybrid | Actual PCM-history seed, square-root companding, centre radius variation, peak/VU presence and colour fallback; do not replace it with Hybrid K1’s dot |

The listed constants describe inspected source, not proof that the unreconstructed historical binary used every value.

**Actions**

1. Trace the real dispatcher and compile flags; exclude bypassed or `.wip` implementations.
2. Record every consumed audio field and colour/output transform.
3. Record whether each temporal operation is sample-, analysis-, render- or elapsed-time-based.
4. Use measured reference cadence where available.
5. Where cadence is unrecoverable, use an explicitly labelled reconstruction at Titan’s initial 100 Hz presentation rate. Captain’s reconstructed-reference decision resolves its appearance; do not label it historical parity.
6. Preserve amplitude-position geometry relative to the centre. Do not force every Waveform deposit to the centre merely because the preceding Bloom task required that behaviour.
7. Do not add Bloom returns, quiet phases or scene-busy admission barriers to these effects.

**Pass:** each effect has a deterministic reference trajectory and a separately identified list of defects eligible for correction.

**Fail/repair:** a disagreement about intended behaviour is resolved against source/evidence or the reconstructed-reference comparison before changing that behaviour.

## 4. Implement the audio, rendering and colour contracts

### M05 — Retain one capture owner and coherent chronology

**Implementation seam:** `LiveAudioRuntime::consume()` in `platform/ra8p1/k1_live_runtime.cpp`.

**Actions**

1. Retain the existing microphone/DMA capture owner and descriptor validation.
2. Fan out only admitted PCM downstream of that boundary.
3. Preserve stream epoch, sequence, sample interval and capture timestamps through analysis.
4. Use bounded, statically allocated assemblers for different analysis hops.
5. Discard discontinuous partial windows rather than concatenate samples across loss.
6. Publish immutable completed feature views. Both renderers acquire one frozen publication set per presentation frame.
7. Separate audio-generation advancement from rendering advancement.
8. Preserve producer-observed peaks/events needed between render reads with a bounded summary and per-consumer cursor. Do not introduce a generic event bus.
9. Distinguish continuous use of a still-valid sustained level from replaying an old transient as a fresh event.

**Pass:** deliberate gaps, duplicate descriptors and epoch changes cannot create fabricated feature continuity or stale attacks.

**Fail/repair:** reject the faulty publication, stop new injection from it, retain valid visual history and continue the unaffected renderer where its inputs remain valid.

### M06 — Provide truthful feature views for both families

**Actions**

1. Keep Emotiscope’s 64-bin publication and the in-house 80-bin feature meaning distinct.
2. Add common chronology metadata without pretending their spectrum, VU, chroma or tempo fields are interchangeable.
3. Explicitly configure any reused `AudioPipeline` machinery. Its current production constructor must not process 12.8 kHz samples using the 24 kHz configuration.
4. Begin faithful comparison with the applicable in-house 12.8 kHz/96-sample contract and retained donor 12.8 kHz/64-sample contract.
5. Account for reblocking delay in timing evidence.
6. Preserve pre-gate and post-gate chroma where the source consumer needs both, including A-origin note indexing.
7. Preserve waveform sample history for mode 11.
8. Preserve Tempo’s actual BPM, continuous phase and silence-attenuated confidence semantics. Do not substitute a Boolean lock or fabricate a downbeat.
9. Keep the analysis views available independently of which family renders the bottom edge.

Two analysis views over one captured stream are permitted where required for fidelity. Two capture stacks, two microphone owners and unproved spectrum interpolation are not.

**Pass:** same-input feature comparisons establish units, frequency layout, normalisation, timing and validity for every consumed field.

**Fail/repair:** retain separate proven producers where equivalence fails. Do not set validity flags on approximations and call them reference features.

### M07 — Compare and improve the audio mechanisms

**Purpose:** adopt the stronger mechanisms without delaying visible migration behind a broad DSP programme.

Evaluate only mechanisms with a consumer in this migration:

- peak and level response;
- spectral/chroma accuracy and stability;
- low-level musical preservation and room-noise rejection;
- tempo confidence and phase continuity;
- processing cost, publication age and end-to-end response.

**Procedure**

1. Run reference and candidate mechanisms on identical PCM.
2. Compare a mechanism at a time using existing replay/test infrastructure.
3. Record feature differences before inspecting their rendered consequence.
4. Reject a replacement that loses weak attacks, freezes sustained detail, destabilises palette position or creates false tempo confidence.
5. Adopt a mechanism that improves its intended measure without degrading the others.
6. If improvement trades against latency or another visible property, produce a matched physical comparison and obtain Captain’s specific trade-off decision.
7. Where no improvement is demonstrated, retain the faithful mechanism.
8. Share computation only after equivalence is established. Never merge feature meanings merely to reduce code.

**Pass:** every adopted change has a named consumer, matched evidence and a reversible implementation.

**Fail/repair:** revert that mechanism to its previous accepted implementation. Do not broaden into unrelated analysis or model work.

### M08 — Dispatch each edge independently in the active runtime

**Implementation seam:** `platform/ra8p1/palette_runtime.cpp`.

The present donor branch returns before native per-edge dispatch. Replace that exclusivity with independent family selection for each edge.

**Actions**

1. Represent renderer identity as **family plus effect ID**.
2. Prepare shared musical observations once, regardless of the primary edge’s selected family.
3. Dispatch each edge to its selected renderer using its own controls and state.
4. Retain the existing isolated donor instances.
5. Give each Waveform instance its own histories, followers, scroll accumulators, palette anchors, colour phase, timestamps and event cursors.
6. Ensure selecting Waveform bypasses that edge’s composed-Bloom scene override. Preserve stored donor-scene settings for explicit later selection.
7. Do not let an inactive donor scene’s quiet ownership or pair state suppress an unrelated Waveform edge.
8. A mode change resets only the selected edge’s effect state. Ordinary parameter and palette edits do not globally reset history.
9. A failed render on one edge must not prevent the other from rendering.

**Pass:** any ordered pair of retained Emotiscope and migrated Waveform modes can run independently.

**Fail/repair:** cross-edge history, control or scheduling influence is a release blocker. Fix ownership before physical promotion.

### M09 — Preserve precision, geometry and colour history

**Actions**

1. Add a bounded high-precision Waveform state/renderer inside the existing visual core.
2. Preserve the source fixed-point colour arithmetic initially; use the source’s `SQ15x16` semantics or an independently verified equivalent.
3. Do not route Waveform history through `Pixel8`.
4. Render Waveforms directly on 160 logical pixels per edge. Do not pass them through Emotiscope’s 128-to-160 expansion.
5. Preserve centre geometry, fractional placement, transport remainder and source clipping rules.
6. Preserve palette stop data, interpolation, coordinate selection, held coordinates, dark stops and fallbacks.
7. Save history before display-only transforms. Do not feed master gain, board attenuation, gamma or limiter output back into history.
8. Preserve the hue relationships of additive material; do not replace hue-preserving clipping with independent channel saturation.
9. Convert genuinely frame-dependent behaviour using the frozen reference cadence and elapsed time. Keep each effect’s distinct response.
10. Treat software gamma/correction improvements as separate changes after faithful output is established.

**Pass:** deterministic feature replay preserves the selected effect’s spatial, temporal and colour signature; target output retains meaningful low-order precision.

**Fail/repair:** white mixing, shortened trails caused by quantisation, altered transport diffusion or recolouring of old history requires correction in the owning stage.

## 5. Controls and complete composition authoring

### M10 — Publish a capability-aware control contract

Extend the existing firmware schema, broker and Surface. Do not add a parallel control service.

**Required interface changes**

- Add renderer-family identity alongside effect ID.
- Advertise supported family/effect combinations and per-effect controls.
- Apply family/effect selection together at one frame boundary for the selected edge.
- Return active family, effect, effective controls and settings revision.
- Preserve existing donor IDs and semantics.
- Interpret legacy composition documents as the existing donor family.
- Reject unsupported selections before mutation; never silently substitute another mode.
- Preserve unknown newer host-document fields when safely round-tripping; refuse application where the firmware cannot represent required behaviour.

The existing two-board apply operation is not atomic. Report partial application accurately and restore the pre-apply state if completing the requested composition fails.

**Pass:** displayed, saved, acknowledged and effective settings are distinguishable and agree after successful application.

**Fail/repair:** an acknowledged field without a consumer is not a working control.

### M11 — Implement meaningful per-edge controls

| Control group | Required behaviour |
|---|---|
| Effect and palette | Independent selection; curated palette stops and source sampling behaviour |
| Musical response | Actual amplitude/feature response, applied at its owning stage; never substituted with master brightness |
| Sensitivity | Preserve each effect’s real transfer; distinguish input influence from mode 8’s displacement scaling |
| Chroma and automatic colour | Preserve source coordinate behaviour; independent phase ownership where independently controlled |
| Mood | Audio-spectrum smoothing where supported; never relabel as Waveform speed |
| Contrast/saturation | Expose only in paths that consume them; show conditional inapplicability |
| Waveform tunables | Preserve existing supported shift, fade, threshold, blend and fallback controls only in the modes that use them |
| Geometry | Preserve authorised centre-based geometry; no control may alter physical routing |
| Output treatment | Preserve meaningful source output controls with explicit applicability; no accidental double scaling |
| Master/edge gain | Display-only operating controls with clearly identified scope |

**Specific traps to address**

- Mode 8 already scales new material by Photons inside the effect. Preserve or deliberately reconcile the complete transfer before adding Titan output gain.
- Mode 7’s updated peak follower does not position its dot.
- Mode 8’s source elapsed-time/shift arguments do not control its transport.
- Historical secondary Mood and automatic-colour storage do not prove independent consumers.
- Prism and secondary output-treatment settings have default bypass flags.

Per-edge controls must operate on per-edge conditioning. Hardware calibration remains a shared input property; do not pretend there are two independently calibrated microphones.

Do not expose source constants as new artistic sliders merely to inflate control coverage. Such additions belong to a separately justified enhancement.

**Pass:** each exposed control has a source-bound range, default, consumer, application timing and focused behavioural test.

**Fail/repair:** hide unsupported controls with an explanation or implement the required consumer. Do not leave decorative sliders.

### M12 — Save and recall the whole composition

**Actions**

1. Extend the existing host scene document to include both family/effect selections, palettes, controls, relationships and applicable software output profile.
2. Preserve draft, requested, active and saved distinctions.
3. Preserve complete undo/redo and existing parameter locks.
4. Round-trip legacy donor compositions without altering their effective settings.
5. Save a dedicated Waveform migration composition without overwriting the retained Bloom comparison unintentionally.
6. Verify recall through a fresh host process and fresh device readback.
7. Test failed/partial application and restoration.
8. Keep SQ4 explicitly identified as host persistence.
9. Keep boot composition separate from the last saved host composition.

**Pass:** recalling a composition reconstructs both edges and their relationship, not only the currently selected board’s widgets.

**Fail/repair:** preserve the last complete saved document and restore the previous active composition.

## 6. Integrate and qualify the five effects, one at a time

### M13–M17 — Repeat this complete delivery sequence

| Task | Effect |
|---|---|
| M13 | Hybrid K1 |
| M14 | Tempo |
| M15 | Waveform |
| M16 | Fast |
| M17 | Hybrid |

Each task includes all of the following:

1. Bind the selected reference and configuration.
2. Implement the required effect and its actual controls.
3. Run source/reference feature replay against the production renderer.
4. Verify geometry, transport, colour, silence and precision.
5. Test on either edge, two identical instances and one mixed-family pair.
6. Build the actual dual-output Titan image.
7. Deploy through the established owner and read back identity/settings.
8. Record matched approved music through the assembled plate.
9. Inspect the complete recording at playback speed with music, including the post-music interval.
10. Correct one demonstrated mechanism at a time.
11. Retain the effect only when the owner has rejected obvious faults and the relevant physical comparison passes.
12. Preserve its accepted source, build, settings and evidence before starting the next effect.

A host test, successful flash or effect name in the Surface does not complete any of M13–M17.

**Failure handling:** correct the selected effect or restore its previous accepted checkpoint. Do not abandon it for another effect merely because the first comparison is poor.

### Required reference and regression cases

| Case | Failure it protects against |
|---|---|
| Successive attacks while material is travelling | Busy-state suppression, overwritten peaks, missing later deposits |
| Strong and weak attacks | Thresholds that erase quiet music; saturated response |
| Sustained harmonic material | Chroma collapse, false silence, excessive colour instability |
| Abrupt input stop and room quiet | New noise-driven births versus retained travel |
| Zero, invalid and stale input | Stale replay and history that never drains |
| Tempo confidence rise/fall | Phase jumps, frozen unlocked behaviour, fabricated timing |
| Timestamp jitter and skipped render opportunities | Frame-rate-dependent speed/decay and accumulated catch-up bursts |
| Palette change during history | Global recolouring or history reset |
| Gain/output changes | Display attenuation feeding back into history |
| Same effect on both edges | Hidden static-state sharing |
| Mixed families in both orientations | Primary-family ownership assumptions |
| Change/reset/failure on one edge | Accidental peer interruption |
| Low-intensity multicolour history | Early 8-bit quantisation and hue distortion |

Expected trajectories are frozen from the reference before judging a candidate. Deterministic fixed-point paths use exact comparisons. Any necessary floating-point tolerance must be derived from the arithmetic boundary and documented before inspecting candidate failures.

## 7. Physical comparison, timing and improvement

### M18 — Use one repeatable physical comparison method

Use:

`docs/emotiscope-port-001/evidence/audio-windows-001/sonic-samba-20-44s.wav`

Retain the same:

- assembled light-guide/film stack;
- playback level and cue;
- camera position and framing;
- locked exposure, white balance and focus;
- master brightness, board gains and electrical limits.

**Actions**

1. Verify recording and full-speed review capability before relying on new physical evidence.
2. Capture a pre-music interval, the complete approved passage and a bounded post-music interval.
3. Correlate audio publication, effective injection, final payload and plate footage.
4. Measure quiet recovery from the last admitted injection, not the playback-stop request.
5. Identify actual acoustic excitation separately from retained travel or false births.
6. Include timestamps for attacks, sustain, dense passages, colour separation and recovery.
7. Review the complete recording; contact sheets alone do not satisfy motion/music inspection.
8. If a temporary independent-edge observation is needed to isolate blending, restore both edges for the actual comparison.

**Pass:** the claimed improvement is visible on the assembled plate under matched conditions.

**Fail/repair:** fix the demonstrated contributor. Blanket dimming, extinguishing the peer, changed exposure or a different palette cannot stand in for the requested improvement.

Camera or review-tool loss blocks new physical claims. Continue independent software work and preserve autonomous music output.

### M19 — Qualify timing under the actual Titan workload

Initial presentation remains the current **100 Hz / 10 ms** runtime. Donor analysis starts from its **64 samples / 5 ms** contract. The in-house reference assembler has its separately declared cadence and delay.

**Actions**

1. Measure capture admission, reblocking, analysis, publication, rendering, output submission and completion separately.
2. Compare baseline and candidate under the same pair, audio, controls and capture workload.
3. Record worst-case and distributional timing, missed releases, capture gaps and output faults.
4. Test the worst observed mixed pair, not merely two inexpensive modes.
5. Separate diagnostic overhead from normal operation.
6. Report publication-to-render latency under that name. Claim microphone-to-light latency only where the measurement actually covers it.
7. Reject new missed deadlines, dropped capture or unreliable output.
8. Submit any proposed latency increase with its concrete physical benefit for Captain’s trade-off decision.

Do not import an old ESP32 frame budget as the Titan acceptance limit.

**Pass:** the final image meets its declared scheduling contracts and has no unexplained timing regression.

**Fail/repair:** restore the accepted implementation and optimise the measured bottleneck. Do not reduce visual scope or hide misses.

### M20 — Exploit Titan capability through measured improvements

This phase follows a faithful checkpoint; it does not postpone the first physical result.

**Actions**

1. Profile actual costly operations and precision loss.
2. Evaluate available compiler/processor, memory-layout and bounded scheduling improvements against the scalar/reference implementation.
3. Preserve a deterministic reference path for arithmetic validation.
4. Change one mechanism per comparison.
5. Re-test affected renderers and mixed pairings after shared changes.
6. Evaluate software gamma/correction separately from audio or motion changes.
7. Reject changes that improve counters but degrade musical readability, colour structure or darker space.
8. Retain only demonstrated improvements and record their incremental cost.

No processor feature, wider arithmetic or richer analysis is valuable merely because it executes. Adoption requires useful output or a measured reduction in cost without loss of behaviour.

## 8. Deployment procedure and rollback matrix

### M21 — Build and install each admitted candidate

**Before every flash**

1. Recheck source manifest, dirty inputs and branch.
2. Run the focused regressions for the actual changes.
3. Build with the established dual-output configuration.
4. Bind local source identity separately from the upstream donor pin.
5. Bind build ID, schema hash, HEX/ELF hashes and target UID.
6. Verify the correct rollback inputs remain available.
7. Verify sole serial ownership and current hardware identity.

Use the established deployment route:

```sh
TITAN_DEPLOY_PYTHON=/opt/homebrew/bin/python3.13 \
  /Users/spectrasynq/.codex/skills/smc-titan-mixer/scripts/deploy_titan_image.sh \
  <verified-build-directory> \
  <matching-admission.json> \
  <fresh-programme-directory>
```

The arguments must come from the current candidate manifest. They must not be copied from a historical helper without reconciliation.

**After every flash**

1. Read back UID, build and schema.
2. Read back both edges’ effective configuration.
3. Confirm autonomous rendering, both-edge output and fault status.
4. Perform the applicable physical checks in that executing session.
5. Keep candidate and failed evidence even if rollback is required.

**Protected hardware**

- 160 pixels per optical edge, split into two 80-pixel DIN halves.
- Bottom: P601/GPT6/DMA0 and P604/GPT8/DMA3.
- Top: P603/GPT7A/DMA6 and P714/GPT12B/DMA7.
- Microphone: P502/P812/DMA1/2.
- Probe target-power OFF; established 1 MHz SWD route.
- No GDB, chip erase, configuration-area writes or guessed routing changes.

### Mandatory rollback actions

| Failure | Required action | Resume condition |
|---|---|---|
| Bad parameter/palette trial | Restore the captured pre-trial settings and read back both edges | Original behaviour/settings restored |
| Surface partial apply | Report partial state; restore the complete prior composition | Both edges agree with restored document |
| Renderer/analysis regression with reachable device | Restore the last accepted image through the normal verified route, then its compatible settings | UID/build/schema/settings and physical operation verified |
| Candidate loses CDC | Use only the existing absent-CDC identity procedure and pinned allowlisted image; then restore both-edge operation and, when reachable, the immediate working image | Fresh identity and normal communication restored |
| Deployment helper reports failure | Determine installed-image state; do not equate old broker admission with old firmware | Actual installed image established and repaired/restored |
| Unknown UID, ownership conflict or probe mismatch | Stop affected hardware writes | Exact identity/ownership prerequisite resolved |
| Timing/output fault | Do not promote; restore accepted operation and correct the scoped defect | Same failing case passes |
| Camera unavailable | Preserve useful autonomous output; continue independent source work | Existing physical capture/review works |
| Historical reference incomplete | Use the specified reconstruction path | Captain confirms the reconstructed reference, or the last-resort predicate is met |
| Necessary out-of-scope work | Stop that expansion; retain usable operation and name the exact unmet dependency | A permitted solution or explicit scope change exists |

No blind retries. A repeated operation requires a diagnosis and an intervening correction or changed condition.

## 9. Final composition, boot and delivery

### M22 — Qualify all independent selections

**Actions**

1. Enumerate the seven retained donor modes and five Waveforms as family-qualified identities.
2. Exercise all 144 ordered pairings in the production host path for dispatch, validity, bounds and state ownership.
3. Exercise meaningful parameter changes against each mode’s advertised controls.
4. With deterministic input and shared limiting inactive, verify that changing one edge leaves the peer payload unchanged.
5. Test legitimate shared limiting separately and identify its expected cross-edge consequence.
6. Physically inspect every Waveform and the mixed-pair cases most likely to expose integration or timing faults.
7. Confirm legacy donor compositions still recall and run.

**Pass:** independence is demonstrated by state and payload tests, with corresponding physical coverage.

**Fail/repair:** fix the common integration defect and rerun the affected pairings. Do not remove difficult combinations from the promised capability.

### M23 — Select and embed the fixed boot composition

**Actions**

1. Present a small, source-bound set of fully inspected two-edge compositions.
2. Include concrete recordings and the owner’s judgement of their musical strengths and remaining weaknesses.
3. Captain selects the boot composition from these actual results.
4. Embed its complete settings in firmware.
5. Save the same composition through the existing host authoring path.
6. Rebuild and deploy the exact boot candidate.
7. Verify software restart and an authorised cold-start test.
8. Verify startup and continuing response without a host, camera or diagnostic session.
9. Verify that host disconnect and diagnostic timeout do not stop normal music visualisation.
10. Verify host recall separately from embedded boot defaults.

**Pass:** both edges autonomously run the selected composition after restart, and host recall restores the same intended composition.

**Fail/repair:** fix startup/configuration precedence. Do not add device-NVM persistence as an unrequested workaround.

### M24 — Deliver the final image and complete evidence

Update the existing migration document and current-state record without removing original assignments or failed results. Commit only relevant changes; include any remaining candidate diff in the source identity.

The final packet must contain:

1. **Running identity:** source commit, relevant dirty diff, build ID, HEX/ELF hashes, schema and target UID.
2. **Composition identity:** complete saved settings, embedded boot settings and verified physical mapping.
3. **Reference identity:** historical versus reconstructed provenance for each effect.
4. **Physical comparison:** actual clips, approved audio cue and timestamps supporting the claimed result.
5. **Causal evidence:** the minimum feature → injection → payload → plate evidence needed to explain response and quiet recovery.
6. **Timing evidence:** actual workload, deadlines, latency scope and observed faults/misses.
7. **Control/persistence evidence:** independent control consumers, complete recall and startup behaviour.
8. **Rollback record:** exact retained images, admissions, settings and executable recovery routes.
9. **Owner assessment:** concrete visual judgement, remaining defects and their disposition.
10. **Leave-state:** useful selected composition installed, saved and running on both edges.

### Fixed final acceptance table

| Criterion | PASS requires |
|---|---|
| Faithful effect identity | All five retain their distinct source-bound motion, colour and response |
| Fresh musical response | Later attacks remain visible while earlier material travels |
| Colour and optical structure | Curated colour variation and darker space survive the assembled stack |
| Quiet recovery | Invalid/stale/silent input stops new material; existing history drains; weak music remains responsive |
| Independent operation | Either edge accepts any supported effect; changing one does not accidentally alter or stop the other |
| Meaningful controls | Every advertised control changes its intended consumer and reports its effective state |
| Complete recall | Both edges, palettes and relationships survive host save/reload |
| Installed operation | Final UID/build/schema/settings match the delivered packet |
| Autonomous boot | Selected fixed composition starts and runs without host-dependent admission |
| Timing and reliability | Declared deadlines met; no unexplained capture/output regressions |
| Visual quality | Owner has corrected observable faults; Captain’s decisions refer to concrete comparisons |

Report each criterion as **PASS, FAIL, BLOCKED or NOT TESTED**. Report aesthetic preference separately as **PREFERRED, ACCEPTED, REJECTED or PENDING**; it cannot conceal an engineering or visibly observable defect.

The migration closes only when the final image, controls, saved composition and physical evidence satisfy the applicable criteria. It does not close on a simulator, build, flash, test count or an uninspected recording.

**ESP32-S3 last-resort condition:** if original hardware is ultimately necessary, document the exact unresolved question, the results of archive/source/Titan reconstruction, and the observation that will resolve it. Use the separate assembled K1 with verified identity, backup and recovery. Do not disturb the Titan stack or convert original-hardware use into a new prerequisite for the rest of this migration.
</proposed_plan>
<!-- END APPROVED WAVEFORM EXECUTION BRIEF 20261013 -->


---

## Execution checkpoint — first Hybrid candidate timing failure, 13 October 2026

THE TITAN MUST CONTINUE AUTONOMOUS MUSIC. Root remains the sole live owner.
This checkpoint records a failed candidate and completed rollback; it neither
alters the approved M00–M24 brief above nor establishes migration completion.

### Candidate and actual failure

Firmware source `ef305443faf24955e26b50dd4c4abd2173d8173d` produced build
`66e71a4f16abd6ca7691536b4269cafb207aa92e6628b517fa1dd42442936fed`, schema
`ce1b968f85c4f6a4ef0f1e56db5564c31c3b9c2feab0bde2c5cc13a817940787`.
Programming began at 00:46 AWST on 13 October; the deployment log records
programming return code 0 and a matching post-programming identity. Its subsequent
health display briefly retained an old cached build while the broker returned;
that cached display is not the new image's identity evidence.

The candidate **FAILED timing before Hybrid K1 was selected**. The retained
telemetry still identifies `visual_path: emotiscope` and zero in-house frames.
At the captured failure point it reported 3,372 frames and 282 skipped releases,
3,334 emitted frames, and zero emit errors. Poll 17 took approximately 1.121 seconds
in the retained timing summary; a later poll failed with the exact broker error
`USB timeout 1120/3822`, followed by transport quarantine. The Surface apply
operation also timed out. These observations reject promotion; they are not
evidence about the Hybrid renderer's optical quality.

Preserved failure evidence:

- [Timing failure summary](../../_scratch/waveform-migration-001/matched-002/timing-failure-summary.json).
- [Broker failure log](../../_scratch/waveform-migration-001/matched-002/broker-failure.jsonl).
- [Deployment log](../../_scratch/waveform-migration-001/matched-002/deploy.log).
- [Failed apply log](../../_scratch/waveform-migration-001/matched-002/apply.log).
- [Single-session recording](../../_scratch/waveform-migration-001/matched-002/original.mov),
  [camera events](../../_scratch/waveform-migration-001/matched-002/camera.jsonl),
  and [before-pass capture](../../_scratch/waveform-migration-001/matched-002/before/capture.json).

### Physical evidence limits

The camera completed one locked session from 00:45:56.848 to 00:47:48.863 AWST
(approximately 112 seconds), with the existing EiP UID. Deployment occupied
approximately 55 seconds including identity/reconnection work; the subsequent
apply failure prevented the candidate's after passage. There is **no completed
matched baseline/candidate musical comparison** from this session. Recording
success is not visual acceptance.

The separately retained `before-002` observation follows Captain's reframing;
root observed both physical ends with a small upper-right support occlusion.
Keep the earlier wrongly cropped `before-001` as rejected capture evidence.
Neither still-frame inspection nor these recordings fulfil the required complete
playback-speed audiovisual review: the available review-tool capability remains
unavailable. **M18 full-speed audiovisual inspection is BLOCKED** until an existing
review route can actually play and expose the entire recording with music to the
executing reviewer. This blocker does not suspend independent source repair.

### Actual rollback and restored operating state

Root used the healthy-CDC recovery route to reprogramme the retained image
`037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272`. The
[rollback receipt](../../_scratch/waveform-migration-001/rollback-hybrid-002/receipt.json)
records UID `545433931bd25436593630352d068363`, E2 Lite `5AS079228B`, 1 MHz SWD,
probe power OFF and programming return code 0. The immediate
[readback](../../_scratch/waveform-migration-001/rollback-hybrid-002/readback.json)
reports that build, autonomous emission, approximately 100.3 frames/s and zero
reported LED faults. This readback precedes restoration of the operator settings.

The later [restored active composition](../../_scratch/waveform-migration-001/rollback-hybrid-002-active.json)
records both edges on continuous Bloom (scene 3), master 255/255, both board gains
0.25, secondary output enabled, and phrase selection 0 on both boards. Body
return remains the unsaved 0.25 trial; detail return is approximately 0.55.
**SQ4 remains unchanged.** The intermediate
[observed restoration record](../../_scratch/waveform-migration-001/rollback-hybrid-002-active-observed.json)
retains the earlier secondary phrase value 1; it must not be substituted for the
corrected active record or described as an exact restore.

The rollback exposed a host bug dropping an explicit phrase value 0. Root reports
host commit `b855677` fixes that preservation error, the relevant host run passed
246 tests, and the corrected service was restarted. This is host repair evidence,
not a Waveform physical pass.

### Current disposition and next executable correction

- **M13 first Hybrid candidate:** incomplete; the image failed before the effect
  received its complete physical passage.
- **M19 timing/reliability:** FAIL for build `66e71a4f…`; do not promote it.
- **Recovery:** retained working image reinstalled and entry two-edge composition
  restored; keep the receipts and unsaved/saved distinction above.
- **M18 physical comparison/review:** no completed matched after passage; full-speed
  audiovisual review BLOCKED. No overall physical completion is claimed.
- **Repair in progress:** root is correcting unused analysis/catch-up scheduling
  and the source-exact four-lane Goertzel computation before another candidate build.
  Neither correction is recorded as fixed or target-verified yet. The next
  deployment requires the corrected source's focused regressions, exact image
  identity, and a bounded repetition of the previously failing timing workload.

Original assessment, approved plan, unsuccessful candidate evidence and working
recovery image remain intact. This checkpoint adds no new campaign or acceptance
criterion and makes no preference decision on Captain's behalf.

---

## Execution checkpoint — candidate 003 audio failure and rollback, 13 October 2026

THE TITAN MUST CONTINUE AUTONOMOUS MUSIC. Root remains the sole live owner.
This checkpoint preserves the failed candidate and recovery. The complete
assignment, approved M00–M24 plan and earlier failed evidence remain unchanged.

### Candidate 003: donor timing did not establish Hybrid operation

Source `f2521acfeb326e6691ca0c6c54d1936f0759e20b` produced candidate build
`c8bead87cd4419cfe7613946c7e2f2297b255426bc203ab88c63569abd29950c`.
The [donor-only start](../../_scratch/waveform-migration-001/hybrid-003-donor-start.json)
and [end](../../_scratch/waveform-migration-001/hybrid-003-donor-end.json)
record 3,343 → 4,852 frames, with skipped releases unchanged at 69: zero new misses
within that observation, not zero lifetime misses. Both modes were donor Spectrum
(1/1); ASRC discards and rejected pushes were zero, and stream epoch remained 1.

The file named
[hybrid-003-mixed-timing.json](../../_scratch/waveform-migration-001/hybrid-003-mixed-timing.json)
likewise contains only modes **1/1**, following a refused mode change. Its filename
must not be treated as proof of a mixed-renderer workload.

After actual selection of Hybrid K1 on the first edge, the
[active timing samples](../../_scratch/waveform-migration-001/hybrid-003-active-timing.json)
confirm modes **32/1**. The later
[failure snapshot](../../_scratch/waveform-migration-001/hybrid-003-capture-failure/live-stats.json)
records **911,356 ASRC samples discarded, 839 rejected pushes, stream epoch 840,
and maximum PDM service gap 18,751 microseconds**. Hybrid reports invalid/fresh=0
input, zero deposit strength and **zero injections**. The
[broker tail](../../_scratch/waveform-migration-001/hybrid-003-capture-failure/broker-tail.jsonl)
is retained alongside that snapshot. Root rejected the candidate and rolled it
back. This is a failed live audio workload; successful donor-only timing cannot
clear it. No candidate physical comparison was completed.

### Recovery and host corrections

The [rollback-hybrid-003 receipt](../../_scratch/waveform-migration-001/rollback-hybrid-003/receipt.json)
and [identity readback](../../_scratch/waveform-migration-001/rollback-hybrid-003/readback.json)
record the retained baseline build
`037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272` on Titan UID
`545433931bd25436593630352d068363`. The programming receipt records return code 0.
The immediate readback precedes restoration of the operator configuration.

The subsequent [restored two-edge settings](../../_scratch/waveform-migration-001/rollback-hybrid-003-active.json)
record scene 3 and phrase 0 on both boards, master 255/255, both board gains 0.25,
body return 0.25 and detail return approximately 0.55. This snapshot is an exact
copy of the latest readback written by the existing `restore-entry.py` helper:
that helper reused the older `rollback-hybrid-002-active.json` filename. The
candidate-003 copy removes that filename ambiguity; installed build identity
comes from the separate rollback readback above, not from the settings document.
The useful baseline is the operating leave-state at this checkpoint; the failed
Hybrid build is not promoted.

Two host corrections belong to this attempt:

- `727b07b` corrected the decoder's `version` → `state_version` contract so an
  actual version-7 state admits the supported in-house mode selection.
- `41853d0` repaired complete mixed-renderer → donor restoration. Root reports
  254 relevant tests passed and the corrected host service was loaded.

These host repairs do not establish working Hybrid audio or physical quality.

### Candidate 004 preparation and remaining work

Firmware source `597525697d2f14da384622848275777cfa47bc25` adds exact SMULL products
and minimal MIR stage-cycle observations for the failing audio computation.
The [first 004 build receipt](../../_scratch/waveform-migration-001/arm-hybrid-004/receipt.json)
retains the failed import gate: the command omitted the established `--tempo-field`
option. The corrected command uses the same adaptation as candidate 003.
The [corrected build receipt](../../_scratch/waveform-migration-001/arm-hybrid-004-corrected/receipt.json)
subsequently completed at 01:11:09 AWST with build
`1bb0cb1d5b2546b65d11d31bd19053e37f9b05c420fac4805236cc8219e8d81f`.
This is **host build evidence only**. No deployment, repaired target timing or
physical comparison for candidate 004 is established by this checkpoint.

**Disposition:** candidate 003 audio/timing FAIL; recovery recorded; M13 remains
incomplete and M14 is not advanced. M18 still lacks a complete candidate physical
comparison and the required full-speed audiovisual inspection. The next action
is root's bounded deployment and measurement of the corrected computation against
the actual mode-32 failure, followed by the approved physical comparison only if
that workload passes. Preserve the failed build receipt, donor-only observations,
actual Hybrid failure and rollback as distinct evidence. No overall migration
completion or visual preference is claimed.

---

## Execution checkpoint — candidates 004 and 005, 13 October 2026

THE TITAN MUST CONTINUE AUTONOMOUS MUSIC. Root remains the sole live owner.
These attempts measured a useful reduction in audio computation, but neither
qualified the Hybrid candidate. The complete original assignment, approved
M00–M24 and all earlier failures remain preserved above.

### Candidate 004: capture loss despite exact DSP optimisation

Source `597525697d2f14da384622848275777cfa47bc25`, build
`1bb0cb1d5b2546b65d11d31bd19053e37f9b05c420fac4805236cc8219e8d81f`, was deployed
and rejected during the first three bounded observations. The
[actual observation log](../../_scratch/waveform-migration-001/hybrid-004-bounded.log)
records audio gaps rising 7 → 18 → 29, invalid in-house input and zero injections;
rejected pushes reached 22 by the third observation. Unchanged skipped-release
counts do not erase that audio failure.

MIR stage costs at the reported 1 GHz clock were approximately 1.40–1.48 ms donor
analysis, 2.10–2.124 ms in-house analysis, and 1.14–1.16 ms rendering. These are
measured stage costs, not end-to-end microphone-to-photon latency. The individual
[sample records](../../_scratch/waveform-migration-001/hybrid-004-bounded/0/mir-snapshot.json)
retain the cycle counts and validity context.

Root restored baseline build
`037dd22f0450f2286bce8f833722e5d410ea83fe658add7d29d91d2e97fa5272` using the
[004 rollback receipt](../../_scratch/waveform-migration-001/rollback-hybrid-004/receipt.json).
The [restored 004 settings](../../_scratch/waveform-migration-001/rollback-hybrid-004-active.json)
retain both boards on scene 3/phrase 0, master 255/255, board gains 0.25, body
return 0.25 and detail return approximately 0.55. Candidate 004 is rejected;
there is no completed physical comparison for it.

### Candidate 005: faster computation, incomplete and failed comparison

Source `b3ca7739209308950c185c7b3ea69abad84c85f2` produced build
`2f2e0432a88ae0ac97c81549aa5351fead041ef2a3e946a83b36ccae09488cfd`.
Its scalar recurrence reduced actual in-house stage cost to approximately
1.00–1.07 ms, roughly half candidate 004's measured cost. The
[eight initial bounded observations](../../_scratch/waveform-migration-001/hybrid-005-bounded.log)
show valid in-house publications, fresh renderer observations, injection counts
advancing to 576 and 1,180, no audio gaps or rejected pushes, and skipped releases
remaining at 69. That is a useful engineering improvement over 004; it is not
qualification of the later sustained comparison workload.

The [matched-005 recording](../../_scratch/waveform-migration-001/matched-005/original.mov)
used one locked camera session, approximately 57.27 seconds long. Its
[camera event log](../../_scratch/waveform-migration-001/matched-005/camera.jsonl)
records start 01:20:21.341 and finish 01:21:18.601 AWST with no recording error.
The donor-composition before passage completed the approved 24-second audio
window and requested 12-second post interval, retaining
[132 payload/telemetry samples](../../_scratch/waveform-migration-001/matched-005/before/capture.json).
The [before observation](../../_scratch/waveform-migration-001/matched-005/before/observation.json)
reports unchanged physical controls. This before-state ran on **candidate 005's
firmware with the donor renderer**, as its build identity confirms; it is not a
recording of the retained `037dd22f…` binary.

The after passage started music at 01:21:07.238 AWST and failed approximately
6.17 seconds later. The [after log](../../_scratch/waveform-migration-001/matched-005/after.log)
records stale MIR snapshot failure followed by failure to restore the selected
Surface board through fresh readback. Only
[three complete after samples](../../_scratch/waveform-migration-001/matched-005/after/capture.json)
were retained. The
[observation result](../../_scratch/waveform-migration-001/matched-005/after/observation.json)
does not verify unchanged physical controls after that loss; this is missing
verification, not proof that an operator changed their values. The
[broker tail](../../_scratch/waveform-migration-001/matched-005/failure-broker-tail.jsonl)
records quarantine, and a retained [exchange error](../../_scratch/waveform-migration-001/matched-005/emit-error.json)
reports `USB timeout 0/32`. The retained last complete after-sample MIR body
already reports **audio gaps 2, stream epoch 3 and invalid input**, while its
separately cached PDM fields still show the older zero/epoch-1 state. The
[bounded original wire-log excerpt](../../_scratch/waveform-migration-001/matched-005/mir-timeout-wire-excerpt.jsonl)
retains the MIR request at host monotonic 139898.070789 and the next command at
139900.092955, approximately 2.02 seconds later, followed by quarantine errors.
Root observed that MIR USB timeout and two ASRC rejected pushes during this
attempt. The cached `matched-005/live-stats.json` predates the later observation
and still reports zero; it must not be used to assert zero failures throughout
the attempt. Root also
observed mixed-renderer USB response becoming roughly ten times slower before
the music began, so playback onset alone does not establish the failure's cause.

**The matched comparison did not pass.** No complete after passage or post-music
recovery was captured. Root opened the retained
[before frame at 8 seconds](../../_scratch/waveform-migration-001/matched-005/before-8s.png)
and [after frame at 50 seconds](../../_scratch/waveform-migration-001/matched-005/after-50s.png),
observing bright lower-edge deposits towards the ends and a darker centre. Those
are separate recording times, not an asserted match of musical cue points.
They do not demonstrate motion, successive musical response or superiority.
Complete playback-speed audiovisual review remains unfulfilled.

### Rollback, bounded host repair and current disposition

Root rejected candidate 005 and restored the retained `037dd22f…` baseline.
The [005 rollback receipt](../../_scratch/waveform-migration-001/rollback-hybrid-005/receipt.json)
records target UID `545433931bd25436593630352d068363` and programming return code 0.
The [005 restored settings](../../_scratch/waveform-migration-001/rollback-hybrid-005-active.json)
confirm both boards scene 3/phrase 0, master 255/255, both board gains 0.25,
body return 0.25 and detail return approximately 0.55.

Host commit `529ab99` prevents another write after an incomplete USB exchange
by quarantining the transport; root reports 62 focused tests passed. This is a
failure-containment correction, **not a fix for the initial USB timeout**.
Root is now implementing a bounded transition/catch-up yield correction in the
existing fixture path. At this checkpoint that work is neither deployed nor
accepted; no claim about candidate 006 is made.

**Status:** actual DSP cost improved; candidate 004 capture reliability FAIL;
candidate 005 sustained capture/control reliability FAIL; physical comparison
incomplete; M13 remains open, M14 is not advanced, and M18 audiovisual review
remains BLOCKED. The executable continuation is to verify the scoped scheduling
correction against the observed mixed workload and USB exchange failure, restore
reliable operation, then complete the same approved musical comparison. No
new effect or overall migration completion is authorised by these results.

## Execution checkpoint — 13 October 2026, candidate006

Candidate source `f8d2c002` / image `ce969aa81e2b2855cee970038758d87ffcd47063c3fddb01a530a695abf724ec` tested a six-line catch-up yield when LED transmission ended during an audio hop. Nine production-loop cases and three rejected mutations passed; the build matched all 211 recorded source inputs. The actual mixed32/1 target nevertheless failed: two retained MIR samples show gaps1→2, epochs2→3 and donor valid→warming/invalid. PDM telemetry reports one ASRC rejection, 1,030 discarded samples, zero raw-DMA overflows/drops/recoveries, zero pair-skew drops and a 14,395µs maximum PDM service interval. Cached telemetry predates the second MIR gap and cannot deny it. Evidence: `_scratch/waveform-migration-001/hybrid-006-bounded/`. No new physical comparison was attempted.

Root restored baseline `037dd22f…` and the complete two-edge entry settings; receipts are `rollback-hybrid-006/` and `rollback-hybrid-006-active.json`. Both outputs remain autonomous, enabled at master255 with board gains0.25/0.25. SQ4 was not overwritten. The unsuccessful scheduler change was reverted in `6e07b19`.

Source accounting establishes the rejected queue admission: 734 retained samples plus the incoming296 block exceed the unchanged1024 capacity by6. This proves queue pressure, not sustained CPU overload or a raw microphone dropout. No queue enlargement is justified: existing scheduling does not guarantee a sub-hop residue, and a drain-until-empty poll can admit newly completed slots while it runs. The next scoped correction under inspection removes repeated circular-index wrap work from the existing95-tap FIR while preserving coefficients, summation order, sample rate and exact output. It is not yet target-tested or accepted. M13/M18 and overall migration remain incomplete.
