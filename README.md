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
