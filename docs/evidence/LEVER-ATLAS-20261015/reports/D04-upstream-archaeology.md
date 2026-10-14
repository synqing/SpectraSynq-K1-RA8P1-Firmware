---
abstract: "D04 upstream archaeology of SensoryBridge, Emotiscope and Lightwave against the Titan audio-to-light chain. Read-only. Every claim is marked FACT, INFERENCE or HYPOTHESIS and tied to a path, commit or URL. No device, flash, commit or push."
id: D04-upstream-archaeology
date: 2026-10-15
images: N/A — this deliverable is a source-history report. No render was produced. The visual claims below are about what the code computes, not a photograph of the Titan.
---

# D04 — upstream constraint and design archaeology

The three upstream projects are one family, written by Connor Nishijima / Lixie Labs, then continued inside SpectraSynq. Each generation threw information away so an ESP32 could finish a frame. Some of those cuts are the musical look (smoothing, a soft mood, a warm tint). Some are only the old budget (8-bit palettes, a 12-note colour, a note bank that starts at 110 Hz). Titan already undid one of them on the v9 path: the trail can carry brightness and a palette position, and the 16-bit colour is chosen per pixel at the output. It still folds up to 80 frequency bins into 12 notes before that, and the note bank still starts at 110 Hz.

The shared memory service failed a mental-model refresh during this session (Hindsight, 300 s timeout). Nothing below depends on it. Source was read directly.

Scores in the lever records are analyst judgment, marked [INFERENCE]. They are not measurements.

## How the copies were identified

[FACT] `/Users/spectrasynq/SensoryBridge-main` is an empty directory (listing dated this session). The June 2026 commits named in the swarm brief are not in `github.com/connornishijima/SensoryBridge` (GitHub API 422 on 2026-10-15 for `d408433` and `6109a1e`). They are in `/Users/spectrasynq/SensoryBridge-main 9`, remote `https://github.com/synqing/SpectraSynq-K1-Firmware-SB9-.git`, HEAD `3709862c77684503f1b4788c6f42c1a182fef417` (2026-06-24).

[FACT] The original Connor Nishijima tree that is still a complete git history is `/Users/spectrasynq/Workspace_Management/Software/SensoryBridge`, detached at tag `4.1.1`, commit `5f329261987144242be4c3729f6d344631d3be1e` (2024-03-21, "Added FreeRTOS vTaskDelay(1) to LED thread"), remote `https://github.com/connornishijima/SensoryBridge.git`. Its tracked firmware is `SENSORY_BRIDGE_FIRMWARE/*.h`. The working tree has deleted those tracked paths and has an untracked `SENSORY_BRIDGE_FIRMWARE/src/` copy. `GDFT.h`, `lightshow_modes.h`, `led_utilities.h`, `i2s_audio.h` and `noise_cal.h` in that copy are byte-identical to the tag. `globals.h` and `system.h` differ only by a later palette-button insertion below the defaults quoted here. `constants.h` differs by one `#define PALETTE_PIN`. Line numbers below are from `git show 5f329261:SENSORY_BRIDGE_FIRMWARE/...` unless noted.

[FACT] Emotiscope git HEAD `d0fef9a40aabc0b623d2e14c9dffdc9706a1ec90` (2024-10-02, "Update README.md"), remote `https://github.com/Lixie-Labs/Emotiscope.git`. Tracked sources are `src/`. The working tree shows them deleted. Untracked `src1/` is byte-identical to HEAD for every file compared this session (`global_defines.h`, `goertzel.h`, `gpu_core.h`, `leds.h`, `microphone.h`, `light_modes.h`, `tempo.h`, `led_driver.h`, `EMOTISCOPE_FIRMWARE.ino`, `light_modes/active/bloom.h`, `light_modes/beta/waveform.h`). Citations use the tracked `src/` paths.

[FACT] `/Users/spectrasynq/.claude-worktrees/Lightwave-Ledstrip` is empty. LightwaveOS working tree `src/constants.h` and `src/lightshow_modes.h` are modified versus HEAD `b0141c881b49`. The sample-rate and bin-count lines cited for HEAD were taken with `git show HEAD:src/constants.h`, not from the dirty file. The August 2025 palette lines were taken with `git grep` at commit `30b084e2`.

[FACT] Titan product worktree HEAD this session is `1ef53952ab7a50af38414321c8a91e515744098a`. That commit is an evidence-storage move. The waveform and audio files cited were read from the working tree.

## Per-project archaeology

### Sensory Bridge 4.1.1 — Connor Nishijima, the origin

**Audio pipeline.** [FACT] `GDFT.h:8-44` at `5f329261` says this is not an FFT. It is 64 parallel Goertzel detectors ("GDFT") on piano semitones, default 110 Hz to 4186 Hz, so each half-step is its own bin, and each bin has its own window length because an FFT would force one power-of-two length and linear Hz spacing. [FACT] `constants.h:4-14`: `DEFAULT_SAMPLE_RATE 12800`, `SAMPLE_HISTORY_LENGTH 4096`, `NATIVE_RESOLUTION 128`, `NUM_FREQS 64`, `NUM_ZONES 2`, `SPECTRAL_HISTORY_LENGTH 5`. [FACT] `globals.h:45-80`: factory defaults include `NOTE_OFFSET 12`, `SQUARE_ITER 1`, `SAMPLES_PER_CHUNK 96`, `LED_COUNT 128`, `CHROMAGRAM_RANGE 60`, `MOOD 0.05`, `PHOTONS 1.00`, `CHROMA 0.00`, mode `LIGHT_MODE_GDFT`, mirror on, temporal dithering on, auto colour shift off, incandescent filter `0.50`, prism count `0`, base coat off, sweet-spot min `750`, max `30000`.

[FACT] `system.h:209-255` sets `frequencies[i].target_freq = notes[i + CONFIG.NOTE_OFFSET]`. The note table in `constants.h:56-65` is equal-tempered semitones from 55 Hz. Offset 12 is index 12, which is `110.0000` (`constants.h:58`). Bin 63 is index 75, which is `4186.009` (`constants.h:62`). [INFERENCE] That is A2 through C8, 64 semitones, about 5.25 octaves. Nothing below 110 Hz is a bin.

[FACT] Window length is `block_size = SAMPLE_RATE / (max_distance_hz * 2.0)`, capped at 2000 (`system.h:238-241`). Neighbor distance is the larger gap to the adjacent table entry. [INFERENCE] At 110 Hz the semitone gap is about 6.6 Hz, so the window is about 12800/13.1 ≈ 980 samples ≈ 76 ms. At 4186 Hz the gap is about 250 Hz, so the window is about 26 samples ≈ 2 ms. The 2000-sample cap is 156 ms. Low notes buy pitch resolution by giving up time resolution. High notes do the opposite. That is the design, and it is also the CPU budget: the comment at `GDFT.h:36-44` says shorter high-frequency windows are there "to keep a good balance" and "this also helps with speed".

[FACT] A 4096-point Hann table is built (`system.h:200-205`, coefficient `0.54 * (1 - cos)`). The multiply that would apply it is commented out. The running loop uses the raw sample (`GDFT.h:91-92`). `window_pos` is still incremented (`GDFT.h:98`) and then ignored. [FACT] The coefficient is stored as Q14 (`system.h:251`). The sample is shifted down 6 bits before the recurrence (`GDFT.h:94`), which throws away the bottom 6 bits of each 16-bit-ish sample inside the detector. Magnitude is then `sqrt`, divided by `block_size/2` (`GDFT.h:108-112`).

[FACT] After the detectors, three more compressions run every frame (`GDFT.h:118-198`):

- An exponential average, 0.3 new + 0.7 old (`GDFT.h:118`).
- If noise calibration has finished (256 frames, `GDFT.h:129`), subtract `1.5 ×` the saved per-bin noise and clamp at 0 (`GDFT.h:138-145`).
- `low_pass_array(..., SYSTEM_FPS, 1.0 + 10.0 * MOOD)` (`GDFT.h:149`). Default mood 0.05 makes that third argument 1.5. The body of `low_pass_array` was not opened, so that 1.5 is not claimed here as a Hertz cutoff.
- A slow peak follower: attack coefficient 0.005, release 0.0025, floor 4.0, then every bin is divided by that peak (`GDFT.h:169-198`). Absolute loudness is destroyed. The shape of the 64 bins is kept, scaled to about 0–1.

[FACT] `interlace_flip` is toggled with a comment that lower notes should alternate fields "to save execution time" (`GDFT.h:65-66`). Nothing in `process_GDFT` reads the flag. [FACT] `avg_bins()` is an empty `// TBD` (`lightshow_modes.h:51-53`). Neighbor-bin averaging was planned and not implemented in this tag.

[FACT] Novelty is the positive bin-to-bin change versus a 5-frame spectral history, averaged over 64 bins, then square-rooted (`GDFT.h:201-236`). Five frames at the hop below is about 37 ms. That is a transient tick, not a tempo memory.

[FACT] Hop is `SAMPLES_PER_CHUNK` 96 at 12800 Hz (`i2s_audio.h` read of the byte-identical file; the shift of `sample_window` by `CONFIG.SAMPLES_PER_CHUNK` is in that file). [INFERENCE] 96/12800 = 7.5 ms = 133.3 analysis frames per second. [FACT] The file header still says "256 new samples per frame" (`GDFT.h:47-48`). [FACT] The README claims "at 100 FPS" (`README.md:5`, same commit's tracked README). Those three rates disagree. The config number is 133 Hz. The README number is marketing or an older hop. The 256 in the comment matches neither.

**Visual pipeline.** [FACT] Internal pixels are `CRGB16` of `SQ15x16` channels. The struct comment says "Unsigned Q8.8" (`constants.h` in the untracked copy; the type is libfixmath `SQ15x16`, a 32-bit fixed value). [FACT] `show_leds` writes `uint8_t(leds_scaled[i].r * 255)` (`led_utilities.h:270-272` of the byte-identical file). The wire format of this tag is 8 bits per channel. [FACT] `TEMPORAL_DITHERING` defaults true (`globals.h:72`), and a 4-entry dither table exists (`constants.h` dither_table). The dither compare itself sits in the commented-out chromagram mode (`lightshow_modes.h:117-120`), not in the `show_leds` cast that was read. [INFERENCE] The flag is armed in config and not on the live cast path of this tag.

[FACT] Default show `light_mode_gdft` (`lightshow_modes.h:65-96`): one LED per bin on the first 64, then `shift_leds_up` by 64 and `mirror_image_downwards`. Centre is between the two halves of 128. Brightness is the smoothed bin, passed through one square-iter of `bin²·0.65 + bin·0.35` (default `SQUARE_ITER` 1, plus one extra when `chromatic_mode` is true). Hue, when chromatic, is `note_colors[i % 12]` (`lightshow_modes.h:86`): twelve equal hue steps, repeated every octave. That is a hue wheel by pitch class, not a curated palette. When chromatic mode is off, hue is `chroma_val + hue_position` plus a small energy term and a 10% spatial term (`lightshow_modes.h:88`).

[FACT] `chromatic_mode` is initialised `true` (`globals.h:151` area of the same struct block that was read; the assignment is in the byte-compared region below the defaults and was not in the palette-button diff). `AUTO_COLOR_SHIFT` defaults false. So boot colour is the 12-hue wheel, and the novelty-driven hue walk is off until asked.

[FACT] Chromagram fold: `make_smooth_chromagram` (`led_utilities.h:1209-1213`) sums `spectrogram_smooth[i]` for `i < CHROMAGRAM_RANGE` into 12 pitch classes. Default range 60. [INFERENCE] 60/12 = 5 bins per class, and bins 60–63 never vote. Bloom then sums twelve `hsv(i/12)` colours with share `1/6`, clamps each channel at 1, squares them `SQUARE_ITER` times, forces saturation, and writes the one colour into pixels 63 and 64 (`lightshow_modes.h:451-482`). Adding twelve hues in RGB is how this family goes white on dense chords. `force_saturation` is the repair, and it throws the chord mixture away, leaving one saturated colour.

[FACT] Bloom transport: `draw_sprite(..., 0.250 + 1.750 * CONFIG.MOOD, 0.99)` (`lightshow_modes.h:402`) scrolls the previous image. The new colour is only at the centre pair. The trail is the scrolled image, which already contains colour. This is not "brightness only, colour at the end".

[FACT] Other live painters in the same file: `light_mode_chromagram_gradient` interpolates the 12-note chromagram across 64 LEDs and mirrors (`lightshow_modes.h:343-363`); `light_mode_chromagram_dots` draws 12 pitch-class dots outward from centre (`lightshow_modes.h:366-395`); `light_mode_vu_dot` starts at line 180. `light_mode_gdft_chromagram` and the old two-speed bloom are inside block comments (`lightshow_modes.h:98-178`). The README still lists six modes including both bloom speeds (`README.md:52-76`). The README is ahead of, or behind, the code that is actually compiled. [INFERENCE] The videos the README points at are the spectrogram (octave-coloured notes) and bloom (centre injection, outward diffusion). Those are the two ideas the later trees keep rebuilding.

[FACT] Knobs, from the README and the code. Photons is brightness, and several call sites square it (`PHOTONS * PHOTONS`, `led_utilities.h:126`). Mood is reaction speed: README lines 36-38 say a low mood shows the chord progression and a high mood is "a little too flashy". The code agrees: mood scales bloom sprite speed, the GDFT low-pass argument, and shift speed in the kaleidoscope painter. Chroma chooses manual colour versus notation colour (`README.md:32-34`). Sweet spot is three base LEDs, not strip pixels: too quiet, in range, or near clipping (`README.md:83-86`; thresholds in `i2s_audio.h` around the lines read, min raised by 10% during calibration). Noise button: 256 GDFT frames of per-bin max, saved, then subtracted at 1.5× (`noise_cal.h`, `GDFT.h:121-135`). README calls that "3-second"; 256 frames at 133 Hz is about 1.9 s. [INFERENCE] The README's 3 seconds assumes a slower frame.

[FACT] Incandescent lookup is `{1.0000, 0.4453, 0.1562}` (`constants.h:202` at HEAD). `apply_incandescent_filter` mixes every pixel toward that tint (`led_utilities.h:277-284`). Default mix 0.50. It exists to "reduce harsh blues" (serial help text in `serial_menu.h:330`). Base coat draws a dim centre-origin line behind the image when enabled (`led_utilities.h:642-667`); default off; the serial help says it "improves appearance". Prism (`led_utilities.h:1133-1143`) repeatedly halves the image, shifts it 64, mirrors, and adds it back. Default iterations 0. Auto colour shift (`led_utilities.h:1150-1199`) cubes positive novelty, caps the step at 0.05, and walks `hue_position`. It is a slow hue drift keyed to "something new happened", not a palette.

**What 4.1.1 solved that the others had not yet.** [INFERENCE] It is the first of the three to make pitch class visible as space: one semitone, one light, mirrored from centre, with a separate "how twitchy" knob. The per-bin Goertzel length is the part an FFT cannot do without zero-padding tricks. Bloom is the part that turns a chord into one travelling colour instead of a bar graph.

**Budget cuts, with numbers.**

| Cut | Kept | Destroyed |
|---|---|---|
| 64 semitone bins, offset 12 | Pitch identity from 110 Hz to 4186 Hz | Everything below 110 Hz; inharmonic energy between semitones |
| Per-bin window, cap 2000 | ~2–156 ms time/pitch trade | A single global time resolution |
| Hann built, not applied | CPU | Sidelobe rejection. Rectangular window leaks into neighbors |
| Sample `>> 6` inside Goertzel | Headroom in the Q14 recurrence | 6 bits of sample |
| EMA 0.3 then mood low-pass then 0.75 spectrogram smooth | A readable envelope | Attack transient shape |
| Peak divide, floor 4 | Relative spectrum | Absolute loudness, so a drop and a verse can look the same size |
| Noise floor × 1.5 subtract | Silence stays dark | Quiet musical partials near the room tone |
| 64 → 12 chromagram, range 60 | One colour for bloom | Which octave, and bins 60–63 |
| `SQUARE_ITER` | Contrast | Low-level harmonics |
| 8-bit cast at show | WS2812 wire | The SQ15x16 render |
| 5-frame novelty | An onset tick | Tempo over bars |

### Emotiscope — same author, float image, tempo as a second Goertzel

**Audio.** [FACT] `src/global_defines.h:13-38` at `d0fef9a`: `NUM_LEDS 128` ("MUST be divisible by 2"), `NUM_FREQS 64`, novelty logged at 50 Hz, history 1024 (the comment says "50 FPS for 20.48 seconds"), `NUM_TEMPI 96`, `TEMPO_LOW 48`, `TEMPO_HIGH` is `TEMPO_LOW + NUM_TEMPI` so the macro equals 144, `BEAT_SHIFT_PERCENT 0.16`. [FACT] `src/microphone.h:23-26`: `CHUNK_SIZE 64`, `SAMPLE_RATE 12800`, `SAMPLE_HISTORY_LENGTH 4096`. [INFERENCE] The audio hop is 64/12800 = 5.0 ms = 200 Hz, faster than Sensory Bridge's 133 Hz, on the same 12800 Hz clock.

[FACT] `src/goertzel.h:20-21`: `BOTTOM_NOTE 12`, `NOTE_STEP 2`, and the comment says the table is quarter-steps (24 per octave) while the step of 2 selects half-steps. [FACT] `init_goertzel_constants_musical` (`goertzel.h:85-89`) sets bin `i` to `notes[12 + i*2]`. The table starts at 55 Hz and the second entry is a quarter-tone (`goertzel.h:26-27`). [INFERENCE] Bin 0 is `55 * 2^(12/24) ≈ 77.8 Hz`. Bin 63 is `55 * 2^(138/24) ≈ 2960 Hz`. This bank is lower and shorter than Sensory Bridge's 110–4186 Hz bank. It still misses the bottom of a kick if the kick is below about 78 Hz, and it gives up the top octave Sensory Bridge kept.

[FACT] Bandwidth is `neighbor_distance_hz * 4`, and the neighbor is `notes[note ± 1]`, a quarter-step, not `± NOTE_STEP` (`goertzel.h:107-111`). Block size is `SAMPLE_RATE / bandwidth`, forced down to a multiple of 4, capped at `SAMPLE_HISTORY_LENGTH - 1` = 4095 (`goertzel.h:57-68`). [INFERENCE] The cap is about 320 ms, twice Sensory Bridge's 156 ms cap. The `* 4` on a quarter-step is roughly two semitone-widths. Whether that was meant to match the semitone bin spacing is a hypothesis: the index arithmetic does not use `NOTE_STEP`.

[FACT] A Gaussian window is built (`goertzel.h:115-128`, sigma 0.8; Hamming and Blackman-Harris are commented alternatives) and it is applied (`goertzel.h:195`). Sensory Bridge built a Hann and did not apply it. Emotiscope also discards phase on purpose: `//float sine = sin(w); // Unused since phase info is not needed` (`goertzel.h:81`).

[FACT] A linear bank `full_spectrum_frequencies[64]` from 50 Hz to 6400 Hz in equal Hz steps sits in `goertzel.h:29-37`. No call of it was found in `goertzel.h` or `cpu_core.h`. [INFERENCE] It is unused by the musical init. 64 linear bins at 12800 Hz would be about 100 Hz wide and would include energy from 50 Hz, which the musical bank skips. It was the obvious way to see sub-bass, and this revision does not run it.

[FACT] Smoothing: `NUM_SPECTROGRAM_AVERAGE_SAMPLES 12` (`goertzel.h:52-54`), a 12-frame box, plus the smooth array. At a 200 Hz hop that is 60 ms. There is also a per-bin magnitude scale that is `(bin/64)^4 * 0.9975 + 0.0025` (`goertzel.h:207-210`), a strong tilt that lifts high bins and nearly silences bin 0's contribution to the returned magnitude (the added floor is 0.0025). [INFERENCE] That tilt is a budget-free loudness compensation, and it is also why a low bin can be numerically present and still almost dark.

[FACT] Noise calibration waits 256 frames then collects 512 (`goertzel.h:23-24`).

**Tempo bank.** [FACT] `src/tempo.h` builds 96 tempi. The loop is `progress = i / NUM_TEMPI`, `tempo = (TEMPO_HIGH - TEMPO_LOW) * progress + TEMPO_LOW` (`tempo.h:53-56`). [INFERENCE] That is BPM `48 + i` for `i` in 0..95, so 48 through 143 BPM, 1 BPM apart. The macro `TEMPO_HIGH` prints as 144 and is never itself a bin. Each bin is another Goertzel, this time on the novelty curve, windowed, with phase `atan2 + π * 0.16` (`tempo.h:145-160`). History is 1024 novelty samples at 50 Hz = 20.48 s (`global_defines.h:25-28`). The phase is what lets a dot swing with the beat instead of merely flashing on an onset. Sensory Bridge's novelty memory is 5 frames and cannot do this.

**Visual.** [FACT] The image is `CRGBF` floats (`src/leds.h:11` and the `leds[NUM_LEDS]` declaration). [FACT] `transmit_leds` (`led_driver.h:247-258`) quantises that float image to 8-bit with temporal dithering, explicitly "so the 8-bit LEDs emulate a higher bit-depth" by persistence of vision, then splits the 128 pixels across two RMT channels. The dither is a budget repair for WS2812, not an aesthetic. On a 16-bit WS2816 wire it is the wrong repair: it adds flicker in exchange for bits the wire can now carry directly.

[FACT] GPU core (`gpu_core.h:78-107`), Core 0: every frame, low-pass every pixel, tone map, warmth, white balance `{1.0, 0.9375, 0.84}` (`leds.h:23` area), master brightness, gamma, then the 8-bit transmit. Softness sets the cutoff: `0.5 + (1 - sqrt(softness)) * 14.5` (`gpu_core.h:85`). Default softness is 0.25 (`configuration.h:35`). [INFERENCE] That expression equals 7.75 at softness 0.25, 15 at softness 0, and 0.5 at softness 1. The unit is whatever `apply_image_lpf` treats as a cutoff; the name is frequency. [FACT] `apply_frame_blending` exists (`leds.h:534-543`) and the call is commented out (`gpu_core.h:92`), as is phosphor decay. Frame blending was tried and parked. Softness survived as the image low-pass and as `fade_display`, which multiplies the whole image by softness (`leds.h:705-706`).

[FACT] Dual core, from `EMOTISCOPE_FIRMWARE.ino:35-40` and `:118-137`: Core 1 runs audio and the web server; Core 0 runs graphics, started with `xTaskCreatePinnedToCore(..., 4096, ..., 0)`. The author's comment in `gpu_core.h:82-84` is the budget story in his own words: in 2012 he had a 16 MHz AVR; the ESP32-S3's DMA is why a per-pixel low-pass at hundreds of frames per second is allowed. [FACT] README line 11 says the 128 LEDs refresh at up to 450 FPS. That claim was not measured here. The audio hop is 200 Hz, so a 450 Hz LED rate would be repeating or interpolating analysis.

**Modes, and why they read as music.** Registered active modes (`light_modes.h:38-44`): Analog, Spectrum, Octave, Metronome, Spectronome, Hype, Bloom. Neutral and Self Test are not shows. Perlin is not in this tree. Waveform is `src/light_modes/beta/waveform.h` and is not in the registered table.

- [FACT] Spectrum (`spectrum.h:1-15`): mirror mode paints `spectrogram_smooth[i]` on both halves with `get_color_range_hue(progress)`. Non-mirror interpolates 64 bins across 128 LEDs. Colour follows position. Brightness follows the bin. Same idea as Sensory Bridge GDFT, with a user colour range instead of a fixed 12-hue wheel.
- [FACT] Octave (`octave.h:1-14`): the same painter fed by the 12-bin chromagram. All octaves of a pitch class collapse into one spatial hump. This is the "chord" view. It is also the over-average the later palette work had to climb back out of, if someone uses it as the only colour.
- [FACT] Bloom (`bloom.h:1-38`): a float trail `novelty_image`, sprite speed `0.125 + 0.875*speed`, decay 0.99, injection of `vu_level` at array index 0. The mirror painter uses index `i` as distance from centre. So index 0 is the centre sample, not LED 0. Colour is still `get_color_range_hue(progress)` at paint time, while the trail array stores only the novelty/VU scalar. [FACT] This is the brightness-only trail, already, in Emotiscope bloom: the history buffer is `float novelty_image_prev[NUM_LEDS]`, and colour is applied when the float is written to `leds`. Sensory Bridge 4.1.1 bloom stored `CRGB16` and scrolled colour.
- [FACT] Analog (`analog.h`): one dot whose position is a smoothed VU, drawn as a pair leaving centre when mirrored. It is a loudness needle, not a spectrum.
- [FACT] Metronome (`metronome.h:1-40`): each of 96 tempo bins contributes a dot whose position is `sin(phase)`. Phase is the thing Sensory Bridge threw away. The dot swings. Opacity is that bin's share of tempo power.
- [FACT] Spectronome (`spectronome.h:1-8`): draw the spectrum, then darken it by tempo confidence, then draw the metronome on top. The spectrum is the harmony. The dots are the beat. Confidence is allowed to dim the harmony when the tempo guess is bad, which is an honesty choice.
- [FACT] Hype (`hype.h:1-20`): sums even tempo bins and odd tempo bins separately, then paints two colours. It is a two-bar or polyrhythm split of the same 96-bin bank. Whether even/odd bins correspond to a musical subdivision depends on the 1 BPM grid; that mapping was not proved here. [HYPOTHESIS] Even versus odd on a 1 BPM grid does not cleanly mean "on-beat versus off-beat". It still shows two tempo centres at once, which a single BPM number cannot.
- [FACT] Beta waveform (`waveform.h:1-42`): copies 128 raw samples, low-passes them at `110 + 2000 Hz` (the bass term is commented out, so the cutoff is fixed), peak-normalises, and maps sample height to brightness. Mirror writes `leds[63-i]` and `leds[64+i]`. Centre of the 128 is explicit. Colour is a spatial ramp of `configuration.color`, not the spectrum. Peak-normalise means a quiet waveform fills the strip as much as a loud one.

[FACT] Downloads copies, not git: `~/Downloads/Emotiscope.sourcecode/Emotiscope-1.1/src/global_defines.h` has the same 128 / 64 / 96. `Emotiscope-2.0/main/microphone.h:20-21` uses `CHUNK_SIZE (SAMPLE_RATE / 100)` so 128 samples, a 100 Hz hop. `Emotiscope-2.0/main/perlin.h` exists. Perlin is a 2.0 download, not the git HEAD mode list. `~/Downloads/Emotiscope-1` is the same git HEAD `d0fef9a`.

**What Emotiscope solved that Sensory Bridge had not.** [INFERENCE] A 20-second tempo hypothesis bank with phase, so light can swing between beats; a float image so the low-pass and the dither see fractions of a code; a Gaussian window that is actually applied; bloom's trail reduced to one float per pixel so colour can be chosen at the end. The cost is a second full Goertzel (96 more detectors) on a decimated novelty curve, which is why it lives on the other core.

### Sensory Bridge, SpectraSynq fork, June 2026 — 80 bins, palettes, Spectrum River

This is the tree the swarm brief means by commits `6109a1e` and `d408433`. It is not the 2024 tag.

[FACT] `6109a1eeb4fe6f6f14b1669995561ea5c90ac9dd`, 2026-06-02, author SpectraSynq, subject `feat(effects): effect-library overhaul`. The message says Spectrum River is "80-bin spectrum mapped to space, flowed outward. Captain 10/10." The same commit's message says GDFT, the chromagrams, VU dot, kaleidoscope and others are disabled, and the default mode changes from GDFT to BLOOM. A tempo tracker `sb_tempo` is compiled and "UNWIRED". Sample rate is not changed. Build note in the message: RAM 25.6%, flash 8.9% on `k1_hardware`.

[FACT] At that commit, `SENSORY_BRIDGE_FIRMWARE/constants.h:21-24`: `NATIVE_RESOLUTION 160`, `NUM_FREQS 80`, with the comment that each bin maps to one pixel of a half and the mirror anchor is `NATIVE_RESOLUTION/2`. [FACT] `config_types.h` at `d408433` still has `DEFAULT_SAMPLE_RATE 12800` and `DEFAULT_SAMPLES_PER_CHUNK 96`, and `globals_config.cpp:70-71` still has `CHROMAGRAM_RANGE 60`. So the strip grew from 128 to 160 and the bin count from 64 to 80, and the chromagram default still only looks at 60 of those bins.

[FACT] Spectrum River at `6109a1e`, `light_mode_spectrum_river.cpp:7-70`: bin 0 (bass) is the centre side of the upper half, bin 79 the outer edge. Each frame `draw_sprite` carries the previous `CRGB16` image outward (drift `0.55 * (160/128)`, trail alpha `0.90`). Then bin `k` is added at pixel `HALF + k` with `hue = k / (NUM_FREQS - 1)` and brightness from the bin after up to 4 squarings, gain 0.90, floor 0.015. Colour comes from `palette_manual_colour`, and the comment says "NEVER raw HSV". The history buffer stores colour, not a scalar. Bass wells at the centre and streams out. This is the mode that paints the spectrum as space. It was not made the default. Bloom was.

[FACT] `d408433067ddc61cf1252ce13e33d17943a17859`, 2026-06-11, `feat(vp): HD palette sampling`. The message says the old chain lost resolution three times: `CRGBPalette16` flattened every gradient to 16 entries, the coordinate was rounded to `uint8`, and the sample came back as 8-bit `CRGB`. [FACT] The code at `visual/lightshow_modes.h:109-248` of that commit: unpack up to 48 gradient stops (`PAL_HD_MAX_STOPS 48`) as floats once per palette switch; interpolate the real stop segment; apply auto-shift phase in float; emit `CRGB16`. Cold cache falls back to `ColorFromPalette(..., uint8_t(h * 255), 255, LINEARBLEND)` (`lightshow_modes.h:226-229`). [INFERENCE] A palette with 2 stops is unchanged. A palette with 40 stops used to be resampled to 16 and then to 256 indices. The HD path keeps the authored stops. That is the "over-averaging" inside the palette, and the commit message does not mention the 80 bins. The 80-bin map is the river, nine days earlier.

[FACT] At the same commit the spectrogram smoother comment says the symmetric 0.75/0.75 "flattened transients" and that attack/release are now split (`visual/lightshow_modes.h:25-32`). The constants are still both `0.75f` with the comment "prior symmetric" (`system/constants.h:288-289` at that commit, and still at working-tree `SENSORY_BRIDGE_FIRMWARE/system/constants.h:351-352` on HEAD `3709862c`). [FACT] The split was named and not given two different numbers. Titan's reconstruction of "the 0.75 smoother" matches the numbers that actually shipped, not the comment that wished they had changed.

**What this fork solved.** [INFERENCE] It separated three things the 2024 tag had glued together: which pitch is lit (the bin's place on the strip), which palette colour that place should be (`k / 79`, not a hue wheel), and how the palette is stored (real stops, 16-bit, not a 16-entry 8-bit table). It also moved the geometry to 160 with the centre at the half, which is the Titan strip's logical shape. It did not turn the river on by default, and it left the 12-note bloom path as the boot show.

### Lightwave and LightwaveOS

[FACT] `/Users/spectrasynq/Workspace_Management/Software/LightwaveOS_Official` HEAD `b0141c881b49` (2026-05-23). `git show HEAD:src/constants.h` lines 10, 14, 15, 16: `DEFAULT_SAMPLE_RATE 16000`, `NATIVE_RESOLUTION 160`, `NUM_FREQS 96`, `NUM_ZONES 2`. Working-tree `src/constants.h` is dirty; those four macros match HEAD. [FACT] The mode enum in the dirty working tree (read, so treat mode names as the dirty file, not a clean HEAD line) lists GDFT, two chromagrams, bloom, VU dot, kaleidoscope described as "three color channels 2D Perlin noise affected by the onsets of low, mid and high", quantum collapse, and snapwave ("snappy attack with trails"). Two LED data pins are `LED_STRIP1_DATA 4` and `LED_STRIP2_DATA 5` in that same working-tree header. [INFERENCE] Two pins plus `NUM_ZONES 2` is the dual-strip idea. Whether each pin is a full scene or a half of one scene is not proved from the pin macros alone. Titan's later ruling is one 160-pixel scene split 80/80, and that ruling is the one to keep.

[FACT] LightwaveOS commit `30b084e2dcdae0e9ac7688823dfced7945b9b46a`, 2025-08-12, subject `feat(visual-pipeline): complete 96-bin restoration`. `git grep` at that commit, `src/lightshow_modes.h:194` and `:208`: `paletteIndex = uint8_t(float(freq_prog) * 255)` and `ColorFromPalette(pal, paletteIndex, uint8_t(float(bin) * 255))`. Colour is a palette index from `freq_prog`. Brightness is the bin, quantised to 8 bits, through a 16-entry FastLED palette. [INFERENCE] If `freq_prog` is LED progress along the half, as the name and the later forensics quote say, then colour follows space and energy follows the interpolated bin. That is the split Emotiscope bloom already had for a float trail, here applied to a spectrum painter, still crushed to 16 palette entries and 8-bit colour. The `freq_prog` assignment line was not re-opened in this pass; the index and the `ColorFromPalette` call were.

[FACT] K1.Lightwave commit `63f0cac` (seen in `git log` this session,  the subject is `feat(dsp): migrate to 32kHz sample rate with 96-bin Goertzel and perceptual weighting`). That is a later, faster analysis rate than LightwaveOS HEAD's 16 kHz. The body of that commit was not opened. It is recorded so the 32 kHz attempt is not forgotten. [HYPOTHESIS] 32 kHz was a try at hearing more of the top octave and shortening the same hop in milliseconds; it is not what Titan's production contract selected.

[FACT] Lightwave-Ledstrip checkout `4d71bc012d18` (2026-08-20). February 2026 history on that repo is mostly repository restructuring (the dated log from 2026-02-04 and 2026-02-27 was read). No February 2026 subject in the sample that was printed says "trails carry brightness only". March 2026 does contain trail work (`2afdead2` silence gating and trail decay, `d6736fa3` trail persistence on spectrum bars, `45e0e066` trail persistence on SB Full Spectrum). [FACT] `firmware-v3/src/effects/PatternRegistry.cpp` tags many patterns `CENTER_ORIGIN`, and some `DUAL_STRIP` (holographic, modal resonance, interference scanner, wave collision; lines read in the registry around 44–47). A `zone-mixer/` project exists beside the firmware. [FACT] The brightness-only sentence the swarm brief attributes to "Lightwave Feb 2026" was not found as a February commit in this pass. The same design is already in Emotiscope bloom (float trail, colour at paint) and in the August 2025 LightwaveOS spectrum line (bin scales brightness, `freq_prog` picks the palette index). Titan's own header states the later form, quoted in the next section.

**What Lightwave added.** [INFERENCE] A 96-bin bank at 16 kHz on a 160-pixel, two-pin, zone-tagged product, snapwave as a "hit" with a trail, and the explicit palette-index-from-position rule a year before the HD sampler. It also kept the 16-entry 8-bit `ColorFromPalette` path that `d408433` later called the three resolution losses.

### Titan, as it stands in this worktree

[FACT] Production audio contract `src/k1/core/audio/audio_rate_config.h:7-27` and `:41-48`: 24000 Hz, hop 180 samples, 80 bins, crossover bin 40, novelty decimated by 3. The comment says the hop stays 7.5 ms, so the analysis rate stays 133.333 Hz and novelty stays 44.444 Hz, the same as the retired 12800 Hz / 96-sample tuple, which is kept as `kLegacy12800AudioRate`. [INFERENCE] The sample rate went up so the same 7.5 ms hop contains 180 samples instead of 96. Time resolution of the frame did not improve. Frequency headroom did: Nyquist moved from 6400 Hz to 12000 Hz. The note table did not follow it to the new Nyquist.

[FACT] `src/k1/core/audio/gdft_goertzel.cpp:18-26`: `kNoteOffset = 12`, `kMaximumBlockSize = 2000`, and the note table begins `55, 58.27, ...` with `110.0000` twelve semitones in. The file contains no line matching `window`. [INFERENCE] The rectangular Goertzel of Sensory Bridge 4.1.1, including the 2000-sample cap and the 110 Hz floor, is the port. Emotiscope's Gaussian window was not ported into this file.

[FACT] `src/k1/core/audio/sub_band.h:3-7`: the note bank starts at 110 Hz, so kick and sub-bass are invisible to it. A separate biquad watches 40–110 Hz, centred on `sqrt(40×110)`, and only as a release trigger. It does not paint.

[FACT] `src/k1/core/visual/inhouse_waveform.cpp:144-175`: the spectrogram smoother is the symmetric 0.75, rewritten as `1 - 0.25^(dt * 100)` so it matches a 100 Hz frame. Then up to 80 bins are folded with `chroma[i % 12]`. At a range of 80, pitch classes 0–7 receive 7 bins and classes 8–11 receive 6, because 80 is not divisible by 12. The fold is then normalised by its own peak (`chroma_max_`). A flatness gate can zero it. The colour coordinate of this stage is the circular mean of the 12 classes (`atan2` of the hue-wheel sum, lines 167-176).

[FACT] v8, compiled when `K1_INHOUSE_MAPPING_V9` is absent (`scripts/live_boot_composition.py:10-14`: profile 8 adds no define, profile 9 adds `-DK1_INHOUSE_MAPPING_V9=1`). For mode 32 the file samples the palette once from `centroid + walk` and once from a fallback, blends those two RGB triples, and exponential-averages the dot's red, green and blue (`inhouse_waveform.cpp:209-238`). One coordinate, one mixed colour, per frame. The fallback can still pick the brightest of 16 compiled FastLED entries (`inhouse_waveform.cpp:222-231`).

[FACT] v9, inside the define (`inhouse_waveform.h:16-47`): modes 32 and 18 only. The constexpr palette variant is `kPerPixelMapped`. The comment at lines 40-41 says trails carry intensity and the deposited palette offset, and colour is sampled per pixel at output. [FACT] `outputV9` (`inhouse_waveform.cpp:872-901`) does that: `intensity_[i]` is the level, `offset_[i]` is the deposited position, and when mapped the pixel's colour is `sampleProductPaletteHd` at `offset + distance/80`, with distance 0 at the centre and about 1 at the edge of a half. `product_palette.cpp` holds both `sampleProductPaletteHd` and `sampleProductPaletteFastLed16`. The 16-slot sampler remains compiled, and v9's host probe can force it with fault `kSlot16` (`inhouse_waveform.cpp:895-898`).

[FACT] `build_scalar.py:1175` records `precision_bits: 16` and `pixels_per_board: 160` for the in-house waveform sources. [FACT] `palette_runtime.cpp` around the lines grepped splits one scene at logical pixels 79/80 onto P601 and P604, 80 pixels each. That matches the continuous-160 ruling. It is not two independent Lightwave strips.

[FACT] v9 release and accent structs in `inhouse_waveform.h:53-76` default to off (`boost = 0`, `enabled = false`). The "hits harder" lever exists in source as a disabled envelope. Sub-bass can drive it only if `trigger_sub` is turned on. The default does not.

**What Titan has already taken.** The 80-bin, 160-pixel, centre-at-79/80 geometry. The per-bin Goertzel length and the 2000 cap. The 7.5 ms hop. The 0.75 smoother. The 12-note fold. The HD stop sampler. On the v9 build only, the brightness-plus-position trail and per-pixel colour. A 40–110 Hz side-channel that does not yet drive the picture.

**What Titan has not taken.** Spectrum River's `hue = k/(N-1)` with each bin's energy at its own pixel. Emotiscope's 96-tempo phase bank. Emotiscope's Gaussian window. Emotiscope's per-pixel image low-pass as a softness knob. The linear 50 Hz–Nyquist bank that Emotiscope defined and did not run. Lightwave's 96-bin / 16 kHz (or the 32 kHz attempt). Making the river, rather than bloom/waveform, the boot picture.

## Cross-project comparison

| | Sensory Bridge 4.1.1 `5f329261` | Emotiscope `d0fef9a` | SB9 river `6109a1e` / HD `d408433` | LightwaveOS HEAD `b0141c88` and `30b084e2` | Titan this worktree |
|---|---|---|---|---|---|
| Clock | 12800 Hz | 12800 Hz | 12800 Hz, hop 96 | 16000 Hz at HEAD; a K1.Lightwave subject says 32 kHz was tried | 24000 Hz, hop 180. Legacy 12800/96 kept |
| Frame | 7.5 ms, 133 Hz. README says 100. Comment says 256 samples | 5 ms, 200 Hz audio. README says up to 450 LED FPS | 7.5 ms | not re-measured at HEAD | 7.5 ms analysis, novelty 44.4 Hz, LED wire budget is a separate 120 Hz target in the swarm brief |
| Bins | 64 semitones, 110–4186 Hz | 64 semitones, about 78–2960 Hz. Unused linear 50–6400 Hz table | 80. Chromagram range still 60 | 96 | 80, offset 12, floor 110 Hz. Sub-band 40–110 Hz is a side chain |
| Window | Hann table, multiply commented out. Cap 2000 samples (156 ms) | Gaussian applied. Cap 4095 samples (320 ms). Phase discarded | inherited Goertzel; body not re-read | not re-opened | no `window` string in `gdft_goertzel.cpp`. Cap 2000 |
| LEDs | 128, mirror, centre between halves | 128, waveform mirror at 63/64 | 160, centre at half | 160, two data pins, zone tags | 160 logical, split 80/80 at 79/80 |
| Colour storage | SQ15x16, then `uint8` at show. Hue wheel of 12 | float `CRGBF`, then 8-bit temporal dither | up to 48 real stops, float coordinate, `CRGB16`. Fallback still 16×8-bit | `30b084e2`: 16-entry palette, index `freq_prog*255`, brightness `bin*255` | HD sampler present. v8 uses one RGB mix and can fall back to 16 slots. v9 path P samples HD per pixel |
| Trail | Bloom scrolls `CRGB16` (colour already in the trail) | Bloom trail is one float per pixel; colour at paint | River scrolls `CRGB16`; colour baked in at injection | spectrum painter bakes palette colour using position, brightness from the bin | v9 path P: intensity and palette offset in the trail, colour at output |
| What it alone did | Per-bin Goertzel length as the product, mood knob, sweet-spot meter, noise floor | 20.48 s, 96-tempo phase bank on the other core; float image low-pass | `k/(N-1)` palette position per bin; stop the 16-entry flatten | 96 bins, zones, two pins, snapwave | 1 GHz host, 16-bit wire, the v9 path that can use both |

## Upstream ideas not yet taken

Each line cites the upstream evidence and the Titan line that shows the idea is absent or only partial.

1. **Paint each bin on its own pixel with its own palette position.** [FACT] River `6109a1e` `light_mode_spectrum_river.cpp:58-65`, `hue = k / (NUM_FREQS - 1)`. [FACT] Titan mode-32 v8 uses one centroid (`inhouse_waveform.cpp:216-217`). v9 path P adds `distance/80` to one deposited offset (`inhouse_waveform.cpp:885-890`). That is a spatial ramp, not "bin 17 has the colour of stop 17 and the brightness of bin 17". The 80 energies still collapse through the 12-note fold before a single centroid is chosen (`inhouse_waveform.cpp:154-176`), and path P then replaces that centroid from "the pre-knee shape" (`inhouse_waveform.cpp:186-191`). The shape becomes one number, then a ramp.

2. **A tempo phase bank.** [FACT] Emotiscope 96 detectors, 48–143 BPM, phase drives the metronome dot (`tempo.h`, `metronome.h`). [FACT] Titan's in-house header records a 22.5 ms tempo update (`inhouse_waveform.h:11-13`) and a novelty rate of 44.4 Hz (`audio_rate_config.h:7-9`). There is no 96-wide phase bank in the files read. The swarm brief's half-time lock (88 BPM prior, P7 receipt) was not re-measured in this pass. [INFERENCE] A 20-second multi-tempo phase is the upstream tool that could move light between beats. Whether it would also fix the half-time lock is a hypothesis, not a result.

3. **Apply a window.** [FACT] Emotiscope multiplies by a Gaussian (`goertzel.h:195`). Sensory Bridge's Hann multiply is a comment (`GDFT.h:91`). [FACT] `gdft_goertzel.cpp` has no `window` line. [INFERENCE] Titan inherited the rectangular detector. Leakage between adjacent semitones is the cost. The CPU reason for skipping it was an ESP32. It is a small multiply on an M85.

4. **Hear below the first semitone as part of the picture, not only as a trigger.** [FACT] `sub_band.h:3-7` states the gap and limits the filter to the release trigger. The linear 50 Hz table in Emotiscope was never wired. Lightwave's 96-bin bank at 16 kHz still, from the files opened, starts from the same note-table family; the exact lowest LightwaveOS bin was not re-derived this pass.

5. **Softness as a per-pixel image low-pass the user can feel.** [FACT] Emotiscope `gpu_core.h:85-87`, default softness 0.25. Titan's waveform has its own followers (peak stages, silence scale, the 0.75 spectrum mix). There is no softness parameter of that GPU low-pass in `inhouse_waveform.cpp` as read. [INFERENCE] Some of the "smooth and musical" quality Captain remembers from Emotiscope is this low-pass, not the Goertzel.

6. **Stop crushing to 8 bits anywhere the wire is 16.** [FACT] Emotiscope dithers because the wire is 8-bit (`led_driver.h:247-253`). [FACT] Titan's build receipt records 16-bit precision, and v8 can still pick a colour from `compiled_fastled16` (`inhouse_waveform.cpp:225-231`). The HD path exists so this crush is no longer required for the palette. It remains as a fallback and as the v8 mix.

7. **Spectrum as the boot picture.** [FACT] `6109a1e` message: river scored "10/10" and the default was still changed to bloom. Titan's waveform modes 32 and 18 are the bloom/waveform family (`inhouse_waveform.h:19`, `build_scalar.py:1175` lists mode ids 32, 18, 8, 7, 11). The river was praised and then parked. [INFERENCE] Parking it is why a spectrogram of the song is not what the strip is known for now.

Not recommended, and not "missed value": restoring `note_colors[i % 12]` or any hue-wheel sweep. That was the 2024 chromatic default (`lightshow_modes.h:86`). The product rule is curated multi-colour palettes, and the river already replaced that wheel with palette position. Also not recommended: a brightness floor. Emotiscope and Sensory Bridge both clamp silence toward dark (noise subtract, sweet-spot min, bloom floor 0.015 on the river). A floor that lights the strip in silence fights true black.

## Inherited compromises that the Titan no longer needs for the old reason

1. **The 12-note fold in front of colour.** [FACT] It is how bloom made one colour (`lightshow_modes.h:451-460` at 4.1.1; still `CHROMAGRAM_RANGE 60` in SB9). [FACT] Titan still does it for every waveform frame before v8 or v9 (`inhouse_waveform.cpp:154-176`). The fold is useful for "what chord is this" and for a single centre colour. It is not useful if the picture is supposed to be the 80 bins. On an M85 the 80-bin paint is cheaper than the Goertzel that already computed the 80 bins. [INFERENCE] Keeping the fold for the waveform's colour is an inherited aesthetic of bloom, not a cycle budget.

2. **110 Hz floor and the 2000-sample cap, copied as constants.** [FACT] `gdft_goertzel.cpp:18-20` names `kNoteOffset = 12` and `kMaximumBlockSize = 2000`, the Sensory Bridge numbers (`system.h:238-241`, `globals.h:55`). At 24000 Hz a 2000-sample window is 83 ms, not 156 ms. [INFERENCE] The cap's original job was "do not scan 4000 samples of a 50 Hz bin on an ESP32". At 24 kHz the same cap is a shorter window, so low-note pitch resolution got worse in milliseconds while the CPU got much faster. The offset of 12 is why a drop's sub stays invisible to the bins that paint. The sub-band filter admits the authors already know this.

3. **Rectangular window.** See idea 3 above. The Hann was written and then commented out to save the multiply (`GDFT.h:91`). That multiply is no longer the frame budget.

4. **16-entry, 8-bit palette sampling as a live fallback.** [FACT] `d408433` exists so this would stop. [FACT] v8 and the cold/fallback path still have it (`inhouse_waveform.cpp:222-231`, and `kSlot16` in v9). [INFERENCE] Needed only for a host fault probe, not for the LEDs.

5. **One analysis hop of 7.5 ms, preserved on purpose.** [FACT] `audio_rate_config.h:7-10` says the 133 Hz rate is intentional parity with the old tuple. [INFERENCE] That one is not only a leftover. Emotiscope already ran the audio side at 200 Hz and claimed a much faster LED rate. Whether 7.5 ms is still the right musical grain is a taste and latency question. The LED wire time in the swarm brief is 5.1 ms, so the analysis hop is longer than the wire. Shortening the hop is a real lever. It is also the one most likely to be a chosen parity rather than an accident.

6. **Symmetric 0.75 attack and release.** [FACT] The SB9 comment says this flattens transients, and the constants stayed 0.75/0.75. [FACT] Titan encodes that same 0.75 against a 100 Hz frame (`inhouse_waveform.cpp:144-147`). [INFERENCE] This one may be wanted: it is the "musical" smear. It may also be why a transient does not look like a hit. The upstream author had already decided it was a problem and had not changed the number. Treat it as a knob, not as sacred parity.

7. **Peak-normalised spectrum.** [FACT] Sensory Bridge divides by a slow peak (`GDFT.h:192-198`). Emotiscope waveform divides by `max_val` (`waveform.h:8-15`). Titan's chroma stage divides by `chroma_max_` (`inhouse_waveform.cpp:158-163`). [INFERENCE] All three make a quiet chorus and a loud drop occupy the same numeric range. That is the opposite of "hits harder", and it was done so auto-gain would not clip the 8-bit LEDs or the fixed-point bins. With 16-bit headroom and an explicit brightness cap, absolute energy can survive further into the picture. The v9 level ceiling at −9 dBFS (`inhouse_waveform.h:49`) is a later, separate compressor. Stacking it on a peak-normalised chroma is two compressors.

8. **Square-iter contrast.** [FACT] Default 1 in 4.1.1, and the river squares up to 4 times (`RIVER_MAX_ITERS 4`). [FACT] Titan v8 does `value*value*1.5` inside the chroma energy (`inhouse_waveform.cpp:206`). [INFERENCE] Squaring is an aesthetic (make the peaks pop, bury the haze). It also deletes the quiet partials that tell similar chords apart. It does not save CPU in any way that matters now.

## Lever records

Scores are [INFERENCE]. "Still needed" means "needed for the look or the wire", not "needed because the ESP32 was slow".

### D04-L01 — semitone bank and the 110 Hz floor

- id: D04-L01
- container: AP
- stage: `src/k1/core/audio/gdft_goertzel.cpp:18-32` (`kNoteOffset`, `kNotes`); upstream `SENSORY_BRIDGE_FIRMWARE/system.h:209-238` at `5f329261`
- what it controls: which pitches can move colour, brightness and space. Motion of a sub-drop cannot start here.
- current setting on Titan: v8 and v9 share this bank. 80 bins. First painted bin 110 Hz. v9 adds `sub_band.h` 40–110 Hz only as a release trigger, default off (`inhouse_waveform.h:73` `trigger_sub = false`).
- information preserved or destroyed: preserves 80 semitones from index 12 (110 Hz) through index 91. The table in `gdft_goertzel.cpp` is 96 entries ending at 13289.75 Hz; index 91 is the second value of the last row, 10548 Hz, which is under the 12 kHz Nyquist of a 24 kHz clock. Destroys 40–110 Hz inside the picture. The four table entries above index 91 are not in the 80-bin production count.
- origin: inherited from Sensory Bridge `NOTE_OFFSET 12` (`globals.h:55` at `5f329261`). S3 budget plus the artistic choice "one piano key, one bin". The floor is a consequence of the offset, not a measured musical decision about kicks.
- still needed on Titan: the semitone spacing, yes, if the look is melodic. The 110 Hz floor, no. Cost of adding bins below 110 Hz was the long window on the ESP32 (up to 156 ms at 12800 Hz, cap 2000). On the M85 that window is the thing to re-budget, not to forbid.
- manipulation options: lower `kNoteOffset` toward 0 (55 Hz) or insert a few low bins; let `sub_band` scale deposit width or level; do not replace the semitone bank with a hue wheel.
- interactions: D04-L02 (window cap), D04-L07 (peak AGC hides the new energy), D04-L10 (river would show the new bins as inner pixels), v9 release struct.
- visual impact: 4. A drop whose energy is under 110 Hz can finally move the picture.
- novelty: 3. The filter already exists. Wiring it into the paint is the new part.
- effort: 3.
- risk: low bins have long windows and will lag the hit by tens of milliseconds. A naive "more bass, more light" will also swell on rumble.
- smallest decisive experiment: freeze, before looking, that a 50 Hz sine at a fixed digital level moves at least one of {deposit width, inner-pixel energy} by a threshold written with `scripts/freeze_tolerance.py`, and that a 200 Hz sine at the same level does not take that same slot. Planted fault: compile with `kNoteOffset` forced so the lowest bin stays 110 Hz and `trigger_sub` ignored; the 50 Hz case must go red and the 200 Hz case must stay green.

### D04-L02 — per-bin window length and the unused Hann

- id: D04-L02
- container: AP
- stage: `gdft_goertzel.cpp:20` cap 2000; upstream `GDFT.h:91-92` and `system.h:238-241` at `5f329261`; Emotiscope apply site `goertzel.h:195` at `d0fef9a`
- what it controls: how fast a note can appear, and how much it leaks into the next semitone. Leakage becomes false colour and false width.
- current setting: cap 2000 samples (83 ms at 24 kHz). No window multiply in `gdft_goertzel.cpp`.
- information preserved or destroyed: keeps a different time constant per note. Destroys sidelobe rejection. The `>> 6` inside the old recurrence (`GDFT.h:94`) was an extra 6-bit cut; whether the Titan recurrence still shifts was not confirmed in the inner loop, which was not read past the constants.
- origin: S3 budget. The Hann was written and commented out. Emotiscope later applied a Gaussian and did not come back into this file.
- still needed: the per-bin length, yes. The missing window, no. The 2000 cap should be re-derived at 24 kHz, not copied.
- manipulation options: apply the existing Gaussian or the written Hann; raise the cap only for bins under 110 Hz; leave high bins short so cymbals stay fast.
- interactions: D04-L01, D04-L06 (leakage pollutes the 12-note fold).
- visual impact: 3. Cleaner pitch identity, slightly slower low notes if the window grows.
- novelty: 2. Emotiscope already did the window.
- effort: 2.
- risk: a long window on a low bin adds lag and can make the drop look late. That is the trade the original comment described (`GDFT.h:36-40`).
- smallest decisive experiment: freeze a two-semitone pair (A440 and A#) at equal amplitude. With a window, the idle bin must stay under a frozen fraction of the driven bin. Planted fault: the multiply forced to 1 (rectangular). The idle bin must rise above the fraction and fail the frozen check.

### D04-L03 — spectrogram smoother 0.75 / 0.75

- id: D04-L03
- container: AP to VP
- stage: `inhouse_waveform.cpp:144-152`; upstream `lightshow_modes.h:1-15` at `5f329261` and `visual/lightshow_modes.h:25-32` plus `constants.h:288-289` at `d408433`
- what it controls: how quickly brightness in a bin, and therefore width and trail injection, follows the music.
- current setting: symmetric, matched to 0.75 at a 100 Hz frame, v8 and v9 (the stage is above the v9 split).
- information preserved or destroyed: keeps a smooth envelope. A step input is geometrically decayed. Attack and release are the same, so a hit falls as slowly as it rose. The SB9 comment says this flattened transients; the constants were never given two values.
- origin: aesthetic, later mislabeled as a fix. Not an S3 cycle save of any size (it is one multiply per bin).
- still needed: a smoother, yes, or the strip chatters. This amount, unknown. Captain has to see the A/B.
- manipulation options: attack 0.9, release 0.2, as two frozen pairs; or the inverse. Do not remove the smoother entirely in the first test.
- interactions: D04-L07, D04-L09, the disabled v9 accent.
- visual impact: 4. This is "hits" versus "washes".
- novelty: 2. The comment already asked for it.
- effort: 1.
- risk: fast attack looks nervous on vocals. That is the mood knob's whole range (`README.md:36-38`).
- smallest decisive experiment: freeze the frame index of peak deposit width after a 20 ms noise burst following 500 ms of silence. Asymmetric attack must peak earlier than the 0.75/0.75 control by a frozen number of frames. Planted fault: coefficients forced equal at 0.75; the time-to-peak difference must collapse and fail.

### D04-L04 — 80 bins folded into 12 notes

- id: D04-L04
- container: map
- stage: `inhouse_waveform.cpp:154-176`; upstream `led_utilities.h:1209-1213` at `5f329261`, `CHROMAGRAM_RANGE 60`
- what it controls: the single hue-wheel angle that v8 paints everywhere, and the centroid v9 starts from before path P's spatial ramp.
- current setting: `chroma_range` clamped to 1..80, modulo 12, then peak-normalised. v8 and v9 both compute it. v9 path P can ignore it for the per-pixel colour and still uses a shape centroid.
- information preserved or destroyed: 80 numbers become 12, and 12 become 1 angle. Octave is destroyed. At range 80 the classes are uneven (7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6). Range 60 would have been exactly 5, which is the old default, and Titan's clamp allows 80.
- origin: bloom needed one colour (`lightshow_modes.h:451-460`). Inherited aesthetic. The ESP32 could have summed 12 adds easily; this was not a cycle crisis.
- still needed: for a chord-coloured centre, yes. For a spectrum picture, no.
- manipulation options: keep the fold for bloom's centre colour; stop using it as the only colour of a waveform that claims to be spectral. Do not return to `note_colors[i%12]`.
- interactions: D04-L09, D04-L10.
- visual impact: 5 if the alternative is the river. 2 if the show stays a single travelling colour.
- novelty: 4 on Titan, 1 in the upstream river.
- effort: 3, because path P's spatial ramp must not be confused with a per-bin paint in the test.
- risk: a per-bin paint looks like a graphic equaliser if the outward motion is weak. The river's sprite is what kept it from being a bar graph.
- smallest decisive experiment: freeze, on a two-tone fixture (low bin and high bin), that the output contains at least two palette positions separated by a frozen distance along the palette, in the same frame. Planted fault: `kFrozenPosition` already in `inhouse_waveform.h:29`, which forces one coordinate. That fault must fail the two-position check. The healthy path must pass.

### D04-L05 — peak / chroma AGC

- id: D04-L05
- container: AP and map
- stage: upstream `GDFT.h:169-198` at `5f329261`; Titan `inhouse_waveform.cpp:158-163` (`chroma_max_`) and v9 ceiling `inhouse_waveform.h:49` (−9 dBFS)
- what it controls: whether a louder section is literally brighter and wider, or only differently shaped.
- current setting: chroma is divided by its own recent peak. v9 then applies a level ceiling. Accent boost defaults to 0.
- information preserved or destroyed: shape kept, absolute level destroyed, then a second compressor on what remains.
- origin: inherited. Sensory Bridge did it so the fixed-point bins and the 8-bit LEDs stayed in range (`goertzel_max_value` floor 4). Emotiscope waveform divides by the block max (`waveform.h:15`). Aesthetic auto-exposure, born as headroom management.
- still needed: some exposure control, yes, or a whisper and a drop both clip or both vanish. Full peak-normalise before the artistic compressor, no.
- manipulation options: let a slow room gain ride, and let a fast peak through for one decay time. The v9 accent struct is already that shape and is off.
- interactions: D04-L01, D04-L03, brightness cap (24 versus 100, unresolved below).
- visual impact: 5 for "hits harder". This is the lever that decides whether the drop is louder in light, not just differently coloured.
- novelty: 3. The struct exists. Turning it on with a frozen rule is the work.
- effort: 2.
- risk: if the slow baseline never falls during a build, the drop ratio stays small and nothing changes. If it falls too fast, every snare "drops".
- smallest decisive experiment: freeze a ratio of mean pixel level in the 200 ms after a burst to the mean level in the 200 ms before it, on a fixture that is 12 dB louder after the burst. Healthy path must exceed a frozen ratio. Planted fault: `chroma_max_` update forced to track the burst instantly (or accent `boost` forced to 0 and peak-divide left on). The ratio must fall to about 1 and fail.

### D04-L06 — 8-bit and 16-slot palette residue

- id: D04-L06
- container: VP
- stage: `d408433` `visual/lightshow_modes.h:109-229`; Titan `inhouse_waveform.cpp:222-231` and `:895-898`; `product_palette.cpp` `sampleProductPaletteHd` / `sampleProductPaletteFastLed16`
- what it controls: how many distinct colours a gradient can show along the strip.
- current setting: v9 path P uses the HD sampler per pixel. v8 can still paint from one of 16 compiled slots. The 16-slot function remains in the v9 binary as a fault.
- information preserved or destroyed: HD keeps up to 48 authored stops and a float position (`PAL_HD_MAX_STOPS 48`). The old path keeps 16 stops, an 8-bit index, and an 8-bit colour. Three quantisers, as the commit message says.
- origin: FastLED on 8-bit LEDs. S3 and WS2812 budget. `d408433` already removed it from the live K1 effects. Titan v8 grew it back as the single-colour mix.
- still needed: no, on a 16-bit WS2816 wire, except as a named fault in the host probe.
- manipulation options: v9 path P is the option. Profile 8 does not compile it.
- interactions: D04-L09. Profile version selects the whole branch (`live_boot_composition.py:14`).
- visual impact: 4 on gradients with many stops. 1 on a two-stop palette.
- novelty: 2. Already implemented.
- effort: 1, once the flashed image is known to be profile 9. Effort becomes "confirm the image" rather than "write the sampler".
- risk: a host-only define. If the flashed image is profile 8, none of path P is on the LEDs. That is PL14's lesson and it is still the risk.
- smallest decisive experiment: freeze a count of distinct palette positions across 160 pixels on a multi-stop palette, threshold already used by the P6 palette work (more than one position). Planted fault: `kSlot16`. The distinct-stop count must drop and fail. This probe already exists as `InhouseV9Fault::kSlot16`.

### D04-L07 — one colour per frame versus colour at the output

- id: D04-L07
- container: VP
- stage: v8 `inhouse_waveform.cpp:209-238`; v9 `inhouse_waveform.h:40-47` and `outputV9` at `inhouse_waveform.cpp:885-901`
- what it controls: whether two ends of the strip can be different colours in the same frame, and whether the trail smears old RGB or only old brightness.
- current setting: profile 8, one mixed RGB, then a dot EMA on the three channels. Profile 9 path P, intensity and offset stored, HD sample at output using centre-origin distance.
- information preserved or destroyed: v8 destroys spatial colour (1 coordinate). Path P keeps a position per pixel and destroys the bin index, replacing it with distance-from-centre. Emotiscope bloom (`bloom.h:1-20`) stored the scalar and coloured at paint. Sensory Bridge bloom and the river stored RGB in the trail, so old hues smear.
- origin: v8 is the bloom single-colour inheritance. Path P is the Emotiscope/Lightwave "colour at the end" idea, written into Titan on 14 Oct as the comment says, not taken from a February 2026 commit that this pass could find.
- still needed: the v8 mix, no, if profile 9 is the show. The spatial ramp, until a per-bin paint exists, is the current colour motion.
- manipulation options: stay on path P; or replace `distance/80` with `bin/79` when a bin is the thing being drawn (that is L10, not a tweak of this one).
- interactions: D04-L04, D04-L06, which profile is flashed.
- visual impact: 5. This is the difference the Captain already saw between one colour and many.
- novelty: 2 on paper, 4 if profile 8 is what is actually running.
- effort: 1 in source, unknown on the bench until the image is identified.
- risk: path P's position is still one deposited offset plus a ramp. Loud-end stall was a previous bug when the offset came from the knee'd chroma. The code comment at `inhouse_waveform.cpp:186-188` says path P now takes the pre-knee shape. Do not move that back.
- smallest decisive experiment: the two-position freeze in L04, plus a second check that a pixel's colour changes when only its stored offset changes and the palette is held still. Planted fault: `kRgbBlend` (`inhouse_waveform.h:29`), which mixes RGB in the trail. Offset-only motion must stop producing a clean palette step and fail.

### D04-L08 — Spectrum River as a show, not as a comment

- id: D04-L08
- container: VP
- stage: upstream `6109a1e:SENSORY_BRIDGE_FIRMWARE/light_mode_spectrum_river.cpp:32-70`. Titan has no `light_mode_spectrum_river` in the in-house waveform file.
- what it controls: space. Bass at the centre, treble at the edge, the whole picture flowing outward, each bin its own brightness and its own palette position.
- current setting: not a Titan mode. Default upstream at that commit was bloom, with the river added and GDFT disabled (commit message).
- information preserved or destroyed: the river keeps 80 spatial channels of energy and 80 palette positions, then scrolls them. It destroys fine time structure inside a bin via the 0.90 trail and the squaring. It does not fold to 12 before paint.
- origin: aesthetic, Captain "10/10" in the commit message, then not selected as default. Not an S3 impossibility; the commit says the K1 build fit in 26% of RAM.
- still needed: unknown until it is seen beside path P. It is the missing "spectrum as space".
- manipulation options: port the injection rule onto the existing centre-origin mirror, using HD sampling, without the 12-hue wheel and without scrolling baked RGB if path P's scalar trail is the better smear. Drift and alpha are the motion knobs (0.55 px/frame scaled by 160/128, alpha 0.90).
- interactions: D04-L01 (low bins would sit nearest centre), D04-L04 (do not fold first), D04-L07.
- visual impact: 5.
- novelty: 4 on Titan. 1 historically.
- effort: 4. It is a new painter, and it must share the 160-split and the brightness cap.
- risk: it can look like a graphic equaliser, which is why the outward drift exists. Also a second show can starve the waveform the bench is currently judging. Ship it as an additional mode id, do not replace mode 32 in the same change.
- smallest decisive experiment: freeze that bin 0's energy lights a pixel nearer the centre than bin 60's energy, and that those two pixels' palette positions differ by about 60/79, within a frozen tolerance. Planted fault: both pixels forced to the centroid coordinate (the v8 behaviour). The position gap must fail.

### D04-L09 — Emotiscope tempo phase bank

- id: D04-L09
- container: AP
- stage: upstream `src/tempo.h:40-160` and `src/light_modes/active/metronome.h:1-40` at `d0fef9a`. Titan: no equivalent table in the files read. Novelty decimation is `kProductionNoveltyDecimation = 3` (`audio_rate_config.h:27`).
- what it controls: motion between beats, and a second colour that belongs to the tempo rather than the pitch.
- current setting: a single tempo update period of 22500 µs is documented on the waveform header (`inhouse_waveform.h:13`). One rate, not 96 phases.
- information preserved or destroyed: upstream keeps 96 phases over 20.48 s at 50 Hz novelty. Titan's novelty stream is 44.4 Hz, close to that log rate, so the input rate already exists. The bank of phases does not. A single BPM destroys every other tempo in the same bar.
- origin: Emotiscope put it on the other core because it is another 96 Goertzels (`EMOTISCOPE_FIRMWARE.ino:39-40`). On one M85 at 1 GHz this is a small fraction of a 7.5 ms frame. [INFERENCE] Cycle cost is not the reason it is absent. It was never ported. SB9 compiled a tempo tracker and left it unwired (`6109a1e` message).
- still needed: unknown. It is the strongest unused upstream idea if the complaint is timing rather than colour. It will not by itself make a drop brighter (that is L05).
- manipulation options: 96 phases driving dot position or trail speed; or a much smaller bank (on-beat, half, double) if 96 is more than the eye can use. Even/odd hype (`hype.h`) is a warning: a raw index split is not automatically a musical split.
- interactions: the half-time failure named in the swarm brief. A bank can show both 88 and 176 at once instead of locking one of them. That is a hypothesis.
- visual impact: 4.
- novelty: 5 on this product.
- effort: 4. Needs the same frozen onset fixtures the drop work already uses, or it will "pass" on a click track and fail on trance.
- risk: 96 dots is the metronome look, which is a different product from the waveform. Use the phase to move the existing waveform, do not add a dot cloud as the first demo.
- smallest decisive experiment: a click at 120 BPM and a second click at 60 BPM, not harmonically masked. Freeze that the trail's step size (pixels per second) contains both periods, measured by autocorrelation, above a frozen peak ratio. Planted fault: the 60 BPM detector forced to zero gain. Its autocorrelation peak must fail while the 120 peak stays.

### D04-L10 — image softness low-pass

- id: D04-L10
- container: VP
- stage: upstream `gpu_core.h:78-87` at `d0fef9a`, default softness 0.25 in `configuration.h:35`. Frame blend is implemented (`leds.h:534`) and the call is commented (`gpu_core.h:92`). Titan has no `softness` in the waveform file as read.
- what it controls: how glued together successive frames look. This is blur in time, per pixel, after the painter.
- current setting: absent as a user control. Titan uses a small set of named followers instead.
- information preserved or destroyed: a 0.5–15 Hz-scaled cutoff (the expression; the unit is the callee's) removes sparkle and also removes the edge of a hit. Frame blending was judged not worth running.
- origin: aesthetic, and the author's note that the S3 was finally fast enough to low-pass every pixel every frame (`gpu_core.h:82-84`). So this cut is not a compression. It is a look that became affordable and then was not brought over.
- still needed: unknown. Taste.
- manipulation options: one coefficient on the output image, default equal to today's followers so the A/B starts from the current look.
- interactions: D04-L03. Two smoothers in series will feel laggy. Change one at a time.
- visual impact: 3.
- novelty: 3.
- effort: 2.
- risk: blur plus the 0.75 bin smoother makes everything late. Also a blur can hide a bug in the painter.
- smallest decisive experiment: freeze the high-frequency energy of the pixel time series (frame-to-frame difference) under two softness values. The higher softness must cut that energy by a frozen ratio. Planted fault: the low-pass coefficient forced to 0 (bypass). The ratio must fail.

### D04-L11 — incandescent / warmth

- id: D04-L11
- container: output
- stage: upstream `constants.h:202` and `led_utilities.h:277-284` at `5f329261`, default mix 0.50; Emotiscope `apply_warmth(configuration.warmth)` at `gpu_core.h:96` and white balance `leds.h` `{1, 0.9375, 0.84}`
- what it controls: blue versus amber. It is a fixed mix toward `{1, 0.445, 0.156}` in Sensory Bridge, described as cutting harsh blues.
- current setting: not in the Titan waveform path that was read. Product palettes are the colour authority. No rainbow, and no new global tint, unless Captain asks.
- information preserved or destroyed: multiplies B down to about 16% and G to about 45% at full mix. Hue detail in the blue stops is destroyed.
- origin: aesthetic. The serial text says so. Not a budget cut.
- still needed: unknown. Taste. Do not install it as a silent default.
- manipulation options: a single mix on the palette output, default 0, so the current palettes stay put.
- interactions: D04-L06. A tint after HD sampling undoes authored blue stops.
- visual impact: 2, and only if he wants warmer palettes. The palettes can already be authored warm.
- novelty: 1.
- effort: 1.
- risk: it fights curated palettes and can look like a white-balance bug.
- smallest decisive experiment: only if he asks. Freeze blue-channel sum of a known blue stop before and after a mix of 0.5. Planted fault: mix forced to 0. The blue reduction must fail.

### D04-L12 — prism, base coat, photons-squared, sweet spot

- id: D04-L12
- container: VP / system
- stage: `led_utilities.h:1133-1143` (prism), `:642-667` (base coat), `:126` (photons squared), sweet-spot thresholds `globals.h:64-65`, all at `5f329261`
- what it controls: extra spatial copies (prism), a dim centre bar (base coat), gamma-ish brightness (photons squared), and a hardware "too loud / too quiet" meter that is not the strip.
- current setting: none of these four are in the Titan waveform file. Photons-squared is a control law. Base coat defaults off. Prism defaults to 0 iterations. Sweet spot is three GPIO LEDs on the Sensory Bridge base (`constants.h` sweet-spot pins), not a strip mode.
- information preserved or destroyed: prism adds a half-scale copy and will clip when the add exceeds the channel. It invents spatial frequencies. Base coat lifts the floor and fights true black if left on. Photons squared makes the bottom of the knob very dark, which is how they dimmed without a visible 8-bit step (README lines 28-30, together with FastLED dither).
- origin: aesthetic, all optional, all defaulting to "off" or "linear brightness 1.0" except the sweet-spot thresholds which are a calibration aid.
- still needed: no, for the music picture. Sweet spot's job on Titan is the existing level meter in logs, not three extra LEDs.
- manipulation options: leave them documented and unported.
- interactions: base coat versus true black; photons-squared versus the 24-versus-100 brightness argument.
- visual impact: 1 unless prism is turned up, in which case it is a different product.
- novelty: 1.
- effort: 2 to port, 0 to leave.
- risk: base coat is a brightness floor. Do not port it.
- smallest decisive experiment: none until someone asks. A prism port would need a frozen check that iteration 0 matches the unprismed frame exactly, and a planted fault of `iterations = 1` that must change the frame hash.

### D04-L13 — hop length held at 7.5 ms

- id: D04-L13
- container: AP / system
- stage: `audio_rate_config.h:7-18`. Upstream hop 96 at `globals.h:61` (`5f329261`) and Emotiscope hop 64 at `microphone.h:23`.
- what it controls: how often a new spectrum exists, which caps how fast the trail can react. LED wire time is a separate 5.1 ms in the swarm brief; this lever is the analysis period.
- current setting: 180 samples at 24 kHz = 7.5 ms = 133.3 Hz. Novelty every third hop, 44.4 Hz. v8 and v9 share the contract. The comment says this parity is deliberate.
- information preserved or destroyed: preserves comparability with old 12800/96 captures. Destroys the chance to update the picture every LED frame. Emotiscope already used a 5 ms audio hop on the same class of chip.
- origin: inherited parity, explicitly, not an accident (`audio_rate_config.h:7-10`). The original 96-sample chunk was an I2S DMA size on the ESP32.
- still needed: the parity mode, yes, as the frozen reference (`kLegacy12800AudioRate`). As the only live rate, unknown.
- manipulation options: a 120 Hz hop (8.3 ms would be longer; 120 Hz at 24 kHz is 200 samples) or a 200 Hz hop (120 samples) beside the legacy tuple. Do not delete the legacy tuple; tests pin it (`audio_rate_config.h:44-48`).
- interactions: D04-L03's 100 Hz assumption (`inhouse_waveform.cpp:145`). If the hop changes, that 100 must change with it or the smoother's time constant silently changes.
- visual impact: 3. Latency and tightness, not a new kind of picture.
- novelty: 2.
- effort: 3, because every smoother written in "per 100 Hz frame" has to be re-timed.
- risk: high if the 100 Hz constant is left behind. Also DMA block size comments say 180 is not a Teensy-sized block (`audio_rate_config.h:21-23`); a new hop has to compose with the 32-sample capture block.
- smallest decisive experiment: freeze end-to-end sample-to-pixel delay on a host graph for two hops, and freeze that a 100 Hz constant left unchanged fails a time-constant check when the hop changes. Planted fault: `dt` in the smoother forced to 10 ms while the hop is 5 ms. The measured smoother time constant must disagree with the hop and fail.

### D04-L14 — square-iter and the 12-hue sum that goes white

- id: D04-L14
- container: VP
- stage: upstream `lightshow_modes.h:75-77` and bloom `451-474` at `5f329261`; Titan `inhouse_waveform.cpp:206` (`value*value*1.5`) and the RGB blend at `233-236`
- what it controls: contrast, and whether dense music turns white.
- current setting: v8 squares chroma energy and then blends two RGB colours, which is the same family as summing hues. v9 path P samples a palette instead of summing RGB, so the white-sum is mostly gone there. The square remains in the shared chroma energy used before the v9 return (`inhouse_waveform.cpp:206` is on the v8 mode-32 path only; the fold above the `#if` is shared).
- information preserved or destroyed: squaring pushes small bins toward 0. Summing RGB hues pushes mixed chords toward white, which `force_saturation` then repaints as one hue (`lightshow_modes.h:473-474`). Both destroy chord identity.
- origin: aesthetic, and a fix for 8-bit pastels looking grey. `d408433` message says saturation-preserving composition was checked separately from the HD sampler.
- still needed: mild contrast, maybe. RGB summing, no. Path P already avoids the sum.
- manipulation options: leave path P. Do not reintroduce `note_colors` or an RGB sum of pitch classes.
- interactions: D04-L04, D04-L07.
- visual impact: 3 on v8, 1 on path P.
- novelty: 1.
- effort: 1.
- risk: low if the change is "do not RGB-blend on v9". High if someone "fixes" white by clamping to a single hue, which is the old `force_hue` (`led_utilities.h:1103`).
- smallest decisive experiment: two loud bins at opposite palette ends must not produce a pixel whose RGB is the average of the two stops. Freeze the distance from the pixel to each stop; the healthy pixel matches one stop, not the midpoint. Planted fault: `kRgbMixPalette` (`inhouse_waveform.cpp:892-894`), which averages two samples 0.25 apart. That fault must land near the midpoint and fail.

## Contradictions between sources

1. [FACT] The swarm brief's SensoryBridge commits `d408433` and `6109a1e` are real, and they are not in Connor Nishijima's GitHub repo and not in the empty `/Users/spectrasynq/SensoryBridge-main`. They are in `SensoryBridge-main 9`. Citing "upstream SensoryBridge" without the path mixes a 2024 product with a 2026 fork.

2. [FACT] README at `5f329261` says 100 FPS and a 256-sample frame. Config hop is 96 samples, 133 Hz. The noise-cal "3 seconds" in the README is about 1.9 s at 133 Hz.

3. [FACT] `CRGB16` is commented "Unsigned Q8.8" and typed as `SQ15x16`. The show path casts to `uint8`. Three different precisions are named for one pixel.

4. [FACT] SB9's smoother comment says attack and release were split because 0.75 flattened transients. Both constants are 0.75.

5. [FACT] `6109a1e` calls Spectrum River a 10/10 and then sets the default mode to bloom, and disables GDFT.

6. [FACT] `CHROMAGRAM_RANGE` stays 60 after `NUM_FREQS` becomes 80. The chromagram does not see the new top bins.

7. [FACT] Emotiscope `TEMPO_HIGH` is 144 by macro and the loop never reaches 144. The bins are 48..143.

8. [FACT] Emotiscope frame blend is written and the call is commented out. Softness is live. They are not the same lever.

9. [FACT] LightwaveOS working tree `constants.h` / `lightshow_modes.h` is dirty versus `b0141c88`. HEAD does contain 16000 Hz, 160 pixels, 96 bins, 2 zones. Do not cite dirty line numbers for the rest of those files.

10. [FACT] The February 2026 Lightwave-Ledstrip log that was sampled does not contain the brightness-only trail sentence. Emotiscope bloom already stored a float trail. LightwaveOS `30b084e2` (2025-08-12) already took brightness from the bin and colour from `freq_prog`. Titan's header describes the same split as path P without a February hash.

11. [FACT] Brightness: this worktree's `Agents.md` permanent block says the default minimum is 100/255 (29 Sep 2026) and an older block in the same file says preserve maximum 24/255. `build_scalar.py:1074` records a global gain string of `100/255` on one donor path. The swarm brief says to report this and not resolve it. It is unresolved. No lever above changes the cap.

12. [FACT] `chromatic_mode` defaults true (12-hue wheel) while `AUTO_COLOR_SHIFT` defaults false. The README's chroma knob and the code's boot state are easy to describe as opposites. Boot is the wheel, not the auto-shift.

13. [FACT] `interlace_flip` and `avg_bins` advertise CPU savings and neighbor averaging. One is unread, the other is `// TBD`.

## Open questions

- Which profile is in the last image that was actually programmed is not in this pass. Profile 8 omits path P entirely. Until that image is named, "v9 is current" is a source fact about a build flag, not a fact about the LEDs.
- The inner sample loop of `gdft_goertzel.cpp` was not read. The file has no `window` string. A window under another name is still possible and was not found by that search.
- `full_spectrum_frequencies` in Emotiscope may be referenced from a file other than `goertzel.h` and `cpu_core.h`. Those two do not call it.
- LightwaveOS lowest note at HEAD was not recomputed from its note table. The 16 kHz / 96-bin macros were.
- `low_pass_array`'s last argument was not traced to Hertz. Mood's effect on it is real as a parameter and unquantified as a cutoff.
- Whether even/odd tempo bins in Hype mean anything musical is open.
- Captain's taste between river and waveform is not a source fact. The commit message records a 10/10 and a different default. He would have to see both on the glass.

## Paths and commits read

Commits opened with `git show`, `git log -1`, `git grep`, or `git diff` against HEAD:

- Sensory Bridge tag 4.1.1 `5f329261987144242be4c3729f6d344631d3be1e`. Blobs: `SENSORY_BRIDGE_FIRMWARE/GDFT.h`, `constants.h`, `globals.h`, `system.h`, `lightshow_modes.h`, `led_utilities.h`, `i2s_audio.h`, `noise_cal.h`, `README.md`. Untracked `src/` copies of those were diffed; GDFT, lightshow, led_utilities, i2s and noise_cal match. `https://github.com/connornishijima/SensoryBridge` returned 422 for `d408433` and `6109a1e` on 2026-10-15.
- `SensoryBridge-main 9`: `6109a1eeb4fe6f6f14b1669995561ea5c90ac9dd`, `d408433067ddc61cf1252ce13e33d17943a17859`, HEAD `3709862c77684503f1b4788c6f42c1a182fef417`. Files: `constants.h` at 6109, `light_mode_spectrum_river.cpp` at 6109, `visual/lightshow_modes.h` and `system/constants.h` and `system/config_types.h` and `system/globals_config.cpp` at d408433. Working-tree `SENSORY_BRIDGE_FIRMWARE/system/constants.h:351-352`.
- Emotiscope `d0fef9a40aabc0b623d2e14c9dffdc9706a1ec90`, paths under `src/` as cited, confirmed identical to untracked `src1/` for the files listed above. README. Downloads `Emotiscope-1.1/src/global_defines.h`, `Emotiscope-2.0/main/microphone.h`, `Emotiscope-2.0/main/perlin.h` (grep only).
- LightwaveOS `b0141c881b49` via `git show HEAD:src/constants.h`; `30b084e2dcdae0e9ac7688823dfced7945b9b46a` via `git grep` on `src/lightshow_modes.h`. Working tree of those two files is dirty.
- Lightwave-Ledstrip `4d71bc012d18`, `git log` for Feb–Mar 2026, and `firmware-v3/src/effects/PatternRegistry.cpp` lines around 38–57. `zone-mixer/` listed, not read through.
- K1.Lightwave log subject `63f0cac` only.
- K1 firmware, presence check only: `ed56dfb575a33f304fb4365afa618961b694ca6b`, `024591dd3fbf155cd352da021965943cfbb11038`, `c2738b881d87ce53b04879332344ac272c4797d1`, `ae5d90a37480ce859d3ae058d0b186b9c456935f`. Their blobs were not re-read; an earlier forensics note in this worktree already quotes them (`docs/evidence/P6/palette-forensics-2/grok-lightwave-git.md`), and that note was read as a map, then the SB9 blobs were opened directly.
- Titan HEAD `1ef53952ab7a50af38414321c8a91e515744098a`. Files: `src/k1/core/audio/audio_rate_config.h`, `gdft_goertzel.cpp` (head and a `window` search), `sub_band.h`, `src/k1/core/visual/inhouse_waveform.h`, `inhouse_waveform.cpp` (cited ranges), `scripts/live_boot_composition.py`, `scripts/build_scalar.py` (cited lines), `platform/ra8p1/palette_runtime.cpp` (grep hits for the 79/80 split).

Looked at and not used as evidence: empty `/Users/spectrasynq/SensoryBridge-main`; empty `/Users/spectrasynq/.claude-worktrees/Lightwave-Ledstrip`; `Kx-Titan/_scratch/lightwave_native_20261012` (a run folder, not a second firmware); SensoryBridge-1 and SensoryBridge-2 heads were identified and their effect source was not read; Downloads `Sensorybridge.sourcecode` version folders were listed and not diffed against tag 4.1.1.

This report does not change the device. If a lever is chosen, the remaining path is: (1) the source above already contains path P, the sub-band filter, and the HD sampler, and does not contain the river or the tempo bank; (2) a host change with a frozen criterion and a planted fault that fails that criterion; (3) the existing image build and E2 Lite programming, by whoever already owns that bench step; (4) the stamp is the programmed image's hash plus the host check having gone red on the planted fault before it went green on the real change.

---

## Verifier addendum (Opus 5.5 orchestrating subagent, 2026-10-15) — corrections and extra levers

Lane: Grok 4.7 x-high wrote everything above (rc=0). The verifier re-read source for 19 of its [FACT] citations. 16 hold. 3 fail or need qualifying. The verifier also adds 7 levers found in its own read.

### Spot-check results

| # | Grok claim | Verdict | Evidence |
|---|---|---|---|
| 1 | SB 4.1.1 rectangular Goertzel, `>> 6` (`GDFT.h:91-94`) | PASS | `git show 5f32926:SENSORY_BRIDGE_FIRMWARE/GDFT.h:91-94` |
| 2 | SB bloom scrolls colour in the trail (`lightshow_modes.h:402`, `451-482`) | PASS, plus a missed cut | The injected colour also makes an 8-bit round trip: `CRGB temp_col = {uint8_t(sum_color.r*255),...}` then `force_saturation`, before going back to CRGB16 (`lightshow_modes.h` ~472-482 at `5f32926`) |
| 3 | SB temporal dither "armed and not on the live cast path" | **FAIL** | `show_leds()` calls `quantize_color(CONFIG.TEMPORAL_DITHERING)` (`led_utilities.h:684`). The 4-phase ordered dither (`:219-266`) is live. The `uint8_t(x*255)` cast at `:270-272` is the dither-off `else` branch |
| 4 | Emotiscope bloom: scalar float trail, colour at paint (`bloom.h`) | PASS | `novelty_image_prev[NUM_LEDS]` float; `hsv(get_color_range_hue(progress), sat, novelty_pixel)` |
| 5 | Emotiscope `(bin/64)^4` tilt (`goertzel.h:207-210`) | PASS | |
| 6 | Emotiscope dither exists for the 8-bit wire (`led_driver.h:247-253`) | PASS, refine | It is error diffusion (`quantize_color_error`), not SB's ordered table |
| 7 | Emotiscope softness default 0.25 (`configuration.h:35`) | PASS | |
| 8 | SB9 smoother 0.75/0.75 labelled "prior symmetric" (`d408433 system/constants.h:288-289`) | PASS | `MAGNITUDES_AVG_ATTACK/RELEASE 0.3f` are also symmetric (`:286-287`) |
| 9 | SB9 river `hue = k/(NUM_FREQS-1)` | PASS | `6109a1e light_mode_spectrum_river.cpp` ~58-65 |
| 10 | Titan smoother, 80→12 fold, `chroma_max_` (`inhouse_waveform.cpp:144-176`) | PASS | |
| 11 | Titan v8: two samples, RGB blend, 16-slot fallback, RGB EMA (`:209-238`) | PASS | |
| 12 | v9 accent and release off, `trigger_sub=false` (`inhouse_waveform.h:53-76`) | PASS | |
| 13 | `sub_band.h:3-7` (40–110 Hz, release-trigger only) | PASS | |
| 14 | `audio_rate_config.h:7-10` (7.5 ms parity on purpose) | PASS | |
| 15 | `live_boot_composition.py` profile 8 has no define, profile 9 adds `-DK1_INHOUSE_MAPPING_V9=1` | PASS | |
| 16 | `build_scalar.py:1074` 100/255 string; `:1175` precision 16 | PASS | `:1074` is `receipt.get('live_boot_brightness',100)`, so 100 is a fallback default, not a measured setting |
| 17 | "No 96-wide phase bank" on Titan | **FAIL / qualify** | The Titan Emotiscope native port has it: `platform/ra8p1/emotiscope_port.cpp:411` `static_assert(... NUM_TEMPI==96)`, donor `platform/ra8p1/emotiscope_donor/tempo.h`. It is absent only from the in-house waveform path (modes 32/18) |
| 18 | Contradiction 10: no February 2026 brightness-only trail in Lightwave | **FAIL** | Lightwave-Ledstrip `947bd481` (2026-02-08) "HDR-ish history buffer stores per-LED intensity values … advected outward". `firmware-v3/src/effects/ieffect/sensorybridge_reference/SbK1BloomV2Effect.h:8-9` (file added 2026-03-13, `cf765c19`/`55b11dbd`): "GRAYSCALE TRANSPORT — scroll state stores intensity only … Colour is applied AFTER mirror at output stage". The brief's "Lightwave Feb 2026" is supported |
| 19 | L02 open question: does the Titan recurrence still shift `>> 6`? | Resolved: yes | `src/k1/core/audio/gdft_goertzel.cpp:55` `(sample >> 6)`. `:47-63` emulates the donor's int32 wrap on purpose (`wrapToInt32`). `:85-96` truncates the sqrt magnitude to an integer before normalisation ("Preserve that truncation") |

Additional caveats:
- [FACT] `inhouse_waveform.h` changed between two reads in this session: the v9 gains moved from `0.395878/0.591745` to `0.580092/0.830949` (now at `:38-39`). Another lane is editing these files live. Line numbers above may drift by a few lines.
- [FACT] The brief's `c2738b88` is real. Its subject is "paint C1/C2 colour in 16-bit after device PASS". Its diff swaps `ColorFromPalette(pal, palette_index_with_phase(...), uint8_t(bin*255))` for `palette_manual_colour(pal, freq_prog, bin)` in `light_mode_gdft.cpp`, plus six other modes.
- [INFERENCE] **Correction to L03.** `spectrum_mix = 1 - 0.25^(dt*100)` is 0.75 per 10 ms. That is a time constant of 1/(100·ln 4) ≈ 7.2 ms. It is a light smoother and is unlikely to be the main transient flattener. The heavier compressors in that stage are:
  - `chroma_max_` (`inhouse_waveform.cpp:157-160`): rise 0.05 per 10 ms (τ ≈ 195 ms) and decay 0.999 per 10 ms (τ ≈ 10 s). After a quiet build, the normaliser catches up with a drop within about 200 ms.
  - The Sensory-Bridge-inherited `gdft_postprocess.cpp` stage: symmetric 0.3 magnitude EMA at `:10-11`, a mood low-pass of `1 + 10·mood` Hz at `:107-117`, and the per-band AGC with a soft knee of 0.45 at `:126-231`.
  Re-score L03's impact from 4 to 2, and raise L05 (normaliser) to the top "hits harder" lever.
- [INFERENCE] **Red-team of the brief's v9 metric.** Path P colour is `offset_[px] + distance/80` (`inhouse_waveform.cpp` `outputV9`). That is Emotiscope bloom's rule: colour by place, audio only in brightness (`bloom.h`), plus one deposited offset. "9–16 palette positions across the strip" can therefore be produced almost entirely by geometry. A frozen check of audio-driven colour must plant an **audio-blind** fault, where `offset_` is held constant, and must show that the metric goes red. If it does not, the metric measures the ramp, not the music.

### Extra levers (verifier's own read)

**D04-V15 — integer-parity Goertzel on a float/Helium core.**
- Container: AP.
- Stage: `gdft_goertzel.cpp:47-96`.
- What it controls: low-level spectral detail. That covers quiet passages, fade tails and the approach to true black.
- Current setting (v8 = v9): the input is shifted `>> 6` (6 bits gone). The recurrence wraps to int32 as an emulation. The magnitude is truncated to an integer before `× 2/block`. For a 2000-sample bass block, the normalised step is 1/1000.
- Origin: S3 fixed-point budget, kept deliberately for host/target bit-parity.
- Still needed? As a frozen parity reference, yes. As the live path, no. A float or Helium int32 four-lane path is cheap on the M85.
- Options: a float recurrence behind a define, with the legacy path kept as the oracle.
- Interactions: L01, L02, L05.
- Scores: impact 2, novelty 3, effort 2.
- Risk: it breaks byte-parity captures. Keep both paths.
- Experiment: freeze the bin-magnitude SNR on a −50 dBFS 440 Hz sine; the float path must beat legacy by a frozen dB margin. Planted fault: force the float path through `>> 6` and integer truncation; it must fail.

**D04-V16 — per-pixel phosphor decay (instant attack, slew-limited release).**
- Container: VP.
- Stage: upstream Emotiscope `leds.h:476-505` (`apply_phosphor_decay`). The call is commented out at `gpu_core.h:93`.
- What it controls: the attack edge of a hit and the length of the tail.
- Current setting: Titan v9 `fadeV9` (`inhouse_waveform.cpp` ~800) already separates attack from release on the intensity field. Partly taken.
- Origin: aesthetic. It was parked, not a budget cut.
- Options: cap the per-pixel fall rate while leaving the rise uncapped.
- Scores: impact 3, novelty 2, effort 1.
- Experiment: freeze the frames-to-peak after a burst (≤1) and the half-life of the tail. Planted fault: symmetric slew; frames-to-peak must exceed 1.

**D04-V17 — tempo bank refreshes one bin per call.**
- Container: AP.
- Stage: Titan donor `platform/ra8p1/emotiscope_donor/tempo.h:265-277`.
- What it controls: how quickly the beat estimate can follow a tempo change, and therefore beat-locked motion.
- Current setting: round-robin, one of 96 bins per call, so the full bank refreshes every 96 calls. The upstream 1.x HEAD interleave (`tempo.h:263-286` at `d0fef9a`) refreshes only 48 of 96 bins and never recomputes bins ≡ 0 or 3 mod 4. That was confirmed by simulating the exact loop. The Titan donor fixed it.
- Origin: S3 budget (one Goertzel per frame).
- Still needed? No. 96 short Goertzels on 1024 samples is about 100k MACs per frame, under 1% of an M85 frame.
- Options: refresh every bin every call.
- Interactions: the half-time lock (P7), D04-L09.
- Scores: impact 2, novelty 2, effort 1.
- Experiment: step the click tempo 120→128 BPM and freeze the latency until the tempo argmax moves. Planted fault: round-robin restored; the latency must exceed the frozen bound.

**D04-V18 — 8-bit-authored product effects lifted ×257 (REPLICATE8) onto the 16-bit wire.**
- Container: output.
- Stage: `platform/ra8p1/fixture_app.cpp:665` and `:748` (`liftPixel8`). `src/k1/core/visual/product_output_treatment.cpp` works on `Pixel8` with the SB 4-phase ordered dither (`kDitherThreshold`) and incandescent/prism/base-coat.
- What it controls: tail smoothness and gradient banding on every non-waveform product mode.
- Current setting: only the in-house waveform and the donor are TRUE16.
- Origin: inherited from the S3/SB output stage.
- Still needed? No. `hd_pixel16.h` warns "Pixel8*257 is REPLICATE8 … made WS2816 look 8-bit".
- Options: port the product effects' last stage to `Pixel16`, or drop dither on TRUE16.
- Scores: impact 3, novelty 2, effort 3.
- Experiment: freeze the count of distinct 16-bit codes across a 2 s fade tail. Planted fault: force `liftPixel8`; the count must collapse to ≤256.

**D04-V19 — vectorised multi-bin Goertzel: Titan already solved what Emotiscope 2.0 abandoned.**
- Container: AP/system.
- Stage: Emotiscope 2.0 `main/goertzel.h:165-191`, commented out with "Tried DSP/SIMD … fixed block size made it slower". Titan `gdft_goertzel.h:58-60` uses a four-lane common-newest-prefix recurrence.
- Asymmetric capability: Helium MVE on the M85 can run 4×int32 or 4×f32 lanes. That frees the budget for V15, L01 (longer bass windows) and V17 at the same time.
- Experiment: freeze cycles per frame on target (DWT counter). Planted fault: the scalar loop must exceed the bound.

**D04-V20 — Emotiscope 2.0 decimate-by-2 Goertzel without an anti-alias filter.**
- Container: AP.
- Stage: `Emotiscope-2.0/main/goertzel.h:210-212` (`sample_ptr[i*2]`, with block sized from `SAMPLE_RATE*0.5`).
- What it does: the effective rate becomes 6.4 kHz, and energy above 3.2 kHz aliases into the bins.
- Origin: S3 budget.
- On Titan: not inherited. The Titan bins run at the full 24 kHz. Record it as a trap: do not port 2.0's Goertzel.
- Scores: impact 1, effort 0.

**D04-V21 — adopt Lightwave's VP layer taxonomy as the Titan VP audit frame.**
- Container: system.
- Stage: Lightwave-Ledstrip `firmware-v3/docs/audit/VP_RENDER_PATH_LAYER_AUDIT_2026-05-05.md`. It classifies each layer as creative, corrective, protective, transport, control or legacy, gives a 15-step ordered path, a colour/brightness/temporal impact matrix, and 8 ordering rules ("author first; compose before correct; correct the actual output buffer; hardware correction last"). It also documents the wire-time fence as the cause of "pure saturated colour".
- Related: `SbK1BloomEffect.h:6-8` records that "uint8 CRGB truncates sub-1.0 propagation values to zero, killing trails after ~4 LEDs". The Titan trail is Q16, so that is already taken.
- Use: an architecture lever. It is the ready-made container map the Captain asked for, applied to the Titan VP chain (`inhouse_waveform` → `outputV9` → `palette_runtime.cpp:715-737` `scale16` + `attenuate` + `board_gain` → `packPixel`).
- Scores: impact 3 (decision quality), novelty 3, effort 2.
