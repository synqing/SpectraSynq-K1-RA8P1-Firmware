# P3 — EXPRESSIVENESS AUDIT — RECEIPT, iteration 4 (answers REVIEW-ITERATION-3.md)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This iteration was offline and read-only: no device, ports, broker or flash. `a8215f75` ran v8 autonomously throughout.

- **Page:** https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM, **version 5** (private). The source is `P3-CAPTAIN-PAGE.html`; version 4's source is kept as `P3-CAPTAIN-PAGE.iteration3.html`.
- **New data:**
  - `analysis/iteration4.json` and `.log`;
  - `analysis/iteration4-red.json` and `.log`;
  - `analysis/TOLERANCE-FROZEN-iteration4.json` (sha256 `d8a68bda63bf7bae…`).
- **Committed copies:** `docs/evidence/P3/iteration4/` and `docs/evidence/P3/iteration3/`, with `docs/evidence/P3/iteration-3-4-SHA256SUMS.txt`.
- **Scripts:** `scripts/p3_iteration4.py` (new) and `scripts/p3_plots.py` (Q3 figure rewritten; Q1 legend and tempo annotation moved).
- Iteration-1/2/3 JSONs are not regenerated.

## Plain English

The reviewer was right that "bar, not beat" was wrong. In the host replay, the light does carry the song's beat. Measured properly, it also carries a strong once-per-bar swell. The reviewer's "bar only 3–5%" was itself an artefact of a smoothing step that removes most of the bar rate.

On camera no beat is found. The band near the lower edge does follow the music's loudness closely: r 0.66, about 67 ms behind, and not when the light is put ahead.

The beat test was frozen before it ran. It tells a planted beat from an accent pattern at the camera's and the host's rates, but not at the audio's 20 Hz. So it gives no verdict on the audio itself, and the run exits 1 as the frozen rule says.

## Corrections 1–8

1. **Q3 periodicity (F1, F7). Done.**
   - **Withdrawn** from the pill, lede, Q3 row, stat tiles, figure title, record and this receipt line:
     - "Bar, not beat" and its variants;
     - "almost nothing at one beat";
     - the "99th percentile of lags" tile.
   - **Rate shares at four rates** (bar 0.575 Hz, half-time 1.15 Hz, accent 1.725 Hz, beat 2.30 Hz) and the 72-period comb scan are in `iteration4.json` A/B. They cover host luma (both boards, both inputs), host input level, both camera bands and the room audio (linear RMS and the accent envelope).
     - Primary: no pre-detrend.
     - Secondary: 1 s edge-padded detrend, which reproduces the reviewer's figures (Tempo, room: bar 4.1%, beat 16.1%).
   - **Frozen criterion:** the comb's best period lies within 0.425–0.445 s AND the beat share is at least 0.08.
     - **Disclosure:** it was frozen before the lane's run but after the reviewer's numbers were read.
     - **Timestamp erratum:** the file's hand-typed `frozen_utc` field says 10:20Z, which is wrong. The file was last written at 10:11:01Z (mtime), before the first run. Its sha256 has been unchanged since then. The frozen file is not edited.
   - **Planted check (C)** uses a 3+3+2 accent synthetic with and without a 2.30 Hz component.

     | Rate | With beat | Without beat |
     |---|---|---|
     | 30 Hz | found | rejected |
     | 100 Hz | found | rejected |
     | 20 Hz | **missed** (comb chose 0.65 s) | rejected |

     - Run rc 1, per the frozen rule.
     - Each series carries `beat_present_admissible`, which is false for the two 20 Hz audio series.
     - **Red run** (`--red`, the beat removed from the with-beat signal): with-beat is false at all three rates, rc 1.
     - **[FACT] Gap:** the criterion is not qualified at 20 Hz. It was **not** re-tuned.
   - **Results (frozen test, admissible series):**

     | Series | Beat found |
     |---|---|
     | Host luma, Tempo board, song file | yes |
     | Host luma, Tempo board, room recording | yes |
     | Host luma, Hybrid K1, room recording | yes |
     | Host luma, Hybrid K1, song file | no (comb 0.420 s, beat 5%) |
     | Host input level, both inputs | yes |
     | Camera lower-edge band | no |
     | Camera upper-edge band | no |

   - **Q3 restated in three parts on the page:**
     - host replay carries the beat and a bar swell;
     - the camera does not show the beat;
     - the payload cannot resolve it.
   - "Survives to the plate" is kept as an open [INFERENCE].
2. **Music reference (F2). Done.**
   - The envelope is renamed an "accent-weighted rise envelope": 0.2% at the beat, against 16.7% for linear RMS.
   - The linear RMS envelope is now the beat reference in the Q3 figure (spectrum panel, and the alignment panel in both directions).
   - The alignment test was re-run with it (`iteration4.json` D, every target, both directions).
   - **Lower-edge camera band:**
     - against linear RMS, light lagging: r 0.664 at 67 ms, p 0.001;
     - against linear RMS, light leading: r 0.343, p 0.172.
   - **Host luma, linear RMS, light lagging:** p 0.001 for the song-file Hybrid K1 board and the room-recording Tempo board; 0.068 and 0.086 for the other two.
3. **Lede (F3). Done.** It now reads "On the lower board that is mostly a colour change from gold to red; on the upper board the light itself halves as well as turning redder."
4. **Direction (F4). Done.**
   - The accent-envelope match is stated as shared rhythm (light leading 367 ms, p 0.001).
   - The loudness match is directional (leading side p 0.17). The page still says it is not a latency measurement.
5. **Upper-edge band (F5). Done.** It now reads:
   - follows neither reference (p 0.82–0.96; one light-leading test at p 0.022 out of 24 run);
   - comb best 0.51 s, so no musical period;
   - half-time 8%, or 14% after a 1 s moving mean;
   - no board attribution.
6. **Evidence storage (F6). Done.**
   - The iteration-3 and iteration-4 outputs are copied to `docs/evidence/P3/iteration3/` and `iteration4/` with a SHA256SUMS file, and committed (see the commit below).
   - Iteration 3's "committed evidence" wording is superseded by this line.
7. **Figures (F8). Done. Every changed PNG was read:**
   - `fig-q3` is rewritten: left, four stacked spectra with lines at the four rates; right, the lower-edge band's correlation against delay in both directions.
     - First render defects: the accent curve was clipped at −0.42, and the "light lags" label sat on the curve. Both were fixed and re-read.
   - `fig-q1`: the bottom legend is above the axes, clear of the 48 s peak.
   - `fig-tempo`: the annotation is in empty space (14 s, 104 BPM), on two lines.
   - "at most 15%" is now "15–16%".
8. **Republish. Done.**
   - Headless renders: 1280 px light and dark (scrollWidth 1280), 400 px (scrollWidth 400), 0 broken images.
   - The header, the answers table (both themes and phone) and the Q3 section were read.
   - One defect, the new Q3 pill overflowing into the text, was fixed ("Replay only") and re-read.
   - Published as **version 5**. The published text was read back.

## Supplementary, post hoc (not frozen)

- **Peak-to-neighbour ratio** at each rate, against the same ratio at 7 off-grid midpoints between bar harmonics (`iteration4.json`, per series `E_peak_to_neighbour_posthoc`).
  - **Beat line:** audio 8.6, host Tempo board (room) 6.1, host Hybrid K1 (room) 5.2, camera lower band 3.5, camera upper band 2.45.
  - **Off-grid controls**, highest per series: 1.23, 1.53, 3.09, 1.59, 2.25.
- **Camera lower band:** its small beat line is reported on the page as post hoc, and not claimed.

## Checks run

- `p3_iteration4.py analysis`: rc 1 (20 Hz planted case; 30/100 Hz pass).
- `p3_iteration4.py analysis --red`: rc 1, with-beat false at all rates.
- `p3_plots.py analysis`: rc 0. The PNGs were read.
- Page render at three sizes: read.
- Version 5 read back.
