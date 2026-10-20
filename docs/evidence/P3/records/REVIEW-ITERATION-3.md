# P3 iteration 3 — independent review (fresh context) — VERDICT: ACCEPT AS PARTIAL

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was read-only and offline. It opened no serial port and used no bridge, broker, build, flash, deploy or launchctl. Host builds and runs happened only in the reviewer's scratch directory (`/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/review-p3c/`). This file is the only repository file written.

Reviewer: independent fresh-context reviewer (agent:claude, Reality Checker role), 14 Oct 2026.
Reviewed at `a3572ee`: `REVIEW-ITERATION-2.md`, `RECEIPT-ITERATION-3.md`, the iteration-3 block of `P3-RECORD.md`, `P3-CAPTAIN-PAGE.html` (published version 4, whose text matches the local source), `analysis/iteration3.json`, `q3-iteration3.json` and its log, `q3-iteration3-red-plant0.json` and its log, all ten `analysis/fig-*.png`, `scripts/p3_q3_alignment.py`, `p3_iteration3.py`, `p3_plots.py`, `p3_agc_trace.py`, `tests/host/test_p3_detrend.py`, `tests/host/p3_tempo_probe.cpp`, and `docs/evidence/P3/` (the CSVs, the meta file and the gate list in `agc-trace.html`).

## In plain English

Iteration 3 fixed what it was asked to fix:

- The smoothing bug is gone, and a test now guards it.
- The gate's control goes red when it should.
- The tempo statement now holds at every replay level.
- The silence and ceiling wording is right.
- The figures are cleaner.

I reproduced every number the receipt quotes. Most of them came out byte for byte.

**One headline is wrong.** The page's answer to the beat question is "Bar, not beat": the light repeats once per bar, but not once per beat. The lane's own host replay says otherwise. In the drop, the replayed output's strongest rhythm is the song's 138 BPM beat. On the Tempo board it carries **13–17 %** of the output's rhythmic power, against **3–5 %** at the bar rate.

The lane missed this for two reasons:

1. It read the self-similarity at exactly one beat. That one value is small, but the curve rises and falls once per beat all the way out to the bar.
2. Its "music onsets" signal is almost blind to the beat. That signal is built from rises in loudness, and only **0.2 %** of its power sits at the beat. The plain loudness of the same recording puts **15.6 %** there.

The camera tells a different story. The band near the lower edge carries the music's accent pattern (17 %), not the beat (3 %).

So the honest Q3 answer has three parts:

- **Host replay:** yes, the beat is there, plus a once-per-bar repeat.
- **Camera:** the beat is not visible.
- **Device payload:** its snapshots are too far apart to tell.

One smaller wording error: the lede says the dim drop is "mostly a colour change, not less light". That is true for the lower board only. On the upper board the light itself halves (×0.53).

Q1, Q2, Q4, the tempo section and corrections 1–9 and 11 are accepted. The Q3 periodicity answer (correction 10) is not. The page should not go to the Captain as the final P3 answer until it is fixed.

## Corrections 1–11 checked

| # | Review-2 ask | Verdict | Evidence (reviewer's own run) |
|---|---|---|---|
| 1 | Edge-padded detrend, new files, withdraw r 0.87 / "does not repeat" / r 0.18, regression test | **Done** | See detail 1 below. |
| 2 | Gate control on shifted light, red run on lower band | **Done** | See detail 2 below. |
| 3 | Robust tempo statement, gain sweep, [INFERENCE] mechanism, what tempo drives, effective gate | **Done** | See detail 3 below. |
| 4 | Renderer wording | **Done** | Page provenance and record now say "compiled but discarded at link … not on the mode-32/18 path". |
| 5 | Darkness gates | **Done** | See detail 5 below. |
| 6 | Ceiling | **Done** | `65535 × 0.25 × (0.75 + 0.25 tanh 1)` = 15,407.25. Hits: 4 of 178 (BOOM) and 1 of 183 (Dopex) lower-board snapshots, so 5 of 436 (`H`, reproduced). The upper ≈12,250 ceiling is kept as [INFERENCE]. |
| 7 | Follow-up consistency | **Done**, one lede overreach (finding F3) | Luma is named. The light × colour split matches `agc-trace.html` (lower ×0.93 · ×0.68; upper ×0.53 · ×0.46). The proposal separates beat from dim drop. |
| 8 | Time-series row | **Done** | See detail 8 below. |
| 9 | Provenance and numbers | **Done**, one wording issue (finding F6) | See detail 9 below. |
| 10 | Q3 for rendered brightness | **Not accepted** | The scope sentence is right. The conclusion drawn from the host data is wrong (finding F1). |
| 11 | Republish and render | **Done** | Reviewer render at 400 px light (scrollWidth 400), 1280 px dark and 1280 px light (scrollWidth 1280). No broken images. I Read the top of the 400 and 1280 captures. The published version-4 text matches the local source. |

**Detail 1 (detrend).**

- The pytest file gives 3 passed.
- I mutation-tested the production `detrend()` in scratch:
  - `mode="constant"`: 2 failed.
  - Back to `np.convolve(mode="same")`: 2 failed.
  - No detrend: 1 failed (the drift test).
  - `mode="reflect"`: 3 passed, which is correct, because reflect padding is also edge-safe.
- So the zero-pad red check is meaningful. The production test goes red on the real defect. The `test_zero_padded_form_is_red` case on its own only proves the oracle can fail.
- `q3-iteration3.json` and `q3-iteration3-red-plant0.json` regenerate **byte-identical**, with rc 0 and rc 1.

**Detail 2 (gate control).** I planted two logic faults in scratch:

- lag search reversed;
- lag window set to 0.

Both give "planted detected: False" on **both** bands and rc 1. So the control catches broken test logic, not only an absent plant.

**Detail 3 (tempo).** I rebuilt the probe in scratch and replayed at gains 0.7, 1.0 and 1.3. The shares near 138 in 55.5–75 s are 0 / 0 / 0.155, and the top shares match `F` exactly. From the committed CSVs I get 0.153 at 1.155 and 0 for the room recording. The figure plots the post-smoothstep gate.

**Detail 5 (darkness gates).** The first exact-zero lower payload after Dopex is at 76.30 s; the first `inhouse_silence` true is at 80.71 s, with every sample before it false. The admission thresholds 0.0048 / 0.0034 are confirmed in `platform/ra8p1/composed_scene_runtime.h` (`BloomAdmission`), wired by `palette_runtime.cpp:258–264`. The [INFERENCE] tag is correct. The gate list matches `agc-trace.html`.

**Detail 8 (time-series row).**

- The BOOM confidence panel shows the host confidence labelled HOST, with the gate band.
- The donor line is labelled "no v8 effect reads it".
- The caption lists the unrecorded features.
- The AGC lines are no longer clipped.
- The `fig-q4` title says "in-house AGC".

**Detail 9 (provenance and numbers).**

- The Q2 medians reproduce at both music windows. At 2 s … end − 0.5 s: ÷192.99 / ÷13.49, Sonic ÷33.30 / ÷1.143. At 0 … end: ÷191.7 / ÷13.34 / ÷32.66 / ÷1.144. The difference is stated, and the window is printed on the page.
- Dopex post-roll: 0.677 at 0.0177.
- Donor BPM: 139 / 103.

**Whole-file regeneration.** I ran `p3_iteration3.py` from scratch; only the work directory and root paths were redirected. It regenerates **`iteration3.json` byte-identical**.

## Numbers re-derived with independent code

My code used no lane functions: `scipy.ndimage.uniform_filter1d(mode="nearest")` for the detrend, a direct-sum ACF, my own lag search and my own 1,000-shift null with a different seed.

| Quantity | Lane | Reviewer |
|---|---|---|
| Camera band vs band, BOOM 55.5–75 s (detrended / raw) | 0.123 / 0.329 | 0.123 / 0.329 |
| Lower-edge band vs onsets: r, lag, p | 0.306, 267 ms, 0.001 | 0.306, 267 ms, 0.001 |
| Upper-edge band vs onsets: r, p | 0.063, 0.84 | 0.063, 0.84 |
| Camera ACF, lower band: one bar / one beat | 0.311 / −0.137 | 0.311 / −0.137 |
| Camera ACF, upper band: one bar / one beat / two beats | 0.026 / −0.312 / 0.133 | same |
| Host luma ACF at one bar (song L, song U, room L, room U) | 0.373 / 0.360 / 0.486 / 0.360 | same |
| Host luma ACF at one beat | 0.072 / 0.006 / 0.142 / 0.128 | same |
| Host luma vs onsets, r (p 0.001 each) | 0.278 / 0.521 / 0.304 / 0.386 | same |
| Tempo, drop share near 138 (song 1.155 / room) | 0.153 / 0.000 | 0.153 / 0.000 |
| Q2 medians | ÷193 / ÷13.5 / ÷33 / ÷1.14 / Dopex 0 | same at the lane's window |
| Dopex dark / silence flag | 76.3 s / 80.7 s | 76.30 s / 80.71 s |

The lane's arithmetic is right. The finding below is about what the numbers mean.

## Findings

### F1. The host-replay output carries the 138 BPM beat; "Bar, not beat" is not supported (material) [FACT, host replay]

The lane reads the luma autocorrelation only at lag 0.435 s, where it is 0.01–0.14, and concludes "almost nothing at one beat". The whole curve tells a different story. Data: `docs/evidence/P3/agc-trace-boom-{source,recording}.csv`, BOOM 55.5–75 s, 1 s edge-padded detrend.

**ACF at whole beats against half beats.** The ACF peaks at every whole multiple of the beat and dips between them.

| Host luma | ACF at 1, 2, 3, 4, 5 beats | ACF at ½, 1½, 2½, 3½, 4½ beats |
|---|---|---|
| Room, upper (Tempo) | 0.13, 0.29, 0.35, 0.36, 0.34 | −0.33, −0.15, −0.17, −0.27, −0.13 |
| Room, lower | 0.14, 0.13, 0.19, 0.49, 0.20 | −0.19, −0.02, −0.03, −0.11, −0.07 |
| Song, upper | 0.01, 0.12, 0.27, 0.36, 0.22 | −0.32, −0.01, −0.08, −0.32, −0.04 |

The lane's own `fig-q3.png` left panel shows this: the red and teal host curves have local peaks near 0.43, 0.87, 1.30 and 1.74 s.

**Trial-period scan.** For each trial period P from 0.30 to 0.66 s, I took the mean ACF at kP minus the mean at (k+½)P, for k = 1–4. P = 0.435 s is the best of 72 trial periods for:

- room upper (1 period scores higher);
- room lower (0 higher);
- song upper (0 higher).

Song lower is 4/72.

**Spectrum.** I used Welch with 8 s segments over 1.6–3.0 Hz. The 2.28–2.31 Hz line is the **largest bin in the band for all four host series**. This holds with no detrend, a 1 s detrend and a 2 s detrend.

**Share of rhythmic power.** Each cell is the percentage of 0.3–5 Hz power within ±0.06 Hz of the rate.

| Series (BOOM 55.5–75 s) | Bar 0.575 Hz | Half-time 1.15 Hz | Accent 1.725 Hz | **Beat 2.30 Hz** |
|---|---|---|---|---|
| Room audio, linear RMS envelope | 1.6 | 2.1 | 8.2 | **15.6** |
| Lane's "music onsets" (rectified dB rise, 20 Hz) | 0.2 | 1.2 | 10.1 | **0.2** |
| Host luma, room, upper (Tempo) | 3.8 | 5.0 | 6.4 | **16.6** |
| Host luma, room, lower (Hybrid K1) | 5.4 | 7.0 | 7.1 | **13.1** |
| Host luma, song, upper | 3.2 | 5.0 | 7.4 | **13.2** |
| Host luma, song, lower | 2.8 | 6.1 | 3.7 | 5.1 |
| Camera, lower-edge band | 6.9 | 6.6 | **17.3** | 3.1 |
| Camera, upper-edge band | 2.3 | **12.1** | 5.0 | 1.7 |

**Why the one-bar ACF is high.** The bar lag (1.74 s) is where every periodic component lines up: 4 beats, 2 half-time cycles, and one full 3+3+2 accent cycle. A high ACF there does not show that the light pulses at bar rate rather than beat rate. The bar rate itself carries only 3–5 % of the power.

**The input carries the beat too.** The host `raw_rms` (device-calibrated input level) also has its band maximum at 2.31 Hz.

**Consequences:**

- These must be withdrawn or restated on the Q3 pill, lede, Q3 row, stat tiles, `fig-q3.png` title, record and receipt:
  - "Bar, not beat";
  - "repeats once per bar, but not once per beat";
  - "The light pulses per bar, not per beat";
  - "with almost nothing at one beat".
- The answer the evidence supports:
  - **Host replay:** the output carries the 138 BPM beat. It is the strongest of the four rates on the Tempo board and on the lower board's room replay. It also repeats once per bar.
  - **Camera:** the beat is not detected. The lower-edge band carries the accent rate, and the upper-edge band carries the half-time rate.
  - **Device payload:** unresolvable, because the snapshots are 0.3–0.5 s apart.
- Whether the host beat survives emission, the plate and the 24/255 cap is not shown. The camera not seeing it is evidence in that direction, but auto-exposure and the 30 fps camera limit what it can show. That part is [INFERENCE].

### F2. The music reference used for Q3 is nearly beat-blind (moderate) [FACT]

`p3_q3_alignment.py` builds "music onsets" as `max(0, diff(rms_dbfs))` at 20 Hz. As the table above shows, it keeps the accent rate and almost removes the beat: 0.2 % at 2.30 Hz, against 15.6 % for the linear RMS of the same audio.

Both the ACF comparison panel ("music onsets (received audio)") and the onset-alignment test use this signal as the music. Fixes:

- Label it as an accent-weighted rise envelope.
- Add the linear RMS envelope as the music's beat reference in the ACF panel.
- Re-run the alignment test with it, or say why not.

### F3. Lede overstates the colour explanation (moderate) [FACT]

The lede says: "That is mostly a colour change from gold to red, not less light." The split the page itself quotes is:

- **Lower board:** light ×0.93 × colour ×0.68. This is mostly colour, so the statement holds.
- **Upper board:** light ×0.53 × colour ×0.46. Light and colour contribute about equally.

Scope the sentence to the lower board, or say "on the lower board mostly colour; on the upper board, less light and redder colour in about equal parts".

### F4. The onset alignment is not directional (minor) [FACT]

I reversed the lag search, so that light leads the music by 0–500 ms. The lower-edge camera band still correlates: r 0.297 at 367 ms lead, p 0.001.

The reference is strongly periodic, so a 267 ms lag one way and a 367 ms lead the other way score alike. The host-luma best lags (0, 133, 300 and 433 ms; 433 ms is one beat) show the same ambiguity.

"Follows the onsets" is acceptable as a shared-rhythm correlation. Add one line: this is shared periodicity, not a measured response delay. The page already says it is not a latency.

### F5. The camera's upper-edge band is described as "shows nothing" (minor) [FACT]

The record says "The upper-edge band shows nothing." It does not follow onsets (p 0.84). It does have structure:

- ACF −0.31 at one beat (7th percentile) and +0.13 at two beats (95th);
- 12.1 % of its rhythmic power at 1.15 Hz, the half-time rate.

That is consistent with the Tempo tracker's 69 BPM half-time, but camera bands are not attributable to a board, so this is [INFERENCE] at most. Report the observation; do not interpret it as board behaviour.

### F6. "Committed evidence" is inaccurate (minor) [FACT]

`git status` shows `?? _scratch/`, and `git ls-files _scratch` lists 0 files. `iteration3.json`, `q3-iteration3*.json` and the run directory are untracked. Only the scripts (`a3572ee`) and `docs/evidence/P3/` (`7422b23`) are committed.

The JSON regenerates byte-identical from the committed script, which I checked, but only from untracked inputs. Either:

- commit the iteration-3 outputs to `docs/evidence/P3/`; or
- reword the receipt to "produced by committed `scripts/p3_iteration3.py`; output in the untracked run directory".

### F7. The percentile among lags is a descriptive rank and is not labelled as one (minor) [FACT]

`percentile_among_lags_0p3_2p2` ranks one ACF value against the same ACF at lags 0.3–2.2 s. It is not a null, for three reasons:

- Neighbouring lags are strongly dependent.
- The 1 s moving-mean detrend biases short lags (below 1 s) slightly negative compared with long ones.
- As F1 shows, the one-bar lag is where every periodic component coincides, so a high rank there is nearly guaranteed for any rhythmic signal.

The page shows "99th percentile of lags" in a stat tile without saying it is descriptive. Fixes:

- Label it "descriptive rank among lags of the same curve, not a significance test", or drop it from the tiles.
- Use a measure that can tell rates apart: the trial-period scan or the rate-share spectrum. Give it a reference, such as other trial periods, other song windows, or a planted synthetic signal.

### F8. Figure defects (minor) [FACT, Read]

- **`fig-q3.png` left title:** "The light repeats once per bar, not once per beat" (F1).
- **`fig-q3.png` legend label:** "music onsets (received audio)" (F2).
- **`fig-q1.png` bottom panel:** the legend box ("lower board (Hybrid K1) / upper board …") covers the upper board's build peak at 48–48.5 s, about 15 ‰. That peak is the point of Q1's build-against-drop comparison. Move the legend outside the axes.
- **`fig-tempo.png`:** the annotation "69 = half-time · 92 = 2/3 of 138" sits on the room-recording trace near 0–2 s. Move it into empty space, for example the 15–30 s band at about 100 BPM.
- **Page text:** "at most 15 %" should read "15–16 %" (gains 1.155 / 1.3 / 1.6 give 15.3 / 15.5 / 15.7 %).

All other PNGs (colour, q2, q4, stages and the three time series) were Read. Their text is not clipped, no legend sits on data, and the AGC lines stay inside the axes.

## What is accepted as-is

- **Q1.** The ratios, the luma metric, the light × colour split and the located flattening stage (effect mapping, cited to the trace). The lede needs the F3 fix.
- **Q2.** The medians, Dopex 76.3 s against 80.7 s, the [INFERENCE] admission-gate cause, and the Sonic post-roll as a room-sensitivity question.
- **Q4.** The AGC medians, the payload medians and the ceiling occupancy.
- **Tempo section.** "Verse 69 (half-time); about 138 at 33–43 s; drop onset about 112, gate nearly closed; drop mostly 92 or 69", with the per-input shares. I verified "mostly 92 or 69": 92 and 69 together are 70–93 % of the drop in every replay. Also accepted: the [INFERENCE] tag, the octave-equivalence note and the travel-speed citation.
- **Gate machinery.** The detrend fix, its regression test, the shifted-light control and the recorded red run. All are mutation-confirmed.
- **Renderer wording, ceiling and provenance**, except for F6.

## Required corrections (iteration 4):

1. **Q3 periodicity (F1, F7).**
   - Withdraw "Bar, not beat", "repeats once per bar but not once per beat", "the light pulses per bar, not per beat" and "almost nothing at one beat". These appear on the page (pill, lede, Q3 row, stat tiles), in the `fig-q3.png` title, the record and the receipt.
   - Write a new `analysis/iteration4.json`; do not regenerate the iteration-1/2/3 files. It must hold, for host luma (both boards, both inputs), both camera bands and the room audio:
     - a rate-share or spectral measure at the bar, half-time, accent and beat rates;
     - a trial-period comb scan.
   - Add a planted check that must go red. A synthetic signal with a known 2.30 Hz component plus a 3+3+2 accent pattern must be found at the beat. The same signal without the 2.30 Hz component must not be.
   - Restate Q3 in three parts:
     - host replay carries the beat (strongest rate on the Tempo board) as well as a bar repeat;
     - the camera does not show the beat (lower-edge band at the accent rate, upper-edge band at half-time);
     - the device payload cannot resolve it.
   - Keep "whether the beat survives to the plate" as an open, [INFERENCE]-tagged question.
   - Label or drop "percentile among lags" as a descriptive rank.
2. **Music reference (F2).**
   - Rename the onset envelope as an accent-weighted rise envelope.
   - Add the linear RMS envelope as the beat reference in the Q3 ACF panel.
   - Re-run the onset-alignment test with it, or state why not.
3. **Lede (F3).** Scope "mostly a colour change, not less light" to the lower board. State that the upper board's light halves (×0.53) as well as reddening (×0.46).
4. **Direction (F4).** Next to "follows the onsets", add that the same correlation appears with the light leading by 367 ms (p 0.001). It is shared rhythm, not a response delay.
5. **Upper-edge band (F5).** Replace "shows nothing" with what was measured: no onset following, ACF −0.31 at one beat, 12 % of power at the half-time rate. Make no board attribution.
6. **Evidence storage (F6).** Commit the iteration-3/4 outputs to `docs/evidence/P3/`, or reword "committed evidence" to "produced by a committed script; stored in the untracked run directory".
7. **Figures (F8).**
   - Fix the `fig-q3` title and its legend label.
   - Move the `fig-q1` bottom legend off the 48 s build peak.
   - Move the `fig-tempo` annotation off the trace.
   - Change "at most 15 %" to "15–16 %".
   - Re-Read every regenerated PNG.
8. **Republish.**
   - Re-render at 1280 px (light and dark) and 400 px, and Read the captures.
   - Republish to the same URL as version 5.
   - Until then, version 4 should not be presented to the Captain as the final P3 answer.
