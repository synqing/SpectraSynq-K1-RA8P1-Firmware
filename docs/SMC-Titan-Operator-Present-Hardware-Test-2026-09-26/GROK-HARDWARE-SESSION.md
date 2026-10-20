# SMC–Titan: operator-present hardware test

**Issued:** 26 September 2026, Australia/Perth  
**Task:** SMC-TITAN-CONTROL-001 / first physical control session  
**Operator:** Elroy (Captain), with the commercial SMC-Mixer present  
**Executor:** Grok CLI  
**Outcome:** one real fader controls one real Titan parameter, followed by an explicitly authorised visual assessment—not another readiness-only handback.

## 1. Mission and limits

Bring the Captain into the testing loop now. Start with a brief receive-only check of the physical mixer. Then execute the identified-board test when the existing, specific entry conditions are satisfied. Finally, conduct the separately authorised light test with the Captain observing the plate.

The path is:

`SMC-Mixer USB Master → Mac adapter → restricted local IPC → sole Titan CDC broker → PATCH → applicable renderer`

The first binding is **physical fader 7 → zero-based pitchwheel channel 6 → mode 8 → `visual_a.vp_waveform_shift_rate`**. Range: 0–240 in the field's documented units. Do not expand this session to eight controls, other effects, presets, encoder mapping, a UI redesign, or a governance rewrite.

The commercial mixer replaces the retiring 2.41/S3/nine-slider surface for this task. Do not edit, flash, or require K1.Sliders, DualMCU, Teensy, the old ESP32 controller experiments, or K1-System-R3.

**Physical input, target acceptance, applicable-render consumption, output completion and observed light are distinct results.** Test labels must preserve that distinction. The Captain's assessment of the visible effect is required in the light phase; a dump is not a substitute.

This is an execution addendum to the existing programme. The earlier C4 correction and its evidence are retained unchanged under `reference/`. Its named defects are not waived. This document adds the bench sequence and operator cues; it is not another competing architecture plan.

## 2. Source basis and authority

The source-derived facts below come from the existing SMC assignment, capture-backed profile and C4 correction, not a fresh device inspection for this document:

- Production input: `SINCO SMC-Mixer-Master`, USB, one selected source. Do not merge Private or Bluetooth, silently fail over, or use D2/D3 synthetic sources.
- Observed faders: pitchwheel channels 0–7; `raw14 = signed_pitch + 8192`; range 0–16256 in steps of 128. For this profile, `coarse = raw14 / 128`, position `coarse / 127`, motion request `240 * coarse / 127`. This is 128 observed positions, not a claim of 14-bit physical precision.
- Expected Titan UID: `545433931bd25436593630352d068363`. USB enumeration alone does not establish it.
- Relevant LED pair: P601/GPT6 and P604/GPT8, subject to the actual selected candidate and existing bench binding. Do not move wires to accommodate an obsolete image.
- Titan programming uses the existing E2 Lite/SWD/`rfp-cli` router, probe power off. No GDB, serial-ROM recovery or request for the Captain to press programming/reset buttons.
- The saved `980eb3a9…` recovery archive boots an emitting mode-32 show. It is not automatic recovery in this session.

**The durations, sequence and prompts below are proposed bench procedure—not reported measurements or estimates of implementation time.** No hardware operation has been performed by preparing this document.

### Permissions

On handoff, start the receive-only SMC phase under the user's instruction. It may enumerate and open the exact mixer input; it must not open a MIDI output, a Titan connection, a programming probe, or a target-control lease.

Titan installation/attachment/control and emission remain subject to actual existing authority. Reuse a valid, applicable session authorisation when one exists; do not ask again for every gesture. Do not interpret the Captain being present, this document, a fake checkpoint, or a passing build as approval to install a known-defective image.

Before a target operation, resolve one exact candidate and one explicit bounded permission. Before emission, resolve the separate light scope. A generated file may record genuine approval; it cannot create it.

## 3. Workspaces and ownership

Continue in the existing worktrees, after checking their actual owner and working bytes:

- `/Users/spectrasynq/Workspace_Management/Software/SMC-Mixer/.worktrees/titan-control-001`
- `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-live-control-001`

Use the existing input, adapter, broker, collector, programming router and scorer. Do not invent command names in the handback. Discover the actual current entrypoints and record the commands used. If a receive-only entrypoint is missing, add only the minimal wrapper around the existing input/decoder needed for H1. That is not permission to build another controller framework.

Use one new evidence directory inside the existing SMC-TITAN-CONTROL-001 evidence area. Retain raw inputs, requests, replies, decoded records, software/image identities and the operator's observations there. Do not overwrite previous receipts or build directories.

## H1 — Real mixer input, now; no Titan access

**Purpose:** establish the current physical control and its mapping, without waiting for firmware installation. Skip a duplicate sweep if an equivalent same-session physical check has already been recorded.

### Agent actions

1. Enumerate inputs. Select exactly `SINCO SMC-Mixer-Master`; refuse missing or ambiguous selection. If absent, ask only for the mixer USB connection, then retry the enumeration once after the operator connects it. Do not start Bluetooth troubleshooting or factory resets.
2. Open receive-only input using the existing tested environment. No MIDI output, Titan serial, fake target presented as real, or queued future target writes.
3. Display and record the selected input, physical fader label, MIDI channel, raw bytes/raw14, coarse position, physical percentage and locally proposed motion value. The target column must say **NOT CONNECTED / NOT APPLIED**, not mirror the requested value as an acknowledgement.
4. Only after recording is active, issue the cue below.

### Operator cue and sequence

> **MIXER INPUT RECORDING — NO TITAN WRITES.** Keep every control still for five seconds. Then move physical fader 7 slowly to the bottom, slowly to the top, then approximately halfway and stop. Leave the other faders alone.

Record five seconds of rest after the sweep. Then:

> Move fader 6 a small amount and stop. Do not move fader 7.

The neighbouring control is an identification/isolation check only; it remains unbound to the target for this session. Do not repeat a full all-control capture campaign.

### Result

Confirm that fader 7 is the expected channel 6, its observed values follow the admitted grid, and moving fader 6 is not mislabelled as fader 7. Record the endpoints actually reached; do not invent a full-range pass if the operator did not reach them.

Print a plain result, for example:

> **PHYSICAL INPUT VERIFIED:** USB Master; fader 7/channel 6; actual range recorded; motion mapping displayed. Titan has not been written.

Stop the temporary monitor or hand ownership to the production adapter without creating duplicate listeners. **Discard all H1 pending gestures before any future target session. Never automatically replay this capture into Titan.**

If the board candidate is still blocked, say so immediately after this short useful check:

> **MIXER VERIFIED; TITAN STAGE BLOCKED:** [one concrete defect or missing permission]. No more movement is required from you while this is corrected.

Do not keep the Captain moving faders during compilation or unrelated repair work.

## H2 — One target-entry decision, not another review cycle

**Purpose:** ensure the hardware test will exercise one working combination, not a known incompatible historical image.

Before requesting installation, verify the existing evidence for the selected adapter/broker/codec/schema/firmware/scorer combination once. Reuse passing evidence whose applicable source identity is unchanged. Repair only an actual failed entry condition.

### Required entry facts

- Actual host and broker source identities are frozen; codec and target agree on the wire format and boot/session identity. Python-produced bytes have been checked against the candidate's real C++ decoder.
- The selected target has no unresolved known restart-binding or applicable-render defect relevant to this test. It rejects obsolete-boot PATCHes before mutation.
- The scorer's demonstrated missing-evidence, contradictory-state, value and revision false passes are repaired. It reads actual values from captured replies/configuration, not a duplicate `--applied-value` supplied by the operator.
- The exact target image, schema, defaults, mode 8 and emit-off startup are bound. Applicable pin/resource configuration and live-audio path are recorded. A source commit alone is insufficient when required files remain dirty.
- The real collector can record request identity, expected revision, acceptance, applicable-render request/revision/frame, emission observations and audio continuity. A simulated “next hop” is not the actual consuming frame.
- The programming path and stop-on-failure policy are ready. Do not perform a rehearsal that secretly opens the board.

**Historical candidate warning:** R3 `addc00a2…` / schema `e3dca88c…` has documented restart/preview-consumption limitations. R4 `51b73cdb…` / schema `af180050…` has documented boot-provider/completion-attribution defects. Do not install either just because its hashes match, and do not auto-select the newest image. See the retained C4 correction for the exact bounded repairs. Target corrections justify a new identified build; a newer directory alone does not.

Output-completion attribution is not a requirement to open a receive-only mixer input. Keep it distinct from emit-off application/consumption evidence. Before crediting a later emitted generation, its attribution must be correct; do not use an attempt counter as completion proof.

### Single installation/emit-off request

When the entry facts are satisfied, Grok must fill the following with actual verified values—not ask the Captain to choose a hash:

> **READY FOR BOARD TEST.** Proposed build: [full build ID]. HEX: [full SHA-256 and path]. Schema: [full digest]. Target: `545433931bd25436593630352d068363`. Mode 8; startup emission off; P601/P604 configuration verified for this candidate. Request one installation through the existing E2 Lite/SWD router, its necessary programmer-managed startup, one persistent broker attachment, read-only identification/configuration/snapshots, and bounded motion PATCHes followed by the physical fader-7 test. No output arming. On failure, stop; no automatic restore of the emitting `980eb3a9…` archive. Reply **GO INSTALL + EMIT-OFF** to authorise this named scope.

If that exact scope is already genuinely authorised, record the existing authority and proceed without asking again. Otherwise, no programming or real Titan connection precedes the decision. This template itself is not approval.

## H3 — Install once and establish the real emit-off baseline

**Operator:** no programming buttons and no speculative cable/wiring changes.

### Agent actions after applicable approval

1. Rehash the named HEX immediately before use. Program that exact artifact via the loaded `rfp-cli` / E2 Lite router, probe power off. Use the validated target range and settings from that router—not a copied broad range from a different campaign. Do not rebuild inside the upload action, repeatedly reset, or silently substitute an image.
2. Preserve the programming/verification result. A failure stops the installation path; do not try another image or automatically restore `980eb3a9…`.
3. Establish sole CDC ownership using the existing procedure. Reuse the authorised persistent broker where applicable. Do not kill another owner's process, open serial per gesture, or close/reopen CDC to cure an error.
4. Read actual UID, build, schema, runtime kind, boot/session identity, mode and configuration. Require the named tuple, mode 8 and explicit emission-off observations. Identity bootstrap/INFO and schema reads are read-only prerequisites; they do not broaden write scope beyond the named motion PATCH.
5. Record a short baseline, proposed 10 seconds: actual motion value **V0**, revision, boot/session, audio validity/progress, render state and applicable faults. Preserve palettes, other parameters, channel B, capture configuration and output authority.

If the effect is in preview/dwell or there is no usable live audio, do not manufacture an applicable-render PASS. Cue the Captain to supply ordinary programme audio through the already-used capture path:

> The target is identified and emission is off. Please start a familiar steady section of music at your normal listening level; keep the faders still.

Do not raise system volume, alter gain, change capture settings or replay a media corpus without approval. Use the audio that makes this effect's actual branch applicable.

**Emission-off is not a claim that the plate is visibly black.** Record its actual appearance. Do not send a blackout fixture or change strip power just to make the appearance match a software label; that would be a separate output operation.

### Baseline exit

The collector is receiving candidate-bound records; mode and emission state match; the intended effect can render when appropriate; no new relevant error is hidden. Missing or contradictory state is a stop or an explicit unavailable result, not zero.

Do not demand advancing emitted-frame/pair-completion counters while emission is intentionally off. Audio and applicable renderer progress are the relevant active paths here.

## H4 — Two direct protocol probes, to separate device behaviour from the fader

**Operator:** leave the controls alone; keep the audio context steady. This stage is not the physical-input proof.

1. Use the real controller transaction path to request motion **40.0**, with the current boot/session, captured revision and a new request identity. One request is outstanding at a time.
2. Collect authoritative acceptance/value readback and wait for the same request/revision to be consumed by the applicable mode-8 renderer. Record the actual frame/hop context and any supersession. Emission must remain off; unrelated controls must remain unchanged.
3. Repeat for **200.0**, only after the preceding result is resolved.
4. Restore **V0** once through the same scoped scalar PATCH when the session is healthy and both probes succeeded. Verify the restore. Do not call the resetting whole-configuration path or restore firmware to recover a scalar value.

Use the runner's fixed functional-response deadline. Where it has none, implement a two-second functional observation deadline for this first paced probe. This is an availability timeout, **not** a two-second latency target or a sub-50-ms performance pass. Timeout means unknown until the established reconciliation path resolves it; do not send another untracked write or blindly retry.

An accepted-but-not-consumed request remains awaiting applicable render or superseded as appropriate. It does not pass consumption merely because later audio hops advance.

## H5 — Captain moves the real fader against the real board

**Purpose:** prove the full path the Captain asked for. Synthetic PATCHes alone cannot close this stage.

The production adapter now owns the exact Master input. Clear diagnostic queues, read the actual target value, and arm pickup without sending parked positions. Show separate **physical**, **requested**, **target-accepted**, **renderer-consumed** and **pending/blocked** states. Never label a local echo “applied”.

### Cue A — stationary attach

> **LIVE BOARD CAPTURE RUNNING — EMISSION OFF.** Titan [short verified build], mode 8. Motion is [actual value]; pickup is approximately [inverse-mapped percentage]. Leave fader 7 still for five seconds.

Require no unsolicited parameter write from attachment or parked positions. Preserve the baseline value. Silence from an untouched mixer is not a disconnect.

### Cue B — pickup

> Move only fader 7 slowly through the displayed pickup point. Stop briefly when the display says **PICKUP ACQUIRED**.

Do not require an exact physical 120, 40 or 200 value. For the observed grid, crossing between adjacent values can acquire pickup even when a target value lies between steps.

### Cue C — controlled changes and final value

> Move fader 7 to roughly one-quarter travel and pause, then three-quarters and pause, then around halfway and stop. Keep the other controls still.

Use approximately three seconds at each pause and five seconds at the final rest. These are operator pacing cues. Score the values actually received, not the suggested percentages. Motion request is `240 * coarse / 127`; nearby 40/200 steps are approximately 39.6850 and 200.3150.

For each paced point, correlate raw MIDI → mapped float32 request → acceptance → applicable consumption. Check that motion follows, only the scoped field changes, audio does not restart, emission remains off, and the final settled value is retained without an endless stream of stationary writes. The normal coalescer may skip intermediate motion messages; it must preserve the final eligible value.

### Cue D — neighbouring control isolation

> Leave fader 7 still. Move fader 6 a little and stop.

Fader 6 is deliberately unbound in this session. Record its physical input, but require no motion PATCH or other target write. Do not silently enable another binding.

### Cue E — one mixer reconnect

> Leave fader 7 where it is. Disconnect only the SMC-Mixer USB cable, then reconnect it. Do not unplug Titan or the LED assembly. Leave the controls still after reconnecting.

The broker stays the same CDC owner; the engine's accepted parameter remains in place. Discard obsolete/unsubmitted gestures, reacquire current state, and rearm pickup. There must be no send-all or automatic replay. Then cue one fresh slow crossing and one change to show control resumes.

Do not add a target-reset campaign to this operator sequence. The known boot boundary must already have passed its scoped tests; arbitrary hardware resets require their own admitted scope and are not needed to prove this first physical movement.

### H5 exit

Report **PHYSICAL FADER → TARGET ACCEPTANCE → APPLICABLE RENDER VERIFIED, EMISSION OFF** only when the real source and correlated target evidence support every part. Otherwise identify the failed boundary and do not ask for repeated random sweeps.

## H6 — Visible light: the Captain is the visual evaluator

This phase follows H5 and requires actual output permission. Do not leave it out of the session plan or substitute a software dump for it.

### One bounded light approval

Use the recorded power/brightness limit already approved for this exact assembly and candidate, if one exists. A provisional host ceiling of 1.0 or an old firmware brightness constant is not, by itself, that approval. Where the bound is missing, ask for that concrete decision; do not request a full unrelated electrical qualification campaign simply to record a limited operator observation.

The agent must state:

> **READY FOR VISIBLE TEST.** Same identified build and mode 8. Proposed output limit: [actual approved numerical limit and source]. Proposed visual window: 60 seconds. Only fader-7 motion changes. End state: [output off, or approved scene left running]. [State whether starting output involves one deliberate scene reset.] Reply **GO LIGHT** to authorise this scope.

Use the existing reviewed output-control mechanism. Output arming is never a fader assignment and never hidden inside a normal motion PATCH. If the only established scene-start operation resets/configures the effect, disclose that one-time cut in the approval and take a fresh baseline after it. This is not permission to invoke whole-configuration writes per gesture, and the setup reset cannot count as non-resetting operation. Do not build a new arming framework merely to avoid an explicitly accepted initial scene start.

Do not silently reflash an emit-on image. If no applicable output-control operation or approved limit exists, state that exact light-stage blocker; H5 remains a valid emit-off result.

### Operator test

Keep brightness, palette, audio source and other parameters fixed. With capture running:

> Watch the actual plate. Move fader 7 slowly lower and pause, then higher and pause. Tell me whether the motion becomes slower/faster, stays unchanged, jumps, resets, sparkles or drops out. Then make several small movements and say whether the response feels useful or sluggish.

Use the agreed window and record the Captain's actual words. A visual response is expected from an applicable motion control, but do not manufacture a success report if the operator sees no change.

The agent simultaneously checks target request/value correlation, actual effect-path continuity and applicable fault indicators. Credit output completion only from the correct completed generation's metadata. Completion is not independent optical evidence. Qualitative “feels prompt” is not a measured millisecond latency; do not subtract unsynchronised host/MCU timestamps or infer physical latency from an ACK.

At the end, apply the agreed leave-state via its verified operation. Do not silently extinguish a show the Captain agreed to leave running, silently leave output armed when off was agreed, or restore the historical firmware archive. If state is uncertain, stop further speculative writes and report the uncertainty immediately.

## 4. STOP rules and behaviour when a stage fails

Stop further target-control submissions on wrong or changed identity/schema/boot, conflicting ownership, forbidden whole-config traffic, unknown prior application, unintended field/mode/emission change, or relevant audio/renderer/output regression. Preserve logs and the actual leave-state.

Do not automatically reset, reflash, clear pairing, change wiring, restore the emitting archive or send black fixtures. Existing independently implemented safety behaviour remains authoritative. During an authorised light test, use the explicitly approved, verified stop/off operation when it is available and appropriate; do not invent one or repeatedly retry uncertain control.

Describe the failure in one actionable sentence: **boundary / expected result / observed result / next discriminator**. Keep the Captain involved only when another physical observation can discriminate that fault. Do not ask them to keep sweeping a fader while software is being repaired.

## 5. Definition of done and handback

This is a bounded first-control milestone, not full K1 product release. Hand back one compact result table:

| Evidence | Result to report |
|---|---|
| Real mixer | exact source, fader/channel, captured range and mapping; neighbour isolation |
| Installed target | exact verified image/UID/schema, or explicitly NOT INSTALLED |
| Real control | requested versus accepted value and request/revision |
| Real applicable render | consuming request/revision/frame and continuity, not synthetic next-hop |
| Reconnect | parked value preserved, no replay, pickup reacquired |
| Emission | actual before/during/after state and agreed leave-state |
| Visual | Captain's observation; NOT RUN if no light admission |

Add the exact commands, minimal raw-evidence paths/hashes, relevant faults/deltas, any unmet criterion and device/owner leave-state. Record direct-protocol probes separately from physical fader input. Do not call a simulated or read-only result target control.

**Closing sentence must say whether the Captain can now manipulate the actual visual instrument.** If not, name the single remaining physical or implementation boundary—not merely a passing build, completed document or request for another general review.
