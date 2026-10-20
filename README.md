THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# SpectraSynq K1 — RA8P1 / Titan Mini Firmware

Greenfield portability and evaluation repository for running the SpectraSynq K1 AP/VP compute architecture on the Renesas RA8P1 Titan Mini.

This repository is **not** a fork of `SpectraSynq-K1-DualMCU-Firmware` and does not inherit its Git history. The DualMCU repository is a pinned behavioural/reference authority only. The Titan BSP is likewise a pinned vendor/platform reference, not product-owned source.

## Current mission

`K1-RA8P1-001 — DualMCU Core Portability Probe`

Prove that the platform-neutral K1 compute stack can execute correctly on the RA8P1 Cortex-M85 before making any product-architecture decision. Initial target ownership is:

- Cortex-M85: AP + VP
- Cortex-M33: PARKED
- Ethos-U55: PARKED
- ESP32-S3: future RADIO-only companion; not part of first parity gate

Start with `docs/EXECUTION-BRIEF.md`.
