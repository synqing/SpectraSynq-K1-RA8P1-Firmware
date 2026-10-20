# P3 — EXPRESSIVENESS AUDIT — RECEIPT, iteration 3 (answers REVIEW-ITERATION-2.md)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This iteration was offline and read-only: no device, ports, broker or flash. `a8215f75` ran v8 autonomously throughout.

- **Page:** https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM (version 4, private), source `P3-CAPTAIN-PAGE.html`. Earlier versions are kept as `P3-CAPTAIN-PAGE.iteration1.html` and `.iteration2.html`.
- **New data:** `analysis/iteration3.json`, `analysis/q3-iteration3.json` (with log), `analysis/q3-iteration3-red-plant0.json` (with log). Iteration-1 and iteration-2 JSONs are not regenerated.
- **Scripts:** `scripts/p3_iteration3.py` (new); `scripts/p3_q3_alignment.py` (detrend and control changed); `scripts/p3_plots.py`; `tests/host/test_p3_detrend.py` (new).

## Plain English

The reviewer found that my camera analysis smoothed the data in a way that made spikes at the start and end of each trace. Those spikes manufactured three results, which are now withdrawn. With the fix, the answer to "does the light carry the beat" is better than reported:

- In the host replay, the output follows the music's onsets and repeats once per bar.
- It does not repeat once per beat.

The headline tempo numbers have also been made robust across replay levels. The Tempo board's tracker reads half-time (69 BPM) in the verse and mostly 92 or 69 in the drop, never the song's 138 on the room recording.

## Corrections 1–11

1. **Detrend (N1). Fixed.**
   - **The fix.** `p3_q3_alignment.detrend()` now pads with edge values. The zero-padded form is documented in the docstring as the pre-iteration-3 behaviour.
   - **Regression test.** `tests/host/test_p3_detrend.py` (3 tests) passes, and it checks that the zero-padded form **fails** the same edge test, so the test is shown to go red.
   - **Corrected results** (`iteration3.json`, `q3-iteration3.json`):
     - The camera bands correlate at r **0.123** after detrending (0.329 raw). "Bands r 0.87" is withdrawn everywhere.
     - **Lower-edge band:** onset r **0.306**, lag 267 ms, p **0.001**. **Upper-edge band:** r 0.063, p 0.84.
     - **Light self-similarity** (with the percentile among lags 0.3–2.2 s): the lower-edge band reads **0.311 at one bar (98th percentile)**, 0.08 at 3/8 bar and −0.14 at one beat. The upper-edge band reads 0.03 at one bar and −0.31 at one beat (7th percentile). "The light does not repeat" is withdrawn.
     - **Power** (planted into shifted light, 20 trials each): 0 / 0 / 13 / 19 at 0.1 / 0.2 / 0.3 / 0.5× std. False positives: 0 of 40.
   - **Board attribution.** Still not established: the tilted plate, and the raw-level ρ against the upper payload.
2. **Gate control (N2). Fixed.**
   - The exit-code positive control now plants into **circularly shifted** light, not the observed band.
   - The run at 0.5× is detected on both bands (rc 0, `q3-iteration3.json`).
   - The recorded red run at plant scale 0 has the control fail on **both** bands, including the lower one, and exits 1 (`q3-iteration3-red-plant0.json` and log).
3. **Tempo headline (N3). Replaced.**
   - "113 → 92 → 69" is gone from the lede, the row, the tiles, `fig-tempo.png` and the record.
   - **Committed evidence** (`iteration3.json` F): a gain sweep of 0.7–1.6× on the song file, plus the room-recording replay.
   - **The robust statement:**
     - verse 69 (half-time);
     - about 138 at 33–43 s;
     - drop onset about 112, effective gate 0.0–0.13;
     - 55.5–75 s mostly 92 or 69.
   - **How often 138 appears in the drop:**
     - room recording: 92 for 84%, never near 138;
     - song file at gain 1.155 or above: 138 for 15%;
     - song file at gain 1.0 or below: 0%.
   - Iteration 3 first drafted "never 138 in the drop". That draft was rejected after measuring these shares and is not published.
   - The mechanism is tagged [INFERENCE], and half-time is noted as octave-equivalent.
   - **What the tempo drives:** the mode-18 travel speed, `inhouse_waveform.cpp:349–352`.
   - `fig-tempo.png` now plots the **effective (post-smoothstep)** gate for all three replays.
4. **Renderer wording. Fixed.** It now reads "compiled but discarded at link (rtthread.map 'Discarded input sections'; no symbols), not on the mode-32/18 path (`palette_runtime.cpp:403–406, 433`)".
5. **Darkness gates (N4).**
   - **[FACT]** The Dopex boards are dark from 76.3 s, but the silence flag only sets at 80.7 s (39% of post-roll samples). The silence flag is not what darkens the first ~4 s.
   - **[INFERENCE]** The donor admission gate (`palette_runtime.cpp:258–264`, PCM peak² 0.0048 / 0.0034) is consistent with the donor's falling PCM RMS of 0.012–0.025. It cannot be traced, because the snapshot's `peak` field is the donor's `peak_scaled`, not the gate's PCM peak² (`k1_live_runtime.cpp:816/838`), and the PCM peak² is not recorded.
   - The gate list now matches the AGC trace.
6. **Ceiling (N5).**
   - "The packer does not clip / 15,407 of 65,535" is replaced by: "15,407 is the display stage's full-scale code (65,535 × 0.25 × (0.75 + 0.25 tanh 1)); payloads reach it in 5 of 436 lower-board snapshots" (`iteration3.json` H).
   - The "display brightness 0.75" inference is removed. The upper ceiling (≈12,250) is kept as [INFERENCE]: a palette or effect ceiling, not traced.
7. **Follow-up consistency (N6). Done.**
   - Rec.709 luma is named as the Q1 metric.
   - The light × colour split is added: lower ×0.93 · ×0.68; upper ×0.53 · ×0.46.
   - The Q1 flattening stage now points to the trace (the effect mapping).
   - The proposal now separates the two problems: the beat (the tempo tracker) and the dim drop (colour and fade mapping).
8. **Time-series row (N7). Done.**
   - The BOOM confidence panel shows the host in-house confidence from the room recording, labelled HOST, with the gate band.
   - The donor confidence is labelled "no v8 effect reads it".
   - The caption lists what the device doesn't record.
   - Clipped lines are fixed (axis limits from the data), legends are moved off the data (`fig-ts-*`, `fig-q1`, `fig-q2`), and the `fig-q4` title now reads "in-house AGC".
   - Every PNG was re-read.
9. **Provenance and numbers (N8).**
   - The Q2 medians and the Dopex post-roll medians are in `iteration3.json` D, with the music window 2 s … end − 0.5 s.
   - **Values:** BOOM ÷193 / ÷13.5; Sonic upper ÷33 and lower **÷1.14** (the direction is corrected to dimmer); Dopex 0.
   - The reviewer's ÷192 / ÷13.3 comes from a slightly different music window. The difference is stated, not hidden.
   - The camera range is now 0.7–3 dB.
   - Donor BPM by window: 139 (28–55.5 s), **103** (55.5–75 s).
   - The record's iteration-1 body is headed "superseded where it conflicts".
10. **Q3 for rendered brightness. Done.**
    - The page states that payload snapshots (0.3–0.5 s apart) cannot resolve a 0.435 s beat.
    - **Host-replay payload luma, 55.5–75 s:**
      - one bar: 0.36–0.49 (99th percentile);
      - one beat: 0.01–0.14;
      - onset alignment: r 0.28–0.52, p 0.001, both boards, both inputs (`iteration3.json` G).
11. **Republish. Done** as version 4, after headless renders at 1280 px (light and dark) and 400 px (scrollWidth 1280 / 1280 / 400). The captures were read.

**Checks run:**
- `pytest tests/host/test_p3_detrend.py`: 3 passed, including the red test against the zero-padded form.
- `p3_q3_alignment.py analysis 0.5 q3-iteration3.json`: rc 0.
- `p3_q3_alignment.py analysis 0.0 q3-iteration3-red-plant0.json`: rc 1.
- `p3_iteration3.py`: rc 0.

**Not affected by the detrend bug:** the follow-up `docs/evidence/P3/agc-trace.html`. Its figures and correlations use centred rolling means with `min_periods=1`, not zero padding.
