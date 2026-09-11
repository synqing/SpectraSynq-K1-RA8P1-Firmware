# Selected WS2812 showcase on WS2816 — reset fix and submission tap

Captain selected the later midnight showcase and instructed execution. No palette
tables, centre-effect renderer, GPIOs, wire timings or brightness transform were
changed in this repair. The WS2816 gain remains 128/255; the historical WS2812
gain was 24/255. Optical equivalence between those settings is not established.

## Observed fault and immediate repair

Fresh INFO-bound STATUS on build `11b95cdf` showed active mode 0, palette 33,
`automatic_cycle=false`, `showcase=false`, and transition duration zero. Thus the
running state no longer matched the previously applied showcase. The source's
WS2816 autostart override explicitly selected precisely that preview. This does
not establish who or what reset/reconfigured the board, or explain every earlier
appearance complaint.

Reapplied the selected configuration: palettes 33/43, modes 100/101, flags 23,
1,500 ms fades and 4,000 ms centre-out travel. The later STATUS showed modes
102/103, palettes 12/22, both cycles active and advancing emissions without
software emission errors. CDC was released after each transaction session.

Receipts, relative to the campaign artifact root:

- `ws2816-showcase-restore-02/receipt.json`
- `ws2816-showcase-restore-status-02/receipt.json`

## Implemented

- `platform/ra8p1/fixture_app.cpp`: WS2816 autostart no longer replaces the
  inherited morph/showcase configuration with single-palette preview. Non-morph
  builds retain catalogue cycling; no-autostart builds remain inactive.
- Same file: read-only opcode 19 returns a coherent snapshot of the actual two
  emitter input buffers, alongside their source RGB8 and submission return codes.
  Static storage, no heap; both emit calls finish before a request can read it.
  This is a submission tap, not a measured GPIO waveform or LED response.
- `scripts/run_titan_palettes.py --capture-frames N --output DIR`: identity-bound,
  read-only capture and scoring at one sample per second, 2..60 samples. A run of
  50 or more samples requires all four effects, changing palettes and aggregate
  coverage of all 160 positions. No host-generated pixel stream.
- `scripts/palette_wire_snapshot.py`: independently reconstructs GRB48 values
  from each captured RGB8 frame and its gain; rejects bad shape/configuration and
  fails on byte mismatches or emitter errors.
- Protocol tests cover boot defaults with/without morph and autostart, both
  backends, actual emitter-buffer equality, unavailable/malformed snapshots and
  failed emissions. A disposable-source mutation restores the old boot bug and
  must fail. Four scorer tests cover both lanes, low bytes and malformed input.

## Snapshot v1 contract

Opcode 19, empty request; only WS2816 palette builds implement it. Before the
first completed submission it returns status 8, never invented zero evidence.
Payload is 1504 bytes: 64-byte header, 480-byte native RGB8, 960-byte GRB48.
GRB48 contains lane A's 80 pixels followed by lane B's 80 pixels, big-endian
16-bit G/R/B components. These buffers themselves are passed to the emitter.
Header consists of 16 little-endian uint32 words:

`version, sequence, output_channel, brightness, lanes, pixels_per_lane,
bits_per_pixel, wire_profile, selected_mode, selected_palette, flags, period_us,
result_a, result_b, time_low, time_high`.

Signed emitter errors are retained as their uint32 bit patterns. A capture is
the last completed attempt and may be stale after output stops; the host requires
advancing sequence values across samples. Native frame and both lanes belong to
the same attempt. There is no re-render in the snapshot command. Mirror metrics
are reported separately; the buffer scorer does not mistake software symmetry
for measured direction or physical strip orientation.

## Build and checks

Candidate: `ws2816-showcase-build-02`.
Build ID: `70c2323a9e85611ad969096b45d73dfb9a1dda8a7e108abc1a2f89233c489500`.
HEX SHA-256: `dba5e6e7cdc07d5bd3a7f027a1cfa8eada7c28b998ed85a1cc2229b78380273e`.
Repo HEAD at entry: `f28e3a4a4e60562eef90dc8ae41fd0f8b83d637b`; dirty work
preserved, no blanket staging or commit. Built source is pinned by the build
receipt, not that HEAD alone.

Commands (project Python `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python`):

```sh
python scripts/test_palette_runtime.py
python scripts/test_palette_wire_snapshot.py
python scripts/build_scalar.py --palette-runtime --palette-morph \
  --palette-ws2816 --palette-autostart --output ABSOLUTE_NEW_BUILD_DIR
python scripts/run_titan_palettes.py --build ABSOLUTE_BUILD_DIR \
  --capture-frames 52 --output ABSOLUTE_NEW_CAPTURE_DIR
```

First three commands passed. Logs: candidate `host-tests.txt`,
`wire-scorer-tests.txt`, `build.log`, `receipt.json`. The snapshot-capture command
requires this candidate installed; it must not be run against the old image as
though its missing opcode were evidence of corrupt LED data.

## Target completion

**Completed on the re-arm request:** `ws2816-showcase-prog-03/receipt.json`
reports PASS for the specified UID and build. `programming.log` records full
readback verification of **217,408 bytes**. After Captain's normal RESET, INFO
returned build `70c2323a...`; STATUS showed showcase and catalogue cycling active
without any opcode-16 configuration command. The boot override is fixed on the
identified target, not merely in the source.

Read-only capture `ws2816-showcase-wire-02/receipt.json` passed: 52 coherent
snapshots over 58.586 seconds between first/last host observations, all four
modes 100–103, 15 distinct palettes, all 160 positions nonzero across the capture,
zero scaling/packing byte mismatches, zero mirror pixel mismatches and zero
reported emitter errors. The runtime advanced 3,531 frames between the enclosing
STATUS reads. Do not divide this count by the differently bounded snapshot
duration to claim a precision frame-rate measurement.

**BOOT_SHOWCASE = PASS; SUBMITTED_WIRE_BUFFERS = PASS.** The runner closed CDC
and left the native showcase active. No listener remains armed. This does not
measure GPIO pulse widths or establish calibrated optical equivalence with the
WS2812 strip. If the appearance remains wrong despite these correct submitted
values, the next agent-owned step is a paired instrumented GPIO/LED-response
measurement; do not repeat palette recovery or silently add a gamma curve.

### Earlier failed attempt (preserved)

Programming listener `ws2816-showcase-prog-02` waited 180 seconds and exited 1:
no RA8P1 ROM device appeared; the observed Titan remained application CDC
`045b:5310`. No programming connection or flash write occurred. The listener is
**no longer armed**. At that point candidate readback and capture were pending;
the successful retry above supersedes that status. Preserve both receipts.

The successful retry used the following command (do not reuse its output path):

```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python -u scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-showcase-build-02 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-showcase-prog-03 \
  --wait-seconds 300 --execute
```

Any future reflash requires an unused output directory. Do not tell Captain a
listener is armed until its `WAITING_FOR_IDENTIFIED_ROM` output actually appears.

1. Completed: agent programmed only UID `545433931bd25436593630352d068363` and verified
   complete image readback; Captain performs the required ROM/reset buttons.
2. Completed: after normal RESET, agent verified candidate INFO and boot showcase without
   sending a configuration command, then captures/scores 52 submission snapshots.
3. Completed: boot/showcase and wire-buffer checks stamped on matching receipts.
   If correctly scored buffers still produce the wrong appearance, the remaining
   discriminator is instrumented GPIO/LED response against the selected reference,
   not another arbitrary palette substitution or assumed gamma curve.

All artifacts are under
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`.
This LED repair does not modify the separate G4 deadline-miss verdict.
