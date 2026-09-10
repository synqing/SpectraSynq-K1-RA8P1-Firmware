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

## Centre-origin expansion — 2026-09-10

User mandate: all VP motion originates at the centre and travels outward, or
originates at the edges and travels inward. Native geometry is 160 pixels,
centre pair 79/80. The current 128-pixel adapter retains centre pair 63/64.

The earlier morph image `d354879b…` was programmed with complete readback and
verified by live UID/build identity. All 44 palette selections passed on Titan.
A single CDC session observed eased fade progress from 54 through 65,535 and
an interrupted fade retaining three palette contributors on each channel,
with zero reported emission errors. Captain confirmed that the visible result
looked good. No photometric measurement or audio-reactive physical claim follows.

Evidence under `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`:

- `palette-morph-programme-01/receipt.json`: programming/readback pass.
- `palette-morph-live-01/receipt.json`: all 44 IDs selected on Titan.
- `palette-morph-transition-live-02/receipt.json`: intermediate progress,
  completion and interrupted retarget pass.
- `palette-morph-transition-live-01/receipt.json`: earlier observer failed to
  catch an intermediate frame because it reopened the CLI for each sample.
  That negative record is retained; the single-session observation supersedes it.

The 1,500 ms transition completed after approximately 2.17 host seconds in that
image. The old animation timebase used RTOS ticks while the GPIO transmitter
masked interrupts. This build instead extends the existing free-running DWT
cycle count with fractional conversion and unsigned wrap handling. It never
resets DWT. Polling must occur more often than one 32-bit wrap (4.295 seconds
at 1 GHz); debugger halts and deep sleep are outside this bench clock contract.
Protocol timeout behaviour still uses its original clock. This is not a DMA fix
or a claim of externally calibrated oscillator accuracy.

### Implemented native effects

| ID | CLI name | Behaviour |
|---|---|---|
| 100 | ribbons | Broad flowing colour bands with narrow palette-derived highlights |
| 101 | aurora | Overlapping soft colour curtains |
| 102 | embers | Narrow bright crests with trailing colour |
| 103 | pulse | Travelling pulse fronts with a wider halo |

These four effects run autonomously without an audio fixture. They are ambient
effects, not fabricated musical analysis. Their background, body and highlight
colours all come from the selected palette mixture. Composition uses floats
until the existing RGB8 frame boundary; the bench gain and WS2812 quantisation
remain. No new temporal dither or TRUE16 physical path is introduced.

Each effect evaluates one radial coordinate and copies the result to both
halves. Reversing travel reverses the radial coordinate; it does not reverse the
whole 160-pixel strip into a horizontal wipe. The original preview mode 0 now
also obeys the centre-origin mapping. That intentional preview change is
separate from the pinned K1 musical effects, which remain unchanged.

Showcase mode moves to the next effect every 12 seconds, blending old and new
effect envelopes over 800 ms at the same current time. Palette cycling remains
every four seconds with the selected palette-transition duration. Autostart
uses showcase mode, 1,500 ms palette fades and four-second centre-to-edge travel.
New effects add no frame history, allocation or queue.

### Controls and protocol

```sh
python scripts/run_titan_palettes.py --build <centre-build> --showcase --cycle --palette 33 --palette-b 43 --transition-ms 1500
python scripts/run_titan_palettes.py --build <centre-build> --effect-a aurora --effect-b embers --palette 2 --palette-b 23 --transition-ms 1500
python scripts/run_titan_palettes.py --build <centre-build> --effect-a pulse --effect-b ribbons --inward --travel-ms 3000
```

Version 3 of opcode 16 is exactly ten little-endian u32 words:
version=3, palette A, palette B, mode A, mode B, flags, brightness, output channel,
transition milliseconds, travel milliseconds. New flags: bit 3 inward travel;
bit 4 effect showcase. Travel range is 500–30,000 ms for the four new effects.
Mode 0 retains its existing preview colour-scroll rate.
Versions 1/2 retain their earlier payload lengths and controls.
Inward/showcase controls are rejected for unsupported legacy effect selections.
Status reports effective effect IDs/names, direction, travel duration and showcase.

### Checked result

The host suite covers all 352 new effect/palette/direction combinations,
337,920 mirror/direction comparisons and the physical 128-pixel mapping.
All 4,224 sampled outward frames contain visible values. A pulse front is at
radial index 20 outward and 59 inward after one quarter of the journey.
All 8,096 existing effect comparison frames also satisfy mirror symmetry for
the supplied fixtures and retain digest `d3a379fc7b760bb5`.
Mirror checks alone are not a proof of every legacy mode's temporal trajectory.
Clock tests cover wrap, fractional conversion, and actual protocol transitions
while the mocked RTOS tick is frozen.

Built image:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/centre-effects-build-01`

Build ID:
`3decd1541b7a5fb1b77b65343542a6562d2286be69c3871713fca1d1b762bdb6`

Cross-build passes: text 198,612; data 18,104; BSS 204,332 bytes.
The new image is now programmed and verified; see the live result below. This image retains the GPIO bench transmitter; live capture,
production AP timing, DMA output and U55 coexistence are separate outstanding work.

### Live centre-effects verification — 2026-09-10

Programmed build `3decd1541b7a5fb1b77b65343542a6562d2286be69c3871713fca1d1b762bdb6`
on UID `545433931bd25436593630352d068363` with complete readback PASS.
The first programmer wait expired without finding ROM; `centre-effects-programme-02`
completed successfully. Normal reset then returned the exact expected application identity.

One exclusive CDC session checked all four new modes in both directions (eight
cells), both complete 160-pixel native channel frames, nonzero output, exact
mirror symmetry, advancing physical-output callbacks and zero emission errors.
This checks selected live frames; host trajectory checks provide the direction
comparison. No host-generated pixel frames were sent.

The requested 1,500 ms palette fade progressed monotonically through intermediate
Q16 values and completed at the host's 1.690689 s observation, including request,
response and polling overhead. This is an improvement over the previous 2.166862 s
host observation, not a calibrated optical timing result.

Final status: 16,576 rendered frames, 16,565 emissions, zero skipped releases and
zero emission errors at that observation. Showcase and all-44 automatic palette
cycle remain enabled, centre-out, four-second travel, 1,500 ms palette fades,
brightness 24/255, output channel A. The CDC session is closed.

Receipts and reproducible checker:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/centre-effects-programme-02/receipt.json`
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/centre-effects-live-01/receipt.json`
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/centre-effects-live-01/check.py`

The installed path remains the 128-pixel WS2812 GPIO bench backend. This does not
prove optical fidelity, the WS2816 TRUE16 shipping path, physical audio-to-light
operation, production AP deadlines, or simultaneous U55 operation.
