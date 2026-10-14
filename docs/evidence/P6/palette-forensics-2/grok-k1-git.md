# K1 git: the 80-bin palette was looked up through a 16-slot blender

Abstract: Read-only archaeology of `/Users/spectrasynq/SpectraSynq_K1_Firmware` (HEAD `68a95773`, 2026-10-14). The spatial palette bug is the GDFT half-strip: each of 80 LEDs turned its frequency position into an 8-bit index and sampled a 16-entry FastLED palette with linear blend. That path entered this repo already broken at the clean-slate fork and was replaced on 22 Aug 2026. The July vibrancy commit is a different bug. Titan’s waveform uses the fixed sampler, but only once per frame.

Pictures: not applicable. This is git history and source lines, not a render.

## Plain English

[FACT] On the spectrum display (GDFT), the first half of the strip is 80 LEDs. `NATIVE_RESOLUTION` is 160 and `NUM_FREQS` is 80, and the constants comment says each frequency bin maps to one canvas pixel before the mirror fill (`SPECTRASYNQ_K1_FIRMWARE/system/constants.h:164-167`).

[FACT] From the day this repository was created until 22 August 2026, each of those 80 LEDs got its palette colour like this, in `light_mode_gdft.cpp`:

```cpp
uint8_t paletteIndex = uint8_t(float(freq_prog) * 255);
CRGB rgb_color = ColorFromPalette(pal, palette_index_with_phase(paletteIndex), uint8_t(float(bin) * 255));
final_color = crgb_to_crgb16(rgb_color);
```

`freq_prog` is LED index divided by 80. Brightness was the bin magnitude, also crushed to an 8-bit number. `palette_index_with_phase` added the auto-shift as another 8-bit add.

[FACT] The shipping comment in `lightshow_modes.h:113-119` names the three losses: `CRGBPalette16` flattens every authored gradient to 16 entries, the old sampler rounded the coordinate to `uint8`, and it returned an 8-bit `CRGB`.

[FACT] The host FastLED stub (`scripts/regression-harness/stubs/FastLED.h:224-286`) shows what that lookup does. The constructor walks authored stops and fills 16 slots (`index / 16`). `ColorFromPalette` defaults to `LINEARBLEND`: `hi4 = index >> 4` picks one of 16 slots, `lo4` blends into the next slot. The GDFT call omits the blend argument, so that default applies.

[INFERENCE] That is the Captain’s “not accurately mapped” / “over-averaging.” Neighbouring LEDs are only `255/80 ≈ 3.2` index units apart, while each of the 16 slots is 16 index units wide, so about five adjacent LEDs share one blended pair of slots. The authored curve was already crushed into those 16 slots before the blend. The words “over-averaging” and “accurately mapped” do not appear in any commit message searched (`git log --branches --remotes --tags -G 'over-?averag|accurately mapped|accurate.?map'` returned nothing).

[FACT] The cure already existed in the same file, unused by GDFT. `palette_manual_colour` (`lightshow_modes.h:244-295`) keeps the position as a float, walks the real gradient stops (up to 48), interpolates the two stops on either side, applies auto-shift in float, and multiplies by brightness in 16-bit. A cold cache still falls back to `ColorFromPalette(..., LINEARBLEND)` at line 267.

### Who introduced it, inside this repo

[FACT] `ed56dfb575a33f304fb4365afa618961b694ca6b`, 2026-06-23 16:50:31 +0800, message `chore(fork): initial SpectraSynq K1 firmware — clean-slate rebrand from Sensory Bridge`. The broken GDFT lines above are in that commit’s `light_mode_gdft.cpp`. A pickaxe of `float(freq_prog) * 255` on that file returns only this commit and the fix below. There is no earlier commit in this repository.

[INFERENCE] The broken formula lived in Sensory Bridge before the clean-slate import. This repo cannot name a pre-fork hash. Lightwave is a different dig.

[FACT] At that same import, Spectrum River was already on the good sampler: `hue = k / (NUM_FREQS - 1)` into `palette_manual_colour`. The 80-bin map that was broken-then-fixed inside this repo is GDFT, not Spectrum River.

### Who fixed it

[FACT] `c2738b881d87ce53b04879332344ac272c4797d1`, 2026-08-22 17:18:38 +0800, message `feat: paint C1/C2 colour in 16-bit after device PASS`. Body: “Geometric hsv and palette_manual_colour on seven live modes were flashed to Main RPL and scored PASS_TRUE16 before this commit.” Author SpectraSynq. 23 files, +1868/−125. Ancestor of HEAD `68a95773` and of `main` (`a5e71961`).

[FACT] The GDFT hunk is only the colour call:

```diff
-        uint8_t paletteIndex = uint8_t(float(freq_prog) * 255);
-        CRGB rgb_color = ColorFromPalette(pal, palette_index_with_phase(paletteIndex), uint8_t(float(bin) * 255));
-        final_color = crgb_to_crgb16(rgb_color);
+        final_color = palette_manual_colour(pal, freq_prog, bin);
```

[FACT] The same commit swapped the other six live `ColorFromPalette` sites: chromagram dots, chromagram gradient, kaleidoscope, quantum collapse, VU, VU dot. The ADR `docs/architecture/ADR-2026-08-21-vp-ws2816-working-precision.md:149` records “Seven live ColorFromPalette sites → palette_manual_colour (C1).” Line 185 says the live sampler is `palette_manual_colour`; `K1_PALETTE_HD_V2` is not the compiled path on the product env named there.

### What the fix did not remove

[FACT] GDFT still blends neighbouring bin magnitudes for brightness: `bin = bin1 * (1 - fract) + bin2 * fract` (`light_mode_gdft.cpp:46-56`). The temporal smoother on `spectrogram_smooth` is still there. Those change how bright a pixel is. They do not choose the palette position any more. The position is `freq_prog`.

[FACT] `024591dd` (message begins “Keep NUM_FREQS=80 canvas…”) changed which analysis bins are painted: `freq_index_f = freq_prog * analysis_hi` instead of `freq_prog * (NUM_FREQS - 1)`. The palette line in that commit was still `ColorFromPalette`. It retired ghost bins. It did not fix the sampler.

### The July commit is a different bug

[FACT] `ae5d90a37480ce859d3ae058d0b186b9c456935f`, 2026-07-02 19:18:54 +0800, `feat(vp): K1_PALETTE_VIBRANCY_V1 — fix palette-coordinate collapse on dense/loud music`. The helper still in source (`lightshow_modes.h:297-317`) blends a held hue toward the live 12-note centroid when the centroid is weak. It does not index the 80 bins. The previous report in `../P6-PALETTE-FORENSICS/FORENSICS.md` found this one. It is real, and it is not the 80-LED map.

### Current K1 formula

[FACT] HEAD and `main` both call the fixed function. GDFT, `SPECTRASYNQ_K1_FIRMWARE/effects/light_mode_gdft.cpp`:

- Line 21: loop `i` from 0 to `NATIVE_RESOLUTION/2 - 1` (80 LEDs).
- Line 23: `freq_prog = i / (NATIVE_RESOLUTION / 2)`.
- Lines 24-56: magnitude is a lerp of `spectrogram_smooth` at `freq_prog * analysis_hi`.
- Line 86: `final_color = palette_manual_colour(pal, freq_prog, bin)`.

`palette_manual_colour` (`lightshow_modes.h:244-295`): wrap `hue` to 0..1, add `hue_position` in float if auto-shift is on, walk `PaletteStopsHD` until the stop past `h`, lerp RGB by `(h - pos[i]) / (pos[i+1] - pos[i])`, multiply by clamped brightness, return `CRGB16`.

[FACT] Spectrum River, the explicit 1:1 map, `light_mode_spectrum_river.cpp:59-70`:

```cpp
const float hue = float(k) / hue_denom;   // hue_denom = analysis_bins - 1
CRGB16 col = palette_manual_colour(pal, SQ15x16(hue), SQ15x16(e * RIVER_INJECT_GAIN));
```

Pixel is `HALF + k`. This path was already on `palette_manual_colour` at the fork. The denominator changed later from `NUM_FREQS - 1` (79) to the nyquist-safe bin count.

### Titan

[FACT] Titan’s sampler matches the fixed K1 sampler, not the old 16-slot blender. `src/k1/core/visual/inhouse_waveform.cpp:41-71`, `sample()`: float coordinate, walk authored stops, lerp the two neighbours, multiply by brightness. The comment on line 42 says it matches `palette_manual_colour`.

[FACT] The waveform does not give each of the 80 bins its own palette position. Lines 145-150 smooth `spectrum_[i]` over time. Lines 151-172 fold those bins into 12 chroma notes (`i % 12`) and compute one centroid. Lines 209-210 sample the palette once:

```cpp
observation_.palette_coordinate = wrap(centroid + walk);
auto chroma_colour = sample(controls.palette_id, centroid + walk, bright);
```

plus one fallback sample at the held coordinate (line 211). The v9 branch (lines 182-189) still consumes that one centroid; it does not set a palette coordinate to `i/80`.

[FACT] Titan therefore reproduces neither K1’s current per-bin GDFT/Spectrum River map nor the old per-bin `ColorFromPalette` map. It uses the fixed stop sampler on the single-position waveform/chroma rule.

### Ship path for this finding

This file is archaeology. It does not flash a board.

1. Already in K1 source: `c2738b88` is an ancestor of HEAD `68a95773` and of `main` `a5e71961`. GDFT line 86 is the fixed call today.
2. Remaining, and not this task: if Titan’s waveform is supposed to paint a palette position per frequency bin the way GDFT and Spectrum River do, that is a Titan source change. Nobody has made it here.
3. Who acts: a Titan firmware change, then that tree’s own build and programme. This dig does not name a Titan stamp.
4. What “shipped” means for the K1 fix: the commit above is already the history that current GDFT compiles. A new flash is not required to make the K1 source true, and this session did not flash one.

## Evidence

### Search that isolates the formula

[FACT] `git log --branches --remotes --tags --oneline -S 'float(freq_prog) * 255' -- SPECTRASYNQ_K1_FIRMWARE/effects/light_mode_gdft.cpp`

```
c2738b88 feat: paint C1/C2 colour in 16-bit after device PASS
ed56dfb5 chore(fork): initial SpectraSynq K1 firmware — clean-slate rebrand from Sensory Bridge
```

[FACT] Phrase search for the Captain’s wording returned no commits (command above).

### Introducer, GDFT palette block at `ed56dfb5`

[FACT] `git show ed56dfb5:SPECTRASYNQ_K1_FIRMWARE/effects/light_mode_gdft.cpp` (palette branch):

```cpp
uint8_t paletteIndex = uint8_t(float(freq_prog) * 255);
CRGB rgb_color = ColorFromPalette(pal, palette_index_with_phase(paletteIndex), uint8_t(float(bin) * 255));
final_color = crgb_to_crgb16(rgb_color);
```

[FACT] Same commit, Spectrum River already correct (`git show ed56dfb5:.../light_mode_spectrum_river.cpp`):

```cpp
const float hue = float(k) / float(NUM_FREQS - 1);
CRGB16 col = palette_manual_colour(pal, SQ15x16(hue), SQ15x16(e * RIVER_INJECT_GAIN));
```

### Fix diff, `c2738b88`, GDFT only

[FACT]

```
commit c2738b881d87ce53b04879332344ac272c4797d1
Date:   Sat Aug 22 17:18:38 2026 +0800
feat: paint C1/C2 colour in 16-bit after device PASS

-        uint8_t paletteIndex = uint8_t(float(freq_prog) * 255);
-        CRGB rgb_color = ColorFromPalette(pal, palette_index_with_phase(paletteIndex), uint8_t(float(bin) * 255));
-        final_color = crgb_to_crgb16(rgb_color);
+        final_color = palette_manual_colour(pal, freq_prog, bin);
```

Files in that commit that changed a live colour call: `light_mode_chromagram_dots.cpp`, `light_mode_chromagram_gradient.cpp`, `light_mode_gdft.cpp`, `light_mode_kaleidoscope.cpp`, `light_mode_quantum_collapse.cpp`, `light_mode_vu.cpp`, `light_mode_vu_dot.cpp`.

### Old lookup, host stub

[FACT] `scripts/regression-harness/stubs/FastLED.h:262-277`:

```cpp
uint8_t hi4 = index >> 4;
uint8_t lo4 = index & 0x0F;
// LINEARBLEND mixes pal[hi4] with pal[hi4+1] by (lo4 << 4)
```

Slot fill is `indexstart / 16` at lines 246-252.

### Current sampler

[FACT] `SPECTRASYNQ_K1_FIRMWARE/visual/lightshow_modes.h:270-294`: find stop `i` with `hd.pos[i+1] < h`, then `t = (h - hd.pos[i]) / span`, then `cr = hd.r[i] + (hd.r[i+1] - hd.r[i]) * t` (same for g, b), then multiply by brightness.

### Not this bug

[FACT] `ae5d90a3`, 2026-07-02 19:18:54 +0800, vibrancy message quoted above. Current helper comment says the old behaviour froze the palette coordinate when centroid strength was under 0.08.

[FACT] `024591dd` GDFT diff replaces `freq_prog * (NUM_FREQS - 1)` with `freq_prog * analysis_hi`. No `palette_manual_colour` in that GDFT hunk.

## Changelog

| Date | Author | Change |
|------|--------|--------|
| 2026-10-14 | agent:grok | First write. Isolated the 80-LED GDFT uint8 `ColorFromPalette` path, introducer `ed56dfb5`, fix `c2738b88`, and the Titan single-centroid sample. |
