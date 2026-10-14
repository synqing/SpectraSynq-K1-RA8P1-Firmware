---
abstract: "P7 steps 1 and 2 receipt (14 Oct 2026, host only): diagnosis of the Titan tempo half-time / 3:2 lock, the frozen synthetic gate, its planted-fault proof and the current tracker's baseline (0 of 326 pass), plus the unfrozen real-corpus proposal. No product code changed; no device touched."
---

# P7 — tempo half-time / 3:2 lock: steps 1 and 2 receipt

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

Host-only work:

- no flash, broker, serial, USB, E2 Lite or `launchctl`;
- no commit and no push;
- the product source tree (`src/`, `platform/`, `tests/`, `scripts/`) is unchanged (`git status` clean for those paths).

The Titan was not touched.

## In plain English

**What causes the lock.** The tracker has a built-in preference for 88 BPM. That preference is what drags it to half time (69) and, on BOOM, to two-thirds time (92).

- Remove or re-centre that preference on the host and BOOM reads 138 in the intro (100 % of updates) and in the drop (82–92 %). This holds at three replay levels and on the room recording.
- Changing only what the tracker listens to (a kick-band input instead of the current one) does not fix it. The intro stays at 69, and 92 still appears in the drop.
- The same preference halves Sonic Samba (71 instead of about 142) and Dopex (75 instead of about 150).
- On clean synthetic four-on-the-floor, every tempo from 140 to 200 BPM is reported at half.

**The 3:2 idea is only partly right.**

- *Right part.* In BOOM's drop, about 40 % of the 92 BPM score comes from the dotted-quarter gap of the 3+3+2 pattern. That pushes 92 just past what the preference needs to win.
- *Wrong part.* The music has no actual 92 BPM pulse. And no synthetic 3+3+2 pattern I built produced 92; they all produced 69.
- So the 3+3+2 pattern contributes on BOOM, but the preference is the necessary cause.
- Removing the preference alone is not enough either. With it removed, a bar-length pattern with no four-on-the-floor kick still reads 69. Step 3's explicit beat-level stage is still needed.

**The gate is frozen and works both ways.**

- It covers 326 generated cases with known answers. The criteria are frozen with the tool, at 12:25:06Z, before the tracker ran on any case.
- It goes green on a "perfect tracker", and red on half-time, 3:2, late-lock and pad false-lock faults. Ten mutated versions of the checker are all caught by its tests.

**The current tracker fails all 326 cases.** It can never lock in under about 10.5 s, because of a 512-update warm-up. It also:

- halves everything from 140 BPM upward;
- shows a confident tempo on near-silent noise and on drumless pads;
- stays locked through silence gaps;
- never locks on hats-only or quiet (−20 dBFS and below) input, because its own silence detector zeroes the input.

**Real corpus.** Proposal only (`CORPUS-PROPOSAL.md`). Seven of the twelve slots need tracks from the Captain (the list is in that file).

**Left:** the Captain's rulings below, then step 3 (design) and step 4 (host gate green, then device). Neither is started.

## What ran (exact commands, in order)

Worktree `…/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `7e1d69b`. Run folder: `_scratch/waveform-migration-001/runs/P7-20261014T121412Z/permitted/` (below, `R`). Python is `/Users/spectrasynq/miniforge3/bin/python` (numpy 2.3.3, scipy 1.16.3, pandas 2.2.3, librosa 0.11.0). Compiler: host Apple clang 17.

1. **Probe smoke test (pre-freeze, real file only).** Build `R/src/p7_tempo_probe.cpp`; run it on `runs/P2-…/raw/boom-40k-mono.s16` at unity gain. This was the only tracker run before the freeze. It touched no synthetic case.
2. **Host tests, mutation check and gate creation:**
   - `python -m pytest -q src/test_p7_gate.py` → 25 passed;
   - `python src/p7_mutation_check.py gate/mutation-check.json` → exit 0, 10 of 10 mutants killed (12:23:20Z);
   - `python src/p7_make_gate.py gate/GATE-SYNTHETIC-v1.json gate/truth` → 326 cases.
3. **Freeze:** `python scripts/freeze_tolerance.py freeze R/gate/GATE-SYNTHETIC-v1.json --status …/runs/STATUS.md --lane "P7 tempo gate"`.
   - Stamped `frozen_utc 2026-10-14T12:25:06Z`, body sha256 `44dd47aa…`, file sha256 `7b521d03…`, mode 0444.
   - The STATUS line was written by the tool.
4. **Dry verify, no tracker:** `python src/p7_run_gate.py verify gate/GATE-SYNTHETIC-v1.json` → `{"verified": true}`. It regenerates all 326 PCM and truth files and matches the frozen hashes.
5. **Planted faults through the real CLI**, for each of 9 traces: `python src/p7_plant.py write gate/GATE-SYNTHETIC-v1.json gate/truth <tmp>/<f> <f>`, then `python src/p7_gate.py score gate/GATE-SYNTHETIC-v1.json gate/truth <tmp>/<f> gate/planted/<f>.json`. The table below gives each exit code.
6. **Baseline, the current unmodified tracker:** `python src/p7_run_gate.py run gate/GATE-SYNTHETIC-v1.json baseline` → exit 1 (12:27:40Z).
   - It writes `baseline/gate-result.json`, `baseline/probe-log.json` and `baseline/traces/*.csv`.
   - Then `freeze_tolerance.py verify` → `{"ok": true}`: the criterion is unchanged, the file is read-only, and the results postdate the freeze.
7. **Step 1:**
   - `python src/p7_step1_diagnose.py` → `diagnosis/step1-summary.json`, `diagnosis/csv/*.csv.gz` (12:27:14Z);
   - `python src/p7_threetwo.py` → `diagnosis/threetwo.json` (12:28:29Z);
   - `python src/p7_accent_sweep.py` → `diagnosis/accent-sweep.json` (12:33:52Z, second run; see the disclosure in Step 1, item 6);
   - `python src/p7_figures.py`.
8. **Corpus:** `python src/p7_corpus_scan.py corpus/corpus-scan.json`.
9. **Close-out checks:**
   - `freeze_tolerance.py verify` with the baseline, planted and step-1 results → ok;
   - `rhythm_evidence.py selftest` → `all_red_as_expected: true`;
   - pytest 25 passed.

**Probe parity.** Every probe run asserts that the probe's external tracker copy equals the product tracker inside `InhouseAudio` on every update (BPM and confidence, bit for bit). A mismatch would mean exit 9. Every run reported `parity_mismatches: 0` (`diagnosis/step1-summary.json`, `baseline/probe-log.json`).

## Step 1 — reproduce and diagnose (host, 24 kHz)

**Chain** (as installed `f2f1983a`, `k1_live_runtime.cpp:52–53`):

- 40 kHz int16 → `k1_poly24` → `InhouseAudio` with `analysis_rate_hz 24000`, 180-sample hops;
- tempo demand on board 1;
- the product `tempo_tracker.cpp` / `tempo_acf.cpp`, with novelty updates at 44.44 Hz.

**Inputs, at device-calibrated level.** BOOM gains are from the committed `docs/evidence/P3/agc-trace-meta.json`. Sonic and Dopex are matched to the device's in-house raw-RMS medians from P3 (0.1271 and 0.1181).

- BOOM file (×1.155, plus −6 dB and +6 dB) and BOOM room (×89.6);
- Sonic file (fixture `sonic-samba.wav`) and Sonic room (6–30 s slice);
- Dopex file and Dopex room.

**Host-only probe switches** (`p7_tempo_probe.cpp`; nothing in the product source changed):

- `prior_flat`;
- `prior_120`;
- `gdft_low` (flux of GDFT bins below 200 Hz);
- `pcm_kick` (log-energy rise below 150 Hz from the PCM);
- `flat_kick`.

### Findings

1. **[FACT, code]** The tracker cannot reach 156–200 BPM.
   - Its range is 60–155 BPM in 96 one-BPM bins (`tempo_tracker.cpp:17,508`; `tempo_tracker.h:12`).
   - The prior is a log₂-Gaussian centred on 88 BPM, σ 0.75 octave (`:23–24,514–515`). Prior weights: 69 → 0.896, 92 → 0.996, 138 → 0.688.
   - Therefore 92 beats 138 whenever comb(92)/comb(138) > 0.690, and 69 beats 138 whenever comb(69)/comb(138) > 0.767. This ignores the switching hysteresis (a challenger must lead by 10 % for 5 updates, `:170–186`).
   - Selection is the argmax of `acf.comb_salience × tempo_prior` (`:135–143`). The comb takes 4 teeth at 1–4 × the period, weights 1, 0.8, 0.7, 0.6 (`tempo_acf.cpp`).
2. **[FACT, code]** Locking is impossible for the first 512 updates, which is 11.52 s at 24 kHz/180/3 (`kConfidenceWarmupUpdates`, `:39,242,250`).
   - Measured on the host: first lock 10.53 s after music starts, in every one of the 315 synthetic cases that locked at all.
3. **[FACT, code]** The tempo novelty is blind below 110 Hz, where a kick's fundamental lives.
   - The GDFT bank's lowest bin is 110 Hz at 24 kHz (printed from `makeGdftConfiguration`).
   - The novelty is a positive flux of a mood-low-passed (about 1.5 Hz cutoff) spectrum (`inhouse_audio.cpp` `condition()`).
4. **[FACT, host] The prior moves the lock; the novelty input does not.** BOOM, share of tracker updates by reported BPM (±1):

| Input | Section | Product prior | Flat prior | Prior centred 120 | Kick-band novelty, product prior |
|---|---|---|---|---|---|
| file (calibrated) | intro 12–28 s | 69: 100 % | 138: 100 % | 138: 100 % | 69: 100 % |
| file (calibrated) | drop 55.5–75 s | 69: 60 %, 92: 14 %, 138: 18 % | 138: 86 % | 138: 87 % | 92: 35 %, 138: 51 % |
| file −6 dB | drop | 92: 93 % | 138: 82 % | 138: 86 % | 92: 35 %, 138: 51 % |
| file +6 dB | drop | 69: 63 %, 92: 12 % | 138: 92 % | 138: 92 % | 92: 37 %, 138: 49 % |
| room (calibrated) | drop | 92: 59 %, 69: 34 %, 138: 0 % | 138: 84 % | 138: 90 % | 92: 33 %, 138: 39 % |

   - The GDFT low-band novelty makes it worse: 92 for 88–96 % of the drop, and it never locks.
   - **[FACT, host]** Sonic file: the product reads 71 (81 %); a flat prior reads 142 (91 %). Dopex file 28–75 s: the product reads 75 (90 %); a flat prior reads 150 (89 %).
   - The room recordings show the same pattern. Sonic room: the product reads 71 (64 %); a flat prior reads 142–144 (75 %) and 71 (25 %). Dopex room: 75 (83 %), and 151 (84 %) with a flat prior.
   - The true level of these two songs is unconfirmed (`CORPUS-PROPOSAL.md`).
   - Figure: `diagnosis/fig-boom-variants.png` (inspected).
5. **3:2 inference.** The claim tested: 92 = ⅔ × 138 is the dotted quarter of BOOM's 3+3+2. Tools: `p7_threetwo.py`, `rhythm_evidence.py` (selftest red as expected), a numpy replica of the comb, and the C++ comb (replica max |error| ≤ 1e-5).
   - **[FACT, host] Supported, as a contributor.**
     - In BOOM's drop the autocorrelation of the tracker's novelty is +0.37 (file) and +0.42 (room) at 1.5 beats of 138.
     - At the 1.25- and 1.75-beat controls it is −0.07 to −0.12.
     - Dopex, a control song, shows −0.03 and −0.04; plain synthetic four-on-the-floor shows −0.26.
     - The 1.5-beat tooth supplies 39–40 % of comb(92). That lifts comb(92)/comb(138) to 0.73 (file) and 0.77 (room), above the 0.690 needed.
   - **[FACT, host] Not supported, as a periodicity.**
     - `line_test` on the novelty: ratio at 1.533 Hz (92 BPM) 0.54 (file) and 0.57 (room), below every off-grid control (maxima 0.87, 1.00).
     - The beat line at 2.30 Hz scores 5.3–5.4.
     - Reach check: a polymetric control with a real dotted-quarter pulse scores 18.4 at 1.533 Hz.
     - Disclosure: that control's own off-grid controls reach 25.8, through leakage from its 0.767 Hz grid. `line_test`'s formal flag is therefore False for it too. The 18.4 shows reach, not a formal pass.
     - Conclusion: 92 is an interval inside each bar, not a pulse in the music.
   - **[FACT, host] Not reproduced synthetically** (`diagnosis/accent-sweep.json`). At 138 BPM, every pattern returns 69 with the product prior (100 % of updates, 13–31 s):
     - a 3+3+2 stab over a four-on-the-floor kick, stab/kick ratio 0 to 4;
     - stab only;
     - a 3+3+2 kick with snare on 2 and 4.
   - **[INFERENCE]** On BOOM the 3+3+2 interval tips 69 into 92 only together with BOOM's particular onset spectrum and level. 92 against 69 changes with replay gain (PL14-C8). The prior is the necessary cause; the 3+3+2 is a contributing one.
6. **[FACT, host] A flat prior alone does not deliver the kick/backbeat level.** Stab-only 3+3+2 and the 3+3+2 kick with snare on 2 and 4 read 69 with a flat prior too.
   - Disclosure: the "kick 0.5, stab 4" case read 69 under a flat prior on the first sweep run and 138 on the second. The only difference was the draw order of the noise seeds (a snare template was added). It is borderline, so it is not counted either way.
   - **[INFERENCE]** Step 3 needs the explicit metrical-level stage. Re-centring the prior is not a guarantee.

## Step 2(a) — synthetic gate (frozen)

### The suite

`src/p7_synth.py` generates 326 cases, seeded per case id. The truth is the beat list the generator placed. Bit-reproducibility is tested, and PCM and truth sha256 are frozen per case.

| Family | Cases |
|---|---|
| four-on-the-floor, 55–200 BPM, 1 BPM steps | 146 |
| the same with a 3+3+2 stab overlay | 146 |
| half-time (snare on beat 3) at 100/120/140/150/174 | 5 |
| swung eighths (2:1) at 90/110/128/160 | 4 |
| hats only at 100/128/174 | 3 |
| drumless chord-change pads | 3 |
| tempo steps ±8 % at 120 and 140; ×2 at 80 and 90; ×½ at 160 and 180 | 8 |
| ±5 % rubato ramps at 90/120/140 | 3 |
| silence gaps of 2/5/10 s | 3 |
| levels −40/−30/−20/−10/0 dBFS (music peak; −72 dBFS RMS noise floor everywhere) | 5 |

### Definitions

Frozen in `gate/GATE-SYNTHETIC-v1.json`; implemented in `src/p7_gate.py`.

- **Asserting** = `locked`, or confidence ≥ 0.30. 0.30 is the renderer's tempo gate floor, `inhouse_waveform.cpp:347`.
- **LOCK** = every update from (start + grace × beat) to the segment end is locked and within ±2 %. Grace is 4 beats for an acquisition and 2 beats after a step.
- **BPM** = every asserting update after grace is within ±2 %.
- **FLIP** = zero locked updates at ×½, ×2, ×⅔, ×3/2, ×⅓, ×3, ×¾ or ×4/3 that are not correct. The only exemption is holding the previous tempo inside a step's 2-beat grace.
- **LOWCONF** = no asserting update on the pad, the lead-in, or a silence gap after 2 beats of release.
- **Phase** is reported, not gating.

### Proof that it can fail

- **Tests:** `src/test_p7_gate.py`, 25 tests. Among them:
  - truth checked against independent arithmetic and against kick onsets detected in the audio;
  - every planted fault going red;
  - the real CLI entry point;
  - refusal of an edited scorer or an unfrozen gate;
  - the ±2 % boundary.
- **Mutation check:** `gate/mutation-check.json` has 10 mutants; all 10 make pytest fail. They are:
  - FLIP disabled;
  - acquire grace 8 beats;
  - step grace 4 beats;
  - release grace 20 beats;
  - assert threshold 0.6;
  - tolerance widened 10 %;
  - LOWCONF ignored;
  - missing trace passes;
  - truth and audio shifted 10 ms;
  - audio shifted 2 ms.

### Planted faults through the frozen gate

`gate/planted/*.json`, via the CLI:

| Trace | Exit | Cases passing | Criterion that went red |
|---|---|---|---|
| oracle (perfect tracker) | 0 | 326 / 326 | none |
| half-time output (×½) | 1 | 3 / 326 (the pads) | FLIP, LOCK, BPM on all 323 tempo cases |
| 3:2 output (×⅔) | 1 | 3 / 326 | FLIP, LOCK, BPM on 323 |
| late lock (6 beats) | 1 | 3 / 326 | LOCK on 323; BPM and FLIP stay green |
| false lock on the pad | 1 | 323 / 326 | LOWCONF on the 3 pads |
| slow re-acquire (3 beats) | 1 | 318 / 326 | LOCK on the 8 steps |
| one ×3/2 update | 1 | 3 / 326 | FLIP and LOCK |
| locked through silence | 1 | 323 / 326 | LOWCONF on the 3 gaps |
| unlocked but asserting ×⅔ | 1 | 3 / 326 | BPM; FLIP stays green |

### Baseline: the current, unmodified tracker

`baseline/gate-result.json` (sha256 `e70f6db5…`), exit 1. **0 of 326 cases pass.**

| Criterion | Pass / fail |
|---|---|
| LOCK | 0 / 323 |
| BPM | 131 / 192 |
| FLIP | 173 / 150 |
| LOWCONF | 0 / 326 |

**[FACT, host]** Why it fails:

- **LOCK fails everywhere.** The first lock comes 10.53 s after music starts (the warm-up), against a 4-beat target (1.2–4.4 s).
- **Half-time from 140 BPM.** On clean four-on-the-floor, the locked tempo is correct from 60 to 139 BPM (up to 1 BPM low in places) and half from 140 to 200. 55–59 BPM read double. 134–138 BPM never lock and sit at about 67–69 with confidence of about 0.57.
- **Flips.** 140 cases flip to ×½ and 10 to ×2. There are no ×⅔ flips: the synthetic 3+3+2 family does not trigger the 92 mode in this tracker (see the coverage gap below).
- **Half-time and swing patterns** at 120 BPM and above are reported at half (60, 70, 75, 87, 64 and 80).
- **Steps.** Never re-acquired within 2 beats. ×2 steps stay at the old tempo; ×½ steps settle at the new tempo only because the old tempo was already reported at half.
- **LOWCONF fails on all 326.**
  - On the −72 dBFS noise lead-in, confidence reaches ≥ 0.30 by 0.55 s, reporting 155 BPM in all 326 cases: the in-house conditioning normalises the noise up.
  - The pads assert on 53, 179 and 153 of their 1,399 updates.
  - The tracker stays locked through silence gaps: 100 % of the 2 s gap, 71 % of 5 s, 64 % of 10 s.
- **Never locks** on hats-only and on −20, −30 and −40 dBFS. The in-house silence flag (RMS below 0.04 for 5 s, exit at 0.08) sets at 5.0 s and zeroes the tracker's input for 90 % of the case.

Figure: `baseline/fig-baseline-fotf.png` (inspected).

**Gate coverage gap (disclosed).** The frozen synthetic suite does not reproduce the 92 (3:2) lock on the current tracker. For this tracker, 3:2 coverage comes only from:

- the planted-fault proof that the scorer catches ×⅔;
- BOOM in the real corpus.

A synthetic case that reproduces BOOM's 92 would be a narrowing re-freeze. RULING NEEDED, item 4.

## Step 2(b) — real corpus

`CORPUS-PROPOSAL.md` is marked PROPOSAL — NOT APPROVED and is **not frozen**.

## Files

Paths are under `R`; all sha256 are in `SHA256SUMS.txt`. Its last line hashes this receipt.

- **Code:**
  - `src/p7_tempo_probe.cpp`, the probe;
  - `src/p7common.py`;
  - `src/p7_synth.py`, the generator;
  - `src/p7_gate.py`, the scorer;
  - `src/p7_plant.py`;
  - `src/p7_run_gate.py`;
  - `src/p7_make_gate.py`;
  - `src/test_p7_gate.py`;
  - `src/p7_mutation_check.py`;
  - `src/p7_step1_diagnose.py`;
  - `src/p7_threetwo.py`;
  - `src/p7_accent_sweep.py`;
  - `src/p7_corpus_scan.py`;
  - `src/p7_figures.py`.
- **Gate:**
  - `gate/GATE-SYNTHETIC-v1.json` (frozen; file sha256 `7b521d03dcf606f07b0978c428424b929a12d2a1e82a7045bce3ccbf679653b5`);
  - `gate/truth/*.truth.json` (326);
  - `gate/mutation-check.json`;
  - `gate/planted/*.json`.
- **Baseline:**
  - `baseline/gate-result.json`;
  - `baseline/probe-log.json`;
  - `baseline/freeze-verify.json`;
  - `baseline/traces/*.csv` (326; 67 MB, scratch);
  - `baseline/fig-baseline-fotf.png`.
- **Diagnosis:**
  - `diagnosis/step1-summary.json`;
  - `diagnosis/threetwo.json`;
  - `diagnosis/accent-sweep.json`;
  - `diagnosis/csv/*.csv.gz`;
  - `diagnosis/fig-boom-variants.png`.
- **Corpus:** `corpus/corpus-scan.json`; `CORPUS-PROPOSAL.md`.
- **Not stored:** generated PCM (deterministic, regenerated by `p7_run_gate.py`) and the BOOM, Sonic and Dopex replays. Both live in `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p7/`.
- **Not committed:** everything here is untracked scratch. Per PL14-D4, the lane owner copies the evidence to `docs/evidence/P7/` with SHA256SUMS and commits. The tools may move to `tests/host/` and `scripts/`; the gate's tool hashes pin the run-folder copies.

## Not verified on hardware, and why

Everything above is a host oracle: Apple clang, not the M85 binary. Device behaviour is not claimed.

- The installed image's tempo path matches this source by reading only (`k1_live_runtime.cpp:52–53`, `inhouse_audio.cpp:121–123,289–291`).
- Bit-exact host-to-M85 parity for the tracker is not re-measured here.
- Words used: **configured** (the gate), **demonstrated** (planted faults and baseline, host). Nothing is **qualified**.

## Ship path for P7 (Captain ruling: any gate failure is no-ship)

1. **Done, on the host:**
   - diagnosis;
   - the synthetic gate, frozen and proven red and green;
   - the current tracker's baseline: 0 of 326, which is no-ship;
   - the corpus proposal.
2. **Remaining:**
   - (a) the Captain rules on the corpus and the definitional items below;
   - (b) freeze the corpus gate (tuning and held-out) with `freeze_tolerance.py`;
   - (c) step 3: design the metrical-level stage, plus warm-up and silence fixes (agent);
   - (d) step 4: host gate 100 % on the synthetic suite and both corpus sets, then the M85 build;
   - (e) device programming and readback by the lane owner, with the rtrace / instrument check.
3. **Who acts:** the Captain for (a). The lane agents for (b)–(e).
4. **What means shipped:** an image whose tracker passes both frozen gates at 100 % on the host and on the device, programmed with the identity-checked deploy, and admitted with a `mir-snapshot` behaviour proof.

## RULING NEEDED (Captain)

1. **Corpus tracks and annotations.** See `CORPUS-PROPOSAL.md` → RULING NEEDED:
   - supply the missing slots;
   - confirm the BPMs: BOOM 138, Sonic about 142–144, Dopex about 150–152 (uncertain), Carte Blanche about 140, PR — Happy Daze about 122, Grants — PunchDrunk about 98–99;
   - approve the split and the room recordings.
2. **Definitions frozen in the synthetic gate.** Changing any of these is a re-freeze, and loosening one would widen the gate.
   - (a) "Asserting" = locked, or confidence ≥ 0.30.
   - (b) LOCK must hold to the end of the segment, not just be reached.
   - (c) Silence gaps allow 2 beats of release, and the music after a gap is a fresh 4-beat acquisition.
   - (d) Half-time drums annotate the grid tempo (snare on 3 of the grid); hats-only annotates the accented quarter.
   - (e) Level means the music's peak in dBFS, before the −72 dBFS noise floor.
3. **Phase ±25 ms.** It appears in the P7 defaults but not in the pass list. It is reported, not gating. Adding it is a narrowing re-freeze.
4. **3:2 coverage.** The synthetic 3+3+2 family does not trigger the 92 lock in the current tracker. Choose one:
   - approve adding a synthetic case that reproduces it (narrowing re-freeze);
   - or accept that BOOM in the corpus carries that coverage.
5. **Low levels conflict with the silence detector.** −20, −30 and −40 dBFS can never pass while the in-house silence detector (0.04 / 0.08 RMS, 5 s) zeroes the tracker's input. Choose one:
   - the silence threshold becomes part of the step-3 design;
   - or the level cases are redefined relative to the device-calibrated level.

## Process notes

- **Governance tools not called.** The SpectraSynq governance workflow tools (`workflow_bind`, `workflow_verify`, `workflow_gate`) were not called in this dispatch. Native task state is left to the lane owner, to avoid the stale-task Stop-gate trap (PL14-E8).
- **STATUS.md.** The only line added is the one `freeze_tolerance.py` wrote.

## Changelog

| Date | Author | Change |
|---|---|---|
| 2026-10-14 | agent:claude (P7 host worker, Opus 5.5) | Created: steps 1 and 2(a) executed; 2(b) proposal. |
