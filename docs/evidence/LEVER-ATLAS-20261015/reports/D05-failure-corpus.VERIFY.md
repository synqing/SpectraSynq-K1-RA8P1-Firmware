# D05 failure corpus: adversarial verification (15 Oct 2026)

## Plain-English verdict

The D05 report holds up well. I checked 26 of its most decision-relevant claims against source, git history, run receipts and the memory records it cites. 21 are correct as stated. 5 are wrong or overstated. None of the errors overturns its top finding.

That top finding is that the Titan copies the K1's 0.25/0.005 peak follower, and that this follower caps how far the drop can reach. That is confirmed in source. It is still an [INFERENCE] that this is *why* the drop does not hit harder; no test has been run.

The errors:
1. The sub-bass filter is described as uncommitted. It is committed.
2. One file:line for the admission gate points to the wrong place.
3. The "0.014 vs 0.038 drop flux" is quoted as the main onset (novelty) signal. It is actually a 110–208 Hz flux from an accent candidate.
4. Two "default" claims are true only in the uncommitted working tree.
5. The 0.25 output gain is presented as fixed. It is a runtime setting that has also been read back at 1.0.

**Where the report is.** The D05 report was never written to disk. Its content was recovered from the excavator's own result in the workflow transcript `~/.claude/projects/-Users-spectrasynq-Workspace-Management-Software-SpectraSynq-PRISM/c8f0d616-…/subagents/workflows/wf_2a73b8fd-675/agent-a58c5c62006c12c74.jsonl`, saved locally at `/tmp/d05_result.json`.

**What was checked.**
- Product worktree HEAD `1ef5395` (`lane/titan-lagfree-20261014`), plus its uncommitted working-tree changes to `inhouse_audio.cpp`, `inhouse_waveform.{h,cpp}` and others.
- K1 repo `/Users/spectrasynq/SpectraSynq_K1_Firmware`.
- claude-mem observations. Memory was DEGRADED according to the hook: the observations are history, not authority.

Images: N/A. This is a check of source code and documents; there was no visual output to inspect.

## Claim-by-claim

| # | D05 claim | Verdict | Evidence |
|---|---|---|---|
| 1 | Peak follower attack 0.25 / release 0.005 at `inhouse_audio.cpp:250` | CONFIRMED [FACT] | `platform/ra8p1/inhouse_audio.cpp:250` `alpha(drive > follower_ ? 0.25F : 0.005F, dt)`; peak_scaled smoother 0.65/0.15 on the following line |
| 2 | Mode-32 extent = 80 + peak_stage2·0.7·80 via 16 + 23 ms EMAs (`inhouse_waveform.cpp:973-977`) | CONFIRMED [FACT] | `:974-976`. The code clamp is `min(159, …)`; with peak ≤ 1 the effective maximum is 136, as D05 says |
| 3 | Same follower constants as the K1 "short leash" (#93761) | CONFIRMED (memory) | #93761: "The 50:1 attack/release asymmetry (0.25 / 0.005) is the 'short leash'" |
| 4 | Spectral AGC: single broadband scalar, target 0.25/envelope, clamp 0.1–10, envelope attack .28 / release .02, gate opens at 4× and closes at 2.5×, gain frozen while gated (`:329-341`) | CONFIRMED [FACT] | `inhouse_audio.cpp:330-339`. Omission: the gain's own adaptation rate (`alpha(0.05)`) is not stated |
| 5 | Soft knee `0.5 + e/(1+e)` at `:352-355` | CONFIRMED [FACT] | `:352-355` |
| 6 | "Path P already reads the pre-knee linear_shape" (`:347-350`) | PARTIAL | True only in the uncommitted working tree. `git show HEAD:platform/ra8p1/inhouse_audio.cpp` has no `linear_shape` assignment |
| 7 | Display stage: brightness .3 + .7b, tanh shoulder above .75, warmth, white {1, .9375, .84} (`palette_runtime.cpp:281-294`) | CONFIRMED [FACT] | `palette_runtime.cpp:281-292` |
| 8 | Tempo prior `kTactusBpm 88` at `:23`, warm-up 512 at `:39`, prior near `:515` | CONFIRMED [FACT] | `src/k1/core/audio/tempo_tracker.cpp:23,39,514` |
| 9 | Render 100 Hz, not 120: `kPalettePeriodUs 10000` under `K1_EMOTISCOPE_PORT`, measured 99.7 fps | CONFIRMED [FACT] | `palette_runtime.h:40`. 8333 µs is the `#else` branch. `scripts/build_scalar.py:887` sets the define. STATUS.md:76 records 99.7 frames/s |
| 10 | At 81ac031 `sample()` was already the HD stop sampler; `compiled_fastled16` was only a near-black fallback | CONFIRMED [FACT] | `git show 81ac031:…inhouse_waveform.cpp`: `:41-71` interpolates stops; `:216-226` uses compiled_fastled16 only when the fallback is below 0.02. The collapse came from one sample per frame, RGB mixing and the RGB EMAs at `:227-233` |
| 11 | "1–11 % before" is v9, not v8; v8 moved in 94.8 % of seconds | CONFIRMED [FACT] | `docs/evidence/P6/palette-forensics/FORENSICS.md:113` (94.8 % vs 1.0 %) and `:118` (9 % Sonic, 4 % Dopex) |
| 12 | Path P spatial term `d = |i−centre|/80` at `inhouse_waveform.cpp:888-890` | CONFIRMED [FACT] | `:888-889`. Note: path P as the default (`kPerPixelMapped`, `.h:47`) is working-tree only; HEAD `.h:46` is `kRestrictedRange`. D05 labels L12 "working tree" correctly |
| 13 | v9 level −33..−9 dBFS → 0..1; deposit gain 0.396 | CONFIRMED [FACT] | `inhouse_waveform.cpp:948-949`, `.h:35` `kInhouseV9Gain32 = 0.395878F`, `.h:49` ceiling −9 dB. The `l^1.5` exponent was not traced |
| 14 | `updateReleaseV9` disabled by default | CONFIRMED [FACT] | `inhouse_waveform.h:66` `bool enabled = false`; `.cpp:762` returns early |
| 15 | Ceiling saturates in 34 % of song-file drop frames | CONFIRMED [FACT] | `runs/STATUS.md:155` |
| 16 | In-house AP p50 ≈ 716k cycles | CONFIRMED [FACT] | `runs/STATUS.md:43` (270k → 716k, ×2.6) and `:45` |
| 17 | VP p50 ≈ 182k cycles | CONFIRMED [FACT] | `runs/STATUS.md:110` (195.7k → 182.1k) |
| 18 | Tempo 0/326 on GATE-SYNTHETIC-v1; 88 BPM prior is the necessary cause; flat/120 prior gives BOOM 100 % intro and 82–92 % drop | CONFIRMED [FACT] | `runs/STATUS.md:61` |
| 19 | BOOM reads 69/92 against a true 138 | CONFIRMED [FACT] | `runs/P7-20261014T121412Z/permitted/RECEIPT.md:117-121` |
| 20 | Wire time settled at 5.1 ms; the older 9.6 ms figure is in the P6 RECEIPT | CONFIRMED [FACT] | `runs/STATUS.md:178,193`; `runs/P6-…/permitted/RECEIPT.md:113` (9.6 ms) |
| 21 | Brightness conflict: Kx-Titan AGENTS 100/255; runtime master 255 with gains 0.25 | CONFIRMED [FACT] | `Kx-Titan/AGENTS.md:12` ("default brightness 100/255 supersedes historical 24"); `docs/emotiscope-port-001/CURRENT-STATE.md:24` (master 255, gains 0.25) |
| 22 | K1 c2738b88 dated 22 Aug 2026 | CONFIRMED [FACT] | `git log` 2026-08-22 17:18:38 +0800; ae5d90a3 dated 2026-07-02 |
| 23 | #93755 (gamma root cause) was reversed by #93763 (3.5 mA drive + AGC frozen at floor + follower) | CONFIRMED (memory) | #93763 title: "Do NOT Flash De-Gamma — LED Drive Current and Frozen AGC Are the True Causes"; #93761 names the follower |
| 24 | v9 sub-band 40–110 Hz, `sub_band.h:44-53`, **uncommitted** | **REFUTED** | `src/k1/core/audio/sub_band.h` (not `platform/ra8p1`) is tracked and clean. It was added in commit `1416cb5` (2026-10-14 22:57 +0800), an ancestor of HEAD |
| 25 | Donor admission gate PCM peak² 0.0048 / 0.0034 at `palette_runtime.cpp:258` | **REFUTED (location)** | The values are correct but live at `platform/ra8p1/composed_scene_runtime.h:31-32`. `palette_runtime.cpp:258` is only the `inhouse_admission_[board].update(...)` call |
| 26 | "BOOM drop flux 0.014 < build 0.038" used as the novelty / onset energy (summary, L04, T10), and as "110–400 Hz flux ratio ~0.37" (L02) | **REFUTED (attribution)** | `runs/STATUS.md:167`: "accent silent on BOOM drop (**110-208 Hz** flux drop 0.014 < build 0.038)". It is the low-band accent candidate's flux, not the broadband `novelty` at `inhouse_audio.cpp:356-362`, and not a 110–400 Hz band |

Additional overstatements, counted inside the refuted total:
- **L10 "gain 0.25, every colour value pushed into the chip's dark region".** The gain is runtime state (`output_intensity_q16_`, `palette_runtime.cpp:~709`), not a constant. CURRENT-STATE.md also records readbacks at "gains 1.0" (`:105,129`). [FACT] The setting varies, so the claim should be conditional.
- **L03 "already reads pre-knee".** Working tree only (row 6).

## Unverifiable

- **The datasheet claim that the WS2816C has "4-bit in-silicon gamma, no disable".** No primary datasheet was read. The only support is memory (#93755; #93763 says "bend capped at γ1.25").
- **The +17 dB sub-bass at the BOOM drop.** The P6 RECEIPT:203 says "+17 dB on BOOM", but the measurement itself was not re-run.
- **The 72–117 ms sub-bass lag.** It was not traced to a receipt line in this pass.
- **Camera reach ~13 % (T18) and the Dopex dark-at-76.3 s timing (T16).** No source was opened for either.

## Origin claims asserted without evidence

| Lever | Asserted origin | Assessment |
|---|---|---|
| L05 Goertzel floor at 110 Hz | "S3 budget [INFERENCE]: long low-frequency windows at 12.8 kHz were expensive" | **Unevidenced.** It is the only lever whose origin is called an S3 budget trade-off, and no commit, comment or cycle measurement is cited. The first bin is 110.0 Hz (`gdft_goertzel.cpp:26`), but nothing shows that the reason was budget rather than inheritance (A2 = 110 Hz is a musical anchor) |
| L01 follower | "inherited Emotiscope/K1 donor" | The K1 half is supported (#93761). The Emotiscope half is unverified: a grep of `/Users/spectrasynq/Workspace_Management/Software/Emotiscope` found no 0.25/0.005 pair |
| L14 100 Hz cadence | "donor 100 Hz reconstruction" | Partly supported: the compile branch exists (`palette_runtime.h:40`). Why the donor chose 100 Hz is not evidenced |
| L02 AGC | "S3 lineage (K1 AGC v2)" | Lineage is supported (#72835). D05 does not claim a budget origin, which is correct |
| L07, L11, L12, L15 | "aesthetic" or "K1 design" | Labelled as taste. No budget claim is made, so these are acceptable |

No lever in D05 has an S3-budget origin backed by evidence. This matters to the Captain's "why compress?" question. The corpus shows most of the compressions are inherited or aesthetic, not proven budget trade-offs. The one budget claim (L05) is an inference.

## Effect on decisions

- **L01 stands.** The source facts are right. The causal link to "drop doesn't hit harder" is still an untested [INFERENCE]. Its frozen experiment, a follower-release sweep with a planted attack-equals-release fault, is the correct next step.
- **L02/L04 need their criterion re-stated.** Name the band and the signal: 110–208 Hz accent flux, or broadband novelty. Do this before freezing, or the criterion will not reproduce.
- **L05 needs a corrected premise.** The sub-band is already committed (`1416cb5`). The work left is to publish `sub_rms` to the product contract and enable it, not to commit it.
- **L03/L12 need the right baseline.** Path P and the pre-knee tap are uncommitted. Any A/B must name the exact image built, because PL14-B14 recurs if the compiled default differs.
