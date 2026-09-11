---
abstract: "2026-09-11: Captain selects the later midnight WS2812 showcase as the working reference. Restoration of appearance is NOT established. Archived midnight-showcase and current logical frames match in a focused HOST comparison, but output transport, gain, mapping and cadence differ. See PALETTE-RECALL-2026-09-11.md."
---

# Handover — Titan WS2816 dual-DIN (2026-09-10)

Next agent: execute. Do not re-ask GO. Do not ask Captain to inspect LEDs. Do not invent a second worktree. Do not treat CRC as photons.

## Latest execution — boot fix flashed; full-cycle submission capture passed

`ws2816-showcase-prog-03` identified the Titan UID and verified all 217,408 bytes.
Build `70c2323a9e85611ad969096b45d73dfb9a1dda8a7e108abc1a2f89233c489500`
is now running. Normal RESET booted directly into showcase/catalogue cycling;
no configuration command was needed. `ws2816-showcase-wire-02/receipt.json`
passed 52 actual-submission snapshots: all four effects, 15 palette selections,
all 160 positions, zero packing/scaling or mirror mismatches and zero emitter
errors. Native output remains active; CDC is released; no listener is armed.
This closes boot-default and submission-buffer checks, not calibrated optics.

### Earlier restoration and preparation

Fresh STATUS found the old image back in single-palette mode 0, not the selected
showcase. Restored modes 100/101 plus catalogue/showcase cycling on the identified
Titan; later STATUS confirmed progression to modes 102/103. No timing, pins or
gain change. Candidate `ws2816-showcase-build-02` removes that boot override and
adds a post-gain, actual-two-emitter-buffer snapshot. Host/build checks passed;
programming and target-capture progress is recorded in
[SHOWCASE-RESTORE-2026-09-11.md](SHOWCASE-RESTORE-2026-09-11.md).

`ws2816-showcase-prog-02` timed out without any flash write. The successful
`prog-03` retry above supersedes that blocked state; preserve both receipts.

## Latest — visual restoration rejected; recall and comparison

Captain subsequently clarified: "I believe it would be the later version".
Use `centre-effects-build-01` / `centre-effects-live-01`, the midnight WS2812
showcase, as the working reference. Do not repeat demo-selection archaeology.

Captain: "This look NOTHING like the ws2812 version. NOT EVEN CLOSE."
The preceding "restored" claim was not supported as visual equivalence and is
withdrawn. Read [PALETTE-RECALL-2026-09-11.md](PALETTE-RECALL-2026-09-11.md)
before another demo change. It records both historical candidates, the missing
session evidence and a byte comparison of the archived midnight renderer against
the current renderer. No LED configuration or firmware was changed during that
investigation. At that earlier point reset still booted the single-palette
default; the latest execution above fixes that behaviour.

## Earlier — showcase settings applied, not optical equivalence

Captain rejected the single-palette preview and requested the palettes used on
WS2812 earlier. Reused `centre-effects-live-01/receipt.json`'s final selection:
palette bases 33/43, effect bases 100/101, automatic catalogue cycle and effect
showcase enabled, 1,500 ms palette fades, centre-out, four-second travel.
Kept the WS2816 gain at 128/255 (historical WS2812 gain was 24/255); no optical
brightness equivalence is assumed. Kept the current profile-4 transport, two
80-pixel DINs and 16,667 us nominal schedule. No flash or firmware edit.

The existing runner verified UID and build before configuration. Receipts:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-palette-showcase-01/receipt.json`
and `live-check.txt` in that directory. Three live status samples over a
13-second host wait confirmed both palette and effect changes, advancing
emissions, 160 pixels, showcase/cycle enabled and zero emission errors.
The observer closed CDC; native output remains active. No new optical claim.

Restore after reset with:

```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/run_titan_palettes.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-palette-build-01 \
  --showcase --cycle --palette 33 --palette-b 43 --brightness 128 \
  --transition-ms 1500 --travel-ms 4000
```

Current flash autostart still selects single Iris Apricot. This command changes
runtime state, not persistent boot defaults. Chromatic Score is a separate K1
system, not the name of this showcase.

## 11 September — motion confirmed; native palettes requested

After physical RESET and `ws2816-after-reset-motion-01`, Captain reported:
"Okay I see that stupid red blue comet trails... okay stop playing that shit,
give me the colour palettes". This supersedes the earlier lack of optical
confirmation for motion, without identifying a sole cause of intermittent darkness.
Black was sent on both DINs with `ws2816-stop-comet-01-p601` and
`ws2816-stop-comet-01-p004`; the serial port was released.

Native palette build `ws2816-palette-build-01`, identity
`11b95cdffd55f1a53a8fb9193a6aaa79a7ff023ce6737b9e0be60122b3366922`,
contains the existing 44 K1 palettes, with all 160 native pixels mapped directly
to two 80-pixel GRB48 lanes. Existing Pixel8 colours are expanded and brightness
scaled once; this is not a new 16-bit-precision renderer. Profile 4 uses the
same experimental transport as the visible comets. Nominal scheduling is 60 Hz
for the sequential diagnostic emitter, not a production audio coexistence claim.
Autostart selects Iris Apricot (ID 33), brightness 128/255, centre-origin preview;
no catalogue cycling or comet showcase. Host tests passed all 44 palettes,
both output channels, brightness 0/128/255, full lane extents, invalid lane/buffer
rejection, WS2812 compatibility, and native emission after CDC disconnect.
Programming `ws2816-palette-prog-01` identified the expected UID and passed
readback of 217,120 bytes. HEX SHA-256:
`cb908c6f840ab84e4499c45cc92eae89a676c8cb5f313a675a39896298df956b`.
After normal RESET, INFO returned the new build ID and correct UID. Autonomous
palette status advanced from 443 to 570 emitted frames across two reads with a
two-second host pause, changed both native frame CRCs, and retained zero emission
errors and zero skipped releases in that sample. This is not a measured frame-rate
benchmark or optical palette acceptance. The same selected channel's native
pixels 0–79 feed DIN-A and 80–159 feed DIN-B; palette B is a separate logical
renderer, not the palette independently assigned to physical DIN-B.

Live configuration `ws2816-palette-live-01/receipt.json` leaves Iris Apricot
running at brightness 128, with 1,500 ms transitions enabled for future palette
changes. The serial port is released; animation requires no host pixel stream.
`ws2816-palette-live-01/autostart-check.json` retains the startup check.
`ws2816-palette-build-01/host-checks.txt` retains host results.

To select a different canonical palette, use the existing command with a fresh
output directory (not an old receipt path):

```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/run_titan_palettes.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-palette-build-01 \
  --palette 33 --brightness 128 --transition-ms 1500
```

`--list` returns all 44 names, `--status` opens status, and `--stop` sends black
then disables native output. Do not use the older WS2812-only console's frame
commands for this WS2816 stick. No BSP or DualMCU import-slice changes were made
for the palette adaptation; G4 remains unchanged.

## 11 September — WS2816 first light confirmed on both DINs

Captain reported: "Okay LEDS 1-8 are on and 81-88" after the full-channel
red, green and blue input test. Each colour used value 65535 on one channel,
eight pixels per DIN and four-second holds; the final command was blue.
This is direct optical evidence for both halves of the connected WS2816 stick.

- UID: `545433931bd25436593630352d068363`.
- Build: `7257a335462725beb7de909ee1760452be26b7f54f096e04c050dafa7ecf54f2`.
- Profile 4: GRB48, nominal 250/875/1250 ns, reset 300 us.
- P601/U18-7: LEDs 1-8; P004/U18-16: LEDs 81-88.
- Receipts: `ws2816-input-check-01-{red,green,blue}-{p601,p004}/receipt.json`
  under `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`.
- Both final blue replies returned `pfs_after=3076`, configuration error 0,
  and packed-buffer CRC 2561809235. These accompany Captain's observation;
  they are not measured GPIO waveforms.

Earlier dark runs used different patterns and lower values. First light does
not isolate brightness, timing, or connection changes as the sole cause.
No reflash occurred between the last dark motion run and this successful
full-channel input test. Use the demonstrated brightness for the next motion
test. The earlier failure history below remains valid for those runs.

On Captain's subsequent "Get it", `ws2816-full-bright-motion-01` completed
three centre-to-edge-and-back cycles at brightness 65535: 477 motion steps,
958 wire frames including initial black and final edge frames. The final
red/blue edge comets remain commanded on, and CDC was released. The receipt
passed its command checks; Captain's optical confirmation of this full motion
run is not yet recorded.

## Earlier 11 September — WS2816 optical failure and FastLED timing experiment

The live Titan was identified as UID `545433931bd25436593630352d068363`, build
`3decd1541b7a5fb1b77b65343542a6562d2286be69c3871713fca1d1b762bdb6`.
That image contains opcode 14 and the WS2816C profile. Its autonomous WS2812
palette output was stopped successfully; receipt:
`EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-stop-palette-01/receipt.json`.

`scripts/run_ws2816_centre_out.py` sent 318 full-stick motion steps / 640 wire
frames, but Captain observed **no WS2816 light**. The command-path receipt is
`EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-centre-out-live-01/receipt.json`.
That is a physical bring-up failure, not a pass.

An alternative transport profile was added: the earlier run used the
WS2816C-1313-specific 650 ns one-high profile, while FastLED's actual WS2816
controller expands to GRB48 and forwards those bytes through
`TIMING_WS2812_800KHZ` (250 ns zero-high, 875 ns one-high, 1,250 ns period).
Profile 4 now represents that FastLED-compatible 48-bit transport; profile 3
remains the distinct C-1313 datasheet profile until the populated LED is bound.
This difference is a testable hypothesis, not an established cause of darkness.
The 875 ns profile is outside the retained C-1313 one-high bound; FastLED's use
alone does not establish applicability to the fitted LED or measured pin timing.

The runner sends exact
GRB48 frames to both 80-pixel inputs: red travels from logical pixel 80 to 1 on
P601, blue travels from logical pixel 81 to 160 on P004, then both return to the
centre. It exercises every logical position, retains target reply/CRC/timing
markers, and sends black to both lanes on normal completion. The command path
does not claim photons or measured GPIO pulse widths.

Host contract `scripts/test_ws2816_centre_out.py` passes centre 79/80, endpoints
0/159, low-byte-preserving TRUE16 packing, both pin identities and negative
reply mutations. Existing WS281x packing/runner and fixture protocol tests also
pass. Dedicated experimental image
`ws2816-fastled-build-01` has build ID `7257a335462725beb7de909ee1760452be26b7f54f096e04c050dafa7ecf54f2`
and HEX SHA-256 `330649d6a955f3894aad00bf7595816aad8dd00330f46cf22c99f6a1c5338548`.
Two earlier ROM waiters expired. The subsequent `ws2816-fastled-prog-03` run
identified the expected UID, programmed this image and passed readback.
After normal RESET, live INFO confirmed build `7257a335…`.
`ws2816-fastled-live-01` then completed 318 motion steps / 640 wire frames using
profile 4. The run used `--leave-lit`: its final submitted pattern contains red
and blue edge comets with fading tails. CDC was closed on completion.
The retry `ws2816-fastled-live-02` could not open because application CDC was
absent. A listener subsequently identified the expected UID/build after USB
returned and executed `ws2816-fastled-live-03`: 318 steps / 640 wire frames,
with the final edge pattern submitted and CDC closed.
Captain then reported: "I DO NOT SEE ANYTHING ON THE STRIP NOW".
**WS2816 optical bring-up remains FAIL.** The timing change has not established
a fix; neither a defective stick nor a particular waveform defect is proved.
Keep the original command receipts intact: their `pass` fields apply only to
the software checks. The runner checks PFS and configuration errors but does
not retain those fields per frame, and there is no measured signal at the
shifter input/output or first LED DIN. USB reconnection explains the failed
start of live-02, not the darkness after live-03.
Next discriminator: measure the physical power/data path and decode the signal
at the first LED input, binding observations to the actual fitted part. No
oscilloscope/logic-analyser integration was found in the available tools or
project scripts; the USB product listing showed none identifiable as such.
Receipts are under
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`.

```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python \
  scripts/run_ws2816_centre_out.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-fastled-build-01 \
  --profile fastled \
  --leave-lit \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2816-fastled-live-01
```

The command above records the completed run; use a fresh output directory for
any authorised repeat. No further flash is required to use profile 4.

## Interactive Titan LED console

`scripts/titan_led_console.py` implements the proven K1 interaction shape for the
Titan binary fixture protocol: immediate safe hotkeys, `:` plus Enter for typed
commands, `h` help, `;` status, exclusive CDC ownership, live UID/build binding,
CRC-checked replies and terminal restoration. It uses opcode 14 raw frames; it
does not weaken the device parser by treating arbitrary bytes as commands.

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/titan_led_console.py
```

Immediate keys: `c` centre-out, `r/g/b/w` solid colour, `[`/`]` brightness,
`p` pause, `0` or Space off, `;` status, `h` help and `q` quit. Typed commands
include `:brightness 1..128`, `:fps 1..60`, `:solid red`, `:centre`, `:off`,
`:status`, `:help` and `:quit`. Clean quit sends an all-off frame before releasing
CDC. There are no flash, erase, reset or calibration commands.

Host contract: `scripts/test_titan_led_console.py`. The first live console launch
attempt did not open a port because no `045b:5310` Titan CDC was enumerated; it
restored the terminal and retained a failure receipt at `titan-led-console-live-01`.
The previously completed opcode-14 centre-out run proves the same current build's
frame route, but is not a successful interactive-console session.

## Latest result — arbitrary 128-pixel motion executed (10 September 2026)

Identified build `5a85ab314d7e0e09dfc194c59e2407d8969420104afdf1c30f3d589c6f547a46`
was flashed with `PROGRAMME_VERIFY_PASS`. New opcode 14 accepts exact raw per-pixel
WS281x frames rather than the older uniform-prefix request. The live runner sent
381 WS2812 GRB frames: two red comets travelled from centre pixels 64/65 to
endpoints 1/128 and back for three cycles.

[Motion receipt](/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2812-centre-out-live-02/receipt.json)

- Completed frames: 381; radii covered: 0–63.
- Frame payload: 128 pixels / 384 bytes; arbitrary per-pixel GRB.
- Median host transaction: 31.94 ms; maximum 132.70 ms. This is a CDC-driven diagnostic, not a production realtime render-loop result.
- Target emit markers: 3,872,387–3,872,411 cycles per frame at reported 1 GHz.
- Host checks passed for centre origin, full extent, malformed frame length, protocol overflow, CRC, reset, reconnect and transport chunking.
- The initial motion image was correctly rejected because the old transport allowed only 360-byte requests. The corrected transport permits the required 400-byte WS2812 request and the maximum 784-byte WS2816 request; 785 bytes is rejected.

This establishes repeatable changing frame submission and addressing of both ends
of the 128-pixel WS2812 model at the command/readback layer. Optical motion is not
inferred from the receipt; record Captain's observation separately if supplied.
It is not yet the on-device FastLED-compatible production render path.

## Latest result — WS2812 first light achieved (10 September 2026)

**ON-SILICON optical result for the connected WS2812 setup:** Captain connected a
known-working 128-pixel WS2812 strip with DIN on P601/U18 pin 7, VSYS_5V power and
common ground. On the identified Titan UID `545433931bd25436593630352d068363`,
opcode 13 completed and Captain reported that LEDs 1–8 were visibly lit bright red.

[Instrumented command receipt](/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/ws2812-p601-live-01/receipt.json)

- Runtime build: `0d379882eb8c41b10d30295728ceee288c5d4436a12b7c1efd75cee0eb727659`.
- Flashed HEX SHA-256: `960e12993d8580e5443fe78b9a325cca0d7038de5832ac97d8d53c3462bf97a8`.
- Request: WS2812 profile, P601, 80 transmitted pixels, eight at red 255 and the rest off.
- Target readback: P601 GPIO output, 240 bytes, CRC `3629216573`, result 0.
- Software markers: bit periods 1,261–1,263 cycles at reported 1 GHz and reset-low 300,011 cycles.
- Optical evidence source: Captain's direct observation. The receipt itself proves command execution and packed-buffer emission markers, not photons.

This proves the chosen header pin, strip input direction, shared power/ground and
basic WS2812 signalling for this exact connected setup. It does not yet prove the
separate WS2816 stick, both DIN lanes, GPIO waveform margins or production render
integration. A host-tested 128-pixel diagnostic image exists at
`/Users/spectrasynq/Workspace_Management/Edts/Titan/k1-ra8p1-002/ws281x-build-02`;
it is not flashed as of this entry.

## Latest result — smoke 06 executed (10 September 2026)

**The serial blocker is cleared and opcode 11 completed.** Live INFO matched UID
`545433931bd25436593630352d068363` and build `f0205ef8…` before the blink command.
The existing image was used: no firmware edit, reflash, process kill or import change.
The runner exited **0** and released CDC. Existing unrelated dirty work is preserved.

[Original receipt](/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-smoke-06/receipt.json)
and [execution, identities and limitations](/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-smoke-06/EXECUTION.md).

- Both bright packed-buffer CRCs match: A `2700312459`, B `997782662`.
- Eight nonzero pixels per DIN and six bright/dark cycles reported; 80 pixels / 480 bytes per lane.
- Last bright emit **5.112104 ms**, latch **0.300011 ms**; IRQ-disabled measured span **5.412115 ms** plus unmeasured entry/exit overhead.
- Software loop-start periods **1,302–1,466 ns** at the reported 1 GHz versus configured 1,250 ns. These are not GPIO pulse-width measurements; T0H/T1H and PFS remain unverified.
- **Emission smoke passed; photons NOT_CLAIMED.** CRCs are calculated from packed buffers, not captured pins or LED reception. No current optical observation is inferred.

This supersedes the historical "blink smoke never ran" and "immediate next step"
sections below. Keep their history; do not rerun the command into the already-created
`led-smoke-06` directory. It also corrects the older ship-path wording: matching CRC
and software timing do **not** establish visible first light.

Remaining numbered path:

1. Obtain instrumented pin/timing evidence; if an optical failure is established,
   investigate timing/pinmux and the 80/81 data break, not the known HCT shifter.
2. Prove DualMCU treatment/gain/limit/pack/latch parity without adding the packer to import-slices.
3. Resolve and revalidate G4 under existing authority: **2,005/6,000 = 33.4% deadline misses remains FAIL**. No LED-output or product pass is claimed.

## What is true now

The **blink image is on the board**. Captain saw **no light** from the earlier smoke because that image lit **two dim pixels, once**. The strip **has a 74HCT2G34GW** on the data lines — 3.3 V into HCT is legal. Do **not** reopen “missing shifter.”

| Item | Value |
| --- | --- |
| Implementation repo | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware` |
| Branch | `lane/k1-ra8p1-002` (dirty, **uncommitted**) |
| EdgeAI | `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab` `lane/k1-ra8p1-002` |
| Board | Titan Mini HW V1.0, UID `545433931bd25436593630352d068363` |
| Stick | One 160 WS2816, 80/80 split, **74HCT2G34GW on DIN** |
| DIN-A | P601 / U18 pin 7 / leds 1–80 |
| DIN-B | P004 / U18 pin 16 / leds 81–160 |
| 5 V / GND | U18 2/4 `VSYS_5V`, pin 6 GND |
| Live USB (last seen) | App CDC `045b:5310` `/dev/cu.usbmodem00000000000011` loc `0-1.3` |
| Current image | `led-build-04` flashed via `led-prog-03` `PROGRAMME_VERIFY_PASS` |
| Build ID | `f0205ef836e6be37f0a700d7da9c8c8a86b4243a0986666d4435085a87da56be` |
| HEX SHA-256 | `ff89a3820bc266caf8e3f818554ebe3d0913f821b326aeace5874b05b87fd7d4` |
| Interpreter | EdgeAI `.venv` Python 3.12.11 |

## What already passed

1. HOST: `scripts/test_ws2816_pack.py`, `scripts/test_ws2816_emit_protocol.py`, `scripts/test_fixture_protocol.py`.
2. TRUE16 packer: pixel 0 is `0x12AB` (low byte `0xAB`, **not** `×257`). Mutation that expects `0x12` in the low slot goes red.
3. `led-prog-02` flashed `led-build-03` (dim one-shot). `led-smoke-02` **CRC matched** both packed lanes. Emit 5.362 ms, latch 300 µs, bit-period **1352–1500** cycles vs 1250 ns target (function-pointer overhead). Captain: **no visible LEDs**.
4. `led-prog-03` flashed `led-build-04` (direct PCNTR3 bit-bang + 8 visible TRUE16 pixels/DIN + 6 on/off blinks). **Blink smoke never ran** — Cursor helper PID 20487 held `/dev/tty.usbmodem00000000000011` (`EBUSY`). Failed dirs: `led-smoke-01/03/04/05`.

## What the flashed `led-build-04` does on opcode 11

- Lane A leds 1–8: pixel 0 red `0x12AB`, pixels 1–7 red `0x7A3C`.
- Lane B leds 81–88: pixel 0 blue `0x12AB`, pixels 1–7 blue `0x7A3C`.
- Six bright/dark cycles, ~83 ms gaps (~1 s total). USB reply after that.
- JSON: `true16=0x12AB`, `visible_u16=0x7A3C`, `level_shifter=74HCT2G34GW`.
- CRC is of the **bright** packed buffers. Host expected lanes: `scripts/run_led_smoke.py` `expected_lanes()`.

## Programming law (the only USB-ROM path)

**Arm the waiter first. Then Captain holds USER/BOOT, taps RESET, keeps holding until `PROGRAMME_VERIFY_PASS`.** Then he releases and taps RESET normally.

```sh
# already done for led-build-04 — do not reflash unless the HEX changed
python scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-build-04 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-prog-NN \
  --wait-seconds 180 --execute
```

ROM boot = `045b:0261` `RA USB Boot`. App = `045b:5310` `CDC USB Demonstration`.  
Do **not** ask for buttons before `WAITING_FOR_IDENTIFIED_ROM` is printing.

## Immediate next step (no reflash)

Own the CDC. Cursor serial-monitor on that usbmodem is a competing writer. Close it or wait until `lsof` is empty. Then:

```sh
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware
# confirm INFO build id == f0205ef8… before claiming this image
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/run_led_smoke.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-build-04 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/led-smoke-06
```

Opcode 11 holds IRQs ~5 ms per emit and runs ~1 s of blinks — read timeout must stay ≥15 s. If `exclusive=True` hits `EBUSY`, retry; do not multiplex Serial Studio. Add a retry loop in `run_led_smoke.py` if Cursor keeps grabbing the port after reset.

INFO must show `build=f0205ef836e6be37f0a700d7da9c8c8a86b4243a0986666d4435085a87da56be` and UID `545433931bd25436593630352d068363`.

## If still dark after blink smoke CRC match

Suspects **in order** (shifter is **not** on this list):

1. Bit timing still fat (measure `bit_period_min/max`; T0H must stay 200–320 ns). Direct PCNTR3 in `ws2816_gpio_emit.c` was the fix for the 1352 ns first emit; **unproven on silicon**.
2. P601 still muxed GPT1 — PinCfg at runtime; confirm PFS if still dark.
3. 80 DO still joined to 81 DI — both DINs fight.
4. Power/GND: must be `VSYS_5V` + common GND, not 3.3 V VDD.
5. Wrong header pins.

Do **not** ask Captain to validate occupancy. Score the JSON. Photons-not-seen after a 1 s 16-pixel blink is a real fail; then fix timing/pinmux, **arm wait**, reflash.

## Files (uncommitted)

| Path | Role |
| --- | --- |
| `src/k1/core/visual/ws2816_pack.h` | TRUE16 packer; **not** in DualMCU import-slices |
| `platform/ra8p1/ws2816_gpio_emit.{h,c}` | Packed-lane port; target bit-bang is **direct PCNTR3**, HOST tests still use the header callback sink |
| `platform/ra8p1/titan_led_pins.h` | P601=0x0601, P004=0x0004 |
| `platform/ra8p1/fixture_app.cpp` | Opcode **11** |
| `scripts/build_scalar.py` | `RA8P1_LOCAL_K1_FILES` + emit `.c` |
| `scripts/run_led_smoke.py` | Host CRC of packed lanes |
| `docs/evidence/K1-RA8P1-002/OUTPUT-PATH-COMPOSITION.md` | DualMCU post-renderer gaps; blocks “VP done” |

Do not add packer to `import-slices.json` / `IMPORT-MANIFEST.tsv`. Do not edit Titan BSP `pin_data.c`. Do not import FastLED. Do not put emit on hop 2/5 or G4. Do not waive G4 (2,005/6,000 = **33.4%** miss, tempo-hop fraction 33.3%).

## Ship path

1. **Already:** F1 renderer 320 Pixel8 on this UID; `led-build-04` programmed and read back; dim-image CRC smoke (`led-smoke-02`); HCT shifter on the stick.
2. **Next:** exclusive CDC → `run_led_smoke.py` on `led-build-04` → receipt with matching CRC **and** bit-period nearer 1250 ns. If still dark, timing/pinmux/data-break, then wait-armed reflash.
3. **Agent** runs smoke. **Captain** only the wait-armed boot hold for a **new** image.
4. **Stamps:** blink-smoke CRC + sane bit-period = wire first light people can actually see. `K1_RA8P1_LED_OUTPUT=PASS` still needs DualMCU treatment/gain/limit/pack/latch compared. Not K1 ship. Not VP done.

## Interpreter / bans

- Python: `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python`
- No sibling worktree. No cadence runner. No FastLED. No production `SpectraSynq_K1_Firmware`. No MERT/MuQ on Titan.
- Restore image if you brick: `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex` SHA `a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a`

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:grok | Handover after blink image flash; smoke blocked on Cursor CDC. |
| 2026-09-10 | agent:codex | Executed smoke 06 on identified build; CRCs match; recorded software timing and optical limits, released CDC, retained G4 failure and historical handover. |
