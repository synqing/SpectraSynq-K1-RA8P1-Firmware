THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Task Plan: Titan RA8P1 takeover execution plans

## Goal
Correct the execution index (live-runtime un-deferred), reconcile recovery identity, bind existing host tests, then execute Phase 1 checkpoint and onward live-runtime work. Takeover brief is historical; do not amend it.

## Current Phase
Phase 1 checkpoint executing; live-runtime in-scope

## Key Questions
1. Is the 20 September live-runtime rewrite in scope? **Yes — transmitter admission is not a prerequisite for the development runtime.**
2. Is WP0 still required given STATUS identity PASS? **Yes — verify receipts; do not skip.**
3. Commit? **No unless Captain asks.**
4. Does a BLOCKED handback mean incomplete work? **No — handoff complete ≠ ship.**

## Phases

### Phase 1: Requirements & Discovery
- [x] Treat the takeover brief as locked PRD
- [x] Restore planning-with-files working memory
- [x] Confirm GSD was uninitialised; bootstrap from the brief rather than interactive new-project questioning
- [x] Run platform-memory `--check` and recall pins/dma/ws2816
- [x] Distinguish audited commit `4311408` from the dirty Mac tree and the 20 September STATUS entry
- **Status:** complete

### Phase 2: Planning & Structure
- [x] Write `.planning/PROJECT.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `STATE.md`, `config.json`
- [x] Write per-phase CONTEXT.md covering every brief step
- [x] Research current APIs/tests/receipts (SSA wave, ceiling 3)
- [x] Write GSD `*-PLAN.md` files plus `docs/superpowers/plans/2026-09-20-titan-ra8p1-takeover.md`
- **Status:** complete

### Phase 3: Verification
- [x] Red-team / second-order review against the brief
- [x] gsd-plan-checker
- [x] Confirm every WP step, finding, acceptance row, and parked item has a plan task
- **Status:** complete

### Phase 4: Delivery
- [x] Present GSD status, file index, and `$gsd-execute-phase 1`
- [x] Do not auto-execute, flash, or commit
- **Status:** complete

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| Brief is locked PRD | Captain ordered every stage/step implemented as plans |
| Fine granularity, 9 GSD phases | One phase per WP/section; WP7 SOP lives in Phase 1 so later Rearms have a prepared waiter |
| Sequential WP execution | Brief forbids relaunching independent-review agents and says run sequentially |
| Dirty-tree host repairs are hypotheses | STATUS claims host scorer PASS; plans must re-run negatives before treating them as closed |
| Do not execute this session | User asked for a plan, not Rearm/flash |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| `.planning/` missing | 1 | Bootstrap GSD from the brief instead of aborting `$gsd-plan-phase` |
| `gsd-planner` skill path missing | 1 | Use Task `gsd-planner` + `templates/phase-prompt.md` |

## Notes
- Authoritative spec: `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md`
- Audited commit in the brief: `431140853dd8b58af53240ef84d5fb1a08bd8b45`
- Current branch: `lane/k1-ra8p1-002` at that commit, with uncommitted host/docs work
