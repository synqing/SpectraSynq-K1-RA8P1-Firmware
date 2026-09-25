# WS2816 dual-lane WP-A resource map

Date: 2026-09-21  
Candidate: `WS2816-PAIR-CANDIDATE-20260921-A`  
Authority: this checkout at `431140853dd8b58af53240ef84d5fb1a08bd8b45` plus dirty pair-implementation work. Resident image receipts named below remain the recovery show.

## What is true now

Titan is still running the 21 September live-K1 image. That image drives one GPT/DMA lane on P601 with a 24-bit WS2812 profile. It is not a WS2816 dual-data backend.

The stick contract in this repository remains one 160-pixel logical channel on two independent 80-pixel data lanes. P603 is a documented GPT candidate for the second lane. It is not an approved wire route until the operator confirms the physical move from P004.

## Bound live image

Do not substitute an earlier HEX.

| Field | Value |
| --- | --- |
| Build directory | `docs/evidence/K1-RA8P1-002/live-k1-runtime-build-20260921-01/` |
| `build_id` | `90e1b84f5665008fa262d22bd9882deb5f11981542b7b48fb30edba61f8578bd` |
| HEX | `cbcadb41de10cb7f884edb38118d8ad58967f2edfe7c5afcd1eda5e62828dd37` |
| ELF | `646ea527901c50b9f673c1ef42b075a3584d8741f2f0342e6960e863920961d3` |
| Programme | `programme-20260921-02-palette-preview` WRITE_VERIFIED |
| UID | `545433931bd25436593630352d068363` |
| Programmer | `8e6f5ccf78ab6cff2af359c12be0320529d8cfaf1dae126074860448f50acf59` |
| Image window | `0x2000000`–`0x2040f77` |
| USB at programme | `/dev/cu.usbmodem1401`, VID 1115 PID 609, bootloader identity `RA USB Boot` |
| Branch | `lane/k1-ra8p1-002` |
| Working tree | dirty: live-runtime, control-surface, palette, ASRC, programmer handoff. Preserve it. |

Programme-20260921-01 of the same HEX failed. Programme-02 passed. There is no fresh INFO capture in the programme-02 directory; identity is the build receipt plus the programme receipt.

Rollback HEX recorded by that programme is `EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex`. That is the previous restore asset, not the current working show. Restoring the current show means re-programming the 21 September HEX above.

## What the installed image actually owns

From `live-k1-runtime-build-20260921-01/receipt.json`:

| Flag | Installed |
| --- | --- |
| `palette_gpt_dma` | true |
| `palette_ws2816` | false |
| `palette_runtime` | true |
| `palette_autostart` | true |
| `live_emit` | true |
| `pdm_target` | true |
| `dmac_lane_map` | **led-first** |
| `dmac_priority` | fixed |
| `dcache` | disabled |
| `gpt_checkpoint` | `PRE_SILICON_ONLY` — timer clock is still `R_GPT_InfoGet`, never the 1 GHz CPU identity |
| `product_output_chain` | true |

`--palette-gpt-dma` forbids `--palette-ws2816`. The live path therefore uses the single-lane GPT transmitter and `packBenchGrb` (24-bit), not the P601/P004 GPIO WS2816 adapter.

led-first remaps DMA as:

| Channel | Owner on this image |
| --- | --- |
| DMAC0 | LED GPT duty → GPT6 GTCCRC |
| DMAC1 | PDM rise |
| DMAC2 | PDM fall |
| DMAC3–7 | unused in `platform/ra8p1` sources; still must be claimed at init, not assumed from silence |

This is not the historical “PDM 0/1, LED 2” story. `ws281x_gpt_dma_hw.c` now records that the default channel is 2 and that led-first remaps LED to DMAC0. PDM fall still needs the DMAC2 vector. The pair image claims DMAC3 for A1 and must not seize DMAC2.

Current GPT/ELC/pin ownership of the working backend:

| Resource | Owner |
| --- | --- |
| P601 / U18 pin 7 | GTIOC6A, LED lane now (logical A0) |
| P004 / U18 pin 16 | recorded DIN-B, GPIO only, **no GPT** |
| GPT6 | saw-wave PWM, DMA into GTCCRC |
| GPT0 | event-count stop of GPT6 after exact bit count |
| ELC GPT A | GPT6 compare-A → GPT0 count-up |
| ELC GPT B | GPT0 overflow → GPT6 hardware stop |
| DMAC0 IRQ | LED transfer complete (not pair success) |
| GPT0 overflow IRQ | hardware stop (not reset complete, not light) |
| Profile submitted | `1` (FastLED WS2812 250/875/1250 ns, 3 bytes/pixel) |

GPIO lockstep `ws2816_gpio_emit.c` stays a diagnostic. It masks interrupts across the whole frame and must not enter the live music path.

## Named candidate `WS2816-PAIR-CANDIDATE-20260921-A`

One logical channel `channel_A`. Two physical lanes `lane_A0` and `lane_A1`. Do not call the halves logical A and B.

| Lane | Logical pixels | Bytes | Centre wire index | Pin | GPT PWM | Stop GPT | DMA | ELC |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| A0 | 0–79 | 480 | 79 | P601 keep | GPT6 / GTIOC6A | GPT0 (existing) | DMAC0 keep (led-first) | A/B keep |
| A1 | 80–159 | 480 | 0 | **P603 candidate** | GPT7 / GTIOC7A | **GPT1 candidate** | **DMAC3 candidate** | **C/D candidate** |

Profile: **3** (WS2816C-1313 T0H 250 ns, T1H 650 ns, period 1250 ns, reset 300 µs, 6 bytes/pixel). Do not blend with profile 4 (GRB48 on the FastLED 875 ns one-high). Historical first light on profile 4 is not datasheet qualification.

Duty storage already sized for one 80-pixel WS2816 lane: `K1_WS281X_GPT_DUTY_CAP = 3840` words = 15 360 bytes. Pair needs a second independent buffer. Active pair = 30 720 bytes of duty, DMA-reachable, 32-byte aligned, same cache policy as now (DCache disabled on this image). No heap. No unverified TCM move.

3840 bits with the existing two-word preload implies 3838 subsequent DMA values. Prove it on the pair implementation; do not copy the 3072-bit WS2812 count.

Mutual exclusion:

- Candidate WS2816 pair and the installed WS2812 P601 transmitter must not both own GPT6/P601.
- `--palette-ws2816` GPIO path must stay off while GPT pair is selected.
- Keep the 21 September image as the recovery show.

P603 SCI TXD0_B/MOSI0_B is a pin capability. This firmware tree does not open SCI0. That is not a live PFS readback. Claim GPT7 on P603 only after the operator confirms the physical move and init reads the PFS.

P004 cannot become GPT by a flag. If DIN-B must stay on P004, the alternate is a hardware-paced GPIO/DMA transmitter with its own timing proof. Not a millisecond interrupt-masked bit-bang.

## Red team (what this plan would get wrong)

1. Treating historical DMA 0/1/2 as today’s map, then colliding with PDM fall on DMAC2.
2. Stopping both lanes on the first GPT0 overflow, truncating a one-bit on the slower last pulse.
3. Crediting pair success from DMA-complete.
4. Enabling `--palette-ws2816` and believing that is the GPT pair.
5. Packing RGB8 × 257 (`packBenchGrb48Lane`) and calling it TRUE16.
6. Moving DIN-B to P603 in firmware before the wire move, or driving two outputs into one net.
7. Using Titan USB as the stick supply.
8. Another unmeasured DMA-priority rerun after the 16 September underruns (one of 3070 words remaining on both fixed and round-robin, and on the LED-first isolation image). Dual 3840-bit lanes increase DMA occupancy; the next discriminator is a same-frame wire capture, not another policy tweak.
9. Mixing this work into DualMCU Lever-2 or the Titan BSP tree.
10. Flashing over the working show without the exclusive CDC handoff already in `programme_scalar.py`.

## Second-order

Serialising two 80-pixel lanes misses the 8.33 ms / 120 fps budget. Concurrent lanes fit the wire (about 5.1 ms with reset). That still does not prove AP coexistence. A 5 ms interrupt blackout would starve 24 kHz / 180-sample capture.

Success of the pair driver will tempt a second 160-pixel channel. Four concurrent lanes still occupy about 5.1 ms. Two serialised pairs miss 120 fps. Inventory GPT/DMA/ELC/power before that extension. Two inputs on one split stick are not independent physical A/B.

## Operator gate (stops implementation from going live)

Confirm one of:

1. **P603 move** — physically relocate DIN-B from P004 / U18 pin 16 to P603 / U18 pin 33, through the existing 74HCT2G34GW, with the LED 80/81 data break verified power-off; or
2. **Keep P004** — then WP-B becomes the GPIO/DMA alternate, not GPT7.

No silent pin relocation. No Rearm until that confirmation and a named image are prepared.

Electrical checks before any new route is powered: independent data break, polarity and voltage at the stick under load, HCT supply and common ground, sparse bounded colour, declared current limit.

## Closed allocation (source and staging)

Inspected against FSP `bsp_elc.h` / `r_gpt.h`, `vector_data`, and the pair driver. Comments are not ownership.

| Owner | Waveform / pin | DMA | Hardware stop | Additional allocation |
| --- | --- | --- | --- | --- |
| A0 | GPT6 / P601 / U18 pin 7 / GTIOC6A | DMAC0 (`DMAC0_INT_IRQn` = 14) | GPT0 overflow IRQ 63 | ELC GPT_A = 0, event `ELC_EVENT_GPT6_CAPTURE_COMPARE_A` = 0x1B7; ELC GPT_B = 1, event `ELC_EVENT_GPT0_COUNTER_OVERFLOW` = 0x187; stop source `GPT_SOURCE_GPT_B` (1<<17); count-up `GPT_SOURCE_GPT_A` (1<<16); clock from `R_GPT_InfoGet` on GPT6 |
| A1 | GPT7 / P603 / U18 pin 33 / GTIOC7A | DMAC3 (`DMAC3_INT_IRQn` staged as 77) | GPT1 overflow IRQ 47 (already in USB PCDC table) | ELC GPT_C = 2, event `ELC_EVENT_GPT7_CAPTURE_COMPARE_A` = 0x1C0; ELC GPT_D = 3, event `ELC_EVENT_GPT1_COUNTER_OVERFLOW` = 0x190; stop source `GPT_SOURCE_GPT_D` (1<<19); count-up `GPT_SOURCE_GPT_C` (1<<18); clock from `R_GPT_InfoGet` on GPT7; P603 SCI is pin capability only — this tree does not open SCI0 |
| PDM | current acquisition preserved | DMAC1 rise, DMAC2 fall | unchanged | `ELC_EVENT_PDM_DAT2` / `ELC_EVENT_PDM_DAT0`; PDM IRQs unchanged; pair init unwinds only GPT6/7, GPT0/1, DMAC0/3 |

Common start: one write `GTSTR = (1<<6)\|(1<<7)` through the GPT6 register block. Two sequential `R_GPT_Start` calls are not used for the PWM pair.

Generated BSP objects `g_timer1` and `g_timer7` exist in `hal_data.c` and are not opened by this fixture. Init refuses the resource if GPT1 or DMAC3 ISR context is already occupied.

Duty storage: two static 3840-entry 32-bit arrays, 32-byte aligned (`duty_a0`, `duty_a1`) = 30 720 bytes, plus two 480-byte owned payloads and one latest-pending pair. Not double-buffered duty.

## What is left

1. Operator confirms P603 wiring: **DIN-B moved to P603; 80/81 break checked.**
2. Rearm of the named pair candidate (not this recovery HEX).
3. Bounded mapping fixtures, then the two-minute music run, then the ten-minute stability segment.
4. GPT clock, pair wire capture, and full TRUE16 through the real renderer remain open.

## Host pair model and target driver

`platform/ra8p1/ws281x_gpt_dma_pair.*` is the tested coordinator (admission, pending replace, `pair_lanes_complete`). `ws281x_gpt_dma_hw_pair.c` is the target driver and uses that predicate. `python3 scripts/test_ws281x_gpt_dma.py`, `test_ws281x_gpt_dma_pair.py`, `test_ws2816_pack.py`, `test_ws2816_pair_pipeline.py`, `test_ws281x_gpt_dma_hw.py` and `test_ws281x_gpt_dma_hw_pair.py` PASS.

Named ARM image: `docs/evidence/K1-RA8P1-002/ws2816-pair-candidate-20260921-a` build `fa54c7818a3fdf87344514854141dc06f7d000f5727e91b23a5fcc4245fe46b6`. Not flashed.

Ruling: an earlier WP6 plan forbade `k1_ws281x_gpt_pin_can_pwm(P603)`. The 21 September brief names P603/GTIOC7A as the second GPT lane. The host model now accepts P603 as a PWM-capable pin. That is silicon pin-function, not a wiring approval. `titan_led_pins.h` still records DIN-B as P004. P004 remains not PWM.
