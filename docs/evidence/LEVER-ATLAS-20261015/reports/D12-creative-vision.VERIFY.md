---
abstract: "Adversarial verification of D12 creative-vision (15 Oct 2026). The D12 report itself was never written to disk, so only the factual anchors in concept_sketch.py, its figures, and the brief facts it depends on could be checked. 22 claims checked: 17 confirmed, 5 refuted or corrected."
---

# D12 creative vision: verification

## What happened, in plain English

The D12 report was never saved. The harness refused to let the agent write REPORT.md, and the text went back only to the orchestrator. This verifier could not see it. So its concept claims (C01–C19), scores and cycle estimates are **unverifiable here**. The orchestrator must check them against the structured result it holds.

What could be checked was the sketch script. Its source citations are correct, line for line. It runs and reproduces its pictures byte for byte. Four facts it leans on from the brief are wrong or overstated. Its drop-bundle concepts also overlap two drop levers that already exist in the Titan code and are switched off. Any claim that those ideas are new or untested is wrong.

## Checked claims

Product tree: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `1ef53952`. Paths below are relative to it unless absolute.

| # | Claim (source) | Verdict | Evidence |
|---|---|---|---|
| 1 | Mode 32 transport is `inhouse_waveform.cpp:972-1022` (sketch docstring) | CONFIRMED | :972 `if (mode32) {`, :1022 `depositV9(...)` |
| 2 | Deposit at `80 + 0.7*peak*80` | CONFIRMED | `src/k1/core/visual/inhouse_waveform.cpp:976` |
| 3 | Outward scroll 405 px/s | CONFIRMED, with a caveat | :967 `rate = 405.0F * motion`; the sketch drops the `motion` control factor |
| 4 | At most 8 scroll steps per frame | CONFIRMED | :968 `maximum_steps = 8U` |
| 5 | Two-stage peak smoothing, 16 ms then 23 ms | CONFIRMED | :974-975 |
| 6 | "Release tau 0.29 s" | PARTLY WRONG (naming) | 0.29 s is the **intensity fade** (`fadeV9`, :801). The product's separate *extent release* is `InhouseV9Release` (`inhouse_waveform.h:64-76`). It has `release_s = 0.6` and `enabled = false`. |
| 7 | Deposit amplitude `0.396 * level^1.5` | CONFIRMED | `kInhouseV9Gain32 = 0.395878F` (.h:35), emphasis 1.5 (.h:50), `kLevelEmphasis` (.h:34), cpp:837 |
| 8 | Deposit half-width `0.5 + 3.5*level` | CONFIRMED | cpp:826 |
| 9 | Mirrored about pixel 79/80 | CONFIRMED | cpp:881, :888 |
| 10 | Extra 10/s drain in confirmed silence | CONFIRMED | cpp:801 |
| 11 | Palette id 38 `K1_Crimson_Gold_gp` stops at `product_palette_data.cpp:1084-1095` | CONFIRMED, all 10 stops exact | `src/k1/core/visual/product_palette_data.cpp:1084-1095`; registry entry :1301 |
| 12 | Path P colour is `wrap(offset + d)`, spread k = 1 | CONFIRMED, with a simplification | cpp:888-889. The sketch uses one global offset (0.45) per frame. The product keeps a per-pixel `offset_[]` that scrolls with the trail. |
| 13 | Sketch numbers: A 1.14 / 0.81, B 9.29 / 1.78, C 1.08 / 1.00 | REPRODUCED | Re-run in `verify_rerun/` with `/usr/bin/python3`. `kymographs.png` md5 `c27a35b9…` is identical to the original. |
| 14 | "Baseline drop is no larger than late build" | CONFIRMED on real data, not only in the sketch | `docs/evidence/P6/REVIEW-ITERATION-1.md:18,34` (F1): on peaks, v9's drop does not exceed the build |
| 15 | In-house analysis is blind below 110 Hz (brief) | REFUTED as stated | Only the note bank starts at 110 Hz. A 40–110 Hz `sub_rms` already exists (`src/k1/core/audio/sub_band.h:3-18`). It is published at `platform/ra8p1/inhouse_audio.cpp:240`. It is used only by the release trigger, which is off. |
| 16 | Three drop detectors "failed to fire within +50 ms" (brief) | REFUTED as stated | v1 hit BOOM at 55.74 s (`runs/P6-DROP-DETECTOR/DETECTOR.md:56`). v2 hit Carte Blanche at 27.45 s (`v2/DETECTOR.md:11`). v3 had no hits. No detector passed the full set. |
| 17 | Tempo half-time lock comes from an 88 BPM prior; downbeats 26–380 ms off | CONFIRMED in part | `docs/evidence/P7/RECEIPT.md:19,105`. −380 ms is at `v3/DETECTOR.md:45`. The 26 ms figure was not found. Sonic Samba locks at 71 BPM (`v3/DETECTOR.md:9`), not 88. 88 is the prior's centre. |
| 18 | About 16 ms typical latency from sample to wire end | CONFIRMED | `docs/evidence/P5/device-budget/BUDGET.md:15`: 16.4 ms |
| 19 | LED wire time 5.1 ms per frame | CONFIRMED | `BUDGET.md:13`. Arithmetic: 80 × 48 bits × 1.25 µs + 300 µs = 5.1 ms |
| 20 | "120 Hz logical render target" (brief) | REFUTED for the measured device | `BUDGET.md:13`: the picture starts on a **10 ms tick**, so the cadence is 100 Hz. The sketch's `FPS = 100` matches the device. The brief's 120 Hz does not. |
| 21 | Captain: v9 is "overall superior to v8" | CONFIRMED | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/Kx-Titan/docs/migration/MUSIC-VISUAL-VALUE-TAKEOVER-2026-10-08.md:1904` |
| 22 | Brightness 24/255 conflicts with the Kx-Titan default | CONFIRMED conflict, reported and not resolved | `Kx-Titan/AGENTS.md:12`: "default brightness 100/255 supersedes historical 24". `~/.claude/CLAUDE.md` says to preserve 24/255. |

## Origin flags (S3 budget or aesthetic, asserted without evidence)

- [FACT] **The drop bundle reinvents existing switched-off levers.** Bundle B (C01 spring, C12 iris, the release pulled to origin) overlaps two existing structures:
  - `InhouseV9Release` (`inhouse_waveform.h:64-76`): surge release, `trigger_sub` on the 40–110 Hz band, a dip gate that needs a cut first, and a pull to origin at cpp:977.
  - `InhouseV9Accent` (.h:52-61): drop boost and widen, keyed to low-band onset density.

  Both came from frozen P6 design candidates (`P6-EXT-DESIGN-CANDIDATES-FROZEN.json`, `P6-IT6-DESIGN-CANDIDATES-FROZEN.json`). Both are off by default. Their origin is **tested and not shipped**, not the S3 budget. D12 must cite them and explain why they were left off before it claims novelty.
- [FACT] The deposit gain 0.395878 was **calibrated** to match v8's mean light (.h:31-33). That makes it an aesthetic calibration, not an S3 budget limit.
- [FACT] The 0.29 s fade, the 405 px/s scroll and the 8-step cap are commented as "v8's" (cpp:963, :800). They are **inherited**. No source found ties them to the S3 budget. Any D12 lever that labels them "S3 budget" is unsupported.
- [INFERENCE] **Bundle B's 9.29× drop/build ratio is not evidence of lift.** The model sets it by hand:
  - The iris holds the build back by up to 55 % (`concept_sketch.py:93`).
  - Drop amplitude is multiplied by 2.2 (:116).

  Most of the gain comes from dimming the build, as the contrast plot shows: the B build is about 0.05 against A's 0.12. Whether a dimmer build is acceptable is the Captain's taste call. It also conflicts with his description of a visible, contracting build.
- [FACT] With path P wrapping and palette 38's near-black ends (stops 0 and 255 are both (12, 0, 10)), a fixed-radius dark seam appears. The sketch shows it near pixels 35 and 124. The same wrap is in the product (cpp:889), so this is a real product colour lever, not a sketch artefact.

## Visual inspection

I opened `kymographs.png` and `contrast.png`. Both are clearly labelled synthetic. Panel B shows the front leaving the centre at the drop, and a dimmer build. Panel C's horizontal banding is the sub warp, applied for the whole build up to 4 s. The figures match the code. They are schematic, not renders of the Titan.

## Not verifiable here

The D12 report text: concept list C01–C19, impact, novelty and effort scores, any M85 cycle costs, and any quotes of the Captain beyond the brief.
