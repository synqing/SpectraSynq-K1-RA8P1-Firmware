# Decision — Titan audio-to-light ceiling (canon)

Date: 2026-09-10 · Authority: Captain · Lane: K1-RA8P1 (Titan Mini) · Status: CANON

## Ruling

The Titan Mini lane accepts an audio-to-light turnaround **no worse than the shipping ESP32-S3
as measured on 2026-09-10**. Ceiling: **p50 ≤ 12,000 µs**. Captain: "use that as the ceiling of
what we're going to accept for the audio to light target."

## Definition (equal-method to the S3 evaluation)

- start `newest_sample_estimate_us` — capture DMA block return of the hop's newest sample
  (S3 convention `I2S_DMA_RETURN_ESTIMATE_V1`)
- end `emit_complete_us` — LED transfer-complete interrupt of the first frame carrying the
  published AP generation (S3: RMT `on_trans_done`)
- terms reported separately: analysis (start → `ap_publish_us`), wait (`ap_publish_us` →
  `render_start_us`), render, wire (`emit_submit_us` → `emit_complete_us`)
- conditions: music through the microphone; ≥ 1,000 joined frames on Titan (S3 baseline used
  43); observer off; configuration vector recorded (cache, TCM use, FPU mode, clock, code/data
  region, lanes, pixels per lane, LED part and bits per pixel)
- excluded from the traced figure: microphone / PDM / ADC decimation group delay and LED latch.
  These are measured and reported separately per platform. A Titan front end that adds delay
  the S3 does not is a regression even when the traced figure passes.

## Bounds

| Bound | Value | Status |
| --- | ---: | --- |
| p50 | ≤ 12,000 µs | CEILING — Captain 2026-09-10 (S3 measured 11,576) |
| p99 | ≤ 15,444 µs | no-regression companion (S3 measured) — PROPOSED, awaiting stamp |
| AP service p99 | ≤ 8,000 µs | remains in force (`G2_SERVICE_8000_2026-08-16`) |

## S3 baseline (cited, not yet receipt-bound)

Bench B489A500, 150 px per lane on 2 RMT lanes, music through the microphone, 43 joined frames:
analysis 4,742 / 7,144 µs; wait 2,240 / 5,269; wire 4,594 / 4,658; frame interval 5,229 µs
(191 FPS); **total 11,576 / 15,444 µs (p50 / p99)**. Receipt path and sha256: TO BE BOUND — the
capture is not yet under `docs/forensics/runtime-evidence` in `SpectraSynq_K1_Firmware`. Until
bound, this baseline is a cited figure, not a receipt.

## Consequences for the lane

- New gate **K1-L** (audio-to-light, this definition) joins the lane matrix. K1-A's "fresh
  comparable S3 campaign" is satisfied for this term by the bound baseline above.
- DualMCU `K1-DM-070` (acoustic reference → photodiode < 8,000 µs on 100 samples) is not this
  definition and is not reachable for reactive onsets; it is to be rewritten against this canon
  plus a separate physical (acoustic → photodiode) cell.
- The resident schedule's 7,500 µs per-hop deadline stays as the lane-A period. Fitness against
  this canon is measured end-to-end; it is not inferred from hop deadlines.

Related: K1 AP Requirements v0.2 (R2.1a / R2.1b); `docs/evidence/K1-RA8P1-002/STATUS.md`;
S3 contract `K1_SCHEDULING_GATE0_2026_08_15`; `K1 Twelve-Millisecond Plan` (artifact).
