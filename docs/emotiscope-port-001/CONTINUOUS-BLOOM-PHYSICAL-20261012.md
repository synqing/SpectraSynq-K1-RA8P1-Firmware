# Continuous Body/Detail Bloom — physical delivery, 12 October 2026

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

The candidate is running and saved as SQ4. Source repairs, deployment, a matched physical comparison, one body-spacing correction, current-image timing and complete two-board save/recall are delivered. **Aesthetic preference PENDING. Overall acceptance remains incomplete: the physical quiet interval is not fully qualified.** The footage shows history draining after the probe, then faint new material with nonzero microphone input. Known-zero and stale PCM pass the production-path regression; neither proves acoustic silence in this room.

## Review these actual recordings

- [Final baseline/candidate comparison — 24 seconds](../../_scratch/continuous-bloom-r2/physical-001/final-001/baseline-candidate.mp4): retained image above, candidate below. Same approved Sonic Samba cue, playback level, assembled stack, camera position and one continuously locked exposure/white-balance/focus session. Crop/scale only; no colour or brightness correction. Audio is the approved source aligned to host playback requests; uncalibrated player/acoustic startup delay remains. This is not microphone-to-photon timing.
- [Final attack/detail/silence probe — 22 seconds](../../_scratch/continuous-bloom-r2/physical-001/causal-002/causal-review.mp4): five 82 Hz bursts at 0.8-second intervals, 2200 Hz sustain, mixed passage, then digital silence. [Exact stimulus](../../_scratch/continuous-bloom-r2/physical-001/probe-definition.json), [actual received input during the final interval](../../_scratch/continuous-bloom-r2/physical-001/causal-002/silence-received-input.json).
- [Uncut paired camera recording](../../_scratch/continuous-bloom-r2/physical-001/final-001/pair-original.mov), [camera/playback/programming events](../../_scratch/continuous-bloom-r2/physical-001/final-001/pair-events.json), [timing/configuration comparison](../../_scratch/continuous-bloom-r2/physical-001/final-001/comparison-summary.json), [media hashes and integrity checks](../../_scratch/continuous-bloom-r2/physical-001/final-001/integrity.json).

The owner opened and inspected the complete baseline/candidate plate sequences, the final comparison frame, the complete probe sequence, and a 10 fps three-second movement sequence from the final candidate. Later material enters while existing colours travel outward. Body has compact changing lower deposits; detail sustains a more continuous upper field. Darker separation survives between deposits, although the optical stack blends both fields across the plate. Some emitter-adjacent camera highlights approach clipping; highlight colour is less reliable than the wider plate. No optical superiority or Captain approval is inferred from payload differences or numerical scores.

## Exact running identity and settings

| Item | Delivered state |
|---|---|
| Checkout | Existing `.worktrees/titan-emotiscope-port-001`, branch `feat/titan-emotiscope-port-001` |
| Firmware source | `7ae880ef05e13912bead58a73a520ad862507ca9`, including boundary/history repair `b32fde5` |
| Installed build | `4c94653ddc3d9d2a6a56500e1422d953a48c0be50b0486d7deaec55f6e3a43e4` |
| Target UID | `545433931bd25436593630352d068363` |
| Schema | `6d68932e22c0796bcac290a233df9e235410b7af724ac71112525a86017b561b` |
| Retained baseline | Build `47bc25b5f69c9e27c90dfe089343412569825beef07bed1a4f94e47718ef28d4`, original settings preserved |
| Operating output | Master **255/255**, both board gains **0.25**, secondary and emission ON; current operator setting preserved across both passages |
| Composition | Both scene 3, unlinked, body/detail roles at full strength, palette 1, phrase policy OFF, ML influence/background/screensaver OFF in this preset only |
| Travel | Both 1200 ms outward / 1800 ms return, one return, return strength 0.55 |
| Body spread | Outgoing width **0.08**, return width **0.08**, trail **40 ms** |
| Detail spread | Outgoing width **0.20**, return width **0.14**, trail **120 ms** |
| Musical envelopes | Both attack 40 ms / release 160 ms; independent state |
| Edge Response | Editable per board; saved default OFF/depth 0 |

[Build receipt](../../_scratch/continuous-bloom-r3-single-attack/arm-001/receipt.json), [final programmer receipt](../../_scratch/continuous-bloom-r2/physical-001/final-001/programme-candidate/receipt.json), [installed identity](../../_scratch/continuous-bloom-r2/physical-001/final-001/programme-candidate/post-info/handoff-info.json), [complete saved composition](presets/continuous-body-detail-bloom.json), [save/edit/recall readback](../../_scratch/continuous-bloom-r2/physical-001/final-001/recall-and-leave-state.json), [leave health](../../_scratch/continuous-bloom-r2/physical-001/final-001/leave-health.json). All 207 compiled inputs were matched to the firmware correction. The old embedded donor source label is not substituted for that input-hash binding.

SQ4 is saved through the existing host authoring path, including both boards and their relation. It is **host persistence**, not device NVM. A body-only Edge Response mode 2/depth 0.6 edit was acknowledged, rendered and read back; SQ4 recall restored every authored field on both boards. Reboot retains the existing autonomous firmware defaults; SQ4 is recalled through the host. Camera and playback helpers are stopped; broker and mixer services are restored and healthy. Ending capture did not disarm output.

Configured mapping is unchanged: bottom P601/GPT6/DMA0 plus P604/GPT8/DMA3; top P603/GPT7A/DMA6 plus P714/GPT12B/DMA7; microphone P502/P812 DMA1/2. Each 160-pixel optical edge has two 80-pixel electrical DIN halves, not two separate optical channels. The controlled probe links body to the lower optical contribution and detail to the upper contribution. Gamma, output correction, current limits, pins and LED counts were not changed. Programming temporarily interrupted the application; final programming succeeded and autonomous output returned. The camera contains that interruption, outside the two compared passages.

## Fixed criteria and their evidence

| Criterion | Engineering and physical result |
|---|---|
| Fresh music remains visible | **PASS within observed passage.** Repeated bass changes inside four seconds create new lower material while previous deposits travel. Existing real donor PCM → injection/history → both renderers → GRB48 tests cover the exact producer/consumer boundary and the previously missed sub-25 ms attack. The final probe retains 97 bottom payload captures and both board event/conditioning readbacks alongside the plate footage. |
| Independent behaviour | **PASS within tested contract.** Low bursts and high sustain give visibly different temporal responses. Deterministic input with shared limiting inactive leaves every peer payload byte unchanged after a body-only Edge Response edit. Acoustic replay is not bit-identical input and is not used as that isolation oracle. |
| Controls and silence | **Control/known-input PASS; acoustic quiet NOT QUALIFIED.** Edge Response changes its intended production output and live setting; complete recall passes. Zero/stale PCM stops births and drains history. In the real probe, material decays after playback ends, but fresh faint births return. During digital silence, actual microphone peaks remain 1805, 1497 and 1036 in the existing PCM telemetry; fresh event cursors advance. This does not establish a below-floor acoustic interval or stale replay. No threshold was guessed to suppress quiet music. |
| Optics preserve design | **Observed, preference PENDING.** Matching real-stack footage shows centre-origin moving colour, stronger body pulses, sustained detail and darker regions. Three existing body-spread controls were narrowed to reduce smearing. Dense music still fills substantial plate area; this is visible in the supplied comparison. |
| No timing regression | **PASS for the bounded matched workload.** Both images stay within the actual 10 ms render period; no new publication misses, skipped LED releases or emit errors. Detailed values below; no end-to-end photon claim. |
| Honest quality | The running candidate and comparison are delivered. **Overall engineering acceptance INCOMPLETE for physical quiet qualification; aesthetic PENDING.** No claim that Captain preferred it or that all physical criteria passed. |

Bottom payload taps cover the bottom edge's two DIN halves, not top payload or GPIO waveforms. Top independence is established by the deterministic production-path check and physical temporal observation. Existing legacy payload scorer fields still report false scaling/mirroring failures because they omit board gain and donor dithering; raw payloads are retained, not relabelled PASS. The unchanged board-gain formula was verified in the prior scoped regression.

## Timing for these exact two recordings

| Metric | Baseline | Candidate |
|---|---:|---:|
| Samples | 108 | 108 |
| Render duration minimum | 1.523 ms | 1.547 ms |
| Render duration median | 1.546 ms | 1.564 ms |
| Highest sampled render duration | 1.754 ms | 1.768 ms |
| Added publication deadline misses (5 ms publication budget) | 0 | 0 |
| Added skipped LED releases | 0 | 0 |
| Added emit errors | 0 | 0 |
| Highest observed capture-to-publication software interval | 10.875 ms | 10.598 ms |

Cycle conversion uses the current 1 GHz clock. These are sampled observations, not exhaustive worst-case bounds. `physical_ts_valid=0`; software timing is not microphone-to-photon latency. Reset stale-clock protection counted 13 baseline / 41 candidate prevented early reset admissions. Source `ws281x_gpt_dma_hw_pair.c:924` identifies these as the existing fresh-clock safeguard, not missed render deadlines; output errors and skipped releases remained zero. The safeguard was preserved.

[Implemented source/test diff](../../_scratch/continuous-bloom-r2/physical-001/final-001/candidate-source.patch). [Detailed audio-to-light mechanism and named regression evidence](CONTINUOUS-BLOOM-R2.md#exactly-how-audio-becomes-material).

## Scoped correction, failures and remaining closure

The owner observed excessive body spread in the first pair. Narrowing outgoing/return width and trail reduces the outgoing spread from approximately 360 ms to 136 ms (width × travel plus trail). The detail board, travel times, return strength, attack/release and global output stayed unchanged. [Clean before/after run](../../_scratch/continuous-bloom-r2/physical-001/spacing-002/original.mov) and its per-sample settings establish this one-mechanism correction. The final comparison then used the narrowed candidate against the retained baseline.

The earlier `spacing-001` comparison remains **INVALID**: external controls changed palettes, modes, scenes, ML and screensaver. The owner stopped writes. Captain subsequently confirmed the mixer was no longer being manipulated, permitting the clean repeat. [Invalid-run evidence](../../_scratch/continuous-bloom-r2/physical-001/configuration-integrity.json) is retained. Other retained failures: initial 116-second camera request exceeded its bounded reserve; 110/112-second corrected requests completed. Local ffmpeg lacks drawtext; title strips outside the plate replaced it. Neither is counted as physical success. The earlier camera unavailable/brown-view records remain historical failure evidence.

Source/diff scope: `emotiscope_port.cpp` (coherent publication and response stage), `musical_conditioning.h` (per-board elapsed time and single attack), `composed_scene_runtime.h` (consistent silence units and independent admission), `composed_motion.h` (continuous retained history), focused production tests, and this one existing preset. Adjacent `emotiscope_port.h` carries the bounded amplitude/publication fields required by those consumers. `titan_broker.py` changed only because the existing payload capture needed the admitted live candidate identity; wrong identity rejection is retained. This continuation changes only the preset's three body-spread values and the existing delivery records. No new effect, choreography, ML target, output limit or framework.

Commands and receipts are linked above: established `deploy_titan_image.sh` for baseline and candidate; existing camera helper/afplay plus bounded wire snapshots in `run_final.py` and `run_probe_final.py`; `review_final.py` for the derived comparison; `verify_packet.py` for identity/settings/timing/media consistency. The final integrity command passed through governance receipt `09bdeb607bcbe62d2b8671f6270367b0aee09f75c343c5a26975ee5891b5021a`. Formal `workflow_accept` returned “Bind an assignment before admission”. The final gate reports no ledger bound and explicitly is not a completion certificate. No replacement registry or acceptance wrapper was invented.

Remaining path is specific: (1) qualify an actually below-floor received-input interval through the physical chain, distinguishing ambient microphone input from silence, and verify no births while prior history drains; (2) Captain judges preference from the supplied final matched clip. The acoustic playback method did not provide that verified quiet interval, and the current live control interface supplies no admitted zero-PCM injection route. This limitation is not a camera/firmware deployment blocker and has not been disguised as full completion. Preserve this usable running candidate and retained baseline; all parked work stays parked.

## Execution handover — Captain mandate, 12 October 2026, 21:43 AWST

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

Execute this continuation through implemented changes, deployment where firmware changes are necessary, physical review, correction and delivery. Preserve the complete original assignment and historical results above. This is the active continuation; the preceding “remaining path” is retained as the earlier handback, not a reason to wait for an artificially below-floor room or for Captain to diagnose the effect.

### Finish line and starting point

Deliver an installed, saved Body/Detail Bloom composition in which successive low musical attacks make distinguishable centre-origin lower-edge deposits while the upper detail remains sustained and independent. Existing material must continue travelling and return more weakly without obscuring every new attack. Address demonstrated unwanted post-music births without suppressing quiet music. The owner must inspect and judge the result; Captain's personal preference cannot be claimed or used to defer unfinished engineering.

Worktree: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `feat/titan-emotiscope-port-001`.
Host worktree, only if the existing control/capture path genuinely needs a scoped repair: `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001`.
Starting firmware: `7ae880ef05e13912bead58a73a520ad862507ca9`; installed build `4c94653ddc3d9d2a6a56500e1422d953a48c0be50b0486d7deaec55f6e3a43e4`; preset/evidence revision `0c75601`; UID `545433931bd25436593630352d068363`; schema `6d68932e22c0796bcac290a233df9e235410b7af724ac71112525a86017b561b`. Recheck current identity and operator settings before writes. If changed, reconcile the change rather than resetting newer work.

Use this document, `CONTINUOUS-BLOOM-R2.md`, `CURRENT-STATE.md`, and `presets/continuous-body-detail-bloom.json`. Existing recordings and capture helpers are in `_scratch/continuous-bloom-r2/physical-001/`. Use the delivered 7ae880e/SQ4 state as the immediate before-state for the next improvement; the older retained image remains recovery/history, not a mandatory additional flash.

The review inspected full-passage sampled sequences and a 10 fps movement sequence, not a calibrated audiovisual latency recording. It found tighter lower deposits but substantial broad pink/red blending through dense music. The quiet probe shows later lower-edge activity after the initial decay. Those are issues to resolve, not proof of one specific cause.

### Scope and fixed conditions

Apply current `/Users/spectrasynq/AGENTS.md`, local instructions, the product charter and established workflow. Load the existing `spectrasynq-lightshow-loop` skill for physical review and `smc-titan-mixer` before live controls/deployment. The first live command is:
`python3 /Users/spectrasynq/.claude/skills/smc-titan-mixer/scripts/smc_titan.py health`.

Keep the existing worktree, render architecture, continuous unlinked scene, independent board state, centre-origin geometry, curated multicolour palette, existing control surface and save path. Primary targets are the current preset, `platform/ra8p1/musical_conditioning.h`, `composed_scene_runtime.h`, `composed_motion.h` and their existing `emotiscope_port.cpp` integration. Change only the mechanism shown to cause the remaining visible weakness. Copied donor `emotiscope_donor/vu.h` may be repaired if evidence identifies it as the cause; the original donor repository remains read-only.

Preserve bottom P601/GPT6/DMA0 + P604/GPT8/DMA3, top P603/GPT7A/DMA6 + P714/GPT12B/DMA7, microphone P502/P812/DMA1/2, 160 pixels per optical edge split into 80+80 electrical halves, gamma, current limits and output adapter. Keep ML/background/screensaver off in this preset; new choreography, new ML targets, catalogue sweeps and new frameworks remain parked. No cosmetic light floor or black masking over continuing births.

Last verified physical settings were master 255/255, both board gains 0.25, emission and secondary ON. Preserve the actual operator-selected settings across the comparison; do not use global dimming, camera exposure or grading to manufacture better separation. The normal default minimum remains 100/255. SQ4 is host persistence and does not silently establish device NVM or restore all physical controls.

### 1. Make the musical roles visibly legible

Start from the existing Sonic Samba before-state and current preset. Body widths are 0.08/0.08 with 40 ms trail; detail widths are 0.20/0.14 with 120 ms trail; both travel 1200 ms out/1800 ms back with return strength 0.55 and one return; both musical envelopes are 40 ms attack/160 ms release.

Identify which contribution fills the gaps between lower attacks: incoming body level, body trail/return, or upper detail spread/persistence. Use the existing independent-board controls for a short isolation observation only if needed to distinguish those causes, then restore both boards for evaluation. Do not infer visual separation from band labels or payload independence alone.

Make the smallest supported adjustment in the existing preset or its owning source mechanism. Keep fresh lower attacks entering while older deposits travel. Preserve an upper response to sustained detail, brighter outgoing material and a weaker return. A useful change produces distinguishable successive lower patches and darker separation in the actual assembled plate; simply making everything dimmer or killing upper detail is a failure. Change one causal mechanism at a time and keep or revert it from the observed result; do not run an arbitrary parameter grid.

### 2. Resolve unwanted post-music activity without losing quiet music

Inspect `composed_scene_runtime.h::audible`, the producer's peak/floor publication, `emotiscope_donor/vu.h::run_vu`, and the conditioning-to-Bloom feed together. The current VU floor is 0.90 times the rolling average of 20 peaks logged at 250 ms intervals. The audibility predicate compares squared peak with that floor and a fixed minimum. Automatic amplitude scaling follows the residual. This can admit stable background above its own moving floor, but the recording does not prove that this mechanism caused every later birth.

Use a bounded music-to-room-quiet observation on the existing microphone path. Observe received peak and floor in the same publication/time context, role-conditioned feed, actual new history injection and the plate. Cached host peak readbacks and event-cursor advances alone do not establish every injection's cause. Use existing capture/publication seams; if a missing field prevents diagnosis, add only the minimal temporary observation needed for this repair.

Distinguish retained travel, fresh acoustic excitation, adaptive-floor behaviour and any invalid/stale replay. An acoustic microphone is not expected to produce digital zero in a quiet room. Do not wait indefinitely for a below-floor interval generated by the algorithm under investigation.

If ordinary steady room input is repeatedly creating unwanted material, correct the proven admission/scaling mechanism with a bounded noise margin or hysteresis derived from the observed input, preserving weak musical attacks and sustained detail. Do not guess an absolute cutoff from the three cached peak numbers. If the activity is a legitimate distinct audible sound, demonstrate that correspondence and retain the response rather than blanket-muting it.

Use the existing production-path test to cover the actual correction: the observed failure pattern, a quiet musical attack that must survive, sustained detail, and zero/stale input draining existing material. Do not create a separate test framework. Current history retention bounds after the last admitted injection are approximately 3.305 seconds for body and 3.637 seconds for detail from the existing renderer/preset; recompute if relevant settings change. Time the drain from the final injection, not solely the playback-stop request.

### 3. Install, inspect, correct and deliver

For preset-only changes, apply through the existing authoring path, save the complete two-board composition to SQ4 and verify recall plus physical output controls. For firmware changes, run the focused existing regression, build with the established dual-output options, bind the exact source inputs/image/UID, deploy through `deploy_titan_image.sh` and the E2 Lite router, then read back UID/build/schema/configuration. Existing deployment authority applies. Keep probe target power OFF and the established 1 MHz SWD route; do not introduce GDB, chip erase or another serial owner. The previously working deployment Python override is `TITAN_DEPLOY_PYTHON=/opt/homebrew/bin/python3.13`; use it if the default Python's known pyexpat failure persists.

Capture the delivered before-state versus the corrected candidate using the approved `docs/emotiscope-port-001/evidence/audio-windows-001/sonic-samba-20-44s.wav`, same playback level, assembled stack, framing and locked camera settings. Reuse existing camera/capture helpers, reviewing their image and preset bindings before reuse; historical scripts contain historical identities and must not be blindly rerun. Keep source audio alignment labelled as host-request alignment unless measured otherwise. Test tones remain engineering probes, never the review soundtrack.

Inspect the complete comparison at playback speed with music and inspect the post-music interval. Locate specific moments demonstrating stronger attack readability, preserved upper sustain and remaining blending. Correct a defect that is still visible and within scope before handback. Use the existing bounded runtime check to reject new faults, deadline misses or skipped LED releases; stop once sufficient. Timing is supporting evidence, not the visual deliverable.

| Acceptance | Required result |
|---|---|
| Musical readability | Successive low attacks form distinguishable new lower material while earlier material travels; upper sustained detail remains visible and temporally different. Give clip timestamps demonstrating the result. |
| Spatial restraint | The comparison shows improved separation in the same dense passage at matched global output and camera settings. No blanket dimming, extinguished detail or single-colour substitution. |
| Quiet and recovery | Ordinary steady room input does not repeatedly create unwanted material; retained material drains within the bound from the last injection. Quiet musical attacks still restart output. Any remaining activity has an evidenced cause or is explicitly unresolved. |
| Independence and controls | Both boards remain independent; necessary affected regressions pass; complete SQ4 recall restores authored composition and physical operating state is verified separately. |
| Deployment and operation | The exact candidate is installed where source changed; intended settings are applied; both edges are enabled; no new runtime faults or lost releases in the bounded run. |
| Actual review | The owner has inspected the physical recordings and states its own visual judgement with concrete strengths/weaknesses. Captain's preference remains Captain's decision, not an engineering stop condition. |

### Real stop conditions and closeout

Stop only affected writes for changed/unknown hardware identity, failed relevant prerequisites, concurrent operator/source ownership that would invalidate writes, native permission denial, genuine unsafe output, an explicit HOLD or a necessary out-of-scope change. State the exact resumption condition. Camera loss blocks new physical claims, not independent source repair or normal autonomous music output. Do not bypass safeguards or invent additional image approvals.

Append actual results to this existing document and update the existing preset/current-state record as needed; preserve historical assignments and failed evidence. Commit only the relevant changes. Leave the useful corrected composition running with both edges enabled and the broker healthy; capture helpers must not disarm normal output.

The final handback must lead with what visibly improved, link the actual comparison, identify exact source/build/preset/UID and leave-state, state each acceptance result truthfully, and identify any specific remaining defect with its next executable action. Do not stop at another proposal, a timing summary or “aesthetic verdict pending.” If the objective is achieved, close it and do not invent a further campaign.
