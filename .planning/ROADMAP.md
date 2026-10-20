THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Roadmap: Titan RA8P1 takeover (corrected 2026-09-20)

## Overview

Reconcile the live Titan checkpoint, bind existing host-test evidence, then implement the live K1 runtime (clock, audio ownership, AP owner, controlled VP, controls, MIR) through a named ARM-built candidate and Rearm development campaign. Q1–Q5 timing and P601/DIN diagnosis are separate tracks. Combined-image qualification, physical lanes, and K1-L follow transmitter admission. A hashed handback with BLOCKED rows is a complete handoff, not a ship stamp. Hardware absence blocks only the affected measurement. The 20 September takeover brief is historical; do not amend it.

## Phases

- [ ] **Phase 1: WP0 checkpoint and WP7 programming SOP** — Truthful source/device/evidence checkpoint and a prepared Rearm waiter contract
- [ ] **Phase 2: Remaining host negatives** — Only tests still red or missing; do not restart green F-01/F-04/F-06 campaigns
- [ ] **Phase 10: Live K1 runtime** — `Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md` Phases 0–6; emit-disabled candidate allowed
- [ ] **Phase 11: Live development campaign** — Rearm + real-music checks → TITAN_LIVE_K1_DEV_READY with stated output limits
- [ ] **Phase 4: WP3 uncapped timing** — Separate Q1–Q5 track; not a runtime gate
- [ ] **Phase 5: WP4 LED first divergence** — Separate P601/DIN track; missing analyser → BLOCKED, continue runtime
- [ ] **Phase 6: WP5 combined live admission** — After transmitter admission only
- [ ] **Phase 7: WP6 product output** — Host slice anytime; physical after transmitter
- [ ] **Phase 8: Latency, comparison, ruling** — After combined image for K1-L
- [ ] **Phase 9: Acceptance, docs, handoff** — Matrix rows; BLOCKED handoff ≠ ship

## Phase Details

### Phase 1: WP0 checkpoint and WP7 programming SOP
**Goal**: Establish a consistent source/build/device/evidence checkpoint and a prepared programming protocol without disturbing unrelated dirty work or flashing unless identity work requires an already-authorised observe path.
**Depends on**: Nothing (first phase)
**Requirements**: [WP0-01, WP0-02, WP0-03, WP0-04, WP0-05, WP0-06, WP0-07, WP7-01, WP7-02, WP7-03, WP7-04, WP7-05]
**Success Criteria** (what must be TRUE):
  1. Checkout ownership, SHAs, and uncommitted work are recorded; nothing unrelated was reset.
  2. Device UID/build/contract/cache/M33/U55 are bound or explicitly unavailable, with CDC ownership known.
  3. STATUS and `external-receipts.json` current fields match verified receipts; unknown stays unknown.
  4. A fresh programme directory, exact `programme_scalar.py` waiter command, recovery image hashes, and bound runner command exist on disk before any Rearm is announced.
**Plans**: 2 plans

Plans:
- [ ] 01-01: Reconcile live checkpoint (WP0-01–WP0-07)
- [ ] 01-02: Prepare WP7 programming SOP artefacts (no flash)

### Phase 2: WP1 capture gate
**Goal**: Repair scoring so unsupported provenance cannot become physical or current-build acceptance, and a new acquisition can be scored without impersonating lost frame 2030.
**Depends on**: Phase 1
**Requirements**: [WP1-01, WP1-02, WP1-03, WP1-04, WP1-05, WP1-06, WP1-07, WP1-08]
**Success Criteria** (what must be TRUE):
  1. The F-01 synthetic-physical challenge is rejected as physical qualification.
  2. Capture provenance and waveform conformance are distinct fields; a valid bad waveform remains retained evidence.
  3. Parser unit tests run without a local-Mac witness; historical integration tests are optional and explicit.
  4. No wire PASS is granted.
**Plans**: 2 plans

Plans:
- [ ] 02-01: Split scorer outcomes and add executable negatives
- [ ] 02-02: Capture manifest, dual-probe decode, protocol for a new acquisition

### Phase 3: WP2 host hygiene and observation
**Goal**: Bind observation to an accepted checkpoint, correct units, stop runaway logs, repair F-06, and align documentation with machine-readable authority.
**Depends on**: Phase 1
**Requirements**: [WP2-01, WP2-02, WP2-03, WP2-04, WP2-05, WP2-06, WP2-07, WP2-08, WP2-09, WP2-10, OBS-01, OBS-02, OBS-03, OBS-04, OBS-05, OBS-06]
**Success Criteria** (what must be TRUE):
  1. Unbound build strings on the expected UID do not qualify.
  2. Real-audio runner restores settings, rejects truncated frames, and cannot be called an acceptance gate.
  3. Current-image claims in receipts/docs match STATUS or are marked superseded.
  4. SS-03 replay/reconnect remain prepared, not reduced to four generic dashboard values.
**Plans**: 3 plans

Plans:
- [ ] 03-01: Broker expected-build binding, staleness, CDC ownership, units
- [ ] 03-02: F-06 real-audio observation runner and palette CLI alignment
- [ ] 03-03: F-05 metadata/docs and observability field contract

### Phase 4: WP3 uncapped timing
**Goal**: Answer Q1–Q5 (or name a precise blocker for each) with empty-TCM and DTCM configurations clearly separated.
**Depends on**: Phase 1
**Requirements**: [WP3-01, WP3-02, WP3-03, WP3-04, WP3-05, WP3-06, WP3-07, WP3-08, WP3-09, WP3-10]
**Success Criteria** (what must be TRUE):
  1. A DTCM result is never labelled empty-TCM.
  2. Q1–Q5 are answered from 6,000 raw rows or each has an evidence blocker.
  3. Hop-136 5 ms delay negative is preserved as a negative.
  4. No new product/combined PASS is inherited.
**Plans**: 2 plans

Plans:
- [ ] 04-01: Named empty-TCM build choice, overlay, receipt schema
- [ ] 04-02: Collect/decode 6,000 rows, Q1–Q5, mutation negative (Rearm checkpoint if image must change)

### Phase 5: WP4 LED first divergence
**Goal**: Capture the LED failure with both probes armed, identify the first measured divergence, implement its bounded repair, and repeat the original controls.
**Depends on**: Phase 1, Phase 2
**Requirements**: [WP4-00, WP4-01, WP4-02, WP4-03, WP4-04, WP4-05, WP4-06, WP4-07, WP4-08, WP4-09, WP4-10, WP4-11]
**Success Criteria** (what must be TRUE):
  1. First physical divergence is identified from same-frame P601 and DIN (or BLOCKED with a ready-to-run acquisition task).
  2. Repair (if any) keeps the nonzero DMCRA guard and does not restore `DMCRA <= 2`.
  3. Zero-output, warm low-load, warm USB-stress controls complete at `--phase-s 20`.
  4. Optical claim remains a separate row.
**Plans**: 3 plans

Plans:
- [ ] 05-01: Select diagnostic candidate, verify wiring, arm capture (WP7 Rearm checkpoint)
- [ ] 05-02: Score first divergence and bounded repair plus defect-specific negative
- [ ] 05-03: Repeat colour-integrity controls and keep optical separate

### Phase 10: Live K1 runtime
**Goal**: Implement the live-runtime plan through a host-verified ARM-built named candidate. Transmitter admission is not a prerequisite. First candidate may default physical emit off.
**Depends on**: Phase 1 checkpoint; Phase 2 remaining host negatives
**Requirements**: live-plan Phases 0–6 (clock, hop descriptor, atomic ASRC, AP owner, VP/controls, MIR, build isolation)
**Success Criteria** (what must be TRUE):
  1. Common protected monotonic clock and `k1_audio_hop_t` host tests pass.
  2. Capture/ASRC ownership is atomic; pending-hop mailbox is gone from the live path.
  3. One AP owner publishes; fixture Trajectory is out of the live build.
  4. Named ARM candidate exists with hashes; emit limitation stated.
**Plans**: `.planning/phases/10-live-k1-runtime/`

### Phase 11: Live development campaign
**Goal**: Programme the named candidate under WP7 Rearm and run live-plan Phase 7 real-music development checks.
**Depends on**: Phase 10 LIVE_RUNTIME_IMPLEMENTED
**Success Criteria**: WRITE_VERIFIED + INFO bind; development baseline scored; output limitations explicit. Not PRODUCT_QUALIFIED.

### Phase 6: WP5 combined live image admission
**Goal**: Admit one named current live image with explicit combined preflight/acceptance and truthful remaining limitations.
**Depends on**: Phase 5 transmitter admission. Independent of Phase 10 development image.
**Requirements**: [WP5-00, WP5-01, WP5-02, WP5-03, WP5-04, WP5-05, WP5-06, WP5-07, WP5-08, WP5-09, WP5-10, WP5-11]
**Success Criteria** (what must be TRUE):
  1. Autostart is mode 32, active+emit, no carousel, with recorded settings.
  2. Live SINCDEC/SINCRNG, rate, ASRC lock, and cadence are read back on the named image.
  3. Dwell/decay/centre-origin behaviour is measured; travelling structure is not inferred from centroid alone.
  4. No product four-lane or physical latency stamp is inherited.
**Plans**: 3 plans

Plans:
- [ ] 06-01: Named integration candidate, autostart, PDM/ASRC readback
- [ ] 06-02: Acoustic identity, rails/clips, real-music observation, dwell/decay/motion
- [ ] 06-03: Combined preflight, observer on/off, remaining limitations

### Phase 7: WP6 product output
**Goal**: Host-admit the full logical output composition; state physical WS2816/multi-lane as PASS/FAIL/BLOCKED/NOT TESTED at actual scope.
**Depends on**: Phase 6 for physical continuation; host slice may be prepared earlier but must not conceal a blocked transmitter
**Requirements**: [WP6-01, WP6-02, WP6-03, WP6-04, WP6-05, WP6-06, WP6-07, WP6-08, WP6-09]
**Success Criteria** (what must be TRUE):
  1. Effects→blend→treatment→edge policy→gain→joint current limit→stage/show is compared on both logical channels.
  2. TRUE16 `0x12AB` survives the wide path without 8-bit collapse.
  3. Four-lane/P603 work is not silently reduced to a pass; Captain owns physical DIN move.
**Plans**: 2 plans

Plans:
- [ ] 07-01: Slice admission, composition order, TRUE16 host vectors
- [ ] 07-02: WS2816 profile, topology/memory budget, physical status or BLOCKED

### Phase 8: Latency, comparison, ruling
**Goal**: Produce generation-joined latency evidence and an explicit ruling packet; Captain decides.
**Depends on**: Phase 6
**Requirements**: [LAT-01, LAT-02, LAT-03, LAT-04]
**Success Criteria** (what must be TRUE):
  1. Joined timestamps exist at the named points or each missing join is explicit.
  2. K1-L is PASS, FAIL, BLOCKED, or NOT TESTED under the 12 ms p50 definition.
  3. All fourteen EXECUTION-BRIEF §20 questions are answered or marked unavailable.
  4. No self-stamped K1-C.
**Plans**: 2 plans

Plans:
- [ ] 08-01: Generation-tagged joins and K1-L campaign
- [ ] 08-02: Bind S3/RT1062 baselines and write ruling packet

### Phase 9: Acceptance, docs, handoff
**Goal**: Close the applicable acceptance rows honestly and leave the next owner a complete handback, including parked work with resumption conditions.
**Depends on**: Phases 1–8 artefacts (partial rows allowed with BLOCKED/NOT TESTED)
**Requirements**: [ACC-01, DOC-01, DOC-02, DOC-03, DOC-04, STOP-01, STOP-02, STOP-03, STOP-04, STOP-05, STOP-06]
**Success Criteria** (what must be TRUE):
  1. Every §8 row has PASS/FAIL/BLOCKED/NOT TESTED plus receipt path/hash or explicit absence.
  2. 001↔002 crosswalk maps thirteen original deliverables.
  3. Remaining tasks name owner or **unassigned**, next action, inputs, prerequisite, prohibited claim.
  4. Parked §7 work is listed with resume-when, not scheduled as this programme's implementation.
**Plans**: 1 plan

Plans:
- [ ] 09-01: Acceptance matrix, crosswalk, STATUS/receipts, handback packet

## Progress

**Execution Order (Captain 2026-09-20):**
1 → remaining host negatives (2/3) → 10 live runtime → 11 Rearm + real-music
Separate: 1 → 4 (Q1–Q5) and 1 → 5 (P601/DIN), no blanket dependency either way
After transmitter: 5 → 6 → 7 physical and 8
1–11 artefacts → 9 handoff (BLOCKED rows allowed; that is complete handoff ≠ ship)

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. WP0 + WP7 SOP | 2/2 | Checkpoint written; waiter held | 2026-09-20 |
| 2. Remaining host negatives | bound | Green campaigns retained | 2026-09-20 |
| 10. Live K1 runtime | 10-01 + 10-02 | Clock/hop host-tested; AP owner next | 2026-09-20 |
| 2. WP1 capture | 0/2 | Not started | - |
| 3. WP2 hygiene | 0/3 | Not started | - |
| 4. WP3 timing | 0/2 | Not started | - |
| 5. WP4 LED | 0/3 | Not started | - |
| 6. WP5 live admit | 0/3 | Not started | - |
| 7. WP6 product | 0/2 | Not started | - |
| 8. Latency/ruling | 0/2 | Not started | - |
| 9. Docs/handoff | 0/1 | Not started | - |
