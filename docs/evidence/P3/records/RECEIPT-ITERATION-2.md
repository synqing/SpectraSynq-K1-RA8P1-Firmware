# P3 — EXPRESSIVENESS AUDIT — RECEIPT, iteration 2 (answers REVIEW-ITERATION-1.md)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. P3 stays offline and read-only. No flash, readback or bridge was used, and the Titan ran `a8215f75` with v8 autonomous throughout.

The iteration-1 receipt ([`RECEIPT.md`](RECEIPT.md)) stands except where this file corrects it. Its line "the P3 deliverable is complete" is withdrawn; this iteration is submitted for re-review.

- **Page:** https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM (version 3, private). Source: [`P3-CAPTAIN-PAGE.html`](P3-CAPTAIN-PAGE.html). The iteration-1 page is kept as [`P3-CAPTAIN-PAGE.iteration1.html`](P3-CAPTAIN-PAGE.iteration1.html).
- **Record:** [`P3-RECORD.md`](P3-RECORD.md) (iteration-2 section).

## Plain English

The reviewer's numbers check out, and so do mine. The main changes:

1. **Q3 is about the plate, not a board.** The camera cannot tell the two boards apart, so the beat test now describes the plate as a whole.
2. **Sonic's "silence" was not silent at the device.** Its own microphone heard bursts after the music ended, and the board responded to them.
3. **New and decisive: the Tempo board's tempo is wrong.** A host replay of the song file through the product's own analysis shows the tracker reads 69 BPM (half-time) for most of the verse. Through the drop it reads 113, then 92, then 69 BPM. The song is about 138.

   This changes the reviewer's lead. The Tempo board does not read the donor tempo (confidence ≤ 0.25). It reads its own in-house tracker, whose confidence on the song file is mostly high. The fault is the tempo it locks to, not a closed gate.

## Corrections 1–14

| # | Disposition | Evidence |
|---|---|---|
| 1 Q3 attribution | **Done.** Q3 is relabelled everywhere: page card, record and `fig-q3.png` legends and title ("camera band near upper/lower edge … cannot separate the boards"). The board-level claim is withdrawn. Captain question 2 is rewritten. | `analysis/iteration2.json` `A_crosstalk`: bands r 0.866 detrended (0.329 raw). Lower band vs upper/lower payload ρ 0.558/0.362; upper band 0.506/0.347; payloads vs each other 0.676. |
| 2 Sonic silence | **Done.** The Q2 result is restated, and device raw RMS is added to `fig-q2.png` (dotted teal). Captain question 3 is replaced with a room-sensitivity question. | `B_silence_device_level.sonic`: post-roll raw RMS up to 0.161 at 32.1 s, against a music median of 0.127. Silence mean 0.042 (BOOM 0.025, Dopex 0.019). |
| 3 Recorded features | **Done, with a correction to the reviewer** (see below). Donor BPM, confidence and beat-lock are plotted for all three windows (`fig-ts-*.png`). The 0.30–0.60 gate is quoted from the renderer the v8 boards actually use. | `C_donor_features`: confidence max 0.248 / 0.119 / 0.124 (BOOM / Sonic / Dopex). `beat_locked` is never true. |
| 4 Renderer inputs | **Done, with a correction to the reviewer.** Every input is listed. "AGC output the effects read" becomes "in-house AGC output (one of several renderer inputs)". The inference is revised to "brightness weakly follows pre-AGC level, not the AGC output". | Raw-RMS ρ is quoted with p ≤ 0.007 for 4 of 6 board-song pairs. |
| 5 Citation | **Done.** All `product_effect_renderer.cpp` citations are removed. That file is not in this build (`build_scalar.py:728`: `product_output_chain = palette_runtime and not emotiscope_port`). Cited instead: `palette_runtime.cpp:243–288` → `inhouse_waveform.cpp` (mode 32 at 117–265, mode 18 at 334–374, gate at :347), with tempo from `inhouse_audio.cpp:357–359`. | Source. |
| 6 Time-series row | **Done.** `fig-ts-boom.png` (28–75 s), `fig-ts-sonic.png` (6–30 s) and `fig-ts-dopex.png` (0–75 s) each show iPhone audio, device raw RMS, donor VU, in-house AGC, donor tempo confidence with the gate band, and both payloads. | Figures; `analysis/timeseries.json`. |
| 7 Lag | **Done.** The lag is reported as about 0.1–0.27 s, set by the broad r plateau and the 50 ms bins. The AV-map reason is removed. It remains not a latency measurement. | `q3-correction.json` curve. |
| 8 Units | **Done.** One convention per quantity (`E_stage_ranges_db`): 20·log10 for amplitudes; 10·log10 for payload luma and for sRGB-decoded camera levels (approximate). The camera number is one figure everywhere: about 1–3 dB, against 4–11 dB for the payload. `fig-stages.png` is redrawn as grouped bars with no lines joining parallel outputs. | `iteration2.json`. |
| 9 Q1 edge spikes | **Done.** Smoothing now pads with edge values. The figure was regenerated and inspected. | `fig-q1.png`. |
| 10 Gate | **Done.** | `G_q3_gate`. Power (planted, decorrelated light, 0–350 ms lag): 5/20 at 0.1×, 18/20 at 0.2×, 20/20 at 0.3× and 0.5×. Shifted-light false positives: 0/40. Mismatched section: r 0.049, p 0.42, not detected. Recorded red run: `q3-red-run-plant0.json` with log, exit 1 at plant scale 0. Iteration-1 JSONs are not regenerated. |
| 11 Scope words | **Done.** The data is now called "submitted LED payload (pre-emission buffer, not photons)". The snapshot `evidence_level` and the uncovered 24/255 cap are stated. A ceiling check is added. | `F_payload_ceiling`: Sonic upper is within 5% of its maximum (12,252 = 18.7% of full scale) in 77.6% of snapshots; BOOM upper 10.3%, Dopex upper 2.0%. Lower board ≤ 2.1% everywhere. |
| 12 Text fixes | **Done.** "Three songs" throughout. The camera claim is narrowed to silence edges; during music the camera and payload co-vary (ρ 0.35–0.56). Donor-VU-vs-brightness ρ (−0.39…0.07) is stated beside its ×3.93. The hue method is named. | Page and record. |
| 13 Stage evidence | **Done.** Section means lead (raw ×5.58 against AGC ×1.52). AGC in silence: Dopex post-roll median `peak_scaled` 0.68 at raw RMS 0.018, against a music median of 0.69 at 0.096 (`D_agc_in_silence_dopex`). The chart is marked supporting only. | `iteration2.json`. |
| 14 Receipt | **Done.** "Submitted for iteration 2". n per Q1 window: 9–12 snapshots (page eyebrow, record). The page was rendered at 1280 px (light and dark) and 400 px (scrollWidth 400), every regenerated figure was read, and it was republished to the same URL (version 3). | This file. |

## Where this iteration corrects the review (evidence, not opinion)

**F3 and F4: wrong renderer.** The review's renderer citations (`renderWaveformK1`, `renderWaveformTempo`, `product_effect_renderer.cpp`) come from a file that the v8 image does not run.

- **[FACT, source]** In the port build the product output chain is disabled (`build_scalar.py:728`).
- **[FACT, source]** Modes 32 and 18 render through `PaletteRuntime::renderInhouse` → `InhouseWaveformRenderer` (`palette_runtime.cpp:243–288`; `inhouse_waveform.cpp`).
- **[FACT, source]** The in-house renderer has its own mode-18 gate, `clamp01((tempo_confidence − 0.30)/0.30)` then smoothstep (`inhouse_waveform.cpp:347–348`).
- **[FACT, source]** Its tempo, phase and confidence come from the in-house tracker (`inhouse_audio.cpp:357–359`), not from the donor.
- **Consequence.** The recorded donor confidence (≤ 0.25) is not the gate's input, and "if `visual.tempo` is the donor tempo, the gate stays at 0" does not apply.

**Measured instead (new host probe `tests/host/p3_tempo_probe.cpp`).** The probe replays the BOOM source file (`runs/P2-…/permitted/raw/boom-40k-mono.s16`) through the product ASRC, `InhouseAudio` and `InhouseWaveformRenderer`. Output: `analysis/host-tempo-boom.csv`, summarised in `iteration2.json` `H_*`.

| Window (s) | BPM mode (share) | Mean beat gate |
|---|---|---|
| 0–7 | 139 (91%) | 0.89 |
| 7–33 | 69 (99%) | 0.93 |
| 33–43 | 138 (100%) | 0.90 |
| 43–50 | hunting | 0.10 |
| 50–55.5 | 113 (61%) | 0.43 |
| 55.5–57 | 113 (100%) | 0.06 |
| 57–67 | 92 (83%) | 0.92 |
| 67–75 | 69 (100%) | 0.88 |

- **[FACT, host]** For most of the song the gate is open, at a wrong tempo: half-time 69, and 92, which is one 3/8 group of a 138 BPM bar.
- **Scope.** Song file, not the room microphone; host, not M85. The device's in-house tempo is not recorded in the snapshots.

## Commands (all offline)

```
c++ … tests/host/p3_tempo_probe.cpp platform/ra8p1/inhouse_audio.cpp src/k1/core/visual/inhouse_waveform.cpp src/k1/core/audio/{gdft_goertzel,tempo_tracker,tempo_acf}.cpp src/k1/core/visual/product_palette{,_data}.cpp k1_asrc_24k.o   (flags: -std=c++17 -O2 -fno-fast-math -DK1_EMOTISCOPE_PORT=1 -Iplatform/ra8p1 -Isrc -Isrc/k1)
p3_tempo_probe runs/P2-20261014T071112Z/permitted/raw/boom-40k-mono.s16 > analysis/host-tempo-boom.csv
python scripts/p3_q3_alignment.py analysis 0.0 q3-red-run-plant0.json   # exit 1 (recorded red run)
python scripts/p3_iteration2.py analysis                                  # iteration2.json, timeseries.json
python scripts/p3_colour.py analysis && python scripts/p3_plots.py analysis
```

## Limits that remain

- **Not re-recorded on the installed image.** The Titan now runs `a8215f75` (P2) and has not been recorded with music.
- **The device's in-house tempo is unknown.** It is not in the MIR snapshots. The host replay uses the song file, not the room microphone.
- **Ceilings are inferred, not traced.** The upper board's ceiling of about 12,250 codes is consistent with full white × edge gain 0.25 × display brightness 0.75 (`palette_runtime.cpp` `.3 + .7 × brightness`). That chain is [INFERENCE], not traced to the stored controls.
- **No musical verdict.** Agent cannot listen.
