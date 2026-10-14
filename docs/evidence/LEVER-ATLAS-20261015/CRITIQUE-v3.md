# Lever atlas v3 — completeness critique (persisted by the lane owner from the workflow journal)

**Verdict: there are three Critical gaps.** Each one sits in one of the atlas's next three experiments. As written, each experiment would probably give a red result or a misleading verdict that has nothing to do with the question it asks. Every fix is cheap, and none of them changes the ranking of the top 3 levers (R01, R05, R03). I ran no build, replay, flash, device read or commit, and wrote no files. Images do not apply because this is a text critique. I did compute palette brightness from the source palette stops and ran a 20-line simulation of the v9 transport.

Product worktree at `01ec2dc` (paths below are relative to it).

## CRITICAL

**C1 — Experiment 3 (the riser speed puppet) cannot produce valid evidence as designed.**
- **One plate, not two.**
  - [FACT] "The product is ONE light-guide plate driven by TWO 160-pixel edge fields" (`docs/effects-language-001/DUAL-EDGE-CORRECTION.md:9-11`, Captain, 28 Sept).
  - [FACT] "Both strips feed one light guide". The two camera bands correlate r 0.866, and "the camera cannot separate the boards" (`docs/evidence/P3/records/REVIEW-ITERATION-1.md:73-82`). P3 iteration 2 already rewrote its Captain question to "about the plate, not a board" (`RECEIPT-ITERATION-2.md:14`).
  - The atlas still asks "On the lower plate…" (atlas:1021), ramps board 0 only, and uses "lower/upper plate" throughout. A ramp on one edge gets mixed with the other edge's unchanged light, so the Captain would be judging edge mismatch, not tension.
- **Speed is not brightness-neutral in today's transport.**
  - [FACT] There is one deposit per frame (`inhouse_waveform.cpp:1022`). The triangle reach is width+1, about 1.5–5 px (`:841`). The rate is 405·motion px/s, capped at 8 steps per frame (`:967-968`).
  - [INFERENCE, my simulation of `:800-1022` with steady input] Going from speed 0.5 to 1.0 cuts total half-strip intensity by 19–36 % and lit pixels by 12–37 %. The cause is deposits landing 8 px apart: the trail breaks into dashes.
  - Consequence 1: the frozen clause "per-frame packed luma within ±5 % of arm A" fails even for a correct pure-speed puppet.
  - Consequence 2: arm B dims the riser, and then speed snaps back to 0.5 at the drop (atlas:1020), so brightness jumps up by roughly 25–55 % at the drop. A "lands harder" verdict would then be credited to velocity when brightness contrast caused it.
  - The snap-back is also a slowdown at the drop. That is the opposite of the Shocklines lineage, where speed is peak-held and decays with τ ≈ 1.66 s.
- **The instrument cannot measure the frozen clauses.**
  - [FACT] A wire snapshot is a single latest-submission buffer, fetched once per broker file request (`tools/serial-studio/titan_broker.py:870-912`).
  - [FACT] P3 got 9–12 snapshots per 5 s window, about 2 Hz (`REVIEW-ITERATION-1.md:34`).
  - A trail crosses the half in about 30–150 ms. "Trail-front velocity from cross-correlation of successive right halves" and per-frame luma therefore cannot be measured.
  - [INFERENCE] Comparing frames one by one across two separate playbacks through the room is not reproducible anyway.
- **Repair:**
  - Pre-screen the exact speed schedule on the host. `p6_effect_probe` already outputs `scroll_steps` and `intensity_mean`.
  - Take velocity from the speed readback plus the arithmetic at `:967`.
  - Ramp both edges. Mode 18 also scales with motion (`:992`).
  - Hold the raised speed through the drop and let it decay, instead of snapping back.
  - Freeze the luma clause on windowed means against a measured run-to-run noise floor (the identical-pair run).
  - Ask the Captain about the plate as a whole.

**C2 — Experiment 1's planted positive will probably fail on board 0 for colour reasons, not geometry.**
- Path P colours each pixel as `sample(wrap(offset + d))`, where d = distance/80 (`inhouse_waveform.cpp:888-890`). A front travelling from the edge to the centre therefore sweeps most of a full palette turn.
- [FACT] Board 0 samples catalogue entry 31, "BlacK_Red_Magenta_Yellow_gp" (`product_palette_data.cpp:876`). Its stops run from black (0,0,0) at position 0 to yellow (255,255,0) at 255, so the black→yellow seam sits at the wrap.
- [INFERENCE, computed from those stops] 21.8 % of the turn is below 10 % of peak brightness. Entry 38 has 14.8 %.
- [FACT] EXT finds the innermost pixel at ≥ 10 % of the half's peak brightness (`scripts/p6_ext.py:4-6`; EXT2 `signal`).
- [INFERENCE] In about 80 % of drop offsets the front's head crosses the dark band.
  - When it does, the innermost lit pixel stalls and then jumps. That breaks "inward on ≥ 95 % of frames".
  - If the dark band falls at d < 0.1, extent never reaches 0.9, so T90 never arrives.
  - The atlas's decision rule would then redesign EXT-v3 for the wrong reason.
- **Repair:**
  - Score direction on the front's own intensity channel and keep brightness as a separate clause, or
  - fix the front's palette position (offset_ = c − d) so the front keeps one colour as it moves, or
  - include R08's dark-end fix in the planted positive.

**C3 — Experiment 2 arm A (the onset detector test) has no planted positive, and its key clause may be impossible to pass.**
- [FACT] Transient strength is capped at 1.0 (`onset_beat.cpp:290`), and full_flux is a sum over 75 bins.
- [INFERENCE] Build-section kicks will saturate that cap, so "flux strength ≥ 2 × the build p95" becomes ≥ 2.0 against a ceiling of 1.0.
- [FACT] The flux is a log ratio (`:107-108`), so it ignores loudness by design. A 2× margin over build kicks measures how much the spectrum changes, not how big the drop is.
- Arm A lists only planted negatives. The atlas's own rule (atlas §5: "green on a planted positive") is broken.
- [INFERENCE] If every variant comes out red, nothing flags it, and the stated conclusion ("needs FFT arm C or biquad arm D") would be wrong.
- **Repair:**
  - Split the timing clause (an event within [0, +30 ms]) from the margin clause.
  - Name the strength field to use (uncapped band flux).
  - Add a positive: BOOM with a known clean transient injected at 55.7025 s, which must pass.

## HIGH

- **H1 — The one-light-guide topology is missing from the whole portfolio.** R22, Q5, C09, Q7, Q-R25 and §8.5 Q5 treat the two boards as separable plates. EXT scores each board separately but never the plate as a whole. The source conflict (two independent 160-pixel scenes vs one continuous scene split 80/80) is already logged as X11, but the physical fact (one plate, shared light) is missing.
- **H2 — The T90 limit in the EXT-v3 freeze is contradictory.**
  - EXT-v2's T90 ≤ 0.15 s is kept "unchanged", and the atlas also states T90 ≤ 0.18 s (atlas:997, :740, X12).
  - [INFERENCE] With today's E_climax ≈ 0.4, planted negative (f) (linear travel, 200 ms) reaches 0.9 at about 166 ms. It passes 0.18, so the atlas's reason that "(f) fails T90 ≤ 0.18 s" is wrong.
  - Freeze one number. Q15 should be put to the Captain with that number.
- **H3 — Nothing constrains how R01 exits its hold.**
  - After the 1–2 s hold, normal flow resumes at drop loudness. [FACT, atlas L17] `peak_scaled` sits around 0.9, so the deposit moves back near pixel 130.
  - The light would snap back to the edges, a second gesture running the opposite way.
  - Neither R01 nor Experiment 1 freezes a "no jump ≥ 0.3 within 50 ms" check after the hold.
- **H4 — The noise floor for R05's log flux changes with pre-AGC input.**
  - [FACT] Today's spectrum is Q16-quantised and clamped (`inhouse_audio.cpp:350-358`), so bins below 1/65536 read exactly 0, which acts as a floor.
  - [FACT] Pre-AGC magnitudes are floats (`:274`), and the log floor is only 1e-6 (`onset_beat.cpp:41`).
  - [INFERENCE] Quiet bins full of noise would count as much as loud ones.
  - "No rescaling needed" is true for gain but not for the floor. Freeze a compression constant or floor, chosen from the build window, before scoring.
- **H5 — R25 depends on fractional transport (R14), which ranks 18th.** With integer steps and one deposit per frame, any speed-up causes dashed trails and stepped motion (C1). R25's product version should be gated on R14, or on depositing at every step.
- **H6 — The brightness-envelope conflict (X1) is not a gate before device steps.** The global Titan mandate says "preserve 24/255", and the running image is about 64/255-equivalent. The host experiments are unaffected because their brightness clauses are ratios to the envelope. Device steps should wait for the ruling.

## MEDIUM

- **M1.** The click corpus is never located in the atlas. [FACT] It exists at `_scratch/waveform-migration-001/audio-p5-001/p5-clicks-boom.{wav,truth.json}`.
- **M2.** The replay set is still at the volatile path `~/.claude/jobs/c8f0d616/tmp/p6`. [FACT] It is 3.2 G and present today, and the external scratch drive exists, but the move (M0-9) has not been done.
- **M3.** The tuning set is thin: two songs, with the BOOM drop counted twice, R05's BOOM clause resting on one drop, and a single holdout.
- **M4.** R03's "contract" part is already today's behaviour (the atlas's own D18: deposits park at the edge). What R03 actually adds is holding back riser brightness. State that, so nobody builds redundant contraction logic.
- **M5.** The drop-labelling convention (ruling 5) is still open, yet the bars in Experiments 1 and 2 are anchored at 55.7 and 55.7025 s. Write the convention into the frozen file.
- **M6.** The host driver for arm A must fill in the gate fields (`spectral_energy`, `novelty`, `event_flags`, `nyquist_safe_bin_hi`, `low_energy`). With pre-AGC input their scale changes. List them under "gate floors re-derived".
- **M7.** Snapshots at about 2 Hz cannot classify camera dark lines shorter than 0.5 s (Experiment 3, Step 1).
- **M8.** The light guide's spatial blur is unmeasured. It decides whether a 100–180 ms front and 8-px dashes are visible. One static frame with locked exposure would answer it.

## Checked and holding

- OnsetBeat uses 80 bins with bands 1–76, 1–25, 25–50 and 70–80 (`onset_beat.cpp:14-21`).
- Speed sets motion_scale (`palette_runtime.cpp:250`).
- `p6_ext.py` exists and computes extent with the 10 % rule.
- `test_p6_product_path_p.py` exists with its planted-gain red case.
- Per-pixel `frames.bin` replays exist for path P (`replay-pfix/P`).
- `harness/tests/test_composed_motion.cpp` exists.
- Live control writes do not persist to non-volatile memory.