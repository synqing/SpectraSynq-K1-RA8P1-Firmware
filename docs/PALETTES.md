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

## Palette morph implementation — 2026-09-10

Build with `--palette-runtime --palette-autostart --palette-morph`.
This explicit derivative stages two small VP changes: a per-channel palette
provider and use of that provider for the existing HD sampler and brightest-stop
accent selection. The pinned imported source files remain byte-identical;
`verify_imports.py --slice product --enforce` still applies. The build receipt
hashes the overlay, transition code, and before/after staged source.
Without `--palette-morph`, there is no renderer overlay.

The implementation adds:

- Independent A/B palette targets across the existing 44 IDs.
- Smoothstep transitions lasting 1–10,000 ms; zero selects an immediate cut.
- Mid-transition retargeting from the current palette mixture.
- Continuous accent-phase movement along the shortest palette-phase arc.
- No frame history interpolation, second render, added queue, or allocation.
- 392 bytes of fixed transition state per channel on the checked host ABI.
  A normal fade samples two palettes. Repeated rapid retargets can reach the
  fixed maximum of 44 contributing palettes; target timing for this case is pending.
- The existing RGB8 frame and output treatment remain. Fractional transition
  weights do not make the physical path TRUE16 or establish colour calibration.

The runtime now sets `photons_id=65535` before applying its separate bench
brightness gain. Previously the default zero could make existing effects black.
The old composition test inherited that same default and did not establish
visible effect output. This corrects runtime configuration, not pinned renderer
arithmetic or the frozen fixture controls.

### Native demo

Boot autostart uses a 1,500 ms fade and cycles the catalogue every four seconds.
The configuration is consumed on Titan; no host-generated pixel stream is needed.

```sh
python scripts/run_titan_palettes.py --build <morph-build-directory> --palette 33 --palette-b 43 --cycle --transition-ms 1500
python scripts/run_titan_palettes.py --build <morph-build-directory> --palette 43 --palette-b 33 --transition-ms 1500
python scripts/run_titan_palettes.py --build <morph-build-directory> --palette 43 --transition-ms 0
```

These commands use preview mode 0. Existing effect selection remains available
through `--mode-a` and `--mode-b` once the shell has a valid AP view.
Morphing does not connect live capture to that view.

Opcode 16 retains its 32-byte version-1 request. Version 2 is exactly nine
little-endian u32 words: the original eight words with version=2, followed by
transition duration in milliseconds. Version/length mismatches, unsupported
versions and out-of-range durations are rejected atomically. Non-morph builds
reject version 2. Status reports support, duration, eased progress 0–65535 and
contributor count for each channel. Duration applies to both channels.

### Verified result and remaining physical work

`scripts/test_palette_runtime.py` builds separate pinned and derivative
executables. With transitions off, both produce digest
`d3a379fc7b760bb5` across 8,096 channel frames covering 23 modes and all
44 palettes. Of those frames, 2,982 are visibly nonzero with the short supplied
fixture; the other cases are not claimed as visible demonstrations.

All 11,264 palette-index endpoints are exact; 44 interruption cases preserve
current colour and accent phase. At zero progress, active morph rendering matches
the old palette across all 23 modes; nine modes visibly change at midpoint for
the supplied audio fixture. The suite rejects malformed controls and an overlay
source mutation, exercises the 44-contributor bound and preserves output after
CDC disconnect using the existing physical-output stub. The existing fixture
protocol regression also passes.

Built image:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/palette-morph-build-01`

Build ID:
`d354879b4ab051d151e64c49efe1cad6b5d6de965b9fe5dc1dd28ce08b863725`

Cross-build passes: text 196,284; data 18,104; BSS 204,316 bytes.
This image uses the current GPIO bench transmitter and does not enable PDM,
SSIE, resident AP scheduling, M33 or U55 work.

The GIF preview uses rows A/B from `tests/host/palette_morph_preview.cpp`.
It is a host visualisation of native C++ frame bytes, not optical evidence.
The extra synthetic-audio effect rows emitted by that utility are diagnostic
data and are not presented as successful effect demonstrations.

Programming, physical LED appearance and on-target morph/capture coexistence
remain unverified. The earlier palette programmer timed out without flashing;
application USB enumeration subsequently returned. The next physical action
is ROM boot, complete programming/readback, then native catalogue and transition
checks. No new silicon, pin, DMA or timing fact is asserted by this feature.
