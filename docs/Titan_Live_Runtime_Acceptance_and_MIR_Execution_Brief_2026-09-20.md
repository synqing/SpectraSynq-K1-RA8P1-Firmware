# Titan live K1 runtime — development acceptance and MIR execution brief

Date: 20 September 2026. This is a NEW onward execution brief. Do not amend the earlier takeover_execution_brief.

## 1. Assignment and finish line

Execute the remaining work on the already programmed live K1 candidate. Demonstrate that real microphone input is consumed correctly by the actual K1 audio pipeline, reaches both controlled logical visual channels, exposes useful musical information and controls, and survives the declared observation and freshness cases. Then deliver working recording, offline observation playback, deterministic algorithm replay and same-input comparison.

The next result is an operational music-intelligence development baseline. Do not finish at INFO, a bind-only receipt, an ARM build, a revised plan, or a dashboard containing only generic health counters.

Use these existing milestones:

| Milestone | Required result |
|---|---|
| LIVE_RUNTIME_IMPLEMENTED | Source/host/build evidence for the actual live implementation. Reconcile the existing evidence; do not infer full coverage solely from the image starting. |
| TITAN_LIVE_K1_DEV_READY | All required development-acceptance rows in this brief pass on ONE named firmware/configuration, scoped to physical emit off and both logical visual channels. |
| MUSIC_INTELLIGENCE_BASELINE_READY | Development acceptance plus usable source material, full observations/controls, recording/playback, deterministic replay and a proven same-input comparator. |
| K1_FEATURE_MIGRATION_COMPLETE | Separate full canonical-function ledger closure. A working MIR baseline does not establish every original application feature migrated. |
| PRODUCT_QUALIFIED | Separate transmitter, physical-lane, latency and Captain-owned ruling evidence. Not granted by this campaign. |

The runtime is no longer waiting on the analyser. Q1–Q5 and P601/DIN remain separate tracks. Preserve their diagnostic configurations and failed evidence.

### Execution authority and working method

- One persistent lead owns source integration, the current candidate, CDC, programming and result interpretation.
- Bounded subagents may implement/review assigned source or host surfaces. Coordinate overlapping files; never create competing serial owners or flashers.
- Ordinary implementation review is not independent G8 qualification.
- Execute reversible source, host, recording and development-test work within this assignment. Do not return for permission between ordinary steps.
- Use the existing Rearm procedure only when an actual replacement image must be programmed. No flash is needed merely to start testing the image already running.
- The report says the board is emit off. Keep physical emit off throughout THIS development campaign, including restoration. This is a scoped test configuration, not a permanent output prohibition.
- Preserve unrelated dirty work and the current checkout. No sibling worktree, blanket reset/stash, blind git add -A, sibling edits or history rewrite.
- Follow existing commit authority. If committing is not authorised, leave a precise patch/new-file manifest and source hashes; lack of a commit does not prevent execution.
- Use named real recordings and bounded playback. No synthetic room tones, white noise or indefinite same-song loop. Silent digital fixtures remain permitted.
- Finish the required checks, repair actual failures and continue to the developer workflow. Do not restart the historical portability programme.

## 2. Current checkpoint — consume this before older status paragraphs

The latest user/implementer report establishes the following. This brief's author has not independently opened the Mac or Titan; the executing agent must consume the local receipts named below.

| Item | Current fact / required interpretation |
|---|---|
| Target UID | 545433931bd25436593630352d068363 |
| Reported live build ID | fde39fece2d07d5fcddbd7df3b38b0a0f81bb6622272149264f51ba7379457d3 |
| Build directory | live-k1-runtime-build-20260920-02 |
| Application INFO | Reported read from /dev/cu.usbmodem00000000000011 after programming; matches that build |
| Behavioural source pin | 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a |
| Contract | sr24000.hop180.bins80.xover40 |
| Latest execution evidence | WRITE_VERIFIED plus application INFO: application started |
| Existing run directory | live-k1-runtime-run-20260920-01: BIND ONLY, emit off; do not reuse as a fresh acceptance run |
| Qualification | Identified; no accepted checkpoint bound. Hops/MIR/controls/freshness/resource performance not yet demonstrated by the supplied report |
| Core policy | M85 AP+VP; M33 and U55 parked |
| Cache policy | D-cache off is the retained contract; bind actual candidate configuration from its receipt/readback |
| Checkout at last inventory | lane/k1-ra8p1-002 at 431140853dd8b58af53240ef84d5fb1a08bd8b45 plus uncommitted work; record any subsequent change |
| CDC ownership observation | Lock metadata names dead PID 18445; flock was free; lsof times out; lock file was not unlinked; permitted reader successfully obtained INFO through flock |
| Phase 8 | Recording/replay/comparison remains scheduled, not completed |

Build ID is not the HEX SHA-256, and the behavioural pin is not the complete dirty-source identity. Recover the NEW candidate's full HEX/ELF hashes, exact programming-directory path, write receipt and raw INFO path from the build and bind records. These values are not supplied here; do not guess them or reuse the older image's hashes.

### Established fallback assets — distinct roles

| Role | Identity |
|---|---|
| Programmer safety-fuse asset | SHA-256 a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a; EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex |
| Conservative live return image | live-audio-gpt-20260920-03; HEX SHA-256 3aa0913950c5815c28c3ae06b37ee4462c5e2537adc738517cccd8b97b60eabd |
| That return image's build ID | c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c |
| Current candidate | fde39fec…; preserve its complete artifacts, but do not call it development-accepted before the campaign |

The old prepared live-audio-gpt-prog-20260920-04 waiter targets the return image. It is not the deployment command for a repaired fde39fec derivative.

The September 19 g4-uncapped-raw-hops-build-20260919-02 directory is PRESENT and is a DTCM experiment. The old absent-directory assertion is stale. Empty-TCM profiler HEX 22d405c1d45871c45c0bfa5b88fc1868c79198de493efeef900a47f89cb81a8b exists for the separate Q1–Q5 track. Neither is the default next image.

### Source locations

| Role | Absolute location |
|---|---|
| Firmware root | /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware |
| Evidence root | /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002 |
| Behavioural reference, read-only | /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware |
| BSP reference, read-only | /Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini |
| BSP pin | 6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7 |

Read current applicable AGENTS.md and local platform authority. Then read the current candidate/bind receipts, docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md, current STATUS.md, WP0-CHECKPOINT.md and corrected execution index. The latest observed fde39fec application supersedes WP0's older blocked-INFO state and c7f6034a residence. Historical facts remain retained at their original timestamps.

## 3. Scope boundaries and result states

Separate these facts in the runner, broker and report:

1. Artifact integrity: files have verified hashes.
2. Candidate identity bound for testing: expected UID/build/source/contract/schema matches observed INFO and the programme.
3. Development acceptance: measured campaign passed.
4. Transmitter admission: separately supported physical result.
5. Product acceptance: separate completed product gates and ruling.

An identified candidate must be testable without an already-passing development receipt. If the current host code conflates “expected candidate” with “physically accepted,” repair that state distinction and its negative tests. Do not set an accepted boolean to get past the gate.

Status values are PASS, FAIL, BLOCKED and NOT_TESTED. For an intentional negative, record BOTH the induced runtime failure/state and whether the negative test correctly detected/recovered from it. Do not turn induced loss into normal-run success.

New APIs/CLI surfaces named later are REQUIRED INTERFACES TO CONFIRM OR COMPLETE. They were specified in the earlier runtime plan; their presence in the current Mac tree is not established by INFO. Reuse an equivalent existing implementation and record its mapping. Do not build a parallel framework.

## 4. Work package A — bind this candidate and acquire the existing owner

**Owner:** lead. **Exit:** exact candidate-for-test record, exclusive permitted transport, saved configuration and known capabilities.

A1. Inspect the existing build receipt and live-k1-runtime-run-20260920-01. Resolve actual programme and INFO artifacts; hash them. Record source dependency hashes, build flags, schema/control hashes, clock, TCM placement and whether the live profile actually excludes fixture execution. Reuse valid build/host results covering the same dependencies and flags.

A2. Keep the existing bind-only directory immutable. Select the next UNUSED live-k1-runtime-run-YYYYMMDD-NN directory for execution, using the actual date. Creation must fail if it already exists. A failed/partial directory is never reused.

A3. Use the permitted flock-based ownership path that already succeeded. If a live broker already owns CDC, route through that broker. Otherwise acquire through the existing owner mechanism. Do not start an additional reader beside it. Preserve DTR/RTS false on observe paths.

A4. Do not require lsof to become reliable as a new prerequisite after the supported reader has successfully established ownership. A free advisory lock alone is not a universal proof that no non-cooperating process exists; retain the mechanism's actual exclusivity checks and detect contention/errors. Do not kill an unknown process, unlink a live lock, or weaken ownership checks. A dead PID label is metadata, not authority to bypass locking.

A5. Obtain/retain INFO through that owner when establishing the campaign session. Verify exact expected UID, full build ID, pin and contract. Bind schema/capabilities and session identity. If they differ, stop mutations and scoring against the old manifest; retain the response and identify the actual image.

A6. Write the candidate-for-test binding separately from acceptance fields. Set development_ready=false and transmitter_admission=unproven unless independently supported. Source code, hashes and a green parser test cannot set those fields true.

A7. Acquire the existing campaign control lease. In the specified implementation it is a 128-bit token, 30-second lifetime, renewed every 5 seconds, bound to the local connection and candidate/session. Renew locally through the broker; renewal must not introduce hidden periodic firmware reads during the observation-off interval.

A8. Read the complete base configuration, revision, effective state, playback settings and output state. Require emit=false before campaign mutations. Save an immutable restoration snapshot including UID/build/session/schema/revision. Preserve physical brightness and gain; this task does not require enabling output or changing microphone gain.

A9. Inspect the capability contract through supported queries and source binding. Confirm runtime_kind=live and obtain the required function mapping below. Use a short structural read, not a long soak, to locate missing capabilities.

### Minimum executable function map

| Function | Required behaviour | Existing/proposed surface to inspect |
|---|---|---|
| Identity and schema | Exact expected candidate; no circular acceptance | INFO, titan_broker, titan_snapshot, protocol manifest |
| Live audio owner | One AudioPipeline/window/history owner; direct complete-hop consumption | live_audio_runtime, live_app, pdm_target |
| MIR snapshot | One coherent publication with full feature groups and validity | live_protocol; snapshot operation advertised by schema |
| Events | Retained IDs/times, cursor reads, explicit gaps | Event-history operation |
| Timing | Unsaturated per-hop record history and cumulative deadline/resource counters | Timing-history operation |
| Config read/write | Complete typed validation, safe boundary, revision and request identity | GET_SCHEMA/GET_CONFIG/SET_CONFIG equivalents |
| Recording | Raw-response binding plus decoded records and phase markers | Existing broker recorder / run_live_k1 |
| Scoring | Recomputable normal/negative verdicts; missing evidence fails | score_live_k1 |
| Freshness test | Bounded one-shot source-delivery suppression with retained target transitions | TEST_STALE_SOURCE_120MS capability |
| Offline playback | Recorded observations with replay origin; no mutation access | Existing viewer/recorder client |
| Algorithm replay | Same LiveAudioRuntime on frozen PCM and deterministic metadata/clock | Host replay driver |
| Comparison | Same input/config identity; exact/tolerant rules fixed before comparison | Baseline/candidate comparator |

**Missing capability rule:** mark the affected row NOT_IMPLEMENTED/NOT_TESTED in the working capability record, retain the actual response/source finding, and execute Work Package B. Continue independent checks. Do not advertise the missing field with zero values, quietly substitute a fixture, or claim a complete runtime from INFO alone.

## 5. Work package B — complete only the missing implementation

**Dependency:** specific gap from A or measured failure from later packages. **Exit:** existing candidate can execute the required case, or a verified named replacement is ready/running under the established transition.

This is a conditional repair branch. Do not reimplement already working components.

### B1. Host-only gaps

Inspect/extend scripts/run_live_k1.py, scripts/score_live_k1.py, tools/serial-studio/titan_live_control.py, live_protocol.py, the existing broker and recording/view surfaces. Confirm actual names and --help first.

Implement only missing identity binding, command encoding, typed readback, bounded record draining, phase scheduling, raw preservation or scoring. Keep one outstanding device transaction and one pending mutation. Do not use the older observation-only runner as an acceptance verdict.

Meaningful host negatives cover wrong UID/build/schema, malformed/oversized/CRC-invalid responses, stale revisions, partial config failure, cursor gaps, invalid floats, recorder failure and uncertain acknowledgement. Retain already-valid tests; rerun affected dependency sets when implementation, flags or inputs change.

### B2. Audio/capture gaps

The intended live path has no replaceable pending-hop mailbox and no fixture::Trajectory. Capture ingests complete blocks; the foreground owner reads one complete 180-sample hop directly into its own buffer.

- ASRC push preflights PCM AND source-metadata capacity before mutation.
- Failed readiness leaves ring state, phase, counters and caller PCM unchanged except explicit rejection counters.
- For relative index s, fractional phase f and increment inc=floor(source_hz*65536/24000), a 180-sample pull requires max(s+floor((f+179*inc)/65536)+2, s+floor((f+180*inc)/65536)) available samples. Validate indices/occupancy before capacity subtraction.
- Raw DMA tokens retain their raw epoch/sequence/slot. Software stream resets cannot rewrite acquired hardware tokens.
- Stream epoch, hop sequence, source support, rate segment and completion receipt accompany each hop.
- Exactly one AP owner advances the window and processes each accepted hop once. On discontinuity, invalidate old publication/prediction and reset audio-owned histories before resuming.
- The 4,096-sample window reaches full warmup after 23 contiguous 180-sample hops. Warmup is distinct from tempo lock.
- One protected common clock supplies extended cycles and microseconds to capture, AP and publication. Verify ISR/foreground protection, wrap ambiguity and clock-change handling in the actual linked implementation.
- Publish actual result availability after processing; do not replace event coordinates with completion time.
- No invented zero PCM for a source stall.

Validate the specific defect with atomicity, mixed-epoch, delayed-consumer or reset tests before cross-building. Successful-path AP numerical behaviour remains governed by the admitted reference.

### B3. Renderer/control gaps

Use the actual two-channel PaletteRuntime. Each logical channel is 160 pixels, centres 79/80, with the existing centre-origin behaviour. Render controlled modes and palettes once per due pair; fixture carousel/test render channels cannot execute in the live profile.

Preserve effects → blend → treatment → edge policy → gain → joint current limit → stage/show. Preserve base user settings separately from director/policy-derived effective settings. Refresh musical presence only for a new valid publication, not every reuse of cached loud data.

Controls include the admitted per-channel visual parameters, mode/palette/transitions, complete AudioFocusProfile including 80 spectrum/12 chroma gains, global runtime policy and master gain. Input rate/hop remain fixed read-only 24,000/180. Pin mapping, caches and arbitrary memory are not tuning controls.

At a safe AP/VP boundary, apply a validated transaction atomically and increment revision once. Preserve controls across audio reset. Use the existing manifest's types/ranges/history-reset semantics; do not invent UI-only clamps.

### B4. Observation gaps

Provide all groups in Work Package C. Keep missing/invalid/warming/stale data explicit.

The specified protocol has a 4,096-byte response-body cap, explicit little-endian encoding, exact uint64/Q32 handling and manifest-derived operation IDs. Proposed IDs 23/24/25/26 are NOT assumed on the resident image: use its advertised schema and verify the host codec matches.

Bound large schema/config reads with pagination. Config pages must share one immutable revision; a revision change invalidates the assembled read. A complete transaction applies atomically even if staged in chunks. Retain request ID/digest/result so a lost SET acknowledgement can be resolved without blind replay.

Event history: 256 records, at most one aggregate record per AP generation; specified 192-byte record, 20 per read at 10 Hz. Timing history: 1,024 records, 56 bytes each, 64 per read at 10 Hz. If the actual candidate uses a different declared version, reconcile its exact layout/capacity against the existing contract before scoring. Do not silently change sizes, drop fields or accept truncated records.

A slow dashboard may coalesce snapshots. Recorder/event/timing loss must be reported. No observer or disk failure may block AP.

### B5. Replacement-image procedure, only when firmware changes are required

1. Preserve the failing candidate/run and available fault/diagnostic evidence. Use the permitted diagnostic reader under exclusive ownership; do not remove a broker forbidden-operation rule merely to obtain a witness.
2. Make the smallest repair, preserving the frozen transmitter configuration and unrelated controls/algorithms.
3. Run affected host gates, ARM build and map/resource checks. Verify actual live source selection, DMA SRAM placement, M33/U55 parked and D-cache off.
4. Create a new build directory and full source/ELF/HEX/config/schema manifest. Never overwrite live-k1-runtime-build-20260920-02.
5. Prepare exact programmer dry-run, fresh programme directory, recovery assets and post-write runner. A dry-run for the old return image does not validate the replacement.
6. At the established operator Rearm transition: start the prepared waiter first, then reply exactly WAITING. Consume events WAITING_FOR_IDENTIFIED_ROM → ROM_SEEN → ROM_IDENTIFIED_WRITING → WRITE_VERIFIED. Bind application INFO separately.
7. Start a fresh campaign identity. Do not pool old/new-image results into one candidate pass. For a firmware replacement, rerun the defined Phase 7 campaign on the final image; retain unaffected host evidence where its bindings remain valid.

Existing programmer command TEMPLATE:

```sh
python3 scripts/programme_scalar.py \
  --build "$TITAN_REPLACEMENT_BUILD" \
  --output "$TITAN_FRESH_PROGRAMME_DIR" \
  --wait-seconds 180 --execute
```

Do not invoke this template for initial testing of the already-running fde39fec image.


## 6. Work package C — verify complete live data and controls

**Dependency:** A, plus required B repairs. **Exit:** structural correctness and configuration ownership established before the long normal sequence.

### C1. Required musical and runtime observations

Collect a bounded initial 5-second observation at the declared 10 Hz profile. This is a structural smoke test, not development acceptance. It must show advancing successful AP consumption/publication, coherent generations and the following usable fields or explicit validity states.

| Group | Required information and checks |
|---|---|
| Identity | UID/build/schema/session, input origin, stream epoch, hop/publication sequence, config revision |
| Capture integrity | Completed/ingested/discarded samples, producer errors, raw slot ownership/release, both rail signs, post-gain clipping, rate/calibration state |
| AP integrity | Successful/failed/inflight consumption, exact hop intervals, ASRC occupancy/high-water, discontinuity/recovery reasons |
| Spectrum and level | All 80 bins with mapping identity, Nyquist-safe upper bin, peak/RMS where actually measured, low/mid/high/total energy, novelty |
| Tonality | All 12 A-origin chroma values, strength, chord type/root/confidence; unknown/no-chord remains valid behaviour |
| Events | Onset/bass onset, transient/kick/snare/hihat IDs, strengths and levels, event coordinates and availability |
| Tempo | BPM, phase, confidence, lock/coasting, beat tick/strength/update; no forced lock on silence or unsuitable material |
| Saliency | Harmonic/rhythmic/timbral/dynamic raw and smoothed novelty, overall/dominant state, thresholds/events |
| Musical time | Epoch/anchor/frame, beat index, Q32 period/phase error, prediction coordinates, availability validity and expiration |
| Logical VP | Both channel generations, mode/palette/transition, base/effective controls, frame identity/CRC or native dump when checking pixels |
| Resources/timing | AP work, ready-to-publication delay, deadline misses, raw release delay, render cost/skips, telemetry cost, stack/heap reserves |
| Physical scope | emit=false; active output disabled; configured backend separately named; transmitter unproven |

Use A,A#,B,C,C#,D,D#,E,F,F#,G,G# chroma labels. Preserve uint64/Q32 as exact integers and decimal strings at JSON/JavaScript boundaries. Mark live versus replay origin.

An activity counter alone does not prove valid AP consumption. A finite value alone does not prove musical accuracy. A frame CRC alone proves neither centre-origin motion nor correct pixels; use retained source/host/native-frame tests for those claims.

Use the retained contract tests to verify that first descriptor boundaries [0,180) and [180,360) correspond to exclusive media boundaries 360 and 720 in the 48 kHz coordinate system. On the already-running target, check the available sequence/interval relationships; do not reset or add instrumentation solely to recover its first two hops. This coordinate system does not mean physical capture runs at 48 kHz.

AP publication must be coherent: features, tempo, waveform association, quality and its recorded AP-side configuration refer to the same committed result. Consumers cannot splice different AP generations. A visual transaction may legitimately apply after AP publication and before rendering: associate each render with both its consumed AP publication key and its own applied configuration revision. Do not require those revisions to be identical or rewrite an old publication to make them match. Event time remains distinct from result availability.

### C2. Control transaction protocol

1. Save complete base configuration and its revision under the campaign lease.
2. Select only fields advertised as writable with implemented validators. Record accepted ranges/types and reset semantics from the manifest.
3. Apply one transaction at a safe boundary with expected revision and request ID/digest.
4. Require exactly one revision advance; obtain complete revision-bound readback and the first affected AP or render generation, explicitly naming which owner applies the change.
5. Verify only declared base fields changed. Effective director/policy values may differ according to their documented behaviour.
6. Verify a visual-only change does not reset audio epoch/history. A deliberately source-affecting control follows the documented discontinuity path; do not include it silently in a normal continuity case.
7. Send one stale-revision transaction and one out-of-range/unsupported value through the actual control path. Require atomic rejection and unchanged revision/configuration.
8. For a timed-out SET, stop further mutations and reconcile request ID/digest/revision/readback. Identical known duplicates return the prior result; no blind repeat across a boot.
9. Exercise lost-acknowledgement, staging, lease-contention and schema/paging negatives in existing deterministic host tests. Do not force an unnecessary target disconnect to recreate every host negative.
10. Restore changed settings under the tracked revision before leaving the test case.

Complete schema validation/apply/readback coverage is required for every advertised writable field. Host enumeration may cover the full field set; target exercises the real dispatcher and representative fields from each group. Document that scope instead of claiming every possible combination ran on silicon.

### C3. Deterministic representative control schedule

The 600-second session in D includes these actions. Freeze the exact values in the manifest BEFORE starting. Read existing catalogue IDs; the pinned enabled set is 3,7,8,9,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,28,29,32. Preserve all 38 ordinals and 44 palette IDs. Do not enable disabled entries to make a test convenient.

| Session time | Action | Required observation |
|---|---|---|
| 0 s | A mode 32; B mode 3; distinct valid palettes | Both channels active logically, independent settings and frame progress |
| 60 s | A mode 3; B mode 32 | Both controlled dispatch paths work |
| 120 s | A mode 26; B mode 24 | Percussive/harmonic consumers receive valid live data |
| 180 s | A mode 24; B mode 27 | Predictive/musical-time validity handled honestly |
| 240 s | A mode 27; B mode 26 | No fixture carousel or unintended A/B reassignment |
| 300 s | Start one A palette morph, interrupt midway with a different valid palette | A transitions from current blended state; B base state/transition remains unchanged |
| 360 s | Change one A scalar audio-focus gain and one spectrum/chroma gain entry; restore; then repeat on B | Channel-specific input transformation; no global AP-history mutation |
| 420 s | Change declared master output gain, then restore | Gain applied once in logical composition; current-limit coupling reported separately |
| 480 s | Begin 15 s playback pause, microphone still running | Captured quiet is distinguishable from missing source; existing dwell/history decay behaviour |
| 495 s | Resume the named recording at the manifest's next offset | Renewed live features; no artificial epoch break from host playback pause |
| 540 s | Restore complete baseline controls through the normal transaction path | Complete readback matches baseline base fields |
| 600 s | End phase | Consistent accounting, resources and trace boundaries |

Value selection rules eliminate hidden defaults:

- Choose distinct palette IDs from the advertised valid set in ascending order, excluding the currently selected ID as needed.
- For interrupted morph, use an advertised valid 2-second transition if supported; otherwise use the nearest supported positive duration. Schedule the interrupt near halfway, retain actual device application time/progress, and require strictly mid-transition progress (0 < progress < 1). Exact halfway is required only if the firmware advertises target-scheduled application; asynchronous USB delivery does not guarantee it. An interrupt arriving after completion does not exercise this case; retain that attempt and repeat only this control case with a suitable longer supported duration.
- For gain changes, use half the saved value when distinct and legal. If saved value is zero or halving is illegal, use the canonical default if distinct/legal; otherwise the manifest's minimum legal distinct increment. If no distinct valid value exists, record that field as fixed and select another writable field in the same group.
- Use the lowest advertised spectrum/chroma array index for the focused array check. Preserve all other entries. Raw microphone gain is outside this control schedule.
- All playback files, offsets and pause boundaries are fixed in the manifest. Do not substitute a generated test tone for an inconvenient musical passage.
- Physical emit remains false. Positive hardware emit-on behaviour belongs to later combined qualification. Host tests cover emit gating where applicable.

A/B isolation is checked before the joint limiter. Changed channel A can legitimately affect channel B's final limited intensity when combined demand changes; that is not base-configuration corruption.

## 7. Work package D — run the complete normal development campaign

**Owner:** lead through existing broker/runner. **Exit:** complete normal-run evidence or a specific reproducible failure, never a green aggregate over missing data.

D1. Freeze a versioned campaign manifest containing candidate/programme/INFO/schema identities; source/input route; clock/cache/placement; configuration; recording hashes/offsets; durations; observer profile; thresholds; output scope; control transactions; recording boundaries; restoration identity and paths.

D2. Use one fresh normal-run directory. Keep intentional negatives in separately identified receipts. Store raw frames or hash-bound response files and decoded data. Snapshot sampling, event-history completeness and timing-history completeness are different claims.

D3. Prime declared code paths/resources, then establish the steady-state measurement boundary. Record startup/warmup separately; do not move boundaries afterward to remove an outlier.

D4. Execute this fixed sequence:

| Case | Duration and activity | Required evidence |
|---|---|---|
| Readiness | Up to 30 s; bind candidate, observe rate lock and full-window warmup | Explicit readiness or timeout; do not reset an already-running image just to manufacture cold-boot evidence |
| Quiet input | 15 s of actual microphone input | Noise/rails/clipping/validity; room input need not be numerically zero |
| Host observation off | 60 s real music; AP+logical VP on; no periodic snapshot/event/timing requests | Consistent before/after counters and valid retained maxima/resources; local lease renewal continues |
| MIR off, timing recorded | 60 s real music; drain timing at 10 Hz; no snapshot/event polling | Complete per-hop timing; event interval intentionally unrecorded |
| MIR on, timing recorded | 60 s real music; snapshot/event/timing polling each at declared 10 Hz schedule, one outstanding request | Complete required histories; measured observation cost and bounded queues |
| Music and controls | 600 s using C3; continue full 10 Hz snapshot/event/timing profile | Useful features, complete controls/readbacks, quiet/resume behaviour, continuity and resources; readbacks do not silently suspend history draining |
| Final normal snapshot | After all completed normal work, before the injected negative | Complete counter reconciliation, final maxima/reserves and raw-record hashes |

This is approximately 13 minutes 15 seconds of fixed normal phases, plus readiness and setup. It is a bounded development campaign, not an elapsed-time estimate for fixing failures and not historical 240,000-hop G4 qualification.

D5. Do not require an analyser, enable physical emission or switch to the empty-TCM profiler for these cases. The CPU/driver/source configuration of THIS live image is what is being measured.

### D6. Timing and accounting definitions

Use common-clock extended cycle timestamps and recorded clock_hz. Compare thresholds in exact cycle/rational arithmetic before rounding display values.

| Quantity | Definition / criterion |
|---|---|
| ap_work | AP finish minus AP start; state whether this is AudioPipeline::process only |
| ready_to_publish | Publication minus supporting DMA receipt for the newest source support needed by the hop |
| Development deadline | ready_to_publish <=7,500 µs for every declared steady-state normal hop; zero misses |
| handoff_wait | AP start minus that supporting DMA receipt; do not start at dequeue and erase waiting |
| capture-to-publication proxy | Publication minus back-projected newest-sample receipt estimate; labelled software proxy, not acoustic time |
| raw release | Completion receipt to release of that exact DMA slot/token |
| Raw-slot bound | Existing 296-sample slot at maximum allowed 44,000 Hz gives 6,727.272… µs; preserve the declared conservative service bound |
| ASRC | Capacity 1,024 source samples; bounded occupancy without growing age; capacity does not relax raw-slot ownership |
| Stack reserve | At least 4,096 untouched bytes under the existing measurement convention |
| Heap reserve | At least 32,768 bytes free at recorded maximum; zero used/maximum growth after declared priming |
| Render cost | Logical A/B render/compose work, with skips and cadence separate |
| Telemetry cost | Encoding/service cost and bytes; never reported as AP compute or LED latency |

Do not reuse last_emit_cycles as hop_max_us. Do not infer timing from how quickly the host receives USB packets.

Accounting snapshots must be consistent, same-session and outside an in-flight ownership transfer:

```text
raw completed =
    raw ingested
  + explicitly discarded complete samples
  + READY or consumer-owned complete samples

hops returned =
    AP completed + AP failed + AP in flight

AP in flight is 0 or 1
```

Reconcile ASRC consumed/buffered/discarded samples separately. Do not fabricate unknown hardware-loss counts to balance an equation.

Normal phases require zero unexpected raw/ASRC overflow, failed AP hops, unexplained sequence loss, mixed epochs, rearm/ownership faults or unplanned recoveries. Expected control changes and sampled snapshot coalescing are not sample loss; exact event/timing gaps remain visible.

### D7. Recording limits and correct statistics

- The 1,024-record timing ring retains about 7.68 seconds at 133.3 hops/s. An undrained 60-second interval cannot supply complete per-hop distributions.
- The 256-record event ring retains about 1.92 seconds at maximum one event record per hop. Intentional no-event-polling phases cannot claim complete event history.
- Declare unrecorded boundaries in advance. Rebase host cursors at the current boundary when recording starts. Do not relabel accidental losses afterward as deliberate.
- During required recorded intervals, missing records fail completeness. A recent-window trace is not the whole phase.
- Before/after differences apply to monotonic counters with unchanged identity/reset state. NEVER calculate an interval maximum as after.max - before.max.
- For observation-off maxima, use existing phase-scoped retained statistics or an enclosing cumulative maximum that proves the whole relevant interval stayed within bounds. If an earlier excluded startup maximum obscures the answer, that interval's maximum is unresolved; use an existing scoped statistics mechanism or repair this evidence gap.
- Keep lifetime counters intact. Any statistics-scope operation must be explicit, bounded and distinct from resetting AP/capture.
- Use nearest-rank percentiles for this campaign: sorted N values, rank ceil(p*N), one-based. Record this convention. Preserve raw cycles and sample count so the result is reproducible.
- Never saturate large values into a histogram's final bucket and call that an exact p99.
- Different music passages do not establish causal percentage observer overhead. Report measured telemetry costs and normal-run loss/deadline behaviour. Use identical deterministic replay input if claiming an overhead percentage.

### D8. Input quality and useful MIR

Report raw positive/negative rail counts and post-gain clipping separately. Retain interval boundaries and settings. A noisy room or low-confidence chord is not automatically an implementation failure.

The MIR baseline needs identified, unclipped, continuous real-music material for meaningful comparisons. If no clean interval exists, repair the measured input/headroom issue or select a suitable existing recording/level; preserve the failed result. Do not silently change the microphone route, gain or algorithm thresholds until their role is measured.

No claimed chord/onset/tempo “accuracy” follows merely from changing numbers. Label confidence and validity honestly. Tempo may remain unlocked on sparse/ambiguous input. Semantic accuracy needs labelled reference material and a predeclared metric.

## 8. Work package E — freshness, isolation and restoration negatives

**Dependency:** usable candidate/controls. **Exit:** intended rejection/recovery demonstrated without contaminating normal-run statistics.

### E1. Fixed 120 ms stale-source negative

Use the advertised TEST_STALE_SOURCE_120MS capability, exactly once in its separate receipt. If absent, execute B rather than sending an unknown opcode or substituting a host sleep.

Required behaviour:

1. Bind injection ID/request identity, candidate/session, current epoch/publication and configuration revision.
2. At a safe boundary suppress new AP-hop delivery for 120,000 µs while raw DMA service/copy/release continues.
3. Intentionally discarded complete programme blocks are counted; do not fill ASRC until overflow or fabricate silence samples.
4. The retained previous publication ages naturally. Once older than 30,000 µs, consumers mark it stale, suppress new musical events and stop re-arming presence from it.
5. Existing authored visual history may decay. Staleness does not require instantaneous black.
6. Suppression ends in firmware even if the control client disappears. No unbounded queued pause.
7. Discard/account pre-test ASRC state and use the controlled software discontinuity path. Preserve raw DMA ownership identity, advance logical stream epoch and resume from a complete valid block.
8. Reset audio-owned AP/VP/prediction histories as specified, preserve base controls, and complete 23-hop full-window warmup.
9. Confirm no old-epoch event/prediction is presented as new valid music.

**Evidence required:** retained target injection start/end, stale-entry, epoch-transition and warmup-complete markers or equivalent latched counters/timestamps. Bind them to the same injection and clock origin.

A 120 ms pause with a 30 ms stale threshold may leave only a 90 ms stale window. Ten-Hz snapshots can miss it. Do not claim the test failed or passed merely because a sampled dashboard did/did not show STALE. Inspect retained target transitions.

Evaluate stale entry at the first actual consumer freshness check after the threshold; retain that check's time and observed service interval. Do not demand host packet arrival within 30 ms or silently move the threshold to match polling.

This test proves stale-consumer behaviour and controlled software reset. It does NOT prove recovery from a physical DMA failure. Keep existing host raw-loss/token/recovery tests scoped accordingly.

### E2. Observer isolation

- Disconnect/reconnect the display client while the broker retains CDC. Require continuous source/AP accounting; rebind display session without replaying old mutations.
- Exercise slow subscriber and recorder-failure paths through the existing host harness. If tested on target, mark that interval negative and explicitly record lost observation data.
- Require AP/capture to continue despite observation failure; do not call a failed recorder a complete recording.
- A decoder sees schema/CRC/length/request mismatches as errors, not zeros or a fabricated frame.
- Do not replay a control request on reconnect merely because it was queued before disconnection.

### E3. Restoration

Restore the complete saved BASE configuration only when all of UID/build/session/schema, held lease and the campaign's tracked final revision still match. Fetch complete revision-bound readback and compare every base field. Keep emit false for this campaign.

If a SET outcome is uncertain, reconcile it before restoration. If identity or revision changed outside the campaign, retain original/attempted/current states, stop automatic mutations and report the conflict. Do not fetch a newer revision just to force old settings over it.

Restore playback settings where changed. Release the lease and close the agent-owned CDC/broker session according to its ownership contract. If an already-running user-owned broker remains intentionally active, report it; do not kill it to claim a released port.

### E4. Failure disposition

| Failure | Immediate action | Repair / resumption |
|---|---|---|
| Wrong identity/schema | Stop mutations and scoring against old binding; retain raw response | Reconcile exact image/schema; start a fresh bound run |
| Missing MIR/control/timing capability | Mark specific row open | B repairs only that surface; independent work continues |
| Raw loss/overload/deadline miss | Retain first offending hop, occupancy, release and stage data | Smallest measured service/compute repair; new candidate if firmware changes |
| Capture-only service hook needed | Use existing bounded AP/tempo slice boundary | Ingest/release only; no recursive AP/render/control; invalidate in-flight output if discontinuity occurs |
| Clock ambiguity/bad mapping | Invalidate timing; retain provenance | Repair source/clock association; never clamp into apparent validity |
| Full trace lost | Fail required recording completeness | Repair scheduling/draining or named resource design; no percentile substitution |
| Clipped music | Preserve rail/gain evidence and settings | Measured input-quality correction; no silent “clean” relabelling |
| Config corruption/restoration conflict | Stop further mutations; preserve revisions/digests | Repair ownership/atomicity or reconcile actual outside change |
| Freshness markers absent | Negative remains unproved | Add bounded retained evidence, not higher-rate USB polling as a guess |
| GPT fault while emit off | Preserve witness; inspect actual backend/ownership state | Contain output fault; no speculative wire repair or physical pass |
| Target recovery fails | Leave explicit FAULT and exact leave-state | Prepare verified recovery under existing operator protocol |

Do not lower the 24 kHz/180 contract, omit feature groups, reduce declared observation rate or hide dropped samples to obtain a pass.

## 9. Work package F — score and establish development acceptance

Use the actual score_live_k1 implementation or complete its equivalent. It reads frozen manifest and retained raw evidence; it must not require a device connection to recompute the verdict.

1. Validate all file hashes, candidate/config/schema identities and expected phase coverage.
2. Decode raw records with exact integer handling; reject malformed data and incomplete required phases.
3. Reconcile accounting, epochs, timing, resource reserves and normal/negative boundaries.
4. Evaluate each criterion in Section 13. A missing requirement remains open; a negative's successful rejection does not erase normal-run failure.
5. Emit machine-readable row verdicts plus a concise human report identifying the first failure and its evidence.
6. Set TITAN_LIVE_K1_DEV_READY only when all required development rows pass on the final candidate. Scope: real PDM → actual K1 AP → controlled logical A/B VP, emit off, physical transmitter unadmitted.
7. Bind the acceptance record to full source/build/HEX/config/schema and campaign hashes. Configuration changes outside the tested scope invalidate automatic inheritance.
8. Proceed to Work Package G immediately. Do not stop at another “next up: replay” planning handback.

A host-only scorer self-test, a successful flash, 5 seconds of advancing counters or a bind-only run cannot grant this milestone.


## 10. Work package G — deliver the music-intelligence development workflow

**Dependency for final baseline stamp:** F passes. Host implementation and fixture preparation may proceed earlier while a target-specific repair is in progress.

### G1. Freeze the baseline

Retain the accepted target's source/build/programme/INFO/schema/configuration identities, normal/negative scores and identified clean music intervals. Record the host replay executable/compiler identity separately. Host and target are different execution environments; one does not automatically prove numerical equivalence of the other.

The host BASELINE must use the accepted target's exact K1/shared/live-runtime source dependency set, with only explicitly declared and hashed host clock/input/output adapters and compiler differences. A later dirty-tree runtime change is a CANDIDATE, not silently a new baseline.

Reuse the current developer tools. Implement a missing operation in the existing client/runner rather than adding another dashboard, broker or unrelated orchestration system.

### G2. Distinguish the three replay claims

| Operation | Inputs | What it proves |
|---|---|---|
| Observation playback — REQUIRED | Recorded MIR/events/timing/configuration history and raw-response bindings | The developer can inspect a previous observation session offline |
| Deterministic algorithm replay — REQUIRED | Frozen real-music PCM plus deterministic descriptor, clock, control and render timeline | Baseline and candidate can be compared on identical digital inputs using the real runtime |
| Exact microphone-session reanalysis — NOT promised by this slice | The exact PCM consumed by AP plus complete descriptor/reset/control history | Reanalysis of the actual captured microphone stream |

MIR NDJSON is not audio. A file played through a speaker is not the exact PCM recorded by the microphone. Do not claim exact live-session reanalysis unless those actual AP-input samples and histories were captured. Do not create a new mandatory firmware PCM-streaming project solely to complete the first useful replay baseline.

### G3. Prepare one frozen real-music fixture

Complete scripts/prepare_live_music_fixture.py or the existing equivalent.

Inputs: identified existing real recording, source hash, selected time range, channel-mix rule and output path.

Outputs:

- Raw mono signed-int16 little-endian PCM at exactly 24,000 samples/s.
- Source and output SHA-256, converter/version/settings, channel layout and mix coefficients, sample count and conversion-clipping information.
- A sidecar describing origin=frozen_recording, not live microphone capture.
- Complete-hop count and excluded trailing-sample count.

If the source is already mono 24 kHz signed-int16 PCM WAV, decode without resampling. Otherwise use an installed identified converter, record exact settings, and freeze the resulting bytes ONCE. Baseline and candidate both read those same bytes. Do not regenerate a separate input or golden for each side.

Process only complete 180-sample hops. Record any trailing partial hop as excluded; do not silently pad it with zeros. Select a source interval long enough for warmup and all declared replay events; require at least 1,000 complete hops for the initial regression asset. This is a NEW bounded host-fixture minimum, not a target qualification length.

### G4. Implement deterministic replay around the actual runtime

Complete scripts/replay_live_k1.py or the existing equivalent.

1. Instantiate the same LiveAudioRuntime and actual controlled renderer used by the live profile. Do not rewrite the MIR algorithms in Python or use fixture::Trajectory as a substitute.
2. Inject deterministic monotonic time and descriptors. Do not use host execution speed or speaker playback as the analysis clock.
3. Use a clearly labelled host replay source identity for already prepared AP-rate PCM. Preserve the 24 kHz/180 analysis contract. Do not pass synthetic replay receipts off as measured PDM DMA timing or apply the physical PDM-rate validity rule to a declared AP-rate replay source.
4. Preserve the same exclusive analysis/media boundaries and event-versus-availability semantics. Document synthetic receipt/availability construction in the replay manifest.
5. Freeze initial base controls, epoch, seed for any randomized effect state, every control transaction at a named hop/boundary and every logical VP render timestamp.
6. Emit full MIR/validity/quality, epoch/hop/publication keys, event identities/coordinates, configuration revision and representative A/B frame data or bound dumps.
7. Export timing provenance as simulated replay. Virtual zero-cost or delayed availability is not a measured target CPU duration.
8. Run the SAME baseline executable/config/input twice. Require deterministic fields and trajectories to match. Repair unexplained nondeterminism before promoting the baseline.

Retain four replay cases, each with its own manifest:

- Normal continuous input with the fixed control/render schedule.
- Known availability delay: at replay hop 136, introduce exactly 5,000 virtual microseconds AFTER analysis and BEFORE publication. This is a host replay negative, distinct from the separate Q1–Q5 on-target hop-136 mutation. Check changed availability without rewriting already-detected event coordinates.
- Epoch break: after 512 complete hops, end the old stream epoch and reset through the normal runtime interface. The next complete hop is sequence 1 in a new epoch; verify warmup and absence of old-epoch state. Bind the new epoch's time origin and subsequent sample positions explicitly.
- Rate-segment contract: use the existing deterministic source/descriptor fixture for a supported rate change at a complete-hop boundary. Preserve fractional continuity, avoid mixed increments within a hop and reset the local correlation fit. Keep its synthetic metadata provenance explicit. Retain the existing ASRC numerical tests separately; synthetic descriptor playback does not establish resampling fidelity.

For the delay case, compare source-derived event fields up to the affected publication and the intended reset/availability contract. Do not demand later stateful VP output remain identical when scheduling availability has deliberately changed.

### G5. Build the comparator and prove it can fail

Complete scripts/compare_live_k1.py or equivalent. It must run offline on preserved artifacts.

Require matching PCM hash, declared input range, schema and comparable source/control/render schedule before evaluating algorithm differences. For the availability-delay negative, permit only its predeclared virtual availability mutation through the frozen comparison policy and report that expected difference explicitly; all other structural checks remain active. Likewise, a reset/rate scenario is compared against its own declared scenario contract, not silently treated as the unchanged normal schedule. Reject undeclared missing/extra hops, incompatible schemas, invalid required fields or unmatched epochs as structural errors.

Comparison rules:

- Same executable/environment/input/config repeatability: exact deterministic output equality.
- Baseline versus candidate: preserve existing reference exact/tolerance policy. Freeze any justified new tolerance BEFORE running the comparison; never loosen it after seeing a difference.
- Compare integers, IDs, epochs, event coordinates, control revisions and geometry exactly unless the declared experiment intentionally changes that field.
- Report floating-point field differences separately, with named tolerances and worst absolute/relative errors where applicable.
- Separate platform/virtual-clock diagnostics from source-derived event coordinates. Do not exclude musical timing differences merely because “timestamps differ.”
- Report first divergent hop, group and field, a bounded surrounding window, mismatch totals and candidate/input identities.
- Preserve all required MIR groups and both logical channels in the comparison. Do not score just an aggregate brightness or centroid.

Required negative proofs:

1. Alter one known feature value beyond its declared tolerance; comparator identifies its hop/field and fails.
2. Remove one hop or change an epoch key; comparator rejects structural incompleteness.
3. Use a different PCM hash; comparator refuses a same-input claim.
4. Use the availability-delay case; comparator identifies the intended availability change while preserving the event-coordinate check.
5. Feed a schema/length-invalid record; decode fails rather than filling zeros.

A changed confidence value or brighter rendering is not an accuracy improvement. An algorithm experiment needs a named objective and metric. Use labelled held-out recordings when available; otherwise describe results as exploratory trajectories. Do not invent a new model task to claim the development platform finished.

### G6. Deliver the complete developer command surface

These logical operations must work, have --help, explicit inputs, bounded execution, useful errors and machine-readable output:

| Operation | Required result |
|---|---|
| Inspect live MIR | Full current publication and validity, not four generic counters |
| Read history | Event and timing cursors with explicit gap reporting |
| Record observations | Named duration/output, raw binding, complete declared history and honest disk-failure result |
| Playback observations | Offline inspection, origin=replay, no target mutations |
| Read/save configuration | Complete revision-bound base configuration and compatibility metadata |
| Apply/restore configuration | Atomic revision-checked transaction with outcome reconciliation |
| Prepare PCM | Frozen conversion artifact and exact provenance |
| Replay algorithm | Actual runtime over deterministic PCM/clock/control inputs |
| Compare runs | Same-input structural checks, numerical/event report and first divergence |

Put the actual tested command for each operation in ONE developer usage file and machine-readable command map under the campaign's normal evidence convention. Reuse an existing suitable document; do not create multiple competing “current” ledgers. The first-use sequence must be executable by the user without reconstructing this conversation.

### G7. Music-intelligence baseline exit

Set MUSIC_INTELLIGENCE_BASELINE_READY only when:

- Development acceptance is bound to the named target candidate and stated emit-off scope.
- Identified usable music input exists, with quality limitations retained.
- Full MIR/control/record/history operations work through the existing owner.
- Observation playback works without device access.
- The frozen PCM fixture, deterministic repeated replay and comparator negative proofs pass.
- The developer commands and preserved baseline artifacts are complete.

M33/U55 remain parked. A later useful-model task must identify the feature, input/normalisation/quantisation, accuracy objective, CPU/memory deadline, stale/failure behaviour and coexistence test. Generic accelerator activity is not this milestone.

## 11. Work package H — close the feature ledger and preserve onward physical work

Update the existing LIVE-K1-FEATURE-COVERAGE.json or equivalent single authoritative ledger. Do not turn this into another prerequisite planning project.

Every row records canonical source identity, actual live call path, controls/observations, test scope/evidence and limitation. Copied/linked code remains IMPORTED_ONLY until its actual runtime function is demonstrated.

Minimum required groups:

- Boot/recovery/identity/faults; standalone no-host boot separately scoped.
- PDM routing, unpacking, gain/headroom, ASRC, source-rate and sample accounting.
- Source/analysis/media clocks, epochs, warmup and event/availability semantics.
- GDFT/spectrum, energy/peak/RMS/novelty and silence.
- Chroma/chords, onset/percussion, tempo/beat, saliency and musical time.
- Independent focus, base/effective controls, director/policy/standby.
- All 23 enabled modes and 44 palettes, with disabled ordinals preserved.
- Both channels' histories, transitions, blend/treatment/edge/gain/current limit and centre-origin behaviour.
- Observation/control ownership, recording/history, host presets, replay and comparison.
- Canonical target NVM, additional audio inputs/USB Audio Class, HMI/radio/peer sync and updater adapters.
- Physical transmitter, WS2816 lanes, latency/optical and final ruling.

Use retained full-catalogue host coverage; target representative modes do not become a claim that every mode was physically inspected. Host save/restore is not target NVM. CDC is not USB Audio Class. Device enumeration is not bridge or peer-synchronisation proof.

If exact original SpectraSynq_K1_Firmware authority remains unresolved, follow the existing provenance lookup and continue the pinned-core development task. Only unsupported full-parity claims remain blocked. Do not move the DualMCU pin to a newer HEAD or import another lane's changes incidentally.

Separate tasks and resumption conditions:

| Track | Prerequisite | Required next work |
|---|---|---|
| Empty-TCM Q1–Q5 | Its named builds and operator Rearm | Same frozen 6,000-hop input, normal raw run, known-delay negative, matched observer; retain DTCM distinction |
| P601 / first DIN | Actual analyser/probes/common ground and fresh armed acquisition | Capture same event before reset; identify first divergence; smallest measured repair; original zero/warm/USB-stress controls |
| Combined live output | Named transmitter admitted at its actual bench scope | Integrate and rerun applicable combined/runtime cases on the exact image |
| Physical WS2816/four lanes | Verified parts/routes/ownership and actual assembled hardware | TRUE16 wire profile, concurrent lanes, reset/latch/underrun/current-limit evidence |
| K1-L | Applicable combined image and generation joins | At least 1,000 real-music joined frames, observer off, defined traced p50 <=12 ms; optical/group delay separate |
| Optical and RT1062 ruling | Comparable named physical measurements | Report the real limitations; Captain makes the architecture ruling |

The 160-pixel ×48-bit single lane takes 9.6 ms payload at 800 kbit/s, so the retained 120 Hz design requires its real multi-lane solution; do not conceal this by reducing pixels/precision/rate. P004 cannot acquire a GPT route through a software flag. These facts do not block the emit-off MIR baseline.

## 12. Commands and artifact rules

### 12.1 Confirm the current executable interfaces

The brief writer cannot verify the new Mac scripts from the supplied INFO report. Start by reading the actual parser/source and invoking --help on existing files. Do not present the templates below as already executed.

Known project-root checks:

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
git status --short
git rev-parse HEAD
git diff --stat
```

Do not recursively search all historic campaigns before inspecting the named current build/run receipts.

### 12.2 Required campaign interface

Reuse equivalent options if already implemented and record the exact mapping. Otherwise complete the specified interface with parser tests before target execution.

```sh
TITAN_ROOT=/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
TITAN_EVIDENCE=/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002
TITAN_CURRENT_BUILD="$TITAN_EVIDENCE/live-k1-runtime-build-20260920-02"
```

Resolve these variables from actual records: TITAN_PROGRAMME_DIR, TITAN_CAMPAIGN_JSON, TITAN_BROKER_SOCKET, TITAN_FRESH_RUN_DIR, TITAN_SCORE_FILE, TITAN_CONFIG_REVISION, TITAN_CONFIG_SNAPSHOT. The output directory is the first unused run name, not the bind-only run-01.

TEMPLATES after interfaces/variables are confirmed:

```sh
python3 scripts/run_live_k1.py \
  --build "$TITAN_CURRENT_BUILD" \
  --programme "$TITAN_PROGRAMME_DIR" \
  --campaign "$TITAN_CAMPAIGN_JSON" \
  --broker-socket "$TITAN_BROKER_SOCKET" \
  --output "$TITAN_FRESH_RUN_DIR"

python3 scripts/score_live_k1.py \
  --run "$TITAN_FRESH_RUN_DIR" \
  --output "$TITAN_SCORE_FILE"

python3 tools/serial-studio/titan_live_control.py \
  --socket "$TITAN_BROKER_SOCKET" get-config

python3 tools/serial-studio/titan_live_control.py \
  --socket "$TITAN_BROKER_SOCKET" save-config \
  --output "$TITAN_CONFIG_SNAPSHOT"

python3 tools/serial-studio/titan_live_control.py \
  --socket "$TITAN_BROKER_SOCKET" restore-config \
  --expected-revision "$TITAN_CONFIG_REVISION" \
  --file "$TITAN_CONFIG_SNAPSHOT"
```

The campaign runner does not flash, open a second CDC handle, silently reset the target or generate room tones. It refuses missing prerequisites with an exact reason.

### 12.3 Replay interfaces to confirm or implement

Use equivalent existing tools if present. The following proposed names/options define the required operations, not a claim about the current checkout.

```sh
python3 scripts/prepare_live_music_fixture.py \
  --audio "$TITAN_REAL_RECORDING" \
  --manifest "$TITAN_FIXTURE_SPEC" \
  --output "$TITAN_FRESH_FIXTURE_DIR"

python3 scripts/replay_live_k1.py \
  --fixture "$TITAN_FROZEN_FIXTURE_MANIFEST" \
  --runtime-build "$TITAN_HOST_REPLAY_BUILD" \
  --scenario "$TITAN_REPLAY_SCENARIO" \
  --output "$TITAN_FRESH_REPLAY_DIR"

python3 scripts/compare_live_k1.py \
  --baseline "$TITAN_BASELINE_REPLAY_DIR" \
  --candidate "$TITAN_CANDIDATE_REPLAY_DIR" \
  --policy "$TITAN_COMPARISON_POLICY" \
  --output "$TITAN_COMPARISON_RESULT"
```

Every unresolved variable must be replaced with a validated absolute path in the final developer command map. Record exact compiler/converter invocations inside the output receipts. An unknown flag is an interface gap to fix; it is not a reason to hand the user an unusable command.

Exit code zero means the command's requested applicable checks passed. A recorder successfully writing a FAIL receipt must not return a qualification PASS. Negative-test harnesses may pass for detecting the intended failure, with that distinction explicit in the result.

### 12.4 Evidence contents

Use existing external evidence conventions and a concise repository index. Keep large audio/build/raw assets outside Git. Preserve original failures.

Required contents, mapped to existing filenames where equivalents exist:

1. Candidate identity: source dependency set, build/config/schema, ELF/HEX, programme, INFO and session.
2. Immutable campaign manifest, music identities, full saved configuration and control schedule.
3. Raw protocol responses or hash-bound files, decoded snapshots, event/timing records and exact recording boundaries.
4. Phase-scoped/cumulative counters, resource measurements, retained freshness transitions and negative receipts.
5. Complete configuration transactions, uncertain-outcome resolution and restoration readback.
6. Recomputable development score with every required row.
7. Frozen PCM fixture/provenance, host replay executable/scenarios, repeatability outputs and comparator negatives.
8. Feature ledger and usable tested developer commands.
9. Final image, ownership, emit/configuration and fault state.

Do not rewrite live-k1-runtime-run-20260920-01 to make it appear it contained these tests. A new result gets a new run identity.


## 13. Acceptance matrix and milestone decisions

Populate every row with PASS/FAIL/BLOCKED/NOT_TESTED, exact candidate/configuration scope and evidence location. Reuse valid evidence only when its relevant source dependencies, flags and inputs match.

| ID | Required result | Positive evidence | Failure condition |
|---|---|---|---|
| D00 | Exact candidate-for-test binding | Full build/programme/INFO/schema agreement | Wrong/unknown identity, fabricated accepted checkpoint |
| D01 | One permitted device owner | Broker/lock acquisition and session record | Competing reader/flasher or bypassed ownership |
| D02 | Truthful clocks and coordinates | Protected common-clock tests plus target ordering/health | Wrap ambiguity, mismatched origins, availability presented as event time |
| D03 | Complete source/AP accounting | Consistent counters and sequence/epoch records | Silent overwrite, missing consumption, unexplained loss/recovery |
| D04 | Actual K1 live AP | Linked/call-path evidence and bound numerical tests | Fixture substitute, duplicate AP owner or duplicate processing |
| D05 | Complete MIR visibility | C1 groups with source, units and validity | Missing fields replaced by zeros; mixed generations |
| D06 | Both controlled logical channels | Representative target transitions and full-catalogue host coverage | Fixture carousel, missing channel, broken topology/state isolation |
| D07 | Validated atomic controls | Complete manifest/readback plus stale/invalid-update rejection | Partial apply, revision error, base-control compounding |
| D08 | Complete declared event history | Cursor sequence, raw records and gap checks | Undeclared overwrite or stale event replay |
| D09 | Complete declared timing history | Raw per-hop cycles and scoped observation-off evidence | Sampled trace presented as full distribution; invalid max arithmetic |
| D10 | Steady-state service bounds | Zero deadline misses; raw release and bounded backlog | Hidden waiting, late publications, raw-slot loss |
| D11 | Resource safety | Stack/heap reserves and zero post-prime growth | Under-reserve or growing resource use |
| D12 | Honest source quality | Separate positive/negative rail counters, including valid zero counts; gain-clipping and quiet/music interval labels | Missing samples called silence; clipping hidden |
| D13 | Freshness and software recovery | Retained target injection/stale/epoch/warmup evidence | Cached loud data stays fresh; injected pause unbounded; no transition proof |
| D14 | Observation independence | Off/on cases, display reconnect, meaningful observer-failure tests | AP requires dashboard or observer failure stalls capture |
| D15 | Complete safe restoration | Same-session/lease/revision full readback; emit false | Blind restore, unresolved SET or wrong-image mutation |
| D16 | Declared runtime configuration | ARM/map/source binding, M33/U55 parked, D-cache off | Unnamed build change or unsupported qualification inheritance |
| D17 | Recomputable final score | Immutable manifest/raw files and row-by-row scorer output | Green aggregate over a required missing/failed case |
| G00 | Baseline preserved | Accepted target plus separate host executable/config identities | Moving input/config/golden or confused target/host evidence |
| G01 | Observation recording/playback | Bound records replay offline with correct origin and gaps | MIR log claimed as raw audio; replay can mutate device |
| G02 | Frozen real PCM | Single hashed 24 kHz S16LE asset with conversion provenance | Different bytes per side, silent padding or undocumented conversion |
| G03 | Deterministic real-runtime replay | Same-executable repeatability and actual runtime call path | Reimplemented algorithm or unexplained nondeterminism |
| G04 | Delay/reset/rate cases | Declared scenarios with correct event/availability/reset semantics | Retimed music, old-epoch state or physical claims from synthetic timing |
| G05 | Comparator can fail correctly | Value mutation, missing hop, wrong input/schema negatives | Vacuous pass or post-hoc tolerance changes |
| G06 | Usable developer workflow | Tested command for every G6 operation, plus usable identified input | Missing operation or only another plan/CLI template |

- D00–D17 PASS → TITAN_LIVE_K1_DEV_READY at the stated emit-off scope.
- D00–D17 and G00–G06 PASS → MUSIC_INTELLIGENCE_BASELINE_READY.
- Feature-migration and physical rows remain separately reported. Their legitimate exclusion from the emit-off development scope is explicit, not a hidden skipped prerequisite.
- Known clipping may coexist with an honest runtime result, but G06 cannot pass without useful identified input for the music-intelligence baseline.
- A required development FAIL/BLOCKED keeps that milestone open while independent work proceeds.

## 14. Risk register and bounded decisions

| Risk | Prevention / detection | Required decision |
|---|---|---|
| Testing the wrong build | Exact expected full identity and session/schema binding | Stop only target mutations/scoring until reconciled |
| Reopening completed bring-up | Start from fde39fec and bound receipts | Reuse completed work; investigate only changed/missing facts |
| Circular “accepted before tested” gate | Separate candidate binding and qualification fields | Repair host state logic; never forge acceptance |
| Lock/CDC contention | Existing permitted owner path, one broker | Resolve actual contention; no arbitrary process killing or lock bypass |
| Observation alters service | Bounded requests, measured cost, off/on cases | Repair measured interference; no hidden polling in off phase |
| Freshness missed by polling | Retained target transition evidence | Repair observability; no inference from sampled UI |
| Failed SET applied twice | Request identity/digest and revision readback | Resolve uncertain outcome before mutation/restore |
| Oversized telemetry/config | Explicit wire limits, paging/staging, bounded memory | Reject/repair structure; no truncation or heap growth |
| False interval timing | Complete raw trace or valid scoped summaries | Missing evidence remains unresolved, never subtracted maxima |
| Replayed logs mistaken for DSP replay | Separate G2 operation types and input provenance | Claim only the executed operation |
| False musical “accuracy” | Named labels/metric or exploratory classification | Do not promote confidence/brightness changes as quality |
| Source/reference drift | Frozen behavioural pin, source dependency hashes | Explicit derivative or new candidate; no incidental sibling import |
| Physical fault stalls all software | Emit-off scope and preserved diagnostic image | Continue runtime/MIR work; preserve separate physical ticket |
| Repeated planning replaces work | Concrete D/G exits and next command | Complete implementation/testing/replay before handback |

A new firmware defect requires a new named candidate; a new receipt does not retroactively repair an old result. Broad regression, longer soaks or new infrastructure are justified only by a concrete remaining risk or an existing required gate.

## 15. Required final handback

Update the existing STATUS.md and external-receipts.json with the actual latest image/run and links to the evidence. Correct stale current-state claims with dated entries. Preserve the earlier takeover brief and historical failed receipts.

The lead's final response must state:

1. **What now works:** actual real-audio processing, both logical channels, full musical observations, control and replay capabilities demonstrated.
2. **Exact leave-state:** UID, full build/HEX, source/config/schema identity, last INFO, emit state, CDC owner/release and any fault.
3. **Milestone verdicts:** D/G matrix, DEVELOPMENT_READY and MUSIC_INTELLIGENCE_BASELINE_READY with scope; no physical qualification inheritance.
4. **Measured limits:** normal hop/loss/deadline/backlog/resource results, observer effects, clipping/quality and freshness transition evidence.
5. **Changes:** exact source/host paths, commit or patch/new-file identity, why each repair was needed and its negative proof.
6. **Developer entry point:** tested commands to inspect MIR, record, change/save/restore controls, replay observations, replay PCM and compare a candidate.
7. **Remaining work:** actual canonical feature rows and hardware tracks, owner or unassigned, exact prerequisite and next action.
8. **Restoration:** complete readback or precise conflict/failure; no vague “settings should be restored.”

If a real blocker prevents completion, leave the runnable preparation, retained failure and one bounded next action. Do not describe a missing analyser as a blocker to unrelated host/runtime work. Do not finish merely because a new plan or a hashed handoff exists.

## 16. Agent start instruction

Begin with the existing live-k1-runtime-build-20260920-02 and bind-only live-k1-runtime-run-20260920-01. Verify the current fde39fec identity and capabilities through the existing permitted owner, save the emit-off configuration, and execute A → C → D → E → F → G → H. Enter B only for an actual missing capability or measured defect.

The order above is the execution priority, not a ban on useful independent host work: prepare replay/comparison while a target-specific repair waits for its real operator transition. Finish the development baseline and its usable workflow. Preserve physical gates, completed evidence and user work.

## Basis and limits of this brief

- Latest user report: the fde39fec application was write-verified and independently identified, with the exact UID/pin/contract stated in Section 2; the recorded run is bind-only.
- Uploaded WP0-CHECKPOINT.md and corrected 2026-09-20-titan-ra8p1-takeover index: prior source/test/recovery context. Their earlier blocked-INFO state is superseded by the later successful permitted INFO read.
- Existing Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md: implementation contracts, development campaign, replay workflow and physical separation.
- The current Mac's uncommitted code, new programme files and target test results were not available to this brief's author. The implementation/execution agent resolves those exact named artifacts locally. Proposed CLI names are explicitly conditional until parser/help inspection confirms them.
- This document creates instructions only. Preparing it did not flash, reset, commit, alter the running candidate or amend the historical takeover brief.
