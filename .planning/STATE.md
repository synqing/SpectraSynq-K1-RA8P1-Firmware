THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-09-20)

**Core value:** Usable live K1 development runtime plus honest capture/identity gates
**Current focus:** Phase 1 checkpoint, then live-runtime implementation (transmitter not a runtime gate)

## Current Position

Phase: 10 live-runtime clock/hop contracts started after Phase 1 checkpoint
Plan: 10-02 host gate green (`LIVE_CLOCK_CONTRACT_PASS`)
Status: Analyser still BLOCKED; runtime implementation not waiting on it
Last activity: 2026-09-20 afternoon checkpoint + first live-runtime source delta

Progress: Phase 1 complete (held Rearm); live clock/hop host-tested

## Performance Metrics

**Velocity:**
- Total plans completed: 0
- Average duration: n/a
- Total execution time: 0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| - | - | - | - |

## Accumulated Context

### Decisions

- Takeover brief is locked historical PRD; do not amend it. Live-runtime plan is now in-scope.
- Persistent lead + bounded subagents; CDC/programming one owner; max 3 concurrent SSAs.
- Dirty-tree host diffs are evidence to bind by file SHA; do not restart green campaigns.
- Programmer safety HEX `a167833b…` ≠ intended recovery HEX `3aa09139…` / `live-audio-gpt-20260920-03`.
- Hashed BLOCKED handback = complete handoff ≠ ship.
- STATUS 20 September identity: live build `c7f6034a…`, UID `545433931bd25436593630352d068363`, P601 BLOCKED, empty-TCM HEX built not Rearmed.
- Audited brief commit: `431140853dd8b58af53240ef84d5fb1a08bd8b45` on `lane/k1-ra8p1-002`.
- Do not commit planning docs unless Captain asks. Do not auto-advance to execute-phase.

### Pending Todos

Write CONTEXT.md per phase, research APIs, write PLAN.md files, writing-plans master document, plan-check.

### Blockers/Concerns

- No logic analyser attached (STATUS): WP4 physical capture BLOCKED until instrument exists; host WP1 still proceeds.
- Uncommitted work in the same files the plans will later edit — executors must inventory and not reset.
- `commit_docs` forced false despite GSD default true (user git rule).

## Session Continuity

Last session: 2026-09-20
Stopped at: GSD bootstrap; research wave not yet returned
Resume file: None
