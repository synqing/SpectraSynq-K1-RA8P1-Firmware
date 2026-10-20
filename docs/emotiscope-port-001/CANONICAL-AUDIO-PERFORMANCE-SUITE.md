# Canonical audio performance suite v0.1 — execution record

Status: implementation in progress; no new physical improvement claim.

The complete Captain assignment follows unchanged. Execution decisions and evidence are appended, never substituted for its requirements.

## Original Captain assignment — preserved verbatim

Captain:
# SpectraSynq — Canonical Audio Performance and Musical Testing Suite

Execution brief • Revision 0.1 • 13 October 2026, Australia/Perth

THE TITAN MUST CONTINUE AUTONOMOUS MUSIC VISUALISATION. A recording ending must not disable either edge. Preserve the current authorised brightness, palette, geometry and controls. Curated multicolour palettes remain mandatory; no rainbow cycling, single-colour fallback or cosmetic colour floor at silence. An actual fault follows the existing fault procedure.

## 1. Decision and finish line

Develop this suite as an extension of SPECTRASYNQ-LIGHTSHOW-LOOP-01 v1.0.0 and the existing Titan capture/evaluation path. Its purpose is to make specific music-to-light defects reproducible, identify their first supported cause, and prove useful firmware improvements. The deliverable is a better physical K1 performance with reusable regression cases.

The first cycle must produce an audible, source-bound BOOM recording, a supported diagnosis, and—if a real defect is repa bounded installed correction with a matched comparison and a contrasting musical check. A source-only timing report, test count, dataset, or new dashboard cannot close that improvement claim. If BOOM exposes no defect, preserve that honest result and move to an already identified unresolved visual limitation; do not manufacture a change.

This packet is an investigation and implementation handoff. In this review, the MP3 was analysed and the named remote source files were read. No firmware, capture helper, device settings, camera or microphone was changed or activated. The proposed suite is not yet implemented or qualified. Existing in-scope deployment authority remains with the executing CLI owner; this packet does not expand it to other hardware lanes.

## 2. Verified authority and current evidence

Target worktree: /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001

Observed Git HEAD: 71404c46d62b193f3689e0737e8ca51db7a120d5. This is a documentation/source checkpoint, not the running binary identity. Re-read current state before execution because another agent may have advanced it.

Read AGENTS.md, docs/emotiscope-port-001/CURRENT-STATE.md and the latest applicable sections of INHOUSE-WAVEFORM-MIGRATION-20261012.md. Newer two-edge findings supersede historical single-edge descriptions. Load the installed spectrasynq-workflow and spectrasynq-lightshow-loop skills. Before touching controls or deployment, also load smc-titan-mixer and the existing Titan/E2 Lite procedures. Preserve current ownership and native permission boundaries; no new permission pause for already authorised repairs.

The report's latest recorded operating state is lower Hybrid mode32/palette32 and upper Octave mode2/palette41, both outputs enabled, master255 and per-board gains0.25. That is a reported state, not fresh device readback in this review. It is not a Hybrid+Tempo comparison. Preserve it as the starting composition; do not silently overwrite presets SQ1–5 to cate a test.

The report records a real top-edge half-assignment repair in 99c0cf44000471fd30a6b368bd2c18a4403adf46. Top DIN0 receives logical80–159 and DIN1 logical0–79; the lower mapping stays unchanged. Physical appearance previously disagreed with the apparently correct logical payload. Preserve the applicable mapping flag/decoder. Resolve actual geometry from current evidence rather than copying older DIN or pixel-count assumptions.

The installed family008 binary is reported as build a43e4842b3a7cf8b5150787f613b575551dc090d80779dae6474784de59cd606, verify its current identity through supported device UID/build readback and the existing binding to the hashed deployed artifact before dependent operations. Do not require an unsupported full-flash SHA readback. The report's remaining pink/red dominance and clipped highlights are unresolved observations, not an established firmware cause.

Original BOOM SHA-256: 3d60c58eccef198bb8062bef68df4d882c1b49c8f246623c4ba88d337cad8e4d. Metadata identifies Tiëstevenn — BOOM (MR.BLACK Remix). It is 44.1kHz stereo; decoded length approximately 240.071 s. Analysis time zero is the first decoded sample. The container start/encoder delay is a separate timebase issue.

## 3. What BOOM establishes—and what changes next

| Finding | Evidence status | Engineering consequence |
| --- | --- | --- |
| Build pulses become closer around31–42s | Independently corroborated; exact434→219→125→95ms sequence depends on selected events and detector settings | Test admission and coexistence separately; retain the original figures as provisional annotations |
| Strong recurrence after55.5s | Approximately435ms /138BPM periodicity corroborated across three analysed bands | Use as a tempo candidate; do not force every rapid subdivision to become the main beat |
| Later accents near107–110ms | Compatible with roughly109ms sixteenths | Useful dense-response condition; not a complete manually verified beat grid |
| Brief70–73ms gaps around55s | Present among other gaps; regulestablished | Use exact generated triplets for known-answer testing; keep BOOM's label tentative |
| Decay50–55s versus strong55.5–75s | Stereo RMS approximately−19.81 versus−6.69dBFS, a 13.12 dB difference | Tests adaptation recovery and headroom without normalising away the contrast |
| Reduced section contains audio | Not digital silence | Add explicit digital-zero and measured-room-quiet cases separately |
| Whole-file decoded sample peak+3.49dBFS | Float sample overshoot; not a true-peak measurement or acoustic-clipping diagnosis | Preserve float reference; prevent accidental clipping when creating integer fixtures |

The broad interpretation of the CLI report is sound. Its detailed event labels must not become unqualified ground truth. An onset detector has a threshold, bandwidth, minimum separation and timing uncertainty. Store those settings with proposed annotations. A short analysis hop is not equivalent to that much onset accuracy.

Three new diagnostic questions have immediate value:

1.resh attack fail to reach effect state, or did its correctly rendered deposit merge with older material? For locally constant travel speed, nominal deposit spacing is d = v × Δt. Compare that spacing with the observed footprint width, including persistence and diffusion. This relationship is explanatory, not a universal visibility threshold.
2. Does the strong section leave gain/history that suppresses the next weak event? Compare the same probe after quiet and after dense music. Correct strong-input behaviour can coexist with poor recovery.
3. Is colour lost in the final LED commands, physical mixture, or saturated camera pixels? A white camera patch alone cannot identify the cause. Do not fix a camera exposure problem by dimming the product or rewriting palettes.

## 4. Immediate defects in the existing capture implementation

These are directly inspected source findings. The executable's build identity and live device availability were not checked.

| File/location | Observed implementation | Required rrection |
| --- | --- | --- |
| _scratch/visible-dual-20261011/product_camera.swift, lines22–24,37,52–60 | Discovers video devices, checks video permission, adds one camera input and movie output; no microphone input anywhere in141-line source | Explicitly select and add an authorised microphone to the same capture session and require an audio connection |
| capture_existing.py, lines271–274 | Rejects audio that is not24s | Validate the selected manifest entry's actual frame count, rate and duration |
| Same, lines326–328 | Own camera recording fixed at44s | Derive required duration from pre-roll, passage, post-roll and existing reserves |
| Same, lines364–365 | Aborts observation after42s, including external-camera operation | Derive observation deadline from the actual passage and camera's remaining authorised deadline |
| Same, lines377–378 | Accepts playback completion after at least23s | Check completion against that specific fixture's expected duration and recorded playback/audio evidenceines202–234 | External-camera verifier assumes old6/7-argument command shape | Update command parsing and identity checks together with explicit microphone selection |
| Same, lines423–435 | Records the played file but no captured audio-track validity | Record actual captured track identity, sample presence, coverage and passage correspondence |

Full helper path: docs/emotiscope-port-001/evidence/waveform-migration-001/capture_existing.py beneath the target worktree. Inspected SHA-256: 273a005bc901a9be20170e0a8e7cc354049d9a1d1ac8a6d98a49a0c3092cebe3. Camera Swift source SHA-256: 48de5485e82c4184c51701575ab1a86a758048e15e243b323dde92d90a3dd640.

The absence of model audio perception does not prevent recording sound. An iPhone having a microphone does not automatically add it to an AVFoundation session. Repair the actual missing input; do not repeat video-only captures and then call recording impossible.

## 5. Canonical suite content

Canonical means fixed source identities, reproducible initial conditi explicit effect expectations, known evidence boundaries and stable evaluation rules. It does not mean all effects must interpret every transient identically.

| Case family | Minimal stimulus | What it can improve |
| --- | --- | --- |
| Continuous musical transitions | BOOM played0–75s; score28–75s | Dense admission, decay, re-entry, history and colour structure |
| Paired attacks | Same known waveform at separated, overlapping and dense intervals; exact event sample indices | Retrigger suppression versus intended overlap, footprint and release |
| Tempo versus subdivision | Fixed underlying pulse plus progressively faster rolls; distinct true-tempo-change case | Prevent subdivision chasing without freezing genuine tempo adaptation |
| Weak after strong | Identical weak event after quiet versus after dense input | AGC/normalisation history, stale gates and recovery |
| Sustain and release | Held material, controlled stop, complete meaningful tail, restart | Frozen history, premature blackout, lost rel and phantom new injections |
| Known mixtures | A alone, B alone, then exact A+B with documented gains and headroom | Masking, competition and preservation of useful foreground/detail |
| Real musical contrast | Existing Sonic weak/sustained windows and Carte context | Transfer beyond one loud drop |
| True quiet | Digital zero at a declared replay boundary; separately stop playback and measure room input | Separate mathematical silence from real microphone quiet/noise |
| Two-edge interaction | Fixed input while changing only one edge's control through the real owner | Configuration/state isolation and output under dual-edge load |
| Learned contribution, when justified | Model off/on and simple deterministic alternative on identical causal input | A measurable new visible function, with bounded cost and fallback |

Initial coverage uses BOOM, one matched explanatory probe, an existing weak or sustained counterexample, and a late frozen holdout check. Select only the families required by the reproduced problem. Do not run a factorial matrix across all five effects, tracks, palettes and levels.

Reuse existing audio-windows-001/manifest.json. Available entries verified at the cited checkpoint include Sonic Samba20–44s,6–30s and51–75s, plus Carte Blanche0–24s and20–44s. These times refer to retained source files, not independently verified commercial-release timestamps. Do not call any short window representative of an entire genre. Gareth Emery—Dopex is already held out from estimator tuning; preserve that designation.

BOOM is now development material, not an unseen holdout. Carte has prior comparison history and is a transfer/regression check, not a pristine blind test. Group all windows and derivatives of a song when separating development from evaluation. If a holdout failure informs a change, record that exposure; do not keep calling it untouched evidence.

## 6. Ordered execution packages

### WP0 — Bind the actual starting point

Owner: the existing CLI orchestrator. Preserve unrelated duse the existing isolation/checkpoint procedure. Capture current Git source/diff identity, exact deployed artifact hash and supported device-build binding, live UID/build, input path, pixel/edge mapping, complete selected settings and operating owner. Read the relevant existing effect path from acquisition through final packing. Do not infer active behaviour from menu membership.

Use the reported lower Hybrid as the first target unless live readback shows a later intentional selection. Preserve the upper composition as the peer/control. A dedicated Tempo experiment is a subsequent named configuration, selected through the existing control owner and restored afterwards; do not silently redefine the user's baseline.

GO when target and ownership match the current authority. Unknown identity blocks dependent writes, not the source audit or case definition. Snapshot/reported state must never be presented as a fresh device read.

### WP1 — Repair audiovisual capture and passage duration

Modify the existing Sft camera helper and Python observation helper in one bounded capture change. Identify the reproducible build command and bind the resulting executable to source. Do not create another recorder, daemon or dashboard.

Inventory stable audio-device IDs and permission status. Prefer the available iPhone microphone; if unavailable, explicitly select and record the MacBook microphone. Add that input to the existing session with the camera. Check canAddInput and usable audio/video connections before declaring readiness. Honour native microphone permissions; an OS denial blocks that recording operation and must be reported exactly, without bypassing it.

Keep existing framing authority, single ownership, cleanup and120s operation bound. Preserve the external100/112s owner's checks while updating their argument parsing to include the selected microphone. Never stop an external owner's capture. Parameterise all24/23/42/44s assumptions together. Calculate remaining reserve using actual elapsed startup, complete playback, post-roll and finalisation—not just the nominal clip length. Reject a run that cannot finish within its existing bound. Do not simply delete deadline checks.

After finalisation, require an actual video track and decoded microphone samples covering the scored musical interval. Confirm correspondence to several distinctive source passages, not just nonzero room noise. Preserve the original recording and timing metadata. A separately muxed original MP3 may be a labelled presentation soundtrack; it is not received-audio evidence.

Use one short, already authorised EDM passage to prove the repair captures real music before a long run. Necessary negative checks: missing microphone/track is rejected; wrong fixture hash or format is rejected; inadequate remaining camera time is rejected; an early player exit cannot pass as a complete75s passage. Existing24s fixtures remain compatible. These checks target the observed failures; no broad test-count campaign is required.

### WP2 — Add a source-bound BOOM caRetain the original MP3 and its hash. Decode to a float analytical reference without clipping. For an integer fixture, calculate one constant attenuation from the complete derived signal's sample peak plus declared headroom; check again after downmix/resampling. Record gain, channel treatment, filter/rate, decoded time origin, sample count and resulting hash. The whole-file sample peak implies at least3.49dB attenuation before additional margin if mapping that unchanged float signal into nominal full scale; a different transformation requires its own peak check.

Keep this derivation gain separate from speaker level, microphone gain and LED brightness. Do not separately normalise the weak and strong windows. Do not confuse a numerical sample-peak check with intersample true-peak or acoustic clipping qualification.

Add the fixture to the existing manifest with scored intervals31–42,46–50,50–55,55–55.5 and55.5–75s. Keep descriptive annotations and uncertain rhythmic labels distinct from exact synth times. Add an inspectable event list only if a decision needs those individual events; do not spend another campaign refining a tentative triplet label.

For the first canonical natural-history capture, play from0s through75s and score28–75s. Starting directly at28s does not recreate the preceding adaptive/tempo/history state. A shorter cue is allowed later if the actual state dependence has been established, or if it is explicitly a controlled-reset test. Do not score through75s while ending the source at74s.

A75s passage plus2s pre-roll and12s post-roll requires89s of observation before additional start/finish reserves. It fits only if the actual remaining bounded capture window allows it. If the meaningful tail outlasts the available post-roll, mark that tail truncated and use the existing bounded procedure for the specific remaining observation. Do not lengthen unrelated recording authority.

### WP3 — Record one physical baseline and locate the loss

Keep the baseline's effect settings, brightnesalettes, speaker setup and input path fixed. Record both real edges and real sound. Capture only the explanatory fields needed: source/received sample or block timing, relevant level/normalisation state, accepted event or deposit information if the effect actually uses events, tempo/confidence where consumed, history position, final native edge payloads and existing fault/drop counters.

Trace the actual code semantics. A continuously sampled waveform effect need not have a discrete onset gate; name the consumed feature and deposited material rather than inventing an event API. Existing sampled wire snapshots may demonstrate geometry but can miss72ms events. If event attribution requires denser evidence, use the smallest bounded nonblocking trace/capture at the real processing or submission boundary. Quantify its overhead; a full new logging platform is unnecessary.

Locate the earliest supported discrepancy: received audio → consumed feature → admitted/deposited state → final native commands → tranobserved plate. A correct buffer does not clear DMA or physical mapping. A clipped camera does not prove optical saturation. Record the exact failing interval and what observation would distinguish the remaining alternatives.

Full-speed audiovisual review answers whether motion makes musical sense. Traces answer where a defect arises. If this agent cannot perceive audiovisual playback, provide the usable audible clip and complete the measurable engineering work; label the perceptual verdict unassessed and route only that verdict to a capable reviewer. Do not leave capture or an evidenced repair undone because one review capability is unavailable.

### WP4 — Make the smallest justified firmware improvement

| Observed boundary | Candidate intervention | Reject the intervention if |
| --- | --- | --- |
| Received signal loses/overloads the intended feature | Repair input scaling/acquisition or actual preprocessing fault | It hides a downstream effect defect or destroys quieter material |
| Feature exists b deposit is suppressed | Correct the actual gate/retrigger/state update | It causes room-noise injections or converts every subdivision into a main beat |
| Deposits exist but overwrite previous travel | Repair history/update semantics | Older material freezes or new foreground is delayed |
| Valid deposits become a broad wash | Adjust the minimum coupled footprint/decay/foreground relationship; preserve current motion intent | Separation improves only by abandoning colour, quiet detail or approved intensity |
| Tempo follows roll density | Use the existing tempo/confidence/hysteresis mechanism to retain a stable metrical interpretation | Genuine tempo changes cannot be followed, or phase discontinuities increase |
| Weak event fails after loud history | Repair adaptive recovery or stale state | Strong passages clip or noise becomes a false foreground |
| Camera alone clips | Repair exposure/capture validity first | Product settings are changed solely to disguise the recording defect |

Do not hardcode BOOM's timestamps,138BPM or track identity into firmware. Do not require one visible flash per detected transient. Define what this particular effect should preserve, emphasise and intentionally suppress before scoring it.

Keep the baseline immutable. Change one mechanism, or the smallest justified coupled set. Run relevant source/host checks, build the exact candidate, use the established UID/build-bound deployment route, read back identity/settings, and restore autonomous two-edge music. Do not re-run all144 ordered effect pairs unless the change touches their shared boundary or an existing required gate demands it.

### WP5 — Prove the improvement and test its opposite failure

Replay the same BOOM case with the same initial-state rule and settings. For a dense-input correction, use Sonic6–30s or another already identified weak case to expose over-aggressive thresholds or lost detail. For a release correction, include sustained material and an explicit quiet/restart test. Use the matching baseline and cdate conditions; repeat when variation could explain the result, not to inflate run counts.

Camera exposure/WB reacquire separately in the current helper. Matched numerical colour/brightness claims therefore require a supported way to maintain or reproduce the relevant settings. If unavailable, explicitly restrict the comparison to valid structural/temporal findings and report photometric uncertainty. Do not assume that two sessions with the same mode names have the same exposure. In one session, do not deploy firmware in violation of the existing capture ownership contract merely to force a matched comparison.

Once the candidate and evaluation are frozen, evaluate the existing Dopex holdout through the approved path. A passing single holdout is bounded transfer evidence, not universal musical performance. Keep the useful correction only if it improves the declared target without violating the protected visual, timing, control and quiet/recovery requirements.

### WP6 — Extract the reusable suite from t completed loop

Extend existing case/run records with any genuinely missing fields; keep their schema compatible. Reuse current evidence locations and large media by hash/path. Suggested authored specification: docs/emotiscope-port-001/CANONICAL-AUDIO-PERFORMANCE-SUITE.md. This is a proposed destination, not an existing verified file.

Keep the hardware adapter and evidence in Titan's lane. Reference the suite from the existing SpectraSynq-PRISM lightshow-loop work. Shared contracts/scoring can move to the established PRISM shared location when a second actual consumer needs them; do not merge S3, DualMCU or Titan repositories or repin PRISM-Core merely to declare a suite.

Minimal record: case/version and source lineage; playback/scored windows; reset/preroll; effect expectations; source/build/device/topology; parameters and gains; capture/input boundaries; timing map/uncertainty; actual raw evidence; evaluator version; result per claim; retained/rejected correction and reason. Unknowns remain explicit. One record should connect the data, not duplicate it across registries.

## 7. Measurement and acceptance contract

Define each candidate's criterion before evaluating it. Use existing authorised numerical requirements where applicable. For a new comparative question, freeze the baseline, rejection conditions and measurement margin first. Do not invent a global latency limit, minimum dark-pixel percentage or beauty score and call it a product requirement.

| Claim | Method | Passing evidence |
| --- | --- | --- |
| Valid musical capture | Decode actual audio/video; inspect coverage and source correspondence | Audible intended passage, complete scored interval, known capture limitations |
| Identity and invariants | Fresh target/configuration readback and applicable source/runtime checks | Correct candidate/setup; no unintended palette, brightness, topology or peer-control mutation |
| Admission/history | Known probes plus actual consumed state and final payload timing | Intended material survives the named boundary; no unexplained missing/duplicate injection |
| Visible improvement | Matched full-speed AV and relevant spatial/temporal trace | Declared defect improves without the specified regressions; judgement is attributed to an actual reviewer |
| Tempo coherence | Beat/phase and tempo trajectory, lock/reacquisition, metrical-switch count | Effect maintains its intended pulse interpretation and follows actual changes within the predeclared contract |
| Quiet/re-entry | Last input/injection, room/zero distinction, tail and restart | No unintended fresh deposits in declared quiet; intended tails may continue; response resumes |
| Two-edge independence | Change A only; inspect B settings/state and both outputs | No unintended causal mutation of B; shared audio correlation is allowed |
| Runtime integrity | Existing image-bound counters and pertinent deadlines under dual-edge load | No new attributable faults, drops or stalls; any pre-existing fault is disclosed and not passed |
| Transfer | Fixed candidate/settings on contrasting material and frozen holdout | Claimed improvement survives the stated counterexample; scope remains bounded |

Report PASS / FAIL / BLOCKED / NOT TESTED per requirement; use INCONCLUSIVE for a measurement whose uncertainty cannot distinguish outcomes. Capture-invalid is not equivalent to a firmware failure, nor permission to pass the firmware.

Response delay: separate received-audio event time, feature availability, admission, output submission and first visible consequence. Report the measured stages and gaps. A camera microphone is an acoustic proxy, not automatically the MCU's received PCM. Driver completion is not photon emission. Show median/tail statistics only with their sample counts; do not infer a reliable p99 from a handful of events.

Rhythmic alignment: use signed phase error relative to an explicitly annotated pulse and the effect's intended metrical level. Distinguish coherent half-time interpretation from uncontrolled toggling. Correct tempo alone does not prove beat-phase alignment. Do not silently discard startup or reacquisition intervals.

Dense events: use one-to-one matching and report unmatched events. The mir_eval beat F-score defaults to70ms, almost the claimed72ms BOOM subdivision. That default cannot establish dense attack resolution. Predeclare an effect-appropriate window narrower than half the relevant minimum event spacing after accounting for timing uncertainty. If the evidence cannot resolve it, report the limit and use exact probes or a better observation boundary; do not stretch the tolerance until it passes.

Travel and readability: measure trajectory/width/separation in verified native pixels or calibrated plate coordinates. Retain time order and per-edge regions. Compare new foreground against earlier travel and darker gaps. Global brightness correlation, average saturation or a pixel histogram cannot replace the sequence. Camera RGB is not calibrated luminance or a human colour metric.

Synchronisation: preserve original audio sample indices and video presentation timestamps. Estimate offset and drift using independent landmarks spanning the run; retain residual uncertainty and discontinuities. Never align light peaks to audio peaks and then claim the removed delay as low latency. If separate microphone audio must be used, retain a defensible shared clock/sync mapping; merely muxing two files does not create one. Exposure duration and rolling shutter limit top-versus-bottom timing comparisons. Bound that uncertainty instead of assuming frame_number/fps.

Limits apply to claims individually. Missing absolute microphone-to-photon calibration does not prevent an observable repair, valid relative comparison or audible review. A useful timing result need not require new photodiode hardware unless the specific decision cannot otherwise be resolved.

## 8. ML lane and future function deployment

Include ML in the canonical contract now; develop it when there is a named visible function it can improve. The test suite should answer whether inference changes the actual show usefully, not merely whether model output matches a reference vector.

Potential first functions, selected only after evidence: distinguish roll density from stable beat for Tempo; estimate phrase/intensity state to shape bounded foreground/persistence; or choose existing renderer parameters that retain detail across changing density. Compare each with a simple deterministic baseline. Do not train a model just to reproduce renderer logic already available in source.

For an already installed model, record exactly which visual decision consumes it. Evaluate model-off, model-on and the simple alternative using the same received input/history. For a new model, start in shadow mode, then expose only bounded outputs after measured usefulness and runtime checks. State input window, latest available sample, update rate, confidence, output age and control bounds. The live path may use only available past/current samples plus explicitly budgeted lookahead.

Reject stale, nonfinite or out-of-range inference. Fall back through the existing deterministic renderer with a defined transition, preserving valid musical operation; no surprise blackout or uncontrolled brightness. Measure device execution, memory and scheduling impact, including interference with audio/render deadlines. On-device inference success alone is not visual acceptance.

Split by whole song/source lineage and relevant session/probe family; never random neighbouring frames. Keep preference labels attached to entire gestures/passages and their settings/history. Retain rejected runs as diagnosis, not positive imitation examples. Keep SAM teaching, large dataset acquisition, NPU purchase/porting and broad model comparisons parked unless a specific proved limitation makes them necessary.

After the first loop: qualify one dedicated Tempo case; then extend the same contracts to another effect only for its distinct behaviour; then consider a bounded learned controller. Further platform adapters follow actual reuse demand. No catalogue sweep is implied.

## 9. Orchestration, gates and handback

One owner controls the live Titan, Surface, camera and deploy path. Parallel workers may own disjoint read/analysis or source-change scopes: stimulus/scoring, capture repair, firmware diagnosis, independent acceptance review. They do not all operate the bench concurrently. The orchestrator checks their evidence at the capture-repair boundary, reproduced-defect decision, candidate selection and final replay. Nudge a worker away from unbounded annotation or infrastructure work when it no longer changes the next decision.

GO1: verified identity and ownership permit the next already authorised operation. STOP only dependent writes for ambiguity, genuine HOLD or native denial; preserve unaffected progress.

GO2: actual audible capture and complete passage permit audiovisual judgement. Failed capture triggers the named capture repair. Numerical source work and independent firmware diagnosis continue with bounded claims.

GO3: reproduced discrepancy and a supported mechanism permit a targeted correction. No observed firmware defect means no speculative repair; retain the baseline and address the next actual unresolved visual problem.

GO4: relevant software checks, exact image identity and current deployment authority permit installation through the existing E2 Lite route. A failed required check cannot be bypassed or replaced by a hand-authored PASS. Genuine safety faults follow the established stop/recovery procedure.

GO5: candidate-bound comparison plus counterexample permits the stated improvement claim. A missing perceptual verdict remains open; numeric engineering results remain deliverable. Founder taste can choose A/B/both/neither, but routine collection and analysis are the agent's work.

Deliver in the existing lane: exact changed files and commits; installed image/hash/readback; source/fixture manifest; one audible baseline/candidate comparison with intact originals; the specific visible finding and mechanism; concise per-criterion results; retained/rejected settings; remaining limitations; exact leave-state with autonomous two-edge music. Include the next concrete action only where work remains.

The immediate instruction is: repair the missing microphone input and all duration assumptions; capture BOOM 0–75 s on the current verified composition; explain the first real loss of musical structure; implement the smallest warranted correction; replay BOOM and its contrasting check; then preserve that successful procedure as suite v0.1.

## 10. Sources and reproduction

Primary project sources read for this review: the worktree AGENTS.md; INHOUSE-WAVEFORM-MIGRATION-20261012.md, especially lines1503–1540 at 71404c4; existing audio manifest and capture helper; camera Swift source; installed spectrasynq-lightshow-loop SKILL.md and canon/capture/probes/learning references. Source findings are distinct from fresh live-device observations.


$global-skill-router $thinking-toolkit:thinking-router $thinking-bayesian $orchestrator-briefing-discipline $autonomous-agentic-build

## Approved execution addendum

The approved execution details are retained in the conversation; concrete evidence follows below.

## Execution evidence

- Starting source: `71404c46d62b193f3689e0737e8ca51db7a120d5`; fresh live binding at `_scratch/waveform-migration-001/canonical-audio-v01/binding-001/`.
- Fresh selected composition: lower Hybrid K1 mode32/palette32; upper Octave mode2/palette41; both edges enabled; master255; gains0.25; mirrors on. SQ1–SQ5 untouched.
- Installed build: `a43e4842b3a7cf8b5150787f613b575551dc090d80779dae6474784de59cd606`; UID `545433931bd25436593630352d068363`.

### Captain composition amendment — 13 October 2026

Captain explicitly replaced the upper Octave contribution with Waveform Tempo and requested another palette, suggesting Neberius Gold. The actual installed curated palette is **Naberius Gold, ID40**. Applied through the existing Surface owner and freshly read back: upper mode18/palette40; lower mode32/palette32 unchanged; master255, gains0.25, both enabled, mirrors preserved. Receipt: `_scratch/waveform-migration-001/canonical-audio-v01/top-tempo-gold-selection.json`.

The completed `boom-baseline-001` used the former Hybrid/Octave composition. Retain it as historical evidence; it cannot be called a matched firmware comparison against the newly selected Hybrid/Tempo composition. All subsequent baseline/candidate observations use the new pairing. Original brief above remains unchanged.

### Palette correction and S3 reference work — 13 October 2026

The preceding ID40 name was incorrect: Titan control IDs are one-based.
ID40 is Ultraviolet Ascend; ID41 is Naberius Gold. Captain requested a changed
upper palette, so the actual later selection is upper Tempo18 / Crimson Gold39,
lower Hybrid32 / palette32. The complete readback is retained at
`_scratch/waveform-migration-001/canonical-audio-v01/top-tempo-crimson-gold-selection.json`.
Both outputs, master255 and edge gains0.25 were preserved. The earlier receipt
and `boom-tempo-gold-baseline-001` remain historical evidence of18/40; they
are not Crimson Gold footage. No physical capture of18/39 is yet claimed.

Captain subsequently authorised repairing the connected original S3 reference
B489A500 on usbmodem11301. The installed eff9bf47 / k1_bench_im69d_led150 image
uses150 pixels and the old WS2812 route; its capture flags are absent. Firmware
repair is owned in `/Users/spectrasynq/SpectraSynq_K1_Firmware`, branch
`fix/b489-ws2816-reference-capture`, under its dated complete handover. Confirmed
requested LED pins are15/16 lower,17/18 upper,80 pixels per DIN. Microphone
DATA13 / CLK14 is preserved. The installed input is mono RIGHT from the dual
IM69D130 board, not simultaneous two-microphone capture. Two raw samples blocks
contain32 copies of -30935 each; that input is not a music-valid comparison.
The component/acquisition cause is unresolved. Titan remains unchanged and
autonomous while this separate reference repair proceeds.

Existing capture repairs and source-bound BOOM fixture are implemented, with
real microphone/audio-track evidence in sonic-proof-001, boom-baseline-001 and
boom-tempo-gold-baseline-001. Sonic recorded three new capture rejects; BOOM
recorded none. The focused production-host regression reproduces a reachable
queue-pressure loss mechanism and a bounded drain correction, but no Titan
firmware correction is installed yet. These findings do not close WP4/WP5 or
perceptual acceptance, and no Dopex holdout claim is made.

### Direct takeover and queue-pressure candidate — 14 October 2026

Captain's agreed32/18 pairing and32/39 palettes are restored through the sole
Surface owner, gains0.25/master255/both outputs ON. Spectrum runs are outside
this composition's acceptance. New BOOM002 and Sonic6–30 baseline recordings
have complete received audio, valid landmark alignment and no new captured
queue/render faults. Run001's cropped right end is retained as a framing failure.
Current precision1 and recorded other control differences define these baselines.

Predeclared correction: replace only the due-frame early break with a call to
the existing palette_step(true) and continue the unchanged three-hop maximum.
The model-only palettes.renderDue() proposal is not production code.
Keep queue capacity, DSP, acquisition, gain, palettes, mapping and controls fixed.
The targeted host regression must compare the repaired production loop's54/110
PCM hops and chronology to the existing serial oracle, retain old-loop loss
witnesses805/753, and make a restored-old-loop lossless assertion fail.
Target replay must add no attributable queue loss, stream discontinuity, render
skip or LED fault and must preserve complete settings and autonomous output.
Historical Sonic rejects establish a real input-loss boundary; host schedules
remain conditional reproductions, not a reconstruction of the unrecorded cause.
The current baseline did not reproduce that loss. No latency or aesthetic
improvement claim follows from the regression alone. Full-speed musical review
remains UNASSESSED due to explicit native audio-input capability refusal.
Dopex stays frozen until the candidate and these evaluation criteria are frozen.


### 14 October 2026 additive checkpoint — first queue repair installed; coupled admission repair under test

Captain's direct execution instruction supersedes orchestration-only work. Native workers remain stopped. Titan is running lower Hybrid K1 32/palette32 and upper Tempo18/Crimson Gold39, scene0, both outputs enabled, master255, gains0.25; SQ1–SQ5 were not written. The sole broker/controller remain in use.

Commit f79d302cf91cdb97e43c34e4bbb8269845f8e85e installed exact build609e8cf061959106968c6961fc8dadec4d0bde9ebe1a9581a8de23c81698fb14 via arm-queue-pressure-009 and programme-queue-pressure-009. Programming briefly interrupted the application; fresh UID/build/schema matched, then staged control restoration recovered all63 saved writable fields per board. The first full upper63-field patch was rejected as forbidden_or_invalid_field; selected mode/output fields followed by only the11 remaining differences restored the exact state. No cause involving output reinitialisation was established.

The first repair is INSUFFICIENT: before playback, two capture rejections discarded2098 samples. Actual capture-service witness: poll-entry filled161, rejection filled753, third pair, reject_poll_cycles4597276; poll_max_pairs3/poll_max_cycles6710912. Two296-sample pushes reached753 and the third would reach1049 in a1024 ring. BOOM on009 added no capture loss, but that does not clear the startup defect. The Sonic009 recording failed its last audio landmark (correlation0.3166/margin0.0997); retain as capture-invalid, not firmware proof. Dopex remains unplayed; the009 evaluation freeze is superseded by this defect.

The coupled correction is limited to pdm_target.c/.h and the fixture owner: before acquiring another waiting raw pair, ask the existing sole main-loop AP/render owner for at most three hops of queue room; never call from an ISR, retain no raw owner during callback, retain explicit loss fallback, and bound the producer pass to its physical slot count. No microphone acquisition, DSP, slot, gain, calibration, pin, output correction or effect implementation changes.

Sanitised host regression queue-pressure-admission-fix-003.json reproduces the161/753/third-pair loss with callback absent, and preserves110 exact serial-oracle PCM/source-index hops with callback enabled (one drain/one render, zero discard). Restoring the loss fails the required assertion with exit-6. Two prior failed test attempts are retained: an incorrect constructed ASRC prefix, then unnecessary drain after no raw pair remained. The latter was repaired by a READY look-ahead before callback, while actual ownership remains validated by acquire. Target AP/render costs and raw DMA liveness still require the010 installed run. Full-speed audiovisual perception remains UNASSESSED in this session; native audio input was omitted as unsupported. Intact microphone MOV evidence remains usable.

Captain reframed the iPhone again; preview003 is being captured for direct framing inspection before the next long recording. Existing helpers are reused unchanged.


### 14 October 2026 — coupled queue-admission correction installed (010), baseline and weak transfer captured

Commit09ae38717461b10c221928c0405b74e8182feadb owns the five-file correction and focused regression. ARM build9ab90873fa37c4fadb5afca1f02699e603af082635eb4084baf5979f7c8660d7, HEX329697392d2d00aad834f99436a57d30f0d8550162e8dc8f058f645d00983105, ELF1b10dd6ccfe4be57d2885dd591f3119aa7ef76b7a520bbaeab169c75077d7dd6; all213 receipt inputs match HEAD. Only fixture_app.cpp and pdm_target.c/.h compiled inputs changed from009. Text343660/data191128/bss499476; DTCM scratch remains2848B. The callback fits existing memory padding; no queue enlargement.

Guarded E2 Lite programming08:48:35–08:49:04AWST, probe powerOFF/1MHz. UID/build/schema matched08:49:14. Broker53500 replaces33722 through the authorised deployment procedure; controller79093 retained. No full upper63-field transaction retry: selected modes/palettes/outputs first, then actual differences; all63 writable fields per board match the before-deploy operator state. Both outputs continuously run after restoration. Legacy boot defaults remain unselected Spectrum/top-disabled; M23 fixed boot delivery remains incomplete, not silently replaced by the current live selection.

Root opened preview003/frame.png: full plate and both lit edges visible. Captain's latest reframing is retained. BOOM010 original MOV contains real iPhone microphone audio; landmarks correlate0.8527/0.4736/0.9284, map uncertainty0.020s, scored coverage complete. Zero push rejects, discarded samples, stream-epoch changes, raw pair drops, new skipped releases or emitter faults from boot/restoration through this run. Capture poll maximum4,482,612cycles at1GHz; snapshot includes actual AP/render work but does not establish physical microphone-to-photon latency. Root opened source28/60/85s plate frames: both edges illuminated, central gap, pink/red/white rail highlights during music, dim warm residuals during tail. These are stills, not a musical verdict. Separate camera sessions/framing prevent photometric superiority claims.

Sonic6–30s010 recorded actual received audio with landmarks0.7526/0.7710/0.8195; complete coverage, unchanged controls, zero input loss or new output/skipped-frame counters. This valid run is a different installed image and Captain-reframed camera; it does not overwrite the failed009 Sonic evidence. The source-bound host witness establishes why the producer admission changed, not why a prior microphone AV landmark failed.

The010 evaluation is frozen in canonical-audio-v01/admission-fix-evaluation-freeze.json before first Dopex playback. Dopex holdout is now being captured on unchanged image/settings; no holdout tuning. Before this freeze, only its retained lineage/hash/header were checked. First supported loss remains the queue-admission boundary at startup/control restoration; no first loss of musical structure or visual superiority has been established from sparse wire traces/stills. Engineering comparison: queue-repair-boom-engineering-comparison.json.

Existing complete boot consumer is reused. UNSELECTED-current-hybrid-tempo-crimson-v8-20261014.json prepares all63 fields for the current32/18,p32/39 composition; schema bool output_precision encodes integer1 readback as true without changing its meaning. It has not been selected, built as final startup, flashed or saved over SQ1–SQ5. Captain's M23 selection and full-speed physical qualification still bind.

Native workflow_context for actual Titan worktree with existing key56d249405343f8282f08d0b9 refused: Session/project mismatch: not a registered worktree of the launch project. Do not use a PRISM receipt as Titan acceptance or forge another key. Actual guarded deployment and domain tests remain recorded; no governor completion certificate is claimed.


### 14 October 2026 — frozen holdout complete; current leave state and startup decision

Dopex75s holdout010 completed once after the frozen candidate/evaluation, using the retained approved fixture. Real-microphone AV landmarks correlated0.7666/0.7703/0.5268 with0.020s mapping uncertainty and complete scored coverage. No input rejection/discard, stream epoch/skew/stale changes, raw lane overflow/error/recovery, new skipped releases or emitter faults. Root opened actual source28/60/85s frames: both edges and entire plate visible during music, dark plate at85s tail. No tuning or production change followed this holdout exposure. BOOM/weak Sonic/Dopex transfer evaluation is admission-fix-transfer-evaluation.json. Musical aesthetics remain UNASSESSED without a capable full-speed audio/video perception route, not passed by those engineering results.

The first final-scoring attempt incorrectly asserted exact symmetry for every Tempo frame and exited1. That was an added blanket check, not the frozen top-mapping criterion. Fresh comparison found asymmetric frames in both the retained baseline and009, and actual inhouse_waveform.cpp:484–487 intentionally mirrors mode18 only during injection; its drain path can retain asymmetric history. Keep the source behaviour and all observed mismatch counts. Every captured upper tap carries TOP_DIN0_RIGHT_DIN1_LEFT and nativeRGB16 mode18; lower mode32 remains symmetric. The actual compiled effect and top-packer inputs are unchanged. Physical mapping/still evidence and source preservation remain distinct from full-speed release judgement. Do not repair source Tempo merely to force a blanket mirror check green.

Prepared current full boot profile SHAa166a3fb79dd1c9a99f76dbd0520a3dcbe1f8e03046def98951668437470d27b compiled as UNSELECTED build54d9a439e7c4ca598d2bf289f9552764e5b1cc7d87c47e7a0316466d98772cd5. HEX586974852ffd85d23e0b8ff2171f7524dc591d60abe29bd9c5a95ae65682836a, ELF56b61ca7419097c6910bbc45834687ad7a737e6356f1f8ef6105cc464b91b279; text344940/data191128/bss499476.212 tracked inputs match commit09ae387; the213th explicit authored defaults input is hash-bound separately. Complete generated startup requests both outputs ON, brightness255, gains0.25, lower32/p32 and upper18/p39; actual startup/cold-start remain unexecuted. Captain's required M23 final-selection question was presented with these concrete results; elapsed silence is not a selection or permission. SQ1–SQ5 unchanged.

Fresh post-camera readback admission-fix-post-capture-readback.json retains all63 writable settings per board with zero differences; frames80900, upper completions77873, capture losses0/0 and epoch1. Both output flags ON, correct modes/palettes/master/gains after all camera sessions ended. This proves continued rendering/configuration after recording cleanup, not the unperformed full host-disconnect/cold-boot test.

WP6 index linked additively in PRISM docs/lightshow-loop/README.md; original Phase1 and all full assignments remain intact. Current charter was reread and PRISM copy matches its approved hash. No new dashboard/registry/worktree/Core repin. A read-only launch-plist check initially failed under Homebrew Python3.14's pyexpat symbol mismatch; existing supported Python3.13 is used, without changing installations. Hindsight refresh/watchdog failures were disclosed; memory remains clues only.


Continuous-output launch reconciliation: ProgramArguments names the existing titan-broker-launch.sh wrapper, so searching only that plist array for a direct --continuous-output flag returns false. The wrapper's final exec explicitly supplies --live-control --continuous-output; broker normal shutdown closes serial/control/recorders without an emit-disarm call. Existing source and successful bounded captures agree on continuous mode. No launch option, host code or service was changed by this read-only check.


### 14 October 2026 — debrief checkpoint; host-disconnect test failed before hardware action

The standalone host-disconnect attempt host-disconnect-current-010-001 exited1 at09:06:24AWST with queue.Empty: a3s wait for the next camera startup stdout event expired. Its event history contains no recording_started, audio_process_started or sole_broker_stopped. No playback, broker shutdown, disarm, reset or firmware write occurred. The camera process was terminated by this attempt's cleanup; the existing broker53500/controller79093 remained. cleanup.json broker_restored=false means it was never stopped, not that restoration failed. failure-diagnosis.json retains the actual failed scope. Host-independence remains NOT TESTED. The needed test repair is to reuse existing bounded camera-start handling rather than treating3s of quiet stdout as a failure. No retry was launched when Captain requested the full chat debrief.

At Captain's debrief request, fresh debrief-live-state-20261014.json confirms current010 build/UID, correct32/18,p32/39,master255,gains0.25,both outputs enabled, precision1, and input rejects/discards0/0 with epoch1. No camera/player remains. M23 final boot choice has not arrived; the compiled complete profile stays UNSELECTED/UNINSTALLED. Current installed images retain legacy Spectrum/top-disabled boot defaults until explicit runtime restoration: autonomous two-edge cold boot is still UNDELIVERED. Do not describe continuous current runtime as completed startup behaviour.

Timing correction for the debrief: wrong-composition BOOM002 did execute06:03–06:05AWST and passed audio validation, per its own retained events/observation. Any earlier description saying that run never executed is stale. It is immediately before the last3h window and still cannot qualify the agreed Waveform pair. Newly connected9087A500 S3's controlled audio report completed05:47AWST, before that window; no S3 implementation/flash occurred during the latest3h. Older B489 deployment/recovery and S3 incidents are historical context, not fresh completion claimed in this period.
