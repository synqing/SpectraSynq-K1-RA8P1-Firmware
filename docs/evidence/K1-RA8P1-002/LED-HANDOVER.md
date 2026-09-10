---
abstract: "2026-09-10 Titan WS2816 dual-DIN handover. Smoke 06 completed on the identified led-build-04 image: both packed-buffer CRCs match, eight pixels per DIN and six blink cycles reported. Emission only; photons and GPIO pulse widths unverified. Do not rerun into the existing smoke-06 directory or ask Captain to inspect the stick."
---

# Handover — Titan WS2816 dual-DIN (2026-09-10)

Next agent: execute. Do not re-ask GO. Do not ask Captain to inspect LEDs. Do not invent a second worktree. Do not treat CRC as photons.

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
