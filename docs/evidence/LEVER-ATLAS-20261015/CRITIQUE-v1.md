---
abstract: "Completeness critique of the Titan lever swarm and LEVER-ATLAS.md, 15 Oct 2026. It ranks 14 gaps, most important first. The biggest: the decisive drop experiment cannot tell a centre flash from the edge-to-centre thrust the Captain described; half of the light (board 1, mode 18) has no levers; the creative-vision work was mostly dropped; new camera evidence from the Titan is not in the atlas; and two of the three next experiments are confounded or would overclaim. Research only. Every recommendation is PROPOSAL — NOT APPROVED."
id: LEVER-SWARM-CRITIQUE
date: 2026-10-15
author: Completeness critic (Claude Opus 5.5), read-only on all repositories
inputs: "SWARM-BRIEF.md; LEVER-ATLAS.md (all 555 lines); SYNTH-S1/S2/S3; all 13 VERIFY.md files; the 5 REPORT.md files on disk; the 13 recovered dimension results (/tmp/s1synth, now copied to CRITIC/recovered/); product worktree HEAD 01ec2dc; Kx-Titan docs/migration; PRISM-Core music/"
images: "Inspected: docs/evidence/P6/camera/boom-host006-a-strip-40-70.png (camera space-by-time strip of host-006 on the Titan, BOOM 40-70 s). It was opened and read; findings in G4. No LED output was produced by this critique."
changelog:
  - "2026-10-15 critic: first version. Also copied the volatile recovered dimension results into CRITIC/recovered/ with SHA256SUMS (G2)."
---

# Critique of the Titan lever swarm

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.** Nothing below changes that. Nothing here adds a gate, a brightness floor or a rainbow default. Nothing here touches the brightness envelope, UID, pins or DMA assignments.

## In plain English

The atlas is careful and mostly correct about the parts it covers. It is not complete, and two of its headline conclusions go further than the evidence.

1. **The main drop test cannot see the motion the Captain asked for.** The existing "release" jumps the drawing point to the centre in one frame. The frozen test only asks whether the strip is lit from edge to centre. A centre flash and a real edge-to-centre thrust both pass. So "the release already works when it fires" is not shown.
2. **Half of the Titan's light has almost no levers.** The upper board runs a different effect (mode 18, "Waveform Tempo"). Its motion is driven by the half-time tempo tracker today. The atlas maps only the lower board's effect.
3. **Most of the creative work was lost on the way to the atlas.** Eight of the 13 worker reports were never saved to disk. The creative-vision worker proposed 19 concepts; about 4 reached the portfolio. The Lightwave effect library, which the Titan canon names as a reference, was never mined.
4. **The Titan has been filmed since the atlas was written.** The footage shows light pinned to the panel ends, and light returning about 1.4 s before the drop. That steals the drop's contrast. The atlas does not include this.
5. **Experiment 3 starts from a wrong fact.** The colour-speed (Mood) control is per board. Changing it on the lower board does not change the upper board's colour, as the atlas assumes. Changing it on both boards would also change how fast the upper board moves.
6. **The top-ranked lever (microphone bass cut) is unmeasured.** Even if correct, the loudness levellers that the atlas blames for the flat drop sit after it and would cancel much of its effect.

What is left: fix the drop test so it scores direction, add the upper board to the map, recover the dropped creative and hardware levers, fold in the camera footage, and de-confound Experiment 3. The ranked list is below.

---

## Ranked gap list

| # | Gap | Why it matters | Severity |
|---|---|---|---|
| G1 | The decisive drop test (A02, §9 Exp. 1) cannot distinguish a teleported centre deposit from an edge-to-centre thrust | The #2 lever and the "look is solved given timing" conclusion rest on it | Critical |
| G2 | 8 of 13 dimension reports were never on disk; S2/S3 saw only 2–4 KB digests; no lever traceability from 268 worker levers to the atlas | No completeness denominator; dropped levers are invisible | Critical |
| G3 | Board 1 / mode 18 (half the light, the canon's "snappiness oracle") has no pipeline row and no lever | Tempo errors are visible on the device today, not only an enabler | High |
| G4 | Device camera evidence (commits 6c48925, 01ec2dc) post-dates the atlas and adds an unmapped failure | First on-device look at host-006 after path P | High |
| G5 | Creative and artistic vision largely dropped (D12 C01–C19, Lightwave 207 effects / DEC-009 / Shocklines / Veil) | This is the Captain's explicit complaint ("copying and pasting … ARTISTIC and CREATIVE VISION") | High |
| G6 | §9 Exp. 3 assumes board 0's Mood colours both boards; `acquire()` makes it per board (refutes C51). A both-board A/B would change board 1's motion | The Captain's verdict would not mean what it is labelled as | High |
| G7 | A01 (rank 1) impact 5 rests on an unmeasured model and ignores the downstream normalisers | Ranking inflation; experiment 2 does not measure the light | Medium–High |
| G8 | Titan canon rulings not carried into the atlas (acoustic-to-photon 8/12 ms target, hop-triggered render, 1.6 ms capture blocks, snappiness non-regression, MOCO-to-crystal question) | The atlas down-ranks a ruled requirement (A26 rank 26) | Medium–High |
| G9 | No value-multiplier, bundle or expected-value analysis | The Captain asked for multipliers; the I×N/E formula ranks a taste-only colour reorder (#5, impact 2) above the follower fix | Medium |
| G10 | Dropped hardware and instrument levers (on-device deterministic input, MOCO/crystal, sound-detection pre-trigger, SRAM section memory, MRAM warm start, speaker chirp) | Several displace work or make host-vs-device equivalence testable | Medium |
| G11 | Unmapped known failures (noise lifted to music level, beat barely reaching the plate, starved mic, filter-design stall, full-white incident, strobing) and no output safety layer | "Safety still binds" during testing; no flash-rate or full-white guard in the architecture | Medium |
| G12 | Sources and lineages not read: PRISM-Core music runtime, K1-DualMCU, LightwaveOS-Production, several Kx-Titan migration docs | PRISM-Core already holds clean-room answers to C01/C10/C13/A07 | Medium |
| G13 | No transfer table back to the K1 ESP32-S3 (the default product path) | The same follower and AGC constants run on K1 | Medium |
| G14 | Modalities not run: no experiment executed, verification by the same model family, Grok memory down, TypeSafe/JEV unused, 3.2 GB replay set still volatile | Every impact score is a judgement; key inputs can vanish | Low–Medium |

---

## G1. The drop test cannot see direction (Critical)

- [FACT] The release pulls the drawing point with `position = 80 + (position − 80)(1 − release_)` (`src/k1/core/visual/inhouse_waveform.cpp:977` mode 32; `:999` mode 18). `updateReleaseV9` sets `release_ = max(x, release_·e^(−dt/0.6))` with no attack ramp (`:794`), and the header comment says "instant attack" (`:752-754`). At `release_ = 1` the drawing point moves to pixel 80 in one frame.
- [FACT] Transport always scrolls outward (atlas V2; `W:1004-1015`), capped at 8 steps per frame in mode 32 (`W:968`), so no front can travel inward through the scroll.
- [FACT] EXT-v2 defines extent as the innermost pixel at ≥ 10 % of the frame peak, measured from the edge (`TOLERANCE-FROZEN-P6-EXT-v2.json`, "extent(t)"). Its scope is "Host replay only (not the M85, no packer/gamma/cap/dither)".
- [FACT] X3 on BOOM source reached extent 1.0 at T90 = 0.04 s, which is 4 frames (`P6P/results/ext/ext-v2-X3.json`, rows board 0 and 1).
- [INFERENCE] A 4-frame T90 is the teleport, not a travelling front. A centre-born deposit lit while old trails remain near the edges gives extent 1.0 at once. What the viewer then sees is new light streaming outward from the centre, the opposite motion to "left/right edges towards the centre" in the Captain's frozen definition (same file, `captain_definition_verbatim`).
- Effect on the atlas: §1 item 3 ("the release you described already works, when it fires"), §4.1 row 3, §8.1 D9, and §9 Exp. 1's reading "Pass: the look is solved given timing" all overclaim. X6 and E2 flag direction as open, but Exp. 1 is still presented as decisive.
- Fix (PROPOSAL — NOT APPROVED): add a trajectory clause to a new frozen version (EXT-v3) before Exp. 1 runs. For example: from fire time, the innermost lit pixel must move monotonically inward over ≥ 3 frames, with the lit-run front velocity sign negative (edge → centre). Planted fault: the current instant-attack release must fail that clause. Or run E2 with the Captain first and freeze whichever direction he picks. Keep the existing clauses unchanged.

## G2. Lost worker output and no traceability (Critical)

- [FACT] D01, D02, D05, D06, D09, D10, D12 and D13 have no REPORT.md. The harness refused the writes (each VERIFY.md says so).
- [FACT] S2 and S3 received, for those eight, only a digest per dimension. Each digest held the keys `summary`, `lever_count`, `refuted`, `contradictions` and `open_questions`, with summaries of 1.6–3.9 KB. No lever records were included (parsed from `SYNTH-S2-grok/prompt.md`, "DIMENSION DIGEST").
- [FACT] Only S1 recovered the full results, to `/tmp/s1synth/` (`SYNTH-S1-opus.md:466`). `/tmp` is volatile. D05 also exists only at `/tmp/d05_result.json`, which the atlas itself warns may disappear (§4.2).
- [FACT] Done by this critic: copied all 13 recovered results into `CRITIC/recovered/` with `SHA256SUMS`. `D05-failure-corpus.json` is byte-identical to `/tmp/d05_result.json`.
- [FACT] The workers recorded 268 levers: D01 21, D02 25, D03 25, D04 21, D05 16, D06 16, D07 18, D08 18, D09 15, D10 22, D11 13, D12 35 (15 levers, 19 concepts and a ranking row), D13 23. The atlas has 27 portfolio items and 52 ledger rows. No table says which worker lever became which atlas row, or why one was merged or rejected.
- [FACT] D01 (AP map) and D06 (Captain intent) were never verified against their own text. Their verifiers received nothing. D12's concept claims and D13's per-lever origins were also never verified.
- Fix (PROPOSAL — NOT APPROVED): write `TRACEABILITY.md`, a 268-row table with columns worker lever id → atlas id, or "merged into X", or "rejected because Y". Re-dispatch the D01 and D06 verifiers on `CRITIC/recovered/D01…json` and `D06…json`. G3, G5, G10 and G11 below are what a first pass of that table turned up.

## G3. Board 1 / mode 18 is unmapped (High)

- [FACT] The v9 preset runs board 0 in mode 32 with palette 32, and board 1 in mode 18 with palette 39 and warmth 0 (`docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json`). The Titan canon approves the same split: lower Waveform Hybrid K1 32/32, upper Waveform Tempo 18/Crimson Gold 39 (`Kx-Titan/docs/migration/TITAN-CANON-2026-10-13.md` §5).
- [FACT] Mode 18 sets its scroll rate as `w·tempo_rate + (1−w)·energy_rate`:
  - w is up to 0.7 from tempo confidence;
  - `tempo_rate = 24·BPM/60·(1 + 0.5·cos(2π·phase))`;
  - `energy_rate = 30 + 90·clamp(novelty/0.12)`;
  - up to 30 steps per frame;
  - position `80 + peak·80` with no 0.7 cap (`W:980-999`).
- [FACT] D02 records "Tempo half-time lock (P7) drives board 1's mode-18 pulse speed at half rate". D05 T13 records "mode-18 gate open at wrong tempo; weight capped 0.7 in 53 % of frames".
- [INFERENCE] Tempo is therefore a lever visible on the device today, on half the light. It is not only an enabler for A18. A24 (rank 24, "enabler") is under-ranked for that reason. A03's reach cap and the C32 polarity analysis describe mode 32 only. In mode 18 the loud deposit already reaches pixel 159, which is consistent with the camera's "light anchored at the panel ends" (G4).
- Missing atlas rows: a pipeline-map row M5b (mode-18 transport and position), with ledger rows for:
  - the tempo-phase modulation, read without phase extrapolation (P-13);
  - the 30-step cap;
  - the novelty/0.12 onset scaling;
  - the 125 ms fade.
- The canon names Waveform Tempo a "SNAPPINESS ORACLE" (§2). Any AP smoothing change must not make it less snappy, and no atlas experiment guards this.

## G4. New device evidence is missing (High)

- [FACT] After the atlas (integrated at HEAD 045b6b5), commits `6c48925` and `01ec2dc` added an iPhone camera recorder and the first recording of host-006 on the Titan. RUNS `STATUS.md` 18:30Z: "multicolour on device; light anchored at panel ends; cut dark; light returns ~54.3 s (1.4 s before drop) → drop has no contrast; no full-extent release at 55.7".
- [FACT] I opened the strip `docs/evidence/P6/camera/boom-host006-a-strip-40-70.png`. Both halves show light concentrated at the ends with a darker centre band throughout, and no visible change in extent at 55.7 s. There is a lit band from about 54.3 s, and several full-width dark lines at about 57.4, 59.3, 64.5 and 66 s. Alignment is weak: offset 5.73 s, r = 0.58 (`boom-host006-a-strip-40-70.json`).
- [INFERENCE] This adds a failure the atlas does not map: **pre-drop re-entry**. The riser after the cut re-lights the plate before the drop, so the drop has no dark-to-light contrast. Candidate levers: A13 (silence and re-entry), A11 (headroom), and A18's ARMED/PROVISIONAL states, which should hold the contraction through the riser.
- [HYPOTHESIS] The full-width dark lines are camera artefacts, musical stops, or a gate closing during music (P-10). The fix is to correlate them with a wire snapshot, not to judge them by eye.
- The camera is also an instrument the atlas does not use. It can serve A21 (light curve), plate blur and beading (§8.3 Q6), and the canon's ACOUSTIC_TO_PHOTON measurement (G8). The canon rules that this measurement uses the iPhone at 240 fps. The recorder needs the Captain's Cursor-tab server because of TCC (STATUS 18:25Z). Name that as a dependency.

## G5. Creative vision dropped (High)

- [FACT] D12 proposed 19 concepts (C01–C19) plus new stages L09 (output warp) and L10 (vertical composition across the shared diffuser) (`CRITIC/recovered/D12-creative-vision.json`). The atlas contains none of these terms: Spring, GrooveFlywheel, LuminanceIris, EmberTails, NegativeSpaceGlints, RiserAccelerando, PushPull, MetricHierarchy, LatentTexture (EmotiscopeMl latent), Antiphony (reuses the existing `PairedWaveScene`) or ConsonanceSpread. Each has 0 occurrences in LEVER-ATLAS.md. Only fifths (A05), timbre width (A16), sub push (A09), colour chapters (A10) and two-board roles (A17) survive.
- [FACT] The canon makes Lightwave firmware-v3 a reference oracle: 207 effects, DEC-009 eight presets, and Perlin Shocklines and Veil identified from footage (`TITAN-CANON-2026-10-13.md` §1–2). The atlas cites Lightwave only for brightness-only trails. "Shocklines", "DEC-009" and `LIGHTWAVE-FIRMWARE-V3-PORT-ASSESSMENT-2026-10-12.md` appear nowhere in the swarm.
- [INFERENCE] The portfolio is about 80 % forensic repair. The Captain asked for "ARTISTIC and CREATIVE VISION" and value multipliers. The atlas has no section on new visual grammar.
- Fix (PROPOSAL — NOT APPROVED): add a creative portfolio section scored on the same fields. RiserAccelerando (C17) and Spring (C01) speak directly to G4's pre-drop failure. Antiphony and LatentTexture reuse code that already exists (asymmetric leverage). Each needs a frozen guard plus one Captain A/B.

## G6. Experiment 3 rests on a refuted premise, and the obvious repair is confounded (High)

- [FACT] `InhouseAudio::condition` applies each board's Mood low-pass (`magnitude_last`) before AGC, knee and novelty (`platform/ra8p1/inhouse_audio.cpp:318-367`). `publication_` copies board 0's fields (`:302-306`). However, `acquire()` then overwrites `spectrum`, `spectral_energy`, `novelty` and `linear_shape` with `conditioning_[board]`, and tempo with `tempo_[board]` (`:381-392`).
- [FACT, refutes atlas C51 and §9 Exp. 3] Colour analysis is per board. Board 0's Mood does **not** set colour for both boards; it changes board 0 only. The atlas statements "both boards read board 0's `linear_shape`" (C51) and "board 0, which sets colour for both boards" (Exp. 3 step 2) are wrong.
- [INFERENCE] As written, Exp. 3 would change only the lower board's colour. The Captain would watch an A/B where one plate changes and the other does not, which is not the question asked.
- [FACT] If both boards' Mood are set instead, board 1's mode 18 picks up a changed `novelty` through `energy_rate = 30 + 90·clamp(novelty/0.12)` (`W:990-991`), and a changed tempo input (`:296`). Its scroll speed and beat pulse then change together with the colour. That is a confound.
- Fix (PROPOSAL — NOT APPROVED): run the colour-speed A/B on board 0 only, and tell the Captain to judge the lower plate, with board 1 held at its preset. A both-board variant needs a frozen guard: the board 1 scroll-rate trace must stay unchanged within ±5 % across the two settings. Planted fault: Mood applied to board 1 must fail that guard. Correct C51 and §1 item 8 in the atlas.

## G7. A01 is ranked on a model, and its visible effect is unproven (Medium–High)

- [FACT] The host-006 staged BSP carries the same reset HPF coefficients (`_scratch/waveform-migration-001/arm-p6-v9-host-006/stage/ra_gen/hal_data.c:613-619`, shift 0). The atlas cited the v8 stage copy; this confirms the coefficients apply to the image actually on the device.
- [FACT] Kx-Titan's own PDM profile lists "Sinc/compensation/HPF/LPF transfer … qualification" as remaining (`Kx-Titan/docs/migration/native-pdm-profile.json` "remaining"). The open item was already known and is not cited.
- [INFERENCE] Restoring bass raises the build and the drop alike unless the drop carries relatively more bass. Extent is driven by `peak_scaled = drive/follower` (C17) after a broadband AGC (C18), which the atlas says flatten the drop. A01 alone may barely move the light. Exp. 2 measures `sub_rms`, VU and the surge, not extent or the EXT clauses, so it cannot show visual impact.
- [FACT] Every host replay (EXT-v2, PAL-HD, D02 occupancy) feeds PCM straight into the analysis. None of them includes the PDM HPF or the per-slot loud guard. D01 notes "the host replay omits the guard".
- [INFERENCE] Host-scored drop levers are therefore measured on a cleaner input than the device sees (P-8).
- Fix: score A01 with I = 3 [HYPOTHESIS] until Exp. 2 also reports EXT extent. Add the HPF model and the guard as an optional host pre-stage, so host scoring can match the device's input chain.

## G8. Titan canon rulings not carried (Medium–High)

- [FACT] `TITAN-CANON-2026-10-13.md` §3 rules ACOUSTIC_TO_PHOTON as the only "audio-to-light" metric: p50 ≤ 8 ms, p99 ≤ 12 ms, jitter ≤ 4 ms. It also rules render triggered by the audio hop, about 1.6 ms (64-word) capture blocks, and that detection latency is budgeted separately.
- [FACT] §5 rules that the PDM rate comes from MOCO (±10 %) and asks whether PDMIFCLK can take a crystal source; if it can, the resampler is deleted. §5 also rules the core split (if ever needed: M33 for I/O, never render on M33).
- [FACT] The atlas has 0 mentions of ACOUSTIC_TO_PHOTON, 240 fps or MOCO. It ranks render-on-hop (A26) at 26 with "the smoothers dominate latency". The 296-sample (7.15 ms) capture slot (D10 HW-19) and MOCO-to-crystal (HW-04) are absent from the portfolio.
- [INFERENCE] The latency container needs a lever set against a ruled target, measured with the camera that now exists (G4). Detection latency (A07) and pipeline latency (A26, HW-19) are separate budgets under the canon, and the atlas merges them.

## G9. No multipliers or expected value (Medium)

- [FACT] Score = I × N ÷ E (§5). There is no probability of success, no confidence interval on impact, and no table of lever combinations. D12 had five bundle concepts; the atlas has one use of "multiplicative".
- [INFERENCE] The drop is plausibly a conjunction. Input truth (A01/A08), section-scale reference (A06), headroom (A11), release geometry with direction (A02 + G1) and timing (A07/A18) may each be necessary and none sufficient. Ranking them separately hides that.
- [FACT] A05 (circle-of-fifths colour order, impact 2, taste only) ranks 5th, above A06/A07 (impact 5). The atlas admits the formula bias (D13, D14) but keeps the order.
- Fix (PROPOSAL — NOT APPROVED):
  - add an expected-value column (P(success) × impact);
  - add a small interaction matrix for the drop cluster;
  - name one "drop bundle" host experiment that switches the bundle on together and then removes one lever at a time (an ablation).

## G10. Dropped hardware and instrument levers (Medium)

All are from `CRITIC/recovered/D10-hardware-leverage.json`, and none appears in the atlas portfolio:

- HW-20, on-device deterministic input (UAC2 composite beside CDC). This is the only way to run frozen gates on the product image with the source PCM. It closes P-8 directly. Its trigger waits on the USB-CDC-NOATTACH root cause (PL14-A11).
- HW-04, CAC measurement of MOCO against the crystal, and a crystal clock for PDMIFCLK. This could delete the ASRC (133k cycles per hop, 1.14 ms delay) and the PL14-B2 alignment bug class.
- HW-05, the PDM sound-detection interrupt as a transient pre-trigger. The atlas mentions it only in passing in §6.
- HW-16, song-scale memory: 60 s × 80 bins × 100 Hz × int16 ≈ 0.96 MB fits in SRAM. This is the substrate for A18 and for phrase-level composition (D09-L13, D07-L13).
- HW-21, MRAM warm start of AGC and tempo after a flash or reset. It shortens the cold start after every 41–47 s dark flash.
- HW-13, the ES8156 DAC chirp. Whether a speaker is fitted is unknown.

## G11. Unmapped failures and no output safety layer (Medium)

- These failures were recorded by workers but appear in no atlas row:
  - AGC lifts room noise to music level (0.68 at RMS 0.018; D05 T17, D01);
  - "sound-reactive but not music-reactive" (D06; S3 prompt 69611);
  - the beat barely reaches the plate, camera reach about 13 % (D05 T18);
  - the bench mic was starved at −51.9 dBFS with crest 7.6 dB (D05 K10; the Titan capsule level is unmeasured, C46);
  - poly24 filter design can stall a frame 3–6 ms, poll high-water 8.67 ms (D13 L21 / PL14-D8);
  - LedFx-style "almost strobing" (D09).
- [FACT] The words "photosensitive" and "flash safety" appear nowhere in the atlas or the three syntheses. The 8 Oct full-white incident is in D06, with the Captain's verbatim reaction.
- [INFERENCE] The target architecture (§6) needs an explicit output constraint stage before the packer, holding three bounded rules:
  - a full-white budget;
  - a general-flash-rate limit (the common guideline is ≤ 3 flashes per second);
  - exact true black in silence.

  It must never be a brightness floor. An "explosive" release is exactly the kind of event that needs this guard. D09-L14 proposed such a constraint layer, and it was dropped.

## G12. Sources and lineages not read (Medium)

- [FACT] PRISM-Core is never mentioned by any swarm file (0 hits). Its clean-room music runtime (`SpectraSynq-PRISM/PRISM-Core/music/README.md`) has:
  - a 20 Hz DC rejection (compare C01);
  - a periodic Hann window over 4,096 samples (compare C10);
  - 64 bins from 55 Hz (compare C13);
  - 15/150 ms envelopes and onsets with a 60 ms refractory (compare A07);
  - a four-axis saliency tracker (`music/saliency.h:7-34`, opt-in candidate).

  [INFERENCE] These are existing, provenance-clean references for several levers. They also bear on the GPL-lineage question (§8.3 Q7). The canon freezes Kx-Titan as a tree, but not PRISM-Core as a reference.
- [FACT] Never read by any swarm file:
  - K1-DualMCU;
  - LightwaveOS-Production;
  - Kx-Titan `LIGHTWAVE-FIRMWARE-V3-PORT-ASSESSMENT-2026-10-12.md`, `TITAN-ARCHITECTURE-REVIEW-2026-10-10.txt`, `NATIVE-FIRST-BOOT-2026-10-08.md` (named in the brief), `TITAN-SESSION-LESSONS-2026-10-11.md` (named in the brief) and `native-pdm-profile.json`;
  - `_scratch/music_visual_value_20261008`.
- [FACT] D03 read K1.esp32s3, K1.LightwaveOS, k1-ws2816-source-precision (1,039 commits) and K1_Testbed at log-subject level only, for the analyser (`D03 REPORT.md:24-30`). Their visual-side history, including the WS2816 16-bit degamma work relevant to A21/C40, was not mined.

## G13. No transfer back to the K1 ESP32-S3 (Medium)

- [FACT] The atlas says the 0.25/0.005 follower, the broadband AGC, the 80→12→1 colour fold and the 88 BPM prior are inherited from K1 (C17, C18, C26, C21). It contains no table of which Titan findings apply to the K1 S3 (0 hits for back-port, Founders or "K1 product").
- [INFERENCE] K1 is the default product path (global charter). A one-page transfer table would multiply the swarm's value at no research cost. Its columns: finding → applies to K1? → K1 cost on the 240 MHz core → evidence.

## G14. Modalities and durability (Low–Medium)

- [FACT] No lever experiment was executed. Even the host-only Exp. 1 and Exp. 2 are proposals. Every impact and novelty number is a judgement.
- [FACT] Adversarial verification of the eight Opus dimensions was done by Opus verifiers only (STATUS 17:14Z: "per-dimension adversarial verify"). No cross-family check of those lanes.
- [FACT] Grok lanes ran without shared memory ("shared memory service failed a refresh", D03/D04/S2 logs). D03 took three attempts and ended rc=143. Only 2 of 13 dimensions ran on Grok, although the Captain authorised as many as needed.
- [FACT] The Captain invoked `/typesafe:typesafe-ai`; TypeSafe/JEV appears nowhere in the swarm. [INFERENCE] Low impact on the firmware levers. A possible role is typed judgement over camera clips against the Captain's frozen definition, as an evaluator, never as authority.
- [FACT] The replay set `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6` is 3.2 GB, outside the repo and volatile (atlas §8.3 Q11). Under the storage policy it belongs on `/Volumes/Disk Storage/SpectraSynq-Scratch`. This critic did not move it, because it is over 50 MB and outside the run folder.

---

## What holds up

These were spot-checked and confirmed:
- host-006 is the image on the device (STATUS 17:58Z, 127/0 readback).
- host-006 is un-hobbled: D-cache enabled, -O3, MVE (receipt flags; `hal_entry.c:225-231` only disables the cache under `K1_FORCE_DCACHE_OFF`).
- The live analyser uses int64; the int32 wrap belongs to the parked kernel.
- The AP cost holds: host-006 `ap_cycles` p50 0.98 M and p95 1.25 M cycles per record, about 20–25 % of one M85 (`cost-host-006/report.json`).
- Mood is live-settable per board (`k1_live_runtime.h:87-90`, `fixture_app.cpp:1666`). It is applied per board, which is what refutes C51 (G6).
- The chroma fold is at the cited lines (`W:168-176`, `:612-622`).

The ledger's "why compress?" conclusion is sound. Only a few compressions carry a cited budget reason, and none of those reasons holds at Titan headroom.

## Smallest next actions (PROPOSAL — NOT APPROVED)

1. Freeze EXT-v3 with a direction/trajectory clause (G1) before running §9 Exp. 1. Planted fault: the current instant-attack release.
2. Write TRACEABILITY.md over the 268 worker levers from `CRITIC/recovered/` (G2). Re-verify D01 and D06.
3. Add mode-18 rows and a board-1 lever set, including tempo as a live visual lever (G3).
4. Fold the camera footage into §4.1 as "pre-drop re-entry" and use it in A18's ARMED design (G4).
5. Correct C51 and re-scope Exp. 3 to board 0 only (G6).
6. Move the 3.2 GB replay set to the external scratch drive (G14).

Files written by this critic: this file, and `CRITIC/recovered/` (13 dimension JSON files, the D05 /tmp copy, and `SHA256SUMS`). No repository source, device, serial, USB, flash or commit was touched.
