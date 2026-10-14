# P4 — ANALYSIS RATE — RECEIPT, iteration 2 (required corrections 1–3, plus F9)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Installed image: **`f6daf7fb`**, the correction image, deployed 13:52:13Z. It runs v8: readback 127 fields with 0 differences, 99.8 frames/s, 0 LED faults, emission ON. **All frozen device criteria D1–D5 pass** (section "Device" below). The earlier G10 USB absence is recorded in `runs/G10-20261014T1215Z/permitted/RECORD.md`.

This receipt amends `RECEIPT.md` (iteration 1). It does not replace it. Criteria for the new checks were frozen first: `TOLERANCE-FROZEN-corrections.json`, 2026-10-14T12:11:59Z, body sha256 `8780cd10…`, mode 0444.

## In plain English

- **The startup stall was real, and larger than the review estimated.**
  - The device already records the longest single capture poll since boot. On both P4 images it was about **8.66 ms** (8,655,499 and 8,664,475 cycles), longer than one 7.15 ms microphone slot.
  - It caused no capture reset on those boots (donor epoch 1). Under the review's criterion it fails all the same.
  - So the fix removes the stall rather than measuring it. The part of the filter design that does not depend on the clock rate is now a table computed once, offline. The device computes only the rate-dependent part.
  - The new filter matches the old one to one part in a million, and the frozen P4 host gate passes unchanged. Whether the poll now stays under 7.15 ms is a **device** check that has not run yet.
- **The new path's loss counter and epoch are now reported.**
  - They appear in the device's MIR snapshot, along with how often the filter was designed and what each design cost.
  - The resampler's own cycles are now in every hop record.
  - Whether loss is zero on the device has not been run yet. Until it has, iteration 1's "no loss" covers only the counters that existed.
- **The 28.8-hour timing bug is fixed.** The arithmetic is now modular, and a long-index host test covers it. That test goes red when the old arithmetic is planted back.
- **Five text corrections to iteration 1 are below.**

## Correction 1 — the 24 kHz path's counters (review F1)

| What | Where | Status |
|---|---|---|
| `analysis24` object in MIR: `wanted`, `epoch`, `sequence`, `drops`, `source_hz`, `designs`, `design_last_cycles`, `design_max_cycles` | `pdm_target.{h,c}` `k1_pdm_target_analysis24_status`; `k1_live_runtime.cpp` `mirJson` | configured; **device not run** |
| poly24 push + pull cycles per donor hop, in the v2 timing record's spare u16 as cycles/16 (saturating at ~1.05 M) | `k1_live_runtime.cpp`; decoder `live_protocol.unpack_timing` key `a24_cycles_div16` | configured; **device not run** |
| Capture tool reads MIR before and after, and reports drops/epoch/designs deltas and an `a24_cycles` distribution | `scripts/titan_cost_capture.py` | configured |

## Correction 2 — the rate-lock design stall (review F3)

- **Disclosure.** PLAN step 1 said "designed offline". The iteration-1 image designed on the device, at reset and at every `set_rate`. That plan deviation was not stated in iteration 1.
- **Existing measurement** (from snapshots already on record, read before this change). Since-boot `capture_service.poll_max_cycles`:
  - 8,655,499 on `5f1483e8` (`p4-001-mir-snapshot.json`);
  - 8,664,475 on `f2f1983a` (`/tmp/titan-emotiscope-control/mir-snapshot.json`, 19:00 local);
  - both against a 7,150,000 limit. [INFERENCE] This is the design, because the in-window poll max on the same images is about 0.17 M. The direct design-cycle counter in `f6daf7fb` will demonstrate or refute that attribution.
- **Fix.**
  - The Kaiser window (rate-independent) is generated offline: `scripts/gen_poly24_window.py`, which writes `platform/ra8p1/k1_poly24_window.h`, 49.5 kB of flash at `0x0208c770`. Its `--check` guard goes red on a planted edit (H5).
  - The sinc uses a rotation recurrence from each row's centre tap: 2 `sinf`/`cosf` per phase row instead of per tap, with the exact angle in the denominator.
  - The filter still follows the measured rate.
- **Host results (frozen H1–H5):**
  - **H1:** max |coefficient − previous formula| = 1.01e-6 at 36,000 Hz, 5.96e-7 at 40,000, 8.94e-7 at 41,417 and 8.94e-7 at 44,000 (limit 2e-6); step_q32 identical. Host design time: 23–54 µs against 186–193 µs.
  - **H2:** the frozen `TOLERANCE-FROZEN-analysis24.json` gate (sha `7841d4fb…`) re-run with the new design: **PASS**, all four gates and all planted faults (`analysis24-gate-run3-corrections-PASS.json`). Every value moved by at most about 1e-5; gate (iv) is identical. This run started from an accidental `--help` invocation (the script takes no arguments); its output is kept as the H2 record.
  - **H3:** `test_poly24.py` passes. One test harness copied only `.c`/`.h` into a planted directory and now also copies the window header; its assertion is unchanged.
  - **H5:** pass, with the planted edit red.
- **Device (D1, D2):** not run.

## Correction 3 — receipt text (iteration 1)

1. **"Nothing else got slower" is withdrawn (F2).** Capture-to-publish delay (`publish_time_us − capture_time_us`, BOOM 28–75 s, recomputed from each `timing.jsonl` by the lane owner):

   | Image | p50 | p99 | max | hop_dt p50 |
   |---|---|---|---|---|
   | `a8215f75` P2 final | 5,465 µs | 9,043 | 9,589 | 6,617 |
   | `5f1483e8` runs 1 / 2 | 6,017 / 6,027 | 9,561 / 9,568 | 9,763 / 9,940 | 6,842 / 6,845 |
   | `f2f1983a` runs a / b (installed) | 6,017 / 5,998 | 9,575 / 9,572 | 10,208 / 9,984 | 6,843 / 6,842 |

   The installed image publishes **+0.53 to +0.55 ms later at p50** than P2 final. [INFERENCE] The in-house work sits before publication completes. This is inside every deadline, and it is a latency increase in a lag-free programme. The lane owner's figures differ from the review's (5,998–6,046 µs) only by row selection; both give about +0.55 ms.
2. **"All 80 bands are now real" is host-demonstrated (F5).** The device exposes no per-bin data. On the device it is inferred from the contract string, `valid`, and the ×2.6 cost signature.
3. **Gate (i) was narrowed at freeze, before results (F6).** The PLAN said bins below 6.4 kHz; the frozen gate tests the 67 bins below 5,120 Hz, because of the 12.8 kHz path's anti-alias transition band. Transition-band r (bins 67–70): song 0.71, 0.61, 0.57, 0.59; room 0.60, 0.58, 0.58, 0.67.
4. **The 19:06 stop (F4).**
   - **Which clauses were applied.** The plan's stop has two clauses: "image fails to identify" and "no USB serial link after one reset". The post-programme identity was `MISMATCH no-info`, which is the first clause. The lane owner applied the plan's one-reset allowance to both clauses: after one SWD reset, identity matched, UID was checked and the probe was powered off. This reading is stated here because iteration 1 ruled only on the second clause.
   - **The interval.** 37 s known dark (programming, 19:04:22–19:04:59), plus about 3 minutes with the LED state **unobserved** (19:04:59 to about 19:08). It is not "37 s + reset".
5. **Stale group-delay field (F10).** `poly24-measured.json` says 48 source samples. That value was hand-entered before the pre-roll fix and is kept unchanged as history. Corrected and re-measured with the new design: `poly24-measured-corrections.json` gives 47 samples (1.135 ms at 41,417 Hz). Passband, top-bin and alias figures are equal within 0.004 dB / 0.1 dB.

## F9 — 2³² index wrap

- **Fix.** `k1_poly24_capture_lag_q32` computes newest − (base + content) modulo 2⁶⁴ in Q32 and rejects a negative lag. `k1_poly24_last_output_source_q32` clamps only inside the pre-roll.
- **Test (H4).** Exact to the Q32 bit at newest/content indices near 2³², 2³³ and 2⁴⁰, and on long-stream chains (consumed up to 2⁴¹). Content after newest is rejected. A planted pre-fix copy of the product function **goes red**. File: `tests/host/test_poly24_corrections.py`, 17 pass.

## F7 (optional)

The donor-stream max-difference check promised in the frozen file is **not demonstrated** on the stream itself.
- **[FACT]** The donor resampler's source is byte-identical before and after P4. Git blobs from `cfdfdaf` to `HEAD`: `k1_asrc_24k.c` `6b32e74cf23c`, `k1_asrc_24k.h` `ae7bae613038`.
- The only P4 change on the donor's input side is in `pdm_target.c`: the poly24 push reads `capture_pcm` after the donor push.
- `pdm_target.c` depends on FSP DMA/PDM types and cannot be built on the host. So a stream-level host check would test a re-implementation, not the product path.
- [INFERENCE] The donor stream is unchanged. The device-level witness is P4 gate (iii)/(iv) agreement plus unchanged donor cycles.
- The promise stays in the frozen file, which is never edited; this is recorded as not met.

## Commits

`1784b06`: code, guard and tests. Host suites: 112 passed (poly24, corrections, DTCM guards, product guards, tempo placement, freeze tool).

## Device: run 14 Oct 2026, 13:49–13:58Z, against frozen D1–D5 (all PASS)

**Deploy.**
- `f6daf7fb` was programmed through `deploy_titan_image.sh` (`programme-p4-corr-005`). The interpreter was overridden to `TITAN_DEPLOY_PYTHON=python3.13`, because Homebrew 3.14's `pyexpat` fails to import.
- Identity MATCH at 13:52:13Z; 99.8 frames/s, 0 LED faults, emission ON.
- **D1 recurred after every programming or SWD reset in this session's cabling** (E2 Lite on the same hub as the Titan). Two `-run -sig` resets did not clear it. Each time, a power cycle of the Titan's hub port (`uhubctl -n 05e3:0610 -p 4 -a cycle`) cleared it.
- Dark intervals:
  - about 33 s while programming (21:49:12–21:49:46 local);
  - then D1, with light unobserved, until the 21:51:58 programme;
  - about 49 s for the final deploy (21:51:24 to identity 21:52:13).

**Readback.** v8 profile `a166a3fb…`: **127 fields, 0 differences** (`corr-readback/`).

**BOOM 0–75 s capture** (`cost-p4-corr-boom-001/`): 16,200 records, 0 missing, no fault, loss or skip.

| Frozen | Result | Verdict |
|---|---|---|
| D1 stall | since-boot `poll_max_cycles` **808,555** (was 8,655,499 / 8,664,475); donor `stream_epoch` 1 | **PASS** (< 7,150,000) |
| D2 design cost | `designs` 3 since boot, `design_max_cycles` **660,956**, last 616,271; `source_hz` 41,415 | reported. It demonstrates the attribution: the remaining design costs 0.66 M cycles |
| D3 24 kHz loss | `drops` 0 since boot; over the capture, drops Δ 0, epoch Δ 0 (epoch 2), sequence 5,555 → 16,362 | **PASS** |
| D4 resampler cycles | poly24 push + pull per donor hop, BOOM 28–75 s: p50 286,032, p99 293,648, max 295,952 (mean 191,371; about one hop in three carries no pull) | reported |
| D5 no regression vs `f2f1983a` (`cost-compare-dtcm003a-vs-corr001.json`) | in-house p50 700,131 → 699,507 (−0.09 %); VP 196,054 → 195,723 (−0.17 %); emit 127,879 → 128,075 (+0.15 %); queue p50 355 → 355; no non-zero counter delta on either run | **PASS** (±3 %) |

**Correction 1 is now demonstrated on the device.** "No loss" covers the 24 kHz path: 0 drops since boot, and no epoch change in the capture. The plan's "ASRC-24 cycles" are now measured on their own (D4).

**Correction 2 is demonstrated on the device.** The longest poll since boot fell from 8.66 M to 0.81 M cycles, under one PDM slot, with no capture reset.

**Installed now:** `f6daf7fb` (P4 corrections). Recovery images: `f2f1983a`, `a8215f75`.
