# Titan PCM1808 AUX/line-in handover

## Corrected status — 2026-09-10

The prior direct-wiring handover was wrong. U11 is a fine-pitch camera connector,
not a practical wire-soldering point. The build below proved that the SSIE/DTC
software linked; it did not establish a usable physical route and is superseded.

U18 does **not** expose a complete SSIE group. Its apparent SPI_B alternative is
also not a valid framed slave receiver: U18 exposes `SSLB2` and `SSLB3`, while
RA8P1 SPI_B slave mode requires `SSLB0` as the slave-select input. `SSLB1` to
`SSLB3` are outputs in slave mode. `P000`, `P001`, and `P002` cannot substitute
for these peripheral signals.

Therefore the honest state is `BLOCKED_NO_PRACTICAL_CONNECTOR_ROUTE`. The build
tool now fails closed if `--pcm1808-target` is requested. Physical PCM1808 work
requires either:

1. a proper mating breakout for U11 `DF12NB(3.0)-36DS-0.5V(51)`, carrying the
   three native SSIE1 signals; or
2. a small digital-audio bridge board that presents a fully framed interface on
   pins Titan can actually receive.

No direct soldering to U11 is proposed or required.

The exact mating connector for a custom breakout PCB is Hirose
`DF12NB(3.0)-36DP-0.5V(51)` (`CL0537-0391-0-51`): 36 positions, 0.5 mm pitch,
3.0 mm mated height. The Titan part is the matching `DS` receptacle; the breakout
must use the `DP` header. This is a board-to-board connector, not an FFC socket.

## Superseded pre-silicon build record

The working PCM1808 ingress from the existing K1 firmware was compiled into
the Titan build as a bounded SSIE1/DTC capture target. The donor files are pinned
to `/Users/spectrasynq/SpectraSynq_K1_Firmware` commit
`0b542364abeec1337bd74d7b3b9010e37b969401`; the imported resampler and mapping
headers are byte-for-byte checked against that commit.

The final combined pre-silicon image contains both the onboard dual-PDM capture
and external PCM1808 capture:

- build ID: `17152b7c68182d6aa3afdeb25420e7efdf00ea1a963cfffc71532aac75ae4b3d`
- evidence directory: `docs/evidence/K1-RA8P1-002/pdm-pcm1808-build-04`
- linked size: text 199,492 bytes; data 18,256 bytes; BSS 207,028 bytes
- onboard PDM metrics: fixture opcode 6
- PCM1808 metrics: fixture opcode 15

This was a successful cross-build only. It must not be flashed or treated as a
wiring-ready image until one of the two physical adapters above exists.

## Wiring after a proper U11 mating breakout exists

Power the boards off before making or changing these connections.

| PCM1808 module | Titan connection | Titan signal | Purpose |
| --- | --- | --- | --- |
| `5V` | U18 pin 2 or pin 4 | `VSYS_5V` | Module power |
| `GND` | U18 pin 39 | `GND` | Common reference |
| `BCK` | U11 pin 24 | `VIO_D6` / `P702` / `SSIBCK1_B` | PCM1808 bit clock into Titan |
| `LRCK` | U11 pin 33 | `VIO_D5` / `P701` / `SSILRCK1_B` | PCM1808 word clock into Titan |
| `DOUT` | U11 pin 26 | `VIO_D4` / `P700` / `SSIDATA1_B` | Stereo sample data into Titan |
| `MCLK` | no connection | none | PCM1808 module owns its clocks |

U11 is the 36-pin 0.5 mm camera BTB connector
`DF12NB(3.0)-36DS-0.5V(51)`. A suitable mating FFC/BTB breakout is required;
these three SSIE1 signals are not available as a complete group on U18. This
route is mutually exclusive with use of those U11 pins by a camera.

The PCM1808 module must be configured for 48 kHz: short `OP2` only; leave `OP1`
and `OP3` open. Before connecting the signal wires to Titan, confirm approximately
48 kHz at LRCK and 3.072 MHz at BCK and confirm that all signal levels are 3.3 V
safe. The module is the clock master and Titan SSIE1 is the slave receiver.

## What the firmware does

- Receives Philips I2S stereo, 24-bit samples in 32-bit words.
- Unpacks the lower 24 bits because FSP 6.4.0 `r_ssi` explicitly selects
  right-justified FIFO placement (`SSICR.PDTA=1`) for 24-bit PCM; this was
  checked against the RA8P1 SSIE register definition rather than inferred from
  the ESP donor layout.
- Uses DTC-backed block receive rather than a CPU interrupt for every sample.
- Alternates between two 360-frame buffers, one 7.5 ms block each.
- Stops and reports failure if the consumer misses a buffer; it does not silently
  overwrite or stretch the hop.
- Reproduces the donor MID mapping, 4/15 FIR conversion, and Q15 trim of 2048,
  producing 96 signed 16-bit samples at 12.8 kHz.
- Reports callbacks, measured input rate, overflows, idle/error events, processed
  hops, extrema, peak, energy, and an advancing sample hash.
- Starts only after USB is configured, so an external clock cannot fill the
  bounded buffers before the host can identify and observe the image.

## Important rate boundary

The donor PCM1808 path produces the existing K1 12.8 kHz / 96-sample behaviour.
The governing Titan execution brief pins the Titan production candidate to
24 kHz / 180 samples. The current driver therefore proves capture and exact donor
canonicalisation, but it deliberately does **not** feed those 96 samples into the
24 kHz scalar `AudioPipeline`. Doing so would corrupt the AP rate and musical
semantics while appearing to run.

Actual AUX-to-AP integration needs a separately declared 48 kHz to 24 kHz
adapter, its independent frequency/impulse/continuity tests, host AP differential
verification, and then identified-M85 execution. The existing 4/15 donor path
remains the behavioural reference; it must not be relabelled as 24 kHz.

## Bounded physical acceptance after wiring

1. Flash the combined build through the established Titan programming/recovery
   route and verify the exact build ID and Titan UID.
2. Run `python3 scripts/run_pcm1808_target.py \
   --build docs/evidence/K1-RA8P1-002/pdm-pcm1808-build-04 \
   --output docs/evidence/K1-RA8P1-002/pcm1808-silicon-01 \
   --observe-seconds 3`. This is a bounded functional gate, not an arbitrary
   soak.
3. Require SSIE1/DTC identity, 47-49 kHz measured rate, zero overflow/idle/FSP
   errors, advancing callbacks/hops/hash, and non-zero signal energy.
4. Apply a left-only then right-only line fixture and record the physical slot
   map. The donor observed slot 0 as RIGHT and slot 1 as LEFT, but Titan must be
   measured rather than assumed.
5. Only after capture passes, implement and verify the 24 kHz adapter and run the
   real AP/VP on the identified M85. Then repeat with onboard PDM and PCM1808
   capture enabled together to establish bounded coexistence.

## Host validation

The explicit tracked host suite plus the new PCM1808 tests passed 129 tests. The
full directory-level pytest collection is currently obstructed by unrelated
untracked `tests/host/test_gold_extract.py`, which imports itself during
collection; that file was preserved and was not attributed to this port.
