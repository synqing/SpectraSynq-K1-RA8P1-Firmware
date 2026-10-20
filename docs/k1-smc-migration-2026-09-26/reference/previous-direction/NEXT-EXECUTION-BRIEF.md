# K1 R2 — next implementation phase after Grok P0/P1 review

**Issued:** 26 September 2026, Australia/Perth.  
**Assignment:** close P1 semantic/resource defects and establish truthful, isolated BLE peer admission; then execute the first real P2 control transaction.  
**Status:** implementation instructions. This review changed no product source and deployed no firmware.

## 1. Product objective and authority

Deliver a physical console that controls an identified engine through the actual typed transport, reconciles authoritative returned state, and produces qualified light. Startup, display refresh, local CC logging and lower static RAM are intermediate observations, not that outcome.

The original `reference/K1-Implementation-Handoff.md` and its full 56-task programme remain intact. Read this document alongside `reference/K1-Grok-Execution-Brief.md`, `reference/SOURCE-RECHECK.md`, the current task registry and repository instructions. This is a defect-driven execution addendum, not a replacement or a smaller definition of done. Preserve the full mode catalogue, both logical channels, full physical topology, native precision where applicable, separate platform dispositions and independent acceptance.

Primary implementation roots remain K1.Sliders and SpectraSynq-K1-DualMCU-Firmware. Titan is an independently owned downstream/parallel lane. No unrelated PCB, shipping-firmware, BSP, framework or governance redesign is part of this task. Do not import DualMCU history into Titan, reset live trees, create prohibited worktrees, delete dirty work, bypass protected-file procedures or invent a new task system.

Existing programming authority and actual HOLD/ownership constraints remain in force. No general new permission loop is required; an actual failed identity, recovery, output-power or deployment gate still stops the affected write. Source facts are not resident-image identity. Port names are not devices.

## 2. Review disposition

Accept Grok's work as partial bring-up and specific first repairs. Do not accept P0/P1 closure, a complete resource solution, live R2 control, profile durability, physical output, or programme completion.

The reviewed console source and BIN match Grok's selected receipt. Its two new host tests pass under Apple Clang with ASan/UBSan. Four added assertions fail against that same device-preprocessor implementation. The linked target decoder has a 12,080-byte stack frame reachable from a 4,096-byte task. The current S3 receiver rejects a named K1-CONSOLE peer lacking identity, while the console creates only MIDI. Read `evidence/REVIEW-EVIDENCE.md` for exact hashes, commands, locations and limitations.

**The next action is not another unspecified “plug in and chase Bluetooth” session. Begin with the reproducible failures already identified.** Board absence blocks measurements that need it, not source repair, host regression work, build analysis or reference resolution.

## 3. A0 — bind the working candidate and baseline

Read the current ownership records and compare the affected file hashes with the review. Preserve all existing changes. Resolve project registration through the supported workflow and actual directory names, not a second governance framework. If registration remains unavailable, record the exact failure and follow the existing rules about permissible independent work; never declare a mandatory deployment gate passed by omission.

Keep device distinctions:

| Role | Required identity |
|---|---|
| Console 2.41 | FC:01:2C:DA:2B:38 |
| DualMCU S3 1.75 | 30:ED:A0:29:A1:C4 |
| Teensy 4.0 engine | 20731830 |
| Titan | 545433931bd25436593630352d068363, separate owner |

Preserve the known recovery application and the applicable NVS/partition/calibration data. The gold app readback does not back up calibration. Do not erase NVS or weaken encryption to make a connection attempt look cleaner. Do not revive the banned calibration wizard.

Resolve the console test dependency using the existing `tests/host/console/fetch_dualmcu.sh` in a fresh sanctioned reference location, or a reviewed repin with producer/consumer evidence. Never point its checkout operation at the live integration tree. The reported 97dc8cf-to-21de29b mismatch is not an instruction to reset or a permanent inability to run the suite. Read the helper's side effects first. The optional preflight script is in the old package, not necessarily in the repo; its absence in Sliders is not a product blocker.

Record baseline results separately: original host suite, actual K1_UI_R2 cases, input and UI suites, vendor checks, target build and target runtime. A compiler missing libasan is a toolchain failure; use the appropriate verified toolchain and retain that fact. Do not silently strip sanitizers and call the result the same gate.

Prefer scoped commits once reviewable changes pass their gates. Before the next deployed candidate, bind source inputs, configuration, compiler, ELF/map/BIN hashes, post-link checks and the exact guarded write. Do not silently rebuild between checking an image and writing it.

## 4. A1 — repair profile transactions and stack ownership

Map to CTL-3/4, UI-2/3, INT-2 and IVV-2. Preserve the existing compiler/codec contract; repair the application adapter instead of introducing another profile implementation.

### 4.1 Reproduce before editing

Run the supplied host counterexamples or import their assertions beside the existing tests. They currently demonstrate:

1. Two different accepted edits and Undo all use revision 1.
2. An edit expecting revision 999 is accepted against revision 1.
3. Backup allocation failure permits replacement and removes Undo.
4. UI slot 99 is silently applied to slot index 0.

Keep each failing receipt. These four probes are not exhaustive coverage and must not become the entire P1 acceptance denominator.

### 4.2 Transaction laws

Validate the complete captured edit identity: profile, expected base revision, page identity, channel, slot and semantic. Refuse malformed coordinates, unsupported policy values and conflicts; do not substitute page 0, slot 0, a default mapping or another target. Revalidate the read set at activation if compilation runs separately and the active state can change meanwhile.

Every successful content activation receives a new valid revision under the canonical revision/epoch law. Undo restores prior content under a NEW activation identity, never the old transaction identity. Stale imports/queued edits must not become valid simply because a revision was reused. Define counter wrap consistently with the existing contract rather than relying on plain comparison forever.

Stage all resources needed for the promised operation before replacing active state. Allocation failure at any staging, activation, backup, Undo, Import or Export site returns a specific resource outcome. Preserve active content, revision and promised recovery state on failure. No dynamic fallback getter may abort the device merely because memory is exhausted. Tests must exercise empty and already-active stores, every allocation index, warmed/shared scratch, retries and concurrent-operation rejection. An optional feature degradation requires explicit product semantics and visibility; silently dropping the promised Undo is not acceptable.

Propagate local validation, RAM-only/unsaved, durable-save failure, transport pending, engine applied, rejected, unavailable and uncertain-after-disconnect distinctly through the actual UI adapter. Returning false from a no-op persistence helper while ignoring it is not closure. Do not declare every valid local draft an error merely because the engine is absent; show what actually succeeded and what did not.

### 4.3 Remove heavy work from the scan path

The current decoder's linked frame is 12,080 bytes, before its callers, versus 4,096 allocated bytes. Do not exercise the current Import path on target to prove it crashes. A temporary explicit diagnostic refusal can contain the risk, but is not a completed Import implementation.

Use an explicitly owned, bounded editor/compiler workspace, with controlled reuse of non-overlapping scratch. Prefer one source of behavioural semantics shared across host and device, with allocation/storage adapters rather than divergent transaction rules selected by K1_UI_R2.

Keep the scan path bounded: sample, run the admitted input logic, enqueue qualified intent, publish a coherent observation. Dispatch heavy compile/decode/persist work outside the timing-critical scan path through a bounded command/result mechanism. Preserve a single state owner: a worker returns a staged result; it does not mutate input/LVGL/engine state concurrently. Publish a successful new immutable revision only at the owner's boundary. Queue full and stale-result outcomes must be explicit.

A larger scan stack alone is not the solution: it leaves unbounded editor work in a 5 ms task and does not solve duplicated heap lifetime. Generate stack usage and inspect the linked candidate after repair; then measure stack low-water and whole-task duration under maximum-size operations. Include nested calls, framework overhead and interrupt/context requirements when setting bounds.

**A1 exit:** all four counterexamples repaired plus complete relevant allocator/conflict/malformed-input/error-status tests; target-generated stack bounds fit the chosen ownership design; profile operations do not monopolise the scan loop. No durable-profile or live-engine completion is inferred.

## 5. A2 — select one R2 writer and publish truthful observations

Map to INP-2/3, UI-2/3, INT-2 and IVV-2.

**Selected direction:** in the R2 build, R2 owns physical-control intent; legacy MIDI must not independently emit parameter writes. Preserve the legacy instrument as its explicit compatibility configuration/recovery image. Do not ship a hybrid R2-labelled UI whose controls silently use the old fixed map.

Physical sensing and physical-position presentation may run while unsynchronised. Engine writes remain ineligible until an admitted profile, compatible session and authoritative engine snapshot permit them. Disconnected positions are observations, not base values to replay. Applying a real local BindingView must not fabricate engine synchronisation or arm pickup against zeros.

Audit all application egress together: subscribe callback, transmit-loop subscription transitions, ordinary hysteresis, startup baselines, pending queues, synthetic stimulus and pads. Switching owners retires ineligible work; never keep the same sample eligible in two paths. Preserve the legitimate output-authority rules: no global master, blackout or current-limit fader bindings.

Add tests around the real production integration seams, not only a constant:

- Boot with stationary nonzero faders; connect and subscribe: zero parameter writes.
- Stationary faders at endpoints; reconnect, unsubscribe/resubscribe, engine restart: zero parameter writes.
- Ordinary motion under the selected owner: the expected single stream, no duplicate legacy write.
- Navigation during motion/queued final: captured target and revision preserved; no new write simply from selection.
- Backpressure, failed enqueue, lost acknowledgement and reconnect: final value handled under the existing lifetime/session contract, not silently dropped or replayed under a new identity.

Prove the assertions detect removal of the actual caller guard. The previous constant-only policy test is not enough.

Replace LVGL reads of mutable s_inp/s_local/s_profile_view with a safely published observation. Reuse the repository's bounded queues/copy discipline where appropriate, with explicit failed-read behaviour and ownership lifetime. A bare volatile flag or unsafely reused two-buffer arrangement is not a coherency proof. Never hold the UI lock around radio, compiler or flash work.

Preserve producer sample sequence, timestamp, validity, calibration identity and selected profile revision. Before the first valid scan, display unknown; after timeout, stale. Do not turn the zero-initialised fallback into a valid measured position or refresh sample age every time the UI reads it. Physical percentage must be labelled distinctly from requested/applied parameter values.

Measure the entire scan cycle and missed/coalesced releases. `finish_scan(sweep_us)` currently reports only mux duration; do not pass it off as whole-cycle timing.

**A2 exit:** mutation-sensitive no-replay/single-owner tests, real page/view navigation tests, coherent state publication and truthful validity/status presentation. Live actuator connection is still gated on applicable evidence.

## 6. A3 — repair peer identity, then diagnose any remaining BLE failure

Map to TRN-1/2, INT-1/2 and the relevant DUAL deployment/integration tasks. This connection-only work may run in parallel after isolated ownership is established; it must not release commands to the live engine while A2 is open.

Current evidence gives a specific first discriminator: S3 accepts K1-CONSOLE at scan but rejects it after connection if its identity characteristic is missing. Console ble_midi_begin currently creates only MIDI. Do not begin with speculative bond wipes, an encryption downgrade, framework swap or changing the advertised name to evade admission.

Read the canonical peer registry, generated contract, actual console identity/transport and receiver validator. Establish exactly what the existing identity attests. The generated legacy 87-byte fixture includes a fixed layout hash; do not advertise those bytes while implementing a different mapping. Either implement the legitimately corresponding compatible interface, or carry a reviewed versioned R2 identity/capability change through BOTH producer and consumer. Keep wrong product, malformed identity, CRC, layout/registry mismatch and unsupported-version refusals operational.

Add stage-labelled events at bounded rate with monotonic timestamps and connection/session identity:

`scan/name → connect → security outcome → service discovery → identity read/validation → characteristic/CCCD subscription → transport handshake → compatible snapshot`.

Record the actual disconnect reason instead of discarding it. Separate link connected, subscribed, peer admitted, protocol compatible and engine synchronised. “BLE advertising” or “Linked” alone must not unlock writes.

Bind the resident S3 image before interpreting its behaviour. Then run an isolated connection-only campaign with actual command egress disabled. Confirm the identity branch from both ends. Only if logs show a remaining security/bond/discovery failure should a scoped repair of that layer follow; preserve unrelated bonds/calibration and required security.

**A3 exit:** truthful identity and capability agreement; receiver rejection negatives remain effective; measured stable connection/subscription in the isolated test; zero engine parameter applications during the test. This is not yet the complete v2 control loop.

## 7. A4 — admit the complete resource configuration and close P1

Measure the candidate that includes the REAL transport client, reliable lane/reassembly and advertised profile capacities. It may remain input-disabled until P2; it must not be omitted from the resource budget. P1 does not require P2 actuator proof, but does require capacity for the components P2 will use.

The map's 49,152-byte pool is LVGL, not Bluedroid. Static .bss reduction is real, but heap-backed first-use storage has not disappeared. The reviewed code retains substantial per-operation scratch after first use; reduce duplication/lifetime overlap before picking arbitrary page-count cuts or changing stacks/frameworks.

Record internal static allocation, task stacks, DMA buffers, ordinary/internal/external heap, largest available blocks, first-use/steady-state peaks and fragmentation separately. Establish the actual location of heap allocations; do not assume either that all lazy storage is internal or that all external memory is suitable for every callback/DMA/cache state.

The 20-line, 24,000-byte tile is a reasonable candidate to test, not automatic display qualification. Verify even-region rounding, rotation, capacity, actual transfer completion and buffer reuse at the current driver boundary. Do not move DMA draw buffers to unsupported memory. Keep the pinned QSPI-capable platform and DIO configuration unless a separately justified change is admitted.

Load cases include startup, connected radio, all faders, repeated navigation, maximum supported draft, commit/undo/import/export, worst permitted receive/reassembly work, allocation failures, stale worker results and reconnects. Freeze thresholds from existing requirements/platform constraints before collecting acceptance results; do not select thresholds after seeing maxima. Retain stronger original duration requirements; a short smoke cannot replace them.

**P1 closure requires A0–A4 and applicable original gates.** A physically unavailable console leaves its runtime/display rows NOT_TESTED/BLOCKED, while code/build/reference work continues. No inventing measurements, using omitted functionality to fit RAM, or treating the review's four probes as the whole acceptance set.

## 8. Next after P1: P2, then the joined instrument

Begin P2 with one already-admitted fixed profile and one non-output-authority parameter, same content/contract identity on both ends:

`sample → admitted binding → typed gesture → ClientEndpoint → real BLE v2 carriage → S3 RelayV2 → existing bridge → Teensy ControlV2Port → applied receipt/state → coherent console view`.

Reuse existing portable endpoints and relay. Keep S3 transport-only. Verify true terminal-value behaviour, no duplicate application, expected revisions, epoch changes, external edits and snapshot reconciliation. A transport acknowledgement is not engine application. Then exercise all nine faders, both channel scopes, pickup/endpoints and independent console/bridge/engine restart. This is still not physical-output acceptance.

Continue P3 with real console/engine platform stores, interrupted-save and activation tests, distinct local/durable/engine states and external profile-transfer requirements. Do not let durable-profile work become a prerequisite for the first fixed-profile P2 proof, nor let the fixed-profile proof cancel P3.

P4 can proceed independently under a separate bench owner: frozen black, sparse markers, centre/ends and walking-pixel fixtures; immutable submitted buffers; completion/stop/reset/fault evidence; actual electrical and optical observation as required. A scored LED dump is useful but not observed light. Where instruments are absent, mark those measurements blocked and do the software fixture/scorer work. Do not restart speculative renderer or pulse-timing changes from an unproven causal summary.

P5 joins P2/P3/P4 with matched stimulus, configuration, image tuple and output limits. Use Bloom mode 3 first, but preserve catalogue/full-topology obligations. Recompute timing for the actual bits-per-pixel and lane topology; keep the acoustic-event-to-light endpoint.

Titan remains separate: do not replace the local live-audio/paired-output work with the cloud branch. Resolve actual P601/P603/P604 mapping from applicable current receipts, not this report's shorthand. Follow the current E2 Lite/SWD router; no old serial programmer, GDB workaround or unsolicited wire move.

## 9. Handback and next action

Start A0 and the failing A1 assertions now; do not ask for another generic approval or make a board plug-in the condition for all progress. Continue isolated ready work when an unrelated measurement is blocked. Stop at actual ownership/admission/recovery failures for the affected action.

For each work package, return changed files/scoped commits, actual source delta, exact commands and exit codes, before-fix and after-fix evidence, generated resource proof, dependency/manifest IDs, target identity where measured, raw logs and leave-state. Update the existing task records. Do not mark ORCH-CLOSE or any full product criterion from partial component evidence.

The desired next handback is: **the reproduced profile defects are repaired; editor work is bounded; R2 has one writer and coherent truthful state; the peer contract admits the actual console without weakening checks; the full configuration fits; then one real typed gesture is applied and acknowledged.**
