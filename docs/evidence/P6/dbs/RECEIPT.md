# P6 / R05 — Titan spectral-detail (DBS) pilot: receipt

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Nothing in this pilot touched the device.

## What happened, in plain English

The question was whether lower Hybrid K1 (board 0, mode 32) can show fresh musical attacks
more clearly in dense passages by dimming the light it keeps adding between attacks, and
whether a new adaptive grouping of the spectrum is the right signal to drive that.

1. **The comparison probe was repaired.** The old pre-AGC probe read each publication before
   it was published and then forced it "valid". It now reads at or after publication, checks
   every result and writes an explicit rejected row instead. This changed 131 of 11,056
   transient flags on BOOM (the old probe fed 10 empty warm-up hops to the detector as valid);
   spectral values were unchanged. A small regression keeps one valid read and the old
   future-read failure; it goes red when the publication guard is removed.
2. **The loss was located.** In the dense post-drop section of BOOM (55.5–75 s) the individual
   ~109 ms hits are still present in the raw analysis bank and in the per-hop level. They
   disappear at lower Hybrid's deposit: its brightness sits at the maximum in 33% of frames
   (≥ 95% of maximum in 46%; room recording 16% / 23%). The build (28–55.5 s) and weak Sonic
   almost never saturate. So the loss is in composition, not in the analysis bank.
   Figure: `step1/stage-explore.png`.
3. **Four arms were compared through the real renderer**, differing only in what drives a
   deposit-contrast multiplier `gain = 1 − 0.25 × (1 − attack)` on new material: baseline (off),
   the existing onset detector, a full-bank raw change, static 16 groups, adaptive 16 groups.
4. **Result: no arm qualified, so the baseline stays and the intervention is rejected.**
   On the realistic room recording every raw-bank arm loses more than 5% of the attacks the
   baseline already shows (retention 0.92–0.94); the existing-onset arm keeps them (0.99)
   but recovers only 5 of 46 missed arrivals. On weak Sonic the grouping arms lose 9–12% of
   baseline-visible attacks. After loud history the raw arms drop weak attacks
   (adaptive 9 of 12 vs baseline 12). In the payload the change mostly reads as a dimmer
   output (mean luma −18% adaptive, −24% onset) with the same space–time structure.
   Figures: `comparison/deposit-60-62s.png`, `comparison/payload-strip-60-62s.png`.
5. **Adaptive grouping does not earn inclusion.** It does not beat static grouping by the
   frozen margin (BOOM file 78 vs 73 recovered, room 14 vs 14), and both trail the simpler
   full-bank arm (86 file, 20 room). It is worse than static after loud history (9 vs 11 of 12).
6. **The rejected implementation is kept, not shipped.** The four product files are restored
   to their pre-pilot bytes. The whole integration (producer, unread-attack summary, renderer
   multiplier, module, its 28-check test suite and the four-arm probe) is in
   `candidate/dbs-pilot-rejected-integration.patch`, which applies cleanly to f92d683.

## Evidence status

| Claim | Status |
|---|---|
| Probe chronology repair | host-verified; regression red on planted fault |
| Loss location (deposit saturation in dense loud passages) | host-verified on the product v9 path |
| Detail-off path byte-identical (both boards, BOOM file, BOOM room, Sonic room) | host-verified |
| Board isolation (board-1 frames identical in every arm) | host-verified |
| Module: causality, duplicate/gap/epoch handling, no partition-created attacks, 455 splits, hysteresis | host-verified, 28 checks, 23 planted faults red |
| Comparison winner | **none — baseline retained, intervention rejected** |
| Installed / physically accepted | **not applicable**: no candidate earned a build |

## Criterion results (P6-DBS-PILOT-CRITERIA-v1 + v2 amendment)

| Criterion | Result |
|---|---|
| Off-path equivalence | PASS (byte-identical) |
| Board isolation | PASS |
| Causality / stale / epoch / duplicate handling | PASS (module tests) |
| No partition-created attacks | PASS (module test 5) |
| M1 recovery (both BOOM inputs) | arm1 FAIL (room 5/46); arms 2–4 PASS (file 73–86/106, room 14–20/46) |
| M2 retention ≥ 95% (both BOOM inputs) | arm1 PASS; arms 2–4 FAIL on room (0.92–0.94) |
| M3 false births ≤ baseline | arm1 PASS file / FAIL room; arms 2–4 FAIL. **Metric caveat:** the oracle plant (perfect onset knowledge) also fails M3, so M3 as frozen is unattainable for any deposit-contrast design; the verdict does not rest on M3 |
| Sonic transfer S1 retention | arm1 PASS (0.98); arms 2–4 FAIL (0.88–0.91) |
| Probes (sustain, weak, quiet recovery) | arm1 PASS; arm2 FAIL sustain and recovery; arms 3–4 FAIL recovery |
| Adaptive beats static and full-bank | FAIL |
| Metric validity plants | uniform dim: recovers 0 (correctly not credited); random: fails M2/M3; oracle: recovers 103/106 and 40/46 (measure can go green) |
| Scored settings equal built settings | NOT TESTED (no build: no winner) |
| Real control with readback | NOT TESTED (not built: no winner) |
| Physical capture | NOT TESTED (no candidate to install) |

## Reference quality (disclosed)

The onset reference (`reference/`) times attacks well on BOOM (planted p95 3.5 / 2.9 ms) but
fails its own recall requirement (0.40–0.53 at ≥ −18 dB on BOOM; kicks 0.13–0.33) and fires on
attack-free material (16.75/s). The v2 amendment, frozen before any arm was scored, uses only
the 207 onsets stronger than the strongest false detection on the attack-free probe, and
reduces Sonic to a retention check. All replays were rerun after that freeze and were
byte-identical (168 files). A kick an arm reveals but the reference misses counts against
the arm.

## Remaining action and owner

- **P6 lane owner (product composition):** the demonstrated loss is lower Hybrid's level
  transfer saturating (ceiling −9 dBFS, `kInhouseV9LevelCeilingDb`) in loud dense passages.
  A composition change there (ceiling/emphasis, or a contrast design that does not darken the
  whole output) belongs to the owner's existing P6 iterations and Captain verdicts; it needs
  its own frozen criterion. Not changed here.
- **If the pilot is reopened:** fix the reference first (a sub/low-band kick reference) and
  replace M3 with a criterion the oracle can pass, freezing both before results. Do not reopen
  GPU training, U55/M33 or a separator reproduction.

## Identity

- Contract: `/Users/spectrasynq/Downloads/Titan_DBS_Translation.html` sha256 `bf489d3e…cf13a7`.
- Criteria: v1 body `bd48725f…` frozen 2026-10-14T22:48:39Z; v2 amendment body `5df84972…`
  frozen 2026-10-14T23:02:04Z (`criteria/`). Missed sets predeclared from the baseline only:
  `fc838488…` (BOOM file, 106), `fb495d64…` (BOOM room, 46).
- Inputs: boom_source `45edfd09…`, boom_room `95c9d9b8…`, sonic_room `dac2e8c0…`.
- Host build: Apple clang, `-O2 -fno-fast-math -DK1_EMOTISCOPE_PORT=1 -DK1_INHOUSE_MAPPING_V9=1`
  (product v9 path; host variants off).
- Device leave state (read-only `smc_titan.py health`, after the pilot): UID
  545433931bd25436593630352d068363, build bbf8083df5ec (installed by the lane owner, not by
  this pilot), 100 fps, led_faults 0, Waveform Hybrid K1, emission ON — "visualising".
- Scripts: `scripts/p6_dbs_reference_onsets.py`, `p6_dbs_probes.py`, `p6_dbs_salience_floor.py`,
  `p6_dbs_score.py`, `p6_dbs_figures.py`. Run folder (not committed):
  `_scratch/waveform-migration-001/runs/P6-DBS-20261015/`.
