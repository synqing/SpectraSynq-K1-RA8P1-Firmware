# P4 — ANALYSIS RATE — RECEIPT, iteration 1

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. The Titan now runs **`f2f1983a`**. It runs v8 autonomously: readback shows 127 fields with 0 differences, at 100 frames/s with 0 LED faults and emission on. The recovery image `a8215f75` is retained.

## In plain English

The Titan's own sound analysis now runs at 24 kHz instead of 12.8 kHz. All 80 frequency bands, from 110 Hz to 10.5 kHz, are now real. Before, the top nine (above 6.4 kHz, the hi-hat range) were always zero.

The older donor analysis is unchanged. It still runs at 12.8 kHz and only decides when the boards may light.

A host gate was written down before any result:

- It failed on its first run. A real timing bug in the new resampler meant its samples were labelled 2.35 ms later than the old path's.
- The bug was fixed and the same gate was re-run. It passed: the features agree with the old path at median r 0.999, the new hi-hat bands follow the music's high band, and level and tempo agree.

On the device, the new analysis costs about 2.6× the cycles of the old one: 0.72 ms per 7.5 ms hop. It stays far inside the 5 ms deadline, and nothing else got slower.

Two further plan items were measured on the device:

- **CMSIS-DSP's vector dot product.** It links and runs, but it is slightly slower here (+2.6 %), so it stays an option rather than the default.
- **Moving the analysis and scene state into fast DTCM memory.** It is faster beyond run-to-run noise: frame rendering −4.5 % median, −6 % at its worst; analysis −2.4 %. So it is kept.

No fault, loss or skipped frame occurred in any of the five BOOM captures.

## What changed (lane commits 21305dc … 98d705e)

| Item | Where | Evidence |
|---|---|---|
| `InhouseAudio` analysis-rate option: 12.8 kHz / 96 (unchanged default) or 24 kHz / 180 | `platform/ra8p1/inhouse_audio.{h,cpp}` | 12.8 kHz replay of BOOM song and room is **byte-identical** before and after (sha256 `a04950f6…`, `9cbb81b7…`) |
| `k1_poly24` stream clock: output m names source sample m·step − 47, the same as the 12.8 kHz FIR | `k1_poly24.{h,c}` (pre-roll 94 zeros) | Stream-clock test exact to 1e-6 over 556 checks; planted one-sample error goes red (`tests/host/test_poly24.py`, 11 pass) |
| Capture path feeds `k1_poly24` beside the donor ASRC | `pdm_target.{h,c}` | Own epoch and sequence. On overflow, only this path is flushed, with no redesign. Fed only while the in-house analysis is wanted. 24 kHz descriptor back-projected from the newest DMA receipt. |
| Live runtime drains 24 kHz descriptors into `InhouseAudio` at 24 kHz | `k1_live_runtime.{h,cpp}` | Device MIR: `contract sr24000.hop180.bins80.note12`, valid, sample_end = generation × 180 |
| Build flags `--inhouse-analysis24`, `--poly24-cmsis`, `--analysis-dtcm` | `scripts/build_scalar.py`, `SConscript` | Each enters the build identity only when set. Product guards: 64 pass. |
| CMSIS-DSP 1.16.2 `arm_dot_prod_f32` | Extracted at build from the FSP pack (sha256 `76380258…`), never vendored | Linked (map), 16 q-register `vfma`. Host CMSIS path matches the plain loop to ≤ 1 LSB, with a red check. |
| Analysis and scene state in DTCM | `.dtcm_from_flash` (`.dtcm` is NOLOAD zero-fill in `fsp.ld` and would drop initial values) | Exact-content guard (`verify_analysis_dtcm`): planted extra object, missing object and out-of-range region all go red (4 tests). The tempo verifier accepts only declared regions. |

## Host gate (frozen before results)

`TOLERANCE-FROZEN-analysis24.json` (sha256 `7841d4fb…`) was frozen before the 24 kHz path or its probe existed, and was never edited.

- **Run 1: FAIL** (`analysis24-gate-run1-FAIL.json`). Gate (i) failed: median r 0.80, minimum 0.48.
  - **Diagnosis:** against an exact DFT of the 40 kHz source, the 12.8 kHz path matches at a 1.2 ms delay (r 1.000), and the 24 kHz path matches at a 1.2 ms **lead** (r 0.98–1.00). The streams' sample indices named moments 2.35 ms apart.
  - **Fix:** a resampler pre-roll aligning it to the 47-sample causal convention. This was an implementation fix; the tolerance was not touched.
- **Run 2: PASS** (`analysis24-gate-run2-PASS.json`). All planted faults were caught.

| Gate | Result |
|---|---|
| (i) bins below 5,120 Hz, song / room | median r 0.999 / 0.998, minimum 0.919 / 0.916 (limits ≥ 0.97 / ≥ 0.90) |
| (ii) bins above 6,400 Hz | all non-zero; sum against the 6.5–11 kHz source envelope r 0.785 (≥ 0.60) |
| (iii) level streams, three inputs | raw_rms / VU ratio 1.000–1.003, ρ ≥ 0.999; peak_scaled ρ ≥ 0.994; silence agreement 100 %; Dopex post-roll silence onset identical (79.95 s) |
| (iv) tempo, 33–43 s | song 138 / 138; room 140 (12.8 kHz) against 138 (24 kHz), \|Δ\| **2.0, exactly at the limit** |
| Planted faults | 5 % rate error: (i) and (iv) red. Upsample: (ii) red. Gain ×1.25: (iii) red. |

Committed copies are in `docs/evidence/P4/`.

**Pre-existing and disclosed:** `tests/host/test_inhouse_audio.py` fails on a clean HEAD, before any P4 change. The pinned external reference `SpectraSynq_K1_Firmware/.../k1_gdft_core.cpp` changed identity. Its strings targeted by mutation tests were preserved.

## On device: BOOM 0–75 s, scored 28–75 s, cycles at 1 GHz

| Image | Content | In-house p50 / p99 / max | VP (frame render) p50 / max | Whole audio stage max | Records, missing, counters |
|---|---|---|---|---|---|
| `a8215f75` (P2 final) | 12.8 kHz analysis | 270,520 / 545,024 / 550,811 | 205,850 / 298,587 | 845,794 | 16,198, 0, none |
| `5f1483e8` run 1 | 24 kHz, plain loop | 715,662 / 991,461 / 997,068 | 205,279 / 299,891 | 1,292,922 | 16,195, 0, none |
| `5f1483e8` run 2 | same | 717,724 / 993,540 / 998,204 | 205,363 / 299,511 | 1,292,189 | 16,194, 0, none |
| `a330db70` | + CMSIS dot product | 734,216 / 1,010,072 / 1,015,144 | 205,612 / 295,887 | 1,308,426 | 16,197, 0, none |
| `f2f1983a` run a | + DTCM (installed) | 700,131 / 991,815 / 996,299 | 196,054 / 281,160 | 1,290,736 | 16,194, 0, none |
| `f2f1983a` run b | same | 699,959 / 991,371 / 996,597 | 196,668 / 280,716 | 1,292,460 | 16,197, 0, none |

- **Unchanged from P2 final:** capture-poll max (about 162–171 k), donor (about 287 k), queue occupancy (p50 355, max 503), LED packing (about 129 k).
- **Brief's "repeat P2 measurements":** done. The whole audio stage peaks at 1.29 ms against the 5 ms hop deadline.
- **Run-to-run noise:** the same image differs by 0.04–0.3 % at p50.
- **CMSIS:** +2.6 % in-house against the plain loop, so it is not adopted.
- **DTCM:** below both plain runs on every compared stage, so it is kept.
- **Device proof of the new binary:** MIR `contract sr24000.hop180.bins80.note12`, `valid true`, `sample_end` an exact multiple of 180 (`p4-001-mir-snapshot.json`). Only the new code reports this.

## Deploys, dark intervals and one stop

| Image | Programme start → identity | Light out (approx.) |
|---|---|---|
| `5f1483e8` | 18:43:28 → 18:44:14 | 46 s |
| `a330db70` | 18:53:32 → 18:54:19 | 47 s |
| `f2f1983a` | 18:59:44 → 19:00:25 | 41 s |
| `5f1483e8` re-flash | see the stop below | 37 s + reset |
| `f2f1983a` final | 19:11:24 → 19:12:05 | 41 s |

All times are local (+08:00).

**Stop at 19:06 (re-flash of `5f1483e8`):**
- **What happened.** Programming returned rc 0, but identity was MISMATCH: the post-info step found no Titan. The Titan's CDC device (045b:5310) was in the USB registry with no serial port attached. The broker was in an exit loop on the old admission.
- **Action taken.** One SWD signature-and-run reset at about 19:07 (UID matched, probe power off). The serial port then attached, and the device reported `5f1483e8` at 99.7 frames/s with 0 LED faults.
- **Ruling.** The PLAN stop condition, "no USB serial link after one reset", was not met, so no recovery flash was needed.
- **Erratum (lane owner, after receipt):** this is a recurrence of the known defect **D1** (P1 receipt: "device enumerated, interfaces not attached, no /dev node", 2 of 15 debugger resets, the next reset recovers, the show is unaffected). It is not a Mac-side glitch, as first written in STATUS at 11:08Z. D1 root cause is still open. This is its first sighting after a programming reset rather than a debugger reset.
- **Not observed.** Whether the LEDs were lit between 19:05 and 19:07: there was no serial link and there is no camera. [INFERENCE] The boot-on firmware was visualising autonomously.
- **Afterwards.** The admission stayed on `f2f1983a` until the final deploy, so the plain image's second readback timed out on admission; its capture ran directly. Restored by the final deploy (ready True).

## Plan items

1. **Dual-output resampler. Done.** The 12.8 kHz donor output is unchanged. `k1_asrc_24k.c` has no diff; the donor ASRC push path is untouched. Bit-exactness of the donor stream is **documented from source, not demonstrated** on device.
2. **In-house analysis at 24 kHz. Done.** Contract string `sr24000.hop180.bins80.note12`. Thresholds carry over (gate iii).
3. **Tolerance frozen before results. Done.** Run 1 FAIL, run 2 PASS, both disclosed. Gate (iv) room sits exactly at its limit.
4. **Build, flash, readback, BOOM capture, repeat the P2 measurements. Done** (above).
5. **CAC MOCO measurement. Deferred.** It changes the boot path; the plan says to include it only at no risk.
6. **Receipt and fresh-context review.** This receipt; review pending.
7. **DTCM comparison. Done**, two captures per image. Kept.
8. **CMSIS link. Done.** Linked, host gate green, symbol in the map, measured on device. Not adopted, because it is slower.

## Open questions and limits

- **[FACT]** The tempo tracker still reads half-time and two-thirds tempo (P3). 24 kHz does not change that (gate iv).
- **[INFERENCE]** Whether the hi-hat bands visibly change the v8 look is the Captain's call. The bands are now real input to the colour (chroma) and novelty. Agent cannot listen.
- **Optimisation headroom.** GDFT at 24 kHz is about 1.9× the work. A vectorised GDFT is the next lever if headroom is needed. Today it is not needed.
