# P3 iteration 1 — independent review (fresh context) — VERDICT: ACCEPT AS PARTIAL

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was read-only and offline. It opened no serial port and used no bridge, broker, build, flash or deploy. The only file written is this one; scratch computations ran in memory.

Reviewer: independent fresh-context reviewer (agent:claude, Reality Checker role), 14 Oct 2026.
Reviewed: `RECEIPT.md`, `P3-RECORD.md`, `P3-CAPTAIN-PAGE.html` (sha256 `42469409…b634c9`, matches the receipt), all seven PNGs, `analysis/*.json`, the four `scripts/p3_*.py` at `2442955`, raw `sample-*` snapshots of all three recordings, `platform/ra8p1/inhouse_audio.cpp:210–240` and `src/k1/core/visual/product_effect_renderer.cpp`.

## In plain English

The arithmetic is right. Every Q1, Q2 and Q4 number and the Spearman table reproduce exactly from my own code. The wire decoding is right: all 872 snapshots match the device's own `submitted_rgb16` and `submitted_grb48_hex` byte for byte. Retiring the old Q3 "chance level" was correct. The replacement onset test also holds up under my own power check and false-positive check.

The page cannot go to the Captain as it stands, for four reasons:

1. **It names boards it cannot separate.** The Q3 "lower board follows the onsets; the upper board (Tempo) does not" comes from two camera bands on one shared light-guide plate. Those bands move together (r 0.87 after detrending). The "lower" band also follows the *upper* board's wire better than the lower board's.
2. **It asks the Captain about a fault that is probably not one.** The "silence" after Sonic was not silent at the device. Its microphone level rose to 0.161, above Sonic's own music median, and the lower board's glow coincides with those bursts.
3. **It skips evidence already on disk.** The snapshots carry the donor tempo, tempo confidence and beat-lock. The Tempo effect's beat motion is gated by `smoothstep(0.30, 0.60, confidence)`. In the BOOM drop the recorded confidence never exceeds 0.158 and beat-lock is never set. That is a concrete candidate for "why the Tempo board ignores the beat", and it went unreported.
4. **It plots only part of what the brief asks for.** Time plots of audio RMS and AGC state exist only for BOOM 44–62 s, not for BOOM 28–75, Sonic 6–30 or Dopex 0–75.

Several smaller wording, citation and figure defects follow.

## What I recomputed (own code, from `analysis/p3-data.json` unless stated)

### Q1 (BOOM, quiet 50–55, drop 55.5–60, build 45–50): all match the page and record

| Signal | Quiet → drop | Build → drop | n (quiet/drop/build) |
|---|---|---|---|
| iPhone audio power | ×7.63, +8.82 dB | +5.98 dB | — |
| `inhouse_raw_rms` | ×5.577 | ×1.525 | 12/9/9 |
| `inhouse_peak_scaled` | ×1.517 | ×1.338 (0.664 → 0.889) | 12/9/9 |
| `donor_vu` | ×3.932 | ×1.669 | 12/9/9 |
| wire lower `w0` | ×1.388 | ×0.627 | 11/9/10 |
| wire upper `w1` | ×1.308 | ×0.242 | 11/9/10 |

These rest on 9–12 snapshots per window. The record should state the n.

### Q2 (music median vs silence mean): match

BOOM ÷7.9 / ÷7.5; Dopex ÷128 / ÷470; Sonic lower ×1.1, upper ÷11.9. The silence windows hold 4 pre-roll and 21–23 post-roll samples. The comparison is a mean against a median; use the same statistic for both.

### Q4 (Sonic 0.5–23.5 fixture-relative; BOOM and Dopex 28–75): match

Audio median −55.1 / −56.8 / −55.3 dBFS. AGC median 0.775 / 0.765 / 0.736. Lower 1.32 / 3.90 / 3.25 ‰. Upper 9.76 / 2.49 / 2.59 ‰.

### Spearman ρ, wire brightness vs device features, music 0…fixture: match, with additions the record omits

| Song, board | ρ AGC (p) | ρ raw RMS (p) | ρ donor VU (p) |
|---|---|---|---|
| BOOM lower | −0.013 (0.87) | **0.216 (0.007)** | 0.045 (0.59) |
| BOOM upper | −0.061 (0.46) | 0.069 (0.40) | 0.026 (0.75) |
| Sonic lower | −0.014 (0.92) | −0.178 (0.22) | −0.050 (0.73) |
| Sonic upper | 0.290 (0.04) | **0.514 (<0.001)** | 0.065 (0.66) |
| Dopex lower | 0.116 (0.15) | **0.361 (<0.001)** | −0.135 (0.10) |
| Dopex upper | −0.055 (0.50) | **0.362 (<0.001)** | **−0.394 (<0.001)** |

### Stage p90/p10: match `p3-answers.json` `stage_dynamic_range_db_p90_over_p10`

### Q3: reproduced independently, with a different construction

I averaged the camera into the 50 ms audio bins at 20 Hz instead of interpolating the envelope to 30 fps, used a separate RNG, and kept lag selection inside the null.

- Lower band: r 0.157, lag 0.25 s, null p95 0.139, p 0.027 (lane: 0.185 / 0.267 / 0.156 / 0.020).
- Upper band: r 0.069, p 0.29 (lane: 0.075, p 0.30).
- Power, planting the onset envelope into a decorrelated (circularly shifted) light trace at a random 0–350 ms lag: 4/20 detected at 0.1× std, 20/20 at 0.2×, 0.3× and 0.5×.
- False-positive rate on shifted light: 1/40.
- Mismatched-section negative control (audio 30–49.5 s against light 55.5–75 s): r 0.037, p 0.54.

The test is sensitive enough and is not biased upward. The lane's own planted control at 0.5× std (planted r 0.53–0.59) is about three times the observed effect, so on its own it does not show sensitivity at the observed size.

## Findings

### F1. Q3 board attribution is unsupported (material) [FACT]

`p3_expressiveness_audit.py:147` takes horizontal row bands 48–57 and 76–85 across a plate that is tilted in frame. In `mean-frames.png` the bottom rail sits near row 97 at the left and near row 85 at the right. The "lower band" is therefore mid-plate on the left and on the rail on the right. Both strips feed one light guide.

Measured on BOOM 55.5–75 s:

- The two camera bands correlate r 0.866 after the 1 s detrend that Q3 uses (raw r 0.33).
- Over 0–75 s at the wire snapshot times, the camera lower band follows the **upper** wire with ρ 0.540 and the lower wire with ρ 0.343. The camera upper band follows the upper wire with ρ 0.491 and the lower wire with ρ 0.330.
- The two wires themselves correlate ρ 0.666.

So "lower board follows the onsets weakly; upper board (the Tempo effect) shows no evidence" (page Q3 card, record table at `P3-RECORD.md:97–100`, `fig-q3.png` legends) cannot be credited to a board. Say instead: "the camera band nearer the lower edge correlates weakly with onsets; the camera cannot separate the boards." The second Captain question ("is the lower board's flicker on the beat") inherits the same problem.

### F2. Sonic post-roll is not silent at the device (material) [FACT]

Inside the Q2 silence window the device saw sound the iPhone barely registered:

| t (s) | `inhouse_raw_rms` | `inhouse_peak_scaled` | `donor_vu` |
|---|---|---|---|
| 26.8 | 0.054 | 0.81 | — |
| 27.2 | 0.096 | 0.75 | — |
| 31.6 | 0.097 | — | — |
| 32.1 | **0.161** | 0.93 | 0.62 |

For comparison, Sonic's music median raw RMS is 0.129. The iPhone stayed near −61 to −65 dBFS across these moments.

The lower board's wire rises to 4.4–6.3 ‰ at 26.8–27.6 s and 1–3 ‰ through 28–32.8 s, matching those bursts. The silence mean of device raw RMS is 0.042 after Sonic, against 0.025 after BOOM and 0.019 after Dopex (`p3-answers.json q2.*.inhouse_raw_rms.silence_mean`). "The lower board does not dim after Sonic" and Captain question 3 ("acceptable, or a fault?") are therefore mis-posed. The likely answer is "it responded to sound the device heard". Re-state the finding and the question, or drop Sonic's post-roll from the silence test.

### F3. Evidence for "which features each effect reads" is on disk and was not used (material) [FACT + INFERENCE]

Each `mir-snapshot.json` carries `body.donor_dominant_tempo_bpm`, `body.donor_tempo_confidence` and `body.beat_locked`. Over all three windows (BOOM 55.5–75, Sonic, Dopex 28–75), `beat_locked` is never true:

| Window | Donor BPM (mode) | Confidence median | p90 | max |
|---|---|---|---|---|
| BOOM 55.5–75 | 102–106 (true tempo about 138) | 0.091 | 0.146 | 0.158 |
| Sonic | — | 0.054 | — | 0.119 |
| Dopex 28–75 | — | 0.087 | — | 0.113 |

`renderWaveformTempo` (`product_effect_renderer.cpp:1322`, velocity via `tempoVelocityPixelsPerSecond` at 1304/1339) gates its beat motion with `smoothstep(kTempoConfidenceLow 0.30, kTempoConfidenceHigh 0.60, confidence)` (lines 94–95). [INFERENCE: if `visual.tempo` is the donor tempo, the gate stays at 0 for the whole recording, so the Tempo board runs at idle velocity. The mapping is not yet traced.]

This is the most direct candidate explanation in the data for "the Tempo board ignores the beat", and it outranks the AGC story for that board. Separately, the renderers read more than `peak_scaled`:

- `renderWaveformK1` (797–804) reads `max(visual.waveform.peak_scaled, focusedAudio().peak_scaled)`, the un-AGC'd `vu_level`, and the silence event flag.
- `renderWaveformTempo` (1326–1336) reads `max(peak, vu_level)`, `spectral_energy`, `novelty` and `tempo`.

In this data `inhouse_vu == inhouse_raw_rms`, an un-normalised level. That fits brightness tracking raw RMS (ρ 0.22–0.51, significant in 4 of 6 pairs) better than the AGC output. The record's [FACT] "The effects do read `peak_scaled`" (`P3-RECORD.md:69`) and the page labels "AGC output the effects read" (Q1 card, `fig-q4.png` middle panel title, `fig-stages.png` x-label) overstate a single input. The [INFERENCE] "brightness is set mostly by something other than overall level" (`P3-RECORD.md:75`) conflicts with the raw-RMS correlations. Revise it to: "brightness follows pre-AGC level weakly and the AGC output not at all."

### F4. Wrong consumer citation [FACT]

`product_effect_renderer.cpp:746–752` is inside `renderWaveformFamily` (line 722), which dispatch (2094–2099) uses only for modes 7, 8 and 11. Neither board's mode uses it: mode 32 → `renderWaveformK1` (791), mode 18 → `renderWaveformTempo` (1322). Remove 746 from the record (line 69), the page provenance and the receipt.

### F5. The brief's time-series coverage is partial [FACT]

The brief asks to "plot, against time, the received-audio RMS, the consumed level/AGC state, the features each Waveform effect actually reads, and the rendered per-board mean brightness and colour" for BOOM 28–75, Sonic 6–30 and Dopex 0–75.

- Brightness and colour against time: plotted for all three (`fig-q2.png`, `fig-colour.png`). Accepted.
- Audio RMS and AGC/level state against time: plotted only for BOOM 44–62 (`fig-q1.png`; `p3_plots.py:45,50,56`). Missing for BOOM 28–75, Sonic 6–30 and Dopex 0–75.
- Features: `donor_vu` appears only as a number. Donor tempo, confidence and beat-lock are not plotted (F3).

The "NOT TRACED" disclosure is honest about frame-by-frame consumption. It does not cover the omission of features that were recorded.

### F6. The 267 ms lag rationale is wrong, and the lag is reported too precisely [FACT]

Q3 uses the iPhone audio and iPhone video from the **same** `original.mov`. `p3_expressiveness_audit.py:116` and `:150` apply the same `(off, scale)` to both, so AV-map uncertainty cancels. The page caveat "includes recording alignment" and the record (`P3-RECORD.md:104`) give the wrong reason. The real limits are:

- the 50 ms envelope bins;
- a broad correlation plateau (`q3-correction.json light_lower_band.curve`: r 0.151–0.185 from 100 to 267 ms, −0.13 at 0 ms);
- camera exposure;
- the iPhone's capture A/V offset.

Report the lag as roughly 0.1–0.27 s with no claimed precision, and keep "not a latency measurement".

### F7. Mixed dB conventions; contradictory camera numbers [FACT]

`dynamic_range_db` (`p3_expressiveness_audit.py:181`) applies 20·log10 to wire luma, which is linear light intensity, and to camera grey levels, which are gamma-encoded. Audio RMS is an amplitude, so 20·log10 is correct there.

Under 10·log10, the wire "10–19 dB" becomes about 5–9.5 dB. The page also quotes the camera compression two ways: "into about 3 dB" in the camera section and "0.6–2.8 dB" in the stages caption and the record. State one convention per quantity and fix the numbers.

`fig-stages.png` joins "wire lower → wire upper → camera" with lines, as if they were successive pipeline stages. They are parallel outputs, so draw them as separate markers.

### F8. Q1 figure edge artefact [FACT]

`p3_plots.py:33–34,46` smooths dBFS values with `np.convolve(..., mode="same")`. The zero padding is near 0 dBFS, so it produces false spikes at the window edges: about −34 dBFS at 44 s and −30 dBFS at 62 s in the top panel of `fig-q1.png`. Pad by edge values, or smooth in the power domain.

### F9. Planted control is weak and its red path is unproven [FACT]

The `p3_q3_alignment.py:86` control plants at 0.5× std (planted r 0.53–0.59), against an observed effect of 0.18. It passes, but the binding rule needs every new gate to go red under a planted fault. `run.log` and the receipt show no run where the gate exited 1, for example with the plant removed, inverted, or the detector broken.

Add the power curve (0.1/0.2/0.3× std), the negative control (shifted light or mismatched section) and one demonstrated exit-1 run, and record them. My numbers above show these will pass.

### F10. Packer and "rendered" claims are mis-scoped [FACT]

`packer-clipping.json` tests only 65,535. Every snapshot's own metadata says `score.evidence_level = "ON_TARGET_SUBMISSION_BUFFER_NOT_GPIO_OR_PHOTONS"`, `submission_accepted = "UNKNOWN"` and `score.brightness = 255`. The 24/255 cap and anything after the submission buffer are not covered.

There is also no test for a ceiling below full scale. Tempo's highest codes cluster at 12,126 / 12,252 / 10,186 across the three songs. In Sonic, the upper board's per-snapshot maximum is within 5 % of its ceiling in 51 % of snapshots, against 1–11 % elsewhere.

Call the data "submitted LED payload (pre-emission buffer)", not "rendered/light on the wire". Add a check for an effect or palette ceiling below full scale.

### F11. Page and record inconsistencies and overstatements [FACT]

- Page lead: "Two different songs reach the effects at the same level". The Q4 card says "All three". The record says "Three songs".
- Q2 caption: "its picture rises exactly when the light falls" and "the phone video shows the opposite of all this". That holds at the silence boundaries only. During music the camera bands co-vary *positively* with the wire (ρ 0.33–0.54, F1). Narrow the claim to transitions into and out of silence.
- Q1 card: donor VU "tracks the drop far better". True for the audio, but donor VU does not track brightness (table above; Dopex upper ρ −0.39). Say so, so the reader does not infer that switching to donor VU would fix brightness.
- `colour.json` hue is the circular mean of each snapshot's mean colour (`p3_colour.py:63–64`), not of pixels. State this, because within-frame red/gold mixes average to orange.
- `RECEIPT.md:5` says "the P3 deliverable is complete". Not true given F1–F5.

### F12. Stage-flattening conclusion: right in direction, weak in metric [FACT + INFERENCE]

The p90/p10 of per-hop raw RMS (a 96-sample hop at 12.8 kHz, about 7.5 ms, sampled every 0.3–0.5 s) is compared with the smoothed `peak_scaled`. Part of the "halving" is smoothing, not gain normalisation.

Better evidence is already present and should replace or lead:

- **Section means.** Quiet → drop: raw ×5.58, AGC ×1.52.
- **AGC in true silence.** After Dopex (76–84 s), `peak_scaled` sits at 0.61–0.82 while raw RMS is 0.015–0.018. That equals the music median of 0.736, so the AGC lifts room noise to music level. The boards stay dark only because of other gates (the silence flag, `present`, raw-margin).

The AGC conclusion is otherwise scoped appropriately: [FACT] compression, plus [INFERENCE] that an AGC-only fix is unlikely to suffice. Keep that scope.

## What is accepted as-is

- **Numbers.** Q1, Q2 and Q4 numbers, the Spearman table and the stage table reproduce exactly.
- **Decoding.** The GRB48 decode (bytes 544–1504, big-endian, G,R,B, 160 px) matches the device's own `submitted_rgb16` and `submitted_grb48_hex` in 872 of 872 snapshots. No channel ever reaches 65,535.
- **Hues.** They are plausible against the mean RGB, for example BOOM lower [15.6, 1.1, 2.0] ‰ → about 356°, and Sonic upper [17.7, 7.5, 1.8] ‰ → about 21°.
- **Q3 null.** Retiring the phase-randomised null is correct reasoning: those surrogates preserve the power spectrum and therefore the autocorrelation. The original JSON is kept unaltered, not regenerated.
- **Q3 replacement.** The circular-shift test is valid. Lag search is inside the null. With two bands, a Bonferroni-adjusted p of 0.04 is still under 0.05. Note that the periodic music makes the circular-shift null conservative: the planted null p95 rose from 0.156 to 0.336.
- **Rhythm reading.** "3+3+2 at 138 BPM" is a reasonable [INFERENCE]. The ACF is 0.58 at 0.65 s, 0.49 at 1.10 s and 0.60 at 1.74 s, and only 0.06 at 4/8 (0.87 s). At 50 ms resolution this gives 138 ± about 5 BPM. Note that 138 was set a priori (`acf_peak target`), and that the donor estimated about 102–106.
- **Scope wording.** Build `9ab90873` is distinguished from the installed `a8215f75`, and equivalence is not claimed. The page carries "Agent cannot listen", the auto-exposure caveat, a "Proposal — not approved" label, and no external-hardware proposal. The windows are honoured, and the one deviation (Q4 Dopex 28–75) is disclosed.
- **Figures.** `fig-colour.png` is strong, honest imagery. `fig-q3.png`, `fig-q4.png` and `fig-q2.png` are legible, with no colliding or cut-off text. The specific defects are F7 and F8.

## Required corrections (iteration 2):

1. **Q3 attribution (F1).** Relabel the Q3 light series everywhere (page card, record table, `fig-q3.png` legends and title, Captain question 2) as camera bands near the upper and lower edge of a shared plate. Add the measured crosstalk (detrended band r 0.87; lower band against wires ρ 0.54 upper vs 0.34 lower). Withdraw "upper board (the Tempo effect) shows no evidence" as a board-level claim.
2. **Sonic silence (F2).** Re-state the Sonic Q2 result. The device heard sound in the post-roll: raw RMS up to 0.161 at 32.1 s, bursts at 26.8–27.2 s. Show it on the Q2 plot by adding device raw RMS. Replace Captain question 3, or exclude Sonic's post-roll from the silence test.
3. **Recorded features (F3).** Plot and report donor tempo BPM, tempo confidence and beat-lock for all three windows. Quote the Tempo renderer's 0.30–0.60 confidence gate. Label the gate-at-zero reading [INFERENCE] until `visual.tempo` is traced to the donor.
4. **Renderer inputs (F3).** List every input the two renderers read (K1: max of two `peak_scaled`, `vu_level`, silence flag; Tempo: `max(peak, vu_level)`, `spectral_energy`, `novelty`, `tempo`). Replace "AGC output the effects read" with "in-house AGC output (one of the inputs)". Report the raw-RMS ρ with p-values. Revise the [INFERENCE] to "brightness weakly follows pre-AGC level, not the AGC output".
5. **Citation (F4).** Remove the line-746 citation (`renderWaveformFamily`, modes 7/8/11). Cite 791/797–804 and 1322/1326–1339.
6. **Time-series row (F5).** Add per-song time plots for BOOM 28–75, Sonic 6–30 and Dopex 0–75 of iPhone audio RMS, device raw RMS, `peak_scaled`, donor VU, donor tempo confidence, and both boards' wire brightness. Put them in one imagery row, so the brief's "plot against time" is met for every window.
7. **Lag (F6).** Correct the lag caveat: the AV map cancels inside one `.mov`. Report the lag as a range of about 0.1–0.27 s, set by the broad r plateau and the 50 ms bins.
8. **Units (F7).** Fix dB conventions: 10·log10 for linear intensity, and state the camera's gamma encoding. Make the camera-compression figure one consistent number across page, record and caption. Redraw `fig-stages.png` without lines joining parallel outputs.
9. **Q1 figure (F8).** Remove the zero-padding edge spikes in `fig-q1.png`. Re-Read the regenerated PNG.
10. **Gate (F9).** Extend the gate with a power curve (0.1/0.2/0.3× std), a negative control (shifted or mismatched-section light, which must not be detected), and one recorded run that exits 1 under a planted fault. Commit the evidence. Do not regenerate the iteration-1 JSON; add a new file.
11. **Scope words (F10).** Call the wire data "submitted LED payload (pre-emission buffer, not photons)", citing the snapshot's own `evidence_level`. Note that the 24/255 cap and post-buffer stages are not covered. Add a check for an effect or palette ceiling below full scale, including Sonic's upper board, which sits within 5 % of its maximum in 51 % of snapshots.
12. **Text fixes (F11).** Fix "Two different songs" to "three". Narrow the camera "opposite / exactly" wording to silence transitions. Add donor-VU-vs-brightness ρ beside the "tracks the drop far better" sentence. Name the hue method.
13. **Stage evidence (F12).** Lead the stage-flattening evidence with section means and the AGC-in-silence observation (Dopex post-roll `peak_scaled` 0.61–0.82 at raw RMS about 0.016). Show the p90/p10 chart only as supporting evidence, with its smoothing caveat.
14. **Receipt.** Change "deliverable complete" to "submitted for iteration 2". List the n per Q1 window (9–12 snapshots). Re-render the page headless at 1280 px (light and dark) and 400 px, Read every regenerated figure, and republish to the same published-page URL.
