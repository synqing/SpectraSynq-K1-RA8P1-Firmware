THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Findings & Decisions — Titan takeover planning

## Requirements
- Implement every WP0–WP7 step, F-01–F-06, §6 latency/ruling, §7 parked work, §8 acceptance matrix, §9 handoff as executable plans.
- Preserve AGENTS.md invariants, Rearm protocol, centre-origin mandate, scalar-first, M33/U55 parked.
- Do not flash, do not invent wire evidence, do not start a fourth DMA arbitration-map experiment.

## Research Findings

### Authority
- Brief audited commit `4311408` on `lane/k1-ra8p1-002` / `main`.
- Dirty tree already touches the brief's target files (`score_p601_capture.py`, `titan_snapshot.py`, `titan_broker.py`, `build_scalar.py`, `run_mode32_real_audio.py`, STATUS, external-receipts, untracked host tests).
- STATUS newest entry (20 September takeover) claims: live INFO identity `c7f6034a…` on UID `545433931bd25436593630352d068363`; no broker CDC owner; capture scorer host PASS; P601 BLOCKED (no analyser); empty-TCM HEX built `22d405c1…` but not Rearmed; resident is live-audio not profiler.
- `docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md` is the **onward runtime sequence**. Transmitter admission is required for combined physical-output acceptance, not for AP/VP development. The takeover brief remains historical and is not amended.
- `docs/superpowers/plans/2026-09-19-k1-ra8p1-002-remaining.md` records Rearm over `a3f37e8a` before capture; preserve-resident is waived, not captured.

### Platform memory (`platform_memory.py --check`)
- Records valid, SHA-256 `fd316def…`, 26 facts, `hardware_validated: false`.
- P601 = GTIOC6A; P004 has no GPT/serial output; P603 = GTIOC7A later route only with Captain physical approval.
- Default FastLED WS2812 profile is not WS2816C; share a transmitter only with part-specific profiles.

### Key source files exist
All brief targets exist: capture scorer, GPT fault witness, real-audio runner, colour integrity, programme_scalar, run_scalar_schedule, build_scalar, stage_profile, tempo_dtcm_overlay, titan_snapshot/broker, palette_runtime, ws281x_gpt_dma_hw, k1_asrc_24k, pdm_target, STATUS, external-receipts, timing brief, EXECUTION-BRIEF §20, ruling stub, latency decision.

## Technical Decisions
| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Nine GSD phases | Fine granularity matching WP0–WP6 + latency + docs; WP7 SOP in Phase 1 | Locked |
| Sequential execution waves across WPs | Brief: run sequentially; prior independent-review dispatch rejected | Locked |
| Host-only WP1/WP2 may proceed if hardware absent | Brief exit criteria | Locked |
| No fourth arbitration map | Three DMA policies already failed at terminal word | Locked |
| Live-runtime rewrite deferred | Would confound LED diagnosis | Locked |

## Resources
- Spec: `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md`
- GSD output: `.planning/`
- Writing-plans output: `docs/superpowers/plans/2026-09-20-titan-ra8p1-takeover.md`
- External evidence root: `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`
- Titan skill: `/Users/spectrasynq/Workspace_Management/Software/agent-skills/packages/ra8p1-titan-engineering`

## Open questions for execution (not this planning session)
- Is a logic analyser on USB now? STATUS said no.
- Are empty-TCM images still waiting for Rearm?
- Which uncommitted host diffs already satisfy F-01/F-04/F-06 negatives?
