# P3 iteration 2 — independent review (fresh context) — VERDICT: ACCEPT AS PARTIAL

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This review was read-only and offline. It opened no serial port and used no bridge, broker, build, flash, deploy or launchctl. Host builds and runs happened only in the reviewer's scratch directory (`/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/review-p3b/`). This file is the only repository file written.

Reviewer: independent fresh-context reviewer (agent:claude, Reality Checker role), 14 Oct 2026.
Reviewed: `RECEIPT-ITERATION-2.md`, `P3-RECORD.md`, `P3-CAPTAIN-PAGE.html` (and published version 3 at the receipt URL, whose body matches the local source), `REVIEW-ITERATION-1.md`, all ten `analysis/fig-*.png`, `analysis/iteration2.json`, `timeseries.json`, `host-tempo-boom.csv`, `q3-red-run-plant0.json/.log`, `scripts/p3_iteration2.py`, `p3_q3_alignment.py`, `p3_plots.py`, `tests/host/p3_tempo_probe.cpp` at `dbd053b` and at HEAD `7422b23`, the build-`9ab90873` receipt, map and symbols (`_scratch/waveform-migration-001/arm-queue-pressure-010/`), and `docs/evidence/P3/agc-trace.*` (light consistency check only).

## In plain English

Most of iteration 2 holds up. I rebuilt the host tempo probe, and it reproduced the recorded CSV byte for byte. Every iteration-2 JSON regenerated identically from the lane's scripts. My own code reproduced the silence ratios, Sonic's post-roll microphone bursts, the payload ceilings and the donor-confidence maxima. The author was right to correct the previous review: the v8 boards do not run `product_effect_renderer.cpp`, and the Tempo board reads the in-house tempo, not the donor's.

Three new problems stop the page going to the Captain as it stands.

1. **The beat answer rests on a smoothing bug. The previous review shared the same bug.** Q3 removes slow drift with `np.convolve(..., mode="same")`. That call pads with zeros, so the first and last half-second of every camera trace jump by about 0.25, against a real wobble of about 0.013. Those edge jumps cause three errors:
   - They create the "two camera bands move together, r 0.87" figure. With the edges handled correctly it is 0.12.
   - They flatten the light's self-similarity, which produced "the light does not repeat". Corrected, the band near the lower edge repeats at one bar (0.31 at 1.733 s).
   - They dilute the onset result. Corrected, the band near the lower edge follows onsets more strongly than reported (r 0.31, p 0.001, against 0.18 / 0.02). The band near the upper edge shows no evidence.

   They also inflate the test's claimed sensitivity: 0/20 at 0.2× std, not 18/20.
2. **The headline tempo numbers "113 → 92 → 69" depend on how loud the replay is.** At 1.155× gain, the level the follow-up trace calibrated to the device, the 57–67 s stretch reads 69, not 92. The replay of the room recording reads 92 for all of 57–75 s. What holds in every replay: no 138 in the drop, and 69 (half-time) in the verse.
3. **One [fact] about silence is wrong.** After Dopex, the boards go dark at 76.4 s while the device's silence flag is still false, so the silence flag is not what holds them dark. The follow-up trace names the donor admission gate instead.

Q1 (the drop), Q2 (silence) and Q4 (song vs song) are accepted on substance, with wording fixes below.

## What I re-derived

### Host tempo probe (correction 3, and the receipt's "corrects the review" section)

**Build and run.** I built `tests/host/p3_tempo_probe.cpp` as it was at `dbd053b`, and again at HEAD, in scratch. The build used the receipt's flags (`-std=c++17 -O2 -fno-fast-math -DK1_EMOTISCOPE_PORT=1`) and the same sources; `k1_asrc_24k.c` was compiled with `cc`. I ran both on `boom-40k-mono.s16` (sha256 `d099174e…26b5a1`). [FACT]

**The `dbd053b` output equals `analysis/host-tempo-boom.csv` byte for byte (`cmp`).** [FACT]

**Palette independence (the lead's question).** The HEAD probe uses palette ids 31/38, the v8 controls and display stage. Its `tempo_valid`, `tempo_bpm`, `tempo_confidence`, `tempo_phase01`, `peak_scaled`, `vu_level`, `spectral_energy`, `novelty` and `silence` are identical in every one of 14,998 rows to the iteration-2 probe, which used ids 32/39 and default controls. Max difference 0.0. The claim holds: tempo and level do not depend on palette or controls. [FACT]

**Beat gate.** The HEAD `beat_gate` equals smoothstep(iteration-2 `beat_gate`) to 1.1e-4. The iteration-2 numbers ("mean beat gate 0.89 / 0.93 / … / 0.06") are therefore the linear clamp **before** the smoothstep that `inhouse_waveform.cpp:348` applies. The effective gate is slightly different: 0.02, not 0.06, at 55.5–57 s, and 0.07, not 0.10, at 43–50 s. The conclusion does not change. [FACT]

**Section table.** It reproduces exactly from my own code: 139 (0.91), 69 (0.99), 138 (1.00), hunting, 113 (0.61), 113 (1.00), 92 (0.83), 69 (1.00). [FACT]

**Sensitivity to input level (new).** The same probe, at HEAD, with the song file scaled before replay. Each cell is the BPM mode, with its share of frames in brackets.

| Gain | 7–33 s | 33–43 s | 55.5–57 s | 57–67 s | 67–75 s |
|---|---|---|---|---|---|
| 0.70 | 69 (0.91) | 139 (0.64) | 112 (0.97) | **92** (0.82) | 92 (0.50) |
| 0.85 | 69 (0.91) | 138 (1.00) | 112 (1.00) | **92** (0.83) | 69 (0.86) |
| 1.00 (iteration 2) | 69 (0.99) | 138 (1.00) | 113 (1.00) | **92** (0.83) | 69 (1.00) |
| 1.155 (follow-up's device-calibrated gain) | 69 (0.99) | 138 (1.00) | 113 (1.00) | **69** (0.42) | 69 (1.00) |
| 1.30 | 69 | 138 | 113 | **69** (0.42) | 69 |
| 1.60 | 69 | 138 | 113 | **69** (0.42) | 69 |

The follow-up's room-recording replay (`docs/evidence/P3/agc-trace-boom-recording.csv`, upper board) reads 69 (0.79) at 7–33 s, 140 (0.67) at 33–43 s, 112 at 55.5–57 s and **92 at 57–75 s**, with the gate open (0.99–1.0). [FACT]

So the robust result is "half-time in the verse, near 138 at 33–43 s, never 138 in the drop (92 or 69)". The exact sequence "113 → 92 → 69" is one gain's outcome. It is the page's lede, and it does not survive a 15 % level change.

### Renderer correction (corrections 4 and 5)

**[FACT] The author is right on substance.**

- Build `9ab90873` (`arm-queue-pressure-010/receipt.json`) records `emotiscope_port` and `titan_top_output` enabled.
- `k1_emotiscope_is_inhouse_mode()` (`emotiscope_port.h:14`) admits modes 7, 8, 11, 18 and 32.
- `PaletteRuntime` dispatches in-house modes to `renderInhouse()` (`palette_runtime.cpp:403–406`, `:433`).
- `renderInhouse()` calls `InhouseWaveformRenderer::render()`.
- `symbols.txt` contains `InhouseWaveformRenderer::render` and `::renderFamily`. It contains no `renderWaveformTempo`, no `renderWaveformK1` and no `ProductEffectRenderer`, and `disassembly.txt` has 0 `ProductEffectRenderer` references.
- The mode-18 gate is `clamp01((c−0.30)/0.30)` then smoothstep (`inhouse_waveform.cpp:347–348`). Tempo comes from `readTempoTracker(tempo_[board])` (`inhouse_audio.cpp:357–359`).
- Every relevant source hash in the build receipt matches the current worktree: `inhouse_audio.*`, `inhouse_waveform.*`, `palette_runtime.*`, `tempo_tracker.*`, `tempo_acf*`, `gdft_*`, `product_palette*`. `k1_asrc_24k.c` differs only by a target-only ITCM section attribute (P2), which is inert on the host.

**[FACT] The cited evidence is imprecise.** "That file is not in this build (`build_scalar.py:728`)" is wrong as worded:

- `product_effect_renderer.cpp` **is** compiled into build `9ab90873`. It is hashed into `receipt['sources']` and `LOAD`ed in `rtthread.map:5566`.
- Its sections are discarded at link (`rtthread.map:4634–4640`, under "Discarded input sections").
- `build_scalar.py:728` (`product_output_chain`) governs `frame_blend` and `product_runtime_policy`, not that renderer.

Say instead: "compiled but discarded at link; not on the mode-32/18 call path", and cite the dispatch lines, the map and the symbol table.

### Numbers reproduced with my own code

**[FACT] Iteration-2 numbers.** These match iteration 2 except where noted.

- **Silence, median against median** (silence = t < −0.3 or t > end + 0.5):
  - BOOM lower 4.073 → 0.0212 ‰ (÷191.7). The record says ÷193.
  - BOOM upper 3.686 → 0.2763 ‰ (÷13.3). The record says ÷13.5.
  - Sonic upper ÷32.7.
  - Sonic lower 1.267 → 1.108 ‰. That is ÷1.14 (dimmer). The record's "lower ×1.1" keeps the iteration-1 mean-based direction.
  - Dopex: both boards' silence median is exactly 0.
  - **No committed script or JSON produces these Q2 median ratios.** They exist only in prose.
- **Sonic post-roll:** raw RMS 0.0537 (26.8 s), 0.0955 (27.2 s), 0.0971 (31.64 s) and 0.1608 (32.08 s), against a music median of 0.1271. The lower payload was 4.44 / 3.8 / 1.6 / 2.0 ‰ at those moments.
- **Payload ceiling:** I decoded `wire-snapshot*.bin` bytes 544–1504 big-endian myself, windowed on the same MIR times. Maxima are 15,407 / 12,126 (BOOM), 14,738 / 12,252 (Sonic) and 15,407 / 10,186 (Dopex). The fraction of snapshots within 5 % of the maximum is 0.021 / 0.103, 0.020 / 0.776 and 0.013 / 0.019.
- **Donor features:** BOOM 28–75 s confidence max 0.2478, Sonic 0.1189, Dopex 0.1237; `beat_locked` is never true.
  - The window split matters. In BOOM 55.5–75 s the donor's BPM median is **103** (values 102–107) with confidence max 0.158. In 28–55.5 s it is 139.
  - The iteration-2 single figure "BOOM donor BPM median 137" hides that the donor itself drops to about 103 in the drop. It is harmless now that the donor is shown to be not read, but it should not be quoted as the donor tracking the song.
- **Dopex post-roll:** median `peak_scaled` 0.677 at median raw RMS 0.0177 (n 23). This matches the page, but neither median is in `iteration2.json`, which stores only min/max.
- **Raw-RMS and AGC ρ** (`level_tracking_spearman`): match.

**[FACT] Re-running the lane's own scripts in scratch.** `p3_iteration2.py` regenerates `iteration2.json` and `timeseries.json` byte-identical. `p3_q3_alignment.py analysis 0.0 red.json` exits 1 and is byte-identical to `q3-red-run-plant0.json`. The committed `q3-correction.json` differs from a fresh run only by the absent `plant_scale` key, confirming that the iteration-1 file was not regenerated.

## Findings

### N1. Zero-padded detrend drives the crosstalk number, the "light does not repeat" claim and the Q3 power curve (material) [FACT]

`p3_q3_alignment.detrend()` computes `x − np.convolve(x, ones(k)/k, mode="same")`. Zero padding halves the moving mean at both ends, so the detrended camera band starts at +0.246, 0.229, 0.209… The middle of the trace has a std of only 0.0131. `p3_iteration2.py` section A imports this function.

Measured on BOOM 55.5–75 s, lane construction otherwise unchanged:

| Quantity | Lane (zero-pad) | Edge-padded detrend | Zero-pad, 30 edge frames trimmed |
|---|---|---|---|
| Camera band vs band, r | **0.866** | **0.123** | 0.105 |
| Lower-edge band vs onsets: best r, lag, p | 0.185, 267 ms, 0.020 | **0.306**, 267 ms, **0.001** | 0.299, p 0.001 |
| Upper-edge band vs onsets | 0.075, p 0.30 | 0.063, p 0.83 | 0.082, p 0.75 |
| Light ACF, max abs over 0.4–1.2 s (upper / lower) | 0.055 / 0.054 | 0.375 / 0.226 | — |
| Lower-edge band ACF at one bar (1.733 s) | — | **0.311** | — |
| Power at 0.1 / 0.2 / 0.3 / 0.5× std (of 20) | 5 / 18 / 20 / 20 | **0 / 0 / 13 / 19** | — |
| False positives (of 40) | 0 | 0 | — |

Notes on the table:

- Detected positives at 0.3× std and above still hold.
- The audio onset ACF (0.58 / 0.49 / 0.60) is unaffected. Its edges are small relative to its variance.
- The edge-padded lag curve for the lower-edge band is −0.42 at 0 ms, 0.20 at 100 ms, 0.12–0.13 at 167–200 ms and 0.31 at 267 ms. "0.1–0.27 s, broad plateau" should become "peaks at 100 and 267 ms with a dip between". It is still not a latency.
- `fig-q3.png` shows the artefact directly. Both camera ACFs decay identically and smoothly to 0 by 0.4 s. That is the autocorrelation of the edge spikes, not of the light.

Consequences:

- The crosstalk sentence "bands r 0.87 after detrending" on the page (Q3 caption, `fig-q3.png` title), in the record and in the receipt is withdrawn. It came from iteration-1 F1, which made the same error.
- "The music repeats in a 3+3+2 pattern and the light does not" (page Q3 row) is unsupported. The lower-edge band repeats at one bar (0.31) but not at 3/8 or 5/8 (0.08 at 0.652 s). A null for these ACF values is still needed before any verdict.
- "The plate as a whole follows onsets weakly (r 0.18)" also changes. The two bands now carry different fast content: the lower-edge band follows onsets clearly and the upper-edge band does not.
- Board attribution is still not established, for two reasons. The tilted-plate geometry from iteration-1 F1 still stands. Both bands' undetrended level still follows the upper payload more than the lower (ρ 0.558 / 0.506 against 0.362 / 0.347; these use raw values and are unaffected). The honest statement is "the band near the lower edge follows onsets; which board drives it is not established".

### N2. The Q3 gate's positive control is confounded with the measurement (minor) [FACT]

`p3_q3_alignment.py` plants into the **observed** band. At plant scale 0 the lower band's "planted" detection is simply the real result: `detected: True`, p 0.020. The recorded red run exits 1 only because the upper band has no real effect.

The run is genuine, but the gate cannot go red on the lower band while the real effect exists. The iteration-2 power curve plants into circularly shifted (decorrelated) light, which is the correct construction. Move the gate's exit-code control to that construction, so a planted fault (for example a broken lag search) turns it red on both bands.

### N3. The tempo headline is gain-sensitive (material) [FACT]

See the gain table above. "113 → 92 → 69 BPM through the drop", which appears in the lede, the Q3 row, the stat tiles, `fig-tempo.png`'s title and the record, is one replay at gain 1.0.

The robust, quotable result across five gains and the room recording:

- **Verse 7–33 s:** 69 (half-time).
- **33–43 s:** about 138.
- **Drop onset, 55.5–57 s:** about 112–113, gate nearly closed.
- **57–75 s:** 92 or 69, never 138, gate open.

The room-recording replay is the closest to the device's input and reads 92 for all of 57–75 s.

Labels also need to separate arithmetic from mechanism:

- "92 is one 3/8 group of a 138 bar" (138 × 2/3) and "69 is half of 138" are arithmetic facts.
- "The tracker locks onto the accent pattern … rather than its pulse" (page Q3 `fact` tag) is a mechanism. It is [INFERENCE].

Half-time is an octave-equivalent metrical level, and standard tempo scoring accepts it (2× and ½× are allowed). Whether 69 or 92 looks wrong on the plate is the Captain's call. Say what the tempo drives: in mode 18 it sets the travel rate, `24·(bpm/60)·(1+0.5·cos 2π·phase)` px/s (`inhouse_waveform.cpp:349–352`), so the beat shows as a once-per-beat surge in travel speed, not in brightness.

### N4. "Other gates hold the boards dark: the silence flag, presence, raw-margin" is mislabelled [FACT] (moderate)

Dopex post-roll: both payloads are 0 from 76.4 s while `inhouse_silence` is **false** in every sample up to 80.2 s. Only 39 % of post-roll samples have the flag set, because of the 5 s dwell. So the silence flag is not what darkens the first five seconds.

`renderInhouse()` sets `input.silence = !bloom_admitted || confirmed_quiet` (`palette_runtime.cpp:258–264`). The follow-up `agc-trace.html` names the donor admission gate (PCM peak² 0.0048 / 0.0034) as one of the forcing gates. Either trace it or tag the sentence [INFERENCE]; drop "the silence flag" for this window; and make the gate list consistent with the follow-up page.

### N5. Payload-ceiling inference conflicts with the follow-up; "no clipping" is mis-framed (moderate) [FACT + INFERENCE]

**Ceiling inference.** The receipt infers the upper ceiling as "full white × edge gain 0.25 × display brightness 0.75". The follow-up fact sheet states display brightness 1.0 and output intensity 0.25.

The lower board's 15,407 equals `65535 × 0.25 × (0.75 + 0.25·tanh(1))` exactly. That is the display stage's code for a renderer channel at full scale, the hard ceiling (asymptote 16,384). It is reached in 4 BOOM and 1 Dopex snapshots (10 channels).

"The packer does not clip: the largest code is 15,407 of 65,535" therefore implies headroom that does not exist. It should read: "15,407 is the display stage's full-scale code; the renderer saturates there in 5 of 436 lower-board snapshots, so clipping is rare and is not the flattening stage." Re-derive the upper ceiling (12,252) from the Crimson Gold palette's brightest entry through the same stage, or keep it [INFERENCE] with the 0.75 brightness removed.

### N6. Consistency with the Captain's follow-up trace (moderate) [FACT]

`docs/evidence/P3/agc-trace.html` (commit `7422b23`) finds:

- From build to drop, the lower board's **light** (channel sum) is ×0.93, while its **luma** is ×0.63. The difference is colour, gold to red, which switches at about 49.5 s.
- Level speeds the fade (`inhouse_waveform.cpp:222`, `:341`).

P3's Q1 "boards are only ×0.63 / ×0.24 as bright" and "the drop renders dimmer" use Rec.709 luma without saying so. Q1 is exactly the "locate the stage that flattens them" question the brief asks.

Fixes for iteration 3:

- Name the metric (luma).
- Add the light × colour split from the follow-up.
- Replace "[NOT TRACED] which features each v8 effect reads frame by frame" in the record with a pointer to the follow-up trace.
- Explain why the page's proposal (tempo lock first) and the follow-up's verdict (the dim drop comes from colour mapping and fade) are not in conflict. They address different questions: beat versus drop brightness.

### N7. Time-series row still plots a feature neither effect reads, beside a gate it does not feed (minor) [FACT]

The third panel of `fig-ts-boom/sonic/dopex.png` draws the 0.30–0.60 gate band over the **donor** confidence, which neither v8 effect reads. The caption explains this, but the picture invites the reader to compare the two.

The features the effects actually read are not recorded on the device: in-house tempo and confidence, `observed_peak_scaled`, spectral energy, novelty, chroma. Fix in two parts:

- For BOOM, put the host-replay in-house confidence (room recording) in that panel, labelled as host.
- State plainly on the row which read features the device snapshots do not carry.

Minor figure defects:

- The AGC line clips off the top of the middle panel (Sonic about −0.5 s; Dopex about 17 s).
- Legends overlap data: the middle panels of `fig-ts-*`, `fig-q1` middle, and the `fig-q2` BOOM panel's upper-right post-roll.
- `fig-q4.png`'s title still says "reach **the effects** at the same level". It should say "the in-house AGC", per correction 4.
- `fig-tempo.png`'s gate legend shows the pre-smoothstep clamp.

### N8. Provenance and small number fixes (minor) [FACT]

- The Q2 median ratios and the Dopex post-roll medians must come from a committed script into `iteration2.json`, not prose.
- Record: ÷193 → ÷192, ÷13.5 → ÷13.3, Sonic lower "×1.1" → "÷1.1 (median)".
- Camera range: "about 1–3 dB" → "about 0.7–3 dB" (Dopex upper band 0.73 dB).
- The record's iteration-1 body below the iteration-2 block still carries the old Captain questions, "Three songs reach the effects", "[NOT TRACED]" and the old proposal. Keep it (plans and records are not compressed), but head it "Iteration 1 — superseded where it conflicts", so a reader does not take the old questions as current.

### On the receipt's disagreements with the previous review

| Receipt claim | Judgement |
|---|---|
| F3/F4: the review cited a renderer the v8 image does not run | **Correct** on substance (dispatch, symbols, disassembly). The evidence citation needs N-fix: compiled but discarded at link, not "not in this build". |
| The donor confidence is not the mode-18 gate input; the gate-at-zero reading does not apply | **Correct** (`inhouse_audio.cpp:357–359`). |
| Probe output does not depend on palette or default controls | **Correct**: identical tempo and level columns across the two probe versions. |
| "113 → 92 → 69" | Reproduced at gain 1.0, but **not robust** (N3). |

## What is accepted as-is

- **Correction 1.** The relabelling to "camera band near the upper/lower edge" and the withdrawal of board-level claims are correct in direction. The numbers change under N1.
- **Correction 2.** Sonic post-roll restated as a room-sensitivity question, and Captain question 3 rewritten. Accepted.
- **Correction 3.** Donor features are plotted and reported, with the correct reason they do not gate mode 18. Accepted, subject to N7.
- **Corrections 4 and 5.** Renderer inputs are listed and the citations moved to `inhouse_waveform.cpp`. Accepted, subject to the N-fix on "not in this build".
- **Correction 6.** Per-song time plots for all three brief windows exist. Accepted, subject to N7.
- **Correction 8.** One dB convention per quantity, with the sRGB decode applied to 0–1 grey levels (I checked the range 0.43–0.77). `fig-stages.png` is redrawn as grouped bars. Accepted.
- **Correction 9.** The `fig-q1.png` edge spikes are gone; I inspected it.
- **Corrections 11–14.** Scope words, ceiling check, text fixes, n per window, receipt wording, republish: done.
  - The page renders headless at 400 px (scrollWidth 400) and 1280 px dark (scrollWidth 1280); I Read the 400 px capture.
  - The page carries "Agent cannot listen", the auto-exposure caveat, "Proposal — not approved" (rendered uppercase), and the scope of the submitted payload and the 24/255 cap.
  - It leads with plain English.
- **Q1, Q2, Q4.** The substance stands. Q1's quiet-to-drop and build-to-drop ratios, the AGC squeeze (×5.58 → ×1.52) and the AGC-lifts-silence observation all reproduce.

## Rendered brightness and the 138 BPM question (for iteration 3, not a defect of iteration 2)

The brief asks whether *rendered brightness* has 138 BPM periodicity in 55.5–75 s. The payload snapshots (every 0.3–0.5 s) cannot resolve a 0.435 s beat, and the page should say so.

The HEAD probe already logs per-frame payload luma at 100 Hz with the v8 controls. On the song file, with a 1 s edge-padded detrend, I measured [FACT, host, song file]:

| Board, window | ACF at one bar (4 beats) | ACF at 2 beats (0.87 s) | ACF at one beat |
|---|---|---|---|
| Upper (Tempo), 55.5–75 s | **0.39** | 0.17 | 0.02 |
| Lower (K1), 55.5–75 s | **0.32** | — | 0.07 |

That is bar-level periodicity in host-rendered brightness, with no beat-level periodicity. With a shift null and the room-recording replay beside it, this answers Q3 for the payload directly, scoped as host replay.

## Required corrections (iteration 3):

1. **Detrend (N1).** Replace the zero-padded `detrend()` in `p3_q3_alignment.py`, which `p3_iteration2.py` imports, with an edge-padded or trimmed version.
   - Re-run Q3, the crosstalk (section A), the light ACFs, the power curve and the false-positive count into **new** files. Do not regenerate `q3-correction.json` or `iteration2.json`.
   - Withdraw "bands r 0.87" everywhere: page Q3 caption, `fig-q3.png` title, record, receipt.
   - Withdraw "the light does not repeat" and "|ACF| ≤ 0.055".
   - Restate the Q3 row, card and figure from the corrected numbers, with a null for the ACF values.
   - Add a planted-edge regression test that goes red on the zero-padded version.
2. **Q3 gate control (N2).** Make `p3_q3_alignment.py`'s exit-code positive control plant into circularly shifted light, not the observed band. Record one exit-1 run in which the **lower** band's control fails under a planted fault.
3. **Tempo headline (N3).**
   - Replace "113 → 92 → 69" in the lede, the Q3 row, the stat tiles, `fig-tempo.png`'s title and the record with the robust statement: verse at half-time 69; about 138 at 33–43 s; drop onset about 112–113 with the gate nearly closed; 57–75 s at 92 or 69, never 138.
   - Commit the gain sweep (0.7–1.6×) and the room-recording replay as evidence.
   - Tag the "locks onto the accent pattern" mechanism [INFERENCE].
   - Note that half-time is octave-equivalent.
   - Say what the tempo drives on the plate: the per-beat travel-speed surge (`inhouse_waveform.cpp:349–352`).
   - Report the effective (post-smoothstep) gate, or label the plotted gate "before smoothstep".
4. **Renderer evidence wording.** Replace "not in this build (`build_scalar.py:728`)" with "compiled, discarded at link (`rtthread.map` Discarded input sections; no symbols), not on the in-house dispatch path (`palette_runtime.cpp:403–406, 433`; `emotiscope_port.h:14`)".
5. **Darkness gates (N4).** Trace which gate darkens Dopex post-roll at 76.4–80.2 s, when the silence flag is false (`renderInhouse` admission, `palette_runtime.cpp:258–264`), or tag the sentence [INFERENCE]. Make the gate list match `docs/evidence/P3/agc-trace.html`.
6. **Ceiling (N5).**
   - Restate the packer line: 15,407 is the display stage's full-scale code, reached in 5 lower-board snapshots.
   - Remove "display brightness 0.75" from the ceiling inference, or reconcile it with the follow-up's 1.0.
   - Derive the 12,252 upper ceiling from the palette, or keep it [INFERENCE].
7. **Follow-up consistency (N6).**
   - Name Rec.709 luma as the Q1 metric.
   - Add the follow-up's light × colour decomposition (lower: light ×0.93, colour ×0.68; upper: light ×0.53, colour ×0.46) as the located flattening stage for the dim drop.
   - Replace the record's "[NOT TRACED]" with a link to the trace.
   - Say how the tempo-first proposal relates to the follow-up's colour/fade verdict.
8. **Time-series row (N7).**
   - Put the host in-house confidence (room-recording replay) in BOOM's confidence panel, labelled host.
   - Say on the page which read features the device does not record.
   - Fix the clipped AGC lines and the legends over data.
   - Fix `fig-q4.png`'s title to "in-house AGC".
   - Re-Read every regenerated PNG.
9. **Provenance and numbers (N8).**
   - Compute the Q2 median ratios and the Dopex post-roll medians in a committed script, written to a new JSON.
   - Correct ÷193 → ÷192, ÷13.5 → ÷13.3, Sonic lower ×1.1 → ÷1.1 (median), camera 1–3 dB → 0.7–3 dB.
   - Head the record's iteration-1 body "superseded where it conflicts".
   - Quote the BOOM donor BPM by window (28–55.5 s: 139; 55.5–75 s: 103), if it is quoted at all.
10. **Q3 rendered brightness.** State that the payload snapshots cannot resolve a 0.435 s beat. Add the host-replay payload-luma periodicity (song file and room recording, shift null) as the scoped answer to "is there a 138 BPM periodicity in rendered brightness".
11. **Republish.** Re-render at 1280 px (light and dark) and 400 px, Read the captures, and republish to the same URL. Until then, the published version 3 should not be presented to the Captain as the final P3 answer.
