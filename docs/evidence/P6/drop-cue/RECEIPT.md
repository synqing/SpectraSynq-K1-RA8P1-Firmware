# P6 drop cue feasibility — 15 Oct 2026 (lane owner, by hand)

PROPOSAL — NOT APPROVED. Host research only. Nothing was flashed.

## In plain words

1. **The drop labels were early.** The hand labels are written to 0.1 s. Measured in the audio itself, the bass of each drop lands:
   - BOOM 55.7: 66 ms after its label;
   - Sonic 27.7: 42 ms after;
   - Sonic 54.7: 76 ms after;
   - Carte 27.4: 42 ms after.

   The kick band agrees to within about 10 ms.
   - The earlier detectors were scored on a [0, +50 ms] window after those labels. For Sonic 54.7 no real-time detector could ever have passed: the bass arrives after the window closes.
   - "The sub-bass lands 70–120 ms late" (v2) was mostly this label error.
2. **With honest anchors, the live drop cue still does not exist in loudness.**
   - Five causal features were measured: bass, kick and broadband loudness jumps against the previous 1–2 s. None puts every drop above everything else.
   - In the source files each drop does jump by 10–17 dB. So do other moments:
     - BOOM's climax at 47.1 s, which jumps further than the drop;
     - BOOM 34 s;
     - Carte 13.8 s;
     - Sonic 19 s and 23 s.
   - In the two room recordings, which are what the Titan's microphone hears, the drop's bass is no louder than the build's pulses in the second before it.
3. **Consequence.**
   - A live trigger must know where it is in the music (a drop lands on the first bar of a phrase after a build), not just hear a loud bass hit. That is the tempo-predicted downbeat, and it depends on the P7 tempo fix.
   - The existing v2 sub-bass detectors were re-scored, unchanged, on the measured anchors. Each still hits only Carte, because their "quiet bass floor for 1 s" gate never opens in BOOM or Sonic.
4. **Side effect for the release on the host.**
   - EXT-v3, HOLD and HOLD-LIVE place the release at the hand labels. On the host the release therefore reaches full extension about 12–66 ms before the bass is heard.
   - Any re-score of those criteria should use the measured anchors. The frozen files are not edited.

## Ruler

- Script: `scripts/p6_drop_cue_probe.py reference`.
- Method: zero-phase 30–110 Hz band, 10 ms level at a 2 ms step. The sustained step (median over the next 150 ms minus the median over the previous 150 ms) is searched in label ±250 ms; the anchor is the half-rise frame.
- Error: 40 planted steps at known times, including chopped 60 Hz build pulses and attacks up to 20 ms, read 1.2–17.7 ms early (mean −7.1 ms) and never late. An early anchor makes a [0, +50 ms] window stricter, not looser.
- Figure: `reference-onsets.png`, opened and read. The gold band is the old window; the bass steps sit at its right edge or beyond it.

## Frozen before results

| File | Frozen (UTC) | Body sha256 |
|---|---|---|
| `runs/P6-DROP-DETECTOR/v4/DETECTOR-CRITERIA-v4.json` (v2 rule, anchors replaced by the measured onsets, plus a label-anchored plant) | 2026-10-14T22:28:32Z | adc04962… |
| `CUE-SEPARABILITY-FROZEN.json` (feasible = every drop ≥ 3 dB above every non-drop hop) | 2026-10-14T22:29:44Z | 7ed25462… |

## Results

**v4 planted faults all go red:**
- always-on;
- 200 ms late;
- broadband loudness;
- **fires exactly on the old labels** (now collar fires, 0 hits).

**v4 re-score** of floor_step, ratio_gate and tension_release (v2 constants unchanged):
- 1 of 6 drops hit (Carte), 0 false, 0 collar fires;
- no fire at all near BOOM or Sonic.

**Separability margins** (the minimum drop value minus the worst non-drop value; a feature needs ≥ 3 dB):

| Feature | Margin | Worst non-drop |
|---|---|---|
| F1 bass vs 1 s median | −72.4 dB | song start, silence to sound |
| F2 bass vs 1 s max | −28.1 dB | BOOM 47.09 s, +21.7 |
| F3 kick vs 1 s max | −27.8 dB | BOOM 47.09 s |
| F4 broadband vs 1 s max | −25.8 dB | BOOM 31.40 s |
| F5 bass vs 2 s p90 | −27.9 dB | BOOM 47.12 s |

- Room drops sit at −6.5 dB (BOOM room) and −0.8 dB (Sonic room) on F2.
- Planted anchor faults (−100 ms, −10 s): both "not feasible". This is uninformative, because the measured result is already infeasible, so the plant cannot discriminate.
- Figure `separability-F2.png`, opened and read:
  - The source files show the drop spikes, along with rival spikes of the same size.
  - In the room traces the drop does not stand out.

## Next decisive observation

The tempo-predicted downbeat candidate cannot be measured until the P7 tracker locks the right tempo; it is at 35/326 on GATE-SYNTHETIC-v1 and needs the Captain's corpus. The smallest decisive observation that does not need P7 is in two parts:
- **Does a build show up in the room recordings?** That is seconds-scale high-band rise plus bass absence. If it does, a "build state" can gate a bass-return trigger.
- **Does a bass return after a detected build separate the drops from BOOM 47.1 s?** That instant is labelled a climax, not a drop.

Freeze that test before measuring it.
