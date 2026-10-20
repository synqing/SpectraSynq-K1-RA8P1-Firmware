# Debug session and settings read-back — 22 September 2026

The pair image was running with light output off. A later debugger connect halted the chip and never resumed it, so the application USB port is currently absent.

## Settings, read from the board

Build `e5d51385318c6ee75bf0d40ab835ee2d94551e7ac744bdc0cb8de22309f4ea1f`. Board `545433931bd25436593630352d068363`.

- Mode 32, palettes 33 and 43, brightness 24, flags 1, emit off.
- Configured backend `ws2816_gpt_pair`. Output backend `disabled` while emit is off. Emitted frames 0. Pair completions 0. No lane faults.
- Wire profile 3, 80 pixels on P601 and 80 on P603, source precision `rgb8_to_grb48`.
- Microphone capture is running: about 118,362 analysis hops, no overflow, no dropped slots, no recovery. D-cache is off. Stack still has untouched bytes.

The input-rate figure is still 0 and the rate lock is not set, after a long run. That is recorded, not patched in this step.

## Debugger

The E2 Lite was on USB and its host lock was clear. The server was started at 10 kHz with probe power off, on the image whose file contains this build id. The 21 September image does not contain that id, and it was not loaded.

After the target-power wire was reseated, the probe connected. The log says `Finished target connection`, User Vcc about 3.32 V, DLM OEM, AL2, at both 10 kHz and 1 MHz. That log line is the debug proof.

Attaching GDB and asking for the program counter makes the server call `getAllRegisters`. That call does not return at either speed. Interrupting GDB leaves `e2-server-gdb` unkillable (processes 13102 on port 61234 and 15517 on port 61240). Neither process holds the probe USB device. Do not attach GDB again. The router skill now says to stop at `Finished target connection`.

## Identity guard

The symbols ELF contains build `e5d51385…`. The older WS2812 ELF does not. That check refused the wrong file before any debugger symbol load.
