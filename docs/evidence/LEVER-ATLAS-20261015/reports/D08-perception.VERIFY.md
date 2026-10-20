# D08-perception: adversarial verification

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.**

Verifier: Opus 5.5, 15 Oct 2026, about 01:30–01:40 AWST. Read-only. No device, serial, USB, flash or commit.

## Summary

The D08 report holds up well. I checked 25 decision-relevant [FACT] claims against the source. 22 are confirmed as written. One is now out of date because the header was edited while I was verifying. Two claims in the Opus addendum are overstated. All the arithmetic re-runs to the same numbers.

The weakest part of the report is the "origin" field. Five levers call their setting "inherited" or "aesthetic" without citing any lineage. None of the 18 levers shows that a setting was an S3 compute-budget trade-off. That is the distinction the Captain asked for.

## Source identity

- [FACT] Worktree HEAD `1ef53952ab7a50af38414321c8a91e515744098a`.
- [FACT] At the start of verification:
  - `inhouse_waveform.h` sha256 was `88cea47b…`.
  - `inhouse_waveform.cpp` sha256 was `0be2360e…`.
  - Both match the hashes the report recorded.
- [FACT] **At 01:38:14 AWST, during this verification, another agent edited `inhouse_waveform.h` again.**
  - The new sha256 is `79459ef9e448850e9a1b4818a67ec115ae17e89551ca39cd13a7e5138eccd165`.
  - `.cpp` is unchanged at `0be2360e…`.
  - The uncommitted diff against HEAD now changes the v9 gains and the palette default, and adds `linear_shape` and `shape_`.
  - Header line numbers from line 35 onward move down by 3. For example, the palette default is now at H:50.

## Confirmed (22)

| # | Claim in REPORT.md | Evidence I read |
|---|---|---|
| 1 | Both source hashes match the recorded identity | `shasum` at start of verification |
| 2 | `outputChannel()` gives a Q16→u16 clamp, W:37–39 | W:37–39 |
| 3 | Mode-32 cascade of 16 ms and 23 ms (v8 W:203–204; v9 W:974–975). The step reaches 10/50/90 % at 10.23 / 32.50 / 76.22 ms | Source lines read; my independent Python recompute agrees exactly |
| 4 | v9 fade uses 290 ms (mode 32) and 125 ms (mode 18), plus 10/s in silence, W:800–807. 95 % decay takes 869 / 374 ms, or 223 / 166 ms in silence | W:801; recompute 868.8 / 374.5 / 222.8 / 166.4 ms |
| 5 | Emotiscope period is 10,000 µs; the alternate branch is 8,333 µs, RH:39–47 | RH:40, 47 (see note N1) |
| 6 | `setHardwareBrightness` accepts up to 255. The boot macro defaults to 100, with `static_assert ≥100` | R:226–227; RH:19–23 |
| 7 | At 24/255, levels are 6,169 (native 16-bit), 256 (8-bit lifted) and 25 (8-bit attenuated). Gamma-2.2 zero thresholds are 23.58 % and 1.893 %. Level counts are 250, 2 and 362 | HD:28–31; recompute matches every figure |
| 8 | The Emotiscope build bypasses the RGB8 lift. Pixel8 is a preview only | FIX:657–666 (`if (false)`); R:273, 296–297 |
| 9 | Wire path applies `scale16`, then `attenuate`, then GRB48 at 16 bits per channel | R:735–737; PACK:28–34 |
| 10 | The v8 `sample()` interpolates authored stops. Only the fallback scans 16 compiled entries | W:41–70; W:225–231 |
| 11 | Path P samples the HD palette per pixel at offset + d/80 | W:885–901, W:888 |
| 12 | At the recorded hash the header selected `kPerPixelMapped`. Before the concurrent edit it read `kRestrictedRange` | `git diff` of the header: `-kRestrictedRange +kPerPixelMapped` |
| 13 | `offset_` is 160 floats (640 B) and `shape_` is 80 floats (320 B) | H:237, H:258 at the recorded hash |
| 14 | The pre-knee `linear_shape` is taken before gain, knee and clamp | A:346–350 |
| 15 | Path P folds bins with `i % 12` and has no tonality gate and no speed cap | W:611, W:627–652 |
| 16 | The accent uses the lowest 12 bins (110–208 Hz) | W:725–741 |
| 17 | Accent defaults: boost 0, widen 0, 0.4 s / 4 s, ratio 3. Release is disabled, 30 ms / 2 s, 3 dB + 6 dB | H:53–76 at the recorded hash |
| 18 | Transport is 405 × motion px/s. v8 takes at most 8 steps. Position is `80 + 0.7·80·peak_stage2` | W:259–268; W:967–976 |
| 19 | v9 width is `0.5 + 3.5·level` | W:826 |
| 20 | BUDGET: stack 16.452 ms, percentile stack 24.972 ms, render 0.197 ms; the stack already includes the 5.1 ms wire | BUDGET:13–15, 29–34 |
| 21 | A late frame increments `skipped_` and jumps ahead | R:358–361 |
| 22 | Lineage, lesson and authority citations: K1 `c2738b881d87…` (LINEAGE:39); PL14-B14 (PL14:148–152); `Kx-Titan/AGENTS.md:12` ("default brightness100/255 supersedes historical24"); `build_scalar.py:933` (raises below 100); `palette_runtime.h:23` | All read; all match |

I also confirmed the white-balance factors 1 / 0.9375 / 0.84 at R:286. They are inherited from Emotiscope `src1/leds.h:23`, which settles D08-11's origin claim.

## Refuted, stale or overstated (3)

1. **STALE: D08-13 and D08-07 v9 gains and the H:31–36 provenance.**
   - **What the report says:** the gains are 0.395878 / 0.591745, chosen by the frozen P6 matched-energy rule.
   - **What is true now:** the working-tree header now has `kInhouseV9Gain32 = 0.580092F` and `kInhouseV9Gain18 = 0.830949F`. The new comment reads "P6 path P matched-average-light calibration (calibration-P-fix.json)", and says these values are the "%.6f" argv the scored probe executed.
   - **Why it matters:** the report's figures were true only at hash `88cea47b…`.
     - [INFERENCE] Under path P, deposit amplitude is now about 1.47× (mode 32) and 1.40× (mode 18) higher than the report states.
     - Any experiment that used the old gains, including D08-13's matched-energy A/B, must re-read the header first.
     - All H:≥35 citations shift by +3.

2. **OVERSTATED: Opus addendum D08-18, "fresh deposit reaches at most 56 of 80 pixels; only scrolled trails reach the outer 24".**
   - The deposit centre is capped at `80 + 56` (W:976). However, the triangle's `reach = width + 1` is at most 5.0 px with default accent (W:826, 841–843).
   - Fresh light can therefore land up to about pixel 140, which is about 60 of the 80 pixels in a half.
   - Only the outer ~19 pixels depend on scrolling. The direction of the finding stands: a spatial cap exists beside the speed cap. The numbers are off by about 4–5 pixels.

3. **OVERSTATED: Opus addendum, "a 24/255 owner cap can exist only as a runtime `setHardwareBrightness(24)`" and "brightness authority is a four-way conflict".**
   - At least three other gain stages sit in the live path and could carry an envelope:
     - the Emotiscope config display scale `0.3 + 0.7·config.brightness` (R:283);
     - the per-lane `output_intensity_q16_`;
     - the per-board `board_intensity_q16_` (R:706–722).
   - The build-default part is correct: RH:23 and `build_scalar.py:933` stop the boot macro going below 100. "Only via `setHardwareBrightness`" is wrong.
   - So the composite envelope has more than four sources of authority. Reported, not resolved.

## Origin field audit (S3 budget vs aesthetic)

None of the 18 levers cites evidence that its current setting was an ESP32-S3 compute-budget compromise.

**Asserted without evidence** (labelled [INFERENCE], with no lineage cited):

| Lever | Origin as asserted | What is missing |
|---|---|---|
| D08-01 | "Inherited filtering and scheduling" | No lineage given for the 16/23 ms cascade. I could not find these constants in Emotiscope `src1` or K1 `i2s_audio.h` in a quick search |
| D08-02 | "Configuration/inheritance" | Not tied to any S3 frame-rate limit |
| D08-03 | "Inherited implementation" | Integer scroll at 405 px/s and the 8-step cap are not traced to any source |
| D08-04 | "Inherited/aesthetic smoothing" | Nothing supports "aesthetic": no Captain ruling or design note is cited for 16/23 ms |
| D08-15 | "Harmonic/aesthetic representation" | The mod-12 chroma fold is inherited code (comment at W:144 names the reference `get_smooth_spectrogram`), but no source says it was chosen for taste rather than cost |

**Supported:**

- D08-05: explicit v9 code.
- D08-09: H:40–41 and LINEAGE:39.
- D08-10: A:347 and W:593–595.
- D08-11: Emotiscope `leds.h:23`.
- D08-13: header comment. It is now superseded by the new path-P calibration comment.
- D08-14 and D08-16: H:51 and H:63 cite frozen P6 files.

## Notes

- **N1.** RH:41–45 has a third period branch, 16,667 µs (60 Hz), for `K1_DUAL_EDGE_DONOR` and `K1_PALETTE_WS2816`. The report omits it. That does not change its 100 Hz Emotiscope finding. `Kx-Titan/AGENTS.md:9` still states a "120 Hz logical rendering" ruling. That conflicts with 100 Hz, as the report itself says.
- **N2.** The R:703–704 comment says "The Emotiscope profile supplies RGB8 expanded by 257". For Inhouse modes this is stale: R:273 renders Pixel16 straight into `wide_`. For native donor modes it may still be true. D08-06's 16-bit claim is correctly limited to the waveform path, but the comment could mislead a later agent.
- **N3.** The v9 path compiles only under `K1_INHOUSE_MAPPING_V9` (H:16). The report correctly says that source selection does not establish the installed image (PL14-B14).

## Not checked

- The content of literature L1–L15, including the BT.1359-1 figures (−125 / +45 ms detection, −185 / +90 ms acceptability). The sign convention matches my knowledge of the standard, but I did not fetch it.
- The `workflow_verify` receipt `7151d65a…`.
- Whether the resident firmware image selects path P and the new gains.
