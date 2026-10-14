---
abstract: "Read-only git archaeology for the Captain's memory that palette colour was not accurately mapped onto each of the 80 frequency bins because of over-averaging, and that SpectraSynq_K1_Firmware definitively fixed it. No commit in the searched histories shows that before/after. The closest landed formulas are recorded with hashes."
---

# Palette across the 80 bins — Lightwave git archaeology

Claims are [FACT] when the line was seen in `git log`, `git show`, or `git grep` this pass, and [INFERENCE] otherwise. No commit, checkout, or edit was made in any repository. Images are not applicable: the deliverable is a history note, and the evidence is the quoted diffs.

## Plain English

The bug the Captain described — palette colour smeared across the 80 frequency bins by over-averaging, then repaired — does not have an introducing commit and a fixing commit in these histories. Nothing searched uses the words "over-averaging" or "accurately mapped".

What does exist, and what it actually changed:

The early palette on the music visualiser is in LightwaveOS_Official, June–August 2025. By 12 August 2025 each LED already took its own palette index from its position along the strip, and the bin only set brightness. That tree counts 96 frequency bins (84 of them used), never 80. The phrase "weighted averaging" in that tree is a 12-note chromagram fix for white overflow. It is the same family as the 2 July 2026 K1 vibrancy commit the Captain has already set aside.

The first code that says the 80 bins are mapped one-to-one onto palette position is Sensory Bridge commit `6109a1e` (2 June 2026), a new mode called Spectrum River: `hue = k / (NUM_FREQS - 1)`, colour from `palette_manual_colour`, brightness from that bin's energy. K1 did not invent that formula later. K1's first commit, `ed56dfb5` (23 June 2026), is a clean-slate copy of Sensory Bridge, and the fork message says the Lightwave-Ledstrip tree was left out. The only later change to the river's palette formula inside K1 is `024591dd` (5 August 2026): the divisor becomes `(analysis_bins - 1)` so aliased bins above the Nyquist-safe bin are no longer painted. The message calls those "ghost bins", not an average.

A second, separate accuracy change is Sensory Bridge `d408433` (11 June 2026), also already inside K1 at the fork. The old sampler flattened every gradient to 16 entries, rounded the position to an 8-bit index, and returned an 8-bit colour. The replacement reads the palette's real stops in floating point. That is palette-entry averaging, and the commit does not mention the 80 bins.

Titan's current waveform (`inhouse_waveform.cpp`) still folds up to 80 bins into 12 notes and samples the palette once from the chromagram centroid. That is the path the previous forensics already covered. A per-bin palette position lives on the GDFT and Spectrum River painters, which this waveform file does not use.

No commit message or doc in the searched trees says a Lightwave palette-bin fix was then carried into SpectraSynq_K1_Firmware.

## Repositories

[FACT] Git repos, current checkout, and whether commit `4d71bc01` is in `--all`:

| Path | Checkout | `4d71bc01` |
|---|---|---|
| `Workspace_Management/Software/Lightwave-Ledstrip` | `4d71bc012d18` | present |
| `Workspace_Management/Software/Lightwave-Ledstrip-baseline` | `9734cca1ee7d` | present |
| `Workspace_Management/Software/Lightwave-Ledstrip-k1-effect-workbench` | `a7bc03bd21c0` | present |
| `SensoryBridge-main 9/Lightwave-Ledstrip` | `b9fa983a09bb` | present |
| `Workspace_Management/Software/LightwaveOS_Official` | `b0141c881b49` | different history (LightwaveOS.git) |
| `Workspace_Management/Software/LightwaveOS-Production` | not a git repo | skipped |

[FACT] The four Ledstrip checkouts are one object history. Pickaxes were run once with `--all` on `Lightwave-Ledstrip`.

[FACT] Two further trees were opened read-only because the Captain named K1 as the place of the fix, and K1's first commit names its parent. They are outside the six assigned paths:

- `/Users/spectrasynq/SpectraSynq_K1_Firmware`, fork `ed56dfb575a33f304fb4365afa618961b694ca6b`, 2026-06-23 16:50:31 +0800. Message: "clean-slate rebrand from Sensory Bridge" and "Excluded sibling projects … and Lightwave-Ledstrip".
- `/Users/spectrasynq/SensoryBridge-main 9` (the parent repo, not the nested Ledstrip checkout). Spectrum River and the HD sampler were authored here. Pre-fork Sensory Bridge history is not in the K1 repo.

`git log --all` on LightwaveOS and Ledstrip printed `exhaustive rename detection was skipped` (rename limit in the thousands). A rename-only move of a small palette helper can be missing from a pickaxe. The file contents quoted below were read at named commits, so those quotes do not depend on rename following.

## The candidate already set aside

[FACT] K1 `ae5d90a37480ce859d3ae058d0b186b9c456935f`, 2026-07-02 19:18:54 +0800, subject `feat(vp): K1_PALETTE_VIBRANCY_V1 — fix palette-coordinate collapse on dense/loud music`. The previous report `../P6-PALETTE-FORENSICS/FORENSICS.md` ties this to the 12-note chromagram. It is a different mechanism from a palette index per frequency bin.

[FACT] LightwaveOS `30b084e2dcdae0e9ac7688823dfced7945b9b46a`, 2025-08-12 12:06:21 +0800, subject `feat(visual-pipeline): complete 96-bin restoration`. The message's colour section says: fixed chromagram colour overflow causing white saturation; weighted averaging instead of direct RGB addition; chromagram normalised by actual bins per note (84/12 = 7). In `src/lightshow_modes.h`, `calc_chromagram_color()`:

- Before: for each of 12 notes, `get_mode_color(...)`, convert to 8-bit, and return `sum_color` (the message calls this direct RGB addition).
- After: `sum_color.r += col16.r * bright` (and G and B), `total_magnitude += bright`, then `sum_color.r /= total_magnitude` when `total_magnitude > 0.01`.

[FACT] That loop is 12 notes. The 96-bin work in the same commit retunes band edges (low 0–23, mid 24–59, high 60–83) and `CHROMAGRAM_RANGE` to 84. A tree grep of `docs/PALETTE_INTEGRATION_ADR.md` and `docs/VISUAL_PIPELINE_ARCHITECTURE.md` at this commit printed no `SpectraSynq`, `K1_Firmware`, or `over-averag`.

[FACT] LightwaveOS `d6bc2282`, 2025-08-30 15:38:07 +0800, subject `PHASE 2: Fix chromagram and spectrum processing for full 96-bin utilization`. Same 12-note fold, later than the river's 80-bin world.

## What the 80-bin painters actually do

### Spectrum River — one palette position per bin, from the day it was added

[FACT] Sensory Bridge `6109a1eeb4fe6f6f14b1669995561ea5c90ac9dd`, 2026-06-02 05:54:23 +0800, `feat(effects): effect-library overhaul — palette colour engine, new effects, fit-for-purpose pruning`. The message says Spectrum River is "80-bin spectrum mapped to space, flowed outward. Captain 10/10." The same commit disables GDFT (and several other modes) via `light_mode_is_enabled()` and changes the default mode from GDFT to BLOOM. BLOOM is the single-sample chroma path, so this commit did not make the 1:1 map the default.

[FACT] At that commit, `SENSORY_BRIDGE_FIRMWARE/light_mode_spectrum_river.cpp`:

```
// The 80 frequency bins map 1:1 onto the 80 pixels of the upper half
// bin k -> pixel (HALF + k). Colour from the palette BY FREQUENCY
for (uint16_t k = 0; k < NUM_FREQS && k < HALF; k++) {
  const float hue = float(k) / float(NUM_FREQS - 1);   // frequency -> palette position
  CRGB16 col = palette_manual_colour(pal, SQ15x16(hue), SQ15x16(e * RIVER_INJECT_GAIN));
}
```

`light_mode_spectrum_river_v2.cpp` in the same commit uses `hue = float(k) / float(NUM_FREQS - 1)` as well. There is no earlier revision of this file in that commit: the files are added (`84 +++++` / `92 +++++`). [INFERENCE] This is the first accurate 80-bin palette map in the line that became K1. It is new code beside the old painter, not a patch that deleted an average.

[FACT] K1 `024591dd3fbf155cd352da021965943cfbb11038`, 2026-08-05 23:45:11 +0800, `feat(audio): retire Nyquist ghost bins as resolution`. Message: keep `NUM_FREQS=80` but skip Goertzel and VP paint above `sb_gdft_nyquist_safe_bin_hi`. In `light_mode_spectrum_river.cpp` the hue line changes from `float(k) / float(NUM_FREQS - 1)` to `float(k) / hue_denom` with `hue_denom = analysis_bins - 1`, and the loop stops at `analysis_bins`. The comment added in that hunk says ghosts 71–79 are retired.

### GDFT — palette index is LED progress; brightness is a two-bin mix

[FACT] LightwaveOS `07621f148890107b66112c8d4997dcde8d0887d1` content of `src/lightshow_modes.h` (`light_mode_gdft`):

```
// Map the 96 frequency bins across the first half (NATIVE_RESOLUTION / 2 LEDs)
SQ15x16 freq_prog = (SQ15x16)i / (SQ15x16)(NATIVE_RESOLUTION / 2);
SQ15x16 freq_index_f = freq_prog * (NUM_FREQS - 1);
bin = bin1 * (1.0 - freq_fract) + bin2 * freq_fract;   // adjacent bins, brightness only
uint8_t paletteIndex = uint8_t(float(freq_prog) * 255);
CRGB rgb_color = ColorFromPalette(pal, paletteIndex, uint8_t(float(bin) * 255));
```

The diff at that commit adds the `paletteIndex` lines. `freq_prog` uses the LED count as the divisor, so the last LED is `(N-1)/N`, which never reaches palette index 255.

[FACT] K1 `docs/forensics/2026-05-25-vp-palette-auto-colour-audit.md` (carried in the fork) records the same shape on the 80-bin strip: GDFT maps `freq_prog * 255` and ends at index 251 "because the denominator is 80". The audit's root cause for incomplete palettes on Bloom and Waveform is one chromagram-centroid sample per frame. It does not describe an over-average across the 80 bins, and it does not record a fix.

[FACT] Current K1 `SPECTRASYNQ_K1_FIRMWARE/effects/light_mode_gdft.cpp` still does:

```
SQ15x16 freq_prog = (SQ15x16)i / (SQ15x16)(NATIVE_RESOLUTION / 2);
SQ15x16 freq_index_f = freq_prog * (SQ15x16)analysis_hi;
SQ15x16 bin = bin1 * (1.0 - freq_fract) + bin2 * freq_fract;
final_color = palette_manual_colour(pal, freq_prog, bin);
```

`024591dd` changed `freq_index_f`'s multiplier from `(NUM_FREQS - 1)` to `analysis_hi`. It left `freq_prog` — the value passed as the palette coordinate — on `i / (NATIVE_RESOLUTION / 2)`. The two-bin mix is brightness. The comment "Phase 1 2026-05-20: hoist palette construction OUT of per-LED loop (was 80x redundant)" is a performance hoist of palette setup, not a change of the index formula.

### HD sampler — the average inside the palette, inherited by K1

[FACT] Sensory Bridge `d408433067ddc61cf1252ce13e33d17943a17859`, 2026-06-11 09:58:38 +0800, `feat(vp): HD palette sampling — true gradient-stop evaluation, float coordinates, native CRGB16`. Message: the old chain lost resolution three times — `CRGBPalette16` flattened every gradient to 16 entries, the coordinate was rounded to `uint8`, and the sample came back as 8-bit `CRGB`.

[FACT] Current K1 `SPECTRASYNQ_K1_FIRMWARE/visual/lightshow_modes.h` still carries that comment, dated 2026-06-11. `palette_manual_colour` interpolates the unpacked stop list in float. The cold-cache fallback is the old call: `ColorFromPalette(pal, uint8_t(h * 255.0f), 255, LINEARBLEND)`. A pickaxe of `PaletteStopsHD` in K1 starts at the fork `ed56dfb5`. Later K1 commits touch the cache (diagnostics, a bright-stop excursion); they are not the introduction.

[INFERENCE] If the remembered "over-averaging" was FastLED blending neighbouring palette entries, this is the commit, and it reached K1 only because the fork copied Sensory Bridge. The message never says 80 bins.

### The 50% HSV blend did not land on the LightwaveOS line that survived

[FACT] LightwaveOS `a9776d3a94b06259a95e85a9e1d9510cf08fb3c7`, 2025-06-20 23:47:58 +0800, `EXPERIMENTAL: Add gradient palette integration system`. Message: "50% blend maintains existing dynamics". In `Cuntfucker/palette_manager.h`, `blendWithHSV` is `hsv * (1 - blend) + palette * blend`, and `getFrequencyColor` uses `palette_pos = freq_bin / 64`. `git merge-base --is-ancestor a9776d3a HEAD` is false. The blend is an experiment off the surviving line, not the fix that shipped.

### Ledstrip's spectrum colour is 64 bins and a narrowed window

[FACT] Ledstrip `4964a5a78ec0f6a717c7fd3a388f402930d45415` (2026-01-21) standardises effects onto `ctx.palette.getColor` and removes HSV/rainbow hue. `47325c8316f95caeea352ec5ad58d269565e09a6` (2026-01-06) does the same migration plus `AsymmetricFollower` smoothing. Neither message mentions 80 bins or a palette-index average.

[FACT] Ledstrip `5c9f4f74` (2026-03-13 20:25:07 +0800), `perf(audio): aggressive smoothing reduction for sharp visual response`. Subject is temporal lag ("sharp visual response"), which is a different knob from which palette entry a bin selects.

[FACT] A working-tree grep of Ledstrip markdown for `over-averag` and `accurately mapped` printed nothing. Hits on "weighted average" are WLED's 256-to-16 downscale and an 8-band pitch contour, not a K1 palette-map fix.

## Titan, for comparison

[FACT] `src/k1/core/visual/inhouse_waveform.cpp` folds the spectrum before the palette sample:

```
const unsigned range = std::max(1U, std::min(80U, unsigned(controls.chroma_range)));
for (unsigned i = 0; i < range; ++i) chroma[i % 12U] += ...
observation_.palette_coordinate = wrap(centroid + walk);
auto chroma_colour = sample(controls.palette_id, centroid + walk, bright);
```

[FACT] `sample` in that file walks `productPalette` stops in float (`position/255`), the same idea as the 11 June 2026 HD sampler. `product_palette.h` exposes `sampleProductPaletteHd`. One coordinate still colours the waveform. Bins do not each receive `k / 79`.

## Searches that came back empty

[FACT] On SpectraSynq_K1_Firmware, `git log --all -S 'over-averag'`, `-S 'accurately mapped'`, `-S 'blendWithHSV'`, `-S 'getFrequencyColor'`, and `-S 'weighted averaging'` printed no commits. `git log --all --grep` for those phrases likewise printed no palette-mapping subject. `blur1d` has no hits. A spatial-neighbour pickaxe `spectrogram_smooth[i - 1]` on Sensory Bridge printed no commits.

[FACT] On Sensory Bridge, `git log --all -S 'over-averag'` printed no commits. `-S 'lost resolution'` hits `d408433` only (plus the HD comment). The river hue formula is present at introduction; it was not substituted for a prior average in that file.

[INFERENCE] The memory joins two real events that the repos kept separate: the 12-note colour average (LightwaveOS, August 2025, later re-fixed in K1 as vibrancy), and the 80-bin one-position-per-bin painter (Sensory Bridge Spectrum River, June 2026, copied into K1 at the fork). A single commit that both introduced an 80-bin over-average and then removed it was not found. K1's own history cannot contain the introduction, because that history starts at the fork, after both the river and the HD sampler already existed.
