# K1-RA8P1-002 physical-path inventory

Status: `HOST_INVENTORY_COMPLETE_PHYSICAL_PATHS_OPEN`

Dated correction 2026-09-20: this inventory still does not claim physical
capture, LED output, Titan-S3 integration, or acoustic-to-photon latency.
It does not carry a sub-8 ms Titan latency stamp. "Resident comparator"
below is historical wording, not a current INFO bind.

This inventory identifies what is actually available at the pinned Titan BSP
and what is still required. It does not claim physical capture, LED output,
Titan-S3 integration, or acoustic-to-photon latency.

## Bound authorities

| Authority | Identity |
| --- | --- |
| Titan BSP | `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7` |
| PDM README | SHA-256 `e407d6fd937b8373291a53bfde30332c29840085e4692a42ad34ed17197fdbe6` |
| PDM example source | SHA-256 `bda2f9e274dab17ffc9620a6efecb97db430f9f3f4c3c6cb52d493b10573300c` |
| FSP configuration | SHA-256 `746ba3a2f01ff96d23008dd3197f06cf049ac9671f521820522a2b6f603c9ff8` |
| Titan LED wiring authority | SHA-256 `2e5e843e0bbffc3c8483a7ab899c50695e028c60ee3e9c905ffe9c9e6dde7519` |

## Capture

The Titan Mini schematic identifies the onboard microphone as a LinkMems
`LMD2718T261-OA1` connected to `PDM_DAT1`/P502 and `PDM_CLK1`/P812. The
manufacturer specification identifies a one-bit PDM output and a 150 kHz to
4.8 MHz clock range. This establishes electrical compatibility with a 2.4 MHz
standard-mode clock; it does not establish a 24 kHz PCM configuration.

The pinned BSP donor is not a usable K1 capture backend as written:

- output is configured as mono, 16 kHz, 16-bit PCM;
- `p_transfer_rx` is `NULL` and the FSP PDM DMAC option is disabled;
- it captures 16,000 samples, waits by polling at 1 ms, stops the peripheral,
  converts the complete one-second buffer, then starts the alternate buffer;
- overwrite detection and an error callback exist, but the example retains
  only the latest error code and no loss/overflow count;
- it supplies no acquisition-boundary timestamp or media-time epoch.

The smallest acceptable backend is therefore a new bounded adapter, not a copy
of the example loop:

1. Prove an FSP-supported 24 kHz mono configuration, or select a declared
   higher-rate capture plus a fixed causal 24 kHz conversion with measured
   group delay.
2. Use fixed ping-pong DMA blocks aligned to the AP hop, with explicit cache
   ownership, sequence, epoch, capture-boundary timestamp and overflow count.
3. Convert the actual captured PCM through the existing host reference and
   compare the target AP result on the same samples.
4. Exercise overflow, restart, clock discontinuity and reset recovery before
   integrating physical capture into the realtime schedule.

Until step 1 is proven, `K1_RA8P1_PHYSICAL_CAPTURE=OPEN_RATE_AND_DMA`.

## Visual output

The onboard XL-1615 RGB indicator is three ordinary status GPIOs. It cannot
drive or represent the K1 Light Guide Plate.

The K1 product output is two independent 160-pixel WS2816C-1313 chains. Each
pixel consumes 48 serial bits and requires a reset interval greater than 280 us.
One 160-pixel stick is now identified as an 80/80 centre split: P601 drives
LEDs 1–80 and P004 drives LEDs 81–160 through a 74HCT2G34GW level shifter.
The second 160-pixel product edge remains unwired.

`led-prog-03` programmed and read back build `f0205ef8…` on canonical UID
`545433931bd25436593630352d068363`. `led-smoke-06` then revalidated the live
identity and matched both 480-byte host-packed CRCs. The last bright submission
reported 5,112.104 us emission plus 300.011 us latch with interrupts disabled.
The reported 1,302–1,466 ns loop-start interval is software instrumentation,
not a logic-analyser measurement of T0H/T1H. No photon or GPIO-edge claim is
made.

The smallest acceptable output backend must provide:

1. two concurrently timed WS2816 outputs so one chain does not serialize the
   other and violate the 120 fps schedule;
2. fixed 48-bit packing, no render-time allocation, and the exact two 160-pixel
   buffers already proven by the resident comparator;
3. submission, transfer-complete and latch-complete timestamps plus underrun
   and overrun counters;
4. logic-level and short-stick timing proof before connection to the powered
   dual-strip plate.

Until GPIO waveform/PFS evidence, the second stick and the complete DualMCU
post-render treatment/gain/current-limit/latch path are proven,
`K1_RA8P1_LED_OUTPUT=WIRE_SMOKE_RECORDED`.

## Titan-S3 bridge

Two Espressif USB/JTAG endpoints were present during inventory, but port
enumeration does not identify either as the authorised K1 S3 or prove compatible
bridge firmware. No Titan-S3 electrical transport is connected or identified.

The bridge remains radio-only. It may carry bounded commands, state, health and
telemetry; it must not become an `AudioFeaturesV1` or pixel-streaming workaround.
Closure requires named devices, exact firmware identities, the selected
electrical transport, independent clock/epoch mapping, capacity-one/latest-value
buffering, and disconnect/reset/stale-record recovery.

`K1_B_TITAN_S3=BLOCKED_IDENTIFIED_DEVICE_AND_CONNECTION`.

## End-to-end instrument boundary

No checked-in photodiode or logic-analyser capture rig was found. Software
timestamps can separate capture boundary, AP publication, VP consumption, LED
submission and latch completion, but cannot establish acoustic-to-photon
latency. Physical closure needs a common-clock stimulus/capture instrument with
at least 100 declared samples and every measured result below 8,000 us.

`K1_RA8P1_ACOUSTIC_TO_PHOTON=BLOCKED_COMMON_CLOCK_INSTRUMENT`.

## Sources

- `project/Titan_Mini_pdm/README.md`, `project/Titan_Mini_pdm/src/hal_entry.c`
  and `FSPConfiguration/{configuration.xml,ra_gen/hal_data.c}` at the pinned BSP.
- Titan Mini schematic HW V1.0, sheets MCU ports and PDM microphones.
- [LMD2718T261-OA1 manufacturer specification mirror](https://atta.szlcsc.com/upload/public/pdf/source/20240604/8A996F5B348F7613D841F153E3624C08.pdf).
- `SpectraSynq-EdgeAI-Lab/docs/titan/TITAN_LED_WIRING.md`.
