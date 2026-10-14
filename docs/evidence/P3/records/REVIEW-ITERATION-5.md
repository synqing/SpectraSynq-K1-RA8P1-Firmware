# P3 iteration 5 — independent review (fresh context) — VERDICT: NEEDS WORK (four sentence-level fixes, no new analysis)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was offline and read-only. It opened no serial port and used no bridge, broker, device build, flash, deploy or launchctl. All computation ran in the reviewer's scratch directory (`/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/review-p3e/`). This file is the only repository file written. The agent cannot listen; nothing here is a claim about how the music sounds.

Reviewer: independent fresh-context reviewer (agent:claude, Reality Checker role), 14 Oct 2026.
Reviewed at `055f03c` (branch `lane/titan-lagfree-20261014`): `REVIEW-ITERATION-4.md`, `RECEIPT-ITERATION-5.md`, the iteration-5 block of `P3-RECORD.md`, `P3-CAPTAIN-PAGE.html` (local source of version 6), `P3-CAPTAIN-PAGE.iteration4.html` (version 5 source), the published page read back through the publishing tool, `analysis/iteration4.json` (sections A–E), `analysis/fig-q3.png` (Read), `docs/evidence/P3/inputs/p3-data-boom.json`, `git show 055f03c`, `git ls-files docs/evidence/P3`.

Abbreviations: BPM (beats per minute), RMS (root mean square, the loudness envelope), PNG (image file), JSON (data file), A/V (audio/video).

## In plain English

The lane did what review 4 asked, and the page is now right on every main point:

- The once-per-bar swell is now claimed for the Hybrid K1 (lower) board only, with the line-test numbers from `iteration4.json` section E. I checked every one: 4.5 and 6.2 against controls up to 1.8 and 3.1; Tempo 1.3 and 2.1 against 1.3 and 1.5; Tempo beat line 4.6 and 6.1.
- "Not found on camera" now says how big a beat the test can see.
- The 67 ms is now called a correlation peak, not a latency, in the answers row, the tile and its own paragraph.
- The timeline and the "frozen after the reviewer's numbers were read" qualifier are on the page.
- The BOOM input file is committed. I regenerated `iteration4.json` from committed files only and it came out byte-identical.
- The figure's two control lines are labelled, and no label sits on a curve.
- The published page's text is identical to the local source, and all ten images match.

**But the rewrite introduced four new wording errors, so version 6 should not yet go to the Captain as the final P3 answer:**

1. **A number is turned round.** The page says the test "missed the planted beat (42% of seeds)" at 20 Hz. It actually *found* it in 42% of seeds and missed it in 58%.
2. **One coherence is described as the wrong thing.** On the room recording, 0.58–0.78 is how closely the *light* follows the tracker's phase. The page says it is how closely the tracker's phase follows the beat.
3. **Numbers from my reviews appear without saying they are mine.** These are the coherences, the "about 13%" reach in the tile, and the 42%. Two of them also say more than I measured: "edited once", and the 13% applied to the host's rate as well as the camera's.
4. **"Weak bar line" on camera contradicts the page's own test.** On the same peak-to-neighbour test the page uses, the camera's lower band scores 4.4 against controls of at most 1.6. The page calls the Hybrid K1 board's 4.5 against 1.8 "clear". This one partly comes from my own review-4 wording, and I correct it below.
5. The figure's alt text (the text description of the image) still says both host boards peak at the bar.

None of this needs new analysis. It is about five sentences.

## Corrections 1–8 from `REVIEW-ITERATION-4.md`

| # | Ask | Verdict | Evidence (reviewer's own check) |
|---|---|---|---|
| 1 | Bar swell on the Hybrid K1 board only; line-test numbers from section E, labelled post hoc; the "both bar figures are band shares" wording; camera 23% as mostly slow variation with a weak bar line | **Done, with two residuals** (N4, N5) | Lede ("the lower board also swells once per bar"), Q3 row, tile, post-hoc paragraph, correction box, record and receipt all changed. Every line-test number matches `iteration4.json` section E, recomputed by me: Hybrid K1 bar 4.50 / 6.20 (control maximum 1.79 / 3.09); Tempo bar 1.26 / 2.07 (1.27 / 1.53); Tempo beat 4.57 / 6.07. No "13–19 % bar swell" survives except as the quoted, explained iteration-4 figure. Residuals: the `fig-q3` alt text still says "the host boards also [peak] at the bar"; "weak bar line" on camera is inconsistent with the page's own metric (N4). |
| 2 | State the test's reach next to "Not found" (about 13 %, 0 % detection at about 8 %); say the share clause was never decisive | **Mostly done** (N3) | The tile reads "needs about 13% of the power at the beat to see it reliably; the lower band's 2.30 Hz line holds 3%". The text says "In practice the period clause decides every verdict". The "0 % detection at about 8 %" bound is in the record, not on the page. The 13 % is not attributed to the reviewer in the tile. The fact paragraph extends it to "the camera's and the host's sampling rates", but I measured it at 30 Hz only. |
| 3 | 67 ms caveat; correct receipt item 4 | **Done** | The row says "peak at 67 ms, not a latency". The tile says "A correlation peak, not a latency: the iPhone audio–video offset is not calibrated". A new fact paragraph names the A/V alignment and the 33 ms frame step. The receipt erratum withdraws the false iteration-4 line. |
| 4 | Disclose the 10:15:53Z script edit; move the qualifier into the Q3 text | **Done, one overstatement** (N3) | The Q3 text says "frozen before its final run, but after the iteration-3 reviewer's per-series numbers were read … edited once after a first run; the criterion and the planted recipe did not change". The record gives 10:11:01Z and 10:15:53Z. "Once" cannot be shown from a modification time. The provenance line still says "frozen … before the lane's run", which reads as before any run. |
| 5 | Tracker caveat: room-replay coherences, 2 × 69 BPM, decisive test, keep [INFERENCE] | **Done in substance; one mislabel and no attribution** (N2, N3) | All present in the `fig-tempo` caption and the record. "Where the tracker's phase is also coherent with the beat (0.58–0.78)" is wrong. In review 4, 0.58 / 0.78 is the *light's* coherence with the tracker's phase; the tracker's phase was coherent with the input level at 0.80. The coherences are not in `iteration4.json` and are not labelled as the reviewer's on the page. |
| 6 | Commit the BOOM input slice or mark it untracked | **Done and verified** | `docs/evidence/P3/inputs/p3-data-boom.json` is tracked (commit `055f03c`), and its `boom` object equals that of the untracked `p3-data.json`. I ran `scripts/p3_iteration4.py` into scratch with only the committed frozen file and this slice as inputs. Exit code 1 (the frozen rule), and `iteration4.json` is **byte-identical** to both the committed and the run-directory copies. The provenance line marks the rest untracked. |
| 7 | Label the `fig-q3` control lines; re-Read the PNG | **Done** | Read: "95th pct, loudness" at about 0.46 and "95th pct, accent" at about 0.22, right-aligned at +350 to +490 ms, where both curves are below 0. No label touches a curve. The published `fig-q3` hash equals the new local PNG. Note: `docs/evidence/P3/iteration4/fig-q3.png` is still the unlabelled iteration-4 snapshot. That is acceptable as a record of iteration 4, but it is not the page's figure. |
| 8 | Republish as version 6; render at 400 and 1280 px; Read the board and Q3 | **Done** | Publishing-tool read-back: extracted text identical to the local source (16,295 characters), alt texts identical, all 10 embedded PNGs hash-match `analysis/fig-*.png`. My headless renders: 400 px light, 1280 px dark and 1280 px light. Page width equals the viewport each time, 0 broken images, 0 overflowing tiles, rows or paragraphs. I Read the answers board at 400 px and 1280 px dark, and the Q3 section at 1280 px light and 400 px (tiles and figure). |

## Findings

### N1. The 20 Hz figure is turned round (moderate) [FACT]

The page's Q3 fact paragraph says: "At the audio's 20 Hz it missed the planted beat (42% of seeds)". My 40-seed sweep in review 4 found the planted beat **in** 42 % of seeds at 20 Hz, so it missed it in 58 %. The frozen seed-404 run missed it, which is what `iteration4.json` section C records.

As written, the sentence understates the 20 Hz failure and states a reviewer number backwards on the Captain's page.

Fix: "At the audio's 20 Hz the frozen run missed the planted beat, and across 40 seeds it found it only 42 % of the time (reviewer), so the test gives no verdict on the audio itself."

### N2. The room-replay coherence is attached to the wrong pair of signals (minor) [FACT]

Review 4, R5 table: on the room replay, the light against the tracker's phase pulse cos(2π·phase) gives 0.58 (Hybrid K1) and 0.78 (Tempo). The tracker's phase against the input level gives 0.80.

The page (`fig-tempo` caption) says "where the tracker's phase is also coherent with the beat (0.58–0.78…)". The record's "the tracker's phase is coherent at 0.58–0.78" is ambiguous in the same way.

Fix: "on the room recording the light is also coherent with the tracker's phase (0.58–0.78, reviewer), whose phase itself follows the input level there (0.80); 2.30 Hz is exactly twice 69 BPM."

### N3. Reviewer numbers are unattributed, and two claims go beyond the evidence (minor) [FACT]

The receipt says every new number "is quoted from the reviewer and labelled as such". On the page that holds for "40 of 40 seeds, reviewer" only. None of the following is in `iteration4.json`, and none is labelled:

- the coherences 0.77–0.95, 0.01–0.06 and 0.58–0.78;
- the "about 13%" reach in the camera tile;
- the 42 %.

(The record does attribute the sweep and the 5.2× beat line.)

Two statements go further than the evidence:

- **"Only when the beat holds about 13% or more"** sits in a sentence about "the camera's and the host's sampling rates". My amplitude sweep ran at 30 Hz only. Say "at the camera's 30 Hz".
- **"Edited once after a first run"**: a modification time shows the last edit, not how many there were. Say "edited after a first run".

Also restore the second half of R2's ask, either on the page or in the tile: "0 % detection when the beat holds about 8 %". And change the provenance line's "frozen … before the lane's run" to "before its final run".

### N4. "Weak bar line" on camera contradicts the page's own line test (minor) [FACT; the cause is partly the reviewer's]

Section E, peak-to-neighbour at the bar (0.575 Hz):

| Series | Bar | Control maximum | Page wording |
|---|---|---|---|
| Host, song, Hybrid K1 | 4.50 | 1.79 | "clear" bar line |
| Host, room, Hybrid K1 | 6.20 | 3.09 | "clear" bar line |
| Camera, lower edge | **4.40** | **1.59** | "weak bar line" |

On the metric the page itself presents, the camera's bar line is as strong as the Hybrid K1 song-file line it calls "clear". It is also stronger than the camera's own 2.30 Hz line (3.5 against ≤ 1.6), which the page calls "small".

"Weak" came from my review-4 floor test: 2.4× a fitted slow-variation floor against 2.6× off-grid with multitaper, and 3.9× against 2.3× with Welch's method. That test disagrees with the peak-to-neighbour test here. I asked for "weak" without saying that the two tests disagree. That is my error, not the lane's.

Fix (page post-hoc paragraph and record): "The camera lower band's 23 % bar share is mostly slow variation. Whether it holds a bar line depends on the test: 4.4 times its neighbours (controls ≤ 1.6), but only 2.4 times a fitted slow-variation floor, against 2.6 off-grid (reviewer). Not claimed."

### N5. The `fig-q3` alt text still carries the withdrawn Tempo bar claim (minor) [FACT]

The alt text reads: "both host-replay boards peak at the beat … and the host boards also at the bar". This is the text a screen reader speaks, and the publishing tool's read-back returns it.

Also, the left panel is labelled "bar 15%" for the Tempo board. Beside the new text this invites the reader to see a bar swell. The figure is not wrong, because it is a band share and the caption says so in general.

Fix:

- Alt text: "…the Hybrid K1 board also at the bar; the Tempo board's low-rate hump is broad slow variation, not a bar line…".
- Optionally add to the caption: "the panel labels are band shares; the post-hoc line test below decides whether a bar line exists".

### Minor housekeeping (no fix required for acceptance)

- `RECEIPT-ITERATION-5.md` item 8 ends "see below", but nothing follows.
- The publishing tool reports a version identifier, not a version number. "Version 6" cannot be checked from the read-back. The content can, and it matches.

## What is accepted as-is

- The lede, the Q3 answers row and the bar-line tile. Every number traces to section E, and the Tempo board is correctly given no bar line.
- The camera "Not found" tile structure and "the period clause decides every verdict".
- The 67 ms caveat in all three places, and the receipt erratum.
- The timeline disclosure and the qualifier in the Q3 text (apart from "once").
- The [INFERENCE] tag and the decisive test (a host replay with the tracker's phase held still).
- The BOOM input commit and its byte-identical regeneration from committed inputs only.
- The `fig-q3` control-line labels.
- The republish: text, alt text and images all match the local source, and the layout is clean at 400 px and 1280 px in light and dark.
- Q1, Q2, Q4, the tempo section and everything else accepted in review 4. Unchanged text was compared by diff against version 5; only the lines listed in this review changed.

## Required corrections (iteration 6: text only)

1. **N1.** Reword the 20 Hz sentence so 42 % is the detection rate, attributed to the reviewer.
2. **N2.** Fix what 0.58–0.78 measures (light against the tracker's phase) on the page and in the record; add the 0.80 phase-against-level figure.
3. **N3.**
   - Label the coherences, the 13 % reach and the 42 % as the reviewer's on the page.
   - Restrict the 13 % to 30 Hz.
   - Add "0 % detection at about 8 %".
   - Drop "once".
   - Change the provenance line's "before the lane's run" to "before its final run".
4. **N4.** Replace "with a weak bar line" with the two-test statement above, on the page and in the record.
5. **N5.** Correct the `fig-q3` alt text; optionally add the band-share note to the caption.
6. Republish to the same URL, read it back, and check that the text matches the local source. A render check is needed only if the layout changes.

## P3 ship path (P3 is an audit, not a firmware change)

1. **Already in source:** the scripts and evidence at `055f03c`, including the committed BOOM input. `iteration4.json` regenerates byte-identically from committed files. Published page version 6 is correct apart from the five wording items above.
2. **Remaining:** text corrections 1–6 on the page, the record and the receipt.
3. **Who acts:** the Titan lane owner. Then one fresh-context re-review of the changed sentences only.
4. **Done means:**
   - the corrected page is published to https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM;
   - its read-back text matches the local source;
   - the re-review returns ACCEPT;
   - the record's changelog row is added.

   Only then is the page presented to the Captain as the final P3 answer.

No device action is part of P3. The Titan keeps its autonomous music visualisation throughout; P3 never touches it.
