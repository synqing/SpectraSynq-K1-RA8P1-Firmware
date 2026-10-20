---
abstract: "Fresh-context, read-only review of P6 v9 (image b5595c35, commits ceb51ae/8768985, evidence e9cd6fe), 14 Oct 2026. Verdict ACCEPT AS PARTIAL. The engineering is sound and honestly recorded: the freezes hold, v8 is bit-identical, and constraints a, c, f and g are met in source. But the frozen C1 measures the wrong thing. On the song file the build's peak is as bright as the drop's peaks, or brighter, and the lower board's drop is dimmer in luma. That matches the Captain's 'still a no'. No device light measurement exists, and the B colour fold is test-only."
---

# P6 v9 effect mapping: fresh review, iteration 1

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was read-only. No device, serial, USB, broker or `launchctl`; no flash, no commit. The only file written is this one.

## Verdict: ACCEPT AS PARTIAL

**In plain English.**
- **What is solid.** v9 is built the way the brief asked:
  - light is held per pixel, with no hue mixing;
  - loudness sets how bright and how wide each deposit is, and the fade speed is fixed;
  - colour moves slowly and holds when the harmony becomes unclear;
  - the AGC, the levelling stages, the palettes and the output chain are untouched.
- **v8 is truly unchanged.** The test files were frozen before any result existed, and nobody edited or widened them. Keeping v9 on the device, as the Captain ruled, is supported.
- **What is not solid: the "drop hits harder" pass is not real.**
  - The frozen C1 compares 4.5–5 s *medians* of a channel sum. The build window includes its own quiet ramp-up and fade-out, so the build's median is held down.
  - The project's own figure shows the problem directly. On the song file, the build's peak at about 47.5 s is as bright as the drop's peaks, or brighter. On the lower board, the drop is visibly *dimmer* in luma.
  - That is the Captain's "still a no". The metric is wrong, as the standing rule says.
- **The device side.**
  - There is no measurement of the light the device actually produced: no LED-payload dump and no camera capture. The only optical evidence on the device is the Captain's eyes.
  - The colour fold that made v9 pass (variant B) is a test variant under the Captain's testing ruling. It cannot become product behaviour until the Captain rules on it. STATUS currently says no ruling is needed.

**What is left:** a new drop-impact measure (proposed below, not run), a device payload capture, the B-fold ruling, and one more iteration aimed at the drop.

## Findings

Severity: H = would change a ship or keep decision; M = must be corrected before closeout; L = cosmetic or advisory.

| # | Sev | Label | Finding | Evidence |
|---|---|---|---|---|
| F1 | H | [FACT] | **C1 does not measure "hit harder".** It is a median over the build at 45–50 s and the drop at 55.5–60 s, applied to S (channel sum). The build window runs from −18.5 dBFS through a climax of −11 to −15 dBFS at 47.0–48.5 s, then into the fade (−21.7 to −27.6 dBFS at 49.5–50 s), which drags its median down. In the iteration-4 figure for the song file, B's lower-board S build peak is about 12 per-mille against drop peaks of about 11–12. Its lower-board luma is about 12.5 against about 7–8. Its upper-board S is about 11.5 against about 10–11. On peaks, the drop does not exceed the build. | `figures/iteration4/fig-boom-build-drop-4way.png` (opened); input envelope from `csv/v9_final4.boom_source.csv.gz` (`raw_rms`, 0.25 s bins); `scripts/p6_evaluate.py:48-50` (median statistic) |
| F2 | H | [FACT] | **On luma, B passes C1 in only 2 of 4 cases.** The luma ratios are 0.799 (song file, lower), 1.239, 1.089 (room, lower) and 1.589, against the same 1.122 bar. The frozen definition treats S order as equal to "matched-palette luma" order, because at a fixed chromaticity luma is a fixed multiple of S. That premise is false in C1 itself: the colour coordinate differs between build and drop. So the S-only pass hides a darker-looking drop on the lower board. The receipt does disclose this (RECEIPT:32), but STATUS:116 quotes only "×1.25–1.73 light". | `results/iteration4/eval-v9-emphasis-range.json` C1 `L_ratio`; `TOLERANCE-FROZEN-P6.json` `metrics.L_matched_palette` |
| F3 | H | [FACT] / [INFERENCE] | **The level transfer runs out of range at the drop, and the drop does not use the available headroom.** The transfer maps −9 dBFS to l = 1. On the song file, 33.6 % of drop frames sit at that ceiling (median l = 0.937, against 0.828 at the build climax). On the room recording it is 17.1 % (median 0.803). The mode-32 deposit gain is 0.396 under B, so a full-level drop deposit reaches only about 40 % of the renderer's range. Matching average light then shares a fixed budget between sections instead of letting the drop rise above the build. [INFERENCE] These two effects cap how much harder the drop can hit. | `inhouse_waveform.cpp:729-730` (level map), `:643-647` (deposit), `inhouse_waveform.h:34-35` (gains); `results/iteration4/calibration-emphasis-range.json` |
| F4 | M | [FACT] | **The device side is documented, not demonstrated optically.** What exists: b5595c35 programmed with return code 0, identity MATCH at 21:56:51 local (13:56:51Z); broker identity build = b5595c35 on UID 545433931bd25436593630352d068363; the readback compared 127 fields and found 0 differences; the BOOM cost capture has 16,197 records, 0 missing, 0 analysis24 drops, 0 skipped releases and no pair or lane faults; VP p50 fell from 195.7k to 182.1k cycles (×0.93, one run each). What is missing: there is no LED-payload or rtrace dump and no fixed-exposure camera capture (STATUS:117), so C1–C3 were never scored on device output. That breaches the standing rule "instrument, not Captain eyes". Sonic and Dopex were not run on the device. The cost report itself states `musical_verdict: UNASSESSED`. | `programme-p6-v9-001.deploy.log`; `v9-readback/events.json`; `cost-p6-v9-boom-001/report.json`; `cost-compare-p4corr-vs-p6v9.json` |
| F5 | M | [FACT] | **Readback 127/0 cannot tell v9 from v8.** The two presets differ only in `profile_version`, which is a build-time define (`test_v9_profile_selects_mapping_and_same_setters`: identical `render_composition`). So the readback matches v8's settings equally well. What shows v9 is running is the chain from build ID b5595c35 to its receipt (profile 9 define; `renderV9` and `colourV9` symbols present). The readback also shows the broker identity as `accepted: false` ("no accepted checkpoint bound"). | `v9-readback/before.json`; `tests/host/test_p6_boot_composition_v9.py`; RECEIPT:85 |
| F6 | M | [FACT] / [INFERENCE] | **The testing-ruling application was reasonable and disclosed, but its end state is not recorded.** Variants A and B were frozen at 13:38:31Z, before any variant code ran (body hash verified, mode 0444, every iteration-4 result is later). v8 and `product_palette_data.cpp` are untouched, and the variants are v9-only. Disclosure appears in RECEIPT:13 and :110-112, in commit 8768985 and in STATUS:89-91. However, B narrows palettes 32 and 39 to their bright sub-ranges, which in effect is the "brighter palette" that constraint (e) bars. It is now the build kept on the device. Receipt ruling 2 is still open: the fold maps colours c and 1 − c to the same position, halving colour variety. STATUS:119 says "RULING NEEDED: none". [INFERENCE] The ruling covers testing "for as long as" testing runs. Admitting B as product behaviour needs an explicit Captain ruling. | `P6-IT4-PALETTE-VARIANTS-FROZEN.json`; `inhouse_waveform.cpp:559-566`; STATUS:119 |
| F7 | M | [INFERENCE] | **Host-green is an in-sample result.** Four design iterations were each diagnosed on the same corpus (BOOM and Sonic are in calibration and in C1/C2). Examples: confidence was made graded; presence weighting was "chosen from the iteration-2 trace" (RECEIPT:200); the colour fold was added after iteration 3 failed. The rerun rule permits this, and every step is disclosed. Good discipline holds throughout: no criterion was edited; A was not rescued at 0.0007; B's limits sit on palette stops rather than tuned values. But no build→drop holdout remains (C1 is BOOM-only), so the pass cannot predict a new song. | RECEIPT:318-327, :194-202; commit ceb51ae message |
| F8 | M | [FACT] | **Constraint (d) is met to the letter, but weak in practice.** The tempo weight is w = 0.7·smoothstep((confidence − 0.30)/0.30), there is an onset energy layer, and there is a level → deposit path. No single tempo winner controls motion alone (`inhouse_waveform.cpp:756-773`, `:795`). But on BOOM (song file, board 1) my recomputation from `tempo_confidence` gives w at the 0.7 cap in 53 % of frames (median w 0.698), while the tracker reads 93 BPM in the drop, against a song tempo of about 138 BPM (P7). Confidence does not detect a confident wrong lock. This is an upper bound: the CSV does not carry the freshness timing. No test of (d) exists. | `csv/v9_final4.boom_source.csv.gz`; P7 STATUS:61 |
| F9 | L | [FACT] | **Constraint (b), matched average output, holds on S only.** The three variants were matched to v8's mean S (0.006181 and 0.005431) within 1e-6 relative on BOOM file + BOOM room + Sonic, with Dopex held out, in iterations 2 and 3. That is matched on channel sum, not luma. Under B, only bounded level emphasis was recalibrated and re-run; the other two variants were not re-compared, so the recommendation is carried over rather than re-tested. | `results/iteration4/calibration-emphasis-range.json`; RECEIPT:184-192, :417-425 |
| F10 | L | [FACT] | **Freeze discipline: PASS.** `freeze_tolerance.py verify` returns ok for all three frozen files (body hash, mode 0444). Their mtimes are 12:53:45, 13:11:09 and 13:38:31Z, equal to their stamps. The first results that follow each freeze are at 12:56:56, 13:13:01 and 13:39:42Z. All 148 lines of `SHA256SUMS.txt` match. | re-run in this review |
| F11 | L | [FACT] | **C4 v8 identity: PASS, method sound.** Host: frame SHA-256 equal on 10/10 inputs. ARM: f6daf7fb against 63300519, 62 differing bytes, all inside K1_BUILD_ID (538180–538244) and `__TIME__` (534816–534824); renderer object disassembly equal; both planted faults red. I re-ran `tests/host/test_p6_boot_composition_v9.py`: **8 passed** (0.58 s). Advisory: in `c4-arm.json`, `allowed_regions` lists each region twice. "Selectable" means selectable at build time: going back to v8 needs a reflash, during which the plate is dark for about 25 s. | `results/iteration4/final/c4-*.json` |
| F12 | L | [FACT] | **Constraints a, c, e (as written), f and g in source.** (a) A new deposit replaces a pixel's intensity *and* colour only where it exceeds the decayed value (`inhouse_waveform.cpp:671-674`). Release is a constant τ of 0.29 s / 0.125 s (`:622-625`). Attack is instant, the max. (c) Slow colour layer with follow, anchor and drift caps (`:574-618`); low confidence drifts toward the anchor, never toward a palette end. Under B the fold roughly doubles colour speed in palette space (×1.34 and ×1.38) and reverses direction at c = 0 and 0.5. (e) No floor: the floor exists only as a host fault (`:692-697`), and there is no runtime normaliser. (f) `git diff a36673a..HEAD -- src` touches only `inhouse_waveform.{h,cpp}`. The .cpp diff has 0 deleted lines (hunks at :85, :174, :514). Stage ④ (:133–172), `inhouse_audio.*` and `product_palette_data.cpp` are unchanged. (g) See F11. | source at HEAD e9cd6fe |
| F13 | L | [INFERENCE] | **Out of P6 scope.** Both presets and the readback show `hardware_output_brightness 255`, and the deploy log shows "output 255/255". P6 did not change this. I did not verify where the 24/255 maximum is enforced on this lane. | presets :123 |

## Required corrections (before closeout)

1. **STATUS:116 and the P6 ten-line report.** Report C1 on luma beside light: luma 0.80 / 1.24 / 1.09 / 1.59, which is 2 of 4 at the bar. Add the peak observation from F1. Do not describe the C1 pass as evidence that the drop hits harder.
2. **STATUS:119.** Replace "RULING NEEDED: none" with the open ruling: should the v9-only colour fold B (restricted range; c and 1 − c share a colour) become product behaviour, set against constraints (e) and (f)? The alternatives are A or a palette change. Keeping b5595c35 *for testing* is already ruled.
3. **RECEIPT and STATUS device lines.** State that readback 127/0 does not distinguish v9 from v8, and that v9 selection on the device rests on the build ID and its receipt (F5). Record the broker's `accepted: false`.
4. **Device evidence.** Before the next ruling, capture LED payload (rtrace) on b5595c35 for BOOM, Sonic and Dopex, and score C1–C3 and the proposed drop-impact measure on that payload (F4). The Captain is not the measuring instrument.
5. **C5 and C1 records.** Keep C1 on record exactly as frozen, but mark it **NOT VALID AS A PROXY FOR "HIT HARDER"**, because the eye disagrees (F1, F2). Do the same for C5 as NON-DISCRIMINATING.
6. **Constraint (d).** Record in the receipt that confidence-weighted tempo sits at w ≈ 0.7 on a confident wrong lock (F8), and link it to P7.

## Metric proposal: drop impact (PROPOSAL, NOT APPROVED; not run on any result)

**Why C1 fails** (F1–F3):
- it takes the median, not the peak;
- the window is fixed and includes the build's own ramp and fade;
- it uses channel sum, not luma or lightness;
- it ignores spatial extent;
- it ignores the output chain (gamma, packer, 24/255 cap, dither);
- its bar (+0.5 dB) is set far below any visible step.

**Proposed measure DI, per board, scored primarily on the device LED payload.** The host replay with the full output chain is secondary.

1. **Perceptual signal.** Per frame:
   - P(t) = mean over the 160 pixels of CIE lightness L\* computed from the *emitted* Rec.709 luma Y, after gamma, cap and dither (L\* = 116·(Y/Y_ref)^(1/3) − 16, with Y_ref = the full-scale emitted white). Report S and L beside it.
   - E(t) = the fraction of pixels whose Y is at least 25 % of that frame's maximum Y (spatial extent).
2. **Event anchors, frozen from the audio alone before scoring.**
   - Drop onset t_d: the first frame after the cut where the input envelope reaches the build-climax level. For BOOM that is ≈ 55.75 s song time on the source file.
   - Build climax window B*: the 1.5 s of highest input envelope before the cut. For BOOM ≈ 47.0–48.5 s.
   - Cut window C: 50.5–54 s.
3. **Statistics.**
   - **Impact I** = p95 of P over [t_d, t_d + 1.0 s] ÷ p95 of P over B*. This is the primary measure.
   - **Release R** = median of P over [t_d + 0.05, t_d + 0.35 s] ÷ median of P over C. This targets the "tension release" the Captain noticed.
   - **Extent X** = p90 of E over the drop ÷ p90 of E over B*.
   - **Sustain D** = median of P over [t_d, t_d + 4 s] ÷ p90 of P over B*.
4. **Pass bars come from the eye, not from the agent.**
   - Play the Captain a frozen ladder of synthetic drops on device: the same build, with drop peak light at ×1.0, 1.25, 1.5, 2 and 3 over the build climax.
   - Freeze the smallest ratio he calls "harder" as the I bar, and do the same for X.
   - Until then, the bars are TBD and the measure is report-only.
5. **Validity gates.** These must go red before the measure is trusted:
   - **The known negative:** current b5595c35 BOOM payload must score I below the bar, because the Captain said no. If it passes, DI is wrong too.
   - v8 must fail.
   - A planted flat mapping (constant deposit amplitude) must fail.
   - A planted drop accent (×2 peak at t_d) must pass.
6. **Generalisation.** Freeze a second build/drop track not used in any P6 iteration as a holdout before the next design pass (F7).

## Ship path

1. **Already on silicon and in source.**
   - On silicon: image b5595c35 (v9, variant B) runs on Titan UID 545433931bd25436593630352d068363, deployed 13:56:51Z and kept by the Captain ("do not revert").
   - In source: commits ceb51ae and 8768985; evidence e9cd6fe.
   - v8 remains buildable and bit-identical (profile 8).
2. **Remaining steps.**
   - (a) Make the STATUS and RECEIPT corrections 1–3 and 5–6.
   - (b) Capture LED payload (rtrace) on b5595c35 for BOOM, Sonic and Dopex, and score C1–C3 and DI (report-only) on it.
   - (c) Run the Captain's ladder calibration, and freeze DI's bars and the holdout track before any design change.
   - (d) Make a drop-impact iteration under that freeze. For example: move the level ceiling above −9 dBFS, or use the deposit headroom (mode-32 gain 0.396) for onset accents. Average light stays matched; no floor or normaliser.
   - (e) The Captain rules whether the B colour fold (or A, or a palette change) may become product behaviour.
   - (f) Build, identity-checked deploy (the plate is dark for about 25 s while it programs), readback, payload capture, and the Captain's verdict.
3. **Who acts.**
   - The P6 lane owner: steps a, b, d and f, plus the build for c.
   - The Captain: the ladder judgement in c, the palette/fold ruling in e, and the final visual verdict.
4. **What "shipped" means.** A device readback of the successor v9 build_id under composition v9, plus a payload-scored DI pass at the eye-calibrated bar, plus the Captain saying the drop hits harder, plus the B/A/palette ruling recorded. Only then is v9 the product boot composition and not a kept test build.

## Commands run in this review

- `scripts/freeze_tolerance.py verify` on the 3 frozen files: all `{"ok": true}`.
- A Python SHA-256 check of `SHA256SUMS.txt`: 148/148 match.
- `git --no-pager diff a36673a HEAD --stat`, `-U0 -- inhouse_waveform.cpp`, and the name-only list for `src`.
- `python -m pytest -q tests/host/test_p6_boot_composition_v9.py`: 8 passed.
- Read-only analysis of the existing `csv/v9_final4.boom_{source,room}.csv.gz`, input side only (raw RMS, the level transfer, tempo weight). No new light metric was computed on the results.
- Opened `figures/iteration4/fig-boom-build-drop-4way.png`.

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (fresh-context P6 reviewer) | Created. Verdict ACCEPT AS PARTIAL. |
