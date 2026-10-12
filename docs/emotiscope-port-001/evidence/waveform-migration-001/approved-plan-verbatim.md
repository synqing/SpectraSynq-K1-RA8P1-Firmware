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
