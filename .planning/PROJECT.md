THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Titan Mini / RA8P1 takeover

## What This Is

Executable continuation of K1-RA8P1-002 on Titan Mini: close the live audio-to-light chain with honest measurements, starting from the 20 September 2026 takeover brief. Titan remains an evaluation platform. It is not a qualified K1 product runtime and must not be promoted over RT1062 until the measured integration gaps close.

## Core Value

A usable live K1 development runtime on an identified Titan image, with capture provenance separated from waveform conformance. Transmitter admission remains required for combined physical-output acceptance. It is not required to implement the live processing loop.

## Requirements

### Validated

- Scalar F1 typed-field parity on identified Titan (compute only; not the treated physical chain)
- Historical G4 slim-status O3 DTCM cache-off scalar qualification (that image only)
- LED completion guard: nonzero DMCRA faults; immutable v2 diagnostic; host mutations reject false completion
- Host palette suite including centre-origin / direction checks
- Serial Studio SS-02 assets; stale 1.8 GB broker log archived and hashed

### Active

- WP0 truthful source/device/evidence checkpoint
- WP1 capture gate that cannot stamp physical success from synthetic or clipped frames
- WP2 broker expected-build binding, telemetry units, F-06 real-audio observation vs acceptance
- WP3 uncapped Q1–Q5 in the configuration the timing brief actually requires
- WP4 first-divergence capture, bounded repair, original three controls, optical claim kept separate
- Live K1 runtime implementation (`Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md`) through a named development candidate; transmitter is not a prerequisite
- WP5 named combined live PDM/AP/VP image admission after transmitter repair
- WP6 product-output chain admission; physical WS2816/multi-lane scoped honestly
- WP7 Rearm protocol prepared before any dependent image transition
- K1-L latency joins and RA8P1-vs-RT1062 ruling packet for Captain

### Out of Scope

- M33 partition, U55 product model, actual-K1 NPU coexistence — parked until measured deficit / named model
- Helium/MVE, FPU, D-cache-on — parked; prior cache-on lost USB
- PCM1808, Titan–S3 radio bridge, P603/DIN-B physical move, power/thermal campaign, full optical latency, independent G8 review — parked with brief §7 resumption conditions
- Fourth DMA arbitration-map experiment as the next LED action
- DualMCU Lane F, Teensy, PCB, sibling-repo edits
- Combined physical-output acceptance before transmitter admission (runtime development is in-scope; emit-disabled allowed)
- Promoting Titan over RT1062 from this agent session

## Context

Greenfield RA8P1 firmware repo. Behavioural pin DualMCU `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`. BSP pin `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`. Physical target Titan Mini HW V1.0 UID `545433931bd25436593630352d068363`. Application USB `045b:5310`.

Three colour-integrity DMA policies failed at the terminal one-word remainder. Preserve-resident hold was waived by 19 September Rearm of profiler `32dd1f5f…` / HEX `9f12869a…` over `a3f37e8a…` before wire capture. STATUS 20 September records a later live-audio resident `c7f6034a…` identified by opcode-1 INFO. Host repairs exist uncommitted. P601/DIN remains BLOCKED without an attached analyser.

## Constraints

- **Source**: Shared K1 code must not include Renesas/FSP/RT-Thread headers. Platform under `platform/ra8p1/`.
- **Cores**: M85 owns AP+VP. M33 parked. U55 not an implicit production dependency. D-cache remains off unless a separately scoped experiment is reopened.
- **Time**: Preserve `MEDIA_TIME_48K`, musical time, event vs availability time, affine mapping, epochs, bounded render scheduling.
- **AP contract**: 24,000 Hz, 180 samples, 7,500 µs hop, 80 bins, crossover 40.
- **Geometry**: Two logical channels of 160 pixels; centres 79/80. Bench 128-pixel centre 63/64. Centre-origin or edge-origin motion only.
- **LED pin**: P601 = GTIOC6A. GPT6 pulses, GPT0 hardware stop. P004 has no GPT/serial-output route. P603 later only with Captain physical approval.
- **DMA**: Duty data in DMA-accessible SRAM, not DTCM.
- **Operator**: Rearm starts a prepared waiter and replies `WAITING`. Never reuse programme/run directories.
- **Evidence**: Host/compile is not physical timing, fidelity, LED phase, or multi-K1 sync. Agent dump/score, not Captain eyes, for firmware emit.
- **Git**: Narrow commits. No `git add -A`. No sibling worktree. No history rewrite. Do not reset dirty unrelated work.
- **Audio room**: No synthetic computer tones or white noise. Silent host fixtures OK.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Brief is locked PRD | Captain: implement every stage and step | — Pending execution |
| Sequential WPs | Independent-review dispatch was rejected; LED diagnosis must not be confounded | — Pending |
| Capture outcomes split | F-01: one `pass` flag laundered synthetic data into physical claims | — Pending |
| No fourth arbitration map | Three policies, same remainder | ✓ Locked |
| Empty-TCM vs DTCM named separately | F-03: `--resident-controls` always overlays DTCM | — Pending |
| Expected-build ≠ recognised UID | F-04 | — Pending |
| RT1062 remains primary eval until Captain stamps otherwise | Brief recommendation; agent must not self-stamp K1-C | — Pending |

---
*Last updated: 2026-09-20 after PRD express from takeover brief*
