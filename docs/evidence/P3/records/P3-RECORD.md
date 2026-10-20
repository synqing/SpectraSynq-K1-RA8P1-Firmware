---
abstract: "Lane P3 expressiveness audit record (14 Oct 2026). Does the Titan's light follow the drop, silence, the beat and song loudness? Wire-payload, device-state and iPhone evidence from three recordings on build 9ab90873. Agent cannot listen."
---

# P3 — Does the Titan's light follow the song?

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This audit was offline and read-only. It did not touch the device.

Captain page: https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM. Its local source is `P3-CAPTAIN-PAGE.html`, which loads its figures from `analysis/`. The page is private until it is shared from its Share menu.

## Iteration 6 (14 Oct 2026): text only; supersedes conflicting lines in the iteration-5 block

Answers [`REVIEW-ITERATION-5.md`](REVIEW-ITERATION-5.md) N1–N5.

- **N1.** At 20 Hz the test found the planted beat in 42% of seeds (reviewer). The page no longer says "missed (42%)".
- **N2.** 0.58–0.78 is the light's coherence with the tracker's phase on the room replay; the phase's coherence with the level is 0.80 (reviewer).
- **N3.** The coherences, the 13% reach and the 42% are attributed to the reviewer. The 13% reach is restricted to 30 Hz, "0% detection at about 8%" is added, "edited once" becomes "edited", and the provenance line reads "before its final run".
- **N4.** The camera bar line is not claimed: 4.4 times its neighbours (controls ≤ 1.6), but 2.4 times a fitted floor against 2.6 off-grid (reviewer). The two tests disagree.
- **N5.** The `fig-q3` alt text no longer gives the upper board a bar peak; the caption notes the percentages are band shares.

## Iteration 5 (14 Oct 2026): supersedes conflicting lines in the iteration-4 block and below

Answers [`REVIEW-ITERATION-4.md`](REVIEW-ITERATION-4.md) (text only, no new frozen analysis); itemised in [`RECEIPT-ITERATION-5.md`](RECEIPT-ITERATION-5.md).

- **Bar swell (R1).** The once-per-bar line is shown on the Hybrid K1 board only. The post-hoc peak-to-neighbour ratio is 4.5 (song) and 6.2 (room), against controls up to 1.8 and 3.1. On the Tempo board the bar reads 1.3 and 2.1 against controls of 1.3 and 1.5, which is no line; its beat line is 4.6 and 6.1. Both earlier bar figures were band shares. The 1 s moving mean (review 3, "3–5%") removes about 79% of bar power, and the undetrended 13–19% (iteration 4) includes broadband slow variation. The camera lower band's 23% is mostly slow variation. Whether it holds a bar line depends on the test: 4.4 times its neighbours (controls up to 1.6), but only 2.4 times a fitted slow-variation floor, against 2.6 off-grid (reviewer). Not claimed.
- **"3 of 4" (R6).** This counts light series where the frozen test finds the beat. It does not mean "the fourth has no beat": Hybrid K1 song has a beat line at 5.2× its floor (reviewer).
- **Camera "not found" (R2).** The frozen test reliably sees a beat only at about 13% of the rhythmic power (reviewer amplitude sweep: 0% detection at 7.9%, 95% at 13.6%). The period clause decides every verdict; the share clause was never exercised. The lower band's 2.30 Hz line holds 3%.
- **67 ms (R3).** A correlation peak, not a latency. It includes the iPhone audio–video offset (uncalibrated) and the camera's 33 ms frame step.
- **Timeline (R4).** `p3_iteration4.py` was edited at 10:15:53Z, after a first run and after the freeze (10:11:01Z). The criterion string and the planted recipe did not change. The criterion was frozen after the reviewer's iteration-3 numbers were read; the page now says so beside the Q3 test sentence.
- **Tracker inference (R5).** It is supported on the song-file replay: the light's coherence with the level at 2.30 Hz is 0.77–0.95, against 0.01–0.06 with the tracker's phase. It is undecided on the room replay, where the light is also coherent with the tracker's phase (0.58–0.78), the phase itself is coherent with the level (0.80), and 2.30 Hz is exactly twice 69 BPM (all coherences: reviewer). The decisive test is a host replay with the tracker's phase held still. It stays [INFERENCE].
- **Input evidence (R7).** The BOOM slice of `p3-data.json` is committed at `docs/evidence/P3/inputs/p3-data-boom.json`.
- **Figure (R8).** The `fig-q3` control lines are labelled "95th pct, loudness" and "95th pct, accent".

## Iteration 4 (14 Oct 2026): supersedes conflicting lines in the iteration-3 block and below

Answers [`REVIEW-ITERATION-3.md`](REVIEW-ITERATION-3.md); itemised in [`RECEIPT-ITERATION-4.md`](RECEIPT-ITERATION-4.md). The numbers come from `analysis/iteration4.json`, which is also committed at `docs/evidence/P3/iteration4/`. The criterion is frozen in `analysis/TOLERANCE-FROZEN-iteration4.json` (sha256 `d8a68bda…`).

- **Q3, withdrawn:**
  - "the light pulses per bar, not per beat";
  - "bar, not beat";
  - "almost nothing at one beat";
  - "the upper-edge band shows nothing".
- **Q3, restated** (BOOM 55.5–75 s; rate shares measured as power within ±0.06 Hz of each rate, over 0.3–5 Hz, with no moving-mean step):
  - **Host replay [FACT].** The frozen beat test finds the 138 BPM beat in 3 of 4 light series: the Tempo board from both inputs, and the Hybrid K1 board from the room recording. Those series hold 11–14% of their rhythmic power at the beat and 13–19% at the bar. The input level itself holds 15–17% at the beat.
  - **Camera [FACT].** The frozen test finds no beat in either band.
    - The lower-edge band holds 23% at the bar, 9% at the accent rate and 3% at the beat. It follows the music's loudness (linear RMS) at r 0.66, 67 ms behind, p 0.001; with the light leading, p 0.17.
    - The upper-edge band follows neither reference (p 0.82–0.96). It has no musical period (comb best 0.51 s) and holds 8% at half-time (14% after a 1 s moving mean). It is not attributed to a board.
  - **Device payload:** cannot resolve a 0.435 s beat.
  - **[INFERENCE]** Whether the beat survives emission, the plate and the cap is open.
  - **[INFERENCE]** The host light's beat probably comes from the level the effects read, not the tracker (which reads 92 or 69).
- **Music reference.** Iteration 3's "music onsets" is an accent-weighted rise envelope: 0.2% at the beat, against 17% for linear loudness. Its lower-edge match also scores p 0.001 with the light 367 ms ahead, so it is shared rhythm, not a response.
- **Planted check.** The frozen test passes at 30 and 100 Hz: it finds a planted 2.30 Hz beat and rejects the 3+3+2 accent pattern alone. It **fails at 20 Hz**, where the planted beat was missed, so there is no frozen verdict on the 20 Hz audio. The red run (with the beat removed) makes all three rates fail. Both runs exit with rc 1, by the frozen rule.
- **Bar against beat.** The iteration-3 review's "bar 3–5%" came from a 1 s moving mean applied before the spectrum. That step keeps about 21% of the bar-rate power. Without it, the bar holds 13–19%. Both sets are in `iteration4.json`.
- **Supplementary, post hoc:** the peak-to-neighbour ratio, against 7 off-grid control rates. The lower-edge camera band has a small 2.30 Hz line (3.5×; controls ≤ 1.6). It is not claimed.
- **Lede.** "Mostly colour" now applies to the lower board only (light ×0.93). On the upper board the light halves (×0.53) as well as reddening (×0.46).
- **"Percentile among lags"** (iteration 3) is a descriptive rank, not a test. It is removed from the page.

## Iteration 3 (14 Oct 2026): supersedes conflicting lines in the iteration-2 block and below

Answers [`REVIEW-ITERATION-2.md`](REVIEW-ITERATION-2.md); itemised in [`RECEIPT-ITERATION-3.md`](RECEIPT-ITERATION-3.md). Numbers come from `analysis/iteration3.json` and `analysis/q3-iteration3.json`.

- **Q3.** The zero-padded detrend is fixed. Withdrawn: "camera bands r 0.87" (now 0.12), "the light does not repeat", "r 0.18".
  - **Camera:** the lower-edge band follows onsets (r 0.31, p 0.001) and repeats at one bar (0.31). The upper-edge band shows nothing. Board attribution is still not established.
  - **Host-replay payload luma, 55.5–75 s:** one bar 0.36–0.49 (99th percentile); one beat 0.01–0.14; onset alignment r 0.28–0.52, p 0.001. **The light pulses per bar, not per beat.** The device snapshots cannot resolve a beat.
- **Tempo** (the Tempo board's in-house tracker, host replay, gain sweep 0.7–1.6× plus the room recording):
  - verse 69 (half-time);
  - about 138 at 33–43 s;
  - drop onset about 112, with the gate closed;
  - drop mostly 92 or 69. The room recording reads 92 for 84% of the drop and is never near 138; the song file reaches 138 in 15% of it at gain 1.155 or above.
  - The mechanism is [INFERENCE]. The tempo drives travel speed (`inhouse_waveform.cpp:349–352`).
- **Q1 metric** is Rec.709 luma. Drop against build, luma ×0.63 / ×0.24 = light ×0.93 / ×0.53 × colour ×0.68 / ×0.46. The flattening stage is the effect mapping (colour from harmony, louder = faster fade); see `docs/evidence/P3/agc-trace.html`. This replaces "[NOT TRACED]".
- **Q2**, median against median (music window 2 s … end − 0.5 s): BOOM ÷193 / ÷13.5; Sonic upper ÷33, lower ÷1.14; Dopex 0.
  - The Dopex boards are dark from 76.3 s, before the silence flag (80.7 s).
  - The donor admission gate is the likely cause [INFERENCE]; it cannot be traced because its input is not recorded.
- **Ceiling.** 15,407 is the display stage's full-scale code, reached in 5 of 436 lower-board snapshots. The upper ≈12,250 ceiling is [INFERENCE].
- **Renderer.** `product_effect_renderer.cpp` is compiled but discarded at link, and is not on the mode-32/18 path.
- **Donor BPM**, BOOM: 139 (28–55.5 s), 103 (55.5–75 s). The donor drops to about 103 in the drop. Not read by v8.

## Iteration 2 (14 Oct 2026): supersedes conflicting lines below

This section answers the independent review ([`REVIEW-ITERATION-1.md`](REVIEW-ITERATION-1.md)). Every row is listed in [`RECEIPT-ITERATION-2.md`](RECEIPT-ITERATION-2.md). The numbers come from `analysis/iteration2.json`.

- **Renderer.** Both v8 effects render in `InhouseWaveformRenderer` (`palette_runtime.cpp:243–288` → `inhouse_waveform.cpp`), not `product_effect_renderer.cpp`, which this build does not run (`build_scalar.py:728`).
  - **Hybrid K1** (mode 32) reads `max(peak_scaled, observed_peak_scaled)`, `vu_level` (not AGC'd), the silence flag, and the spectrum folded into a chroma that has its own adaptive normaliser.
  - **Tempo** (mode 18) reads the same peaks and `vu_level`, spectral energy, novelty, silence, and the in-house tempo, phase and confidence (`inhouse_audio.cpp:357–359`). It is gated by `clamp01((c−0.30)/0.30)` (`:347`).
- **[FACT, host] The Tempo board's tempo, on the BOOM song file** (`tests/host/p3_tempo_probe.cpp`, `analysis/host-tempo-boom.csv`):
  - 139 BPM at 0–7 s;
  - **69 at 7–33 s**;
  - 138 at 33–43 s;
  - hunting at 43–50 s (gate 0.10);
  - 113 at 50–57 s (gate 0.06 in the first 1.5 s of the drop);
  - **92 at 57–67 s and 69 at 67–75 s**, with the gate open.

  92 is one 3/8 group of a 138 bar, and 69 is half-time. Scope: song file, host only. The device's in-house tempo is not recorded.
- **Q3 is about the plate, not a board.** The camera bands correlate at r 0.866 after detrending, and each follows the upper payload more than the lower (ρ 0.558 / 0.506 against 0.362 / 0.347).
  - The plate follows onsets weakly: r 0.185, p 0.020 (Bonferroni 0.04), lag about 0.1–0.27 s (plateau plus 50 ms bins).
  - Test checks: power 5 / 18 / 20 / 20 of 20 at 0.1 / 0.2 / 0.3 / 0.5× std; false positives 0/40; mismatched section p 0.42.
  - Recorded red run: `q3-red-run-plant0.json`, exit 1.
- **Silence, median against median.**
  - BOOM ÷193 (lower) and ÷13.5 (upper).
  - Dopex: silence median exactly 0.
  - Sonic: upper ÷33, lower ×1.1. The device's microphone heard post-roll bursts up to 0.161 raw RMS at 32.1 s, against a Sonic music median of 0.127, and the lower board followed them. This is room sensitivity, not a silence fault.
- **Brightness against level.** Brightness weakly follows the pre-AGC level, not the AGC output.
  - Raw-RMS ρ is 0.216 (p 0.007), 0.514 (p < 0.001), 0.361 and 0.362 (p < 0.001) for BOOM lower, Sonic upper and both Dopex boards.
  - AGC ρ is −0.06…0.12 for five of six pairs.
  - Donor-VU ρ is −0.39…0.07: donor VU tracks the drop (×3.93) but not brightness.
- **AGC in true silence.** After Dopex the median `peak_scaled` is 0.68 at raw RMS 0.018, against a music median of 0.69 at 0.096. The AGC lifts room noise to music level; other gates keep the boards dark.
- **Payload ceiling.** Sonic's upper board is within 5% of its maximum (12,252 = 18.7% of full scale) in 77.6% of snapshots, against 10.3% for BOOM and 2.0% for Dopex. It is pinned, which is most of its "3.9× brighter".
- **Units.** 20·log10 for amplitudes. 10·log10 for payload luma and for sRGB-decoded camera levels. The camera spans about 1–3 dB, the payload 4–11 dB.
- **Wording.** "Wire/rendered" now reads "submitted LED payload (pre-emission buffer; snapshot `evidence_level` ON_TARGET_SUBMISSION_BUFFER_NOT_GPIO_OR_PHOTONS)". The 24/255 cap and emission are not covered.
- **Captain questions** (agent cannot listen):
  1. Does the BOOM drop at 55.5 s land?
  2. Does the upper board feel on the beat at 33–43 s and off it at 57–75 s?
  3. After Sonic, should the Titan react to room sound or hold dark?

## Iteration 1 — superseded where it conflicts with the iteration-2 and iteration-3 blocks above

## In plain English

The light changes all the time, but mostly not with what a listener feels:

- The drop in BOOM renders dimmer than the build before it.
- The beat barely reaches the light.
- Three songs reach the effects at the same level.
- Silence works on the wire for most boards. The exception is the lower board after Sonic.

The automatic gain control (AGC) halves the loudness range. But brightness barely follows even the AGC output, so changing the AGC alone is unlikely to make the drop land. That last point is an inference, not a measurement. Musical verdicts belong to the Captain; the agent cannot listen.

## Inputs

- **Recordings:** `_scratch/waveform-migration-001/canonical-audio-v01/` contains `boom-admission-fix-candidate-001` (BOOM 0–75 s), `sonic-6-30-admission-fix-candidate-001` (Sonic Samba 6–30 s) and `dopex-admission-fix-holdout-001` (Dopex 0–75 s, holdout). Each holds `original.mov`, the wire `sample-*` snapshots, `observation.json` and `av-validation.json`.
- **What was running:** the lower board ran Waveform Hybrid K1 32 and the upper board ran Waveform Tempo 18 with the Crimson Gold palette, on build `9ab90873` (14 Oct, before P2). The Titan now runs `a8215f75`, which has not been re-recorded with music.
- **Wire brightness:** the mean Rec.709 luma of each board's 160-pixel GRB48 payload, with 16-bit codes divided by 65,535.
- **Camera bands:** 320×180 grey frames; upper band rows 48–57 and lower band rows 76–85, columns 10–300. The camera auto-exposes.

## Answers (numbers from `analysis/p3-answers.json` and `analysis/q3-correction.json`)

### Colour — what the boards rendered (`analysis/fig-colour.png`, `analysis/colour.json`)
Space-time images of all 160 payload pixels per board from the wire (one shared display gain, γ 0.5). Lower board (Hybrid K1) circular-mean hue 356° / 359° / 357° (BOOM / Sonic / Dopex), concentration ≥ 0.976; gold only in BOOM around 13–20 s and 40–51 s. Upper board (Tempo, Crimson Gold) 352° / 20° / 333°. In BOOM the build before the quiet cut is wide and gold on both boards; at the drop both return to red/magenta and the upper board narrows. [FACT, visual + colour.json] The upper board's colour does change with the song; the lower board's essentially does not.

### Q1 — the drop (BOOM: build 45–50 s, quiet cut 50–55 s, drop 55.5–60 s)

Quiet cut to drop:

| Signal | Change |
|---|---|
| Received audio | +8.8 dB (7.6× power) |
| Device raw RMS | ×5.58 |
| AGC output (`peak_scaled`) | ×1.52 |
| Donor VU | ×3.93 |
| Wire brightness, lower board | ×1.39 |
| Wire brightness, upper board | ×1.31 |

Build to drop:

| Signal | Change |
|---|---|
| Received audio | +6.0 dB |
| Raw RMS | ×1.52 |
| AGC output | ×1.34 (0.664 → 0.889) |
| Wire brightness, lower board | ×0.63 |
| Wire brightness, upper board | ×0.24 |

**[FACT]** The AGC follower (`platform/ra8p1/inhouse_audio.cpp:226–233`, attack 0.25, release 0.005, `peak_scaled = drive/follower`) halves the loudness range in dB. Through the AGC, BOOM goes from 19.3 to 8.6 dB, Sonic from 6.7 to 3.9 dB, and Dopex from 9.0 to 4.8 dB.

**[FACT]** The packer is not clipping. The maximum code is 15,407 of 65,535, and no channel is ever at 65,535 (`analysis/packer-clipping.json`).

**[FACT]** Brightness does not follow the AGC output. Spearman ρ between wire brightness and `peak_scaled` during music:

| Song | Lower board | Upper board |
|---|---|---|
| BOOM | −0.01 | −0.06 |
| Sonic | −0.01 | 0.29 |
| Dopex | 0.12 | −0.05 |

Against device raw RMS, ρ runs from −0.18 to 0.51. *(Iteration 2: the `product_effect_renderer.cpp` citation is withdrawn; that file is not in this build. See the Renderer bullet above.)*

Donor VU, from the second analysis that also runs every hop, rose ×3.93 from quiet to drop. That is far closer to the audio than the in-house AGC.

**[NOT TRACED]** Which features each v8 effect reads frame by frame: the in-house AGC, the donor level, or something else.

**[INFERENCE]** Brightness is set mostly by something other than overall level, so loosening the AGC alone is unlikely to restore contrast between song sections.

### Q2 — silence (the seconds before and after each song)

Wire brightness, music median → silence mean:

| Song | Lower board | Upper board |
|---|---|---|
| BOOM | 4.10‰ → 0.52‰ (÷7.9) | 3.73‰ → 0.50‰ (÷7.5) |
| Dopex | ÷128 | ÷470 (effectively dark) |
| Sonic | 1.27‰ → 1.40‰ (×1.1, does not dim) | ÷12 |

The device's silence flag was set in 0% of silent samples for BOOM, 8% for Sonic and 48% for Dopex (5 s dwell).

The camera shows the opposite: its brightness rises in silence because auto-exposure compensates. It compresses 10–19 dB of rendered variation into about 0.6–2.8 dB. Don't judge brightness dynamics from these videos.

### Q3 — the beat (BOOM 55.5–75 s, about 138 BPM; light measured from the camera at 30 fps)

The music's onset envelope repeats in a 3+3+2 pattern at 138 BPM. Its autocorrelation is 0.58 at 3/8 of a bar (0.652 s), 0.49 at 5/8 (1.087 s) and 0.60 at one full bar (1.739 s). The light's |autocorrelation| from 0.4 to 1.2 s is at most 0.055 (upper) and 0.054 (lower).

Onset-following test: maximum Pearson r over 0–500 ms of light lag, against a null from 1,000 circular shifts of the audio (2 s to T−2 s):

| Board | Best r | Lag | Null p95 | p | Verdict |
|---|---|---|---|---|---|
| Lower | 0.185 | 267 ms | 0.156 | 0.020 | Weak onset-following |
| Upper (the Tempo effect) | 0.075 | 200 ms | 0.113 | 0.30 | No evidence |

Planted control: the onset envelope was mixed in at 0.5× the band's own standard deviation, delayed 100 ms. The test detected it on both boards (p 0.001), so it can see a real effect.

The 267 ms lag includes AV-alignment uncertainty. It is not a latency measurement; that is P5.

**CORRECTION.** The first Q3 answer is retired, not regenerated: the `q3` entries `null_p95` and `exceeds_null_fraction` in `p3-answers.json`. That draft compared each signal's autocorrelation against phase-randomised surrogates, which keep the power spectrum and therefore the autocorrelation, so the "chance level" tested nothing. The draft's statement that the light shows "no pulse at all" is also withdrawn; the lower board shows weak onset-following.

### Q4 — song versus song (Sonic 6–30 s; BOOM and Dopex 28–75 s)

| Measure | Sonic | BOOM | Dopex |
|---|---|---|---|
| Median received level (dBFS) | −55.1 | −56.8 | −55.3 |
| p10–p90 spread (dB) | 7.0 | 15.6 | 7.4 |
| Median AGC output | 0.775 | 0.765 | 0.736 |
| Lower board, median ‰ | 1.32 | 3.90 | 3.25 |
| Upper board, median ‰ | 9.76 | 2.49 | 2.59 |

In these recordings Sonic is not quieter at the median. After the AGC all three songs sit at the same level, yet the boards look very different from song to song, independent of loudness.

## What only the Captain can judge (agent cannot listen)

1. BOOM at 55.5 s: does the drop feel like it lands?
2. BOOM 55.5–75 s: is the lower board's flicker on the beat, or late?
3. Is the lower board glowing on after Sonic acceptable, or a fault?

## Proposal — not approved

Before changing the AGC, trace on the host which inputs move the v8 effects' brightness frame by frame, using the BOOM recording. This needs no hardware change. The AGC is not changed without Captain direction.

## Reproduce

```
/Users/spectrasynq/miniforge3/bin/python scripts/p3_expressiveness_audit.py --out <run>/permitted/analysis   # needs ffmpeg; writes p3-data.json, p3-answers.json
/Users/spectrasynq/miniforge3/bin/python scripts/p3_q3_alignment.py <run>/permitted/analysis                  # exits 1 if the planted control is missed
/Users/spectrasynq/miniforge3/bin/python scripts/p3_colour.py <run>/permitted/analysis
/Users/spectrasynq/miniforge3/bin/python scripts/p3_plots.py <run>/permitted/analysis
```

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (Titan lane owner) | Iteration 6: review-5 N1–N5 wording fixes, page v7. |
| 2026-10-14 | agent:claude (Titan lane owner) | Iteration 5: bar line on Hybrid K1 only, camera test reach, 67 ms caveat, timeline and tracker caveats, BOOM input committed, page v6. |
| 2026-10-14 | agent:claude (Titan lane owner) | Iteration 4: "bar, not beat" withdrawn; rate spectra + comb scan + planted check (frozen, fails at 20 Hz), loudness alignment, lede scoped to lower board, evidence committed, page v5. |
| 2026-10-14 | agent:claude (Titan lane owner) | Iteration 3: detrend fix, robust tempo, host-replay payload periodicity, Q2 medians committed, page v4. |
| 2026-10-14 | agent:claude (Titan lane owner) | Iteration 2: review corrections 1–14; host tempo replay; renderer corrected to InhouseWaveformRenderer; page v3. |
| 2026-10-14 | agent:claude (Titan lane owner) | Created; Q3 null retired and replaced; build-vs-drop and level-tracking added; colour space-time + donor VU + feature-trace gap added (page v2). |
