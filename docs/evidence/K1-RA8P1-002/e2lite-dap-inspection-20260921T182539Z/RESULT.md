# E2 Lite DAP follow-up — target connection remains unresolved

The latest attach-12 log reaches the vendor hot-plug prompt and then fails with 0x00030815. A fresh read-only host check returned HOST_LOCK_CLEAR with no active debugger or communication-library consumers. This is not a recurrence of the stale host semaphore.

The installed vendor launch-parameter documentation was inspected directly. uNoReset=1 suppresses reset at the END of connection; it does not invalidate an initial-reset or hold-reset test. The earlier under-reset test really specified HotPlug=0, ResetCon=1, ResetBeginConnection=1, NoReset=1. Do not claim it was a contradictory no-reset test or repeat it unchanged.

The preserved live-image build stage configures P210 and P211 for peripheral DEBUG. This is static build evidence, not a live-register read. The v1.0 board schematic's J6 wiring was visually inspected; the drawing is not proof of actual adapter continuity.

No GDB launch, target reset/halt, flash, security write, or cable change was performed during this follow-up. No target-level success is claimed. The missing discriminator is electrical: approximately 3.3 V at the probe-end target-reference contact relative to its ground, and approximately 3.3 V at Titan RESET# when idle. Measure with no debugger session, retain externally powered target/probe-output-off configuration, and avoid bridging adjacent contacts. Only perform continuity tests with every supply disconnected.

If both DC levels are correct, inspect actual cable continuity and SWCLK/SWDIO activity before concluding that the MCU is locked or damaged. No erase/unlock is justified by this generic DAP error.
