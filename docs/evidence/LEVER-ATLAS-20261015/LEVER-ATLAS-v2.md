---
abstract: "Definitive Titan lever atlas v2, 15 Oct 2026. Integrates the three v2 syntheses (S1 Opus, S2 Grok, S3 Astra), the v1 atlas, CRITIQUE.md, the 13 dimension reports and the first host-006 camera strip, and settles their disagreements from source at HEAD 01ec2dc. Sections: plain-English summary, pipeline map, compression ledger, failure-to-lever map, one ranked portfolio (24 levers + mandatory safety layer + instruments), target architecture with ship path, Captain taste A/Bs, settled and open contradictions, and the immediate next three experiments. Research only. Every change is PROPOSAL — NOT APPROVED."
id: LEVER-ATLAS-v2
date: 2026-10-15
author: "Integrator v2 (Claude Opus 5.5), read-only on all repositories"
supersedes: "LEVER-ATLAS.md (v1). v1 is kept as history; its settled facts that still hold are carried here with their evidence, and its refuted items are listed in §8.1."
inputs: "SWARM-BRIEF.md; SYNTH-S1-opus-v2.md; SYNTH-S2-grok-v2/SYNTHESIS.md; SYNTH-S3-astra-v2/SYNTHESIS.md; LEVER-ATLAS.md (v1); CRITIQUE.md; CRITIC/recovered/D12-creative-vision.json; INTEGRATOR/settle_checks.json; product source at 01ec2dc; runs/STATUS.md:196-204; P6P/cost-host-006/report.json; P6P/TOLERANCE-FROZEN-P6-EXT-v2.json; Kx-Titan TITAN-CANON-2026-10-13.md:25-55; PL14:147; PRISM-Core music/README.md; RA8P1 manual text (Kx-Titan/_scratch/pdm-profile/hardware-manual.txt)"
images: "Opened and inspected docs/evidence/P6/camera/boom-host006-a-strip-40-70.png (findings in §4.0). This atlas produced no LED output; every appearance question is routed to an instrument (wire snapshot, frozen host gate, camera strip) or to a Captain taste A/B (§7)."
changelog:
  - "2026-10-15 integrator v2: first version. Merged S1/S2/S3 v2 + v1 + CRITIQUE; settled 20 disagreements from source (§8.1); one ranked portfolio by expected value; EXT-v3 direction clause; next three experiments (§9)."
---

# Titan lever atlas v2

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.** It boots with visual output on and runs continuous, autonomous, audio-reactive visualisation whenever it is powered and the application is running. Nothing in this atlas adds a permission gate, a brightness floor, a normaliser that breaks true black, or a rainbow default. Nothing changes the brightness envelope, Titan UID `545433931bd25436593630352d068363`, P601/GPT6 and P604/GPT8, microphone DMA1/2 or LED DMA0/3. Every new behaviour proposed here falls back to the current live mapping (host-006, v9 path P) when its own evidence is missing. Programming a new image darkens the plate for about 41–47 s (PL14-A6); that gap is disclosed, and autonomous visualisation resumes at boot.

**Status.** Research synthesis. The integrator ran no build, replay, flash, device read or commit. It read source and evidence and opened the camera strip. Every proposed change, image and experiment is **PROPOSAL — NOT APPROVED**.

---

## 0. Evidence identity and how to read this

- [FACT] Product worktree `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`, HEAD `01ec2dc66af3e164014e479ab76b46cfe5046655`. Commits after `d557f06` (`045b6b5`, `6c48925`, `01ec2dc`) are tools and camera evidence only; every source line below is valid for `d557f06` (`git log`, `git status` this session).
- [FACT] On the device: host-006, build `e79d1ac0`, programmed 2026-10-14T17:58Z with identity MATCH, 99.9 fps, `led_faults 0`, readback 127 fields / 0 differences against the v9 preset (`runs/STATUS.md:201`). This is the lane's recorded readback; the integrator did not read the silicon.
- Abbreviations (worktree-relative): **W** `src/k1/core/visual/inhouse_waveform.cpp`; **WH** `src/k1/core/visual/inhouse_waveform.h`; **A** `platform/ra8p1/inhouse_audio.cpp`; **PR** `platform/ra8p1/palette_runtime.cpp`; **PRH** `platform/ra8p1/palette_runtime.h`; **G** `src/k1/core/audio/gdft_goertzel.cpp`; **SB** `src/k1/core/audio/sub_band.h`; **TT** `src/k1/core/audio/tempo_tracker.cpp`; **PDM** `platform/ra8p1/pdm_target.c`; **SENS** `platform/ra8p1/k1_pdm_sensitivity.h`; **LR** `platform/ra8p1/k1_live_runtime.h`; **FX** `platform/ra8p1/fixture_app.cpp`; **STATUS** `_scratch/waveform-migration-001/runs/STATUS.md`; **P6P** `_scratch/waveform-migration-001/runs/P6-20261014T124826Z/permitted`; **COST** `P6P/cost-host-006/report.json`; **EXT2** `P6P/TOLERANCE-FROZEN-P6-EXT-v2.json`; **CAM** `docs/evidence/P6/camera/boom-host006-a-strip-40-70.{png,json}`; **MAN** `Kx-Titan/_scratch/pdm-profile/hardware-manual.txt`; **CANON** `Kx-Titan/docs/migration/TITAN-CANON-2026-10-13.md`; **PL14** `Kx-Titan/docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md`; **IC** `LEVER-SWARM-20261015/INTEGRATOR/settle_checks.json`; **CRIT** `LEVER-SWARM-20261015/CRITIQUE.md`.
- Source syntheses: **S1** `SYNTH-S1-opus-v2.md` (TP-01…TP-20, ledger L1–L33); **S2** `SYNTH-S2-grok-v2/SYNTHESIS.md` (S5-01…S5-21); **S3** `SYNTH-S3-astra-v2/SYNTHESIS.md` (L01–L20); **V1** `LEVER-ATLAS.md` (A01–A27, C01–C52). Final ids here are **R01–R24** (ranked), **R00** (mandatory safety layer) and **M0-x** (instruments). §8.3 maps every synthesis id to its final id.
- Labels: [FACT] = the line, commit, measurement or document was read this session or is quoted from a named receipt; [INFERENCE] = arithmetic or a join across facts; [HYPOTHESIS] = unmeasured. Status words: *documented* (comment/receipt), *implemented* (source at HEAD), *demonstrated* (host or device measurement in a receipt), *product-qualified* (look accepted by the Captain on the device). Path P is implemented, default and on the device; it is not yet product-qualified.

---

## 1. Plain-English summary for the Captain

1. **What you see today** (camera, host-006 on BOOM): the light is now multicoloured, but it sits at the two ends of each panel with a darker middle. When the music cuts, the panel goes dark, which is right. Then the riser lights it again about 1.4 s **before** the drop, and when the drop lands nothing sweeps inward. The drop has nothing left to release.
2. **Why the drop cannot hit harder in today's code.** Louder sound pushes new light **towards the edges**, so the loudest moment draws the same edge-hugging picture as the build. The release switch that exists in the code is off. Even when it was switched on in tests, it did not travel from the edges to the centre: it dropped new light at the centre in one frame and let it stream outward. The light can only scroll outward today, so an inward sweep needs new drawing code.
3. **The single most useful next test** fires a proper inward sweep at hand-marked drop times, scored by a test that can tell the direction of travel. That separates "is the look right?" from "can we find the drop in time?".
4. **Processing power is not the problem.** The Titan spends roughly a quarter of its main processor today (an estimate; no idle counter exists yet). Almost every place where musical detail is squeezed was copied from older projects or happened by accident. Only two squeezes had a recorded budget reason on the ESP32, and neither applies on the Titan.
5. **The colour fix is live** on your Titan and the camera confirms many colours. What remains of colour is taste (how much of the palette one half spans, whether related keys get related colours) plus two accuracy fixes (notes are tuned up to a semitone off, and the top fifth of the spectrum never votes for colour).
6. **Three upstream reasons the drop feels flat:** a level tracker that cancels a loudness jump within about 30 ms; a "hit" signal that arrives about a third of a second late because it is computed after heavy smoothing; and possibly a microphone bass-cut filter left at a setting for a slower rate (worked from the chip manual: about −8 dB at 110 Hz, −14 dB at 55 Hz; not yet measured on the board).
7. **Ready-made advantages already on the board or in the code:** a second microphone that is captured and thrown away (a free A/B instrument); a 40–110 Hz bass detector already committed but unused; a colour-speed control that can be changed live per board with no reflash; a host test probe that can already switch the release on; and a clean-room music engine in PRISM-Core with the bass filter, window and onset timing this atlas recommends.
8. **What two older worries turned out to be:** the "32-bit overflow that deletes loud bass" is in a parked analyser the product does not run (the live one uses 64-bit maths with about 155× headroom). And removing the 88 BPM tempo preference alone does not fix tempo (29 of 326 test cases pass).
9. **The upper board** follows a tempo tracker that locks to half speed. A "groove flywheel" that needs no tempo lock can fix the upper board without waiting for the tempo work.
10. **Safety comes with the drop work:** any explosive release ships with a white-limit, a flash-rate limit and exact black in silence. Never a brightness floor.
11. **Your eye decides only taste.** The short side-by-side clips are in §7. One of them (colour speed on the lower plate) needs no reflash and is part of the next three experiments (§9).
12. **Nothing here is shipped.** The ship path is in §6.5.

---

## 2. Pipeline map: AP → mapping → VP → output

Signal order. Line numbers are at `01ec2dc`. In the v9 preset board 0 (lower) runs mode 32 (Waveform Hybrid K1, palette id 31) and board 1 (upper) runs mode 18 (Waveform Tempo, Crimson Gold id 38) (`docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json`; CANON:51). Levers point at §5 (R-ids) and §3 (ledger L-ids).

### 2.1 AP container — microphone to features

| # | Function (file:line) | What it does now | Ledger | Levers |
|---|---|---|---|---|
| A1 | Two LMD2718T261 capsules; U14 is the programme lane, U13 is captured and discarded (PDM:1-4, :607-610) | One capsule feeds the show | L4 | M0-6 witness lane |
| A2 | PDM hardware chain: sinc (SINCDEC/SINCRNG retuned, PDM:764-781) → HPF → compensation → half-band LPF ÷2. HPF coefficients `s0 0x3F61`, `k1 0x3EC1`, shift 0, from the pinned BSP copied into the build stage (`scripts/build_scalar.py:816-820`; staged `ra_gen/hal_data.c:613-619` in all 9 staged evidence copies and in `_scratch/waveform-migration-001/arm-p6-v9-host-006/stage/`). PDM has no HPF override | HPF runs on the sinc result at 2·Fout ≈ 82.8 kHz (MAN §50.3.4.3, §50.3.4.5); reset coefficients are the manual's "cut-off = 100 Hz" design for a 32 kHz filter clock | L1 | R04 |
| A3 | PCM width 16-bit [14:0] | 4 bits of a 20-bit option unused | L2 | R17 |
| A4 | ×16 gain + per-296-sample-slot loud guard, ceiling 30,000 (SENS:9-33; PDM:614-636) | A slot with raw peak > 1,875 has its gain cut to land at 30,000 | L3 | R17 |
| A5 | Slot 296 samples ≈ 7.15 ms; PDM clock MOCO-only, measured 41,417 Hz (PL14:147) | Capture-fill latency; resampler must track the rate | L33 | R20 |
| A6 | Donor ASRC → 12.8 kHz (feeds donor modes and the admission gate) | Admission sees ≤ 6.4 kHz content | L6 | M0-3 |
| A7 | `k1_poly24` → 24 kHz, 180-sample (7.5 ms) hop; host-006 receipt `inhouse_analysis24: true` | In-house analysis rate | — | — |
| A8 | Hop RMS / VU, Q16 square per sample (A:228-246) | Samples with \|s\| < 128 add zero; error > 2 % only below −38 dBFS (IC `rms_truncation`) | L5 | R24 |
| A9 | Sub-band 40–110 Hz, two RBJ band-passes → `sub_rms` (SB; A:239-240) | Committed (`1416cb5`); read only by the disabled release | L11 | R10 |
| A10 | Peak follower on `raw_peak_pcm − drive_floor`: attack 0.25 (≈ 30 ms), release 0.005 (≈ 2 s) (A:250); `peak_scaled` ratio smoothed 0.65/0.15 (A:253) | Ratio to "recent loudest", not level | L17 | R09 |
| A11 | Silence: RMS enter 0.04 / exit 0.08, 5 s dwell (A:256-266) | True-black gate | L32 | R15 |
| A12 | Note bank: 80 bins from offset 12 (bin 0 resonates 104.69 Hz), rounded integer k, Cross40 (G:19, :116), cap 2,000 (G:20), `>> 6` (G:55); live scalar kernel A:36-70 | int64 product and magnitude; int32 state with every overflow counted and the hop invalidated (A:37-42, :66-68, :308) | L7–L10, L12 | R06 |
| A13 | Magnitude EMA 0.3/0.7, τ ≈ 21 ms (A:274) | Smooths every consumer | L13 | R05 |
| A14 | Per-board Mood LPF (cutoff 1+10·mood Hz, coefficient at a 100 Hz basis, run per 7.5 ms hop; A:103-111, :318-326); mood 0.05 → τ ≈ 80 ms; mood 1.0 → τ ≈ 10.9 ms | Colour and novelty smoothing; live-settable per board (LR:87-90; FX:1546, :1666) | L14 | R05, R07 |
| A15 | Broadband spectral AGC, target 0.25, gain 0.1–10, τ ≈ 190 ms, gated (A:329-340) | One scalar for all bins | L15 | R09 |
| A16 | Tilt, soft knee > 0.5, clamp 1 (A:343-358) | Compresses the top half | L16 | R05 |
| A17 | `linear_shape[80]` pre-gain/pre-knee, post-Mood (A:346-350) | Path-P colour input | — | R07 |
| A18 | Novelty = √(mean positive difference) of the knee'd spectrum (A:357-367) | One scalar; BOOM peak ≈ 330 ms late; ±30 ms max at the drop 0.066 < build p95 0.069 | L18 | R05 |
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
| M8 | Transport (W:963-1018) | 405·motion px/s (W:967), ≤ 8 steps/frame (W:968), integer steps; **outward only** (W:1003-1015); source 0..79 zeroed every frame (W:973) | `w·tempo_rate + (1−w)·energy_rate`, w ≤ 0.7 from confidence 0.30–0.60; tempo_rate = 24·BPM/60·(1+0.5 cos 2πφ) (W:988); energy_rate = 30 + 90·clip(novelty/0.12) (W:991); ≤ 30 steps (W:993) | R11, R14 |
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
| O4 | Tick 10,000 µs under `K1_EMOTISCOPE_PORT` (PRH:40; 8,333 µs is the other branch, PRH:47); 100 Hz measured | random 0–10 ms wait; beats against 133 Hz hops | R18 |
| O5 | Wire: 80 px per DIN, two DINs per board in lockstep, 3,840 bits × 1.25 µs + 300 µs = 5.1 ms. Board 0 on P601/GPT6 + P604/GPT8 (LED DMA0/3); board 1 on its own top engine (host-006 receipt `titan_top_output: true`) | | — |
| O6 | Wire snapshot instrument (`scripts/palette_wire_snapshot.py`; packed lanes at FX:640-720) | Instrument | M0-1 |

### 2.5 Measured cost on the device (host-006, BOOM, 16,196 records, COST)

[FACT] Per record (5 ms donor hop): `ap_cycles` p50 983,358 / p95 1,252,397; in-house p50 699,201 / p95 964,658; donor p50 284,933; ASRC p50 133,484. Per record: `vp_cycles` p50 269,903 / p95 354,332; `emit_cycles_last` p50 127,960. `emit_errors` 0, `asrc_discarded` 0, 16,207 AP hops, 8,104 frames emitted. [FACT, S1 I8 citing STATUS.md:110] VP was 182.1k cycles on `b5595c35`; path P raised it about 48 %. [INFERENCE] At 1 GHz the AP is about 20 % of each 5 ms, VP plus emit about 4–8 %, so roughly 25–30 % of the M85 excluding RT-Thread, USB-CDC and ISRs. No idle counter exists; this is an estimate (M0-4).

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
| L14 | Mood LPF τ ≈ 80 ms at mood 0.05 (A14) | spectral motion above ≈ 2 Hz; colour lags ≈ 75–110 ms; novelty peaks 37–83 ms late (model) | aesthetic (live control) + accident (100 Hz basis at 133 Hz) | yes for timing; taste for display | 0 (move to consumers) | H (timing) | R05, R07 |
| L15 | Broadband spectral AGC (A15) | inter-section loudness and band balance | inherited K1 AGC v2; **K1 product now runs per-band ON** (`platformio.ini:187`, `-DK1_AGC_PERBAND_V1=1`, K1 `68a95773`) | yes | trivial | H | R09 |
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
| L28 | Outward-only integer transport, ≤ 8 steps/frame (M8, V1) | sub-pixel motion; any inward front; fronts faster than 800 px/s | inherited source-fidelity arithmetic; the 8 cap is K1 `WFHYB_MAX_SCROLL_STEP`; budget unproven | yes | ≈ 2k cycles | H (inward front impossible today) | R01, R14 |
| L29 | Max-hold deposit (V2) | energy of repeated hits | calibration (K1 "never add palette colours") | partly (keep for colour) | low | L | R14 |
| L30 | Path-P spatial ramp d/80 (V4) | adds audio-independent colour; palette 31 has 17.5 % of positions below 5 % luma and a black→yellow seam | transplant of K1 `freq_prog` (pixel = frequency) into a waveform (pixel = time) | n/a (taste) | trivial | M–H | R08 |
| L31 | Display warmth / white balance after the metric tap (O1) | authored hue (board-0 warmth 0.59: G ×0.67, B ×0.50) | inherited Emotiscope display | n/a | trivial | M | M0-1 |
| L32 | Gates without attribution (A11, A21, freshness, quiet) | four or more paths darken a frame silently; silence entry RMS 0.04 (≈ −28 dBFS) may drain quiet music held 5 s [INFERENCE] | inherited | gates stay | reason word | M | R15 |
| L33 | 100 Hz tick, 7.15 ms capture slot (O4, A5) | 0–10 ms random wait; ≈ 7 ms capture fill | inherited "reconstructed presentation profile"; CANON:35 rules hop-triggered render and ≈ 1.6 ms blocks | yes | scheduler change | M (latency) | R18, R20 |
| L34 | Linear code→light (O3) | unknown; possibly crushed dark ends | accident (K1 degamma `38428a82` not ported) | measure first | LUT ≈ 1–2 % render | M? | R19 |
| L35 | Mirror (V4) | 50 % of spatial freedom | aesthetic (centre-origin identity) | n/a | — | keep | — |
| L36 | Brightness cap / gains (O2) | photon headroom only; code resolution kept (6,169 levels at 24/255) | owner rule (binding; conflicted, §8.2) | n/a | ruling only | — | — |
| L37 | Mode-11 180→96 nearest-sample display (A:216-225) | displayed waveform drops samples (mode 11 only) | geometry inheritance | yes | antialiased adapter | L | parked |
| L38 | Phase discarded (magnitude only) | onset/timbre cues | inherited | yes | covered by R05 | M | R05 |

Already undone on the Titan (keep and guard): TRUE16 to the wire; HD stop sampler; per-pixel colour (path P, on device); pre-knee colour shape; int64 product and magnitude in the live analyser; `output_precision` on in compositions; led-first DMA enforced for every port build (`build_scalar.py:626-627`); in-house analysis at 24 kHz.

**Answer to "why compress?"** [INFERENCE from the ledger and CANON:41] Of 38 rows, two carry a cited ESP32 budget reason (L10 Cross40, L19 decimation), none of which binds at about 1 ms of AP per 5 ms. Six are aesthetic or calibration choices; the rest are inheritance or accident, kept because each port froze them for host bit-parity. The Titan canon now rules that bit-exactness is never a product requirement, only tolerance parity against a lineage oracle (CANON:41). Two deliberate looks must not be undone blindly: the symmetric spectrogram smoother (a slow-release variant was rolled back on 2026-05-20 for light-guide washout, SB9 `d408433`) and the Captain-calibrated 405 px/s scroll. The useful split is not "budget vs not" but **which consumer needs which view**: smoothing, normalising and folding are right for display and colour, and wrong for timing and dynamics. Today they are applied once, upstream, for every consumer.

---

## 4. Failure → lever map

### 4.0 What the device camera shows (host-006, BOOM 40–70 s)

Opened this session: CAM (`boom-host006-a.mp4`, panel rows 107–220 of 360, offset 5.733 s, alignment r = 0.579, 2,641 frames; recorder is 30 fps video-only).

- [FACT, seen] Multicolour on both halves: crimson, magenta, pink, gold, cream, varying along space and time.
- [FACT, seen] Bright bands sit at the ends of each half through the build, with a darker middle.
- [FACT, seen] The cut (≈ 49.7–54 s) shows no LED structure (uniform grey field).
- [FACT, seen] Light returns just after the 54 s guide (lane reading ≈ 54.3 s, STATUS:203), about 1.4 s before the 55.7 s drop.
- [FACT, seen] At 55.7 s there is no sweep from the ends toward the centre in either half. After about 57 s the lower half shows noticeable pink fill across its middle (x ≈ 0.6–0.9) while the upper half keeps a darker middle; this is fill arriving after the drop, not a release front (agrees with S2's spot-check).
- [FACT, seen] Full-width dark lines at about 57.4, 59.3, 64.5 and 66 s.
- [INFERENCE] The grey cut is camera noise or auto-exposure lift (no fixed-exposure capture exists, STATUS:117). Camera **geometry, direction and timing** are usable; camera **luma ratios** are not yet. r = 0.58 makes the 1.4 s gap approximate.
- [HYPOTHESIS] Which camera half is which board is not established (palette ids 31 vs 38 could identify them). The dark lines may be musical stops, a gate closing during music, or camera artefacts: correlate with a wire snapshot before acting. Cream regions may be camera clipping of gold stops rather than near-white output.
- [INFERENCE, S3 Q3] The early return is probably a legitimate response to the riser; the defect is choreography (nothing holds contraction for the drop), not a false detector.

New failure **F-CAM1, pre-drop re-entry**: the riser re-lights the plate before the drop, so the drop has no dark-to-light or contracted-to-extended contrast left to spend.

### 4.1 Titan failures

| ID | Failure (evidence) | Stage | Levers | Status |
|---|---|---|---|---|
| T-DROP | "Drop does not hit harder than the build" (Captain 14 Oct; v9 still "a no") | M5/M6/A10/A15/A18/M3 | R01, R03, R02, R09, R05, R07, R04, R17 | open |
| F-CAM1 | Light re-enters ≈ 1.4 s before the drop; no inward release (CAM) | A11/M5/M6 | R03, R01 | open (new) |
| T-EXT | Waveform hugs the edges; centre dark (P6 RECEIPT; CAM) | M5 polarity; mode 18 reaches 159 | R01, R09, R11 | open |
| T-REL | Release candidates X1–X3/K1–K3 reached EXT-v2 extent 1.0 within 40–70 ms when they fired, but by a centre teleport + outward stream, and fired on risers/climaxes or missed Sonic @54.7 (`P6P/results/ext/ext-v2-X{1,3}.json`; STATUS:176-177) | M6/M8 | R01 (geometry), R05/R13 (timing) | open; EXT-v2 cannot see direction |
| T-DET | Drop detectors v1 1/5, v2 1/6, v3 0/6 within +50 ms; sub arrives 72–117 ms after the musical drop | A9/A12/A18/A19 | R05, R03 (arming), R13 | open |
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
| T-LAT | ACOUSTIC_TO_PHOTON target p50 ≤ 8 ms / p99 ≤ 12 ms / jitter ≤ 4 ms ruled (CANON:31-32); 30 fps camera cannot measure it | A5/O4 | R18, R20 | unmeasured |
| T-STALL | poly24 filter (re)design can stall a frame 3–6 ms; poll high-water 8.67 ms (PL14-D8); host-006 `poll_max_cycles` 809,216 after the run (COST) | A7 | R20 | open |

### 4.2 S3 / upstream failures, and whether they still live on the Titan

| ID | Failure | Fixed on S3? | Still on the Titan? |
|---|---|---|---|
| K01 | 12 repeating hues (`note_colors[i%12]`) | yes | no (palettes) — never return to hue wheels |
| K03 | Flat yellow / all white from summed palette RGB | yes | no (max-hold, path P) |
| K04 | 16-slot uint8 palette | yes (SB `d408433`, K1 `c2738b88`) | no |
| K05 | Palettes duller when louder (post-gate chroma freeze, K1 `ae5d90a`) | yes | partly: novelty and tempo still read post-squash |
| K07 | Louder → dimmer (broadband AGC) | yes (per-band ON) | **yes** (L15) |
| K08 | "30–40 % dimmer, sluggish, very short leash" (0.25/0.005 follower) | partly | **yes**, same constants (A:250) |
| K11/K12 | Twitchy motion from over-fast denominators | yes | watch in R02 |
| K13 | Strobe law: never global brightness on the beat (#58407) | rule | must be encoded in R00 |
| K14 | Half-time lock (Beauty 74 vs 128) | no | **yes** (T-TEMPO) |
| K15 | 8-bit dither flicker at dim tails | yes (WS2816 16-bit) | no; dither is the wrong repair on TRUE16 |
| K16 | Harness PASS while the device was dark; rainbow test palette | process | pattern persists |
| SB9 | Slow spectral release → milky LGP washout | rolled back | constraint on every release lever |
| K-overflow | int32 Goertzel zeroed loud bins | yes (int64) | no on the live path; trap remains in the parked kernel |

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
| P10 Built but switched off treated as absent | — | release/accent; `sub_rms`; AudioPipeline kick/snare/hat (compiled out, `k1_live_runtime.cpp:165, 318`); second mic; 20-bit PDM; sound-detection IRQ; U55; M33; PRISM-Core clean-room music runtime |
| P11 The wrong code path gets the blame | — | the parked kernel's int32 wrap was attributed to the live analyser (D03, S2) |
| P12 A protection becomes the look | — | loud guard, follower, clamps and gates now shape the show |
| P13 Inherited constants re-frozen for bit-parity | int64 fix kept on K1 | rounded k, `>> 6`, Mood 100 Hz basis, integer scroll — CANON:41 now permits tolerance parity |

---

## 5. Ranked opportunity portfolio — PROPOSAL — NOT APPROVED

**One list.** I = visual impact, N = novelty (added product value relative to today's Titan), E = effort **to the smallest decisive result**, each 1–5; P = probability the lever produces a visible improvement if executed [HYPOTHESIS]. **Rank score = I × N × P ÷ E** (expected value; settles the formula disagreement, §8.1 D6). I×N÷E is shown for comparison. Ties break on relevance to the open verdict "the drop does not hit harder", then lower effort. Scores are judgements, not measurements.

**Rules for every experiment.**
- Freeze the criterion with `scripts/freeze_tolerance.py freeze` before any result exists; `verify` after.
- A metric is admissible only after it goes **red on the known negatives** (host-006 replay and `b5595c35`) and **green on a planted positive** that has the property the criterion claims.
- Host replays of drop levers run twice: once as today, once through an optional HPF + loud-guard pre-stage (M0-5), so host input matches the device chain (P4).
- Camera clauses score geometry, direction and timing only; luma clauses use packed 16-bit wire snapshots until a fixed-exposure capture exists. A recording is admissible only at alignment r ≥ 0.7 or with a declared sync marker (host-006-a has r = 0.58).
- Every run includes a real silence segment whose packed codes must be exactly 0. No rainbow. No floor. Dopex stays out of tuning. Carte Blanche is spent.
- Device steps (build, program, readback, record) are performed by the P6 lane owner under existing authority. Nothing here grants it.

### 5.0 Mandatory, unranked: R00 output constraint layer

Before the packer, three bounded rules: zero full-scale-white pixels and lit energy with HSV saturation < 0.25 ≤ 10 % per song; ≤ 3 large luminance flashes per second (change > 25 % of the darker state); all channels exactly 0 within the authorised drain after zero PCM. Never a floor. **Frozen:** those three clauses. **Planted:** an 8 Hz strobe flagged; a white stop flagged; a 1/1024 floor fails true black. Required before any R01/R03/R09 image reaches the device (K13, T-WHITE, CRIT G11).

### 5.1 Ranked table

| Rank | ID | Lever | I | N | E | P | I×N÷E | **Score** | Drop? | From |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | R01 | Inward release geometry on labelled drops (analytic edge→centre front, swept, hold 1–2 s), scored by EXT-v3 | 5 | 4 | 2 | 0.7 | 10.0 | **7.0** | yes | S1 TP-01, S3 L01, S2 S5-06/S5-14, V1 A02, D12 C02 |
| 2 | R02 | Extent instant attack (asymmetric; release unchanged) | 4 | 2 | 1 | 0.8 | 8.0 | **6.4** | yes | S1 TP-05, S2 S5-15, V1 A03 |
| 3 | R03 | Hold the cut contracted through the riser until the drop (ARMED; riser drives velocity, not extent) | 5 | 4 | 2 | 0.6 | 10.0 | **6.0** | yes | S1 TP-02, D12 C01 Spring + C17 RiserAccelerando, S3 L04/L16, CRIT G4 |
| 4 | R04 | Re-derive the PDM bass-cut (HPF) for the real rate: host replay first, then witness-lane sweep | 3* | 4 | 1 | 0.5 | 12.0 | **6.0** | yes | S1 TP-06, S2 S5-01, V1 A01, S3 (conditional) |
| 5 | R06 | Colour tuning truth: exact bin frequencies, `chroma_range` 80 | 3 | 3 | 1 | 0.6 | 9.0 | **5.4** | — | S1 TP-13, S2 S5-11, S3 L19, V1 A12 |
| 6 | R05 | Fast event path: pre-Mood/pre-AGC per-band log flux, timestamped onset events, bounded queue read max-since-cursor | 5 | 3 | 2 | 0.7 | 7.5 | **5.25** | yes | S1 TP-04, S2 S5-16/S5-17, S3 L02+L03, V1 A07+A25 |
| 7 | R07 | Colour chapter carried by the release (snap offset to the pre-Mood centroid; drop luma ≥ build) | 4 | 4 | 2 | 0.6 | 8.0 | **4.8** | yes | S1 TP-07, S3 L12, V1 A10, D12 C15 |
| 8 | R08 | Path-P dark ends + spread k driven by harmonic tension (k ∈ [0.15, 1], never one colour) | 4 | 4 | 2 | 0.6 | 8.0 | **4.8** | — | S1 TP-08, S2 S5-03/S5-07, S3 L17, V1 A14, D12 C05 |
| 9 | R09 | Section-relative dynamics + luminance iris (8–30 s reference; per-band AGC as on the S3; headroom for the drop) | 5 | 3 | 2 | 0.6 | 7.5 | **4.5** | yes | S1 TP-03, S2 S5-08/S5-12, S3 L08, V1 A06/A11, D12 C12 |
| 10 | R10 | Timbre width + sub push (flatness → deposit width; sub envelope displaces light, never brightness) | 4 | 4 | 2 | 0.5 | 8.0 | **4.0** | — | S1 TP-12/TP-10, S2 S5-13, S3 L10/L11, V1 A09/A16, D12 C07/C18 |
| 11 | R11 | Groove flywheel for board 1 (onsets kick scroll velocity; drag 0.4–0.8 s; no tempo lock) | 4 | 3 | 2 | 0.6 | 6.0 | **3.6** | — | S1 TP-09, D12 C06, CRIT G3 |
| 12 | R12 | Push-pull: kicks pull inward, snares bloom outward (needs R05) | 4 | 4 | 2 | 0.45 | 8.0 | **3.6** | — | S1 TP-11, D12 C19 |
| 13 | R13 | Tension state machine for causal release (FLOW→BUILD→ARMED→PROVISIONAL→RELEASE→RECOVER; QUIET); predictions arm but never trigger | 5 | 5 | 4 | 0.55 | 6.25 | **3.4** | yes | S3 L04, V1 A18, D11 §3 |
| 14 | R14 | Fractional transport + swept deposit in palette-position space | 4 | 3 | 2 | 0.55 | 6.0 | **3.3** | — | S1 TP-15, S2 S5-18, S3 L05, V1 A20 |
| 15 | R15 | Darkness truth: gate-reason word per frame; test the −28 dBFS silence entry against quiet music | 4 | 3 | 2 | 0.55 | 6.0 | **3.3** | — | V1 A13, S3 L18, S1 M0 |
| 16 | R16 | Circle-of-fifths colour order (taste) | 3 | 4 | 1 | 0.25 | 12.0 | **3.0** | — | S1 TP-14, S2 S5-04, S3 L09, V1 A05, D12 C04 |
| 17 | R17 | Input headroom: export per-slot guard engagement, then 20-bit PCM / pre-guard level side channel | 4 | 3 | 2 | 0.5 | 6.0 | **3.0** | yes | S1 TP-16, S2 S5-10, S3 L14, V1 A08 |
| 18 | R18 | Hop-locked render with one present time for both boards (CANON:35 ruled) | 3 | 2 | 2 | 0.8 | 3.0 | **2.4** | — | S1 TP-19, V1 A26, CRIT G8 |
| 19 | R22 | Two-board roles at the drop / register split (after a one-board camera overlap capture) | 4 | 3 | 2 | 0.35 | 6.0 | **2.1** | — | S1 TP-18, S3 L06, V1 A17, D12 C08/C09 |
| 20 | R19 | Measure the WS2816 transfer on this plate, then a curve for in-house modes only | 4 | 3 | 3 | 0.5 | 4.0 | **2.0** | — | S1 TP-17, S3 L13, V1 A21 |
| 21 | R21 | Treble time coverage (short windows several times per hop, peak-held) | 2 | 4 | 2 | 0.5 | 4.0 | **2.0** | — | V1 A23 |
| 22 | R20 | Shorter capture blocks (≈ 64–96 samples) + precomputed poly24 filter tables (CANON:35; T-STALL) | 3 | 2 | 3 | 0.6 | 2.0 | **1.2** | — | S3 L15, D10 HW-19, D13 L21 |
| 23 | R23 | Tempo metrical repair (octave candidates, phase extrapolation, silence confidence) — P7 lane | 4 | 3 | 4 | 0.4 | 3.0 | **1.2** | enabler | S2 S5-20, S3 L07, V1 A24 |
| 24 | R24 | Arithmetic hygiene: float RMS, drop `>> 6` with re-frozen gains | 1 | 2 | 1 | 0.5 | 2.0 | **1.0** | — | V1 A27, S3 L20 |

\* R04 impact is capped at 3 until a host replay shows its effect on extent or EXT (follower and AGC sit after it and may cancel much of it; CRIT G7). Its E = 1 is the host replay; the device change is E = 2 and needs a flash.

The Rank column is the order; R-ids are stable labels and do not always equal the rank (R05/R06 and R19–R22 are swapped by score). R18 and R20 score low only on visual impact; both implement architecture the Titan canon already rules (CANON:35), so their low rank is not a reason to drop them. R23 sits in the P7 lane; R11 removes board 1's dependency on it meanwhile.

**Instruments (M0, enable everything; no visible change):** M0-1 packer-tap scoring (luma + hue after warmth/WB, compared with the intended transformed palette); M0-2 EXT-v3 direction clause; M0-3 gate-reason telemetry word; M0-4 idle counter + per-stage worst-case counters; M0-5 host HPF + loud-guard pre-stage; M0-6 second-mic witness lane; M0-7 product-equivalence test extended to Mood, release, cap and ceiling, plus the DIN symmetry byte test; M0-8 runtime A/B knobs (release, cap, ceiling, k) in the live schema with defaults hash-bound to host-006, so taste A/Bs need no reflash; M0-9 move the 3.2 GB replay set `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6` to `/Volumes/Disk Storage/SpectraSynq-Scratch` (volatile today; storage policy).

**Parked, with reason:** Cross40 removal (+59 % MAC, 83 ms bass window; use R05 for timing); lowering the bank below 110 Hz (≈ 150 ms window per 55 Hz bin); lengthening the spectral release (SB9 washout rollback); donor VU and 96-tempo bank in in-house modes (GPL-lineage BENCH dependency; licence decision first; the ×3.93 figure is quiet-cut → drop, not build → drop); U55 model (no measured need; BeatNet streaming documents 84 ms delay); predictive downbeat release (blocked on R23; CANON:43 forbids predicted beats to pass a reactive gate); AudioPipeline kick/snare/hat (compiled out under the port; R05 first); int64 resonator (already int64, §8.1 D1); 2,000-sample cap (binds on no bin); PDMIFCLK crystal source (MOCO-only, PL14:147).

### 5.2 Drop bundle and multipliers

[INFERENCE] The drop failure is a conjunction. Release geometry with direction (R01), held contrast through the riser (R03), on-time attack (R02), a section-scale reference with headroom (R09), colour that arrives with the release (R07), and input truth (R04/R17) may each be necessary and none sufficient. Interaction matrix (✚ multiplies, ⟂ independent, ⚠ conflicts):

| | R01 | R02 | R03 | R04 | R05 | R07 | R09 |
|---|---|---|---|---|---|---|---|
| R01 release front | — | ✚ deeper contraction to release | ✚ contrast reserved | ⟂ | ✚ causal trigger replaces labels | ✚ front carries colour | ✚ iris opens on release |
| R02 instant attack | | — | ⚠ must not re-extend during ARMED | ⟂ | ✚ | ⟂ | ⚠ twitch risk if reference is fast |
| R03 riser hold | | | — | ⟂ | ✚ arming evidence | ⟂ | ✚ |
| R04 HPF | | | | — | ✚ more sub energy in flux | ⟂ | ⚠ follower/AGC may cancel |
| R05 fast events | | | | | — | ✚ | ⟂ |

**Bundle experiment (PROPOSAL — NOT APPROVED).** One host build with R01 + R02 + R03 + R07 + R09 + R00 selectable, driven first by labelled drop times (no detector), scored by EXT-v3 (§9 Exp 1). Then an ablation: remove one lever at a time; a lever whose removal turns the bundle red is necessary; the rest are contributors. Only after the geometry passes on labels does R05 (then R13) replace the labels.

### 5.3 Lever records

Fields per the brief. "Current" is v9 (host-006) unless v8 differs. All experiments are PROPOSAL — NOT APPROVED.

**R01 — Inward release geometry on labelled drops** (5/4/2, P 0.7 → 7.0)
- Container: map + VP. Stage: release `updateReleaseV9` (W:752-795), pull (W:977, :999), transport (W:1003-1015). Controls: space and motion at the drop.
- Current: release off and `inline constexpr` (WH:68-79). When forced on (host probe `tests/host/p6_effect_probe.cpp:138-146`), it teleports the deposit to pixel 80 in one frame; the transport only scrolls outward, so the visible motion is light born at the centre streaming outward. v8 has no release.
- Information: today loudness and "this is the drop" are the same number, so build and drop draw nearly the same length (L17, L26). No inward front can exist (L28).
- Origin: outward map = K1 centre-origin look (aesthetic); release = P6 candidate left off because its triggers failed. Still needed: yes if the Captain's 14 Oct definition is the spec (EXT2 `captain_definition_verbatim`). Cost: an analytic front is ≈ 200 operations per frame [INFERENCE, D12 C02].
- Manipulation: an analytic front composited over the field, `e(t) = e₀ + (0.98 − e₀)[1 − (1 − u)³]`, `u = (t − t_r)/T_r`, T_r 100–180 ms, extent measured edge→centre, the trail swept over the interval in palette-position space, hold 1–2 s, then return to FLOW. Front speed and drag encode release size. Direction is the Captain's verbatim (edges → centre); the outward variant stays available for Q2 (§7).
- Interactions: R02 deepens the build contraction; R03 reserves contrast; R07 carries colour; R09 opens the iris; R14 smooths the front; R00 bounds it; R05/R13 later find the time without labels.
- Risk: reads as a gimmick if it fires every chorus (only after R13); LGP washout if the hold is long; flash-rate risk (R00).
- Smallest decisive experiment: §9 Exp 1 (EXT-v3 frozen with a direction clause; teleport and outward fronts planted red; synthetic inward front planted green).

**R02 — Extent instant attack** (4/2/1, P 0.8 → 6.4)
- Stage: W:974-975 (`ema(…, 0.016F)`, `ema(…, 0.023F)`). Controls: how fast the waveform's length jumps on a hit.
- Current: t50 ≈ 30–33 ms, t90 ≈ 76–80 ms at 100 Hz (D13 fresh verdict; D08 VERIFY 10.23/32.50/76.22 ms). Mode 18 has no EMA.
- Origin: [INFERENCE] anti-flicker; not a budget cut. Still needed: no for CPU; taste for twitch.
- Manipulation: attack ≤ 1 frame through the existing `follow()`, release unchanged (not the SB9 slow release). Optionally cap 0.7 → 1.0 **only together with R01** (with outward polarity a higher cap moves loud deposits further out and shortens the lit extent; V1 D6).
- Interactions: R01, R03 (must not re-extend while ARMED), R09.
- Risk: twitch (K11/K12, rejected Melodic Bloom look).
- Experiment: frozen: extent ≥ 0.9 of final within 15 ms of a synthetic step; sustained-noise per-frame extent RMS change ≤ 1.5 × today. Planted: the symmetric 16/23 ms cascade fails the 15 ms clause. Then Captain Q9.

**R03 — Hold the cut contracted through the riser** (5/4/2, P 0.6 → 6.0)
- Stage: A11 + M5 + a new ARMED state (part of the R13 machine, first driven by labels). Controls: the dark-to-light and contracted-to-extended contrast the drop spends.
- Current: after the cut the riser re-lights the plate ≈ 1.4 s early (F-CAM1); the follower has decayed during the cut (release ≈ 2 s, A:250), so returning sound pins `peak_scaled` at the clamp and parks deposits at the edge.
- Origin: no design for it existed. Cost negligible.
- Manipulation: after a cut (fast envelope fall ≥ X dB within Y ms), keep new deposits contracted to the outer 25 % of each half and capped, and let the riser drive scroll **velocity** (D12 C17 RiserAccelerando) instead of extent, until the drop (labelled first). Timeout ≤ 8 s returns to FLOW. Silence stays exactly zero.
- Risk: songs with a riser and no drop look withheld (timeout handles it); must never black out the riser entirely.
- Experiment: frozen (wire or host packed frames): mean packed luma in [cut end, t_d − 0.1 s] ≤ 0.3 × mean luma in [t_d, t_d + 1 s] on every labelled cut-drop; the riser still produces motion (≥ 1 lit pixel moving per 100 ms); lit region within the outer 25 % of each half until t_d. Planted: host-006 behaviour red; a held state that blacks out the riser fails the motion clause. Camera: riser geometry contracted until t_d; host-006-a red.

**R04 — Re-derive the PDM bass-cut** (3*/4/1, P 0.5 → 6.0)
- Stage: PDM HPF registers per channel (staged `hal_data.c:613-619`, from the pinned BSP via `build_scalar.py:816-820`; PDM has no override). Controls: kick and sub energy for everything downstream (`sub_rms`, follower, release surge, flux).
- Current (v8 = v9): s0 0x3F61, k1 0x3EC1, h {0x4000, 0xC000}, shift 0. [INFERENCE, IC] 259 Hz corner (L1). The 128 Hz alternative assumes the HPF runs at Fout, which MAN §50.3.4.3/§50.3.4.5 contradict (HPF on the sinc result; the LPF halves afterwards).
- Origin: accident. Still needed: a DC block yes, a 259 Hz corner no. Cost: 0 CPU; a flash.
- Manipulation: k1 0x3FE7 / s0 0x3FF4 (20.1 Hz). Apply to the witness lane first, then the programme lane. Model the compensation and half-band filters too before claiming flat response. Kx-Titan `native-pdm-profile.json` already lists this qualification as remaining.
- Interactions: R10 (sub push), R05 (flux), R17 (more low-frequency energy raises guard engagement), capsule/room roll-off.
- Risk: capture path is the most fragile (PL14-A4); a wrong coefficient adds DC or rumble. Reversible by one constant.
- Experiment: §9 Exp 2 (host replay measuring the light), then device: lane B at 20 Hz vs lane A unchanged, log sweep 30–1,000 Hz; frozen B/A ≥ +6 dB at 45–65 Hz and ±1.5 dB at 800–1,000 Hz; planted lane B at fc ≈ 1 kHz must read ≤ −6 dB at 45–65 Hz. No Captain look needed.

**R06 — Colour tuning truth** (3/3/1, P 0.6 → 5.4)
- Stage: G:121-128 (rounded k), WH:126 (`chroma_range = 60`). Controls: which pitch class (hence palette position) each note votes for; whether hats and cymbals move colour.
- Current: bin 0 resonates at 104.69 Hz; low half ≈ 80 cents flat, high half mostly 20–40 cents sharp, so registers disagree by up to a semitone; bins above ≈ 3.5 kHz never vote.
- Origin: inherited; K1's eyes-on rejection of exact centres (`44c3e7a`) may have been overflow-contaminated (`7cd9336`); the live Titan path is int64, so re-test there. Cost: 0 cycles (setup).
- Manipulation: exact-frequency coefficients; range 80; keep the legacy configuration as a named parity oracle (CANON:41 tolerance parity). Optional Hann with coherent-gain correction is a separate arm.
- Risk: colours move; amplitude calibration shifts; re-freeze gains.
- Experiment: frozen: ≥ 95 % of frames assign each pure tone 110 Hz–3.3 kHz to its own pitch class (baseline ≈ 50 %); a 110 Hz tone peaks in bin 0 with the resonator within 0.5 Hz; an 8 kHz tone moves the offset ≥ 0.15 turns at range 80; noise-only drift ≤ 0.02. Planted: integer rounding; range 60.

**R05 — Fast event path** (5/3/2, P 0.7 → 5.25)
- Stage: new branch beside A12 reading raw PCM / pre-Mood magnitudes; publication A20. Controls: timing of every hit-driven gesture, mode-18 energy rate, tempo input, release trigger.
- Current: novelty is one scalar computed after EMA, Mood, AGC and knee (A:357-367); BOOM peak ≈ 330 ms late; ±30 ms max 0.066 < build p95 0.069. `acquire()` carries only the latest hop for non-peak fields.
- Origin: inherited. Cost ≈ 0.3–0.5 % of the M85 [INFERENCE]. Clean-room reference exists: PRISM-Core `music/README.md:29-32` (15/150 ms envelopes, onset above 0.08 with a 60 ms refractory, microsecond onset identity :169-171).
- Manipulation: 6–8 overlapping biquad bands (30–70, 70–150, 150–400, 400–2k, 2k–6k, 6k–11k Hz), log-magnitude positive flux per band before Mood/AGC/knee, onset events `{t_sample, band_mask, strength}` in a ≥ 32-deep queue with an overflow counter, read max-since-cursor per board. Keep the legacy scalar for v8/v9 compatibility. Mood becomes a consumer setting and never feeds rhythm.
- Interactions: enables R12, R13, R11; feeds R23; R04 raises its sub band.
- Risk: noise sensitivity; false releases; must not make mode 18 less snappy (CANON "snappiness oracle", CRIT G3).
- Experiment: frozen (host click corpus + BOOM): onset→event median ≤ 15 ms, p95 ≤ 25 ms; band-flux peak within ±30 ms of 55.7025 s ≥ 2 × build-window (45–50 s) p95; ≥ 99 % of 1,000 random single-hop impulses reach the renderer exactly once and in order across AP/render phase sweeps and a 30 ms render pause; identical PCM at two Mood settings gives identical event times. Planted: computing on the post-LPF spectrum; latest-only publication; an all-band sum broadcast into every band.

**R07 — Colour chapter carried by the release** (4/4/2, P 0.6 → 4.8)
- Stage: M10 (W:630-657), deposit offset (W:956-958). Controls: whether the drop's new harmony arrives with the light.
- Current: path-P colour t50 ≈ 75–113 ms [INFERENCE, D13], so the first ≈ 0.2 s of a drop wears the build's colour; P3 drop walks gold→red into a dark end (T-COL4).
- Manipulation: at release, snap the deposit offset to the current pre-Mood centroid and let the front carry it; require chapter luma ≥ build luma.
- Experiment: frozen: centre-pixel offset within 0.004 of target within one frame of fire; drop-chapter median luma ≥ build-chapter median luma. Planted: no snap; chapter forced to the palette's dark end.

**R08 — Path-P dark ends and tension-driven spread** (4/4/2, P 0.6 → 4.8)
- Stage: V4/V5 (W:888-889). Controls: how much of the palette one half spans; whether dark stops eat deposited light.
- Current: k = 1 (a full palette turn per half, audio-independent); palette 31 has 17.5 % of positions below 5 % luma and a black→yellow seam; palette 38 is dark at both ends.
- Manipulation: `p = o + k·d`; rotate so a palette's dark end lands at the outer edges (or use the authored sub-range for non-cyclic palettes); k a function of harmonic tension / spectral flatness clamped to [0.15, 1] (D12 C05 ConsonanceSpread), never one colour, never a floor.
- Experiment: frozen: masked fraction (lit pixels ≥ 0.01 whose output max channel < 5 % of intensity × palette max) ≤ 1 %; at least two separated non-black palette regions during active music; distinct positions follow an annotated build ramp (Spearman ≥ 0.5) and never fall below 4. Planted: palette 31 unchanged under P (≈ 17 %, red); k = 0 fails the spread floor; a spread clause passable by the d ramp alone with music frozen must be red (closes P6). Then Captain Q6.

**R09 — Section-relative dynamics + iris** (5/3/2, P 0.6 → 4.5)
- Stage: A10, A15, M2, M3, M5. Controls: whether the drop can be longer and brighter than the build.
- Current: follower 0.25/0.005 (τ 30 ms / 2 s) + clamp → BOOM `peak_scaled` +7 % for +8.4 dB; broadband AGC τ 190 ms; ceiling −9 dBFS; gains 0.580/0.831.
- Origin: inherited K1 constants; the K1 product has since moved to per-band AGC.
- Manipulation: publish an unnormalised long-window level beside the normalised one; drive extent and brightness from a fast level relative to an 8–30 s reference; hold deposit gain ≤ 0.6 of its range during BUILD and open fully on release, reopening slowly if no release comes (D12 C12 LuminanceIris); per-band AGC as the S3 ships, with raw shape kept in parallel. Never manufacture the ratio by dimming the build so far that the visible contraction is lost (D12 VERIFY).
- Experiment: frozen (wire snapshot or host packed frames): drop/build p95 luma ≥ 1.5 on every tuning drop; song mean luma within ±10 % of host-006; median head distance from centre in drop + 0.5 s ≤ 0.5 × the last 2 s of build; post-roll room noise yields 0 admitted frames. Planted: today's follower red; an iris also applied to the drop red; a 1/1024 floor fails silence; gain τ 190 ms red.

**R10 — Timbre width + sub push** (4/4/2, P 0.5 → 4.0)
- Stage: M4 (W:826) and a new warp after the field, before colour. Controls: width = character, low end = displacement.
- Manipulation: width from spectral flatness (pure tone thin, noise wide; D12 C18); `x' = x − A·s(t)·sin(πx/80)` with s = high-passed `sub_rms` envelope (D12 C07). Never a brightness gain (strobe law).
- Depends on R04 (the sub band is cut today).
- Experiment: frozen: width vs flatness r ≥ 0.5 and width vs level |r| ≤ 0.3; a 55/66/90 Hz burst gives 4–8 px displacement with integrated intensity within ±5 %; equal-VU 220 Hz/1 kHz control ≤ 1 px. Planted: width = level; `K1_SUBBAND_PLANT_WRONG_CENTRE` (SB:23-24, 660 Hz) silences the 55 Hz arm; a brightness-gain implementation fails the intensity clause.

**R11 — Groove flywheel for board 1** (4/3/2, P 0.6 → 3.6)
- Stage: M8 mode 18 (W:980-998). Controls: upper-board motion at the right rate without a tempo lock.
- Current: up to 0.7 of its speed follows a tracker that locks to half time; energy rate 30 + 90·clip(novelty/0.12).
- Manipulation: each onset (R05; legacy novelty until then) adds an impulse to scroll velocity; drag τ 0.4–0.8 s (D12 C06).
- Experiment: frozen: scroll-velocity spectrum peak within 3 % of the annotated beat rate (or 2×) on BOOM and Sonic; onset→velocity rise ≤ current (snappiness oracle). Planted: a 69 BPM synthetic impulse clock peaks at half time and fails.

**R12 — Push-pull** (4/4/2, P 0.45 → 3.6): kicks pull the light inward for a moment, snares bloom outward (D12 C19). Depends on R05. Frozen: inward step on kicks and outward on snares, each ≥ 70 % hit rate within ±30 ms; integrated intensity within ±5 %. Planted: swapped band proxies.

**R13 — Tension state machine for causal release** (5/5/4, P 0.55 → 3.4)
- Design (D11 §3; S3 L04): FLOW → BUILD → ARMED → PROVISIONAL (inward accent ≤ 0.12, no global brightness) → RELEASE (size = stored charge) → RECOVER; QUIET drains to black. Predictions may arm; only observed evidence fires the release (CANON:43). A false full release costs more than a miss. Substrate: ≈ 1 KB context; optional 60 s feature ring ≈ 0.96 MB SRAM (D10 HW-16).
- Evidence: sub arrives 72–117 ms after the musical drop, so a sub-only trigger cannot meet +50 ms; surge triggers fired on climaxes and risers (X1–X3).
- Experiment: frozen (v2/v3 hit rule unchanged): ≥ 5/6 drops in [0, +50 ms]; zero false full releases including the 54.3 s return and within 2 s of annotated non-drop climaxes; falls back to FLOW on missing evidence. Planted: charge forced to 1; 200 ms-late fire; sub-only wait; broadband-surge-only trigger. Holdout: the Captain's corpus.

**R14 — Fractional transport + swept deposit** (4/3/2, P 0.55 → 3.3): fill the stride between frames in palette-position space (never RGB), keep fractional position until rasterisation (W:1000-1018 accumulates a remainder but moves whole pixels). First, one fixed-exposure camera frame of a static bead decides whether the LGP already hides the 4 px beading (instrument, not the Captain). Frozen: continuity R (5-px min/max over the lit trail) ≥ 0.8 at l = 0…1; centre-of-energy error ≤ 0.25 px across 0.25–4.05 px/frame; integrated-intensity drift ≤ 2 %; PAL-HD-v2 unchanged. Planted: forced integer rounding (≤ 0.6); RGB interpolation.

**R15 — Darkness truth** (4/3/2, P 0.55 → 3.3): one telemetry word per frame naming the gate that closed (silence, spectral gate, tonality, admission, freshness, user); replay Dopex and attribute every dark frame; test whether music held between −33 and −28 dBFS for 5 s drains to black. Frozen: exact packed zero within 1 s of digital-zero input; every Dopex dark frame attributed to exactly one gate; the −30 dBFS 6 s passage is reported (pass/fail is the Captain's ruling on quiet music). Planted: a 1/1024 floor fails black; an unlabelled gate reports "unattributed". Also attributes the camera's full-width dark lines with a wire snapshot.

**R16 — Circle-of-fifths order** (3/4/1, P 0.25 → 3.0; taste): chroma index i → 7i mod 12 before the centroid (W:157, :611). A fifth moves from 150° (shortest chromatic arc) to 30°. Frozen: frames with centroid strength ≥ 0.08 rise ≥ 1.5× on tonal tracks; colour still moves in ≥ 90 % of seconds. Planted: a random fixed permutation misses the rise. Then Captain Q10.

**R17 — Input headroom** (4/3/2, P 0.5 → 3.0): log-only arm first — per-slot guard engagement count and pre-guard peak (`k1_pdm_target_last_hop_gain_q8()` exists, PDM:626); then publish a pre-guard level side channel; then 20-bit capture (`PDM_PCM_WIDTH_20_BITS_0_18`) with the Goertzel headroom re-checked (`>> 6` and ×16 change it). Frozen: report the fraction of drop vs build slots with gain < 4096; a +6 dB step crossing raw 1,875 is recovered within ±0.5 dB in the side channel. Planted: a synthetic slot at peak 4,000 must report reduced gain; post-guard RMS labelled raw must fail.

**R18 — Hop-locked render** (3/2/2, P 0.8 → 2.4; canon-ruled): render triggered by the audio hop (133 Hz) with one GPT/ELC present time for both boards. Frozen (device log): publish→submit p50 ≤ 0.5 ms, p99 ≤ 5.5 ms, zero skipped frames; board-to-board first-edge skew p99 ≤ 20 µs. Planted: a fixed 10 ms tick fails p50. Then ACOUSTIC_TO_PHOTON per CANON:52 (iPhone 240 fps with audio in the same file; ≥ 500 onsets; p50 ≤ 8 ms, p99 ≤ 12 ms). Every `dt`-based constant changes character at a new rate; re-check the fade and EMAs.

**R19 — WS2816 transfer** (4/3/3, P 0.5 → 2.0): locked-exposure camera ramp of 32 steps per channel on one board. Frozen: fitted exponent 95 % CI width ≤ 0.2; code 0 indistinguishable from the dark frame. Planted: a synthetic linear ramp fits γ = 1.0 ± 0.05; +1/1024 on every code fails black. Only if γ > 1.2: a curve for in-house TRUE16 modes (donor modes already carry gamma). Then Captain Q11.

**R20 — Capture age** (3/2/3, P 0.6 → 1.2): 64–96-sample PDM slots (CANON:35 rules ≈ 1.6 ms blocks), offline poly24 filter tables to remove the 3–6 ms redesign stall, every loss counter exported. Frozen: poll high-water ≤ 2 ms over a 60 s BOOM run; zero ASRC discards. Planted: added 10 ms delay fails.

**R21 — Treble coverage** (2/4/2, P 0.5 → 2.0): evaluate short treble windows several times per hop with peak-hold. Frozen: 1,000 random 2 ms 8 kHz clicks detected in bins 70–79 ≥ 95 %. Planted: newest-window-only.

**R22 — Two-board roles** (4/3/2, P 0.35 → 2.1): first a one-board camera capture to measure vertical light overlap in the shared diffuser; then pitch-height or lower-releases/upper-shifts-colour (D12 C08/C09, reusing `PairedWaveScene` edge hand-off). Frozen: changing board 0's artistic control leaves board 1's bytes unchanged under fixed shared features; a shared release timestamp reaches both submissions within one frame. Planted: a shared renderer pointer; swapped top halves. Then Captain Q7.

**R23 — Tempo metrical repair** (4/3/4, P 0.4 → 1.2): octave candidates {½, 1, 2, ⅔, 3/2} with phase, variance and timestamp; phase extrapolation; zero confidence on silence. Use the existing GATE-SYNTHETIC-v1 (326 cases); P7 best is 35/326 and the flat prior alone 29/326 (STATUS:200). Planted: 88 BPM prior; half-tempo output. Stays in the P7 lane; R11 removes board 1's dependency meanwhile.

**R24 — Arithmetic hygiene** (1/2/1, P 0.5 → 1.0): float RMS accumulation; remove `>> 6` with re-frozen gains. Do only alongside another AP change. Frozen: ≤ 1 % RMS error vs double for amplitudes 64–16,000. Planted: per-sample truncation fails at amplitude 64.

---

## 6. Target architecture — PROPOSAL — NOT APPROVED

### 6.1 Principle (carried over from the colour fix)

Keep vectors and timestamps across container boundaries and collapse to a light value only at the pixel, once. Analyse once at full resolution; publish several **views** of the same audio — fast events, tonal shape, context — and let each visual consumer smooth, normalise or fold for its own purpose. Mood and colour speed become per-board consumer settings, never inputs to rhythm. Extend the existing pipeline; no new orchestrator, actor framework, dashboard or NPU graph.

```text
mic U14 (programme) ─┐                         mic U13 (witness; never required for light)
                     ▼
C0 capture: HPF ≈20 Hz, int32 (20-bit later), slot timestamp, applied gain, clip/loss counters
                     ▼
C1 resample 24 kHz ──┬── C2 fast events (bands, flux, onsets, timestamped queue)
                     ├── C3 tonal (80 bins exact-k, raw + display-conditioned, chroma[12], timbre)
                     └── C4 context (levels at 30 ms/0.5 s/2 s/8–30 s, tension vector, tempo candidates)
                                     ▼
C5 publication: versioned MusicFrame (epoch, generation, per-field age; legacy fields unchanged)
                                     ▼
C6 choreographer per board: FLOW/BUILD/ARMED/PROVISIONAL/RELEASE/RECOVER/QUIET → extent, front, velocity, iris, colour offset, spread k, width
                                     ▼
C7 field per board (intensity + palette position, fractional; analytic fronts) → C8 HD palette per pixel
                                     ▼
C9 display + R00 constraint layer → C10 TRUE16 pack → existing DMA/GPT (board 0 P601/GPT6 + P604/GPT8, LED DMA0/3; board 1 top engine)
```

### 6.2 Containers and data contracts

| Container | Runs on | Rate | Output contract (types, resolution) | State and placement |
|---|---|---|---|---|
| C0 Capture | M85 DMA + ISR (mic DMA1/2 unchanged) | PDM ≈ 41.4 kHz (MOCO, CAC-measured); slot 64–96 samples (CANON:35) | int16 now / int32 later per mic; `uint64` slot index + epoch; applied gain; clip and loss counters | DMA rings (existing); DTCM via `.dtcm_from_flash` rule |
| C1 Resample | M85 | 24 kHz; 180-sample hop (7.5 ms) and 64-sample sub-hop view | float32 PCM | poly24 state; offline filter tables |
| C2 Fast events (new) | M85 | every 64 samples (375 Hz) | `BandEnv[8]` dB; `OnsetEvent{t_sample:u64, band_mask:u8, strength:f32, observed:bool}` queue ≥ 32 + overflow counter; raw RMS dBFS; crest | biquads ≈ 0.3 KB, DTCM |
| C3 Tonal | M85 | 7.5 ms | `mag[80]` float unsmoothed (exact k, optional window, int64 or float state); `linear_shape[80]`; `chroma[12]`, centroid, strength, register; centroid/flatness/rolloff; `sub_rms` | window 4,096 samples (8 KB int16) |
| C4 Context | M85 | 7.5 ms light, 22.5 ms tempo | loudness at 30 ms / 0.5 s / 2 s / 8–30 s; section reference; tension {upper-band slope, onset-density slope, sub gap}; tempo candidates with phase/variance/timestamp — **independent of Mood** | ≈ 1 KB; optional 60 s ring ≈ 0.96 MB SRAM; optional MRAM warm start of AGC/tempo after reset |
| C5 Publication | M85 | per hop | `MusicFrame` {epoch, generation, per-feature support interval and availability time}; legacy `InhouseWaveformAudio` fields (WH:84-116) kept, new fields inert by default | double buffer; per-board cursor |
| C6 Choreographer | M85 | per render | state + visual parameters above; consumer smoothing (Mood) | ≈ 0.5 KB per board, independent |
| C7 Field | M85 | per render | `intensity[160]` float, `offset[160]` float, owner event; ≤ 32 analytic fronts | ≈ 2.6 KB per board |
| C8 Raster | M85 | per render | per-pixel HD sample in palette-position space; supersampling only while a front moves | — |
| C9 Display + constraint | M85 | per render | warmth/WB (named separately from calibration), shoulder, measured transfer, R00, the owner's envelope | small LUT |
| C10 Pack + present | M85 + GPT/ELC/DMA | 100 Hz now; hop-locked 133 Hz after R18 | GRB48 TRUE16; one declared present time for both boards | existing engines |
| I Evidence taps | M85 + host | per frame | read-only snapshots of pre-treatment features, state, RGB16, packed lanes, event ids, gate reason, idle counter | existing diagnostics; no new dashboard |

### 6.3 Compute budget per 7.5 ms frame (133 Hz)

| Item | Time | Label |
|---|---|---|
| Donor analysis (1.5 donor hops) | ≈ 0.43 ms | [INFERENCE] from 285k p50 per 5 ms (COST) |
| In-house analysis | ≈ 0.70 ms p50, 0.96 ms p95 | [FACT] COST |
| ASRC (×1.5) | ≈ 0.20 ms | [INFERENCE] |
| VP both boards | ≈ 0.27 ms p50, 0.35 ms p95 | [FACT] COST (per record) |
| Emit / pack | ≈ 0.13 ms | [FACT] COST |
| **Subtotal today** | **≈ 1.8–2.4 ms (24–32 %)** excluding RT-Thread/USB/ISR | [INFERENCE] |
| C2 fast bands | ≈ 0.03 ms | [HYPOTHESIS] |
| C4 + C6 | < 0.05 ms | [HYPOTHESIS] |
| Exact k, window, chroma 80 | ≈ +0.05 ms | [HYPOTHESIS] |
| Fractional transport + fronts | ≈ +0.05 ms; up to +1.5 ms only while an 8× supersampled front moves | [HYPOTHESIS] |
| **Projected** | **≈ 2.1–4.0 ms (28–53 %)** | [HYPOTHESIS]; gate on the idle counter (M0-4) before adoption |

**What runs where.** M85: everything (CANON:55: single compute core until a measurement on the un-hobbled profile shows it cannot hold capture + analysis + two renders + transport in 8.33 ms). Helium/MVE only on a measured hotspot with scalar equivalence (PL14-B5: a CMSIS attempt was slower in situ). Ethos-U55: nothing in this plan; later candidate for a causal onset/section model only after R05/R13 miss their frozen bars, ≤ 2 ms per hop, shuffled weights required to fail. Cortex-M33: parked; if ever needed, I/O only, never render. DTCM: hot bounded state; ITCM not reopened without a boot fix.

### 6.4 Migration from v9 (host-006) that keeps the Titan visualising

Each step is additive. host-006 path P stays the default until a step passes its frozen gates and the Captain's A/B. New fields default to values that reproduce today's output (tension disarmed, section gain 1, release absent); a new field that defaults to dark is a failed migration. Every new state falls back to FLOW on missing evidence; a detector failure returns to v9 behaviour, never to darkness or an invented release. Recovery images f6daf7fb / f2f1983a / b5595c35 remain (STATUS:201).

| Step | Content | Gate before build | Visible change |
|---|---|---|---|
| M0 Instruments | M0-1…M0-9 | each instrument red on its planted fault | none |
| M1 Geometry on labels | R01, R02, R03, R07, R00 behind runtime knobs (M0-8) or a compile flag with v9 default | EXT-v3 + R00 on host; product equivalence (`tests/host/test_p6_product_path_p.py` `mismatches=0`) | drop release at labelled times (test image only) |
| M2 Fast path | R05, R12, R18 | R05/R18 criteria; snappiness not regressed | hits on time; event-driven release candidates |
| M3 Dynamics + input truth | R09, R17, R04 (host replay → witness-lane sweep) | per-card criteria | drop fuller and brighter than build |
| M4 Causal release | R13 replaces labels | R13 hit rule, zero false full releases | release fires on real drops |
| M5 Colour | R06, R08, R10, R16 | per-card criteria | truer, richer colour; width and low-end motion |
| M6 Board 1 + output truth | R11, R22, R19, R14, R20 | per-card criteria | upper board grooves at the right rate; smoother fronts and tails |

### 6.5 Ship path (required)

1. **Already on silicon / in source.** On the Titan: host-006 (`e79d1ac0`), path P with gains 0.580092/0.830949, 127/127 readback (STATUS:201). In source at `01ec2dc`: path P default, pre-knee colour shape, `sub_rms`, release and accent machinery (off).
2. **Remaining steps.** M0 instruments → §9 Exp 1–3 → M1 host build frozen against EXT-v3 + R00 → product-equivalence pytest → build → program → identity readback → wire-snapshot equivalence + camera strip scoring → Captain A/B → repeat for M2–M6.
3. **Who acts.** The P6 lane owner (or the executing agent under that authority) builds, programs, reads back and records. The P7 lane owns R23. The Captain judges the taste A/Bs in §7 and rules on the brightness envelope.
4. **The stamp that means shipped.** The programmed image hash read back from the device matches the build receipt; every frozen criterion for that step went red on its planted fault before going green (`freeze_tolerance.py verify` clean); the wire snapshot reproduces the scored frames; and the Captain's A/B verdict is recorded in STATUS.

---

## 7. What the Captain judges by eye — small A/Bs (PROPOSAL — NOT APPROVED)

Each is one 10–15 s passage, two variants, same palette and matched maximum brightness, selected at runtime where possible (M0-8). Four randomised presentations including one identical-pair control; a pre-declared 3-of-4 preference decides; a tie stays undecided. A machine guard is frozen first and its planted wrong variant must fail before the look. What the LEDs render is checked by wire snapshot and camera, never by asking the Captain; CLI agents cannot listen, so musical congruence is his call (CANON:53).

| # | Clip | A | B | Decides | Flash? |
|---|---|---|---|---|---|
| Q1 | BOOM 50–60 s | release T90 30 / 60 / 120 / 250 ms | × peak ×1 / 1.5 / 2 | frozen bars for R01 and R09 | yes (until M0-8) |
| Q2 | BOOM drop | front sweeps edges → centre | light born at the centre streaming outward (today's release) | EXT-v3 direction clause confirmed or flipped | yes |
| Q3 | BOOM 49–58 s | riser re-lights the plate (today) | held contracted until the drop | R03 | yes |
| Q4 | BOOM drop | release at 0 ms | +50 / +100 ms (labelled) | whether detection must be predictive | yes |
| Q5 | Sonic Samba 20 s | **lower plate** Mood 0.05 | lower plate Mood 0.5 (upper held at preset) | colour liveliness | **no** (live per-board control; §9 Exp 3) |
| Q6 | BOOM build | spread k = 1 | k = 0.25 / tension-driven | R08 | yes |
| Q7 | BOOM drop | both boards release | lower releases, upper shifts colour | R22 | yes |
| Q8 | BOOM cut | current 0.2 s drain | implosion toward the centre (D12 C03) | cut gesture | yes |
| Q9 | BOOM 40–65 s | current extent ease | instant attack | R02 ("twitchy" check) | yes |
| Q10 | tonal passage | chromatic order | circle of fifths | R16 | yes |
| Q11 | quiet passage, dark room | no degamma | degamma (only after R19 measures γ > 1.2) | R19 | yes |
| Q12 | 20 s | path P | Spectrum River (bin k at pixel k, curated palette; SB `6109a1e`) | an alternative view, not a replacement | yes |

**Rulings, not A/Bs:** which brightness envelope binds on this unit (§8.2 X1); the drop annotation convention (grid downbeat vs first transient vs bass arrival); whether quiet music between −33 and −28 dBFS may drain to black (R15); naming tracks for a sealed holdout.

---

## 8. Contradictions and open questions

### 8.1 Disagreements settled from source

| # | Disagreement | Positions | Settled (evidence) | Effect |
|---|---|---|---|---|
| D1 | Does the live analyser wrap int32 and delete loud bass? | S2 yes (ledger #2, S5-02 rank 15, plain-English §1, open Q2); S1, S3, V1 no | [FACT] A:37-42 int64 product and q0, overflow counted; A:66-68 int64 magnitude; A:268-271 comment; A:308 any overflow invalidates the publication; [INFERENCE, IC] max state 13.9 M at guarded full scale, 154.6× below INT32_MAX | S5-02 removed; the trap stays in the parked kernel; re-check if R17 widens input |
| D2 | HPF corner 128 Hz vs 259 Hz | D01 128; D10 259; S2 "neither"; S3 conditional | [FACT] MAN §50.3.4.3: HPF operates on the sinc result; §50.3.4.5: the LPF then halves the rate → HPF clock 2·Fout; [INFERENCE, IC] model 100.1 Hz at 32 kHz, 259 Hz at 82.8 kHz | 259 Hz is the supported model; still [HYPOTHESIS] until the sweep; I capped at 3 |
| D3 | Where do the HPF coefficients come from? (S2: "weak citation, not a tracked file") | S2 | [FACT] `hal_data.c` is generated from the pinned BSP into the disposable stage (`build_scalar.py:816-820`); 0x3F61/0x3EC1 at :617-618 in all 9 staged evidence copies and in the host-006 stage; PDM has no HPF override | citation is sound; it is generated, not hand-tracked |
| D4 | Does the existing release "work when it fires"? | V1 yes ("filled edge to centre"); S1 + CRIT G1 no | [FACT] W:977 pulls the deposit to pixel 80 in one frame; transport only scrolls outward (W:1003-1015); EXT-v2 extent = innermost pixel ≥ 10 % of peak and its planted positive is a teleport (EXT2) | V1 claim withdrawn; inward motion needs new geometry (R01); EXT-v3 required |
| D5 | Is colour per board or board 0 only? | V1 C51 + V1 Exp 3 board 0 only; S1 per board | [FACT] `acquire()` overwrites spectrum, novelty, `linear_shape` and tempo per board (A:381-392); Mood per board (LR:87-90; FX:1666) | per board; Q5 and §9 Exp 3 run on the lower plate only |
| D6 | Ranking formula | S2, S3, V1 I×N÷E; S1 I×N×P÷E | CRIT G9: the plain formula ranks taste-only items above drop levers | expected value adopted; both columns shown |
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
| D17 | Camera: "the middle never fills at the drop" | S2 spot-check vs S2 synthesis | [FACT, seen] no sweep at 55.7 s; lower half fills its middle after ≈ 57 s | finding stands as "no release front"; half-to-board mapping open |
| D18 | Early 54.3 s return: detector fault or choreography? | S1 F-CAM1; S3 Q3 | [INFERENCE] no detector is active; returning sound + decayed follower park deposits at the edge (A:250; W:976) | choreography (R03) |
| D19 | "24/255 is old" (S3 §8 Q2) | S3 wording | global Titan mandate binds 24/255; conflict reported, not resolved | §8.2 X1 |
| D20 | Novelty "lower in the drop" | D01 summary | [FACT] only the median; mean, p90 and max are higher; ±30 ms max 0.066 < build p95 0.069; peak ≈ 330 ms late | the defect is timing and dilution (R05) |

### 8.2 Contradictions still open (reported, not resolved)

| ID | Conflict | Sources |
|---|---|---|
| X1 | **Brightness authority.** Global Titan mandate: maximum 24/255. `Kx-Titan/AGENTS.md:12`: default 100/255 supersedes 24. `PRH:19-23` static floor ≥ 100; `build_scalar.py:933-934` rejects < 100. CANON:51: master 255 × gains 0.25. Runtime readbacks show gains 0.25 and 1.0; 0.25 is a runtime setting (PRH:215, :228 default 65535). At least five gain stages could carry an envelope. Every luma criterion waits on one number from the Captain. | S1 §8.1, S2 §8, S3 Q2 |
| X2 | **Render rate.** Brief 120 Hz; product 100 Hz (PRH:40; 100.01 fps); CANON:35 rules hop-triggered render (133 Hz); `Kx-Titan/AGENTS.md:9` says 120 Hz; K1 S3 locked 120 Hz (`3776e777`). | R18 |
| X3 | **Drop direction.** The Captain's verbatim (EXT2) reads edges → centre; K1 canon says "heart outward"; D12 C02 ThrustFront leaves the centre. EXT-v3 adopts the verbatim; Q2 confirms. | R01 |
| X4 | **AV asymmetry.** ITU-R BT.1359-1 detectability ≈ −125/+45 ms (broadcast conditions); CANON:26 notes it; not a Titan tolerance. The P6 +50 ms bar was agent-chosen. | Q4 |
| X5 | **Latency stack.** ≈ 16.4 ms P5 stack already includes the 5.1 ms wire; adding it again double-counts. CANON:28 quotes 4.8 ms (without the 300 µs reset). | BUDGET.md:33-34 |
| X6 | **"Hard cut never while music plays" vs "fade fully to black on silence"** — compatible only if the silence gate is right. | R15 |
| X7 | **White.** "5–10 % at most, never full white" vs promo "warm-white blooms" (low weight); camera cream regions unexplained. | R00 |
| X8 | **Detector record wording.** "All failed to fire" vs v1 and v2 each hit one drop; none passed the set. Downbeat error "26–380 ms" mixes product nearest-beat errors with the reference clock. | T-DET |

### 8.3 Synthesis traceability (every synthesis id → final id)

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

Worker-level traceability (268 worker levers → atlas rows, CRIT G2) is still not written; `CRITIC/recovered/` holds the inputs with `SHA256SUMS`. D01 and D06 were never verified against their own text.

### 8.4 Transfer back to the K1 ESP32-S3 (default product path)

| Titan finding | Applies to K1? | K1 note |
|---|---|---|
| Follower 0.25/0.005 caps drop extent | yes, same constants (#93763) | trivial on 240 MHz |
| Extent polarity loud→edge; release missing; outward-only transport | yes (Waveform Hybrid K1 source) | same R01/R03 levers, same A/Bs |
| Novelty after the Mood LPF | yes (K1 `process_GDFT`) | ≈ 200 cycles/hop |
| Rounded k | yes | re-test now that int64 ships |
| `chroma_range` 60 | yes (v40102 default) | taste + correctness |
| Path-P ramp across dark palette ends | yes where per-bin HD colour is used | R08 |
| Broadband AGC | no: K1 product runs per-band | the Titan should follow K1 |
| HPF retune | no (I2S microphone on K1) | — |

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

---

## 9. Immediate next three experiments

All three are **PROPOSAL — NOT APPROVED** until the lane owner runs them under existing authority. Two run on the host with the existing frozen-gate machinery; one runs on the device with the camera loop and one Captain look, with no flash. First copy the replay set to the external scratch drive (M0-9), because it is volatile.

### Experiment 1 (host): does an inward release pass a test that can see direction?

- **Question.** With a release fired at the frozen drop anchors, does an edge→centre front pass an extent test that also checks direction, while today's teleport release fails it?
- **Existing machinery.** `tests/host/p6_effect_probe.cpp` (path P via `mapped`, release arguments at :138-146); `scripts/p6_ext.py evaluate --criterion v2`; EXT2 cases BOOM source, BOOM room, Sonic @27.7, Sonic @54.7 (anchors 55.7 / 55.7 / 27.7 / 54.7 s); known negatives `P6P/results/ext/ext-v2-b5595c35.json` and `ext-v2-v8.json`; replay inputs under `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6`.
- **Freeze first** (`freeze_tolerance.py freeze`): `TOLERANCE-FROZEN-P6-EXT-v3.json` = every EXT-v2 clause unchanged, plus a **trajectory clause**: from t_d, the innermost lit pixel i_min(t) (same 10 % luma rule) is non-increasing toward the centre for ≥ 3 consecutive frames before extent first reaches 0.9; T90 ≤ 0.18 s; the lit-run front velocity is inward on ≥ 95 % of frames during the transit; no extent jump ≥ 0.3 within 50 ms anywhere in [t_d − 2 s, t_d). A new evaluator mode `--criterion v3` in `scripts/p6_ext.py` (a tools change, not firmware).
- **Stage A, instrument validation (no firmware change).** Score frame sets built by overlay on the host-006 path-P replay frames: planted negatives (a) host-006 as is; (b) the existing instant release forced at t_d (X1/X3 shape: i_min jumps to 80 in one frame); (c) a centre-born outward front; (d) a correct inward front 200 ms late. Planted positive: an analytic inward front at t_d, T_r = 120 ms. Pass condition for the instrument: (a)–(d) red, positive green on every tuning case and both boards.
- **Stage B, renderer.** Implement the R01 front under the existing `K1_INHOUSE_V9_HOST_VARIANTS` guard, driven by the labelled times; `tests/host/test_p6_product_path_p.py` must still report 0 mismatches, proving the product image is untouched. Decision rule frozen in advance: pass = EXT-v3 passes on all four tuning cases on both boards and C2/C3/C4/C5b still pass; the lag clause is satisfied by construction and is declared non-evidence.
- **What the result means.** Pass: the look is solved given timing; next are R03/R02 in the same harness (bundle ablation, §5.2), then Q1/Q2 for the Captain, then R05/R13 for timing. Fail on size or contraction: R02/R03/R09 come before any detector work. If Stage A cannot make the teleport red, EXT-v3 is redesigned before anything else is scored.

### Experiment 2 (host): does the microphone's bass cut change the light?

- **Question.** If the HPF sits at 259 Hz, how much of the BOOM drop's sub-bass, level surge and **extent** does it remove compared with a 20 Hz DC block, once the device's loud guard is in the chain too?
- **Already done.** IC `hpf`: model reproduces 100.1 Hz at 32 kHz; 259 Hz at 82.8 kHz; candidate 0x3FE7/0x3FF4 = 20.1 Hz.
- **Run.** Build the M0-5 pre-stage (scratch script): first-order high-pass at the model corner (259 Hz), the candidate (20 Hz) and a planted 400 Hz, each followed by ×16 and the per-296-sample-slot guard at ceiling 30,000 (SENS:9-33). Feed BOOM source through each into the same path-P probe. Report `sub_rms`, `vu_level`, `peak_scaled`, guard-engaged slot fraction (build vs drop) and EXT-v2 statistics (E_climax, E_drop_max, J) for the last 2 s of build and first 2 s of drop.
- **Freeze before results.** Model check 100 ± 2 Hz at 32 kHz; a 1-LSB k1 change moves fc by > 0.5 Hz. Effect: drop `sub_rms` with 20 Hz ≥ 6 dB above the 259 Hz model. Sensitivity: the planted 400 Hz reads ≥ 3 dB below the 259 Hz model. Visual: R04's impact rises above 3 only if median extent in [t_d, t_d + 1 s] differs by ≥ 0.05 or J by ≥ 0.10 between 259 Hz and 20 Hz. Guard plant: ceiling 3,000 must push the drop's engaged-slot fraction above 90 %.
- **What the result means.** A large light change justifies the witness-lane device sweep (R04 device step, no Captain look) and raises R10. A small change demotes R04 and confirms that the follower/AGC (R09) is the binding stage. `boom_room` is reported as supporting evidence only (speaker and room roll-off confound it).

### Experiment 3 (device, no flash, one Captain look): make the camera an admissible instrument and settle colour speed on the lower plate

- **Step 1, instrument (no Captain).** On host-006 as installed (identity readback first): take a wire snapshot of a 20 s BOOM passage covering 55–75 s with `scripts/palette_wire_snapshot.py` and compare with the host replay of the same audio. Freeze first: packed-lane equivalence within the existing tolerance on both boards; exact zero during the silent cut; the frames at the camera's dark lines (≈ 57.4, 59.3, 64.5, 66 s) classified as wire-dark or wire-lit. Planted: the host-005 gains (0.396/0.592) or palette B replay must fail the comparison.
- **Step 2, camera loop.** Re-record BOOM 40–70 s with the same framing via `scripts/titan_camera_record.py` (serve/request mode; the server runs in the Captain's Cursor tab because of macOS camera permission, STATUS:202-203) and `scripts/titan_camera_strip.py --panel 107,220 --t0 40 --t1 70 --marks 54,55.7`. Freeze first: admissible only at alignment r ≥ 0.7 or with a declared visible sync marker; identify which half is which board by palette (ids 31 vs 38). This recording becomes the "before" for every later geometry clause.
- **Step 3, one Captain look.** Using the live per-board Mood control (FX:1666; readback FX:1546), set **board 0 only** to Mood 0.05 (default) and 0.5 on the same Sonic Samba 20 s passage, four shuffled presentations including one identical pair. Tell the Captain to judge the lower plate; board 1 stays at its preset. Freeze first: Mood readback equals the command for board 0 each time and board 1's Mood is unchanged; host model shows the path-P offset step response is faster at 0.5 (τ ≈ 80 → ≈ 20 ms). [FACT] by source board 0's Mood cannot change board 1's colour, novelty or tempo (A:381-392). Planted: a presentation labelled 0.5 but commanded 0.05 fails the readback check.
- **Question for the Captain.** "On the lower plate, which colour feels more on the music without becoming nervous?" This settles Q5 and the first half of R07. Restore Mood 0.05 afterwards, confirm by readback, and confirm visualisation never stopped (no flash, no dark interval).

---

## 10. Files read and checks run by this integrator

- Read in full: SWARM-BRIEF.md; SYNTH-S1-opus-v2.md; SYNTH-S2-grok-v2/SYNTHESIS.md; SYNTH-S3-astra-v2/SYNTHESIS.md; LEVER-ATLAS.md (v1); CRITIQUE.md §§ G1–G14 and "What holds up".
- Read in part (this session): A:30-75, :225-330, :370-429; WH:36-82, :120-130; W:750-800, :870-1020; PR/PRH/G/TT/SB line checks; PDM HPF search (no override); `build_scalar.py:816-820`; 9 staged `hal_data.c:613-619`; EXT2 (definitions, cases, bars, planted positive); STATUS:196-204; COST; LR:80-95; FX:1540-1548, :1660-1668; `p6_effect_probe.cpp:130-150`; CANON:25-55; PL14:147; PRISM-Core `music/README.md` (HEAD `89c79df`); `CRITIC/recovered/D12-creative-vision.json` (C01–C19); IC; MAN §50.3.3 note, §50.3.4.3, §50.3.4.5.
- Opened and inspected: `docs/evidence/P6/camera/boom-host006-a-strip-40-70.png`.
- Not done: no build, replay, flash, device read, camera run or commit. The only file written is this atlas.
