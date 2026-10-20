> **Read with `CRITIQUE-v3.md`** (lane owner, 15 Oct 2026): the v3 critic found three Critical gaps, all inside §9's next three experiments (one light-guide plate fed by two edges, not two plates; speed changes are not brightness-neutral in today's transport; wire snapshots at ~2 Hz cannot measure per-frame clauses). Fix those before running §9. The top-3 ranking (R01, R05, R03) stands. Also unresolved: effective brightness on the running image (~64/255 claimed) vs the 24/255 cap.

---
abstract: "Definitive Titan lever atlas v3, 15 Oct 2026. Integrates LEVER-ATLAS-v2 with five gap-closer results (G1 Titan existing machinery, G2 K1 main director/onset/EdgeMixer, G3 Lightwave effect library, G4 Titan canon rulings, G5 creative recovery) and this integrator's own spot-checks at product HEAD 01ec2dc. Adds §0 'What already exists on the Titan' (reuse before build). Re-ranks one list of 20 levers by expected value with effort lowered wherever existing machinery provides the work; moves the canon-ruled latency architecture (hop-locked 120 Hz render, 64-word capture, acoustic-to-photon budget) into an unranked conformance track beside the mandatory safety layer; corrects the camera timing evidence, palette labels, brightness envelope, render rate and the withdrawn '330 ms late' novelty figure. Research only. Every change is PROPOSAL — NOT APPROVED."
id: LEVER-ATLAS-v3
date: 2026-10-15
author: "Integrator v3 (Claude Opus 5.5), read-only on all repositories"
supersedes: "LEVER-ATLAS-v2.md. v2 and v1 are kept as history. v2 facts that still hold are carried here with their evidence; v2 items corrected by this pass are listed in §8.1 (D21 onwards) and marked where they appear."
inputs: "SWARM-BRIEF.md; LEVER-ATLAS-v2.md (all 681 lines); the five gap-closer structured results G1–G5 (passed in by the workflow; G1-F01…F19, G2-F01…F14, G3-F01…F05 + G3-P01…P15, G4-01…18, G5-F01…F05 + G5 concept records); runs/STATUS.md:196-205; product source at 01ec2dc; Kx-Titan TITAN-CANON-2026-10-13.md (working-tree file, mtime 2026-10-14 10:15 +0800); host-006 rtthread.map and symbols.txt; P6-DROP-DETECTOR/v3/results/probe/boom_file.csv; D01-ap-cartography/{REPORT,VERIFY}.md; D12-creative-vision/{drop_zoom.png,concept_sketch.py}; Lightwave-Ledstrip commit 0e2e0778 and SynqMatrix.cpp; SpectraSynq_K1_Firmware platformio.ini at 68a95773."
images: "Opened and inspected D12-creative-vision/drop_zoom.png this pass (it confirms G5-SKETCH (a): the D12 sketch's release lights the whole middle at once and then opens a black wedge from the centre outward; it is not an edge-to-centre travelling front). The v2 camera strip findings are carried from v2 (opened by the v2 integrator). This atlas produces no LED output; every appearance question is routed to an instrument or to a Captain A/B (§7)."
changelog:
  - "2026-10-15 integrator v2: first version (see LEVER-ATLAS-v2.md)."
  - "2026-10-15 integrator v3: added §0 inventory of existing machinery; re-ranked 20 levers (new R25 riser velocity charge, R26 stored charge → release size, R27 rewind-the-build); R18/R20 moved to an unranked canon-conformance track; camera timing evidence downgraded under CANON:52; palette labels, brightness envelope, render rate and novelty timing corrected; Lightwave/K1/Titan lineage cited per lever; do-not-port list; new next three experiments."
---

# Titan lever atlas v3

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.** It boots with visual output on and runs continuous, autonomous, audio-reactive visualisation whenever it is powered and the application is running. Nothing in this atlas adds a permission gate, a brightness floor, a normaliser that breaks true black, or a rainbow default. Nothing changes the brightness envelope, Titan UID `545433931bd25436593630352d068363`, P601/GPT6 and P604/GPT8, microphone DMA1/2 or LED DMA0/3. Every new behaviour proposed here falls back to the current live mapping (host-006, v9 path P) when its own evidence is missing. Programming a new image darkens the plate for about 41–47 s (PL14-A6); that gap is disclosed, and autonomous visualisation resumes at boot. No-flash experiments in this atlas change live settings only and never stop visualisation.

**Status.** Research synthesis. The integrator ran no build, replay, flash, device read or commit. It read source, evidence and the five gap-closer results, spot-checked their load-bearing claims (listed in §10), recomputed one figure from an existing probe CSV, and opened one image. Every proposed change, image and experiment is **PROPOSAL — NOT APPROVED**.

---

## 0. What already exists on the Titan — reuse before build

### 0.1 Evidence identity and how to read this

- [FACT] Product worktree `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `01ec2dc66af3e164014e479ab76b46cfe5046655` (`git rev-parse HEAD` this session). Commits after `d557f06` are tools and camera evidence only; every source line below is valid for `d557f06`.
- [FACT] On the device: host-006, build `e79d1ac0`, programmed 2026-10-14T17:58Z, identity MATCH, 99.9 fps, `led_faults 0`, readback 127 fields / 0 differences against the v9 preset (`runs/STATUS.md:201`). This is the lane's recorded readback; the integrator did not read the silicon. [FACT] STATUS ends at 19:29Z with the v2 swarm completion and the launch of this pass; none of v2's §9 experiments has a run record yet.
- Abbreviations (worktree-relative) carried from v2: **W** `src/k1/core/visual/inhouse_waveform.cpp`; **WH** `…/inhouse_waveform.h`; **A** `platform/ra8p1/inhouse_audio.cpp`; **PR** `platform/ra8p1/palette_runtime.cpp`; **PRH** `…/palette_runtime.h`; **G** `src/k1/core/audio/gdft_goertzel.cpp`; **SB** `src/k1/core/audio/sub_band.h`; **TT** `src/k1/core/audio/tempo_tracker.cpp`; **PDM** `platform/ra8p1/pdm_target.c`; **SENS** `platform/ra8p1/k1_pdm_sensitivity.h`; **LR** `platform/ra8p1/k1_live_runtime.h`; **FX** `platform/ra8p1/fixture_app.cpp`; **EP** `platform/ra8p1/emotiscope_port.cpp`; **CM** `platform/ra8p1/composed_motion.h`; **CSR** `platform/ra8p1/composed_scene_runtime.h`; **CSC** `platform/ra8p1/composed_scene_config.h`; **STATUS** `_scratch/waveform-migration-001/runs/STATUS.md`; **P6P** `_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted`; **COST** `P6P/cost-host-006/report.json`; **EXT2** `P6P/TOLERANCE-FROZEN-P6-EXT-v2.json`; **H6** `_scratch/waveform-migration-001/arm-p6-v9-host-006/` (receipt, `rtthread.map`, `symbols.txt`, `build.log`); **CAM** `docs/evidence/P6/camera/boom-host006-a-strip-40-70.{png,json}`; **PRESET** `docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json`.
- Other repositories: **CANON** `Kx-Titan/docs/migration/TITAN-CANON-2026-10-13.md`; **PL14** `Kx-Titan/docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md`; **TL11/TL14** `Kx-Titan/docs/migration/TITAN-SESSION-LESSONS-2026-10-11.md` / `-2026-10-14.md`; **NFB** `Kx-Titan/docs/migration/NATIVE-FIRST-BOOT-2026-10-08.md`; **LWA** `Kx-Titan/docs/migration/LIGHTWAVE-FIRMWARE-V3-PORT-ASSESSMENT-2026-10-12.md`; **K1** `/Users/spectrasynq/SpectraSynq_K1_Firmware` at `68a95773` (identical to main `a5e71961` for the files cited, per G2); **LW** `/Users/spectrasynq/Workspace_Management/Software/Lightwave-Ledstrip/firmware-v3` at `4d71bc01` (dirty working tree).
- Gap-closer ids: **G1-Fnn** (Titan existing machinery), **G2-Fnn** (K1 main director/onset), **G3-Pnn / G3-Fnn** (Lightwave library), **G4-nn** (canon rulings), **G5-xxx** (creative recovery). Final lever ids: **R01–R28** (R25–R28 new in v3), **R00** (mandatory safety layer), **CC-x** (canon-conformance track, unranked), **M0-x** (instruments).
- Labels: [FACT] = the line, commit, measurement or document was read (this session or by a named gap-closer whose citation this integrator spot-checked where marked ✓ in §10); [INFERENCE] = arithmetic or a join across facts; [HYPOTHESIS] = unmeasured. Status words: *documented* (comment/receipt), *implemented* (source), *compiled-in* (linked into host-006), *demonstrated* (host or device measurement in a receipt), *product-qualified* (look accepted by the Captain on the device).

### 0.2 Titan machinery that the v2 portfolio proposed to build, or ignored

The Titan product tree already contains working versions of several things v2 called "new". None of them runs on the in-house modes the Captain is watching (board 0 mode 32, board 1 mode 18). Most are tied to the donor (Emotiscope, GPL-lineage BENCH) path, to 128-pixel geometry, to a 16-slot palette snapshot, or to the compiled-out `AudioPipeline`. **The reusable asset is the rule and its mathematics, never the renderer or the publication** (§0.6).

| # | Capability | Where | Status (H6 = host-006 image) | Replaces / de-risks | Limits | Effect on effort |
|---|---|---|---|---|---|---|
| E1 | **Analytic edge→centre light front.** `composed_motion::Renderer::renderRibbon(…, inward=true)`: path = 1 − x, two bands separated by `separation_ms`, three-tap analytic trail (0.66/0.17/0.17, "never a frame decay"), smoothstep feather, gesture ids, Ready/Outward/Quiet/Return/Recovery, `holdQuiet`/`releaseQuiet` | CM:81-139 (inward start `phase_ = inward ? Phase::Return …` ✓; `path = returning ? 1 − x : x` ✓; mirror `output[63−i] = output[64+i]` ✓); host test `harness/tests/test_composed_motion.cpp:37, :58` | [FACT] implemented; compiled-in (symbol @0203da30, G1-F01); host-tested; ran on the device 11 Oct with real phases in telemetry (CURRENT-STATE.md:483-516) but **never optically judged** (camera gave near-black video) | R01 geometry; EXT-v3 planted positive generator; Q2 pre-screen | [FACT] 128 px (64 per half); colour from a 16-slot `PaletteSnapshot` captured once per gesture (the flattening path P removed); trails **added in RGB** (`c = add(c, scale(…))` ✓); travel valid only 200–8,000 ms (CM:192 ✓; CSC:47-48 ✓) vs R01's 100–180 ms; linear travel; gain ≤ ≈ 0.5 of palette level; clears output each frame (not an overlay); events while busy are discarded; skipped for in-house modes (EP:710 ✓) and in mixed mode "In-house output is not a participant" (EP:754-766 ✓) | R01 E 2 → 1.5 |
| E2 | **Two-board call/answer.** `composed_scene::Pair`: caller goes outward; at edge arrival + `answer_delay_ms` the other board answers **inward** with the caller's palette identity; counter mode; pair quiet barrier; tempo-grid launch gating (≤ 250 ms wait) | CSR:203-287; CSC:21-24; `docs/emotiscope-port-001/COMPOSED-SCENES.md:27-36` | [FACT] compiled-in (`Pair::render` @02043664, G1-F02); host-tested; exercised on the device 11 Oct (scene 2); not optically judged; bypassed whenever either board runs an in-house mode | R22 antiphony (D12 C09); Q2 direction pre-screen without a flash | donor-only, 16-slot snapshot, 128 px; launch driven by the donor body-event cursor | R22 E 2 → 1.5 |
| E3 | **Authored phrase machine.** `composed_scene::Direction`: Rest/Establish/Develop/Tension/Release/Recover; energy τ 0.8 s, trend τ 0.3 s, dwell ≥ 1.6 s; Tension at energy > 0.45 and trend > 0.02; Release on trend < −0.02 with novelty > 0.15, **or after 6 s**; live policy field 61 (0 off, 1 auto, 2 hold, 3 release, 4 rest) | CSR:38-82, :123-130, :151-154; `emotiscope_control.h:193, :218` | [FACT] compiled-in (inlined into `composed_scene::Board::render`); states reported on the device 11 Oct; never enabled in a judged show (the 13 Oct two-board Bloom ran phrase 0) | R13 skeleton (dwell, overrides, status word) | donor inputs; its own comment: "composition states … never detected song parts" (CSR:40); its Release fires on falling energy or a 6 s timeout, i.e. at the **cut**, never the drop | R13 E 4 → 3 |
| E4 | **Live, no-reflash controls per board.** Composed-scene fields 45–63 (scene, widths, travel, separation, quiet, return, trail, trips, feathers, recovery, choreography, initiator, answer delay, phrase, ml_target, quiet_scope); in-house: mode, palette, `speed` → `motion_scale = speed·2` (PR:250 ✓), mirror, **response gain 0–2** applied to `peak_scaled`, `vu_level`, `vu_average`, `observed_peak_scaled` before the clamps (PR:266-271 ✓; `k1_emotiscope_set_response_gain`, EP:821-825), Mood | `emotiscope_control.h:171-223`; PR:243-271 | [FACT] compiled-in; read every in-house frame | M0-8 partly exists; enables no-flash pre-screens Q3-pre, Q5, C17 puppet | response gain scales level **and** extent (not a pure contraction); no time-varying hold; release, cap, ceiling and k remain compile-time | M0-8 E down; R25 test needs no flash |
| E5 | **Onset detector body.** `k1::core::audio::OnsetBeat` (byte-port of K1 ONSET_V2, `c3e13ffd`): log-magnitude positive flux over bins 1–76 / kick 1–25 / snare 25–50 / hat 70–80; 14-frame median × 1.6 + 0.05; peak-pick (pre-max 4, wait 4); per-band refractory; monotonic per-class event ids; IOI beat-lock tail | `src/k1/core/audio/onset_beat.{h,cpp}`; host test `test_onset_v2_parity` | [FACT ✓] compiled but **linker-discarded**: H6 `rtthread.map` lists `OnsetBeat::update` (0x56c bytes) and `MusicalSaliency::update` (0x7c4) under "Discarded input sections" at 0x0 (map:234, :4430-4431, :4473-4474); only `OnsetBeat::reset()` is linked (symbols.txt:929, 0x02073510) | R05 detector body; monotonic ids solve latest-only loss (L20) for events | reads `AudioFeaturesV1.spectrum`, which inside `AudioPipeline` is the post-EMA/Mood/AGC spectrum (pattern P2); kick band starts at bin 1 (≈ 110–116 Hz), blind to sub-bass; constants derived for 133.33 Hz (K1 12,800/96), which equals today's Titan hop 24,000/180 — and changes if CC-1 changes the hop; **licence: Kx-Titan disposition GPL-3.0 legacy/uncertain, BENCH_ONLY, REFERENCE_ONLY** (G2-F03) | R05 host arm E 2 → 1; product route = clean-room (PRISM-Core music) |
| E6 | **Musical saliency + chord.** Four axes (harmonic, rhythmic, timbral, dynamic) with asymmetric rise/fall; `ChordDetect` (K1_CHORD_V2) root/type/confidence | `src/k1/core/audio/musical_saliency.*`, `chord_detect.h` | [FACT] compiled out of the live path (only `reset` linked) | R08/R07 harmonic-change input; R13 tension features; R16 root reference | post-AGC inputs; chroma inherits rounded-k error (do R06 first); "dynamic" axis is a frame difference of an AGC-levelled energy, so it cannot see the build→drop step (G2-F13) | low priority |
| E7 | **Four-band spectral AGC.** Bands < 200 / 200–800 / 800–3,000 / > 3,000 Hz; envelope, noise floor, gate (open 4× / close 2.5×), gain (target 0.25, 0.1–10), loud-guard trim | `src/k1/core/audio/gdft_postprocess.{h,cpp}` (:56-61, :130-196) | [FACT] compiled out of the live path | R09 per-band arm (K07 "louder → dimmer") | same lineage as K1 `K1_AGC_PERBAND_V1` but not verified bit-identical; 10 ms reference alphas, not 7.5 ms; does not by itself make the drop bigger than the build (G2-F08) | R09 per-band sub-arm E 2 → 1 |
| E8 | **Multi-candidate tempo field.** `TempoFieldV1` (8 candidates, phase01, phase uncertainty, fresh/coasting/stale, generation ids, ratio numerator/denominator, `derived` flag) + `deriveTempoPhase(p/q)`; `composed_timing::Clock` (challenger dwell, slew ≤ 20 %, 1.5 s coasting) | `src/k1/contract/tempo_field_v1.h`; `src/k1/core/audio/tempo_phase_tracks.*`; `platform/ra8p1/composed_timing.h` | [FACT] tempo field compiled out; Clock linked but donor-only; **no source ever sets `derived = true`** (octave candidates declared, never produced) | R23 data contract | inherits the 88 BPM prior and decimation 3; Clock uses the GPL donor tempi | R23 E 4 → 3 |
| E9 | **Two-board colour relation.** `EmotiscopeEdgeColour`: analogous / complementary / split-complementary / triadic / tetradic / saturation veil, pair modes, optional centre mask at 160-px geometry, resampled from the HD palette at u + offset, keeps per-pixel peak luma | `platform/ra8p1/emotiscope_edge_colour.h:9-150`; applied EP:173-187; live fields 28–32 | [FACT] compiled-in; live on donor modes; **disabled in the v9 preset** (`edge_colour_enabled false`, strength 0; G2-F09); the v9 path calls `sampleProductPaletteHd` directly (W:888-889) | R22 colour arm; R08 spread anchor | offsets are fixed palette-turn angles, not musical harmony; a 180° offset can land in a dark end of a non-cyclic palette (T-COL4) | R22 colour arm E 2 → 1 |
| E10 | **HD palette crossfade with bright landing.** `PaletteTransition`: smoothstep ≤ 10 s, interrupted fades preserved, phase aligned to each palette's brightest stop (`brightestPhase`), linear-RGB mix | `platform/ra8p1/palette_transition.h:10-100`; owned by PaletteRuntime `transitions_[2]` (PR:206-209) | [FACT] compiled-in; used by product channels and status; **not used by** `InhouseWaveformRenderer` | R07 chapter landing; T-COL4 dark-end walk | a palette change is a stronger gesture than an offset snap (taste) | R07 E 2 → 1.5 |
| E11 | **Gate telemetry already exported.** statusJson per board: `render_us`, generation, `capture_us`, fresh, `source_silence`, `raw_rms`, `quiet_entry_rms`, admission open, `peak_stage1`, position, `deposit_strength`, `injection_count`; donor BLM1 64-record bloom trace | PR:836-896; EP:353-359, :676-683 | [FACT] compiled-in | R15 / M0-3 | polled snapshot, not a per-frame ring; user and tonality gates not attributed | R15 E 2 → 1.5 |
| E12 | **Donor light modes on HD colour.** Analog, Spectrum, Octave, Metronome, Spectronome, Hype, Bloom; every colour from `sampleProductPaletteHd` per destination pixel; Spectrum = palette position equals strip position; anti-aliased swept palette dot | `platform/ra8p1/emotiscope_donor/light_modes.h:36-50`; EP:104-110, :142-150, :173-218 | [FACT] compiled-in; live-selectable per board; demonstrated on the device in many sessions; two-board Bloom rejected by the Captain 13 Oct (CURRENT-STATE.md:305) | Q12 pre-screen without a flash; R14 prior art (swept palette dot) | **GPL-lineage BENCH; not admitted to the product**; 64 bins from 12.8 kHz analysis; 128 px nearest-mapped to 160 | taste pre-screen only |
| E13 | **In-house v8 family modes** 8 (Waveform), 7 (Fast), 11 (Waveform Hybrid PCM shape) at 160 px | W:189-199, :395-503 | [FACT] compiled-in; live-selectable; root review "Waveform8 clearest darker space; Fast broader/brighter; Hybrid11 broad pink/red blending remains a weakness" (CURRENT-STATE.md:289); no Captain verdict | R22 alternative upper-board role; T-EXT contrast | 16-slot `familyColour`, not path P | live pre-screen |
| E14 | **Shaping primitives on the donor path:** `K1MusicalConditioning` body/detail roles with a rising-body event cursor; `EmotiscopeEdgeResponse` per-radius attenuation; `EmotiscopeMl` autoencoder at 25 Hz on the CPU | `musical_conditioning.h:11-192`; `emotiscope_edge_response.h:12-86`; `emotiscope_ml.*` | [FACT] compiled-in; donor-only; H6 receipt `npu: false` | R05 cross-check (body cursor); R09/R10 spatial-mask variant | edge response attenuates more as loudness rises (pattern P3 risk); ML latent has unknown training data and no claimed meaning (park) | — |
| E15 | **Analytic ribbon objects at 160 px, 16-bit:** `RibbonMaterial`/`RibbonProfile` (fractional radius, Gaussian variance growth, half-life, inward return leg ≤ 0.45 strength), `PairedWaveScene` (lead/lag edge delay 0.18 s) | `src/k1/core/visual/ribbon_material.*`, `ribbon_profile.h`, `paired_wave_scene.*` | [FACT] host-tested; **not linked into H6 and not in the `build_scalar.py` file lists** (G5-F03); **Captain-rejected appearance lineage**: "Motion or musical response is still unconvincing" (OPERATOR-VERDICT-001); "Still too mechanical or repetitive" / "Still unconvincing; the form or motion needs changing" (OPERATOR-VERDICT-002); revision 006 closed without acceptance | R14 implementation reference (continuous time, Gaussian rasteriser, fractional position) | velocity validated 4–240 px/s (too slow for a 100–180 ms front over 80 px, ≈ 450–800 px/s); consumes compiled-out `AudioFeaturesV1` | design reference only; **never port as a look** |
| E16 | **K1/DualMCU product effect catalogue** (38 ordinals: Percussion Burst, Pulse Prism, Tempo Comet Anticipate, River Surge, Spectrum River v1/v2, Snapwave, …) + `AudioFocusProfile` + live kick/snare/hat/transient gains | `src/k1/core/visual/product_catalogue.cpp:10-49`; `product_effect_renderer.cpp`; `platform/ra8p1/k1_performance_control.h` | [FACT] implemented, host-tested; not linked into H6 and unreachable under `K1_EMOTISCOPE_PORT` (PR returns before `renderProductChannel`) | R12 (class → gesture tables), R11 (guarded flywheel), R03 (River Surge arming rule) | Pixel8 (8-bit), outward-only, 16-slot paths; routing a whole effect into v9 would undo path P | rules and constants only |
| E17 | **River Surge build→cut detector:** fast τ 0.5 s / slow τ 20 s / peak τ 4 s; "build" remembered 2 s when fast/slow > 1.3; fires when fast < 0.45 × peak; 4 s refractory | `product_effect_renderer.cpp:44-55, :458-522` | [FACT] implemented; not linked | R03 ARMED entry; R13 BUILD→ARMED | detects the **cut**, not the drop; S3-scaled thresholds; 20 s warm-up | R03 P 0.6 → 0.65 |

Out-of-path visuals that must **not** be routed to the product: centre effects 100–103 (RIBBONS, AURORA_LAYERS, EMBER_TRAILS, PULSE_WAVES) are audio-independent and carry a non-zero base layer (0.005–0.04), which breaks both "always visualising music" and true black (`centre_palette_engine.h:10-97`; compiled out under the port, PR:488-498). `TempoReferenceScene` is a host-only instrument (useful for R23/R11 debugging; envelope 0 on silence) (G1-F19).

### 0.3 K1 ESP32-S3 product assets (lineage; not Titan code)

| Asset | K1 status | Use on the Titan |
|---|---|---|
| ONSET_V2 (`k1_onset_beat.cpp`) | implemented, compiled into `k1_hardware`, flashed since June; host A/B on 12 tracks: onset proxy F1 0.031 → 0.098, recall 6×; **device onset accuracy never measured**; K1's own ledger names the missing step: "full ON-02 needs an upstream un-clamped pre-AGC magnitude path" (forward-graft doc :120, :194, :213) (G2-F01) | E5 is its Titan port; feed it pre-AGC (R05) |
| Per-band AGC (`-DK1_AGC_PERBAND_V1=1`, `platformio.ini:187` ✓) | shipped ON; device band-balance spread 0.000 → median 0.145 (Lateralus); a 30 ms attack / 0.5 s release / ≈ 0.2 s settle leveller per band; the in-source comment "DEFAULT OFF … NO production env defines this flag" (`k1_gdft_core.cpp:520-538`) is stale (G2-F08) | fixes cross-band crush; **does not preserve build→drop contrast** — R09 still needs the long-window reference |
| EdgeMixer | shipped ON; Captain gate-3 plate A/B accepted (`0d837293`); split-complementary, strength 0.65, OKLab, centre mask; palette-honour moves each strip's palette position by ±0.208 turns (150° × 0.5), mixed at 65 % at the strip ends (G2-F09) | **the only Captain-accepted spatial palette-relation anchor in the lineage**; R08 first A/B k ≈ 0.2–0.4 (today k = 1); R22 colour relation |
| DROP_CUT (`-DK1_DROP_CUT_V1`, `platformio.ini:149` ✓) | shipped; darkens after 120 ms of energy < 0.025 while a ≈ 2 s peak-hold > 0.30 ("recently loud"); **relights instantly** when energy > 0.08; 1 s re-cut lockout; fails open; no device measurement found (G2-F10) | R03 entry rule (keyed on pre-AGC RMS on the Titan); its instant-relight exit is F-CAM1 in code form — a planted negative |
| `k1_mode_selection` governor | pure, allocation-free: manual owner, allow-list, min dwell, cooldown, max switches per window (G2-F06) | anti-thrash governor for R13 releases and R22 role switches (clean-room) |
| Smart Director / Beat-Aware Director / visual hooks | Smart Director default OFF; BUILD/BREAKDOWN delta test frame-rate-dependent and practically unreachable [INFERENCE, arithmetic]; DROP has no hysteresis and sets PHOTONS ×1.24; visual hook onset → PHOTONS ×(1 + 0.16·pulse); Beat-Aware Director **failed on device**: lock occupancy 0.417 (P3 solo) / 0.349 (P1+P3), anti-pin Δ −1.45 pp vs +5 pp target (`db6dd3db`) (G2-F05, -F07, -F11) | **do not port** classifier, PHOTONS boost or hooks (§0.5); the lock-occupancy measurement is independent device evidence for R11 and CANON:43 |
| HarmonixSet labels | 0 of 912 JAMS files carry drop annotations; all carry segment boundaries (G2-F14) | R13 negatives ("section change ≠ drop"); drop labels stay hand-made |

### 0.4 Lightwave library (firmware-v3) — the lineage the v2 atlas never cited

[FACT] 207 effect ids (206 active, 1 retired); 226 `registerEffect` sites, 20 behind `FEATURE_AR_1C_EXPERIMENTAL`; ≈ 131 audio-driven (G3-F01). [FACT ✓] Lightwave commit `0e2e0778` (2026-05-17 16:42 +0800) "Director Effect Registry Lock — default-deny allowlist" records the **Captain's locked Tier 1 registry** (`SynqMatrix.cpp:89`): TENSION includes K1 Waveform Hybrid 0x1313, LGP Bass Quake 0x1A03 and **Beat Pulse Resonant 0x1404** (✓ :110); IMPACT (drop) = **Ripple ES tuned 0x1200** (✓ :112) and **Time-Reversal Mirror Mod1 0x1B06**; ATMOSPHERE includes Perlin Shocklines (speed cap 1) and Saliency Bloom. These are the strongest recorded taste evidence about drops in any lineage. They are Lightwave/K1-hardware evidence, not Titan acceptance, and the selection was for Director auto-switching (G3-F02).

| Pick | Mechanism (G3) | Feeds |
|---|---|---|
| Beat Pulse Resonant 0x1404 (Tier 1 TENSION) | two rings **contract inward, edge → centre** ("Both contract inward from edge to centre", `BeatPulseResonantEffect.cpp:8` ✓); attack ring 280 ms linear travel, body ring 480 ms; body colour sampled at the ring's position (colour travels with the front); slams to 1.0 on every tick; 128 BPM metronome fallback when tempo confidence is low (`BeatPulseRenderUtils.h:350-370` ✓) | R01 template and A/B timing arm (Q15) |
| Time-Reversal Mirror Mod1 0x1B06 (Tier 1 IMPACT) | records a damped 1-D wave in a history buffer and replays it **backwards** so waves converge on the centre; timer-driven; per-frame min/max normaliser | R27 rewind-the-build |
| Beat Pulse Void 0x1403 | true black between hits; ring centre = 0.6·intensity, so the hit **implodes as it fades**; naming trap: "outward" variants of this family actually contract | R03 ARMED renderer; direction must be measured, never read from names |
| Perlin Shocklines 0x0B01 / Veil 0x0B00 (footage-identified, CANON:20) | peak-held momentum, push = level⁴ (0.8 vs 0.5 input → ≈ 6.6× push) driving **advection speed, never brightness**; release ×0.99 per 60 Hz frame (τ ≈ 1.66 s); brightness floors 48/255 and 0.18 | R25 velocity charge |
| Ripple ES tuned 0x1200 (Tier 1 IMPACT) | population of outward fronts; kick → speed ×(1 + 0.5·subBass); snare → hue +64; impact spent as front **speed and count**, not brightness | R12, R10, R26 |
| LGP Bass Quake 0x1A03 (Tier 1 TENSION) | shock re-arms only after the previous front passes 0.55 of the half (**position-gated re-arm**); history notes bass-pumped geometry was tuned **out** | R01/R13 re-arm clause; R10 risk |
| Transient Lattice 0x1A08 / Saliency Bloom 0x1A07 | charge integrator `memory·e^(−dt/0.68) + 0.20·transient`; activator minus 0.5× offset inhibitor (lateral inhibition) sharpens a front without raising luma | R26 charge; R01 crispness arm |
| Shockwave Cascade 0x1405 | release size as **number of fronts** (primary + 2 thinner echoes) | R26 |
| Star Burst Narrative 0x0A06 | harmonic commit only after phrase hold ≥ 1.5 s and chord confidence ≥ 0.55; circular key EMA; root at centre, fifth at the edge, third weighted by tension | R07 / R08 / R16 |
| SynqMatrix (show layer, default OFF) | Silence/Ambient/Steady/Build/Drop/Breakdown/Dense/Transition; Drop needs onsetEvent > 0.62 (or beat > 0.78 and onset > 0.45) with rms > 0.34 and a **≥ 250 ms candidate dwell**; 22 suppressed-reason and 10 classification-reason codes; per-state EdgeMixer modes (Drop COMPLEMENTARY, Build STM_DUAL, Steady/Breakdown SATURATION_VEIL) | R13 structure (never its numbers; its Drop rule is a planted negative); R15 reason taxonomy; R22 per-state relation |
| TranslationEngine | phrase change score 0.45·\|ΔRMS\| + 0.30·\|Δflux\| + 0.25·\|Δharmonic\|, hard boundary ≥ 0.18; RECOIL gesture 150–220 ms | R09 section reference; R13 arming evidence |
| Kuramoto Transport 0x1501 | invisible velocity field kicked by events carries visible deposits | R11 / R12 architecture |
| Attack-Only Pitch Velocity Field 0x2101 | chroma target captured only on onsets (60/250 ms follower) | R07 attack-only colour arm |

[FACT] DEC-009 presets Aurora, Ember (Fire) and Hush (Sinelon) contain **zero** `ctx.audio` references; porting them unchanged would breach the always-visualising-music mandate (G3-F03). [FACT] No Lightwave effect is ported to the Titan; five ported to the K1 S3 exist only in non-shipping envs (G3-F05). [FACT] CANON:48 rules "Lightwave effects land in the product tree later. No third tree."

### 0.5 Do-not-port list (each item must turn an R00 or black gate red if imported)

| Defect | Where | Why |
|---|---|---|
| 128 BPM synthetic metronome when tempo confidence is low | LW `BeatPulseRenderUtils.h:350-376`, `AudioReactivePolicy.h:61-70`; kept in the K1 S3 port | fake beat; breaks CANON:43 and exact-zero silence |
| White attacks and cores (`addWhiteSaturating`, Void core 0.6, Resonant 0.85) | LW `BeatPulseCore.h:76` | R00 white budget; use the palette's maximum-luma stop |
| Permanent brightness floors (Shocklines `qadd8(48,·)`, Veil 0.18, Beat Pulse base 0.5, Saliency floor) | LW (G3-F04) | breaks true black |
| Per-frame min/max normaliser | LW TRM Mod1 :389-407 | lifts room noise; breaks black |
| Frame-coupled integer advance | LW Shocklines :82-87 | same defect as ledger L28 |
| Hue-wheel colour ("C = red → B = violet") | LW `LGPChordGlowEffect.cpp:6-7, :32` | no rainbow defaults |
| Global brightness on the beat (Heartbeat, Breathing, Bass Breath, Beat Pulse Stack; K1 visual hooks onset → PHOTONS; Smart Director DROP PHOTONS ×1.24) | LW; K1 `k1_visual_hooks.cpp:122, :133`; `k1_smart_director.cpp:220-222, :265-270` | strobe law K13 |
| Frame-rate-dependent delta classifier; DROP without hysteresis | K1 `k1_smart_director.cpp:223-227, :398-404` | unreachable states and frame-rate flicker |
| Beat-quantised director | K1 `beat_aware_director.cpp` | failed on device (lock occupancy 0.35–0.42) |
| DEC-009 Aurora / Ember / Hush without a musical mapping | LW `factory_presets.h:45-70` | not music visualisation |
| Centre effects 100–103 | `centre_palette_engine.h` | audio-independent, non-zero base |
| `AudioPipeline::process` wholesale | `audio_pipeline.cpp`; `gdft_goertzel.cpp:43-75` | uses the parked int32-wrap kernel; reuse components fed by the int64 in-house analyser instead (G1-F09) |
| Ribbon/paired material as a look | E15 | Captain-rejected appearance lineage |
| `EmotiscopeMl` latent texture (D12 C13) | CSR:132-133, :148-150 | unlabelled model, unknown training, GPLv3 |

### 0.6 Port-and-adapt discipline

Every in-tree capability above is 128 px, 16-slot or Pixel8, donor-fed (GPL BENCH) or `AudioFeaturesV1`-fed (compiled-out pipeline). Routing into v9 therefore means: 160 px (80 radial pixels per half); per-pixel HD palette sample at output in palette-position space (path P), never a snapshot and never RGB addition; features from the int64 in-house analyser; new fields inert by default; `tests/host/test_p6_product_path_p.py` at `mismatches = 0` for the product default; and, for K1- or GPL-lineage algorithms, a **clean-room reimplementation in PRISM-Core** with the lineage port kept as a host tolerance-parity oracle (CANON:41; G2-F03; LWA MUST 8). The licence status of `onset_beat.cpp` is recorded as "uncertain", not settled; resolving it is a Captain/legal decision (§8.2 X10).

---

## 1. Plain-English summary for the Captain

1. **What you see today** (camera, host-006 on BOOM): the light is multicoloured, but it sits at the two ends of each panel with a darker middle. When the music cuts, the panel goes dark, which is right. The riser lights it again before the drop, and when the drop lands nothing sweeps inward. The drop has nothing left to release. (The "1.4 s early" timing came from a silent 30 fps recording lined up by brightness; under your 14 Oct ruling that timing is not admissible until it is re-recorded with sound in the file. The picture itself — ends lit, no inward sweep — stands.)
2. **Much of what v2 proposed to build already exists in the Titan code**, just not in the two modes on your plates. There is a working light front that travels from the edges to the centre, a call-and-answer between the two boards, a six-state build/tension/release machine, the K1 onset detector (compiled in but thrown away by the linker), the per-band level control the K1 ships, a multi-candidate tempo field, a two-board colour-relation module and a palette crossfade that always lands on a bright colour. The job becomes "route and adapt", which roughly halves several efforts.
3. **The best drop evidence in any lineage is yours.** On 17 May you locked Lightwave's Director list: for tension you picked an effect whose rings contract from the edges to the centre (Beat Pulse Resonant); for the drop you picked a ripple population and an effect that plays the build backwards so it converges on the centre. That raises the odds that the inward release is the right look. It is still your eye on the Titan that decides.
4. **The top next test is unchanged in spirit and cheaper:** fire a proper inward sweep at hand-marked drop times, built from the existing edge-to-centre front maths, and score it with a test that can see direction.
5. **The second test is the timing half:** run the existing K1 onset detector on the Titan's raw (unsquashed) audio on the host. Its rates already match the Titan. An earlier claim that the "hit" signal is about a third of a second late was wrong: in the existing probe data it crosses the build's level about 38 ms after the drop. The real problem is that it is diluted, not late.
6. **A new cheap idea to try with no reflash:** speed the light up during the riser using the existing live speed control, and see whether that makes the build feel tenser and the drop land harder (Lightwave's Shocklines does exactly this with motion, never brightness).
7. **Your rulings that change the plan:** the latency target is measured on both boards with a jitter limit; render on each audio hop at 120 Hz (a 200-sample hop gives exactly 120 Hz); 64-word capture blocks; timing evidence needs sound recorded in the same file. Those latency changes are required architecture, so they now sit beside the safety layer instead of competing for rank.
8. **Brightness:** the running image uses master 255 × board gain 0.25, roughly 64/255 in effect. That is neither the 24/255 of the global Titan rule nor the 100/255 default elsewhere. I report the conflict; it needs your ruling.
9. **Things not to copy:** the fake 128 BPM beat, white flashes, brightness floors, brightness-on-the-beat, three DEC-009 presets that do not react to music at all, and the K1 drop-cut's instant relight (that is today's early-return fault in code form).
10. **Safety comes with the drop work:** any explosive release ships with a white limit, a flash-rate limit and exact black in silence. Never a floor.
11. **Your eye decides only taste.** Several looks can now be pre-screened with no reflash (§7). The first one in the next three experiments is the riser speed-up on the lower plate.
12. **Nothing here is shipped.** The ship path is in §6.6.

---

## 2. Pipeline map: AP → mapping → VP → output

Signal order. Line numbers are at `01ec2dc`. In the v9 preset board 0 (lower) runs mode 32 (Waveform Hybrid K1, palette **selection 32 / catalogue index 31**) and board 1 (upper) runs mode 18 (Waveform Tempo, Crimson Gold **selection 39 / catalogue index 38**) (PRESET:33, :84 ✓; PR:248 `palette_id = palette − 1` ✓; CANON:51). v2 wrote "id 31 / id 38"; those are zero-based catalogue indices (TL11 TL-21 one-based vs zero-based trap). Levers point at §5 (R-ids) and §3 (ledger L-ids).

### 2.1 AP container — microphone to features

| # | Function (file:line) | What it does now | Ledger | Levers |
|---|---|---|---|---|
| A1 | Two LMD2718T261 capsules; U14 is the programme lane, U13 is captured and discarded (PDM:1-4, :607-610) | One capsule feeds the show | L4 | M0-6 witness lane |
| A2 | PDM hardware chain: sinc (SINCDEC/SINCRNG retuned, PDM:764-781) → HPF → compensation → half-band LPF ÷2. HPF coefficients `s0 0x3F61`, `k1 0x3EC1`, shift 0, from the pinned BSP copied into the build stage (`scripts/build_scalar.py:816-820`; staged `ra_gen/hal_data.c:613-619` in all 9 staged evidence copies and in `_scratch/waveform-migration-001/arm-p6-v9-host-006/stage/`). PDM has no HPF override | HPF runs on the sinc result at 2·Fout ≈ 82.8 kHz (MAN §50.3.4.3, §50.3.4.5); reset coefficients are the manual's "cut-off = 100 Hz" design for a 32 kHz filter clock | L1 | R04 |
| A3 | PCM width 16-bit [14:0] | 4 bits of a 20-bit option unused | L2 | R17 |
| A4 | ×16 gain + per-296-sample-slot loud guard, ceiling 30,000 (SENS:9-33; PDM:614-636) | A slot with raw peak > 1,875 has its gain cut to land at 30,000 | L3 | R17 |
| A5 | Slot 296 samples ≈ 7.15 ms; PDM clock MOCO-only, measured 41,417 Hz (PL14:147) | Capture-fill latency; resampler must track the rate; CANON:35 rules 64-word blocks (≈ 1.55 ms at 41,417 Hz) | L33 | CC-2 |
| A6 | Donor ASRC → 12.8 kHz (feeds donor modes and the admission gate) | Admission sees ≤ 6.4 kHz content | L6 | M0-3 |
| A7 | `k1_poly24` → 24 kHz, 180-sample (7.5 ms) hop; host-006 receipt `inhouse_analysis24: true` | In-house analysis rate | — | — |
| A8 | Hop RMS / VU, Q16 square per sample (A:228-246) | Samples with \|s\| < 128 add zero; error > 2 % only below −38 dBFS (IC `rms_truncation`) | L5 | R24 |
| A9 | Sub-band 40–110 Hz, two RBJ band-passes → `sub_rms` (SB; A:239-240) | Committed (`1416cb5`); read only by the disabled release | L11 | R10, R25 |
| A10 | Peak follower on `raw_peak_pcm − drive_floor`: attack 0.25 (≈ 30 ms), release 0.005 (≈ 2 s) (A:250); `peak_scaled` ratio smoothed 0.65/0.15 (A:253) | Ratio to "recent loudest", not level | L17 | R09 |
| A11 | Silence: RMS enter 0.04 / exit 0.08, 5 s dwell (A:256-266) | True-black gate | L32 | R15 |
| A12 | Note bank: 80 bins from offset 12 (bin 0 resonates 104.69 Hz), rounded integer k, Cross40 (G:19, :116), cap 2,000 (G:20), `>> 6` (G:55); live scalar kernel A:36-70 | int64 product and magnitude; int32 state with every overflow counted and the hop invalidated (A:37-42, :66-68, :308) | L7–L10, L12 | R06 |
| A13 | Magnitude EMA 0.3/0.7, τ ≈ 21 ms (A:274) | Smooths every consumer | L13 | R05 |
| A14 | Per-board Mood LPF (cutoff 1+10·mood Hz, coefficient at a 100 Hz basis, run per 7.5 ms hop; A:103-111, :318-326); mood 0.05 → τ ≈ 80 ms; mood 1.0 → τ ≈ 10.9 ms | Colour and novelty smoothing; live-settable per board (LR:87-90; FX:1546, :1666) | L14 | R05, R07 |
| A15 | Broadband spectral AGC, target 0.25, gain 0.1–10, τ ≈ 190 ms, gated (A:329-340) | One scalar for all bins | L15 | R09 |
| A16 | Tilt, soft knee > 0.5, clamp 1 (A:343-358) | Compresses the top half | L16 | R05 |
| A17 | `linear_shape[80]` pre-gain/pre-knee, post-Mood (A:346-350) | Path-P colour input | — | R07 |
| A18 | Novelty = √(mean positive difference) of the knee'd spectrum (A:357-367) | One scalar; the first hop after a step carries ≈ 2.7 % of the jump (D01 REPORT:43). [FACT, computed this pass from `P6-DROP-DETECTOR/v3/results/probe/boom_file.csv`, tempo-path `novelty_in`, 22.5 ms rows] first crossing of the build (45–50 s) p95 0.0692 is at 55.74 s, **+37.5 ms** after 55.7025 s; max within ±30 ms 0.057 < build p95. v2's "≈ 330 ms late" has no traceable source (D01 was never verified) and is withdrawn (§8.1 D23) | L18 | R05 |
| A19 | Tempo: 60–155 BPM, 88 BPM prior (TT:23), decimation 3 (TT:18), 512-update warm-up (TT:39); per board (A:280-300) | Half-time / 3:2 locks (P7) | L19 | R11, R23 |
| A20 | `acquire()` per board: spectrum, novelty, `linear_shape` and tempo come from `conditioning_[board]` / `tempo_[board]`; only `peak_scaled` is max-held over ≤ 8 missed hops (A:377-429) | Non-peak fields: latest hop only | L20 | R05 |
| A21 | Donor admission gate, PCM peak² open > 0.0048 / close ≤ 0.0034 (`composed_scene_runtime.h:31-32`; applied PR:254-264) | Peak hysteresis, no dwell | L32 | R15 |

### 2.2 Mapping container — features to visual parameters

| # | Function (file:line) | Mode 32 (board 0, lower) | Mode 18 (board 1, upper) | Levers |
|---|---|---|---|---|
| M1 | Freshness and dt (W:105-141) | capture age ≤ 30 ms | same | — |
| M2 | Response clamp (PR:265-272) | `peak_scaled`, `observed_peak_scaled` clamped to 1 | same | R09 |
| M3 | Level `l = clip((20 log10 vu + 33)/24)` (W:949; ceiling −9 dBFS WH:52) | amplitude g·l^1.5, g = 0.580092 (WH:38) | g = 0.830949 (WH:39) | R09 |
| M4 | Width `0.5 + 3.5·l` (W:826) | width = level | same | R10b (timbre) |
| M5 | Position / extent (W:974-977) | EMAs 16 ms → 23 ms (t50 ≈ 30 ms, t90 ≈ 76–80 ms); `80 + 0.7·80·peak₂`, cap 136 | `floor(80 + 80·peak)`, no EMA, reaches 159 (W:995-999) | R01, R02 |
| M6 | Release `updateReleaseV9` (W:752-795; WH:68-79) | **Off** (`enabled = false`, `inline constexpr`). When on: instant pull of the deposit to 80, decay 0.6 s (W:794, :977) | same pull, floored (W:999) | R01 |
| M7 | Accent `updateAccentV9` (W:725-749; WH:55-64) | Off (boost 0) | same | R01 bundle |
| M8 | Transport (W:963-1018) | 405·motion px/s (W:967), ≤ 8 steps/frame (W:968), integer steps; **outward only** on the in-house path (W:1003-1015); the donor composed scenes already draw analytic inward fronts (§0.2 E1); source 0..79 zeroed every frame (W:973) | `w·tempo_rate + (1−w)·energy_rate`, w ≤ 0.7 from confidence 0.30–0.60; tempo_rate = 24·BPM/60·(1+0.5 cos 2πφ) (W:988); energy_rate = 30 + 90·clip(novelty/0.12) (W:991); ≤ 30 steps (W:993) | R11, R14 |
| M9 | Chroma fold (W:154-176) / path-P centroid (W:596-625) | bins < `chroma_range` = 60 (WH:126) folded mod 12 → one circular centroid per frame | same, per board | R06, R16 |
| M10 | Path-P offset and anchor (W:630-657) | weak strength blends to an anchor | same | R07 |

### 2.3 VP container — parameters to a 160-pixel field

| # | Function | What it does now | Levers |
|---|---|---|---|
| V1 | Shift-register field `intensity_`, `colour_`, `offset_` [160] per board | whole-pixel outward scroll | R14 |
| V2 | `depositV9` (W:812-869): triangle, reach width+1, max-hold owner of intensity, colour and offset | repeated hits do not sum; winner keeps colour | R14 |
| V3 | `fadeV9` (W:800-807): τ 0.29 s (32) / 0.125 s (18), +10/s in confirmed silence (W:801) | constant release (approved fade-to-black) | keep; R22 |
| V4 | `outputV9` (W:872-915): mirror about 79/80; path P colour = `sampleProductPaletteHd(wrap(offset + d))`, d = distance/80 (W:888-889) | per-pixel HD colour; spatial ramp k = 1 | R08 |
| V5 | HD stop sampler clamps past the last stop | black→yellow seam on palette 31 | R08 |

### 2.4 Output container — field to the wire

| # | Function | What it does now | Levers |
|---|---|---|---|
| O1 | Display stage (PR:281-297): brightness 0.3+0.7b, warmth, white balance {1, 0.9375, 0.84}, hue-preserving tanh shoulder above 0.75 (PR:292) | Applied after the renderer frames the PAL metrics score | M0-1, R19 |
| O2 | `scale16` + board/lane gains → GRB48 TRUE16 (PR:687-737); donor modes on composition images take `emotiscopeWirePixel16` (PR:420) | 16-bit to the wire | guard |
| O3 | Code → light linear; WS2816C internal curve unmeasured; no degamma on Titan | unknown transfer | R19 |
| O4 | Tick 10,000 µs under `K1_EMOTISCOPE_PORT` (PRH:40; 8,333 µs is the other branch, PRH:47); 100 Hz measured | random 0–10 ms wait; beats against 133 Hz hops. Rulings: render per audio hop (CANON:35) and 120 Hz logical rendering (Kx-Titan AGENTS.md:9; TL14-037) | CC-1 |
| O5 | Wire: 80 px per DIN, two DINs per board in lockstep, 3,840 bits × 1.25 µs + 300 µs = 5.1 ms. Board 0 on P601/GPT6 + P604/GPT8 (LED DMA0/3); board 1 on its own top engine (host-006 receipt `titan_top_output: true`) | | — |
| O6 | Wire snapshot instrument (`scripts/palette_wire_snapshot.py`; packed lanes at FX:640-720) | Instrument | M0-1 |

### 2.5 Measured cost on the device (host-006, BOOM, 16,196 records, COST)

[FACT] Per record (5 ms donor hop): `ap_cycles` p50 983,358 / p95 1,252,397; in-house p50 699,201 / p95 964,658; donor p50 284,933; ASRC p50 133,484. Per record: `vp_cycles` p50 269,903 / p95 354,332; `emit_cycles_last` p50 127,960. `emit_errors` 0, `asrc_discarded` 0, 16,207 AP hops, 8,104 frames emitted. [FACT, S1 I8 citing STATUS.md:110] VP was 182.1k cycles on `b5595c35`; path P raised it about 48 %. [INFERENCE] At 1 GHz the AP is about 20 % of each 5 ms, VP plus emit about 4–8 %, so roughly 25–30 % of the M85 excluding RT-Thread, USB-CDC and ISRs. No idle counter exists; this is an estimate (M0-4). [FACT, G4-10] These figures come from the canon-conforming **titan-product** profile (H6 receipt: `-march=armv8.1-m.main+mve.fp+fp.dp`, `-ffp-contract=fast`, `-O3`, D-cache enabled, 448 vector instructions, 0 software-double call sites; `hal_entry.c:226-230` disables D-cache only under `K1_FORCE_DCACHE_OFF`), so CANON:42 does not void them and the CANON:50 violations are repaired in the running image. Emitted vector instructions are not measured acceleration (TL14-020).

### 2.6 Parallel machinery compiled in beside this chain

The in-house chain above is not the only light path in host-006. The donor composed-scene path (§0.2 E1–E3, E9, E12, E14) runs analytic inward fronts, two-board call/answer, an authored phrase machine and inter-board colour relations, and is selectable live per board; it is bypassed whenever a board runs an in-house mode (EP:710, :754-766 ✓). The K1-lineage audio components (E5–E8) are compiled but linker-discarded or unreachable. §0.2 lists what each would take to route into v9.


---

## 3. Compression ledger

Every step on the Titan today that destroys musical information. **Origin:** *S3-budget* (a cited ESP32 cycle or deadline reason), *aesthetic* (a cited look decision), *calibration* (P6 design), *inherited* (copied; reason unknown or not budget), *accident*, *hardware*. **Titan removes need?** asks only whether the M85 removes the original reason; taste is separate. Gain is the integrator's [INFERENCE] of visible value (H/M/L).

| L | Step (stage) | What is destroyed (numbers) | Origin (evidence) | Titan removes need? | Cost to undo | Gain | Lever |
|---|---|---|---|---|---|---|---|
| L1 | PDM HPF at reset coefficients (A2) | [INFERENCE, IC `hpf`, MAN §50.3.4.3] first-order corner 259 Hz at the 82.8 kHz filter clock: −16.3 dB at 40 Hz, −13.7 dB at 55 Hz, −8.2 dB at 110 Hz, −3.8 dB at 220 Hz. Model reproduces the manual's 100 Hz at 32 kHz (100.1 Hz). **Unmeasured on the device** | accident: only SINCDEC/SINCRNG retuned for 41.4 kHz; manual warns post-sinc filters "might change" off 32 kHz | yes; a ≈ 20 Hz DC block suffices (k1 0x3FE7, s0 0x3FF4 → 20.1 Hz, −0.55 dB at 55 Hz) | one register pair per channel; 0 CPU; needs a flash | H if real | R04 |
| L2 | 16-bit PCM window (A3) | 4 bits of headroom; 20-bit exists (`r_pdm_api.h:43`) | inherited BSP default | yes | int32 path through ASRC and analysis; re-check Goertzel headroom | M | R17 |
| L3 | ×16 + per-slot loud guard (A4) | any slot with raw peak > 1,875 is levelled to 30,000; a drop louder than the build can arrive at the same peak. Engagement rate unknown: only last-hop gain (4096) and clip counters (0) were captured on BOOM | inherited int16 storage (DualMCU analogue) | yes, with int32 | small; per-slot count first | H if it engages | R17 |
| L4 | Second microphone discarded (A1) | an independent acoustic channel | diagnostic design | DMA already paid | ≈ 133k cycles/hop for a second ASRC | instrument | M0-6 |
| L5 | Q16 RMS truncation (A8) | error −12 % at amplitude 256, −2 % at 590 (−38 dBFS), < 1 % above 1,000 — all below the −33 dBFS level floor and the 0.04 silence entry | inherited SQ15x16 | yes | trivial | L (no visible effect) | R24 |
| L6 | Donor 12.8 kHz feeds admission (A6) | admission blind above 6.4 kHz | inherited; S3 ADR-0008 | yes | gate on the 24 kHz path | L–M | M0-3 |
| L7 | `>> 6` before the resonator (G:55) | 6 bits per sample | unknown (present in SB 4.1.1); no budget citation | yes | re-freeze gains | L | R24 |
| L8 | Rounded integer k (G:121-128) | bin 0 at 104.69 Hz; bins 0–39 (k = 8) −79 to −86 cents; bins 40–79 (k 16–18) mostly +19 to +42 cents, one ≈ −48 | inherited; K1 kept it after an eyes-on possibly contaminated by the int32 overflow (`7cd9336`) | yes | 0 cycles (setup only) | M | R06 |
| L9 | Rectangular window | −13 dB side lobes | inherited exactness contract (K1 LANE4); SB removed Hann between 3.0 and 3.2 with no reason | yes | ≈ 35k multiplies/hop + coherent-gain correction | L–M | R06 (option) |
| L10 | Cross40 ×2 divisor (G:116) | half the bass pitch resolution | S3-budget (gate-bound selection under the 8,000 µs AP gate) | yes | +20.7k MAC/hop (35.3k → 55.9k, +59 %); longest window 2,000 samples ≈ 83 ms | L | parked |
| L11 | 110 Hz bank floor (G:18) | 40–110 Hz exists only as the unused scalar `sub_rms`, and L1 cuts that band first | inherited range placement | yes (CPU); window latency stays (≈ 150 ms at 55 Hz) | `sub_rms` already paid | H with L1 | R10 |
| L12 | 2,000-sample cap (G:20) | nothing today (largest block 1,834 at 24 kHz) | inherited | n/a | n/a | — | — |
| L13 | Magnitude EMA 0.3 (A:274) | attacks within ≈ 20 ms for every consumer | inherited SensoryBridge | yes for timing consumers | 0 | M | R05 |
| L14 | Mood LPF τ ≈ 80 ms at mood 0.05 (A14) | spectral motion above ≈ 2 Hz; colour lags ≈ 75–110 ms; novelty peaks 37–83 ms late (model) | aesthetic (live control) + accident (100 Hz basis at 133 Hz); the same defect class was already ruled and repaired to elapsed time on the donor path (TL11 TL-28 :343-351) | yes for timing; taste for display | 0 (move to consumers) | H (timing) | R05, R07 |
| L15 | Broadband spectral AGC (A15) | inter-section loudness and band balance | inherited K1 AGC v2; **K1 product now runs per-band ON** (`platformio.ini:187` ✓, K1 `68a95773`), a 30 ms / 0.5 s leveller per band that fixes cross-band crush but not build→drop contrast (G2-F08); a 4-band AGC already exists in the Titan tree, compiled out (§0.2 E7) | yes | trivial | H | R09 |
| L16 | Tilt / knee / clamp (A16) | top half of every bin; pre-knee ≥ 1.5 indistinguishable | inherited `VP_FIX_AGC_SOFT_KNEE` | yes for analysis | 0 (path-P colour already bypasses) | M | R05 |
| L17 | Peak follower ratio + clamp (A10, M2) | absolute level and section contrast. BOOM: VU −18.4 → −10.0 dBFS (last 2 s build → first 2 s drop), but `peak_scaled` median 0.844 → 0.900 and root 126.4 → 129.8 px (D02 `clamp_occupancy.json`) | inherited K1 "short leash" constants (#93763) | yes | trivial | H | R09 |
| L18 | Novelty on the conditioned spectrum (A18) | onset timing and the loudness step | inherited `calculate_novelty` | yes | ≈ 200 cycles/hop + per-band | H | R05 |
| L19 | Tempo: decimation 3, 88 prior, 60–155 BPM, 512 warm-up (A19) | beat time within 22.5 ms; metrical level; first ≈ 11 s | decimation S3-budget (indirect); prior inherited V2 (`TT:15-16`) | yes | P7 design work | M (board 1 visible) | R11, R23 |
| L20 | Latest-only publication for non-peak fields (A20) | ≈ 25 % of hops never their own frame [INFERENCE]; little visible loss today because those fields are smoothed ≥ 20 ms | inherited | yes | trivial | L today, M with R05 | R05 |
| L21 | Treble windows shorter than the hop (A12) | bins ≥ ≈ 2.35 kHz see part of each hop; bin 79 sees 40 of 180 samples | inherited | yes | ≈ 12k MAC/hop | L–M | R21 |
| L22 | 80 → 12 → 1 colour fold; `chroma_range` 60 (M9) | octave, polyphony; everything above ≈ 3.5 kHz never votes | inherited bloom design ("one position per frame by design") | yes | ≈ 26 µs/frame for richer signatures | H | R06, R08, R16 |
| L23 | `chroma_max` normaliser (W:157-163) | a loud drop is re-normalised in ≈ 0.2 s | inherited | yes | small | M | R07 |
| L24 | Level transfer clip −33…−9 dBFS (M3) | everything above −9 dBFS (34–35 % of BOOM drop frames at the old gain; re-measure at 0.580) | calibration (P6 matched light) | n/a | trivial | H | R09 |
| L25 | Deposit gains 0.580 / 0.831 (WH:38-39) | ≈ 42 % / 17 % of the cap-relative range unused (rescaled estimate, not re-measured); mode-18 shoulder can now engage | calibration | n/a | rule change | M | R09 |
| L26 | Extent polarity loud→edge, 0.7 cap, 16/23 ms cascade (M5) | the build and the drop draw the same picture; attack slope | aesthetic (0x1313/K1 "loud pushes the trace far from centre") | n/a (taste) | trivial | H | R01, R02 |
| L27 | Release and accent compiled off (M6, M7) | the drop never reaches the light as an event | P6 candidates failed on trigger timing; `inline constexpr` | n/a | flag (rebuild) | H | R01 |
| L28 | Outward-only integer transport, ≤ 8 steps/frame (M8, V1) | sub-pixel motion; any inward front; fronts faster than 800 px/s | inherited source-fidelity arithmetic; the 8 cap is K1 `WFHYB_MAX_SCROLL_STEP`; budget unproven | yes | ≈ 2k cycles | H (inward front impossible on the in-house path; exists on the donor path, §0.2 E1) | R01, R14 |
| L29 | Max-hold deposit (V2) | energy of repeated hits | calibration (K1 "never add palette colours") | partly (keep for colour) | low | L | R14 |
| L30 | Path-P spatial ramp d/80 (V4) | adds audio-independent colour; palette 31 has 17.5 % of positions below 5 % luma and a black→yellow seam | transplant of K1 `freq_prog` (pixel = frequency) into a waveform (pixel = time) | n/a (taste) | trivial | M–H | R08 |
| L31 | Display warmth / white balance after the metric tap (O1) | authored hue (board-0 warmth 0.59: G ×0.67, B ×0.50) | inherited Emotiscope display | n/a | trivial | M | M0-1 |
| L32 | Gates without attribution (A11, A21, freshness, quiet) | four or more paths darken a frame silently; silence entry RMS 0.04 (≈ −28 dBFS) may drain quiet music held 5 s [INFERENCE] | inherited | gates stay | reason word | M | R15 |
| L33 | 100 Hz tick, 7.15 ms capture slot (O4, A5) | 0–10 ms random wait; ≈ 7 ms capture fill | inherited "reconstructed presentation profile"; CANON:35 rules hop-triggered render and ≈ 1.6 ms blocks | yes | scheduler change | M (latency) | CC-1, CC-2 |
| L34 | Linear code→light (O3) | unknown; possibly crushed dark ends | accident (K1 degamma `38428a82` not ported) | measure first | LUT ≈ 1–2 % render | M? | R19 |
| L35 | Mirror (V4) | 50 % of spatial freedom | aesthetic (centre-origin identity) | n/a | — | keep | — |
| L36 | Brightness cap / gains (O2) | photon headroom only. Running envelope: master 255 × board intensity 0.25 (PRESET:127-128 ✓; packer PR:706-737) ≈ 16,384 codes ≈ 64/255-equivalent [INFERENCE, G4-07]; 6,169 levels would apply at 24/255 | owner rule (binding; conflicted chronology 24/255 → ≥ 100/255 → 255 × 0.25, §8.2 X1) | n/a | ruling only | — | — |
| L37 | Mode-11 180→96 nearest-sample display (A:216-225) | displayed waveform drops samples (mode 11 only) | geometry inheritance | yes | antialiased adapter | L | parked |
| L38 | Phase discarded (magnitude only) | onset/timbre cues | inherited | yes | covered by R05 | M | R05 |
| L39 | Onset front-end reads the post-AGC 80-bin spectrum (K1 ONSET_V2; its Titan port E5) instead of its own pre-AGC FFT | kick timing below ≈ 110 Hz; AGC attack shrinks the very frame of a hit; pinned bins give zero flux | **S3-budget**: the donor Lightwave OnsetDetector ran a 1024-point FFT on raw PCM, ≈ 400 µs/hop ≈ 5 % of an 8 ms S3 budget, 14 KB (LW `audio/onset/OnsetDetector.h:1-26`; K1 import-strategy doc :116-120, :182-202) (G2-F04) | yes: [INFERENCE] a Helium 1024-point real FFT ≈ 10–20k cycles ≈ 1–2 % of a hop | small | M–H | R05 arm C |
| L40 | Donor composed scenes: 16-slot `PaletteSnapshot`, RGB-added trails, 128 px, travel floor 200 ms (§0.2 E1–E2) | per-pixel palette position; fronts faster than 200 ms | inherited donor design | yes, when ported | port the maths only | — (donor path) | R01, R22 |
| L41 | Timing instrument: 30 fps video-only recorder (`titan_camera_record.py:31` `-framerate 30 … '{DEVICE}:none'` ✓) aligned by cross-correlating panel brightness with the WAV loudness (`titan_camera_strip.py:8, :24` ✓) | every light-versus-audio timing claim (the alignment absorbs any constant lag, so it is circular) | tool convenience (its own docstring: "a recording, not an instrument") | n/a | one ffmpeg argument (an audio device) + audio-to-audio alignment | H (evidence) | M0-11 |

Already undone on the Titan (keep and guard): TRUE16 to the wire; HD stop sampler; per-pixel colour (path P, on device); pre-knee colour shape; int64 product and magnitude in the live analyser; `output_precision` on in compositions; led-first DMA enforced for every port build (`build_scalar.py:626-627`); in-house analysis at 24 kHz.

**Answer to "why compress?"** [INFERENCE from the ledger and CANON:41] Of 41 rows, three carry a cited ESP32 budget reason (L10 Cross40, L19 decimation, L39 the onset front-end reusing the post-AGC spectrum instead of its own FFT), none of which binds at about 1 ms of AP per 5 ms. Six are aesthetic or calibration choices; the rest are inheritance or accident, kept because each port froze them for host bit-parity. The Titan canon now rules that bit-exactness is never a product requirement, only tolerance parity against a lineage oracle (CANON:41). Two deliberate looks must not be undone blindly: the symmetric spectrogram smoother (a slow-release variant was rolled back on 2026-05-20 for light-guide washout, SB9 `d408433`) and the Captain-calibrated 405 px/s scroll. The useful split is not "budget vs not" but **which consumer needs which view**: smoothing, normalising and folding are right for display and colour, and wrong for timing and dynamics. Today they are applied once, upstream, for every consumer.

---


## 4. Failure → lever map

### 4.0 What the device camera shows (host-006, BOOM 40–70 s) — carried from v2, re-graded under CANON:52

Recording CAM (`boom-host006-a.mp4`, panel rows 107–220 of 360, offset 5.733 s, alignment r = 0.579, 2,641 frames). [FACT ✓] The recorder captures **video only at 30 fps** (`titan_camera_record.py:31`: `-framerate 30 … -i '{DEVICE}:none'`), and the strip tool aligns time by cross-correlating panel brightness with the WAV loudness (`titan_camera_strip.py:8`, FPS 30 at :24). [FACT ✓] CANON:52 (Captain, 14 Oct, overrides earlier text): ACOUSTIC_TO_PHOTON is measured with the iPhone in 240 fps slow motion **with audio recorded in the same file**, plus on-device stamps; "a recording without received audio is invalid". [INFERENCE] Brightness-to-loudness alignment absorbs any constant light lag, so it is circular for any light-versus-audio timing claim.

- [FACT, seen] Multicolour on both halves: crimson, magenta, pink, gold, cream, varying along space and time. (Geometry and colour: admissible.)
- [FACT, seen] Bright bands sit at the ends of each half through the build, with a darker middle. (Admissible.)
- [FACT, seen] At the drop there is no sweep from the ends toward the centre in either half; after the drop the lower half shows pink fill across its middle while the upper half keeps a darker middle. (Admissible as geometry: "no release front".)
- [FACT, seen] The cut shows no LED structure (uniform grey field; auto-exposure lift or noise; no fixed-exposure capture exists).
- [HYPOTHESIS, downgraded from v2] "Light returns about 1.4 s before the 55.7 s drop" (lane reading ≈ 54.3 s, STATUS:203). The order of events (cut → light → drop) is visible; the **interval** depends on the brightness alignment at r = 0.58 and is not admissible until an audio-in-file recording (M0-11) or a wire-snapshot timestamp confirms it.
- [HYPOTHESIS] Full-width dark lines at about 57.4, 59.3, 64.5 and 66 s may be musical stops, a gate closing during music, or camera artefacts; their times carry the same alignment caveat. Correlate with a wire snapshot (R15).
- [HYPOTHESIS] Which camera half is which board is not established (palette selection 32 vs 39 can identify them). Cream regions may be camera clipping of gold stops.
- v2 wrote "the early return is probably a legitimate response to the riser". That is a musical judgement; under CANON:53 it is **agent cannot listen** until the Captain or an audio-capable route rules.

**F-CAM1, pre-drop re-entry** stays open as a choreography failure: the riser re-lights the plate before the drop, so the drop has no dark-to-light or contracted-to-extended contrast left to spend. [FACT] K1 DROP_CUT's exit rule ("relight instantly on any energy above 0.08", `led_utilities.h:448-514`) is the same failure written as code (G2-F10).

### 4.1 Titan failures

| ID | Failure (evidence) | Stage | Levers | Status |
|---|---|---|---|---|
| T-DROP | "Drop does not hit harder than the build" (Captain 14 Oct; v9 still "a no") | M5/M6/A10/A15/A18/M3 | R01, R03, R02, R07, R25, R26, R27, R09, R05, R04, R17 | open |
| F-CAM1 | Light re-enters before the drop (≈ 1.4 s by brightness alignment: timing [HYPOTHESIS] under CANON:52); no inward release (geometry [FACT, seen]) | A11/M5/M6 | R03, R25, R01 | open |
| T-EXT | Waveform hugs the edges; centre dark (P6 RECEIPT; CAM) | M5 polarity; mode 18 reaches 159 | R01, R09, R11 | open |
| T-REL | Release candidates X1–X3/K1–K3 reached EXT-v2 extent 1.0 within 40–70 ms when they fired, but by a centre teleport + outward stream, and fired on risers/climaxes or missed Sonic @54.7 (`P6P/results/ext/ext-v2-X{1,3}.json`; STATUS:176-177) | M6/M8 | R01 (geometry), R05/R13 (timing) | open; EXT-v2 cannot see direction |
| T-DET | Drop detectors v1 1/5, v2 1/6, v3 0/6 within +50 ms; sub arrives 72–117 ms after the musical drop; tempo-path novelty first crosses the build p95 +37.5 ms after the BOOM drop but its ±30 ms max stays below the build p95 (dilution, not a 330 ms delay; §8.1 D23) | A9/A12/A18/A19 | R05, R03 (arming), R13 | open |
| T-TEMPO | Half-time / 3:2 (BOOM 138 → 69/92; Sonic → 71; Dopex → 75); confident on silence; P7 best 35/326 (STATUS:200) | A19, mode-18 transport | R11 (bypass), R23 | open; visible on board 1 today |
| T-COL1/2/3 | One averaged colour per frame (v8); v9-B colour moved in 1 % of seconds; path-P loud-end stall | M9/V4/A16 | — | fixed (path P + pre-knee shape, on device) |
| T-COL4 | Drop renders dimmer: harmony walks gold→red into a dark palette end (P3) | M10/V5 | R07, R08 | open |
| T-B14 | Host fix not in the product default (PL14-B14) | system | M0-7 equivalence | closed for the palette; pattern persists (compile-time release/cap/ceiling) |
| T-C1 | Metric passed while the eye said no (channel-sum medians) | instrument | M0-1 | process fix |
| T-NOISE | AGC lifts room noise to music level (0.68 at RMS 0.018) | A10/A15 | R09 | open |
| T-DARK | Dopex dark at 76.3 s before the silence flag at 80.7 s | A11/A21 | R15 | untraced |
| T-WHITE | 8 Oct full-white half strip (Captain's verbatim in D06) | output | R00 | guard missing |
| T-SWAP | Top DIN halves swapped; scene override; alias plant falsely green (D05 T19–T23) | O5 | M0-7 symmetry test | repaired; untested by the Captain |
| T-BEAT | Beat barely reaches the plate (camera reach ≈ 13 %, P5) | A18/M8 | R05, R13 | open |
| T-LAT | ACOUSTIC_TO_PHOTON p50 ≤ 8 ms / p99 ≤ 12 ms / jitter (p99 − p50) ≤ 4 ms, first visible change on **both** boards, product profile, real music + unpredictable onsets, ≥ 500 onsets (CANON:31-32, :52 ✓); "worst case" retired (CANON:33 ✓); arithmetic p50 today ≈ 18–20 ms [INFERENCE, G4-03] | A5/O4/O5 | CC-1, CC-2, CC-3 | unmeasured |
| T-STALL | poly24 filter (re)design can stall a frame 3–6 ms; poll high-water 8.67 ms (PL14-D8); host-006 `poll_max_cycles` 809,216 after the run (COST); the frozen native lane already has the repair design (coefficient cache by exact source rate, 492 hops bit-identical on host; NFB:202-213) | A7 | CC-2 | open |
| T-INSTR | The only device recordings are 30 fps video without sound, aligned by brightness↔loudness (L41); CANON:52 makes such a recording invalid for timing | instrument | M0-11 | open (tools) |
| T-NEG | Captain-rejected appearance lineage: ribbon audition 001 "Motion or musical response is still unconvincing"; paired candidate "Still too mechanical or repetitive" / "Still unconvincing; the form or motion needs changing" (`docs/effects-language-001/evidence/OPERATOR-VERDICT-001.md`, `-002.md`); unchanged two-board Bloom rejected 13 Oct (CURRENT-STATE.md:305) | VP | R01, R12, R26 must avoid repeated identical packet trains | binding negative evidence |

### 4.2 S3 / upstream failures, and whether they still live on the Titan

| ID | Failure | Fixed on S3? | Still on the Titan? |
|---|---|---|---|
| K01 | 12 repeating hues (`note_colors[i%12]`) | yes | no (palettes) — never return to hue wheels |
| K03 | Flat yellow / all white from summed palette RGB | yes | no (max-hold, path P) |
| K04 | 16-slot uint8 palette | yes (SB `d408433`, K1 `c2738b88`) | no |
| K05 | Palettes duller when louder (post-gate chroma freeze, K1 `ae5d90a`) | yes | partly: novelty and tempo still read post-squash |
| K07 | Louder → dimmer (broadband AGC) | yes (per-band ON) | **yes** (L15); a 4-band AGC is in the Titan tree, compiled out (E7) |
| K08 | "30–40 % dimmer, sluggish, very short leash" (0.25/0.005 follower) | partly | **yes**, same constants (A:250) |
| K11/K12 | Twitchy motion from over-fast denominators | yes | watch in R02 |
| K13 | Strobe law: never global brightness on the beat (#58407) | rule | must be encoded in R00 |
| K14 | Half-time lock (Beauty 74 vs 128) | no | **yes** (T-TEMPO) |
| K15 | 8-bit dither flicker at dim tails | yes (WS2816 16-bit) | no for flicker; but CANON:35 reserves **24-bit + dither as a Captain latency option** if measurement shows the 48-bit wire breaks p50 (G4-03) |
| K16 | Harness PASS while the device was dark; rainbow test palette | process | pattern persists |
| SB9 | Slow spectral release → milky LGP washout | rolled back | constraint on every release lever |
| K-overflow | int32 Goertzel zeroed loud bins | yes (int64) | no on the live path; trap remains in the parked kernel |
| K-DIR | Smart Director: frame-rate-dependent BUILD test, DROP without hysteresis driving PHOTONS ×1.24 (G2-F05) | default OFF on K1 | not ported; keep out (§0.5) |
| K-LOCK | Tempo lock occupancy on music 0.35–0.42; beat-quantised director FAIL; anti-pin Δ −1.45 pp vs +5 pp (G2-F07, `db6dd3db`) | no | same lineage on the Titan (TT is a port of K1 `c3e13ffd`); any lock-dependent lever pays ≈ ×0.4 in expected value |
| K-CUT | DROP_CUT relights instantly on any energy > 0.08 (G2-F10) | shipped as is | Titan has no port; the instant relight is F-CAM1 in code — planted negative for R03 |
| LW-5L | Lightwave 5-layer AR family: stacked smoothing ≈ 420–430 ms attack latency and floors ≈ 51 %; fixed by one 50 ms EMA + 58/500 ms max follower (`aed805bb`; memory #40126, DEGRADED) | yes (Lightwave) | independent lineage confirmation of P7 / L13 / L14 |
| LW-128 | 128 BPM metronome when tempo confidence is low | no (kept in K1 S3 bench ports) | absent; keep absent (§0.5) |

### 4.3 Recurring patterns and where each still hides

| Pattern | Paid for | Still hiding |
|---|---|---|
| P1 Many collapsed into one too early | 16-slot palette; one colour per frame | novelty scalar (80 changes → 1); one tempo winner; `chroma_range` 60; one `peak_scaled` → extent; level sets amplitude **and** width |
| P2 Reading after the squash | post-gate chroma freeze; post-knee stall | novelty and tempo post-Mood/AGC/knee; accent flux on conditioned bins; "pre-AGC" level is post-guard; admission on the 5.2 kHz donor stream |
| P3 Louder gives less | K07, K08, T-DROP, T-COL4 | follower ratio + clamp; loud guard; HPF removing drop bass; mode-18 loud deposit at 159 |
| P4 Input-chain physics ignored | bench mic starved (K10) | HPF corner; 16-bit window; **host replays omit the HPF and the guard**, so host-scored drop levers see cleaner input than the device |
| P5 Host ≠ product | PL14-B14; K16; T-SWAP | metrics tap before warmth/WB; Pixel8 preview bytes; per-board Mood confounds; compile-time release |
| P6 Proxy metric | C1 channel-sum; "9–16 positions" produced by the d ramp | **EXT-v2 cannot see direction and its planted positive is a teleport**; PAL spread passes with audio-blind offsets; camera luma without locked exposure |
| P7 Timing smear | Mood LPF; 16/23 ms cascade; half-time | colour lags ≈ 0.1 s, so the first ≈ 0.2 s of a drop wears the build's colour |
| P8 Unattributed darkness | Dopex 76.3 s | four gates, no telemetry; camera dark lines |
| P9 Safety by luck | 8 Oct full white | any explosive release or iris without a white budget and flash-rate limit |
| P10 Built but switched off treated as absent | — | release/accent; `sub_rms`; OnsetBeat/MusicalSaliency (`update()` linker-discarded in H6); 4-band AGC; TempoFieldV1; PaletteTransition and EdgeColour (not on v9); composed inward fronts and Pair (donor only); second mic; 20-bit PDM; sound-detection IRQ; U55; M33; PRISM-Core clean-room music runtime |
| P11 The wrong code path gets the blame | — | the parked kernel's int32 wrap was attributed to the live analyser (D03, S2) |
| P12 A protection becomes the look | — | loud guard, follower, clamps and gates now shape the show |
| P13 Inherited constants re-frozen for bit-parity | int64 fix kept on K1 | rounded k, `>> 6`, Mood 100 Hz basis, integer scroll — CANON:41 now permits tolerance parity |
| P14 Existing work rebuilt as new | — | v2 proposed new drawing code for an inward front, a new phrase machine and a new onset detector, all of which exist (§0.2) |
| P15 Names lie about direction | — | Lightwave Beat Pulse "outward" rings contract (ring centre = 0.6·intensity, G3-P03); measure direction (EXT-v3), never read it from a name |
| P16 An instrument without the signal it judges | — | camera timing from a silent recording aligned by brightness (L41, T-INSTR) |


---

## 5. Ranked opportunity portfolio — PROPOSAL — NOT APPROVED

**One list of 20.** I = visual impact, N = novelty (added product value relative to today's Titan show), E = effort **to the smallest decisive result, counting machinery that already exists** (§0), each 1–5 (halves allowed where existing machinery removes part of the work); P = probability of a visible improvement if executed [HYPOTHESIS]. **Rank score = I × N × P ÷ E** (expected value, as in v2 D6); I×N÷E shown for comparison. Ties break on relevance to the open verdict "the drop does not hit harder", then lower effort, then a stated reason. Scores are judgements, not measurements. Lock-dependent levers carry a ×≈ 0.4 expected-value penalty unless they degrade gracefully (K1 lock occupancy 0.35–0.42 on music, G2-F07).

**Rules for every experiment.**
- Freeze the criterion with `scripts/freeze_tolerance.py freeze` before any result exists; `verify` after.
- A metric is admissible only after it goes **red on its planted negatives** (host-006 replay, `b5595c35`, and the named lineage negatives in M0-14) and **green on a planted positive** that has the claimed property.
- Host replays of drop levers run twice: as today, and through the M0-5 HPF + loud-guard pre-stage (pattern P4).
- **Camera clauses (CANON:52):** geometry, direction and colour from any framing-matched recording; **timing only from a recording with received audio in the same file** (iPhone microphone; MacBook fallback) via M0-11, plus device stamps; ms-scale latency only from 240 fps slow motion with ≥ 500 onsets. Luma ratios only from packed 16-bit wire snapshots until a fixed-exposure capture exists. Brightness-to-loudness alignment is never evidence of timing.
- **Snappiness oracle (CANON:19):** every lever that changes the audio backend or the scheduler (R04, R05, R06, R17, R21, R23, R24, CC-1, CC-2) carries a frozen non-regression clause — ACOUSTIC_TO_PHOTON p50 and p99 on both boards ≤ the host-006 baseline for modes 32 **and** 18 — plus a Captain-eye A/B. Levers that deliberately withhold response (R03, R09, R13, R14, R25, R27) carry a non-regression clause outside their ARMED/BUILD windows. Until the device baseline exists (CC-3), the host proxy is: onset → first changed packed frame in the host replay not increased.
- **Lineage and licence:** every record cites the lineage the behaviour comes from and what it already tried and rejected (CANON:14). Lightwave and K1 primitives pass the import checklist (M0-13) before scoring; K1-lineage GPL-uncertain code reaches the product only as a PRISM-Core clean-room reimplementation scored against the lineage port on host (§0.6).
- Every run includes a real silence segment whose packed codes must be exactly 0. No rainbow. No floor. Dopex stays out of tuning. Carte Blanche is spent.
- Device steps (build, program, readback, record, live-setting changes) are performed by the P6 lane owner under existing authority. Nothing here grants it.

### 5.0 Mandatory and unranked

**R00 output constraint layer** (unchanged from v2). Before the packer: zero full-scale-white pixels and lit energy with HSV saturation < 0.25 ≤ 10 % per song; ≤ 3 large luminance flashes per second (change > 25 % of the darker state); all channels exactly 0 within the authorised drain after zero PCM. Never a floor. **Frozen:** those three clauses. **Planted:** an 8 Hz strobe flagged; a white stop flagged; a 1/1024 floor fails true black; Lightwave white core (0.85) and `qadd8(48,·)` floor imported unmodified must both turn it red. [FACT, G4-14] R00 sits before the packer and **cannot catch transport-induced white** (wire-edge faults, 8 Oct, NFB:117-122); that is CC-1's transport clause. Required before any R01/R03/R09/R25/R26/R27 image reaches the device.

**Canon-conformance track (CC; ruled by CANON:26, :31-35, :52; judged by the canon and the snappiness oracle, not by visual-impact score).** v2 ranked these 18th and 22nd by visual impact; the canon makes them required architecture ("keep 8 anyway, because it forces the right architecture", CANON:26 ✓).

| ID | Content | Was | Records |
|---|---|---|---|
| CC-1 | Render triggered by the audio hop at **120 Hz logical**, one present time for both boards | v2 R18 (133 Hz) | §5.4 |
| CC-2 | 64-word PDM capture blocks (≈ 1.55 ms at 41,417 Hz) + poly24 coefficient cache by exact source rate | v2 R20 (64–96 samples) | §5.4 |
| CC-3 | ACOUSTIC_TO_PHOTON budget ledger and host-006 baseline (per-onset device stamps + 240 fps audio-in-file recordings) | new (G4-03, G4-06) | §5.4 |

### 5.1 Ranked table

| Rank | ID | Lever | I | N | E | P | I×N÷E | **Score** | Drop? | Reuse (§0) | From |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | R01 | Inward release front on labelled drops, ported from the existing edge→centre maths, scored by EXT-v3 | 5 | 4 | 1.5 | 0.75 | 13.3 | **10.0** | yes | E1; LW Resonant (Tier 1), Bass Quake re-arm, Saliency Bloom inhibition | v2 R01; G1-F01; G3-P01; G5-F01 |
| 2 | R05 | Fast event path: the dormant OnsetBeat re-fed **pre-Mood/pre-AGC**, monotonic event ids per class; product via PRISM-Core clean-room | 4 | 3 | 1 | 0.7 | 12.0 | **8.4** | yes | E5; K1 ONSET_V2; PRISM-Core music | v2 R05; G1-F06; G2-F01/F02/F04 |
| 3 | R03 | Hold contracted through the riser, contracting **progressively** with build charge; K1 DROP_CUT entry, never its instant relight | 5 | 4 | 2 | 0.65 | 10.0 | **6.5** | yes | E3, E17; K1 DROP_CUT; LW Beat Pulse Void | v2 R03; G1-F05; G2-F10; G3-P03; G5-C01 |
| 4 | R02 | Extent instant attack (asymmetric follower; release unchanged) | 4 | 2 | 1 | 0.8 | 8.0 | **6.4** | yes | LW Progressive Ignition 0x1E04 | v2 R02; G3-P11 |
| 5 | R07 | Colour chapter carried by the release; bright landing via `PaletteTransition`; attack-only colour arm | 4 | 4 | 1.5 | 0.6 | 10.7 | **6.4** | yes | E10; LW 0x2101, Star Burst | v2 R07; G1-F12; G3-P09/P15 |
| 6 | R25 | **Riser velocity charge:** the riser drives scroll velocity (power-four, peak-held), never brightness; first as a no-flash live-speed puppet | 4 | 3 | 1 | 0.5 | 12.0 | **6.0** | yes | E4; LW Shocklines/Veil | **new**: G5-C17; G3-P04 |
| 7 | R04 | Re-derive the PDM bass-cut (HPF) for the real rate: host replay first, then witness-lane sweep | 3* | 4 | 1 | 0.5 | 12.0 | **6.0** | yes | — | v2 R04 |
| 8 | R06 | Colour tuning truth: exact bin frequencies, `chroma_range` 80 | 3 | 3 | 1 | 0.6 | 9.0 | **5.4** | — | — | v2 R06 |
| 9 | R08 | Path-P dark ends + spread anchored at K1 EdgeMixer (first A/B k ≈ 0.2–0.4), opening on release | 4 | 4 | 2 | 0.6 | 8.0 | **4.8** | — | K1 EdgeMixer (Captain-accepted); LW Star Burst | v2 R08; G2-F09; G5-SKETCH |
| 10 | R09 | Section-relative dynamics + iris, **one normaliser per field per consumer**; per-band AGC arm from the in-tree 4-band AGC | 5 | 3 | 2 | 0.6 | 7.5 | **4.5** | yes | E7; LW TranslationEngine | v2 R09; G1-F08; G2-F08; G4-15 |
| 11 | R15 | Darkness truth: per-frame gate-reason ring from the existing status fields | 4 | 3 | 1.5 | 0.55 | 8.0 | **4.4** | — | E11; SynqMatrix reason codes | v2 R15; G1-F18 |
| 12 | R26 | **Stored build charge sets release size** (front speed, echo count), never brightness | 4 | 4 | 2 | 0.5 | 8.0 | **4.0** | yes | LW Transient Lattice, Shockwave Cascade | **new**: G3-P08/P12; G5-C01 |
| 13 | R10 | Timbre width + sub push (displacement, never brightness) | 4 | 4 | 2 | 0.5 | 8.0 | **4.0** | — | SB `sub_rms`; E14 mask | v2 R10 |
| 14 | R11 | Groove flywheel for board 1 (onsets kick an invisible velocity field; no tempo lock) | 4 | 3 | 2 | 0.65 | 6.0 | **3.9** | — | LW Kuramoto Transport | v2 R11; G2-F07; G3-P14 |
| 15 | R13 | Tension state machine re-homed from `composed_scene::Direction`; observed evidence fires, predictions only arm | 5 | 4 | 3 | 0.55 | 6.7 | **3.7** | yes | E3, E17; K1 governor; SynqMatrix codes | v2 R13; G1-F03; G2-F06; G3-P07; G5-F04/F05 |
| 16 | R12 | Push-pull: kicks pull inward, snares bloom outward (needs R05) | 4 | 4 | 2 | 0.45 | 8.0 | **3.6** | — | E16 gesture tables; LW Ripple ES | v2 R12 |
| 17 | R27 | **Rewind the build:** replay the board's own recorded build field backwards (edge→centre convergence), then release | 5 | 5 | 3 | 0.4 | 8.3 | **3.3** | yes | LW Time-Reversal Mirror Mod1 (Tier 1 IMPACT) | **new**: G3-P02 |
| 18 | R14 | Fractional transport + swept deposit in palette-position space | 4 | 3 | 2 | 0.55 | 6.0 | **3.3** | — | E15 reference; E12 swept dot | v2 R14 |
| 19 | R22 | Two-board roles: EdgeColour offset on v9 + call/answer grammar (no-flash probe first) | 4 | 3 | 1.5 | 0.4 | 8.0 | **3.2** | — | E2, E9; K1 EdgeMixer; SynqMatrix per-state relation | v2 R22; G1-F02/F11; G5-F02 |
| 20 | R17 | Input headroom: per-slot guard engagement, pre-guard level side channel, then 20-bit PCM | 4 | 3 | 2 | 0.5 | 6.0 | **3.0** | yes | `k1_pdm_target_last_hop_gain_q8()` | v2 R17 |

\* R04 impact is capped at 3 until a host replay shows its effect on extent, EXT or onset strength (follower and AGC sit after it; CRIT G7).

Tie notes. R02 and R07 (6.4): both drop levers; R02 has lower effort. R25 and R04 (6.0): both drop levers with E = 1; R25 first because R04's impact is capped pending §9 Exp 2. R26 and R10 (4.0): R26 is a drop lever. R27 and R14 (3.33 vs 3.30). R17 and R16 (3.0): R17 is a drop lever, so R16 drops to 21.

**Below the line (scored, kept, not in the 20):** 21 R16 circle-of-fifths colour order (3/4/1/0.25 → 3.0; taste); 22 R28 section memory and recall (4/4/3/0.4 → 2.1; new, G5-SECMEM); 23 R19 WS2816 transfer (4/3/3/0.5 → 2.0); 24 R21 treble coverage (2/4/2/0.5 → 2.0); 25 R23 tempo metrical repair (4/3/3/0.35 → 1.4; E lowered by E8, P lowered by K1 lock occupancy; P7 lane); 26 R24 arithmetic hygiene (1/2/1/0.5 → 1.0).

**Ranking changes against v2 and why.** R05 rises from 6 to 2 (existing detector, rate-matched, host-parity-tested: E 2 → 1; I lowered 5 → 4 because the event path alone changes no pixel except the mode-18 energy rate). R07 rises from 7 to 5 (`PaletteTransition` exists). R03 rises from 3 (P 0.6 → 0.65, entry detectors exist). R13's score rises (E 4 → 3, 3.4 → 3.7) but it ranks 15th because new levers enter above it; it stays behind the labelled-geometry bundle. R15 rises (telemetry half-exists). R18 and R20 leave the ranking for CC-1/CC-2. New: R25, R26, R27 (from Lightwave and D12, previously folded or unrecorded) and R28 (below the line). R16 and R19/R21/R23/R24 fall below the line.

**Instruments (M0, enable everything; no visible change).** M0-1 packer-tap scoring after warmth/WB; M0-2 EXT-v3 direction clause; M0-3 gate-reason word (now R15's ring); M0-4 idle counter + per-stage worst-case counters; M0-5 host HPF + loud-guard pre-stage; M0-6 second-mic witness lane; M0-7 product-equivalence test extended to Mood, release, cap, ceiling + DIN symmetry byte test; M0-8 runtime A/B knobs (release enable, T_r, cap, k) added to the existing `InhouseWaveformControls` (WH:119-130) and read in `renderInhouse` beside speed and response gain, defaults hash-bound to host-006; M0-9 move the 3.2 GB replay set `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6` to `/Volumes/Disk Storage/SpectraSynq-Scratch` (volatile today); **M0-10** acoustic-to-photon budget ledger (= CC-3); **M0-11** audio-in-file camera recorder: `titan_camera_record.py` takes an audio device instead of `:none`, and `titan_camera_strip.py` aligns recorded audio to the played WAV (audio-to-audio), never brightness to loudness — a tools change, not firmware; **M0-12** effective-envelope readback per board (master × board intensity × lane gain as one full-scale number; catches drift like TL14-055 100 → 255); **M0-13** lineage import checklist: grep each imported primitive for metronome fallback, `addWhite`/white cores, `qadd8(constant)` floors, normalise-to-range, CHSV/hue wheels, brightness-on-beat and 16-slot snapshots; each hit removed or turns R00/black red; **M0-14** planted-negative library: SynqMatrix Drop rule (≥ 250 ms dwell), D12 centre-born outward release, k-collapse at the drop, snare-bloom luma, 3× dimmed build, composed scope-0 screensaver, K1 DROP_CUT instant relight, 128 BPM metronome, Direction auto-release at the cut, donor 16-slot snapshot colour, linear 200 ms travel.

**Parked, with reason** (v2 list, amended): Cross40 removal (+59 % MAC, 83 ms bass window; use R05 for timing); lowering the bank below 110 Hz (≈ 150 ms window at 55 Hz; R05 arm C covers onset timing); lengthening the spectral release (SB9 washout rollback); donor VU and 96-tempo bank in in-house modes (GPL BENCH; licence first; ×3.93 is quiet-cut → drop); U55 model (no measured need); predictive downbeat release (R23 first; CANON:43); `AudioPipeline::process` wholesale (never; component reuse only, §0.5); int64 resonator (already int64); 2,000-sample cap (binds on no bin); PDMIFCLK crystal source (MOCO-only, PL14:147); edge-origin everyday grammar (taste fork, §7 Q13 — not a lever until the Captain picks it); tempo-adaptive anticipation, phrase-aligned look changes, beat-aware decay, metric hierarchy (all blocked on R23, G5); Palmer colour bias, negative-space glints, alignment-as-consonance, implosion drain, ember tails (G5 scores 3.0–4.5 I×N÷E with P ≤ 0.35; behind R19/R21/R22/R15 dependencies).

### 5.2 Drop bundle and multipliers

[INFERENCE] The drop failure is a conjunction. Release geometry with direction (R01) and size (R26), contrast held through the riser (R03) with tension carried by velocity (R25), on-time attack (R02), colour that arrives with the release (R07), a section-scale reference with headroom (R09), and input truth (R04/R17) may each be necessary and none sufficient. Interaction matrix (✚ multiplies, ⟂ independent, ⚠ conflicts):

| | R02 | R03 | R25 | R26 | R27 | R07 | R09 | R05 |
|---|---|---|---|---|---|---|---|---|
| R01 front | ✚ deeper contraction to release | ✚ contrast reserved | ✚ velocity charge released at t_d | ✚ size set by charge | ⚠ alternative gesture (A/B, Q14) or prelude | ✚ front carries colour | ✚ iris opens on release | ✚ causal trigger replaces labels |
| R02 attack | — | ⚠ must not re-extend during ARMED | ⟂ | ⟂ | ⟂ | ⟂ | ⚠ twitch if the reference is fast | ✚ |
| R03 hold | | — | ✚ riser drives velocity, not extent | ✚ charge sets contraction depth | ✚ the held build is what is replayed | ⟂ | ✚ | ✚ arming evidence |
| R25 velocity | | | — | ✚ same charge | ⟂ | ⟂ | ✚ section reference | ✚ onset density as charge |
| R27 rewind | | | | ✚ | — | ⚠ replays build colours; hand over to R07 | ⟂ | ✚ |
| R08 spread | ⟂ | ⟂ | ⟂ | ⟂ | ⟂ | ⚠ must open on release, never collapse at the drop (G5-SKETCH b) | ⟂ | ⟂ |

**Bundle experiment (PROPOSAL — NOT APPROVED).** One host build with R01 + R02 + R03 + R25 + R26 + R07 + R09 + R00 selectable, driven first by labelled drop times (no detector), scored by EXT-v3 (§9 Exp 1). Then ablation: remove one lever at a time; a lever whose removal turns the bundle red is necessary. R27 is scored as an alternative release gesture against R01, not stacked. Only after the geometry passes on labels does R05 (then R13) replace the labels.

### 5.3 Lever records (the 20)

Fields per the brief. "Current" is v9 (host-006) unless v8 differs. "Reuse" names existing machinery with its status. All experiments are PROPOSAL — NOT APPROVED. Device experiments use the iPhone camera loop (`scripts/titan_camera_record.py` + `scripts/titan_camera_strip.py`) with M0-11 audio-in-file for any timing clause, plus `scripts/palette_wire_snapshot.py` for luma.

**R01 — Inward release front on labelled drops** (5/4/1.5, P 0.75 → 10.0)
- Container: map + VP. Stage: `updateReleaseV9` (W:755-795), mode-32 pull (W:977 `if (release_ > 0) position = 80 + (position − 80)(1 − release_)` ✓), mode-18 pull (W:999 ✓), transport (W:1003-1015). Controls: space and motion at the drop.
- Current: release off and `inline constexpr` (WH:68-79). Forced on (`tests/host/p6_effect_probe.cpp:138-146`) it pulls the deposit to pixel 80 in one frame and the transport streams it outward (v2 D4). v8 has no release.
- Information: today loudness and "this is the drop" are the same number (L17, L26); the in-house transport cannot draw an inward front (L28).
- Origin: outward map = K1 centre-origin look (aesthetic); release left off because P6 triggers failed. Still needed: yes if the Captain's 14 Oct definition is the spec (EXT2 `captain_definition_verbatim`).
- **Reuse:** (a) [FACT ✓] `composed_motion::renderRibbon` inward maths — path = 1 − x, two bands, three-tap analytic trail, smoothstep feather — linked in H6 and host-tested to light the outer pixel at t = 0 (`harness/tests/test_composed_motion.cpp:37`); (b) [FACT ✓] Lightwave Beat Pulse Resonant 0x1404, Captain Tier 1 TENSION (`0e2e0778`), two rings contracting edge → centre with body colour sampled at the ring position; (c) Saliency Bloom's activator − 0.5 × offset inhibitor (front crispness without luma, SB9 washout constraint); (d) Bass Quake's position-gated re-arm (no new front until the previous one passes 0.55 of the half); (e) the K1 governor's cooldown (G2-F06). **Lineage history (CANON:14):** Lightwave Perlin Shocklines (effect 78) is an outward, integer-advance analytic wavefront fired by a fixed 0.3 flux/beat threshold per hop sequence — the same false-fire class as X1–X3; the Captain's ribbon/paired material was rejected as "mechanical or repetitive" (T-NEG). R01 is the direction-reversed, fractional, label-then-state-gated variant.
- Manipulation: an analytic overlay evaluated in `renderV9` before `outputV9`: per pixel d = distance/80 over 80 radial pixels; intensity max-composited into `intensity_[]`; palette position written into `offset_[]` so colour = `sampleProductPaletteHd(wrap(offset + d))` (path P; never a snapshot, never RGB addition); extent `e(t) = e₀ + (0.98 − e₀)[1 − (1 − u)³]`, `u = (t − t_r)/T_r`, T_r 100–180 ms with its own validator (not the donor's 200 ms floor); hold 1–2 s; return to FLOW. Size and echo count from R26. Outward variant kept for Q2.
- Cost: [INFERENCE] ≈ 80 px × ≈ 20 operations < 5k cycles per frame per board.
- Interactions: R02, R03, R25, R26, R27, R07, R09, R14, R00; R05/R13 later find t_d without labels.
- Risk: gimmick if it fires every chorus (only after R13 + governor); LGP washout if the hold is long; flash rate (R00); repeated identical fronts (T-NEG) — vary width and speed by charge.
- Smallest decisive experiment: §9 Exp 1 (EXT-v3 frozen with a direction clause; teleport, centre-born outward, late-inward, 16-slot-snapshot and linear-200-ms fronts planted red; ported composed maths planted green; renderer arm under `K1_INHOUSE_V9_HOST_VARIANTS` with `test_p6_product_path_p.py` at 0 mismatches).

**R05 — Fast event path** (4/3/1, P 0.7 → 8.4)
- Container: AP. Stage: new branch beside A12 reading pre-Mood magnitudes (A:274) or raw PCM; publication A20 (`acquire()`, A:377-429). Controls: timing of every hit-driven gesture, mode-18 energy rate (W:991), tempo input, release trigger.
- Current: novelty is one scalar after EMA, Mood, AGC and knee (A:357-367); the first hop after a step carries ≈ 2.7 % of the jump (D01); tempo-path novelty first crosses the build p95 37.5 ms after the BOOM drop but its ±30 ms maximum (0.057) stays below the build p95 (0.069) (computed this pass, §8.1 D23). Latest-only publication for non-peak fields (L20). v2's "≈ 330 ms late" is withdrawn.
- Origin: inherited (K1 `calculate_novelty`); the onset front-end's use of the post-AGC spectrum is S3-budget (L39).
- **Reuse:** [FACT ✓] `k1::core::audio::OnsetBeat` is in the tree and compiled, but `update()` is linker-discarded in H6 (map:4473-4474); it is a host-parity-tested port of K1 ONSET_V2 whose constants were derived for 133.33 Hz, which equals today's Titan hop (24,000/180). Log-spectral flux ignores a constant gain, so pre-AGC input needs no rescaling; only the gate floors (0.004 / 0.08 on post-AGC energy) are re-derived (G2-F02). At 24 kHz all 80 bins are below Nyquist, so the hat band [70, 80) works (on the S3 it was cut to one bin). Monotonic per-class ids read by cursor remove the latest-only loss for events. **Product route:** PRISM-Core `music/` clean-room onset (threshold 0.08, 60 ms refractory, microsecond onset identity; README:29-35, :169-171), scored against the OnsetBeat port as the tolerance oracle; `onset_beat.cpp` is GPL-3.0 legacy/uncertain, BENCH_ONLY, REFERENCE_ONLY in the Kx-Titan disposition (G2-F03). Cross-check: `K1MusicalConditioning` rising-body cursor (E14).
- Manipulation, arms: **A** legacy post-AGC/knee spectrum (baseline and planted negative); **B** OnsetBeat per board fed pre-Mood magnitudes, publishing `{event ids, strengths, levels}` into `InhouseWaveformAudio` (WH:84-117), inert by default; **C** a 1024-point Hann real FFT every hop with log-flux over bins 2–10 (47–234 Hz), the only lineage route to kick timing below 110 Hz (depends on R04; window delays sub onsets by part of 42.7 ms; cannot fire the drop alone because sub arrives 72–117 ms late); **D** v2's 6–8 biquad bands. Do NOT re-enable `AudioPipeline::process`.
- Cost: [INFERENCE] ≈ 80 `logf` per hop with cached logs + a 14-element sort ≈ 5k cycles per hop per board (≈ 0.07 % of the M85); arm C ≈ 10–20k cycles per hop.
- Interactions: enables R11, R12, R13, R26 and R07's attack-only arm; feeds R23 (onset events into the TT flywheel onset term `kFlywheelOnsetK`, K1's own unexecuted next step, G2-F12); R04 raises its sub band; CC-1's hop change re-derives its frame-count constants (14-frame median, refractories) — freeze R05 at the hop CC-1 chooses.
- Risk: false onsets on noise; must not make mode 18 less snappy; licence for the product.
- Smallest decisive experiment: §9 Exp 2 arm A. Frozen (detection-latency budget, CANON:36): click corpus onset → event median ≤ 15 ms, p95 ≤ 25 ms; BOOM: a kick- or transient-class event within [0, +30 ms] of 55.7025 s whose flux strength is ≥ 2 × the build-window (45–50 s) p95; ≥ 99 % of 1,000 random single-hop impulses produce exactly one id increment, in order, across render-phase sweeps and a 30 ms render pause; identical PCM at two Mood settings gives identical event times. Planted: arm A must fail the BOOM clause (if it passes, the plant is invalid: report and redesign before scoring B); latest-only publication must lose ids; input delayed 30 ms must fail the median. Snappiness: mode-18 onset → velocity rise ≤ today.

**R03 — Hold contracted through the riser, progressively** (5/4/2, P 0.65 → 6.5)
- Container: map + VP. Stage: A11 + M5 + an ARMED state (part of R13, first driven by labels). Controls: the dark-to-light and contracted-to-extended contrast the drop spends.
- Current: after the cut the riser relights the plate before the drop (F-CAM1; interval [HYPOTHESIS]); the follower has decayed during the cut (release ≈ 2 s, A:250), so returning sound pins `peak_scaled` at the clamp and parks deposits at the edge (v2 D18).
- **Reuse:** entry rule from K1 DROP_CUT (120 ms of low energy after a ≈ 2 s peak-hold above "recently loud"; G2-F10) or River Surge (fast/slow > 1.3 within 2 s, then fast < 0.45 × peak; E17), keyed on **pre-AGC RMS** so the threshold does not drift; `composed_scene::Direction` hold/release/rest overrides (E3) for labelled and live tests; Lightwave Beat Pulse Void as the ARMED renderer (small rings confined to the outer band that contract as they fade, true black elsewhere).
- Manipulation: after a cut, keep new deposits contracted to the outer band; **contraction deepens progressively with stored build charge** (the Captain's "slow and surely contracting", SWARM-BRIEF:26; D12 C01 Spring) instead of a fixed 25 % step; the riser drives velocity (R25), not extent; hold until the labelled drop (later R13); timeout ≤ 8 s returns to FLOW; silence stays exactly 0. The exit is never "relight on any energy" (K1 DROP_CUT's exit).
- Interactions: R25, R26, R01, R02 (must not re-extend while ARMED), R09, R13.
- Risk: songs with a riser and no drop look withheld (timeout); must never black out the riser; must not buy contrast by darkening the build.
- Smallest decisive experiment: host packed frames on labels (bundle build, §5.2), then wire snapshot + camera loop. Frozen: mean packed luma in [cut end, t_d − 0.1 s] ≤ 0.3 × mean in [t_d, t_d + 1 s] on every labelled cut-drop; riser motion ≥ 1 lit pixel moving per 100 ms; lit extent over [build start, cut] has Spearman ≤ −0.6 against time; build median luma ≥ 0.8 × host-006 build median luma (G5-SKETCH d); outside ARMED/BUILD packed frames within tolerance of host-006. Planted: host-006 red; K1 instant-relight exit red on the luma clause; a fixed outer-25 % step-hold red on the monotone clause; a riser blackout red on the motion clause; a 3× dimmed build red on the build-luma clause. Camera (geometry): riser contracted until t_d; host-006-a red.

**R02 — Extent instant attack** (4/2/1, P 0.8 → 6.4)
- Stage: W:974-975 (`ema(…, 0.016F)`, `ema(…, 0.023F)` ✓). Controls: how fast the waveform's length jumps on a hit.
- Current: t50 ≈ 30–33 ms, t90 ≈ 76–80 ms at 100 Hz (D13; D08 VERIFY). Mode 18 has no EMA.
- Origin: [INFERENCE] anti-flicker, not budget. **Reuse/lineage:** Lightwave Progressive Ignition 0x1E04: lit radius from an asymmetric follower (rise τ 0.10 s, fall τ 0.40 s, smoothstep edge), named the approved replacement template after twelve layered effects failed from stacked smoothing (memory #47098; failure commits `aed805bb`, `7b640bec`, `45601ce1`).
- Manipulation: attack ≤ 1 frame through the existing `follow()`, release unchanged; cap 0.7 → 1.0 only together with R01 (with outward polarity a higher cap shortens the lit extent, v2 D14).
- Interactions: R01, R03, R09. Risk: twitch (K11/K12).
- Experiment: frozen: extent ≥ 0.9 of final within 15 ms of a synthetic step; sustained-noise per-frame extent RMS change ≤ 1.5 × today; snappiness proxy not worse. Planted: the symmetric 16/23 ms cascade fails the 15 ms clause. Then Captain Q9.

**R07 — Colour chapter carried by the release** (4/4/1.5, P 0.6 → 6.4)
- Stage: M10 (W:630-657), deposit offset (W:956-958), `outputV9` (W:888-889). Controls: whether the drop's harmony arrives with the light, and whether it lands bright.
- Current: path-P colour t50 ≈ 75–113 ms [INFERENCE, D13]; the first ≈ 0.2 s of a drop wears the build's colour; P3's drop walks gold → red into a dark end (T-COL4).
- **Reuse:** [FACT] `PaletteTransition` (E10) is compiled in and owned by `PaletteRuntime` but not used by the in-house renderer; its phase is anchored at each palette's brightest stop, which guarantees a bright landing. Lightwave Attack-Only Pitch Velocity Field 0x2101 (colour target captured only on onsets) and Star Burst Narrative (commit only after a phrase hold ≥ 1.5 s at chord confidence ≥ 0.55; circular key EMA) give two principled alternatives to the per-hop Mood LPF (L14).
- Manipulation, arms: **A** at release, snap the deposit offset to the pre-Mood centroid and let the front carry it; **B** at release, cross-fade to a chapter phase via `transition.hd(position)` in `outputV9` when a `const PaletteTransition*` is supplied (default null: byte-identical); **C** attack-only target updates gated on R05 events, gliding τ 0.25 s between onsets; **D** phrase-gated commit.
- Interactions: R01 (front carries colour), R27 (hand over after the replay), R08 (spread must open, not collapse), R05.
- Experiment: frozen: centre-pixel offset within 0.004 of target within one frame of fire; drop-chapter median luma ≥ build-chapter median luma; arm C: sustained-chord offset drift ≤ 0.02 turns/s and drop offset within 0.02 turns of the pre-Mood centroid within one frame of the first post-drop onset; default build `mismatches = 0`. Planted: no snap; chapter forced to the palette's darkest stop; every-hop update fails the drift clause.

**R25 — Riser velocity charge** (4/3/1, P 0.5 → 6.0) — new in v3
- Container: map. Stage: mode-32 transport rate `405·motion` px/s with ≤ 8 steps per frame (W:967-968 ✓); mode-18 rate (W:988-993). Controls: how fast the light flows during the build and riser, and how that motion is spent at the drop.
- Current: scroll rate is constant (mode 32) or tempo/novelty-weighted (mode 18); nothing couples tension to velocity. [FACT ✓] The live per-board `speed` sets `motion_scale = speed·2` every in-house frame (PR:250); the v9 preset stores speed 0.5 (PRESET:57, :108), i.e. 405 px/s; [INFERENCE] the 8-step cap allows ≈ 800 px/s at 100 fps, so the live control has ≈ 1.98× headroom before the cap binds.
- **Lineage:** Lightwave Perlin Shocklines/Veil (footage-identified, CANON:20) carry a peak-held momentum, push = level⁴ (0.8 vs 0.5 input → ≈ 6.6× the push), release ×0.99 per 60 Hz frame (τ ≈ 1.66 s), driving **advection speed, never brightness**; the Captain capped Shocklines' speed at 1 in his Tier 1 list (slow baseline). D12 C17 RiserAccelerando. G5 found no riser → velocity coupling in Lightwave, PRISM-Core or the K1 S3 tree. Do not import Shocklines' brightness floors or integer advance (§0.5).
- Manipulation: (puppet, no flash) script board 0's live speed over labelled build and riser windows; (product) `v ×= 1 + k·m`, m = peak-held (fast level ÷ section reference)⁴ or onset density, τ_release ≈ 1.5 s, capped inside the existing step limits; k = 0 reproduces host-006.
- Interactions: R03 (velocity carries the riser while extent stays contracted), R26 (same charge), R14 (fractional transport smooths fast motion), R09 (section reference), R11.
- Risk: live-control latency may be too coarse for 0.25–0.5 s ramp steps (measure first); a "busy" look; must never touch brightness.
- Smallest decisive experiment: §9 Exp 3 (no flash, one Captain look). Product version (host): frozen: trail-front velocity in the last 0.5 s before t_cut ≥ 1.7 × at riser start; per-frame packed luma within ±5 % of the k = 0 run; exact zero in silence; outside BUILD/ARMED packed frames equal to k = 0. Planted: a power-one (linear) push fails the ratio; a brightness implementation fails the luma clause.

**R04 — Re-derive the PDM bass-cut** (3*/4/1, P 0.5 → 6.0)
- Stage: PDM HPF registers per channel (staged `hal_data.c:613-619`, generated from the pinned BSP via `build_scalar.py:816-820`; PDM has no override). Controls: kick and sub energy for everything downstream (`sub_rms`, follower, flux, R05 arm C, R10, R26).
- Current: s0 0x3F61, k1 0x3EC1, shift 0. [INFERENCE, IC] 259 Hz first-order corner at the 82.8 kHz filter clock (L1); 128 Hz model refuted by MAN §50.3.4.3/§50.3.4.5 (v2 D2).
- Origin: accident. Still needed: a DC block yes, a 259 Hz corner no. Cost: 0 CPU; a flash.
- Manipulation: k1 0x3FE7 / s0 0x3FF4 (20.1 Hz) on the witness lane first, then the programme lane; model the compensation and half-band filters before claiming flat response.
- Risk: capture path is the most fragile (PL14-A4); reversible by one constant. Snappiness oracle on the device step.
- Experiment: §9 Exp 2 arm B (host replay measuring light and onset strength), then device witness-lane sweep: frozen B/A ≥ +6 dB at 45–65 Hz and ±1.5 dB at 800–1,000 Hz; planted lane B at fc ≈ 1 kHz must read ≤ −6 dB at 45–65 Hz. No Captain look.

**R06 — Colour tuning truth** (3/3/1, P 0.6 → 5.4)
- Stage: G:121-128 (rounded k), WH:126 (`chroma_range = 60`). Controls: which pitch class (palette position) each note votes for; whether hats and cymbals move colour.
- Current: bin 0 resonates at 104.69 Hz; low half ≈ 80 cents flat, high half mostly 20–40 cents sharp; bins above ≈ 3.5 kHz never vote. Also gates the quality of any harmonic signal (MusicalSaliency harmonic axis, ChordDetect, R16, R08 arm).
- Origin: inherited; K1's eyes-on rejection of exact centres may have been overflow-contaminated (`7cd9336`); the live Titan path is int64, so re-test. Cost: 0 cycles (setup).
- Manipulation: exact-frequency coefficients; range 80; legacy configuration kept as a named parity oracle (CANON:41).
- Experiment: frozen: ≥ 95 % of frames assign each pure tone 110 Hz–3.3 kHz to its own pitch class (baseline ≈ 50 %); 110 Hz peaks in bin 0 within 0.5 Hz; an 8 kHz tone moves the offset ≥ 0.15 turns at range 80; noise-only drift ≤ 0.02. Planted: integer rounding; range 60.

**R08 — Path-P dark ends and spread, anchored at K1 EdgeMixer** (4/4/2, P 0.6 → 4.8)
- Stage: V4/V5 (W:888-889). Controls: how much of the palette one half spans; whether dark stops eat deposited light.
- Current: k = 1 (a full palette turn per half, audio-independent); catalogue index 31 (selection 32) has 17.5 % of positions below 5 % luma and a black → yellow seam; index 38 (selection 39) is dark at both ends — [INFERENCE, G4-11] these figures must be re-verified on the catalogue entry the device actually samples (one-based/zero-based trap).
- **Lineage anchor:** [FACT, G2-F09] K1's shipping EdgeMixer, accepted at the Captain's gate-3 plate A/B (`0d837293`), moves each strip's palette position by ±0.208 turns mixed at 65 % with a centre mask — path P's k = 1 is about 5× that excursion. Lightwave Star Burst puts the root at the centre, the fifth toward the edge and the third in proportion to tension (palette-position translation, never a hue wheel).
- Manipulation: `p = o + k·d`; rotate so a palette's dark end lands at the outer edges (or use an authored sub-range for non-cyclic palettes); first A/B at k ≈ 0.2–0.4, then k driven by harmonic tension / flatness within [0.15, 1]; **spread opens on RELEASE** (or yields to R07); never one colour, never a floor.
- Interactions: R07 (⚠: the D12 sketch collapses spread to k = 0.25 at the drop, making the drop the least colourful moment — G5-SKETCH b ✓ by `concept_sketch.py:166-167`), R16, R06.
- Experiment: frozen: masked fraction (lit pixels ≥ 0.01 whose max channel < 5 % of intensity × palette max) ≤ 1 %; ≥ 2 separated non-black palette regions during active music; distinct positions in the drop window ≥ the build window; distinct positions follow an annotated build ramp (Spearman ≥ 0.5) and never fall below 4. Planted: index 31 unchanged under P (≈ 17 %, red); k = 0 fails the spread floor; a spread clause passable by the d ramp alone with music frozen must be red; a k-collapse at the drop fails the drop ≥ build clause. Then Captain Q6.

**R09 — Section-relative dynamics + iris, one owner per field** (5/3/2, P 0.6 → 4.5)
- Stage: A10, A15, M2, M3, M5. Controls: whether the drop can be longer and brighter than the build.
- Current: follower 0.25/0.005 (τ 30 ms / 2 s) + clamp → BOOM `peak_scaled` +7 % for +8.4 dB; broadband AGC τ 190 ms; ceiling −9 dBFS; gains 0.580/0.831.
- **Reuse:** the in-tree 4-band AGC (E7) for the colour/display view (K07 fix; K1 per-band ON); Lightwave TranslationEngine section-change score (0.45·|ΔRMS| + 0.30·|Δflux| + 0.25·|Δharmonic| between consecutive windows; fixed 4 s windows until R23) as the section reference's boundary evidence. [FACT, G2-F08] Per-band AGC is a short leveller and cannot by itself make the drop bigger than the build.
- **One owner per field per consumer** (LWA:399; G4-15): extent from fast level ÷ an 8–30 s section reference (replaces the A10 follower ratio **for that consumer**); brightness from the iris (replaces the broadband AGC for that consumer): deposit gain held ≤ 0.6 of its range during BUILD, fully open on release, reopening slowly if no release comes; colour from per-band-conditioned shape with raw shape in parallel. Never stack a new follower on the old ones; never buy the ratio by dimming the build.
- Experiment: frozen (wire snapshot or host packed frames): drop/build p95 luma ≥ 1.5 on every tuning drop; build median luma ≥ 0.8 × host-006; song mean luma within ±10 % of host-006; median head distance from centre in drop + 0.5 s ≤ 0.5 × the last 2 s of build; quiet → loud → quiet step: no luma overshoot or undershoot > 10 % beyond the settled value (pumping clause); post-roll room noise yields 0 admitted frames. Planted: today's follower red; two stacked followers red on the pumping clause; an iris also applied to the drop red; a 1/1024 floor fails silence; gain τ 190 ms red. All luma clauses are ratios to the running envelope (M0-12), so they survive any brightness ruling.

**R15 — Darkness truth** (4/3/1.5, P 0.55 → 4.4)
- **Reuse:** [FACT] statusJson already exports per board fresh, `source_silence`, `raw_rms`, `quiet_entry_rms`, admission open, position and deposit fields (PR:836-896), and a BLM1 64-record trace exists for donor scenes (E11). SynqMatrix's 22 suppressed-reason and 10 classification-reason codes are a ready taxonomy.
- Manipulation: pack the existing booleans plus the user and tonality gates into a `uint8` reason word per in-house frame, in a 64-entry ring modelled on `BloomTraceRecord`, exposed through the existing status/diagnostic path; replay Dopex and attribute every dark frame; test whether music held between −33 and −28 dBFS for 5 s drains to black.
- Experiment: frozen: exact packed zero within 1 s of digital-zero input; every Dopex dark frame attributed to exactly one gate; the −30 dBFS 6 s passage reported (pass/fail is the Captain's ruling); camera dark lines attributed by wire snapshot. Planted: a 1/1024 floor fails black; an unlabelled gate reports "unattributed".

**R26 — Stored build charge sets release size** (4/4/2, P 0.5 → 4.0) — new in v3
- Container: map. Controls: how big and fast the release is, as motion — front speed, echo count, hold — never brightness.
- **Lineage:** Lightwave Transient Lattice (charge `c ← c·e^(−dt/0.68) + 0.20·transient`; K1 S3 port reconstructed from a spec, memory #66401), Shockwave Cascade (release size as the **number** of fronts: primary + 2 thinner, dimmer echoes), Ripple ES tuned (Captain Tier 1 IMPACT: impact spent as front speed and count driven by sub and kick); D12 C01 Spring (charge → contraction depth).
- Manipulation: charge per board from onset strength (R05; legacy novelty until then) and section-relative level, frozen at fire; T_r = lerp(180, 100 ms, c); echo count = 1 + round(2c) at +40/+80 ms with widths ×0.7/×0.5; contraction depth in R03 from the same c.
- Interactions: R01, R03, R25, R05, R00 (flash-rate budget counts echoes). Risk: identical repeated echoes read as "mechanical" (T-NEG) — vary width and speed with c.
- Experiment (host, on labels + the corpus): frozen: release front speed correlates with build onset density across corpus drops (Spearman ≥ 0.6); drop releases show ≥ 2 distinct luma maxima moving the same direction within 200 ms while ordinary kicks show 1; R00 flash clause holds. Planted: charge forced constant fails the correlation; echo count independent of charge fails the drop/kick contrast clause.

**R10 — Timbre width + sub push** (4/4/2, P 0.5 → 4.0)
- Stage: M4 (W:826) and a warp after the field, before colour. Width from spectral flatness (D12 C18); `x' = x − A·s(t)·sin(πx/80)` with s = high-passed `sub_rms` envelope (D12 C07); never a brightness gain. Depends on R04.
- **Lineage caution:** Lightwave Bass Quake's history records bass-pumped geometry (phase wobble, falloff, cell density) tuned **out** (G3-P06); keep displacement small and slow-released. `EmotiscopeEdgeResponse` (E14) is admissible only as a spatial mask, never as a beat brightness gain.
- Experiment: frozen: width vs flatness r ≥ 0.5 and width vs level |r| ≤ 0.3; a 55/66/90 Hz burst gives 4–8 px displacement with integrated intensity within ±5 %; equal-VU 220 Hz/1 kHz control ≤ 1 px. Planted: width = level; `K1_SUBBAND_PLANT_WRONG_CENTRE` (SB:23-24) silences the 55 Hz arm; a brightness-gain implementation fails the intensity clause.

**R11 — Groove flywheel for board 1** (4/3/2, P 0.65 → 3.9)
- Stage: M8 mode 18 (W:980-998). Current: up to 0.7 of its speed follows a tracker that locks to half time (P7); energy rate 30 + 90·clip(novelty/0.12).
- **Evidence and lineage:** [FACT, G2-F07] K1 device music soaks show lock occupancy 0.35–0.42 on the same tracker lineage, so a no-lock design is favoured; Lightwave Kuramoto Transport separates an invisible velocity field (kicked by events) from visible deposits (G3-P14); K1 Tempo Comet Anticipate's 0.7–1.4 ratio guard (E16) is an optional bound.
- Manipulation: each onset (R05; legacy novelty until then) adds an impulse to a scroll-velocity state; drag τ 0.4–0.8 s; deposits remain V2 max-hold.
- Experiment: frozen: scroll-velocity spectrum peak within 3 % of the annotated beat rate (or 2×) on BOOM and Sonic; onset → velocity rise ≤ today (snappiness oracle for mode 18). Planted: a 69 BPM synthetic impulse clock peaks at half time and fails.

**R13 — Tension state machine for causal release** (5/4/3, P 0.55 → 3.7)
- Design: FLOW → BUILD → ARMED → PROVISIONAL (inward accent ≤ 0.12, no global brightness) → RELEASE (size = R26 charge) → RECOVER; QUIET drains to black. Predictions may arm; only observed evidence fires (CANON:43). A false full release costs more than a miss.
- **Reuse:** re-home `composed_scene::Direction` (E3) — state enum, dwell, policy field semantics (hold/release/rest), status word — as C6 per board; its automatic Release (falling energy or 6 s) becomes the **ARMED** trigger, never RELEASE. Inputs: pre-AGC level at several scales, a cut detector (River Surge / DROP_CUT rule), R05 events, TranslationEngine change score, saliency axes as candidate tension features. The K1 `k1_mode_selection` governor bounds releases (dwell, cooldown, maximum per window, manual owner). SynqMatrix's state/reason structure is copied, **its numbers are not** (they assume ESV11 scaling and a ≥ 250 ms candidate dwell). `MusicalTime` predicted beats (`predicted = true`) may arm only.
- Evidence: sub arrives 72–117 ms after the musical drop, so a sub-only trigger cannot meet +50 ms; surge triggers fired on climaxes and risers (X1–X3); SynqMatrix's own Drop rule is ≥ 250 ms late by construction.
- Experiment: frozen (v2/v3 hit rule unchanged): ≥ 5/6 drops in [0, +50 ms]; zero false full releases, including the post-cut return and within 2 s of annotated non-drop climaxes and of HarmonixSet segment boundaries used as negatives (G2-F14); zero full releases within the governor cooldown; falls back to FLOW on missing evidence; every frame carries exactly one reason code. Planted: charge forced to 1; a 200 ms-late fire; sub-only wait; broadband-surge-only trigger; the SynqMatrix Drop rule with its 250 ms dwell; Direction's auto-release (fires at the cut); cooldown 0 on a chorus-repeat fixture. Holdout: the Captain's corpus.

**R12 — Push-pull** (4/4/2, P 0.45 → 3.6): kicks pull the light inward for a moment, snares bloom outward (D12 C19); class → position/speed/life tables from Percussion Burst and Pulse Prism (E16; rules only, never the Pixel8 renderers); depends on R05. [FACT ✓ by figure, G5-SKETCH c] D12's own sketch adds ≈ +0.08 mean intensity on every snare, so it is not evidence of a luma-neutral push-pull; it becomes a planted negative. Frozen: inward step on kicks and outward on snares, each ≥ 70 % hit rate within ±30 ms; integrated intensity within ±5 %. Planted: swapped band proxies; the D12 snare-bloom luma shape.

**R27 — Rewind the build** (5/5/3, P 0.4 → 3.3) — new in v3
- What the viewer sees: at the drop, the light the board drew during the build plays backwards at high speed, so it converges from the edges on the centre, then the release front takes over.
- **Lineage:** [FACT] Lightwave Time-Reversal Mirror Mod1 0x1B06 is one of the Captain's two Tier 1 IMPACT effects (`0e2e0778`); it records a damped wave into a history buffer and replays it reversed; captured on K1v2 in March (agent capture, memory #40386/#40388, DEGRADED). Its reversal is **timer-driven** and it normalises per frame; neither is ported.
- Manipulation: a ring of the post-deposit field per board (`intensity_`, `offset_`) during BUILD/ARMED; on an **observed** drop (labels first), replay frames reversed at 4–8× for ≈ 150 ms into `intensity_`/`offset_`, then hand over to R01 and R07 (the replay carries build colours; the chapter takes over after it). [INFERENCE] 2 s × 100 fps × 160 px × (int16 + int16) ≈ 128 kB per board, 256 kB for both — SRAM budget check (M0-4).
- Risk: taste; memory; replaying a busy build may read as noise.
- Experiment (host): frozen: the replay's luma-weighted centroid moves strictly toward the centre (EXT-v3 direction); replayed packed frames equal the recorded frames reversed within 1 LSB; silence exact 0. Planted: forward replay fails direction; a per-frame normaliser fails exact black. Then Captain Q14 (rewind vs analytic front).

**R14 — Fractional transport + swept deposit** (4/3/2, P 0.55 → 3.3): fill the stride between frames in palette-position space (never RGB) and keep fractional position until rasterisation (W:1000-1018 accumulates a remainder but moves whole pixels). Implementation references: `RibbonMaterial` continuous-time advance and Gaussian rasteriser (E15; not linked, rejected as a look) and the donor's anti-aliased swept palette dot (E12). First, one fixed-exposure camera frame of a static bead decides whether the LGP already hides the 4 px beading. Frozen: continuity R (5-px min/max over the lit trail) ≥ 0.8 at l = 0…1; centre-of-energy error ≤ 0.25 px across 0.25–4.05 px/frame; integrated-intensity drift ≤ 2 %; PAL-HD-v2 unchanged. Planted: forced integer rounding (≤ 0.6); RGB interpolation.

**R22 — Two-board roles** (4/3/1.5, P 0.4 → 3.2)
- **Reuse:** `EmotiscopeEdgeColour` (E9) relation added to board 1's path-P position in `outputV9` (`p = o + k·d + offset_turns`), read from the existing live fields 28–32; K1 EdgeMixer's accepted anchor (±0.208 turns × 0.65 × centre mask) as the first value; SynqMatrix's per-state relation (Drop complementary, Build/Breakdown veil) as the state mapping once R13 exists; `composed_scene::Pair` (E2) as a no-flash grammar probe of call/answer (G5-F02). `PairedWaveScene` is tests-only and is not on-device evidence (E15).
- Independence (TL11 TL-17/18; G4-09): any shared drop state is published read-only on the audio side after the single phase advance, never as shared mutable scene state; the alias plant must first be shown red, because native fault 005 and the product D05 alias plant were both falsely green (TL14-059).
- Experiment: first a one-board camera capture of vertical light overlap in the shared diffuser; then frozen: changing board 0's artistic control leaves board 1's bytes unchanged under fixed shared features; the board-1 offset equals the relation within 0.004 turns; a shared release timestamp reaches both submissions within one frame. Planted: shared renderer pointer; swapped top halves; an HSV rotation producing off-palette colours (ΔE to the nearest palette stop > 3). Then Captain Q7 and the C09 grammar probe (§7).

**R17 — Input headroom** (4/3/2, P 0.5 → 3.0): log-only arm first — per-slot guard engagement count and pre-guard peak (`k1_pdm_target_last_hop_gain_q8()`, PDM:626); then a pre-guard level side channel; then 20-bit capture (`PDM_PCM_WIDTH_20_BITS_0_18`) with Goertzel headroom re-checked (`>> 6` and ×16 change it; the int64 path has ≈ 155× headroom today). Frozen: report the fraction of drop vs build slots with gain < 4096; a +6 dB step crossing raw 1,875 is recovered within ±0.5 dB in the side channel; snappiness oracle. Planted: a synthetic slot at peak 4,000 must report reduced gain; post-guard RMS labelled raw must fail.

### 5.4 Canon-conformance records and below-the-line records

**CC-1 — Hop-locked render at 120 Hz logical, one present time for both boards** (v2 R18, re-targeted)
- Rulings: render triggered by the audio hop (CANON:35 ✓); 120 Hz logical rendering, physical output cadence measured separately (Kx-Titan AGENTS.md:9; TL14-037: rational 8333/8333/8334 µs deadlines; CANON:55 budgets capture + analysis + two renders + transport in 8.33 ms). Product today: `kPalettePeriodUs = 10000U` under `K1_EMOTISCOPE_PORT` (PRH:40; 8,333 at :47 is the other branch).
- [INFERENCE, G4-04] 200 samples at 24 kHz = 8.333 ms exactly, so a 200-sample analysis hop makes hop-locked rendering exactly 120 Hz and satisfies both rulings (≈ +0.4 ms mean wait vs 180). Alternative: a 100-sample hop rendering every second hop (lower detection latency, ≈ 2 ms render wait). v2's 133 Hz stays available only if the Captain wants the latency gain (§7 rulings).
- Consequences: every `dt`-based constant changes character — adopt the elapsed-time law already ruled on the donor path (TL11 TL-28) as a separately named temporal profile with host-006 as oracle (LWA:127); R05's frame-count constants and the tempo decimation re-derive at the chosen hop.
- Transport safety (NFB:117-122, :131-145; G4-14): keep the donor's compare-match ELC stop and double-buffered GTBER unchanged; diff against the proven transmitter before the first flash; prove off-product with an agent-controlled detector and a written abort condition; one unproven change per flash. Wire snapshots are "last submission, not photons" (PRH:29) and cannot certify the absence of transport white.
- Frozen (device log): publish → submit p50 ≤ 0.5 ms, p99 ≤ 5.5 ms, zero skipped frames; board-to-board first-edge skew p99 ≤ 20 µs; mean render period within ±0.1 % of 8.333 ms over 10 min with ASRC ratio and `measured_hz` logged (CANON:54 MOCO ±10 %); snappiness oracle vs host-006 on modes 32 and 18. Planted: a fixed 10 ms tick fails p50; a resampler ratio frozen at boot under a simulated −2 % MOCO drift fails the period clause.
- [HYPOTHESIS, G4] Event-preemptive frame: at 120 Hz with a 5.1 ms wire, ≈ 3.2 ms per period has the wire idle; an onset on a 64-word block during that time could trigger an immediate render-and-send and re-phase the cadence, latest-pending bounded to one (TL14-037/-040). Only after CC-3 measures the baseline.

**CC-2 — 64-word capture blocks + coefficient cache** (v2 R20, corrected): 64 words ≈ 1.55 ms at 41,417 Hz (CANON:35 ✓; v2's 96-word option = 2.32 ms is outside the ruling). The frozen native lane's repair caches the resampler coefficient table by exact source rate (the old path regenerated 65 × 128 coefficients after every discontinuity and could itself overrun the DMA reserve; host-proven 492 hops bit-identical; NFB:202-213); port it as a reviewed patch (CANON:48). Re-check the poll high-water and USB-starvation history (NFB:185-216). Frozen: poll high-water ≤ 2 ms over a 60 s BOOM run; zero ASRC discards; USB-CDC attach stable. Planted: an added 10 ms delay; the cache disabled (stall returns).

**CC-3 — ACOUSTIC_TO_PHOTON budget ledger and baseline** (new; M0-10). One row per term, measured with DWT stamps at slot-complete, hop-publish, render-start, submit and DMA-complete per onset (DWT already runs, `hal_entry.c:231-232`; COST fields exist), paired with native iPhone 240 fps slow-motion recordings **with audio in the file**, ≥ 500 onsets, on the product profile, both boards, modes 32 and 18. [INFERENCE, G4-03, arithmetic means only] today: capture slot wait ≈ 3.6 ms + hop wait ≈ 3.75 ms + in-house AP ≈ 0.70 ms + free-running tick wait ≈ 5 ms + VP/emit ≈ 0.4 ms + wire 5.1 ms (WS2816 latch at reset [HYPOTHESIS]) ≈ 18–20 ms p50, plus unmeasured PDM-filter and resampler group delay; with CC-1 + CC-2 ≈ 9–11 ms, still above 8 ms because the 48-bit wire dominates. CANON:35 reserves **24-bit + dither** as a Captain choice only once the capture shows the 48-bit wire is what breaks p50. This baseline is also the snappiness-oracle reference every backend lever needs. Frozen: the ledger's terms sum to within 1 ms of the photon measurement p50. Planted: a deliberately added 5 ms render delay must appear in both the ledger and the photon measurement.

**R16 — Circle-of-fifths order** (3/4/1, P 0.25 → 3.0; rank 21; taste): chroma index i → 7i mod 12 before the centroid (W:157, :611). Frozen: frames with centroid strength ≥ 0.08 rise ≥ 1.5× on tonal tracks; colour moves in ≥ 90 % of seconds. Planted: a random fixed permutation misses the rise. Then Captain Q10.

**R28 — Section memory and recall** (4/4/3, P 0.4 → 2.1; rank 22; new, D11-08 via G5-SECMEM): 8 compact descriptors (12-bin pre-knee chroma, mean/peak RMS, sub/broadband ratio, onset density) ≈ 512 B; section change = chroma cosine distance > 0.25 plus a level, sub or density change held 1 s; dwell ≥ 4 s; a recognised return restores palette offset and spread, never brightness or motion. Lightwave SynqMatrix classifies sections live but remembers nothing; PRISM-Core `SaliencyTracker` (music/saliency.h, HEAD `89c79df`) is a clean-room trigger candidate, admission pending (no silent repin). Frozen: stored contexts distinct (cosine > 0.25); return offset within 0.02 turns within 1 s of the return; transient t90 unchanged (≤ +1 ms); silence exact 0. Planted: scalar-centroid-only memory fails distinctness; memory disabled fails the return error.

**R19 — WS2816 transfer** (4/3/3, P 0.5 → 2.0; rank 23): locked-exposure ramp of 32 steps per channel on one board; frozen: fitted exponent 95 % CI width ≤ 0.2, code 0 indistinguishable from the dark frame; planted: synthetic linear ramp fits γ = 1.0 ± 0.05; +1/1024 on every code fails black. Only if γ > 1.2: a curve for in-house TRUE16 modes. Captain Q11.

**R21 — Treble coverage** (2/4/2, P 0.5 → 2.0; rank 24): short treble windows several times per hop, peak-held. Frozen: 1,000 random 2 ms 8 kHz clicks detected in bins 70–79 ≥ 95 %. Planted: newest-window-only. Snappiness oracle.

**R23 — Tempo metrical repair** (4/3/3, P 0.35 → 1.4; rank 25; P7 lane): populate the declared-but-never-produced derived ratio candidates in `TempoFieldV1` via `deriveTempoPhase` (E8; G1-F10), add phase extrapolation and zero confidence on silence; sub-arm: feed R05 onset events into the TT flywheel onset term (G2-F12; does not fix octave choice). GATE-SYNTHETIC-v1 326 cases (best 35, flat prior 29). Planted: 88 BPM prior; half-tempo output; a 69 BPM impulse clock. R11 removes board 1's dependency meanwhile. `TempoReferenceScene` is the host instrument (G1-F19).

**R24 — Arithmetic hygiene** (1/2/1, P 0.5 → 1.0; rank 26): float RMS accumulation; remove `>> 6` with re-frozen gains, only alongside another AP change. Frozen: ≤ 1 % RMS error vs double for amplitudes 64–16,000. Planted: per-sample truncation fails at amplitude 64.

---


## 6. Target architecture — PROPOSAL — NOT APPROVED

### 6.1 Principle (carried over from the colour fix, extended by this pass)

Keep vectors and timestamps across container boundaries and collapse to a light value only at the pixel, once. Analyse once at full resolution; publish several **views** of the same audio — fast events, tonal shape, context — and let each visual consumer smooth, normalise or fold for its own purpose, with **one owner per field per consumer** (LWA:399). Mood and colour speed are per-board consumer settings, never inputs to rhythm. **Reuse before build:** every container below names the existing machinery it adapts (§0). Extend the existing pipeline; no new orchestrator, actor framework, dashboard or NPU graph; no third tree (CANON:48).

```text
mic U14 (programme) ─┐                          mic U13 (witness; never required for light)
                     ▼
C0 capture: HPF ≈20 Hz, 64-word blocks, int32 (20-bit later), slot timestamp, applied gain, clip/loss counters
                     ▼
C1 resample 24 kHz (coefficient cache by exact source rate) ── hop 200 samples (120 Hz) or 100 (render every 2nd)
                     ├── C2 fast events: OnsetBeat-equivalent on pre-Mood magnitudes (host) / PRISM-Core onset (product);
                     │                    per-class monotonic ids; optional 1024-pt FFT sub-kick arm
                     ├── C3 tonal: 80 bins exact-k, raw + display-conditioned (4-band AGC view), chroma[12], timbre
                     └── C4 context: levels 30 ms/0.5 s/2 s/8–30 s, section change score, cut detector, charge,
                                      tension features, tempo candidates (TempoFieldV1, derived ratios)
                                     ▼
C5 publication: versioned MusicFrame (epoch, generation, per-field age; legacy fields unchanged, new fields inert)
                                     ▼
C6 choreographer per board (re-homed composed_scene::Direction + governor):
   FLOW/BUILD/ARMED/PROVISIONAL/RELEASE/RECOVER/QUIET → extent, front, velocity, iris, colour offset, spread k, width
                                     ▼
C7 field per board (intensity + palette position, fractional; analytic fronts from composed_motion maths; rewind ring)
                                     ▼
C8 HD palette per pixel (path P; optional PaletteTransition; EdgeColour relation offset on board 1)
                                     ▼
C9 display + R00 → C10 TRUE16 pack → existing DMA/GPT/ELC (board 0 P601/GPT6 + P604/GPT8, LED DMA0/3; board 1 top engine)
   present hop-locked at 120 Hz logical, one declared present time for both boards
```

### 6.2 Containers and data contracts

| Container | Runs on | Rate | Output contract | Reuses (§0) | State and placement |
|---|---|---|---|---|---|
| C0 Capture | M85 DMA + ISR (mic DMA1/2 unchanged) | PDM ≈ 41.4 kHz (MOCO); **64-word blocks** (CANON:35) | int16 now / int32 later per mic; `uint64` slot index + epoch; applied gain; clip and loss counters | `k1_pdm_target_last_hop_gain_q8()` | DMA rings (existing); DTCM via `.dtcm_from_flash` |
| C1 Resample | M85 | 24 kHz; hop 200 (or 100) samples (CC-1) | float32 PCM | native coefficient cache (NFB:202-213) | poly24 state; cached tables |
| C2 Fast events | M85 | every hop (and 64-sample sub-hop view) | per class `{event_id:u32 monotonic, t_sample:u64, strength:f32, level:f32, observed:bool}`; raw RMS dBFS; crest | E5 OnsetBeat (host oracle), PRISM-Core onset (product), E14 body cursor (cross-check) | ≈ 1 KB per board, DTCM |
| C3 Tonal | M85 | per hop | `mag[80]` float unsmoothed (exact k, int64 or float state); `linear_shape[80]`; per-band-conditioned display shape; `chroma[12]`, centroid, strength, register; flatness, rolloff; `sub_rms` | E7 4-band AGC (display view); E6 chord (after R06) | window 4,096 samples |
| C4 Context | M85 | per hop light, decimated tempo | loudness at 30 ms / 0.5 s / 2 s / 8–30 s; section reference + change score; cut event + timestamp; stored charge; tension features; tempo candidates with phase/variance/timestamp — **independent of Mood** | E17 River Surge / K1 DROP_CUT rule; LW TranslationEngine score; LW Transient Lattice charge; E8 TempoFieldV1 | ≈ 1 KB; optional 60 s ring ≈ 0.96 MB SRAM; optional MRAM warm start |
| C5 Publication | M85 | per hop | `MusicFrame` {epoch, generation, per-feature support interval and availability time}; legacy `InhouseWaveformAudio` fields (WH:84-116) kept; new fields inert by default | — | double buffer; per-board cursor; read-only after the single phase advance (TL11 TL-17/18) |
| C6 Choreographer | M85 | per render | state + visual parameters; consumer smoothing (Mood); reason code per frame | E3 Direction (enum, dwell, policy field), K1 mode_selection governor, SynqMatrix reason taxonomy | ≈ 0.5 KB per board, independent |
| C7 Field | M85 | per render | `intensity[160]` float, `offset[160]` float, owner event; ≤ 32 analytic fronts; optional rewind ring | E1 front maths; E15 continuous-time reference; LW TRM history idea | ≈ 2.6 KB per board (+ ≈ 128 kB per board if R27) |
| C8 Raster | M85 | per render | per-pixel HD sample in palette-position space; optional transition; board-1 relation offset | E10 PaletteTransition; E9 EdgeColour relation | — |
| C9 Display + constraint | M85 | per render | warmth/WB (named separately from calibration), shoulder, measured transfer, R00, the owner's envelope | — | small LUT |
| C10 Pack + present | M85 + GPT/ELC/DMA | 100 Hz now; **hop-locked 120 Hz** after CC-1 | GRB48 TRUE16; one declared present time; compare-match ELC stop + double-buffered GTBER unchanged | existing engines | existing |
| I Evidence taps | M85 + host | per frame | read-only snapshots of pre-treatment features, state, RGB16, packed lanes, event ids, gate reason, idle counter, effective envelope, per-onset stamps | statusJson (E11), BLM1 trace pattern | existing diagnostics; no new dashboard |

### 6.3 Compute budget per 8.33 ms frame (120 Hz)

| Item | Time | Label |
|---|---|---|
| Donor analysis (≈ 1.67 donor hops) | ≈ 0.48 ms | [INFERENCE] from 285k p50 per 5 ms (COST) |
| In-house analysis | ≈ 0.78 ms p50, 1.07 ms p95 | [INFERENCE] scaled from COST 699k / 965k per 7.5 ms hop to an 8.33 ms hop |
| ASRC | ≈ 0.22 ms | [INFERENCE] |
| VP both boards | ≈ 0.27 ms p50, 0.35 ms p95 | [FACT] COST (per record) |
| Emit / pack | ≈ 0.13 ms | [FACT] COST |
| **Subtotal today, re-based** | **≈ 1.9–2.5 ms (23–30 %)** excluding RT-Thread/USB/ISR | [INFERENCE] |
| C2 OnsetBeat-equivalent ×2 boards | ≈ 0.01 ms (FFT arm C: + ≈ 0.02–0.04 ms) | [INFERENCE] G2-F02, G2-F04 |
| C4 + C6 | < 0.05 ms | [HYPOTHESIS] |
| Exact k, chroma 80 | ≈ +0.05 ms | [HYPOTHESIS] |
| Fronts + fractional transport | ≈ +0.05 ms; up to +1.5 ms only while an 8× supersampled front moves | [HYPOTHESIS] |
| Rewind replay (R27) | memory copy of ≈ 160 × 2 × int16 per frame | [HYPOTHESIS] negligible CPU; 256 kB SRAM |
| **Projected** | **≈ 2.1–4.2 ms (25–50 %)** | [HYPOTHESIS]; gate on the idle counter (M0-4) before adoption |

**What runs where.** M85: everything (CANON:55). Helium/MVE only on a measured hotspot with scalar equivalence (PL14-B5). Ethos-U55: nothing in this plan; later candidate only after R05/R13 miss their frozen bars, ≤ 2 ms per hop, shuffled weights required to fail; `EmotiscopeMl` stays parked (§0.5). Cortex-M33: parked; if ever needed, I/O only (PDM servicing, LED transport), never render. DTCM: hot bounded state; ITCM not reopened without a boot fix.

### 6.4 Latency budget ledger (CC-3 fills the measured column)

| Term | Today, arithmetic mean [INFERENCE, G4-03] | After CC-1 + CC-2 | Measured |
|---|---|---|---|
| Capture slot wait | ≈ 3.6 ms (296 samples at 41,417 Hz) | ≈ 0.8 ms (64 words) | — |
| Hop wait | ≈ 3.75 ms (180 at 24 kHz) | ≈ 4.2 ms (200) or ≈ 2.1 ms (100) | — |
| PDM filter + resampler group delay | unmeasured | unmeasured | — |
| In-house AP | ≈ 0.70 ms p50 (COST) | ≈ 0.78 ms | — |
| Render wait | ≈ 5 ms (free-running 10 ms tick) | ≈ 0 (hop-locked; +≈ 2 ms if rendering every second 100-sample hop) | — |
| VP + emit | ≈ 0.4 ms | ≈ 0.4 ms | — |
| Wire (48-bit, 80 px per lane + 300 µs reset) | 5.1 ms (latch at reset [HYPOTHESIS]) | 5.1 ms | — |
| **p50 estimate** | **≈ 18–20 ms** | **≈ 9–11 ms** | target ≤ 8 ms, p99 ≤ 12 ms, jitter ≤ 4 ms, both boards |

The residual floor is the wire; CANON:35 reserves 24-bit + dither (≈ 2.4 ms saved) as a Captain choice after measurement. The P5 measured stack (≈ 16.4 ms newest sample → wire end, BUDGET.md:33-34) excludes the slot-fill wait and is consistent with this arithmetic.

### 6.5 Migration from v9 (host-006) that keeps the Titan visualising

Each step is additive. host-006 path P stays the default until a step passes its frozen gates and the Captain's A/B. New fields default to values that reproduce today's output; a new field that defaults to dark is a failed migration. Every new state falls back to FLOW on missing evidence; a detector failure returns to v9 behaviour, never to darkness or an invented release. Recovery images f6daf7fb / f2f1983a / b5595c35 remain (STATUS:201).

| Step | Content | Gate before build | Visible change |
|---|---|---|---|
| M0 Instruments | M0-1…M0-14 incl. M0-11 audio-in-file recorder and CC-3 baseline | each instrument red on its planted fault | none |
| M1 Geometry on labels | R01, R02, R03, R25, R26, R07, R00 behind runtime knobs (M0-8) or a compile flag with the v9 default | EXT-v3 + R00 on host; `tests/host/test_p6_product_path_p.py` `mismatches = 0` | drop release at labelled times (test image only) |
| M2 Fast path + canon architecture | R05 (host oracle → PRISM-Core clean-room), R12, CC-1, CC-2 | R05/CC criteria; snappiness oracle vs CC-3 baseline; transport safety clause | hits on time; event-driven release candidates; lower latency |
| M3 Dynamics + input truth | R09, R17, R04 (host replay → witness-lane sweep) | per-record criteria | drop fuller and brighter than build |
| M4 Causal release | R13 replaces labels; R27 as an A/B alternative | R13 hit rule, zero false full releases | release fires on real drops |
| M5 Colour | R06, R08, R10, R16, R22 colour arm | per-record criteria | truer, richer colour; width and low-end motion; related boards |
| M6 Board 1 + output truth | R11, R22 roles, R19, R14, R21, R28 | per-record criteria | upper board grooves at the right rate; smoother fronts and tails |

### 6.6 Ship path (required)

1. **Already on silicon / in source.** On the Titan: host-006 (`e79d1ac0`), path P with gains 0.580092/0.830949, 127/127 readback (STATUS:201). In source at `01ec2dc`: path P default, pre-knee colour shape, `sub_rms`, release and accent machinery (off); compiled into host-006 but unused by v9: composed inward fronts, Pair, Direction, EdgeColour, PaletteTransition, live speed and response gain; compiled but linker-discarded: OnsetBeat/MusicalSaliency `update()`; compiled out: 4-band AGC, TempoFieldV1. In PRISM-Core: the clean-room `music/` onset, envelope and saliency runtime (HEAD `89c79df`). In tools: `freeze_tolerance.py`, `p6_ext.py`, `palette_wire_snapshot.py`, `titan_camera_record.py`/`titan_camera_strip.py` (video-only today).
2. **Remaining steps.** M0 instruments (M0-11 recorder and CC-3 baseline first) → §9 Exp 1–3 → M1 host build frozen against EXT-v3 + R00 → product-equivalence pytest → licence route for any K1/GPL-lineage algorithm (PRISM-Core clean-room, tagged, explicitly re-pinned; never edit a vendored copy) → build → program → identity readback → wire-snapshot equivalence + camera strip scoring (audio in file for timing) → Captain A/B → repeat for M2–M6.
3. **Who acts.** The P6 lane owner (or the executing agent under that authority) builds, programs, reads back, changes live settings and records. The P7 lane owns R23. The PRISM-Core owner tags any clean-room component. The Captain judges the taste A/Bs in §7 and rules on the brightness envelope, render rate, 24-bit + dither (after CC-3) and licence admission.
4. **The stamp that means shipped.** The programmed image hash read back from the device matches the build receipt; every frozen criterion for that step went red on its planted fault before going green (`freeze_tolerance.py verify` clean); the wire snapshot reproduces the scored frames; and the Captain's A/B verdict is recorded in STATUS.

---


## 7. What the Captain judges by eye — small A/Bs (PROPOSAL — NOT APPROVED)

Each is one 10–15 s passage, two variants, same palette and matched maximum brightness. Four randomised presentations including one identical-pair control; a pre-declared 3-of-4 preference decides; a tie stays undecided. A machine guard is frozen first and its planted wrong variant must fail before the look. What the LEDs render is checked by wire snapshot and camera, never by asking the Captain; CLI agents cannot listen, so musical congruence is his call (CANON:53). The Titan keeps visualising through every no-flash look; each ends by restoring the preset and confirming the 127-field readback.

### 7.1 No-flash pre-screens (live settings on host-006)

All use existing live controls (§0.2 E4). Pre-screens on donor modes test **grammar, not the product look** (GPL BENCH, 64 bins at 12.8 kHz, 128 → 160 nearest, 16-slot snapshot). **Hazard settings for any donor composed-scene probe** (G5-HAZARD; EP:271-283, :690-697): background 0 (the v9 preset stores board 0 background 0.291, PRESET:12 ✓), screensaver off, `quiet_scope` 1 or 2, and a silent segment scored for exact zero; a planted run with scope 0 and screensaver on must fail exact zero.

| # | Clip | A | B | Decides | Machine guard frozen first |
|---|---|---|---|---|---|
| Q-R25 | BOOM 40–56 s, **lower plate** | speed 0.5 throughout | speed ramped 0.5 → 1.0 over the labelled build and riser windows (board 0 only) | whether velocity can carry tension (R25) | §9 Exp 3 |
| Q5 | Sonic Samba 20 s, lower plate | Mood 0.05 | Mood 0.5 (upper held at preset) | colour liveliness; first half of R07 | Mood readback equals command for board 0; board 1 unchanged (by source, board 0's Mood cannot change board 1: A:381-392) |
| Q2-pre | BOOM drop region | both boards on composed scene 1 (ribbons, trips ≥ 1: outward then inward return) | scene 2 call/answer (inward answer) | release **direction** grammar only, before any v9 build | answers carry the caller's gesture id; answer lit radius decreasing on ≥ 90 % of answer frames; exact zero in silence |
| C09 | BOOM 40–70 s | choreography 0 (independent) | choreography 1 (call/answer), `answer_delay` ≈ one beat | "does one panel answering the other read as conversation or distraction?" (R22) | cross-board lag of lit-energy peaks at hand-off + delay ± 1 frame; choreography 0 shows no lag peak |
| Q12-pre | 20 s | board 0 v9 path P | board 1 donor mode 1 Spectrum (palette position = strip position) beside it | an alternative view, not a replacement | board 0 wire bytes unchanged (pair independence) |
| Q3-pre | BOOM 49–58 s | response gain 1.0 | response gain < 1 on board 0 (contracts extent **and** amplitude) | rough feel of a contracted build (R03); not drop-timed | readback; declared confound: level also changes |

### 7.2 A/Bs that need a test image (flash)

| # | Clip | A | B | Decides |
|---|---|---|---|---|
| Q1 | BOOM 50–60 s | release T90 30 / 60 / 120 / 250 ms | × peak ×1 / 1.5 / 2 | frozen bars for R01 and R09 |
| Q2 | BOOM drop | front sweeps edges → centre | light born at the centre streaming outward (today's release; also the D12 sketch's shape, G5-SKETCH a ✓) | EXT-v3 direction clause confirmed or flipped |
| Q3 | BOOM 49–58 s | riser re-lights the plate (today) | held contracted, progressively, until the drop | R03 |
| Q4 | BOOM drop | release at 0 ms | +50 / +100 ms (labelled) | whether detection must be predictive |
| Q6 | BOOM build | spread k = 1 | k = 0.25 (≈ K1 EdgeMixer excursion) / tension-driven, opening on release | R08 |
| Q7 | BOOM drop | both boards release | lower releases, upper shifts colour (EdgeColour relation) | R22 |
| Q8 | BOOM cut | current 0.2 s drain | implosion toward the centre (D12 C03), reaching zero within the approved drain | cut gesture |
| Q9 | BOOM 40–65 s | current extent ease | instant attack | R02 ("twitchy" check) |
| Q10 | tonal passage | chromatic order | circle of fifths | R16 |
| Q11 | quiet passage, dark room | no degamma | degamma (only after R19 measures γ > 1.2) | R19 |
| Q12 | 20 s | path P | Spectrum River in v9 (bin k at pixel k; SB `6109a1e`) | an alternative view |
| Q13 | ordinary music 20 s | today's grammar (louder → light pushed to the edges) | **edge-origin grammar** (louder → light reaches further toward the centre; the drop is the fullest moment of one continuous mapping; D11 §2/§7) | the larger fork behind Q2; if B wins, R01 and R03 become one mapping |
| Q14 | BOOM drop | analytic inward front (R01) | rewind the build (R27), then front | release gesture |
| Q15 | BOOM drop | cubic ease, T_r 100–180 ms (EXT-v3 bar) | Lightwave Resonant reference timing (linear, 280 ms attack / 480 ms body) | front speed; whether the EXT-v3 T90 ≤ 0.18 s bar is right |
| Q16 | BOOM drop | one front | front + 2 echoes sized by charge (R26) | release size as count |

**Rulings, not A/Bs.** (1) Which brightness envelope binds on this unit: 24/255 maximum (global mandate, 27 Sept), ≥ 100/255 default (PRH:19-23, 29 Sept; Kx-Titan AGENTS.md:11-12, 3 Oct) or master 255 × gains 0.25 (CANON:51, 14 Oct) — the running image is the last, ≈ 64/255-equivalent (§8.2 X1). (2) 120 Hz hop-locked (default under CC-1) or 133 Hz for its latency gain. (3) After CC-3 measures it: whether to take 24-bit output + dither if the 48-bit wire is what breaks p50 (CANON:35). (4) Licence admission for K1-lineage algorithms, or the PRISM-Core clean-room route (§0.6). (5) The drop annotation convention (grid downbeat vs first transient vs bass arrival). (6) Whether quiet music between −33 and −28 dBFS may drain to black (R15). (7) Naming tracks for a sealed holdout.

---

## 8. Contradictions and open questions

### 8.1 Disagreements settled from source

**Carried from v2 (D1–D20, amended where marked).**

| # | Disagreement | Positions | Settled (evidence) | Effect |
|---|---|---|---|---|
| D1 | Does the live analyser wrap int32 and delete loud bass? | S2 yes (ledger #2, S5-02 rank 15, plain-English §1, open Q2); S1, S3, V1 no | [FACT] A:37-42 int64 product and q0, overflow counted; A:66-68 int64 magnitude; A:268-271 comment; A:308 any overflow invalidates the publication; [INFERENCE, IC] max state 13.9 M at guarded full scale, 154.6× below INT32_MAX | S5-02 removed; the trap stays in the parked kernel; re-check if R17 widens input |
| D2 | HPF corner 128 Hz vs 259 Hz | D01 128; D10 259; S2 "neither"; S3 conditional | [FACT] MAN §50.3.4.3: HPF operates on the sinc result; §50.3.4.5: the LPF then halves the rate → HPF clock 2·Fout; [INFERENCE, IC] model 100.1 Hz at 32 kHz, 259 Hz at 82.8 kHz | 259 Hz is the supported model; still [HYPOTHESIS] until the sweep; I capped at 3 |
| D3 | Where do the HPF coefficients come from? (S2: "weak citation, not a tracked file") | S2 | [FACT] `hal_data.c` is generated from the pinned BSP into the disposable stage (`build_scalar.py:816-820`); 0x3F61/0x3EC1 at :617-618 in all 9 staged evidence copies and in the host-006 stage; PDM has no HPF override | citation is sound; it is generated, not hand-tracked |
| D4 | Does the existing release "work when it fires"? | V1 yes ("filled edge to centre"); S1 + CRIT G1 no | [FACT] W:977 pulls the deposit to pixel 80 in one frame; transport only scrolls outward (W:1003-1015); EXT-v2 extent = innermost pixel ≥ 10 % of peak and its planted positive is a teleport (EXT2) | V1 claim withdrawn; inward motion needs new geometry (R01); EXT-v3 required |
| D5 | Is colour per board or board 0 only? | V1 C51 + V1 Exp 3 board 0 only; S1 per board | [FACT] `acquire()` overwrites spectrum, novelty, `linear_shape` and tempo per board (A:381-392); Mood per board (LR:87-90; FX:1666) | per board; Q5 and §9 Exp 3 run on the lower plate only |
| D6 | Ranking formula | S2, S3, V1 I×N÷E; S1 I×N×P÷E | CRIT G9: the plain formula ranks taste-only items above drop levers | expected value adopted; both columns shown (unchanged in v3; effort now counts existing machinery) |
| D7 | Event handoff rank | S3 L02 #2 (10.0); S1 folded into TP-04 | [FACT] today's non-peak consumers are already smoothed (EMA τ ≈ 21 ms, A:274; Mood τ ≈ 80 ms) and the peak is max-held (A:419-425) | a queue alone shows little; merged into R05, where it matters |
| D8 | HPF impact | S2 5 (score 25, #1); V1 5 (#1); S1 3* | CRIT G7: follower and AGC sit after it; host replays omit HPF and guard | I = 3 until §9 Exp 2 reports extent; rank 4 |
| D9 | Ramp gain k rank | S2 #2 (16) | taste; overlaps dark-end fix | merged into R08 (rank 8) |
| D10 | Circle of fifths | S2 #3 (12); S1 #15 | taste only; P 0.25 | R16, rank 16 |
| D11 | Donor VU publication (S2 S5-05 #9) | S2 | [FACT] ×3.93 is quiet cut (50–55 s) → drop (`docs/evidence/P3/P3-RECORD.md:143-150, 178`); GPL-lineage bench code | parked pending licence; not ranked |
| D12 | Is path P on the device? | S2 "different claims, not read" | [FACT] STATUS:201 readback 127/0 against the v9 preset after `d557f06`; camera multicolour | on the device (lane readback); not product-qualified |
| D13 | VP cost of path P | S1 270k (+48 %); S2/S3 0.197 ms (P5 image) | [FACT] COST host-006 `vp_cycles` p50 269,903; P5 0.197 ms is image `f2f1983a` | use host-006 numbers; both true for their images |
| D14 | Reach cap 0.7 as a drop lever | S2 (via S5-14) | [FACT] W:976-977: with outward polarity a higher cap shortens the lit extent | only paired with R01 (inside R02) |
| D15 | Tempo prior removal | S2 expects restoration | [FACT] GATE-SYNTHETIC-v1 flat prior 29/326, best 35/326 (STATUS:200) | design work (R23); R11 bypasses for board 1 |
| D16 | Crystal clock for PDM to delete the ASRC | D10 HW-04, CANON:54 open question | [FACT] PL14:147: PDM clock is MOCO-only (manual §9.10.34) | closed; CAC rate truth still useful |
| D17 | Camera: "the middle never fills at the drop" | S2 spot-check vs S2 synthesis | [FACT, seen] no sweep at 55.7 s; lower half fills its middle after ≈ 57 s | finding stands as "no release front"; half-to-board mapping open; times now [HYPOTHESIS] (D26) |
| D18 | Early 54.3 s return: detector fault or choreography? | S1 F-CAM1; S3 Q3 | [INFERENCE] no detector is active; returning sound + decayed follower park deposits at the edge (A:250; W:976) | choreography (R03); "legitimate response" wording withdrawn (D38) |
| D19 | "24/255 is old" (S3 §8 Q2) | S3 wording | global Titan mandate binds 24/255; conflict reported, not resolved | §8.2 X1 |
| D20 | Novelty "lower in the drop" | D01 summary | [FACT] only the median; mean, p90 and max are higher; ±30 ms max below build p95; the "≈ 330 ms late" part is withdrawn in v3 (D23) | the defect is dilution and margin (R05) |

**Settled or corrected in this pass (v3).**

| # | Disagreement / v2 claim | Settled (evidence) | Effect |
|---|---|---|---|
| D21 | v2 §1 item 2 and L28: "the light can only scroll outward today, so an inward sweep needs new drawing code" | [FACT ✓] true only of the in-house v9 path. `composed_motion::renderRibbon(inward = true)` is an analytic edge → centre front, linked in H6, host-tested to light the outer pixel at t = 0, live-selectable on donor modes (CM:81-139; EP:710, :754-766) | R01 is "port and adapt" (E 2 → 1.5); EXT-v3 planted positive reuses its maths |
| D22 | v2 parked "AudioPipeline kick/snare/hat (compiled out)" | [FACT ✓] OnsetBeat and MusicalSaliency are compiled; `update()` is linker-discarded (H6 map:234, :4430-4431, :4473-4474); only `reset()` is linked. `AudioPipeline::process` uses the parked int32-wrap kernel (`gdft_goertzel.cpp:43-75`) | component reuse only, fed by the int64 analyser; R05 host arm E = 1 |
| D23 | v2 A18/R05: "novelty peaks ≈ 330 ms late at the BOOM drop" (attributed to "fresh verdict D01") | [FACT] D01's REPORT.md has no such figure and D01 was never verified (D01 VERIFY.md: "Claims checked: 0"). [FACT, computed this pass] `P6-DROP-DETECTOR/v3/results/probe/boom_file.csv` (variant baseline only, 3,333 rows at 22.5 ms, tempo-path `novelty_in`): build (45–50 s) p95 0.0692; max within ±30 ms of 55.7025 s 0.0573; first crossing of the build p95 at 55.74 s (+37.5 ms); the largest peak in 55.5–56.7 s is at 56.19 s (+487 ms), which is later music, not the drop onset | "330 ms late" withdrawn; the defect is dilution and margin (first hop ≈ 2.7 % of a step; ±30 ms max below build p95), not a fixed lateness. R05's case stands on those and on K1's own pre-AGC ledger |
| D24 | v2 L15/R09/§8.4: "per-band AGC as the S3 ships; the Titan should follow K1" | [FACT] a 4-band AGC already exists in the Titan tree (`gdft_postprocess.cpp:56-61, :130-196`), compiled out; [FACT, G2-F08] K1's per-band AGC is a short leveller that fixes cross-band crush (device spread 0 → 0.145) but cannot preserve build → drop contrast | per-band arm E 2 → 1; R09 still needs the long-window reference and one owner per field |
| D25 | v2 §2 header: "palette id 31 / Crimson Gold id 38" | [FACT ✓] PRESET stores selections 32 and 39 (:33, :84); PR:248 computes `palette_id = palette − 1` | write "selection 32 / catalogue index 31", "selection 39 / catalogue index 38"; re-verify R08's masked fractions on the entry the device samples |
| D26 | v2 §4.0 / F-CAM1 "about 1.4 s before the drop" and the §5 rule "admissible at r ≥ 0.7 or with a sync marker" | [FACT ✓] CANON:52 (Captain, 14 Oct): timing needs received audio in the same file (iPhone microphone, MacBook fallback), 240 fps for latency, plus device stamps; the recorder is 30 fps video-only and aligns by brightness ↔ loudness (circular) | F-CAM1 interval → [HYPOTHESIS]; geometry stands; new M0-11; rule replaced |
| D27 | v2 R18 "hop-locked at 133 Hz" | [FACT ✓] Kx-Titan AGENTS.md:9 and TL14-037 rule 120 Hz logical rendering; CANON:35 rules render per audio hop; [INFERENCE] a 200-sample hop at 24 kHz = 8.333 ms satisfies both | CC-1 targets 120 Hz; 133 Hz only on a Captain ruling |
| D28 | v2 R20 "64–96-sample slots" | [FACT ✓] CANON:35: "64-word PDM capture blocks (~1.6 ms)"; 96 words = 2.32 ms | CC-2 = 64 words |
| D29 | v2 ranks R18/R20 by visual impact (18th, 22nd) | [FACT ✓] CANON:26 "keep 8 anyway, because it forces the right architecture"; CANON:35 makes them architecture | moved to the unranked canon-conformance track |
| D30 | v2 R18 criterion: p50 ≤ 8 ms, p99 ≤ 12 ms, ≥ 500 onsets | [FACT ✓] CANON:31-32 also require jitter (p99 − p50) ≤ 4,000 µs, first visible change on **both** boards, the product build profile, real music plus unpredictable onsets; "worst case" retired (CANON:33) | all clauses frozen in CC-1/CC-3 |
| D31 | v2 cites the snappiness oracle for R05/R11 and "mode 18" only | [FACT ✓] CANON:19 names Waveform Tempo **and** Waveform Hybrid K1 and "any new audio backend or scheduler" | oracle clause on every backend/scheduler lever, both modes |
| D32 | v2 L36 "6,169 levels at 24/255" | [FACT ✓] running image: master 255 × board intensity 0.25 (PRESET:127-128; CANON:51); [INFERENCE] ≈ 16,384 codes ≈ 64/255-equivalent | luma criteria become ratios to the running envelope (M0-12); conflict reported (X1) |
| D33 | v2 K15 "dither is the wrong repair on TRUE16" | [FACT ✓] CANON:35 reserves 24-bit + dither as a Captain latency choice if the 48-bit wire is shown to break p50 | K15 reworded; option kept open |
| D34 | v2 R22 "reusing `PairedWaveScene` edge hand-off" | [FACT, G5-F03] `PairedWaveScene` is referenced only by host tests and is not in the `build_scalar.py` file lists; the on-device call/answer is `composed_scene::Pair` | cite Pair for on-device probes; PairedWaveScene is a design reference |
| D35 | v2 never cites the Lightwave lineage (0 hits) although CANON:14 requires lineage citation | [FACT ✓] Captain's Tier 1 registry `0e2e0778`; inventory G3 | lineage cited per lever (§0.4, §5.3) |
| D36 | Novelty of R13 and R01 | [FACT] Lightwave SynqMatrix already classifies Build/Drop (default OFF; ≥ 250 ms dwell); Direction already sequences phrases; the inward front exists on the donor path | R13's novelty is the charge gate and observed-evidence firing; R01's is the in-house, labelled, path-P, sub-200 ms version; N adjusted |
| D37 | D12's sketch as support for the inward drop | [FACT ✓, image opened] D12 `drop_zoom.png` panel B lights the middle at once and opens a black wedge outward; `concept_sketch.py:95-104` grows `front_r` from 0 | D12's own evidence supports the outward reading; Q2 must show both |
| D38 | v2 D18: "early return is choreography, not a detector fault" (and S3 Q3 "legitimate response to the riser") | [FACT] no detector is active (unchanged); the musical legitimacy is a listening judgement (CANON:53) | the mechanism (follower + clamp) stands as [INFERENCE]; "legitimate" becomes "agent cannot listen" |
| D39 | Kx-Titan TL14-057 "reference mic + photodiodes on one clock" vs CANON:52 iPhone | [FACT, G4-02] TL14 bound a 44-line canon copy committed 09:10 +0800; canon §5 exists in the working-tree file modified 10:15 +0800 | CANON:52 supersedes TL14-057 |
| D40 | Topology "two data lines / 9.6 ms" in Kx-Titan AGENTS.md:9, README.md:11, TL14:15, TL14-035/-036 | [FACT ✓] CANON:49 (14 Oct, later): four lanes × 80 px, 4.8 ms per lane; product PR:691-737 uses 80 px per lane | atlas uses 80 px / 5.1 ms; stale Kx-Titan text needs a docs fix in that repo (outside this swarm's scope) |

### 8.2 Contradictions still open (reported, not resolved)

| ID | Conflict | Sources |
|---|---|---|
| X1 | **Brightness authority (chronology, G4-07).** 27 Sept global Titan mandate: maximum 24/255 → 29 Sept PRH:19-23 boot default ≥ 100/255 (static_assert) → 3 Oct Kx-Titan AGENTS.md:11-12 "default 100/255 supersedes historical 24" → 14 Oct CANON:51 master 255 × gains 0.25 → running image ≈ 16,384 codes ≈ 64/255-equivalent [INFERENCE]; TL14-055 records an unattributed 100 → 255 drift on 13 Oct. Earlier v2 wording: Global Titan mandate: maximum 24/255. `Kx-Titan/AGENTS.md:12`: default 100/255 supersedes 24. `PRH:19-23` static floor ≥ 100; `build_scalar.py:933-934` rejects < 100. CANON:51: master 255 × gains 0.25. Runtime readbacks show gains 0.25 and 1.0; 0.25 is a runtime setting (PRH:215, :228 default 65535). At least five gain stages could carry an envelope. Every luma criterion waits on one number from the Captain. | S1 §8.1, S2 §8, S3 Q2 |
| X2 | **Render rate.** Brief 120 Hz; product 100 Hz (PRH:40); CANON:35 rules hop-triggered render; Kx-Titan AGENTS.md:9 and TL14-037 rule 120 Hz logical; K1 S3 locked 120 Hz (`3776e777`). v3 reconciles by a 200-sample hop (exactly 8.333 ms) [INFERENCE]; open only if the Captain prefers 133 Hz's latency (§7 ruling 2). | CC-1 |
| X3 | **Drop direction.** The Captain's verbatim (EXT2) reads edges → centre; K1 canon says "heart outward"; D12's own sketch releases from the centre outward (D37); the Captain's Lightwave Tier 1 picks include inward contraction (Resonant, tension) and backward convergence (TRM Mod1, impact). EXT-v3 adopts the verbatim; Q2 (and Q2-pre without a flash) confirms; Q13 asks the larger grammar fork. | R01, R27 |
| X4 | **AV asymmetry.** ITU-R BT.1359-1 detectability ≈ −125/+45 ms (broadcast conditions); CANON:26 notes it; not a Titan tolerance. The P6 +50 ms bar was agent-chosen. | Q4 |
| X5 | **Latency stack.** ≈ 16.4 ms P5 stack already includes the 5.1 ms wire and excludes slot-fill wait; adding the wire again double-counts. CANON:28 quotes 4.8 ms (without the 300 µs reset). §6.4 gives the per-term ledger CC-3 must measure. | BUDGET.md:33-34; §6.4 |
| X6 | **"Hard cut never while music plays" vs "fade fully to black on silence"** — compatible only if the silence gate is right. | R15 |
| X7 | **White.** "5–10 % at most, never full white" vs promo "warm-white blooms" (low weight); camera cream regions unexplained. | R00 |
| X8 | **Detector record wording.** "All failed to fire" vs v1 and v2 each hit one drop; none passed the set. Downbeat error "26–380 ms" mixes product nearest-beat errors with the reference clock. | T-DET |
| X9 | **Lightwave port assessment brightness.** LWA MUST 4: "lane default 100/255 supersedes historical 24" (LWA:112) — one more source in the X1 conflict. | G3 |
| X10 | **Licence of K1-lineage algorithms.** Kx-Titan disposition marks `onset_beat.cpp` "GPL-3.0 legacy/uncertain; BENCH_ONLY, REFERENCE_ONLY" (`accepted-runtime-disposition.json`; `donor-disposition.json:3883-3900`). "Uncertain" is not settled; Lightwave per-file rights were not re-audited (no LICENSE found at firmware-v3 root in G4's read). Product route meanwhile: PRISM-Core clean-room. | G2-F03, G3-F05, G4-17 |
| X11 | **Stale Kx-Titan topology and geometry text** (two data lines / 9.6 ms; "one continuous 160 scene split 80/80" at AGENTS.md:14 / README.md:85 vs AGENTS.md:9 "two independent 160-pixel scenes"). | G4-08, G4-18 |
| X12 | **EXT-v3 T90 ≤ 0.18 s vs the Captain-selected Lightwave reference timing** (Resonant: linear 280/480 ms). The bar is agent-chosen; the reference is Captain-selected on other hardware. Q15 decides. | G3-P01 |
| X13 | **What "drop" means for the hit window.** P6 "+50 ms" bar is agent-chosen (X4); CANON's onset-class target is the 8/12 ms pipeline plus a separately declared detection budget (CANON:34, :36). | G4-13 |

### 8.3 Traceability

**v2 synthesis ids → final ids (carried; R18 → CC-1, R20 → CC-2 in v3).**

| Final | S1 | S2 | S3 | V1 |
|---|---|---|---|---|
| R00 | TP-20 | — | (no-regression gate) | — |
| R01 | TP-01 | S5-06, S5-14 | L01 | A02 |
| R02 | TP-05 | S5-15 | L05 part | A03 |
| R03 | TP-02 | — | L04 part, L16 | A18 part |
| R04 | TP-06 | S5-01 | AP row (conditional) | A01 |
| R05 | TP-04 | S5-16, S5-17 | L02, L03 | A07, A25 |
| R06 | TP-13 | S5-11 | L19 | A12 |
| R07 | TP-07 | — | L12 | A10 |
| R08 | TP-08 | S5-03, S5-07 | L17 | A14 |
| R09 | TP-03 | S5-08, S5-12 | L08 | A06, A11 |
| R10 | TP-10, TP-12 | S5-13 | L10, L11 | A09, A16 |
| R11 | TP-09 | — | — | — |
| R12 | TP-11 | S5-21 part | — | A22 part |
| R13 | — (§5.1) | S5-14 part | L04 | A18 |
| R14 | TP-15 | S5-18 | L05 | A20 |
| R15 | M0 gate word | — | L18 | A13 |
| R16 | TP-14 | S5-04 | L09 | A05 |
| R17 | TP-16 | S5-10 | L14 | A08 |
| R18 | TP-19 | — | L15 part | A26 |
| R19 | TP-17 | — | L13 | A21 |
| R20 | — | — | L15 | — |
| R21 | — | — | — | A23 |
| R22 | TP-18 | — | L06 | A17 |
| R23 | §5.3 | S5-20 | L07 | A24 |
| R24 | — | — | L20 | A27 |
| M0-1 | M0 | — | evidence taps | A04 |
| M0-6 | L4 | S5-19 | AP row | A15 |
| M0-8 | M1 note | — | migration step 3 | A19 |
| removed | — | S5-02 (D1), S5-05 (D11) | — | — |
| parked | §5.3 | S5-21 (licence/robustness) | outside top 20 | A22 |

**v3 gap-closer traceability (every gap-closer id → where it landed).**

| Gap-closer ids | Landed in |
|---|---|
| G1-F01 inward ribbon | §0.2 E1; R01; D21 |
| G1-F02 Pair | §0.2 E2; R22; Q2-pre, C09 |
| G1-F03 Direction | §0.2 E3; R13; R03 |
| G1-F04 live controls | §0.2 E4; M0-8; R25; §7.1 |
| G1-F05 River Surge | §0.2 E17; R03; R13 |
| G1-F06 OnsetBeat | §0.2 E5; R05; D22 |
| G1-F07 saliency/chord | §0.2 E6; R08, R13 inputs |
| G1-F08 4-band AGC | §0.2 E7; R09; D24 |
| G1-F09 AudioPipeline | §0.5; D22 |
| G1-F10 TempoFieldV1 | §0.2 E8; R23 |
| G1-F11 EdgeColour | §0.2 E9; R22; R08 |
| G1-F12 PaletteTransition | §0.2 E10; R07 |
| G1-F13 ribbon objects | §0.2 E15; R14; T-NEG |
| G1-F14 product catalogue | §0.2 E16; R12, R11 |
| G1-F15 donor modes | §0.2 E12; Q12-pre |
| G1-F16 v8 family modes | §0.2 E13; R22 |
| G1-F17 donor shaping | §0.2 E14; R05 cross-check; R10 mask; §0.5 ML |
| G1-F18 telemetry | §0.2 E11; R15 |
| G1-F19 centre effects / TempoReferenceScene | §0.2 note; §0.5; R23 instrument |
| G2-F01/F02 ONSET_V2 and its Titan port | §0.3; R05 |
| G2-F03 licence | §0.6; X10; ship path |
| G2-F04 donor FFT onset front-end | L39; R05 arm C |
| G2-F05 Smart Director | §0.3; §0.5; K-DIR |
| G2-F06 mode_selection governor | §0.3; R13; R01 |
| G2-F07 Beat-Aware Director lock occupancy | §0.3; K-LOCK; R11; scoring penalty |
| G2-F08 per-band AGC | §0.3; L15; R09; D24 |
| G2-F09 EdgeMixer | §0.3; R08; R22 |
| G2-F10 DROP_CUT | §0.3; R03; K-CUT; M0-14 |
| G2-F11 visual hooks | §0.5 |
| G2-F12 k1_tempo / flywheel onset | R23 sub-arm; R05 interactions |
| G2-F13 saliency | §0.2 E6 limits |
| G2-F14 HarmonixSet | §0.3; R13 negatives |
| G3-F01 inventory | §0.4 |
| G3-F02 Captain Tier 1 | §0.4; R01, R25, R26, R27 |
| G3-P01 Resonant | R01; Q15 |
| G3-P02 TRM Mod1 | R27; Q14 |
| G3-P03 Void | R03; P15 |
| G3-P04 Shocklines/Veil | R25 |
| G3-P05 Ripple ES | R12, R26 |
| G3-P06 Bass Quake | R01 re-arm; R10 caution |
| G3-P07 SynqMatrix | R13; R15; M0-14 |
| G3-P08 Lattice / Saliency Bloom | R26; R01 crispness |
| G3-P09 Star Burst | R07; R08 |
| G3-P10 EdgeMixer per state | R22 |
| G3-P11 Progressive Ignition | R02 |
| G3-P12 Shockwave Cascade | R26; Q16 |
| G3-P13 TranslationEngine | R09; R13 |
| G3-P14 Kuramoto | R11 |
| G3-P15 Attack-only colour | R07 arm C |
| G3-F03 DEC-009 | §0.4; §0.5 |
| G3-F04 do-not-port defects | §0.5; M0-13 |
| G3-F05 port status | §0.4; ship path |
| G4-01 metric | T-LAT; D30; CC-1/CC-3 |
| G4-02 instrument | §4.0; D26; M0-11 |
| G4-03 budget arithmetic | §6.4; CC-3 |
| G4-04 render rate | CC-1; D27 |
| G4-05 capture blocks | CC-2; D28 |
| G4-06 snappiness oracle | §5 rules; D31 |
| G4-07 brightness | L36; X1; D32; M0-12 |
| G4-08 topology | X11; D40 |
| G4-09 independent scenes | R22 |
| G4-10 unhobble | §2.5 |
| G4-11 palette indexing | §2 header; D25; R08 |
| G4-12 clocking | CC-1 clause |
| G4-13 latency scope | R05 detection budget; X13 |
| G4-14 transport safety | R00 note; CC-1 |
| G4-15 one normaliser per field | R09 |
| G4-16 elapsed-time law | L14; CC-1 |
| G4-17 Shocklines lineage | R01 lineage; R25 |
| G4-18 respected rulings | §8.1 (no change) |
| G5-F01 inward front | R01; D21 |
| G5-F02 Pair antiphony | R22; C09 |
| G5-F03 PairedWaveScene | D34 |
| G5-F04 Direction | R13 |
| G5-F05 SynqMatrix prior art | R13; D36 |
| G5-C17 riser accelerando | R25; §9 Exp 3 |
| G5-C01 Spring | R03 progressive clause; R26 |
| G5-SECMEM | R28 |
| G5-C03 implosion drain | Q8; parked |
| G5-C09 antiphony rank | R22; C09 |
| G5-C10, C11, C14, C16, C13 | parked (§5.1) / §0.5 (C13) |
| G5-EDGEORIGIN | Q13; parked as a lever |
| G5-ANTICIP, PALMER, PHRASE, DECAY | parked |
| G5-SKETCH | D37; R08; R12; R03/R09 build-luma clause |
| G5-HAZARD | §7.1 hazard settings |
| G5-MOOT | confirms path P keeps palette resolution (V4) |
| G5-COUNT | §8.3 note: D12 "35" = 15 levers + 19 concepts + 1 ranking |
| G5-RERANK | §5.1 (R25, R22 probe, R28) |

Worker-level traceability (268 worker levers → atlas rows, CRIT G2) is still not written. D01 is still unverified (D01 VERIFY.md); any D01-only figure in this atlas is labelled as such.

### 8.4 Transfer back to the K1 ESP32-S3 (default product path)

| Titan finding | Applies to K1? | K1 note |
|---|---|---|
| Follower 0.25/0.005 caps drop extent | yes, same constants (#93763) | trivial on 240 MHz |
| Extent polarity loud→edge; release missing; outward-only transport | yes (Waveform Hybrid K1 source) | same R01/R03 levers, same A/Bs |
| Novelty after the Mood LPF | yes (K1 `process_GDFT`) | ≈ 200 cycles/hop |
| Rounded k | yes | re-test now that int64 ships |
| `chroma_range` 60 | yes (v40102 default) | taste + correctness |
| Path-P ramp across dark palette ends | yes where per-bin HD colour is used | R08 |
| Broadband AGC | no: K1 product runs per-band | the Titan has the same 4-band AGC in tree (E7); follow K1 for the colour/display view only — per-band AGC does not preserve build/drop contrast on either product (G2-F08) |
| HPF retune | no (I2S microphone on K1) | — |
| Pre-AGC onset input (R05) | yes — K1's own ledger names it as the missing step (forward-graft :120) | log-flux on pre-AGC GDFT magnitudes; ≈ 0 extra cost |
| Inward release front, riser velocity charge, stored charge (R01, R25, R26) | yes (Waveform Hybrid K1 source) | same A/Bs; Lightwave Resonant/Shocklines already ran on K1-class hardware |
| EdgeMixer anchor (R08/R22) | origin | the Titan borrows K1's accepted excursion |
| DROP_CUT exit rule (instant relight) | yes | the R03 exit rule applies on K1 too |

### 8.5 Open questions (who can answer)

1. What is the PDM HPF's real response on this board, and how much bass do the capsule and room deliver? — §9 Exp 2, then witness-lane sweep (lane owner).
2. How often does the loud guard engage during a real drop? — R17 log arm.
3. Which camera half is which board, and are the full-width dark lines musical stops, gates or camera artefacts? — palette-id identification + wire snapshot (R15).
4. Does the LGP hide the 4 px beading? What is the WS2816 transfer on this plate? — static bead frame and ramp with locked exposure (R14, R19).
5. How far do the two boards' light fields overlap in the diffuser? — one-board capture (R22).
6. Can the 127-field live schema carry release/cap/ceiling/k so A/Bs need no reflash? — M0-8.
7. Real CPU use including RT-Thread, USB-CDC and ISRs — idle counter (M0-4).
8. Is the GPL-lineage donor VU / tempo bank admissible, or should PRISM-Core's clean-room runtime be the reference? — licence review.
9. Which Captain tracks form a sealed holdout? — the Captain.
10. Is a speaker fitted to J4 (DAC chirp instrument, D10 HW-13)? — hardware owner.
11. Is the licence of K1-lineage onset/saliency/tempo code resolvable, or does every such lever go through PRISM-Core clean-room? — Captain / legal (X10).
12. What is the live control path's command → readback latency per board? — §9 Exp 3 Step 2.
13. How much SRAM is free for a 256 kB rewind ring (R27)? — idle counter and memory map (M0-4).
14. Does the iPhone (Continuity) path deliver audio in the same file via ffmpeg, or only through native Slo-mo files? — M0-11 Step 0 (CANON:52 says the iPhone microphone is always used).
15. Which brightness envelope binds (X1), and does the Captain want 120 or 133 Hz (X2)? — the Captain.

---


## 9. Immediate next three experiments

All three are **PROPOSAL — NOT APPROVED** until the lane owner runs them under existing authority. Two run on the host with the existing frozen-gate machinery; one runs on the device with no flash, the camera loop and one Captain look. First copy the replay set to the external scratch drive (M0-9), because it is volatile. Each experiment answers a different half of "the drop does not hit harder": the **look** given perfect timing (Exp 1), the **timing and input** the look will need (Exp 2), and whether **motion** can carry tension on this plate (Exp 3).

### Experiment 1 (host): does an inward front, built from the Titan's existing edge→centre maths, pass a test that can see direction?

- **Question.** With a release fired at the frozen drop anchors, does an edge→centre front pass an extent test that also checks direction, while today's teleport release and every other planted wrong shape fail it?
- **Existing machinery.** `tests/host/p6_effect_probe.cpp` (path P via `mapped`, release arguments at :138-146); `scripts/p6_ext.py evaluate --criterion v2`; EXT2 cases BOOM source, BOOM room, Sonic @27.7, Sonic @54.7 (anchors 55.7 / 55.7 / 27.7 / 54.7 s); known negatives `P6P/results/ext/ext-v2-b5595c35.json`, `ext-v2-v8.json`; replay inputs (M0-9). **New this pass:** the analytic inward-front maths in `platform/ra8p1/composed_motion.h:81-139` (band, three-tap trail, smoothstep feather; host-tested in `harness/tests/test_composed_motion.cpp:37`) as the generator for the planted positive and the reference for the renderer; the `K1_INHOUSE_V9_HOST_VARIANTS` guard (W:565, :586, :631) for the renderer arm.
- **Freeze first** (`freeze_tolerance.py freeze`): `TOLERANCE-FROZEN-P6-EXT-v3.json` = every EXT-v2 clause unchanged, plus a **trajectory clause**: from t_d, the innermost lit pixel i_min(t) (same 10 % luma rule) is non-increasing toward the centre for ≥ 3 consecutive frames before extent first reaches 0.9; T90 ≤ 0.18 s; the lit-run front velocity is inward on ≥ 95 % of frames during transit; no extent jump ≥ 0.3 within 50 ms anywhere in [t_d − 2 s, t_d); plus the three R00 clauses; plus a **palette clause**: during transit the front's per-pixel colour equals `sampleProductPaletteHd(wrap(offset + d))` within tolerance (catches a 16-slot snapshot). A new evaluator mode `--criterion v3` in `scripts/p6_ext.py` (a tools change, not firmware).
- **Stage A, instrument validation (no firmware change).** Score frame sets built by overlay on the host-006 path-P replay frames. Planted negatives: (a) host-006 as is; (b) the existing instant release forced at t_d (X1/X3 shape: i_min jumps to 80 in one frame); (c) a centre-born outward front (also D12's sketched shape); (d) a correct inward front 200 ms late; (e) the donor ribbon as it is — 16-slot snapshot colour, RGB-added trails (fails the palette clause); (f) linear travel at the donor's 200 ms floor (fails T90 ≤ 0.18 s). Planted positive: the composed maths ported to 160 px / 80 radial pixels with path-P colour, cubic ease, T_r = 120 ms. Pass condition for the instrument: (a)–(f) red, positive green on every tuning case and both boards. If the teleport cannot be made red, EXT-v3 is redesigned before anything else is scored.
- **Stage B, renderer.** Implement the R01 overlay under `K1_INHOUSE_V9_HOST_VARIANTS`, driven by the labelled times, in two arms: B1 cubic ease, T_r 100–180 ms (the candidate); B2 the Lightwave Resonant reference timing, linear 280 ms (an informative arm: it is expected to fail the agent-chosen T90 bar and exists to give the Captain Q15). `tests/host/test_p6_product_path_p.py` must still report `mismatches = 0`, proving the product image is untouched. Decision rule frozen in advance: pass = B1 passes EXT-v3 on all four tuning cases on both boards and C2/C3/C4/C5b still pass; the lag clause is satisfied by construction and declared non-evidence.
- **What the result means.** Pass: the look is solved given timing; next are R03, R25, R26 and R02 in the same harness (bundle ablation, §5.2), then Q2/Q15/Q1 for the Captain, then R05/R13 for timing. Fail on size or contraction: R02/R03/R09 come before any detector work.

### Experiment 2 (host): audio truth — does an onset detector that reads the raw sound find the drop, and does the microphone's bass cut matter?

One replay harness, two arms with separate frozen criteria, sharing the BOOM / Sonic / click corpus.

- **Arm A — onset input (R05).**
  - Existing machinery: `k1::core::audio::OnsetBeat` (`src/k1/core/audio/onset_beat.{h,cpp}`; host parity test `test_onset_v2_parity`) — compiled but linker-discarded on the device, so this runs on the host only. A small host driver builds a `contract::AudioFeaturesV1` per hop from the in-house analyser's output and calls `OnsetBeat::update` per board. No firmware change; no product admission (GPL-uncertain: oracle only).
  - Variants: (a) today's post-Mood/AGC/knee spectrum (planted negative); (b) the pre-Mood magnitude EMA (A:274) — the candidate; (c) unsmoothed magnitudes (to separate the EMA's effect). Gate floors re-derived on (b)/(c) only from the build window of the tuning cases, before scoring the drop.
  - Freeze first: click corpus onset → event median ≤ 15 ms, p95 ≤ 25 ms (detection budget, CANON:36); BOOM: a kick- or transient-class event within [0, +30 ms] of 55.7025 s with flux strength ≥ 2 × the build-window p95; ≥ 99 % of 1,000 random single-hop impulses produce exactly one id increment, in order, across render-phase sweeps and a 30 ms render pause; identical PCM at two Mood settings gives identical event times. Planted: variant (a) must fail the BOOM clause (if it passes, the plant is invalid — report it and redesign); latest-only publication must lose ids; a 30 ms input delay must fail the median.
- **Arm B — bass cut and guard (R04/R17; v2 Exp 2, extended).**
  - Pre-stage M0-5 (scratch script): first-order high-pass at the model corner (259 Hz), the candidate (20 Hz) and a planted 400 Hz, each followed by ×16 and the per-296-sample-slot guard at ceiling 30,000 (SENS:9-33). Feed BOOM source through each into the path-P probe **and** into arm A's variant (b).
  - Freeze first: model check 100 ± 2 Hz at 32 kHz; a 1-LSB k1 change moves fc by > 0.5 Hz; drop `sub_rms` with 20 Hz ≥ 6 dB above the 259 Hz model; the planted 400 Hz ≥ 3 dB below the 259 Hz model; R04's visual impact rises above 3 only if median extent in [t_d, t_d + 1 s] differs by ≥ 0.05 or J by ≥ 0.10 between 259 Hz and 20 Hz; a guard ceiling of 3,000 pushes the drop's engaged-slot fraction above 90 %. New: report the arm-A kick-class strength at the drop under 259 Hz vs 20 Hz (no threshold frozen; descriptive).
- **What the result means.** Arm A pass: R05's host oracle exists; the PRISM-Core clean-room onset is scored against it next, then R11/R12/R13 can consume real events; mode-18 energy rate can switch to events. Arm A fail with (a) also failing: the detector body needs the FFT arm C or the biquad arm D. Arm B large: justifies the witness-lane device sweep (no Captain look) and raises R10 and R05 arm C. Arm B small: demotes R04 and confirms the follower/AGC (R09) is the binding stage. `boom_room` is supporting evidence only.

### Experiment 3 (device, no flash, one Captain look): make the camera able to judge time, then test whether speeding the light up through the riser makes the drop land harder

- **Step 0, instrument (tools only, no device change).** Change `scripts/titan_camera_record.py` to record an audio device (iPhone microphone; MacBook Pro microphone as fallback) instead of `'{DEVICE}:none'`, and `scripts/titan_camera_strip.py` to align the recording's own audio to the played WAV (audio-to-audio cross-correlation), never brightness to loudness (M0-11; CANON:52). Freeze first: offsetting the recorded audio by +100 ms in a copy moves the measured alignment by 100 ± 5 ms; a recording without an audio stream is refused, not scored; a brightness-aligned run is marked inadmissible for timing. 30 fps remains a choreography-scale instrument (≈ 33 ms frames); ms-scale latency waits for native 240 fps slow motion (CC-3).
- **Step 1, wire truth (no Captain).** On host-006 as installed (identity readback first): wire snapshot of BOOM 40–75 s with `scripts/palette_wire_snapshot.py`, compared with the host replay of the same audio. Freeze first: packed-lane equivalence within the existing tolerance on both boards; exact zero in any frame whose source PCM is digital zero; the frames at the camera's dark lines classified wire-dark or wire-lit. Planted: the host-005 gains (0.396/0.592) or a palette-B replay must fail the comparison. This also re-times F-CAM1 from the wire.
- **Step 2, control latency (no Captain).** Twenty live `speed` changes on board 0 via the existing targeted control path, each confirmed by readback. Freeze first: command → readback p95 ≤ 150 ms. If it fails, the puppet moves to a host replay (R25 E becomes 2) and Step 3 is skipped.
- **Step 3, the puppet and one Captain look.** Board 0 only (lower plate); board 1 stays at its preset. Re-record BOOM 40–70 s with the same framing (`titan_camera_record.py` serve/request mode, server in the Captain's Cursor tab because of macOS camera permission, STATUS:202-203; strip `titan_camera_strip.py --panel 107,220 --t0 40 --t1 70 --marks 49.7,54,55.7`). Arm A: speed 0.5 throughout. Arm B: speed ramped 0.5 → 1.0 across the labelled build (45.0–49.7 s), back to 0.5 at the cut, ramped again across the riser (54.0–55.7 s), back to 0.5 at the drop. Four shuffled presentations including one identical pair. Freeze first: every speed readback matches its scheduled value within the measured Step-2 latency; board 1's speed readback never changes; wire-measured trail-front velocity (cross-correlation of successive right halves) rises ≥ 1.7× from ramp start to ramp end − 0.2 s; per-frame packed luma within ±5 % of arm A at matched audio (velocity, not brightness); exact zero where the source is silent. Planted: a "ramp" whose commands are all 0.5 must fail the 1.7× clause; an arm whose speed is scheduled on board 1 must fail the board-1 clause. Camera (audio in file): trail slope visibly steepens over each ramp.
- **Question for the Captain.** "On the lower plate, does the light speeding up through the build and riser make the tension feel stronger and the drop land harder?" Agent cannot listen; his answer settles Q-R25 and decides whether R25 joins the M1 bundle.
- **Restore.** Speed 0.5 on board 0; confirm the 127-field readback against the v9 preset with 0 differences; confirm visualisation never stopped (no flash, no dark interval). Q5 (Mood on the lower plate, v2's Exp 3) is the next no-flash look in the queue (§7.1).

---

## 10. Files read and checks run by this integrator (v3)

- Read in full: SWARM-BRIEF.md; LEVER-ATLAS-v2.md (681 lines); the five gap-closer results G1–G5 (as passed in by the workflow).
- Spot-checked against source this session (marked ✓ above): `git rev-parse HEAD` = `01ec2dc`; CM:81-95, :120-139, :188-197 (inward start, `path = returning ? 1 − x : x`, RGB `add`, mirror, travel validator 200–8,000 ms); CSC:44-50 (200 ms floor); EP:706-712, :752-766 (in-house early return; mixed-mode bypass); PR:243-272 (palette − 1, `motion_scale = speed·2`, response gain before clamps); W:963-999 (405·motion, 8 steps, 16/23 ms EMAs, release pulls, mode-18 rate); `titan_camera_record.py` (:5 "a recording, not an instrument", :24 'EiP Camera', :31 `-framerate 30 … :none`); `titan_camera_strip.py` (:8 brightness↔loudness cross-correlation, :24 FPS 30); PRESET :12 background 0.291, :33/:84 palettes 32/39, :46/:97 quiet_scope 2, :55/:106 screensaver false, :57/:108 speed 0.5, :127-128 intensities 0.25; CANON:14-20, :24-36, :40-56 (incl. §5 rulings); H6 `rtthread.map` :234 "Discarded input sections", :4429-4432, :4472-4475 (OnsetBeat/MusicalSaliency `update` at 0x0); H6 `symbols.txt:929` (`OnsetBeat::reset` linked); Lightwave `git log 0e2e0778` (2026-05-17 16:42 +0800, "Director Effect Registry Lock — default-deny allowlist"); `SynqMatrix.cpp:71, :83, :89, :104-122` (Tier 1 registry; Resonant TENSION; Ripple ES IMPACT); `BeatPulseResonantEffect.cpp:8, :31, :37, :60` (inward contraction; 280/480 ms); `BeatPulseRenderUtils.h:342-370` (fallback metronome); K1 `git rev-parse` = `68a95773`, `platformio.ini:185-188` (`-DK1_AGC_PERBAND_V1=1`), :149 (`-DK1_DROP_CUT_V1`); `tests/host/test_p6_product_path_p.py:1-4`; `K1_INHOUSE_V9_HOST_VARIANTS` at W:565, :586, :631; script presence (`freeze_tolerance.py`, `p6_ext.py`, `palette_wire_snapshot.py`, camera scripts); STATUS:196-205; D01 VERIFY.md and REPORT.md novelty lines; `SYNTH-S1-opus-v2.md:69, :122` (origin of "330 ms").
- Computed this session: novelty timing at the BOOM drop from `P6-DROP-DETECTOR/v3/results/probe/boom_file.csv` (baseline variant, 3,333 rows; §8.1 D23).
- Opened and inspected: `D12-creative-vision/drop_zoom.png` (release fills the middle at once, then a black wedge opens outward); read `concept_sketch.py:95-110, :160-170`.
- Not re-checked by this integrator (carried from the gap-closers with their citations): K1 director/onset/EdgeMixer line numbers and device evidence (G2); Lightwave effect bodies other than Resonant and SynqMatrix (G3); Kx-Titan AGENTS.md, TL11/TL14, NFB and LWA line numbers (G4); composed-scene runtime lines CSR (G1/G5); OPERATOR-VERDICT quotations (G1/G5).
- Not done: no build, replay, flash, device read, live-setting change, camera run or commit. Visual inspection of this atlas's own output: N/A — it is a text document with no rendered output; the only image claims are carried (camera strip, v2) or re-checked (D12 sketch, this pass). The only file written is this atlas.
