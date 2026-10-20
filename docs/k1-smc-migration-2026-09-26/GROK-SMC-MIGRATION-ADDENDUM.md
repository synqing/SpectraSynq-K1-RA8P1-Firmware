# SMC-TITAN-CONTROL-001 — migration precedence and onward execution

**Issued:** 26 September 2026, Australia/Perth.  
**Task:** Fold the Captain's migration from the 2.41-inch ESP32-S3/nine-slider surface to the owned commercial SMC-Mixer into Grok's implementation instructions.  
**Execution lane:** Existing `SMC-TITAN-CONTROL-001`; do not create a replacement programme.  
**Evidence class of this addendum:** USER_DIRECTION + SOURCE_REVIEW. No tests, firmware builds, MIDI sessions, CDC sessions, programming or physical measurements were performed in preparing it.

## 1. Decision and precedence — read this before either old brief

The active physical-control delivery path is the commercial M-VAVE SMC-Mixer (called M-Wave by the Captain), through USB MIDI on the Mac, to Titan. Completing the retired custom 2.41-inch/nine-slider surface is **not** a prerequisite for this migration.

This addendum supersedes the earlier recommendation to execute `P1-CLOSE` on K1.Sliders and then the console → BLE → DualMCU v2 chain as the immediate critical path. In particular, it supersedes that ordering in `K1-Grok-Execution-Brief.md` and `NEXT-EXECUTION-BRIEF.md`. Those files are retained as historical instructions and defect evidence, not independent permission to resume the old surface.

The current instruction changes applicability and scheduling, not historical truth. Do not delete earlier failures or mark retired-surface tests PASS. Preserve the full parent programme. Map each affected task to one of: retained requirement in the SMC/Titan lane; deferred legacy-surface work; separate untouched platform work; or still-required physical/product qualification. Record the reason and successor coverage. One fader is the first milestone, not a declaration that all controls, modes, targets or full-product topology are complete.

The specific existing SMC brief remains required in full:

`/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001/docs/execution/SMC-TITAN-CONTROL-001.md`

Also read the adjacent `SMC-TITAN-CONTROL-001-DISPATCH.md` and `SMC-TITAN-CONTROL-001-GATES.md`, current repository instructions, existing task/ownership records and the retained Titan programme. The original SMC brief's optional Bluetooth alternative is superseded for this assignment by **USB Master only**. Its proposed sibling worktree locations were not used: use the existing in-repository worktrees below. The later source-selected first path in section 5 supersedes stale hue-first descriptions.

## 2. Mission and fixed architecture

Deliver a physical mixer that changes an intended, demonstrably consumed Titan visual parameter, obtains authoritative application feedback, preserves the running audio and visual state, and permits an explicitly saved look to be recalled reproducibly.

```text
Owned SMC-Mixer
  USB MIDI: SINCO SMC-Mixer-Master only
       |
macOS CoreMIDI receive-only input
       |
SMC host adapter: decode / mapping / pickup / coalescing / tuning view
       |
Restricted versioned local IPC
       |
Persistent Titan broker: sole CDC owner, bounded command arbitration
       |
Titan frame-safe partial visual update + authoritative readback
       |
Existing Titan audio / visual renderer / owned LED backend
```

The Mac carries controls and feedback, not audio analysis, rendering or pixel streams. Titan retains AP, VP, scheduling, output and safety authority. The commercial mixer is an input device; do not expect it to implement the old bespoke K1-CONSOLE GATT identity, CTL v2 transactions or profile storage.

**Do not add** an ESP32 receiver, direct USB-host firmware on Titan, Titan BLE stack, new control PCB, Bluetooth failover, DAW control emulation or Blender dependency to get the first tuning path working. Do not replace Titan's existing partial-update protocol with the DualMCU transport merely because the earlier console brief specified that transport.

MIDI OUT stays closed. Button LEDs, display feedback, motor-fader commands, SysEx configuration and controller factory reset are not part of the initial receive-only path. Requested/acknowledged values are shown by the host tuning view; do not promise physical mixer feedback that has not been mapped and separately admitted.

## 3. Owned locations, preserved devices and entry rules

| Area | Existing location | Branch / observed base |
|---|---|---|
| Host adapter worktree | `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001` | `feat/smc-titan-control-001` / `4935fff005768655f5800106616f7a0e86feee6a` |
| Host code | Above worktree: `host/titan_control/` | Current package is `smc_titan_control/` beneath that directory; do not rename it solely to match a proposed layout |
| Titan worktree | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-live-control-001` | `feat/titan-live-control-001` / `8427fccfb1c3b7fface2c2ba0a56a6017b785acf` |
| Titan-owned changes | `tools/serial-studio/`, `platform/ra8p1/`, protocol/schema, necessary build wiring and their tests | Keep controller-specific mappings in SMC; generic target control in Titan |

These were read at 02:58:27 AWST on 26 September. Host additions were untracked; Titan contained modified and untracked work. HEAD is not the candidate. Refresh actual file hashes and owner activity before editing; the source changed beyond the earlier SMC handback while the base commits stayed unchanged.

**Do not edit or flash K1.Sliders or DualMCU for this assignment.** Leave the 2.41 console (`FC:01:2C:DA:2B:38`), DualMCU radio (`30:ED:A0:29:A1:C4`) and Teensy (`20731830`) alone. Do not repair their BLE identity, display buffers, ADC calibration or ProfileStore as a migration prerequisite. Do not erase or reset their state. Preserve existing recovery material in place; this addendum does not authorise a new backup/readback session on those devices.

Keep other SMC historical ESP32/Blender lanes, K1-System-R3, vendor BSPs and global governance code outside the write scope. Do not import DualMCU Git history into Titan, switch the live root checkout or overwrite another owner's dirty work. Reuse the existing worktrees after ownership checks; do not create duplicate ones. Scoped commits follow existing workflow; no automatic merge, push or product promotion.

The expected historical Titan UID is `545433931bd25436593630352d068363`. A source branch, serial-port pathname or broker-returned nonempty string does not bind the resident image. Hardware admission requires the actual applicable source/build/schema/runtime/device tuple and existing owner/lease rules.

## 4. What has already been built — resume, do not restart

The retained SMC handback records a real receive-only capture, host mapping/pickup/coalescing modules and tests, a restricted IPC module and a Titan PATCH codec. That report is historical evidence, not a current completion verdict.

The source reread for this addendum additionally found:

- `cli.py` now exposes `live --socket ... --port ... --hz ...` and connects `UnixBroker`, `RateLoop`, live MIDI input and an HTTP tuning view.
- `live_midi.py` performs explicit Mido input opening; offline import and help must remain free of device I/O. An earlier blanket test that no source imports Mido would now test the wrong property: explicit live functionality needs its backend, while help/tests/replay must not open hardware.
- `live_loop.py` contains a real periodic loop with a 25 Hz default at the CLI.
- `titan_control_server.py` now uses an AF_UNIX socket with mode 0600, not the earlier TCP test server.
- `adapter.py` now checks parts of initial state and acknowledgements, tracks an in-flight request, and uses a mode-applicability table.
- `tests/test_review_regressions.py` has newer negative cases. `profiles/smc-mixer-master-v1.json` selects a new motion-first path.

Do not reimplement those items from the older report's NOT_IMPLEMENTED rows. Inspect and test their integration, failure paths and completeness. This reread did **not** execute those tests or establish that the broker is serving a real target, that the complete target handler was built, or that any physical control has changed Titan.

The old K1.Sliders profile tests and reproduced failures remain useful design warnings. Do not copy its broken heap-backed ProfileStore into the host adapter or require its repair before continuing. Reuse compatible, reviewed portable contracts through approved pinning only when materially necessary; this migration does not authorise edits to the excluded repositories.

## 5. Actual controller dialect and first useful parameter

### 5.1 One physical stream, eight faders

Select the exact input `SINCO SMC-Mixer-Master`. Reject ambiguous duplicate matches and synthetic D2/D3 endpoints. Do not open or merge `SINCO SMC-Mixer-Private` and `SMC-Mixer Bluetooth`, and do not silently switch to either when Master disappears.

The archived capture `docs/evidence/SMC-TITAN-CONTROL-001/20260925-232658/` reports 3,257 ordered messages per endpoint with identical byte sequences. Raw `messages.jsonl` SHA-256 is `5b071a9a42408fd83f6f059f899d195131b536da4e8c1014ec000231efc508eb`. Recheck it against the retained file; it was not rehashed during this addendum.

For this captured profile, physical faders 1–8 correspond to pitchwheel channels 0–7 (zero-based). `raw14 = pitch + 8192`; admitted values are 0…16256 in steps of 128, with zero low byte. Normalise by 16256, not 16383. This is 128 observed positions: seven-bit position granularity inside a 14-bit message format, not proof of a 14-bit sensor.

A changed dialect or value grid is a compatibility event, not a reason to clip silently. Preserve raw bytes, endpoint identity and host receive time. Do not translate the old nine-slider CC20–28 map onto this device. Eight faders do not acquire a ninth physical control by aliasing unrelated input.

### 5.2 The current first path is fader 7, not fader 6

The freshly read profile and applicability table now select:

`physical fader 7 → pitchwheel channel 6 → visual_a.vp_waveform_shift_rate → mode 8`

Range in the current candidate: 0…240 in that field's declared units; suggested distinct comparison values: 40 and 200. The referenced software renderer log records 11,562 changed colour bytes for a mode-8 motion positive control. This establishes a bounded software response, not target liveness, optical quality or measured physical speed.

The earlier fader-6/hue-first text is stale for this selected milestone. The same renderer evidence reports no hue change in mode 32 with `auto_colour_shift` off and a change with it on. Do not silently enable that flag, change modes or call the whole-config setter to manufacture a successful demonstration. Verify the selected mode and field against the actual candidate. If the running scene is not compatible, continue offline validation and use an explicitly admitted discrete scene transition, or identify another demonstrably consumed safe scalar without silently changing the surface map.

Hue wrap means endpoints 0 and 1 are not a sufficient colour-difference experiment. No difference under one fixture means NOT_DEMONSTRATED for that fixture, not proof that the field can never have an effect.

### 5.3 Eight roles are an eventual usability objective, not eight accepted controls today

| Fader | Intended role retained from original SMC brief | Current candidate / admission rule |
|---|---|---|
| 1 | Visual light level within an independent safety ceiling | Do not label `vp_waveform_fallback_brightness` as master light; the current table disables this fallback-only proxy |
| 2 | Visual-response sensitivity | Not raw microphone gain; require mode-specific consumption evidence |
| 3 | Mood | Preserve actual `mood` semantics; do not silently rename or equate it to Liveiness |
| 4 | Chroma | Require current mode/palette-path evidence |
| 5 | Saturation | Do not expose as effective where the renderer forces or ignores saturation |
| 6 | Hue position | Enable only for a demonstrated applicable path; no automatic feature toggle |
| 7 | Active effect's motion | First candidate is mode-8 `vp_waveform_shift_rate` |
| 8 | Active effect's persistence/decay | `vp_waveform_idle_fade` 0.90…0.999 remains a candidate; the reviewed fixture did not demonstrate its effect |

The observed applicability file currently permits only mode-8 motion unconditionally among these bindings, and mode-32 hue conditionally. Do not count eight schema-writable scalars as eight artistically effective controls. Show unavailable/unverified reasons. Expand the real per-mode implementations and evidence in the parent scope; do not conceal a scope cut with permanently disabled rows and declare completion.

Encoders and buttons remain unbound until labelled capture proves direction/edges in the selected mode/bank. The observed 1/65 helper does not prove every encoder dialect; mixed 0/127 CCs must not become giant relative deltas. No output-arm, reset, clock, pin, DMA, sample-rate or safety-ceiling action is assigned by inference. Fine encoder adjustment follows labelled admission; it does not turn faders into higher-resolution sensors.

## 6. Onward work, mapped to the existing SMC brief

The continuation labels below are navigation aids, not a second task registry. Bind them to the current `SMC-TITAN-CONTROL-001` assignments and original A/B/G obligations. Fix project/worktree binding through the existing workflow without changing global governance. A historical missing task file is not permission to claim a registered acceptance result.

### C0 — Reconcile scope, ownership and source identity (G0)

Read this addendum and the full original SMC brief, dispatch and hardware gates. Preserve dirty files, capture evidence and pending changes. Record both worktree identities, affected-file hashes, current admission and the old-to-new task dispositions. Do not spend this step repairing the retired surface or conducting another general USB/Bluetooth investigation.

Deliver an identified candidate plan and exact first failing assertion/integration gap, then implement the next owned task. Revalidate old test claims against current changes, especially applicability filtering that may invalidate an older hue replay fixture.

### C1 — Prove partial application and truthful receipts (B1–B4, B6–B8; G3)

Use Titan's bounded PATCH path, not SET_CONFIG/BEGIN/APPEND/COMMIT. In the earlier candidate, opcode 25/PATCH subcommand 8 had an expected revision, request ID, logical channel and up to eight typed entries; derive the current contract from `docs/contracts/titan-live-v1.json`, generated schema and the C++/Python codec rather than freezing that historical hash after further changes.

Validate a whole batch before state mutation. Enforce exact supported schema/runtime/UID/build admission, types, finiteness, ranges, allowed fields, channel, revision and request/session identity. Merely testing for nonempty UID/build strings is not identity validation. Missing or malformed approval fields must not become an implicit permissive default. An environment flag or file named `accepted-candidate.json` is a mechanism, not authority an agent may manufacture.

Apply scalars at a documented safe render boundary and return an application revision plus a usable frame/hop cursor or equally explicit applied-boundary evidence. A successful parse, queue insertion or configuration read is not automatically proof the renderer has consumed the update. Preserve mode, palette, enable/emission permission, unrelated channel controls, animation histories, clocks, audio processing and backend ownership. Verify the actual `fixture_app.cpp` branch and target compilation, not just the standalone codec.

Use negative tests for duplicate/stale IDs, unknown fields, mixed valid/invalid batches, NaN/Inf, wrong channel, oversized requests, reset/configure calls, forced emit changes, state/history reset and missing/mismatched receipts. Response receipt identity must originate in, or be convincingly bound to, the actual target transaction; a broker echo of the request ID alone is not end-to-end proof.

### C2 — Complete single-owner IPC and live lifecycle (A1, A3, A7; B5–B7)

Complete or verify the wiring from the existing Unix server into the persistent Titan broker. The SMC adapter must not open CDC. Keep one bounded target transaction owner for metrics, snapshots and PATCH; a new lock used only by PATCH is insufficient if telemetry bypasses it.

Prove writer ownership across the **real client/server pair**. `UnixBroker.request()` currently opens a socket for each request while server writer state is connection-associated. Define and test the intended lease/session lifetime; do not assume a per-connection flag guarantees one application writer. Repeated UI readers cannot acquire the writer role accidentally. A UI restart must not close Titan CDC or modify output state.

Keep the Unix socket in a user-owned directory with restricted permissions; the observed socket is 0600 but directory ownership, stale-path handling, competing server start and cleanup need verification. Refuse malformed/non-object/oversized JSON and unbounded requests/replies. Do not unlink another active owner's socket to start a second broker. Do not loosen the whole opcode-25 allowlist: inspect subcommand and payload.

Exercise the actual rate loop, input port and exception paths with hardware doubles first. Validate rate bounds; keep the initial aggregate batch target near 25 Hz unless a measured change is admitted. Bound input-drain work and target I/O. A timeout must not silently kill the worker while leaving the UI looking live. On genuine device removal or session loss, block writes, clear obsolete queued work, release only owned resources, and reconnect Master explicitly. An untouched MIDI controller is normally silent; silence alone is not removal.

Report observation health, not optimistic ready flags. Use coherent owner-published UI snapshots and producer timestamps. Do not infer current position before a measured event, refresh stale data by rereading it, or present a prior acknowledged value as a current requested value.

### C3 — Close takeover, conflicts and unknown outcomes (A3–A5; B6; G1–G3)

On first attach/reconnect/recall/bank or target change, obtain compatible authoritative state before admitting writes; do not send all fader positions. Compare pickup in one consistent domain, with tolerance scaled to the actual mapping and controller grid. One unscaled tolerance for 0…1, 0…240 and 0.90…0.999 is not a justified universal policy.

Distinguish ordinary temporary backpressure from invalidated intent. Preserve the final **still-valid** value through ordinary coalescing/backpressure. On external edit, revision conflict, mode/applicability change, profile change or new session, invalidate obsolete pending intent and rearm against current target state. Never automatically rebase an old queued value onto a new revision and overwrite a newer edit. A delayed accepted receipt from an old session cannot confirm a new one.

An acknowledgement timeout is UNKNOWN until reconciled, not automatically rejected or retried. Retain enough request identity to resolve it without applying twice. Test response loss after application, duplicate request, mismatched/missing/nonfinite applied values, emit mismatch, stale config, worker exception and restart. No blind repeated PATCH under fresh request IDs; no spontaneous replay when an admission HOLD is lifted.

Mode/applicability and enabled state must refresh with authoritative state changes, not only initial attach. Metadata such as raw, physical, requested, acknowledged, applied revision, session and pending/unknown/rejected/disabled status stay distinct. Test real adapters and broker, not only an InProcessBroker that always makes application immediate.

### C4 — Build and qualify the one-fader path (B1, B3; G3–G4)

Perform a candidate-only ARM build of the complete applicable Titan live-runtime image using the existing build wrapper and local platform instructions. Resolve effective paths for the worktree/BSP/import manifest; do not write or mutate a sibling BSP to hide a layout issue. Keep scalar audio/visual/output behaviour and the selected backend; this control task does not require importing the separate cloud wide-output branch.

Record build inputs, full image/schema hashes, compiler configuration, map/stack/resource evidence and post-link results. A build does not authorise programming. Preserve default state and a known recovery tuple. No upload-time rebuild may substitute unqualified bytes.

When an identified candidate and the current hardware admission permit it, run exactly one persistent CDC owner and a bounded tuning lease. Start with fader 7 / mode-8 motion **only when that scene is applicable and admitted**. Read baseline and final state, verify intended-channel application, applied boundary and unchanged emission/history/AP behaviour. Use distinguishable parameter values; do not claim a change merely because the recorded gesture ends at zero.

Target ACK/readback and optical effect remain separate. If emission is held off, do not turn it on for the demonstration: report the control-state gate and the blocked optical gate separately. Apply the existing safe output cap and preserve current mapping. Logical visual A/B are not automatically the physical DIN lanes: the existing pair may be two halves of one selected logical channel.

### C5 — Expand usefulness and retain reproducible looks (A2–A6; B4; G5)

After the first path works, admit more fields per mode with real renderer-consumption evidence, preserve the eight-role intent, and support channel/bank selection with pickup. Do not claim the eighth role works because the codec accepts its field. Discrete mode/palette changes need their own verified transition policy; normal fader PATCH never calls a resetting whole-config path.

Add host-side save/recall/compare of **acknowledged** looks using an explicit versioned schema, build/schema compatibility, channel/mode, units, mapping/profile identity and permitted parameter values. Save is not an actuator write. Do not serialize pending/unconfirmed values as accepted state. Validate a recalled file before any target effect; display incompatible/partial recall rather than silently clipping or changing a scene. Persist atomically through the admitted host mechanism; corrupt/interrupted saves retain a recoverable prior record.

Recall is an explicit user action and creates a new valid application transaction; it never revives an obsolete revision/request identity or replays parked faders. Keep target emission, safety limits, pins, clock/capture config and programming operations outside saved-look application. No flash/NVS write per gesture. Do not make console NVS or the old heap ProfileStore a dependency of host look storage.

Map encoders/buttons only after labelled receive-only evidence. Use the real input gesture semantics and genuine fine steps; preserve event ordering at bounds/bank changes, and treat NoteOn velocity zero as release in the admitted dialect. Do not block the first control milestone on every button or expand into mixer LED feedback without separate scope.

Run the original proposed ten-minute tuning stability session when admitted, including single/simultaneous faders, pending final values, external edits, UI-only restart, controller reconnect, bank/recall transitions and fault conditions. Freeze acceptance bounds before the run. Preserve stronger existing Titan run requirements. Compare audio/LED completion, loss/fault/emit-error and deadline counters before/during/after. The original sub-50-ms host-receipt-to-target-application goal is distinct from physical motion-to-photon latency and the broader acoustic-to-light requirement.

## 7. Disposition of the previous P1-CLOSE and P2–P5 work

| Previous obligation | Migration disposition |
|---|---|
| 2.41 display memory / tile rotation / ADC calibration / Bluedroid start / GATT K1C24 identity | Deferred legacy-surface work; not a prerequisite and no edits in this assignment |
| K1.Sliders ProfileStore revision/OOM/invalid-slot/12,080-byte decoder findings | Preserve as unresolved old-source defects; do not run the vulnerable device path. Carry atomicity/resource lessons to host look handling only if relevant code is reused |
| One writer, no reconnect dump, pickup, coherent feedback | Retained mandatory SMC/IPC/Titan behaviours, tested at the real new boundaries |
| Console → S3 RelayV2 → Teensy activation chain | Separate untouched DualMCU work, not required for the USB-Mac-Titan path |
| Nine physical faders and 2.41 screenshots | Surface-specific evidence no longer applies to the SMC milestone; successor is eight captured SMC faders and host applied-value display. Do not mark old tests PASS |
| Generic typed control, provenance, revisions, truthful acceptance | Retained through Titan's compatible PATCH service; no forced port of the unrelated console protocol |
| Console/Teensy NVS and old custom profile editor | Not on the initial SMC path; host acknowledged-look save/recall is the successor milestone, not proof of their target persistence |
| Optical/current/blackout/full-topology/latency criteria | Retained for actual applicable Titan/product scope; not satisfied by a simulated ACK or software dump |
| Bloom mode 3 / native-wide import as first joined configuration | Not a prerequisite to SMC tuning. Use the current proven applicable scalar first; retain independent wide-renderer and broader artistic programme work |
| Full 56-task programme and outstanding Titan work | Preserve unchanged; record explicit scope/dependency dispositions and separate adoption states instead of shrinking the plan |

Update these dispositions in the existing authorised SMC/Titan records. Do not edit K1.Sliders or DualMCU simply to write PARKED into them when those repositories are outside the write scope.

## 8. Hardware boundary and STOP/GO

The SMC gate file read for this addendum holds Titan CDC attach and live PATCH pending current Captain admission and an identified candidate; programming/reset/emission probes are not authorised there. This request to update instructions does not itself lift that HOLD. Recheck whether a later applicable admission already exists; do not ask again for permission already granted. Equally, do not manufacture approval by creating a JSON file or setting an environment variable.

Continue offline coding, broker simulation, deterministic replay, schema tests, evidence binding and candidate-only target builds while a hardware gate is held. Do not stop all development at a live-access gate.

When programming is specifically admitted, use the current local E2 Lite/SWD router (`rfp-cli`, correct probe/target, probe target power off); not GDB, retired serial-ROM procedures or requests for boot-button choreography. Do not reset merely to inspect state, close/reopen CDC per gesture, delete bonds or change wiring as a tuning recovery strategy. One tuning lease covers normal bounded gestures; it is not a prompt for approval per fader event.

Stop affected writes on wrong identity/schema/runtime, another owner, invalid/expired/revoked admission, uncertain prior application, stale mapping/session, unsafe or unknown power bound for intensity operations, or audio/LED regression. Hold the last accepted scene under existing engine safety authority. Do not arbitrarily enable or disable emission on control loss. Escalate only the actual missing prerequisite; continue independent ready work.

## 9. Existing commands and evidence requirements

Resolve the supported project interpreter and dependencies; do not install globally. These are existing offline entry points observed in the current worktree, not commands executed for this addendum:

```sh
SMC_WT=/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001
TITAN_WT=/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-live-control-001

python3 "$SMC_WT/host/titan_control/run_tests.py"
(cd "$SMC_WT/host/titan_control" && python3 -m smc_titan_control help-offline)
(cd "$SMC_WT/host/titan_control" && python3 -m smc_titan_control replay --first-path-only --json)
python3 "$TITAN_WT/scripts/test_live_protocol.py"
python3 "$TITAN_WT/tools/serial-studio/tests/test_titan_control_server.py"
```

Reconcile changed applicability with old fixtures rather than disabling meaningful checks to recover a green count. Add targeted regressions beside existing tests. Each repaired defect needs a preserved counterexample or bounded mutation at the real relevant boundary; never deploy a deliberately broken image to prove failure detection.

Do not infer an executable broker launch command from a module class. Resolve the real persistent-owner entry point and permitted subcommands from current source/help. `live` exists in the SMC CLI but running it is live device/IPC work, not a harmless replay. Verify the full resource/lifecycle and admission gates before using it against hardware.

Handback must name: actual worktrees and ownership; source/candidate/firmware/schema/profile/capture identities; changed files and commits; commands and exit status; original task and gate mapping; actual supported controls and inactive reasons; request/readback/application-cursor evidence; unknown outcomes and failures; elapsed-time/resource measurements; hardware leave-state; and the next ready task.

Report source review, host test, target build, programmed identity, live parameter application, electrical/optical evidence and adoption separately. Never credit one as the next. Do not report this addendum's source observations as a freshly passing test suite or a running-image claim.

**The new milestone is SMC gesture → truthful, non-resetting Titan application → reproducible acknowledged look. Finishing the retired 2.41 surface is no longer the route to that milestone.**
