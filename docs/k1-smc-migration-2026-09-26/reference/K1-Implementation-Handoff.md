# K1 — complete implementation handoff

24 September 2026. Prepared with six specialist research/handoff agents and an independent consistency review. This document combines the final execution instructions; the companion ZIP preserves individual briefs, source evidence, the task DAG, acceptance catalogue, templates and validators.

**13 roles · 56 tasks · 11 specialist briefs · 40 programme-level criteria.** No firmware implementation or deployment was performed in preparing this handoff. All new implementation acceptance remains NOT TESTED.

Use START-HERE and SWARM-DISPATCH to launch the receiving Captain. Detailed specialist acceptance rows supplement, rather than disappear into, the programme-level catalogue. Source-derived commands and unresolved current programming routes are distinguished explicitly.

<!-- Source: START-HERE.md -->

# K1 implementation swarm — final handoff

Prepared 24 September 2026. This is an implementation assignment packet, not an assertion that its firmware has been implemented or installed.

## What the receiving Captain does first

1. Read `K1-Swarm-Execution-Master.md`, then `K1-Integration-and-Deployment-Runbook.md`.
2. Run `python3 tools/check_packet.py .` to verify the packet's structure and sealed bytes. This checks the handoff, not product readiness.
3. Refresh current project instructions, working-tree identities, active task assignments and device ownership through the installed SpectraSynq workflow. Preserve existing task registries; map the task IDs in `execution-manifest.json` into their actual assigned rows without replacing or shortening existing obligations.
4. Launch the ready specialist assignments using `SWARM-DISPATCH.md`. Give each worker its lane brief, the common authority/contract inputs, explicit write scope and task IDs. Use the native agent surface to launch real workers. A scheduler entry is not a launched worker.
5. Freeze the interface bundle and platform thresholds before dependent implementation/measurement. Continue independent tasks while an unrelated lane is blocked.
6. Integrate, deploy and qualify one identified candidate tuple at a time. The independent verifier returns per-criterion results from `acceptance-catalogue.json`. No worker may mark the whole programme complete from its own local checks.

## Main deliverables

| File | Purpose |
|---|---|
| `K1-Swarm-Execution-Master.md` | Authority, ownership, sequence, capability scope and definition of done |
| `K1-Integration-and-Deployment-Runbook.md` | Preflight, repository integration, firmware migration, bench execution, rollback and leave-state |
| `SWARM-DISPATCH.md` | Ready-to-use orchestrator and specialist prompts; concurrency and ownership rules |
| `K1-Swarm-Lane-Map.html` | Offline interactive lane/dependency/acceptance map; every implementation task starts NOT STARTED |
| `execution-manifest.json` | Proposed task DAG, owners, file boundaries, dependencies and per-target scope |
| `acceptance-catalogue.json` | Claim-level methods, proposed/existing/derived thresholds, evidence and gates |
| `lanes/01-…11-….md` | Eleven specialist execution briefs; exact filenames listed in the manifest |
| `contracts/` | Interface freeze checklist and candidate/evidence handoff templates |
| `source-inputs/` | R2 decision, host reference, current captured source and earlier AP/VP research with provenance |
| `tools/check_packet.py` | Offline completeness, dependency and hash checks |

## Settled direction

Nine physical faders address the selected Primary/Top or Secondary/Bottom channel and selected page. The 600×450 capacitive touchscreen exposes navigation, all nine current bindings, context and deeper editing. Keep the requested spelling **Liveiness**. Remove legacy physical Chroma from the default Perform page; retain AP pitch-class chroma. Saturation remains available on Colour/custom pages. Preserve all artistic feature enables independently from amounts. Page changes produce no parameter writes.

The visual engine preserves wide working values and publishes native RGB16 where the endpoint supports it; a byte compatibility adapter remains explicitly separate. Multi-tempo evidence supplements the incumbent canonical clock. Audio observations, visual expression, parameter bases, effective values, physical inputs and emitted output have distinct owners.

## Scope and limits

The current principal delivery is shared contracts/AP/VP plus **DualMCU, Titan and the active K1.Sliders console**. Standalone S3 has a complete separate implementation lane and compatibility obligations; its target admission is a separate result because its current authoritative source entry has not been established in this packet. An unresolved S3 deployment must not stop unrelated DualMCU/Titan work, and must never be reported as a passed S3 implementation.

The earlier modular study's fixed Appearance9/no-bank/no-pickup recommendation is superseded by R2. Other old instructions are not silently waived: refresh current repository restrictions and hardware HOLDs. The captured calibration-wizard prohibition is handled separately from read-only input characterization and user parameter ranges. No generic ban on flashing, arbitrary quota limit or repeated permission request is introduced.

The existing R2 host model has 26 backend and 43 interaction/model checks. Those checks do not qualify BLE delivery, the actual touchscreen, an MCU image or LED appearance. This packet defines that implementation and evaluation work; all new acceptance claims start NOT TESTED.


---

<!-- Source: K1-Swarm-Execution-Master.md -->

# K1 implementation programme — specialist swarm master

## 1. Objective and authority

Deliver a palette-led, channel-first light instrument with independently composable visual features, truthful multi-tempo audio evidence, useful Liveiness across its advertised modes, configurable physical controls, and reliable native output on DualMCU and Titan. A user must be able to perform immediately, discover consequences through contextual displays, and deliberately reach the full supported control depth. Preserve the standalone S3 expansion architecture through the same semantics and an explicitly qualified adapter.

The governing product decisions are the user's current request and `source-inputs/k1-control-r2/K1-Control-Architecture-R2.md`. The executable R2 host backend is design evidence, not the firmware specification by itself. Its two-page/fifteen-semantic subset does not cap the requested product's custom pages or modular feature catalogue. The earlier modular-pipeline evidence remains authoritative for what was observed at its captured sources; its fixed Appearance9/no-bank/no-pickup proposal is superseded. Do not rewrite historical documents to hide that change.

This turn produces handoffs. Execution begins when the receiving agent is assigned the relevant tasks. Apply already-established implementation/flash authorization and actual current HOLDs; the packet neither performs a deployment nor invents a new approval ceremony. Release publication, push/merge to protected product branches and other externally consequential acts retain their real authorization requirements. Routine local edits, inspection and tests should proceed under the assignment.

Captured HEADs and dirty-file manifests identify earlier observations, not today's worktrees or installed images. Preflight refreshes only the evidence needed for the assigned change: instructions, source identities, current task registry, target ownership, build routes and affected contracts. Do not restart the donor teardown or spend the implementation phase rediscovering settled philosophy.

## 2. Separate two dimensions of work

**Concern lanes** own meaning and portable implementation: CTL contracts/control; AP audio evidence; VP rendering primitives; MOD authored mode behavior; TRN transport; INP physical acquisition/takeover; UI display/interaction; IVV independent verification.

**Deployment lanes** adapt and qualify that work: DUAL RT1062 with S3 radio; TIT RA8P1; S3 standalone; INT coordinated repository integration and active-console assembly. ORCH coordinates assignments, dependencies and closeout. A platform adapter must not quietly invent a different meaning for a portable parameter.

| Owner | Exclusive responsibility | Must not absorb |
|---|---|---|
| ORCH | Scope, assignments, task/evidence accounting, conflict resolution | All feature coding or self-certifying the product |
| CTL | Semantic catalogue, versioning, bounded profile compiler, authoritative base transactions, interface bundle | ADC acquisition, page presentation or radio scheduling |
| AP | Incumbent-preserving tempo sidecar, candidate identity/age/phase, audio fixtures | Palette animation or UI requests that rewrite observed time |
| VP | Wide numeric domains, motion/history, compositing, feature lifecycle and endpoint conversion | Per-mode artistic taste, hardware driver ownership |
| MOD | Mode adapters, Liveiness, parameter merit, useful declared mode coverage | Hidden AP changes or resetting unrelated parameter bases |
| TRN | Encoding, negotiation, async state replication, bounded delivery/recovery | Implicit slot-to-parameter routing or redefining values |
| INP | Measured input qualification, gestures, pickup, relative controls where explicit | Engine modulation, persistent storage policy or display layout |
| UI | Channel/page hierarchy, lenses, advanced editor, truthful applied-state presentation | Second smoothing owner or uncontrolled shader/firmware preview claims |
| DUAL | RT1062/S3 target integration, clocks, memory, native driver and loaded qualification | Titan reference/import decisions |
| TIT | Titan selective configuration, owned reference import, audio/output boundaries | Copying today's DualMCU source as an automatic new reference |
| S3 | Bounded standalone execution and explicit engine-add transition | Simultaneous competing AP/VP owners |
| INT | Shared existing call sites, console assembly, coherent multi-repo candidate tuple | Rewriting specialist behavior without its owner's review |
| IVV | Independent fixture expectations, acceptance evidence and release assessment | Fixing a candidate and then acting as its only reviewer |

## 3. Repository and writer boundaries

Actual roots already identified:

- `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware`
- `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`
- `/Users/spectrasynq/Workspace_Management/Software/K1.Sliders`
- `/Users/spectrasynq/Workspace_Management/Software/K1-HMI-BLE-MIDI` — donor/reference, not the active console destination.

Standalone S3's authoritative firmware entry must be resolved from the current modular project; do not infer it from an old bench tree. EasyEDA/product hardware is not a destination for these firmware changes.

Use one writer per file and one owner per hardware resource. The manifest assigns the main seams; preflight expands them into an exact path ownership record before edits. New portable modules may be authored concurrently in approved isolated locations. Existing cross-cutting files are integration-owned: specialists submit narrowly scoped patches with preimage hashes and tests instead of racing to edit the same renderer, runtime or entry point.

The captured DualMCU instructions restrict sibling worktrees and concurrent feature integration. Do not issue generic `git worktree add` commands or create duplicate project folders by habit. Refresh the applicable rules; use the allowed feature/reference mechanism, external handoff patch artifacts and read-only specialist tasks where simultaneous source authorship is restricted. This still provides parallel analysis, contract design, fixtures, independent verification and work in different permitted repositories. Serialize the repository's guarded integration boundary. A restriction in one repository does not disable every other lane.

The existing console contract generator can write into two repositories. CTL first removes implicit cross-repository output in favour of an explicit staged export. INT and consumer owners import that sealed bundle. No worker may run the old generator opportunistically, relax the v1 exact-count/hash checks to make R2 pass, or overwrite another worktree's generated header.

## 4. Required contract freeze

G1 is a real dependency: platform implementations cannot safely diverge in value meaning, state ownership or epochs. It is not a demand to freeze the whole future product before writing any code.

CTL assembles one reviewed bundle containing versioned parameter/feature descriptors, units/ranges/defaults, enable/bypass/reset rules, base/effective ownership, profile/binding tables, transaction/receipt/snapshot semantics, capability limits, AP/VP publication types, visual-context metadata and v1 coexistence. AP, VP, INP, TRN, UI and platform owners review only their affected boundaries. Each review names an actual uncertainty or states that its consumer can be implemented; silence is not sign-off.

Production numeric IDs are allocated from the current registry only at that freeze. The packet's task IDs and symbolic interface names are not MIDI/controller addresses. Generate producer and consumer expectations from the same source, but independently test serialization and identity; matching two generated bugs is not interoperability evidence.

Capacities are fixed per platform profile, not one guessed global allocation: descriptors, custom pages, bindings, command queues, transaction fragments, simultaneous phase tracks, layers, histories and transition overlap. Freeze the measured/derived budget and failure behavior before enabling a graph. A unsupported capability is reported, not silently reduced to a different effect. Keep parsing, compilation, persistence and UI work outside the realtime AP/VP paths.

## 5. Programme sequence and concurrency

The task DAG in `execution-manifest.json` is the dispatch source. An earlier task can finish before its numbered wave if its actual dependencies are satisfied. Conversely, elapsed time does not release a gate.

**Wave 0 — baseline and interface proposals.** ORCH establishes ownership. CTL, AP, VP, INP, UI/platform discovery and IVV can inspect/draft independently. Rotate specialists through the six worker slots; no one is required to wait for a full serial audit. Capture current build routes, installed images, golden fixtures, timing definitions and source-policy exceptions. No target reset is required for repository discovery.

**Wave 1 — G1 contract bundle.** CTL resolves boundary conflicts with affected owners. Freeze a small first delivery containing the control model, genuine wide profile, modular feature lifecycle and multi-tempo publication. Keep extensible descriptors for later features without requiring every donor feature to be adopted.

**Wave 2 — portable implementation.** Run AP, VP, CTL/TRN, INP, UI and IVV in parallel where ownership permits. MOD begins its semantics work against frozen interfaces, then mode implementations consume the proved VP primitives. The integrator alone wires shared call sites. Workers return source-bound changes plus actual test results, not prose assurances.

**Wave 3 — platform integration.** DUAL, TIT and S3 adapters consume the same interface bundle and independently qualified portable inputs. INT assembles the active console. Build and analyze separate targets concurrently. Target mutation is serialized only across genuinely shared hardware/fixtures; independent identified benches can run concurrently after their resource sets are disjoint.

**Wave 4 — loaded target and system acceptance.** Run the physical tests with all relevant audio, radio, display, fader and LED load. Verify feedback from engine application, not merely a BLE ACK. Test staggered versions and rollback before making the new console profile the normal instrument. IVV assesses each candidate tuple, not just the portable library.

**Wave 5 — functional and artistic acceptance.** Evaluate every advertised Liveiness adapter, material controls, context usefulness, precision and quiet-space behavior on the appropriate physical setup. The user's aesthetic preference is a recorded product decision; numeric accuracy alone cannot substitute for it. Publish separate DualMCU, Titan/console and standalone outcomes. A blocked standalone deployment does not falsely fail a completed DualMCU trial or falsely pass the complete standalone product.

## 6. Function acceptance, not just function presence

Each advertised function needs six linked entries: semantic definition; required capability; implementation owner; deterministic fixture; integrated user action; acceptance evidence. A table row or slider label is not an implemented function.

For Liveiness: identity at neutral; a declared useful effect over its range; no AP timestamp/BPM manipulation; base Motion/Width/Persistence unchanged; per-mode bounds; persistent adapter version; physical comparison. Every mode advertised in the R2 catalogue must pass. During integration, unsupported legacy modes may be honestly labelled; that is an intermediate state, not completion of the universal mode promise.

For feature toggles: enable is independent of amount; bypass has an exact declared output behavior; retained state has a declared freeze/clear/decay policy; re-enable cannot expose unexplained stale light; one feature cannot reset another owner. Test graph-adjacent interactions and worst-cost admitted graphs, not every meaningless Cartesian combination.

For granular controls: changing user ranges or response does not rewrite sensor calibration; stored presets use canonical physical units; mapping is applied exactly once; invalid edits are atomic; out-of-range existing values are displayed without opening a page changing them; final targets survive coalescing and reconnect rules. Absolute fader acceleration is not improvised—relative accelerated gestures require an explicitly different interaction contract.

For context displays: the lens describes the actual selected semantic and its downstream influence; physical/requested/accepted/effective are distinguishable; a colour wheel is labelled as an illustrative model unless calibrated; raw telemetry/noise does not hold a popup open; navigation and timeout never remap controls or change sound/light state.

For multi-tempo: preserve the incumbent clock; identify candidate evidence and its age; distinguish observed/derived/coasting; phase belongs to a media-clock anchor, not channel render count; silence and stale observations cannot fabricate confident beats.

## 7. Metrics and threshold discipline

The acceptance catalogue distinguishes **existing**, **derived** and **proposed** requirements. Existing active gates remain in force. Derived limits use the actual candidate geometry, sample/hop configuration and protocol. Proposed limits become normative at the pre-measurement freeze; if infeasible, resolve them before collecting the candidate's qualification results and preserve the change rationale.

Record minimum/p50/p95/p99/maximum where meaningful, duration/count and failed samples. Zero loss means a nonzero run with explicit loss counters and complete coverage, not missing fields. A percentile cannot excuse a missed hard audio deadline. An earlier 12.8 kHz/96-sample success does not qualify a 24 kHz image merely because both hop durations are 7.5 ms.

Latency stages must be named: physical qualification → command send → engine accept → effective response → render → transfer start → completion/latch. Intentional response damping is measured separately from transport delay. Capture clock relation or uncertainty before subtracting timestamps across devices. Existing ambiguously named gates retain their original endpoint definition once resolved; do not claim an optical latency from API-return timestamps.

Optical comparison uses the same identified bar/optics/scene/transfer at matched cadence first. Direct native16, rounded RGB8-expanded and dithered RGB8-expanded are evaluation adapters, not three different physical products. Sixteen-bit commands do not establish sixteen effective optical bits. Report human preference separately from measured transfer/temporal behavior.

## 8. Gate meanings and definition of done

| Gate | Required proof | Permitted next action |
|---|---|---|
| G0 baseline | Current instructions/source/target identities and ownership; live commands resolved for affected operation | Assigned bounded source work; identified bench only when its prerequisites are met |
| G1 contracts | Reviewed versioned bundle, units/lifecycle/capacity/error semantics and frozen thresholds | Independent producer/consumer implementation |
| G2 host | Required deterministic/compatibility/fault fixtures on exact changes; builds and static memory evidence | Assemble identified platform candidate; hardware trials through actual lane procedure |
| G3 lane hardware | Correct installed image, load/loss/deadline/output/input gates, real display and recovery evidence | Integrated candidate trial on that qualified platform |
| G4 system | Console–radio–engine applied-state behavior, mode/toggle interactions, cross-version recovery and leave-state | Product evaluation of that exact configuration |
| G5 product | All advertised functions, usability/appearance review, unresolved limitations disclosed and adoption recorded | The named target/profile is accepted; publication follows actual authorization |

“Code done” means implementation and local tests exist. “Lane done” means its mandatory claims have their required evidence. “Integrated done” means the complete source/image/profile/target tuple passed G4. “Product done” means G5, with every advertised function admitted. The full principal programme requires DualMCU, Titan and active console results; standalone readiness is separately named. Do not compress these into a single green checkbox.

There are no automatic promotions based on elapsed work, agent confidence, branch merge, number of tests or a global exit code. A failed or untested required criterion stays visible. N/A is allowed only with a concrete scope reason approved before release; it cannot remove a promised function after failure.

## 9. Closeout and maintenance

Each worker returns the template in `contracts/lane-result-template.json`, exact revisions/diff hashes, affected interfaces, commands and raw results, per-criterion disposition, source/target provenance, resource release/leave-state and the next owner/action. INT creates a candidate manifest tying all repositories, generated bundles, image hashes and profiles together. IVV independently checks evidence applicability and reports what it proves.

If a shared interface changes after G1, the owning specialist issues a versioned change with named affected consumers and fixture deltas. ORCH pauses only those consumers, re-freezes the changed boundary and reruns only invalidated proof. No agent silently refreshes a golden fixture to erase a failure. Preserve rejected candidates and failed receipts as history.

The final delivery must include reproducible build routes, deployment/rollback tuple, capabilities, calibrated-input policy, mode coverage, memory/timing results, optical/interaction setup, known limitations and exact last running state. A new agent should be able to resume from that package without reconstructing this conversation.


---

<!-- Source: SWARM-DISPATCH.md -->

# Dispatching the K1 specialist swarm

## Operating model

Use one coordinating Captain and up to six active specialist workers for the seven-session surface available during this study. This is a concurrency plan, not a quota, model or spend restriction. Adapt it to the executing surface's actual capacity. Preserve any explicitly requested model; use installed routing advice without pretending it changes the selected model automatically.

Roles are stable; workers may be resumed or replaced. No single worker should implement the whole stack. ORCH maintains context and dependencies, INT assembles reviewed changes, and IVV independently evaluates them. A feature author does not certify the complete product. A domain expert may complete several related tasks within its own lane; that does not collapse all roles into one agent.

The authoritative active task status remains in the existing project registries/workflow. `execution-manifest.json` is this packet's proposed DAG and task-definition import, not a replacement live governance service. Map each task to the current registry while preserving existing IDs/history/dependencies. All packet tasks start NOT_STARTED; the HTML shows planning relationships, not a live execution result.

## Initial dispatch and slot rotation

1. ORCH completes ORCH-0: current scope/instructions, permitted isolation, source/resource ownership and registry bindings. Read-only specialists can prepare proposals while ORCH resolves an unrelated target identity, but cannot perform the blocked write.
2. Launch CTL-1, AP-1, VP-1, INP-1 and IVV-1; rotate the sixth and newly freed slots through TRN-0, UI-1, INT-0 and source/capacity discovery DUAL-1/TIT-1/S3-1. Preserve required specialist separation—do not give the whole programme to the first available worker.
3. CTL-2 receives all boundary proposals and IVV's threshold definitions. CTL-2 consumes TRN negotiation/payload and DUAL/TIT/console capacity proposals. Exact programming routes/bench identity are separate DEPLOY0 tasks, so an unavailable bench does not prevent source/contract work. Platform runtime fit is proved at admission; a missing S3 source root does not stop the shared freeze.
4. Dispatch ready implementation tasks from the DAG: AP, VP, CTL/TRN, INP and UI. Keep IVV available for independent expectations/review. Rotate CTL from schema ownership to control tests rather than having it write the radio, UI and renderer too.
5. After MOD-1, dispatch MOD-SLICE for the first useful mode and MOD-2 for family coverage. If more capacity exists, split MOD-2 using the four family assignments in its brief; the parent owns the common table and each child gets separate adapter files. Do not let all children edit `product_effect_renderer.cpp`.
6. INT-SLICE integrates AP-A, one authored wide mode and reliable canonical control. IVV-SLICE independently checks it, then DUAL-SMOKE and TIT-SMOKE supply early target feedback while AP-B phase tracks and the complete mode catalogue continue. Console assembly INT-2 independently consumes CTL/TRN/INP/UI; it does not wait for audio/render completion. These are integration checkpoints, not product completion.
7. INT-1 assembles the complete portable candidate after required host proof. DUAL/TIT/S3 adapters and the independent console assembly can build concurrently. IVV-2 reviews the integrated host result independently.
8. Run actual bench tasks under the resource rules. The final DUAL/TIT/standalone system and appearance results remain separate. ORCH-CLOSE requires the principal DualMCU/Titan/console results and explicitly reports standalone status.

## Ready-task selection

A task is ready when every listed dependency's *specific exit artifact* has passed its gate and its source/interface identity still applies. A task can be worked on through mocks before that point only as a clearly provisional draft; it cannot claim integrated acceptance against an unfrozen guessed contract.

Prefer ready tasks on the critical path, then independent verification and bounded discovery that unlocks another worker. Do not keep six workers busy by inventing optional cleanup. When a worker finishes, collect its structured result, release ownership and dispatch the next ready task. A blocked worker returns the exact blocker and resumption condition; ORCH continues ready work elsewhere.

For a capacity-limited schedule, temporarily park finished AP/VP authors during platform qualification and use their slots for target specialists and IVV. The coordinator remains active. Do not substitute a long coordinator wait for checking a newly delivered integration concern.

## Orchestrator launch prompt

Copy this prompt into the receiving Captain's session and supply the packet directory:

> Execute the K1 specialist programme from this packet. Read START-HERE.md, K1-Swarm-Execution-Master.md, the deployment runbook and current repository/workflow instructions. Bind the actual assigned tasks; preserve existing registries and user work. Use the 13 roles and 56-task DAG in execution-manifest.json, launching real specialist subagents for ready tasks. Do not personally implement every concern. Establish exact file and physical-resource ownership before writes; observe repository restrictions on worktrees and feature integration. Freeze the shared interface bundle before dependent implementation. Integrate the early complete path before waiting for every mode, then qualify the full candidate. Preserve R2 channel-first pages, Liveiness semantics, feature enables, native16 and incumbent audio timing. Record actual results per lane and target; code/model success never substitutes for installed hardware or product acceptance. Continue authorized ready work until the assigned scope is complete or genuinely externally blocked. Return the candidate tuple, criteria/evidence, deployment/leave-state, limitations and exact next actions. Never invent a no-flash or quota gate, bypass a real gate, or report an untested function as done.

The numeric task count is a packet identity check, not an acceptance metric; the manifest is authoritative if a later reviewed revision changes it.

## Specialist launch envelope

Use one envelope per worker; fill all bracketed fields from the actual assignment. Do not send a vague “work on firmware” request.

> Role: [ROLE]. Assigned tasks: [EXACT TASK IDs]. Repository/source baseline: [PATH, HEAD plus affected dirty-file hashes]. Contract bundle: [VERSION/HASH or proposal stage]. Read [YOUR LANE BRIEF], the master common constraints, applicable current AGENTS/workflow instructions and the named source inputs. Why this matters: [SPECIFIC FAILURE OR CAPABILITY]. You own [EXACT FILE SET]; [SHARED FILES] belong to [OWNER]. Allowed operations: [READ/HOST EDIT/BUILD/TARGET TASK as actually assigned]. Dependencies: [EXIT ARTIFACTS AND GATES]. Implement the bounded work, run the prescribed meaningful checks and preserve unrelated work. Propose shared-interface changes to CTL instead of silently changing a consumer. Do not run implicit cross-repository generators or operate another lane's hardware. Return lane-result-template.json with source-bound changes, commands, evidence, per-criterion results, interface deltas and next action. Mark unsupported/untested explicitly. If blocked, stop only the affected operation, preserve its evidence and explain the exact resumption condition; complete independent assigned work.

## Specialist continuation and handoff

On resumption, compare actual source/contract identities with the prior result before continuing. Reuse valid prior evidence; do not rerun broad tests because context was compacted. Revalidate only affected claims after a change. Incoming new user instructions steer the active task and supersede conflicting historical product choices.

Every handoff includes: task IDs; owned paths; preimage and postimage/source hashes; applied contract version; semantic changes; tests actually run and their actual exit; referenced acceptance rows; unresolved results; any outstanding process/hardware lease; next consumer and exact import action. Worker prose without these artifacts does not admit a merge.

## Conflict and failure routing

| Conflict | Owner and action | Work that continues |
|---|---|---|
| Two workers need the same file | ORCH names one writer; other supplies a preimage-bound patch/review | Disjoint files and fixtures |
| Parameter/schema meaning differs | CTL plus affected semantic owner resolve/version the boundary | Unaffected modules and current frozen contract tests |
| Source/reference drift | Repository/platform owner compares actual changed surface | Other repositories; no forced reset |
| Hardware lease or identity uncertain | Target owner preserves state and resolves identity/process ownership | Host build/model/independent review |
| Target deadline or loss gate fails | Platform owner with AP/VP cost owner bounds/repairs the actual bottleneck | Other target acceptance; no blanket rollback |
| Appearance/usefulness fails | MOD/VP/UI owners revise the specific behavior using recorded feedback | Numerically valid independent features |
| Reviewer also authored fix | Assign a different qualified reviewer for affected acceptance | Author may debug, but cannot be sole independent approver |

## No hidden serial bottleneck

Single-writer integration protects source consistency; it does not mean one agent designs every feature. Review requests are bounded to changed interfaces. Freeze a minimal initial complete path and extend it through reviewed versioned modules. Treat the contract generator, shared renderer call sites and shared bench as scarce resources; portable algorithms, independent fixtures, UI design, mode families and target builds are parallel work.

A feature branch merge is not a finish line. The exact complete candidate must survive the acceptance and deployment runbook before its advertised functions are accepted.


---

<!-- Source: K1-Integration-and-Deployment-Runbook.md -->

# K1 integration and deployment runbook

This runbook binds source work, installed images and observed behavior. It is used with the specialist briefs, not instead of them. No deployment was performed while creating this packet.

## A. Preflight the actual assignment

The integration owner records one candidate ID and target scope (`dual`, `titan`, `console`, `standalone`). Read current repository instructions and active task ownership. Inspect the installed SpectraSynq workflow and bind only this assignment's real task IDs in the existing registry. Its captured integration guide is included under `source-inputs/k1-pipeline-integration/source-snapshot/governance/docs/AGENT-INTEGRATION.md`; refresh it before execution because the local installation may have advanced.

The following are source-inspection command templates using confirmed repository roots. They are ordinary read-only Git operations; replace the root with the actual assigned repository. They do not prove installed-image identity.

```sh
repo=/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware
git -C "$repo" status --short
git -C "$repo" rev-parse HEAD
git -C "$repo" diff --stat
git -C "$repo" diff --binary
```

Record relevant untracked files and their hashes, not just HEAD. Do not paste secrets or unrelated private diffs into agent prompts. Freeze the affected source preimages and preserve unrelated dirty work. Inspect existing reference/import manifests before touching generated or derivative sources. Resolve whether the current repository permits isolated worktrees; do not create one if its rules prohibit it.

Use the installed workflow's returned session key and actual registry path. Captured CLI forms below are **templates**, not claims that a task registry or native session already exists:

```sh
/Users/spectrasynq/.local/bin/agentctl integration context --project "$repo"
/Users/spectrasynq/.local/bin/agentctl integration route --project "$repo" --task 'The assigned bounded task'
/Users/spectrasynq/.local/bin/agentctl integration bind --project "$repo" --registry ACTUAL_REGISTRY --session RETURNED_SESSION --tasks ASSIGNED_TASK_ID
/Users/spectrasynq/.local/bin/agentctl integration verify --project "$repo" --session RETURNED_SESSION --label 'Exact required check' -- VERIFIED_EXECUTABLE VERIFIED_ARGUMENTS
/Users/spectrasynq/.local/bin/agentctl integration gate --session RETURNED_SESSION
```

Do not invent a registry to make the gate green, assume a scheduler launches native workers, or add a quota veto. If current workflow tooling is unavailable, record the operational failure and obey the actual applicable fallback; continue independent permitted design/check work. Never forge its receipt or bypass a required admission gate.

## B. Resolve executable routes once, before the relevant write

Each platform owner fills `contracts/deployment-command-manifest.template.json`. A row includes repository, source/script authority, working directory, **argv array**, tool/version, target selection, environment inputs, source/config/image hashes, expected exit and postcondition, programming method, readback command and recovery route. Unresolved rows remain null; they cannot become copied prose such as “flash latest.”

Resolve the route by reading the current approved repository script, build configuration and help output. Use existing project wrappers and target selectors. Capture the full successful build argv and artifact identity. Capture the exact approved programming argv only after target/lease/HOLD checks. Never infer a serial port, substitute a raw vendor programmer for a guarded wrapper, or copy another board's pins/sample rate. Source-derived command excerpts in the platform briefs are evidence of the earlier route, not a waiver of current checks.

The handoff can be host-ready while a programming route is unresolved. That route blocks only its dependent hardware mutation. A successful build command or programmer exit does not substitute for target readback and behavior.

## C. Integrate portable changes without collisions

1. CTL freezes the contract bundle, capacities, compatibility policy and generated checksums. Specialists review their boundaries. The existing generator must stage outputs explicitly; inspect the generated destination list before running it.
2. AP/VP/MOD/CTL/TRN/INP/UI return narrow change packages with preimage hashes, exact interface version and relevant checks. Preserve distinct commits or patch boundaries so failures can be bisected by concern.
3. INT reviews file ownership and applies only matching preimages into the permitted candidate location. A mismatch triggers a three-way review with that owner, not a force apply, overwrite or reset. Resolve shared-call-site changes once.
4. Consumer repository owners import the same sealed generated bundle. Record its digest in every build. Re-run generation in a staging directory and compare byte-for-byte; compilation alone does not demonstrate the bundle matches the schema.
5. Compile host references and each selected target. Record warnings, link map, static allocations, stack/queue bounds and runtime allocation policy. “Build successful” does not admit a feature graph whose bounded capacity cannot fit.
6. Run compatibility and deterministic/fault fixtures required by the changed boundaries. Use an independent encoding/reference oracle for critical wire/native-word checks. Preserve old expected output unless an explicitly new profile is selected.
7. Freeze the candidate tuple: per-repository source identity including dirty patches, contract/profile/capability hashes, AP configuration, mode-adapter versions, build flags/toolchain, image hashes and target hardware identities. Any relevant change creates a new candidate revision and invalidates only affected evidence.

## D. Bench ownership and baseline preservation

The bench resource set includes physical MCU identities, debug/programming adapters, serial/USB interfaces, LED fixture, audio source/capture path and any shared power or measurement tool. Grant one owner for every overlapping set. Host builds and unrelated benches continue in parallel. Release occurs only after the owner reports source/image/physical leave-state; a timeout is not proof that a programmer released the target.

Before reset or programming, record the running firmware identity, readable configuration/profile, output-enabled state, clock/sample/hop/output topology, actual physical mapping and recovery artifact. If installed image identity cannot be read, record the gap and obtain it through the permitted lane method rather than treating HEAD as firmware identity.

Preserve an exact known recoverable image/configuration tuple where the platform supports it. Preserve user presets and sensor calibration separately from binding profiles. Do not claim rollback if the artifact or the approved recovery route is unavailable. Current output-hold/drain/black-pair procedures and fault authorities remain in force; observe completion/latched state rather than merely submission. A currently active HOLD applies only within its scope and must have an actual witness.

## E. Upgrade order and mixed-version behavior

The safety principle is to deploy consumers that can retain the old contract before enabling a producer of the new contract. Numeric version labels alone are not enough: verify the actual compatibility intersection.

### DualMCU plus active console

1. Install/qualify an RT1062 candidate capable of the incumbent path plus the staged R2 path, with new profile/features disabled until the tuple is ready. Old console/radio behavior must still pass. If the current boot/transport arrangement requires a coupled engine/radio update, follow its atomic procedure and record that exception rather than pretending an unsupported mixed pair is safe.
2. Install/qualify the S3 radio/bridge candidate that preserves v1 and carries the new versioned envelope. It routes control and state; it does not assume AP/VP authority. Exercise malformed, delayed, duplicated and unsupported traffic against both old and new accepted endpoints.
3. Install/qualify the console candidate after its full static/host tests. On connect it negotiates capabilities, obtains an authoritative snapshot and re-arms physical takeover. It must not transmit nine stationary positions as a baseline.
4. Only after all identities/hashes agree, select the R2 instrument/profile and admitted graph. Read back actual applied state. Enable one-sided controls, then simultaneous inputs and full combined load.
5. Validate final settled targets, applied/effective truth, true black and reconnect. The product tuple includes both images, console image, schemas/profile and physical topology.

### Titan plus active console/host adapter

1. Preserve Titan's current imported portable reference and its approved derivation/update process. Qualify selective configuration before routing frequent fader edits into it; an A-only edit must not reconstruct B or disturb shared audio/presence state.
2. Build/program through the current permitted Titan route with exact capture and output configuration. A debug connection success does not qualify an image; a diagnostic PDM capture does not qualify the production AP. See TIT brief for captured route restrictions.
3. Verify the native GPT/DMA publication and complete-output boundary before enabling new producers. Apply configuration atomically with precise reset masks.
4. Bind the versioned semantic adapter actually implemented for Titan. A host adapter is a legitimate intermediate test path but is not proof of console/BLE integration. If the radio route is absent, mark that required system claim BLOCKED and build the route; do not relabel serial control as BLE acceptance.
5. Import/activate the same instrument profile only after capability/snapshot round-trip passes. Run the same functional scenarios with Titan-specific timing and physical mapping.

### Standalone S3 and engine addition

Resolve the authoritative standalone source/build first. Use the mainboard's actual hardware manifest, not an old bench pin map. Admit a bounded graph and maintain the shared control meanings. When an engine is attached, exactly one owner must acquire AP/VP/output authority through a defined transition; the S3 relinquishes those functions only after the new owner and state transfer are accepted. Test missing/late engine, disconnect and fallback without competing drivers or silent control changes. This is a separately reported platform release, not inherited from the DualMCU radio build.

## F. Required execution sequence per candidate

| Step | Conditions and operation | Stop/result |
|---|---|---|
| Identity | Source/image/schema/profile/target/fixture tuple captured | Unknown target or mismatched bundle stops affected writes |
| Baseline | Reproduce relevant incumbent fixtures and declared loaded gates | Existing failure retained; do not blame or credit new candidate without comparison |
| Host/contract | Run affected numeric, lifecycle, migration, race and transport fault fixtures | Required failure prevents target admission; unaffected work continues |
| Build/program | Approved route, lease and recoverable tuple | Real exit/readback recorded; partial operation investigated before retry |
| Sparse output | Verify actual mapped pixels, channel/half order, native words and completed black state | Wrong mapping/fault cannot restore show |
| Single function | AP sidecar, one wide profile, one control/lens, each enable/bypass | No unsupported label promoted to functional completion |
| Pairwise boundary | A/B isolation, palette/history, Liveiness/base values, page/gesture, retry/epoch, popup/editor | Each declared noninterference property holds |
| Load | Audio + radio + UI + nine faders + worst admitted visual graph | Existing lane loss/deadline/run-duration gates pass; no missing counters |
| Recovery | Disconnect/reconnect, wrong version, duplicate/reorder, reset/epoch and failed transaction | No old intent replay, explicit terminal outcome, actual state converges |
| Product function | Full advertised mode matrix, controls and visual/interaction trials | Metrics and recorded human judgement remain separate |
| Closeout | Candidate evidence checked, profile/leave-state read back, lease released | PASS/FAIL/BLOCKED/NOT TESTED per claim, exact next action |

At least the following user scenarios must be exercised: cold start; dim quiet listening; high-energy performance with rapid faders; channel/page switch mid-movement; revisiting after presets changed externally; invalid advanced edit; multi-fader palette/motion operation; unexpected radio drop; fault blackout; deliberate restore. UI labels and behavior must make each outcome understandable without a hidden gesture.

## G. Timing and physical evidence

Measure the candidate's actual sample rate, hop, output schedule and transfers. Use support/availability timestamps for audio evidence; separate input acquisition, command transit, engine acceptance and deliberate response dynamics. Use completed output/latch evidence for presentation. If no instrument can substantiate the latch/optical claim, mark that claim NOT TESTED; keep valid software timing evidence under its proper name.

The retained 80-pixel native48 arithmetic is 4.8 ms payload at 800 kbit/s plus the captured 300 µs interval: about 5.1 ms before other overhead. It is a topology-specific bound, not a new product frame-rate requirement, and cannot support an unqualified 400 Hz promise. Recompute after identifying the actual candidate's parallel outputs and physical population.

Preserve active run durations and recovery counts from current lane instructions. New proposed thresholds in the catalogue are frozen before qualification, with full test conditions and a resource budget. Never replace a required 30-minute run with a convenient short smoke test. A smoke test can release a debugging step, not the full gate.

## H. Rollback and interruption

Disable new producers before downgrading consumers. Put output into the platform's verified hold/black/drain state, preserve the failed evidence, then restore the compatible console/profile and bridge/engine tuple through approved routes. If forward migration wrote persistent data, use its versioned reverse migration or preserved profile backup; do not reinterpret new bytes through an old schema. Reconnect with a new session epoch and snapshot; stationary faders remain non-writing.

Rollback order may differ for an inseparable firmware pair; its approved coupled procedure takes precedence. Record actual readback and resumed movement after restore. A successful programming command alone is not successful rollback. Do not automatically retry an uncertain flash, reset a shared board or unplug another lane's fixture.

Cancellation leaves a checkpoint identifying the current operation, whether a process remains active, known image/configuration, output state, reserved resources and safe next command. Do not call an interrupted run complete or silently release an uncertain lease.

## I. Final deployment receipt

Use `contracts/candidate-manifest.template.json` and lane result templates, attaching required evidence rather than empty placeholders. The closeout must name:

- exact source/bundle/image/profile/adapter versions and hardware/fixture identities;
- actual build/program/readback routes and leave-state;
- per-criterion result, raw artifacts, hashes and time/configuration coverage;
- known limitations and capabilities deliberately absent;
- proposed vs adopted visual behavior, including who reviewed it;
- the next owner, prerequisite and concrete action, or an explicit accepted terminal state.

Do not push, merge a protected product branch or publish a release solely because this document exists. Honor the user's existing authorization and the actual repository/release boundary for that action. Local implementation and tests need no repeated permission request where already assigned.


---

<!-- Source: contracts/INTERFACE-FREEZE.md -->

# Shared interface freeze checklist

CTL owns the bundle and export procedure. Domain owners own meaning. These are proposed interface names, not pre-existing production APIs or allocated wire IDs.

| Object | Defining owner | Required content and consumer contract |
|---|---|---|
| ParameterDescriptor | CTL with MOD/VP/AP | Stable semantic/version, type, units, legal engine range, default, scope, response owner, capability/inactive reason; distinguish legacy aliases |
| FeatureDescriptor | VP/AP with CTL | Enable independent of amount, inputs/outputs, dependency/order, state bytes, init/bypass/re-enable/reset causes and capacity |
| BindingProfile | CTL with INP/UI | Nine physical IDs per page, channel policy, custom page identities, narrowed ranges, mapping/steps/takeover/response, atomic revision |
| CompiledProfile | CTL | Bounded validated tables outside realtime; exact capacity and export identity; invalid draft leaves active version untouched |
| GestureIntent | INP with CTL/TRN | Captured canonical target/channel, session epoch, sequence, binding/profile revision, absolute/relative mode, final flag |
| ControlTransaction | CTL | Atomic parameter changes, expected base revision, capability/schema, idempotency identity, rejection semantics |
| ApplyReceipt | CTL/TRN | Accepted/rejected/superseded/expired outcome, target, base revision and actual accepted value; transport ACK separate |
| StateSnapshot | CTL/TRN | Complete base state/capabilities/profile identity, consistent revision, bounded fragment reassembly and checksum; no partial activation |
| EffectiveObservation | VP/AP/CTL | Effective value plus base/source revision, modulation context and timestamp/age; bounded telemetry cannot overtake control |
| TempoFieldV1 | AP | Observation support/availability/media clock mapping, stream epoch, candidate identity/salience/age, validity/activity, phase anchor and running frequency |
| WideFrame / module state | VP | Explicit colour domain, channel/geometry, timestamp, finite/overflow policy, ownership/publication lifecycle |
| VisualContextDescriptor | UI with semantic owners | Relevant domain/units, influence relationships, supported illustrative vs measured preview, before/after basis and pending/accepted states |
| ControlCapabilities | Platform owners with CTL | Versions, semantic IDs, admitted pages/layers/objects/phase tracks/buffers, supported mode adapters, transport bounds; rejection instead of silent substitution |

## Resolve these ambiguities before G1

- Palette travel in R2 is **excursion depth**, not a speed slider. Palette position, span, transition duration and any separate travel-rate expert parameter must have distinct units and meanings.
- Liveiness has a neutral identity, versioned mode adapter and bounded modulation; independent base values survive. Observed AP timestamps/tempo remain read-only to VP/HMI.
- Physical CHROMA, internal legacy colour fields and AP pitch-class chroma are distinct. Colour-science chroma in a dashboard is yet another declared model coordinate.
- Channel intensity, legacy Photons/floor, artistic density, global master and electrical/current limits are distinct authorities.
- Sensor qualification, mapping curve, output quantization, transport hysteresis and engine response dynamics are separate stages. Define each exactly once and select one response owner.
- Body geometry remains 160 logical pixels/channel, centre pair79/80; actual physical bench population and parallel half outputs are separate facts.
- RGB16 means three16-bit components; working FP32 is not automatically calibrated linear light. Gamma/device mapping has one declared boundary. The output format does not certify optical effective bits.
- Evidence time, publication time, render time and latch time are separate. A wall-time phase cannot be relabelled media-time without a qualified mapping.

## Freeze record

For each interface record: actual canonical source path; schema/semantic version; digest; owner; producer/consumer reviewers; capacities by target; serialization and rounding rules; negative vectors; migration/bypass/reset policy; generator version; staged output list; acceptance rows; known unsupported cases. Record unresolved implementation decisions separately from frozen meaning.

G1 descriptor/schema vectors are executable contract checks. They do not require the later production implementation to exist. Atomic engine, transport, UI and hardware fixtures belong to G2–G4. This prevents a circular gate.

If platform memory fit is not measured, mark its capacity profile proposed and disable target admission until proved; do not mislabel it as an achieved allocation. A source-only capacity proposal can support the schema shape without requiring a programmer or physical LED bar.

A later change names its consumers and invalidated evidence, versions the boundary and reopens only the affected gate. The shared bundle is published by CTL to staging; each consumer owner imports it. No generator may implicitly write into a sibling repository.


---

<!-- Source: lanes/01-contracts-and-control.md -->

# CTL — shared semantic contracts, profile compiler and control state

## Objective and finish line

Deliver one versioned control contract used by the console, DualMCU engine, Titan adapter and test harness. A fader addresses a stable parameter on its captured channel regardless of later navigation. The engine owns accepted base state; response and Liveiness produce separately observable effective state. Users can create admitted custom pages and edit control handling without changing the meaning of a parameter or breaking legacy peers.

This is an implementation handoff, not a claim that the described production code exists. The supplied R2 JavaScript backend is a tested behavioral reference with 26 host checks; it does not implement radio reliability, persistence, full custom-page compilation or target firmware.

The current user decisions are authority: channel-first Primary/Top and Secondary/Bottom, all nine faders assigned to that channel's page, palette-led colour, optional contextual dashboards, and the exact new label **Liveiness**. The old fixed-bank/no-pickup restriction is superseded. Keep legacy Chroma and Mood compatibility decoding; neither earns a default physical control through age. Never confuse audio pitch-class chroma with the legacy colour knob.

## Sources, repository seams and write boundaries

Read the packet's R2 architecture, source audit, source evidence, `control-core.js`, `Control-Contract.schema.json`, `Control-Profile.json` and `backend-design.md`. Their code snapshots are historical baseline evidence; before implementation record the candidate repository revisions and inspect applicable instructions. Preserve unrelated dirty work.

| Surface | Current evidence | CTL action and boundary |
|---|---|---|
| DualMCU `contract/product/control_map_v1.json` | Named by captured generator as current authority; v1 is 71 controls | Preserve v1 meaning. Add an explicitly versioned semantic registry beside it after allocation review; do not overwrite old meanings |
| `contract/product/k1_console_profile_v1.json` | Captured fixed 16 physical entries: nine faders and seven pad slots | Preserve legacy profile as a compatibility product. Produce separate paged profiles |
| `scripts/generate_console_contract.py` | Captured generator couples product identity to fixed layout and writes directly to an absolute K1.Sliders path | Add explicit output-root/export behavior before swarm use. Generate to CTL's staging directory; never write into another agent's checkout implicitly |
| `contract/k1_deck_state_v1.h` | Captured 71 controls, 68 eligible snapshot entries | v1 stays interpretable. New semantics require a separate negotiated version/extension |
| `contract/bridge_protocol.h` | Captured v1 protocol, 64-byte maximum payload | CTL owns semantic schema approval; TRN owns codec implementation and framing changes after freeze |
| `core/control/control_catalogue.cpp` | Captured engine catalogue | CTL supplies generated descriptors; engine lane performs integration at its owned boundary |
| `core/control/bridge_command_processor.cpp` | Captured mutation/dispatch seam | Engine lane ports authoritative application; CTL provides portable validator/transaction core and fixtures |
| K1.Sliders `k1_profile.h`, `k1_session.cpp`, `main.cpp`, generated header | Captured movement threshold, session/cache, static mapping | HMI imports CTL bundle and TRN client. CTL does not edit console-owned files |

Suggested new surfaces below are **proposed**, not inspected existing paths: `contract/control_v2/` for schemas/registry/generated portable headers; `core/control/semantic_state.*` for portable state/validation; `scripts/compile_control_profile.py` for deterministic profile compilation; `tests/control_v2/` for fixtures. Resolve their final placement through the integrator before editing. Do not create an additional repository merely to host these files.

CTL is the sole editor of the semantic registry and canonical schema source. TRN, HMI, AP, VP and platform specialists submit field/semantic change requests. CTL publishes a digest-bound generated bundle; each consumer owner imports it in their own worktree. Handwritten edits to generated headers are forbidden. AP/VP engine internals and UI rendering are outside CTL's write set.

## Freeze the interfaces before cross-lane integration

Symbolic names in this table are the agreed handoff vocabulary, not allocated production numeric IDs. Preserve the distinction between protocol/schema version, parameter semantic version, binding revision, profile revision and running engine epoch.

| Object | Required fields and rules |
|---|---|
| `ParameterDescriptor` | Stable semantic key/ID and semantic version; type; channel/global scope; units; legal domain; default; formatter; support/inactive reasons; enable/dependency semantics; response owner; context key; migration policy |
| `ControlCapabilities` | Product and engine identity; supported contract versions/catalogue digest; admitted parameters and mode-adapter versions; channel count; compiled page/binding/transaction/state capacities; supported response policies; numeric encodings; optional telemetry groups |
| `BindingProfile` | Schema and profile identity/version/digest; channel pages; physical IDs and slot labels; parameter targets; user ranges; monotonic mapping/inversion; input limits; quantization; takeover; response preferences; context settings; capability requirements |
| `GestureIntent` | Controller/client and session identity; epoch; gesture ID and sequence; captured semantic target/scope; binding/profile revision; typed canonical value; terminal status; originating time/validity policy |
| `ControlTransaction` | Unique transaction identity; finite bounded operation list; expected revisions/read set; reason/source; atomicity scope; apply timing; admitted response policy; idempotency identity |
| `ApplyReceipt` | Original intent/transaction identity; accepted/rejected/unavailable status and reason; engine epoch and commit revision; actual accepted base values; active/inactive reason; affected target revisions |
| `StateSnapshot` | Engine epoch; snapshot identity/revision; catalogue digest; declared count/length; coherent base values and feature enables; integrity check; explicit begin/end semantics |
| `EffectiveObservation` | Canonical target; effective value; associated base revision; observation time/time domain; modulation sources; staleness/availability; optional decimated material-level detail |
| `VisualContextDescriptor` | Typed visualizer key, parameter group, units/scope and declared dependencies. No arbitrary downloaded executable UI code |

The frozen bundle must contain wire-independent schemas, supported numeric encodings/error bounds, reference JSON examples, positive and negative vectors, capacity fields, compatibility table, migration decisions and exact generated-file hashes. Numeric IDs are allocated once through the actual registry; no lane invents spare CC usage independently. Include ordinary features' enable/bypass descriptors separately from their amounts. AP evidence validity and multi-tempo selection semantics come from AP; VP owns Liveiness adapter meaning and actual colour/material units.

**Freeze prerequisite:** all producers and consumers acknowledge the same schema digest and unresolved semantic choices are either resolved or marked unadvertised. This is an interface dependency, not a reason to stop unrelated simulation or specialist work. Any later breaking change opens a new digest/version with explicit consumer impact; never patch the frozen artifact in place.

## Ownership and application law

1. Console owns calibration, physical positions, navigation, movement qualification, pickup and binding transforms. Its requested value is provisional.
2. A single active engine owns canonical parameter values and artistic feature enables. Radio mirrors and routes; it does not become a second engine authority.
3. The engine validates a transaction completely, then commits one immutable state revision at a declared safe control boundary. A/B edits are independent unless an explicit linked transaction requests both. Rendering sees a coherent revision, never partially applied operations.
4. A parameter's accepted base is distinct from its time response and from effective material state after Liveiness, audio modulation and limits. Use unambiguous field names; do not copy the host's historical `appliedValue` alias into a new protocol as if it means accepted base.
5. Engine owns the canonical time response. A binding may request an admitted policy; it must not create a hidden per-page smoother. Duplicate bindings to one canonical target share one base/effective state. UI animation can interpolate markers but cannot claim those interpolated values were applied.
6. Liveiness changes effective appearance through a versioned VP adapter. Neutral is identity. It does not overwrite Motion, Width, Persistence, AP timestamps/BPM/phase/confidence or independent enable states. Unsupported adapters report unsupported.
7. Blackout and required fault handling remain authoritative. Do not ease blackout into a lingering glow. Turning an ordinary feature off follows that feature's declared bypass/lifecycle contract; it does not delete user amounts or implicitly reset unrelated state.

For concurrent writers, implement a bounded per-target active-gesture lease with explicit preset/blackout override, terminal release and timeout. Freeze actual lease duration from HMI movement cadence and link budget before target tests. After override, rearm the losing physical binding from returned base state; do not replay its old pending intent. Independent A and B edits must not reject each other merely because a global revision advanced. Recommended model: monotonic engine commit revision for snapshots plus per-target expected revision for ordinary absolute writes; transactions carry their complete expected read set. Same-target conflicts reject/reconcile explicitly. Sequence, boot epoch and wrap behavior must be specified; wrap forces a fresh epoch before identifiers can alias.

## Ordered work packages

### CTL-1 — compatibility and semantic inventory

Start from the captured registry and refresh only named seams. Record each proposed parameter's unit/domain/response owner and actual engine support. Keep v1 Photons' floor 0.05 (Q0.16 3277), Mood IDs and Chroma decoding unchanged. Do not equate artistic Photons with new electrical intensity. New Liveiness is versioned semantic v1 with neutral 0.5 in the reference; VP must confirm its admitted adapters.

Produce `semantic-inventory`, `legacy-compatibility`, `migration-map` and unresolved-questions tables. Catalogue every Perform/Colour default as supported, planned or unavailable by platform and mode. Missing support must not produce a silently inert control or automatic remapping. Confirm the actual enabled mode catalogue rather than assuming the historical count remains fixed forever.

**Next gate:** TRN can encode only entries whose semantics and numeric domain are frozen; HMI may continue with mocks for unavailable entries clearly marked as such.

### CTL-2 — bounded profile compiler and generated exports

Validate the structural schema using an independent JSON Schema implementation and cross-field semantic validation. The R2 schema enumerates only fifteen semantics and two pages; extend the schema deliberately for admitted custom pages instead of deleting its bounds.

Compile drafts outside audio/render execution into fixed-capacity tables. Each platform supplies actual byte budgets and maxima for pages, bindings, parameters, strings, transactions, telemetry and persistence. The compiler rejects `max+1` cases; it never silently drops bindings or allocates unbounded buffers. Minimum product capability is the two supplied nine-fader pages on each of two channels. A custom-page feature must include at least one admitted user-created nine-fader page per channel on its advertised target, within its frozen resource budget; a target without capacity advertises the limit and does not pretend import succeeded.

Validate known target/version, scope, finite values, canonical and user ranges, monotonicity/inversion, legal positive log domains, supported response policies, unique physical slots, duplicate target policy, page labels, feature dependencies and capability set. The same parameter may appear on multiple pages and must resolve to one canonical value. A default page has nine binding entries; unavailable functions retain a visible reason instead of covert reassignment. Provide explicit unused slots if custom profiles choose fewer functions.

Compilation is deterministic: identical canonical inputs and compiler version produce identical binary tables/digest. Emit platform-neutral fixtures and explicit endian-safe serialized forms, not raw compiler struct layouts. Export to a caller-selected directory. Consumer integration verifies digest, generated version, capacity and source registry identity.

### CTL-3 — portable state, binding math and lifecycle

Port reference behavior into bounded production-suitable types. Keep the 26 host vectors as historical behavior fixtures; write targeted new tests for production differences rather than merely copying the same implementation into a second language.

Absolute control law is calibration → effective input limit → qualified movement/hysteresis → captured binding/takeover → monotonic range mapping → canonical stepping → typed intent. Exact configured minimum and maximum remain reachable even when the range is not divisible by step size. Ranged takeover anchored at a physical endpoint must report its pickup fallback; a narrowed range never silently clamps the current engine value just by opening the page.

Damping uses elapsed monotonic time: `y += -expm1(-dt/tau)*(target-y)` for exponential response, or canonical units/second for slew. Define direct/zero-time response, overshoot policy, exact terminal snap epsilon, finite-duration easing restart/retarget behavior if offered, and long scheduling-gap treatment. No silent `dt=50 ms` clamp that slows real time after missed frames. Response state continues independent of dashboard frame rate and page visibility. Console filtering and engine response have separately measured latency budgets.

Absolute faders retain deterministic position meaning; acceleration is rejected for their mappings. Add relative precision as a separate input modality (touch drag/encoder) with bounded delta integration, explicit gain curve versus velocity, a maximum delta rate, endpoint clamp and reset/rearm semantics. A relative gesture carries event time and ordered deltas or an engine-rebased absolute target, as agreed with TRN. Delta application must be idempotent. Do not let packet retransmission add the same delta twice. If relative acceleration cannot be completed in this milestone, expose it as unavailable; do not advertise the full control editor as complete.

Page/channel selection generates zero engine writes. Physical motion continuing through navigation cannot silently begin a new target: HMI supplies a measured quiet/rearm movement-session boundary. Already accepted pending work stays on its captured target. Reconnect, profile changes, capability loss and external base changes cancel/rearm affected bindings and invalidate obsolete unsent work without erasing unrelated engine history.

### CTL-4 — persistence and migration

Separate physical calibration, user control profile, appearance preset and device/output calibration namespaces. Store profile drafts/commits with schema version, length, checksum and generation using the platform's established atomic persistence mechanism; do not force a new storage technology. Power interruption while saving must recover either the last complete profile or the complete new profile. Save on explicit commit/debounced administrative action, never on each ADC sample. Expose failed save distinctly from active-in-RAM.

Preserve original preset/profile bytes before migration. A new field gets a documented default; a changed meaning requires a migration function and stored source version. Mood-to-Liveiness is not a numeric rename: legacy presets preserve legacy semantics or enter an explicitly migrated new mode adapter. Unknown future major versions reject with a useful reason and leave the current profile/state intact. Rolling back firmware must not destructively rewrite newer user data.

### CTL-5 — publish bundle and hand to consumers

After independent contract review, publish the registry/schema/compiler/core commit, hashes, generated export, compatibility vectors and admission matrix. HMI, TRN and engine agents each consume the exact digest. Re-run consumer conformance on their candidate revisions; one CTL-only pass is not integration proof.

## Acceptance metrics

Numerical entries labeled **new requirement** below are proposed acceptance requirements for this handoff, not prior hardware measurements. Freeze them with the integrator before implementation; an infeasible result becomes a documented failed requirement or pre-test decision change, never a retroactively loosened threshold.

| ID | Method and conditions | Pass condition | Evidence |
|---|---|---|---|
| CTL-A01 | Structural and semantic validation: valid defaults/custom page plus malformed type/range/log/scope/duplicate/capacity/profile inputs | All valid admitted cases compile; every invalid draft rejected atomically; old profile, gestures and engine state unchanged | Input corpus, validator/compiler version and exact results |
| CTL-A02 | Compile identical canonical input twice from clean output directories | Byte-identical generated outputs and digests; no writes outside declared output root | File manifest and filesystem diff |
| CTL-A03 | Verify v1 address/value/photons/identity golden vectors | Every retained v1 wire meaning and floor unchanged; new semantics never alias old IDs | Old/new bytes and decoded comparisons |
| CTL-A04 | Rapid page/channel/profile change with delayed old gesture and mixed A/B edits | Zero writes from navigation; zero target/channel retargets; obsolete events rejected; last unsuperseded terminal gets explicit outcome | Deterministic event trace including revision/epoch/target |
| CTL-A05 | Atomic multi-operation transaction with invalid final operation and concurrent independent writes | No partial mutation; coherent commit; independent A/B edits succeed; same-target conflict explicit | Before/after snapshots and receipts |
| CTL-A06 | Grid over monotonic mappings, inversion, ranges and endpoint cases | No mapping reversal except requested inversion; exact endpoints; inverse error ≤ declared encoding error plus one representable input step | Numeric max error per encoding |
| CTL-A07 | Same step/ramp evaluated at 30, 60, 120 Hz and irregular schedule, same elapsed endpoints | **New requirement:** normalized effective-value difference ≤1e-4 for FP32 reference response, or the stricter declared target encoding envelope; zero NaN/overflow/overshoot where disallowed | Time-stamped response traces, max differences |
| CTL-A08 | Liveiness changes and feature disable/enable against immutable base state | Unrelated bases, other channel and AP evidence fields unchanged; neutral adapter identity within VP's frozen tolerance | State hashes/diff plus VP fixture ID |
| CTL-A09 | Capacity maximum and one beyond; instrument real-time paths | Maximum admitted case fits published allocation; excess rejected; zero heap allocation, JSON parsing or persistent writes in steady AP/render/control-apply critical paths | Map/size report and allocation/IO trace |
| CTL-A10 | Interrupted profile save at every transaction boundary; version upgrade/downgrade corpus | Always previous complete or new complete profile; never mixed data; newer incompatible source preserved | Storage-image fixtures and recovery results |
| CTL-A11 | Retransmitted relative delta and time-dependent acceleration fixtures | Exactly one state effect per accepted delta identity; absolute mappings reject acceleration; gain/rate/clamp law matches descriptor | Duplicate-delivery and velocity traces |
| CTL-A12 | Three declared target adapters: RT1062, Titan, standalone S3 | Each either passes same semantics within encoding bounds or advertises explicit unsupported features/capacities; no silent semantic substitution | Per-platform conformance matrix |

## Gates, deployment and definition of done

**CTL schema freeze GO:** semantic owners approve meanings, TRN approves encodability, engine owners approve bounded representation, HMI approves editor/visualizer requirements, and independent reviewer accepts compatibility vectors. Freeze produces one digest plus versioned open-feature list. Before this, mocks are allowed; production registry mutation or cross-lane integration using guessed IDs is not.

**CTL consumer GO:** every consumer builds against the same generated digest and its interface tests pass. If one target cannot admit a feature, capability negotiation must reject it there while unrelated lanes continue. Do not hold the whole swarm for an optional feature.

Deployment follows TRN's consumer-first plan: engines understand but do not yet advertise active new semantics; bridge understands them; console negotiates them last. Preserve v1 decoding throughout migration. Profile activation is a separate negotiated commit after the complete authoritative snapshot, not a side effect of flashing. Rollback disables the new producer first, drains/cancels sessions and then reverts consumers while preserving saved new-version data.

CTL is **done** when schemas and production allocations are frozen, compiler/core and required custom-page/response functions exist, all CTL criteria are evidence-bound PASS or explicitly unsupported by a capability decision, the generated bundle is consumed by downstream candidates, and v1 migration/rollback is proven. “Host tests pass” alone is not done. Hand the integrator the changed-path list, commit/digest, profile/migration fixtures, capacity report, per-criterion receipt and exact consumer imports; state any unsupported feature visibly and leave no unassigned contract question.


---

<!-- Source: lanes/02-audio-tempo.md -->

# AP — audio evidence and multi-tempo execution brief

Status: **implementation handoff; no firmware changes or target measurements were performed to create this brief.** Owner: audio/DSP specialist. Independent reviewer: audio oracle/verification specialist. Target deployment is owned by the DualMCU and Titan platform specialists after the relevant integration gate. This brief defines portable behavior; it does not assign radio, touchscreen, rendering or hardware-driver ownership to AP.

## 1. Finish line and controlling decisions

Expose the rhythmic evidence that K1 already computes as a bounded, truthful `TempoFieldV1`, without changing the established winner/flywheel, onset events, audio feature trajectory or existing scheduling authority. The field lets VP combine immediate accents, a conservative musical clock and multiple continuous rhythmic contributions. These are complementary descriptions of the input. Several strong tempo bins do **not** establish several independent musicians or verified polytempo.

Finish in two separately admissible increments:

1. **AP-A: evidence publication.** Export salience, validity, activity and evidence age with a local immutable interface. Keep phase invalid until its convention and timing are proven.
2. **AP-B: bounded phase tracks.** Add at most four freshly evaluated tracks per completed field update, with eight fixed slots available for transition overlap. Old overlapping tracks coast with explicit ages; capacity eight does not authorize eight new analyses per update. Preserve the incumbent bank and clock.

The initial deployment does not widen the 60–155 BPM bank, replace peak pooling, promote secondary clocks into hard-deadline scheduling, rewrite the detector, introduce U55/Helium work, or change the audio sample rate. Such work requires a separate attributable experiment justified by a measured remaining need. It is not a prerequisite for this lane.

Current user decisions supersede the old AP research's fixed-instrument/no-bank/no-pickup paragraph. R2 uses channel-first pages, configurable bindings and soft pickup. AP owns no physical assignments. **Liveiness** is appearance modulation; it cannot rescale AP time, rewrite observed BPM/phase, reset the detector or mutate stored independent appearance controls. Audio pitch-class chroma remains independent of the merit decision about the legacy physical CHROMA knob.

## 2. Source authority and baseline capture

The following are historical source snapshots, not claims about the execution-day tree:

| Product lane | Inspected source root | Snapshot identity |
|---|---|---|
| DualMCU | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware` | HEAD `67a1cd0b0ecc626b28af2df9b0d11f855b402379`, dirty tree |
| Titan Mini | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware` | HEAD `431140853dd8b58af53240ef84d5fb1a08bd8b45`, dirty tree |

Read these packet inputs before implementation:

- `source-inputs/k1-pipeline-integration/multi-tempo-architecture.md`, especially “Two important timing traps”, “Proposed bounded contracts” and “Lane-specific integration”. The superseded HMI paragraph is not controlling authority.
- `source-inputs/k1-pipeline-integration/evidence/ap-source-manifest.json` and its hashed `evidence/ap-source/` files.
- `source-inputs/k1-pipeline-integration/evidence/ap-runtime-excerpts.json` and `ap-derived-costs.json`.
- `source-inputs/k1-control-r2/control-architecture.md`, “Liveiness must be a promise every admitted mode can keep”.

First checkpoint the actual candidate through the repository's current worktree/governance process. Record HEAD, relevant dirty-file hashes, compiler/options, generator or overlay identities, sample rate, hop size, novelty decimation, capture source, platform clock mapping and enabled workload. Do not discard unrelated dirty work or assume it belongs to this lane. The portable AP snapshots matched byte-for-byte in the prior inspection; prove current equality before sharing a patch mechanically.

### Existing seams to inspect and preserve

| Surface | DualMCU location | Titan location / difference |
|---|---|---|
| Rate and hop tuple | `core/audio/audio_rate_config.h` | `src/k1/core/audio/audio_rate_config.h` |
| Composition | `core/audio/audio_pipeline.{h,cpp}`: `AudioPipeline::process`, `AudioPipelineOutput` | Equivalent under `src/k1/` |
| Existing bank and selected clock | `core/audio/tempo_tracker.{h,cpp}`: `TempoTrackerState`, `updateTempoTracker`, `readTempoTracker`, `advanceFlywheel`, `buildOutput` | Equivalent portable import; generated overlay may differ |
| ACF | `core/audio/tempo_acf.{h,cpp}`: 512-value ring, two 96-bin salience arrays | Equivalent import plus optional `tempo_acf_slice` |
| Musical coordinate | `core/audio/musical_time.{h,cpp}` and existing media-time definitions | Equivalent portable import |
| AP → VP view | `core/visual/visual_audio_frame.h` | `src/k1/core/visual/visual_audio_frame.h`; current view differs |
| Runtime publication | `firmware/rt1062/production_runtime.{h,cpp}` | `platform/ra8p1/k1_live_runtime.cpp`, `fixture_app.cpp` |
| Scheduling authority | `core/system/realtime_render_scheduler.cpp` | Current `finalisePublication()` explicitly clears predicted-next-beat validity |

AP owns the audio algorithms and proposes the `VisualAudioFrameView` addition for the CTL/AP interface freeze. INT owns edits to existing portable `core/visual` integration call sites, including the `VisualAudioFrameView` seam; DUAL and TIT own their runtime publication and platform wiring. VP must not independently redesign that same struct in a competing branch. Shared-interface changes follow the frozen contract, with one designated writer per surface.

## 3. Contract to freeze before consumers implement

All names below are **proposed output files**, not assertions that the files currently exist. Use the actual repository's naming/test conventions when binding the plan; record any relocation once in the integration manifest.

| Proposed portable output | Purpose and owner |
|---|---|
| `contract/tempo_field_v1.h` | Bounded engine-local semantic contract; contract owner reviews, AP implements |
| `core/audio/tempo_field_publisher.{h,cpp}` | Read-only bank/ACF evidence capture and validity/age state |
| `core/audio/tempo_phase_tracks.{h,cpp}` | AP-B selection, phase observation, bounded correction and lifecycle |
| `tests/audio/tempo_field_contract_test.cpp` | Schema/state/age/capacity and negative cases |
| `tests/audio/tempo_phase_tracks_test.cpp` | Phase oracle, identity, derived-family and transition cases |
| `tests/audio/tempo_incumbent_parity_test.cpp` | Old-versus-new incumbent trajectory comparison |
| `tests/fixtures/tempo_field/v1/manifest.json` | Exact deterministic fixture identities and scoring rules |
| `tools/audio/score_tempo_field.py` | Host metric calculation; no target control or flashing |
| `docs/evidence/tempo-field/<candidate-id>/` | Baseline, candidate, raw traces, scores, compiler/build identity, disposition |

Titan equivalent portable paths are under `src/k1/` except its repository-level tests/tools/docs. Do not establish a new cross-repository shared library solely for this change. Deliver a canonical portable patch and a hash/semantic correspondence manifest; each platform owner imports and validates it in that lane's actual build path.

### Required semantic fields

- Header: schema version, detector configuration revision, stream epoch, field generation, analysed support start/end in `MEDIA_TIME_48K`, monotonic result-available timestamp. Generation advances on completed **new evidence**, not on each cached AP publication.
- Flags: field-valid, warm-up, silence, activity-valid, phase-authority and coasting reason. An unavailable or invalid measurement is not encoded as a convincing numeric zero without a flag.
- Activity: nonnegative unnormalised evidence or a clearly defined measurement derived from pre-normalisation energy. ACF peak-normalisation cannot provide absolute confidence. Name concentration, prominence, activity and incumbent confidence separately; none is a calibrated probability.
- Canonical descriptor: incumbent winner identity, lock and confidence plus the **actual** flywheel running frequency/period, phase and reference time. The existing `TempoTrackerEvent.bpm` reports winner target BPM, which can differ from the running frequency. Preserve that event; do not silently redefine its fields.
- Candidate array: capacity eight, count, stable bank-bin ID (`60 + bin` BPM in the present bank), track generation, selection/point salience, admission state and phase validity. Sorting is presentation order; ID is identity.
- Observation: analysed support, last direct phase-observed media frame, phase reference, oscillator frequency or Q32.32 beat period, phase uncertainty with defined units, and observation age. Extrapolation changes a projected phase but never its last-observed time.
- Family metadata: root ID and root generation, numerator/denominator, derived flag, beat-count/parity anchor. A 180 BPM oscillator derived from 90 BPM is labelled derived because the present bank does not observe 180 BPM.

Prefer existing `AudioTime`, fractional beat-period and epoch semantics. A reference anchored only to a wrapped phase cannot produce stable half-time alignment. Compute derived phase from extended beat coordinate `n + phase`: for ratio `p/q`, use the fractional part of `(p/q) × (n + phase)`, retaining the remainder/parity across wraps. Check denominator zero, integer overflow and epoch mismatch before evaluation. Never recover beat parity from a render frame counter.

For the incumbent clock descriptor, identify the coordinate in which its phase was actually advanced. On DualMCU, a phase advanced using hop-ready wall time cannot simply be relabelled as measured at an arbitrary media endpoint. Use the existing qualified mapping with explicit reference/uncertainty, or mark the new descriptor's media projection unavailable. The new phase adapter is independently media-based. Do not silently repair incumbent time semantics inside the supposedly read-only export. An uncertainty field may be explicitly unknown; it must not contain an invented confidence-derived error bar.

Store no pointers to a mutable AP work buffer in a persistent VP view. Commit a complete publication atomically using the platform's existing ownership model. A const view is valid only for the documented render scope, or the consumer owns a bounded copy. Two channels consume the same immutable measurement without advancing AP state twice. Full 96-bin diagnostics, when enabled, are a separate bounded snapshot sampled on field updates; they are not copied at every LED refresh or streamed across BLE by this work package.

The earlier research's below-2-KiB snapshot estimate is a **design budget to verify**, not a measured allocation. Emit `sizeof`/alignment information plus total state, double-buffer, stack and optional-diagnostic costs. Keep the maximum counts fixed, allocate before steady-state execution, and fail admission if the platform's pre-recorded memory budget is exceeded. Do not add heap allocation, JSON, formatting, flash writes or blocking locks to the AP path.

## 4. Ordered implementation and handoff points

### AP-0 — freeze fixtures, semantic rules and same-build baseline

Dependencies: repository baseline accepted; contract owner has assigned schema ownership. AP author and independent verifier agree the input and result schemas before candidate output exists. Bind the proposed fixture plan in `evidence/AP-Fixture-Plan.proposed.json` into executable generators and tests. Keep synthetic novelty tests separate from PCM end-to-end tests: the former isolate tempo behavior, the latter include spectral/AGC/nonlinear treatment.

Record the existing incumbent outputs at every AP hop and field update: selected BPM, phase, confidence, lock, tick, strength, onset events, audio feature fields and scheduler-facing flags. Use field-by-field serialization; do not compare padding bytes in a C++ struct. Store source/input/configuration hashes. Baseline silence is allowed to retain an incumbent BPM value; absence of activity and event authority matter. Do not invent a silence requirement that the preserved legacy tracker does not meet.

**GO output:** frozen fixture manifest, metrics definitions, current timing/admission contract and baseline traces. If no existing target threshold is available, the platform owner must establish and record a baseline/admission budget before target promotion. Host implementation can proceed independently.

### AP-1 — add read-only salience and age publication

Add the sidecar after the existing tempo update. Preserve existing arithmetic, call order, novelty pooling, challenger logic and flywheel corrections. For the unsliced path, attach the update's actual analysed history endpoint. For a sliced path, capture endpoint/epoch/configuration at job creation and carry them unchanged through completion. A job that crosses an epoch/configuration discontinuity is discarded; it never publishes old evidence under a new epoch.

Track raw bank observation times where the two-bin traversal actually recomputes each bin; an unchanged cached bin keeps its observation time. Do not derive these ages from the enclosing publication timestamp. Ages of never-observed bins remain invalid. Preserve the distinction between fresh ACF salience and a potentially old Goertzel phase.

Expose a snapshot reader and a bounded disabled state. Add an explicit field-generation counter at actual completion; no increment on intervening AP copies. Optional instrumentation must not alter the incumbent's iteration order or state.

**GO output:** AP-A host contract/parity evidence and stub consumed by VP. VP may implement salience-based composition using fixtures before target admission. Phase-driven production behavior stays unavailable until AP-B passes.

### AP-2 — add bounded, separately owned phase observations

Read one stable novelty-history snapshot per completed field update. Select local peaks using declared relative-tempo separation or measured lobe width, with admission/removal hysteresis. Freeze tie-breaking by candidate ID. Candidate count and selection policy are configuration, not raw measurement semantics.

Compute at most four fresh phase observations using the stable snapshot. Establish the phase convention with an independent direct sinusoidal-correlation oracle and known impulse positions; document sample ordering, history endpoint, window, sign, phase offset and group/support delay. Do not transplant the donor's phase offset or the incumbent bank's existing extrapolation and call it a calibrated beat timestamp.

Each admitted ID retains its phase track across sort changes. Modest phase correction is bounded by a declared configuration value in cycles/update or cycles/second. A correction outside that bound demotes phase authority or reinitializes a new generation; it does not teleport an existing VP object. On re-entry, distinguish a retained/coasting track from a genuinely new track. VP owns the visual crossfade/energy policy, AP owns the truth about track state and age.

Freeze coast duration and stale cutoff in media frames, with an equivalent user-facing seconds/beats description. These are configuration values tested at their exact boundaries, not a hidden universal constant. Zero evidence cannot be normalised into full amplitude. Silence can attenuate activity while bounded phase coasting continues with the coasting flag.

**GO output:** AP-B contract, phase oracle results, lifecycle traces and measured portable cost. No claim of better beat detection is required or implied; this stage exposes additional expressive evidence.

### AP-3 — consumers and platform integration

Send VP immutable fixtures with fresh, stale, silent, warming, locked and ambiguous examples. VP may choose normalized `s_i^p / sum(s^p)` weights or a separately named donor-compatible `m_i² / sum(m)` composition. Neither weighting belongs in detector confidence, and zero denominator emits zero. Liveiness-derived subdivisions stay downstream/explicitly derived.

Send control/HMI a compact diagnostic semantic list: feature available/enabled, field generation, activity validity, candidate count/IDs, observation age, lock/coast state and applied configuration revision. HMI visualizations must not mislabel an authored mix as detector certainty. Contract/radio owners choose a bounded negotiated status representation if needed; AP does not add wire IDs, MIDI mappings, BLE packets or a 96-bin radio stream.

DualMCU owner adds sidecar storage to `ProductionRuntime`, updates the local AP→VP view and preserves capture-derived media coordinates. Its current legacy tracker receives hop-ready wall time; changing that clock is a distinct migration, not part of this parity-preserving patch. New phase tracks use media time, with the existing media-to-monotonic mapping for presentation projection. They do not each reserve a hard render deadline.

Titan owner adds the sidecar to `LiveAudioRuntime` private/committed publications and the `fixture_app.cpp` view. Preserve `predicted_next_beat_valid=false` in the current finalization path unless a separate scheduling task has already changed that authority. Record whether the ACF slicing overlay is active and capture the generated effective source hash. A source-only import hash is insufficient when an overlay changes runtime behavior.

**GO output:** independent platform build/runtime receipts for the exact paired integration candidates. A Dual pass never promotes Titan, or vice versa.

## 5. Lifecycle matrix

| Event | Required AP behavior | What must survive |
|---|---|---|
| Rhythm layer disabled on channel A | A stops consuming rhythmic contribution; AP state remains available | Canonical clock, channel B, onsets, other layers |
| Rhythm mix/Liveiness/page/palette changed | No detector mutation, reset or phase re-anchor | Complete AP trajectory under identical input |
| Field publication disabled | Mark feature unavailable/disabled; stop sidecar work allowed by dependencies | Incumbent bank/ACF/winner still required by existing clock |
| Re-enable publication | Publish truthful current ages; phase tracks reacquire as declared | No fabricated fresh observation or replay of old onset |
| Optional phase computation suspended | Phase invalid/coasting only within admitted policy | Salience export and incumbent clock |
| Silence | Activity follows declared measurement; no new synthetic detected attacks | Bounded, labelled coasting if configured |
| Dropped/cached publication | Field generation and observation time stay unchanged | Consumer can identify age rather than fake freshness |
| Stream epoch / rate discontinuity | Invalidate old field, discard pending jobs, clear track identity namespace, require warm-up | Unrelated control base values and channel configuration |
| Detector configuration revision | Apply an explicit state-transition rule; incompatible evidence invalidated | Appearance-only changes never use this route |
| Counter wrap / malformed time / NaN | Detect invalidity, publish bounded unavailable result; never propagate NaN into VP | Last safe incumbent path subject to its existing fault contract |

## 6. Metrics, fixtures and exact acceptance

All reports separate **functional correctness**, **timing admission**, **estimator quality** and **appearance preference**. Do not average them into one score. An attractive render is not a beat-accuracy pass; a correct phase projection is not proof of attractive movement.

### Clock arithmetic and resource facts

Use the tuple observed in the actual build:

| Quantity | 12.8 kHz / 96 samples | 24 kHz / 180 samples |
|---|---:|---:|
| Nominal AP hop | 7.5 ms | 7.5 ms |
| AP rate | 133.333… Hz | 133.333… Hz |
| Three-hop novelty interval | 22.5 ms | 22.5 ms |
| 512-value novelty support | 11.52 s | 11.52 s |
| Two-of-96 raw-bank revisit | 1.08 s | 1.08 s |
| `MEDIA_TIME_48K` per AP hop | 360 frames | 360 frames |
| `MEDIA_TIME_48K` per novelty interval | 1,080 frames | 1,080 frames |

These matching cadence numbers do not establish PCM spectral parity, current live-capture qualification or a common image. The prior source declares 24k/180 as production candidate and retains 12.8k/96 as a frozen reference. Historical Titan 12.8k timing receipts are not a 24k live-input pass. Record source tuple, actual capture tuple and declared contract token; a mismatch blocks that target admission.

The steady max-pool input can come from any of three hops spanning **15 ms**. If a new event path carries an argmax timestamp, preserve that real input time. Do not subtract 15 or 22.5 ms as a guessed universal latency correction. Keep event time, support time, calculation completion and output presentation distinct.

The source-derived four-phase estimate is 2,048 additional Goertzel recurrences/update, approximately 91,022/second. This is not a cycle measurement. Instrument the candidate on each target and report AP work duration, field-stage duration, start lateness, completion lateness, publication age, missed/dropped hops, stack/high-water and allocation counts. Report max and distribution against the same workload baseline. Hard admission uses worst-case/deadline/loss rules already frozen for that target; percentile improvement cannot excuse a missed deadline.

| ID | Criterion and method | Pass condition | Evidence |
|---|---|---|---|
| AP-AC01 | Incumbent parity: baseline versus candidate, new consumers off and on, same compiler/config/input | Serialized incumbent fields identical for every hop; no altered onset/tick/lock sequence. Field comparison excludes struct padding | Fixture IDs, raw dual traces, first-difference scorer, source/build hashes |
| AP-AC02 | Existing salience extraction | Exported selected bin values match the same completed ACF snapshot; no missing/duplicate IDs; capacity never exceeded | Independent snapshot comparison and boundary counts |
| AP-AC03 | Time/age correctness under delayed/sliced/cached publication | Analysed endpoint remains job endpoint; availability reflects actual completion; generation changes once per completed evidence update; never-observed/old phases remain marked accordingly | Trace with scheduled delays and independently computed expected times |
| AP-AC04 | Phase convention and projection | Direct-correlation oracle and implementation agree within a numerical error bound fixed **before** candidate scoring; same reference projected at a given media time is independent of render cadence and processing delay | Oracle derivation, arithmetic/error-budget file, impulses/clicks/fractional tempo traces |
| AP-AC05 | Phase accuracy as a musical measurement | Report circular phase error converted to ms, acquisition/reacquisition time, outliers, ambiguity and valid coverage. Do not label uncalibrated phase a beat instant; any claim of beat-accurate phase needs a predeclared task tolerance tied to input quantization, analysed support and intended VP use | Separate measurement report; exploratory metrics cannot be relabelled PASS without the registered tolerance |
| AP-AC06 | Track identity and correction | Candidate sorting does not exchange state; outgoing/re-entering IDs obey generation rule; no applied correction exceeds configured bound; a discontinuity is explicit | Rank-crossing, retire/re-entry and large-phase-error traces |
| AP-AC07 | Derived ratios | Half/double/3:2 projections match independent extended-phase arithmetic across wraps; half-time parity survives; derived output is never marked independently observed | Exact anchor/ratio vectors and overflow/zero-denominator cases |
| AP-AC08 | Lifecycle and isolation | Every lifecycle row passes; appearance/HMI changes produce no AP difference; epoch changes yield no valid cross-epoch projections | Toggle/reset/reconnect and A/B consumer-order fixtures |
| AP-AC09 | Silence and invalid input | Zero evidence remains zero for field energy; no new synthetic detected onset; invalid numbers do not reach VP as valid evidence; stale cutoff transitions at the configured boundary | Zero/noise/NaN/Inf/stale boundary vectors |
| AP-AC10 | Memory and real-time ownership | Fixed capacities respected; no steady-state allocations; committed snapshot cannot tear; memory/stack within recorded target budget | Compile-time sizes, map/high-water, allocator instrumentation, stress trace |
| AP-AC11 | Target deadline and capture preservation | Candidate meets that target's frozen complete deadline/loss/fault requirements under admitted combined load. For a hop-bound implementation, 7.5 ms is the release interval, not a sufficient standalone gate; start lateness and queue loss also pass | Paired baseline/candidate target runs, counters, workload/configuration/image identities |
| AP-AC12 | Consumer contract | VP rejects unavailable/stale/epoch-mismatched phase; rendering twice cannot mutate AP; compact diagnostics retain truthful validity/age | Independent VP integration fixtures and contract owner's compatibility receipt |

For AP-AC04, the verifier derives a floating-point error budget from the actual recurrence/window length, precision and conditioned fixture amplitude, checked against a double-precision direct-sum reference. Exact fixed-point/time vectors must be exact. Compare fractional phase/time error as well as converted milliseconds. A blanket arbitrary `epsilon` or a tolerance enlarged after seeing failure is not acceptable. AP-AC05 deliberately prevents an untested precision claim: AP-B can be admitted for explicitly continuous expressive motion under AP-AC04 without marketing it as beat-event detection. Beat-event scheduling is outside this lane.

End-to-end fixtures include silence→music→silence, weak nonperiodic noise, 60/90/120/155 BPM clicks, fractional 127.43 BPM, swing, syncopation, half/double ambiguity, 3:2 pulse combinations, tempo ramp, abrupt tempo change, delayed completion, dropped publication, stream reset and invalid values. Synthetic ideal vectors establish mathematical conventions; fixed PCM fixtures establish full-chain behavior. Record PCM synthesis/resampling and expected event samples so a sample-rate conversion cannot silently move the ground truth.

For estimator reporting, score exact-tempo error and metrical-family error separately; a half-time answer is not silently counted as exact. Warm-up/acquisition times are measured from defined input events. Do not demand current confidence lock before the source's 512-update warm-up; also do not imply that new expressive activity must wait 11.52 s to appear. Do not change legacy detector behavior solely to make new fixtures attractive.

For appearance evaluation, hand the same captured fields, timestamps and musical material to VP's single-clock, donor-field and selected-family profiles. VP records brightness-matched preference, negative space, competing accents, motion continuity and stale-evidence behavior. AP supplies evidence identity and supports diagnosis; the visual specialist owns that acceptance.

## 7. Parallel execution without conflicting ownership

The captain assigns one AP implementation owner and one independent verifier; they may work concurrently once AP-0 freezes the contract. The verifier writes fixture generators/oracles/scorers and owns the acceptance result, not the AP implementation files. The implementer cannot approve its own oracle by copying its internal computation into the test.

At AP-A, VP starts fixture-driven composition and HMI starts typed rhythm diagnostics while AP-B proceeds. They do not wait for target flashing. DualMCU and Titan owners prepare their publication adapters/build wiring in separate platform worktrees; each waits for a reviewed portable revision before importing it. Contract/header changes are landed once through the designated integration owner. Every handoff names producer revision, contract version, expected consumer patch and test fixture identity.

Use a short asynchronous change request when a consumer needs additional information: field name/units, concrete use, memory/cadence cost and compatibility effect. Do not let UI convenience drive detector semantics, or let AP enlarge a packet merely because another agent can render it. Prefer optional bounded diagnostics over expanding the hot path.

## 8. Gates, completion and leave-state

| Gate | GO evidence and permitted next action | STOP condition and bounded resumption |
|---|---|---|
| AP-G0 contract/baseline | Baseline bound to source/build, fixtures and semantic fields frozen → AP-A and consumer stubs | Unknown current tuple/clock or overlapping edit ownership → resolve identity/owner; continue unaffected host fixture work |
| AP-G1 publication | AP-AC01/02/03/08/09/10 host checks pass → consumers use AP-A; AP-B begins | Any incumbent difference or false freshness → repair publication/metadata; preserve failed trace |
| AP-G2 phase | AP-AC04/06/07 plus applicable lifecycle checks pass; AP-AC05 claim bounded → continuous VP phase integration | Unknown phase convention, parity loss or unbounded correction → retain salience-only AP-A; repair phase adapter |
| AP-G3 target admission | Platform-specific AP-AC11/12 plus required host rows pass → feature available on that exact target/profile | Missing deadline/loss counters, tuple mismatch, malformed evidence or failed gate → disable/quarantine only new feature; resume after scoped repair and matched run |

These gates are automatic evidence dependencies, not invented permission requests. This handoff-preparation turn does not deploy firmware. During execution, the platform lane follows authorization and HOLD state already applicable to its target; AP does not impose an unrelated universal no-flash rule.

**Module-host done** means AP-A/AP-B scope is explicit, portable implementation and fixtures are reviewed, the applicable host acceptance rows pass, source and output hashes are bound, contract consumer examples are delivered, and all untested target/measurement claims are labelled. It permits target integration, not a blanket product release.

**Target-admitted done** additionally means that particular target's effective generated source/image, actual input tuple and full-load publication behavior pass its recorded runtime gates; the consumer sees truthful field validity/age; compatibility/fallback and current leave-state are verified. DualMCU and Titan dispositions remain separate. A future S3-only port has its own capacity/timing admission and cannot inherit RT1062 or M85 measurements.

The final AP receipt contains: changed files and commits; contract version; portable correspondence hashes; fixture/scorer identities; PASS/FAIL/BLOCKED/NOT TESTED per AP-AC row; baseline/candidate runtime deltas; memory sizes; raw evidence paths; failed/quarantined attempts; unsupported claims; and exact next owner/action. The next owners are VP for expressive composition, contract/controls for compact diagnostics, and the two platform specialists for target deployment. Do not claim the full product done while either target's required admission rows remain open.


---

<!-- Source: lanes/03-visual-core.md -->

# VP — Portable wide visual core, material and precision

Execution handoff · 24 September 2026 · Status: implementation instructions, not an implementation receipt

## Objective and authority

Deliver a portable scalar visual core that retains fractional geometry, colour and history through composition and output treatment, then emits native RGB16 words through each lane's existing owned backend. Deliver one complete centre-origin wide appearance profile first, with independently controlled material and artistic modules. Keep the incumbent appearance as a named compatibility profile and numerical reference. A type conversion or a successful native16 packer alone does not complete this lane.

The current R2 user direction governs controls: channel-first pages, meaningful Liveiness, palette-led colour, separate feature enables and amounts, and full control depth with progressive disclosure. The earlier fixed Appearance9/no-banking/no-pickup proposal is superseded. This file does not revive it. This handoff does not itself edit, build or flash firmware; subsequent execution follows the existing implementation authorization and any real current lane HOLD.

Read, in order:

1. `k1-control-r2/K1-Control-Architecture-R2.md`, especially the precision, source-audit and control-ownership chapters.
2. `k1-pipeline-integration/precision-audit.md`, `visual-modularity.md`, `architecture-decision.md` and `K1-Modular-Pipeline-Execution-Brief.md`. Retain AP/VP findings; discard their superseded HMI restrictions.
3. `k1-pipeline-integration/visual-evidence.json`, `source-snapshot/visual-manifest.json` and `evidence/precision/manifest.json`; `k1-control-r2/source-evidence.json` supplies the newer mode/control audit.
4. Current repository instructions and reference manifests before editing. Source snapshots are research anchors, not an instruction to reset working trees to old commits.

Inspected roots and identities:

| Platform | Source root | Research anchor and qualification |
|---|---|---|
| DualMCU | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware` | `67a1cd0b0ecc626b28af2df9b0d11f855b402379`; working tree dirty; bind implementation to fresh affected-file hashes |
| Titan Mini | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware` | `431140853dd8b58af53240ef84d5fb1a08bd8b45`; relevant platform files dirty/untracked at capture; retained import authority `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` must be handled explicitly |

The useful existing work must survive. DualMCU already has signed Q15.16 colour histories; Titan has a genuine wide staging/packing seam and a bounded interrupted-palette-transition mechanism. Both inspected normal musical paths still narrow through Pixel8. Do not erase either strength, claim an entirely byte-only Dual renderer, or mistake Titan's `REPLICATE8` producer for wide musical rendering.

## Ownership and parallel boundaries

| Owner | Owns | Must consume rather than redefine |
|---|---|---|
| VP | Portable wide primitives; typed histories; material integration; fixed stage order; composition and colour-domain rules; optional treatment operators; scalar quantization oracle; per-module storage/cost descriptors | CTL canonical IDs/base values and immutable applied snapshot; AP immutable observations/timestamps; MOD authored mappings and mode behavior |
| MOD | Mode-specific sources/adapters, Liveiness laws, parameter merit, per-mode semantics and admission evidence | VP primitives/history/clock contracts; AP evidence; CTL descriptors and supported/inactive reporting |
| CTL | Parameter/profile schema, validation, response ownership, transaction/revision rules, canonical base state and feedback | VP feature resource/dependency descriptors; MOD capability/mode matrix |
| Platform adapters | Runtime scheduling, selective configuration dispatch, native staging, DMA buffers/cache/topology, capture/presentation receipts | VP portable scene/output contract; CTL validated transaction; AP publication |
| Independent evaluator | Reference generation review, complete-pipeline scoring, physical/optical comparison and acceptance verdict | Frozen candidate and exact claims; never replace expected values with candidate output |

The portable-core steward/integrator chooses one canonical shared patch and records its exact import into Titan through Titan's current reference process. Do not have VP and the Titan agent independently reimplement the same numerical core. Do not import the whole current Dual tree into Titan.

VP writes new portable files under the existing `core/visual/` convention, plus dedicated host fixtures. Suggested names such as `wide_scene.h`, `wide_material.cpp` and `wide_output.cpp` are **proposed destinations**, not verified existing files. MOD writes separate adapter files. Existing `product_effect_renderer.*`, `channel_render_state.*`, `product_output_treatment.*` and runtime wiring are shared integration seams: reserve a single writer; VP/MOD submit reviewed patch manifests for them. Neither agent edits the other's live worktree. Platform driver files remain owned by the platform agent.

## Exact source seams

| Current seam | Evidence and purpose | Required change owner |
|---|---|---|
| Dual `core/visual/product_effect_renderer.cpp:171–211,3513–3541,3607–3653` | Q15.16 units and RGB Bloom history, retained before output fades | VP preserves reference; MOD adapts mode behavior; integrator wires dispatch |
| Dual `core/visual/product_palette.cpp:60–117,139–160` | Float interpolation followed by common byte exits | VP adds wide sampler/output contract without silently changing legacy palette interpretation |
| Dual `core/visual/render_primitives.*` | Fractional coverage narrows before byte accumulation | VP adds scalar-wide primitives; existing compatibility functions unchanged |
| Dual `firmware/rt1062/production_runtime.cpp:788–850` | Treatment, edge policy, gain, current constraint and staging | Platform agent integrates the selected complete wide path; VP supplies exact stage contract |
| Dual `firmware/rt1062/ws2816_pack.h:35–78`; `fastled_parallel_output.cpp:111–122` | Byte replication and bounded A-only/two-half bench route | Platform agent adds/uses wide ingress, preserving transport and topology |
| Titan `src/k1/core/visual/product_effect_renderer.cpp:326–366,2029–2070` | Imported byte history and Bloom | Canonical shared import + explicit compatibility profile; no opportunistic rewrite |
| Titan `platform/ra8p1/palette_runtime.cpp::configure` and `fixture_app.cpp::live_apply_config` | Broad resets on ordinary config changes | Titan/CTL integration owner implements changed-field/reset dispatch before high-rate edits |
| Titan `palette_runtime.cpp:370–397`; `fixture_app.cpp:359–386,400–410`; `hd_pixel16.h:10–42` | Wide staging exists; normal producer lifts bytes | Platform agent connects real wide output; keep lift as named compatibility adapter |

Line numbers are captured-source locators; resolve symbols and hashes in the fresh worktree. An unexplained delta pauses only the affected import or edit while its owner reconciles it.

## Bounded first release and donor backlog

Release the smallest complete instrument that exercises the architecture. Do not turn every interesting donor function into a simultaneous release dependency.

| Admission | Mechanism | Implementation obligation |
|---|---|---|
| First release | Wide scalar composition and late native16 conversion | Finite bounded working values, preserved fractional coverage, explicit blend operators, output contract below |
| First release | One centre-origin pulse/ribbon material | Continuous objects; independently defined transport, diffusion/width and decay; live edits re-anchor state |
| First release | Two explicit history meanings | Scalar intensity recoloured now versus deposited RGB; selector changes meaning through a cut or bounded transition, never memory reinterpretation |
| First release | SuperPixie-inspired immediate-attack persistence | Separate optional local operator; instant rise, specified exponential release, exact identity bypass |
| First release | Palette position/span/travel and interrupted palette transition | Palette identity remains stable; clipped/wrapped sampling declared; no second advancement of the source during a fade |
| First release | Common saturation, warmth and contrast/tone treatment | Each has enable, neutral, units/domain/order, bounded gamut handling and UI context data; colour defaults are identity |
| First release | Bounded tempo-family accent | Consumes AP's admitted phase field; absent/stale field means no accent, not a fabricated tempo; requires AP acceptance before integrated admission |
| Follow-up | Emotiscope scalar Bloom as a continuous sampled field | Separate from object-based ribbons; characterize transport-induced diffusion and cadence before promising independent material controls |
| Follow-up | Emotiscope spectrum/chromagram/VU bodies, Spectronome weighting and Hype parity family | New authored profiles with AP contracts; parity of tempo bins is not percussion classification |
| Follow-up | Image LPF, novelty colour momentum, general scene crossfade and cross-scene afterglow | Separate features with explicit additional latency/history/resource cost; do not enable to hide discontinuities |
| Follow-up | Fadecandy LUT optimization, packed ARM kernels, optional host/server adapter | Scalar parity and measured target benefit first; server OPC is RGB8/untimestamped and cannot replace canonical wide/timed capture |
| Excluded from native16 | External RGB8 residual dithering, donor DMA/USB drivers, automatic donor gamma | May exist only in a separately labelled RGB8 comparison/output adapter; never default into native WS2816 path |

Float is a representation, weighted blending an operator, continuous transforms geometry, and persistence a temporal rule. These are four distinct choices. Emotiscope 1.1 functions commented out in GPU execution must not be represented as released active functionality. Copying donor code requires its pinned source and notice; independent reproduction still names the source behavior. Donor pins: Emotiscope `634b7ed8eefa2f1913ba1a1e7b29c116f22714d4`, SuperPixie `cacbe615aa31a3c8d5a7d7e66286c67a7a7779ce`, Fadecandy `36f616158f195a327f8486474af2956dad52881d`.

## Fixed graph and numerical contract

Use compiled slots and bounded resources; presets choose known implementations and enables, not arbitrary graph code or reorderable filters.

1. Read one immutable audio view and applied configuration at the frame boundary. Resolve each base plus modulation once. AP clocks are read-only.
2. Evaluate three initial layer roles: body, accent and optional atmosphere. A disabled generator contributes exactly zero. Sample count does not create events.
3. Integrate typed material/object state to the actual presentation time. Apply declared centre-space transforms before rasterization. Preserve 160 pixels/channel and centre 79/80; a platform crop does not redefine logical geometry.
4. Rasterize in FP32. Coverage is applied once. A source declares peak-normalized or integrated-contribution-normalized width; widening must not secretly switch normalization.
5. Apply optional **per-layer** immediate-attack persistence. It acts on that layer's authored value before performance gain. No anonymous final-output feedback is reused as artistic history.
6. Compose the layers with the profile's named operator. The first ribbon profile uses weighted body/accent mix plus separately bounded atmosphere; additive, premultiplied alpha-over and componentwise max primitives have independent contracts, not a stack of all three.
7. Apply the fixed optional artistic colour order: saturation → warmth → contrast/tone. Defaults are identity. Palette sampling occurs at the declared history colour seam, not blindly again here. Clamp or compress once at the declared artistic boundary; do not repeatedly round.
8. Apply channel/master intent and the platform's explicit device conversion/constraint policy. Identity device correction is the default. If calibration is nonlinear, power estimation and limiting must operate in the calibrated drive domain, not assume a post-LUT multiplier is linear light.
9. Final output validates finite values, bounds drive, quantizes once, maps logical pixels and transfers immutable complete frames. Blackout and fault policy dominate every artistic tail and background.

Use distinct types/names: `LegacyPaletteCodeV1` → explicit adapter → `WorkingRgbF32V1` → `DeviceDriveRgbF32V1` → `DeviceRgb16V1`. Existing `PaletteLinearRgb` values are normalized legacy codes, not a measured linear-light calibration. Describe the new space as reference light intent until measured. A calibrated profile is a versioned asset/conversion change. Never apply donor gamma merely because a type name contains “linear”; protect the legacy appearance with a separate adapter.

For positive bounded output, define rounding unambiguously as `q(x) = floor(clamp(x,0,1)*65535 + 0.5)`. Pack the platform's established G16/R16/B16 order. Native16 means three 16-bit components, not RGB565. Invalid configuration is rejected before application. Unexpected NaN/Inf in a renderer produces diagnostic evidence and invokes the existing platform invalid-frame/fault policy before integer conversion; it cannot be accepted as a passing frame by silently casting.

Time uses integer media/presentation timestamps with local bounded elapsed seconds. Do not convert growing absolute microseconds to float. For live object material, integrate piecewise parameter segments:

`r += v*dt`, `variance += 2*D*dt`, `log(amplitude) -= ln(2)*dt/half_life`.

At each effective edit time, first integrate the old law to that time, then install the new law for future time. Width edits must define whether they change base variance through a smooth offset or change future diffusion; they must not teleport the envelope. Long gaps use real elapsed time and the declared retire/fade policy; clamping `dt` to a frame-sized number makes time wrong. Retire objects outside their support or below the declared finite-zero threshold. Freeze/resume is an explicit later feature, not the default when disabled.

Immediate-attack persistence in each nonnegative component is `p(t+dt)=max(input(t+dt), p(t)*2^(-dt/h))`; do not then add the entire current input again. It has an explicit hard bypass returning the dry input. Disable clears owned history after any declared wet ramp; re-enable seeds from current dry input and cannot resurrect stale light. Scalar and RGB persistence are different named operators. An optional image LPF would delay attack and therefore requires a separate accent bypass if prompt attack is promised.

Register these simple **candidate scalar colour laws** for the first evaluation, with explicit semantic versions; audition may reject a candidate before release, but tests must not silently change their meaning:

- Palette window: for source coordinate `u∈[0,1]`, sample `clamp(position + span*(u−.5) + travel_offset,0,1)`. Position .5/span 1 exposes the full palette without travel; the host prototype's legal span is .02…1, while the internal primitive can also test span 0 as a single-colour limit. The default clamps; a cyclic palette implementation must advertise wrap explicitly. Preserve the R2 meaning of Palette travel as **excursion depth**, not speed: `travel_offset=.28*travel_depth*q(t)`, depth∈[0,1], where MOD declares the bounded source `q(t)∈[−1,1]` and its time behavior. Travel never rewrites position. A separate advanced travel-rate control requires its own descriptor; do not overload the depth fader. The dashboard draws this exact sampler/window rather than a decorative approximation.
- Saturation: in the declared working intent coordinates, calculate `Y=.2126R+.7152G+.0722B` and `C'=Y+s*(C−Y)` with initial admitted `s∈[0,1]`. Thus 0 is a reference grey and 1 is exact identity. These are reference RGB weights, not measured LED luminance. Above-unity saturation/gamut expansion is follow-up until its negative/gamut policy is admitted.
- Warmth: initial `w∈[−1,1]`, `C'=(R*(1+.2w),G,B*(1−.2w))`; w0 is exact identity. This is a modest artistic primary-gain law, not calibrated colour temperature or white balance. It may change overall brightness; the UI must not claim invariant luminance.
- Contrast: preserve the R2 exponent range `.25…3` with neutral 1. For a hue-preserving reference candidate, calculate the same reference Y, set `Y'=Y^contrast`, and multiply RGB by `Y'/Y` when Y>0; return exact zero when Y=0 and use exact identity when contrast=1. This reshapes the intensity envelope without independently powering each colour component. It is an artistic tone law, separate from device gamma/calibration. Evaluate in a numerically safe order and check the bounds implied by headroom/exponent. Apply the optional registered knee after this curve and before final drive. Do not call a tonal shape a measured display correction.
- Optional knee primitive: `T(x)=x/(1+x)` for x≥0, exact bypass when disabled. Its compression is a named artistic choice with changed highlight level; do not enable it automatically to make an overbright scene pass. The first profile's default knee is off; final drive clamp still applies.

Fixture anchors: saturation 0 of red 1 is `(.2126,.2126,.2126)`; warmth 1 of `(.5,.5,.5)` is `(.6,.5,.4)`; contrast 2 of grey .5 is grey .25; contrast .5 of grey .25 is grey .5. Palette position .5/span .5/u=0 or 1 samples .25 or .75 without travel. These laws provide an executable starting point, not proof of ideal colour appearance; MOD and physical evaluation own usefulness. All controls stay versioned if an alternative law is adopted.

Every artistic feature declares `enabled`, `amount/base`, `supported`, `active`, inactive reason, semantic version, owned state, reset causes and effective value where useful. Amount zero does not clear enable or erase the remembered amount; disable does not overwrite other controls. Physical bounds, output timing, current protection, identity validation and blackout are infrastructure, not optional artistic switches.

## Implementation sequence and dependency gates

| Step | Work and output | GO evidence |
|---|---|---|
| VP-0 | Freeze affected source hashes, legacy fixtures, topology and stage trace; reserve files | Identified current candidate and single writers; compatibility oracle distinct from candidate |
| VP-1 | Agree typed input/output, clocks, colour domains, module descriptors and CTL response ownership; MOD agrees adapter boundary | No ambiguous units or hidden state ownership; schema fixtures compile independently of platform hardware |
| VP-2 | Implement scalar sampler/raster/composition/quantization and analytic material with synthetic AP fixtures | Mathematical rows below pass without a driver; exact bypass and state isolation demonstrated |
| VP-3 | Add history, persistence, palette/colour operators; expose tap traces for evaluator | Declared lifecycle and pairwise tests pass; first profile renders complete timed fixtures |
| VP-4 | Integrate admitted AP family view and MOD adapters through frozen interfaces | AP's freshness/phase contract accepted; optional accent off leaves compatibility AP behavior unchanged |
| VP-5 | Hand canonical source/hash and expected frames to each platform adapter; connect existing wide endpoint | Complete output retains reference fractions after all treatment/limits; native conversion oracle passes |
| VP-6 | Measure candidate under real lane load, then optical/musical audition | Per-platform budgets and complete-pipeline functional/visual criteria pass; no blanket cross-platform promotion |

VP-2/3 proceed using synthetic immutable observations while AP phase qualification or hardware is incomplete. Missing AP acceptance blocks only AP-dependent accent admission. A real target performance failure blocks that target's release; it does not justify weakening the scalar oracle.

## Fixtures, expected results and tolerances

These are **new proposed acceptance thresholds**, fixed before candidate measurement. They are not historical hardware performance claims. Use an independently expressed double-precision oracle for numerical tests; the reference must not call the candidate primitive. For algebraic/analytic FP32 values use `abs(error) ≤ 2e-7 + 2e-6*abs(reference)`; where native quantization lies near a rounding boundary permit at most one UNORM16 code relative to the oracle. Exact identity bypass, integer packing, black and known integer fixtures have zero tolerance. A platform compiler optimization must not use the ±1-code rule to hide an RGB8 bottleneck.

| Fixture | Expected result / pass criterion |
|---|---|
| Two weak white contributions | `0.002 + 0.002 = 0.004`; final native component **262**. A byte-add implementation returning zero fails |
| Weighted colour | `0.25*(1,0,0)+0.75*(0,0,1)=(0.25,0,0.75)`; native `(16384,0,49151)` |
| Premultiplied coverage | Red at alpha .25 over opaque green gives `(.25,.75,0)`; no second alpha multiplication |
| Tone order | For diagnostic `T(x)=x/(1+x)`, two .5 layers yield `T(1)=.5`, not `T(.5)+T(.5)=2/3`; production tone law separately versioned |
| Native identity | `q(0)=0`, `q(.5)=32768`, `q(1)=65535`; `q(k/65535)=k` for full-domain integer test; finite negatives/overrange saturate as declared |
| LUT/bypass independence | Identity mapping preserves all native words; toggling optional temporal interpolation cannot toggle LUT interpolation. Endpoint/monotonic/finite tests cover every compiled LUT entry |
| Speed edit continuity | Start `r=0,v=20`; at .25 s position 5; set v=60, at .50 s position **20**, not 30; no positional jump at edit |
| Diffusion edit continuity | Start variance .25,D=2; at .25 s variance 1.25; set D=6, at .50 s variance **4.25** |
| Lifetime edit continuity | Start amplitude 1, h=.5 s; at .25 s amplitude `2^-0.5`; set h=1 s, at .50 s amplitude `2^-0.75 = .5946035575…`; no relighting |
| Immediate attack/release | Input 0→1 on one observation produces 1 at that evaluation; zero input for one h produces .5; output gain zero after persistence gives exact native black |
| History meaning | Deposit red 1, age one h, then green 1: componentwise RGB persistence is `(.5,1,0)`; scalar max persistence recoloured green is `(0,1,0)`; selector behavior explicit |
| Cadence | Evaluate identical piecewise input at common checkpoints under 30/60/120 Hz and jitter/dropped updates. Analytic states meet numeric tolerance; rendered centroid differs ≤.01 logical pixel when support remains on strip. Raster-boundary occupancy is scored separately, not mislabeled object-position error |
| Long gap | Gaps .1,1,10 s advance or retire against real elapsed time. A expired pulse never resumes at its old radius/amplitude; zero/negative timestamps follow explicit reject/reset policy |
| A/B isolation | Hash/state traces of B remain unchanged by A-only parameter/palette/mode edits, aside from timestamp advancement both would otherwise receive. Shared limiter may alter B output gain but not B artistic state |
| Enable and amount | Off returns exact identity/zero; amount edit preserves enable; enable cycle cannot revive old image; invalid transaction leaves both channels unchanged |
| Complete output | Compare every native word after colour, gain, limiter and pack to the reference. Unequal high/low bytes alone are insufficient because scaled replicated bytes can create them |

A cadence test must inject events/edits at their recorded times, not snap them differently to each render grid. For slowly varying sampled inputs, report interpolation assumptions; do not claim exact cadence independence for an intentionally sampled legacy material.

## Pairwise interactions, limits and optical acceptance

Required pairs: persistence×master/blackout; RGB/scalar history×palette interruption; width×coverage normalization; Liveiness×Motion/Persistence; tone×additive headroom; saturation/warmth×gamut/limiter; optional accent×stale AP; A edit×B running history; feature enable×transition; quiet gap×persistence/base coat. Also exercise the graph-dependent worst cost: both channels active, maximum objects, all admitted stateful operators and one permitted transition overlap. Exhaustive arbitrary toggle combinations are unnecessary; unadmitted graphs must be rejected before mutation.

Use these **initial candidate admission limits**, subject to target proof before release: three layer roles and 32 live material objects per channel; per-role nonnegative component headroom 0…4 and additive accumulator 0…12; no general scene crossfade in the first release. Palette transitions do not evaluate a second effect. History-meaning changes initially use an explicit owner-local cut; a dual-history crossfade is a later admitted resource class. A scene cannot acquire a fourth layer or extra history by merely setting a feature bit.

Reserve eight of each channel's 32 object slots for primary gestures; decorative gestures cannot consume that reserve. At capacity, expire finished objects first, then reuse the lowest-current-contribution decorative object (stable oldest-ID tie break). A primary event with no remaining decorative slot merges into a compatible same-role pulse if its declared law permits; otherwise it replaces the oldest primary pulse and increments a visible visual-capacity counter. AP event accounting remains unchanged. Decorative overflow is rejected with its own count. Freeze this policy with the scene descriptor; do not allow allocator order to choose the result. Capacity stress tests assert the exact admitted/replaced/rejected sequence and bounded output. An audition showing objectionable replacement is a reason to adjust the registered capacity/policy and remeasure, not hide the overflow.

For initial worst-case surface admission, reserve at most six FP32 RGB surface equivalents per channel: accumulator, reusable layer scratch, one typed body-history slot (large enough for RGB), and three optional local persistence slots. These total **23,040 B** for two 160-pixel channels. Two completed native RGB16 publication frames add **3,840 B**, giving **26,880 B before objects, palette-transition state, alignment, controls, stack, AP and platform driver buffers**. Prove scratch liveness; do not alias a retained history with scratch. Unused mutually exclusive storage may later be reclaimed through an explicit lifetime design.

The prior eight-layer/0–32 accumulator and **23,040 B** total-allocation illustration are different research examples, not this candidate's measured memory budget. For 320 pixels, RGB8/RGB16/FP32 RGB frames are **960/1,920/3,840 B**. Keeping the new path alongside the old **46,080 B** SQ history allocation is additional memory unless an explicit union/lifetime migration is proven. No render-time heap allocation. Produce actual `sizeof`, linker section, stack high-water, buffer alignment/lifetime and peak transition-memory reports. If the current target budget cannot admit this envelope, stop its admission, register a narrower supported graph/capacity and rerun relevant proofs; do not silently remove an advertised feature.

The captured Dual source cites an existing **2 ms effect budget**; revalidate its current definition and preserve it if still applicable. Titan's audio hop/deadline and full-load gates come from the current Titan platform brief, not copied RT1062 numbers. Record cycle/time maximum and p50/p95/p99, frame completion/late/rejected counts, audio loss/late counts and stack margin under the admitted workload. A microbenchmark is evidence for one kernel, not whole-image acceptance. No “twice as fast” claim from 8-bit versus 16-bit size. Optimization needs scalar parity and measured full-lane benefit.

Exact-black acceptance: after the declared existing product dwell/decay lifecycle, every emitted component is zero and stays zero for the registered black-hold fixture; no base coat, persistence, boot preview or stale completion relights it. Immediate blackout must force the next admissible final output black, bypass response smoothing. Record command acceptance, render submission and actual transfer/latch times; do not equate a function return with light. Numeric zero is not a camera's black pixel; electrical/optical claims require their corresponding measurement.

For quiet-space acceptance, freeze a scene-specific mask and reference before the trial. The deterministic zero-gap fixture must produce exact zero inside its mask after the **complete** pipeline. For a perceptual ribbon gap with nonzero tails, register a threshold and occupancy-duration limit before audition; capture the mask, counts, peak and duration. Do not invent one threshold as universally perceptually black.

Evaluate rounded RGB8-expanded, dithered RGB8-expanded and native16 from the **same wide scene**, transfer and actual feasible refresh first; a faster comparison is a separate candidate. The captured 80-pixel WS2816 half-line needs about 4.8 ms of payload at 800 kbit/s, plus actual reset/overhead: 400 Hz is not an achievable promise for that configuration. Record installed LED package/revision, firmware, optics, ambient light, current setting, observed cadence and matched brightness. Use dim ramp, short attack, moving edge, low tail, overlapping weak layers and quiet gap; inspect directly under intended diffuser. Camera footage alone cannot establish flicker threshold or effective optical bits. Keep perceptual preference separate from numerical PASS.

## Definition of done and handoff

VP source/host completion requires the full first-profile graph, typed state/lifecycle, frozen descriptors, all mathematical/bypass/isolation fixtures, bounded storage report and canonical patch import manifest. Platform acceptance additionally requires complete-output parity, current lane runtime/loss gates, actual output mapping/blackout/black and no driver ownership regression. Visual adoption additionally requires the controlled physical comparison and explicit preference/tradeoffs. These are distinct statuses; host completion is a legitimate intermediate milestone, not product completion.

Deliver to MOD: stable primitive interfaces, stage ordering, state/clock contracts, approved numerical fixtures and resource caps. Deliver to CTL: feature and parameter descriptors, enables/dependencies, response owner, inactive reasons and effective-state fields. Deliver to platform owners: canonical commit/patch/hash, build inputs, output pixel/ownership contract, exact expected packed frames and resource/transition liveness. Deliver to evaluator/integrator: candidate identity, baseline hashes, raw fixture traces, failed receipts, complete matrix with PASS/FAIL/BLOCKED/NOT TESTED, scope of physical evidence and exact next owner/action.

STOP dependent integration for ambiguous colour/time domains, incompatible source authority, missing required AP contract, nonfinite output, unsafe buffer ownership, failed numerical oracle or current lane deadline/loss failure. Resume after the named repair and only the affected evidence rerun. Continue independent permitted work. This lane grants no automatic merge, product promotion or unrelated hardware changes.


---

<!-- Source: lanes/04-mode-semantics.md -->

# MOD — Mode semantics, Liveiness and control merit

Execution handoff · 24 September 2026 · Status: implementation instructions and acceptance plan

## Objective and authority

Make the controls promised by the new channel-first K1 instrument meaningful across its advertised mode catalogue. Deliver versioned per-mode Liveiness adapters, truthful capability/inactive feedback, palette-led parameter semantics and a reviewed merit decision for legacy controls. Preserve each mode's character, audio evidence and independent user base values. A renamed Mood variable, a read counted by a grep, or random motion added to make a fader visibly do something is not completion.

Full R2 catalogue release requires Liveiness acceptance for **all 23 modes enabled in the captured current catalogue**, plus any further mode admitted before release. During implementation, missing adapters must be shown as unsupported and remain accessible through an explicitly identified legacy/partial profile; they cannot be silently remapped, shown as working or counted as full catalogue completion. Do not quietly disable difficult modes to make the denominator smaller. Changing the release catalogue is a separate recorded product decision.

Use `k1-control-r2/source-audit.md`, `control-architecture.md`, `source-evidence.json` and `K1-Control-R2-Execution-Brief.md`; use `k1-pipeline-integration/visual-modularity.md` for history/feature behavior. The user explicitly asks for **Liveiness**, with this spelling. R2 supersedes old fixed fader/no-page/no-pickup recommendations. The active console is K1.Sliders; the older K1-HMI-BLE-MIDI tree is a donor/reference, not a replacement authority.

Current mode authority was captured from `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware/core/visual/product_catalogue.cpp`, at source head `67a1cd0b0ecc626b28af2df9b0d11f855b402379` with dirty working tree qualification. Recheck the exact file and generated header at execution start. Titan's imported catalogue/renderer must be mapped explicitly; its reference process cannot be bypassed by assuming the sibling HEAD is identical.

## Ownership and source seams

MOD owns mode adapter code, mode-specific parameter usefulness/capability tables, new-version semantic mappings, migration recommendations, per-mode deterministic fixtures and audition notes. VP owns primitives, histories, stage graph, common colour operators and scalar numerical reference. CTL owns canonical parameter IDs, profile schemas, base/applied state, validation and response dynamics. AP owns observation and oscillator truth. Console UX owns page layout and contextual visualization implementation. Platform agents own runtime scheduling and state-reset integration.

Work in isolated mode adapter files following repository conventions; suggested `mode_semantics_v1.*` and `liveiness_adapters.*` names are proposed, not existing-source claims. Reserve existing `product_effect_renderer.*`, `channel_render_state.*`, catalogue registration and runtime dispatch to one integration writer. Submit minimal patch manifests for those seams. Do not modify `TempoTracker`, profile wire IDs, driver code, the common saturation math or GUI layout from MOD.

| Exact captured seam | Why it matters |
|---|---|
| `core/visual/product_catalogue.cpp:10–49` | 23 enabled stable ordinals; preserve disabled IDs and append-only identity |
| `core/visual/product_effect_renderer.cpp:64–65,2032–2070,3005–3012,3607–3673,3710–3796` | Current six Mood consumers and their differing laws |
| `firmware/rt1062/production_runtime.cpp:643–724,797–830` | Control copy is not universal consumption; fallback frame blending uses hard-coded values |
| `core/control/control_catalogue.cpp:19–24,59–64`; `bridge_command_processor.cpp:750–757,810–817` | Legacy A/B Mood and Chroma identities |
| `core/visual/channel_render_state.h:21–31`; `product_effect_renderer.cpp:222–279,299–363,1224–1290,1371–1456,1874–1918` | Separate Chroma scalar and `chroma_value`; palette brightness floor and bypass branches |
| `core/visual/product_output_treatment.*` | No inspected common palette saturation stage; VP must provide the new shared operation |

Bind claims to the hash in `source-evidence.json`; line numbers locate the snapshot and may shift. A source delta is a reconciliation task, not a reason to repeat the entire donor teardown.

## What is already known

Mood is directly consumed by 6 of 23 current modes: Bloom/Bloom Fast use `.25+1.75*m`; Aurora `.8+1.2*m`; Comet captures `.6+2.8*m` when an object spawns; Dense Forge/Dense Forge Chord use `.85+.30*m` for one history drift component. The other 17 have no direct Mood read in the inspected renderer. This supports the user's practical observation, but it is not an optical measurement of every mode or every older S3 image.

There are three distinct Chroma meanings: AP pitch-class evidence; physical legacy Chroma on IDs 4/24; and internal `chroma_value`, whose normal control assignment was absent in the inspected source. Some palette helpers use physical Chroma as an injection brightness floor through `max(energy,chroma)`. It is not a coherent palette-position control. Preserve AP chroma. Remove legacy Chroma from the default Perform page. Retention as a specialist control requires a distinct useful behavior and honest name.

Saturation belongs on Colour/custom pages by user preference. The new shared saturation stage must apply predictably to admitted palette output. Reusing a legacy ID whose selected mode ignores it does not satisfy the requirement. Keep compatibility semantics separately; eliminate double saturation when a new common stage follows a legacy HSV branch through a declared adapter.

## Liveiness semantic contract

Define a versioned normalized intent `L∈[0,1]` with proposed neutral **0.5**. The neutral is part of the new contract, not a claim about old Mood. CTL assigns the canonical ID/version; legacy IDs 5/25 remain old Mood. Store adapter version in presets and return it with capability information.

Each adapter declares:

- Primary perceptual dimension: transport, excursion, release speed, authored event density or bounded variation; optional secondary dimension only when it improves coherence.
- Neutral law, endpoints, monotonic direction for its primary metric, legal bounds and limits. “More Liveiness” must not mean faster in one region and mysteriously slower in the next.
- Input requirements and inactivity conditions. No amplitude/evidence means a music-driven mode may be inactive; it must not generate fake musical observations to demonstrate a control.
- Effective parameter expression, response owner and response time, continuity policy, state touched, AP fields read, UI explanation key and units.
- Whether edits affect current objects through VP re-anchoring or future objects only. Future-only behavior needs explicit UI explanation and perceptual acceptance; the full default Liveiness promise should favor current visible response where compatible with mode identity.
- Measurable primary output proxy and mode-specific functional/perceptual fixture. A metric is a test aid, not a substitute for judging the mode's character.

The initial reusable candidate factor is `f(L)=2^(2L−1)`, giving **0.5 /1 /2** at low/neutral/high. It is a proposed audition starting point, not a frozen universal law. A mode can narrow or reshape this law before its registration gate when clipping, crowding or intent demands it. Freeze the chosen coefficients and acceptance thresholds before scoring the candidate; do not weaken them after seeing a failure.

Compute effective appearance values from immutable bases. Example: `v_eff=clamp(v_base*f(L),v_min,v_max)`. Do not overwrite `v_base`, width, persistence or AP time. To keep edits independent, the mode declares whether Liveiness modulates transport **or** another dimension; a same-time Motion edit changes its base and yields a deterministic effective result. Publish both when helpful. If a base of zero or another limit masks the macro, report `active=false` with a specific reason rather than secretly raising the base.

Do not put parameter damping in both CTL and MOD. CTL owns the effective input response unless a mode declares an essential artistic dynamic of its own; that second dynamic must be visible and have a separate semantic parameter. Immediate attacks cannot be delayed by hidden smoothing. Mode time remains the actual presentation time; a local trajectory rate may be modulated, but global/sample/AP clocks and evidence ages remain true.

## Catalogue work allocation and candidate behaviors

The following is a task matrix, **not a claim that these behaviors are implemented or accepted**. Split work among family specialists after the shared adapter interface is frozen. Each specialist owns its adapter files/fixtures and submits a family patch to MOD; only the integration writer touches central dispatch. Family subtasks may run in parallel within available slots. Do not have one generalist redesign all 23 modes.

| Family / stable mode ID | Current name | Candidate Liveiness dimension to investigate | Must preserve |
|---|---|---|---|
| Material —3 | BLOOM | Future transport rate with continuous current-field/object treatment; distinct from independent lifetime | Centre injection, authored history meaning, coupled legacy profile |
| Material —9 | BLOOM_FAST | Same semantic direction with mode's faster identity retained | No duplicate unbounded speed multiplier |
| Material —12 | AURORA | Flow/excursion rate within existing auroral geometry | Palette envelope, centre-origin extent, quiet space |
| Material —13 | COMET | Re-anchor active velocity where representable; otherwise declared future-spawn law plus useful response explanation | No teleport, extra onset or false AP event |
| Material —16 | EMBER | Ember drift/variation activity with bounded release | Sparse ember character and dark intervals |
| Waveform —7 | WAVEFORM_FAST | Authored envelope release or bounded centre-space motion | Audio sample order; waveform evidence is not sped up |
| Waveform —8 | WAVEFORM | Same user intent with mode-specific range | Waveform sign/order and prompt amplitude response |
| Waveform —11 | WAVEFORM_HYBRID | Movement/release of presentation layer only | Separation of waveform/body and musical accent |
| Waveform —22 | SNAPWAVE | Snap excursion/release activity | Prompt attack; do not blur away “snap” |
| Waveform —32 | WAVEFORM_HYBRID_K1 | Existing K1 gesture motion/release through explicit mode adapter | Current characteristic colour/history and centre behavior |
| Spectrum —14 | SPECTRUM_RIVER | Display transport/release activity | Frequency ordering and measured energy |
| Spectrum —15 | SPECTRUM_RIVER_V2 | Distinct V2 dynamics with same semantic direction | No arbitrary spectrum gain inflation |
| Spectrum —21 | DENSE_FORGE | Stronger meaningful drift/excursion than the small legacy scalar, bounded against clutter | Lattice character and tone evidence |
| Spectrum —24 | DENSE_FORGE_CHORD | Chord-field excursion/settling | Pitch-class relationships; no synthetic chord evidence |
| Spectrum —25 | CHROMA_CONSTELLATION | Constellation motion/excursion or visual persistence | AP chroma remains the source; stable pitch-class identity |
| Rhythm —18 | WAVEFORM_TEMPO | Tempo-locked visual excursion or labelled subdivisions | Waveform time and canonical tempo phase |
| Rhythm —19 | TEMPO_RIVER | Flow excursion/rate controlled within rhythm law | Stable primary beat and declared derived motion |
| Rhythm —20 | TEMPO_COMET | Comet extent/activity per observed/derived event | No duplicate primary beat or altered observed BPM |
| Rhythm —23 | PULSE_PRISM | Pulse excursion/release or restrained secondary expression | Primary attack, palette bounds and centre geometry |
| Rhythm —26 | PERCUSSION_BURST | Burst spread/release/secondary particle activity | Observed percussion class/timestamp; bounded object count |
| Rhythm —27 | TEMPO_COMET_ANTICIPATE | Anticipatory visual extent with explicit predicted status | Prediction never relabelled observation; correction continuity |
| Rhythm —28 | RIVER_SURGE | Surge excursion/release | Authored surge envelope and quiet recovery |
| Rhythm —29 | TEMPO_RIVER_WALK | Walking extent/density within existing beat structure | Beat-count/parity and canonical phase |

Each row ends with either an accepted adapter or a precise unresolved hypothesis. At the intermediate handoff, unresolved rows advertise unsupported. Full catalogue release waits for all 23 rows; compatibility can remain available meanwhile. Changes to disabled ordinals 0, 1, 2, 4, 5, 6, 10, 17, 30, 31, 33–37 are out of first-release scope unless separately admitted. A donor feature backlog does not require enabling those modes.

MOD may delegate four family batches above to specialists, plus one independent cross-family reviewer, subject to the swarm coordinator's slot and file-ownership schedule. A specialist must read the frozen common contract; it cannot redefine Liveiness's neutral, AP ownership or preset versioning locally. Review cross-family coherence using the same physical low/neutral/high gestures and compare whether the perceived direction matches.

## Implementation sequence

| Step | Work | Output / next gate |
|---|---|---|
| MOD-0 | Confirm current enabled catalogue and map Titan parity; preserve legacy vectors and IDs | Exact 23-row baseline, hash, known differences and active file leases |
| MOD-1 | Agree adapter descriptor and effective-value resolver with CTL/VP; choose measurable semantics and registered candidate ranges | Compilable adapter contract and one small representative fixture per family |
| MOD-2 | Implement one representative each: Bloom, Waveform Hybrid K1, Spectrum River, Tempo Comet | Neutral/state/AP preservation and useful candidate response proven on host; no premature full catalogue claim |
| MOD-3 | Parallelize remaining family adapters using the same interfaces; retain modes' distinct authored laws | Complete per-mode implementation and fixture matrix; centralized registration patch |
| MOD-4 | Integrate common colour semantics and merit outcomes | Every advertised control has mode support or explicit reason; no double common/legacy transformation |
| MOD-5 | Run pairwise control/feature checks and complete-pipeline mode fixtures | Exact base/AP isolation, continuity, black/gap and resource behavior; ready for platform load |
| MOD-6 | Target runs plus matched physical audition; freeze versions/ranges; explicit preset migration | Per-mode functional and perceptual acceptance, all 23 accounted for, no unsupported row concealed |

MOD-2 can run against synthetic AP while real tempo-family work is pending. Rhythm-family observed/derived semantics must consume the AP contract before integrated release. VP must freeze material/colour operators before MOD tunes to their output; otherwise tuning results become stale.

## Quantitative and functional acceptance

Proposed new acceptance numbers below are engineering criteria to register before execution, not existing measurements. The platform brief owns timing and loss budgets; MOD cannot pass a bad runtime result by slowing audio or deleting an advertised mode.

| Criterion | Method | Pass condition |
|---|---|---|
| Neutral identity | Same new-profile state/input with adapter bypass and L=.5 | Same effective values; exact integer outputs where bypass promises identity, otherwise VP scalar tolerance at defined seams. Do not compare new wide neutral to legacy byte output and assume equality |
| Base-state isolation | Sweep L through endpoints while snapshotting bases/mode/palette and AP state | Zero writes to unrelated bases; AP feature/winner/phase/epoch sequence byte-identical to consumer-off replay |
| Shared factor fixture | Where proposed factor is used: base 40 at L=0,.5,1 | Effective 20, 40, 80 before declared clamps; inverse/forward mapping agrees; unsupported limit reports honest inactive/clamped reason |
| Current-object continuity | At .25 s after spawn, change L and/or Motion; inspect immediate and subsequent positions/variance/amplitude | No instantaneous position/energy jump from re-evaluating total lifetime; VP re-anchor fixtures pass |
| Response latency | Timestamp applied edit and effective response; use deliberate unsmoothed test and normal response separately | Unsmoothed target takes effect at next scheduled render boundary; normal response matches registered law/time; no extra frame-count or packet-rate dependence |
| Observable response | Fixed suitable fixture at L=.2,.5,.8, non-clipped representative bases; preselected primary metric | Consistent registered direction and a meaningful endpoint difference. Starting target: ≥25% difference in the primary metric between .2/.8 where a ratio is meaningful; modes need a registered absolute alternative for zero-centered metrics. Human audition also required |
| Endpoint stability | Low/high L, legal base extrema and maximum permitted workload | Finite bounded output; no object overflow ambiguity; deterministic rejection/coalescing; quiet-space and output protection remain valid |
| Repeatability/cadence | Same timestamped observations/edits at 30/60/120 Hz and jitter; same checkpoints | Same declared analytic state within VP tolerances; sampled legacy deviations explicitly reference-bound |
| Mode/palette edit isolation | A edits while B retains history, plus repeated identical configuration | Only declared owner resets; B follows identical baseline trajectory; no AP reset; idempotent reapply no restart |
| Enable/amount independence | Liveiness enable off/on and amount endpoints; colour/material enables independently toggled | Off restores unmodulated bases, remembered setting survives; no unrelated switch/amount changes |
| Capability truth | Each mode × page/control descriptor, missing AP/source and both platform capability sets | No advertised inert control; reason distinguishes unsupported mode, no evidence, amount 0, disabled and resource rejection |
| Legacy compatibility | Old presets/CC commands with adapter absent | Legacy Mood/Chroma semantics and Photons floor unchanged; new semantic version never inferred from old numeric ID alone |
| Complete catalogue | Enumerate current enabled catalogue and compare adapter receipts | All 23 captured IDs have accepted version, tests and physical usefulness result before full R2 claim; any catalogue delta explicitly accounted for |

The ≥25% proxy target is deliberately insufficient alone: doubling invisible motion could pass the number and fail the product. Conversely a musically valuable change with a different metric needs that metric registered **before** scoring, with rationale. Preserve failed audition and numeric receipts; tuning creates a new candidate/version and result. Do not rewrite failed runs to match later coefficients.

Suggested representative inputs: impulse pair, sustained tone, sparse percussion, dense broadband section, quiet-to-loud crescendo, tempo ambiguity/ramp, prolonged silence and palette change during an existing trail. Freeze PCM/event hashes and use at least one withheld musical excerpt for final audition. Synthetic AP inputs test semantics; full PCM replay tests analysis-to-appearance; neither proves emitted light.

For every mode, capture the selected palette, L/base/effective values, feature enables, frame time, AP identity, state-reset trace and final image/words. Inspect low/neutral/high through the actual K1 optics at matched intensity and current ceiling. Record whether a user can identify the intended change, whether it helps a performance gesture, and whether it damages mode character. Acceptable written result names a preference and condition; “looks good” with no candidate/setup identity is insufficient.

## Control merit and toggles

Deliver a control-merit table with frequency-of-use evidence, distinct perceptual leverage, mode coverage, predictability, overlap with adjacent controls, accidental-change cost and page recommendation. Use the user's stated experience as product evidence; do not invent usage analytics.

| Parameter | Default decision | Proof or treatment required |
|---|---|---|
| Legacy Chroma scalar | Off the new default performance page | Determine whether useful injection floor remains distinct from Base coat/intensity. If retained, honest specialist name and range; preserve compatibility state |
| AP chroma | Retain as audio evidence | Never delete because the similarly named physical knob is demoted |
| Palette position/span/travel | Primary colour vocabulary | Exact sampler/window/wrap semantics; automatic travel is modulation, not base overwrite |
| Saturation | Colour/custom page | Common VP stage, declared colour/luminance/gamut policy and identity; no hidden duplicate on HSV-derived mode |
| Liveiness | Perform page after adapter admission | Neutral, visible useful direction, supported all admitted modes, versioned macro |
| Motion/Width/Persistence | Perform candidates | Independent bases; Liveiness effective changes visible; unsupported modes honest rather than secretly remapped |
| Rhythm mix/Audio focus | Perform candidates | Mix is authored preference, not detector confidence; focus is visual input interpretation, not AP mutation |
| Intensity versus Photons | Separate semantics | Preserve old Photons minimum and role; new electrical/output intensity obeys actual stage authority |

Every artistic module has an accessible enable even if the default nine faders carry only amounts. Touch controls/editor expose switches; the fader profile must not define amount 0 as the sole disable mechanism. Common colour, palette travel, persistence, body/accent/atmosphere and Liveiness have documented off behavior. Mode/palette selection is explicit, not accidental continuous palette-ID scanning. Required protections and output constraints remain enforced.

Test meaningful interactions: Liveiness×Motion, Liveiness×Persistence, Liveiness×tempo-derived subdivisions, Liveiness×mode change, saturation×palette history, legacy floor×Base coat, and common colour×current limiter. Full-pipeline true black and quiet gaps are VP/platform acceptance requirements MOD must preserve in every mode. A beautiful pre-treatment buffer cannot pass if final persistence fills the gap.

## Migration, integration and definition of done

Preset migration is explicit and versioned. Preserve old Mood values and compatibility renderer behavior. A defensible translation can map a legacy law into an equivalent new effective value only for the named mode/version; record approximation/clamp errors. Where equivalence is not established, keep the legacy appearance and ask the preset to opt into the new semantics through the product's migration flow. Do not silently map old Mood 1 to new Liveiness 1 merely because both use 0…1.

Deliver to CTL: semantic descriptors, neutral/default/range, enable policy, mode support map, context keys, effective-state fields and migration rules. Deliver to VP: each adapter's primitive/history/time requirements, state touched, bounds and required fixture taps. Deliver to console UX: human description, primary dimension, inactive reasons and example base/effective annotations; no layout or wire ID invention. Deliver to platforms: centralized registration patch, imported source identities and per-mode resource costs. Deliver to evaluator: per-mode fixture manifest, independent expected metrics, full low/neutral/high captures, withheld-audition setup and all failed/accepted receipts.

Source done means implemented adapters/descriptors for the intended catalogue and a complete host matrix. Platform done means admitted target behavior, correct mapping/reset boundaries, real full-load performance and output invariants. Product done means **all 23 currently enabled modes** retain identity and offer accepted useful Liveiness, no inert advertised controls, common colour semantics are truthful, page defaults reflect merit, migrations are explicit and every claimed target has its own receipts. An interim unsupported label is truthful progress but not full release acceptance.

STOP only dependent work for unresolved semantic ownership, missing required AP phase truth, neutral/base mutation, undeclared resets, failed appearance invariants or source/import conflict. Resume with a repaired bounded candidate and affected tests. Keep unaffected family work moving. Close with changed paths/commits/hashes, per-row PASS/FAIL/BLOCKED/NOT TESTED, exact next owner/action and current unproven claims; do not let a family agent's local completion become a blanket product PASS.


---

<!-- Source: lanes/05-transport.md -->

# TRN — asynchronous control delivery, BLE/SPI state and migration

## Objective and authority

Carry stable control intent from the nine-fader console to the active engine, and return authoritative state without retargeting, false confirmation or lost final values. Preserve existing BLE-MIDI/v1 peers and the verified DualMCU SPI ownership model. A radio ACK is not an engine application receipt; an engine receipt is not proof that photons changed.

The R2 host backend supplies capture/queue semantics but acknowledges locally and immediately. `autoApply:false` exposes outbound messages; it is not a production ACK/retry client. This lane must implement the asynchronous gap explicitly.

Read CTL's frozen interface bundle, R2 `backend-design.md`, source audit/evidence and captured `contract/bridge_protocol.h`, `contract/k1_deck_state_v1.h`, `contract/k1_deck_state_controller.cpp`, console `k1_session.cpp` and `firmware/s3_ble/ble_identity.h`. Refresh current paths/revisions during preflight. Current authority remains channel-first paged controls with soft pickup; no legacy fixed-bank instruction can justify interpreting a late slot number against a newly selected page.

## Known constraints and exact ownership

| Existing seam or fact | What it establishes | Required treatment |
|---|---|---|
| `contract/bridge_protocol.h` | v1 64-byte maximum payload, 12-byte header, 4-byte CRC; state/command/result types; stale threshold 250 ms and clock-correction fields | Preserve tested v1 layouts. Fit new bounded messages or define negotiated fragmentation/versioning; never assume a host object fits |
| `contract/k1_deck_state_v1.h` | v1 512-byte maximum packet; 71 map entries and 68 snapshot records; hello with registry/layout identities | Preserve coherent snapshot completion and exact compatibility; negotiate new capacities separately |
| K1.Sliders `src/console/k1_session.cpp` | Session epoch changes on connection state; confirmed cache separate from local command note | Extend through async client; local note remains requested/pending, not accepted proof |
| `firmware/s3_ble/ble_identity.h` | Console identity gate with reserved K1C24/0xC024, profile/layout binding and CRC; Deck16/ENC8 compatibility | Preserve device identity. Decouple new editable profile identity by an explicit version extension; do not spoof old layout hash |
| `scripts/generate_console_contract.py` | Generates endpoints from authoritative registry/profile | CTL owns generator and schema output; TRN consumes sealed export |
| DualMCU SPI link | Existing READY gating, descriptor ownership and independent codec evidence in prior lane | Integrate through current driver. Do not reintroduce deprecated UART or revise DMA ownership incidentally |
| Titan target | Same desired semantic model; transport parity is not established by this packet | Provide adapter to its actual admitted host/control path. Do not pretend DualMCU BLE/SPI is already deployed on Titan |

TRN owns codecs, version negotiation, client/server session state, bounded scheduling, retry/ACK processing, snapshot assembly and transport diagnostics. CTL is sole schema/registry source editor; propose required fields before freeze. Engine lane owns canonical mutation/response/fault behavior. HMI owns physical sessions, pickup and UI truth. Hardware deployment is performed by the designated platform/deployment owner under one bench lease, not concurrently by TRN and another agent.

Suggested new files are **proposed paths**, to place after inspecting local conventions: `contract/control_v2/transport_vectors/`, `core/control/control_session.*`, `firmware/s3_ble/control_v2_transport.*`, and console `src/console/control_client.*`. Existing integration paths such as actual SPI driver implementations were not captured here; resolve their exact names from the current tree before modifying them. Record the target seam and ownership in the worktree manifest.

## Protocol and state machine

Use CTL's `ControlCapabilities`, `GestureIntent`, `ControlTransaction`, `ApplyReceipt`, `StateSnapshot` and `EffectiveObservation` vocabulary. Production numeric IDs and compact widths come from the frozen registry, not this brief.

A session progresses through disconnected → identity checked → capabilities matched → coherent snapshot received → live. Reconnect creates a new session epoch, clears stale transport work and rearms the HMI from engine state. A page change is local navigation; it is neither a reconnect nor a new engine parameter value.

The producer captures semantic target, channel/global scope, profile/binding revision, gesture, sequence and session identity before queueing. Frames may contain stable compact IDs, but downstream recipients never need the current screen page to know the target. The engine checks epoch/version/capability/type/range/revision and returns a receipt containing the actual accepted base. Its effective value has separate timestamped observation semantics.

Keep three independent status layers: transport queued/delivered, engine accepted/rejected, and effective response/active feature. A pending marker may clear only on matching application evidence or explicit terminal failure. Rejected input retains the engine's accepted value and a reason. A timeout is not success, and a resync snapshot must be correlated to a sufficiently recent engine revision before it resolves pending work.

### Continuous and transactional traffic

Coalesce intermediate absolute values by captured target plus stream/gesture identity. Never key by current physical slot. Preserve sequence order for discrete actions, relative deltas, enable toggles, preset transactions, profile commit and terminal intent. A later accepted gesture or explicit override on the same target supersedes an older pending terminal value; an old retry must not resurrect it.

Each accepted terminal intent must reach one observable disposition: applied, superseded, rejected, expired, cancelled by an explicit reset/override, or failed awaiting reconnect/resync. No silent queue eviction counts as final delivery. Queue bounds, in-flight maximum, aggregate rate and fair scheduling are negotiated/configured from measured links. The host reference's 50 Hz per target and 30-target queue are not BLE bandwidth qualification; nine active faders cannot simply multiply a per-target rate without budgeting.

Priority: existing emergency/fault authority and blackout intent, transaction completion/terminal values, ordinary continuous updates, coherent state repair, then optional effective-value telemetry. Prevent lower-priority starvation with bounded quotas; no arbitrary telemetry stream may prevent commands or health traffic. The precise scheduler weights, maximum batch count and retry timings are fixed in the capacity/budget artifact before bench acceptance.

### Idempotency, staleness and clocks

Retry an idempotent operation with its original identity and semantic content. The receiver retains a bounded duplicate/result cache for the admitted in-flight/retry window; duplicate bytes return the original outcome without a second mutation. After validating peer/session identity, look up a retained duplicate outcome before treating its old sequence as an ordinary stale command; otherwise a lost receipt would make a successful retry indistinguishable from an unapplied rejection. Reusing an identity with different content is rejected. Relative deltas and discrete actions require the same at-most-once application guarantee; “retry with a fresh command ID” is forbidden.

Transport delivery itself is not exactly once. At-most-once application plus explicit outcomes/reconciliation is the achievable contract. Cache eviction/window expiration must cause an expired/unknown outcome and snapshot reconciliation, never speculative reapplication.

Preserve original gesture event time separately from relay/transmit time. The captured v1 250 ms stale window cannot simply be reused for arbitrarily delayed finals or large profile fragments. Freeze a validity policy per message class: live movement expires; a deliberate persistent profile/preset transaction has a bounded transaction lifetime; retries do not silently renew old musical intent. On expiry, resolve the UI and request a fresh action or reconcile; do not update timestamps to make stale work look fresh.

Measure latency within one clock domain where possible. Cross-MCU latency claims require recorded synchronization offset/error, or a hardware marker/capture. Wrap handling for boot IDs, sequences and monotonic clocks must be explicit. New engine boot invalidates previous snapshot/receipts even if a revision number happens to repeat.

### Snapshots and live deltas

Stage a snapshot by identity/epoch/revision, declared item count/length and checksum; publish only after a valid end. Missing, repeated conflicting, malformed or mixed-epoch items reject that staged image and request resync while retaining the last coherent display state with stale indication. A snapshot is bounded and never borrows mutable render storage.

Handle a valid single live revision increment promptly. Do not wait for a heartbeat solely because a command caused one revision increment. During snapshot assembly, stage/replay compatible newer deltas after commit or restart from a newer complete snapshot under a defined bounded policy. A replay of a live delta's revision must not trick the client into publishing a partial snapshot. Add regression vectors for the existing “single step versus full replay” fix rather than assuming it survives a new protocol.

Effective telemetry is optional, decimated and freshness-labeled. It cannot overwrite accepted base state or keep a context dashboard alive as if a person moved a fader. Engine capabilities determine whether a material-level value exists; unsupported observations report unavailable.

## Ordered work packages

### TRN-1 — negotiate a feasible contract and budget

Before CTL freeze, produce actual payload-size accounting for every message class and its maximal admitted transaction/snapshot. Include BLE ATT/notification fragmentation, negotiated MTU behavior, bridge framing overhead, queue memory, task cadence, SPI service time, radio connection interval and aggregate nine-fader traffic. Use source values and measurements where available; mark unmeasured inputs. Do not choose MTU, page count, record count or per-frame payload from wishful host defaults.

Submit required fields/limits to CTL. Freeze numeric packing, endian, signedness, rounding, forbidden/reserved values, integrity coverage and unknown-field/version handling. A profile not admitted by both engine and console fails negotiation visibly; old peers continue their valid v1 mode. Unsupported new controls never fall through to arbitrary MIDI CCs.

### TRN-2 — independent codecs and async host simulator

Implement encoder/decoder plus an independent oracle that does not share production parsing helpers. Exercise minimum/maximum sizes, all optional flags, truncated/overlong frames, bad CRC, duplicate/old/future epochs, endian boundary values, invalid enum/type, NaN/Inf where float is permitted, revision rollover and mismatched identity. Match exact generated vectors before platform integration.

Build a deterministic fault-injection transport simulator around the production async client/server state machines: delay, drop, duplicate, reorder, split fragments, transient disconnect, reboot and slow receiver. Preserve original seeds and full timestamps. The simulator must verify correctness under faults, not claim physical radio reliability from simulated success.

### TRN-3 — session/scheduler/replication implementation

Implement bounded pending/in-flight queues and idempotent outcomes. Add reason-coded counters and high-water marks for each relevant boundary. Do not count intentional intermediate coalescing as unexplained packet loss. Admission failures, expired requests, unknown apply outcome, parser drops and state gaps remain distinct.

Integrate producer-local pending state, engine receipts and snapshots. Reconcile conflicting external edits using CTL's per-target revisions and lease policy. Navigation does not cancel an already valid old-target command by accidentally changing its address. A profile transaction retires old-profile sessions in a coordinated commit; incoming old-profile frames are rejected after the boundary.

### TRN-4 — adapter integration and regressions

On DualMCU, route through S3 BLE → existing SPI command path → RT1062 authority → SPI state → BLE state client. Preserve READY/descriptor ownership and current zero-fault link gates. On standalone S3, bind the same semantic apply interface locally without simulating an unnecessary secondary MCU. On Titan, implement only its selected actual control transport and publish its capabilities; test the semantic adapter even if its physical radio companion is a separate milestone.

Reuse existing identity service and v1 peer acceptance. The K1 console remains K1C24/0xC024. Editable profile identity becomes separate from hardware/product identity through a negotiated extension; do not regenerate a fake immutable hardware identity after every page edit. Compatibility tests include old console ↔ new radio/engine, new console in legacy mode ↔ old radio/engine, fully new chain, mismatched catalogue and interrupted upgrade.

### TRN-5 — measured load, deployment and recovery

Run controlled baseline and candidate loads with audio/VP/display/radio active and all nine faders represented. Instrument producer qualification, enqueue, engine apply and returned confirmation; freeze the traffic pattern and clock method before measurement. Use separate normal-load and deliberate-fault runs; injected corruption should increment the appropriate rejection counter rather than satisfy a normal zero-error run.

Coordinate all device operations through the platform owner. Capture before/after firmware identity, active protocol/profile, engine epoch, queue/counter baselines and physical leave-state. Do not flash an image built for a similarly named board or infer the active binary from a repository checkout.

## Acceptance metrics and evidence

The existing payload/count/identity facts above are source observations. The following latency and stress envelopes are **new proposed requirements**, not results or pre-existing bench gates. Freeze a feasibility record before testing. If current hardware cannot meet them, expose the tradeoff before the candidate run; never relax a threshold in response to a failed run.

| ID | Method and conditions | Pass condition | Required evidence |
|---|---|---|---|
| TRN-A01 | Independent encode/decode vectors and malformed-frame corpus | Byte-exact match for all valid vectors; every invalid case rejected without state mutation/buffer violation | Oracle source identity, vectors and raw results |
| TRN-A02 | Mixed pages/channels with delayed, duplicate and reordered intents | Zero retargeted writes; duplicate action/delta applied at most once; stale epoch/profile never accepted | End-to-end target/identity/revision log |
| TRN-A03 | Healthy-load all-nine-fader run with optional telemetry/display/audio/render active, at admitted aggregate rate | **New requirement:** qualified movement to engine acceptance p95 ≤50 ms and p99 ≤100 ms; clock uncertainty reported and included in conservative bound | Raw latency samples, actual sample count, percentile method, clock evidence |
| TRN-A04 | Gesture termination under coalescing/backpressure; then link healthy | **New requirement:** latest unsuperseded terminal outcome confirmed ≤250 ms after healthy service resumes, provided intent is still valid; expired intent explicitly resolved within the same bound | Per-gesture terminal disposition ledger |
| TRN-A05 | Deterministic fault run: 10,000 operations, seeded 10% packet drop, 5% duplication, bounded 0–200 ms delays, reorder and five 2-second outages | Zero wrong-target/partial/duplicate state effects; every admitted terminal has explicit outcome; coherent state convergence ≤2 seconds after link restored and old-expired work resolved | Fault schedule, seed, complete outcome/convergence report |
| TRN-A06 | Max snapshot/transaction split at every legal fragmentation boundary; live delta interleave/replay | No partial snapshot publication; correct count/checksum; single live step applies without heartbeat dependency; one coherent epoch/revision at client | Packet fixtures and published-state trace |
| TRN-A07 | Required v1/new mixed-version matrix and profile/layout mismatch | Retained v1 semantics byte-exact; new semantics unavailable on old chain; wrong identity/digest refused; no automatic CC reinterpretation | Version/identity matrix and receipts |
| TRN-A08 | Queue/capacity maximum and overload, producer faster than admitted service | Bounded memory and work; high-water marks within declared capacities; intermediate coalescing explicit; terminal/discrete loss never silent | Allocation map, high-water counters, overload trace |
| TRN-A09 | Nominal concurrent-load bench run at frozen duration/rate | Existing lane audio/deadline/DMA/link gates unchanged; zero unexpected CRC/sync/overflow/state-gap events in normal run | Platform candidate receipt and full raw counter deltas; missing counters fail the claim |
| TRN-A10 | Reboot each peer separately; disconnect while pending; change page before reconnect | New epoch/snapshot required; zero stationary-position replay; pending state honestly resolved; pickup rearmed; no old boot receipt clears new work | Reconnect matrix and HMI/engine traces |
| TRN-A11 | Lease conflict, preset override, stale expected revision and final retry from previous writer | Bounded owner release; explicit conflict outcome; no post-override resurrection; independent A/B work not globally serialized by false conflicts | Multiwriter scenario log |
| TRN-A12 | Upgrade then rollback with saved legacy and new profiles | Producer-first rollback prevents new traffic into old consumers; complete recoverable data retained; final selected profile/state verified | Image hashes, migration/rollback log and leave-state |

For TRN-A05 use a fixed public seed set chosen before candidate testing; failures remain in the receipt. The 10,000-operation campaign is finite. Add another run only to test a concrete correction or remaining risk. TRN-A09 duration comes from the platform integration gate; do not replace an existing stronger endurance test with a shorter ad hoc transport run.

## Deployment order and rollback

Perform compatible-consumer deployment before enabling a new producer:

1. Seal legacy recovery images/configuration and the current authoritative state snapshot. Record exact console, radio and engine identities and current hashes.
2. Deploy engine support that accepts both retained v1 and frozen new semantics, with new capabilities withheld until state/apply self-checks pass. Existing controller continues its legacy path.
3. Deploy the radio/bridge that understands both paths and negotiates actual engine capabilities. It must not advertise new support while its engine is old, disconnected or incompatible.
4. Deploy console async client and paged UI last. Start snapshot-first; require matching identity/capabilities/catalogue and a coherent snapshot. Install/activate the new user profile as an explicit complete transaction. No nine-position replay.
5. Verify nominal control, page switches, user settings, authoritative state and full-load gates on the exact resulting binaries. Enable optional effective telemetry only within measured spare capacity.
6. Record final running versions, selected channel/page/profile, physical fader baseline and pending/error status. A mixed-version chain that exposes unsupported controls is not release-ready.

Rollback reverses *traffic enablement* first: return the new console to admitted legacy mode or disconnect its new command producer; stop new transactions, drain or explicitly cancel them, and verify engine state. Then revert radio and engine as needed with scoped recovery images. Retain all new profile data in its versioned namespace. Restart through fresh epoch/snapshot; never reuse an old cached confirmation. If the rollback cannot preserve the current new semantic preset, report that difference explicitly and restore a pre-recorded compatible preset—do not invent a numeric Mood/Liveiness translation.

Flashing is an execution activity governed by the user's existing authorization and any actual live HOLD; this document creates no blanket no-flash rule or fresh approval ritual. Unknown device identity, occupied bench ownership, failed prerequisite or mismatched image stops only the affected deployment until resolved.

## Definition of done and downstream handoff

TRN is done when the production asynchronous path—not merely the host reference—has passing codec, ordering, retry, snapshot, compatibility and capacity evidence; measured normal-load requirements pass on each advertised physical path; terminal values have truthful outcomes; and consumer-first upgrade/producer-first rollback are demonstrated. A target without a physical adapter is marked NOT INTEGRATED and cannot be claimed complete because another target's adapter passed.

Deliver exact changed paths/commits, frozen schema digest, wire fixtures/oracle, negotiated capacity and scheduling record, all test receipts/raw traces, compatibility matrix, known unsupported paths and recovery/leave-state. HMI receives the async status/receipt API; engine owners receive decoded transactions and conformance vectors; the integrator receives the matrix proving which complete chains are deployable. No single transport agent owns the entire product release.


---

<!-- Source: lanes/06-hmi-input.md -->

# INP — physical faders, takeover and bounded gesture processing

## Objective and authority

Deliver the production input implementation for the active **K1.Sliders 2.41-inch console**: nine physical faders safely address the selected Primary/Top or Secondary/Bottom channel and selected Perform, Colour or admitted custom page. A page change must never overwrite recalled values or redirect an old gesture. Every advertised mapping, range, response and takeover setting must work on real faders, not only in the host demonstration.

This is an execution instruction for a specialist lane. It does not claim the implementation or its hardware tests have already happened. The swarm integrator assigns an isolated worktree and a single console platform owner; INP owns only the surfaces below. Run the lane to tested closeout, rather than returning another plan.

Read `k1-control-r2/control-architecture.md`, `backend-design.md`, `source-audit.md`, `control-core.js`, `control-core.test.js`, `Control-Profile.json` and the source hashes in `source-evidence.json`. R2 is the architecture authority; its JavaScript is a behavior reference, not production firmware. Its 26 host checks establish useful regression cases, not hardware qualification. The old fixed Appearance9/no-bank/no-pickup proposal is superseded.

Active repository: `/Users/spectrasynq/Workspace_Management/Software/K1.Sliders`. Captured head `2f17b8a967de64a4088cc4e6976ddcf16922bb76` is only an anchor: inspected files included dirty changes, so compare current content and hashes before editing. `K1-HMI-BLE-MIDI` is an older 1.91-inch donor; do not copy its pinmap or assume its minimal sketch is the active console.

Proven source inputs: `src/console/pins.h` declares nine faders, 5,000 µs scan period, 600×450 display, masked discrete pads and a nominal 12-bit ADC scale. `main.cpp` polls screen touch every fourth scan; screen touch does **not** prove slider-cap touch sensing. `k1_profile.h` currently uses one 80-count CC14 movement band and 20 ms continuation cap. `main.cpp::emit_k1()` indexes a fixed generated map. The revised implementation must replace that fixed-slot assumption through stable captured bindings.

**Calibration policy is separate from mapping customization.** Captured `pins.h` explicitly records a 19 September instruction banning the calibration wizard and using the factory 0–3.3 V map; `main.cpp` rejects `c/C`. Preflight must check whether a later recorded user instruction supersedes that policy. Until such evidence is found, preserve the factory map and disabled wizard. Read-only travel/noise characterization, user parameter ranges and input filtering remain in scope. Do not silently restore touch-hold calibration or overwrite stored calibration merely to satisfy a test.

## Ownership and interfaces

| Surface | Owner and action |
|---|---|
| Existing `src/console/k1_profile.h` | INP; preserve explicit v1 compatibility helpers while replacing fixed-profile processing for the new negotiated contract |
| New `src/console/input/control_input.h`, `.cpp` | INP; bounded movement, binding capture, takeover and mapping state machine |
| New `src/console/input/input_characterization.h`, `.cpp` | INP; bounded diagnostic counters/capture hooks, disabled or low-overhead in production |
| New `test/control_input/` | INP; native deterministic fixtures and replay of captured ADC traces; discover repository test conventions before selecting runner |
| Existing `main.cpp`, `display.cpp`, `k1_session.cpp`, `snapshot.h`, build configuration and task wiring | Exclusive console platform owner. INP supplies an integration patch/specification; do not edit these concurrently with UI or transport |
| `slider_channel.h`, mux and settings implementations | Inspect their actual paths; change only through an explicit ownership transfer if the current primitive prevents the required behavior. No guessed path is authorization to rewrite them |
| Generated catalogue/profile headers and profile compiler | CTL lane; never hand-edit generated IDs/tables |
| `ui.cpp`, UI lens/editor modules | UI lane; consume immutable input feedback, never mutate raw ADC or takeover state directly |

The names of the new files above are designated implementation targets, not claims that they already exist. Freeze the following shared C/C++ interfaces with CTL/UI/transport before compiling consumers; one CTL owner writes the common definitions. The records below specify required fields, not new wire opcodes:

- `PhysicalSample`: stable fader ID, raw sample, normalized physical position, monotonic observation time and sensor/calibration identity.
- `ActiveBinding`: physical ID, stable semantic target/version, channel, page/profile identity and revision, binding revision, compiled mapping/range/quantization, takeover policy and response descriptor reference.
- `GestureToken`: client/session epoch, gesture ID, captured target, binding/profile revision and monotonically increasing sequence. No consumer resolves a token using the currently visible page.
- `ControlIntent`: token, typed canonical value, observation time, terminal flag and expected authority/revision fields from CTL. Intermediate values are replaceable only within the same target/gesture; final values and discrete actions retain their semantics.
- `InputFeedback`: physical position, pickup state/direction/ghost, requested value, accepted base, effective value when available, pending/rejected/inactive reason and observation age. Accepted state comes from the engine receipt, never a local send.
- Navigation/profile commit arrives as an explicit event. INP returns a new immutable binding view and cancellation/rearm status. It does not let LVGL callbacks edit live tables in place.

Audio analysis, color arithmetic, rendering, wire packet allocation, full profile parsing, NVS transaction format and UI drawing are outside INP ownership. CTL compiles profiles outside real-time paths. The console consumes fixed-capacity admitted tables; it must not parse JSON per scan or render callback.

## Implementation sequence

### INP-1 — establish the actual input surface and budget

Inspect applicable repository instructions, dirty paths, installed board identity and current build procedure. Preserve unrelated work. Record active slider/pad masks, factory/calibration policy, scan/touch scheduling and the existing queue depth. Do not enable absent headers/pads or infer touch on slider caps.

Capture raw and filtered samples for all nine faders at rest near bottom, middle and top, for at least ten seconds per position under idle and full display/BLE load. Capture three full up/down traversals per fader. Record extrema, span, settling time, median, robust noise estimate, peak-to-peak noise and timestamp gaps. A nominal 12-bit ADC encoding does not establish 12 noise-free bits; report measured usable steps and how estimated. Scaling to CC14 or uint16 adds no sensor information.

Use these traces to choose movement qualification, pickup tolerance and a quiet interval. Freeze values and rationale before candidate acceptance. The host reference's 1.5% pickup tolerance and 0.2% hysteresis are example values, not measured hardware constants. A large tolerance that hides poor input quality is not an acceptable silent repair. Publish the real limits and repair the affected input path if the required control cannot be played reliably.

Timing budget must list worst observed scan execution, scheduling gap, touch-poll cost, input mapping cost, queue capacity and memory high-water marks. Preserve the captured 5 ms scan period unless the platform owner explicitly approves a measured scheduling change; do not invent a display deadline from the nominal polling interval.

### INP-2 — implement movement sessions and navigation boundaries

Use these observable states per physical fader: `UNSYNCED`, `QUIET_WAIT`, `PICKUP`, `ACTIVE`, `FINAL_PENDING`, with explicit invalidation reasons. Name variants are allowed; transitions and tests are mandatory.

1. On startup/reconnect, acquire a complete compatible authoritative snapshot. Remember physical positions without transmitting them. Enter quiet/rearm handling before a fresh gesture can own a target.
2. A session starts only on qualified movement after the required quiet boundary. Noise or a touchscreen press elsewhere does not start it. Capture immutable binding/target identity at this boundary.
3. With default soft pickup, show the physical-to-base offset. Emit no parameter change until the current physical segment crosses the inverse-mapped base target within the measured tolerance. Detect crossings across samples, including a fast move that skips over the target.
4. During ACTIVE, feed the compiled mapping and coalescer. A pause shorter than the quiet interval continues the session. After the declared quiet interval, deliver the final settled target, then close the session. Suppressing tiny noise must not discard a legitimate final step.
5. On channel/page/profile change, cancel future samples carrying old gesture tokens. Already admitted or queued old work keeps its original target; preserve its last legitimate final value. Enter `QUIET_WAIT` for continuing physical motion. Only motion following a new quiet boundary may start a gesture on the newly selected target.
6. Reconnect, authoritative external edits, preset override or a new profile revision re-arm affected takeover states using CTL's conflict policy. Do not resume an expired gesture or replay idle positions.

If physical quiet is a poor release proxy in a demonstrated interaction, an explicit deferred-navigation mode may apply the requested page when movement ends. UI must show that selection is pending. Choose and freeze one default policy; do not silently alternate between immediate remap and deferred remap. A screen tap does not prove that the performer released the physical fader.

### INP-3 — separate control resolution, mapping and dynamics

Processing order: hardware normalization → qualified physical movement → takeover → inverse/forward compiled range and monotonic curve → canonical quantization → captured typed intent. Keep sensor-noise filtering separate from artistic response.

Support every advertised descriptor: linear, logarithmic over a valid positive domain, reviewed exponent/monotonic curve, inversion, user subrange, continuous or stepped output, endpoint reachability and explicit fine-range/relative-trim operation. Exact minima and maxima remain reachable even when step size does not divide the range (regression: step 0.3 over 0…1). Mapping and inverse mapping must agree within one admitted input step or the declared canonical quantization error.

Absolute fader acceleration is not an invisible speed-dependent mapping. If relative trim is admitted, it is a named mode with explicit anchor, sensitivity, acceleration law, reset and display feedback. A normal absolute fader remains deterministic. If a current base value lies outside a newly edited user range, preserve the engine value and show it outside the range; only an explicit set/recall action may change it.

Offer pickup by default and explicit jump/value scaling when requested. Value scaling must reach both endpoints or fall back visibly to pickup at a degenerate physical anchor. Never present a policy that makes part of the configured range unreachable.

Response damping/easing/slew has **one canonical owner**, normally the engine effective-value stage designated by CTL. INP sends the base target plus a descriptor reference; it does not add an undisclosed second smoother. A noise filter is not an artistic damping control. Use true elapsed time for any owned response and declare suspension/long-gap behavior. Master blackout follows the authoritative immediate path and bypasses artistic response.

### INP-4 — profile edits, persistence and bounded transport

UI submits a complete draft to CTL validation/compilation. INP accepts an immutable compiled revision only after atomic admission. No half-applied range, page or mapping may become visible. A rejected draft must leave current values, gesture tokens, pending intents and profile revision unchanged. After accepted remapping, physical positions stay unchanged and affected controls re-arm. Custom pages work through the same machinery; do not hard-code two pages as the production limit.

Coalesce by semantic target plus gesture, not fader index. Preserve final targets under queue pressure, retries and page changes; do not generate unbounded work when all nine move. CTL/transport owns ACK, retry, expiry and epoch rules. INP exposes backpressure and never labels enqueue success as application. Calibration, HMI profile, appearance preset and device color calibration remain separate persistence domains. Do not write NVS on each ADC sample.

### INP-5 — integrate and test on the console

Provide the console platform owner with the exact call sites and ordering required in `main.cpp`: sample → input engine → bounded intent handoff → immutable feedback snapshot, plus navigation event ingestion outside LVGL's drawing callback. The platform owner owns task/lock changes and builds the combined console image. INP reviews integration for races and runs native trace replay before physical testing.

Console build/upload commands were not captured in the R2 snapshot. Discover the current build instructions, available environment and target port in the actual checkout. A template such as `<verified-build-tool> <verified-console-environment>` or `<verified-upload-command> <verified-console-device>` is **not an executable instruction** until resolved and recorded. Do not copy DualMCU, Titan or old 1.91-inch deployment commands. Existing authorized implementation/testing includes flashing unless an actual current HOLD applies; preserve and report target identity, image hash, configuration and leave-state.

## Acceptance and evidence

These are candidate acceptance requirements, not measured results. Freeze the shared timing metric definitions with IVV before measuring; a stricter platform gate also applies.

| ID | Test and conditions | Required result and evidence |
|---|---|---|
| INP-A01 | All nine real faders: bottom/middle/top rests and three full traversals; idle and display/BLE load | Raw/filtered trace, masks, endpoint reachability, usable resolution and noise report for each fader; no synthetic rescale called extra ADC bits |
| INP-A02 | Ten minutes with all physical faders stationary under admitted full console load | Zero phantom accepted parameter intents and zero unintended lens triggers; report actual samples and duration, not only an empty event file |
| INP-A03 | Each fader, both channels, Perform/Colour plus at least one created custom page; stored target deliberately offset from physical position | Zero writes on selection; pickup ghost/direction correct; no change before capture; segment crossing captures; opposite channel unchanged |
| INP-A04 | Rapid navigation during ongoing motion; delayed old packets; reconnect and external edit | Zero retargeted events; old legitimate final target retained only on original target; continuing movement cannot acquire new binding before quiet/rearm |
| INP-A05 | All admitted range/curve/inversion/step/takeover types, including endpoint and out-of-range cases | Forward/inverse agreement at stated precision, exact legal endpoints, no silent clamp on opening editor, explicit value-scaling fallback; deterministic fixture receipts |
| INP-A06 | One invalid field in multi-field profile edit; interrupted save/import; compatible and incompatible catalogue | Atomic rejection/restoration; no partial active state. Valid custom page survives restart without altering physical calibration or appearance preset |
| INP-A07 | All nine controls move with queue pressure; stop between continuation slots | Latest intermediate state coalesced; final targets delivered or explicitly failed with reason within transport contract; no silent final-value loss |
| INP-A08 | Healthy link and captured controls; qualified motion → engine accepted base | Shared product target p95 ≤50 ms and p99 ≤100 ms, measured from matched timestamps with clock uncertainty reported; pickup waiting, packet ACK and optical latency are separate claims |
| INP-A09 | Full-load console run with UI, touch, BLE and all faders | No input task failure, queue corruption, undisclosed dropped final commands, or monotonic post-warm-up memory loss; required platform deadlines and reserve budgets met |
| INP-A10 | Response controls and mode changes | Exactly one artistic response owner; base/effective distinction retained; navigation changes neither AP time nor the other channel; blackout uses immediate authority |

Hardware evidence includes console serial/product identity, source/working diff hash, firmware hash, profile/catalogue digest, peer versions, actual test settings, raw timestamped traces and per-test PASS/FAIL/NOT TESTED. Host success alone does not pass INP-A01–A03 or device integration.

## Gates, done and next owner

GO to implementation after CTL freezes record meanings and a file owner is assigned. Continue input trace characterization and native tests while waiting for the wire adapter. Stop only the dependent integration when profile identity, state ownership or physical target identity is unresolved; resume after the owner publishes the corrected dependency and matching tests.

GO to hardware execution after the console platform owner has a buildable combined image, compatible applied-state adapter and verified board selection. On a target mismatch, invalid required snapshot or failed profile admission, preserve physical positions and applied engine state, show the reason and suppress affected writes. Existing faults and blackout authority remain intact.

**INP is done** when owned production changes are committed, the console platform owner has integrated them, all mandatory INP criteria pass on the declared physical configuration, and IVV can reproduce the meaningful cases from the delivered packet. A mock, uncompiled patch, fixed two-page proof or passing browser slider is not completion. A deliberately unsupported feature must be omitted from the advertised profile; it cannot be called done under a working-looking label.

Deliver code commit/diff, integration patch instructions, frozen constants/budgets, characterization data, native tests, hardware receipts, known bounds and exact running leave-state to the console platform owner and IVV. UI receives the stable feedback/cancellation contract. CTL receives any compiler constraint discovered. State the next action and dependent gate explicitly; do not ask a single integrator to rediscover the implementation.


---

<!-- Source: lanes/07-hmi-ui.md -->

# UI — channel-first instrument, contextual dashboards and control editor

## Objective and authority

Deliver a real **600×450 touch interface on the active 2.41-inch K1.Sliders console** that makes nine physical faders immediately useful and makes deeper control discoverable. Primary/Top and Secondary/Bottom are the top-level scope; Perform, Colour and admitted custom pages follow. Every page assigns all nine faders to its declared selected-channel targets. Users can understand and customize the controls without confronting every advanced setting on the performance screen.

This lane owns interaction and LVGL presentation, not the canonical engine, transport or ADC. The finish line includes actual display/touch operation and applied-state integration. A attractive HTML workbench alone is not a production result.

Use `k1-control-r2/control-architecture.md`, `source-audit.md`, `backend-design.md`, `K1-Channel-First-Control-Workbench.html`, `workbench-app.js`, `workbench-template.html`, `Control-Profile.json`, the five images in `references/` and `Workbench-QA.json`. The 43 existing workbench checks are Node DOM/model checks, explicitly not browser layout, physical touch or LED tests. The preview PNG is a model render. Preserve this distinction in new evidence.

Active source: `/Users/spectrasynq/Workspace_Management/Software/K1.Sliders/src/console/ui.cpp`, `display.cpp`, `pins.h` and `main.cpp`. The captured head is `2f17b8a967de64a4088cc4e6976ddcf16922bb76`, with inspected dirty working files identified by hashes. The FT5x06 screen input is already integrated with LVGL; its nominal polling cadence is 20 ms, not a guaranteed touch-to-light latency. The 1.91-inch `K1-HMI-BLE-MIDI` project is historical reference, not the display/pin authority.

The user has superseded fixed assignments and no-bank/no-pickup restrictions. Legacy Chroma has no automatic claim on a fader. Saturation belongs on Colour/custom pages. **Liveiness** is the requested visible spelling and a versioned mode capability; changing the Mood label is insufficient. Preserve legacy decoding outside the UI, with no silent remapping of old wire IDs.

## Ownership and dependencies

| Surface | Owner and action |
|---|---|
| Existing `src/console/ui.cpp` and its existing `ui.h` interface, after confirming the header path | UI; replace fixed nine-column CC-centric ledger with the new hierarchy and applied-state presentation |
| New `src/console/ui/control_view.h`, `.cpp` | UI; view model, route state and immutable presentation snapshot |
| New `src/console/ui/context_lens.h`, `.cpp` | UI; typed color, motion/history, rhythm and response explanations |
| New `src/console/ui/control_editor.h`, `.cpp` | UI; draft editor, confirmation/review, validation messages and profile workflows |
| New `test/control_ui/` plus visual fixtures | UI; native/view-model tests, native-resolution captures, real-touch task scripts |
| `main.cpp`, `display.cpp`, `k1_session.cpp`, shared `snapshot.h`, fonts/build resources/task wiring | Exclusive console platform owner; UI supplies proposed integration changes, not competing edits |
| Raw faders, qualification, takeover and gesture identity | INP; UI displays input feedback and issues route requests through the interface |
| Parameter metadata, admitted capacities, profile compiler, migration and atomic activation | CTL; UI consumes/generated contracts and submits typed drafts |
| Applied state, effective telemetry, validity/age, response ownership | Engine/transport/AP/VP owners; UI does not manufacture confirmations or inference certainty |

New filenames are designated targets, not previously verified repository files. The console platform owner approves any path collision or repository-convention adjustment before edits. File ownership remains exclusive during parallel work.

Before binding the UI, agree with CTL/INP on `UiSnapshot` and events. A snapshot includes selected route and binding revision, nine descriptors and pickup states, pending/requested/base/effective values with units and observation times, supported/inactive reasons, current profile digest, channel mode/palette, shared master/blackout, engine link status and bounded AP/VP diagnostic context. `Navigate`, `DraftEdit`, `CommitProfile`, `SetParameter`, `ToggleFeature`, `PinLens` and `CloseLens` are distinct events. Only CTL/engine acceptance changes canonical applied state.

## Implementation sequence

### UI-1 — make the real screen an instrument

Produce native-resolution wireframes using actual fonts and declared physical touch sizes before connecting engine writes. Keep these visible in every performance/context state: selected channel, selected page, nine current fader labels/compact values, pickup indication where needed, and access to master/blackout. The nine physical faders are **outside** the screen. Desktop emulator controls are not evidence that labels exist on the real hardware.

Use visible Primary/Top and Secondary/Bottom tabs, then a visible page selector. A double-tap shortcut may supplement a clearly bounded navigation area; essential operation must not depend on discovering it. Opening a page does not send nine values. Do not reorder/remap controls invisibly when modes change; show capability reasons and offer an explicit compatible page choice.

The starting Perform/Colour assignments come from the approved CTL profile, not hard-coded UI arrays. Both can refer to the same canonical target (for example intensity or palette position); do not create duplicate parameter state. Master and blackout are product-level controls outside the nine-fader page. A+B linking is an explicit future/custom scope, never the side effect of showing both channel summaries.

Nine labels in 600 pixels imply about 67 pixels per slot. Treat that legend as read-only information, not nine undersized touch edit buttons. Open a large slot picker or use a clearly armed “select by moving a fader” mode for editing; outside that mode movement continues normal performance. During selection-by-movement, show that engine writes are suppressed and let the user cancel without changing a parameter.

**Program accessibility requirements:** principal touch actions must have at least a 7 mm target in each dimension and at least 1 mm separation from a conflicting action; measure actual active display dimensions and convert to native pixels. A nominal 2.41-inch 4:3 display would put 7 mm near 86 pixels, which is an estimate rather than a measurement of this assembled unit. Use full-screen editors, scrolling and fewer simultaneous actions to meet the physical target; do not scale a desktop dashboard down and call it accessible. Show state through text/shape as well as color. Provide reduced motion, adjustable/persistent help duration and readable enlarged detail for compact legend text. Validate contrast and legibility on the actual AMOLED under declared indoor conditions. These are project acceptance rules, not a claim of certification against an external accessibility standard.

### UI-2 — present truthful state and control significance

Default performance values are accepted engine bases with native units or meaningful normalized displays. When the performer is moving a control, distinguish physical position, requested target, pickup capture and effective response. Transport ACK, local echo, accepted base and resulting photons are different stages. A pending request uses a pending indicator; a rejection displays its reason without retaining a false applied value. A parameter with a valid value but no current effect is “inactive in this mode/context,” not broken or silently removed.

Use metadata for label, units, bounds, default/neutral, explanation, feature enable state, supported modes, response owner and visualizer. Examples:

- Liveiness shows its current mode mapping and neutral point; if unsupported, say why. It must not claim a new detected BPM or overwrite independently saved Motion/Width/Persistence bases.
- Palette transition displays the next transition time/path when no transition is active; do not fake immediate strip changes to make its fader seem active.
- Chroma labels distinguish palette position, legacy brightness-floor behavior if retained, color-model chroma and audio pitch-class chroma.
- Audio focus/rhythm controls display AP validity and age. A stale or low-confidence detector state must not appear as a confident beat; authored rhythm mix is not confidence.
- Artistic enable toggles are separate from amounts. Bypass should preserve the stored amount and explain which stage is inactive. Power/fault authority is not an optional artistic toggle.

Use producer observation timestamps plus transport age and declared clock uncertainty. Freeze staleness thresholds with AP/transport; if those thresholds are missing, show unknown/stale rather than inventing “live.” Effective telemetry is a bounded diagnostic feed, not full pixel streaming over BLE. Local animations must identify themselves as models when they interpolate between real observations.

### UI-3 — implement the semantic context lenses

The context lens is a view layered over the current route. It never remaps the faders. Accepted qualified activity from INP or an explicit user request may open it; raw ADC noise, packet receipt and telemetry refresh may not.

| Trigger | Required visual explanation | Truth boundary |
|---|---|---|
| Palette position/span/travel | Full selected palette path, exactly matching reachable window, present sample/direction and travel amount | Window uses the actual sampler's wrap/clamp law and span, not an independently drawn approximation |
| Saturation/warmth/contrast/intensity | Before/after palette strips, labeled transfer/tonal region and clipping/limiting indication where known | Name the color model; monitor RGB is not calibrated LED color or measured luminance |
| Motion/width/Liveiness | Centre-origin path, direction/velocity, width envelope and base-versus-effective change | Existing material should integrate future changes continuously; UI must not teleport history to illustrate speed |
| Persistence | Impulse/trail example with half-life or declared lifetime units, old/new overlap | Distinguish immediate attack from history release and demonstrate how gaps can fill |
| Rhythm mix/focus | Primary and secondary evidence, mix weighting, validity/confidence and age | Musical evidence and authored subdivisions remain separate |
| Mapping/response editor | Physical input → mapped target → effective response traces with range/step/curve | Filtering, mapping, stepping, response and transport delay are distinct causes |

The five supplied hue/chroma/value trees and color wheels are design inspiration. A full atlas belongs in the advanced Colour view; routine palette work should emphasize the selected palette and reachable region. If displaying OKLCH or another colorimetric space, use its actual transforms and name gamut handling; do not relabel HSV as Munsell/CIELAB/perceptually uniform light. Reuse VP’s material/transfer reference kernel or validated fixtures to keep equations aligned.

Lens state machine: closed → open(domain, captured channel/route) → pinned or timed → restored. Default timeout is **5,000 ms of semantic inactivity**. Restart only for meaningful relevant user activity; pause while a physical movement session is active or an editor requires attention. Pin disables auto-dismiss. Close suppresses reopening by the same continuing gesture. Navigation closes or deliberately rebinds the lens to the newly selected route; no Top heading may remain above Bottom data. Restore the exact previous route, focus and scroll state without parameter writes.

Choose deterministic multi-fader arbitration: keep a domain lens stable while relevant controls move; group same-domain gestures; switch domains only after the explicit ownership rule (for example a newly started most-recent gesture) permits it. Do not flicker between dashboards at scan cadence. Pending/rejected/inactive feedback and blackout remain accessible after the explanatory lens disappears.

### UI-4 — provide full depth through progressive disclosure

Implement three levels: performance route; contextual explanation/quick adjustment; dedicated Control settings editor. The latter must expose admitted assignment, channel scope, page/slot, native-unit user range, monotonic mapping/curve, inversion, output stepping, effective input-resolution ceiling, pickup/jump/value-scaling, response mode/time/slew and supported relative-trim precision/acceleration. Do not offer unsupported controls as active widgets. Read-only sensor precision is labeled as such; a finer wire value does not add ADC bits.

Create, rename, reorder and remove custom pages within CTL-advertised bounds; every admitted page uses nine physical slots and valid scope. Provide a slot picker with accessible touch targets, a live mapping preview, explanation of side effects and explicit apply/cancel. Surface capacity limits before a user loses a draft. Default pages can be restored. The two-page host reference is not the production extension mechanism.

Draft edits never mutate the active profile. Send the whole draft to CTL's semantic validator/compiler, display field-specific errors, then activate one compatible immutable revision. Show changed assignments/ranges before commit. Preserve current engine values if a new range excludes them; offer a separate explicit bring-into-range action. Undo is a new admitted revision, not a partial mutation. Export/import preserves profile identity, schema and migrations. Appearance presets do not silently replace the user’s HMI profile. Hardware normalization/calibration remains separate; the captured factory-map/no-wizard policy is not revoked by this editor.

UI renders typed fields and submits structured drafts. CTL owns validation, compiled fixed tables and bounded persistence. Do not put JSON parsing, file import, dynamic layout allocation or flash writes in AP/VP/ADC timing paths. A companion editor may supplement the console; essential page selection, assignment and the advertised control settings must remain available through the declared product interface. If a full setting is companion-only, label that boundary clearly and test the actual round trip before advertising it.

### UI-5 — declare and meet display/resource budgets

Measure the current console before imposing a frame-rate claim. Record actual draw-buffer allocation, memory class, fonts/assets, LVGL task high-water mark, worst redraw cost, touch scheduling and BLE/input interference. The captured source uses partial buffers and a 6 KiB LVGL task stack; these are implementation inputs, not proof of available headroom.

Publish a budget table before candidate acceptance: performance/lens/editor redraw targets, worst allowed callback duration, telemetry cadence, bounded view/lens memory, required heap/largest-block reserve and native touch-to-feedback limit. Derive memory from admitted descriptors, buffers and measured allocation; do not invent an unconstrained 60 fps requirement. Dirty-region updates and preallocated views should preserve input deadlines. UI must remain usable if explanatory animation is reduced under load; values, scope, rejection and blackout are higher priority than decoration.

A material reference preview is useful even when exact pixel telemetry is unavailable. Label three different modes clearly: **illustrative mechanism**, **engine-material reference using matched state**, and **observed output data** if truly supplied. None is a calibrated optical preview without measured LED/optics/display transforms. Preserve logical 160-pixel/channel geometry and centre pair 79/80 in material views, while showing physical crop separately. No centered browser strip proves a real LED mapping test passed.

### UI-6 — integrate, deploy and evaluate on the actual surface

Use host/native view-model fixtures while CTL/INP/AP/VP mature. Gate live values behind matching catalogue/profile capabilities. Send integration requirements for shared task wiring/display callbacks to the console platform owner; no LVGL draw callback may issue transport writes or mutate engine state. Keep task locking/snapshots bounded.

Build/upload commands were not captured for this console. Resolve them from the active checkout and installed toolchain, then record exact invocations and resulting image hash. Do not present a guessed PlatformIO environment or borrow a Titan/old-HMI flashing command. Existing execution/test authorization applies, including flashing unless a current target-specific HOLD exists. Only the appointed device owner uploads while holding the hardware lease; other specialists continue simulation and review.

Evaluate the native screen with actual touch and physical faders. Capture annotated 600×450 renders plus camera evidence of real touch targets and status on the installed panel. Browser tests and native renders complement physical evaluation; they do not replace it. Test under the full admitted console/engine load with actual peers and applied-state round trips.

## Acceptance and evidence

The following are requirements to prove. Timing thresholds must be frozen with IVV before measuring; they are not prior results.

| ID | Test and conditions | Pass condition and evidence |
|---|---|---|
| UI-A01 | Both channels, all admitted default/custom routes on native screen | All nine dynamic labels and compact values exist inside 600×450; selected scope remains explicit; physical faders outside screen; master/blackout accessible; no essential secret gesture |
| UI-A02 | Instrumented touch hitboxes and actual active-area measurement; principal navigation/editor actions | Every principal action meets ≥7 mm target and ≥1 mm conflict separation; no overlapping targets; native actual-touch captures and hitbox manifest |
| UI-A03 | Real navigation during moving faders, fast page changes, preset/remote edit | Zero route-driven parameter writes or old-event retargets; pickup state correct; pending route/quiet rearm visible; confirmed independent A/B state retained |
| UI-A04 | Every lens domain using matched fixtures, wrap/clamp/span edges and palette changes mid-transition | Labels/equations match producer metadata and sampler; no fake AP certainty; correct base/effective distinctions; model versus observed preview scope explicit |
| UI-A05 | Open, active hold, noise-only input, telemetry-only input, pin, edit, close and page switch | Default dismissal at 5,000 ms with ≤100 ms scheduling deviation after eligible inactivity; no dismissal while held/pinned/editing; same gesture cannot reopen after close; restoration sends zero writes |
| UI-A06 | Two and then nine simultaneous fader gestures | Deterministic stable domain policy; no scan-rate lens oscillation; every relevant value remains discoverable |
| UI-A07 | Create custom page; edit every advertised mapping setting; cancel, invalid apply, valid apply, undo, export/import, reboot | CTL-admitted atomic profile; invalid/cancel leaves active state unchanged; all advertised settings affect the actual input/engine as declared; no JSON parsing in real-time paths |
| UI-A08 | Delayed/rejected/missing response; stale AP, unsupported mode, lost/recovered link | Pending/applied/effective/inactive/stale are distinguishable; no optimistic application claim; successful resync re-arms input; no idle replay |
| UI-A09 | Native full-load performance/lens/editor run | Predeclared redraw, memory, touch and platform input/transport deadlines satisfied; no growing allocation loss after warm-up; decorative degradation preserves core controls |
| UI-A10 | Task evaluation on installed hardware: select Bottom Colour, move correct palette fader, interpret pickup, create narrow-range custom page, undo, reach blackout | For each task, record completion/time/errors and observer notes. Every participant completes without a hidden-gesture hint; zero wrong-channel edits; explanatory wording understood or corrected and re-evaluated |
| UI-A11 | Each feature shown as available in shipped profile, with the selected mode/palette and real peer | Function produces its declared response or honest inactive state for a legitimate context; a permanently inert advertised control fails release |

For UI-A10, use at least the product owner and one person unfamiliar with the new pages where available; an unavailable second participant leaves independent usability validation NOT TESTED, not fabricated. This is a focused usability check, not a statistical population claim. For timing, record total trials and distributions rather than a few selected screenshots. Screenshot color comparisons do not establish optical superiority.

## Gates, done and next owner

GO to parallel UI implementation after CTL publishes descriptor/event contracts and the console platform owner allocates files. Use typed fixtures until the actual applied-state path is available. Stop only the affected live integration for contradictory units, missing capability identity, stale/unknown source identity or ownership conflict; continue visual and interaction work on agreed fixtures.

GO to physical acceptance after INP, transport and the combined console candidate have their required checks. A screen can be visually complete while application feedback is absent; that state is a prototype, not a product release. Do not relax acceptance by hiding a required feature behind an unsupported badge at closeout. A smaller explicitly agreed release scope may omit a feature; all advertised functionality must pass.

**UI is done** when owned production code is integrated, all required default and custom workflows run on the real console, every advertised control has truthful state/meaning, accessibility and resource metrics pass, and IVV has the raw evidence and reproducible task scripts. Deliver source revision/diff, native captures, touch hitbox manifest, budget/latency data, profile/capability identities, functional matrix, limitations and actual device leave-state to the console platform owner and IVV. AP/VP receive any semantic discrepancy; CTL receives profile/editor constraints. The integrator receives a tested consumer, not a request to invent its missing behavior.


---

<!-- Source: lanes/08-dualmcu-platform.md -->

# DUAL — RT1062 engine and S3 bridge integration

## Objective, authority and owner

Deliver the agreed portable audio, visual and control functions on the identified DualMCU configuration, with RT1062 owning capture/AP, control application, VP and output, and ESP32-S3 owning radio/transport. The DUAL specialist owns RT1062 target entry/runtime, platform adapters and target evidence, not new DSP, colour semantics, UI design or catalogue IDs. INT is the sole writer of existing portable-core integration call sites and assembles the repository candidate. DUAL consumes that sealed integrated core. TRN owns transport codec/scheduler source; DUAL does not concurrently edit those files. Freeze exact per-file ownership at preflight because a directory label alone does not prevent overlapping writes.

Source root: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware`. The reviewed `67a1cd0b0ecc626b28af2df9b0d11f855b402379` HEAD had dirty working files. Treat that as historical source identity, not a checkout instruction. Reconcile the current `AGENTS.md`, `docs/agent/AGENT_EXECUTION_STANDARD.md`, `docs/agent/DUALMCU_BENCH_IDENTITY_CANON.md`, `PROTECTED_FILES.md`, master plan and current owner before edits. The captured rules forbid sibling worktrees and extra project folders; do not blindly apply a generic worktree recipe. Parallelism can use disjoint analysis/patch preparation and independent repositories while a single writer integrates this repository. Resolve any current policy conflict before changing its worktree arrangement.

The R2 channel/page model supersedes fixed Appearance9/no-bank/no-pickup design. Preserve existing v1 semantics; do not reinterpret Mood IDs as Liveiness or rewrite the 71-control/68-state legacy identity. Source details and executable-command provenance are in `../deployment-source-notes.md`.

## Ownership and exact seams

| Surface | DUAL integration work | Ownership boundary |
|---|---|---|
| `firmware/rt1062/production_runtime.*` | Capture-epoch routing, immutable AP publication including the new local tempo sidecar, frame-boundary control application, scheduler inputs and truthful telemetry | AP algorithm and phase definitions come from AP; no feature/pixel streaming across SPI |
| `core/system/realtime_render_scheduler.cpp` | Supply measured target budget/time-mapping requirements to INT and consume its sealed change | INT owns existing portable scheduler edits; preserve event/availability times |
| `contract/`, `core/control/` | Consume CTL's versioned catalogue/transaction application and bind its platform adapter to applied-state telemetry | CTL/INT own canonical code/import edits; no private wire-number allocation |
| `firmware/s3_ble/`, current SPI implementation | Consume TRN's bounded relay/admission/reassembly/coalescing; own only separately assigned target entry/driver files | TRN owns codecs/schedulers; no overlapping file writers; no bridge DSP/VP |
| `hal/pixel_port.h`, `firmware/rt1062/ws2816_*`, existing edge adapter | Integrate wide-to-native words and prove DMA lifetime/packing/output completion | Do not retarget hardware pins or physical topology to match an old source capture |
| `core/visual/*` | Consume INT's sealed integrated portable core and run full-chain target fixtures | VP supplies algorithms; INT owns existing call-site edits; DUAL does not fork or directly patch them |

Keep core free of Arduino, FastLED, FreeRTOS and peripheral headers. `AudioFeaturesV1` and `TempoFieldV1` are engine-local AP→VP semantics, not the inter-MCU transport. Retain current SPI READY/descriptor handshake; do not reintroduce the retired UART link. HMI settings remain on the console and engine; bridge state must not become an alternative policy authority.

## Inputs and dependency gates

Accept an immutable contract revision, bounded control/reference fixtures, AP sidecar/reference vectors, the selected VP graph/reset matrix, native-output vectors and an integration manifest listing exact commits plus dirty-file hashes where applicable. Every feature descriptor must include capacity and its unsupported reason. A candidate whose required schema is unresolved is not ready for physical deployment; adapter scaffolding and host tests can continue.

Before builds, record current source status, toolchain/environment, board role, physical LED protocol, topology/crop, sample-rate/hop configuration, current ceilings and verified rollback images. The prior pin-0 WS2812 bench and native WS2816 product route are different admitted variants. Proving one does not qualify the other. Do not assume a historical USB serial, MAC, pin mapping, or a separate Audio Adapter Rev D setup remains this lane's active target.

## Implementation and integration sequence

1. **Inventory and baseline:** run the current mandated preflight after inspecting side effects; capture HEAD, status, affected hashes, firmware/configuration and current test results. Preserve unrelated work. Archive existing candidate and rollback bytes before building into an output directory that might overwrite them. Create a source-to-image manifest and a compatibility tuple for engine, bridge, console, catalogue, profile, saved-state schema and hardware variant.
2. **Build adapters without changing feature meaning:** add bounded sidecar publication, staged control transactions and accepted/effective telemetry at existing seams. Own preallocated publication/command buffers; document producer, consumer, lifetime and overflow policy. No allocation or persistence writes in AP/VP call graphs. Use epoch and sequence checks before mutation.
3. **Integrate compatibility first:** old peers remain supported under their actual schema. Run existing source-bound vectors and bridge tests. New controls remain unadvertised until their receiver, transport and applied-state path exist. Reject incompatible peers explicitly without changing the running show. Do not silently clamp unknown controls into old ones.
4. **Integrate native visual output:** retain fractions through treatment and gain, then quantise once into native component words. Verify complete output against the VP reference; do not prove precision merely from unequal high/low bytes after multiplying lifted RGB8. Keep compatibility RGB8 expansion as an explicitly identified adapter where required.
5. **Integrate the console transaction path:** receive stable channel/parameter targets, session epoch, gesture sequence and binding/profile revision. Intermediate continuous updates may coalesce; accepted terminal targets and discrete actions must be delivered or explicitly rejected. A page change cannot retarget queued work. Return authoritative applied state before the console claims application.
6. **Measure and admit capacities:** build/link map, static buffer tally, observed stack high-water, AP cycles, effect/render cycles, output pack time, output completion and queue high-water under the worst admitted feature graph. Turn measured capacities into advertised platform capabilities; do not invent post-failure thresholds or reduce enabled scope silently.
7. **Install and verify using the deployment sequence below:** only after host/contract gates pass and the active bench lease is held. Keep independent host specialists working while physical tests serialize.

## Deployment runbook

The packet does not include the full current RT1062 target build/upload scripts or identity canon. Resolving them is a bounded preflight deliverable, not licence to guess an environment or serial. Inspect current PlatformIO configuration and lane build wrappers, read the identity guard, invoke safe help/self-tests, and write the exact working directory, executable path, argv, environment, output names and exit-code capture into `resolved-commands.json` before use. Commands cited in `deployment-source-notes.md` distinguish captured script behavior from prior documented recipes.

1. Acquire the existing exclusive physical-bench/USB lease, covering all devices disturbed by the campaign and the output rail. Record actual owner, scope and release procedure. Do not seize another agent's CDC session, kill Teensy.app or treat a port path as device identity.
2. Verify the board identities against the current canon and actual source/variant. Capture current engine/bridge/console versions, config, calibration/NVM preservation method and available rollback bytes. Inspect any active scoped HOLD. Old assignment-specific no-reflash restrictions are not automatically permanent, but an actually current HOLD remains binding.
3. Run affected host suites and the upload-target self-test. Keep its known-bad target case failing. Freeze image hashes, build flags and capabilities before any installation.
4. For a backward-compatible migration, update **consumers first**: engine accepting old and new contracts; bridge capable of carrying both; console advertising new controls last. Build the new parsers/adapters into consumers while keeping new profile advertisement and new artistic-feature activation disabled. Verify those flags in the build/runtime manifest. Once compatibility/round-trip passes, enable the new producer/profile and explicitly select admitted artistic features. Which devices actually require installation follows the diff, not an obligation to reflash every board. After each step establish boot identity, health, old-client operation and truthful state before the next.
5. If the first consumer cannot retain old-client compatibility, use a declared coordinated maintenance transaction for the entire compatible tuple: stable output handling, stopped new command production, coherent installation and recovery. Do not leave a mixed unsupported tuple operating. A generic ACK never proves the new schema is applied.
6. Restore the saved compatible configuration; verify readback; establish engine capability/state snapshot; allow new profile selection only after matching schema/capabilities. Initial console stationary positions must not overwrite engine state.
7. Run functional fixtures, the G3 lane-local loaded test and the final G4 coherent-system campaign from IVV's catalogue. The final campaign is at least 30 uninterrupted minutes plus ten individually scored recoveries; a ten-minute platform run is intermediate evidence. Capture actual interval counters before/after, boot epochs and progress. Mark intentionally replaced pending frames separately from dropped/lost audio or failed output. Finish with a documented running state, lease release and current image tuple.
8. If a component fails, preserve logs and return to a **known compatible tuple**, including profile/configuration schema migration where needed. Restoring only an old engine beneath a new incompatible console is not rollback. A failure to recover identity/readback stops that device write path; retain independent software work.

## Timing and memory acceptance

Derive `T_hop = hop_samples / sample_rate`. The captured shared AP configuration is 24 kHz/180 = 7.5 ms; legacy 12.8 kHz/96 has the same nominal hop but a different workload and signal path. Bind the actual compiled/captured configuration. Source-derived wire time is `pixels_per_serial_chain × bits_per_pixel / wire_bit_rate + reset_interval`; parallel chains use the longest chain, not the sum of parallel chains. An 80-pixel native48 chain at 800 kbit/s plus an illustrative 300 µs interval consumes about 5.1 ms before additional overhead. This is not a promise of 400 Hz operation.

The captured DualMCU requirements are effect work below 2.0 ms at 120 FPS and end-to-end audio-to-visual latency below 8 ms. Confirm current authority and define exactly which event, capture, availability and physical-latch timestamps they govern before measuring. Do not add a full hop and a full wire period without the actual scheduling relationship, and do not remove either from the relevant metric. A conflict or unavailable measurement blocks that timing claim until resolved; it does not turn a build into physical evidence.

| Criterion | Conditions and pass result | Evidence |
|---|---|---|
| Ownership | AP/VP/control execute on RT1062; bridge carries bounded controls/state only | Dependency inspection and role-specific map/counters |
| G3 loss and deadlines | At least 10 uninterrupted minutes at worst admitted lane load; zero audio loss/discards/overruns, deadline misses, unexplained output faults or resets | Monotonic interval, boot epochs, raw counters, worst and distribution timings |
| G4 coherent system | Final at least 30-minute loaded campaign plus ten recoveries under IVV catalogue; retain any stronger applicable gate | Individually scored trials and candidate-bound 30-minute log; no substitution by G3 |
| Memory | Static regions fit the actual linker map; no AP/VP allocations; measured stack high-water leaves the predeclared reserve; buffers stay within admitted capacities | Map, ownership/capacity table, stack and queue maxima |
| Precision/output | All full-chain native words match the reference; true-black words are zero; output completion tied to generation/epoch and fault-free driver completion | Pack vectors, DMA lifecycle trace; physical latch timing where claimed |
| Control recovery | Stale epochs/revisions do not mutate; reconnect acquires state without idle-fader replay; accepted terminal targets accounted for | Negative tests plus installed round trip |
| Channel/reset isolation | A-only mode/palette/range changes leave B history, phase and AP epoch intact except a declared global operation | Pre/post private-state and output fixtures |
| Physical scope | Actual emitted channel/crop and output protocol are recorded | Hardware manifest, direct observations; no qualification of an unconnected channel |

**Done:** accepted portable features work through the installed identified tuple, current lane timing/loss/memory gates pass, native-output and control failures can be detected by negative fixtures, applicable physical tests are scored, and rollback/leave-state is reproducible. Deliver exact changes, build commands, images/hashes, the capability manifest, per-criterion PASS/FAIL/BLOCKED/NOT TESTED, raw logs and next owner. “Host PASS”, “upload succeeded” and “LED submission returned success” are separate partial milestones.


---

<!-- Source: lanes/09-titan-platform.md -->

# TIT — Titan Mini scalar integration and qualified deployment

## Objective and authority

Deliver the accepted shared AP/VP/control functions on the Titan Mini's actual RA8P1 runtime while preserving captured audio epochs/publication, GPT/DMA output ownership, native precision and the lane's reference process. First prove scalar integration. Helium, U55 and M33 work remains a separately costed subsequent experiment; it must not become a dependency for functional acceptance.

Source root: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`. Reviewed HEAD `431140853dd8b58af53240ef84d5fb1a08bd8b45` was dirty. The captured `docs/REFERENCE-MANIFEST.md` retains DualMCU authority commit `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`; it does not authorize importing today's moving DualMCU HEAD. The BSP reference is platform evidence, not the product's behavioural authority.

Read current repository instructions, reference manifest, current Titan engineering skill/platform memory and `docs/evidence/K1-RA8P1-002/TITAN-MIC-LED2-SESSION-CANON.md` before platform edits. Read current E2 Lite router instructions before programming. The captured router contract specifies E2 Lite/SWD with `rfp-cli`, probe power off, no GDB attachment and no `programme_scalar.py` write path. Working E2 target attachment was subsequently reported; do not restart obsolete harness repair or assume attach success proves an image or application. Current source and current target identity decide the actual procedure.

## Ownership and interfaces

| Surface | TIT action | Boundary |
|---|---|---|
| `src/k1/core/audio/*`, `src/k1/core/visual/*` | Import the explicitly accepted portable delta with reference receipts | No independent algorithm fork; no Renesas/FSP/RT-Thread headers in shared code |
| `platform/ra8p1/k1_live_runtime.cpp` | Preserve hop acceptance, stream epochs, support/availability times and immutable publication; integrate tempo sidecar | Older evidence completion is not fresh evidence because publication is later |
| `platform/ra8p1/palette_runtime.cpp/.h` | Replace broad configure resets with per-field/per-channel handlers and frame-boundary atomic application | Channel-only edits cannot reset the other channel, audio presence or publication bookkeeping |
| `platform/ra8p1/fixture_app.cpp` and staged config path | Validate entire transaction before mutation, bind applied revision, integrate selected wide output producer | Legacy lift-to16 remains compatibility-only where the new wide path is selected |
| GPT/DMA pair driver and output adapter | Retain current proven timer/ELC/DMA lifecycle; qualify completed generation and true black | Submission, one DMA flag or a callback is not a completed two-lane physical transaction |
| `tools/serial-studio/` and current broker/scorer | Consume versioned capabilities, query applied state and score installed candidate | Shared semantics do not imply DualMCU SPI/BLE transport exists on Titan |

The TIT specialist owns platform adaptation, selective-reset implementation and build/deployment evidence. AP owns DSP and phase meaning; VP owns composition; CTL owns semantic transactions. TIT imports INT's sealed accepted portable export through the reference process rather than editing shared algorithm meaning. Coordinate overlapping `palette_runtime`/fixture changes through one Titan integration writer. Reviewers and fixture authors can work in parallel without operating the target.

## Implementation sequence

1. **Pin current reality:** record current source/dirty hashes, toolchain, BSP configuration, installed image identity, hardware UID, output pins/profile, physical crop, capture source, rates and live owner. Run the mandated reference checker before importing/comparing portable code. A moved reference HEAD is neither an automatic update nor a reason to reset unrelated work.
2. **Create an explicit portable import receipt:** old reference commit, approved shared-contract revision, exact changed files/patch hashes, expected behavioural deltas, preserved compatibility vectors and current Titan derivative changes. Have the portable owner review this receipt before merging it into the Titan candidate. Do not import DualMCU history or edit the reference/BSP repositories.
3. **Fix configuration isolation first:** compute a validated delta; distinguish per-channel mode, palette selection/transition, continuous controls, output mapping, audio epoch and full lifecycle reset. Apply through owner-specific handlers at a defined boundary. Publish the new revision only after every field becomes effective. Both direct `SET_CONFIG` and staged `COMMIT_SET` must use the same semantic path.
4. **Prove reset meaning:** run B with nonzero phase, trail and transition; change each A-only field and compare B state/output. Reverse channels. Invalid input must preserve all applied state. A mode change may use its declared local reset; palette transition may change its own history policy; neither may clear capture epoch, both-channel dwell/presence state or unrelated timers. A genuine stream reset uses the separately specified audio lifecycle.
5. **Integrate AP evidence and controls:** retain hop/stream acceptance, media time, result-available time and the incumbent canonical clock. If ACF slicing is compiled, preserve the analysed interval across slices. Export new candidates with actual age and validity. All control updates target stable IDs/epochs and return applied revision.
6. **Integrate complete native output:** bind FP32/working-domain reference to `hd_pixel16`/packing at the final conversion point; test after all brightness/treatment operations. Maintain independent A/B and centre mapping. Preserve driver buffer ownership while a transfer is active; queue/replacement policy must be explicit and bounded.
7. **Compile and measure before expanding scope:** scalar build, host parity, full output vectors, linker map and buffer/stack budgets first. Measure the actual worst loaded AP/VP/driver path; admit feature capacities only with evidence. Optimisation proposals must retain scalar vectors and show whole-pipeline target benefit.
8. **Run matched deployment and physical qualification:** use the guarded runbook below, then deliver a reproducible leave-state and image-bound acceptance.

## Rate and timing manifest — mandatory before a run

Record these independently: microphone peripheral clock/decimation, PCM rate and format, hop samples, spectral window, novelty cadence, AP configuration, optional slicing overlay, visual schedule and actual output chain timing. The captured common source selects 24 kHz/180 samples, while earlier scalar campaigns used 12.8 kHz/96. Both yield 7.5 ms nominal hops; neither proves the other's cycle bound, microphone fidelity or sample continuity. A 16 kHz microphone diagnostic is not acceptance of either product configuration. Do not use the known old 40 kHz decimation problem as a universal description of the current pipeline.

Bind timers to actual clocks from the generated build/runtime. Convert cycle maxima using that identified clock, accounting for counter wrap. Freeze required completion deadlines and resource reserves before measuring. Carry the support interval and publish time separately; a later sliced ACF result must not receive the new hop's observation age.

## Deployment and recovery runbook

1. Acquire the existing Titan physical-bench lease and CDC ownership mechanism. Freeze a source/image/configuration tuple and preserve verified rollback bytes plus live configuration. Query actual board identity using the authorized identity workflow; do not substitute the historical hardcoded UID in a captured Python client for current verification.
2. Load the actual E2 Lite router skill. Resolve its current exact `rfp-cli` executable, device/serial identification arguments, image-format/address rules, write/verify and post-write procedure. The earlier target inventory identifies modified `scripts/build_scalar.py` and untracked `scripts/programme_rfp_swd.py`; read those current files as concrete starting points, including their argparse/help safety and router applicability. Their contents/argv were not captured, so do not assume an option spelling. Capture the executable version and supported safe help before producing `resolved-commands.json`. The packet intentionally does not fabricate a target write command absent the captured router content.
3. Verify the working target/probe pairing and applicable power policy. Do not power the target through the probe, attach GDB contrary to the route, run a retired serial ROM writer or rework the already working harness based on stale errors. An abandoned host-lock repair is permissible only under its precise no-active-consumer guard; it is not a routine reset step.
4. Build the frozen candidate with the lane's current wrapper and verified configuration. Record source, generator/overlay inputs, compiler/linker flags, ELF/map/BIN hashes and required regions. Check that the native producer and chosen GPT/DMA backend are actually compiled. Resolve omitted wrapper arguments from current scripts; no guessed `make`/`scons`/flash recipe.
5. With prerequisites passing and normal programming authority applicable, write and verify the exact bytes through the router. Bind the receipt to target UID, probe, address/range and image hash. Programming success qualifies those bytes only. If no image change is required, retain identified installed bytes and do not reflash merely because a host fixture changed.
6. Reacquire application identity and live config through the existing broker. Check boot epoch, sample-source/rate claims, output backend and capacities. Run the candidate's source-matched scorer. Do not claim a former image's host/target result applies to this one.
7. For output/mapping tests: save live config; enter the currently authorized scoped output hold; drain old work; emit tagged fixtures; require matching generation and live epoch, both lanes' DMA completion/hardware-stop conditions, and zero faults; verify a completed black pair; restore saved config; read back; prove continuing output movement. A wrong-epoch/faulted black pair cannot release the hold. This is a specific test transaction, not a blanket no-flash policy.
8. Recovery uses the saved compatible image plus matching config/schema. After rollback, verify bytes, application identity, restored readback and continuing output. Preserve failed run evidence and identify a new run after any repair. Release leases only with a truthful target/output leave-state.

**Captured client caution:** `titan_live_control.py config-set` forces `emit_on=0` and clears a flag. It is not a neutral live palette editor. Its `snapshot`, `config-get`, `schema`, `events` and `timing` subcommands still acquire CDC/campaign leases; inspect the current client before use. See `../deployment-source-notes.md` for exact captured command syntax and provenance.

## Acceptance and evidence

| Criterion | Required result | Evidence |
|---|---|---|
| Reference/import | Approved exact delta against the retained reference; no unexplained compatibility change | Reference check, import receipt and unchanged golden vectors |
| Selective reset | A edits preserve B history/phase/transition; continuous edits preserve AP/publication state; invalid batch is fully rejected | Actual state/output fixtures on direct and staged config paths |
| Rate identity | Compiled/captured/declared PCM rate, hop, clock and capture source agree | Build defines, live identity and sample/counter evidence |
| AP timing/loss | Every hop completes inside the actual bound under maximum admitted combined load; zero late starts, overrun, loss/discard or publication-deadline increments | Start/end interval counters, every-hop or bounded trustworthy maxima, raw log |
| VP/output timing | Renderer/output scheduler meets the frozen release/presentation budget; monotonic advancing generations; no unexplained frame freeze | Cycle/release trace, completion records and progress samples |
| GPT/DMA completion | Expected live epoch and requested generation completed on both lanes, hardware stopped appropriately and all fault counters zero | Tagged completion receipt; missing/wrong/partial/faulted negative cases |
| Native precision/black | Fractional full-chain fixtures produce reference native words; sparse centre/edge fixture maps correctly; black completes before restore | Native capture/packer vectors, completion + physical observations where claimed |
| Memory | Fixed capacities fit actual memory regions; no unbounded hot-path allocation; stack and DMA ownership margins meet predeclared reserves | ELF map, maximum queues/stacks and buffer lifecycle trace |
| G3 loaded run | At least ten uninterrupted minutes of admitted audio + VP + control + output; zero unexplained faults/loss/resets | Image-bound run with monotonic duration and a scorer proven to reject missing/rising fault counters |
| G4 coherent system | Final at least 30-minute loaded campaign plus ten individually scored recoveries under IVV catalogue; retain stronger current gates | Candidate-bound run and individual recovery receipts; G3 is intermediate only |
| Progress | Samples far enough apart to show continued motion; a run that moves once then freezes fails | Existing source-matched progress scorer and its negative test |

Add focused mode/palette transition, one-channel edit, rapid enable/disable, silence→wake→silence, reconnect and output fault/recovery scenarios around the continuous run; do not replace them with a long idle soak. Retain prior 40-loop or other stronger governing campaign if applicable. Optical “true black” and apparent smoothness need direct observation or correctly scoped instruments; a zero framebuffer alone does not prove emitted darkness.

**Done:** reference import, selective resets, shared feature semantics, native output and actual target timing are all evidenced on the identified image; the installed interface reports correct capabilities/applied state; restore and movement are verified; each physical claim has matching evidence. Hand integration/IVV the image tuple, exact resolved commands, source/map/config hashes, all outcome rows, raw failures and final lease/device/output state. An unresolved programming command or target identity blocks writes, not host integration.


---

<!-- Source: lanes/10-s3-standalone.md -->

# S3 — standalone engine adapter and later engine-module transition

## Objective and bounded uncertainty

Deliver the same admitted semantic instrument on a mainboard that operates with its resident ESP32-S3 and can later gain an engine module without replacing that mainboard. In standalone operation the S3 may own AP/VP/control/output. In a DualMCU configuration it becomes the communications-side adapter while the selected engine owns those domains. These are different deployment roles; do not relax the DualMCU bridge's transport-only rule to implement standalone mode.

The R2 study inspected DualMCU, Titan and console source, not a confirmed current standalone K1-System-R3 firmware entrypoint or product hardware manifest. A legacy S3 source may exist, but its location, current AP backend, build environment and product pins are **unconfirmed for this lane**. The first bounded deliverable resolves these inputs. Do not invent a repository path, copy bench pins into product code or treat historical ESP32-S3 DevKit mappings as production authority.

The S3 specialist owns platform adaptation, admitted-capability limits, firmware-role selection and the standalone→engine transition. Shared AP/VP/control semantics come from AP/VP/CTL, through INT's accepted portable export. Hardware routing, power-domain changes and mezzanine connector decisions remain the hardware lane's authority; this brief does not authorize board redesign.

The initial programme's mandatory physical admission is DualMCU, Titan and the active console. Standalone contributes the semantic/role contract and its explicit implementation lane now; its target release is a separately named admission once source and hardware are resolved. This unresolved source root does not block shared schema freeze or independent DualMCU/Titan work. Do not report the standalone hardware rows as completed merely because the initial programme closes.

## Preflight that makes implementation executable

1. Locate the active K1-System-R3 source inventory and standalone firmware owner using the current local project index/instructions. Record the exact repository/root, HEAD/dirty state, build wrapper, SDK/toolchain version and license obligations. If two candidate trees disagree, compare their current authority/readme and ask the responsible integrator to resolve that specific conflict; do not duplicate both implementations.
2. Obtain the accepted hardware manifest for the particular board revision: S3 module, flash/PSRAM modes, microphone/capture path, actual sample rate, LED protocol and driver route, engine detection/select mechanism, ownership of output pins, engine supply sequencing, current limits and recovery behavior. Preserve independently established bench variants as separate manifests.
3. Identify whether standalone and assisted roles use separate images, a build-time role selection or an admitted run-time mode. Publish this decision with its update path. A generic capability flag cannot safely switch pin/clock/DMA ownership by itself.
4. Recover current AP/VP compatibility vectors and output behaviour. The active legacy backend and current portable-core migration seam must be identified from current source; a convenient old architecture is not automatically the migration target.
5. Bind these results in `standalone-source-and-hardware.json`, a portable-boundary map and `resolved-commands.json`. That resolves the initial source gap; it is not permission to end the task at another plan. Continue through implementation, target integration and acceptance once the inputs are available.

## Implementation sequence

1. **Define explicit roles and owners.** Standalone owns capture, AP, canonical controls, VP and output through S3 HAL adapters. Assisted mode has exactly one selected engine owner for those functions; S3 owns communications and allowed non-real-time duties. Make role identity visible in health/state. Do not allow two independent canonical states or simultaneous drivers of the same LED line.
2. **Integrate portable semantics before optimisation.** Consume the approved parameter catalogue, feature graph, tempo sidecar and native-output contract. Keep ESP-IDF/FreeRTOS, I2S/PDM, DMA, PSRAM and radio details at the platform boundary. Do not fork units or reinterpret Liveiness, palette travel, trail lifetime or applied-state meaning to make a performance problem disappear.
3. **Admit a bounded standalone feature set.** Measure AP, render, pack and DMA work with radio and the real capture path active. Choose fixed capacities for layers/objects/histories/phase tracks and declare unsupported combinations through capabilities. Disable a feature explicitly if it cannot meet the budget. The default show must remain coherent; do not silently lower colour precision or drop final commands under load.
4. **Place memory deliberately.** Separate DMA-capable/internal memory requirements, cache-sensitive working data, PSRAM-eligible retained state and persistent settings. Prove actual SDK allocations/capabilities and maximum footprints. Do not assume nominal N16R8 module capacity is all usable, or that a PSRAM buffer is directly admissible for every DMA engine. Bound stack, queue and scratch lifetime before enabling the widest graph.
5. **Preserve native output when the actual endpoint supports it.** The product's native16 strategy does not require every scalar to be uint16. Retain fractions through composition and quantise once at the appropriate endpoint. If a particular standalone hardware variant has 8-bit LEDs, use an explicitly declared RGB8 output adapter with its own cadence/dither evidence; never present it as a native16 qualification.
6. **Implement deterministic role transition.** Default engine-add support is a controlled boot/power-cycle transition unless accepted hardware proves powered insertion safe. Discover identity and compatible capabilities, select one owner, migrate/read back canonical base state and only then arm output. Cold-plug support does not imply hot-plug support. A console page/profile is revalidated against the new capabilities while physical faders remain unarmed until takeover.
7. **Define absent/failed engine handling.** On boot, a supported absent engine permits standalone ownership. An incompatible or unresponsive attached engine follows the accepted hardware/role policy, with a visible reason. Runtime engine loss must not cause the S3 to seize connected output pins or run a second engine without a qualified arbitration path. Implement the available output-safe behavior and report its limits; a dead remote MCU cannot be assumed to transmit a black frame on command.
8. **Preserve state and rollback.** Saved appearance state uses versioned canonical units; calibration, device calibration and HMI bindings remain separate. Migration is atomic and reversible for the declared supported versions. Preserve unsupported fields for round-trip where specified, but do not apply them to an incapable engine. New firmware must not corrupt an older compatible role's configuration.

## Shared-state migration transaction

| Step | Required behavior | Evidence |
|---|---|---|
| Quiesce | Suspend new engine-directed gestures; account for accepted terminal values; retain base config | No pending accepted value lost or retargeted |
| Establish authority | Verify selected engine identity/schema/capabilities and ensure exclusive hardware ownership | Exactly one AP/VP/output owner; no concurrent drive |
| Transfer | Migrate a complete canonical snapshot with version/revision and acknowledged application | No partially applied A/B configuration |
| Reconcile | Read back applied base state and effective capabilities; revalidate bindings | Unsupported controls show a reason, not a fake applied value |
| Resume | New session epoch; re-arm faders for pickup; recover page/lens context | Stationary physical positions do not overwrite state |
| Fail/recover | Preserve previous config and use the admitted electrical/firmware recovery path | No uncontrolled dual ownership or silent partial migration |

For boot-only switching, the transaction spans the controlled shutdown/startup lifecycle rather than pretending both images are simultaneously alive. Whether old engine history is transferred, cleared or re-warmed is a declared policy; canonical settings are not the same as ephemeral AP/VP state. Never reconstruct a historical trail using a new speed parameter unless the feature explicitly defines that behavior.

## Build and deployment runbook

No standalone build, flash or serial command is verified by the source packet. Resolve the current wrapper from the identified repository rather than installing guessed tooling or copying DualMCU commands. Produce exact executable/argv, working directory, version, image partition/address ranges, hardware manifest and output names. Inspect existing upload-target guards and preserve their negative tests.

Once the source and hardware gates pass: isolated authorized edits → source-bound host vectors → explicit target environment build → map/capacity review → image/config/rollback manifest → exclusive physical-bench lease → target identity validation → authorized guarded installation if necessary → byte/application identity verification → startup/functional tests → loaded test → controlled engine-add/recovery tests → final state/readback. Reuse existing implementation/programming authority; this handoff creates neither a universal no-flash rule nor a fresh permission ceremony. Unknown identity or ownership stops only affected device writes.

Deployment is consumers-first across mixed-role systems. New standalone or assisted firmware must continue to understand old clients while HMI/catalogue updates roll out, or use a coordinated complete compatible tuple. Console profile activation happens after engine capability confirmation. Rollback restores a compatible image/configuration/profile tuple, not merely the S3 binary.

## Acceptance and what done means

| Criterion | Pass condition | Required evidence |
|---|---|---|
| Source/hardware resolved | One named current standalone path and accepted board manifest with exact pins/rates/memory/output role | Authority links, inventory, hashes and resolved build/upload commands |
| Functional parity | Accepted compatibility vectors pass; intentional R2 behavior is separately identified | PCM→features and full VP/output comparisons |
| Control semantics | Same canonical units/scopes; stale transactions rejected; atomic changes; truthful state; correct pickup after reconnect | Shared fixtures plus actual standalone round trip |
| G3 standalone timing | Worst admitted combined load meets `hop_samples/sample_rate` and frozen VP/output deadlines with zero unexplained loss/deadline/fault/reset increments | At least ten uninterrupted minutes, actual maxima/interval counters; intermediate lane-local evidence |
| Standalone release campaign | Before the separately named standalone hardware release, complete IVV's final coherent-system campaign: at least 30 minutes plus ten individually scored recoveries; retain stronger existing gates | Candidate-bound duration, fault/progress counters and transition/recovery outcomes |
| Resource bounds | Internal/DMA/PSRAM usage fits real regions and predeclared stack/reserve budgets; no AP/VP unbounded allocation | Linker/SDK allocation maps, queue/stack high-water and ownership trace |
| Output correctness | Correct admitted protocol, centre-origin geometry, native words or declared RGB8 adapter and completed black/output generation | Pack/capture vectors plus target/physical scope evidence |
| Engine absent | S3 runs the admitted standalone show after supported boot sequence | Identified source/image/board run |
| Engine added | Supported engine takes ownership through the declared lifecycle; settings survive; HMI capabilities and takeover update correctly | Recorded transition with applied-state/readback and no dual driver |
| Incompatible/lost engine | Declared failure behavior occurs without unsafe ownership or fabricated continuity claims | Negative identity/capability/liveness trials with real limits recorded |
| Role rollback | Compatible previous role/image/config restored and functioning | Restore receipt and post-restore movement/state |

**Done** requires the actual standalone show and the declared engine-add transition, not merely compilation of shared code. If the product hardware is unavailable, complete source/host/target-ready work and label physical rows NOT TESTED with exact required hardware and next owner. Do not call the overall lane physically accepted. Deliver changed files, accepted feature/capability matrix, command/image/config manifests, bounded resource results, individual acceptance outcomes and final device leave-state.


---

<!-- Source: lanes/11-independent-verification.md -->

# Lane IVV — independent verification, acceptance and release evidence

**Purpose:** decide exactly which capability is established by a candidate, on which lane and hardware. IVV does not implement that candidate and cannot replace a physical observation with a host model. This is an execution handoff; every new acceptance item begins `NOT_TESTED`.

## Mandate, boundaries and dependencies

IVV owns the acceptance catalogue, independent test fixtures/scorers, candidate applicability review, evidence index, defect reports and the final acceptance recommendation. Implementation owners own repairs. The system integrator owns the coherent cross-repository image set and deployment; the orchestrator owns dependencies and assigns resource leases. No author may declare global success for their own change. An IVV reviewer may contribute a failing fixture, but a separate reviewer verifies a scorer change if that change could turn its candidate from FAIL to PASS.

Inputs are the contracts and frozen catalogue, source-bound compatibility references, each lane's declared capacity and timing budgets, candidate commits/dirty-patch identities, build and deployment receipts, and hardware identity. IVV may begin source review and deterministic fixture design in parallel with implementation. Physical claims wait for the actual coherent image set. HMI work does not block AP fixture work; an unavailable Titan bench does not invalidate independently completed DualMCU evidence.

The catalogue `../acceptance-catalogue.json` is normative for this handoff. A local lane brief may impose stricter current repository requirements. Before the first candidate measurement, reconcile any disagreement and freeze the chosen criteria, thresholds, test inputs and evaluator versions. Never lower a limit after observing an inconvenient candidate result. A justified revised limit requires a new evaluation identity, rationale and a new run; preserve the failed run.

Existing instructions require fresh reading at execution. In the captured DualMCU `AGENTS.md`, render work must remain below 2.0 ms per frame at 120 FPS, and audio-to-visual latency below 8 ms. The latter has an unresolved measurement-endpoint ambiguity in the captured evidence. Resolve it with the current lane authority at G1; do not silently substitute a convenient API timestamp. The captured console source bans calibration writes/wizards and uses the factory 0–3.3 V map. Read-only input characterization is permitted. Later recorded authority may supersede either restriction; IVV records the actual authority, not a new permission policy.

## Claim levels and what done means

| Gate | Meaning of done | Evidence that cannot stand in for it |
|---|---|---|
| G0 — preflight | Identity, current instruction, ownership and evaluator-plan checks for the operation being released; future candidate image is bound after build | Requiring an unbuilt image or completed implementation before source work |
| G1 — contracts | Interfaces, domains, capacities, lifecycle/reset matrix, wire compatibility, targets, metric definitions and dependencies reviewed; evaluator inputs frozen | A diagram with undefined units; a proposed identifier treated as allocated |
| G2 — host | Source-bound deterministic fixtures pass against the production implementation or explicitly identified portable core; compile/configuration checks pass | The R2 JavaScript model proving target behavior; a compile proving timing |
| G3 — lane hardware | Exact image installed and identity read back; relevant real inputs, outputs, completion counters, resources and leave-state observed on one admitted lane | Build success, a transport ACK, or submission without generation-matched completion |
| G4 — system | Coherent console/radio/engine contracts and versions; end-to-end control and loaded AP/VP/output behavior; recovery scenarios pass on the declared scope | Independent green lane reports from incompatible commits or profiles |
| G5 — product | Physical geometry, visual semantics, usability, retained compatibility and selected optional features accepted under intended optics; remaining limitations explicitly scoped | A browser animation, screenshot, camera auto-exposure or a numerical precision claim alone |

Each criterion has `PASS`, `FAIL`, `BLOCKED`, `NOT_TESTED`, or reviewed `NOT_APPLICABLE`. A mandatory required criterion cannot be omitted or automatically marked N/A because hardware is unavailable. Use BLOCKED with a dependency and next action. A capability can be “host verified; hardware blocked” without the whole programme being called a failure. “Product done” means every required criterion for the declared product configuration passes, accepted exceptions are explicit and do not contradict a user requirement, regression scope is closed, and deployment/rollback/leave-state receipts are complete. Optional experiments may remain unadopted. Their incompletion does not block the required baseline.

Historical reference evidence is reusable only for the identical claim and unaffected applicability. The R2 packet contains 26 backend checks and 43 DOM/model checks; those are valid historical model evidence, not new firmware, browser-layout, touchscreen, radio, timing or optical evidence. The current catalogue starts clean and never imports those counts as a production pass.

## Bounded execution sequence

1. **Freeze and challenge the plan.** Read current repository instructions; bind source and dirty changes; identify devices, actual crop/full geometry, rate/hop/build flags and active HOLDs. The integrator identifies one coherent candidate set. IVV checks the catalogue, references, negative cases and instrument clocks before candidate measurement. A missing latency definition blocks that latency claim, not unrelated implementation.
2. **Host contract pass.** Execute the targeted CTL/TRN/INP fixtures, AP phase/age fixtures and VP/MOD reference comparisons. Test invalid input and absent evidence as well as success. A scorer that accepts absent counters or zero work is a defect. Apply schema checks to both fixtures and receipts, then semantic checks.
3. **Lane smoke.** Build and inspect memory map; deploy using the lane's existing authorized procedure and target identity. Read back identity and capabilities. Run a short functional smoke to detect boot, mapping, stream, output-generation and blackout faults before long measurements. Record the resulting image and target, not just a flasher exit code.
4. **Focused physical functions.** Exercise each affected feature with the corresponding scenarios below. Test ADC behavior using the real faders; use injected commands for reproducible transport stress but label them separately. Confirm native screen navigation and true output completion.
5. **Loaded system campaign.** For the final admitted maximum-cost configuration, run at least 30 uninterrupted minutes with live or exact replayed audio, active display/context drawing, connected radio, controls and actual LED output. Preserve a stricter current lane requirement if one exists. This minimum carries forward the prior console closeout requirement where applicable and is a proposed final-system minimum for other targets; a ten-minute platform-local G3 run does not substitute for this G4 campaign. It is not permission to replace an existing Titan 40-loop or other mandatory campaign. Include ten separately counted reconnect/recovery cycles in a bounded recovery campaign. Fault injection is separate from the uninterrupted healthy window. Repairing a defect restarts only the affected campaign on the repaired coherent set.
6. **Physical evaluation.** Perform matched precision/appearance comparisons and the user-task walkthrough. Record genuine outcomes including “no visible advantage under these conditions.” Do not make optional dither superiority a prerequisite for a native16 baseline.
7. **Independent closeout.** Verify hashes and applicability, replay scorers from raw data, cross-check reported statuses against the catalogue, review defects/exclusions, and sign the scoped recommendation. Integrator supplies exact deployment and rollback instructions plus physical leave-state. Orchestrator decides promotion using the independent recommendation and existing authority.

Avoid a full combinatorial sweep. Cover each node's identity/on/off/lifecycle, each declared dependency edge, each shared-state boundary, the specific pairwise conflicts below and the actual worst-cost admitted configuration. If a new failure identifies an untested interaction, add one discriminating fixture and record why. Stop adding optional tests once the concrete risk and required gate are resolved.

## Scenario matrix

| Scenario | Minimum fixture / observation | Principal criteria | Required distinction |
|---|---|---|---|
| Silence and low noise | Exact zeros, recorded quiet noise, warm-up, stale data, then an onset | AP-04, VP-03 | Idle animation is not a detected beat; no normalized noise becoming full rhythmic output |
| Musical acquisition | Known clicks at supported bins, metrical ambiguity, tempo ramp, withheld music | AP-01–05 | Canonical clock and expressive field; detector observation versus derived subdivision |
| Delayed publication | Reorder/delay field snapshots, repeat generation, change epoch | AP-02–03, TRN-02 | Publication time cannot refresh old support/observation age |
| Material edit | Change speed, width and persistence with an existing moving object and trail | VP-02, MOD-01 | Future integration versus recomputing all age with the new value |
| Palette continuity | Change palette during a retained RGB trail and during a live-colour field | VP-02–03, MOD-01 | Deposited RGB and live colour have declared different histories |
| Immediate attack | Impulse/pair with persistence and optional attack smoother toggled independently | VP-02, MOD-03 | Declared prompt accent path remains prompt; double-counted attack is caught |
| Fractional colour | Sub-byte overlaps, low ramp, dim tail, clipping, nonfinite inputs, black | VP-01, VP-05 | Complete production path through treatment, pack and output; not unequal bytes alone |
| One-sided edit | A mode, palette, parameter and feature toggle while B has running phase/history | MOD-01, CTL-03 | Only declared global limits may change both channels |
| Page during movement | Moving slider, page/channel switch, late old packet, continued movement | CTL-02, INP-02, UI-01 | Old accepted work retains old target; motion cannot silently acquire new target |
| Multi-fader gesture | Two and nine faders with different targets and a pending final target | CTL-05, TRN-03, UI-02 | Latest-value coalescing does not discard final/discrete/transaction semantics |
| Profile/range edit | Invalid compound draft, valid narrow range, endpoint stepping, out-of-range base | CTL-01/03, INP-03 | Atomic validation; no navigation-time clamping; both endpoints reachable |
| External control | Engine-side accepted base update while fader is idle or captured | CTL-04, INP-02 | Re-arm against accepted base, not a continually changing modulated value |
| Reconnect/reboot | In-flight gesture, lost ACK, remote reboot, profile mismatch, delayed snapshot | TRN-01–03 | No idle dump; no stale epoch replay; correct authoritative snapshot |
| Lens lifecycle | Colour/motion/rhythm lens, two gestures, hold/edit/pin/close/navigation | UI-02–03 | Visibility changes cannot alter binding/command semantics |
| Liveiness meaning | Neutral/min/max at identical music and mode, plus underlying controls | VP-02, MOD-01, UI-03 | Declared perceptible behavior; AP timing and base values preserved |
| Load and capacity | Maximum admitted objects/layers, full output, control/display/radio load | AP-05, MOD-02, DUAL/TIT/S3-02, INT-03 | Admitted limit versus silent drop, overflow or speculative unlimited capacity |
| Blackout/fault | Master zero, authorized fault fixture, recovery using verified black completion | VP-04, DUAL/TIT-02, INT-05 | No smoothing delay past output opportunity; configuration retained; recovery policy preserved |
| Standalone/upgrade | S3-only profile and the actual permitted engine attachment transition | S3-01–02, INT-02 | Capability changes visible; no dual engine ownership, unsafe live hot-plug assumption or silent semantic change |

## Metric definitions and pre-measurement freeze

### Controls and UI

The following are **proposed new product targets**, not measurements already achieved. CTL and HMI owners have been asked to use the same endpoints.

- Healthy accepted-control latency: qualified physical movement event to the corresponding engine **accepted base** revision, p95 ≤50 ms and p99 ≤100 ms. Qualification includes the declared input filtering threshold; also report raw ADC movement-to-qualification delay separately so filtering cannot hide poor response. This metric excludes intentional engine smoothing and optical transfer time. Use at least 2,000 valid correlated events, with at least 500 in each stratum: one moving fader, multiple moving faders, context/settings display activity, and maximum declared normal audio/render/radio load. Score each stratum separately; no pooled percentile can hide a failing stratum. Automated input/command injection is labelled and cannot establish the human sensor segment.
- Correlation requires gesture/sequence, canonical target, channel, epoch, accepted revision, monotonic timestamps and a common or explicitly calibrated clock. Timestamp uncertainty must be ≤1 ms for the 50/100 ms target; otherwise report that uncertainty and do not score a borderline result as PASS. Report counts, min/median/p95/p99/max, empirical CDF or raw sample list, drops and rejected commands. Use nearest-rank quantiles (`ceil(p*N)`) for deterministic scoring. A timeout stays in the denominator and fails the healthy-delivery condition; never discard it as an invalid sample after the fact.
- Final-value delivery: exact terminal target accepted ≤250 ms after a valid healthy session and authoritative snapshot have been re-established. Freeze what “healthy” means (identity, schema, epoch, capacity and link established). A still-valid same-session final target may be retained; a reboot/new epoch requires revalidation and must never replay obsolete intent. The criterion permits a deliberate discard with a visible re-arm state when the old intent is no longer valid; it does not permit a falsely displayed accepted value. For an already healthy session, the terminal-outcome window begins at qualified release. Intermediate coalescing cannot restart that timer.
- Lens idle: 5.0 s from last meaningful eligible activity, with ≤100 ms native-device timer deviation. Gesture hold/settings/pin suspend the relevant timer as specified. Noisy ADC samples, passive telemetry and animation must not keep it alive. Host fake-clock tests establish state-machine correctness; actual display/touch timing establishes device behavior.
- Read-only input characterization: all nine faders traverse their factory-mapped domains and exhibit correct direction; capture endpoint and stationary noise at bottom/middle/top. No phantom accepted parameter changes during ten stationary minutes under full normal load. Report actual useful repeatable steps and endpoint reachability; do not call CC14 or higher software resolution physical ADC precision. No calibration writes or wizard activation under the captured ban.

### Audio, rendering and output

`hop_period = hop_samples / sample_rate` is derived, not a free global constant. Both 12.8 kHz/96 and 24 kHz/180 yield 7.5 ms, but filters, execution cost, capture path and evidence applicability differ. Record actual build rate and hop plus independent audio frame counters. A historical 12.8 kHz maximum does not qualify a 24 kHz candidate. Preserve each active lane's loss/late/deadline gates and instrumentation; missing counters fail measurement validity. Require zero unintended sample loss, output faults and late/deadline violations in the declared healthy run unless the current named contract explicitly defines a different tolerated behavior. Do not silently interpret expected decimation as loss.

Freeze rendering budget by actual configuration before candidate data. For DualMCU retain the active <2.0 ms effect-work ceiling at 120 FPS, using the original boundary plus separately measured packing/output/service costs. Report maxima and distributions rather than only averages. For Titan and S3 use current authority or an explicitly reviewed pre-measurement allocation; absent budgets remain G1 BLOCKED. Include interrupt/service pressure, stack/heap high-water, fixed pool occupancy, dropped/admitted object counts and DMA buffer lifetime. Dynamic allocation in the real-time path is forbidden where the current contract forbids it.

Use this latency decomposition, with timestamps appropriate to the actual platform:

`audio support end → result available → render selection → frame submission → DMA/timer completion → latch/presentation → observed light`.

Also preserve the onset's acoustic or injected sample position where the claim concerns stimulus response. A result can be fresh at publication but based on older support. A command ACK is not a render selection; DMA submission is not completion; completion may precede optical presentation. Capture each available segment and identify uninstrumented ones. The captured <8 ms DualMCU requirement must be reconciled with these endpoints rather than deleted or presumed met. If no valid optical/timing instrument exists, the optical latency claim remains NOT_TESTED; software segments still yield useful scoped evidence.

The 80-pixel native48 half-chain bound `80*48/800000 + 0.0003 = 0.0051 s` is a **derived example from captured timing**, corresponding to at most about196.1 updates/s before other overhead. Reconfirm the actual endpoint, clock, reset/completion policy and chain length. It is neither a measured cadence nor a promise of 400 Hz. Logical320 pixels and a physically admitted crop must be separate fields in every receipt.

### Functional and perceptual assertions

Integer/wire invariants use exact equality. Floating reference comparisons freeze an absolute/relative tolerance derived from the operation and reference precision before the candidate is measured; “looks close” is not a numeric tolerance. In a complete wide fixture, compare final words against the independently computed bounded/rounded reference and include inputs that a byte round trip demonstrably destroys. Retained compatibility uses existing source-bound goldens; a new feature may differ only under its separately identified profile.

Appearance comparisons use the same bar, optics, geometry, timed wide scene, output transfer and matched measured brightness where available. Randomize labelled candidate order and retain a repeat to expose inconsistency. Record the observer, setup, clips, order and judgements for near-black ramp, tail, fractional motion, overlap, brief attack and quiet gap. No arbitrary “16 effective bits” claim follows from a command width or video. Numerical correctness can pass while a candidate remains visually unadopted.

For HMI usability, run five concrete tasks with the user or a representative operator: independently adjust Top and Bottom; find/change a Colour parameter; recover pickup after a page switch; interpret then pin/close a context lens; edit a user range and undo it. Record completion, wrong-channel actions, help needed and explanation of the lens. Proposed acceptance is all five completed without hidden-gesture instruction and zero unintended wrong-channel output changes. This is a formative product check, not a statistically general usability study. Every enabled mode in the frozen promised target catalogue additionally needs its Liveiness neutral/min/max comparison and its declared motion/material effect judged noticeable. Historical23 is refreshed at preflight. Unavailable adapters must be visibly unavailable during integration; hiding them does not satisfy the full-release universal-mode promise in MOD-04.

## Evidence identity and receipts

Every result binds at least:

`run_id + criterion_id + lane + source-set identity + build/image identity + configuration/profile/schema identity + fixture/evaluator identity + hardware/scope identity`.

Source-set identity includes repository URL/path, commit, dirty diff hash and hashes of affected working files. Build identity includes toolchain and flags, rate/hop, feature capacities, map-file hash and image hash. Hardware identity includes board/MCU, serial/MAC when applicable, debugger/programmer, LED model/revision if known, physical chain length/crop/topology, power/brightness/current constraint, optics and firmware readback. Mark unknowns explicitly. Configuration identity includes parameter values, schema/capability hashes, channel/profile/page, enables and reset epoch. Test identity includes input PCM hash/sample format, scenario version, random seed/order, evaluator version and instrumentation clock/calibration.

The receipt records start/end monotonic and wall-clock times, planned versus actual duration/work count, raw paths and SHA-256 hashes, sample/counter completeness, thresholds and their provenance, result with reason, measured values/units, limitations, author and independent reviewer. Hardware, target-injected, host-portable, model and visual-render evidence are distinct classes. Do not upgrade one by changing its filename. SHA-256 checks protect artifact identity; they do not prove test correctness or physical behavior.

Retain raw failures, retries and excluded data with reasons. Any invalid-run exclusion is decided by a predeclared instrumentation rule, not by the result being poor. Evidence regeneration must not modify the frozen baseline to fit the candidate. An instrument self-test checks that missing counters, zero progress, wrong generation/epoch, late completion and unexpected reset cannot pass the scorer.

## Isolation, deployment handover and closeout

IVV issues defects to the owning lane with failing fixture, exact identity, observed/expected result and dependency impact. A CTL atomicity defect blocks HMI applied-state integration; it does not block AP math work. A Titan output-completion defect blocks Titan hardware/product acceptance; it does not block a completed DualMCU host claim. A shared-contract identity conflict blocks every dependent consumer until resolved. Only the orchestrator changes the dependency graph; agents cannot silently invent a substitute contract.

Hardware access uses one lease per physical target, owned by the integrator or designated platform specialist. IVV observes or schedules a bounded campaign under that lease; it never concurrently flashes a board another agent is measuring. Flash/build tools follow current authorized permissions and actual HOLDs. This handoff grants no new quota restriction or user-confirmation cycle.

The final report names capability status by lane, source/image/profile identities, pass/fail/blocker matrix, exact measured quantities, physical coverage, accepted limits, rejected alternatives, open defects and a reproducible command index. It records the running image, selected profile/mode/page, output state, transport state and any disconnected instrument. Rollback names the known-good image and matching contract/profile, with preconditions and verification; “revert code” alone is insufficient across several devices.

Global completion requires an integrator recommendation plus independent IVV applicability review. A lane author can state “my lane meets its local criteria on this candidate”; the author cannot promote that into “the K1 system is done.”


---

<!-- Source: deployment-source-notes.md -->

# Deployment command provenance and unresolved inputs

This handoff is source-backed planning. It does not report a new build, installation, hardware measurement or fresh inspection of the user's current Mac. Captures may contain dirty working-file bytes; a historical HEAD alone is insufficient for reproduction. Refresh only the affected inputs at execution, retaining previous receipts.

## Source locations used

The following paths are relative to the preceding source packets, available in this workspace when the handoff was prepared:

| Evidence | Material finding |
|---|---|
| `k1-pipeline-integration/evidence/precision/dualmcu/AGENTS.md` | Engine/bridge ownership, core HAL boundary, no hot-path allocation, effect 2 ms/120 FPS and audio-to-visual 8 ms requirements, no sibling worktrees, identity guard |
| `k1-pipeline-integration/evidence/precision/titan/AGENTS.md` | Scalar-first, portable reference, platform memory, E2 router/rfp-cli and no GDB/retired writer rules |
| `k1-pipeline-integration/source-snapshot/titan/docs/REFERENCE-MANIFEST.md` | DualMCU authority `6b1e7bc…`, BSP evidence `6dd0a70…`, required reference checker |
| `k1-control-r2/source-evidence.json` | 24 September source paths and exact hashes for active console and DualMCU files |
| `k1-pipeline-integration/evidence/ap-source-manifest.json` and `evidence/ap-runtime-excerpts.json` | Portable AP/capture publication source identity and timing seams |
| `k1-pipeline-integration/source-snapshot/titan/platform/ra8p1/palette_runtime.cpp`, lines 122–184 | Broad A/B resets and shared runtime reset on config; new selective-reset task is substantive |
| Same file, status/completion section around lines 402–492 | Different output profile pin variants plus submitted/completed generations and per-lane DMA/stop/fault observations; select actual compiled profile |
| `k1-pipeline-integration/source-snapshot/titan/tools/serial-studio/titan_live_control.py` | Real argparse syntax, CDC/campaign acquisition, hardcoded identity and emit-off mutation behavior |
| `k1-control-r2/source-snapshot/SpectraSynq-K1-DualMCU-Firmware/scripts/generate_console_contract.py`, main | Writes both repositories' generated headers; fixed v1 registry/count guard |
| `k1-pipeline-integration/prior-K1-Console-DualMCU-Execution-Brief-2026-09-23.md` | Documented known environments/host recipes and older assignment-specific device/hold restrictions; not a universal current policy |
| `emotiscope-review/packet/evidence/target-lanes-inventory.json` | Titan inventory names modified `scripts/build_scalar.py`, modified retired `scripts/programme_scalar.py` and untracked `scripts/programme_rfp_swd.py`; no script content or safe argv captured |

## Exact commands and their confidence

“Captured script” means the relevant script/argument parsing was read; dependencies and the user's current copy still require preflight. “Prior documented recipe” means a previous source-bound execution brief gave that exact recipe; the full current target config or wrapper was not captured here. Neither means a fresh invocation succeeded in this handoff.

### Source-captured Titan client syntax

The following are **syntax templates** to run from the currently verified Titan repository after acquiring the appropriate external bench ownership and reviewing current client side effects. `TITAN_RUN_DIR` must name a new absolute evidence directory. Each command opens CDC and takes its own existing campaign lease; they are not safe to launch in parallel against the same device.

```sh
python3 tools/serial-studio/titan_live_control.py schema --run-dir "$TITAN_RUN_DIR"
python3 tools/serial-studio/titan_live_control.py config-get --run-dir "$TITAN_RUN_DIR"
python3 tools/serial-studio/titan_live_control.py snapshot --run-dir "$TITAN_RUN_DIR"
python3 tools/serial-studio/titan_live_control.py events --run-dir "$TITAN_RUN_DIR" --after 0
python3 tools/serial-studio/titan_live_control.py timing --run-dir "$TITAN_RUN_DIR" --after 0
```

These commands verify neither installed bytes nor physical photons. The captured client hardcodes a UID; review against current bench identity and reject a mismatch. Do not weaken that check to reach an arbitrary board. `config-set` is intentionally excluded from the routine readback recipe because it forces emit off and changes flags. It must be part of an explicit saved-config/test/restore transaction if used.

### Prior documented DualMCU/console host recipes

Resolve `DM_SOURCE`, `CONSOLE_SOURCE`, `PIO_EXE` and `HOST_RUN_DIR` to current absolute paths before use. These are variable names for a command template, not discovered environment settings. The earlier documented PlatformIO binary was `/Users/spectrasynq/.platformio/penv/bin/pio`; verify it exists and belongs to the current toolchain. Run these from `DM_SOURCE`:

```sh
"$PIO_EXE" run -e native_s3_radio_core
.pio/build/native_s3_radio_core/program
"$PIO_EXE" run -e native_ble_dualmcu_host
.pio/build/native_ble_dualmcu_host/program
```

Earlier console host recipes:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  "$CONSOLE_SOURCE/tests/test_k1_profile.cpp" -o "$HOST_RUN_DIR/console-profile"
"$HOST_RUN_DIR/console-profile"
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  "$CONSOLE_SOURCE/tests/test_k1_snapshot.cpp" -o "$HOST_RUN_DIR/console-snapshot"
"$HOST_RUN_DIR/console-snapshot"
```

Current sources may require new include paths or a different wrapper. Resolve a concrete build failure from the actual current build configuration; do not relabel these historical recipes as fresh passing tests. Capture real exit codes even when logging through pipes. New R2 contract/gesture tests supplement these legacy suites.

Prior documented target build environments are `console` in K1.Sliders and `s3_radio_bridge_ble` in DualMCU. A target build template is `"$PIO_EXE" run -e "$VERIFIED_ENV"`, run from its owning repository after checking current `platformio.ini`. **No upload command is given here** because the complete current guard wrapper/address/identity procedure was not captured. The captured DualMCU instructions require `python3 harness/check_upload_target.py --self-test` and the current identity canon before flash; read that script and current build wrapper rather than substituting an unguarded `-t upload`.

### Mutating generator warning

The captured `scripts/generate_console_contract.py` has no CLI options; `python3 scripts/generate_console_contract.py` executes its main routine. That routine validates fixed v1 identity/counts, then writes `DUAL_HEADER` and `SLIDER_HEADER`. Inspect their currently resolved destinations first. This is **a cross-repository mutation**, not a dry-run check. The R2 handoff requires CTL to stage exports first and each consumer owner to import reviewed outputs; change/add a bounded export path before using this legacy tool in the new workflow. Do not let parallel DUAL/HMI agents invoke its implicit cross-repository writer. If maintaining the old v1 regeneration procedure, reserve both output paths and use its existing coordinated owner procedure separately. Preserve v1 assertions; add a separately versioned generator/contract for R2 where designed, rather than weakening the old validator.

### Required current-command resolution

| Missing command or input | Resolution owner and bounded action | Consequential action waiting on it |
|---|---|---|
| RT1062 actual target environment, guarded upload argv and native vs WS2812 variant | DUAL reads current `platformio.ini`, wrapper and identity canon; records exact argv and output filenames | Engine build variant acceptance / installation |
| Titan generated scalar build wrapper/overlays and rfp-cli write/verify argv | TIT loads current platform memory/router skill; reads known inventory paths `scripts/build_scalar.py` and `scripts/programme_rfp_swd.py`; inspects their argparse and side effects before safe help; resolves exact image/address/target procedure | Titan image build identity / write |
| Titan current pin/profile selection and microphone configuration | TIT binds current build defines + hardware manifest + live identity | Physical mapping and timing claim |
| Standalone repository, build environment, board revision/pins and engine arbitration | S3 resolves current project authority and accepted hardware manifest | Standalone target implementation and physical operation |
| Active bench identities, leases, current HOLDs and rollback images | Each platform owner reads current canon and owner state once at campaign preflight | Device access/write and restore |

Do not execute text containing unresolved placeholders. Resolve and record exact commands before their dependent action; keep independent design/host tasks progressing. Routine reversible implementation/testing uses already applicable authorization. Unknown identity, missing rollback prerequisites for a destructive update, conflicting active ownership or a failed required test creates a specific stop/resumption condition, not a blanket request for permission or quota restriction.

## Unified deployment receipt

Each platform's `resolved-commands.json` is an implementation deliverable with: command purpose, source script/path/hash, exact executable and argv array, working directory, tool version, selected environment/defines, inputs and expected outputs, identity guard, known side effects, success predicate, and logging location. Store secrets outside the receipt. Record actual invocation results separately, including start/end monotonic times, exit code and raw log hashes. A planned argv is not an execution result.

An installation receipt additionally identifies hardware UID/serial/MAC as appropriate, board manifest, image hash, verified byte address/range, configuration schema/revision, rollback tuple, post-boot runtime identity and final leave-state. Preserve all failed attempts; source modifications or image changes start a new candidate/run identity. Independent IVV must be able to distinguish source present, host verified, built, installed, target functional, physically observed and adopted.


---

# Programme acceptance catalogue

All rows start NOT TESTED. Full structured fields are in acceptance-catalogue.json.

## CTL-01 — Versioned contracts have complete, bounded and unambiguous semantics.

Owner: CTL. Gate: G1. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Review descriptors, generated schema/profile tables and schema-level positive/negative vectors before production implementation.

Pass: Every admitted parameter/feature declares type, units, scope, range, identity, enable, dependencies, capacity, lifecycle, response owner and capability behavior; schema-level invalid cases rejected; legacy identifiers retain meaning. This G1 gate freezes contracts and does not require production transaction behavior, which is tested at G2 CTL-03.

Evidence: reviewed-contract-hashes, capacity-and-reset-matrix, schema-vector-validation-results.

## CTL-02 — Captured gestures cannot be retargeted by page, channel or profile changes.

Owner: CTL. Gate: G2. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Delay/reorder old gesture events through route changes, external edits and binding revisions.

Pass: Zero commands reach a different canonical target/channel; stale epoch/sequence/binding work rejected; accepted old work keeps old target; continuing physical motion cannot silently acquire new binding.

Evidence: correlated-command-trace, before-after-state, negative-fixture-results.

## CTL-03 — Profile and multi-parameter transactions apply atomically without unrelated state mutation.

Owner: CTL. Gate: G2. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Valid/invalid compound edits, field deltas, A-only changes and boundary publication.

Pass: Invalid edit leaves all state/revisions unchanged; valid edit publishes only after all fields apply; A-only edit preserves B and AP except declared global effects.

Evidence: transaction-state-trace, reset-matrix, atomicity-fixture-results.

## CTL-04 — Requested, accepted base, effective and unavailable states remain truthful.

Owner: CTL. Gate: G4. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Delay, reject, supersede and independently modify commands; compare UI receipts with engine state.

Pass: Local echo or transport ACK never claims engine acceptance; receipts identify target/epoch/revision; pending/rejected/unavailable states visible; effective modulation does not overwrite base targets.

Evidence: apply-receipt-trace, engine-state-snapshot, UI-state-capture.

## CTL-05 — Healthy control delivery is responsive and terminal outcomes are retained.

Owner: TRN. Gate: G4. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Instrument qualified movement, engine accept and producer confirmation under four load strata; inject recovery.

Pass: Per-stratum p95 movement-to-engine-accept <=50 ms and p99 <=100 ms; valid final target or explicit expired/discarded outcome confirmed <=250 ms after healthy session/snapshot service resumes; no obsolete replay.

Evidence: raw-correlated-latency-samples, clock-uncertainty, per-stratum-statistics, terminal-outcome-trace.

## AP-01 — Disabled new consumers preserve incumbent analysis and winner/flywheel behavior.

Owner: AP. Gate: G2. Requirement origin: existing. Targets: dual, titan, standalone.

Method: Run identical source-bound PCM and golden fixtures with new expression consumers disabled.

Pass: No unexplained legacy feature/winner/beat/phase delta against frozen applicable baseline; changed optional field is separately identified.

Evidence: PCM-and-baseline-hashes, numeric-differential-results, detector-configuration.

## AP-02 — Tempo evidence retains stable identity and honest freshness.

Owner: AP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Known snapshots with rank changes, cached copies, delayed publications, reset and differing family ages.

Pass: Candidate IDs identify bins rather than ranks; generation advances only on new evidence; support/observation/availability times remain distinct; stale or wrong-epoch data cannot gain fresh authority.

Evidence: tempo-field-trace, epoch-and-generation-fixtures, phase-observation-age-results.

## AP-03 — Expressive phases are coherent without altering the canonical clock.

Owner: AP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Known click inputs, phase anchors, half/double relationships, wrap, reset, delayed evaluation and reversed A/B render order.

Pass: Frozen phase convention and frequency match reference within predeclared tolerance; half-time retains beat parity; render order/count cannot advance shared AP phase; derived subdivisions are labelled.

Evidence: phase-reference-definition, numeric-phase-fixtures, clock-immutability-trace.

## AP-04 — Silence, noise, warm-up and stale evidence do not fabricate musical authority.

Owner: AP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Zero/quiet noise, acquire/coast/stale transitions, invalid fields and zero-salience normalization.

Pass: Zero evidence contributes zero; no fabricated detected onset; warm-up/stale/invalid reasons exposed; declared bounded coast expires; idle animation is separately identified.

Evidence: silence-and-noise-fixtures, authority-state-trace, coast-policy.

## AP-05 — New analysis fits actual lane capture and scheduling budgets.

Owner: AP. Gate: G3. Requirement origin: existing. Targets: dual, titan, standalone.

Method: Run actual rate/hop image under admitted full load with timestamped AP counters and cost distribution.

Pass: No unintended sample loss or missed applicable AP deadlines; max and tails within pre-frozen lane budgets; memory/state bounded; 12.8k/96 evidence never qualifies 24k/180 by arithmetic alone.

Evidence: rate-hop-build-identity, AP-timing-and-loss-counters, lane-budget, memory-high-water.

## VP-01 — The new appearance path preserves fractional composition to native RGB16.

Owner: VP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Independent wide scalar fixtures through complete palette/treatment/packing path including sub-byte overlaps.

Pass: Final native words match frozen bounded/rounded reference; byte-roundtrip-destroyed cases survive; no hidden byte narrowing; legacy path remains a separately named adapter; native path has no RGB8 residual dither.

Evidence: wide-reference-inputs-and-words, complete-stage-trace, packing-output-fixtures.

## VP-02 — Motion and material parameter changes have declared continuous behavior.

Owner: VP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Edit speed, width, decay, Liveiness and palette during moving objects/trails; vary dt and long gaps.

Pass: No teleport, retroactive recomputation or unintended relight; immediate-attack promise holds; dt results meet pre-frozen tolerances; Liveiness neutral is identity and bases/AP time preserved.

Evidence: temporal-reference-fixtures, material-lifecycle-definition, Liveiness-adapter-matrix.

## VP-03 — Palette, colour-domain and output treatment meanings remain explicit.

Owner: VP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Identity and nonidentity treatment fixtures, palette transition, deposited-RGB versus live-colour history.

Pass: Identity correction exact at designated boundary; no double gamma or undocumented linear-light claim; histories follow declared policy; final gain/current/quantization do not feed back unintentionally.

Evidence: colour-domain-table, identity-treatment-fixtures, history-transition-trace.

## VP-04 — Centre-origin topology and true-black behavior survive the full output path.

Owner: VP. Gate: G3. Requirement origin: existing. Targets: dual, titan, standalone.

Method: Sparse 0/79/80/159 logical fixtures per channel and authorized output black/fault sequence.

Pass: Logical160/channel and centre79/80 correct; admitted physical crop documented; no wrong-channel/pixel output; blackout bypasses artistic smoothing at next permitted output opportunity; exact-zero words and observed black on tested hardware.

Evidence: topology-map, submitted-and-completed-generations, native-output-fixtures, physical-observation.

## VP-05 — Precision and appearance claims are supported on actual intended optics.

Owner: IVV. Gate: G5. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Matched same-bar rounded8-expanded/dithered8-expanded/native16 scenes and direct observation.

Pass: Transfer/cadence/geometry/brightness constraints documented; benefits/tradeoffs recorded per scene; no optical-bit-depth claim from code width/video; optional adapters may remain unadopted without blocking correct native16 baseline.

Evidence: comparison-setup, scene-and-output-identities, actual-cadence-data, observer-record.

## MOD-01 — Module and channel state are isolated under real edits.

Owner: MOD. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Exercise each reset-matrix cause and targeted dependency-edge pairs while another layer/channel is active.

Pass: Only declared owner state resets; unaffected channel/history/phase remains unchanged; config applies at declared boundary; common audio is immutable.

Evidence: reset-matrix-coverage, state-and-output-differentials, module-ownership-review.

## MOD-02 — Admitted graphs have fixed bounded resources and deterministic overload behavior.

Owner: MOD. Gate: G3. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Fill each pool to capacity and one beyond; max graph; inspect allocation and ownership.

Pass: No overflow/alias/use-after-publish; declared drop/admission policy; no forbidden RT allocation; high-water within frozen capacity and headroom budget.

Evidence: capacity-definition, pool-boundary-results, memory-map-and-high-water, ownership-trace.

## MOD-03 — Every admitted artistic feature has a useful independent enable and amount.

Owner: MOD. Gate: G2. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Bypass, zero amount, disable/re-enable, missing dependency and transition tests for each admitted node.

Pass: Bypass has declared identity and lifecycle; enable distinct from amount; no accidental dependency reset; unavailable reason explicit; selected pairwise interactions preserve module contracts.

Evidence: feature-toggle-matrix, bypass-traces, dependency-coverage.

## MOD-04 — Universal Liveiness and parameter merit are complete for the frozen enabled-mode catalogue.

Owner: MOD. Gate: G5. Requirement origin: proposed. Targets: dual, titan, standalone.

Method: Refresh and freeze current target mode list; review adapter contract and physical neutral/min/max comparisons for every mode; verify common colour and advanced control availability.

Pass: Every enabled mode in the promised target catalogue has a versioned useful Liveiness adapter with neutral identity and accepted behavior; bases/AP time and mode identity preserved. Historical23 is refreshed, not assumed permanent. Interim unsupported labels cannot pass this full-release claim; default Chroma/Saturation placement follows declared merit.

Evidence: frozen-enabled-mode-catalogue, per-mode-adapter-and-visual-results, parameter-merit-matrix, base-and-clock-invariance-results.

## TRN-01 — Compatibility identities and capability negotiation reject mismatched peers safely.

Owner: TRN. Gate: G2. Requirement origin: existing. Targets: dual, titan, console, standalone.

Method: v1 fixture replay, cross-version hello/snapshot/profile/schema mismatches and bounds.

Pass: Existing v1 channel/CC semantics unchanged; no identity/hash weakening; incompatible peer rejected without engine mutation; matched profile/capabilities required before edits.

Evidence: wire-golden-fixtures, negotiation-traces, rejection-state-snapshots.

## TRN-02 — Reconnect and reboot recover authoritative state without stale replay.

Owner: TRN. Gate: G4. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Ten distinct recoveries including in-flight terminal, lost confirmation, reset and mismatch.

Pass: Each reaches declared valid snapshot state or explicit safe incompatibility; no stationary fader dump, obsolete epoch command, false applied state or wrong target; terminal policy satisfies CTL-05.

Evidence: ten-recovery-case-log, session-and-epoch-trace, snapshot-integrity-checks.

## TRN-03 — Queues and snapshots remain bounded under backpressure.

Owner: TRN. Gate: G2. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Nine-fader bursts, delayed ACKs, duplicates, discrete actions, snapshot fragmentation and saturated queues.

Pass: Latest target per valid gesture retained with terminal/discrete ordering; bounded storage and explicit overflow policy; no partial snapshot/transaction commit or AP/VP starvation.

Evidence: queue-occupancy-trace, burst-and-fragment-fixtures, terminal-order-results.

## INP-01 — Physical fader behavior is measured without inventing ADC precision or changing banned calibration.

Owner: INP. Gate: G3. Requirement origin: proposed. Targets: console.

Method: Read-only full travel and bottom/middle/top noise on nine real faders; ten-minute stationary full-load window.

Pass: Correct direction/endpoints characterized; zero phantom accepted changes; useful repeatable resolution reported; factory map/calibration policy unchanged unless later recorded authority supersedes.

Evidence: nine-fader-travel-record, stationary-ADC-and-intent-log, calibration-authority-reference.

## INP-02 — Nonmotorized takeover prevents navigation and reconnect jumps.

Owner: INP. Gate: G3. Requirement origin: proposed. Targets: console.

Method: Physical page/channel remap, crossing, continuous movement, external base change and reconnect.

Pass: Navigation emits zero parameter writes; target/physical markers separate; pickup crossing works; held motion cannot acquire new target without declared quiet/re-arm condition; stationary positions never replay.

Evidence: physical-gesture-video-or-observation, ADC-and-command-trace, pickup-threshold-rationale.

## INP-03 — Mapping, fineness and response controls preserve reachable bounded domains.

Owner: INP. Gate: G2. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Monotonic/inverse, log/curve, coarse quantization, zero/endpoints, narrow range and variable-dt fixtures.

Pass: Both configured endpoints reachable; mapping/filter/transport threshold/damping distinct; out-of-range base not silently clamped on navigation; absolute acceleration rejected or absent; long-gap policy explicit.

Evidence: mapping-reference-fixtures, endpoint-and-quantization-results, response-owner-and-gap-policy.

## UI-01 — Channel-first pages make all nine current assignments and global authority clear.

Owner: UI. Gate: G3. Requirement origin: proposed. Targets: console.

Method: Native600x450 capture and actual navigation with physical faders.

Pass: Primary/Top versus Secondary/Bottom visible; all nine dynamic labels and values on-screen; global master/blackout outside bank; no hidden remap or assumed secret gesture; unsupported slots explained.

Evidence: native-screen-captures, navigation-command-trace, physical-task-record.

## UI-02 — Context dashboards explain changes without taking over controls.

Owner: UI. Gate: G3. Requirement origin: proposed. Targets: console.

Method: Colour/motion/rhythm lens, simultaneous gestures, pin/close/settings and idle timing.

Pass: Correct channel/domain/context; opening/closing never remaps; exact prior page restored; default5.0s idle within100ms native deviation; held/edit/pinned state honored; noise/telemetry not activity.

Evidence: lens-state-fixtures, native-timer-measurement, context-meaning-review.

## UI-03 — Progressive disclosure provides usable depth on the physical instrument.

Owner: UI. Gate: G5. Requirement origin: proposed. Targets: console.

Method: Five defined operator tasks plus every advertised mode Liveiness neutral/min/max comparison.

Pass: All five tasks complete without hidden-gesture instruction and zero unintended wrong-channel output; every advertised Liveiness response judged meaningful under declared conditions; settings edit/undo accessible; illustrative colour labelled.

Evidence: operator-task-record, Liveiness-visual-matrix, native-touch-layout-review.

## DUAL-01 — DualMCU preserves engine/radio ownership and actual SPI boundaries.

Owner: DUAL. Gate: G3. Requirement origin: existing. Targets: dual.

Method: Review current source and run link under concurrent AP/VP/radio/control load.

Pass: RT1062 owns AP/VP/control/output; S3 remains transport bridge; existing READY/descriptor/CRC and buffer ownership contracts pass; no pixel streaming or policy migration to S3.

Evidence: source-ownership-review, SPI-link-counters, target-identities.

## DUAL-02 — DualMCU candidate meets current real-time and output gates.

Owner: DUAL. Gate: G3. Requirement origin: existing. Targets: dual.

Method: Source-bound loaded image measurement and sparse/native/fault completion fixtures.

Pass: Effect work <2.0ms at120FPS if current authority retained; current loss/deadline/fault gates pass; <8ms audio-to-visual requirement uses resolved G1 endpoint definition; generation/epoch completion correct and scope explicit.

Evidence: resolved-timing-contract, runtime-distributions-and-counters, native-output-completion, image-readback.

## TIT-01 — Titan retains authoritative import, configuration and runtime boundaries.

Owner: TIT. Gate: G3. Requirement origin: existing. Targets: titan.

Method: Review portable reference/import identity; configuration delta tests and actual capture/publication path.

Pass: No unreviewed DualMCU source overwrite; A-only configuration preserves B/history/dwell except declared causes; actual rate/hop and staged/direct control semantics bound to image; M85 path primary unless separately qualified.

Evidence: import-reference-and-manifest, configuration-state-trace, build-and-rate-identity.

## TIT-02 — Titan output completion and loaded timing are physically established.

Owner: TIT. Gate: G3. Requirement origin: existing. Targets: titan.

Method: Generation/epoch fixtures, both-lane DMA/timer stop observations, black recovery, full load.

Pass: Matching submitted generation+live epoch+both lane DMA-complete/hardware-stop bits+explicit zero faults; incomplete/wrong/faulted black cannot restore output; current AP/hop/output budgets and counter-progress requirements pass.

Evidence: generation-completion-trace, negative-completion-fixtures, AP-and-output-counters, target-readback.

## S3-01 — Standalone S3 implements the same meanings within explicit reduced capabilities.

Owner: S3. Gate: G1. Requirement origin: proposed. Targets: standalone.

Method: Resolve actual source root and platform; review/host test admitted descriptor/profile set.

Pass: Canonical units and control meanings preserved; unsupported features explicit; finite S3 capacity/performance budgets frozen; no false native16 promise if endpoint differs; missing root leaves standalone scope blocked only.

Evidence: standalone-source-and-platform-identity, capability-profile, resource-and-timing-budget.

## S3-02 — Standalone execution and later engine upgrade preserve one authority.

Owner: S3. Gate: G4. Requirement origin: proposed. Targets: standalone.

Method: Actual S3 image load and supported attach/migration/reboot scenario with negotiated capabilities.

Pass: Healthy loaded capture/render/output meets frozen budget; only one AP/VP authority; parameters migrate or explicitly reject unsupported values; no silent semantics changes; live hot-plug assumed only if hardware qualifies it.

Evidence: standalone-loaded-run, upgrade-state-transition-trace, capability-and-state-readback, physical-transition-policy.

## INT-01 — Every claim is applicable to an exact coherent candidate and evidence class.

Owner: IVV. Gate: G0. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Preflight identities, current instructions, hardware lease, hash verification and scorer challenge.

Pass: For the operation being released, required current source/dirty changes, baseline/configuration, fixture/evaluator, target/resource ownership and instruction identities are bound; future image hash is added after build and before deployment. Missing counters, wrong identity, zero progress and incomplete raw data cannot pass scorer self-checks. Numeric gates and instrumentation definitions frozen before candidate qualification.

Evidence: preflight-identity-tuple, frozen-evaluation-plan, scorer-negative-self-checks, hardware-lease-record.

## INT-02 — Cross-repository integration preserves compatibility and independent channel state.

Owner: INT. Gate: G4. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Deploy matched console/radio/engine set; current v1 and new profile regression; required route/reset interactions.

Pass: No incompatible mixed set treated as qualified; old semantics retained; optional features off preserve baseline; navigation/profile/one-channel changes leave unrelated AP/VP state intact; admitted platform differences explicit.

Evidence: coherent-set-manifest, integration-scenario-results, compatibility-differential, cross-channel-state-trace.

## INT-03 — The final admitted configuration survives meaningful full-system load.

Owner: IVV. Gate: G4. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: At least30 uninterrupted minutes full normal load plus separate focused recovery campaign; preserve stricter current lane campaigns.

Pass: Required audio/capture/loss/late/output/CRC/fault/resource counters present and within frozen gates; no unintended loss/fault/deadline violation; nonzero progress throughout; max-cost admitted configuration and actual geometry documented.

Evidence: raw-loaded-run, counter-completeness-check, timing-and-memory-statistics, configuration-and-duration-record.

## INT-04 — Latency claims use declared real endpoints and actual presentation constraints.

Owner: IVV. Gate: G4. Requirement origin: derived. Targets: dual, titan, standalone.

Method: Timestamp support/onset, availability, render, submission, completion/latch and optical observation where claimed.

Pass: All claimed segments instrumented or explicitly unmeasured; active lane latency limits retained after endpoint resolution; output cadence obeys actual chain/protocol bound; no API-return proxy or400Hz native48 promise.

Evidence: latency-endpoint-contract, correlated-segment-trace, wire-bound-calculation, instrument-uncertainty.

## INT-05 — Complete product geometry, safety authority and intended visual behavior are accepted.

Owner: INT. Gate: G5. Requirement origin: existing. Targets: dual, titan, standalone.

Method: Actual mapping/current/blackout/recovery and intended optics observation on declared full product scope.

Pass: Both160-pixel logical channels and their admitted full physical output mapping qualified; centre79/80 preserved; current/blackout/fault authority correct; partial bench crop cannot pass full product claim; visual/user acceptance recorded.

Evidence: full-product-topology-observation, blackout-and-recovery-results, current-limit-configuration, product-acceptance-record.

## INT-06 — Completion and deployment are independently reproducible and scoped.

Owner: IVV. Gate: G5. Requirement origin: proposed. Targets: dual, titan, console, standalone.

Method: Independent hash/applicability/scorer review and exact install/readback/rollback rehearsal or verified procedure.

Pass: Required criteria for declared target scope pass; blockers/exclusions explicit; author cannot self-declare global pass; source/image/profile/devices match; reproducible commands and known-good rollback/leave-state provided.

Evidence: independent-review, scoped-acceptance-matrix, deployment-and-rollback-instructions, physical-leave-state.

