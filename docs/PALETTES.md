# Titan K1 palette runtime

All 44 palettes retain their canonical IDs, names, gradient stops and compiled
FastLED-compatible tables from the pinned K1 catalogue. No palette source data
or imported renderer arithmetic was changed.

## Runtime

- Independent palette and existing effect selection for channels A and B.
- Mode 0 is an on-device palette preview, not an audio-reactive effect.
- Existing modes use the actual K1 renderer and output treatment, with the
  latest valid AP view supplied by the fixture shell. This does not connect
  the separate PDM capture experiment to AP.
- Cycle mode advances every four seconds through all 44 entries.
- Two native 160-pixel frames; the current single-strip bench adapter samples
  the full extent to 128 WS2812 pixels, preserving the centre pair.
- CDC sends controls and reads status; it does not supply animation frames.
- The preview continues when CDC disconnects.
- Output currently uses the identified P601 GPIO diagnostic transmitter.
  Its interrupt blackout remains; this change is not DMA or audio-coexistence proof.
- Brightness defaults to 24/255. It is a bench output gain, not a replacement
  for the full production current-limiting composition.

## Build

From the repository root:

```sh
python3 scripts/build_scalar.py --output <new-build-directory> --palette-runtime --palette-autostart
```

The palette options are explicit build-identity inputs. Autostart boots into a
native full-catalogue preview on the existing WS2812/P601 128-pixel bench setup.
Without these options the existing fixture route is retained.

## Control

Use the EdgeAI Python environment and the exact build directory:

```sh
python scripts/run_titan_palettes.py --build <build-directory> --list
python scripts/run_titan_palettes.py --build <build-directory> --palette K1_Night_Sea_Amber_gp
python scripts/run_titan_palettes.py --build <build-directory> --palette 0 --palette-b 43 --cycle
python scripts/run_titan_palettes.py --build <build-directory> --status
python scripts/run_titan_palettes.py --build <build-directory> --stop
python scripts/run_titan_palettes.py --build <build-directory> --verify-all --output <new-receipt-directory>
```

The runner checks UID, firmware identity and the full catalogue before changing
controls. Verification selects every palette and checks native frames and output
callbacks. It leaves the autonomous full-catalogue preview running.
Existing nonzero effect modes wait for a valid supplied AP view; preview mode 0
does not manufacture musical analysis.

## Protocol

Existing K1S1/K1R1 framing and CRC apply.

| Opcode | Request | Reply |
|---|---|---|
| 15 | Empty | Full catalogue JSON, 44 IDs and names |
| 16 | Eight little-endian u32 words: version=1, palette A, palette B, mode A, mode B, flags, brightness, output channel | Runtime status |
| 17 | Empty | Runtime status and native-frame CRCs |
| 18 | One little-endian u32 channel (0 or 1) | Complete native 160-pixel RGB8 frame, 480 bytes |

Flags: bit 0 active; bit 1 automatic catalogue cycle; bit 2 physical bench output.
Unknown flags, IDs or modes are rejected atomically; no fallback to palette zero.
Flags zero stops updates and leaves the last frame latched. The CLI --stop first
submits black, then stops updates.

## Verification

```sh
python3 scripts/test_palette_runtime.py
python3 scripts/test_fixture_protocol.py
```

The palette suite covers all 44 entries on both channels, 14,080 preview pixels,
8,096 differential rendered channel frames across the 23 imported K1 modes,
invalid controls, catalogue capacity, bench mapping, cycle wrap, actual protocol
framing and output continuing after CDC disconnect. The existing parser regression
suite remains separate.

Built image and complete staged source:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/palette-build-01`

Build ID:
`09531e8764b4b954fc280c40df6fb315ad7ea931247dca449501367c53b1b6c2`

Host checks and cross-build passed. Board execution must be recorded separately.
