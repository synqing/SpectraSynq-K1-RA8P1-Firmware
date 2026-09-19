# Titan colour-integrity integration — 2026-09-16

## Decision and present boundary

Captain has prioritised the frozen EdgeAI `docs/Titan_Colour_Integrity_Tests_and_Patch`
packet over onwards development. Lead implementation is in this existing firmware
checkout, not the packet or a sibling worktree. All 46 packet manifest entries
remain intact. Preparation performed no flash, reset, serial transaction, room
playback or commit. The subsequent Captain-authorised execution is recorded below.

The actual production driver now rejects every nonzero remaining DMA count at
timer stop. A regression first failed against the inherited waiver, then passed
against the correction. Valid DMA IRQs arriving before **or after** timer stop
still pass. This repairs false completion reporting; it cannot undo transmitted
bits or guarantee the strip becomes black. Visible blue/white corruption is not
closed.

## Current live execution — 2026-09-16, after Rearm

The first programmer waiter (`colour-integrity-fixed-programme-20260916-01`)
timed out after 180 seconds without seeing a ROM device. Its failed receipt is
retained. The second waiter (`...programme-20260916-02`) saw ROM at 01:24:04 UTC,
identified UID `545433931bd25436593630352d068363` and verified the fixed-image
write at 01:24:07 UTC. The integration agent launched both correctly but ended
the turns without consuming/reporting terminal progress: the operator workflow
failed even though the second programming operation succeeded. Do not confuse
launch acknowledgement with active event monitoring.

The application returned. The prepared runner independently verified the full
UID/build/source/protocol, then ran with exclusive CDC ownership. Evidence is
`colour-integrity-fixed-run-20260916-01/`. During the zero-output phase it
captured a real first fault: frame 3468, fault 6 (DMA), **one word remaining**
at hardware timer stop, DMCNT enabled, no DMA IRQ pending. The immutable payload
is 384 all-zero GRB bytes, 3072 bits, expected DMA length 3070 after preloads;
payload and reconstructed duty hashes verify. It credited 3467 frames from
3467 DMA completions and 3468 stops, with one error: the guard did not fabricate
success for the incomplete frame.

The runner stopped immediately, verified restoration to the declared Mode 32
settings and released CDC. It did not reset or retry. In the retained start-to-
fault interval AP hops advanced 996 and paired slots 1045; both lanes added zero
overflow/drop/recovery/error/packing-mismatch or positive/negative rails. Pipeline
rearm denial, skew drops, starvation and gain-clip deltas were also zero. This
short interrupted control is not a complete microphone soak or an arbitration
qualification. Warm low-load/stress phases did not run.

The fixed-priority fault remained latched until the next physical gate. It was
then superseded by the matched round-robin image. No P601/DIN waveform or optical
capture exists for either event.

The round-robin waiter saw ROM at 03:21:19 UTC, identified the same UID and
verified HEX `a9551442…` at 03:21:21 UTC. After release/reset, the runner
independently verified UID/build/source/protocol and failed in zero-output on
frame 2063. It retained 2062 DMA completions, 2063 hardware stops, 2062 credited
frames and one fault: exactly one of 3070 DMA words remained, DMCNT was enabled,
and no DMA IRQ was pending. The 384-byte all-zero payload and reconstructed duty
hashes verify. Elapsed start-to-stop was 3,840,732 CPU cycles at 1 GHz. The runner
restored the declared settings and released CDC without reset or retry.

In that start-to-fault interval AP hops advanced 114 and paired slots 120. Both
PDM lanes added zero overflow, drop, recovery, callback error, packing mismatch
or raw rails; pipeline rearm denial, skew drops, starvation and gain clipping
also stayed zero. This rejects round-robin arbitration as the cure. It does not
prove that all transfer-service contention is irrelevant: both policies fail
stochastically at the same terminal request, after different numbers of good
frames.

The next controlled candidate retains fixed arbitration, GPT timing, payload,
renderer, PDM configuration and IRQ priority, but gives the LED transmitter the
highest-priority channel: LED DMAC0, programme PDM DMAC1, measurement PDM DMAC2.
Build `a3f37e8a49ac1afbc13448c835f2dd21f07aa20e41c7f696c2a6a11c8f4c9717`
and HEX `aa57fd82ac8ecb6f5fa06296bb584c62535636aecb94784bbbd42595166af9a8`
passed the ARM build and programmer dry run. Two LED-first waiters timed out
without seeing ROM; their failed receipts `...programme-20260916-01/-02` are
retained and wrote nothing. The third waiter saw ROM at 07:08:39 UTC, identified
the same UID and verified the write at 07:08:42 UTC. The application returned
and the runner independently verified UID/build/source/protocol.

LED-first also failed during zero-output: frame 2030 stopped with exactly one
of 3070 DMA words left after 2029 valid completions. The immutable 384-byte
all-zero payload and reconstructed duty hash verify; source advanced 3069 words,
DMCNT stayed enabled, no DMA IRQ was pending, and elapsed start-to-stop was
3,840,728 cycles at 1 GHz. The guard credited no false completion. During the
retained start-to-fault interval AP hops advanced 223 and paired slots 234,
with zero added PDM error/overflow/drop/recovery/packing/rail, rearm-denial,
skew-drop, starvation or gain-clip counters. Settings restoration passed and
CDC was released without reset/retry. Warm phases were not reached.

The linked LED-first ELF has DMAC configuration objects at `0x0203a5d0` (LED,
channel 0/IRQ14), `0x0203a5b0` (PDM rise, channel 1/IRQ74) and `0x0203a598`
(PDM fall, channel 2/IRQ76). The on-target platform JSON misleadingly still
printed PDM channels 0/1: those numbers were hard-coded in `hal_entry.c`, not
read from the active DMAC configuration. That telemetry defect is fixed in
source and a separate build `be3d3b24…` cross-compiles it; **that later build is
not resident or target-tested**. The tested LED-first image really moved all
three linked channel/IRQ owners, but its captured PDM channel fields are not
valid readback. The round-robin and priority-remap hypotheses both failed.
The LED-first image is now resident with its transmitter fault latched. No
electrical waveform or optical capture exists for any of the three runs.

## Route and first product delta

- Task: implement the supplied confirmed completion defect and qualification path.
- Product artefact: production GPT/DMA driver, explicit diagnostic protocol,
  source-bound ARM candidates and executable negative tests.
- Current gate: guard and first-fault witness host/ARM qualified; three
  controlled policies/lane maps failed on silicon with the same terminal
  one-word underrun. The same-frame wire measurement remains outstanding.
- First semantic delta: `DMCRA == 1/2` changed from false success to latched DMA
  fault, with zero credited completions, before any new qualification machinery.
- Progress gate: production-driver runtime assertion RED before patch, GREEN
  after patch. Host mocks do not reproduce actual request arbitration or pulses.
- Skills: global router, thinking router, authoritative-artifact-first,
  scientific method, retained RA8P1/Titan platform context. No new subagents.

## Implementation

`platform/ra8p1/ws281x_gpt_dma_hw.c/.h` retains an immutable first-fault copy of
the submitted packed GRB payload, frame identity, profile, expected words after
the two preloads, FNV-1a payload/duty hashes, initial DMA source pointer, actual
terminal DMA/GPT registers before cleanup, DMA status/global policy, software
completion flags and elapsed DWT cycles since start was requested. Payload copy
and hashes are prepared before transmission, not computed in the ISR. A fault
raised by submission's initial poll cannot be overwritten by another frame.

Opcode 22 with an empty request remains **version 1, 728 bytes**. An explicit
four-byte little-endian version request `2` returns **version 2, 1420 bytes**:
the old 728-byte prefix, three summary words (live DMCTL and last/maximum
preparation cycles), and a 680-byte first-fault witness. The absent witness is
zeroed. `scripts/gpt_fault_witness.py` strictly checks the layout, identity,
publication and both hashes against the actual encoder profile. Packed bytes
are intended transmission data, not a wire capture.

`scripts/build_scalar.py` now supports a globally consistent staged
`DMAC_CFG_PRIORITY_MODE` of fixed or round-robin. Both arms use the same safe
FSP Open correction: apply a differing policy only while global DMA activation
is disabled; refuse an unsafe live change; skip redundant writes when the
existing policy already matches; activate DMA afterwards. The source BSP is
untouched. PDM channels, IRQ priorities, timing, fade and audio settings are not
changed. The priority experiment changes only arbitration, not IRQ scheduling.

Fixed priority places PDM channels 0/1 before LED channel 2. Round-robin is a
controlled hypothesis test and has now failed with the same one-word terminal
underrun. The build tool now also admits an explicit `led-first` lane map:
LED/PDM/PDM channels 0/1/2, with each IRQ owner moved with its channel. The
original `pdm-first` map remains the default. The runner verifies the declared
PDM channel identities before accepting evidence. Local authority:
RA8P1 manual R01UH1064EJ0130 Rev 1.30, section 17.2.22; pinned FSP
`r_dmac.c` Open. The old Open implementation also fails an executable host
assertion for its unsafe write/activation ordering in both priority arms.

## Preparation and current checkpoint

- Actual driver host tests pass, including later-frame identity, caller-buffer
  reuse, before-cleanup snapshots, delayed valid IRQ, late callback and fault
  immutability.
- Strict decoder, actual pinned FSP Open, lane-map and bounded capture tests pass.
  The current focused suite passes 42 tests, including the selected-channel
  status regression.
  Capture error and restoration error injections both release CDC and fail the
  receipt; an unexpected board is never configured. Restoration rechecks identity.
- Fixed ARM build `colour-integrity-fixed-build-20260916-02` passed and was
  programmed for the retained fixed-policy run:
  `d9700b14f939e16ae3d7b5b3e11229199baad0d3125310e82ca8cfa63ce16e7f`.
  Text 228160, data 18528, BSS 236612 bytes.
- Earlier fixed build `...-01` uses superseded 1408-byte diagnostics; retained
  as a draft receipt and **not selected for programming**.
- Round-robin ARM build `colour-integrity-round-robin-build-20260916-01` passed
  and was programmed for the retained round-robin run:
  `49c582dbc5425faa17bfcfb0b2b1f179e17040666372d997bcfcc0f5c1eca3cf`.
  Same text/data/BSS sizes.
- At the original two-build checkpoint, both candidates' 97 source hashes
  matched the then checkout and each other; all eight retained build artefacts
  verified per arm. The subsequent status-telemetry correction changed the
  checkout, so these historical builds must not be treated as current-source
  matches. Staged FSP hashes match. Receipt
  configuration differs only in declared priority after excluding identity,
  timestamps and artefact paths/hashes. Against the frozen baseline, exactly
  four manifest entries change: driver C/header, fixture opcode branch and build
  script. Fade, audio, PDM and renderer sources remain byte-identical.
- Linked ARM disassembly contains the nonzero-count fault branch, v2 diagnostic
  entry and safe DMCTL-before-activation/refusal ordering for both policies.
- Five production-driver mutations are rejected by **runtime assertions**:
  omitted ELC start, omitted first pulse, completion before DMA IRQ, reintroduced
  unfinished-transfer waiver and wrong fault-frame identity.
- Current-source wake replay passes. The unmodified wake wrapper's hard-coded
  20260914-10 historical source comparison fails compilation due to its old
  channel const API; it is not a current-image or colour qualification result.
- Existing platform records retain the safe global-policy finding with exact
  source hashes and a regression. All four bindings (manual, pinned FSP, staging
  script and Open test) match; 17 platform-record tests pass, including packaging.
- LED-first ARM build `colour-integrity-led-first-build-20260916-01` passed and
  was programmed for the retained third failed run:
  `a3f37e8a49ac1afbc13448c835f2dd21f07aa20e41c7f696c2a6a11c8f4c9717`.
  Text 228152, data 18528, BSS 236612 bytes. Its build log carries all six
  channel/IRQ overrides and fixed `DMAC_CFG_PRIORITY_MODE=0` into the real ARM
  compile. The receipt binds the lane map and channel identities.
- LED-first programmer dry run passed. The image lay in the code area and the
  retained recovery image verified. The later source-only telemetry correction
  is separately ARM-built, not programmed.

## Commands, files and retained proof

Firmware sources changed: `platform/ra8p1/ws281x_gpt_dma_hw.c/.h` and the small
opcode-22 branch in `fixture_app.cpp`. Build/host files changed:
`scripts/build_scalar.py`, `gpt_fault_witness.py`, `run_colour_integrity.py`,
`test_ws281x_gpt_dma_mutations.py`; `tests/target_mock/ws281x_hw_mock.h`,
`tests/test_ws281x_gpt_dma_hw.c`; `tests/host/test_gpt_fault_witness.py`,
`test_dmac_priority_stage.py`, `test_colour_integrity.py`. STATUS and the existing
continuation ledger now prioritise this defect. Inherited work is preserved.
The selected-channel status fix is in `platform/ra8p1/hal_entry.c`, with a
regression in `tests/host/test_pdm_capture.py`; it is ARM-built only.
The frozen packet is not edited. No screenshot/render or electrical/optical
capture is claimed: this work uses source, actual driver executions and compiler
logs/maps/disassembly.

Commands run from the firmware root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_ws281x_gpt_dma_hw.py
PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_ws281x_gpt_dma_mutations.py
PYTHONDONTWRITEBYTECODE=1 python3 -m pytest -q tests/host/test_quiet_gap_score.py tests/host/test_quiet_gap_runner.py tests/host/test_colour_integrity.py tests/host/test_gpt_fault_witness.py tests/host/test_dmac_priority_stage.py
SKIP_RESIDENT=1 PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_mode32_wake.py
git diff --check
```

Each ARM build uses `scripts/build_scalar.py --pdm-target --palette-runtime
--palette-autostart --palette-morph --palette-gpt-dma`, the named output path
below, and either `--dmac-priority fixed` or `--dmac-priority round-robin`.
The third candidate additionally uses `--dmac-lane-map led-first`.
Receipts, build logs, symbols, map, disassembly, staged code, ELF and HEX are
retained under
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`:

| Artefact | Build / HEX SHA-256 |
| --- | --- |
| `colour-integrity-fixed-build-20260916-02` | Build `d9700b14f939e16ae3d7b5b3e11229199baad0d3125310e82ca8cfa63ce16e7f`; HEX `1633cb15055d16f66274882654e67b140e805917213ee05879a17a7b558c19dd` |
| `colour-integrity-round-robin-build-20260916-01` | Build `49c582dbc5425faa17bfcfb0b2b1f179e17040666372d997bcfcc0f5c1eca3cf`; HEX `a955144268587f799829e4c8bbd962548673f28cdc340391a7961b4b82b6e21c` |
| `colour-integrity-led-first-build-20260916-01` | Build `a3f37e8a49ac1afbc13448c835f2dd21f07aa20e41c7f696c2a6a11c8f4c9717`; HEX `aa57fd82ac8ecb6f5fa06296bb584c62535636aecb94784bbbd42595166af9a8` |
| `colour-integrity-fixed-programme-dry-run-20260916-01/receipt.json` | PRE-SILICON PASS; fixed HEX, UID-gated programmer and recovery image prepared |
| `colour-integrity-led-first-programme-dry-run-20260916-01/receipt.json` | PRE-SILICON PASS; LED-first HEX, UID-gated programmer and recovery image prepared |
| `colour-integrity-led-first-programme-20260916-03/receipt.json` | ON-SILICON verified write, UID `545433931bd25436593630352d068363`; waiters `-01/-02` expired without writing |
| `colour-integrity-led-first-run-20260916-01/` | ON-SILICON failed at frame 2030; immutable fault witness, restored settings and released CDC retained |
| `colour-integrity-led-first-telemetry-build-20260916-01` | PRE-SILICON PASS, build `be3d3b249a90580723b0e866fa3b543329d804c7d992bb1e67d9504664ec9f63`; correct channel JSON source only, not flashed |

The package regression command is `PYTHONDONTWRITEBYTECODE=1 python3 -m pytest
-q tests/test_platform_memory.py` from the existing `ra8p1-titan-engineering`
package root. Packet verification is `shasum -a 256 -c MANIFEST.sha256` from
the packet directory: all 46 entries pass. Its historical host runner is not
rerun in place, because it overwrites frozen measured output.

## Executed LED-first path — historical, do not rerun

The third waiter and runner were executed with these exact commands. Their
output directories now exist; never reuse them or auto-reset the faulted image.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/scripts/programme_scalar.py --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/colour-integrity-led-first-build-20260916-01 --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/colour-integrity-led-first-programme-20260916-03 --wait-seconds 180 --execute
```

After write verification and Captain's completed release/reset, the
exclusive-owner capture ran. The named frozen settings explicitly restored Mode 32,
palettes 0/1, brightness 255, no automatic cycling, accepted transition/travel;
without that argument the runner restores its captured pre-test settings instead.
Startup defaults are not silently rewritten by this patch.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/scripts/run_colour_integrity.py --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/colour-integrity-led-first-build-20260916-01 --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/colour-integrity-led-first-run-20260916-01 --phase-s 20 --restore-settings /Users/spectrasynq/SpectraSynq-EdgeAI-Lab/docs/Titan_Colour_Integrity_Tests_and_Patch/live/control-original-settings.json
```

The runner saves actual 480-byte native RGB reads (not fabricated black fields),
raw v2 endpoints/first fault, full PDM metrics, identity, settings and restoration.
It uses the same dim moving warm preview as the packet, not a new injection
ontology. Three bounded phases: zero brightness, warm 40 ms read pause, warm
continuous frame/v2 reads. An incomplete transmission terminates the campaign
immediately after retaining the witness. Zero-fault software counts still do
not prove correct pulses. Added preparation and diagnostic cost remains subject
to on-silicon deadline/latency admission; DWT cycles must use CPU frequency,
not the separately resolved GPT clock.

## Remaining numbered closure path

1. Agent: stop the software-priority trial sequence. Fixed, round-robin and
   LED-first all produced the same terminal one-word underrun. Preserve all
   receipts and faulted resident state; do not claim a cure or issue another
   flash merely from a host candidate.
2. Agent: measure GPT requests/terminal pulse counting and decode the **same
   retained submitted frame** at P601 and first-LED DIN: all 3072 GRB24 bits,
   first/last pulses and reset low against the actually populated LED spec.
   The predicted 3072-bit / 3070-DMA reference now exists and a missing capture
   fails closed (`scripts/score_p601_capture.py`). The packet still records no
   Mac-attached logic analyser/scope or established P601 capture connection.
   This physical measurement is the current blocker. Do not stamp
   `WAVEFORM_CAPTURED` from the predicted JSON. Captain Rearm 2026-09-19
   replaced resident `a3f37e8a…` with profiler `32dd1f5f…` before capture;
   the colour-integrity retained frame is no longer on this silicon.
3. Agent: repair the first measured divergence (timer/DMA, signal path or
   supply/ground/LED hardware), then repeat the matched zero-output, warm
   low-load and USB-stress controls with PDM guardrails and same-frame wire
   evidence. Restore settings and release CDC on every run.
4. Agent: retain applicable optical evidence and Captain's product ruling;
   only then close the visible-corruption ticket and resume onwards development.
