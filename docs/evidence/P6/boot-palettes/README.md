# Random start-up palettes — 15 Oct 2026

**What it does.**
- At each power-on, the Titan waits 0.5 s.
- It then mixes the low bits of the live microphone spectrum and loudness, plus the render loop's timestamps, into a seed.
- From that seed it picks one warm/cool pair out of 47, and picks at random which edge gets the warm one.
- The two edges never share a colour family. That rule is yours: "MAKE SURE THAT BOTH CHANNELS ARE NOT USING THE SAME… COLOUR".
- If a host changes a palette before the pick, the host's choice stays.

**Pair list.**
- Generated from the frozen catalogue by `scripts/boot_palette_pairs.py` into `src/k1/core/visual/boot_palette_pairs.h`.
- Each palette must be at least 90 % in its own family, cover no more than three hue sextants (so no rainbows), and be no more than 30 % near-black. The two palettes in a pair must be at least 120° apart in hue.
- The 90 % purity rule was added after the first swatch sheet let in mixed palettes, such as Sunset_Real ending in blue and Blue_Cyan_Yellow ending in yellow.
- `pairs.png` is the sheet that was looked at:
  - warm (11): retro2, pinksplash 07, vintage 57, lava, fire, Magenta Evening, autumn 19, Black-Red-Magenta-Yellow, K1 Coral Sunset, K1 Crimson Gold, K1 Vepar Pink;
  - cool (5): ocean breeze 036 and 068, Coral Reef, landscape 64, emerald dragon.

**Tests.** `tests/host/test_boot_palette_random.py`:
- the header matches the catalogue;
- every pair is warm plus cool, and a planted 37/39 pair (both warm) is caught;
- the picker uses all 47 pairs, puts warm on the upper edge 50.0 % of the time, always returns the same pair for the same seed, and returns different pairs for different microphone noise.

**Device.**
- Image host-009, build fe4bf14a; lane commit 898b868; 217/217 receipt sources match.
- Flashed with the NOATTACH cycle, identity matched, 100 frames/s, 0 LED faults.
- Three boots, each a re-flash of the same image. In every readback the only fields that differ from the preset are the two palettes.

| Boot | Lower (board 0) | Upper (board 1) |
|---|---|---|
| 1 | 13 es_landscape_64 (cool) | 29 es_autumn_19 (warm) |
| 2 | 29 es_autumn_19 (warm) | 8 Coral_reef (cool) |
| 3 | 3 es_ocean_breeze_036 (cool) | 27 Magenta_Evening (warm) |

**Not done.** No camera recording of these boots, because the camera server was not running. Palette identity comes from the device readback.
