# D03-s3-archaeology — K1 ESP32-S3 constraint archaeology

Research only. No device, serial, USB, flash, commit, or working-tree change. Claims are [FACT] when a line, commit, or document was read, [INFERENCE] when the reason is deduced, [HYPOTHESIS] when it is unmeasured. The Titan must keep visualising music; nothing here changes a build.

Shared memory refresh failed (wall-clock limit). This report does not use Hindsight. Memory is history, not authority.

## 1. Plain-English summary

The ESP32-S3 did not invent most of these cuts. Emotiscope already ran 64 Goertzel bins at 12,800 Hz on 128 LEDs. K1 kept that clock, widened the canvas to 160 LEDs, and therefore kept 80 bins. The S3 then spent years defending that lattice because Core 0 was already full: a 16 kHz trial lost the beat tracker, and a written decision forbids running the analyser at 48 kHz.

What the light loses is specific. The analyser starts at 110 Hz, so the sub-bass that should punch a drop is absent unless a later v9 side path measures it. Each low note is allowed at most 2,000 samples, so bass is blurrier than the sample rate suggests. The resonator still throws away the bottom 6 bits of every sample and, on Titan, still folds the multiply back into 32 bits, which on the S3 zeroed loud bins. Colour was crushed twice: a 16-colour palette average, later fixed, and a loud-music freeze of the palette position, also later fixed. Titan's in-house show still defaults to the 12,800 Hz / 96-sample clock. A 24 kHz path exists and was measured at about 1.3 ms against a 5 ms allowance, so the old clock is no longer what keeps the processor alive. v8 still paints one colour decision per frame. v9, when the build actually selects it, samples the palette per pixel.

This report does not flash anything. Using one lever means: (1) the cut is already in the files cited below, (2) change one constant in a host test that must go red when the old value is planted back, (3) the Titan lane owner builds and programs only after that test, (4) shipped means the new image's identity was read back and the frozen host numbers still match.

## 2. Sources read

| repo | HEAD read | what was used |
|---|---|---|
| `/Users/spectrasynq/SpectraSynq_K1_Firmware` | `68a95773d414` branch `fix/b489-ws2816-reference-capture`, 1039 commits, 2026-10-14 | full message grep, `git log -S` on the identifiers in the brief (notes/`k1-pickaxe.txt`, 597 lines), `config_types.h`, `constants.h`, `system.h`, `platformio.ini`, `lightshow_modes.h`, `k1_gdft_core.cpp`, ADR-0008, ADR-2026-08-21, 16 kHz stage verdict |
| `/Users/spectrasynq/SensoryBridge-k1-converge` | `2263f5499232`, 2026-06-19, 490 commits | anchors `6109a1e` (2026-06-02) and `d408433` (2026-06-11). `/Users/spectrasynq/SensoryBridge-main` is an empty directory [FACT] |
| `/Users/spectrasynq/Workspace_Management/Software/Emotiscope` | `d0fef9a40aab`, 2024-10-02, 327 commits | `src1/global_defines.h`, `src1/microphone.h` |
| Titan worktree | `1ef53952ab7a` branch `lane/titan-lagfree-20261014`, 2026-10-15. Brief's `81ac031` is an ancestor (2026-10-14), not HEAD | `audio_rate_config.h`, `gdft_goertzel.cpp/.h`, `inhouse_audio.h/.cpp`, `inhouse_waveform.h`, `pdm_target.h`, `emotiscope_port.h`, `scripts/build_scalar.py`, `scripts/live_boot_composition.py` |
| Kx-Titan lessons | file, not a firmware HEAD | `TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md` PL14-B12, PL14-B14 |
| `K1.reinvented` | `60a8a6614423`, 2025-10-29, 304 commits | `firmware/src/audio/goertzel.h` |
| `K1.esp32s3` | `1b676a9ee794`, 39 commits | log subjects only; Wi-Fi and a 10-effect restore, not the analyser lattice |
| `K1.LightwaveOS` | `71a7d8a9e16b`, 31 commits | hub/node log; not the Goertzel lattice |
| `K1.Lightwave` | `e461b093a8a1`, 23 commits | trail/gamma notes in `src/led_utilities.h:293` and `K1_V3_PARITY_HANDOFF.md` |
| `K1.MultiTempo`, `K1.tempogram` | not git repositories [FACT] | AGENTS.md says canonical firmware is `SpectraSynq_K1_Firmware`. Tempogram README describes a multi-scale tempo package. Their source was not line-audited |
| `k1-ws2816-source-precision` | `c34e0ed1cd96`, 1039 commits | same-sized history, eval HEAD, not a second lattice |
| `SpectraSynq.K1_Testbed` | `8cef2d7613db`, 7 commits | host workbench, not device DSP |

Search notes live beside this file in `notes/`. `git log -S` was run on the K1 repo for every identifier in the brief. `SMOOTHING`, `GDFT_BINS`, and `NOTE_COUNT` had no hits.

## 3. MASTER TABLE

Stage paths are current HEAD unless a commit is the introduction. "Titan inherits" means the live-tree source still contains the cut. v8/v9 differ only where noted.

| id | repo | commit | date | stage | what it does | stated reason (quote) | S3 cost saved | visual information destroyed | later fixed? | Titan inherits? | origin class |
|---|---|---|---|---|---|---|---|---|---|---|---|
| C01 | Emotiscope | present at `d0fef9a` (introduction predates the readable log start used here) | file as of 2024-10-02 | `src1/microphone.h:23-24`, `global_defines.h:16` | 12,800 Hz, 64-sample chunks, 64 Goertzel bins | reason not stated. The clock comment says "BCLK frequency for a 2kHz sample rate" (`microphone.h:45`), which does not match 12800 [FACT stale comment] | unknown in that file | Nyquist 6,400 Hz. Time step 5.0 ms. 64 bins for 128 LEDs | K1 kept 12800 and changed the chunk to 96. Not reverted | yes, donor path: `emotiscope_port.h:9-10` `K1_EMOTISCOPE_SAMPLE_RATE=12800`, `K1_EMOTISCOPE_FREQS=64`, `K1_EMOTISCOPE_CHUNK=64` | inherited. Later S3 budget is why it was *kept* (C05) |
| C02 | Emotiscope | same | same | `global_defines.h:13-16` | `NUM_LEDS 128` must be divisible by 2; `NUM_FREQS 64` | "things *may* break if you change this number" | one bin per mirrored half, not a measured cycle count | second half of the strip is a mirror, not a second spectrum | K1 scaled the same rule to 160/80 (C06) | yes for the donor show (64). In-house show uses 80 | inherited geometry |
| C03 | Emotiscope | same | same | `global_defines.h:25-31` | novelty logged at 50 Hz into 1,024 points (20.48 s); 96 tempo bins from 48 BPM | "Number of times per second novelty is logged" / "50 FPS for 20.48 seconds". No CPU sentence | half the 100 Hz render rate | tempo/novelty time grid is 20 ms | not reverted | yes: `emotiscope_port.h:11-12,66-68` novelty[1024], tempi[96], render 100 Hz | inherited. [INFERENCE] also a CPU cut because a full-rate novelty log is twice the history work |
| C04 | K1.reinvented | `60a8a661` tree | 2025-10-29 file | `firmware/src/audio/goertzel.h:35,45,51` | 16,000 Hz, 64 bins, 64 tempi, bottom note in quarter-steps | reason not stated | unknown | different lattice than product: Nyquist 8 kHz, 64 bins, 180 LEDs (`led_driver.h:66`) | product did not keep it. June 2026 16 kHz trial was blocked (C05) | no | unknown experiment. Not the shipping cut |
| C05 | K1 / SensoryBridge | kept at fork `ed56dfb5`; locked by ADR-0008 | fork 2026-06-23; ADR 2026-08-25 | `config_types.h:36-41`; ADR-0008:20-34 | shipping hop is 12,800 Hz × 96 samples = 7.5 ms (133.333 Hz) | ADR: "tempo-emitting work already sits on the hop wall (admitted music compact p99 7712 µs under the 8000 µs gate). Putting even 100–300 µs of SRC inside loopTask … would steal the wrong core." Option B, 96 samples at 48 kHz: "Dead on arrival." | the entire alternate rate. 16 kHz/120 dropped AP cadence 133.33→123.15 Hz (`docs/forensics/2026-06-15-16k120-ap-stage-profile-verdict.md:5-21`) | nothing above 6,400 Hz is real. Hop cannot shrink below 7.5 ms without a new engine | 24 kHz/180 exists on Titan as an optional same-duration hop (C26). 12.8 kHz remains the in-house default | yes default: `inhouse_audio.h:15-16,46`. Optional 24 kHz behind `K1_INHOUSE_ANALYSIS24` (`build_scalar.py:888-891`) | S3 budget for the *keep*. The number itself is inherited from Emotiscope |
| C06 | K1 | `ed56dfb5` (present at fork); comment still at HEAD | 2026-06-23 | `constants.h:163-167`, `config_types.h:117-121` | 80 bins because 160 LEDs / 2 | "NUM_FREQS = NATIVE_RESOLUTION / 2 because each freq bin maps to one canvas pixel before mirror-fill" | not stated as cycles. [INFERENCE] Goertzel cost is linear in bin count, so 80 instead of a dense FFT is also the CPU plan | 80 musical targets, not a full spectrum. Mirror throws away an independent right-hand spectrum | not reverted | yes: `gdft_goertzel.h:12` `kGdftBinCount = 80`; `audio_rate_config.h:25` | inherited geometry. Not a stated CPU commit |
| C07 | K1 | present at HEAD; comment cites v40102 | `config_types.h:309-318` | `constants.h:348-351` plus `gdft_goertzel.cpp:18` | default note offset 12, so bin 0 is 110 Hz (A2). Chroma range 60, not 80 | "the v40102 values (NOTE_OFFSET=12 / CHROMAGRAM_RANGE=60)". Ghost comment: "Default profile fs=12800 / NOTE_OFFSET=12 → hi=71 (nine ghosts: 71..79)" | 9 Goertzel bins skipped above Nyquist. The 110 Hz floor is not given a cycle number | no analysis of 55–103 Hz on the default profile. Nine top bins are labels, not sound, and are skipped after `024591dd` (2026-08-05, "retire Nyquist ghost bins as resolution") | ghosts: treated as not-resolution. The 110 Hz floor is still the default | yes: `kNoteOffset = 12` at `gdft_goertzel.cpp:18`. v9 adds a separate 40–110 Hz RMS (`inhouse_waveform.h:110`) and does not move the 80-bin bank | inherited musical default. Ghost skip is honesty, which also saves work. [INFERENCE] the floor is why drops feel late: sub-bass is outside the bank |
| C08 | K1 | cap present in `system.h` at HEAD; no introducing message found that states why 2000 | `system.h:310-311` | window longer than 2,000 samples is clipped | reason not stated at the line | [INFERENCE] each extra sample is another multiply on every low bin, on a core whose p99 is already the hop | frequency resolution cannot be finer than fs/2000. At 12.8 kHz that is 6.4 Hz. At 24 kHz the same cap is 12 Hz, so the "better" rate is coarser on capped bass bins | not reverted | yes: `kMaximumBlockSize = 2000` `gdft_goertzel.cpp:20,120` | unknown at the line. [INFERENCE] S3 budget. Also smaller than the 4,096-sample history, so it is not only "fit the buffer" |
| C09 | K1 | shipping override `platformio.ini:100-104` dated in comment 2026-08-16 | `system.h:296-308` | bins below 40 use a ×2 divisor, so a shorter window (~2 semitones). Bins at/above 40 use one semitone | "Legacy used ×2 everywhere → ~2-semitone cells by construction." Header default 0 would drop ×2 globally. Shipping: "Cross40 + exact Lane-4. Service gate is AP p99 ≤ 8000 µs" | low 40 bins do about half the window work of a 1-semitone window | bass and low-mid are resolved about twice as coarsely as the note spacing | the global drop of ×2 was not what shipped | yes: `kX2CrossoverBin = 40` `gdft_goertzel.cpp:19,115-116` | S3 budget (the 8000 µs gate is the stated reason for the shipping choice) |
| C10 | K1 | true-centre flag `K1_GDFT_TRUE_CENTER_V1` default 0, `config_types.h:72-80`; shipping explicitly excludes it `platformio.ini:90` | `system.h:337-343` | Goertzel coefficient uses rounded integer bin k, not the exact note frequency | "production is byte-identical". "TRUE_CENTER is deliberately EXCLUDED (failed eyes-on @44c3e7a)". Device note in the header: rounded k pulls A4 440 Hz to ~420 Hz | not a cycle save; the exact form is a different cosine | every bin rings at a nearby DFT line, not at the note. About 20 Hz low at A4, as stated | not shipped. Eyes-on failed, so the error was kept | yes: `gdft_goertzel.cpp:121-125` computes `rounded_bin` and never the exact-frequency branch | inherited donor match, then kept after a failed look test. Not a CPU commit. [INFERENCE] class is closer to aesthetic/accident than budget |
| C11 | K1 | bug named 2026-06-21 (SensoryBridge `7cd9336` area); promoted `a7dfe3ae` / `platformio.ini:79-99` on 2026-07-25 | `k1_gdft_core.cpp:268-286` | int32 multiply overflowed and zeroed loud bins. Shipping K1 uses int64 multiply and int64 magnitude | "can OVERFLOW at sustained resonance (undamped q ~160k) -> near-resonance bins wrap negative and clamp to 0.00, ZEROING those bins on loud audio" | int64 was accepted *despite* the note that Core 0 is already tight. Cost of the bugfix is extra, not saved | before the fix: loud partials disappear. The `sample >> 6` (`k1_gdft_core.cpp:277`) stays in both branches. Reason for the shift is not stated. [INFERENCE] it was an earlier headroom cut: 6 bits, a factor of 64, before the resonator | fixed on shipping K1 (`-DK1_GDFT_INT64_RECURRENCE_V1` and magnitude). `>> 6` not removed | partly, and the dangerous way: Titan multiplies in int64 then `wrapToInt32` *before* `>> 14` (`gdft_goertzel.cpp:52-56,71-77`). That is the overflow the K1 fix removed. `>> 6` is also still there | S3 inherited bug, fixed on S3, re-frozen on Titan as "legacy ARM target narrows modulo 2^32" (`gdft_goertzel.cpp:61-63`) |
| C12 | K1 | `platformio.ini:77-78` and `config_types.h:58-59` | shipping | tempo novelty decimation 3 | comments and the 16 kHz verdict: the ACF/update stage is what breaks a faster clock. Decimation is 3 in the production tuple `12800/96/decim=3` | novelty/tempo at 44.44 Hz instead of 133.33 Hz. Emit-hop tempo was measured 2.6 ms vs 0.86 ms mean (`9294e03d` message, 2026-10-02) | beat phase is quantised to 22.5 ms. A peak-hold can keep the hit, but the clock of the hit is coarse | prefetch of ACF rows (`9294e03d`) was bit-exact and reverted the same day (`0aaaca46`) because the device still spiked. Decimation itself remains | yes: `audio_rate_config.h:27` `kProductionNoveltyDecimation = 3`. `inhouse_waveform.h:11-13` `kInhouseTempoUpdatePeriodUs = 22500` | S3 budget |
| C13 | K1 | Phase-1 comment 2026-05-20, values rolled back the same day | `constants.h:634-644`, `lightshow_modes.h:23-37` | spectrogram smoother 0.75 attack and 0.75 release; magnitude average 0.30 / 0.30 | asymmetric smoothing "flattened transients" was tried, then "ROLLBACK … Slow release made notes persist ~3× longer … milky/washed". Reverted to symmetric | none. This spends no fewer cycles than a different alpha | each bin moves only part of the way to the new value each frame. Attacks are rounded off | the washout rollback is the current value. Not a later "restore the raw bin" | in-house audio still publishes a smoothed magnitude path (`inhouse_audio.cpp` holds `magnitude_average_`). The waveform header says the spectrum handed to v9 is "BEFORE visual smoothing" (`inhouse_waveform.h:79-80`) | aesthetic. The rollback is explicit. Not budget |
| C14 | SensoryBridge then K1 | `d408433` | 2026-06-11 | `lightshow_modes.h:113-119` (comment still in K1) | stop using a 16-entry FastLED palette, uint8 coordinate, and 8-bit CRGB sample | "The old chain lost resolution three times before the 16-bit pipeline saw a colour: CRGBPalette16 flattened every gradient to 16 entries, the coordinate was rounded to uint8 … and the sample returned as 8-bit CRGB." | the 16-slot form is a library shape, not a measured S3 cycle win. [INFERENCE] it was FastLED's cheap palette, inherited, and it also happens to be less math | 16 positions across a whole gradient. One rounded coordinate. 256 levels per channel at the sample | fixed in SensoryBridge `d408433`, present in K1 from the fork. `c2738b88` (2026-08-22) is a *later* "paint C1/C2 colour in 16-bit after device PASS", not the commit that invented per-bin sampling. Brief's label of `c2738b88` as the HD-sampling commit does not match the message [FACT] | partly. v9 path P is the per-pixel HD sample (`inhouse_waveform.h:40-46`). v8 is the old one-colour-per-frame path (`inhouse_waveform.h:19-20`). Donor Emotiscope still has its own palette | inherited FastLED, then fixed. Not an S3-cycle commit |
| C15 | K1 | `ae5d90a37480` | 2026-07-02 | `lightshow_modes.h:333-340` and the commit message | loud/dense music flattened chroma, the palette position froze, and the show collapsed to a few swept colours | "loud/dense music -> AGC+loud-guard attenuate GDFT -> chroma flatness sinks into the sparseness-gate band … -> held-hue hard freeze". Host: "DENSE: 2 coords / 0.02 bits / 99.8% frozen -> 11 coords / 3.27 bits / 0%". Quiet hold kept on purpose | none stated | before the fix: one frozen hue on dense music. After: still holds hue on true quiet (`QUIET: identical hold`) | fixed on K1 behind `K1_PALETTE_VIBRANCY_V1` | the in-house v9 path does not use this chromagram freezer. The failure mode was re-found as "sampled once per frame" and addressed by path P. v8 still has the one-sample collapse the planted fault `kFrozenPosition` names (`inhouse_waveform.h:29`) | accident (a gate meant for silence fired on loud music), then fixed. Not budget |
| C16 | K1 | ADR-2026-08-21; `quantize_color` still in `led_utilities.h` | 2026-08-21 | ADR lines 14-18 and 45-64 | WS2812 path quantises the Q16 canvas to uint8. Lever-2 Main RPL skips `quantize_color` and packs 16-bit | "128 → 32896 (REPLICATE8) … would throw away the reason for owning WS2816". Also: palette stops are still "uint8/255.0f" so occupancy is not the same as 16-bit authoring | 8-bit wire is the WS2812 protocol, not a CPU shortcut. The peephole *was* a protocol mismatch | 256 codes per channel on WS2812. On the bad WS2816 path, only the replicated 8-bit codes | Lever-2 early-return is the product Main RPL column. `c2738b88` records device PASS of 16-bit paint on seven modes | Titan pack is 16-bit (`build_scalar.py` receipt text `precision_bits: 16`). Donor `quantize_color` still exists at `platform/ra8p1/k1_dual_edge_donor.cpp:762` for the donor canvas. Palette *vertices* remain 8-bit authored even when interpolation is float | inherited LED protocol plus FastLED. The 8-bit authoring is not removed by 16-bit transport |
| C17 | K1 | `3776e777` (cherry-pick of `857706bc`) | 2026-10-02 | Emotiscope live env on the S3 | lock the picture to 120 Hz; do not render catch-up frames | "Captain decision 2 Oct 2026: fixed visual frame period; free-running stays a diagnostic build". "An overrun counts one late frame, skips the slots already started and renders no catch-up frames. Audio … unchanged." | caps CPU at one render per 8.333 ms. `constants.h:208` `VP_PERF_FRAME_BUDGET_US 8333` | any motion faster than 120 Hz is not drawn. Late frames are dropped, not delayed-and-shown | not reverted | Titan comment sets presentation at 100 Hz, not 120: `inhouse_audio.h:41-43` "Explicit 100 Hz reconstructed presentation profile, not the 133.333 Hz audio cadence." `emotiscope_port.h:10` `K1_EMOTISCOPE_RENDER_HZ=100` | aesthetic pacing, chosen by Captain, which also bounds CPU. Wire time is a separate 5.1 ms fact in the swarm brief; this commit does not cite wire time |
| C18 | K1 | `44e5d36b` | 2026-10-02 | Emotiscope live frames | skip smoothing when a direct-live frame is settled | "skip unused smoothing only for settled direct-live frames. Retain legacy preparation for startup, pending control …" | the skipped smoother. Message does not give microseconds | a settled frame keeps the previous smooth state. The commit says both 64-pixel outputs remain | not reverted in the message | not the in-house waveform. Emotiscope donor on Titan is a separate port; this exact flag was not found under `platform/ra8p1` in the greps above | S3 budget, narrowly scoped |
| C19 | K1 | `dec5fb53` | 2026-08-19 | AGC followers | retune GDFT/i2s follower alphas to a 100 Hz time constant | "Make GDFT and i2s follower alphas τ-referenced at 100 Hz (α=dt/(τ+dt))" | none. It stops the smoother from changing character when FPS changes | if the picture runs faster than 100 Hz, the follower is still the 100 Hz one, so extra frames do not make attacks faster | not reverted | in-house mood LPF uses `reference_presentation_hz` (`inhouse_audio.cpp:104-111`), default 100 | aesthetic stability. Not a bin cut |
| C20 | K1 | `config_types.h:62-69` | header at HEAD | Hann window on the Goertzel block | default off | "reduces inter-bin spectral leakage at the cost of a per-sample multiply on Core 0". "DEFAULT OFF: production is byte-identical" | one multiply per sample per bin, which they refused to spend | more leakage between notes. Adjacent bins share energy | still default off. A/B exists | no Hann multiply in `gdft_goertzel.cpp` `step()`. The window path is absent | S3 budget |
| C21 | K1 | `constants.h:156-160` | evidence 2026-06-15, flag at HEAD | static per-bin noise subtraction | forced off | "Runtime evidence on 2026-06-15 showed AP peak drive alive while VP chroma was zeroed … this static spectral floor erasing magnitudes_final[]" | none (disabling it adds the energy back) | while it was on: quiet and valid bins became silence, chroma died | fixed by disabling | Titan comment: "Static spectral-noise subtraction is disabled. Loud trim is unity" (`inhouse_audio.h:30-32`) | accident, repaired. Not budget |
| C22 | K1 | `constants.h:624-632, 646-652, 696-699` | 2026-05-20 rollback | dither, colour correction, output gamma, photons curve | dither/correction/gamma off; photons curve set to sqrt by Captain | "SUSPECT #1 for washout", "SUSPECT #2 … Crushes midtones", "Captain decision 2026-05-20" for the sqrt curve | gamma LUT exists (`gamma8_lut`) but is compiled out. Not a speed patch | gamma off keeps midtones that the LUT would crush. Dither off leaves low-brightness steps. Sqrt brightness is a look, not a bin cut | the rollback is the current product state | Titan brightness is a separate 100..255 boot rule (`build_scalar.py:933-934`), not this LUT | aesthetic |
| C23 | K1 | `667894a0` | 2026-07-08 | EdgeMixer OKLab | LUT for OKLab transcendentals | subject: "perf(edgemixer): LUT-optimise OKLab transcendentals" | transcendental calls replaced by a table. Cycle count not in the subject | a LUT is exact only at its entries. [INFERENCE] a dense table can be visually lossless; the commit was not opened far enough to count entries | not treated as reverted | not the in-house waveform path. Edge rotation is a K1 mixer feature | S3 budget. Visual loss unknown |
| C24 | K1 | whole Goertzel design, `k1_spectral_honesty.h:8-27` | header at HEAD | 80 independent Goertzels, not an FFT | "This firmware does NOT run an FFT." "more bins is NOT more evidence." | Goertzel of ~70 live bins was "heavy but survivable"; the ACF was what broke 16 kHz (verdict lines 11-12) | only the chosen note frequencies exist. Inharmonic energy between notes is invisible. No zero-padding, so bin count is not resolution | not replaced by an FFT. `k1_stm_fft512_bench` exists as a bench name in the tree and is not the product loop | yes: `analyseGdftRaw` is 80 Goertzels | inherited algorithm. The honesty header warns against pretending it is an FFT |
| C25 | Titan port | `pdm_target.h:10-29` | current HEAD | PDM capture at 40 kHz, then ASRC | "40 kHz capture + measured-rate ASRC (legacy 24 kHz/180 or Emotiscope 12.8 kHz/64)." Profile string under the Emotiscope port: `emotiscope_40k_asrc12800` | the second rate cut is the inherited lattice, not the microphone. The PDM sinc itself is how a PDM mic becomes PCM | everything between 6.4 kHz and 20 kHz is discarded on the donor path before analysis | 24 kHz analysis is optional (C26), not the Emotiscope default | yes for the donor. The mic is already faster than the analyser | inherited S3 lattice on top of a faster capture. The capture proves the cut is no longer an ADC limit |
| C26 | Titan | `audio_rate_config.h:6-17`, `inhouse_audio.cpp:220-223` | current HEAD | optional 24 kHz / 180-sample analysis, same 7.5 ms | "The 12.8 kHz rate is retained only as a frozen host-parity reference." Display: "At 24 kHz the 7.5 ms frame is shown as the same 96 points (nearest sample) … the analysis itself uses every sample." | PL14-B12 measured the 24 kHz GDFT at about 1.9×: p50 270k → 716k cycles, audio stage peak 1.29 ms against a 5 ms deadline | mode 11's drawn waveform is still 96 points, so the picture of the wave is decimated even when the spectrum is not | the 24 kHz path is implemented. Default `analysis_rate_hz` is still 12800 (`inhouse_audio.h:46`) | partly: code yes, default no. Nearest-96 display yes | the rate raise is a Titan undo of C05. The 96-point picture is a geometry shortcut. [INFERENCE] not needed for CPU after the 1.29 ms measurement |
| C27 | K1 | `1be4930a` subject only | 2026-08-18 | mode 11 trail | "mode-11 origin trail-deposit" | not fully quoted here | n/a | a trail keeps older brightness and paints new colour at the head. The swarm brief's Lightwave note matches `K1.Lightwave` `src/led_utilities.h:293`: "Gamma 2.2 correction — skip for trail-based modes (waveform) where it crushes dim trails" | v9 path P is this idea taken further: the trail stores brightness and palette position (`inhouse_waveform.h:40-41`) | aesthetic. Gamma-skip is to protect dim trails, not to save cycles |
| C28 | K1 | `810846bc` | 2026-08-17 | Palette HD V2 | extra artist stops only when a flag is on | "insert the eight approved artist colours only when K1_PALETTE_HD_V2 is on, so shippable builds stay on the existing sampler" | none on the shipping sampler | shipping palettes do not gain those eight stops | flag remains eval-gated in the message | Titan product palettes are a separate table (`product_palette`). This flag was not found as a Titan default | aesthetic / product-line split. Not budget |

Bit-exact S3 work that does **not** destroy a picture, so it is not a lever to "turn back on": Goertzel moved to IRAM (`abbf7967`, 2026-10-02, "placement only"), clamp inlined (`33781142`, "bit-exact"), ACF prefetch tried and reverted (`9294e03d`, `0aaaca46`). They show the team was shaving microseconds. They did not hide musical detail.

## 4. Aesthetic (non-budget) decisions

| id | repo | commit / place | date | what it decides | evidence it is not a budget cut | Titan |
|---|---|---|---|---|---|---|
| A01 | SensoryBridge | `6109a1e` | 2026-06-02 | Spectrum River maps 80 bins to space, hue = position along the bin. New effects pruned unfit modes. Default moved GDFT → BLOOM | message is product look and "fit-for-purpose pruning", and it reports RAM 25.6% / Flash 8.9% as a build result, not as the reason | the one-bin-one-position idea is what v9 path P restores for colour. The mode prune is K1 history |
| A02 | K1 | `ae5d90a` quiet clause | 2026-07-02 | on silence, keep the last hue. Do not snap to palette index 0 | "QUIET: identical hold both variants". The dense-music freeze was the bug; the silence hold was kept | v9 can fade a quiet deposit; do not add a brightness floor. The mandate is true black |
| A03 | K1 | `constants.h:634-644` | 2026-05-20 | symmetric 0.75 spectrogram glide | washout rollback, quoted in C13 | smoothing remains a look choice |
| A04 | K1 | `constants.h:696-699` | 2026-05-20 | photons/brightness curve = square-root | "Captain decision" | separate from the 24-vs-100 argument |
| A05 | K1 | `constants.h:624-652` | 2026-05-20 | no FastLED dither, no channel correction, no output gamma | named washout suspects | not the Titan packer |
| A06 | K1 | `platformio.ini:90` | comment 2026-07-25, still the file | do not ship exact-frequency Goertzel | "failed eyes-on" | Titan still uses rounded k (C10) |
| A07 | K1 | `3776e777` | 2026-10-02 | 120 Hz locked picture, no catch-up frames | "Captain decision" | Titan text says 100 Hz presentation |
| A08 | K1.Lightwave | `src/led_utilities.h:293` | file at `e461b093` | do not gamma trail modes | "crushes dim trails" | path P stores brightness and colour position separately so a dim tail can stay dim without going grey |
| A09 | K1 | centre-origin comments and the 160-pixel mirror | ongoing | motion leaves the middle | geometry mandate, not a cycle count | Titan centre is logical 79/80 of 160. Mirror is still the spatial model |
| A10 | Emotiscope | `global_defines.h:31-35` | 2024-10-02 file | tempo search starts at 48 BPM, 96 candidates | a musical range, not a stated RAM cap | `K1_EMOTISCOPE_TEMPI=96` |
| A11 | K1 | `config_types.h:317` | header | default chroma profile is the old v40102 window (offset 12, range 60), not "full" | three profiles exist, including bass (offset 0, range 24) and full. The default is a musical choice among them | in-house bank uses offset 12 (C07) |

## 5. Lever records

Scores are [INFERENCE] from the evidence above: visual impact is how much the picture would change if the lever moved, novelty is how unlike the current show that change is, effort is host-test plus a Titan build, not a new product. Cost on M85 uses the measured 1.29 ms peak (PL14-B12) and the 7.5 ms hop. 1.29 / 7.5 = 17% of the hop [INFERENCE]. The S3 hop was full (7,712 µs inside an 8,000 µs gate, ADR-0008). Helium does not rewrite this scalar integer loop by itself [INFERENCE].

### L01 — Donor sample rate 12,800 Hz

- id: L01
- container: AP
- stage: `platform/ra8p1/emotiscope_port.h:9-10`; capture profile `pdm_target.h:28-29`
- controls: timing, and which highs exist at all
- current setting on Titan: v8 and v9 donor audio are both 12,800 Hz, 64-sample chunks. v9 does not change the donor clock
- information preserved or destroyed: preserved below 6,400 Hz at 5 ms steps. Destroyed: 6.4–20 kHz, and any transient shorter than the chunk
- origin: inherited from Emotiscope `microphone.h:24`, then kept for S3 budget (ADR-0008)
- still needed on Titan? no for CPU. Capture is already 40 kHz. [INFERENCE] a 24 kHz or 40 kHz donor bank is the same class of work already measured at 1.29 ms
- manipulation options: feed the donor from the 24 kHz hop; or leave the donor and only upgrade the in-house bank (L02)
- interactions: L02, L04, L07, palette content above 6.4 kHz
- visual impact 1–5: 4
- novelty 1–5: 3
- effort 1–5: 3
- risk: the donor's Goertzel coefficients and tempo history are baked to 12,800. A rate change without retuning them moves every bin
- smallest decisive experiment: freeze, before running, "a 8 kHz sine at −12 dBFS must raise a high bin, and a 4 kHz sine must not raise that same bin." Plant the fault by compiling the 12,800 coefficients against 24 kHz audio; the high sine must then fail the criterion. Script pattern: `scripts/freeze_tolerance.py`

### L02 — In-house clock, default 12,800 / 96, optional 24,000 / 180

- id: L02
- container: AP
- stage: `inhouse_audio.h:15-16,44-46`; `audio_rate_config.h:6-17`; `build_scalar.py:888-891`
- controls: timing and high-note presence for modes 7, 8, 11, 18, 32
- current setting: v8 and v9 share this. Default `analysis_rate_hz = 12800`. 24 kHz is a build flag, same 7.5 ms hop, novelty still every third hop
- information preserved or destroyed: default destroys content above 6.4 kHz. The 24 kHz option keeps content up to 12 kHz and still publishes every 7.5 ms. It does not add time resolution
- origin: S3 budget lattice, copied. The file itself calls 12.8 kHz a "frozen host-parity reference"
- still needed on Titan? the 7.5 ms hop is not forced by the 1.29 ms measurement. The default 12.8 kHz is not forced. [INFERENCE] 716k cycles at 1 GHz is 0.72 ms if those counts are core cycles; the lesson's peak is 1.29 ms. Either number is inside 7.5 ms
- manipulation options: make 24 kHz the default; then try a shorter hop (90 samples = 3.75 ms) only after L06's window cap is raised, or the bass gets worse
- interactions: L06 (the 2,000 cap gets coarser in hertz at 24 kHz), L04, L05
- visual impact 1–5: 4
- novelty 1–5: 3
- effort 1–5: 2 for flipping the existing flag; 4 for a shorter hop
- risk: host oracles frozen at 12.8 kHz will disagree for a good reason. Do not call that a failure of the music
- smallest decisive experiment: freeze "bin index of an 8 kHz tone is inside the safe range and its magnitude is at least 4× the 1 kHz bin on a two-tone file." Plant 12.8 kHz coefficients; the 8 kHz tone must fall in the ghost/zero region and fail

### L03 — 80 bins and the mirror

- id: L03
- container: map
- stage: `constants.h:163-167` inherited as `kGdftBinCount = 80` (`gdft_goertzel.h:12`)
- controls: space and colour placement
- current setting: v8 and v9 both use 80 bins on a 160-pixel board, mirrored. v9 path P colours per pixel from palette position; it does not add bins
- information preserved or destroyed: preserved: one value per semitone-ish note across 80 notes. Destroyed: the other 80 pixels are not a second analysis. Energy between notes is absent (L11)
- origin: inherited geometry (Emotiscope 128/2, then K1 160/2). Not a stated cycle commit
- still needed on Titan? the mirror is a product shape, not a CPU need. More bins are cheap next to the measured 1.29 ms [INFERENCE] but only help if the windows can resolve them (L06, L07)
- manipulation options: keep 80 for the look; add a parallel dense magnitude used only for "how bright is this region"; do not silently change the mirror
- interactions: L07, L08, centre-origin
- visual impact 1–5: 3 if bins increase inside the same mirror; 5 if the mirror is removed
- novelty 1–5: 2 for more bins; 5 for an unmirrored strip
- effort 1–5: 3
- risk: removing the mirror breaks centre-origin. Captain has not asked for that
- smallest decisive experiment: freeze "pixels 0 and 159 match, and pixels 79 and 80 are the centre pair, on a one-sided impulse." Plant an independent right-half renderer; the match must fail

### L04 — Analysis starts at 110 Hz

- id: L04
- container: AP
- stage: `gdft_goertzel.cpp:18` `kNoteOffset = 12`; note table begins 55 Hz but bin 0 is `notes[12] = 110`
- controls: whether a drop's sub-bass moves the waveform before the harmonics arrive
- current setting: v8 has no sub-bass bank. v9 adds `sub_rms` for 40–110 Hz (`inhouse_waveform.h:110`) as a side measurement, not as extra Goertzel bins. The 80-bin bank is unchanged
- information preserved or destroyed: destroyed in the bank: 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104 Hz (12 notes). v9 can see their energy only as one RMS number, and only when v9 is compiled
- origin: inherited v40102 profile (`config_types.h:317`). Not given a cycle justification. [INFERENCE] it also skips 12 long windows, which is budget-shaped
- still needed on Titan? no. Twelve more low bins at the current cap are a small fraction of 1.29 ms [INFERENCE]
- manipulation options: set offset to 0; or keep offset 12 and let v9's sub-bass RMS drive the drop release. Those are different pictures
- interactions: L06 (low bins hit the 2,000 cap), L05 (tempo), the drop brief
- visual impact 1–5: 5
- novelty 1–5: 4
- effort 1–5: 2 for wiring the existing sub_rms into the release; 3 for moving the bank
- risk: offset 0 without raising the window cap makes those new bins 12 Hz wide at 24 kHz and they will smear
- smallest decisive experiment: freeze "a 60 Hz tone raises sub-band energy above 4× the no-tone case, and does not have to raise bin 0." Plant `sub_rms` forced to 0; the criterion must fail. Do not require Captain to watch the strip

### L05 — Tempo every third hop (44.44 Hz, 22.5 ms)

- id: L05
- container: AP
- stage: `audio_rate_config.h:27-38`; `inhouse_waveform.h:11-13`
- controls: timing of beat-locked motion
- current setting: v8 and v9 both use decimation 3. v9 can mix tempo weight (`inhouse_waveform.h:150`) but does not change the update period
- information preserved or destroyed: a beat is dated to a 22.5 ms bucket. Preserved: the tracker still runs. Destroyed: anything about *when* inside those 22.5 ms
- origin: S3 budget. The 16 kHz verdict names the tempo stage as the thing that misses cadence. `9294e03d` measured the emit hop's tempo work at 2.6 ms on the S3
- still needed on Titan? no for CPU. [INFERENCE] 2.6 ms × (240/1000) = 0.62 ms if the same scalar work scales with clock and IPC is similar. That fits beside 1.29 ms. Helium is not assumed
- manipulation options: decimation 1; or keep decimation and stamp the beat with the hop's sample index instead of the publish time
- interactions: L02 (shorter hop makes 22.5 ms an even larger fraction), drop alignment
- visual impact 1–5: 4
- novelty 1–5: 3
- effort 1–5: 3
- risk: the tracker's smoothing constants were tuned at 44 Hz. Running it at 133 Hz without retuning will feel nervous
- smallest decisive experiment: freeze "a click at a known sample lands in a ±5 ms window of the published beat." Plant decimation 3 and a click halfway between publishes; the error must exceed 5 ms

### L06 — 2,000-sample window cap

- id: L06
- container: AP
- stage: `system.h:310-311` copied as `gdft_goertzel.cpp:20,120`
- controls: how sharp a low note is, which is the width and steadiness of bass-driven motion
- current setting: v8 and v9, both rates. Cap is 2,000 samples. History buffer is 4,096 (`gdft_goertzel.h:13`)
- information preserved or destroyed: at 12.8 kHz the finest bass resolution is 6.4 Hz. At 24 kHz it is 12 Hz. A semitone at 55 Hz is about 3.3 Hz, so the cap cannot separate the bottom notes
- origin: reason not stated at the line. [INFERENCE] S3 budget. It is not required by the 4,096 history
- still needed on Titan? no up to 4,096. [INFERENCE] doubling the longest windows on the bins that hit the cap is well under the spare 5 ms, because only the low bins are capped and PL14 already ran the 24 kHz bank inside 1.29 ms peak
- manipulation options: 4,096; or a per-bin cap in hertz (for example 4 Hz) instead of samples, so 24 kHz does not get blurrier
- interactions: L02, L04, L07. Raising the rate without raising this cap is a trap
- visual impact 1–5: 4
- novelty 1–5: 3
- effort 1–5: 2
- risk: a longer window is slower to forget. Bass will lag unless the hop stays short. Say so in the test
- smallest decisive experiment: freeze "two tones at 55 and 62 Hz produce two different bin winners." Plant cap 2000 at 24 kHz (12 Hz cells); the winners must collapse and fail

### L07 — Low bins kept at two-semitone windows

- id: L07
- container: AP
- stage: `platformio.ini:100-104`; Titan `gdft_goertzel.cpp:19,115-116`
- controls: bass versus mid clarity
- current setting: v8 and v9. Crossover 40. Bins 0–39 use the shorter ×2 window
- information preserved or destroyed: low 40 notes are about twice as wide as their spacing. High notes are closer to one semitone, until L06 caps them
- origin: S3 budget. The ini comment ties Cross40 to the 8,000 µs service gate. Cross0 (everyone at one semitone) was the finer option and did not ship
- still needed on Titan? no. [INFERENCE] the extra samples are the same kind of work already inside the 1.29 ms
- manipulation options: crossover 0 (all one-semitone); or crossover 80 (nobody pays for the long window) as the planted fault
- interactions: L06
- visual impact 1–5: 3
- novelty 1–5: 2
- effort 1–5: 2
- risk: longer low windows slow bass attacks. Judge with an onset time number, not a vibe
- smallest decisive experiment: freeze "a tone halfway between two low notes leaks at least 6 dB less into both neighbours when crossover is 0 than when it is 40." Plant crossover 80; leakage must get worse and fail

### L08 — Rounded Goertzel frequency

- id: L08
- container: AP
- stage: `system.h:337-343`; Titan `gdft_goertzel.cpp:121-125`
- controls: whether a note lights its own bin or the neighbour
- current setting: v8 and v9. Rounded integer k. Exact-frequency code exists on K1 only behind a flag that shipping turns off
- information preserved or destroyed: K1's own header says A4 moves from 440 Hz to about 420 Hz. That is a third of a semitone, enough to bias chroma
- origin: donor match, then "failed eyes-on" (`platformio.ini:90`). Not a cycle saving
- still needed on Titan? no. The exact form is one cosine per bin at setup, not per sample
- manipulation options: use `w = 2π f / fs` for bins under Nyquist, as `K1_GDFT_TRUE_CENTER_V1` already specifies
- interactions: L04, chroma, path P palette position
- visual impact 1–5: 3
- novelty 1–5: 2
- effort 1–5: 2
- risk: the eyes-on failure on the S3 may have been the overflow (L09) contaminating the A/B, which SensoryBridge said out loud (`7cd9336` subject: overflow was the true-centre contaminant). Do not repeat that confound
- smallest decisive experiment: freeze "a 440.0 Hz tone's winning bin is the 440 Hz bin, not the 415 Hz bin, within 3 dB." Plant rounded-k; the winner must flip and fail

### L09 — 32-bit resonator wrap and the 6-bit input shift

- id: L09
- container: AP
- stage: K1 shipping uses int64 (`platformio.ini:98-99`, `k1_gdft_core.cpp:268-277`). Titan `gdft_goertzel.cpp:49-65` and `:71-77`
- controls: whether a loud note vanishes
- current setting: v8 and v9 share this resonator. Titan shifts the sample by 6 (`>> 6`) and wraps the coefficient multiply to int32 before the shift. K1 shipping does the multiply in int64
- information preserved or destroyed: `>> 6` keeps 10 bits of a 16-bit sample (a factor of 64). The wrap, on the S3, zeroed loud bins ("clamp to 0.00"). [HYPOTHESIS] Titan still does that on loud program; not measured in this pass
- origin: inherited headroom hack (`>> 6`, reason not stated) plus an S3 bug that was fixed on the S3 and then re-copied as "legacy ARM narrows modulo 2^32"
- still needed on Titan? no. int64 or float32 is cheap beside 1.29 ms [INFERENCE]. The shift is not required for headroom if the state is wider
- manipulation options: match K1's int64 recurrence and magnitude; drop `>> 6`; keep a host oracle that the clean (non-overflowing) signal stays bit-close so the change is only the loud case
- interactions: L08 (do not retest true-centre on a wrapping resonator), brightness of peaks
- visual impact 1–5: 5
- novelty 1–5: 4
- effort 1–5: 3
- risk: magnitudes will jump on loud files. Gains downstream (L16) must be re-frozen or the strip will clip
- smallest decisive experiment: freeze "a 0 dBFS sine holds its bin above 0.2 normalised for 2 seconds, with overflow_count == 0." Plant `wrapToInt32` on the product before the shift; overflow_count or a zeroed bin must fail the run

### L10 — 16-bit sample memory

- id: L10
- container: AP
- stage: `gdft_goertzel.h:15-16` `array<int16_t, 4096>`; K1 `sample_window`
- controls: quiet detail and the noise floor of the spectrum
- current setting: v8 and v9. int16 history. Emotiscope's own mic path stored float after an 18-bit scale (`microphone.h:29`, `1/131072`)
- information preserved or destroyed: codes below 1/32768 of full scale are gone before Goertzel. A 24-bit PDM decimator has already thrown bits away to reach int16
- origin: inherited. The S3 used int16 because that is the I2S/GDFT working type. Not a separate "we downcast to save RAM" commit with a number
- still needed on Titan? the 4,096 × 2 bytes is nothing on this SRAM. [INFERENCE] int32 history is 8 KB extra. No
- manipulation options: keep the PDM decimator wide one stage longer; store int32 in the analysis window only
- interactions: L09
- visual impact 1–5: 2
- novelty 1–5: 1
- effort 1–5: 3
- risk: easy to "improve" the type and forget the `>> 6`, which changes gain by 64
- smallest decisive experiment: freeze "a −80 dBFS tone is above the empty-bin floor by 6 dB in int32 history and not in int16." Plant int16; it must fail

### L11 — Goertzel note bank, not a transform of every frequency

- id: L11
- container: AP
- stage: `k1_spectral_honesty.h:8-27`; Titan `analyseGdftRaw`
- controls: colour and motion only at musical notes
- current setting: v8 and v9. 80 notes. No FFT in the product loop
- information preserved or destroyed: inharmonic sound, noise between notes, and the real width of a bass drum are not in the bank. The honesty header says extra bins without a longer window are not extra evidence
- origin: inherited algorithm. The S3 could have run a small FFT; the 16 kHz verdict says Goertzel alone was survivable and the ACF was the wall. So this is inherited design, not the measured bottleneck
- still needed on Titan? no as a CPU wall. [HYPOTHESIS] a 1,024-point real FFT at 133 Hz is a small piece of 1 GHz. This pass did not measure one, so no cycle number is claimed
- manipulation options: keep the note bank for colour identity; add one low-resolution magnitude curve (for example 32 linear bands) for brightness and hits. Do not replace the note bank on the first experiment
- interactions: L03, L06, drop impact
- visual impact 1–5: 4
- novelty 1–5: 4
- effort 1–5: 4
- risk: a full FFT plotted as 160 pixels will look like a new product. The first test should only drive the existing width/brightness
- smallest decisive experiment: freeze "a noise burst with no tonal peak raises a broadband energy feature, while a sine of the same RMS does not." Plant the feature tied to the peak bin only; the burst must fail

### L12 — Spectrogram glide 0.75

- id: L12
- container: map
- stage: K1 `lightshow_modes.h:23-37`, `constants.h:643-644`. Titan in-house split: analysis can be pre-smooth (`inhouse_waveform.h:79-80`) while `magnitude_average_` still exists
- controls: how fast a note's brightness follows the sound
- current setting: K1 symmetric 0.75. Titan v8/v9 need a per-mode reading before anyone changes them; the waveform claims the handed spectrum is pre-smooth, and the audio object still averages
- information preserved or destroyed: a step in a bin becomes a ramp. At 0.75, a large part of the old frame remains. The 2026-05-20 asymmetric try made notes last about 3× and looked washed
- origin: aesthetic, with an explicit rollback
- still needed on Titan? unknown as a look. Not needed as CPU
- manipulation options: attack 1.0 (no lag) and release 0.75; or attack 1.0 and release 0.4. Keep the washout numbers as the thing that must not return
- interactions: L09 (a zeroed loud bin will still glide if the average remembers it), L16
- visual impact 1–5: 4
- novelty 1–5: 2
- effort 1–5: 2
- risk: fast attack plus the old overflow looks like flicker. Fix L09 first
- smallest decisive experiment: freeze "a 1-frame bin impulse is still above 50% on the next frame with alpha 0.75, and below 5% with attack 1 and no release memory." Plant alpha 0.75 in the no-memory build; it must fail

### L13 — Twelve-note chroma, gate, and held hue

- id: L13
- container: map
- stage: K1 `lightshow_modes.h:320-387`; `ae5d90a`. Titan donor snapshot `emotiscope_port.h:66` `chromagram[12]`
- controls: colour
- current setting: donor Emotiscope on Titan still has 12 chroma bins and a smoothed spectrogram of 64. In-house v9 path P does not take colour from this centroid. v8's one-colour frame is the sibling failure
- information preserved or destroyed: 80 bins become 12 pitch classes (octave is gone). The sparseness gate can zero the vector. Before `ae5d90a`, dense music froze (99.8% of frames). Quiet still holds the last hue on purpose
- origin: inherited musical chroma, plus an accident on loud music that was fixed on K1
- still needed on Titan? the 12-class summary is a choice. The freeze is not. CPU cost of 12 bins is nothing
- manipulation options: drive palette position from the pre-gate spectrum shape (v9 already started this), never from a single held hue, except on true silence
- interactions: L08, L15, L16
- visual impact 1–5: 5
- novelty 1–5: 3
- effort 1–5: 2 if path P is the build; 4 if the donor modes must change too
- risk: silence hold versus true black. A hold that also holds brightness will glow in a gap. The quiet test in `ae5d90a` kept hue and must not be copied onto brightness
- smallest decisive experiment: freeze "across a dense 10 s mix, unique palette positions ≥ 8 and frozen-frame fraction ≤ 5%; across digital silence, brightness reaches 0." Plant the old hold on the dense section; frozen fraction must fail. Plant a brightness floor; the silence clause must fail

### L14 — Sixteen palette slots versus per-pixel sample

- id: L14
- container: VP
- stage: K1 `lightshow_modes.h:113-119` after `d408433`. Titan `inhouse_waveform.h:16-46`
- controls: colour along the strip
- current setting: v8 (profile 8, no `K1_INHOUSE_MAPPING_V9`) is the one-colour frame. v9's compile-time palette at current HEAD is `kPerPixelMapped` (`inhouse_waveform.h:46`). PL14-B14 says image `arm-p6-v9-host-004` was built when that default was still `kRestrictedRange`, so a green host test shipped the wrong look. Preset files named UNSELECTED in this worktree are profile 8. This pass did not read the image that is on the board
- information preserved or destroyed: 16 slots, or one sample per frame, destroy the gradient. Per-pixel sampling keeps up to the stop list (K1 caps the cache at 48 stops, `PAL_HD_MAX_STOPS`)
- origin: inherited FastLED, fixed in 2026, then missed again by a compile default (accident)
- still needed on Titan? no
- manipulation options: profile 9 plus `kPerPixelMapped` as the only product default; keep `kSlot16` and `kFrozenPosition` as planted faults (`inhouse_waveform.h:29`)
- interactions: L13, L15, L17
- visual impact 1–5: 5
- novelty 1–5: 3 (the fix exists; the miss was the default)
- effort 1–5: 2
- risk: building v9 without checking the default repeats PL14-B14
- smallest decisive experiment: freeze "a product-config build (host-variant switch off) matches the scored path-P frames, and a build whose default is planted back to `kRestrictedRange` or `kSlot16` does not." The second build is the fault that must go red

### L15 — Palette stops are still bytes

- id: L15
- container: VP
- stage: ADR-2026-08-21 lines 14-18 and 88-94; K1 HD unpack divides a byte by 255 (`lightshow_modes.h:148-149`)
- controls: colour resolution
- current setting: v8 and v9. Interpolation can be float. The stored stops are 8-bit. WS2816 can show 65,536 codes. The ADR says most of those codes are blends and fades of 8-bit paint
- information preserved or destroyed: 256 levels per channel at the authoring point. Smooth fades invent the in-between codes; a flat field cannot
- origin: inherited FastLED / CRGB. Not an S3 cycle limit. The wire can carry more
- still needed on Titan? no
- manipulation options: store stops as 16-bit in the product palette table; do not invent a rainbow to fill the codes
- interactions: L14, L16
- visual impact 1–5: 2 until a slow fade is the test; then 3
- novelty 1–5: 2
- effort 1–5: 3
- risk: a wider table that is still a copy of the bytes changes nothing. The test has to use a stop that is not a multiple of 1/255
- smallest decisive experiment: freeze "a two-stop ramp uses more than 256 distinct green codes across 160 pixels." Plant an 8-bit quantise before the ramp; the count must fall to ≤ 256 and fail

### L16 — uint8 quantise versus 16-bit pack

- id: L16
- container: output
- stage: K1 `led_utilities.h` `quantize_color`; ADR column 3 (Lever-2 skips it). Titan donor still has `quantize_color` at `k1_dual_edge_donor.cpp:762`
- controls: brightness steps, especially when dim
- current setting: Titan live pack is described as 16-bit in `build_scalar.py` (precision_bits 16) for the in-house waveform. The donor function remains in the tree
- information preserved or destroyed: quantise-to-8 leaves 256 steps. Temporal dither (`ENABLE_FASTLED_DITHER 0` on K1) was off because of washout, so the steps are not hidden
- origin: WS2812 protocol plus FastLED. Aesthetic rollback of dither. Not CPU
- still needed on Titan? no for the WS2816 pack. Yes only if a path still calls `quantize_color` before the packer
- manipulation options: confirm the live packer never calls the donor quantise; keep dither off until a separate test, because K1 already rejected it for washout
- interactions: L15, the 24-versus-100 brightness argument
- visual impact 1–5: 3
- novelty 1–5: 2
- effort 1–5: 2
- risk: calling the pack 16-bit while a quantise still runs is the peephole the ADR describes
- smallest decisive experiment: freeze "packed low bytes are not a copy of the high bytes on a slow fade" (that is the REPLICATE8 signature, `v * 0x0101`). Plant `quantize_color` in front of the packer; low bytes must match high bytes and fail

### L17 — 100 Hz picture, 133 Hz analysis

- id: L17
- container: VP
- stage: `inhouse_audio.h:41-43`; `emotiscope_port.h:10`; K1 `3776e777` is 120 Hz on the S3 Emotiscope build
- controls: timing, smoothness
- current setting: v8 and v9 presentation text is 100 Hz. Audio hop is 133.33 Hz. About one audio hop in four never becomes its own frame
- information preserved or destroyed: motion is sampled at 10 ms. The extra audio hops can still update features that the next frame reads
- origin: K1's 120 Hz lock is a Captain pacing decision that also caps CPU. Titan's 100 Hz line is a "reconstructed presentation profile." [INFERENCE] 100 Hz matches Emotiscope's render constant more than the S3's later 120 Hz decision
- still needed on Titan? a ceiling near the wire is real: the brief's 5.1 ms wire time means 120 Hz (8.3 ms) fits and 200 Hz does not. 100 Hz is not the wire limit. [FACT] wire time is from the swarm brief, not re-measured here
- manipulation options: 120 Hz to match the later Captain decision; do not skip "catch-up" in a way that drops the drop
- interactions: L05 (22.5 ms tempo vs 10 ms frames), L12
- visual impact 1–5: 3
- novelty 1–5: 2
- effort 1–5: 3
- risk: raising FPS without retuning alphas makes the same tau feel slower or faster (that is why `dec5fb53` pinned tau to 100 Hz)
- smallest decisive experiment: freeze "frame period is 8333±100 µs and a marker hop is not skipped." Plant the 100 Hz wait; the period must fail 8333

### L18 — Mode 11 draws 96 points even at 24 kHz

- id: L18
- container: VP
- stage: `inhouse_audio.cpp:220-223`
- controls: the shape of the waveform mode
- current setting: v8 and v9. Nearest-sample decimation to 96 points. Comment says analysis still uses every sample
- information preserved or destroyed: 180 samples shown as 96, so about half the time detail of that 7.5 ms is not in the drawn wave. Spectrum path is not this decimation
- origin: geometry ("mode 11 keeps its geometry"). [INFERENCE] not CPU; the samples are already in hand
- still needed on Titan? no
- manipulation options: draw all 180, or draw 160 to match the pixels, with an explicit map
- interactions: L02
- visual impact 1–5: 3 for mode 11 only
- novelty 1–5: 2
- effort 1–5: 2
- risk: changing the point count moves the centre if the mapper assumes 96
- smallest decisive experiment: freeze "a single-sample impulse at index 90 of 180 is visible in exactly one drawn column." Plant nearest-96; the impulse must land on a neighbour or vanish and fail

### L19 — Hann window left off

- id: L19
- container: AP
- stage: K1 `config_types.h:62-69`. Absent in Titan `step()`
- controls: how much a loud note paints its neighbours
- current setting: v8 and v9. No Hann on the Titan resonator read here
- information preserved or destroyed: leakage. A strong partial raises bins that are not its note. The header says the window costs a multiply per sample and was left off so production stayed byte-identical
- origin: S3 budget
- still needed on Titan? no. One multiply per sample per bin is small next to 1.29 ms [INFERENCE]
- manipulation options: on, with the coherent-gain fix the header already names (`K1_HANN_COHERENT_GAIN = 0.5`), so loudness does not drop by half and get mistaken for a new bug
- interactions: L07, L12
- visual impact 1–5: 3
- novelty 1–5: 2
- effort 1–5: 2
- risk: forgetting the 0.5 gain makes the whole show dimmer and invites a brightness "fix"
- smallest decisive experiment: freeze "a full-scale sine's neighbour bin is at least 12 dB down with the window, and within 6 dB without." Plant the window off; the 12 dB clause must fail

### L20 — Static noise floor (already off) and loud trim

- id: L20
- container: AP
- stage: K1 `constants.h:156-160`; Titan `inhouse_audio.h:30-32`
- controls: whether quiet music is erased
- current setting: v8 and v9. Subtraction off. Loud trim unity. The S3 loud-guard that helped cause `ae5d90a` was not transplanted
- information preserved or destroyed: the old floor destroyed bins and zeroed chroma. Current code preserves them. A future "helpfulness" floor would destroy them again
- origin: accident, repaired
- still needed on Titan? the repair must stay. A new floor is not needed for CPU
- manipulation options: do not reintroduce a spectral subtract. If a room is noisy, gate on a real measured floor with a host capture, per room, not a constant
- interactions: L13
- visual impact 1–5: 5 if someone turns it back on
- novelty 1–5: 1 (the win is already in)
- effort 1–5: 1 to keep the test
- risk: reintroducing it looks like "stability"
- smallest decisive experiment: freeze "broadband noise at a known low level still yields non-zero chroma." Plant `K1_GDFT_STATIC_NOISE_SUBTRACTION_ENABLED 1`; chroma must go to 0 and fail

### L21 — Brightness ceiling argument (24 versus 100)

- id: L21
- container: output
- stage: `scripts/build_scalar.py:933-934` requires boot brightness in 100..255. Older standing text in this workspace still says 24/255 for bench bounds
- controls: brightness, and therefore which dim codes are visible
- current setting: the build script rejects a boot value below 100. v8 and v9 share that check. What is on the silicon was not read in this pass
- information preserved or destroyed: a 24/255 ceiling throws away headroom the WS2816 can show. It is not an S3 CPU cut
- origin: two Captain instructions of different dates. Reported, not resolved
- still needed on Titan? a ceiling for a hot bench may still be wanted. It is not a processor limit
- manipulation options: keep the script's 100 default; do not silently restore 24; do not add a floor that prevents true black
- interactions: L16, true black
- visual impact 1–5: 4
- novelty 1–5: 1
- effort 1–5: 1 in software; the silicon value is a readback
- risk: arguing from the old bench rule and dimming a show the later mandate set to 100
- smallest decisive experiment: freeze "boot constant is 100 and a silence frame packs zeros." Plant 24; the constant check fails. Plant a floor of 1; the zero frame fails

### L22 — Donor 64 bins beside the in-house 80

- id: L22
- container: system
- stage: `emotiscope_port.h:9` versus `gdft_goertzel.h:12`
- controls: which show is even playing
- current setting: Emotiscope port is the live runtime flag (`K1_EMOTISCOPE_PORT`). In-house modes are 7, 8, 11, 18, 32 (`emotiscope_port.h:14-15`). Other modes are the donor's 64-bin show. v9 only changes in-house modes 18 and 32 (`inhouse_waveform.h:19`)
- information preserved or destroyed: switching mode can switch analyser. The donor is 64 bins at 12.8 kHz with a 100 Hz render. The in-house path is 80 bins. They are not the same picture
- origin: the port kept both. Inherited, plus a product split
- still needed on Titan? both can run. The confusion is the risk, not the CPU
- manipulation options: one analyser feeds every mode. The donor look can stay; its bins should not
- interactions: every AP lever above applies twice until this is one path
- visual impact 1–5: 5
- novelty 1–5: 3
- effort 1–5: 4
- risk: "fixing" a donor mode with an in-house lever does nothing, which is how a green test misses the show on the board
- smallest decisive experiment: freeze "mode 1 and mode 32 report the same bin count and the same sample rate in the snapshot." Plant the donor 64; the counts must differ and fail

## 6. Contradictions between sources

1. The swarm brief says `c2738b88` is the per-bin HD palette fix. The commit message is "paint C1/C2 colour in 16-bit after device PASS" (2026-08-22). The 16-entry flattening was removed earlier, in SensoryBridge `d408433` (2026-06-11), and the K1 comment at `lightshow_modes.h:113` dates that work 2026-06-11. [FACT] both texts. The brief compressed two commits into one.

2. The brief says `ae5d90a` means "chroma position never freezes." The commit keeps the quiet hold on purpose and only stops the *hard* freeze on dense music (blend when strength < 0.08). Quiet frames stay frozen. [FACT] commit message versus the brief.

3. `audio_rate_config.h:6-9` calls 24 kHz / 180 the production candidate and 12.8 kHz retired. `inhouse_audio.h:46` sets `analysis_rate_hz` to 12,800. `pdm_target.h:28-29` names the Emotiscope profile `emotiscope_40k_asrc12800`. Three defaults. [FACT]

4. PL14-B14 (15 Oct) says the v9 compile default was still `kRestrictedRange`, so a passing host test was not the image. Current `inhouse_waveform.h:46` says `kPerPixelMapped`. [FACT] the lesson and the file disagree because the file moved. Silicon was not read, so the board may still be the old default.

5. Presentation rate: K1 `3776e777` locks 120 Hz (Captain, 2 Oct 2026). Titan `inhouse_audio.h:41-43` and `emotiscope_port.h:10` say 100 Hz. The swarm brief says a 120 Hz logical target. [FACT] three numbers.

6. Brightness: `build_scalar.py:933-934` refuses a boot value under 100. Older standing bench text in this workspace still says 24/255. The brief says to report this and not resolve it. [FACT]

7. `K1_GDFT_X2_CROSSOVER_BIN` header default is 0 (`constants.h:173-174`, "global drop of ×2"). Shipping `platformio.ini:103` sets 40. Titan hardcodes 40. A reader of only the header will think the finer window shipped.

8. int64 Goertzel is on in shipping K1 (`platformio.ini:98-99`) and off in the Titan copy (`gdft_goertzel.cpp:52-56`), which documents itself as matching the legacy narrow. The S3 fix and the Titan port disagree.

9. Emotiscope `microphone.h:45` says the clock is for a 2 kHz sample rate and also sets 12,800 Hz. The 2 kHz clause is stale. [FACT]

10. `constants.h:208` budgets a frame at 8,333 µs (120 Hz) while `led_utilities.h` has a comment about a ~200 FPS loop (seen in the constants grep at line 2369: "at the ~200 FPS render loop"). The 120 Hz commit is the later rule. The 200 FPS comment is leftover.

## 7. Open questions

1. Which profile JSON was last programmed, 8 or 9, and was `kPerPixelMapped` in that image? Source cannot answer. A read of the build id on the board would. Not done here.

2. Does the Titan resonator's `wrapToInt32` actually zero loud bins on program material, or does the `>> 6` plus current gain keep `q` under 2^31? Needs a host impulse at 0 dBFS with `overflow_count` logged. [HYPOTHESIS] until that runs.

3. Why is the window cap 2,000 rather than 4,096? No commit message found. Treating it as CPU is an inference.

4. K1.MultiTempo and K1.tempogram are not git repos. Their tempo code may contain a better onset than decimation-3. This pass did not line-audit them. They are not the shipping lattice.

5. K1.reinvented's 16 kHz / 64-bin / 180-LED tree (2025) has no stated reason for 16 kHz in `goertzel.h`. It is a rejected cousin, not evidence that 16 kHz was once comfortable. The June 2026 measurement says 16 kHz was not comfortable once the tempo stage was attached.

6. Palette HD V2 (`810846bc`) adds eight artist stops only behind a flag. Whether those stops are in the Titan product palette was not compared stop-by-stop.

7. Cycle cost of a Helium FFT on this chip was not measured. Do not replace the note bank on a guessed cycle count.

8. The 5.1 ms wire figure and the "blind below 110 Hz" drop story in the swarm brief match the offset-12 bank, but the 70–120 ms sub-bass arrival time was not re-measured from audio in this pass. It stays a brief claim, consistent with L04, not a new measurement.

## 8. Verification and corrections by the dispatching Opus agent (15 Oct 2026)

Lane: Grok 4.7 x-high wrote sections 1–7. Attempt 1 stopped after a 966-byte skeleton plus `notes/`; a continuation pass (`prompt2.md`) finished it. Sections 1–7 above are Grok's text, unedited.

### 8.1 Spot-checks (14 of 14 located as cited)
| # | claim | checked against | result |
|---|---|---|---|
| 1 | Titan wraps the multiply and magnitude to int32 | `gdft_goertzel.cpp:52-56, 71-77` | PASS |
| 2 | K1 shipping uses int64 recurrence and magnitude | K1 `platformio.ini:98-99`; `k1_main_rpl_im69d` `extends = env:k1_hardware` (c3e13ffd) | PASS |
| 3 | `kNoteOffset = 12`, `kX2CrossoverBin = 40`, cap 2000 | `gdft_goertzel.cpp:18-20,120`; K1 `system.h` (cap `> 2000`) | PASS |
| 4 | d408433 "lost resolution three times" quote | `SensoryBridge-bifrost72-waveshare` d408433 message | PASS |
| 5 | ae5d90a DENSE 2→11 coords, QUIET hold kept | commit message | PASS |
| 6 | 3776e777 120 Hz, no catch-up frames | commit message | PASS |
| 7 | `emotiscope_40k_asrc12800` profile | `pdm_target.h:29` | PASS |
| 8 | ADR-0008 "7712 µs", "Dead on arrival" | `docs/architecture/ADR-0008-usb-audio-keeps-12800-ap.md:32,54` | PASS |
| 9 | build script refuses boot brightness < 100 | `scripts/build_scalar.py:933-934` ("user mandate") | PASS |
| 10 | donor `quantize_color` | `k1_dual_edge_donor.cpp:762` | PASS |
| 11 | 2026-05-20 smoothing rollback "~3× longer … milky/washed" | K1 `constants.h:635-644` | PASS |
| 12 | A4 440 → ~420 Hz rounded-k note | K1 `config_types.h:80` | PASS |
| 13 | PL14-B12 270k → 716k cycles, 1.29 ms peak | Kx-Titan `TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md:144-145` | PASS |
| 14 | 7cd9336 "int32 overflow … true-center A/B contaminant"; honesty header; mode-11 nearest-96; donor enum | `SensoryBridge-k1-converge` 7cd9336; `k1_spectral_honesty.h:8,25-26`; `inhouse_audio.cpp:220-223`; `emotiscope_port.h:9-12` | PASS |

Wording nuances (not failures): C12 says the ACF prefetch was reverted "because the device still spiked". `0aaaca46` says "bit-exact but no measured gain … tempo stage max still 2.2-2.9 ms". Line numbers: `kPerPixelMapped` is at `inhouse_waveform.h:47`, not 46.

### 8.2 Corrections
- **C01 origin.** [FACT] The 12,800 Hz rate is older than Emotiscope. SensoryBridge rates by release (`Workspace_Management/Software/SensoryBridge-1`):
  - 2.0.0 (`ae67830`, 2022-11-16): 18,750 Hz.
  - 3.0.0 (`0c9a6b0`, 2023-01-09): 12,558 Hz, 192-sample chunks, 8,192-sample history.
  - 3.2.0 (`a27d5db`, 2023-05-08): 24,400 Hz, 128-sample chunks, 4,096-sample history.
  - 4.0.0 (`a17ec09`, 2023-07-11): 12,800 Hz, 96-sample chunks; it also introduced SQ15x16 `spectrogram[]` and `CRGB16`.
  
  Emotiscope's first commit is 2024-02-05. The stated reason in 4.0.0 is only "See the Releases page for the changelog!", so the reason is not in the repo. [INFERENCE] Halving 24.4 kHz to 12.8 kHz and moving to fixed point in the same release is budget-shaped.
- **Rectangular window origin.** [FACT] SensoryBridge 4.0.0 `GDFT.h:91` has the window multiply commented out (`//sample = (... * window_lookup[...]) >> 16;`). That is where "no window" began, before K1's later decision to keep the Hann window off (C20/L19).
- **Dead budget hack.** [FACT] `interlace_flip` ("Switch field every frame on lower notes to save execution time") is toggled but never read in SB 4.0 `GDFT.h:65-66`. It is still toggled and unread in K1 `k1_gdft_core.cpp:125-126`, and it is not present on the Titan. It is harmless. It is listed only so nobody "restores" it.
- **L21 takes a side it should not.** "Keep the script's 100 default; do not silently restore 24" resolves the brightness conflict. The always-on Titan mandate (27 Sep 2026, global CLAUDE.md) says "preserve the current maximum brightness 24/255", while `build_scalar.py:933-934` enforces 100..255 as a "user mandate". Report both; Captain decides. [FACT] The ceiling is applied once, in 16 bits, at the wire (`palette_runtime.cpp:735-737`, `hd_pixel16.h:28-31` `value*brightness/255`). [INFERENCE] So 24/255 costs photon headroom, about 6,168 top codes instead of 65,535, and does not cost code resolution. It is not the 24-level crush that an 8-bit scale would cause.
- **Brief anchor c2738b88.** Grok is right that the brief compressed two commits into one. [FACT] c2738b88's `led_utilities.h` diff replaces `CHSV(uint8_t(h*255), uint8_t(s*255), 255)` with float geometric HSV, with the comment "that was the C2 chroma crush (256 hues / 256 sats)". It is an HSV 8-bit-round-trip fix, plus mode call-sites moved to `palette_manual_colour`.

### 8.3 Levers Grok missed (verified by me)
**L23. Per-slot PDM loud guard (instant peak limiter before analysis)**
- container: AP. stage: `pdm_target.c:615-636`; `k1_pdm_sensitivity.h` (`K1_PDM_SENSITIVITY_Q8 4096u` = ×16, `K1_PDM_LOUD_GUARD_CEILING 30000u`); ceiling constant first appears in Titan `fa83ac0` (2026-09-19); ×16 sensitivity restored in `fe4c94b` (2026-09-20, "Restore DualMCU ×16 PDM sensitivity after 2.4× hops stayed buried").
- controls: brightness and extent at the loud end, which is the "hits harder" problem.
- current setting: v8 and v9 share it. Each 296-sample slot (7.4 ms at 40 kHz) is scaled ×16. If peak×16 would exceed 30,000, that slot's gain is cut so its peak lands exactly on 30,000. The guard has no attack or release.
- information destroyed: [INFERENCE] every slot louder than raw peak 1,875 leaves the guard with the same peak. A drop that is 6 dB louder than the build reaches the Goertzel at the same peak level. The dynamics above the knee are gone before any feature sees them. This is a mechanical candidate for "the drop does not hit harder than the build".
- origin: inherited int16 storage (SensoryBridge `short sample_window[4096]`, Titan `GdftSampleWindow = array<int16_t,4096>`). The file comment says "Loud-guard … is what stops a snap railing int16".
- still needed on Titan? no, if the analysis window is int32/float (8 KB extra). Cost is about zero.
- options: widen the window and remove the guard; or keep int16 and log `gain_q8` per hop as a loudness side-channel (a secondary-capability reuse: the guard already measures the loudness it removes).
- interactions: L09 (`>> 6`), L10 (int16), the v9 level transfer (`kInhouseV9LevelCeilingDb`), the drop detectors.
- impact 5, novelty 4, effort 2. risk: Goertzel q-state grows 64× if `>> 6` is also removed; do L09 first.
- experiment: freeze "a +6 dB step between two 2-s pink-noise sections raises published pre-AGC level by ≥ 4 dB". Planted fault: the current guard. It must hold the step under 1 dB and go red.

**L24. Donor modes 0–6: 128-pixel, 8-bit render replicated ×257 to WS2816**
- [FACT] stages: `emotiscope_port.h:9` (`K1_EMOTISCOPE_PIXELS=128`); `emotiscope_quantize.inc:10-40` (8-bit `raw_led_data` with error-diffusion dither threshold 0.055); `palette_runtime.cpp:704-705` ("The Emotiscope profile supplies RGB8 expanded by 257").
- what is destroyed: 8 bits per channel and a 128→160 spatial resample, on a 16-bit LED. That is the REPLICATE8 signature `hd_pixel16.h:11-12` warns about.
- origin: Emotiscope hardware (128 LEDs, WS2812) inherited; not M85 budget.
- options: float `leds[]` → `quantiseHd16` directly, skipping `quantize_color`.
- impact 3, novelty 2, effort 2. experiment: same as L16 (low byte ≠ high byte on a slow fade), applied to mode 1.

**L25. Q16.16 emulation of SQ15x16 with unit clamp in the in-house renderer**
- [FACT] stage: `inhouse_waveform.cpp:16-18` `q(x)` = `clamp01(x)*65536`, used for colour, gain, fade and deposit. SQ15x16 came in with SB 4.0 (`a17ec09`).
- [FACT] K1 comments name the soft-float budget: `k1_gdft_core.cpp` "was sqrt() (soft-double on S2)" and "no new soft-float introduced".
- what is destroyed: [INFERENCE] any intermediate above 1.0 is clipped at each `q()` call. Examples: summed deposits and a boosted accent are flattened before the final per-pixel max-normalise. The M85 has a hardware FPU, so the emulation buys nothing.
- options: float internal state; normalise once at output.
- impact 2, novelty 2, effort 3. risk: the bit-exact host-parity tests are frozen to the Q16 numbers.

### 8.4 Measured M85 headroom (makes "still needed?" concrete)
[FACT] Titan `docs/evidence/P5/device-budget/BUDGET.md`: audio processing `ap_cycles` median 0.978 ms per 5 ms hop (p99 1.28 ms; donor 0.285, in-house 0.693); render 0.197 ms per frame; LED pack 0.128 ms per frame. That capture ran at **100 fps** (`render_hz` 100, 47,505 frames, none skipped). [INFERENCE] About 20 % of one core is in use for audio and about 3 % for visuals at 100 fps. Every S3 budget cut in the master table (C05, C08, C09, C12, C20) fits inside the remaining ~4 ms per hop.
