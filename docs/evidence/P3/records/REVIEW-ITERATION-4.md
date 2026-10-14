# P3 iteration 4 — independent review (fresh context) — VERDICT: ACCEPT AS PARTIAL

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was offline and read-only. It opened no serial port and used no bridge, broker, device build, flash, deploy or launchctl. All computation ran in the reviewer's scratch directory (`/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/review-p3d/`). This file is the only repository file written. The agent cannot listen; nothing here is a claim about how the music sounds.

Reviewer: independent fresh-context reviewer (agent:claude, Reality Checker role), 14 Oct 2026.
Reviewed at `c61991e` (branch `lane/titan-lagfree-20261014`): `REVIEW-ITERATION-3.md`, `RECEIPT-ITERATION-4.md`, the iteration-4 block of `P3-RECORD.md`, `P3-CAPTAIN-PAGE.html` and the published version 5 (read back through the publishing tool), `analysis/TOLERANCE-FROZEN-iteration4.json`, `analysis/iteration4.json`, `iteration4-red.json` and both logs, `scripts/p3_iteration4.py`, `scripts/p3_plots.py`, `fig-q3.png`, `fig-q1.png`, `fig-tempo.png` (Read), `docs/evidence/P3/iteration3/`, `iteration4/` and `iteration-3-4-SHA256SUMS.txt`.

Abbreviations used below: BPM (beats per minute), RMS (root mean square, here the loudness envelope), ACF (autocorrelation function), PNG (image file), JSON (data file), A/V (audio/video).

## In plain English

Iteration 4 fixed the big mistake. "Bar, not beat" is gone everywhere. The new analysis is right on its main point: in the host replay the light does carry the song's 138 BPM beat. I rebuilt every key number with my own code and got the same values. The script also regenerates its output byte for byte.

The beat test was frozen before its final run, and the lane says honestly that it was written after reading my numbers. It does what was asked:

- It finds a planted beat at the camera's and the host's sampling rates and rejects the accent pattern on its own.
- It fails at the audio's 20 Hz, and the lane says so instead of re-tuning it.

I ran it on 40 different random seeds. At 30 and 100 Hz it found the planted beat every time, with no false alarms. At 20 Hz it found it only 42 % of the time. So the 20 Hz failure is real, not bad luck with one seed.

**Three things are still not right on the page.**

1. **The "once-per-bar swell" is claimed for both boards but is only shown for one.**
   - On the lower board (Hybrid K1) there is a clear bar-rate line in the spectrum.
   - On the upper board (Tempo) there is none. The "13–15 % at the bar" there is ordinary slow wobble, which piles up at low rates in any signal. The lane's own post-hoc check shows this: on the song file, the Tempo board's bar line is 1.26 times its neighbours, while the control rates, where no musical rate sits, reach 1.27.
   - So my iteration-3 "bar only 3–5 %" was too low for the Hybrid K1 board, and the lane's "13–19 % bar swell" is too high for the Tempo board.
2. **"Beat not found on camera" needs its limit stated.**
   - The frozen test reliably notices a beat only when it carries about 13 % or more of the rhythmic power (30 % detection at 11 %, none at 8 %).
   - The camera's lower band does have a small line at the beat, about 3 % of its power.
   - So "not found" means "below what this test can see", not "absent".
3. **The receipt says the page still calls the 67 ms delay "not a latency measurement". It does not.** That caveat has dropped off the page.

Q1, Q2, Q4, the tempo section, the lede's lower/upper board wording, the "15–16 %" fix, the figure fixes, the evidence commit and the republish are accepted. The Q3 beat answer is accepted. The bar-swell wording, the camera "not found" wording and the missing delay caveat need a short text correction before the page goes to the Captain as the final P3 answer. No new analysis run is needed for that, only rewording backed by numbers already in `iteration4.json`.

## Corrections 1–8 from `REVIEW-ITERATION-3.md`

| # | Ask | Verdict | Evidence (reviewer's own check) |
|---|---|---|---|
| 1 | Q3 periodicity: withdraw "bar, not beat"; new `iteration4.json` with rate shares and comb scan; planted check that goes red; restate Q3 in three parts; keep the plate question open; label or drop "percentile among lags" | **Done, with one new overclaim** (finding R1) and one missing limit (R2) | All four withdrawn phrases are gone from the page text, the `fig-q3` title, the record and the receipt (searched the extracted page text). `iteration4.json` holds rate shares and a 72-period comb for all 10 series. The planted check exists, passes at 30/100 Hz, fails at 20 Hz and is disclosed; the red run is red. The three-part restatement is on the page. "Whether the beat survives to the plate" is tagged [INFERENCE]. The percentile tile is removed. |
| 2 | Rename the onset envelope; add linear RMS as beat reference; re-run alignment | **Done** | The envelope is renamed "accent-weighted rise envelope" on the page, figure legend, record and receipt. Linear RMS appears in the `fig-q3` spectra and alignment panels. Section D re-runs every target in both directions. I reproduced the lower-edge result independently (table below). |
| 3 | Lede: "mostly colour" for the lower board only; upper board halves (×0.53) and reddens (×0.46) | **Done** | The lede now reads: "On the lower board that is mostly a colour change from gold to red; on the upper board the light itself halves as well as turning redder." The Q1 row keeps ×0.93 for the lower board. |
| 4 | Direction: shared rhythm, not response delay | **Partly done** (R3) | The accent-envelope match is now stated as shared rhythm (367 ms lead, p 0.001). The loudness match is genuinely one-sided; my own curve rises from r 0.34 at 0 ms to a single peak of r 0.66 at +67 ms and is negative at every light-leading lag. However, the receipt's "the page still says it is not a latency measurement" is false: the word "latency" does not occur on the page, and "67 ms behind" stands with no caveat. |
| 5 | Upper-edge band: replace "shows nothing" | **Done** | The page reports: p 0.82–0.96 on three of four tests, one light-leading p 0.022 out of 24 tests, comb best 0.51 s, half-time 8 % (14 % after a 1 s moving mean), and no board attribution. All of it is reproduced. |
| 6 | Commit iteration-3/4 outputs, or reword | **Done; one input residual** (R7) | `git ls-files` lists 11 files under `docs/evidence/P3/iteration3/` and `iteration4/`. `shasum -c` gives all OK. All nine copies are byte-identical to the run-directory files. Residual: the camera-band and room-audio input `analysis/p3-data.json` is untracked, so the committed JSON regenerates only from an untracked input. |
| 7 | Figures | **Done** | I Read `fig-q3.png`. The title is new, the legend says "accent-weighted rise envelope", and the panel labels match `iteration4.json` (audio bar 7 · beat 17; Tempo 15 · 14; Hybrid K1 18 · 12; camera 23 · 3). No label sits on a curve and the accent curve is not clipped. `fig-q1.png`: both legends are above their axes, clear of the 48 s peak. `fig-tempo.png`: the annotation sits at 14 s / about 100 BPM, in empty space. "15–16 %" is on the page. |
| 8 | Re-render, Read, republish as version 5 | **Done** | The publishing tool's read-back of version 5 gives extracted text identical to the local source (14,079 characters). All 10 embedded PNGs hash-match the local `analysis/fig-*.png`. My headless renders: 400 px light (scrollWidth 400) and 1280 px dark (scrollWidth 1280), 0 broken images. I Read the answers board at both widths and the Q3 stat tiles at 400 px. The "Replay only" pill does not overflow. |

## Independent re-derivation

My code (`indep.py`, `indep_align.py`, `planted_mut.py`, `extra.py` in scratch) uses none of the lane's functions for these numbers:

- my own Welch estimator (Hann window, per-segment least-squares detrend);
- an independent multitaper estimate (discrete prolate spheroidal sequence tapers, NW 2.5);
- `scipy.ndimage.uniform_filter1d(mode="nearest")` for the 1 s moving mean;
- a direct-sum ACF for the comb;
- my own lag search, with a 2,000-shift null and a different seed.

The planted-check seed sweep and the mutation tests deliberately call the lane's functions, because they test the lane's logic.

**Regeneration.** `p3_iteration4.py` was run from scratch with only the root paths redirected:

- `iteration4.json`, `iteration4-red.json` and both logs are **byte-identical** to the committed copies;
- rc is 1 and 1, as the receipt states.

### Rate shares, BOOM 55.5–75 s

Each figure is the percentage of 0.3–5 Hz power within ±0.06 Hz of the rate.

| Series | Lane, no moving mean (bar / half / accent / beat) | Reviewer Welch, no moving mean | Reviewer Welch, 1 s moving mean | Reviewer multitaper |
|---|---|---|---|---|
| Room audio, linear RMS | 6.8 / 1.5 / 5.7 / 16.7 | identical | 1.7 / 2.3 / 8.7 / 15.8 | 5.9 / 1.2 / 5.2 / 15.5 |
| Host, song, Hybrid K1 | 11.1 / 4.4 / 2.5 / 5.0 | identical | 3.0 / 6.9 / 3.8 / 4.8 | 14.7 / 4.1 / 2.9 / 5.7 |
| Host, song, Tempo | 13.2 / 2.8 / 4.0 / 11.2 | identical | 3.7 / 4.9 / 7.8 / 13.1 | 10.5 / 2.7 / 4.3 / 10.1 |
| Host, room, Hybrid K1 | 18.5 / 4.0 / 3.9 / 11.6 | identical | 5.5 / 7.0 / 7.0 / 12.9 | 15.8 / 3.4 / 3.8 / 11.3 |
| Host, room, Tempo | 14.5 / 3.6 / 3.6 / 13.9 | identical | 4.1 / 6.2 / 6.8 / 16.1 | 10.2 / 2.7 / 3.2 / 14.0 |
| Input level, song / room | 10.1 · 7.3 bar; 16.9 · 15.3 beat | identical | — | 8.8 · 6.3 bar; 16.5 · 14.5 beat |
| Camera, lower edge | 23.4 / 3.9 / 9.4 / 2.7 | identical | 7.2 / 7.2 / 17.8 / 3.2 | 17.1 / 2.6 / 7.6 / 2.6 |
| Camera, upper edge | 10.9 / 8.4 / 3.2 / 1.9 | identical | 2.4 / 13.6 / 5.6 / 2.0 | 9.0 / 6.1 / 2.9 / 1.7 |

### Line strength above the slow-wobble floor

This is the reviewer's own test, not frozen. I fitted a power law to the multitaper spectrum over 0.3–5 Hz, leaving out ±0.1 Hz around every bar harmonic. I then took the ratio of each rate's band power to the fitted floor. "Off-grid max" is the same ratio at the seven midpoints between bar harmonics, where no musical rate sits.

| Series | Bar | Beat | Off-grid max | Lane's post-hoc peak/neighbour at bar (controls max) |
|---|---|---|---|---|
| Host, song, Hybrid K1 | **7.6** | 5.2 | 2.2 | 4.50 (1.79) |
| Host, room, Hybrid K1 | **6.8** | 16.7 | 1.7 | 6.20 (3.09) |
| Host, song, Tempo | 1.3 | 16.5 | 2.9 | **1.26 (1.27)** |
| Host, room, Tempo | 1.3 | 26.3 | 1.9 | 2.07 (1.53) |
| Camera, lower edge | 2.4 | 4.5 | 2.6 | 4.40 (1.59) |
| Camera, upper edge | 0.8 | 1.6 | 2.6 | 1.37 (2.25) |

### Other quantities

| Quantity | Lane | Reviewer |
|---|---|---|
| 1 s moving mean, power kept at bar / half / accent / beat | bar about 21 %, beat about 79 % | Boxcar theory 0.213 / 1.267 / 1.300 / 0.789. The half-time and accent rates are *boosted* by about 27–30 %. |
| Comb best period (frozen variant: linear detrend) | Tempo ×2, Hybrid K1 room, both input levels, audio: 0.435 s. Hybrid K1 song: 0.420 s (5 above). Camera lower: 0.585 s (17 above). Camera upper: 0.510 s (39 above). | identical, every series |
| Frozen `beat_present` | yes: Tempo song, Tempo room, Hybrid K1 room, both input levels, audio (not admissible). no: Hybrid K1 song, both camera bands. | identical |
| Lower band vs linear RMS, light lagging | r 0.664 at 67 ms, p 0.001 | r 0.664 at 67 ms, p 0.0005 (2,000 shifts) |
| Same, light leading | r 0.343 at 0 ms, p 0.172 | r 0.343 at 0 ms, p 0.160 |
| Upper band vs linear RMS, lag / lead | p 0.963 / 0.022 | p 0.954 / 0.040 |
| Planted check, seed 404 (with / without beat) | 20 Hz: no / no. 30 Hz: yes / no. 100 Hz: yes / no. | identical |
| Planted check, 40 seeds (detection / false positive) | not reported | 20 Hz 0.42 / 0.00 · 30 Hz 1.00 / 0.00 · 100 Hz 1.00 / 0.00 |
| Red run | with-beat false at all rates, rc 1 | identical (byte-identical regeneration) |

## Findings

### R1. The bar swell is shown on the Hybrid K1 board, not on the Tempo board (moderate) [FACT]

The page makes the claim in four places:

- the lede: "the boards' output … swells once per bar";
- the Q3 row and stat tile: "a once-per-bar swell (13–19 %) … on the Tempo board from both inputs, on the Hybrid K1 board from the room recording";
- the Q3 correction box;
- the record.

All of them attach the bar swell to the Tempo board. The evidence does not support that.

**Why the share is the wrong measure here.** A share of ±0.06 Hz of power cannot tell a periodic line from the broadband slow variation that every one of these signals has. The fitted floor falls with slope −0.4 to −1.9, so the band at 0.575 Hz sits on the steepest part of the spectrum. On top of that, an 8 s Hann segment has a main lobe of ±0.25 Hz. The "bar peak" in `fig-q3` is therefore a hump about 0.5 Hz wide, covering roughly 0.35–0.8 Hz.

**What the line test shows:**

- Bar line above floor: Hybrid K1 song 7.6×, Hybrid K1 room 6.8×.
- Tempo board: 1.3× on both inputs, below that series' own off-grid controls (2.9× and 1.9×).
- The lane's own post-hoc peak-to-neighbour ratio agrees: Tempo song bar 1.26 against a control maximum of 1.27, so there is no line. Tempo room bar 2.07 against 1.53, so it is weak.
- The beat line on the Tempo board is very strong: 16.5× and 26.3× the floor.

**How to read the two bar figures.** "The reviewer's bar 3–5 % was that artefact" is half right.

- The 1 s moving mean does remove about 79 % of bar power. My iteration-3 table understated the Hybrid K1 board's bar. I withdraw "the bar rate itself carries only 3–5 % of the power" as a general statement.
- The undetrended 13–19 % overstates the Tempo board's bar for the reason above.
- Neither share is a line test. The defensible statement is:
  - **Hybrid K1 board:** a clear once-per-bar line, plus the beat (room recording) or a weaker beat line (song file).
  - **Tempo board:** a strong beat line, and no bar line distinguishable from slow variation.

**The camera's lower band** has a weak bar line by my estimate: 2.4× the multitaper floor against off-grid 2.6×, or 3.9× against 2.3× with Welch. The lane's peak-to-neighbour is 4.4. Its 23 % bar share should be described as "mostly slow variation, with a weak bar line", not as a bar swell.

### R2. "Beat not found on camera" needs the test's sensitivity next to it (moderate) [FACT]

I swept the planted beat's amplitude at 30 Hz over 20 seeds:

| Amplitude (× accent std) | Median beat share | Detection rate |
|---|---|---|
| 0.3 | 7.9 % | 0 % |
| 0.4 | 10.7 % | 30 % |
| 0.5 | 13.6 % | 95 % |
| 0.6 | 16.7 % | 100 % |

So the frozen test only reliably reports a beat that holds at least about 13 % of the rhythmic power. Its 8 % share threshold is not the effective limit; the comb is.

**Mutation tests** (lane functions monkeypatched in scratch, frozen seed-404 check, judged at the qualified rates of 30 and 100 Hz):

| Mutant | Planted check result |
|---|---|
| Comb clause removed | survives |
| Share clause removed | survives |
| AND changed to OR | survives |
| Accent share read in place of beat share | survives |
| Share threshold lowered to 0.02 | survives |
| ±0.20 Hz beat band | survives |
| 1 s moving mean put back into the primary share | survives |
| Bar share read in place of beat share | goes red |
| Threshold 0.25 | goes red |
| Beat share computed at 1.15 Hz | goes red |
| Comb argmin | goes red |
| Comb with k = 1 only | goes red |
| Always-true | goes red |
| Always-false | goes red |

The check therefore guards the comb clause well. It cannot see the share clause, because the accent-only synthetic is always rejected by the comb (best period 0.65 s). On the real series the share clause also never decides a verdict:

- every "no" has a comb outside 0.425–0.445 s;
- every "yes" has a share of at least 11 %.

The verdicts are not wrong. But "at least 8 % of rhythmic power sits at the beat" describes a clause that was never exercised.

**Camera lower band.** It has a modest beat line: 4.5× the floor against off-grid 2.6×. This is my own estimate; the lane reports 3.5× post hoc, and both are post hoc. It carries 3 % of the power, well below the test's reach. "Not found" is accurate as a test result, but on its own the stat tile reads as "absent". Add the bound. Example: "Not found by the frozen test, which needs about 13 % of the power at the beat; the lower band's 2.30 Hz line holds 3 %."

### R3. The 67 ms figure lost its caveat, and the receipt says otherwise (minor) [FACT]

`RECEIPT-ITERATION-4.md` item 4 says: "The page still says it is not a latency measurement." Neither the local source nor the published version 5 contains "latency" or any equivalent for the loudness match. The stat tile and the Q3 row read "67 ms behind".

The 67 ms includes the iPhone's own A/V alignment and the camera's 33 ms frame quantisation, as well as any device delay. My own curve is single-peaked and one-sided, so "the band follows loudness, with the light behind" is supported. "The light lags by 67 ms" as a delay figure is not. Fixes:

- Add one clause to the page: "a correlation peak, not a latency measurement; the iPhone's audio–video offset is not calibrated".
- Correct the receipt line.

### R4. Timeline: the analysis script was edited after a run, and that is not disclosed (minor) [FACT]

UTC modification times:

| File | Time | Note |
|---|---|---|
| `REVIEW-ITERATION-3.md` | 10:07:44 | |
| `TOLERANCE-FROZEN-iteration4.json` | born 10:10:44, modified 10:11:01 | |
| `fig-q3.png` | 10:14:59 | built from an existing `iteration4.json`, so a run had already happened |
| `scripts/p3_iteration4.py` | **10:15:53** | |
| `iteration4.json` | 10:16:00 | |
| `iteration4-red.json` | 10:16:07 | |
| page | 10:17:54 | |
| commit | 10:20:49 | |

What this shows:

- **The frozen file predates the final run, and the criterion applied is the frozen one.** The script hard-codes the window and threshold, but asserts the frozen criterion string verbatim. The frozen file's sha256 matches the committed copy.
- **The `frozen_utc` erratum is honest.** The hand-typed 10:20Z matches the commit time, not the freeze.
- **"Before the first run" cannot be shown from modification times.** Each run overwrites its outputs. The script was changed at 10:15:53, after at least one run and after the freeze.
- **The figure's printed numbers match the final JSON**, so the edit probably did not touch the rate shares or alignment. [INFERENCE] It may have been the post-hoc section E or the printing.

Fixes:

- Disclose "script edited after a first run; criterion and planted recipe unchanged".
- Put the "frozen after the reviewer's numbers were read" qualifier beside the Q3 "frozen before it ran" sentence, not only in the provenance list. The criterion's window and threshold were chosen with my iteration-3 per-series numbers in view, so its "frozen" status adds little beyond the planted check.

### R5. The [INFERENCE] "beat from the level, not the tracker" holds for the song-file replay only (minor) [FACT for the coherences, INFERENCE for the attribution]

Coherence at 2.30 Hz (4 s segments; about 0.45 is the rough 95 % level for this record length):

| Replay | Light vs input level | Light vs tracker's phase pulse cos(2π·phase) |
|---|---|---|
| Song file, Hybrid K1 / Tempo | 0.77 / 0.95 | 0.01 / 0.06 |
| Room recording, Hybrid K1 / Tempo | 0.87 / 0.95 | **0.58 / 0.78** |

In the room replay, the tracker's phase is itself coherent with the input level at 2.30 Hz (0.80), even though its BPM reads 92. Also, 2.30 Hz is exactly twice 69 BPM.

The page's inference is therefore supported on the song file and undecided on the room recording. Keep the tag. Add the caveat. Name the decisive cheap test: a host replay with the tracker's phase held constant, or the beat gate forced to 0, then compare the 2.30 Hz line.

### R6. "3 of 4" is a frozen-test count, not "three boards beat, one does not" (minor) [FACT]

The Hybrid K1 song-file series fails the test:

- its comb is at 0.420 s, one step outside the window;
- its share is 5.0 %.

It still has a beat line at 5.2× the floor, against off-grid 2.2×. The tile's wording ("where the frozen test finds the beat") is correct. The record's "finds the 138 BPM beat in 3 of 4 light series" should keep the same qualifier, and should not be read as "the fourth has no beat".

### R7. The committed evidence does not include the camera and audio input (minor) [FACT]

`p3_iteration4.py` reads `analysis/p3-data.json` (862 kB: camera bands and room-audio RMS). `git ls-files` finds it nowhere. The committed `iteration4.json` regenerates byte-identically, but only with this untracked file. Either:

- commit it, or the BOOM slice of it, to `docs/evidence/P3/`; or
- say on the provenance line that this input is untracked.

### R8. Figure note (minor) [FACT, Read]

In the `fig-q3` right panel, the two pairs of dotted control lines are told apart only by white against grey. Their 95th-percentile levels for the two references (0.46 and 0.22) look like two global thresholds. The caption's "on each side" does not say which line belongs to which curve. Label the two lines directly.

No other defect was found in the three re-read PNGs.

## What is accepted as-is

- Q1, Q2 and Q4 wording and numbers (unchanged from iteration 3, which was accepted).
- The lede's lower/upper board sentence.
- "15–16 %".
- The tempo section and figure.
- The Q3 beat answer:
  - host replay carries the 138 BPM beat;
  - the frozen test does not detect it on camera;
  - the payload cannot resolve it;
  - the plate question stays open.
- The withdrawal of "bar, not beat" and of the percentile tile.
- The accent-envelope renaming and its shared-rhythm reading.
- The upper-edge band paragraph.
- The planted check and its 20 Hz disclosure. The 40-seed sweep confirms it.
- The red run.
- The evidence commit and checksums.
- The republish.

## Required corrections (iteration 5: text only, no new frozen analysis needed)

1. **Bar swell (R1).**
   - In the lede, the Q3 row, the Q3 stat tile, the Q3 correction box, the record and the receipt, restate it as:
     - a clear once-per-bar line on the Hybrid K1 board;
     - no bar line distinguishable from slow variation on the Tempo board.
   - Use the line-above-floor or peak-to-neighbour numbers already in `iteration4.json` section E, labelled post hoc.
   - Replace "the reviewer's bar 3–5 % was that artefact" with: "both bar figures are band shares; the 1 s moving mean removes about 79 % of bar power, and the undetrended share includes broadband slow variation; the line test shows a bar line on the Hybrid K1 board only."
   - Describe the camera lower band's 23 % as mostly slow variation with a weak bar line.
2. **Camera sensitivity (R2).**
   - Next to "Not found", state the test's demonstrated reach: about 13 % beat share for reliable detection at 30 Hz, and 0 % detection at about 8 %.
   - State that the share clause was never decisive, so the verdicts rest on the comb.
3. **Delay caveat (R3).**
   - Add "correlation peak, not a latency; the iPhone audio–video offset is uncalibrated" beside "67 ms behind".
   - Correct receipt item 4.
4. **Timeline (R4).**
   - Disclose that `p3_iteration4.py` was edited at 10:15:53Z, after a first run and after the freeze, with the criterion unchanged.
   - Move the "after the reviewer's numbers were read" qualifier into the Q3 text.
5. **Tracker inference (R5).** Add the room-replay coherence caveat and the 2 × 69 BPM note, and name the decisive replay test. Keep [INFERENCE].
6. **Evidence (R7).** Commit the BOOM input slice of `p3-data.json`, or mark it untracked on the provenance line.
7. **Figure (R8).** Label the two control lines in the `fig-q3` right panel. Re-Read the PNG.
8. **Republish** to the same URL as version 6. Re-render at 400 px and 1280 px, and Read the board and the Q3 section. Until then, version 5 should not be presented to the Captain as the final P3 answer.

Ship path for P3 (it is an audit, not a firmware change):

1. **Already in source:** the scripts and evidence at `c61991e`. Published version 5 is correct apart from the items above.
2. **Remaining:** text corrections 1–8.
3. **Who acts:** the Titan lane owner, then one fresh-context re-review of the changed text only.
4. **Done means:** version 6 published, its read-back text matching the local source, the re-review returning ACCEPT, and the record's changelog row added.

No device action is part of P3.
