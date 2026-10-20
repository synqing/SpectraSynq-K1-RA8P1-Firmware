# E2 Lite writes and talks without buttons

22 September 2026. The procedure agents must follow is `~/.cursor/skills/ra8p1-e2lite-router/SKILL.md`. This file is the proof that procedure was run.

The probe is E2 Lite `5AS079228B`. The chip is `R7KA8P1KFLCAC`, board `545433931bd25436593630352d068363`. Probe power stayed off. No button was pressed.

## What was run

Tool: `/Users/spectrasynq/Applications/Renesas/RFP_CLI_V3.24.00/rfp-cli`.

A write at 100 kHz stopped at 54% with `E3000105` (BFW 3053). Log: `e2lite-rfp-20260922/program.log`.

The same file at 1 MHz wrote and verified code MRAM `02000000–020430DF`. The range stopped before config `02C9F020`. `-run` released the chip. Log: `e2lite-rfp-20260922/program-1mhz.log`. Hex CRC `23D60128`.

The application USB port then answered build `3a7ebd7c0d75f5d415fee6257e54bb6410d1c2d2c07a30fca2cc59ccdcc9ed2f`, emit 0, microphone rate locked at 41575 Hz.

A later identity read, `rfp-cli -d RA -t e2l:5AS079228B -if swd -s 1000000 -noquery -run -sig`, restarted the application and left it running. The same build came back, emit stayed 0, and the rate locked again at 41558 Hz. Log: `e2lite-rfp-20260922/signature-run.log`.

## How the two talks split

The running application is read on USB `0x045B:0x5310`. That path does not reset the chip.

Silicon identity and image writes use the commands in the skill. A signature read does restart the application. GDB is not used. The serial ROM programmer is not used.
