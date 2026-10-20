# K1 R2 — Grok CLI execution brief

**Prepared:** 25 September 2026, Australia/Perth.  
**Purpose:** Turn the Captain's development assessment into bounded implementation and qualification work.  
**Execution lead:** Grok CLI, within existing repository governance and live device ownership.  
**Status:** Execution instructions, not an implementation receipt. No product source was changed, no firmware was flashed, and no hardware test was run in preparing this handoff.

## 1. Outcome and reason for this assignment

Deliver a functioning, evidence-bound K1 instrument: a real physical console controls an identified engine through the actual transport; the engine returns authoritative applied state; the identified output reproduces the submitted frames; reconnects, errors and power interruptions do not create uncommanded changes.

The earlier assessment is strategic context, not a patch specification. Its strongest conclusion remains: close the integration and qualification gaps before expanding effects or architecture. The most consequential immediate gaps are the R2 console's resource/behaviour split, its disconnected control path, incomplete persistent/profile activation paths, and unresolved physical-output evidence.

**The first complete use case is a milestone, not a scope reduction.** Use an already-admitted channel/parameter/profile and palette-led Bloom mode 3 for the first controlled comparison. Retain the complete programme: both 160-pixel logical channels, intended full physical topology, all advertised mode obligations, native-16 qualification where applicable, independent verification, and separate target dispositions. A 128-pixel WS2812 bench or one functional fader cannot close those broader criteria.

Do not return only another assessment. Once the entry checks and applicable gates pass, implement the next ready, owned task, test it, bind its evidence, and continue through the dependency order. A genuinely blocked physical measurement does not stop independent software work.

## 2. Authority, full plan, and permitted boundaries

Read this brief alongside `reference/K1-Implementation-Handoff.md`. That file is the complete retained technical handoff, copied without rewriting or abridgement. Its 56-task programme and detailed specialist requirements remain in force. This brief is an execution-order and freshness addendum, not a substitute for them.

Load the actual installed SpectraSynq workflow, current task registry, repository instructions and relevant local skills before editing. Reconcile this work with the existing ownership records; do not create a replacement governance system or a second shortened task list. Use the current `execution-manifest.json`, `acceptance-catalogue.json`, programme checkpoint and acceptance matrix where available. The retained Markdown is not the companion raw-source ZIP, and this package does not claim to include that ZIP's tools or raw snapshots.

Apply the current user decisions and repository procedures. Historical documents remain evidence for the scope they actually covered, not automatic current wiring or deployment authority. If authorities disagree, preserve the disagreement, resolve the affected facts using newer source/image/device-bound evidence and existing decisions, and stop only the affected operation if it cannot be resolved. Do not choose whichever old passage makes a convenient implementation pass.

**Primary implementation scope:**

- `/Users/spectrasynq/Workspace_Management/Software/K1.Sliders`
- `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware`

**Separate, downstream/parallel platform scope:**

- `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`

Preserve each repository's boundaries. In particular, do not import DualMCU Git history into Titan, create prohibited sibling worktrees, reset an active checkout, discard another owner's dirty work, or edit unrelated PCB, legacy shipping-firmware, BSP or governance projects. Existing worktrees are not permission to create more. Scoped commits follow the current repository workflow; this brief does not independently authorise merges, pushes, production promotion or purchases.

Existing firmware-programming authorisation remains applicable where it exists. Do not invent a universal no-flash rule or repeatedly ask for already-granted permission. An actual HOLD, occupied device, missing target identity, unresolved physical mapping or failed admission gate still stops that operation. Firmware changes do not move wires.

## 3. Mandatory freshness correction

The first assessment inspected these local anchors:

| Repository | Anchor | Branch |
|---|---|---|
| DualMCU | `21de29be3fff18244a4da7a81b21e13f35bb2694` | `int/reconcile-migration-lane` |
| K1.Sliders | `f143da58eb8d5ff71bef32579d77aa2add8bc65d` | `r2-console-flash` |
| Titan Mac work | `8427fccfb1c3b7fface2c2ba0a56a6017b785acf` | `wip/k1-ra8p1-002-mac-hold-2026-09-25` |

These are historical inspection anchors, not checkout instructions. The follow-up read confirmed the console branch still pointed at `f143da5`, while its working source had changed materially. **A matching HEAD is not proof that the source, executable or running device is unchanged.**

Read `SOURCE-RECHECK.md` before implementing. At the follow-up read:

1. `K1_UI_R2` selected a heap-backed, RAM-only profile-store implementation rather than the host persistent-store implementation. Its `persistActiveDraft()` returned true without writing storage.
2. The device `main.cpp` used a counted gesture-send stub rather than the earlier instantiated v2 transport client. Local profile outcomes were displayed through `TxnStatus::Accepted` without engine confirmation.
3. `ProfileStore::applyDraft()` could leave `outcome.accepted=true` after an allocation failure, and `commit()` ignored the function's boolean result. This is a source-level false-success path requiring a deterministic regression test; it was not reproduced on hardware in this handoff.
4. Legacy MIDI emission and subscribe-time send-all remained reachable in the R2 build. Restoring Bluetooth must not release stationary fader values or compete with the R2 control authority.
5. The current host test script did not define `K1_UI_R2`, so those tests did not exercise these device-only branches.
6. Navigation/profile-to-input binding and a safely published coherent UI snapshot remained integration work, not something established by a compiling UI.

These observations supersede only the corresponding earlier source descriptions. They do not establish the current flashed image or justify discarding the new work. Recheck exact bytes at execution start and before committing; another owner may still be editing them.

## 4. P0 — establish ownership, source identity and recovery

**Why:** No implementation or test result is usable if it refers to another source variant, another target, or an image rebuilt after qualification.

Read the relevant instructions, including:

- DualMCU `AGENTS.md`, `docs/agent/AGENT_EXECUTION_STANDARD.md`, `docs/agent/DUALMCU_BENCH_IDENTITY_CANON.md`, `docs/plans/master-plan-v3.md`, relevant boundary READMEs and protected-file procedure.
- Console `AGENTS.md`, `K1_CANONICAL_CONTEXT.md`, current R2/console handoffs, `platformio.ini`, `docs/AMOLED_CRITICAL.md`, vendor manifests and test runners.
- Existing programme `docs/evidence/k1-programme-2026-09-24/close/`, `harness/ivv/acceptance_matrix.json`, `harness/ivv/ivv2_verdict.json` and the current task registry.
- Before Titan work: its current `AGENTS.md`, `docs/REFERENCE-MANIFEST.md`, canonical Titan engineering memory and `~/.cursor/skills/ra8p1-e2lite-router/SKILL.md`.

The top-level DualMCU `PROTECTED_FILES.md` referenced by its instructions was not found in the follow-up read. Locate the actual current protection authority; absence at that path does not make protected interfaces unprotected. Continue independent unprotected work while resolving any affected permission boundary.

Use `tools/read_only_preflight.py` as an optional starting inventory. It reads Git status and hashes selected source files; it does not build, fetch, write repositories, enumerate hardware or open ports. It is not a complete transitive build-input manifest and grants no GO by itself.

Before editing, record the actual HEAD, branch, affected-file hashes, staged/unstaged changes, active ownership, intended scope and a fresh evidence location sanctioned by the repository. Run the required existing harness entry after checking its instructions. Do not require a destructively cleaned whole workspace; preserve unrelated changes and bind the actual candidate through the accepted source-identity mechanism. Prefer a committed, reproducible scoped candidate. A permitted dirty candidate needs explicit content identity, not just a HEAD hash.

### Device separation

| Device role | Identity to re-establish | Boundary |
|---|---|---|
| Console AMOLED 2.41 | `FC:01:2C:DA:2B:38` | Console image only; not the DualMCU bridge |
| DualMCU S3 AMOLED 1.75 | `30:ED:A0:29:A1:C4` | BLE bridge or Wi-Fi AP image according to the test |
| DualMCU engine, Teensy 4.0 | `20731830` | A correctly guarded Teensy 4.0 environment, never a Teensy 4.1 image |
| Titan | Application UID `545433931bd25436593630352d068363` | Separate platform and programming owner; rebind candidate/probe before writes |

Ports are discovery results, not identities. Opening serial is not assumed side-effect-free. Use the existing owner/lock and transport-specific non-resetting read path; do not kill an editor, Teensy.app, or another agent's serial owner.

Keep both S3 images as already decided. The BLE image is the console integration target; the Wi-Fi AP image remains separate. Do not switch the carrier back to UART because a surviving older section says UART: the inspected canon contains old UART/H2 passages alongside later SPI source/receipts. Resolve transport and pins from the actual candidate and applicable physical evidence. Retain all valid identity refusals.

Preserve the console recovery application:

`K1.Sliders/preserved-bins/gold-ledger-FC012CDA2B38-factory-0x10000.bin`

Recorded SHA-256: `c27da0d74bc5d2f2ffa3c4ba67d9b457deeed3f8fa9f519e901196cc31963272`.

This is an application-partition readback, not an NVS/calibration or whole-flash backup. Before a storage change, identify and preserve the applicable partition table, namespace layout and recoverable user data through the approved route. Do not erase calibration to make startup warnings disappear. Do not re-enable a banned calibration wizard.

**P0 exit:** one named owner per edited area/device; reproducible baseline and recovery scope; affected documents/contradictions resolved or explicitly bounded. This is not product qualification.

## 5. P1 — restore R2 semantic integrity and resource feasibility

**Priority:** first software implementation package. It has two inseparable parts: make the functions fit and preserve what they mean.

### P1a. Add device-path regression coverage before increasing connectivity

Build tests against the actual `K1_UI_R2` implementation and integration adapters, not only the alternate host branch. Share portable behavioural logic between variants wherever appropriate; platform allocation and storage adapters may differ, transaction semantics must not silently differ.

First prove the following failures can be detected:

| Case | Required result |
|---|---|
| Allocation failure at each profile staging/activation allocation | No accepted outcome, no reset/abort, no partial active-state change; a specific resource error |
| Storage adapter absent, write fails, or verification fails | No claim of durable save; prior durable state remains recoverable |
| Engine missing or rejects a command/profile | No engine-applied status; local draft/validation state remains separately labelled |
| R2 connects/subscribes with stationary faders | No legacy send-all and no v2 base-value writes |
| R2 and legacy paths both compiled | Exactly one admitted control path owns each physical event in the selected mode |
| Navigation while a gesture is queued | The gesture retains its captured target/revision; no retargeting to the newly viewed page |
| Concurrent scan/update and UI refresh | Coherent snapshot publication, no unsynchronised mutable-structure access |

In the currently read source, `on_subscribe`, `ble_tx_task`, `maybe_hysteresis` and R2's `next_intent` drain must be audited together. Do not remove legacy compatibility globally; isolate it correctly from R2. **Before reconnecting R2 to a live engine, prove the no-replay/no-competing-write gate.** Connection-only resource testing may use an isolated receiver that cannot affect the engine.

Make local validation, local durable save, transport pending, engine applied, rejected, unavailable and uncertain-after-disconnect distinguishable using the existing view-model/contract wherever possible. Do not invent a parallel protocol or show a generic green success for a different operation. A transport delivery acknowledgement is not an application receipt.

### P1b. Rebuild the memory budget from the real candidate

Inspect the new linker map and generated configuration. The earlier table (49,152-byte LVGL pool, 35,404-byte profile store, 29,728-byte input object, 24,696-byte transport object, 16,000-byte UI object, 11,554-byte export buffer and a 54,000-byte draw buffer) belongs to an earlier identified binary. It is not a current allocation census after the device-only heap refactor.

Record internal static memory, DMA-capable buffers, ordinary heap, any external-memory allocation, task stacks, largest free blocks, low-water marks and peak scratch separately. Include first-use allocations for Commit, Undo, Import and Export; moving storage from static objects to lazy heap is not proof of reduced peak demand. Include the real transport client, receive/reassembly state and profile-transfer load in the final budget. Removing them to boot does not solve their resource requirements.

Reduce unnecessary lifetime overlap and duplicate storage first. Consider bounded display tiles and measured placement of cold data. The earlier 15-line tile was only an arithmetic example, not an approved target: the current display has a rounder that expands areas to even boundaries. Select and prove compatible tile geometry, rotation, alignment, transfer capacity and flush completion before adopting it. Do not mark a buffer reusable before the real transfer releases it.

Use the installed target SDK/driver constraints to decide memory placement. Do not assume all PSRAM allocations are suitable for every DMA, interrupt or cache-disabled path. Preserve the pinned QSPI-capable platform and DIO setting; do not change framework, display driver or Bluetooth stack merely to get more RAM without an evidence-backed scoped decision.

Inspect actual generated stack usage: `decodeDraft()` currently declares a whole `ProfileDraft` local, while the scan task was created with 4096 stack bytes. Quantify compiler output and the full call path; do not assert a measured overflow from a source declaration alone. Move heavy compilation/persistence out of the time-critical scan path using the existing bounded ownership model, or prove the current path meets its budget. A larger stack is not an unmeasured universal cure.

**P1 exit:** the real R2 feature set, display, scanning and Bluetooth coexist within frozen resource/timing bounds under startup, editing, navigation and reconnect load; all P1a negatives pass. A UI-only diagnostic build may remain labelled diagnostic, but cannot pass this gate.

## 6. P2 — close the actual console-to-engine control loop

**Why:** Host endpoints already exist. The missing work is the platform/application connection, not another simulator or a generic Bluetooth rewrite.

Trace and implement every leg:

`physical sample → admitted binding → typed gesture/transaction → ClientEndpoint → BLE carriage → S3 RelayV2 → existing bridge carriage → RT1062 ControlV2Port/control authority → applied receipt/state → console reconciliation → coherent UI snapshot`.

Use the existing canonical v2 codec, versions, identifiers, capacities, session/epoch rules, receipt adapter and relay. Derive any missing BLE characteristic interface from the current transport contract and a reviewed producer/consumer update. Do not invent UUIDs, opcode values or message-size assumptions in isolation.

The S3 remains radio/transport-only: no AP, VP, pixel frames or control-policy ownership. Preserve bounded buffers and existing DMA/READY ownership. Transport callbacks enqueue bounded work; they do not directly mutate a running control engine or LVGL state. Credit exhaustion, fragmentation, queue saturation and disconnect have explicit, tested outcomes. Do not drop a terminal fader value merely because a send could not currently be queued, and do not retry an obsolete transaction under a fresh identity that permits a second application.

Wire the admitted compiled profile to the input runtime and page/channel selection. A menu that merely drains Navigate intents is not navigation integration. The console must use an accepted engine snapshot for pickup/reconciliation before physical writes become eligible. A BLE link alone is not a synchronised engine session.

Begin with one already-admitted fixed profile, one non-output-authority parameter and both sides using the same descriptor/contract identity. This closes the platform path without simultaneously inventing arbitrary profile transfer. Do not bind global master, blackout or current-limit parameters to faders against the existing output-authority rule.

**P2 exit:** one real movement is applied and reported by the identified engine exactly as the frozen gesture/receipt contract requires; the terminal value is correct; an idle connection/navigation/reconnect causes zero base-value writes; stale or conflicting work is rejected. Then exercise all nine physical faders, both channel scopes, endpoint pickup, final sub-hysteresis movement, an external state edit and engine/bridge/console restart independently. This is a control-path milestone, not optical or full-catalogue acceptance.

## 7. P3 — durable profiles and custom-profile activation

Reuse CTL's compiler, profile format and power-safe persistence abstractions. Audit the real platform adapters; do not call a no-op successful save. Preserve active, staged and durable identities explicitly.

A compiled local profile, a persisted console draft and an engine-active profile are different states. The earlier identity-only notice (`0x50` in the inspected host helper) does not transfer or establish the engine's full compiled table. Either both sides already possess and verify the exact approved artefact, or implement the canonical bounded transfer/validation/activation mechanism. Receipt agreement must cover actual content identity and the relevant revision/session, not merely matching display labels or independently chosen numeric IDs.

For the promised durable edit, stage and validate the whole candidate, persist/verify according to the established store contract, and activate atomically. Preserve the previous usable revision on failure. Where product behaviour allows a RAM-only local draft, label it explicitly as unsaved and unapplied rather than promoting it to the durable/engine result. Do not attempt a fictitious atomic flash transaction across two devices: use explicit pending states, identity-bound acknowledgement and recovery reconciliation.

Use the current revision law. Undo must create a new admitted activation, not revive an obsolete transaction/revision identity; re-import must not allow stale gestures to become valid again. Allocation failure, future schema, CRC failure, semantically invalid binding, a lost activation acknowledgement, partial transfer, reboot mid-save and full storage all require bounded recovery tests.

A ByteStore backed by NVS/blob operations is not automatically equivalent to raw byte-addressable flash. Prove the adapter's partial-write, commit, durability, wear and recovery semantics against the existing store contract. Keep flash writes out of the audio/LED/scan critical paths. Use debounced explicit saves, not writes on every fader sample. Preserve calibration namespaces separately. RT1062 storage needs its own platform-specific adapter and capacity/ownership check; do not assume console storage rules apply to it.

**P3 exit:** a power interruption leaves a valid old or new record, never an accepted corrupted mixture; the same declared profile is recovered, compiled and reconciled on reboot; idle faders are not replayed; local/durable/engine statuses remain truthful. Complete the required external import/export surface under the parent plan; a same-process export-then-import loop is not that surface.

## 8. P4 — output qualification, independently owned bench work

This can proceed in parallel with P1–P3 only when edit and device ownership are disjoint. Otherwise execute sequentially; do not imply multiple agents are working when none were launched.

Read the actual latest `RECOVERY_AND_DMA.md`, driver source, output format, pin mapping, image identity and hardware receipts. The earlier all-zero software dumps plus reported sparkles localise a useful next investigation, but do not prove a particular electrical threshold cause. Do not rewrite the renderer or repeat speculative timing sweeps as the default response.

Prepare fixed, low-output fixtures: all-black; isolated colour/bit markers; one walking pixel; centre pair; ends; channel/half-line identity; return to black. Bind submitted bytes and completion generation to each fixture. Check immutable DMA ownership and matching completion, including fault and timeout cases, before attributing disagreement solely to the strip.

Compare the same known frame at the controller output, first-strip DIN and observed LEDs using available appropriate instruments. Record sample timing, voltage/logic levels and power-at-load observations where measured. A digital analyser decode alone does not establish analogue signal quality; a photograph alone does not establish pulse timing. Use an explicit safe power limit and preserve blackout authority; do not run unrestricted full-white merely because the software limiter is inactive.

For the 128-pixel cropped bench, physical centre 63/64 maps to native logical 79/80; record the actual mapping and current pin instead of reusing the earlier IO1 text or later pin-0 text indiscriminately. A successful crop remains crop evidence.

If instruments or physical access are unavailable, preserve that BLOCKED measurement, complete offline fixture/scorer/ownership tests, and continue ready software tasks. Do not replace missing waveform/optical evidence with a software PASS or task the user with another speculative rewiring experiment.

**P4 exit:** the identified physical output reproducibly stays black for black input and reproduces the admitted bounded fixtures; errors do not credit completed emission; restoration of prior configuration and continuing output is verified. Full product qualification still needs the full intended topology.

## 9. P5 — join the paths and qualify one musical candidate

Join P2, P3 and P4 on a single declared candidate tuple. Compare compatibility and admitted wide behaviour with the same stimulus, palette, configuration, physical mapping, output cap and optics. Use Bloom mode 3 first; include independent channel values, centre-origin motion, a silence/dwell interval, palette transition, enable/disable, blackout, reconnect and recovery.

Record resource and timing distributions under simultaneous display/input, transport, audio and output load. Retain prior failures and all required counters. Do not substitute a shorter smoke test for existing longer or stronger qualification requirements. If a new resource threshold is needed, freeze it from the requirement and platform constraints before measuring; do not choose it after seeing the result.

Recompute latency for the actual audio rate/hop, render schedule, pixels/lane, bits/pixel, lane parallelism, reset interval and optical endpoint. The previous 5.2 ms combined model used 24-bit four-by-80-pixel output. At 800 kbit/s, an 80-pixel payload is 2.4 ms for 24 bits and 4.8 ms for 48 bits. Holding its other assumptions fixed changes that model to 7.6 ms for native-16, before the omitted costs. These are calculations, not a guaranteed or observed latency.

Do not redefine acoustic-event-to-light as an internal software interval. Keep canonical tempo behaviour unchanged while measuring whether a separately admitted fast-onset path is warranted.

**P5 exit:** per-criterion results are bound to the exact candidate and physical scope, with an independent review and explicit adoption disposition. A successful first profile unlocks the remaining catalogue and full-topology work; it does not mark the entire programme complete.

## 10. Titan — preserve the proven platform while importing wide output

The inspected cloud wide-output candidate `0339c19` and Mac WIP `8427fcc` solve different parts of the problem. Re-read their current descendants, pinned import receipts and platform diffs. Preserve the local live-audio/paired-output/recovery work while integrating the reviewed portable wide modules. Do not replace the Mac checkout with the cloud branch merely because its host tests pass.

The cloud native-16 fixture emitter inspected in the assessment was sequential diagnostic output with interrupt-masked bit trains; it is not evidence of paired GPT/DMA execution under live load. Connect the genuine wide frame to the applicable owned paired backend, preserving pin/profile/generation checks, stop/reset completion predicates and fault accounting. Confirm route provenance and same-RGB8-code/different-native-word cases under a frozen neutral treatment/gain configuration; off-lattice words alone can be created by output scaling and are not sufficient proof of a genuine wide producer.

Do not select P004, P603 or P604 from a stale narrative. Establish the exact candidate's PFS/timer/DMA assignments and actual physical wire route with current receipts. In particular, old programming and wiring passages inside the cloud handoff are not automatic authority over the later local bench workflow.

Use the current local E2 Lite/SWD router, probe power off, exact target and image binding, and existing lease/recovery rules. The inspected local authority retires button-driven serial programming and `programme_scalar.py` as the writer. Do not revive it from the historical parent handoff. Do not use GDB or reset/signature reads as harmless substitutes for non-resetting application inspection.

Preserve default-off compatibility. Qualify native output separately, with a real safe power envelope: the cloud endpoint's deliberately inert limiter is compatibility behaviour, not power-safety admission. Do not enable M33/U55/Helium or reopen platform choice as part of this scalar integration.

## 11. Existing tests, added tests, and evidence rules

Keep the existing console host gate, input gate, UI rendering gate, vendor checks, DualMCU architecture/identity/transport/control/IVV gates and Titan import/runtime gates. Resolve exact current command lines from the local runners; do not guess a target or skip a failing baseline.

The console runner at the follow-up read is `tests/host/console/run_host_tests.sh` and requires an explicit `DUALMCU_ROOT` matching `tests/host/console/DUALMCU_PIN` (`97dc8cf73318dfea1329e29a879fa8b9db1a7a6b` at inspection). Do not reset the active DualMCU integration checkout to satisfy that pin. Use the repository-approved reference/fetch procedure with its side effects inspected. An unpinned override is diagnostic evidence, not the reproducible pinned gate. A deliberate repin must carry reviewed producer/consumer deltas and regenerated/verified manifests.

Minimum added or explicitly mapped checks:

- Actual device-preprocessor path: allocation-failure atomicity, no-op persistence rejection, truthful local versus engine status, bounded maximum-size profile operations, and generated stack/memory evidence.
- Real application boundary: no send-all on R2 subscribe; no dual legacy/R2 ownership; navigation does not write; dropped/duplicated/out-of-order frames; epoch/revision conflict; no stale replay after any endpoint restarts; terminal fader value survives backpressure without stale reapplication.
- Admitted binding/profile: real `apply_view` path; channel/global scope; capability loss; external edits rearm pickup; all physical faders and endpoints; forged semantic envelope and slew-endpoint cases corresponding to the parent's still-open R2 coverage rows.
- Persistence/activation: missing/full storage, interrupted saves and transfers, acknowledgement loss, unknown future schema, valid old-or-new reboot recovery, and exact content/revision agreement.
- Output: black, mapping, colour/bit order, same-frame ownership, completion/reset/fault negatives, true wide provenance, matched target output and verified restoration.

Add missing tests beside existing suites rather than building a parallel test universe. For each repaired defect, preserve a before-fix reproduction or bounded mutation that fails the intended assertion, then record the repaired pass. Use scratch mutation sources; never run a deliberately broken image on the live instrument merely to demonstrate that a test can fail.

Evidence classes remain distinct: SOURCE_REVIEW, HOST_TEST, TARGET_BUILD, TARGET_RUNTIME, ELECTRICAL, OPTICAL and ADOPTION. Do not call a source review a test or a buffer dump a waveform. A proposed check is NOT_TESTED until executed. Do not call your own second reading an independent verification receipt.

For each built/programmed candidate, bind: source identity and affected bytes; canonical contracts and vendor pins; compiler/platform configuration; ELF/map/flash-image hashes; post-link checks against that exact ELF; exact guarded write; application/device identity; profile/configuration; test/scorer/stimulus; raw logs with timestamps and exit status; and final leave-state. No silent rebuild between gating and writing. Rebuild means new hashes and rerunning the affected gates.

The prior production receipt recorded PASS despite a raw missing-ELF failure and an empty effect-reachability log. Repair by producing valid new evidence and an append-only correction, not deleting those files or restating the receipt as proof.

## 12. Map this order into the full programme

The mapping below is a navigation aid. Verify exact dependencies and assignments in the actual 56-task manifest; it does not remove the remaining rows or alter gate requirements.

| Package | Existing task families to reconcile |
|---|---|
| P0 source/recovery/ownership | ORCH-0, INT-0, DUAL-1/DEPLOY0, TIT-1/DEPLOY0, IVV-1 |
| P1 device semantics/resources | CTL-3/4, INP-2/3, UI-2/3, INT-2, IVV-2 |
| P2 real control path | TRN-1/2, INT-1/2, DUAL-2/3, INT-D, INP-2/3 |
| P3 storage and custom profiles | CTL-2/3/4, UI-2/3, DUAL-2/3, INT-D |
| P4 output qualification | DUAL-SMOKE/3, INT-D, MOD-D, IVV-D; corresponding Titan rows only on its identified candidate |
| P5 integrated musical/latency evidence | VP-2/3, AP-4, MOD-SLICE/D, INT-D, IVV-D and UI-3 |
| Titan scoped integration | TIT-2/SMOKE/3, INT-T, MOD-T, IVV-T |

Do not restart completed host work simply because a new agent took over. Re-run only the baseline and affected dependency evidence required by the candidate. Record separate source, build and bench status. ORCH-CLOSE remains dependent on its full parent criteria; standalone-S3 work retains its separate authorisation and status, not an inferred cancellation or new write permission.

## 13. First response and continuing handback

Grok's first substantive response should name the actual source delta since this handoff, ownership/device constraints, mapped next task and the first failing assertion or measurement it will close. It should then execute that task rather than request generic permission to begin.

Each checkpoint must state: files and commits changed; exact commands and exit codes; source/build/target identities; what the result proves and does not prove; raw evidence locations; remaining blockers with the next discriminator; device/config/output leave-state; and the next existing task IDs. Update existing records instead of creating another unsupported success summary.

Do not stop the entire assignment because one instrument, lane or acceptance row is blocked. Do stop writes or emission when its identity, power, ownership or recovery prerequisites are not met. Do not continue a speculative loop without a named discriminator and bounded stop rule.

**The success condition is an identified, controllable, recoverable instrument with verified light—not merely less RAM, a green host suite, a responsive display, or another completed report.**
