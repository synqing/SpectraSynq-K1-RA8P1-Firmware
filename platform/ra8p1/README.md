# RA8P1 Platform Boundary

Only hardware/RTOS/toolchain integration belongs here. Shared K1 source under `src/k1/` must remain free of Renesas, FSP, RT-Thread, J-Link and board-specific headers.

The first backend must eventually provide:

1. boot and monotonic timer;
2. canonical `MEDIA_TIME_48K` observations at the audio capture boundary;
3. PDM and/or SSIE/TDM capture through bounded DMA;
4. an explicit `(media_frame, monotonic_time, uncertainty)` observation;
5. render-cost measurement;
6. physical LED output/latch cost;
7. fault/overrun and epoch-break reporting.

Do not copy RT1062 timing constants into this backend. Measure or analytically derive RA8P1-specific values.

For K1-RA8P1-001, execute on the M85 only. Do not split work onto M33 and do not use Ethos-U55.

## K1-RA8P1-002 scalar fixture shell

`scripts/build_scalar.py --output <new-external-directory>` stages the pinned
RT-Thread/FSP USB PCDC project in an isolated directory. It copies, never edits,
the BSP; explicitly includes every required C++ implementation; disables MVE,
automatic vectorisation, fast-math and FP contraction; and retains the map,
disassembly, compiler macros, stack-usage files and source/build hashes.
`--debug` builds a separately identified debuggable image.

The inherited FSP `SystemInit` invokes the C++ constructor table. The INFO
response also checks the constructed channel-B identity. M33 activation status
is reported from `R_CPU_CTRL->CPU1ACTCSR`; no secondary-core start or U55 open
call is linked. The client refuses a running M33 or failed constructor witness.
D-cache is disabled as in the proven P2/P3 USB path. Main-thread stack is 32 KiB;
compiler stack reports are not runtime stack high-water proof.

`scripts/test_fixture_protocol.py` exercises the actual parser on HOST. Binary
requests are single-flight, little-endian, CRC-protected and bounded to 360
payload bytes. Commands: INFO (1), one 180-sample PCM16 hop (2), explicit epoch
reset (3), time assertions including one million beats (4). Full output fields
and all 320 pixels are returned; there is no feature/pixel stream to the S3.
USB-paced execution is a parity fixture, not a deadline or physical-output test.

The fixture preserves the pinned GDFT postprocessor's explicitly frozen 10 ms
AGC tuning clock and 100 Hz mood parameter (`gdft_postprocess.h`, K1-DM-112).
These are deliberately distinct from the actual 7.5 ms AP release coordinate.
Changing them to the AP hop would change the reference behaviour.

`scripts/programme_scalar.py --build <build-directory> --output <new-run-directory>`
performs an offline image/restore preflight. Adding `--execute` uses the proven
Lab ROM programmer and requires exact canonical UID before any write. Hold
USER/BOOT through reset and programming; release only after
`PROGRAMME_VERIFY_PASS`, then reset normally. No permanent security operations
are exposed. No software ROM-entry command is available in the current P2/P3
application.

After normal reset, use the Lab `.venv/bin/python` to run:

```
scripts/run_scalar_target.py --build <build-directory> --output <new-run-directory> --corpus <host-product-directory> --stage smoke
```

Then run `--stage corpus` into another new directory. The client requires the
exact runtime UID, source pin, build ID and adapter hash. Its current comparator
is a strict exact diagnostic: any target arithmetic divergence stops with the
first differing field; no unrelated P3 tolerance is borrowed. Runtime scalar
parity remains open until this executes. G4 additionally needs a separately
frozen scheduled workload and identified NPU load; this USB shell cannot close it.

Physical inventory: the pinned `Titan_Mini_pdm/README.md` documents an onboard
PDM microphone and a 16 kHz loopback example. It is not a proven 24 kHz K1 capture
backend. PDM qualification waits for E1. Captain-reported Titan LED map
(2026-09-10): one 160-LED WS2816 stick, DIN-A **P601** leds
1–80, DIN-B **P004** leds 81–160, 80/80 centre-split (data break between 80/81
required). Second 160-pixel edge unwired. USB opcode **11** (payload 0) emits a
TRUE16 fixture (`0x12AB` on A[0] red / B[0] blue), 80/80 Lever-2 pack, lockstep
GPIO on PCNTR3. Reply CRCs prove **emission**, not photons. Bit-period min/max are in the
JSON. Host: `scripts/run_led_smoke.py` (read wait ≥50 ms). ON-SILICON:
`led-prog-03` verified build `f0205ef8…` on UID
`545433931bd25436593630352d068363`; `led-smoke-06` matched both host-packed
CRCs (emission only). The 74HCT2G34GW shifter is known present and 3.3 V HCT
input is valid, so do not reopen a missing-shifter theory. Software loop-start
periods were 1,302–1,466 cycles versus the configured 1,250 ns; GPIO pulse
widths and photons remain unmeasured. `K1_RA8P1_LED_OUTPUT` =
`WIRE_SMOKE_RECORDED`, not product PASS. Output-path gaps vs DualMCU:
`docs/evidence/K1-RA8P1-002/OUTPUT-PATH-COMPOSITION.md`. Authority:
`SpectraSynq-EdgeAI-Lab/docs/titan/TITAN_LED_WIRING.md`. Titan-to-S3 wiring is
still unestablished; USB enumeration of an S3 does not establish that bridge.
