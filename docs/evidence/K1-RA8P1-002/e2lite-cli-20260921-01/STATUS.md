# E2 Lite CLI handover — 21 Sep 2026

**RESOLVED 2026-09-22:** full SWD target connection achieved over the hand-soldered six-wire harness (pins 1,2,3,4,9,10). The probe read User Vcc 3.313 V, found the DAP, and read the RA8P1's DLM state (OEM, AL2) from silicon. Receipts: `../e2lite-harness-attach-20260922/`. Everything below is preserved as history of the blocked period.

**Authority:** agent classification from the logs named below. **Captain ratification: OPEN.**

**Status field (handover §9):** `BLOCKED` *(superseded — see RESOLVED note above)* — no verified CPU0 debug session at the time of writing.

The probe is on this Mac and the debugger now names the Titan chip correctly. The chip still does not answer on SWD.

## What happened

The first CLI start used `/tmp/renesas-host-check-e4rag280` (gdb binary only, no RA8P1 MCU file table). Log `attach-01-hotplug.log` says `No matching device found : Device = <E2LITE>, CPU = <R7KA8P1KF>`.

The installed runtime `/Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0` contains `ra8p1.6.6.0.xml` with `cpuName="R7KA8P1KF"` and `ARM/E2_v2.8.1/MCUFiles/RA8P1/R7KA8P1KF.mcu`. From that directory, this session ran SWD at 10 kHz, probe power off, no download. Log `attach-02-runtime-10khz.log` says `Error 0x00030815` DAP cannot be found.

Earlier files already in that runtime (`prior-diagnostic-state.json`, `prior-standard-connection-100khz.log`, `prior-connect-under-reset-100khz.log`, `prior-hotplug-after-power-cycle.log`) record the same DAP error at 100 kHz and 1000 kHz, connect-under-reset, and hot-plug with the ribbon already fitted. The hot-plug log also says `User program was reset`.

Live USB this session: E2 Lite serial `E2L: 5AS079228B`, configuration 1. No second Renesas `045b` device (Titan USB-DEV CDC not enumerated).

No erase or program was requested on the attach command lines.

## What is true now

- gdb server is not running.
- e² studio is not running.
- Probe power was commanded off (`-w 0`).
- Flash contents were not rewritten by these attaches.

## Host lock (22 Sep 2026)

Abandoned POSIX semaphore `/CommuniDLL_SemE2L: 5AS079228B` was the Mac-side cause of `0x0003080A`. Live re-check 2026-09-21T17:18:32Z: `HOST_LOCK_CLEAR`, no debugger processes. Receipts: `docs/E2Lite-macOS-host-lock-recovery-20260922/`. This does **not** prove SWD/DAP.

Titan USB-C 5 V is a constant. Do not query it.

**Live blocker now:** SWD DAP `0x00030815`. Host lock is recovered. E2 Lite USB `5AS079228B` is present.

## What is left

Host lock `0x0003080A` is closed. Delayed hot-plug (attach-12) and reset-begin (attach-13) both returned `0x00030815` DAP cannot be found at 10 kHz, probe power off, AuthLevel None, no download. Unchanged clock sweep, UART query, and probe voltage-on stay closed.

Next discriminator is the E2 Lite **20-pin** socket (1.27 mm) plus 20-to-10 into J6. The 14-pin socket is for RL78/RX, not RA; Renesas’ own 30815 text names that swap. Official pin map (R20UT4686 Table 2.3): J6.1 VCC, 2 SWDIO, 4 SWCLK, 9 UCON to GND, 10 RESET#. Then one more attach.

**Shipped stamp for this handover:** gdb server listening and CPU0 registers readable with no image download.

## Evidence directory

`docs/evidence/K1-RA8P1-002/e2lite-cli-20260921-01/`
