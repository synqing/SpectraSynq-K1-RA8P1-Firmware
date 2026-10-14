# D11 mapping and choreography: adversarial verification

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

Verifier: Opus 5.5, 15 Oct 2026. This was a read-only check: no device, build, commit or edit outside this run folder.
Inputs: `SWARM-BRIEF.md` (read in full), then `D11-mapping-choreography/REPORT.md` (all 620 lines, including Appendix V).
Titan worktree HEAD: `1ef53952ab7a50af38414321c8a91e515744098a`. The renderer, audio and build files have uncommitted changes.

## Verdict

I checked 32 decision-relevant [FACT] claims against the source. **30 hold** and **2 do not**.

The source citations are accurate almost throughout: formulas, constants, time constants, line numbers, detector scores, receipts, lineage commits and frozen tolerances.

The one material failure is that the report's headline configuration fact is now out of date. It says the compiled default still selects palette B (`kRestrictedRange`), not the path-P colour fix. That was true at HEAD. In the current working tree, however, `inhouse_waveform.h:47` selects `kPerPixelMapped`. The header's SHA-256 has also changed since the report recorded it.

The appendix's most decision-driving inference also overreaches. It claims that a louder build contracts naturally, but the source normalises peak against an adaptive follower, so the claim does not follow from the code.

## Refuted

### R1. "The inspected header still selects `kRestrictedRange`" and the WH SHA-256 `2e0013b8…`

The claim appears in REPORT §0, D11-12, Contradiction 2 and Appendix V row 1.

- [FACT] The working-tree file `src/k1/core/visual/inhouse_waveform.h:47` now reads `inline constexpr InhouseV9Palette kInhouseV9Palette = InhouseV9Palette::kPerPixelMapped;`. The comment above it reads "TEST VARIANT under the Captain's testing ruling (14 Oct 2026)".
- [FACT] The current SHA-256 of WH is `88cea47b90ccc528a855fefb1cad12208b880607e47adb49cec3e29b29fc2272`, not `2e0013b8…`. The file's modification time is 15 Oct 01:19, and REPORT.md was last modified at 01:29.
- [FACT] HEAD `1ef5395` still has `kRestrictedRange` at line 46 (`git show HEAD:…inhouse_waveform.h`), as PL14-B14 records.
- **Correction:**
  - Path P is the compile-time default in the uncommitted working tree.
  - It is not the default in any commit.
  - No built or resident image has been shown to contain it.
  - The PL14-B14 product-configuration equivalence test is still the open gate.
  - D11-12's "current setting" and Contradiction 2 must be restated against both HEAD and the working tree.
  - The SHA-256 table in the report no longer identifies the inspected header.

### R2. Appendix V inference: "a build that gets louder therefore contracts naturally … the current mapping renders the drop as the most-contracted moment"

- [FACT] In mode 32, `position = 80 + peak₂·0.7·80` (`W:976`). Here `peak` is `peak_scaled` (`W:966`), which is `drive / follower_` (`A:251–253`). `follower_` rises with a rational time parameter of about 30 ms and falls with one of about 1990 ms (`A:23–26,250`).
- [INFERENCE] With a 30 ms attack, the follower tracks a steadily louder signal within a few hops. The ratio is therefore driven by attacks relative to a recent peak, not by absolute loudness. A gradual crescendo does not by itself move the root towards the edge.
- [INFERENCE] The post-breakdown part of the claim is plausible: a slow-falling follower plus a loud return pushes the ratio towards 1 or above, and `palette_runtime.cpp:265–267` clamps it to 1. But a steady loud passage also sits near a ratio of 1, so a contracted drop is not specific to drops.
- **Correction:** relabel this as [HYPOTHESIS]. It needs a replay trace of `observation_.position` across BOOM's build and drop before it is used to rank D11-03 as the top lever. That lever's value does not depend on this claim, because the report's own §3 geometry stands without it.

## Confirmed (30)

| # | Claim | Evidence checked |
|---|---|---|
| 1 | HEAD `1ef5395`; working-tree hashes for W, A, PR and LR | `git rev-parse`; `shasum` matches all four |
| 2 | Profile 9 only adds `-DK1_INHOUSE_MAPPING_V9=1` | `scripts/live_boot_composition.py:14` |
| 3 | Rational α gives 30/1990 ms and 5.38/56.67 ms | `A:23–26`, `A:250–252`; τ = 0.01(1−r)/r |
| 4 | Magnitude mix .3/.7 gives τ 21.03 ms; Mood LPF gives τ 79.58 ms at Mood .05 | `A:276`; `A:107–111`, where α = 1−e^(−2π·1.5/100) per 7.5 ms hop |
| 5 | The hop is 7.5 ms at 24 kHz/180 samples (and at 12.8 kHz/96) | `inhouse_audio.h:20–26` |
| 6 | VU level maps −33 dBFS to 0 and −9 dBFS to 1 | `W:946–949`, `WH:49` |
| 7 | Amplitude is g·l^1.5 with gains .395878/.591745; width is .5+3.5l; reach is width+1 | `WH:35–36`, `W:826–841` |
| 8 | Mode-32 root uses a 16/23 ms EMA pair, p = 80+56·peak₂, and an optional release inversion | `W:972–977` |
| 9 | Transport is 405·motion px/s with an 8-step cap; motion = 2·speed | `W:963–965`, `PR:250` |
| 10 | Mode-18 tempo weight is .7·smoothstep, the tempo rate is 24·BPM/60·(1+.5cos), and the fallback is 30+90·onset | `W:986–993` |
| 11 | Trail τ is 290/125 ms, with +10 s⁻¹ in confirmed quiet | `W:801` |
| 12 | Max-intensity ownership of colour and offset | `W:864–868` |
| 13 | v9 colour constants: 0.6/0.25/15/8 s with caps .25/.03 | `W:668–673` |
| 14 | Restricted-range fold lo+(hi−lo)(1−\|2c−1\|) | `W:574–580` |
| 15 | Path-P per-pixel sampling HD(wrap(o+d)) with d = dist/80 | `W:893–897` |
| 16 | Spectrum mix 1−.25^(100dt) gives τ 7.21 ms; chroma max decays at .999 (τ ≈ 9.995 s) and rises at .05; the flatness gate spans .08–.28 | `W:147–166` |
| 17 | v8 mode 32: RGB 80 ms EMA, decay 0.8+3.5·peak, fallback scans `compiled_fastled16` | `W:205–260` |
| 18 | Output stage applies .3+.7·brightness and a tanh shoulder above .75 | `PR:283–294` |
| 19 | `InhouseWaveformAudio` has no downbeat, bar or section field | `WH:80–116` |
| 20 | Tracker has an 88 BPM prior and a 512-update warm-up; P7 reports half-time failures | `tempo_tracker.cpp:23`; `docs/evidence/P7/RECEIPT.md:19–43` |
| 21 | Detectors: v1 hit 1 of 5 plus a holdout miss; v2 hit 1 of 6; v3 hit 0 of 6; v3 excludes cancels from scoring | `P6-DROP-DETECTOR/DETECTOR.md:54–69`; `v2:113`; `v3:79–93` |
| 22 | Detector-v1 prose says the build pulls "from the edges toward the middle" | `DETECTOR.md:7` |
| 23 | P6 variant B: luma ×0.80 while light ×1.42 from build to drop | `docs/evidence/P6/RECEIPT.md:32,48` |
| 24 | Live boot brightness must be 100..255, which conflicts with 24/255 | `scripts/build_scalar.py:933–934` |
| 25 | PL14-B14 exists, cites `:46`, and instructs that path P be selected | `PL14:148–152` |
| 26 | 100 Hz under `K1_EMOTISCOPE_PORT`, 120 Hz otherwise | `platform/ra8p1/palette_runtime.h:39–47` |
| 27 | GDFT's first used bin is 110 Hz (`kNoteOffset` = 12); the sub-band is 40–110 Hz, 4th order | `gdft_goertzel.cpp:18,26`; `sub_band.h:3–8,17–18` |
| 28 | Frozen guards: HD fidelity ≤.004 with ≤.001 fraction; motion > 1/32 with ≥ v8 − .02 and the ladder; spread median ≥ 4 bins with span ≥ .25 | `TOLERANCE-FROZEN-P6-PAL-HD-v2.json`, `TOLERANCE-FROZEN-P6-PAL.json` |
| 29 | S3 lineage: `palette_manual_colour` HD with a FastLED cold-cache fallback; c2738b88 replaces GDFT's `ColorFromPalette`; Hybrid uses 16/23 ms, 80 ms, 405 px/s and 3.5 | `S3/visual/lightshow_modes.h:244–294`; `git show c2738b88`; `S3/effects/light_mode_waveform_hybrid_k1.cpp:10–26,47–77` |
| 30 | Lightwave `5c9f4f7` changes only `AudioActor.cpp`; SB `6109a1e` has hue = k/79 and persistence .90; P5 reports ≈16.4 ms typical with unstamped air, microphone and photon intervals | `git show --stat`; `k1-ap-evidence-lab` at 6109a1e, `light_mode_spectrum_river.cpp:27,64`; `P5/device-budget/BUDGET.md:9–17,87–95` |

Nuances, none of which refutes a claim:

- The report gives no repository path for SensoryBridge `6109a1e`. The commit is in `/Users/spectrasynq/k1-ap-evidence-lab`. `/Users/spectrasynq/SensoryBridge-main` is not a git repository.
- The default `analysis_rate_hz` is 12.8 kHz, not 24 kHz. The 7.5 ms hop holds at both rates.
- The S3 Hybrid header comment says "tau 0.163" but the constant is 0.080. The report's "80 ms" matches the code.

## Origin audit (S3 budget versus aesthetic)

- [FACT] The report never assigns any lever an "S3 budget" origin. Every origin it states cites source:
  - inherited quirk at `A:313–317`;
  - Captain-calibrated 405 px/s at `S3 hybrid:23,54`;
  - P6 design records;
  - PL14.
- [FACT] It marks budget justification as "unknown" where it has none (D11-10, the integer transport) and states that no inspected source proves the S3 Hybrid constants were compute-saving.
- [INFERENCE] This is the correct, conservative position. It also means D11 adds no evidence for the Captain's central hypothesis that the compression was forced by the S3 budget. Every D11 lever traces to inherited character, aesthetic calibration or P6 experiment design, never to a measured ESP32-S3 deadline. That gap should be stated in the swarm synthesis rather than filled by assumption.
- [FACT] All M85 cycle costs in the lever records are "unknown". The only measured figures nearby are P5's 0.20 ms render and 1.0 ms analysis per hop, and PL14's 716k cycles p50 for in-house analysis at 24 kHz. Neither covers the proposed state machine.

## Visual output

N/A. This is a source-verification task. No render or LED output was produced or claimed.
