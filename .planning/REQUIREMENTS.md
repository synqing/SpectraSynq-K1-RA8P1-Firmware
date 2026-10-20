THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Requirements: Titan RA8P1 takeover

**Defined:** 2026-09-20
**Core Value:** Trustworthy LED transmitter plus honest capture/identity gates, so later live and architecture claims cannot launder unsupported success.
**Source:** `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md` (locked)

## v1 Requirements

### WP0 — Live checkpoint

- [ ] **WP0-01**: Read `AGENTS.md` and run Titan `platform_memory.py --check`; recall `pins`, `clocks`, `dma`, `ws2816`. Inspect branch, local/remote SHAs, tracked/untracked, ownership. Preserve unrelated dirty work. Do not assume the old `c2d4dca + unpublished week` description.
- [ ] **WP0-02**: Inspect CDC ownership and broker/programmer sessions before opening the port. Do not kill an unknown owner or remove a live lock.
- [ ] **WP0-03**: Bind full `g4-uncapped-raw-hops-build-20260919-02` receipt, programme events, application INFO, frozen header/profile, any raw timing run. Verify hashes. Search named assets first.
- [ ] **WP0-04**: With CDC available, bind UID, full build ID, source pin, contract, cache/M33/U55, boot/run context via the existing identity path. USB enum and write receipt are not app identity.
- [ ] **WP0-05**: Preserve on-device diagnostics before reset or image change. Extract profiler results if still useful. Record available vs absent.
- [ ] **WP0-06**: One dated STATUS current-state entry and matching `external-receipts.json` current fields. Historical campaigns immutable. Unknown fields stay unknown.
- [ ] **WP0-07**: Run the suggested read-only source checks (`git status/branch/rev-parse/log/diff`, `scripts/check_references.sh`). Record sibling DualMCU/BSP pin ahead/dirty without resetting them.

### WP1 — Capture gate (F-01, F-02)

- [ ] **WP1-01**: Scorer accepts an explicit verified witness/run pair for a new candidate. Validate full UID/build, boot/run identity, frame ID, profile, actual pixel/byte/bit lengths, payload hash, duty hash, two-preload accounting. Do not replace historic constants with accept-anything.
- [ ] **WP1-02**: Capture manifest names raw files, channels/probe points, acquisition/session identity, sample interval/timebase, thresholds, trigger/correlation, LED part/profile, decoder version. Resolve only declared paths; recompute SHA-256 from actual files.
- [ ] **WP1-03**: Decode P601 and first-LED DIN. Check complete payload, bit count, first/last pulse, all high/low/period timings, reset low vs populated-part limits. Quantify instrument resolution so uncertain measurements cannot pass a tight margin.
- [ ] **WP1-04**: Same-acquisition/same-frame correlation. Payload equality alone is insufficient for all-black traffic. No invented spare pin; no silent waveform change.
- [ ] **WP1-05**: Separate outcomes: `capture_present`, `raw_files_verified`, `frame_correlated`, `waveform_conforms`, `first_divergence`. Synthetic/parser tests never create physical qualification. Valid capture of bad pulses remains a failing signal result.
- [ ] **WP1-06**: Executable negatives: missing/tampered raw file, non-hex hash, synthetic relabelling, wrong build/session/frame, missing probe, invalid/non-finite timing, payload mismatch, wrong profile, 3071/3073 bits, clipped final pulse, insufficient reset. Valid parser fixture with synthetic provenance is not physical evidence. The F-01 reproduction (`complete_synthetic(..., source='physical')` with −10 ns widths, reset 0, hash `'z'*64`) must not return physical PASS.
- [ ] **WP1-07**: Remove local-Mac witness dependency from pure parser/scorer unit tests. Optional historical-receipt integration tests stay separate and explicit about missing prerequisites.
- [ ] **WP1-08**: Protocol footer and scorer no longer insist on historic `a3f37e8a` frame 2030 as the only executable contract. Keep the old witness immutable. New acquisition has new run/boot/frame identity; instrument armed before the fault.

### WP2 — Host hygiene (F-04, F-05, F-06)

- [ ] **WP2-01**: Do not re-implement completed gold-extract path import, scheduler LED2 linkage, SConscript missing-K1 guard, or bounded GPT mutations.
- [ ] **WP2-02**: Broker qualification binds an explicit accepted build/application checkpoint (UID, full build/source, contract, relevant configuration). Recognised UID with a different build is unaccepted, not qualified. `identity_ok()` must not return ok for `superseded-unbound-build` / `new-unverified-build`.
- [ ] **WP2-03**: Invalidate binding and metric validity on disconnect, age expiry, restart, or identity change. Rebind before current-image claims. Tests: build change same UID, stale-to-fresh, rejected opcode, failed cleanup, ownership handoff.
- [ ] **WP2-04**: One CDC owner. Broker and target runners must not compete. Read-only monitoring has no hidden config/flash/reset.
- [ ] **WP2-05**: Correct telemetry units. Do not map `last_emit_cycles` to `hop_max_us`. Do not substitute renderer frames for completed transmission or emit duration for hop execution. Missing measurements stay unavailable.
- [ ] **WP2-06**: Record rotation/limits and write-failure behaviour prevent another runaway log. Preserve archived log hash `ba47ecba0bf61a6b3aae8218e9cfac0e637bd3d143efb60b8b75d4fd752d478d`.
- [ ] **WP2-07**: Remaining subprocess timeouts where needed (`test_palette_runtime.py` ordinary compile/run). A harness timeout is not a rejected firmware mutation or a PASS.
- [ ] **WP2-08**: CLI help for `--palette-autostart` matches live mode 32, flags 5 (not carousel). Source, help, tests, boot receipt aligned.
- [ ] **WP2-09**: F-06: `run_mode32_real_audio.py` is observation, not acceptance. Exact-length validation; full identity/configuration and recording hashes; required-counter validation; both rail signs; restore starting settings on every path including exceptions; honest failure receipt. Reuse `run_colour_integrity.py` ownership/restoration. Do not call missing samples silence or black. Negatives: short frame, missing counters, capture exception, restoration failure. `authored_stats()` must not treat frames shorter than 480 bytes as black. `hop_max` must not be documented as execution time if it is sample peak amplitude.
- [ ] **WP2-10**: F-05 docs/metadata: `external-receipts.json` current image; `combined-runtime-contract.json` GPT completions and DMA map; onboard-mic SINCRNG 10; complete 001↔002 crosswalk; retire obsolete sub-8 ms and false resident claims. Do not untrack/delete the historical PCM1808 stage tree as this task.

### WP3 — Uncapped timing (F-03)

- [ ] **WP3-01**: Reuse any completed raw run found in WP0. Compact 8,000 µs histograms are not authoritative percentiles.
- [ ] **WP3-02**: Explicit configuration record: O3 variant, frozen header and PCM hashes, sliced/unsliced tempo, DTCM/ITCM placement, cache/FPU/vectorisation, clock, observer mode, full build/HEX identity. ELF/map evidence for placement.
- [ ] **WP3-03**: Preserve qualified DTCM image. Add an explicit build choice so empty-TCM can actually be built. Current default remains a named variant. Never call a DTCM result empty-TCM. Fix receipt schema label: raw v2 decoded but `K1T1.v1` string.
- [ ] **WP3-04**: Same frozen input bytes for profile/control comparisons. Do not regenerate both sides' goldens. Do not re-run the entire old corpus for document edits.
- [ ] **WP3-05**: Collect all 6,000 rows with required fields. Decode after the timing window. Check sequence/completeness vs target counters.
- [ ] **WP3-06**: Answer Q1–Q5 or record a precise evidence blocker for each.
- [ ] **WP3-07**: Known 5 ms delay at hop 136; confirm index and magnitude. Mutation is a negative, not qualification. Do not conflate with host one-hop fixture hop 0.
- [ ] **WP3-08**: Distinguish ordinary hop too slow vs late start from previous tempo job. Do not hide ordinary misses.
- [ ] **WP3-09**: Report whether tempo p99 fits 22,500 µs. That answer alone does not pass 7,500 µs release-to-completion. Later slicing must preserve publication age/epoch and pass its own gate.
- [ ] **WP3-10**: Use verified `run_scalar_schedule.py` interface with fresh output directories. Separate `--timing-mutation` directory. `--qualification` is a different gate.

### WP4 — LED fault capture and bounded repair

- [ ] **WP4-00**: Prerequisites: WP0 identity, WP1 capture path, instrument available, selected image and WP7 transition prepared. Keep WP3 results safe before changing images.
- [ ] **WP4-01**: Select a source-bound diagnostic candidate. Prefer retained guarded image if artefacts and recovery verify. Narrow diagnostic build only if telemetry/correlation requires it. Do not combine a speculative driver cure with new gain/render behaviour.
- [ ] **WP4-02**: Verify strip part, supply, ground, shifter route, pin ownership. 74HCT2G34GW is documented present. Prove electrical defects by measurement.
- [ ] **WP4-03**: Arm P601 and first-LED DIN before reproducing the fault. Capture both ends of the same transmission plus pre-trigger context.
- [ ] **WP4-04**: Retain v2 opcode-22 bytes, strict decoded witness, settings, PDM counters, raw waveforms, identity, timeline in a fresh run directory. Empty opcode-22 remains v1/728 bytes; LE v2 requests v2/1420. Do not break existing clients.
- [ ] **WP4-05**: Identify first divergence using the brief's five hypothesis branches. Each is a test, not a preselected diagnosis. Do not write a fourth arbitration-map experiment as the next action.
- [ ] **WP4-06**: Patch the smallest implicated surface. Keep nonzero-count guard, immutable first-fault retention, independent DMA/waveform/latch completion. Never restore `DMCRA <= 2` waiver. Never manufacture a completion callback.
- [ ] **WP4-07**: Production-driver host and mutation tests plus one negative specific to the measured defect. Cross-build, inspect linked configuration/register path, programme identified candidate, new same-frame comparison.
- [ ] **WP4-08**: Repeat zero-output, warm low-load, warm USB-stress controls with documented `--phase-s 20`. Preserve first failure. Restore declared settings and release CDC on every exit. No reset/retry inside the scorer.
- [ ] **WP4-09**: Require AP/PDM progress and both completion events; no new DMA errors; no PDM overflow/drop/recovery/packing error, rearm denial, skew drop, starvation, or gain clips. Both raw-rail signs. Transport pass does not qualify acoustic clipping.
- [ ] **WP4-10**: Visible-corruption ticket closes only with optical evidence tied to this candidate plus the wire result. Agent capture/score does diagnostic work.
- [ ] **WP4-11**: If no instrument: report exact missing acquisition capability, with candidate, wiring/probe instructions and decoder ready. Continue independent work. Do not invent wire evidence.

### WP5 — Live PDM/AP/VP admission

- [ ] **WP5-00**: Prerequisite: repaired transmitter admitted at bench scope.
- [ ] **WP5-01**: One named integration candidate with accepted driver and current intentional live changes. Bind all sources and configuration. Module host success is not integration qualification.
- [ ] **WP5-02**: Reset/autostart mode 32 both logical channels, active+emit, no four-second carousel. Record actual brightness/palettes/direction/settings. Do not silently substitute historical brightness 255.
- [ ] **WP5-03**: Read back live SINCDEC/SINCRNG, PDM channel/DMAC ownership, nominal/measured rate, ASRC lock, AP cadence. Source uses SINCDEC 49/SINCRNG 10 and nominal ×16 with loud guard. Changing gain is not the default next task.
- [ ] **WP5-04**: Preserve reserve-before-rearm/FILLING ownership repair and programme/measurement separation. Bounded overflow/restart/epoch recovery with explicit failure status. Do not silently feed 16 kHz diagnostic to 24 kHz AP or fall back to fixture audio.
- [ ] **WP5-05**: Acoustic capsule identity when controlled observation is available; otherwise `ACOUSTIC_IDENTITY_UNRESOLVED` with valid transport facts.
- [ ] **WP5-06**: Qualify both raw rail signs and post-gain clipping under identified quiet/normal/transient input. Characterise PDM/filter/resampler response, delay, drift. Bounded filtering/rate-tracking repair only if measurement requires it. Reproduce historical frontend probes (no bins below 110 Hz, low-bin tone/label disagreement, 160→80 half-time tempo) against the current contract before calling them defects.
- [ ] **WP5-07**: Real music via repaired `run_mode32_real_audio.py --audio ... --build-id ...`. `run_music_baseline.py` continues to refuse synthetic room playback. Bound playback; respect session permission.
- [ ] **WP5-08**: Separate presence, colour injection, propagation, decay. Verify `last_live_valid_` at zero time and wrap, threshold crossings, uninterrupted rendering during five-second dwell. Measure elapsed-time decay and declared five-second quiet/black tail without history reinitialisation or minimum-one residuals.
- [ ] **WP5-09**: Score travelling structure, occupancy, quiet gaps separately from brightness and centroid. Native frames for algorithm checks; full-cadence/optical for physical smoothness. An 8 Hz dump cannot exclude a one-frame glitch at 120 Hz.
- [ ] **WP5-10**: Measure AP execution and release-to-completion, render/preparation, DMA service, queue/backlog, memory on the combined image. 45-second combined runner is preflight, not G4 240,000-hop qualification. Do not extend soak without a stated remaining risk.
- [ ] **WP5-11**: Observer-off/on checks. Dense 20 Hz USB previously froze emission. Dashboard traffic must have bounded cost and must not become a necessary production data path.

### WP6 — Product output

- [ ] **WP6-01**: Admit existing local modules as a named reviewed slice. Do not silently change the DualMCU manifest to fake byte-identity. Add build/source bindings and linked-use evidence.
- [ ] **WP6-02**: Reproduce order: effects → blend → treatment → edge policy → gain → joint current limit → stage/show. No double brightness/treatment. Compare both logical channels through the full chain.
- [ ] **WP6-03**: TRUE16 independently of Pixel8. Test pattern `0x12AB` through wide submit/packer without double packing, scaling, or residual 8-bit dither. `packBenchGrb48Lane()` ×257 expansion is not extra colour precision.
- [ ] **WP6-04**: Validate populated WS2816C-1313-4P timing and GRB48 on the proven single output before adding lanes. Known-good-strip/control comparison from `FASTLED-PORT-DESIGN.md`. WS2812 does not automatically qualify WS2816.
- [ ] **WP6-05**: Resolve physical topology before multi-lane. 160×48 bits at 800 kbit/s = 9.6 ms payload, exceeding 8.333 ms/120 Hz. Each 160-pixel edge needs parallel 80/80 halves; two edges need four concurrent lanes.
- [ ] **WP6-06**: P004 cannot run the selected GPT backend. Prepare approved route options and ownership. Only Captain authorises physical DIN move. Do not silently reduce pixel count, colour depth, or render rate to obtain a pass.
- [ ] **WP6-07**: Budget DMA-accessible memory: 80×48-bit lane 15,360 bytes; four lanes 61,440 per bank; two banks 122,880 before other buffers. Verify map placement, channels, worst-case service.
- [ ] **WP6-08**: Prove halves/edges launch and latch, preserve centre topology, enforce joint current limit, expose submission/transfer/latch timestamps and underrun/overrun. GPIO bit-bang (~5.4 ms IRQ-off) remains diagnostic only.
- [ ] **WP6-09**: Physical WS2816/multi-lane status is PASS, FAIL, BLOCKED, or NOT TESTED at actual scope. Do not conceal a blocked physical prerequisite.

### WP7 — Live-programming protocol

- [ ] **WP7-01**: Before announcing ready: candidate finished, host gates, cross-build, full source/ELF/HEX hashes, verified recovery image, programmer dry run, fresh programme directory, exact waiter command, bound target-runner command. Check ownership before operator transition.
- [ ] **WP7-02**: On Rearm: start prepared waiter first, reply exactly `WAITING`. Consume `events.jsonl` continuously. Report ROM seen, identified/writing, WRITE_VERIFIED, separate application identity promptly.
- [ ] **WP7-03**: Give “Release USER and BOOT, then RESET” once only if operator has not already done it. If app responsive, proceed to bound runner.
- [ ] **WP7-04**: Never reuse programme/run directories. A timed-out waiter that wrote nothing is not failed firmware execution. Do not loop blind retries. Do not use an alternative programmer/reset path to bypass the procedure.
- [ ] **WP7-05**: Programming success is not application startup, peripheral transaction, register state, or emitted light. Run the image-bound target scorer. Optical evidence remains a distinct boundary.

### Latency, comparison, ruling

- [ ] **LAT-01**: Implement/join bounded generation-tagged timestamps at capture / AP publication / VP use / submit / transfer-complete. Missing joins, dropped frames, epoch changes cannot be silently discarded. Never use USB-read time as event time.
- [ ] **LAT-02**: K1-L: ≥1,000 joined Titan frames, real music through microphone, observer off, full configuration recorded. Traced audio-to-light p50 ≤12,000 µs per `docs/decisions/2026-09-10-audio-to-light-ceiling.md`. Report analysis, publish-to-render wait, render, submission-to-transfer-complete. Frontend/decimation/ASRC delay and LED latch reported separately. p99 ≤15,444 µs is not an accepted hard gate without Captain stamp. AP service p99 ≤8,000 µs stays distinct from 7,500 µs frozen resident contract. Acoustic-to-photon is separate.
- [ ] **LAT-03**: Bind the actual S3 comparison baseline (43 frames, p50 11,576 µs) and method before using it. Do not relabel S3 as RT1062. Do not modify or flash the RT1062 lane from this task.
- [ ] **LAT-04**: Update `docs/RA8P1-vs-RT1062-RULING.md` after required measurements. Answer all fourteen questions in `docs/EXECUTION-BRIEF.md` §20. Allowed recommendations only. Captain owns the decision. Present evidence and a recommendation; do not self-stamp production migration.

### Observability questions (WP2 useful fields)

- [ ] **OBS-01**: Live audio health: measured input rate/lock, paired-slot/AP-hop progress, queue ownership/high-water, drops/overflow/recovery, both raw rails, post-gain clips, ASRC starvation.
- [ ] **OBS-02**: Cadence: AP execution and completion/lateness, render cost/skips, queue pressure, instrumentation cost, observer on/off comparison.
- [ ] **OBS-03**: Transmission: submitted / DMA-completed / hardware-stopped / latched counts separately; fault code/frame; remaining words; profile; preparation cost.
- [ ] **OBS-04**: Visual state: input publication/generation, musical presence, dwell age/path, injection/transport/decay evidence, frame CRC/occupancy with declared sampling.
- [ ] **OBS-05**: Run trust: UID/build/contract, boot/connection epoch, metric age/validity, CDC owner, host drop/write errors, live vs replay provenance.
- [ ] **OBS-06**: SS-03 Historian replay and restart recovery close only when current proof assets support them. Retain SS-02. Do not reduce dashboard to four generic values or invent firmware numbers.

### Acceptance matrix

- [ ] **ACC-01** through **ACC-13**: Every row in brief §8 uses PASS/FAIL/BLOCKED/NOT TESTED with method, conditions, evidence. A diagnostic capture of a failing signal is not waveform PASS. A skipped dependency must not return a green aggregate.

### Documentation and handoff

- [ ] **DOC-01**: Complete 001↔002 crosswalk mapping the thirteen original deliverables to current source-bound receipts and limitations.
- [ ] **DOC-02**: STATUS remains the concise current index. Update `external-receipts.json` with hashes of actual new files. Preserve failed and superseded receipts. No duplicate ledgers.
- [ ] **DOC-03**: Narrow commits; separate harness/source repairs from evidence updates when useful; inspect staged paths; exclude large logs and new vendor stage trees.
- [ ] **DOC-04**: Final handback states: exact commit/diff/files, candidate configuration, full build/HEX, UID, last-observed resident; every acceptance row with receipt paths/hashes; which finding was repaired, mechanism, negative, unmeasured remainder; CDC owner/release, fault state, settings left, pending waiter/run; remaining tasks with owner, next action, inputs, prerequisite, prohibited claim, or **unassigned**.

### Stop/Go

- [ ] **STOP-01**: Unknown/mismatched identity or CDC ownership → no device config/programming.
- [ ] **STOP-02**: New first-fault without required evidence → preserve; no reset/overwrite until capture complete or Captain changes the constraint.
- [ ] **STOP-03**: Capture provenance/correlation or applicable timing limits cannot be established → no physical qualification.
- [ ] **STOP-04**: Required build/negative/resource/timing gate fails → no dependent qualification; retain failure; bounded measured repair; rerun that gate.
- [ ] **STOP-05**: Pin ownership, wiring, or instrument unavailable → no invented route or physical claim; leave a concrete ready-to-run task; continue independent work.
- [ ] **STOP-06**: Reference conflicts with newer explicit authority → reconcile; do not silently change pins, fixture bytes, thresholds, or the source pin.

## v2 Requirements (parked — brief §7)

- **PARK-M33**: M33 partition — resume when measured M85 service/resource deficit justifies a split.
- **PARK-U55**: U55 useful feature / E2 / G7 — resume when a qualifying model and new material utility exist.
- **PARK-F2**: Actual-K1 NPU coexistence — named justified NPU workload and accepted scalar/combined baseline.
- **PARK-MVE**: Helium/MVE or FPU/cache — honest baseline and measured hot stage; one controlled derivative at a time.
- **PARK-PCM1808**: Practical Hirose DF12 breakout/bridge exists.
- **PARK-BRIDGE**: Titan–S3 radio / K1-B — named radio-only S3, firmware identity, physical transport.
- **PARK-P603**: P603/DIN-B and second edge — Captain authorises exact physical work.
- **PARK-POWER**: Power/thermal — identified instrument and declared operating/load conditions.
- **PARK-MIR**: Musical feature promotion — labelled held-out recordings and a declared usefulness question.
- **PARK-OPTICAL-LAT**: Full optical latency — common-clock acquisition setup.
- **PARK-G8**: Independent G8 review — Captain explicitly reopens the previously rejected dispatch path.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Fourth DMA arbitration map as next LED action | Three policies already failed at the same remainder |
| DualMCU/Teensy/PCB work from this tree | Separate programmes |
| Sibling DualMCU or Titan BSP edits | AGENTS.md |
| Synthetic room tones / white noise | Brief invariant |
| Combined physical-output acceptance before transmitter admission | Runtime development is in-scope; emit-disabled allowed |
| Self-stamping K1-C / production migration | Captain owns the ruling |
| History rewrite / force-push / untracking PCM1808 evidence | Brief housekeeping is optional after archival copy |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| WP0-* , WP7-* | Phase 1 | Pending |
| WP1-* | Phase 2 | Pending |
| WP2-* , OBS-* | Phase 3 | Pending |
| WP3-* | Phase 4 | Pending |
| WP4-* | Phase 5 | Pending |
| WP5-* | Phase 6 | Pending |
| WP6-* | Phase 7 | Pending |
| LAT-* | Phase 8 | Pending |
| ACC-* , DOC-* , STOP-* | Phase 9 | Pending |
| PARK-* | v2 / Phase 9 ledger only | Parked |

**Coverage:** v1 requirements mapped to phases 1–9. Parked items listed in Phase 9 ledger, not scheduled as implementation plans.

---
*Requirements defined: 2026-09-20*
*Last updated: 2026-09-20 after PRD express from takeover brief*
