# D07 audio features: adversarial verification

Verifier: Claude Opus 5.5 subagent, 15 Oct 2026. I only read files. I made no device, serial, USB, flash or commit actions. The only file I wrote is this one.

## Verdict in plain English

The D07 report holds up. I checked 24 of its decision-relevant [FACT] claims against the actual source, evidence receipts and reference repositories. All 24 are supported. I refuted none of them. The numbers are exact where I could recompute them: the source hashes, the tempo constants, the sub-band defaults, the limiter threshold, the RMS dead zone, the GDFT window sizes and the 104.69 Hz true bin centre, the detector hit and miss tables, and BeatNet's 84 ms delay.

Two weaknesses remain. Neither changes a decision:
1. For L11, the claim that the colour mapping was "inherited" is labelled [FACT], but the lines it cites show the v9 pre-knee path, not where the mapping came from. The forensics document supports where it came from, but the report does not cite it at that point.
2. L08 and L13 state that a missing feature is "not an S3 budget compression". These are absence-of-evidence statements. They are correctly labelled [INFERENCE], but nothing cited establishes them.

One identity has moved since the report: Kx-Titan HEAD is now `a92d141`, one docs commit after the reported `2654ff3`, which is an ancestor. The Titan worktree is unchanged: HEAD is `1ef5395`, and all four dirty-file SHA-256 values match exactly.

## Identity checked

- [FACT] Titan worktree HEAD `1ef53952ab7a50af38414321c8a91e515744098a`, branch `lane/titan-lagfree-20261014`. The worktree is dirty, including the four files below.
- [FACT] `shasum -a 256` gives `inhouse_audio.cpp 98f0b748…`, `inhouse_audio.h 309388a8…`, `inhouse_waveform.cpp 0be2360e…` and `inhouse_waveform.h 88cea47b…`. All are identical to REPORT.md lines 19–22.
- [FACT] BeatNet `40336132…`, madmom `27f032e8…` and hybrid-beat-tracker `31981ea7…` match.
- [FACT] Kx-Titan `2654ff3` exists and is an ancestor of the current HEAD `a92d141`.
- [FACT] The arithmetic receipt `~/.local/state/spectrasynq-agents/verification/9c7c3b11…json` exists (2.6 KB). I did not re-run it. I recomputed the arithmetic by hand instead (see below).

## Claims checked

| # | Claim in REPORT.md | Source checked | Result |
|---|---|---|---|
| 1 | Sub-band 40–110 Hz exists; release `enabled=false`, `trigger_sub=false`; accent boost and widen 0 | `sub_band.h:17-18`; `inhouse_waveform.h:52-76` | CONFIRMED |
| 2 | Tempo 60 BPM low, 96 one-BPM bins (60–155), 88 BPM prior σ 0.75, confidence prior 120 BPM σ 0.9 | `tempo_tracker.cpp:17,23-26,508-518`; `tempo_acf.h:12` | CONFIRMED |
| 3 | Comb teeth weights 1, 0.8, 0.7, 0.6 | `tempo_acf.cpp:27-28` | CONFIRMED |
| 4 | Three-hop max aggregation; warm-up 512 updates; 512 × 22.5 ms = 11.52 s | `tempo_tracker.cpp:18,39,577-592`; `tempo_acf.h:11` | CONFIRMED |
| 5 | Challenger must lead by 10% for 5 updates | `tempo_tracker.cpp:170-179` (`*1.1F`, `>=5U`) | CONFIRMED |
| 6 | Flywheel cap 0.10 cycle, frequency pull ±4%, coast 8 beats | `tempo_tracker.cpp:43,44,47` | CONFIRMED |
| 7 | The tracker preserves the S3 V2 path from `c3e13ff…` | `tempo_tracker.h:125-128`; the commit exists in `/Users/spectrasynq/SpectraSynq_K1_Firmware` | CONFIRMED |
| 8 | P7: BOOM 138→69/92, Sonic ≈142→71, Dopex ≈150→75; the prior is the main cause; kick-band novelty alone did not fix it | `docs/evidence/P7/RECEIPT.md:103-147` | CONFIRMED |
| 9 | P7: noise asserts tempo; locks persist through silence; hats-only and −20/−30/−40 dBFS never lock; 326 cases; phase is reported, not gated | `RECEIPT.md:170-177,230-237,328` | CONFIRMED |
| 10 | v1 hit the BOOM file only; v2 hit Carte Blanche only; v3 hit none; all three failed the set | `P6-DROP-DETECTOR/DETECTOR.md:54-61`; `v2/DETECTOR.md:60-70`; `v3/DETECTOR.md:83-91` | CONFIRMED |
| 11 | v2 Sonic sub-bass steps arrived 72 ms and 117 ms late | `v2/DETECTOR.md:7,82-87` (the detector measures 30–90 Hz sub-bass) | CONFIRMED |
| 12 | v3 product nearest-beat errors −380 to +188 ms; reference clock −74 to +65 ms; Carte Blanche −26 ms nearest beat and −454 ms bar downbeat; 10 cancels | `v3/DETECTOR.md:41-48,60-69,93` | CONFIRMED (the correction of the brief's "26–380 ms" is valid) |
| 13 | BeatNet streaming documents 0.084 s delay | `BeatNet.py:157` | CONFIRMED |
| 14 | BeatNet: two unidirectional LSTM layers, 150-unit projection, 3-output head; particle filter 1,500/250 particles, 55–215 BPM, 2–4 beats per bar | `model.py:18-30`; `particle_filtering_cascade.py:108-120` | CONFIRMED |
| 15 | PDM 40 kHz, 296-sample slot, sincdec 49, SINCRNG 10 | `pdm_target.h:13-24` | CONFIRMED |
| 16 | Gain ×16 (Q8 4096), loud-guard ceiling 30,000; limiter engages above raw peak 1,875 | `k1_pdm_sensitivity.h:9-22` (4096·p ≤ 30000·256 ⇒ p ≤ 1875) | CONFIRMED |
| 17 | poly24: 96 taps, 128 phases, 11.25 kHz cutoff, Kaiser β 7; delay 47 source samples ≈ 1.135 ms | `k1_poly24.h:16-26,68-69` (96/2−1 = 47; 47/41417 = 1.135 ms) | CONFIRMED |
| 18 | Q16 RMS dead zone for \|sample\| < 128 (≈ −48.2 dBFS) | `inhouse_audio.cpp:17-18,233-235` (`mul` = (a·b)>>16; (2s)² < 65536 ⇔ \|s\| < 128; 20·log10(128/32768) = −48.16) | CONFIRMED |
| 19 | Spectrum 0.3/0.7 average; Mood 0.05 gives a 1.5 Hz cutoff computed at 100 Hz | `inhouse_audio.cpp:274`, `105-110`; `inhouse_audio.h:36-43` | CONFIRMED |
| 20 | Gate 4× / 2.5× noise, target 0.25, gain 0.1–10, tilt 1.30 below 200 Hz and 0.85 above 3 kHz, knee at 0.5, novelty = sqrt(mean positive flux) | `inhouse_audio.cpp:329-367` | CONFIRMED |
| 21 | Silence: enter 0.04, exit 0.08, 5 s dwell, uses the previous hop's RMS; "reconstruction settings, NOT microphone calibration" | `inhouse_waveform.h:14`; `inhouse_audio.h:29-40`; `inhouse_audio.cpp:255` | CONFIRMED |
| 22 | GDFT windows 40–1,834 samples at 24 kHz; bin 39 = 192, bin 40 = 364; bin 0 rounds to 104.69 Hz | `gdft_goertzel.cpp:18-39,102-131`, recomputed by hand: bin 0 → 24000/(2·6.541) = 1834, round(1834·110/24000) = 8 → 104.69 Hz; bin 39 → 192; bin 40 → 364; bin 79 → 40 | CONFIRMED |
| 23 | P5: in-house ≈ 0.70 ms, AP max ≈ 1.29 ms; 41,408 Hz; 100.01 fps | `docs/evidence/P5/device-budget/BUDGET.md:25,40,~80` | CONFIRMED |
| 24 | v9 tempo weight ≤ 0.7, ramps over confidence 0.30–0.60; level transfer −33 to −9 dBFS, emphasis 1.5; accent uses the lowest 12 bins | `inhouse_waveform.cpp:947-949,983-985,736-741`; `inhouse_waveform.h:49-50` | CONFIRMED |
| 25 | Sub-band commit `1416cb5` is P6 release work | `git log`: "wip(P6 kick): 40-110 Hz sub-band level…" | CONFIRMED |
| 26 | FORENSICS.md:22 says no historical 80-bin per-bin bug was found | `docs/evidence/P6/palette-forensics/FORENSICS.md:22` | CONFIRMED |

## Origin audit (S3 budget versus aesthetic)

The report does not claim "S3 budget" as a fact for any lever. That is the correct stance, and it is the main thing the brief asked verifiers to police.

| Lever | Origin as stated | Assessment |
|---|---|---|
| L01 RMS dead zone | No evidence that it is intentional; it is inherited Q16 arithmetic | Supported. The source comment at `inhouse_audio.cpp:232` names the source's SQ15x16 arithmetic. |
| L02 sub-band | P6 addition, `1416cb5` | Supported. |
| L03 windows | Inherited Cross40; budget origin "plausible but unproved" | Correctly hedged. |
| L04 novelty | `calculate_novelty` preserved | Supported (`inhouse_audio.cpp:365-366`). |
| L06 range and prior | Preserves S3 V2; reason for 88 BPM unknown | Supported. It honestly does not assert budget or aesthetic. |
| L08 downbeat | "[INFERENCE] Missing feature, not … compute-saving compression" | **Flagged.** This asserts a negative with no evidence. Rate the origin as "unknown". |
| L10 silence | "Explicit reconstruction defaults" | Supported (`inhouse_audio.h:29`). |
| L11 chroma | "[FACT] Inherited colour mapping" | **Weak citation.** The cited `A:346-349; V:594-595` show the v9 pre-knee path, not where the mapping came from. Origin is supported elsewhere (`FORENSICS.md:13-20`, SB/K1 lineage), but that is not cited here. |
| L12 timbre | Three-sum order preserved; no evidence for omission | Supported (`inhouse_audio.cpp:361`). |
| L13 sections | "[INFERENCE] … not demonstrated S3 budget debt" | **Flagged.** Same absence-of-evidence pattern as L08. It is labelled correctly, but nothing establishes it. |
| L14 vocal | Not implemented; no evidence of a removed feature | Acceptable. |

## Other observations (not refutations)

- [FACT] `inhouse_waveform.h:47` in the dirty source now selects `kPerPixelMapped`. The report cites `W:42–47` next to PL14-B14 ("default did not select the fix"). That lesson describes an earlier state. The current dirty source does select path P, but the resident image is still unidentified, which the report already says.
- [FACT] P5 notes that "the content stamp advances by 5.000 ms every row" on the measured Emotiscope image (`BUDGET.md:38`). The report's 7.5 ms hop applies to the in-house 24 kHz/180 profile. A future budget should name which profile it means; the report's correction 4 already asks for this.
- [INFERENCE] All cycle costs in the report are labelled [HYPOTHESIS] planning estimates. None should be cited as a measurement.

## Images

Not applicable. The output is a text verification. I checked it against source lines, git objects and evidence documents, and recomputed the numbers by hand.
