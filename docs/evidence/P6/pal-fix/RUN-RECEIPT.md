---
abstract: "Lane P6 (effect mapping, Captain ruling 1), host work only, 14 Oct 2026. Kick trigger at top: a new 40-110 Hz sub-band detector (host-tested, planted red) drives the extent release; best candidate K3 releases edge-to-centre on the BOOM file and Sonic@27.7 drops (T90 0.09-0.10 s, J 1.0) but fails the frozen +-50 ms congruence (lag 0.09-0.10 s against 0.1 s-quantised anchors; 0.05-0.06 s against the true onsets), the room recording, and Sonic@54.7; stopped, holdout unspent, no build. EXT below: the Captain's 'hits harder' (waveform extent release) frozen as the primary metric; b5595c35 and v8 red, planted instant release green (after a disclosed v2 fix of one infeasible clause); three level-surge release candidates all fail (BOOM file drop releases in 40-70 ms but the build climax also releases; BOOM room releases on the riser; Sonic drops late or not at all); stopped, holdout unspent, no build. Iteration 6 below: drop accent (low-band onset density) and colour luma hold, frozen first; no candidate passes DI on the four tuning cases (best minimum 0.904, luma hold only); the accent never fires on BOOM's drop because 110-208 Hz flux is lower there than in the build; stopped, holdout unspent, no build. Iteration 5 below: new frozen drop-impact metric DI (red on device build b5595c35 as required; v8 red; planted x3 accent green). No iteration-5 candidate (level ceiling -3 dBFS, gamma 2/3/4) passes DI on the tuning inputs: the BOOM song-file lower board stays at 0.71-0.85; no host-003 build; Carte Blanche holdout declared and left unspent. Iteration 4 below: v9 with test palette variant B (restricted colour range) is HOST-GREEN on every frozen criterion (C1 4/4, C2 9/9+18/18 both boards, C3, C4, C5b); image b5595c35 built, not deployed. Variant A (lifted low end) misses C1 by 0.0007. Iteration 3 below: still NOT host-green (C1 3/4, C2 lower 5/9, upper PASS; C3, C5b, C4 PASS); remaining blocker is the palettes' dark low end. Boot composition v9 implemented behind the existing composition profile; v8 proven bit-identical. Host replay against criteria frozen before any result: v9 is NOT host-green. Light before colour rises with loudness everywhere, but colour moving into the palettes' dark red end cancels it in 3 of 4 BOOM cases. Agent cannot listen; no device step taken."
---

# P6 — effect mapping (Captain ruling 1): host receipt

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This work was host only. No flash, no broker, no serial or USB, no E2 Lite, no `launchctl`, no commit, no push.

## The product image now uses path P. host-004 is superseded

host-004 would have drawn the restricted-range colour fold. The colour tests were scored on path P, and that image does not contain path P. It is superseded. Do not flash it.

The product build has no host palette switch. It follows one compile-time choice. That choice is now path P, the K1 per-bin colour mapping (TOLERANCE-FROZEN-P6-PAL / PAL-HD-v2). The testing-ruling comment stays. This is not a new palette.

[FACT] Both boards, every frame, every pixel. The product probe (v9, no host switch) matches the host probe with the palette forced to path P. The firmware gains were left as they are on both sides.

- BOOM file: 8,299 frames, 2,655,680 pixels, 0 mismatches
- BOOM room: 9,840 frames, 3,148,800 pixels, 0 mismatches
- Sonic room: 4,695 frames, 1,502,400 pixels, 0 mismatches
- Dopex room: 9,851 frames, 3,152,320 pixels, 0 mismatches
- Total: 32,685 frames, 10,459,200 pixels, 0 mismatches

[FACT] Plant: setting the constant back to the restricted-range fold made that test fail. On the BOOM file, 1,024,380 pixels differed. The first was frame 8, board 0, pixel 68. The constant was restored to path P. C4 then passed, 12 tests, including that pixel check and the v8 bit-identical guard.

[FACT] The restricted-range fold is still in the file. The product image takes it only when the constant is the restricted-range variant. No other product code selects it. The host switch is not compiled into the product image.

[FACT] The deposit gains in the firmware are still 0.395878 (mode 32) and 0.591745 (mode 18). The scored path-P replays also passed 0.623076 and 0.943857 through the host switch. Those overrides are not in the product image. This pixel check held the firmware gains on both sides. [INFERENCE] Light on the plate will be path P at the firmware gains, not the calibrated path-P loudness.

[FACT] host-005 is built and not programmed.

- build_id `99ea37231221ff7b4ccc4741481fcfa91ce50a742ca019354e6cc7887e16a213`
- hex sha256 `68ded08dbfbe59d11ed264a3bda1c7caeb0451ec714bbf99802118cd2b32e126`
- elf sha256 `8a62b18a6170dbe83d70181c681167da698371a7782bd0bcc3d2a82f22db6b6c`
- directory `_scratch/waveform-migration-001/arm-p6-v9-host-005`
- flags and profile match host-002
- 217 receipt source hashes match the working tree
- the only source hash that differs from host-004 is `src/k1/core/visual/inhouse_waveform.h`

Changed this session: `src/k1/core/visual/inhouse_waveform.h`, `tests/host/p6_effect_probe.cpp`, `tests/host/test_p6_product_path_p.py`, `scripts/p6_effect_mapping.py`.

host-004, build_id `49308b9a5762b110ffc14cb24ce912b37aa18a1ce478dd76f612fe4c0a4db458`, is SUPERSEDED. It does not contain path P.

The image is on disk, not on the board. A later session programmes `_scratch/waveform-migration-001/arm-p6-v9-host-005` through the existing E2 Lite check. The stamp is build_id `99ea37231221ff7b4ccc4741481fcfa91ce50a742ca019354e6cc7887e16a213` after a verified write.

## PAL-HD v2 — the dark end of palette 38 is on the curve

The upper board was drawing the palette. The test could not see the last part of it.

On the BOOM file, 752 lit pixels on the upper board sat past the old line. Every one is a real colour from palette 38, at a position from 0.9967 to 0.9999, where a bright pink falls into the dark red at the end of that palette. Before the colour is rounded onto the LEDs, the farthest of those pixels is 0.00000018 from the true palette curve. After rounding, the farthest is 0.00224, inside the 0.004 line. The old "0.012 away" number was the distance to the nearest of 4,097 stored samples of the curve. On that steep fade the samples are too far apart. The renderer was not changed.

The new measurement was frozen at 17:00:45Z on 14 Oct 2026, before any score (`TOLERANCE-FROZEN-P6-PAL-HD-v2.json`, body `894ef9f8ce2f3ab5`). The 0.004 line, the one-in-a-thousand allowance, the cutoff for a lit pixel, the frames, and the motion test are the same as v1. The curve is now the palette itself.

[FACT] Scores, both boards, four songs:

- P-fix passes. No lit pixel is off the curve. The worst miss is 0.00279. Colour still moves with the music, including the loudness ladder.
- The 16-slot plant fails the colour test. Between 5.0% and 12.9% of lit pixels are off the curve.
- The mixed-colour plant fails the colour test. Between 46.9% and 80.3% of lit pixels are off the curve.
- The held-colour plant stays on the curve and fails the motion test.

The older palette test passes. Loudness order passes. Silence and true black pass. The hue check passes, and both of its planted faults still fail. The host tests pass, 11 of 11. The known loudness-ratio miss on the BOOM file's upper board is unchanged and stays report-only.

The image is built and not programmed. This session did not touch the board.

- build_id `49308b9a5762b110ffc14cb24ce912b37aa18a1ce478dd76f612fe4c0a4db458`
- hex sha256 `16d764626cdf1da7172bdb4fd8e0353945c3b1538003392e4b91e83ce269e2a5`
- elf sha256 `8df6652590da76ec40d8fbac139386cfc43106b0c48c5d37afff9866f6d62b17`
- directory `_scratch/waveform-migration-001/arm-p6-v9-host-004`
- The receipt lists 217 source hashes. 216 match files in the working tree by path. The remaining one is the v9 live-defaults file, recorded as `external/live_defaults.json`, and it matches that file.

Figure, inspected: `figures/pal/fig-palhd2-true-curve-boom-source.png`. The left plot is the last segment of palette 38. The right plot is the 752 pixels: many are past 0.004 on the old sample table, and all are under 0.004 on the true curve.

Changed for this measurement:

- `scripts/p6_pal_hd.py` reads the exact curve when the frozen file asks for it. The v1 file still uses the 4,097 samples.
- `scripts/build_scalar.py` copies `src/k1/core/audio/sub_band.h` only for a profile-9 boot. The v9 audio header already includes it, and the first build stopped without that copy. A profile-8 build does not take the file.
- `src/k1/core/visual/inhouse_waveform.cpp` was not edited in this session.

## PAL fix — stopped, 15 Oct 2026. The tight colour line does not contain the upper palette's dark end

The upper board is a little past the tight colour line. The pixels are real colours from that palette. The line is the part that cannot see them. The 0.004 line was not moved, the renderer was not changed, and no new image was built.

On the BOOM file the upper board draws with mode 18, palette 38, mirror on and full saturation. 752 of 389,638 lit pixels sit past the line (0.0019; the limit is 0.001). The worst miss is 0.011. Every one of those pixels is in the last four thousandths of palette 38, where a bright pink fades into the dark red that palette uses at both ends (stops at 240/255 and 255/255). The same edge pixel and its mirror carry the same colour. Their internal level is 7,526 to 50,804 on the 65,536 scale, inside the levels the test already checked. They look dim because that end of the palette is dark (the palette colour's brightest channel is about 0.05 to 0.10).

The test compares each pixel with 4,097 stored samples of the palette, not with the continuous colour. On that final fade the colour ratio moves faster than those samples. For 608 of the 752 pixels the true palette colour is already past 0.004 before any rounding, and the worst true colour is 0.012 away. Rounding does not create the miss. A higher-precision output would still leave more than 1 in 1,000 lit pixels over the line.

The lower board uses the other palette, which stays bright at the end, so it remains inside the line. Holding the colour position still misses this thin slice, so the frozen-position plant stays on the curve (max error 0.0028) and still fails the motion test. The 16-slot plant and the mixed-colour plant still fail the colour test.

Record of the 752 pixels: `results/pal/palhd-off-pixels-boom-source-b1.csv`. The sample is taken at `inhouse_waveform.cpp:888-901` (`d`, `wrap(offset_ + d)`, `sampleProductPaletteHd`). The 4,097-point curve is `scripts/p6_pal_hd.py:50-64` and `tests/host/p6_effect_probe.cpp:50-71`. Figures: `figures/pal/fig-palhd-strip-boom-source.png`, `figures/pal/fig-palhd-error-scatter-boom-source.png`.

The older palette test still passes, including colour motion on the loudness ladder. Loudness order (C2), silence and true black (C3), and the hue check (C5b) are the scores already on disk from the previous host run. This session did not re-run them, because the renderer was not changed. C1 stays report-only.

Scores: `results/pal/palhd-P-fix.json`, `palhd-slot16.json`, `palhd-rgb_mix.json`, `palhd-frozen.json`.

## Latency note for P5 (read-only, while paused), 14 Oct 2026

**Correction.** The kick section below says the pipeline adds "about 40 ms". That was an estimate, and it is wrong.

I measured it on the host with a synthetic step: broadband bed, a 2 s breakdown, then a 66 Hz burst at exactly 6.000 s, using K3 settings (`results/latency/synthetic-step-k3.csv`). Every stage responded at the first 10 ms render tick after the step: `sub_rms` past half its burst level, the release envelope R at 1.0, and extent ≥ 0.9. So the host path from audio onset to release is no more than about 10 ms.

The +50–60 ms lag on the real songs comes from the signal, not the pipeline. The kick's sub energy builds over tens of milliseconds, and it must clear lo 6 dB against a 2 s baseline that already holds the riser's sub-bass.

**Where latency sits on the v9 path:**

| Stage | Location | Latency |
|---|---|---|
| PDM DMA slot | `platform/ra8p1/pdm_target.h:16` (`K1_PDM_TARGET_SLOT_ELEMENTS 296`) | 296 samples at the measured 41.4 kHz ≈ 7.1 ms per block (device only; the host probe pushes 296-sample blocks at 40 kHz, 7.4 ms) |
| Resampler | `platform/ra8p1/k1_poly24.h:19,73` (96 taps; delay 47 source samples) | ≈ 1.1 ms |
| Analysis hop | `platform/ra8p1/inhouse_audio.h:23` (180 samples at 24 kHz); capture time = hop end, `inhouse_audio.cpp:213` | Up to 7.5 ms before a sample is published |
| Sub-band detector | `src/k1/core/audio/sub_band.h:44-53`; group delay of the 4th-order band-pass | 9.1 ms at 66 Hz, 9.7 ms at 50 Hz, 4.0 ms at 100 Hz (computed). Runs per hop at `inhouse_audio.cpp:240`. |
| Trigger | fast power follower `inhouse_waveform.cpp:698` (τ 15 ms for K3); time to threshold depends on the surge size | One render tick on a large step |
| Freshness gate | `inhouse_waveform.cpp:125` (≤ 30 ms capture age accepted) | Not a delay |
| Render cadence | `platform/ra8p1/palette_runtime.h:40` (`kPalettePeriodUs 10000`) | Up to 10 ms |
| Device emission | WS2816 pair output, 160 px × 48 bit at about 1.25 µs per bit ≈ 9.6 ms per frame plus latch | [INFERENCE] arithmetic, not a run; to be measured by P5 |

---

## EXT with a sub-110 Hz kick trigger (lane owner addendum), 14 Oct 2026

### In plain English

- **What I added.** A small 40–110 Hz detector: two band-pass stages on the analysis audio, v9 only. It touches no gain stage.
  - Its own host test passes, and a planted wrong-frequency version fails as it should.
  - A new guard shows the v8 audio code compiles exactly as before.
- **What I tried.** I used the detector to trigger the waveform release, with colour luma hold as a fixed part of the base. Three settings were frozen at 14:51:32Z, before scoring.
- **Result: none passes, so I stopped.** No holdout run, no build.
- **The best setting (K3) does the right thing on two drops.** On the BOOM song file and Sonic @27.7 it releases edge to centre at the drop: full length in 90–100 ms, the whole contraction released, and BOOM's build contraction kept (the cut gate stops the climax firing).
- **It fails on timing.** The release arrives 90–100 ms after the frozen anchor; the bar is ±50 ms.
  - The anchors are quantised to 0.1 s. The true audio onsets, measured at 10 ms, are 55.73 s and 27.75 s, so the real lag is about +50–60 ms.
  - The analysis pipeline itself adds about 40 ms.
- **Other failures.**
  - BOOM room recording: the iPhone audio carries little sub-bass (+6 dB at the drop), and the trigger fires at the 47 s climax.
  - Sonic @54.7: missed by about 1 dB on the new-high gate.

### Per case (EXT v2; lower/upper board)

| Case | K1 (lo 9 dB, dip gate) | K2 (no dip gate) | **K3 (lo 6 dB, fast 15 ms, dip gate)** |
|---|---|---|---|
| BOOM file | T90 0.10, lag 0.10, J 1.0, contraction kept | same lag, contraction lost (climax fires) | **T90 0.09, lag 0.09, J 1.0, contraction kept** |
| BOOM room | no release (J 0.30/0.41) | same | T90 0.01, lag −0.06 (climax/riser), contraction lost |
| Sonic @27.7 | no release | no release | **T90 0.10, lag 0.10, J 1.0** |
| Sonic @54.7 | no release | no release | no release |
| Board×case passes | 0/8 | 0/8 | 0/8 (fails only on congruence for the BOOM file and Sonic @27.7) |

**Other checks:**
- All three candidates: C2 and C3 pass.
- DI minimum: K1 0.831, K2 0.816, K3 0.972.
- C5b passes. Host tests pass: 11 in all (v8 guard incl. new `inhouse_audio` guard, sub-band test and its planted red).
- The defaults reproduce b5595c35 frame for frame.

### Figures (inspected)

`figures/kick/` (strips: v8, b5595c35, K3):

- `fig-kick-extent-tuning.png`
- `fig-kick-strip-boom-source.png`
- `fig-kick-strip-sonic-27.7.png`

### Files (uncommitted)

**New:**
- `src/k1/core/audio/sub_band.h`
- `tests/host/test_p6_sub_band.{cpp,py}`
- `P6-EXT-KICK-CANDIDATES-FROZEN.json`
- `run-kick-candidate.sh`
- `results/kick/`
- `figures/kick/`

**Changed** (v9-only blocks):
- `platform/ra8p1/inhouse_audio.{h,cpp}`
- `src/k1/core/visual/inhouse_waveform.{h,cpp}`: `trigger_sub`, `dip_db`, dip ring
- `tests/host/p6_effect_probe.cpp`: `sub_rms` column, kick arguments
- `tests/host/test_p6_boot_composition_v9.py`: `inhouse_audio` v8 guard

### Stop

No further iteration without a new freeze.

What would decide it:

1. Re-anchor EXT to the 10 ms audio onsets in a new frozen version. K3 would then sit at about +50–60 ms, right at the bar.
2. Fix the Sonic @54.7 new-high margin.
3. Decide whether the room recording's missing sub-bass represents the device's microphone. That needs a device capture.

---

## EXT: the Captain's "hits harder" (waveform extent release), 14 Oct 2026

### In plain English

**What happened.**
- I turned the Captain's definition into a measurement, frozen at 14:37:12Z (`TOLERANCE-FROZEN-P6-EXT.json`).
  - The measurement is how far the lit waveform reaches from each edge toward the centre, frame by frame.
  - Then: did it contract through the build, and does it shoot back to full length within 150 ms of the audio drop?
- The test behaves as required on the two builds the Captain already rejected: the device build b5595c35 fails every case, and so does v8. The frame strips confirm the picture: on b5595c35 the waveform hugs the edges and the centre stays dark.
- **One validity fix, disclosed.** The planted perfect release failed one case, because Sonic's breakdown leaves the waveform already 77 % extended. I corrected that one clause (v2, frozen at 14:38:37Z, before any design ran) and kept v1 on record. Under v2 the planted case passes every check.
- I designed a release driven by a sudden rise in loudness over a 2 s baseline. It pulls new light to the centre instantly, then relaxes over 0.6 s. Three settings were frozen at 14:40:25Z.

**What is true now.**
- **No setting passes, so I stopped.** No holdout run, no `arm-p6-v9-host-003`.
- **BOOM song file: the drop itself is right.** It reaches full length in 40–70 ms, with the release 40–50 ms after the audio drop. **But the build climax also fires a release** (the build's own loudness surge at about 47 s), which destroys the contraction the Captain wants: climax extent 0.47 → 0.69.
- **BOOM room recording:** it fires on the riser at 54 s, 0.19 s before the drop.
- **Sonic:** the drops ramp in over 0.15–0.3 s, so the release comes 90–120 ms late (best setting) at 27.7 s, and not at all at 54.7 s (the 1.2 s breakdown is too short for a 2 s baseline).
- **Why level alone cannot do this.** The drop is mostly sub-100 Hz bass (+17 dB on BOOM), below the in-house analysis's lowest bin (110 Hz). Broadband loudness surges also happen at build climaxes and risers. A real-time drop detector needs a feature that separates "drop" from "climax" and "riser"; none of the current analysis outputs does.
- **The device build is unchanged:** the source defaults reproduce b5595c35 frame for frame on 10/10 inputs.

### Results (`results/ext/`; bars PROVISIONAL pending the Captain's eye)

**Validity** (EXT v2, tuning cases BOOM file, BOOM room, Sonic @27.7 and @54.7, both boards):
- **b5595c35:** FAIL on all 8. T90 never reached, J 0.10–0.57.
- **v8:** FAIL on all 8.
- **Planted instant release:** PASS on all 8.
- **v1** failed its own planted case, sonic@54.7 board 0 (`ext-b5595c35-planted.json`).

**Extent during the build** (b5595c35, early → climax, the contraction the Captain sees):

| Case | Lower board | Upper board |
|---|---|---|
| BOOM file | 0.49 → 0.47 | 0.38 → 0.29 |
| BOOM room | 0.46 → 0.68 (no contraction on the room recording) | 0.29 → 0.55 |
| Sonic @27.7 | 0.42 → 0.47 | 0.24 → 0.26 |
| Sonic @54.7 | 0.47 → 0.45 | 0.28 → 0.24 |

**Candidates.** Gains are matched to v8's average light. Values are lower/upper board.

| | X1 (surge 3–9 dB) | X2 (+ new-peak gate) | X3 (surge 2–6 dB + gate) |
|---|---|---|---|
| BOOM file | T90 0.07, lag 0.04/0.07, J 1.00; climax 0.69/0.57 against 0.47/0.29 (contraction lost) | same; lag 0.04/0.05 | T90 0.04, lag 0.04, J 1.00; contraction lost |
| BOOM room | lag −0.19/−0.06 (fires on the riser) | same | lag −0.19; contraction lost |
| Sonic @27.7 | T90 inf, J 0.36/0.41 | same | T90 0.12/0.15, lag 0.09/0.12 (late) |
| Sonic @54.7 | no release, J 0.57/0.56 | same | no release |
| Result | FAIL | FAIL | FAIL |

**Other checks, all three candidates:**
- C2 passes: 9/9 and 18/18 on both boards.
- C3 passes.
- C5 passes.
- DI, secondary, minimum: 0.767, 0.732, 0.688.

**On the final source:**
- C5b passes with palette range: 0 off-palette pixels, both planted faults red.
- The v8 guard passes 8/8.
- Default frames equal b5595c35 on 10/10 inputs.

### Figures (inspected)

- `figures/ext/fig-ext-extent-tuning.png`: extent against time for v8, b5595c35 and X3, across early build, climax and drop, with the audio onset dashed and the 150 ms window shaded.
- `figures/ext/fig-ext-strip-boom-source.png`: space × time frame strips, BOOM drop ±1 s.
  - v8 and b5595c35 stay pinned near the edges through the drop.
  - X3 puts new light at the centre at the drop and the lit band spans edge to centre (the chevron).
  - [Captain's eye needed] X3's release lights the centre first and streams outward. The Captain described thrust from the edges toward the centre. Whether centre-born light reads as that release is a visual judgement.

### Files (uncommitted)

**Frozen:**
- `TOLERANCE-FROZEN-P6-EXT.json` (v1, on record)
- `TOLERANCE-FROZEN-P6-EXT-v2.json`
- `P6-EXT-DESIGN-CANDIDATES-FROZEN.json`

**Source:**
- `src/k1/core/visual/inhouse_waveform.{h,cpp}`: `InhouseV9Release`, `kInhouseV9ReleaseConfig` (off), `updateReleaseV9`, position pull on both modes, host `setV9HostRelease`, observation `release`.
- `tests/host/p6_effect_probe.cpp`: `extent_edge` and `release` columns; release arguments.

**Scripts:**
- `scripts/p6_ext.py` (new)
- `scripts/p6_ext_figures.py` (new)

**Run folder:**
- `run-ext-candidate.sh`
- `results/ext/`
- `figures/ext/`
- `replay-records/ext-*`

### Stop

Stopped as the frozen rule requires. No further iteration without a new freeze from the lane owner. Carte Blanche remains unspent.

**Evident next step (not done).** A drop detector that tells a drop from a climax or riser, for example: a surge that follows a sustained dip (the cut or breakdown), plus a whole-band or sub-bass presence feature. The current analysis lacks sub-110 Hz content, so that may need a new analysis output, which is an AGC-adjacent scope decision.

---

## Iteration 6: drop accent and colour luma hold (lane owner instruction after 315cf91, 14 Oct 2026)

### In plain English

**What happened.** I froze three candidates at 14:30:12Z (`P6-IT6-DESIGN-CANDIDATES-FROZEN.json`), before computing any of their drop-impact (DI) numbers:

- **(a) Drop accent.** Deposits get brighter and wider when kick/bass onsets in the 110–208 Hz note bins rise against a 4-second baseline.
- **(b) Colour luma hold.** Without strong harmonic confidence, the colour may not move to a darker palette position.
- **(c) Both together.**

All three keep the device build's design. Average light still matches v8.

I added Sonic Samba's song file as tuning input, because it has two build→drop moments (27.7 s and 54.7 s). That gives four tuning cases with BOOM's file and room recording.

**What is true now.**

- **No candidate passes DI.** I stopped as the rule requires: no holdout run, no `arm-p6-v9-host-003` build.
- **Luma hold (b) helps most.** The best case is b, with every case ≥ 0.90, against b5595c35's 0.71. The BOOM file's lower board rises 0.76 → 0.97, and Sonic@54.7's lower board reaches 1.31. Every case is still short of 1.25.
- **The accent (a) cannot see BOOM's drop.** In the 110–208 Hz bins, positive flux is *lower* in BOOM's drop than in its build: 0.014 against 0.038 on the song file, and 0.003 against 0.003 in the room recording. So the accent stays at 0 there. [INFERENCE] The drop's kick energy sits below the 110 Hz lowest analysis bin, or the spectral AGC flattens it.
- **On Sonic the accent does fire.** Drop p95 is 0.46–0.61. But recalibrating to matched average light lowers the gains, and the upper board stays at 0.68–0.90.
- **The device build is unchanged.** The source's v9 defaults still reproduce b5595c35 on 10/10 inputs. The new features are host knobs, off by default.

### Results (DI, bar 1.25 PROVISIONAL; `results/iteration6/`)

| DI lower / upper | BOOM file | BOOM room | Sonic @27.7 | Sonic @54.7 | Minimum | Qualifies |
|---|---|---|---|---|---|---|
| v8 | 1.046 / 1.407 | 0.933 / 0.765 | 1.077 / 1.285 | 1.545 / 3.991 | 0.765 | no |
| b5595c35 (device) | 0.758 / 1.087 | 1.114 / 1.181 | 0.996 / 0.753 | 0.942 / 0.706 | 0.706 | no |
| a: accent (gains 0.358 / 0.539) | 0.738 / 1.027 | 1.115 / 1.182 | 1.154 / 0.790 | 0.909 / 0.677 | 0.677 | no |
| **b: luma hold** (0.376 / 0.571) | **0.973** / 1.062 | 1.127 / 1.200 | 1.056 / 0.904 | **1.312** / 1.044 | **0.904** | no |
| c: accent + hold (0.340 / 0.520) | 0.935 / 1.005 | 1.128 / 1.199 | 1.193 / 0.897 | 1.179 / 0.997 | 0.897 | no |

Other checks, all three candidates:

- C2 passes: 9/9 and 18/18 on both boards.
- C3 passes: silence ratio and true black.
- C5 passes.
- C1 is reported only: it fails for a and c (not valid as a "hit harder" proxy).

On the final source:

- C5b passes with palette range.
- The v8 translation-unit guard passes 8/8.
- Default frames equal b5595c35 on 10/10 inputs.

**An observation for the next freeze.** On Sonic@54.7, v8's Tempo board scores DI 3.99. It flares just before and inside the drop (figure, bottom-right panel). That is the kind of release the Captain may be missing.

### Why it fails, from the data

- **BOOM:** the accent's feature does not respond. The luma hold recovers most of the colour loss on the lower board, but the build climax remains about as bright as the drop.
- **Sonic, upper board:** the Tempo board's light falls in both drop windows under v9. Its motion keeps deposits small relative to the build.
- **Neither is a gain problem.** Matched average light is applied before DI is scored.

### Figure (inspected)

`figures/iteration6/iteration6-fig-di-lstar-tuning.png` plots L\* against time, with DI per run, for v8, b5595c35, b and c across the four tuning cases.

### Files (iteration 6, uncommitted)

- `src/k1/core/visual/inhouse_waveform.{h,cpp}`:
  - `InhouseV9Accent`, `kInhouseV9AccentConfig` (off) and `kInhouseV9LumaHold` (false);
  - `updateAccentV9`;
  - luma hold in `updateColourV9`;
  - host `setV9HostAccent`;
  - the observation fields `low_flux`, `accent` and `luma_ref`.
- `tests/host/p6_effect_probe.cpp`: accent and luma-hold arguments; low_flux, accent and luma_ref columns.
- `scripts/p6_di.py`: several drops per input (`tag`).
- `scripts/p6_effect_mapping.py`: `add-song`; `calibrate --level` takes more arguments.
- `scripts/p6_figures.py`: multi-drop DI figure and a titled filename.
- Run folder:
  - `P6-IT6-DESIGN-CANDIDATES-FROZEN.json`
  - `run-it6-candidate.sh`
  - `results/iteration6/`
  - `figures/iteration6/`
  - `replay-records/it6-*`

### Stop

No further iteration without a new freeze from the lane owner, as instructed. Carte Blanche remains an unspent holdout.

---

## Iteration 5: drop impact (lane owner instruction after REVIEW-ITERATION-1, 14 Oct 2026)

### In plain English

**What happened.**
- **New test, frozen first.** I froze a new "does the drop hit harder" test (DI) at 14:18:24Z, before any design change and before scoring any build. DI compares the brightest perceived lightness (CIE L\*) in the first second of the drop with the brightest in the build's climax.
- **The test works as intended.**
  - It fails on the build now on the device (b5595c35), matching the Captain's "still a no".
  - It fails on v8.
  - It passes when I plant a ×3-brighter drop.
- **Holdout chosen in advance.** Carte Blanche was never used in iterations 1–4. Dopex was rejected: its 0–75 s has no build→cut→drop.
- **Three design candidates, frozen before running (14:20:42Z).** All three raise the level ceiling from −9 to −3 dBFS. The emphasis exponent is 2, 3 or 4. Average light still matches v8.

**What is true now.**
- **No candidate passes DI on the tuning songs, so I stopped, as the frozen rule requires.** No `arm-p6-v9-host-003` was built, and the holdout was not spent.
  - The room recording does improve: lower board up to 1.61, upper board 1.30 at γ = 4.
  - **The BOOM song file's lower board stays at 0.71–0.85.**
- **Why** (γ = 3, song file):
  - **Lower board:** the light before colour rises ×1.37 into the drop. But the colour layer moves from palette position 0.54 to 0.33 between the build climax and the drop, so lightness per unit of light falls (luma/S 1.11 → 0.68). The perceived lightness falls (L\* p95 20.7 → 16.2).
  - **Upper board:** it is already near its ceiling in the build (max channel p95 0.93 → 1.00), so there is no headroom left.
  - **Input:** at the 7.5 ms analysis level, the song file's build climax is almost as loud as the drop (level p95 0.79 against 0.88). There is little loudness contrast for any level mapping to use.
- **The device build is unchanged.** The source's v9 defaults still reproduce b5595c35 frame for frame on all 10 inputs. The candidate settings are host-only knobs.

### DI results (`TOLERANCE-FROZEN-P6-DI.json`, sha `3c34a4de…`; bar 1.25 PROVISIONAL pending a Captain ladder calibration)

**Anchors** (from the stimulus audio only):
- BOOM: drop 55.7 s, build climax 47.0–48.5 s, cut 49.7–54.0 s.
- Carte Blanche: drop 27.4 s, climax 24.4–25.9 s.

| DI (lower / upper) | BOOM song file | BOOM room recording | Carte Blanche (holdout) | Verdict |
|---|---|---|---|---|
| v8 | 1.046 / 1.407 | 0.933 / 0.765 | not run | red (as required) |
| **b5595c35** (device, v9 B) | **0.758 / 1.087** | **1.114 / 1.181** | not run | **red (known negative, as required)** |
| b5595c35 + planted ×3 drop accent | 1.343 / 1.804 | 1.991 / 1.994 | — | green (as required) |
| it5 γ 2.0 (gains 0.753 / 1.135) | 0.711 / 1.135 | 1.146 / 1.210 | not run | fail |
| it5 γ 3.0 (gains 1.191 / 1.876) | 0.782 / 1.119 | 1.358 / 1.250 | not run | fail |
| it5 γ 4.0 (gains 1.786 / 3.035) | 0.846 / 1.083 | 1.611 / 1.297 | not run | fail |

**Secondary measures, γ = 3**, in the order song file lower, song file upper, room lower, room upper:

| Measure | Values |
|---|---|
| Release R (drop start ÷ cut) | 181, 40, 9.3, 8.0 |
| Extent X | 0.91, 1.18, 0.82, 1.20 |
| Sustain D | 0.69, 0.73, 0.97, 1.10 |

Per candidate (`results/iteration5/eval-g*.json`; C1 reported only, marked not valid as a "hit harder" proxy):
- **C2:** pass for all three (9/9 and 18/18 on both boards).
- **C3:** pass for all three.
- **C5:** pass (still non-discriminating).

On the current source (`results/iteration5/final/`):
- C5b passes with palette range: 0 off-palette pixels; both planted faults red.
- C4 host identity: v8 frames 10/10.
- v8 translation-unit guard: 8/8.

**Disclosed fix.** Adding the host γ knob first changed `gain·level·√level` to `gain·(level·√level)`, a 1-LSB difference. The default replay caught it. I restored the original order, and the default now reproduces b5595c35 on 10/10 inputs. The candidate runs used `powf` paths that this order change does not affect.

### What would move DI (not done; rule says stop)

- On the song file, the drop differs from the build mainly in bass and kick density and in spectral breadth, not in 7.5 ms loudness. A drop accent keyed to bass or onset density, rather than broadband level, is the evident next candidate.
- On the lower board, colour must not lose luma at the drop. That needs either colour holding through the cut (the confident tonal cut moves it), or the B fold weighted toward higher-luma positions.
- Each of these needs a fresh frozen design, and the holdout stays reserved for it.

### Files (iteration 5)

**Criteria and declarations:**
- `TOLERANCE-FROZEN-P6-DI.json`
- `P6-IT5-DESIGN-CANDIDATES-FROZEN.json`

**Results:** `results/iteration5/`
- anchors, sources
- `di-v8.json`, `di-b5595c35.json`, `di-b5595c35-planted-accent-x3.json`
- `calibration-g*.json`, `eval-g*.json`, `di-tuning-g*.json`
- `final/`

**Figure:** `figures/iteration5/fig-di-lstar-tuning.png` (inspected).

**Source** (uncommitted, behaviour-identical defaults):
- `src/k1/core/visual/inhouse_waveform.{h,cpp}`: `kInhouseV9LevelCeilingDb`, `kInhouseV9Emphasis`, host `setV9HostLevel`.
- `tests/host/p6_effect_probe.cpp`: Lstar, Lstar_x3 and extent columns; level arguments.
- `scripts/p6_di.py` (new).
- `scripts/p6_effect_mapping.py`: `prepare-it5` and `calibrate --level`.
- `scripts/p6_figures.py`: the DI figure.

**Driver:** `run-it5-candidate.sh`.

### Ship path

1. **Already in place:** b5595c35 stays on silicon (Captain: "do not revert"). Its source defaults are unchanged.
2. **Remaining:**
   - a fresh frozen design aimed at bass/onset-keyed drop accent and luma-holding colour;
   - tuning evaluation, then a single holdout run;
   - on green, build host-003, deploy, read back, payload capture and score DI on the payload;
   - the Captain's ladder calibration of the DI bar, and his verdict.
3. **Who acts:** the P6 worker (design, host), the lane owner (deploy, capture), and the Captain (ladder, verdict).
4. **What "shipped" means:** a successor build_id read back under composition v9, with payload DI above the eye-calibrated bar, and the Captain saying the drop hits harder.

---

## Iteration 4 (Captain testing ruling, relayed 14 Oct 2026): supersedes conflicting lines below

### In plain English

**What happened.** The Captain allowed palettes and the v9 colour range to be varied as **test variants**. I declared two, and froze them at 13:38:31Z before any variant code ran (`P6-IT4-PALETTE-VARIANTS-FROZEN.json`). The frozen tests themselves were not edited. Both variants live only inside v9; v8 and the product palettes are untouched. Everything else is iteration 3 unchanged: intensity, level emphasis, the colour anchor and the release times.

- **Variant A, lifted low end.** The dark stops at the bottom of each palette are raised but keep their own hue:
  - palette 32: 0:(0,0,0)→(150,0,0); 42:(42,0,0)→(200,0,0);
  - palette 39: 0 and 255:(12,0,10)→(150,0,125); 32:(65,0,45)→(190,0,131); 64:(145,0,80)→(220,0,121).
- **Variant B, restricted colour range.** The v9 colour circle is folded continuously onto a palette sub-range that leaves out the near-black ends:
  - palette 32: 0.3294–1.0 (from the first full-red stop up);
  - palette 39: 0.2510–0.9412;
  - fold: p = lo + (hi − lo)·(1 − |2c − 1|). It has no jump at the wrap, but colours c and 1 − c share a palette position.

**What is true now.**
- **Variant B passes every frozen criterion on the host.**
  - BOOM drop is brighter than the build in all 4 cases.
  - Louder gives more light in every pair, on both boards.
  - Silence still goes dark and reaches true black.
  - The strict hue test passes, and its planted faults still go red.
  - v8 is still bit-identical.
- **Built:** image `b5595c35…` in `arm-p6-v9-host-002`. **Not deployed.** The Captain's visual verdict on the plate is the remaining step; the agent cannot listen.
- **Variant A misses one test by 0.0007.** BOOM room recording, lower board: ×1.1213 against the ×1.122 bar. By the rule I did not tune it.
- **One thing to watch on the device.** In variant B the lower board's *luma* falls ×0.80 from build to drop on the song file, while its *light* rises ×1.42. The drop's colour is a darker-looking red/magenta than the build's (Rec.709 weights green heavily). Whether it reads as a lift is the Captain's call on the plate.

### Iteration 4 results (frozen C1–C4 and C5b unchanged; `results/iteration4/`)

Gains were recalibrated per variant to v8's mean light (0.006181 / 0.005431):

| Variant | Gains (mode 32 / mode 18) |
|---|---|
| A | 0.583 / 0.640 |
| **B** | **0.395878 / 0.591745** (baked as firmware constants) |
| as-is control | 0.853 / 0.965 |

The as-is control reproduces the iteration-3 frame hashes on all 10 inputs.

| Criterion | Case | **B restricted range** | A lifted low end | iteration 3 (as is) | v8 |
|---|---|---|---|---|---|
| C1 S drop/build ≥ 1.122 | file, lower | **1.422** (L 0.799) | 1.230 (L 1.420) | 1.196 | 1.245 |
| | file, upper | **1.260** (L 1.239) | 1.352 (L 0.789) | 1.141 | 0.969 |
| | room, lower | **1.251** (L 1.089) | 1.1213 FAIL (L 1.121) | 0.895 FAIL | 0.803 |
| | room, upper | **1.726** (L 1.589) | 1.869 (L 1.830) | 1.641 | 0.747 |
| C2 same / cross (S) | lower | **9/9, 18/18** (L 9/9, 18/18) | 9/9, 18/18 | 5/9, 11/18 | 3/9, 8/18 |
| | upper | **9/9, 18/18** (L 9/9, 16/18) | 9/9, 18/18 | 9/9, 16/18 | 0/9, 5/18 |
| C2 median slope (dB light per dB) | lower / upper | 0.50 / 0.36 | 0.36 / 0.44 | 0.15 / 0.21 | −0.05 / −0.06 |
| C3a silence R (v8 R) | BOOM l/u | 0.38 / 0.29 (0.61 / 0.82) | 0.38 / 0.30 | pass | — |
| | Sonic l/u | 0.31 / 0.35 (0.88 / 1.30) | 0.31 / 0.36 | pass | — |
| | Dopex l/u | 0.068 / 0.101 (1.10 / 0.92) | 0.066 / 0.106 | pass | — |
| C3b true black | 8 cases | **PASS**, 0.09–0.91 s after the zeros begin | pass | pass | pass |
| C5b | mode 32 / 39 | **0 / 0 off** in 17,712 lit px each. Per-channel max 6,788 / 6,816 off; RGB blend 10,568 / 10,752 off (red) | 0 / 0. Planted 4,688 / 7,786 and 10,738 / 12,024 (red) | pass | — |
| C4 v8 identity | host | **PASS**, 10/10 frame hashes (v8 rebuilt from iteration-4 source) | | | |
| | ARM | **PASS.** `f6daf7fb` against `63300519` (v8 rebuilt after iteration 4): 62 differing bytes, all inside the build-ID string and `__TIME__`, 0 outside; renderer disassembly identical | | | |
| Overall | | **HOST-GREEN** | FAIL (C1, 1 case) | FAIL | FAIL |

**Planted faults:**
- v8 is red on C1 and C2 (the earlier `results/eval-v8.json` stands, v8 being byte-identical).
- The brightness floor on B is red on C3b.
- Per-channel max and RGB blend are red on C5b for B, A and as-is.
- C4: a one-byte edit and B-vs-v8 frame hashes are both red (`results/iteration4/final/c4-planted.json`).

**Firmware-default proof.** With B's constants baked, the default replay (`v9_final4`) reproduces the variant-B run's frame hashes on all 10 inputs, and the frozen evaluation exits 0 (`results/iteration4/final/eval-v9-default.json`).

**Freeze checks.** `freeze_tolerance.py verify` returns ok for `TOLERANCE-FROZEN-P6.json`, `TOLERANCE-FROZEN-P6-C5b.json` and `P6-IT4-PALETTE-VARIANTS-FROZEN.json` against the iteration-4 results.

### Build (do not deploy until the lane owner's identity-checked route)

- **Image:** `_scratch/waveform-migration-001/arm-p6-v9-host-002`, **build_id `b5595c35275b1495c03d52e60d843983511f64d7d09c2499577aed73e8c8a46f`**.
  - pass true; deploy_admissible true; profile titan-product.
  - The f2f1983a configuration plus `--live-defaults …crimson-v9-20261014.json` (`profile_version 9`).
- **Product guards:**
  - MVE present.
  - Software-double call sites 0.
  - int64 conversions {1, 2, 2}, unchanged.
  - Vector instructions 444.
- **Size:** text 518,644, data 121,824, bss 659,412.
- **Firmware content:** `renderV9` and `colourV9` are present. `setV9Host*`, `probeV9Deposit` and the planted faults are absent.
- **Receipt against the working tree:** all 216 `sources` hashes in `receipt.json` match the current working-tree files (0 mismatches), and the preset hash matches. The build was made from the uncommitted iteration-4 tree; HEAD is the lane owner's `ceb51ae` (iteration 3).
- **v8 control build after the change:** `633005193c40…` (C4).

### Files changed in iteration 4 (to commit)

- `src/k1/core/visual/inhouse_waveform.h`: the `InhouseV9Palette` enum; `kInhouseV9Palette = kRestrictedRange`; gains 0.395878 / 0.591745; host `setV9HostPalette`; `colourV9`.
- `src/k1/core/visual/inhouse_waveform.cpp`: lifted stop tables, `sampleStops`, `colourV9`; v9 sampling goes through `colourV9`.
- `tests/host/p6_effect_probe.cpp`: optional palette argument.
- `tests/host/p6_c5b_hue_probe.cpp`: palette argument, with an independent oracle for the declared stops and fold.
- `scripts/p6_c5b.py`: `--palette`.
- `scripts/p6_effect_mapping.py`: calibrate/run palette argument.
- `scripts/p6_figures.py`: `--compare`, the 4-way figure.

Plus all iteration-1 to iteration-3 files already listed below (`ceb51ae` holds iteration 3).

### Figures (inspected)

- `figures/iteration4/fig-boom-build-drop-4way.png`: BOOM 40–65 s, S and L, both boards × file/room, for v8, v9-it3, A and B.
- `figures/iteration4/fig-song-order.png`: v8 against B. All B lines rise with loudness on both boards.
- `figures/iteration4/fig-boom-build-drop.png`: v8 against B.
- `figures/iteration4/fig-v9-colour-split.png`: B.

### Rulings needed

1. **Deploy B (`b5595c35`) for the Captain's visual verdict.**
   - Variant B is a TEST VARIANT under the testing ruling, not a palette change. It is selected by `kInhouseV9Palette` and is v9-only.
   - Whether it becomes product behaviour is a separate decision after the verdict.
2. **The B fold maps two harmony positions to one colour, which halves colour variety.** Accept it, or prefer A after a deliberate re-test.
   - A is not re-tuned and is 0.0007 short on one case.

### Ship path

1. **Already in place:** the iteration-4 source is in the worktree, uncommitted; image `b5595c35` is built and host-green.
2. **Remaining:**
   - Lane owner commits iteration 4.
   - Identity-checked SWD deploy of `b5595c35` (the plate is dark for about 41–47 s while it programs), then readback.
   - BOOM, Sonic and Dopex captures at fixed exposure; rtrace/payload scoring.
   - The Captain's visual verdict.
3. **Who acts:** the lane owner deploys and captures; the Captain judges.
4. **What "shipped" means:** a device readback reporting build `b5595c35` under composition v9 admission, plus the Captain's verdict.

---

## Iteration 3 (lane owner instruction, 14 Oct 2026): supersedes conflicting lines below

### In plain English

**What happened.**
- I replaced the invalid hue test with a stricter one that can fail. C5b was frozen at 13:11:09Z, before v9 ran on it.
- I changed only the colour layer:
  - Colour now follows harmony at full speed only while light is actually being drawn.
  - When harmony is unclear, colour drifts toward its own long-term confident colour, not toward a palette end.
- I recalibrated to the same average light and re-ran every frozen test.

**What is true now.**
- **v9 is still not host-green, so no `arm-p6-v9-host-002` was built.**
  - BOOM drop is brighter than the build in 3 of 4 cases (iteration 2: 1 of 4).
  - Louder-gives-more-light now passes on the upper board, but the lower board got worse (5/9 same-song pairs, against 7/9 in iteration 2).
  - Silence and the new hue test pass.
  - v8 is still bit-identical.
- **Same failure mechanism as iteration 2, so I stopped tuning.** Brightness before colour rises with loudness in every case. But the harmony these recordings give, when confident, maps to palette positions 0.12–0.30, which on palette 32 is black-to-dark red. So the light falls, most of all on louder input (Sonic: 0.59 → 0.20 → 0.12).
- **This is no longer something the colour-hold rule can fix.** The colour is following *confident* harmony, which ruling (c) requires. Even the long-term "held" colour lies in the dark region (`figures/iteration3/fig-v9-colour-split.png`).
- **The remaining blocker is the palette question.**

**Image status.** The iteration-2 image `3587db5f` no longer matches the source (the iteration-3 colour layer and gains). **Do not flash it as current v9.**

### Iteration 3 results (frozen criteria unchanged; `results/iteration3/`)

**v9 per case, bounded level emphasis**, gains 0.853441 / 0.965298, recalibrated to matched v8 mean light (mean S 0.006181 / 0.005431):

| Criterion | Case | v9 iteration 3 | v9 iteration 2 | v8 |
|---|---|---|---|---|
| C1 S drop/build ≥ 1.122 | file, lower | **1.196** (L 1.381) | 0.831 | 1.245 |
| | file, upper | **1.141** (L 0.669) | 0.994 | 0.969 |
| | room, lower | 0.895 (L 0.895) FAIL | 0.896 | 0.803 |
| | room, upper | **1.641** (L 1.597) | 1.487 | 0.747 |
| C2 same-song / cross-song | lower | 5/9, 11/18 FAIL | 7/9, 15/18 | 3/9, 8/18 |
| | upper | **9/9, 16/18 PASS** | 8/9, 17/18 | 0/9, 5/18 |
| C3a silence R (v8 R) | BOOM l/u | 0.32 / 0.26 (0.61 / 0.82) | pass | — |
| | Sonic l/u | 0.19 / 0.32 (0.88 / 1.30) | pass | — |
| | Dopex l/u | 0.052 / 0.098 (1.10 / 0.92) | pass | — |
| C3b true black after zeros | all 8 | PASS, 0.09–0.95 s | pass | pass |
| C5 (original, non-discriminating) | all 8 | 0 off-palette; kept on record as NON-DISCRIMINATING | — | — |
| **C5b** (new, discriminating) | mode 32 / pal 32 | **PASS**: 0 off in 17,584 lit px. Per-channel max: 3,708 off (495 / 864 scenarios). RGB blend: 10,014 off | — | — |
| | mode 18 / pal 39 | **PASS**: 0 off in 17,632. Per-channel max: 6,924 off. RGB blend: 11,850 off | — | — |
| C4 v8 identity | host | PASS, 10/10 frame hashes | | |
| | ARM | PASS. `f6daf7fb` against `5d155cc7` (rebuilt after iteration 3): 67 differing bytes, all inside the build-ID string and `__TIME__`; renderer disassembly equal | | |

**Post hoc (not frozen): the intensity field before colour.**
- C1: ×1.54, 1.49, 1.21, 1.81.
- C2: 9/9 and 18/18 on both boards.
- Lower-board palette coordinate at 0 dB: BOOM 0.24, Sonic 0.20, Dopex 0.26. At +6 dB: 0.21, 0.12, 0.21.

**Planted faults this iteration:**
- Brightness floor: red on C3b.
- Per-channel max and RGB blend: red on C5b, both boards.
- C4: a one-byte edit against `5d155cc7` and the v9-iteration-3-vs-v8 frame hashes are both red (`results/iteration3/final/c4-planted.json`).

**Variants, iteration 3 at matched energy (frozen recommendation rule):**

| Variant | Worst C1 | C2 same-song (lower/upper) | C3 |
|---|---|---|---|
| Constant peak | 0.692 | 0/9, 0/9 | fail |
| Constant energy | 0.548 | 0/9, 0/9 | fail |
| **Bounded level emphasis** | **0.895** | **5/9, 9/9** | **pass** |

Bounded level emphasis remains the recommendation.

### Iteration 3 design (colour layer only)

`inhouse_waveform.cpp` `updateColourV9`, header `colour_anchor_`:

- **Confidence** = gate × min(1, strength/0.08) × **min(1, level/0.5)**.
  - **Why presence, and why 0.5.** Colour moves at full speed only when audible light is being deposited. Level 0.5 is −21 dBFS, 2.5 dB below the corpus music median of −18.5 dBFS.
  - **Evidence.** In BOOM's near-silent tonal cut (level 0.01–0.07) the gate opened fully and recoloured a dark plate, and the drop inherited that colour. I chose this from the iteration-2 trace, so it is a disclosed design choice, not an independent finding.
- **Low confidence** → drift toward the **long-term confident anchor** at Δ/8 s, capped at 0.03 palette/s. Iteration 2 drifted toward the weak centroid. No drift goes toward a palette end or toward `colour_offset`.
- **Anchor.** It learns only from confident colour, weighted by confidence, with **τ 15 s**. That is about 8 bars at 128 BPM (one phrase): long enough to be the section's harmonic colour, short enough to follow a song's key change.
- **Unchanged:**
  - follow τ 0.6 s (about 1/3 bar at 120–140 BPM: harmony, not beats);
  - momentum 0.25 s;
  - follow cap 0.25/s.
- **Refactor, same arithmetic order.** Release, deposit and output were moved into `fadeV9`, `depositV9` and `outputV9`. `probeV9Deposit` (host only) drives them for C5b, and the new host-only planted fault `kRgbBlend` was added.
  - I did not hash-verify the refactor separately. The final default replay reproduces the iteration-3 run's frame hashes (10/10), and C5b exercises the same functions.
- **Untouched:** palettes, AGC and levelling stages ①–④, gamma, dither. No floor and no normaliser.

### Pre-existing suite failures: confirmed

Evidence: `results/iteration3/preexisting-failures-evidence.txt`. Each failing test reads only files with **no diff against HEAD `a36673a1`** (`git diff --quiet HEAD -- <path>`), and each fails for a reason independent of P6.

| Failing test | What it reads | Why it fails |
|---|---|---|
| `test_effects_operation_binding` ×2 | `git rev-parse HEAD`, the protected list, `docs/effects-language-001/*` | It asserts HEAD == checkpoint `a96278c3`. HEAD is `a36673a1`. None of my files are on its protected list. |
| `test_stage_profile` ×3 | `src/k1/core/audio/{audio_pipeline,tempo_tracker,tempo_acf,clock_affine}.cpp`, `scripts/stage_profile.py` | All 5 are unchanged against HEAD, so the identical inputs give the identical "anchor count 0". |
| `test_arm_numeric_probe::test_ac4…` | the sibling BSP folder `.worktrees/sdk-bsp-ra8p1-titan-mini/project/Titan_Mini_npu_ai_face_detection` | The folder does not exist. The test and `scripts/run_useful_npu_probe.py` are unchanged. |

I did not run the suite at HEAD separately: no stash, no export or new worktree.

### Files to commit (lane owner)

- `src/k1/core/visual/inhouse_waveform.cpp` and `.h`
- `scripts/live_boot_composition.py`, `scripts/build_scalar.py`
- `docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json`
- `tests/host/test_p6_boot_composition_v9.py` (8/8 pass after iteration 3)
- `tests/host/p6_effect_probe.cpp`, `tests/host/p6_c5b_hue_probe.cpp`
- `scripts/p6_effect_mapping.py`, `scripts/p6_evaluate.py`, `scripts/p6_c5b.py`, `scripts/p6_identity.py`, `scripts/p6_figures.py`

### Rulings needed (iteration 3)

1. **The palette's low-luma end is now the only remaining blocker found.** The confident harmony on these recordings maps to 0.12–0.30. That is black-to-dark red on palette 32 (black at 0, full red only at 84/255) and dim on palette 39 (near-black at both ends).
   - Options:
     - change the palette;
     - restrict the colour coordinate range away from the black ends (a palette/colour-mapping decision);
     - accept that light follows the palette.
   - Each is outside this task's authority.
2. **v9 boot selection.** It cannot be host-green while ruling 1 stands. If the Captain wants a look at the plate anyway, a fresh build of the iteration-3 source is needed first. `3587db5f` is stale.

### Ship path (not shipped)

1. **In source, uncommitted:** v9 iteration 3, C5b, tests, scripts.
2. **Remaining:**
   - Captain rules on the palette.
   - Re-run the frozen C1–C4 and C5b.
   - On green, build `arm-p6-v9-host-002`.
   - Lane owner commits, deploys over SWD (identity-checked; the plate is dark for about 41–47 s while it programs) and reads back.
   - BOOM, Sonic and Dopex captures at fixed exposure.
3. **Who acts:** the lane owner (build, deploy, capture) and the Captain (palette ruling and visual verdict).
4. **What "shipped" means:** device readback of the v9 build_id under its composition admission, plus the Captain's verdict.

---

## Iteration 1–2 record (below)

## In plain English

**What happened.**
- I built boot composition **v9** as a second mapping inside the same Waveform renderer. It is selected by `profile_version: 9` in the boot composition. v8 is still the default reference and is unchanged.
- In v9, loudness sets how bright and how wide each new deposit is. Trails always fade at the same speed. Colour is a slow layer that holds when the harmony becomes unclear. The Tempo board now has a loudness-to-brightness path, and its motion no longer depends only on the tempo tracker.
- I froze the pass/fail tests before writing any v9 code. I then replayed the BOOM song file and the BOOM, Sonic and Dopex room recordings through v8 and v9 on the host.

**What is true now.**
- **v8 is bit-identical.** The host replay matches byte-for-byte on all 10 inputs. The v8 firmware image matches its pre-change build except for its own build-ID text and the compile-time clock.
- **v9 is not host-green.** It passes silence (quiet passes go dark and digital silence reaches true black) and it passes the colour-integrity check. It fails two frozen tests:
  - BOOM build→drop: v9 passes 1 of 4 cases; v8 passed 1 of 4.
  - Louder input gives more light: v9 has 7/9 and 8/9 same-song pairs in order; v8 had 3/9 and 0/9.
- **Why v9 fails.** The light *before* colour (the intensity field) rises with loudness in every case: build→drop ×1.21–1.81, and every ladder pair in order. That check is post hoc, not a frozen criterion. But in the tonal quiet cut before the drop, the colour layer moves toward the red part of the palette, as the harmony asks. Both v8 palettes are near-black at that end: palette 32 starts at black, and palette 39 ends at black at both ends. So the same intensity gives less light. This is the palette question the Captain asked me to flag, not change.
- **Recommended variant: bounded level emphasis.** At matched average light it is best or equal on every measure. The other two make the drop dimmer than the build in 3 of 4 cases (worst ×0.48 and ×0.41), and are kept host-only.
- **A v9 image is built (`3587db5f…`) and passes the product build guards. It has not been deployed.** Quiet passages in v9 go much darker than in v8: the BOOM quiet cut is close to black. Whether that reads as music on the plate is for the Captain to judge on the device.

**What is left.** Two rulings (below). Then, if the Captain wants it on the device, the lane owner flashes it and records BOOM, Sonic and Dopex at a fixed camera exposure, and the Captain gives the visual verdict. The agent cannot listen.

## 1. Line numbers and behaviour re-verified (file `src/k1/core/visual/inhouse_waveform.cpp`, at `52c1c21`)

| Brief citation | Found at 52c1c21 | Drift |
|---|---|---|
| Decay 0.8 + 3.5 × peak `:222` | `:222` `observation_.decay_rate = 0.8F + 3.5F*peak + (quiet<0.9 ? 10(1-quiet) : 0)` (mode 32) | none |
| Fade target 1 − 0.10 × peak `:341` | `:341` `target_fade = inject ? 1.0F-0.10F*source_peak : 0.82F` (mode 18) | none |
| Colour = chroma centroid `:284–304` | `familyColour` `:270–305` (modes 18/7/8/11). Mode 32's colour is the centroid at `:149–158`, read at `:184–187`, plus an amplitude walk `peak_stage2*0.25` (`:181`) | mode 32 colour lives at `:149–187`, not `:284–304` |
| Tonality gate `:130–157` | spectrum smoothing `:126–135`, chroma fold and normaliser `:136–145`, gate `:146–148`, centroid `:149–158` | none |
| Tempo board has no level→brightness path `:334–352` | confirmed: the mode-18 colour amplitude is chroma energy (`familyColour`, `:301–304`); level sets only the fade (`:341`) and the position (`:354`) | none |

Renderer path, proven from the symbols of the v9 image: `palette_runtime.cpp:243 renderInhouse` → `InhouseWaveformRenderer::render` → `renderV9`. `renderV9` and `updateColourV9` are present in `arm-p6-v9-host-001/symbols.txt` and absent from the v8 builds. `setV9Host*` is absent from firmware.

**Stages not touched.** Each was mapped first; the change avoids all of them.

| Stage | Location | P6 change |
|---|---|---|
| ① Loud guard | `pdm_target.c:578–588`, `k1_pdm_sensitivity.h` | none (file untouched) |
| ② Peak AGC | `inhouse_audio.cpp:241–248` (`follower_`, `peak_scaled_`) | none (file untouched) |
| ③ Spectral AGC | `inhouse_audio.cpp` ≈ 296–353 | none (file untouched) |
| ④ Chroma normaliser + tonality gate | `inhouse_waveform.cpp:136–158` | Unchanged and shared. v9 only *reads* its outputs (`gate`, `centroid`, strength) at new lines 174–181 |
| Palettes, display stage/gamma, dither, packer | `product_palette_data.cpp`, `palette_runtime.cpp:281–294`, wire packer | none (files untouched) |

The v8 guard test proves the renderer translation unit is unchanged without the v9 define. It preprocesses the file against `52c1c21`, and goes red on a planted `.cpp` edit and on a planted header field.

## 2. Frozen criteria (frozen before any v9 code or replay output)

- `TOLERANCE-FROZEN-P6.json`, frozen by `scripts/freeze_tolerance.py` at **2026-10-14T12:53:45Z**.
  - Body sha256: `e3c4470c8ca40488f9af6ea1f36e013cc99af9467c488c43b99d8ed152bc85f2`.
  - File sha256: `e5b21fa8f220002d061ed39ae47595415e253509b08d61b7095aabe815b3fb08`.
  - Mode 0444. `verify` returns ok, and every result file postdates the freeze.
- **The criteria themselves:**
  - **C1:** BOOM build 45–50 s → drop 55.5–60 s, both boards, song file and room recording. Light S (mean (R+G+B)/3 after the display stage ×0.25) must rise ≥ ×1.122 (+0.5 dB). Precondition: the input loudness rises.
  - **C2:** loudness ladder. BOOM, Sonic and Dopex room recordings at −6, 0 and +6 dB. For pairs whose loudness differs by ≥ 3 dB, the louder one has more light S. Required: all 9 same-song pairs, and ≥ 80 % of cross-song pairs.
    - "Matched-palette luma" was defined as the S order: at a fixed chromaticity, luma is a fixed multiple of S. Rec.709 luma L is reported beside it.
  - **C3a:** Q2 preserved. Room post-roll R = silence/music median S. Where v8 R ≤ 0.1, v9 must be ≤ 0.1; elsewhere v9 ≤ 1.25 × v8.
  - **C3b:** true black. Every renderer output channel of both boards must be exactly 0 from 5.0 s (the existing dwell) after the start of the appended 8 s of digital zeros.
  - **C4:** v8 bit-identity, host and ARM.
  - **C5:** palette hue. Off-palette lit pixels ≤ 0.1 %.
- **Planted faults that must go red:** v8 on C1 and C2; a brightness floor on C3b; per-channel max on C5; a one-byte ARM edit and v9-vs-v8 hashes on C4.
- **Inputs:** the P3 inputs, each scaled by one gain so that the host in-house raw RMS median equals the device's recorded P3 median (P3 agc-trace method). Manifest: `replay-records/inputs.json`.
- **Replay chain:** k1_poly24 at 24 kHz/180 → InhouseAudio analysis24 → renderer at a 10 ms period → the renderInhouse display stage. It omits the donor admission gate, the packer, gamma, dither and the 24/255 cap. This is a host oracle, not the M85.

**Disclosed iterations (the criteria were never edited).**

- **Iteration 1.** All three variants failed.
  - Cause: my frozen colour rule ("confident" = gate ≥ 0.5 and strength ≥ 0.08) held in only 3–23 % and 1–15 % of frames. Colour therefore stayed at `colour_offset` 0.05, which is near-black on both palettes. The colour coordinate also fell with level (sonic 0.58 → 0.05).
  - Records: `results/iteration1/`.
- **Iteration 2** (the shipped design). Confidence is graded: follow speed × gate × min(1, strength/0.08), and drift only when strength ≥ 0.02. This is disclosed in source at `inhouse_waveform.cpp:521–524`.
  - Probe and header changes after iteration 2 were diagnostics only (added CSV columns and observation fields), the baked gain constants, and a `default` probe mode.
  - The final firmware-default replay reproduces the iteration-2 emphasis frame hashes on all 10 inputs (`replay-records/v9_final` = `v9_iteration2_emphasis`).
  - `p6_evaluate.py` and `p6_effect_mapping.py` are byte-unchanged across every v9 run (`results/*/sources.sha256`).
- **A hypothesis I tested and withdrew.** I suspected v8's level→position mapping (louder deposits further out, so they scroll off sooner) caused "louder = dimmer". The data refuted it: position moved 121 → 125 px while the intensity rose. The colour move is the cause. No change was made for it.

## 3. What changed

| File | Lines | Change |
|---|---|---|
| `src/k1/core/visual/inhouse_waveform.h` | 16–33 | `K1_INHOUSE_MAPPING_V9`: the variant enum; planted-fault enum (host only); `kInhouseV9Variant = kLevelEmphasis`; gains 0.710454 (mode 32) and 0.873925 (mode 18) |
| same | 99–106 | v9 observation fields: level, tempo_weight, tonality_gate, centroid_strength, intensity_mean |
| same | 120–128 | `setV9HostConfig`, compiled only with `K1_INHOUSE_V9_HOST_VARIANTS` (the host probe) |
| same | 152–171 | v9 state: intensity field[160] Q16, deposited colour[160], colour position/velocity |
| `src/k1/core/visual/inhouse_waveform.cpp` | 85–91 | v9 state reset |
| same | 174–181 | modes 32/18 hand off to `renderV9` after stage ④ (which is read, not changed) |
| same | 515–548 | `updateColourV9`: the slow colour layer |
| same | 550–700 | `renderV9`: level map, intensity field with constant release, transport, deposit kernel, output |
| `scripts/live_boot_composition.py` | 12–17, 39–40 | `profile_version` 8 or 9; `profile_defines`: 8 → none, 9 → `-DK1_INHOUSE_MAPPING_V9=1` |
| `scripts/build_scalar.py` | 25, 938–941 | applies the profile define; v9 receipts carry `inhouse_mapping` |
| `docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json` | new | v8 preset with only `profile_version: 9` (test-proven) |
| `tests/host/test_p6_boot_composition_v9.py` | new | 8 tests: profile mapping, same setters, refused profiles 7/10/"9"/9.0, v8 TU guard plus 2 planted reds |
| `tests/host/p6_effect_probe.cpp`, `scripts/p6_effect_mapping.py`, `scripts/p6_evaluate.py`, `scripts/p6_identity.py`, `scripts/p6_figures.py` | new | replay probe, input preparation and calibration, frozen evaluator, C4 identity, figures |

**v9 design, with τ values in seconds.**
- **Level.** l = clamp01((20·log10 vu_level + 33)/24), taken from the pre-AGC RMS through a fixed transfer, not an adaptive one. Deposits happen only when admitted and l > 0.
- **Deposit.** Triangular kernel of half-width 0.5 + 3.5 l px, with peak g·l^1.5 (bounded level emphasis).
- **Intensity field.** y = max(x, y·e^(−dt/τ)). τ is 0.29 s for mode 32 and 0.125 s for mode 18: v8's own release at the corpus-median AGC level, now held constant.
  - Confirmed quiet keeps v8's extra 10/s drain.
  - The max is taken on intensity, not per RGB channel.
- **Trail colour policy (declared): trails keep their deposited colour.** This is v8's behaviour. A pixel takes a new colour only where a new deposit exceeds its decayed intensity.
- **Colour layer.**
  - Follow toward the centroid at speed × confidence ÷ 0.6 s, with momentum τ 0.25 s and a cap of 0.25 palette/s.
  - When the gate is closed: hold, and drift toward a weak centroid at Δ/8 s, capped at 0.03 palette/s. The colour never snaps to a palette end.
  - The v8 amplitude walk is removed: it was a fast level→colour path.
- **Tempo board.**
  - Motion = (w·tempo + (1−w)·energy). w = 0.7·smoothstep((confidence − 0.30)/0.30), so the tracker never carries more than 70 %.
  - Energy layer = 30 + 90·clamp(novelty/0.12) px/s.
  - Deposit amplitude comes from level (the new level→deposit path).
  - Nothing assumes the current tracker confidence is good: confidence 0 leaves the energy layer driving 100 % of the motion.

## 4. Results (host replay; numbers from `results/`)

**C1 — BOOM build → drop, light S ratio (L in brackets). Pass needs ≥ 1.122.**

| Case | Input loudness | v8 | v9 (final) | v9 intensity, post hoc |
|---|---|---|---|---|
| file, lower | ×2.21 | 1.245 (1.236) | 0.831 (0.969) | ×1.54 |
| file, upper | ×2.21 | 0.969 (0.981) | 0.994 (0.543) | ×1.49 |
| room, lower | ×1.71 | 0.803 (0.906) | 0.896 (0.907) | ×1.21 |
| room, upper | ×1.71 | 0.747 (0.581) | **1.487 (1.404)** | ×1.81 |

C1 result: v8 FAIL (planted red as required); v9 FAIL, 1 of 4.

The palette coordinate moved from 0.59 (build) to 0.27 (drop) on the file, and from 0.37 to 0.28 in the room. It moved during the quiet cut at 50.5–54 s, where the gate was fully open (0.76–0.95). In the drop it holds, with the gate mostly closed (`figures/fig-v9-colour-split.png`).

**C2 — loudness ladder.** v8 is red as required.

| Board | Same-song pairs in order (S) | Cross-song pairs in order (S) | Result |
|---|---|---|---|
| v8 lower | 3/9 | 8/18 | fail |
| v8 upper | 0/9 | 5/18 | fail |
| v9 lower | 7/9 | 15/18 | fail |
| v9 upper | 8/9 | 17/18 | fail |

- Both v9 same-song misses are Sonic +6 dB, where the colour moved from 0.30 to 0.18.
- v9's median same-song slope is +0.35 and +0.33 dB of light per dB; v8's is −0.05 and −0.06.
- At 0 dB the three songs lie within 0.7 dB of each other (−18.5, −17.9, −18.6 dBFS). These are near-ties and are reported, not scored.
- Post hoc, on the intensity field: 9/9 same-song and 18/18 cross-song, on both boards.

**C3 — silence: v9 PASS.**
- C3a, v9 R (v8 R in brackets):

  | Song | Lower | Upper |
  |---|---|---|
  | BOOM | 0.32 (0.61) | 0.27 (0.82) |
  | Sonic | 0.27 (0.88) | 0.34 (1.30) |
  | Dopex | 0.079 (1.10) | 0.115 (0.92) |

  The host post-roll is not dark for v8: the iPhone room floor sits at about −26 dB. v9 is darker than v8 in all six cases.
- C3b: true black from 0.09–0.18 s (lower) and 0.63–0.94 s (upper) after the zeros begin, well inside the 5 s dwell (v8: 0.58–0.83 s).

**C5 — palette hue: v9 PASS, but the gate is NON-DISCRIMINATING.** 0 off-palette lit pixels on all 8 input×board cases (v8: 0.17–7.3 % on the lower board). The planted per-channel max **also passed C5**. Because colour now moves slowly, neighbouring deposits carry nearly the same colour, so a per-channel max manufactures almost no new hue on this corpus. Under the frozen rule, C5 is non-discriminating and is not a proof against hue mixing. The intensity-field max rules out hue mixing by construction (`inhouse_waveform.cpp`, deposit block).

**C4 — v8 bit-identity: PASS.**
- **Host:** frame SHA-256 equal before and after on all 10 inputs (`results/identity/c4-host.json`).
- **ARM:** v8 composition build `f6daf7fb` (pre) against `603b4add` (post).
  - 13,234,244 loadable bytes each. 68 differ, all inside the K1_BUILD_ID string (64) and RT-Thread `__TIME__` (4); 0 differ outside them.
  - `inhouse_waveform.o` disassembly is identical (`results/identity/c4-arm.json`).
  - The pre-change command reproduced `f6daf7fb` exactly (the P4-corrections image). Two `f6daf7fb` rebuilds differ only in `__TIME__`.
- **Planted:** a one-byte edit and v9-vs-v8 hashes both go red (`c4-planted.json`).

**Planted faults (summary).** v8 is red on C1 and C2. The floor is red on C3b. Channel max is green on C5 (non-discriminating, see above). The ARM byte and the host hash are red on C4.

## 5. Variant comparison at matched optical energy (iteration 2)

Each variant's per-mode gain was set so that the mean light on BOOM file + BOOM room + Sonic equals v8's (0.006181 and 0.005431). Dopex was held out. All three matched within 0.0001 %.

| Variant | Gains (32/18) | C1 min S ratio | Same-song ladder (lower/upper) | C3 | C5 |
|---|---|---|---|---|---|
| Constant peak (level → width only) | 0.357 / 0.438 | 0.479 | 2/9, 2/9 | fail (BOOM lower 0.87 vs v8 0.61) | pass |
| Constant integrated energy | 0.609 / 0.742 | 0.410 | 0/9, 0/9 | fail | pass |
| **Bounded level emphasis** | **0.710 / 0.874** | **0.831** | **7/9, 8/9** | **pass** | **pass** |

**Recommendation: bounded level emphasis.**
- By the frozen rule, no variant passes all four, so I recommend the one that passes most. It passes 2 of 4; each of the others passes 1.
- It also has the best C1 and C2 numbers.
- [INFERENCE] Constant peak and constant energy lose because, at matched average light, the drop's extra width cannot outweigh the colour moving into the palettes' dark end.
- The other two remain host-only (`InhouseV9Fault` and `setV9HostConfig` are not compiled into firmware).

## 6. Build check (do not deploy)

- `_scratch/waveform-migration-001/arm-p6-v9-host-001`: **build_id `3587db5f82ab89989e07f9ce92dc0843d5c4772f08c478b10377f76d8555b2c9`**.
  - pass true; deploy_admissible true; profile titan-product.
  - The f2f1983a configuration plus `--live-defaults …-v9-20261014.json`.
- **Product guards:**
  - MVE attribute present.
  - Software-double call sites: 0.
  - int64 conversions {d2lz 1, d2ulz 2, l2d 2}, unchanged.
  - Vector instructions: 444 (v8: 443).
  - The DTCM guards passed (the build runs them).
- **Size:** text 518,164 (−320). data 121,824 (+5,216, the v9 per-board intensity and colour arrays). bss unchanged.
- The generated composition header is identical to v8's (`65462a7a…`).
- **Not deployed.**
- v8 rebuild after the change: `603b4add…` (the C4 control).
- **Host suite:** 318 passed, 6 failed (`results/host-suite-after.txt`). [INFERENCE] The 6 failures predate this change:
  - None of them reads a file I changed.
  - Two check the historical checkpoint `a96278c3…` and its protected sources (my four files are not in that list; `audio_pipeline.cpp`, `palette_runtime.cpp` and others there already differ at HEAD).
  - Three are the stage-profile anchor in `audio_pipeline.cpp`.
  - One is the NPU missing-goldens check.
  - I did not run a pre-change baseline, to avoid mutating the shared worktree.
- The new test file passes: 8 of 8.

## 7. Figures (each opened and inspected)

- `figures/fig-boom-build-drop.png`: per board × input, L and S against time, v8 (grey) against v9 (red), for BOOM 40–65 s.
  - v9 has far more contrast: bright build crescendo, near-black quiet cut, bright drop.
  - v8 stays flat.
- `figures/fig-song-order.png`: median L and S against input loudness, three songs × three levels.
  - v8's lines slope down.
  - v9's slope up, except Sonic +6 dB.
- `figures/fig-v9-colour-split.png`: intensity field, palette coordinate with the gate, and S, through build, cut and drop.

## 8. Evidence

`SHA256SUMS.txt` covers everything in this folder. Replay CSVs for v8 and final v9 at 0 dB are in `csv/` (gzip). The frame bytes themselves were hashed and then deleted; they are reproducible from `replay-records/inputs.json` and the scripts. No commit was made: the lane owner commits.

## 9. RULING NEEDED

1. **v9 boot selection.**
   - Options:
     - (a) Keep v8 as the boot default, and flash the v9 image `3587db5f` for the Captain's visual verdict.
     - (b) Do not take v9 to the device until the palette ruling.
   - Evidence: v9 is not host-green (C1 1/4, C2 7/9 and 8/9). It is far more level-responsive than v8 at the same average light, and its silence is darker.
   - **Recommendation: (a).** The host failures are colour/palette-driven, not mapping-driven (the intensity field passes post hoc), and only the plate can show whether the drop lands.
   - Known risks to watch on the device:
     - quiet passages render near-black (the BOOM cut);
     - the level map is absolute (−33 to −9 dBFS pre-AGC), so a quieter room renders dimmer overall.
2. **The palette's low-luma red end (flagged, not changed).**
   - Palette 32 (`BlacK_Red_Magenta_Yellow`) starts at black (0,0,0) and reaches full red only at 84/255.
   - Palette 39 (`K1_Crimson_Gold`) is near-black (12,0,10) at both ends.
   - The harmony→colour map puts BOOM's quiet cut and drop at about 0.27, which is dark red on palette 32. Palette arithmetic: the same intensity gives about ×0.41 the light on palette 32 and ×0.70 on palette 39, compared with the build colour at 0.59. Replay: S ratio ÷ intensity ratio = ×0.54, 0.67, 0.74 and 0.82 across the four C1 cases.
   - Options: a palette change; restricting the coordinate range away from the black ends; or accepting it.
   - This is a palette question for the Captain. Every option is outside P6's authority.

## Ship path (not shipped)

1. **Already in source (uncommitted in the lane worktree):**
   - v9 behind `profile_version 9`;
   - the v9 preset;
   - the tests;
   - the v9 image `3587db5f` in `_scratch/waveform-migration-001/arm-p6-v9-host-001`.
2. **Remaining steps:**
   - the lane owner reviews and commits;
   - the Captain rules on 1 and 2;
   - an identity-checked deploy of `3587db5f` (or of a rebuild after any palette ruling) over SWD, with the plate dark for about 41–47 s while it programs;
   - readback;
   - BOOM, Sonic and Dopex captures at a fixed camera exposure, and rtrace/payload scoring.
3. **Who acts:**
   - the lane owner deploys and captures;
   - the Captain gives the visual verdict (the agent cannot listen).
4. **What "shipped" means:** a device readback reporting the v9 build_id under its composition admission, plus the Captain's verdict. Only then does v9 become the boot composition.

## Governance note

`workflow_context` refused this worktree ("not a registered worktree of the launch project"), so no workflow_verify or gate record exists. The gates above are command exit codes and receipts.

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (P6 host worker, session c8f0d616) | Created: criteria frozen 12:53:45Z; v9 implemented; two disclosed design iterations; host replay, variant comparison, C4 identity, v9 build `3587db5f`, figures. |
