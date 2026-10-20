# P3 — EXPRESSIVENESS AUDIT — RECEIPT (iteration 1)

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. P3 is offline analysis of existing recordings. It made no flash, readback or bridge use, and the Titan kept running `a8215f75` throughout.

**Status claimed for review:** the P3 deliverable is complete. The imagery-led Captain page is published, and the .md record is written. Evidence level is *demonstrated on recorded data* for build `9ab90873` (14 Oct, before P2), not for the installed P2 image. Agent cannot listen.

## 1. Deliverables

| Item | Path / link | sha256 |
|---|---|---|
| Captain page (published, private) | https://claude.ai/artifact/DhgaZNnZhG1BFSEBKVChKM (version 2) | publish copy has the figures inlined |
| Page source | `P3-CAPTAIN-PAGE.html` (figures from `analysis/`) | `424694098760b515e3e7131012a2a55b04979cf6eafb0326c09fa662c0b634c9` |
| Record | [`P3-RECORD.md`](P3-RECORD.md) | — |
| Data | `analysis/p3-data.json` | `6861f69b14fb6038dfde031c948bdb18e0c47e478a5e2e405b7db6ae3566b12a` |
| First answers (unaltered) | `analysis/p3-answers.json` | `3f3edd6ba2b9b6b66f76921162eb73d093b4a55a24af7b3bdc8621a71023edaf` |
| Q3 correction, build-vs-drop, level tracking | `analysis/q3-correction.json` | `68d097b738693a81292aa75556892eb7ba3b90d224dbe9bac10cf320ffa30ea1` |
| Packer clipping | `analysis/packer-clipping.json` | `5d232d6504d93a31cf030cccd3b613e82fe19559d46432e853538c96ceffcac7` |
| Figures | `analysis/fig-q1..q4.png`, `fig-stages.png`, `fig-colour.png` | `p3_plots.py`, `p3_colour.py` |
| Colour stats | `analysis/colour.json` | — |
| Scripts | `scripts/p3_expressiveness_audit.py`, `scripts/p3_q3_alignment.py` (new), `scripts/p3_colour.py` (new), `scripts/p3_plots.py` | committed on `lane/titan-lagfree-20261014`: `7b5106c` (audit + first plots), `abc72d6` (Q3 correction + fixes) |

## 2. Inputs

The three 14 Oct recordings under `_scratch/waveform-migration-001/canonical-audio-v01/`:

- `boom-admission-fix-candidate-001`
- `sonic-6-30-admission-fix-candidate-001`
- `dopex-admission-fix-holdout-001`

Each has `original.mov`, the wire `sample-*` snapshots, `observation.json` and `av-validation.json`. The lower board ran Hybrid K1 32 and the upper board ran Tempo 18 with Crimson Gold.

## 3. Results

See the record. The findings that reached the Captain:

- **Q1:** at the drop, the light rises only ×1.39/×1.31 against +8.8 dB of audio. Against the build, the drop is ×0.63/×0.24 as bright while 6.0 dB louder.
- **Q2:** silence dims on the wire, except the lower board after Sonic. The camera shows the opposite because of auto-exposure.
- **Q3:** the music has a 3+3+2 rhythm at 138 BPM. The light does not repeat. The lower board shows weak onset-following (r 0.185, p 0.020); the upper board shows none (p 0.30).
- **Q4:** AGC output medians are 0.736–0.775 across songs, and the board differences do not follow loudness.
- **Stage:** the AGC halves the dB range.
- **Level tracking:** brightness is uncorrelated with the AGC output (ρ −0.06…0.12 for five of six board-song pairs).
- **Colour** (the brief asks for brightness and colour): the lower board is red in all three songs (hue 356–359°, concentration ≥ 0.976). The upper board is 352° / 20° / 333° (BOOM / Sonic / Dopex); see `fig-colour.png`. Donor VU rises ×3.93 at the drop, against ×1.52 for the in-house AGC.

### Gate evidence

- `p3_q3_alignment.py` exits 1 if the planted positive control is not detected. That control is the onset envelope at 0.5× the band's standard deviation, delayed 100 ms. It was detected on both bands at p 0.001, so `controls_ok: true`.
- The circular-shift null preserves each signal's own autocorrelation and breaks only the alignment between them.

## 4. Corrections made during P3

The first draft is still in `p3-answers.json`; this iteration corrects it:

1. **The Q3 "chance level" is retired.** The draft's phase-randomised surrogates keep the power spectrum, and with it the autocorrelation, so the comparison was vacuous. The audio's 0.260 against a "p95" of 0.260 was the tell.
2. **The Q3 claim "the light shows no pulse" is withdrawn.** The lower board shows weak onset-following.
3. **The Q1 framing changed.** "The light barely moves" became "the drop renders dimmer than the build", from wire data over 45–50 s against 55.5–60 s.
4. **The P3 lead "AGC is the flattening stage" is narrowed.** The AGC compresses section loudness, but brightness barely tracks even the AGC output. That makes an AGC-only fix unlikely to suffice (inference).

## 5. Not done and limits

- **Not re-recorded on `a8215f75`.** The installed P2 image has not been recorded with music, and the P2 host tolerance gate failed, so equivalence of rendered output with `9ab90873` is not claimed.
- **Wire snapshots are about 0.3–0.5 s apart.** That is too sparse for beat-level analysis, so Q3 uses the camera, which auto-exposes.
- **The 267 ms onset lag is not a latency measurement.** It includes AV-alignment uncertainty; latency is P5.
- **The features each effect reads are not traced.** The brief asks for "the features each Waveform effect actually reads". Plotted are the in-house level/AGC state and the donor VU, both computed every hop. Which of them each v8 effect consumes, frame by frame, is not traced. Correlation is not causation; the host trace is a PROPOSAL — NOT APPROVED.
- **Window choice.** Q4 uses Dopex 28–75 s to match BOOM. Dopex 0–75 s, the brief's window, is plotted in Q2 and in the colour figure.
- **No musical verdict.** Agent cannot listen. Three Captain questions are on the page.

## 6. Commands

```
/Users/spectrasynq/miniforge3/bin/python scripts/p3_expressiveness_audit.py --out _scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis
/Users/spectrasynq/miniforge3/bin/python scripts/p3_q3_alignment.py _scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis   # rc 0, controls_ok true
/Users/spectrasynq/miniforge3/bin/python scripts/p3_colour.py _scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis
/Users/spectrasynq/miniforge3/bin/python scripts/p3_plots.py _scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis
```
Commits: `7b5106c`, `abc72d6`, `2442955`.

## 7. Visual inspection

- Every figure was opened and read after each regeneration. Fixes made: the Q1 annotation overflowed, the Q3 legend overlapped a label, and the stage title was cut off.
- The page was rendered headless at 1280 px in light and dark themes and at 400 px. It has no horizontal scroll at 400 px (scrollWidth 400).
- Screenshots: `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/shot-*.png`. These are temporary; the page itself is the durable surface.
