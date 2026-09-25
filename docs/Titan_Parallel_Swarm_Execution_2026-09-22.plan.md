---
name: Titan Parallel Swarm Execution
overview: "Replace the T1-to-T10 queue with six concurrent engineering lanes, one integration owner and one exclusive Titan/E2 Lite operator. Preserve all acceptance requirements; dispatch on actual dependencies, not phase numbers."
todos:
  - id: swarm-handoff
    content: "Coordinator: checkpoint the current agent safely, reconcile newer records, reuse completed evidence, seal the actual dirty baseline and assign isolated staging."
    status: pending
  - id: swarm-debug
    content: "PARALLEL D: repair and test debug/deployment tooling offline; prepare a bounded safe probe experiment only where evidence warrants it."
    status: pending
  - id: swarm-transport
    content: "PARALLEL X: pair ownership/completion, six retained tests, packing/capture negatives and four-lane resource feasibility."
    status: pending
  - id: swarm-audio
    content: "PARALLEL A: audio/ASRC/sample ownership/time, host fault injections and audio-feature regressions."
    status: pending
  - id: swarm-visual
    content: "PARALLEL V: behaviour/parity, two logical channels, centre-origin motion and staged TRUE16 work in one ownership lane."
    status: pending
  - id: swarm-workflow
    content: "PARALLEL M: real-PCM fixture, MIR tools, controls/presets, host-side ingress/persistence/peer contracts."
    status: pending
  - id: swarm-performance
    content: "PARALLEL P: timing/latency/scorers, robustness recipes, Q1-Q5 readiness and comparison evidence."
    status: pending
  - id: swarm-integrator
    content: "CONTINUOUS I: accept independently tested patches into isolated integration staging; cut the smallest ready candidate without waiting for all lanes."
    status: pending
  - id: swarm-bench
    content: "SERIAL H: sole Titan/E2 Lite/CDC owner; run admitted packets, publish sealed evidence, preserve faults and leave-state. Never wait on an entire phase."
    status: pending
  - id: swarm-verification
    content: "ON DEMAND Q: independent review of each completed change/evidence unit, then return the slot to useful work."
    status: pending
  - id: swarm-handback
    content: "Coordinator: publish integrated progress and exact unresolved gates in the existing ledger; retain original T1-T10 acceptance identities."
    status: pending
isProject: true
---

# Titan RA8P1 — parallel engineering, single-device execution

**Revision:** proposed scheduling replacement, 22 September 2026.  
**Deliverable of this document:** an executable division of work and dispatch policy, not a claim that agents have started or hardware tests have passed.  
**Applies to:** `SpectraSynq-K1-RA8P1-Firmware`, the existing K1-RA8P1-002 campaign.  
**Replaces:** serial phase scheduling and blanket phase-entry barriers in the attached plan.  
**Retains:** technical scope, acceptance thresholds, evidence rules, physical authority, source provenance, and all applicable T1–T10 requirements.

## 1. The scheduling correction

T1–T10 are acceptance categories, not ten jobs that must execute in order. A task waits only for the particular source, interface, artifact, measurement or authority it actually needs.

Remove “parallel after T1.” T1's physical-output rows are currently blocked by deferred wiring; making them a predecessor of host audio, replay, timing preparation or integrations unnecessarily stops work that is explicitly allowed.

The operating model is:

```text
                            COORDINATOR C
             ┌─────────┬─────────┬─────────┬─────────┬─────────┐
             D         X         A         V         M         P
           tooling  transport   audio    visuals   workflow   timing
             └─────────┴─────────┴────┬────┴─────────┴─────────┘
                                     I
                           one integration owner
                                     │
                         ready, identified run packets
                                     H
                        one Titan / E2 Lite / CDC owner
                                     │
                           sealed raw evidence
                                     └────> all relevant analysts
```

Six engineering lanes + I + H + the parent coordinator give **nine logical roles**. Q is a short independent verification assignment, preferably using an available slot rather than creating a permanently idle tenth agent. A role is not a requirement to keep an LLM generating continuously.

Start the six independent lanes in one launch wave, after their common baseline and paths are available. Do not launch D, await its final answer, then launch X. Do not run six competing implementations of the whole application. Read-only discovery can begin while the coordinator seals source; writes wait for a verified private staging tree.

## 2. Reconcile the starting point once; do not restart completed work

### What the supplied revision establishes

The revised attachment marks T1.1 completed. It explicitly defers the P603 move and physical observation, forbids pair emit and an A0-only workaround, retains Rearm for programming, and distinguishes the retained no-symbol candidate from a new `-O2 -gdwarf-2` identity. Preserve those restrictions. A completed checkbox points to evidence to reuse; it is not an instruction to repeat the entire baseline campaign.

### Important newer local records found during this review

A read-only inspection of the Mac found a leading `STATUS.md` entry reporting build `3a7ebd7c…`, an emit-off rate-lock image programmed through E2 Lite/RFP at 1 MHz. Current `AGENTS.md` and the E2 Lite router specify that programming route and warn against the previously hanging GDB attach. The linked T1 acceptance table is older and still describes an earlier resident. These are **document observations, not a fresh device query or independent programming verification**. See `sources/READ_ONLY_INSPECTION.md`.

Consequences for startup:

1. The current agent finishes or checkpoints its present operation safely. Do not kill a writer, programmer, debugger or broker to start the swarm.
2. C/I resolve the latest source, build, programme and application receipts; H alone performs any necessary, permitted current INFO binding. Do not infer residence from an old candidate manifest or from a status heading alone.
3. Reuse the valid T1.1 and six-test results at their demonstrated identities. Recompute only what changed or whose applicability is unknown.
4. Do not flash the old `fa54c781…` candidate simply because the attachment names it. Do not rebuild an existing symbol-bearing image until its real gap has been established.
5. Do not execute the obsolete serial-ROM sequence as the default if current qualified RFP receipts supersede it. Conversely, newer prose without matching receipts is not sufficient to qualify a command.
6. Full-register GDB attach is not a default startup step. D investigates the evidenced tooling failure independently. Useful source debugging remains an original T1 requirement; it is not silently downgraded to “server connected.”

The review did not open CDC, connect the probe, reset, program, or modify the Mac checkout. This replacement does not grant a hardware-write authorization.

## 3. Non-negotiable boundaries

Preserve M85 AP+VP ownership, parked M33/U55, D-cache off, RT-Thread/FSP, 24 kHz / 180 samples / 7.5 ms analysis, time/epoch semantics, two logical 160-pixel channels and centres 79/80. Keep DMA storage in verified accessible SRAM, no per-frame allocation, and no hidden reduction in workload. The prepared pair is one logical channel on two physical 80-pixel lanes; it is not physical A/B and RGB8 expansion is not TRUE16.

No worker may open serial, contact a GDB server, use E2 Lite/RFP, reset the board, change controls through a broker, start a competing reconnect loop, or perform physical actions. H is the only target operator. Read commands also belong to that rule: an extra observer is still target traffic.

No new sibling product repository or worktree is created under this plan. Use the isolated staging already allowed by the supplied revision. No DualMCU, BSP, S3 or Teensy source writes, commits, flashes or moving-reference imports. No cloud upload, new paid service, quota bypass, toolchain installation or global permission relaxation is implicit in the swarm request.

Physical pair enable remains blocked until the current wiring and power gates are explicitly satisfied. Emit-disabled deployment, if separately authorized and proven to keep the output inactive, does not require falsely declaring physical mapping complete. Unknown persistent-region ownership still blocks NVM writes. Security/boot/calibration changes remain outside this work.

## 4. Source isolation: independent work without independent product forks

### One sealed source basis, many private staging trees

I preserves the original dirty checkout. Record HEAD, tracked modifications, relevant untracked and build-required ignored inputs, reference pins, tool versions, build flags and an explicit source/dependency manifest. A clean `git archive HEAD` alone is not the dirty runtime. Do not copy credentials or unrelated files just to make the snapshot large.

Use the existing staging procedure after checking its write paths. Namespace private trees within the existing artifact workspace, for example:

```text
EXT/<unique-swarm-run>/
  base/                 sealed source/dependency snapshot, no shared .git
  lanes/D/work/         tooling work + private build/test outputs
  lanes/X/work/         transport work + private build/test outputs
  lanes/A/work/         audio work + private build/test outputs
  lanes/V/work/         visual work + private build/test outputs
  lanes/M/work/         workflow work + private build/test outputs
  lanes/P/work/         performance work + private build/test outputs
  integration/<cut>/    I-owned assembled source and artifacts
  incoming/<lane>/     immutable handoff bundles
  runs/<unique-run>/    H-owned raw records and manifests
```

These are staging copies, not extra product repositories. Historical candidate and recovery artifacts stay immutable at their existing identities. Keep an explicit distinction between a historical candidate's source snapshot and the newly sealed development snapshot.

Private writable files must not be hardlinks to the original. Check resolved symlinks and build dependency paths; a private directory that writes into the sibling BSP or a shared generated directory is not isolated. Do not chmod the live checkout to implement worker isolation. Use existing sandbox/path controls where supported.

Snapshot under a short coordinated writer pause or use before/after manifests to detect a torn copy. Retry the changed snapshot input, not every historical task. Do not freeze all engineering for the duration of a test campaign.

### Exact ownership, not broad promises

C/I expand the following seed assignments into an exact path list before granting write access. The paths below were observed on the Mac; ownership is a proposed division of work, not a claim that every file needs changing.

| Owner | Initial private-staging ownership | Explicit exclusions / interfaces |
|---|---|---|
| D | `scripts/build_scalar.py`, `scripts/programme_scalar.py`, scoped debug/identity tooling and its own tests | Does not operate the tools on hardware; no broker edits or unreviewed compiler-policy change |
| X | `platform/ra8p1/ws281x_gpt_dma*`, `ws281x_waveform.*`, `ws281x_diag.*`, `titan_led_pins.h`; pair/packing/capture tests and runners | No PDM ownership changes, renderer rewrite or active physical enable |
| A | `pdm_capture.*`, `pdm_target.*`, `k1_pdm_*`, `k1_asrc_24k.*`, `k1_rate_adapt.*`, `k1_live_clock.*`, `k1_exact_stream.*`; identified portable audio files and tests | Clock/runtime integration changes submitted to I; no rate/gain adjustments merely to make a scorer pass |
| V | `palette_runtime.*`, `palette_transition.h`, `palette_clock.h`, `centre_palette_engine.h`, `hd_pixel16.h`; identified portable visual modules and tests | V owns both parity and TRUE16 to prevent two teams rewriting the same colour path |
| M | `tools/serial-studio/` client/broker/UI sources in private staging; fixture/replay/config and explicitly assigned new integration modules | No live broker replacement, target requests or unknown NVM writes; target protocol glue stays I-owned |
| P | Explicit timing/latency/scoring/replay-analysis scripts, timing tests and comparison analysis | X owns waveform conformance scorer changes; no unilateral shared-clock or algorithm edits |
| I | `fixture_app.*`, `hal_entry.c`, `SConscript`, `k1_live_runtime.*`, `k1_live_protocol.*`, `k1_live_schema.inc`, `k1_shared_snapshot.*`, other shared glue after inspection | Sole application of accepted changes into the canonical checkout; no target operation |

Prefix scope is `platform/ra8p1/` unless another path is shown. Existing tests are assigned by exact filename. One path gets one editing owner at a time. Unassigned paths are read-only until I allocates them. Generated build files are private per build, even when their input owner is I.

Each lane sends a patch plus before/after path hashes and tests. It does not merge into the live checkout. No global formatting, whole-file replacement for a small fix, `git add -A`, reset, clean, automatic stash or blanket cherry-pick.

### Shared-interface changes

A lane that needs a glue change sends I the caller/callee, exact proposed fields/signatures, ownership/time implications, backwards-compatibility rule and a failing test. It continues independent tests using the existing interface or an explicitly test-only adapter. I resolves the small contract with the affected owner; other lanes do not stop. An adapter is not product integration evidence.

I applies changes to integration staging first. A conflict returns to the relevant owner or is resolved with that owner; it is not silently overwritten. Canonical promotion is narrow and compare-before-write: if the live path changed after the snapshot, reconcile before applying.

## 5. Six engineering lanes and their deliverables

Each lane owns **implementation, repair, tests and handoff**, not just an audit. A report listing fixable problems is not completion. Routine fixes inside its assigned files and frozen requirements do not require a new architecture meeting.

### D — debug and deployment tooling

Inspect matched ELF sections/source paths, identity guarding, current RFP and fallback programmer receipts, allowed image regions and the actual GDB failure boundary. Preserve optimization; `--debug` and `--debug-info` must not be conflated. Verify any derivative's flags, source and image identity.

Reproduce parser/identity/receipt failures offline using retained logs and fake endpoints. Repair the demonstrated host-tool defect and retain a wrong-build negative. Do not treat mock success as device success. Do not repeat a known full-register connect that hangs the vendor server. Submit one narrow, evidence-backed bench experiment only after the supported command path and recovery procedure are ready.

Deliver: usable host tooling changes if needed; symbol/source binding result; exact permitted probe experiment or a precise vendor limitation; build/write/INFO checks; a recovery-compatible deployment packet. D does not gate A/V/M/P host work or an independently safe emit-off campaign.

### X — transmitter and physical-output preparation

Rebind all six retained pair tests to applicable source. Fix demonstrated admission, generation, callback, reset/stop and active-buffer ownership errors. Preserve atomic pair admission, one replaceable latest-pending complete pair and both-lane completion. Test malformed size/profile/backend, stale callback and one-lane-busy negatives.

Prepare mapping fixtures, independent GRB48 byte expectations, final-bit combinations 0/0, 0/1, 1/0 and 1/1, and capture-scorer negatives. Preserve 480 bytes and 3,840 bits per lane, with the applicable two-preload design's 3,838 follow-on values. Actual LED specification supplies wire limits.

Inventory four-lane pins/GPT/DMA/ELC/IRQs/SRAM/power without implementing a speculative backend. Deliver a conflict-free feasibility result or exact resource conflict, not a hardware stamp.

Deliver: tested transport patch or verified unchanged source, mapping/capture packets and four-lane resource report. Physical rows remain blocked until actual wiring/equipment are available.

### A — live audio, ownership and musical features

Trace capture→ASRC→180-sample hop→analysis→publication and raw-slot release. Test exactly-once sample processing, delayed consumption, overflow, stale ownership, wrap, source loss and epoch restart. Missing, stale and silent input remain distinct. Separate source-clock assumptions from observed rate; the reported rate-lock image is a named configuration, not general rate accuracy proof.

Use digital fixtures for numerical/frequency negatives. With the shared real-recording fixture, compare the applicable spectrum/energy/novelty/chroma/onset/percussion/tempo outputs against the pinned reference and frozen tolerances. Do not generate new goldens to erase a failure. Separate raw clipping from post-gain clipping.

Deliver: independently tested fixes and host regressions, a source/time contract, expected live counters and a compact capture/negative-test packet. The hardware packet uses the current candidate's actual instrumentation, not guessed fields.

### V — behaviour, colour and logical channels

Keep 23 admitted mode IDs and 44 palettes unless an explicit newer catalogue is validated. Trace actual runtime call paths, not just linked symbols. Test both logical channels, complete output-stage order, independent controls before joint limiting, interrupted transitions, quiet/wake/dwell, centre-origin direction, history, gaps and return motion.

First close the existing behaviour path. Then retain meaningful low bits through the real RGB16 pipeline and submit boundary, with low-byte-only, asymmetric and fade tests. RGB8×257 is not extra precision. Keep optional choreography as a named derivative, not an unannounced baseline rewrite.

Deliver: parity/behaviour fixes and replay artifacts; then a separate TRUE16 patch when ready. No optical or physical mapping claim from a host frame dump. X consumes V's frozen output contract rather than separately editing the colour path.

### M — MIR workflow and required integrations

Freeze one real 24 kHz mono signed-16 PCM fixture from an identified, permitted recording: source/conversion hashes and exact sample count. If unavailable, record that specific dependency and keep digital fixture work moving. Do not fetch a replacement recording or license silently.

Extend the existing broker/client/UI: bounded recording, gaps, atomic revision-checked controls, readback, full configuration save/load, observation replay and PCM reprocessing. Keep replayed observations distinct from recomputed audio. Use a recorded/fake endpoint for host tests; do not open the real broker as a convenience.

Prepare required T8 branches independently: persistence record logic under a mock storage backend; ingress adapters under the shared source/epoch contract; Titan-side peer/HMI version/ownership/handback and malformed/stale/reordered/disconnected-command tests. No target NVM, actual ingress fidelity or multi-device result without its prerequisites.

Deliver: executable workflow changes, one shared fixture, tested host adapters/records and exact remaining physical/authority gates. Target glue is requested from I, not edited competitively.

### P — performance, robustness and final evidence

Prepare current run/scorer commands and negative tests; retain the original deadlines rather than inventing a universal frame deadline. Rebind the empty-TCM Q1–Q5 assets, 6,000 raw hops, matched observer/control conditions and known 5 ms delay at hop 136. Do not substitute a live-runtime measurement for that campaign.

Specify compatible acquisition fields for AP/sample ownership, controls, resources and latency. Score sealed raw data while H runs the next compatible segment. Keep observer-off K1-L separate from observer-on or stressed traffic. Retain ≥1,000 generation joins and p50 ≤12,000 µs under the existing definition; no new hard p99 bound.

Prepare host-disconnect/reset/observer/control/audio/LED-fault recipes with explicit expectations and safe restoration. Collect the existing comparable RT1062 evidence read-only and keep the fourteen ruling questions/crosswalk current. T9 starts only from an actual measured benefit hypothesis; no standing accelerator swarm.

Deliver: recomputable scorers, complete compatible bench packets, latency/resource analysis and a comparison draft with missing evidence exposed. No invented power/temperature numbers or guessed optical accuracy.

## 6. Integration is a flowing queue, not a six-lane reunion

I validates and assembles the **next smallest useful change set**. D/X/A/V/M/P need not all finish. A behaviour-only patch must not wait for a persistence research branch. Conversely, do not flash six separate candidates merely because six agents returned patches.

Acceptance into a cut requires:

- Exact source base and touched paths; path ownership and preimage hashes match.
- A demonstrated defect or scoped requirement, a relevant negative, and passing affected tests without weakened acceptance.
- Declared interface dependencies, target reruns and rollback implications.
- Integration-build validation, actual HEX/ELF/map hashes, memory placement and protected-region checks when an image changes.

The integration gate reruns the affected suites **together**, not only each worker's isolated result. Include cross-lane contracts: audio→runtime; runtime→render; render→pair; controls→applied state; timing→generation/epoch. A buildable collection of patches is not enough.

Begin with a validated current baseline where possible; do not force a rebuild. Cut a new image only for an integrated benefit or a required diagnostic. Separate normal-runtime and intrusive diagnostic/profiler images. Proposed release labels such as `R0`, `R1` are scheduling labels, never replacements for complete build identities.

Allow one accepted next candidate and one image under test, not an unbounded queue of half-integrated variants. When integration backlog grows, redirect spare workers to cross-lane tests, first-fault analysis or review rather than generating more speculative changes.

Image changes invalidate applicable live claims. Reuse immutable host results only with explicit unchanged-input applicability; do not copy an old-image PASS onto a new binary. Settings, observation load, source fixture and boot/epoch changes also affect applicability even when the HEX is unchanged.

## 7. The bench is one exclusive service

### Ownership covers more than E2 Lite

H owns the **whole experimental session**: Titan state, probe, GDB/programmer processes, CDC broker lease and reconnect policy, control writes, actual output configuration and measurement interval. Two agents using different connectors to the same device are not independent.

Use the existing broker/lease and operator mechanisms. Add only missing coordination fields to an existing mechanism; do not build a new daemon, dashboard or general-purpose scheduler before doing engineering.

Each acquired session records owner/process identity, probe serial, target UID, device path identity, expected image/config/boot epoch, session generation, purpose, allowed operations and leave-state. A lease timeout marks ownership uncertain; it never automatically hands the board to a second process. Confirm a safe release before reassignment. Recovery of a stuck process follows the latest evidence-backed router, not generic kill/unlink loops.

C is the scheduling authority; H is the only command authority. All lanes submit packets to H. They do not compete for the lease themselves.

### Admission-ready packet

Every request contains:

```text
packet_id; originating lane; original T-row(s)
exact build/source/config/fixture identities; actual target UID
purpose and predeclared acceptance; expected positive and negative outcomes
verified command/runner and its version/hash; no unimplemented command names
required resources/authority; emit requirement; observer traffic mode
required raw fields, units and generation/time joins
bounded duration; first-fault actions; permitted reads and restoration
fresh output location; complete compatible recovery/leave-state
host preflight evidence; exact unresolved prerequisites
```

A packet with missing commands, unknown source identity, unverified write regions or absent authority does not reserve the board. H returns the missing field and chooses another ready request. Do not keep the target halted while an agent reads documentation or writes a parser.

### Current operating modes

| Mode | What may happen | What is excluded |
|---|---|---|
| Resident normal / emit off | Identified audio/runtime/control observations within declared traffic | Pair emit, hidden control changes, debug attach |
| Controlled debug | Only the newly justified, bounded supported inspection; quiesce first, epochs recovered after | Known hanging full-register attach; normal timing/latency scoring |
| Programming | Qualified route, exact authorized image and region checks; independent UID/build INFO afterward | Competing GDB/CDC owners; unapproved writes |
| Physical normal | Mapping or music on verified wiring/power; defined configuration/control script | Unplanned debugging or negative injections |
| Fault experiment | One declared failure/recovery experiment, preserved witness | Calling the injected interval normal-operation PASS |
| Profiler campaign | Identified matched Q1–Q5 variants and conditions | Replacing them with an unrelated combined runtime |
| Host-independent | Predeclared host-disconnected operation with suitable capture method | A hidden broker/debugger dependence |

Quiesce before a potentially disruptive attach, not afterward. CPU halt need not halt DMA/audio/peripheral clocks. Do not assume CDC can respond while CPU0 is halted; obtain pre/post evidence at valid running boundaries. Boot-epoch comparison is useful only when that image's epoch behavior has been established; unchanged or reused counters alone do not prove absence of reset.

Use a proven black/latch action where supported or the declared safe LED supply condition. Emit OFF or DIN low alone is not proof of black LEDs.

## 8. Schedule by image and test compatibility; publish once, analyse many times

The current deferred-wiring frontier is productive:

```text
seal source ─┬─ D tooling repair ─────────────> bounded debug packet (separate gate)
            ├─ X pair/capture host tests ────> future physical packets
            ├─ A audio ownership tests ─────┐
            ├─ V behaviour / logical VP ────┤
            ├─ M MIR / fixture / adapters ──┼─> integration + emit-off evidence
            └─ P scoring / Q1–Q5 prep ───────┘

wiring confirmed later ─> mapping ─> 2-minute music ─> 10-minute stability
instrument available ──> actual waveform/optical work at its own prerequisites
```

No arrow from physical T1 completion to the six host lanes. No arrow from a successful full-register GDB session to unrelated emit-off audio tests. Debug and physical proof remain necessary for the original full T1 stamp, but need not precede every useful intermediate result.

### Group A: current emit-off runtime

Bind the real resident once and reuse its compatible normal-operation capture for audio continuity, AP timing, control readback, logical rendering and memory observations **only where the trace contains the required fields**. No pair-completion/physical-output claims when emit is disabled. Existing known-good tooling may be sufficient; do not wait for every new workflow feature to land.

### Group B: physical pair, when the operator gate is released

Run the mapping fixtures, then the retained 15 s quiet → 60 s real music/control changes → 15 s pause → 30 s resume sequence. Score it immediately; only then run the ten-minute stability segment on the same qualified configuration. Compatible T3/T4/T6 observations can use the same sealed intervals.

Keep the mapping/payload/visible-response axes separate. A single capture can support several claims; it does not automatically satisfy every phase. If waveform conformance needs different patterns or instrument setup, acquire those distinct segments.

### Group C: separate observer-off latency

K1-L uses its exact ≥1,000 joined-frame and observer-off contract with the required physical configuration. Do not merge it into a heavily polled ten-minute control session. Confirm how the existing observer-off measurement obtains its raw generation joins without introducing disallowed traffic.

### Group D: negative and restart tests

Run after normal baseline data is sealed. Each injected failure has separate counters, epochs and acceptance. Restore the approved baseline between incompatible tests. First-fault preservation takes priority over completing the queue.

### Group E: named profiler images

P submits the whole prepared, matched Q1–Q5 campaign. H executes its authorized image transitions together where practical and returns to the compatible runtime afterward. Do not require T1's deferred wiring before a valid emit-off profiler campaign. Do not assume this plan authorizes those image changes; retain the current explicit transition approvals.

### Scheduling priority

Safety/recovery first; then a ready experiment that removes an actual critical-path blocker; then high-evidence-yield normal work on the current image; then the next justified image campaign. Use age to prevent starvation. A ten-minute test is not rejected for exceeding an arbitrary short-slot target. No context switches mid-measurement to satisfy another agent.

## 9. Evidence fan-out without extra target traffic

H captures once through the established owner. Several analysts consume the same **sealed** raw records. Publish immutable closed segments with hashes and a completion manifest; no worker edits or tail-scores an incomplete file as final evidence. The recorder must not block or alter device behavior because an analyst is slow.

A reusable run key includes image/build/source, target UID, boot and audio epoch, backend/profile, configuration revision, fixture, observation load, relevant physical setup, raw record hash and scorer identity. Retain counter denominators, missing samples, rejected joins, replacements/in-flight pairs and interval boundaries.

P scores timing; A scores sample and epoch ownership; V scores features/frames; M scores controls/recording; X scores generation and output records. Each cites the same run rather than initiating a duplicate experiment. Reuse is allowed only when its acquisition conditions and completeness meet that specific criterion.

Scores are derived artifacts linked to immutable raw evidence. A corrected scorer can rescore valid raw data with a new scorer identity; a changed image or missing acquisition field requires the appropriate new measurement. Never repair evidence by editing the original recording.

## 10. Prevent local CPU, disk and tool contention

Nine roles are a starting division of work, not a claim that this Mac or subscription can sustain nine active agents. Verify actual background-task capability, quota and local resources. Reuse existing approved model choices; no silent fallback to an unapproved model/service or new paid budget.

Start with **two heavy host-build/test jobs total**, each with private object, staging, temporary and writable cache directories. This is a proposed resource cap, not a measured optimum. Adjust using observed duration/memory pressure. Local heavy-process count and number of remote reasoning agents are different limits.

Reserve H's host service quality. During a measurement whose timing/capture depends on host service, stop admitting heavy local jobs and let current jobs reach a safe checkpoint before starting the segment. Remote reasoning and analysis of already-local small sealed records may continue if compatible. Do not kill a build halfway merely to make a utilization chart look good.

Never let a worker modify scripts imported by an active broker/programmer; H runs identified frozen tooling for the whole packet. Candidate publication is atomic. Unique run directories prevent two jobs consuming the same reserved output path. Inspect the existing scripts for hidden shared output paths before running them concurrently.

## 11. Repair autonomy and bounded escalation

Within its lane, an agent reproduces a failure, identifies the first evidenced divergence, implements the smallest justified fix, adds a negative/regression test, reruns the affected tests, and hands I a ready patch. It continues its other independent work while awaiting a bench result.

Escalate only the specific dependency: protected file/interface change, conflicting evidence, hardware observation, unavailable prerequisite, authority change or exhausted compute budget. A problem is not delegated back to Captain merely because debugging is difficult.

Use a **20-minute no-new-evidence review trigger**, not a 20-minute stop-work rule or a new acceptance limit. The agent reports the failed hypothesis, unchanged evidence and next discriminating test; C adds a short read-only challenger or redirects the experiment. Do not keep repeating the same flash, GDB attach or retry loop.

For a hardware fault, H preserves the first witness and stops the affected operation. Parallel work on sealed data and unrelated modules continues. A focused challenger may propose an alternate diagnosis, but only the assigned owner implements the chosen fix. No two agents repair the same path competitively.

## 12. Cursor launch and containment

Official Cursor documentation describes background subagents, custom files under `.cursor/agents/`, and explicit parallel delegation. It also warns that subagents share a checkout by default and inherit parent tools, including MCP. Separate context windows therefore do not establish file or device isolation. Documentation was checked for this design; actual installed availability and policy still require inspection.

The pack includes role prompts using documented frontmatter, not a new orchestration program. The coordinator can use native background task invocation and assign private staging paths. Do not assume an `async`, tools allow-list, worktree or concurrency flag exists merely because a plan names it.

Before write-capable fan-out, verify the effective per-role path and tool controls with a harmless scratch-boundary test. A CWD change or an instruction saying “no hardware” is not an OS sandbox. Keep hardware-capable MCP/remote-command access out of workers where supported, deny out-of-lane writes, and ensure no symlink escapes. Do not open the actual device to test the deny policy.

If native workers cannot be safely restricted, retain read-only analysis workers and route their complete patches and isolated test jobs through controlled local executors/I. This still parallelizes reasoning and test processes; report the actual reduced write autonomy instead of pretending isolation exists. Do not loosen a global guard or create unauthorized worktrees to make the swarm launch.

Evidence that parallel launch occurred is returned agent/task handles and overlapping running intervals—not six unchecked to-do items or shell processes that only print role names. Respect installed concurrency limits; keep the full ready queue and start a replacement job as soon as a slot frees. Resume existing lane agents rather than creating duplicate owners.

## 13. Crosswalk: no scope is lost

| Original scope | Work that starts without waiting for physical T1 | Real bench / external dependency |
|---|---|---|
| T1.1 | C/I reconcile and reuse completed baseline evidence | H alone obtains any needed current identity |
| T1.2 | D symbol/identity/tooling repair, offline negatives | Supported controlled debug; original full T1 row remains open until proved |
| T1.3 | X allocation, six tests, mapping preparation | Operator P603/chain-break/power confirmation |
| T1.4 | D/I artifact/tool preflight and image applicability | H, exact transition authority, verified current programming route |
| T1.5 | X/V/M/P fixtures, controls, scoring and acceptance prebinding | H physical mapping → 2-minute sequence → 10-minute segment, correct recovery |
| T2 | X ownership/capture-scorer negatives and first-divergence repair | Same-generation MCU/DIN capture with adequate instruments |
| T3 | A ownership/time/ASRC/numerical tests | Current real input, sample accounting, measured acoustic/input limitations |
| T4 | A/V/M feature implementation, shared fixture, replay and controls | Applicable live call-path, controls and physical/optical observations |
| T5 | V meaningful wide pipeline; X four-lane resource inventory | Second stick, pins/resources/power; actual four-lane and optical tests |
| T6 | P scripts/negatives/campaign preflight; measured-cost patches | Named Q1–Q5 plus actual combined image, distinct traffic conditions |
| T7 | P/A/M fault and latency scorers/recipes | H separated latency, reset, disconnect and fault intervals; optical apparatus separately |
| T8A | M record format, corruption/interruption tests in mock storage | Approved NVM region and authorized write test |
| T8B | A/M source-switch and ingress adapter tests | Actual required ingress hardware and fidelity measurement |
| T8C | M Titan endpoint/protocol and peer emulator tests | Assigned real peer/link; no sibling writes from this lane |
| T9 | P identifies whether a measured need exists | Conditional named derivative/coexistence campaign only; no default accelerator work |
| T10 | P/C comparison and fourteen-question evidence assembly | Applicable required evidence and Captain's architecture ruling |

The original supplied plan is retained unchanged in `sources/`. Its detailed acceptance is still the specification except where this document explicitly supersedes scheduling or identifies newer records requiring reconciliation. Do not reintroduce its obsolete default programmer or hazardous debug sequence.

## 14. Milestones and honest completion

Use the existing status/feature/receipt ledger. The task graph in this pack is a planning appendix, not another live truth database. C records operational state; I alone applies canonical ledger edits. Workers submit their evidence rows rather than editing shared status files.

Operational states such as ready/running/review/awaiting-hardware are scheduling states. Acceptance remains **PASS / FAIL / BLOCKED / NOT TESTED**, with scope and evidence. No “host complete” status can silently close physical or target acceptance.

Report separately:

- **Host work delivered:** merged applicable patches, tests and fixtures, named source cut.
- **Emit-off runtime evaluated:** identified target audio/control/logical results, with physical-output claims explicitly absent.
- **Original T1 accepted:** every original required row, including useful debugging and physical mapping/music/stability/recovery, actually supported.
- **Full lane ready for ruling:** required T2–T8 work addressed at its true scope, conditional T9 resolved, comparable T10 evidence and exclusions explicit.

The first two are useful milestones while wiring is deferred. They are not a renamed T1 PASS or permission to cancel the remaining requirements.

## 15. Planning horizon and throughput checks

These are planning targets under available compute, usable source/tooling and no major newly discovered defect—not measured estimates or completion promises:

| Window from launch | Intended output |
|---|---|
| First 10–15 minutes | Safe handoff, evidence reconciliation, source/isolation manifest, real concurrent lane handles |
| Following 30–60 minutes | First independent fixes/tests and ready experiment packets; I accepts the first useful cut without an all-lane barrier |
| Approximately 1–2 hours | Integrated bounded fixes and emit-off evidence where approved/ready; scorers analysing sealed data during subsequent bench work |
| Approximately 2–4 hours | Further integrated repair/test cycles and explicit remaining external gates, subject to actual defect and run duration |

Do not promise complete optical, four-lane, NVM or peer qualification within this window while their physical or authority prerequisites remain unavailable.

For illustration only: six independent 40-minute host work packages plus 60 minutes of serial bench work take 300 minutes in a pure serial queue. Parallelizing those packages gives 40 + 60 minutes before integration overhead; with an illustrative 20-minute integration allowance, 120 minutes. This is not a forecast for this repository. Actual performance depends on dependencies, review, token limits, compute and the hardware queue.

Track accepted changes/evidence per wall-clock hour, ready-work waiting time, image transitions, invalid experiments, duplicate runs, integration rework and genuine external blockers. Do not optimize agent-count or hardware occupancy as ends in themselves. A healthy board left alone while specialists repair host software is better than a low-value intrusive experiment.

## 16. Dispatch instruction for the current Cursor agent

> Stop treating the phase list as a sequential program. Finish/checkpoint the current operation safely and preserve every existing source/evidence change. Reconcile the newer status, programmer and debug-router records before scheduling another hardware action. Retain the deferred-wiring, no-pair-emit, no-A0-only, explicit image-transition and sibling-repository restrictions.
>
> Become coordinator C. Establish one source/integration owner I and one hardware owner H. Launch D, X, A, V, M and P concurrently using the attached role briefs and isolated staging, within actual runtime limits. Do not wait for T1 completion and do not await each worker before launching the next. Every worker implements and tests its assigned repairs; no competing writers or direct target access.
>
> I integrates ready subsets. H executes only complete, authorized, image-bound packets through the current qualified mechanism and publishes sealed evidence for all analysts. Host work continues during hardware waits; debug and physical blockers stop their dependent rows only. Reuse valid evidence and preserve the first fault.
>
> Return actual task handles, source cut, accepted changes, tested results, hardware occupancy/leave-state and precise remaining gates. Do not return another plan-only checkpoint, fake a swarm with shell placeholders, or label host-only work full T1 acceptance.

## Source register

- Supplied revised plan: `t1_ws2816_runtime_execution_71aaef60.plan(1).md`, preserved byte-for-byte in `sources/`; source hash in the pack manifest.
- Earlier Captain scope and review remain source documents, not new implementation receipts.
- Read-only Mac observations and their limitations: `sources/READ_ONLY_INSPECTION.md`.
- Cursor official subagents documentation, checked for design: `https://cursor.com/docs/subagents`.

**This is a concurrency and dependency correction. It is not a hardware redesign, a new application framework, an authority expansion or a reduced acceptance standard.**
