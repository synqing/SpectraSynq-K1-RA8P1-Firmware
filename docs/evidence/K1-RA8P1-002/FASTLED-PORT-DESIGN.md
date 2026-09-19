# FastLED-derived WS2812 / WS2816 output on Titan

10 September 2026. Design and HOST experiments complete. Target GPT6/DMA
driver is in `platform/ra8p1/ws281x_gpt_dma_hw.c` and is cross-compiled in
`ws2812-gpt-dma-build-01` / `ws2816-gpt-dma-build-01`. Not flashed in this
note, not waveform-captured, P004 still has no GPT route. G4 remains FAIL.

## Decision

Keep the existing TRUE16 packer. Extract FastLED's useful boundary: wire-ordered
bytes, an explicit timing profile, and a platform-specific transmitter. Implement
a small RA8P1 GPT/PWM + DMA transmitter under `platform/ra8p1/`, initially **one
lane on P601 / GTIOC6A**. Use Captain's working WS2812 strip as the control before
testing the uncertain WS2816 strip. Do not import the whole FastLED runtime,
Arduino, Teensy register code, effects, colour pipeline, or global controller list.

This is a scoped answer to “how to migrate/copy/refactor”; it does not implement
or authorise a flash. It supersedes the earlier blanket “no FastLED” instruction
only for investigation and the proposed narrow reuse. It does not change the
existing wiring, import-slices, BSP, DualMCU source, G4 or hardware authority.

The first implementation outcome is an independently testable single-lane
transmitter, not a complete product LED backend. Dual-lane and realtime integration
follow separate gates below.

## Evidence and current state

Firmware repository: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`,
branch `lane/k1-ra8p1-002`. Initial inspected HEAD was
`11d4a03e664265c3ae72396b6b5d4e4e82f18f53`; another lane advanced it during this
investigation to `fb36f2d1edbe60f4f6073b4158a029bafad4c112` (cache-mode work).
Final identity check observed `3b8bb88877e7e57bc8172f3966f09448a7d1d2f6`
(comparator-evidence work); packer and GPIO emitter hashes remained unchanged.
No branch switch, reset or source edit was performed here. Existing untracked
`docs/BRIEF-2026-09-10-uncapped-ap-timing.md` and `docs/decisions/` were preserved.

`scripts/check_references.sh` passed for both pinned references:

- DualMCU `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`; its PlatformIO FastLED dependency
  is **3.10.3**. The RT1062 adapter uses `__FIBCC<WS2812Controller800Khz,...,GRB>`
  and Pixel8/CRGB storage. It is behavioural reference, not a RA8P1 register driver
  or proof that native WS2816 data can be sent through it unchanged.
- Titan BSP `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`, FSP register/driver authority.

FastLED release API reported **3.10.4**, pinned to
`adedfc40e73fb80f8e930318781036d8fe1dbd9f`. This is the source used by the HOST
experiment. Master was separately inspected at
`e7b89037d6b0a6c993d6f6fa48b7163440c09715` for its porting guidance only; do not
silently substitute master in the build or oracle.

[Experiment and replay instructions](/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/fastled-port-2026-09-10/README.md)
include identities, commands, outputs and limits. **65,536 vectors matched the
unmodified FastLED encoder; channel-swap and low-byte-loss mutations were detected.**
Existing packer and protocol HOST tests passed. This proves byte packing, not GPIO
timing, cache coherency, current delivery, LED reception or photons.

The earlier `led-smoke-06` receipt remains historical-target evidence for UID
`545433931bd25436593630352d068363`, build `f0205ef8…`. No live target was opened
during this investigation. A newer checkout does not change the flashed image.

## What to reuse and what to replace

All FastLED links below are pinned to the experiment revision.

| Source | Decision |
| --- | --- |
| [`src/fl/chipsets/encoders/ws2816.h`](https://github.com/FastLED/FastLED/blob/adedfc40e73fb80f8e930318781036d8fe1dbd9f/src/fl/chipsets/encoders/ws2816.h) | Use `packWS2816Pixel` as the HOST oracle; retain our already-equivalent fixed-buffer `packPixel`. Inputs to upstream helper must already be in **G,R,B** wire order. |
| [`src/fl/chipsets/led_timing.h`](https://github.com/FastLED/FastLED/blob/adedfc40e73fb80f8e930318781036d8fe1dbd9f/src/fl/chipsets/led_timing.h) | Reuse named timing-profile semantics. T1 = zero-high; T1+T2 = one-high; T1+T2+T3 = bit period. Select the exact LED variant, not just “800 kHz”. |
| [`src/chipsets.h`, `WS2816Controller`](https://github.com/FastLED/FastLED/blob/adedfc40e73fb80f8e930318781036d8fe1dbd9f/src/chipsets.h#L576) | Architectural evidence: 48-bit pixels are packed into twice as many 24-bit containers and sent through a WS2812 transport. Do **not** wrap already-packed TRUE16 bytes in this controller; that repeats packing/scaling and can allocate buffers. |
| [`src/platforms/arm/renesas/clockless_arm_renesas.h`](https://github.com/FastLED/FastLED/blob/adedfc40e73fb80f8e930318781036d8fe1dbd9f/src/platforms/arm/renesas/clockless_arm_renesas.h) | Reference only. It describes RA4M1/M4, resets `ARM_DWT_CYCCNT`, uses CPU-dependent delay compensation, blocking transmission and unconditional interrupt enabling. None transfers safely to the M85 application unchanged. |
| `src/platforms/arm/renesas/fastpin_arm_renesas.h` | Reference only: board pin tables/Arduino pinMode and whole-PODR writes are not Titan pin ownership. `IOPORT_PERIPHERAL_GPT1` is a mux selector, **not timer channel 1**. |
| `src/platforms/esp/32/drivers/spi/wave8_encoder_spi.h` | Alternative encoding concept, not the selected first backend. ESP-IDF DMA/SPI and dual/quad-lane capabilities are not RA8P1 capabilities. |
| [`LICENSE`](https://github.com/FastLED/FastLED/blob/adedfc40e73fb80f8e930318781036d8fe1dbd9f/LICENSE) | MIT; retain its copyright/permission notice in any copied or substantially derived code, record exact source SHA and modifications. No code has been vendored into firmware in this investigation. |

If upstreaming a complete FastLED RA8P1 platform later, use its `PORTING.md`:
separate platform detection, pin map, integer/clock hooks and controller adapter.
That broader port is not necessary for the small K1 raw-byte backend.

## Timing profiles: shared engine does not mean identical settings

Times below are **nominal source/design values**, not measured waveforms.

| Profile | Bits/pixel | T0H | T1H | Period | Reset low |
| --- | ---: | ---: | ---: | ---: | ---: |
| FastLED 3.10.4 default WS2812 reference | 24 | 250 ns | 875 ns | 1,250 ns | upstream 280 us; bench request 300 us |
| FastLED WS2812B-V5/Mini-V3 reference | 24 | 225 ns | 580 ns | 1,225 ns | upstream 280 us |
| WS2816C-1313-4P selected nominal | 48 | 250 ns | 650 ns | 1,250 ns minimum | 300 us, guaranteed **greater than** 280 us |

The working WS2812 strip's exact revision and successful controller configuration
are not yet supplied. Preserve its known-good setup as the control; select its
profile explicitly before the target trial. Do not sweep timing blindly or label
the default universal. Requantise all four pulse widths against the resolved timer
clock; use ceiling for the WS2816 minimum period and reset, then validate bounds.

Worldsemi **WS2816C-1313-4P**, page 4, specifies T0H 200–320 ns, T1H 520–800 ns,
T0L 800–1,200 ns, T1L 480–1,000 ns, both complete bit cycles at least 1,250 ns,
and reset greater than 280 us. The default FastLED WS2812 profile violates its
one-high and one-low limits even though it is called an 800 kHz transport. The
HOST probe rejects it. A common three-SPI-bit 2.4 MHz encoding also fails these
WS2816C bounds; “use SPI” alone is not a timing solution.

The manufacturer's page-1 prose mentions 24 bits, contradicting its page-4 48-bit
diagram and 16-bit/channel specification. Use the explicit GRB48 diagram, the
upstream encoder and the existing TRUE16 contract; retain the contradiction.
The blanket claim that a WS2812 transport necessarily “bricks” WS2816 is too broad:
reuse of the engine is possible; correct payload and applicable timing are required.

## Pin and peripheral decisions

RA8P1 datasheet Rev.1.30, Table 1.17, visually checked pages 32–33:

| Pin | Actual function | Consequence |
| --- | --- | --- |
| P601, U18 pin 7 | **GTIOC6A**; also SCI SCK0_B, not SCI MOSI | Selected first PWM lane. The stock `GPT1` mux constant must not be “fixed” to mean channel 1 or 6. |
| P004, U18 pin 16 | GPIO / IRQ9-DS / AN004 / IVCMP2; **no GPT or serial-output function** | Cannot implement direct peripheral PWM here. Keep it inactive in the new single-lane experiment; do not silently relocate the existing DIN-B. |
| P603, U18 pin 33 | GTIOC7A / SCI TXD0_B-MOSI0_B | Candidate second PWM lane; requires a separately approved physical DIN-B move and SCI ownership check. |
| P604/P605, U18 pins 12/32 | GTIOC8B / GTIOC8A | Alternative matched timer pair for later multi-lane design; pin/package availability and wiring must be bound. |

The board/header mapping comes from the existing wiring authority, not a new
continuity measurement. Its “P601 GPT1” label was the peripheral-select name,
not evidence of GPT channel 1. The selected pin function is independently confirmed
by the current datasheet. GPIO output on P004 remains possible; lack of GPT does
not mean the pin is broken. No HCT diagnosis is reopened.

The vendor `R7KA8P1KF_core0.h` defines `R_GPT6` and `R_GPT0`, 32-bit GPT registers,
and DMAC. `bsp_feature.h` reports 14 32-bit/event-count GPT channels and eight DMAC
channels. `bsp_elc.h` defines GPT6 compare/overflow, GPT0 overflow and ELC GPT A–H.
Manual sections 23.1, 23.2.6, 23.2.8, 23.2.17 and 20.3 corroborate the mechanism.
These are vendor capabilities, **not an already-working LED DMA driver**.

## Selected transmitter design

One waveform GPT, one hardware event-count GPT, one DMA channel and two ELC slots.
Proposed initial allocation: GPT6 output, GPT0 frame counter, one explicitly free
DMAC channel. The implementer must claim/read back all resources, not assume a
generated `g_timer6` object is unused. `led-build-04/stage/rtconfig.h` contains no
`BSP_USING_PWM` definition, but the current build must be rechecked. Never seize
another lane's timer, interrupt, ELC route or DMA channel.

1. Check pin configuration errors and read back PFS. Configure GPT6A normal
   saw-wave PWM, high at the bit boundary, low on compare, low at stop. Use the
   vendor FSP/CMSIS definitions, not guessed register structs.
2. Resolve GPT's actual count clock through `R_GPT_InfoGet` / FSP's
   `gpt_clock_frequency_get`. RA8P1 can use **GPTCLK or PCLKD**; INFO's 1 GHz CPU
   value and RT-Thread's PCLKD-only helper are not adequate. Do not change shared
   clock domains to make a table fit.
3. Convert raw bytes MSB-first into a static 32-bit duty table, one value per bit.
   FSP's saw-wave buffer encoding uses `duty_counts - 1`; period is
   `period_counts - 1`. Prove the first pulse, not only steady-state duty.
4. Preload active GTCCRA with bit 0 and single-buffer GTCCRC with bit 1 while
   stopped. Feed subsequent compare values on GPT6 overflow through DMA:
   source incrementing, destination fixed `GTCCR[2]` / GTCCRC, 32-bit normal
   transfers. For N bits, feed exactly N−2 values. No ISR per bit. DMA completion
   means the last value reached a buffer, not the last bit left the pin.
5. **Hardware end-of-frame:** route GPT6 compare-A events to an ELC GPT slot
   that increments GPT0. Compare is associated with, but is **not** the physical
   falling edge: FSP documents a one-count output-transition delay in buffered
   mode. Include that offset in stop-path admission. GPT0 has no output pin,
   starts at zero, counts N events, and overflows at its Nth event. Route that
   overflow via the other ELC GPT slot to GPT6's hardware stop source. GPT6 must
   stop low before another bit begins. Configure count/stop sources narrowly;
   don't globally rewrite other ELC slots or reset DWT.
6. Validate the two event-link delays using manual section 20.4.4 and the actual
   clock domains. **Stop propagation must fit inside the final bit's low phase**
   before the next rising edge. Clock/resource admission and captured N=8/16,
   all-zero/all-one/alternating tests are mandatory. If this fails, do not fall
   back silently to CPU-timed finalisation; reassess the backend with evidence.
7. Report hardware-stop observation separately from its IRQ/service timestamp.
   Enforce at least the profile reset interval from a conservative confirmed-low
   time before accepting the next frame. A delayed callback may delay readiness;
   it must not add another LED pulse. Reset/latch waiting leaves interrupts enabled.

Do not append integer zero to GTCCRC and assume it means continuous low: FSP
handles 0%/100% duty via GTUDDTYC, separately from the ordinary compare values
(manual 23.3.6). The hardware counter/stop path is selected to avoid a runt-pulse
or extra-pixel tail and avoid the manual's DMAC-destination/ELC-destination race
(20.4.1). The composition is source-supported but still requires cross-build and
pin-capture proof. No hard realtime latency is asserted from register existence.

Buffers live in a linker-verified **DMAC-reachable** SRAM region with explicit
M85 cache maintenance or a verified non-cacheable mapping; not assumed DTCM and
not inferred from another bus master's access. A single 80-pixel GRB48 duty table
is 3,840 × 4 = 15,360 bytes before bookkeeping. Keep storage static, bounded and
owned until DMA finishes; do not allocate in render. Concurrent cache-mode work
must be integrated explicitly, including cache-on and cache-off checks.

SPI is the fallback design alternative, not the initial selection: port a
profile-driven packed-wave encoder to RA8P1 SPI_B, verify actual baud quantisation,
gap-free frames, DMA/shift-register drain and idle-low tail. It needs a verified
MOSI pin; P601 and P004 are not suitable dedicated MOSI replacements, and P708
shares the LCD connection. No ESP32 pin matrix or dual-SPI assumption is allowed.

## Small implementable API boundary

Proposed names, not existing commands or declarations:

```cpp
struct Ws281xProfile {
  ProfileId id;              // versioned, source-backed or explicitly experimental
  uint8_t bytes_per_pixel;   // exactly 3 or 6
  uint32_t t0h_ns, t1h_ns, period_ns, reset_us;
};
struct WireFrame {
  ProfileId profile;
  const uint8_t* bytes;      // already in wire order; no correction/gamma/scaling
  size_t byte_count;
  size_t pixel_count;
  uint64_t sequence;
};
SubmitResult submit(const WireFrame&); // accepted, busy, invalid, unavailable
TxStatus poll();                       // idle, emitting, latch_pending, ready, fault
void abort_low();                      // stops, holds low, invalidates partial frame
```

The C platform equivalent is acceptable. Pixel8→GRB24 and Pixel16→GRB48 are
separate packers. The private waveform encoder can test arbitrary bytes; the
public frame boundary rejects empty frames, null buffers, overflow,
`byte_count != pixel_count * bytes_per_pixel`, unsupported profiles/pins,
incompatible timer quantisation and over-capacity. Initial bench maximum is
80 pixels per lane, not a claim about maximum hardware capacity. All-off is a
normal nonempty zero-valued frame. Busy never corrupts the in-flight table.

Receipts bind UID/build/image, pin readback, profile/clock, frame and duty-table
hashes, submitted byte/bit counts, DMA completion, hardware-stop observation,
latch readiness, overruns, aborts and evidence level. CRC is not reception.
Do not relabel existing opcode 11's fixed 480-byte WS2816 contract as WS2812.
Add a separately versioned diagnostic request/opcode after checking the current
protocol allocation; keep compatibility and record the selected profile.

## Implementation order and ownership

One firmware implementer owns all platform/build/protocol writes. Another lane is
actively editing `fixture_app.cpp` and `build_scalar.py`; coordinate before touching
either. No parallel writers or sibling worktree are required.

1. **HOST protocol boundary:** add `platform/ra8p1/ws281x_profile.{h,cpp}` and
   `ws281x_waveform.{h,cpp}` plus separate `tests/host/test_ws281x_waveform.cpp` and
   `scripts/test_ws281x_waveform.py`. Keep `ws2816_pack.h` unchanged; add GRB24
   packing without altering the product Pixel16 path. Acceptance: exact raw bytes,
   24/48 bit counts, reversed-bit/order and double-packing negatives, timer-rounding
   failures, strict reset bound, maximum length and arithmetic overflow.
2. **PRE-SILICON backend:** add `platform/ra8p1/ws281x_gpt_dma.{h,c}`. Integrate
   through the existing `platform/ra8p1/SConscript` and build script only after
   coordination. Compile the actual BSP headers; retain map/disassembly and memory,
   resource and clock evidence. Test preload/DMA/event-counter state models with
   N=8,16 and maximal frame; shifted preload, missing stop and extra event must fail.
   These models are not hardware proof. Do not remove the existing GPIO smoke yet.
3. **Identified-target WS2812 control:** bind strip count/revision and the existing
   known-good source/controller; connect only the declared test load with verified
   power/common ground/level translation appropriate to that separate strip. Start
   with a few bounded pixels, not full-white product load. Compare the same bytes
   and capture GPIO pulse widths/count/reset against the control. Use instrumented
   evidence, not Captain's eyes. Any reflash follows WAITING_FOR_IDENTIFIED_ROM first.
4. **WS2816 and second lane:** change only profile and packing on the already-proven
   single-lane engine; use unequal high/low bytes and GRB colours. A failed WS2816
   trial after a passing WS2812 trial still leaves profile/strip/wiring hypotheses;
   it does not prove the WS2816 strip dead. Resolve the DIN-B hardware route and
   80/81 data break before dual drive. P603 is a candidate, not an authorised move.
5. **Product integration:** prove complete DualMCU treatment/gain/current-limit/
   packing/latch parity, all required physical segments and simultaneous output.
   Measure CPU availability, underrun/contention and AP/VP scheduling. G4's
   2,005/6,000 = **33.4% deadline misses stays FAIL**; no waiver or import-slices
   change is included. A single 160×48-bit chain needs 9.6 ms payload at 800 kHz;
   it cannot meet an 8.333 ms frame period by itself. The proven 80-pixel segment
   topology, not merely “two logical channels”, must drive the final concurrency plan.

### How the control removes unknowns

| Observation | Justified next conclusion |
| --- | --- |
| Known-good controller + WS2812 succeeds; Titan waveform/count is wrong | Fix the Titan transport before judging either strip. |
| Titan waveform is valid but WS2812 output fails | Investigate measured power, level translation, ground, connector/pin and strip applicability; software CRC is insufficient. |
| WS2812 succeeds on Titan; WS2816 fails | Shared transmitter has evidence for the tested profile, not universal clearance. Check GRB48/profile and the WS2816 hardware independently. |
| WS2816 on a separate known-good correctly configured driver also fails | Evidence against that strip/wiring strengthens; isolate those two before concluding strip failure. |

## Source integrity and limits

- Worldsemi PDF: `/Users/spectrasynq/Workspace_Management/Software/WS2816-Testbed/docs/eval/silicon/WS2816C_1313_4P_V1.1_EN.pdf`,
  SHA-256 `c5231ae4137837292a74ea4cfa4cac0cf3d506c6f314c81b67cfb77013deeae0`, page 4 visually inspected.
- Renesas datasheet R01DS0439EJ0130: SHA-256
  `2b40dfb896a82301cf7d76ec0d98dcb802423a7f8c879fb31a0c38a509c6d058`, pages 32–33 visually inspected.
- Renesas manual R01UH1064EJ0130: SHA-256
  `3f9fa489f6b17eff52516f8e9d31d5aec97eec20883ca43e902f95da0757cdc2`.
  A fresh download from the official URL matched. The high-accuracy PWM row on
  page 889 contains `DEADBEEF`; it is not used to infer supported high-resolution
  channels. Ordinary GPT/event/count/buffer capabilities were corroborated in
  the vendor headers and relevant sections. No source file was altered.

Subagent launches were rejected twice by the local wave guard despite a bounded
brief; no child ran and no guard was disabled. Research was completed locally.
`find-skills` found FastLED's own `platform-port` skill (3 reported installs;
upstream repository 7,487 stars at inspection). Its vendor-header/peripheral
existence checks informed this design; no new skill was installed. No agent
performance or production qualification follows from those counts.
