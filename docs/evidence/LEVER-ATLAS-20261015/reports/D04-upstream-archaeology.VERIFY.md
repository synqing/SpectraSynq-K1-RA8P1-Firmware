---
abstract: "Adversarial verification of D04-upstream-archaeology/REPORT.md. 26 decision-relevant FACT claims checked against git objects and the live Titan worktree; 21 confirmed, 5 refuted. Read-only; no device, flash or commit."
id: D04-VERIFY
date: 2026-10-15
images: N/A. This is a source-check of a text report. No visual output was produced, so there is no render to inspect.
---

# D04 verification

## What happened, in plain English

Most of D04 holds up. The upstream numbers it quotes (sample rates, bin counts, note offsets, the Hann window that was built and never applied, Emotiscope's Gaussian window, tempo bank and softness filter, the SB9 river and HD palette commits, Titan's 24 kHz / 180-sample contract, the v9 per-pixel colour path, and the disabled release and accent switches) all match the source.

Five claims are wrong, and two of them would change what the swarm recommends:

1. **The asymmetric smoother was tried and then deliberately rolled back.** D04 says the SB9 author "named the split and never gave it two numbers", and calls the 0.75/0.75 setting "aesthetic, later mislabelled as a fix". That is false. At `d408433`, `SENSORY_BRIDGE_FIRMWARE/system/constants.h:278-285` records "Change 6 — Asymmetric attack/release". It also records "2026-05-20 ROLLBACK: SUSPECT #3 for washout. Slow release made notes persist ~3× longer, causing more LEDs lit simultaneously → more LGP …-mixing → milky/washed appearance. Reverted to prior symmetric values." (The elided word is the American spelling of colour, quoted from source.) D04-L03 proposes attack 0.9 / release 0.2, and that slow release is the setting already rolled back for washout on the light-guide plate. The experiment must be redesigned: fast attack with release unchanged, and a washout metric added.
2. **The 2000-sample window cap does not bind on any production bin.** D04 says that at 24 kHz the cap shortens the bass windows, so low-note pitch resolution "got worse in milliseconds". It also says the cap was the reason for the 110 Hz floor. The window is `SR / (2·gap)` (`gdft_goertzel.cpp:117-120`). At the 110 Hz bin the gap is 6.54 Hz. That gives about 1835 samples at 24 kHz and about 978 samples at 12.8 kHz, and both are 76 ms. The cap first binds below roughly 101 Hz at 24 kHz. Resolution did not get worse. The cap matters only if the bank is extended below 110 Hz, which is D04-L01's own proposal.
3. **Titan's default colour fold reads 60 bins, not 80.** `InhouseWaveformControls::chroma_range` defaults to `60` (`src/k1/core/visual/inhouse_waveform.h:126`). So by default bins 60–79 (about 3.5–10.5 kHz) never vote for colour. D04's "80 numbers become 12" and the 7/6 uneven split describe a non-default setting. This is a missed lever: the top quarter of the spectrum is colour-blind on the boot path.
4. **Titan already has a 96-tempo phase bank.** D04-L09 says "no equivalent table in the files read… It was never ported". The Emotiscope donor on Titan defines `NUM_TEMPI (96)` and `TEMPO_LOW (48)` (`platform/ra8p1/emotiscope_donor/global_defines.h:41,44`). The bank is missing only from the in-house waveform path (modes 32 and 18). The report's own addendum already says so, but the lever body was not corrected.
5. **The February 2026 Lightwave brightness-only trail does exist.** D04 says it was not found. Lightwave-Ledstrip `947bd481` (2026-02-08, "add BeatPulseBloomEffect with subpixel transport") says "HDR-ish history buffer stores per-LED intensity values … advected outward". The brief's attribution stands. The addendum agrees.

Several claims about **why** a setting exists ("S3 budget") are stated without evidence. They are listed in the origin flags below. Treat those origins as unknown until a source says so.

## Claims checked

Status: C = confirmed, R = refuted.

| # | Claim in REPORT.md | Source checked | Status |
|---|---|---|---|
| 1 | SB 4.1.1 `constants.h:4-14`: 12800 Hz, history 4096, 128 px, 64 bins, 2 zones, spectral history 5 | `git show 5f329261:SENSORY_BRIDGE_FIRMWARE/constants.h` | C |
| 2 | SB defaults: MOOD 0.05, NOTE_OFFSET 12, SQUARE_ITER 1, SAMPLES_PER_CHUNK 96, CHROMAGRAM_RANGE 60, dithering true, incandescent 0.50; `chromatic_mode = true` | `5f329261:globals.h:49,55,56,61,67,72,74,151` | C |
| 3 | Hann table built and its multiply commented out; `sample >> 6` in the recurrence | `5f329261:GDFT.h:91-94`, `system.h:198-205` | C. Note: the coefficient is `0.54·(1−cos)`, which is neither Hann (0.5·(1−cos)) nor Hamming (0.54−0.46·cos). D04 quotes it correctly but still calls it "Hann" |
| 4 | Per-bin `block_size = SR/(2·max_gap)`, capped at 2000 | `5f329261:system.h:238-241` | C |
| 5 | Chromatic hue `note_colors[i % 12]` | `5f329261:lightshow_modes.h:86` | C |
| 6 | Emotiscope: `BOTTOM_NOTE 12`, `NOTE_STEP 2` (quarter-step table, bin 0 ≈ 77.8 Hz); Gaussian applied at `goertzel.h:195`; phase discarded at `:81`; tilt `(bin/64)^4·0.9975+0.0025` at `:207-210` | `d0fef9a:src/goertzel.h` | C (`notes[12]` = 77.78175) |
| 7 | Emotiscope tempo: 96 tempi, 48–143 BPM, 1024 novelty samples at 50 Hz | `d0fef9a:src/global_defines.h`, `tempo.h:53-56` | C |
| 8 | Softness low-pass `0.5 + (1−√softness)·14.5`, default 0.25; frame blending and phosphor decay commented out | `d0fef9a:gpu_core.h:85-93`, `configuration.h:35` | C |
| 9 | Emotiscope bloom trail is one float per pixel, colour applied at paint | `d0fef9a:src/light_modes/active/bloom.h:1-25` | C |
| 10 | SB9 `6109a1e`: river "Captain 10/10", default GDFT→BLOOM, tempo UNWIRED, RAM 25.6%; `NUM_FREQS 80`, `NATIVE_RESOLUTION 160`; river `hue = k/(NUM_FREQS−1)` via `palette_manual_colour`, "NEVER raw HSV" | commit message; `6109a1e:constants.h`, `light_mode_spectrum_river.cpp:17,26-30,64-65` | C |
| 11 | SB9 `d408433`: `PAL_HD_MAX_STOPS 48`; cold fallback `ColorFromPalette(..., uint8_t(h*255))`; 12800 / 96 kept; CHROMAGRAM_RANGE 60 | `d408433:visual/lightshow_modes.h:116,228`; `system/config_types.h:36-41`; `system/globals_config.cpp:70` | C |
| 12 | "The split was named and not given two different numbers"; "upstream author had already decided it was a problem and had not changed the number"; L03 origin "aesthetic, later mislabeled as a fix" | `d408433:SENSORY_BRIDGE_FIRMWARE/system/constants.h:278-289` | **R**. Asymmetric smoothing was implemented as "Change 6", then rolled back on 2026-05-20 as "SUSPECT #3 for washout" (slow release → notes persist about 3× longer → LGP colour mixing → milky). The symmetric 0.75 is a deliberate aesthetic revert backed by an on-device finding |
| 13 | Titan production contract 24000 Hz / hop 180 / 80 bins / crossover 40 / novelty ÷3; 7.5 ms parity is intentional | `src/k1/core/audio/audio_rate_config.h:7-27` | C |
| 14 | Titan `kNoteOffset = 12`, `kMaximumBlockSize = 2000`, no window multiply; `>> 6` still present | `gdft_goertzel.cpp:18-20,55`; grep `window` returns no window function | C |
| 15 | "At 24000 Hz a 2000-sample window is 83 ms … low-note pitch resolution got worse in milliseconds"; L01 says the bass-window cost under the 2000 cap explains the floor | `gdft_goertzel.cpp:117-120` (block from `rate.sample_rate_hz`); arithmetic below | **R**. 110 Hz bin: gap 6.54 Hz → 1835 samples at 24 kHz and 978 at 12.8 kHz, 76 ms at both. The cap does not bind for any bin ≥110 Hz at either rate. It binds below about 101 Hz (24 kHz) |
| 16 | `sub_band.h`: 40–110 Hz biquad, release trigger only, independent of AGC | `src/k1/core/audio/sub_band.h:3-8` | C |
| 17 | Titan chroma fold `chroma[i % 12]`; `chroma_max_` rise 0.95^(dt·100) and decay 0.999^(dt·100); v8 `value*value*1.5`; v8 blends two palette samples, applies a dot EMA on RGB, falls back to the brightest of 16 `compiled_fastled16` entries | `src/k1/core/visual/inhouse_waveform.cpp:144-238` (the file is modified in the working tree; another lane is editing it) | C |
| 18 | Titan stage folds "up to 80 bins"; L04 says "80 numbers become 12", classes 7/6 uneven | `inhouse_waveform.h:126` `std::uint8_t chroma_range = 60;` | **R (partial)**. The default range is 60. Bins 60–79 are excluded from colour on the default path, and the classes are an even 5 each. The 7/6 split applies only at a non-default range of 80 |
| 19 | v9: `kPerPixelMapped`, faults `kSlot16` and `kFrozenPosition` (host-only), `outputV9` colour = HD sample at `offset + d/80`; accent `boost = 0`; release `enabled = false`, `trigger_sub = false`; ceiling −9 dBFS | `inhouse_waveform.h:16-76`; `inhouse_waveform.cpp:872-902` | C |
| 20 | Profile 8 adds no define; profile 9 adds `-DK1_INHOUSE_MAPPING_V9=1` | `scripts/live_boot_composition.py:12-14` | C |
| 21 | `build_scalar.py:1175`: precision 16, 160 px per board, modes 32/18/8/7/11 | `scripts/build_scalar.py:1175` | C |
| 22 | LightwaveOS HEAD: 16000 Hz / 160 / 96 / 2 zones; `30b084e2` `:194` `paletteIndex = uint8_t(freq_prog*255)`, `:208` `ColorFromPalette(pal, paletteIndex, uint8_t(bin*255))` | `git show HEAD:src/constants.h`; `git show 30b084e2:src/lightshow_modes.h` | C |
| 23 | K1 `c2738b88` subject "paint C1/C2 colour in 16-bit after device PASS" | `/Users/spectrasynq/SpectraSynq_K1_Firmware` git log | C (2026-08-22) |
| 24 | L09: "Titan: no equivalent table … never ported" | `platform/ra8p1/emotiscope_donor/global_defines.h:41,44` | **R**. The bank is on Titan in the donor path. It is absent only from in-house modes 32/18 |
| 25 | "The brightness-only sentence the swarm brief attributes to 'Lightwave Feb 2026' was not found" | Lightwave-Ledstrip `947bd481` (2026-02-08) message | **R**. A February 2026 intensity-only history buffer exists |
| 26 | SB README says "100 FPS" and "3-second" calibration (cited as `README.md:5`) | `5f329261:README.md:7,48` | C. The line number is off by 2 |

Totals: 26 checked, 21 confirmed, 5 refuted (one of them partial).

## Origin flags: "S3 budget" or "aesthetic" stated without evidence

These do not contradict any source. No source supports them either. Each one should be relabelled [HYPOTHESIS] until a commit, comment or measurement is found.

- **D04-L01 (110 Hz floor = "S3 budget plus artistic choice").** The cap argument is refuted (row 15). With a fixed 64 bins on a 6400 Hz Nyquist, offset 12 puts the top bin at 4186 Hz; offset 0 would have stopped at about 2093 Hz. The evidence fits a **range-placement trade-off** (bass versus treble with a fixed bin count) at least as well as a CPU cut. Origin: unknown.
- **D04-L02 ("S3 budget. The Hann was … commented out to save the multiply").** No comment at `GDFT.h:91` gives a reason. The written coefficient is also not a true Hann (row 3). Origin: unknown.
- **D04-L03 ("aesthetic, later mislabeled as a fix").** Refuted (row 12). The 0.75/0.75 is a documented aesthetic revert caused by LGP washout.
- **D04-L05 (peak divide exists "so the fixed-point bins and the 8-bit LEDs stayed in range") and Inherited compromise 7.** No source comment says this. The SB 4.1.1 code only shows a slow peak follower with a floor of 4. Origin: unknown, and auto-exposure for taste is equally plausible.
- **D04-L09 (Emotiscope "put it on the other core because it is another 96 Goertzels").** `EMOTISCOPE_FIRMWARE.ino:39-40` says Core 0 runs graphics and Core 1 runs audio plus the web server. Tempo is part of the audio core. The core split is graphics versus audio, not "tempo needed its own core". Unsupported.
- **Inherited compromise 2 ("the cap's original job was 'do not scan 4000 samples of a 50 Hz bin on an ESP32'").** Even at the lowest table entry (55 Hz, gap 3.27 Hz) the 12.8 kHz block is about 1957 samples, under the cap. The cap's purpose is not shown anywhere. Unsupported.
- **D04-L14 ("a fix for 8-bit pastels looking grey").** Not checked in this pass (listed as unverifiable below).

## What this changes for the swarm

- **L03 experiment.** Do not ship or A/B "release 0.2". Test a fast attack with the release left at 0.75. Freeze a second criterion that measures washout: the count of simultaneously lit pixels, or the mean saturation of the lit pixels, after a burst. Then plant the rolled-back slow release as the fault that must turn that criterion red. The SB9 finding is LGP-specific, so it applies to the Titan's light guide too.
- **L01 cost.** Adding bins below 110 Hz is where the 2000 cap first starts to matter (about 101 Hz at 24 kHz). A 55 Hz bin at 24 kHz wants about 3670 samples (153 ms) uncapped. That latency is the real cost of a "see the sub" lever. It is a latency cost, not a CPU cost.
- **New lever from row 18.** Raise `chroma_range` from 60 to 80, or stop using the fold for waveform colour. On the default path, nothing above about 3.5 kHz influences colour. The smallest test: a hi-hat-only fixture must move the colour coordinate. Plant `chroma_range = 60` as the fault.
- **L09.** The donor's 96-tempo bank is a reuse candidate (asymmetric-capability doctrine). Wire it into the in-house modes; there is no need to port it again.

## Unverifiable in this pass

- D04-L14 origin ("fix for 8-bit pastels looking grey"): the `d408433` message text for this was not re-read.
- Emotiscope README "up to 450 FPS": the claim's text was not opened. D04 already marks it unmeasured.
- K1.Lightwave `63f0cac` "32 kHz" subject: not re-run.
- Addendum D04-V17 ("upstream interleave refreshes only 48 of 96 bins"): the simulation was not reproduced here.

## Paths and commits read

- `/Users/spectrasynq/Workspace_Management/Software/SensoryBridge` @ `5f329261987144242be4c3729f6d344631d3be1e`
- `/Users/spectrasynq/Workspace_Management/Software/Emotiscope` @ `d0fef9a40aabc0b623d2e14c9dffdc9706a1ec90`
- `/Users/spectrasynq/SensoryBridge-main 9` (remote synqing/SpectraSynq-K1-Firmware-SB9-) @ `6109a1e`, `d408433`; HEAD `3709862c`
- `/Users/spectrasynq/Workspace_Management/Software/LightwaveOS_Official` @ HEAD and `30b084e2`
- `/Users/spectrasynq/Workspace_Management/Software/Lightwave-Ledstrip` @ `947bd481`
- `/Users/spectrasynq/SpectraSynq_K1_Firmware` @ `c2738b88`
- Titan worktree `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `1ef53952`. `inhouse_waveform.{h,cpp}` are modified in the working tree, so line numbers may drift.
