---
abstract: "The definitive Titan lever atlas, 15 Oct 2026. It merges three independent syntheses (Opus S1, Grok S2, Astra S3) of the 13-dimension lever swarm and settles their disagreements from source. It contains a plain-English summary, the AP to VP pipeline map, the compression ledger, the failure-to-lever map, one ranked portfolio of 27 levers, the target architecture, the Captain's taste A/Bs, the settled and open contradictions, and the next three experiments. Research only. Every change is PROPOSAL — NOT APPROVED."
id: LEVER-ATLAS
date: 2026-10-15
author: Integrator (Claude Opus 5.5), read-only on all repositories
inputs: "SWARM-BRIEF.md; SYNTH-S1-opus.md; SYNTH-S2-grok/REPORT.md (incl. wrapper §11); SYNTH-S3-astra/REPORT.md; D02/D10/D11 VERIFY and artefacts; source at d557f06 (HEAD 045b6b5); runs/STATUS.md; P6 EXT results; RA8P1 manual R01UH1064EJ0130 Rev.1.30"
images: "N/A. This is a text atlas. No LED output was produced or judged. Every appearance question is routed to an instrument (wire snapshot, frozen host gate) or to a Captain taste A/B in section 7."
changelog:
  - "2026-10-15 integrator: first version. Merged S1/S2/S3; settled 14 disagreements (§8.1); added integrator arithmetic INTEGRATOR/settle_checks.{py,json}; added §9 next three experiments."
---

# Titan lever atlas

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.** It boots with visual output on and runs continuous, autonomous, audio-reactive visualisation whenever it is powered and the application is running. Nothing here adds a permission gate, a brightness floor, a normaliser that breaks true black, or a rainbow default. Nothing here changes the brightness envelope, Titan UID `545433931bd25436593630352d068363`, P601/GPT6 and P604/GPT8, microphone DMA1/2, or LED DMA0/3. A flash makes the light go dark for about 41–47 s (PL14-A6). That gap is disclosed, and the previous image remains the fallback.

**Status:** research synthesis. No device, serial, USB, flash, build or commit was done by the integrator. One host arithmetic script was run inside this run folder (`INTEGRATOR/settle_checks.py`, output `INTEGRATOR/settle_checks.json`). Every proposed change, image and experiment below is **PROPOSAL — NOT APPROVED**.

---

## 0. Evidence identity and how to read this

- [FACT] Product worktree `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`, branch `lane/titan-lagfree-20261014`. HEAD at integration is `045b6b5`, a tools-only commit (iPhone camera recorder) on top of `d557f06`. `git status` shows no change to the four v9 source files, so every line number below is valid for `d557f06`.
- [FACT] `d557f06` makes path P (per-pixel HD palette) the compiled default: `kInhouseV9Palette = kPerPixelMapped` (`inhouse_waveform.h:50`), gains `0.580092 / 0.830949` (`:38-39`). Its image is `arm-p6-v9-host-006`, build_id `e79d1ac0e50c3d66…` (`docs/evidence/P6/pal-fix/host-006-receipt.json:388`).
- [FACT] **That image is on the Titan.** `runs/STATUS.md:201` records at 2026-10-14T17:58Z: "host-006 e79d1ac0 programmed (identity MATCH, 99.9 fps, led_faults 0, VERDICT OK visualising) … Readback 127 fields / 0 differences vs v9 preset". All three syntheses said it was not programmed or not checked. The integrator did not read the device back; this is the lane's own recorded readback.
- [FACT] The host-006 receipt also records: in-house analysis at 24 kHz (`inhouse_analysis24: true`, `:424`), top board output on (`titan_top_output: true`, `:411`), DMA map `led-first` (`:410`, which gives mic DMA1/2 and LED DMA0/3, D10 VERIFY row 17), `npu: false` (`:397`).
- Abbreviations (worktree-relative): **W** `src/k1/core/visual/inhouse_waveform.cpp`; **WH** `…/inhouse_waveform.h`; **A** `platform/ra8p1/inhouse_audio.cpp`; **AH** `…/inhouse_audio.h`; **PR** `platform/ra8p1/palette_runtime.cpp`; **PRH** `…/palette_runtime.h`; **G** `src/k1/core/audio/gdft_goertzel.cpp`; **SB** `src/k1/core/audio/sub_band.h`; **TT** `src/k1/core/audio/tempo_tracker.cpp`; **PDM** `platform/ra8p1/pdm_target.c`; **SENS** `platform/ra8p1/k1_pdm_sensitivity.h`; **BUD** `docs/evidence/P5/device-budget/BUDGET.md`; **RUNS** `_scratch/waveform-migration-001/runs`; **P6P** `RUNS/P6-20261014T124826Z/permitted`; **MAN** the RA8P1 manual text at `Kx-Titan/_scratch/pdm-profile/hardware-manual.txt`; **IC** `INTEGRATOR/settle_checks.json` in this folder; **PL14** Kx-Titan `docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md`.
- Source syntheses: **S1** `SYNTH-S1-opus.md` (ledger C01–C45, portfolio T01–T20); **S2** `SYNTH-S2-grok/REPORT.md` (S2-01…S2-21; §11 is the wrapper's correction); **S3** `SYNTH-S3-astra/REPORT.md` (S3-L01…L20). Dimension files are `D01`…`D13` in this folder. D01 and D06 were never verified as reports; their figures are used only where another source confirms them.
- Labels: [FACT] = the line, commit, measurement or document was read. [INFERENCE] = arithmetic or a join across facts. [HYPOTHESIS] = unmeasured.

---

## 1. Plain-English summary for the Captain

1. **The Titan is not short of processing power.** Sound analysis takes about 1 ms in every 5 ms, and drawing a frame takes 0.2 ms in every 10 ms (BUD:29-40). Almost all the "squeezing" of musical detail was copied from SensoryBridge (2023) or the K1, or happened by accident. Only a handful of steps were ever true budget cuts, and the Titan no longer needs them.
2. **Why the drop does not hit harder.** In the BOOM replay the drop is about 8 dB louder than the end of the build. But the number that sets the waveform's length only rises from 0.84 to 0.90, because it is divided by a "recent loudest" tracker that catches up within about 30 ms. So the point where new light is drawn barely moves, from pixel 126 to pixel 130 of 160 (D02 `clamp_occupancy.json`). The light gets brighter at the drop; it does not get longer.
3. **The release you described already works, when it fires.** Three release designs tested on 14 Oct filled the strip from edge to centre within 40–70 ms of the BOOM drop. They were rejected because they fired at the wrong moments (on risers) or missed other drops, not because the motion was wrong (`P6P/results/ext/ext-v2-X1.json`, `-X3.json`).
4. **So the next test separates the look from the timing.** It fires the existing release at hand-marked drop times and scores it with the same frozen test. If it passes, what is left is purely a timing problem.
5. **The microphone's built-in bass cut is probably far too high.** Its settings were made for a slower rate. Worked through from the chip manual, it cuts 110 Hz by 8 dB and 55 Hz by 14 dB before any analysis runs (IC `hpf`). The fix is one pair of register values. It still has to be measured on the device.
6. **The colour fix (path P) is now on your Titan.** The lane recorded it as programmed at 01:58 on 15 Oct (`STATUS.md:201`). Much of its "9–16 colours along the strip" is a fixed palette ramp from centre to edge. The music still moves one colour number per frame. Whether you like that ramp is your call.
7. **Two earlier worries are retired.** The "32-bit overflow" in the note analyser does not apply to the analyser the product actually runs, which has at least 155 times headroom (IC `goertzel`). And switching off the 88 BPM tempo preference on its own does not fix the tempo (29 of 326 cases pass, `STATUS.md:200`).
8. **What only your eye can settle** is set out as short side-by-side clips in section 7. One of them, colour speed, can be done today on the image already on the Titan, with no reflash.

---

## 2. Pipeline map: AP → mapping → VP → output

Containers in chain order. "Levers" are portfolio items A01–A27 (section 5) and ledger rows C01–C52 (section 3).

| # | Container / function | Where (d557f06) | What it does today | Rate / resolution | Levers |
|---|---|---|---|---|---|
| A1 | PDM capture, hardware filters | `hal_data.c:617-619,643` (staged BSP, D10 VERIFY 1, 6); PDM:764-781 | Sinc → HPF → compensation → half-band LPF ÷2. HPF and compensation run at the sinc output (2·Fout) (MAN 213207, 213325-213327). Reset HPF coefficients. 16-bit window [14:0] | Fout ≈ 41.4 kHz (BUD:25); HPF clock ≈ 82.8 kHz | A01, A15, C01, C02 |
| A2 | Lane select | PDM:607-610 | Programme mic kept; the second mic (U13/U14) is metered and discarded | 296-sample slots, 7.15 ms | A15, C04 |
| A3 | Gain + loud guard | SENS:9-22; PDM:612-636 | ×16 (Q8 4096). Gain cut per slot so the slot peak ≤ 30,000; trips at raw peak 1,875 | per slot | A08, C03 |
| A4 | Resample | `k1_poly24`; donor ASRC 12.8 kHz | In-house analysis 24 kHz / 180-sample hop **in the product image** (receipt `:424`); header default is 12.8 kHz (AH:46) | 7.5 ms hops (133.3 Hz); donor 5 ms | C05 |
| A5 | Level / RMS | A:228-246 | raw_rms, vu_level (pre-AGC); per-sample Q16 truncation | per hop | A27, C06 |
| A6 | Peak follower | A:247-254 | follower τ 30 ms up / 1.99 s down; `peak_scaled = drive/follower`, then τ 5.4/57 ms | per hop | A06, C17 |
| A7 | Sub-band (v9) | SB:3-24; A:239-241 | 40–110 Hz band → hop RMS `sub_rms`; consumed only by the disabled release | per hop | A09, C13 |
| A8 | Note bank (Goertzel) | G:102-131 (configuration); A:36-69 (active scalar kernel) | 80 bins from 110 Hz; rounded integer k; Cross40 halves bins 0–39; rectangular; `>>6`; **int64 products and magnitude**; any int32 excursion is counted and invalidates the hop (A:39-40, :308) | windows 1.67–76.4 ms | A12, A23, C07–C12 |
| A9 | Spectrum smoothing | A:274; A:103-111, 313-326 | 0.3/0.7 hop EMA (τ 21 ms), then Mood LPF (τ 79.6 ms at 0.05; a 100 Hz coefficient run at 133 Hz) | per hop | A07, A10, C15, C16 |
| A10 | AGC, tilt, knee, clamp | A:329-358 | one broadband gain (target 0.25, 0.1–10, τ 190 ms), noise gate, tilt, knee > 0.5, clamp 1 | per hop | A07, C18, C19 |
| A11 | Pre-knee shape (path P) | A:346-350; published from board 0 only (A:302-306) | `linear_shape[80]` before gain/knee, after A9 smoothing | per hop | A10, A14 |
| A12 | Novelty | A:357-367 | sqrt(mean positive difference) of the A10 spectrum: one scalar | per hop | A07, C20 |
| A13 | Tempo | TT:17-48, 500-595 | 60–155 BPM, 88 BPM prior, 512-update warm-up, decimation 3 | 44.4 Hz | A24, C21 |
| A14 | Publication / acquire | A:370-430 | latest hop; only the peak has max-since-read over ≤ 8 hops | 133 Hz → 100 Hz read | A25, C22 |
| M1 | Response clamp + admission | PR:252-272 | peak/vu clamped to 1; donor-PCM admission; quiet confirmation | per frame | A13, C23 |
| M2 | Spectrum mix + chroma fold | W:144-176, 596-625 | bins `< chroma_range` (60) folded to 12 classes → one circular centroid | per frame | A05, A12, C24–C27 |
| M3 | Colour offset | W:630-657 (path P) | weak-strength anchor blend; one offset per frame | per frame | A10, A14, C28 |
| M4 | Level transfer | W:940-950; WH:52-53 | −33…−9 dBFS → 0…1, emphasis 1.5 | per frame | A11, C29 |
| M5 | Extent (mode 32) | W:970-977 | 16 ms then 23 ms EMA; `position = min(159, 80 + 0.7·80·peak_stage2)`; release pull (off) | per frame | A02, A03, A06, C32, C33 |
| M6 | Accent / release (v9) | W:725-796; WH:55-79 | low-band onset accent; surge release; both off; release config is `inline constexpr` (WH:79), so a device A/B needs a rebuild | per frame | A02, A18 |
| V1 | Deposit | W:812-870 | amplitude g·l^1.5, width `0.5 + 3.5·l` (W:826), max-hold owner | per frame | A11, A16, A20, C30, C31, C35 |
| V2 | Transport | W:963-1019 | outward scroll `405·motion` px/s (W:967), integer steps, ≤ 8 steps/frame (W:968) | per frame | A20, C34 |
| V3 | Fade | W:798-807 | τ 290 ms (mode 32), 125 ms (mode 18), extra drain in silence | per frame | keep; C38 |
| V4 | Output / shade | W:872-915 | path P: `sampleProductPaletteHd(wrap(offset + d))`, d = distance from centre/80 (W:888-890); mirror 79/80 | per frame | A14, C36, C37 |
| O1 | Display treatment | PR:281-294 | brightness `.3 + .7·b` (PR:283), warmth (PR:284-285), white balance {1, .9375, .84}, tanh shoulder > 0.75 (PR:292) | per frame | A04, C39 |
| O2 | Brightness / gains / pack | PR:687-740 | scale16 cap, board/lane gains, GRB48 TRUE16 | per frame | A21, C40, C41 |
| O3 | Tick + wire | PRH:40; BUD:13, 70-81 | 10,000 µs tick (100 Hz) under the port build; 80 px per DIN in lockstep, 5.1 ms wire | 100 Hz | A26, C42 |
| O4 | Second board | `ws281x_gpt_dma_hw_top.c` (D10 VERIFY 16); receipt `:411` | top board on its own engine (GPT7/12, DMA6/7, P714 among its pins), enabled in host-006; reversed half order in the packer (PR:725-733) | 100 Hz | A17, C37 |
| O5 | Donor modes 0–6 | PR:411-420 | RGB8×257 by default; 128 → 160 px | 100 Hz | C43, C44 |

[FACT] Measured cost (image `f2f1983a`, BUD:29-40, 81): audio stage median 0.978 ms per 5 ms donor hop (in-house 0.70 ms), render 0.197 ms per frame, pack 0.128 ms; 47,505 frames, 0 skipped. host-006 on BOOM: poll-cycle p50 161,990 cycles per poll at 1 GHz, 0 missing records (`P6P/cost-host-006/report.json`). [INFERENCE] About 20–28 % of one M85; there is no idle counter, so this is an estimate.

---

## 3. The compression ledger

Every step on the Titan today that destroys musical information. **Origin** uses only what is evidenced: *budget* (a cited S3 cycle or deadline reason), *aesthetic* (a cited look decision), *inherited* (copied; reason unknown), *accident*. **Titan removes need?** asks whether the Titan's headroom removes the original reason. **Gain** is the integrator's [INFERENCE] of visual value (H/M/L). Row IDs follow S1 where S1 had the row; integrator corrections are marked ⚑.

| ID | Step (where) | What is destroyed (numbers) | Origin | Titan removes need? | Cost to undo | Gain | Lever |
|---|---|---|---|---|---|---|---|
| C01 ⚑ | PDM HPF reset coefficients (`hal_data.c:617-619`; PDM:764-781 overrides only SINCDEC/SINCRNG/threshold) | [INFERENCE, IC `hpf`] First-order DC blocker, k1 = 0x3EC1. The model reproduces the manual's 100 Hz at a 32 kHz filter clock (100.1 Hz). At the product filter clock of 82.8 kHz: fc 259 Hz; −16.3 dB at 40 Hz, −13.7 dB at 55 Hz, −8.2 dB at 110 Hz, −3.8 dB at 220 Hz. Not yet measured on the device | accident (rate retuned to SINCDEC 49; HPF never re-derived) | yes; a ~20 Hz DC block is enough (candidate k1 0x3FE7, s0 0x3FF4 → 20.1 Hz, −0.55 dB at 55 Hz) | one register pair per channel; 0 CPU; needs a flash | **H** | A01 |
| C02 | PCM width 16-bit [14:0] (`hal_data.c:643`) | about 4 bits of headroom; 20-bit exists (`r_pdm_api.h:43`) | inherited BSP default | yes | int32 path through ASRC and analysis; **re-check Goertzel headroom** (C08) | M | A08 |
| C03 | ×16 + per-slot loud guard (SENS:9-22; PDM:612-636) | any slot whose raw peak exceeds 1,875 is scaled to peak 30,000, so a drop louder than the build can reach analysis at the same peak. How often it engages is unknown: only the last hop's gain (4096) and the clip counters (0) were captured on BOOM (`P6P/cost-host-006/counters.json`) | inherited int16 storage | yes, with int32/float storage | small; per-slot engagement count first | **H** if it engages | A08 |
| C04 | Second mic discarded (PDM:607-610) | a full independent acoustic channel | inherited | DMA already paid | ≈ 2.7 % CPU to analyse [INFERENCE, D10] | M (instrument) | A15 |
| C05 ⚑ | Donor analysis at 12.8 kHz (donor modes only) | content above 6.4 kHz on donor modes. **In-house analysis already runs at 24 kHz in host-006** (receipt `:424`); S2's ledger row C4 is out of date | inherited SensoryBridge 4.0.0; S3 budget (ADR-0008) | yes | retune donor | L | — |
| C06 ⚑ | Q16 RMS truncation (A:233-237) | samples with abs < 128 add zero. [INFERENCE, IC `rms_truncation`] RMS error −12 % at amplitude 256, −2 % at 590 (RMS 0.013, −38 dBFS), < 1 % above 1,000. All of this is below the level-transfer floor (−33 dBFS, W:948-949) and the silence entry (RMS 0.04, WH:14), so it has **no visible effect** | inherited arithmetic | yes | trivial | L | A27 |
| C07 | `>> 6` input shift (A:39) | about 6 bits per sample before the resonator | inherited SensoryBridge | yes | small; re-freeze gains | L | A12 |
| C08 ⚑ | int32 wrap (S1 C08, S2 C8) | **Not present in the active analyser.** The in-house kernel uses int64 products and magnitude (A:36-43, 60-66); an int32 excursion is counted (A:40) and invalidates that hop (A:308) and resets tempo (A:282-285). [INFERENCE, IC `goertzel`] At guarded full scale (30,000, 24 kHz) the largest state is 13.9 M, 155× below INT32_MAX. The wrap belongs to the parked kernel (`G` with `wrapToInt32`), which the product does not run | — | — | none today. If 20-bit capture (C02) is widened through ×16 and `>>6`, headroom falls below 1× and must be re-checked | — | (A08 risk) |
| C09 | Rounded integer k (G:121-128) | bin 0 resonates at 104.7 Hz, not 110 (IC `goertzel`; D07 VERIFY 22); low half about 80–86 cents flat | inherited | yes | 0 cycles (setup only) | M | A12 |
| C10 | Rectangular window (A:58-59) | −13 dB sidelobes; leakage across semitones | inherited (removed between SB 3.0.0 and 3.2.0, no reason) | yes | ~35k multiplies per hop | L–M | A12 |
| C11 | Cross40 ×2 divisor on bins 0–39 (G:115-119) | bass resolution halved | budget, indirect (K1 `e911f86d`) | CPU yes; latency cost stays | dual-resolution bass | L–M | A12 |
| C12 | 2,000-sample cap (G:20, 117-120) | **nothing today**; largest block is 1,834 at 24 kHz (IC) | inherited | n/a | n/a | — | — |
| C13 | Bank floor 110 Hz (G:18) | 40–110 Hz only as the scalar `sub_rms`, which no live consumer reads; and C01 cuts that band by 8–16 dB first | inherited | yes | `sub_rms` already paid | **H** | A09 |
| C14 | Treble windows shorter than the hop | bins ≥ 2.35 kHz see about half of each 7.5 ms hop; bin 79 sees 40 of 180 samples | inherited | yes | ~12k MAC per hop | L–M | A23 |
| C15 | Hop EMA 0.3/0.7 (A:274) | τ 21 ms on every spectrum consumer | inherited SB | yes for timing consumers | free | M | A07 |
| C16 | Mood LPF (A:103-111, 313-326) | τ 79.6 ms at Mood 0.05 (D02 `vp_numbers.json`); colour lags about 0.1 s; onsets 37–83 ms late [INFERENCE, D13 model] | aesthetic (Mood control) + accident (100 Hz basis) | yes for event consumers | free; **Mood is live-settable per board** (`k1_live_runtime.h:87-88`; `fixture_app.cpp:1666`) | **H** (timing) | A07, A10 |
| C17 | Peak follower → `peak_scaled` (A:247-254) | BOOM: VU −18.4 dBFS (last 2 s of build) → −10.0 dBFS (first 2 s of drop), but `peak_scaled` median 0.844 → 0.900 and root 126.4 → 129.8 px (D02 `clamp_occupancy.json`, windows anchored 27 ms late per D02 VERIFY) | inherited K1 "short leash" constants | yes | small | **H** | A06 |
| C18 | Broadband spectral AGC (A:329-341) | build-to-drop spectral contrast; frozen while gated | inherited K1 AGC v2 ("louder music → dimmer") | yes | per-band or slow room normaliser | **H** | A07 |
| C19 | Soft knee + clamp (A:352-358) | pre-knee values ≥ 1.5 indistinguishable | inherited | yes for analysis | free (path P already bypasses) | M | A07 |
| C20 | Novelty = one post-AGC/knee scalar (A:357-367) | 80 changes → 1; sub-bass absent; feeds tempo and mode-18 speed | inherited | yes | 5–20k cycles per hop | **H** | A07 |
| C21 ⚑ | Tempo: decimation 3, 88 BPM prior, 60–155 BPM, 512-update warm-up (TT:17-48, 500-518) | half-time locks. **Removing the prior alone is not a fix:** GATE-SYNTHETIC-v1 flat prior 29/326, best candidate 35/326, and a planted 88 prior does not discriminate (`STATUS.md:200`) | prior inherited S3 `c3e13ff`; decimation indirect budget | yes | design work | M (enabler) | A24 |
| C22 | Acquire newest-only except peak (A:370-430) | about 25 % of hops never rendered for spectrum/vu/novelty at 100 Hz [INFERENCE] | inherited | yes | extend the existing peaks ring | M | A25 |
| C23 | Response clamp peak/vu ≤ 1 (PR:265-272) | top of the range identical | Titan-specific | yes | soft-saturate downstream | L–M | A11 |
| C24 | Spectrum mix 0.75/10 ms (W:147) | τ 7.2 ms; light | inherited Emotiscope | n/a | — | L | — |
| C25 | `chroma_range = 60` (WH:126) | bins 60–79 (≈ 3.5–10.5 kHz) never vote for colour | inherited SB default | yes | one constant | M | A12 |
| C26 | 80 → 12 fold → 1 centroid (W:154-176, 596-625) | octave, polyphony and multi-peak structure; one colour number per frame | inherited bloom design (#54824 "by design") | yes | ~26 µs per frame for richer signatures [INFERENCE, D02] | **H** | A05, A14, A16 |
| C27 | chroma_max normaliser (W:157-163) | rise ~195 ms, decay ~10 s: a loud drop is re-normalised in ~0.2 s | inherited | yes | small | M | A10 |
| C28 | Non-P colour follower/caps (W:655-722) | colour moved in 1 % of seconds on `b5595c35` | P6 design | path P replaces it | done in `d557f06`, on the device | done | — |
| C29 | Level ceiling −9 dBFS (W:948-949) | 34–35 % of BOOM drop frames at l = 1 (D02) | P6 design | yes | small | **H** | A11 |
| C30 | Deposit gain (WH:38-39) | top deposit 0.580 (mode 32) of the post-cap range [INFERENCE: ~42 % of the cap unused] | P6 matched-average-light calibration | n/a | rule change | M–H | A11 |
| C31 | Width = level (W:826) | width duplicates level; timbre unexpressed | P6 iteration 1 | n/a | small | M | A16 |
| C32 | Extent polarity + 0.7 cap + 16/23 ms cascade (W:974-976) | louder → root nearer the edge; a fresh deposit stops at px 136 (≈ 140 with width); t90 ≈ 70–76 ms | inherited Hybrid K1 look ("loud pushes the trace far from centre") | n/a (look) | zero CPU | **H** | A02, A03 |
| C33 | Release/accent compiled off (WH:55-79) | the drop never reaches the light as an event | P6 candidates X1–X3/K1–K3 failed on trigger timing (§4.1) | n/a | flag; constexpr (rebuild) | **H** | A02 |
| C34 | Scroll fixed 405 px/s, integer, ≤ 8 steps/frame (W:967-968) | speed channel unused; beading 4.05 px stride vs 3.0 px deposit at l = 0 | 405: Captain-calibrated on S3; cap inherited | yes | small | M | A20 |
| C35 | Max-hold deposit (W:864-868) | energy of repeated hits not summed; keeps hue purity | P6 (K1 "never add palette colours") | n/a | intensity-only soft sum | L | A20 |
| C36 | Path-P ramp `d/80` (W:888) | adds audio-independent colour; palette 31 has 17.5 % near-black positions and a black→yellow seam (D02 `vp_numbers.json`) | transplant of K1 `freq_prog` (pixel = frequency) into a waveform (pixel = time) | n/a (taste) | expose k; seam rule | M–H | A14 |
| C37 | Mirror (W:876-888) | 50 % spatial redundancy; hazard: mode 32 with mirror off leaves the left half dark (D02-L13) | product identity + porting accident | n/a | fix hazard; symmetry is taste | M | A17 |
| C38 | Fade τ 290/125 ms | — keep (Captain's ruling: fade to black on cuts was an improvement) | aesthetic (approved) | keep | — | — | — |
| C39 | Warmth + white balance after the colour tests (PR:281-294) | authored hue changed after the metric tap | inherited Emotiscope display controls | WB maybe needed; warmth is taste | score after it | M | A04 |
| C40 | Linear code-to-light; WS2816 internal curve unmeasured | unknown | accident (no K1 degamma `38428a82` ported); donor modes already gamma'd | measure first | camera ramp | M | A21 |
| C41 | Brightness cap | photon headroom only; code resolution kept | owner rule (binding; conflicted, §8) | binding | none | — | — |
| C42 | 100 Hz tick (PRH:40) | 0–10 ms wait (mean 5) | inherited donor profile | yes (wire allows ~196 Hz) | small | L–M | A26 |
| C43 | Donor modes RGB8×257 (PR:411-420) | 256 levels on donor modes | inherited 8-bit LED | yes | default flip | L | — |
| C44 | Donor 128 → 160 px | spatial resample | inherited | yes | port | L | — |
| C45 | 8-bit palette stops | 256 levels at authoring vertices | inherited FastLED authoring | yes | 16-bit stops | L | — |
| C46 | Acoustic port / capsule bass response (S3) | unknown; more CPU cannot restore missing acoustic input | unknown | n/a | measure (A15) | ? | A01, A15 |
| C47 | Mode-11 180→96 nearest-sample display (A:216-225; S3) | displayed waveform drops samples | geometry inheritance | yes | antialiased adapter | L (mode 11) | — |
| C48 | Phase discarded (magnitude only, A:60-69; S3) | onset/timbre cues | inherited | yes | event branch instead (A07) | M | A07 |
| C49 | Three-hop tempo peak hold (A:289-297; S3) | event time collapses to a 22.5 ms bucket | inherited | yes | keep timestamps | L–M | A25 |
| C50 ⚑ | Silence entry RMS 0.04 (WH:14) with 5 s dwell (AH:40) vs level floor −33 dBFS | [INFERENCE] music held between −33 and −28 dBFS RMS for 5 s is declared silence and drained to black, though the level map would draw it | inherited K1 reconstruction | taste/safety | expose and test (A13) | M | A13 |
| C51 ⚑ | Colour analysis follows board 0's Mood only (A:302-306) | [INFERENCE] per-board Mood changes board 1's tempo but not its colour; both boards read board 0's `linear_shape` | port wiring | n/a | make analysis Mood-independent | L–M | A07, A10 |
| C52 | Admission/freshness/quiet gates (PR:258-278; W:120-142) | several independent paths can block new light without saying which | inherited | gates stay | reason word (A13) | M | A13 |

**What the ledger says about "why compress?"**

- [FACT] The original compressions were real budget trades, mostly made by SensoryBridge 4.0.0 (July 2023: 24.4 → 12.8 kHz, 96-sample chunks, fixed point; SB-1 `a17ec09`) and kept on the S3 because its core was full (ADR-0008: 7,712 µs of an 8,000 µs gate).
- [FACT] On the Titan only C05, C11, C21 (decimation) and arguably C10 carry a cited budget reason, and none of those reasons applies at about 1 ms per 5 ms. No verifier found a budget origin for any VP constant.
- [INFERENCE] The steps that stop the drop hitting harder (C01, C03, C17, C18, C29, C30, C32, C33) are accidents, inheritances or test-fairness choices. They persisted because each port froze them for host bit-parity, not because the hardware needs them.
- [FACT] Two compressions are deliberate looks backed by device findings and must not be undone blindly: the symmetric 0.75 spectrogram smoother (an asymmetric slow release was rolled back on 2026-05-20 for light-guide washout, D04 VERIFY), and the 405 px/s scroll (Captain-calibrated). A fast attack is safe to test; a slow release is the rolled-back fault.

---

## 4. Failure → lever map and recurring patterns

### 4.1 Titan failures

| Failure (evidence) | Levers |
|---|---|
| Drop does not hit harder than the build (Captain, RUNS/STATUS "still a no") | A02, A03, A06, A08, A01, A11, A09, A10 |
| BOOM replay: root 126.4 → 129.8 px; brightness rises (deposit 0.69 → 1.80) but length does not release (D02 `clamp_occupancy.json`) | A06, A02, A03 |
| **Release candidates X1–X3 and K1–K3 failed EXT-v2, but the motion worked when they fired.** On BOOM source, X3 reached extent 1.00 at T90 0.04 s, jump 1.00; it failed only `build_contraction_kept` (it fired during the build). On BOOM room it fired 0.19 s early on the riser. On Sonic @54.7 it never fired (`P6P/results/ext/ext-v2-X3.json`; X1 similar). K3 released edge-to-centre on BOOM source and Sonic @27.7 with lag +0.09–0.10 s against 0.1 s-quantised anchors (`STATUS.md:176-177`) | A02 (look, now separable), A18/A07 (timing) |
| Known negative: `b5595c35` fails EXT-v2 everywhere (BOOM source extent 0.46 → 0.53, T90 ∞); positive plant passes (`ext-v2-b5595c35{,-planted}.json`) | gate machinery is live (A02 uses it) |
| Drop renders dimmer: harmony moves into palette dark ends (P3) | A14, A10 |
| Colour over-averaging (v8: one sample per frame + RGB mix + RGB EMA) | fixed by path P; **now on the device** (`STATUS.md:201`) |
| v9 `b5595c35` colour moved in 1 % of seconds (FORENSICS:113) | path P (done) |
| Path-P loud-end colour stall (post-knee centroid) | pre-knee `linear_shape` (done) |
| PL14-B14: product default did not select the fix; host-005 had the wrong gains (`STATUS.md:198`) | repaired in `d557f06` with an equivalence test; host-006 programmed with 127-field readback match |
| Three drop detectors: v1 hit BOOM only, v2 Carte only, v3 none; sub arrives 72–117 ms after the musical drop (DETECTOR v1/v2/v3) | A07, A18, A24 |
| Tempo half/two-thirds time; P7 beat stage best 35/326 (`STATUS.md:200`) | A24 |
| C1 light-ratio metric passed while the eye said no | A04 |
| Dopex dark at 76.3 s before the silence flag (untraced gate) | A13 |
| Native first-boot dark/white, ELC/DMA stall, DIN halves swapped, scene alias (D05 T19–T23) | output identity tests (A04) |
| P5 camera could not see per-click flashes | A07, A03 |

### 4.2 S3 and upstream failures still present in some form

| Failure | Where it still hides on the Titan |
|---|---|
| "12 colours that repeat … 81.25 % wasted" (`note_colors[i%12]`) | C26 fold — never return to hue wheels |
| 11 Jun 2026 "tending towards white … not the whole range of the palette" | fixed by HD sampling (SB `d408433`, K1 `c2738b88`) and path P |
| "Palettes duller, worse louder" (post-gate chroma + held-hue freeze) | C18/C19 read-after-squash; K1 `ae5d90a` |
| "Louder music → dimmer effects" (#72835) | C18 |
| Main RPL "30–40 % dimmer, sluggish, very short leash" (Aug; #93763) | C17: the same 0.25/0.005 constants |
| Lightwave 9 Feb "unable to display the palette colours accurately" | brightness-only trails = path P (Lightwave `947bd481`) |
| Melodic Bloom "twitchy"; PULSE BLOOM strobe ("never global brightness on the beat", #58407) | guard on A03, A09, A16 |
| Half-time 74 vs 128 BPM | C21 |
| Dithered 8-bit flicker at dim tail | C43/C45 |
| Harness PASS while the device was dark; rainbow test palette | A04 |
| Waveform lit ~40 of 160 LEDs; lacked "spirited galloping" | C32, C34 |
| Washout from asymmetric slow release (SB9 rollback) | do not repeat (C15/C24) |

S3 §5 carries the complete recovered D05 inventory (24 Titan + 17 predecessor entries) with lever links. It cites `/tmp/d05_result.json`, which is outside the run folder and may disappear; the inventory table in S3 is the durable copy.

### 4.3 Recurring patterns and where each still hides

| Pattern | Already paid for | Still hiding |
|---|---|---|
| P-1 **Collapse to one number too early** | 16-slot palette; one colour per frame | 80→12→1 colour centroid; 80 changes → 1 novelty → tempo; one `peak_scaled` → extent; level sets amplitude **and** width |
| P-2 **Read after the squash** | post-gate chroma freeze; post-knee stall | novelty after AGC/knee; peak clamp; loud guard and HPF before everything |
| P-3 **Louder gives less** | K1 louder → dimmer; v8 louder → faster fade | extent polarity; follower normalising the surge; colour into dark palette ends; ceiling clip |
| P-4 **Blind edges of the spectrum** | hi-hat bands zero at 12.8 kHz (fixed P4) | HPF cuts 40–220 Hz; 40–110 Hz only as an unused scalar; treble windows cover half a hop; colour ignores bins 60–79 |
| P-5 **Smoothers stacked across containers** | K1 "sluggish"; Lightwave cascade reduction | 0.3 EMA → Mood → 0.75 mix → chroma_max ≈ 0.1 s colour lag; 16 → 23 ms extent cascade; no container owns the total |
| P-6 **Rate mismatch drops events** | — | 133 Hz hops → 100 Hz frames; three-hop tempo hold |
| P-7 **Inherited constants re-frozen for parity** | int64 fix kept on K1 | Mood 100 Hz basis; rounded k; `>>6`; 88 BPM prior |
| P-8 **Host model is not the product** | PL14-B14; host-005 gains; harness PASS while dark | metrics tap before warmth/white balance; wire curve unmodelled |
| P-9 **Bit-depth narrowing** | REPLICATE8 on K1 WS2816 | donor RGB8×257; 8-bit stops |
| P-10 **Darkness without attribution** | — | four or more gates; Dopex 76.3 s; silence threshold above the level floor (C50) |
| P-11 **Built but switched off is treated as absent** | — | release/accent; `sub_rms`; AudioPipeline kick/snare/hat (compiled out, `k1_live_runtime.cpp:165/318`); donor VU and 96-tempo bank; second mic; 20-bit PDM; sound-detection IRQ; U55; M33 |
| P-12 **Output identity faults** | DIN swap; scene alias | mirror-off hazard; top board on a separate engine with reversed halves |
| P-13 **Prediction from a wrong clock** | v3 downbeats −380…+188 ms | unextrapolated phase; novelty delay inside the PLL |
| P-14 **The wrong code path gets the blame** (integrator) | — | the parked kernel's int32 wrap was attributed to the live analyser by D03/D04 and S1; check the call chain before repairing |
| P-15 **A protection becomes the look** (S3) | — | loud guard, follower, clamps and gates now shape the show |
| P-16 **Geometry counted as music** (S2) | — | path P's `d` ramp passes a spread metric with music frozen; the frozen-position plant only guards the motion clause (`TOLERANCE-FROZEN-P6-PAL-HD-v2.json` "c_motion") |

---

## 5. Ranked opportunity portfolio — PROPOSAL — NOT APPROVED

**One list.** Score = visual impact × novelty ÷ effort (each 1–5). Ties are broken first by relevance to the Captain's open verdict ("the drop does not hit harder"), then by lower effort. Every experiment is frozen with `scripts/freeze_tolerance.py freeze` before any result exists, run through the same consumer for baseline, candidate and planted fault, and then `freeze_tolerance.py verify`. A planted fault that does not go red makes the experiment void. Known-negative rule: `b5595c35` must fail any "hits harder" criterion before that criterion is trusted (it already fails EXT-v2). Every run includes a real silence segment whose packed codes must be 0. Device steps need the lane owner's existing authority; nothing here grants it.

**Ship path for anything in this list.** (1) In source and on silicon today: `d557f06` / host-006 only. (2) Remaining: host experiment with frozen gate → candidate image with the new behaviour behind a flag whose default reproduces host-006 → product-equivalence test → program → identity readback → wire snapshot matches the scored frames → Captain A/B where taste decides. (3) Who acts: the P6/P7 lane owner or executing agent; the Captain only for the taste A/B. (4) Stamp: `freeze_tolerance.py verify` clean, device readback of the new build_id, wire-snapshot equivalence, and the Captain's recorded verdict.

| Rank | ID | Lever | I | N | E | Score | Drop? | S1 / S2 / S3 sources |
|---|---|---|---|---|---|---|---|---|
| 1 | A01 | Re-derive the mic's bass-cut filter for the real rate | 5 | 5 | 2 | 12.5 | yes | T01 / S2-07 / L12 |
| 2 | A02 | Fire the existing release at hand-marked drop times | 5 | 4 | 2 | 10.0 | yes | T06 / S2-01 / L08 |
| 3 | A03 | Tension depth: reach cap and extent attack | 4 | 2 | 1 | 8.0 | yes | T15 part / S2-13, S2-14 / L05 part |
| 4 | A04 | Score colour and light at the packer, after warmth and white balance | 4 | 2 | 1 | 8.0 | — | T03 / S2-19 / L01 |
| 5 | A05 | Order colour by the circle of fifths (taste) | 2 | 4 | 1 | 8.0 | — | T02 / §7 / L18 part |
| 6 | A06 | Extent against a section-length reference, not the last 2 s | 5 | 3 | 2 | 7.5 | yes | T13 / S2-02 / L05 |
| 7 | A07 | Fast onset tap before Mood, AGC and knee | 5 | 3 | 2 | 7.5 | yes | T19 / S2-03 / L07 |
| 8 | A08 | Count the loud guard, then keep absolute level | 4 | 3 | 2 | 6.0 | yes | T05 / S2-08 / L11 |
| 9 | A09 | Let the existing sub-bass level move space | 4 | 3 | 2 | 6.0 | yes | T11 / S2-16 / L04 |
| 10 | A10 | Colour arrives with the light (Mood A/B, snap at release) | 3 | 4 | 2 | 6.0 | yes | T10 / §7 / — |
| 11 | A11 | Headroom reserved for the drop (level ceiling, deposit gain) | 4 | 3 | 2 | 6.0 | yes | T14 / — / L17 |
| 12 | A12 | Pitch honesty: exact bin centres and `chroma_range` 80 | 3 | 2 | 1 | 6.0 | — | T07 / S2-10, S2-11 / L14 |
| 13 | A13 | Silence and darkness truth (gate reasons, silence threshold) | 4 | 3 | 2 | 6.0 | — | T03 part / §8 / L10 |
| 14 | A14 | Palette spread k and the dark seam (taste) | 3 | 4 | 2 | 6.0 | — | T09 / S2-12 / L18 |
| 15 | A15 | Second mic as a witness lane | 3 | 4 | 2 | 6.0 | — | E3 / S2-09 / L12 |
| 16 | A16 | Timbre shapes the deposit instead of flashing it | 3 | 4 | 2 | 6.0 | — | — / §7 / L03 |
| 17 | A17 | Two boards with complementary roles | 4 | 4 | 3 | 5.3 | — | §7 A11 / §7 / L09 |
| 18 | A18 | Tension state machine / qualified drop release | 5 | 5 | 5 | 5.0 | yes | T20 / S2-05 / L13 |
| 19 | A19 | Runtime A/B knobs so taste tests need no reflash | 3 | 3 | 2 | 4.5 | — | E4 / S2-01 note / — |
| 20 | A20 | Fractional transport and swept deposit | 3 | 3 | 2 | 4.5 | — | T17 / S2-21 / L15 |
| 21 | A21 | Measure the WS2816 light curve before compensating | 3 | 3 | 2 | 4.5 | — | T18 / S2-18 / L16 |
| 22 | A22 | Wake idle machinery (kick/snare/hat, donor VU, tempo bank) | 4 | 3 | 3 | 4.0 | — | T12 / S2-15 / — |
| 23 | A23 | Treble time coverage | 2 | 4 | 2 | 4.0 | — | T16 / — / — |
| 24 | A24 | Tempo clock repair (octave, phase, warm-up) | 4 | 3 | 4 | 3.0 | yes (enabler) | E1 / S2-04 / L19 |
| 25 | A25 | Event chronology through publication | 4 | 2 | 3 | 2.7 | — | — / — / L20 |
| 26 | A26 | Render on the hop / faster cadence | 2 | 2 | 2 | 2.0 | — | E2 / S2-17 / — |
| 27 | A27 | Wide RMS accumulation (hygiene only) | 1 | 2 | 1 | 2.0 | — | — / — / L02 |

**Removed from the portfolio, with reasons:** int64 Goertzel (S1 T04, S2-20): the live analyser is already int64 with 155× headroom (C08). Slow-release spectrum smoother (D04 L03): repeats the 2026-05-20 washout rollback. 2,000-sample cap: binds on no current bin. Raising in-house analysis to 24 kHz: already done in host-006.

### Lever records

Fields per the brief: container; stage; controls; current v8/v9; information; origin; still needed; manipulation; interactions; I/N/E; risk; smallest decisive experiment (frozen criterion + planted fault).

**A01 — Re-derive the mic's bass-cut filter** (5/5/2 = 12.5)
- Container: AP (hardware). Stage: PDM HPF registers PDHFCS0/PDHFCK1 per channel, `hal_data.c:617-619`; PDM:764-781 does not touch them. Controls: brightness and motion of kick and sub-bass, and everything downstream that reads them (`sub_rms`, follower, release surge).
- Current (v8 and v9 identical): s0 0x3F61, k1 0x3EC1, h {0x4000, 0xC000}, the manual reset values (D10 VERIFY 1). Information: [INFERENCE, IC] 259 Hz first-order corner at the 82.8 kHz HPF clock; −13.7 dB at 55 Hz, −8.2 dB at 110 Hz. The integrator's model is the documented structure (MAN Table 50.12, Figure 50.4) with these coefficients; it reproduces the manual's "cut-off = 100 Hz" (MAN 211471) at 32 kHz.
- Origin: accident. Only SINCDEC/SINCRNG were retuned for the 41.4 kHz rate; the manual warns post-sinc filter characteristics "might change" away from 32 kHz (MAN 212836). Still needed? A DC block yes; a 259 Hz corner no. Cost on M85: zero.
- Manipulation: k1 0x3FE7 / s0 0x3FF4 (20.1 Hz corner, −0.55 dB at 55 Hz; IC `candidate_20hz`); apply to the witness lane first (A15), then the programme lane. The compensation and half-band filters also run off-design; model them before claiming flat response.
- Interactions: A09 (sub-bass is cut before `sub_rms`), A06/A02 (release surge), A08 (more low-frequency energy raises peaks and guard engagement), C46 (the capsule and speaker may also roll off bass).
- Risk: medium. The capture path is the most fragile (PL14-A4); a wrong coefficient adds DC or rumble. Reversible by one constant. Disagreement S1 vs S2: S1 wanted to retune; S2 wanted to measure first. Settled: the host model now supports S1's figure (D01's 128 Hz assumed the filter runs at Fout, which contradicts MAN 213325-213327), but the device measurement is still required before the programme lane changes.
- Experiment: (a) host, done in part (IC): freeze that the model gives 100 ± 2 Hz at 32 kHz and ≥ 6 dB loss at 55 Hz at 82.8 kHz; planted fault: k1 − 1 LSB must move fc by > 0.5 Hz. (b) host replay (experiment 2 in §9). (c) device, paired lanes under lane authority: lane B at 20 Hz, lane A unchanged, log sweep 30–1,000 Hz; frozen: B/A ≥ +6 dB at 45–65 Hz and within ±1.5 dB at 800–1,000 Hz; planted: lane B at fc ≈ 1 kHz must read ≤ −6 dB at 45–65 Hz. No Captain look is needed; the instrument decides.

**A02 — Fire the existing release at hand-marked drop times** (5/4/2 = 10.0)
- Container: map. Stage: `updateReleaseV9` (W:752-795) and the pull `position = 80 + (position − 80)(1 − release_)` (W:977). Controls: space and motion at the drop.
- Current: v9 release off (WH:69), config `inline constexpr` (WH:79); the host probe can set it (`tests/host/p6_effect_probe.cpp:138-146`). v8 has no release.
- Information: loudness and "this is the drop" are the same number today, so the build and the drop draw nearly the same length.
- Origin: the outward map is the K1 centre-origin look; the release is a P6 candidate left off because its triggers failed. Still needed? Yes if the Captain's 14 Oct definition is the spec; CPU cost negligible.
- Manipulation: drive `release_` from the frozen EXT-v2 anchors (BOOM 55.7, Sonic 27.7 and 54.7 s; `TOLERANCE-FROZEN-P6-EXT-v2.json`) instead of a live detector. Shape options: the existing instant attack / 0.6 s release, then the §7 A3 ladder.
- Interactions: A03 deepens the build contraction the release inverts; A06 decides whether the build sits compressed; A07/A18/A24 are how the time gets found without annotation; A10 carries colour with it.
- New evidence (integrator): X1–X3 already reach extent 1.0 within 40–70 ms of the BOOM drop when they fire (§4.1). The visual half is close; the timing half failed.
- Risk: the lag clause is trivially satisfied by an oracle time, so it is not evidence in this run. A false full release on a riser is worse than a miss once automated (A18).
- Experiment: §9 experiment 1.

**A03 — Tension depth: reach cap and extent attack** (4/2/1 = 8.0)
- Container: map. Stage: W:974-976 (`0.7F`, 16 ms and 23 ms EMAs). Controls: how far the build compresses toward the edges and how fast the root moves.
- Current: a fresh deposit stops at px 136 (≈ 140 with width); t90 ≈ 70–76 ms (D08 VERIFY).
- Origin: unknown; not a budget cut. Still needed? Not for CPU; the look is taste.
- Manipulation: cap 0.7 → 1.0 (deeper build contraction); cascade 16/23 ms → one ≤ 10 ms attack with the release unchanged (not the SB9 slow release).
- Disagreement S2 vs integrator: S2 ranked the cap #2 as a drop lever. With today's outward polarity a higher cap moves loud deposits further out, which **shortens** the lit extent. It is a contrast amplifier only when paired with A02. Ranked here as A02's partner, not alone.
- Interactions: A02, A06, A20 (the outer pixels are painted by the scroll today).
- Risk: a zero attack twitches (rejected Melodic Bloom look).
- Experiment: host, with release on at labelled times (after §9 exp. 1). Frozen: EXT-v2 `jump` ≥ 0.9 on all tuning cases and `E_climax` ≤ the cap-0.7 run's `E_climax` − 0.05 (deeper contraction), step-response t90 ≤ 30 ms with ≤ 6 px per-frame jump on a stationary noisy tone. Planted faults: the 16/23 ms pair must fail t90; cap 0.7 must fail the contraction clause.

**A04 — Score colour and light at the packer** (4/2/1 = 8.0)
- Container: output/instrument. Stage: PR:281-297, 690-738. Controls: confidence in every other lever.
- Current: PAL-HD-v2 scores renderer frames before warmth/white balance; the product-equivalence test pins path P and gains (`tests/host/test_p6_product_path_p.py`). The frozen-position plant already turns the motion clause red (`TOLERANCE-FROZEN-P6-PAL-HD-v2.json`), so S1's "add an audio-blind fault" is half done; what is missing is a spread clause that cannot be passed by the `d` ramp alone, and a packer tap.
- Manipulation: reconstruct the packed 48-bit lanes ÷ gains; score luma (Rec.709) and hue there; compare against the **intended transformed palette**, not the raw palette (S3's caution: otherwise intentional white balance becomes a false failure).
- Experiment: frozen: packer reconstruction within 1 LSB of `wide_`; exact zero preserved; > 256 distinct codes on a ramp at the installed gain. Planted: old gains (host-005), restricted palette default, an RGB8 intermediate, and reading 8-bit preview bytes must each fail its clause.

**A05 — Circle-of-fifths colour order** (2/4/1 = 8.0; taste)
- Stage: chroma index i → 7i mod 12 before the centroid (W:172, 618 per D12). Related keys get related colours. Palette stays the source (no rainbow).
- Experiment: frozen: share of frames with centroid strength ≥ 0.08 rises ≥ 1.5× on tonal tracks; colour still moves in ≥ 90 % of seconds. Planted: a random fixed permutation must not reach the rise. Then §7 A7.

**A06 — Extent against a section-length reference** (5/3/2 = 7.5)
- Stage: A:247-254. Controls: length. Current: follower 0.25/0.005 (τ 30 ms / 1.99 s); BOOM build→drop VU +8.4 dB → `peak_scaled` +7 % (C17).
- Origin: inherited K1 "short leash". Manipulation: publish an unnormalised long-window level beside the normalised one; drive extent from a two-timescale contrast against an 8–15 s reference, or slow the follower while a build is armed. Not a slow release on the light trail.
- Risk: every chorus looks like a drop; room-level independence must survive.
- Experiment: release off (isolate). Frozen: `peak_scaled` (or the new reference ratio) at drop + 80 ms ≥ 1.5× its value at build − 0.5 s on BOOM source and room; elsewhere extent ≤ current. Planted: current constants must fail; follower release = attack (30 ms) must fail.

**A07 — Fast onset tap before Mood, AGC and knee** (5/3/2 = 7.5)
- Stage: a second branch from `magnitude_average_` or raw bins (A:274) plus 2 ms band envelopes, published per hop with timestamps; keep the slow path for colour. Controls: timing of colour change, tempo, any release trigger.
- Origin: novelty after compressors is inherited. M85 cost: ≈ 0.5 % [INFERENCE, D13].
- Experiment: frozen: click-corpus onset |error| median ≤ 3 ms, p95 ≤ 8 ms; a +12 dB step after a loud plateau gives a fast-tap peak ≥ 2× the post-knee novelty peak; 2 s steady tone stays < 0.25 of step peak; identical PCM at two Mood settings gives identical event times. Planted: tap re-pointed at post-knee novelty; Mood stage inserted into the tap. Do not use the BOOM 0.014/0.038 pair (that is 110–208 Hz accent flux).

**A08 — Count the loud guard, then keep absolute level** (4/3/2 = 6.0)
- Stage: SENS:9-22; PDM:625-635. Partial telemetry exists: `k1_pdm_target_last_hop_gain_q8()` and clip counters (PDM:147-148, 626, 631-633, 1000). Missing: a per-slot engagement count.
- Experiment: log-only arm (no LED change): frozen: report the fraction of slots with gain < 4096 on BOOM room and a 0 dBFS tone; planted: a synthetic slot with peak 4,000 must report reduced gain. Behaviour arm (later): publish pre-guard peak/RMS; frozen: a +6 dB step crossing raw 1,875 is recovered within ±0.5 dB; planted: post-guard RMS labelled raw must fail. Widening to 20-bit must re-run the C08 headroom check.

**A09 — Let the existing sub-bass level move space** (4/3/2 = 6.0)
- Stage: SB:3-24; A:240; consumers off (WH:76). Map `sub_rms` to width or a bounded outward/inward warp, not brightness (strobe law #58407).
- Interactions: depends on A01 (the band is cut 8–16 dB today) and C46.
- Experiment: frozen: a 55 Hz (or 66 Hz) burst gives 4–8 px displacement with no global intensity multiplication; an equal-VU 220 Hz or 1 kHz control gives ≤ 1 px; silence adds no light. Planted: `K1_SUBBAND_PLANT_WRONG_CENTRE` (SB:23-24, 660 Hz) must silence the 55 Hz arm; a brightness-gain implementation must fail the no-intensity clause.

**A10 — Colour arrives with the light** (3/4/2 = 6.0)
- Stage: Mood LPF (A:313-326) and the path-P offset (W:630-657). Path-P colour t50 ≈ 75–113 ms [INFERENCE, D13], so the first ~0.2 s of a drop wears the build's colour.
- Asymmetric find: in-house Mood is live-settable per board on the resident image (`k1_live_runtime.h:87-88`; `fixture_app.cpp:1666`), so the colour-speed question needs no reflash. Board 0's Mood sets colour for both boards (C51).
- Manipulation: (i) Mood 0.05 vs 0.5 (τ 80 → 20 ms); (ii) at release, snap the deposit offset to the pre-Mood centroid and let the front carry it.
- Experiment: §9 experiment 3 for (i). For (ii): frozen: centre-pixel offset within one frame of fire equals the target (|d| ≤ 0.004) and drop-chapter luma ≥ build-chapter luma; planted: no snap; chapter forced to the dark end.

**A11 — Headroom reserved for the drop** (4/3/2 = 6.0; taste on the build)
- Stage: W:940-949 (ceiling −9 dBFS), WH:38-39 (gain 0.580). 34–35 % of BOOM drop frames already sit at l = 1.
- Risk: most of D12's synthetic 9.29× contrast came from dimming the build (D12 VERIFY); never manufacture the ratio. No floor.
- Experiment: frozen: every +3 dB input step gives a positive intensity change up to the declared ceiling; wire-snapshot drop p95 luma ≥ 1.5× build p95; song mean light within ±10 %; nothing above the cap. Planted: the −9 dBFS clamp must fail the upper ladder step; a cap + 1 write must fail.

**A12 — Pitch honesty** (3/2/1 = 6.0)
- Stage: G:121-128 (rounded k: bin 0 at 104.7 Hz), WH:126 (`chroma_range` 60). Keep the legacy configuration as a named parity oracle.
- Experiment: frozen: a 110 Hz tone peaks in bin 0 with resonator within 0.5 Hz of 110; ≥ 95 % of frames assign pure tones 110 Hz–3.3 kHz to the right pitch class; an 8 kHz tone shifts the offset ≥ 0.15 turns with range 80; noise-only drift ≤ 0.02. Planted: integer k; range 60. K1's earlier eyes-on rejection of true centres (`7cd9336`) still needs a fresh look.

**A13 — Silence and darkness truth** (4/3/2 = 6.0)
- Stage: A:255-266 (RMS silence, enter 0.04, exit 0.08, 5 s dwell), PR:258-278 admission, W:120-142 freshness, W:798-803 drain. Integrator finding C50: the silence entry (−28 dBFS) sits above the level floor (−33 dBFS).
- Manipulation: one telemetry word per frame naming the gate that closed; test whether quiet music between −33 and −28 dBFS drains to black.
- Experiment: frozen: exact packed zero within 1 s of digital-zero input; every dark frame of the Dopex replay attributed to exactly one gate; a −30 dBFS music passage held 6 s is reported (pass/fail declared by the Captain's ruling on quiet music, not by the agent). Planted: a 1/1024 floor must fail black; an unlabelled gate must report "unattributed".

**A14 — Palette spread k and the dark seam** (3/4/2 = 6.0; taste)
- Stage: W:888 (`d`). Expose p = o + k·d; place the dark end at the edges or use a seam-aware rule; never a floor. S2's experiment proves whether a spread metric counts the ruler.
- Experiment: frozen: masked fraction (lit but output < 5 % of palette max) ≤ 1 %; at k = 0.25 hue-band count ≤ 50 % of k = 1 with offset motion unchanged; at least two non-black palette regions during active music. Planted: palette 31 unchanged under P (≈ 17 %, red); k = 0 must fail the spread clause; an endpoint jump must fail continuity.

**A15 — Second mic as a witness lane** (3/4/2 = 6.0)
- Stage: PDM:607-610. Use only as an instrument for A01/A08; never sum it into the show without an acoustic test.
- Experiment: frozen: the two lanes' 300 Hz–3 kHz envelopes correlate ≥ 0.8 over 5 s and the programme lane keeps publishing non-zero peaks during music. Planted: dropping the programme lane in the harness must fail the publish clause.

**A16 — Timbre shapes the deposit** (3/4/2 = 6.0; taste)
- Stage: `depositV9` (W:809-867). Use attack slope, centroid and flatness: compact head for percussion, broader body for sustained sound. Intensity and palette ownership stay separate.
- Experiment: frozen: on equal-RMS percussion/sustained fixtures, width separation ≥ 2 px, integrated intensity within ±5 %, exact silence zero. Planted: a constant timbre vector must fail separation.

**A17 — Two boards with complementary roles** (4/4/3 = 5.3)
- Both boards are on in host-006 (receipt `:411`); the top board uses its own engine and reversed halves. One shared musical clock; different material (one carries release geometry, the other harmonic texture).
- Experiment: frozen: changing board 0's artistic control leaves board 1's bytes unchanged under fixed shared features; a shared release timestamp reaches both submissions within one frame. Planted: a shared renderer pointer; swapped top halves.

**A18 — Tension state machine / qualified drop release** (5/5/5 = 5.0)
- Design (D11 §3): FLOW → BUILD → ARMED → PROVISIONAL (inward accent ≤ 0.12, no global brightness) → RELEASE (size = stored charge) → RECOVER; QUIET drains to black. A false full release costs more than a miss. Prediction stays off until A24 passes.
- Evidence: sub-bass arrives 72–117 ms after the musical drop, so a sub-only trigger cannot meet +50 ms; surge triggers fired on climaxes and risers.
- Experiment: frozen (unchanged v2/v3 hit rule): ≥ 5/6 drops in [0, +50 ms], zero false full releases including within 2 s of annotated non-drop climaxes. Planted: charge forced to 1; 200 ms-late fire; sub-only wait. Holdout must be the Captain's corpus (Carte Blanche is spent).

**A19 — Runtime A/B knobs** (3/3/2 = 4.5)
- Release, accent, cap and ceiling are compile-time today (WH:52, 79), so every taste A/B except Mood costs a flash and ~45 s of darkness. Add them to the existing live schema with hash-bound defaults equal to host-006. Experiment: frozen: default readback equals host-006 (127/127); a planted non-default boot value must fail the equivalence test.

**A20 — Fractional transport and swept deposit** (3/3/2 = 4.5)
- Stage: W:963-1019. Fill the stride between frames in palette-position space (never RGB). Re-freeze matched-light gains if the deposit changes.
- Experiment: frozen: trail continuity (5-px min/max) ≥ 0.8 at l = 0…1; a 0.25 px/frame translation with centre-of-mass error ≤ 0.05 px. Planted: integer steps (≤ 0.6). Then one fixed-exposure camera frame decides whether the plate already hides beading (instrument, not the Captain).

**A21 — Measure the WS2816 light curve** (3/3/2 = 4.5)
- Measurement, not a fix. Locked-exposure ramp on one board; donor modes already apply gamma (`emotiscope_port.cpp:134`), so any degamma is for in-house TRUE16 modes only. Code 0 stays 0.
- Experiment: frozen: fitted exponent with 95 % CI width ≤ 0.2; code 0 indistinguishable from the dark frame. Planted: a synthetic linear ramp must fit γ = 1.0 ± 0.05; +1/1024 on every code must fail the black clause.

**A22 — Wake idle machinery** (4/3/3 = 4.0)
- AudioPipeline kick/snare/hat/saliency runs only outside `K1_EMOTISCOPE_PORT` (`k1_live_runtime.cpp:165, 318`). Donor VU and the donor 96-tempo bank are GPL-lineage bench code; licence check before product use. Measure M85 cost first (PL14-B5: CMSIS was slower).
- Experiment: frozen: kick events hit ≥ 90 % of a synthetic kick track within ±20 ms; `pipeline_.process` p50 < 1.0 ms or defer. Planted: all-zero onset output; kick/snare label swap.

**A23 — Treble time coverage** (2/4/2 = 4.0)
- Evaluate short treble windows several times per hop with peak-hold. Frozen: 1,000 random 2 ms 8 kHz clicks detected in bins 70–79 ≥ 95 %. Planted: newest-window-only.

**A24 — Tempo clock repair** (4/3/4 = 3.0; enabler for A18)
- New evidence: P7 beat stage best 35/326 on GATE-SYNTHETIC-v1; flat prior 29; four-on-the-floor 15 %; planted 88 prior does not discriminate; autocorrelation ~149k cycles per hop (`STATUS.md:200`). Disagreement S2-04 vs P7: S2 expected prior-off to restore 138/142/150 BPM; it does on those tracks, but it fails the gate as a whole. Design work, possibly the U55 option, is needed.
- Experiment: the existing 326-case gate unchanged; no accepted wrong octave; zero confident lock on silence. Planted: 88 BPM prior; half-tempo output.

**A25 — Event chronology** (4/2/3 = 2.7)
- Bounded event queue with event time, availability time, epoch and expiry; continuous features interpolated; never add the wire delay twice. Frozen: impulses at every AP/render phase are consumed exactly once, timing error ≤ one analysis sample. Planted: latest-only overwrite; one-hop timestamp shift.

**A26 — Render on the hop / faster cadence** (2/2/2 = 2.0)
- The 100 Hz wait averages 5 ms; the smoothers dominate latency. Frozen: p50 publish-to-submit ≤ 2 ms in the kicked arm. Planted: tick on.

**A27 — Wide RMS accumulation** (1/2/1 = 2.0; hygiene)
- S3 ranked it #2. Settled by IC: the error only matters below −38 dBFS, under both the level floor and the silence threshold. Do it only alongside another AP change. Frozen: ≤ 1 % RMS error vs double for amplitudes 64–16,000. Planted: per-sample truncation must fail at amplitude 64.

---

## 6. Target architecture — PROPOSAL — NOT APPROVED

**Principle carried over from the colour fix:** keep vectors and timestamps across container boundaries, and collapse to a light value only at the pixel, once. Separate three time layers: fast events (≤ 10 ms), musical context (beat/bar/section), and presentation smoothing (owned by the consumer). Extend the existing pipeline; do not add a new orchestrator or dashboard.

```text
Capture (both mics, HPF re-derived, gain provenance)
   ├── fast tap: raw level, sub, band flux, onset events (timestamped) ─┐
   └── slow tap: 80-bin tonal shape, Mood/AGC-conditioned spectrum ───┤
                                                                       ↓
                 shared musical clock + tension state, with confidence
                                                                       ↓
          mapping per board: ordinary response + release state + colour chapter
                    board 0 material            board 1 material
              (intensity + palette position)  (intensity + palette position)
                                                                       ↓
          HD palette per pixel → display transforms → TRUE16 packer → existing DMA
```

| Container | Runs on | Rate | Output contract | State |
|---|---|---|---|---|
| A0 Capture | M85 ISR/DMA (mic DMA1/2 unchanged) | 41.4 kHz | int16 now (int32 later), HPF ≈ 20 Hz, per-slot raw peak, applied gain, clip counters | DMA rings |
| A1 Resample | M85 | 24 kHz, 7.5 ms hop (as today) | hop + capture timestamp (existing epoch/sample clock) | poly24 |
| A2a Tonal bank | M85 | per hop | `tonal[80]`, exact-k option, int64 as today; legacy configuration kept as parity oracle | Goertzel state |
| A2b Event bank | M85 | sub-hop envelopes, per-hop publish | `band_env[8]`, `band_flux[8]` pre-AGC, `events[]` {band, t_event, t_available, strength} | biquads, event ring |
| A2c Level/timbre | M85 | per hop | raw RMS, unnormalised long-window level, crest, centroid, flatness | followers |
| A3 Context | M85 | 44 Hz / per beat | tempo candidates {½, 1, 2, ⅔, 3/2} + confidence; beat phase + variance; tension vector; section memory (≈ 512 B); slow room reference. **Independent of per-board Mood** | SRAM/DTCM |
| M Feature frame | shared memory, existing generation counter | per hop | vectors, events since last read, scalars with provenance (raw vs normalised), per-field age; legacy fields unchanged | double buffer |
| V1 Choreography | M85 | per frame | extent target E, front velocity, iris, colour chapter, spread k, state ∈ {FLOW…QUIET} | per board |
| V2 Field | M85 | per frame | intensity + palette position per pixel, sub-pixel geometry until rasterisation; max-owner for colour | per board |
| V3 Shade | M85 | per frame | HD palette sample, 16-bit; palette-domain warmth; measured transfer (after A21); cap once | — |
| O1 Emit | M85 + GPT/DMA/ELC | 100 Hz (hop-locked later) | GRB48 TRUE16; board 0 P601/GPT6 + P604/GPT8 (LED DMA0/3); board 1 top engine as today | — |
| I Instruments | M85 + host | per frame | packer-tap scoring, gate-reason word, witness-lane A/B, product-equivalence test | — |

**Budget per 10 ms frame (proposed ceiling, frozen before any move to the U55):** all AP work ≤ 2.0 ms per 5 ms interval; both-board mapping and rendering ≤ 0.75 ms; packing ≤ 0.25 ms; new event/feature storage ≤ 16 KiB static; zero heap in the render path; zero hidden event loss. Today ≈ 20–28 % of one M85 [INFERENCE]. Add an idle counter before adopting anything expensive.

**What runs where.** M85: everything above, scalar or MVE chosen by measurement (PL14-B5: CMSIS lost once). Ethos-U55: nothing in the first architecture; considered only for a causal beat/downbeat model that beats A24/A18 gates with ≤ 2 ms per hop (BeatNet's public source documents 84 ms streaming framing delay, so it is not a ready answer). Cortex-M33: parked (`hal_entry.c:259`). Cheap extras with a role: second mic (instrument), PDM sound-detection IRQ (optional transient pre-trigger), speaker DAC for a latency chirp if one is fitted. The IMU tap is a product-control decision, not part of this pipeline (D10's schematic claims for the IMU, amplifier and speaker were not verified).

**Migration from host-006 that keeps the Titan always visualising.** Each stage puts new behaviour behind a flag or runtime knob whose default reproduces host-006; the product-equivalence test goes red if the default changes; the previous image is the fallback; the image boots with visuals on; flash darkness is disclosed.

0. **Close what is on the device.** host-006 is programmed (`STATUS.md:201`). Remaining: a wire snapshot of a BOOM passage that reproduces the scored path-P frames, and the Captain's look (§9 experiment 3).
1. **Instruments** (A04, A13 telemetry, A19 knobs).
2. **AP input truth, host first** (A01 model and replay, A08 logging, A12, A07). Device proof for A01/A08 through the witness lane (A15) under lane authority.
3. **VP geometry on labelled events** (A02, A03, A06, A11, A10). Scored host-side on annotated times, then one image with both variants selectable for the Captain's A/Bs.
4. **Fast events and causal tension** (A07, A18, A09, A22), scored separately from geometry; provisional fires count as visible errors.
5. **Clock** (A24), then predicted downbeats only after the P7 gate and a downbeat score pass.
6. **Refactor** (V2 field, render on hop, optional supersampling), then the optional U55 model.

---

## 7. What the Captain must judge by eye — small A/Bs (PROPOSAL — NOT APPROVED)

Each is one passage of 10–20 s, two variants, the same palette and matched maximum brightness, selected at runtime where possible. Four randomised presentations including one identical-pair control; a pre-declared 3-of-4 preference decides; a tie stays undecided. A machine guard is frozen first and its planted wrong variant must fail before the look. These are taste decisions; what the LEDs render is checked by the wire snapshot, never by asking the Captain.

| # | Clip | A | B | Question | Needs a flash? |
|---|---|---|---|---|---|
| E1 | BOOM 40–65 s | host-006 | host-006 + release at the labelled drop (A02) | Does the drop hit harder than the build? | yes (release is constexpr) |
| E2 | BOOM drop | deposit point sweeping edges → centre | light born at the centre streaming outward | Which reads as your "edges to centre" thrust? | yes |
| E3 | synthetic drop ladder | release T90 30 / 60 / 120 / 250 ms | × peak ×1 / 1.5 / 2 | The slowest one that still reads "harder" becomes the frozen bar | yes |
| E4 | same release | at 0 ms | +20 / +45 / +90 ms | Which still feels on the hit? | yes |
| E5 | BOOM build | today's brightness | build held to ≤ 0.4 of the cap (A11) | Does a dimmer build betray the contraction? | yes |
| E6 | Sonic Samba 20 s | Mood 0.05 (default) | Mood 0.5 | Colour speed: more on the music, or too nervous? | **no** (live control) |
| E7 | same | path-P ramp k = 1 | k = 0.25 (or `d` off) | How much palette spread along the strip? | yes |
| E8 | tonal passage | chromatic order | circle of fifths (A05) | Do related chords look related? | yes |
| E9 | BOOM silent cut | 0.2 s drain | 1 s drain | Is the lingering liked? | yes |
| E10 | everyday passage | mode 32 outward bouncing root | edge-origin extension everywhere | Everyday character | yes |
| E11 | drop | colour arrives ~0.1 s after the light | colour snaps with the light (A10) | Aligned or afterglow? | yes |
| E12 | two boards at the drop | both release together | one releases, the other shifts colour (A17) | Synergy between the plates | yes |
| E13 | kick-heavy groove | current | sub-push / kick-in snare-out (A09, A16) | Should low end move the light? | yes |
| E14 | 20 s | path P | Spectrum River (each bin its own pixel and palette position; SB `6109a1e`) | Spectrum as space? | yes |
| E15 | after A21 | no degamma | degamma (in-house modes) | Dark-end richness | yes |

Rulings, not A/Bs: which brightness ceiling binds on this unit (§8.2 X1); the drop annotation convention (grid downbeat vs first transient vs bass arrival); whether quiet music between −33 and −28 dBFS may drain to black (C50); naming 7 tracks for a sealed holdout.

---

## 8. Contradictions and open questions

### 8.1 Disagreements between the three syntheses, settled by the integrator

| # | Disagreement | S1 | S2 | S3 | Settled from source | Effect |
|---|---|---|---|---|---|---|
| D1 | Is path P on the Titan? | not programmed | not read back | resident unchecked | [FACT] Programmed 2026-10-14T17:58Z, identity MATCH, 127/127 readback (`RUNS/STATUS.md:201`) | Stage 0 reduces to the wire snapshot + Captain look |
| D2 | int32 wrap in the note analyser | T04, rank 4 | S2-20 (wrapper corrected) | removed | [FACT] A:36-69 int64; [INFERENCE, IC] 155× headroom at guarded full scale | removed from the portfolio; risk carried to A08 |
| D3 | HPF corner | ≈ 259 Hz (D10), retune | 128 or 259, measure first | 259 more likely, measure | [FACT] HPF runs on the sinc result, LPF then halves the rate (MAN 213207, 213325-213327), so the HPF clock is 2·Fout; [INFERENCE, IC] model gives 100.1 Hz at 32 kHz and 259 Hz at 82.8 kHz; 128 Hz needs the filter at Fout, which the manual contradicts | A01 rank 1; device measurement still precedes the programme-lane change |
| D4 | In-house analysis rate | 24 kHz | default 12.8 kHz (C4) | 24 kHz enabled | [FACT] header default 12.8 kHz (AH:46) but host-006 `inhouse_analysis24: true` (receipt `:424`) | S2's C4 row is out of date |
| D5 | RMS truncation priority | low (C06) | — | rank 2 | [INFERENCE, IC] error > 2 % only below −38 dBFS, under the level floor (−33 dBFS) and silence entry (0.04) | A27, rank 27 |
| D6 | Reach cap as a drop lever | part of T15 | rank 2 | part of L05 | [FACT] W:976-977: with outward polarity a higher cap shortens the lit extent; only the release inverts it | A03 paired with A02 |
| D7 | Tempo prior removal | enabler E1 | S2-04, "restores 138/142/150" | L19, "a candidate, not a solution" | [FACT] GATE-SYNTHETIC-v1: flat prior 29/326, best 35/326, planted prior does not discriminate (`STATUS.md:200`) | A24 is design work |
| D8 | Second board | top engine on own pins | P603/P714 behind a default-off flag | both boards enabled in the receipt | [FACT] host-006 `titan_top_output: true` (`:411`); top engine GPT7/12, DMA6/7 (D10 VERIFY 16) | the product has two boards on; brief's "both on P601/P604" is wrong for the top |
| D9 | Does the release motion work? | needs labelled test | needs labelled test | needs labelled test | [FACT] X1–X3 reach extent 1.0, T90 0.04–0.07 s on BOOM source when they fire; failures are build-time fires, a riser fire, and no fire on Sonic @54.7 (`P6P/results/ext/`) | A02 is close to decisive; §9 exp. 1 |
| D10 | Audio-blind path-P plant | must add | S2-12 must add | metric may count the ruler | [FACT] the frozen-position plant exists and already fails the motion clause (`TOLERANCE-FROZEN-P6-PAL-HD-v2.json`); the spread figure is not protected | A04 adds a spread clause only |
| D11 | Loud-guard logging | export `capture_last_hop_gain_q8` | add a logger | publish provenance | [FACT] last-hop gain and clip counters exist; on BOOM host-006 they read 4096 / 0 at start and end only (`cost-host-006/counters.json`) | A08 adds a per-slot count |
| D12 | Release is runtime-switchable | E4 hypothesis | constexpr (wrapper) | — | [FACT] `inline constexpr` (WH:79); host probe can set it (`p6_effect_probe.cpp:138-146`); in-house Mood is live (`k1_live_runtime.h:87-88`) | A19; E6 needs no flash |
| D13 | Ranking formula vs product priority | score order | drop cluster first | formula bias noted | integrator: one score list with drop relevance as the tie-break, plus §9 picking the decisive drop test first | — |
| D14 | Fifths circle | rank 2 (score 12) | taste only | taste A/B | integrator: impact 2 (unproven look), rank 5, taste A/B E8 | — |

### 8.2 Contradictions still open (reported, not resolved)

| ID | Conflict | Sources |
|---|---|---|
| X1 | **Brightness authority.** Global mandate: maximum 24/255. `Kx-Titan/AGENTS.md:12`: default 100/255 supersedes 24. `palette_runtime.h:23` static_assert ≥ 100; `build_scalar.py:933-934` rejects < 100. Canon: master 255 × gains 0.25; readbacks of gains 1.0. At least five gain stages could carry an envelope | D01–D13; S1/S2/S3 |
| X2 | **Frame rate.** Brief: 120 Hz. Code and device: 100 Hz under the port build (PRH:40; BUD:81). 120 Hz is another branch | PRH:39-47 |
| X3 | **Source identity.** Brief `81ac031`; dimensions `1ef5395` + dirty tree; now `d557f06`/`045b6b5` | §0 |
| X4 | **"16-colour copy" root cause.** At `81ac031` `sample()` was already the HD sampler; the collapse was one sample per frame + RGB mix + RGB EMA. `c2738b88` is the 16-bit paint commit; the 1:1 map is SB `6109a1e`; the 16-entry flatten removal is SB `d408433` | D02/D03/D06 VERIFY |
| X5 | **"Colour moves in 1–11 % of seconds before"** refers to v9 `b5595c35`, not v8 (v8 moved in 94.8 %) | FORENSICS:113,118 |
| X6 | **Drop direction.** The Captain's quote says "edges towards the centre"; K1 canon says "heart outward"; EXT-v2 measures extent only, not direction. The quote has no on-disk source other than the frozen EXT files | E2 in §7 |
| X7 | **AV asymmetry.** ITU-R BT.1359 tolerates light early more than light late; broadcast data, not a music-impact contract | E4 in §7 |
| X8 | **Latency.** ≈ 16.4 ms stack already includes 5.1 ms wire and a modelled 5 ms tick; 24.97 ms is a percentile stack; the 9.6 ms wire figure is withdrawn | BUD; STATUS 16:49Z |
| X9 | **"Hard cut never while music plays" vs "fade fully to black on silence"** — compatible only if the silence gate is right (C50) | A13 |
| X10 | **D02 window anchor** 55.73 s vs 55.7025 s — medians are slightly mis-windowed, still descriptive | D02 VERIFY |

### 8.3 Open questions (who can answer)

1. Does a wire snapshot of host-006 reproduce the scored path-P frames? — lane owner, existing snapshot tool (`scripts/palette_wire_snapshot.py`).
2. What is the HPF's real response on this board, and how much bass do the capsule and room deliver? — witness-lane sweep (A01, A15).
3. How often does the loud guard engage during a real drop? — per-slot count (A08).
4. Is quiet music between −33 and −28 dBFS being drained to black? — A13 replay, then the Captain's ruling.
5. WS2816 curve at the installed gain — locked-exposure ramp (A21).
6. Does the light-guide plate hide beading; do the two boards' latches agree within 2 ms? — camera frame plus wire snapshot.
7. Is the GPL-lineage donor VU/tempo admissible in the product? — licence review before A22.
8. M85 idle fraction at the target cadence — idle counter.
9. Drop annotation convention and a sealed holdout of 7 tracks — the Captain.
10. Is "80 bins" the frequency bins or the 80 pixels per half? (claude-mem suggests pixels.)
11. The replay working directory `/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6` (probe executables, inputs, replays) is outside the repo and may be cleaned; copy it to the external scratch drive before relying on it.

---

## 9. Immediate next three experiments

Each is runnable now: two on the host with the existing frozen-gate machinery, one on the device with one Captain look and no flash. All three are **PROPOSAL — NOT APPROVED** until the lane owner runs them under existing authority.

### Experiment 1 (host): does the release hit, if its timing is perfect?

- **Question.** With the existing release fired at the frozen drop anchors, does the v9 path-P image pass the unchanged EXT-v2 "hits harder" test?
- **Machinery that already exists.** Host probe `p6_effect_probe` with release arguments (`tests/host/p6_effect_probe.cpp:138-146`); evaluator `scripts/p6_ext.py` with frozen `TOLERANCE-FROZEN-P6-EXT-v2.json` (cases BOOM source, BOOM room, Sonic @27.7, Sonic @54.7); runner pattern `P6P/run-ext-candidate.sh`; known negative `ext-v2-b5595c35.json` (fails) and positive plant (passes); inputs `…/tmp/p6/inputs6`.
- **One small host-only addition.** An argument that forces `release_ = 1` at listed song times (instant attack, the existing 0.6 s decay) instead of the level surge. Product source is not touched.
- **Freeze before any run** (`freeze_tolerance.py freeze`): a candidates file naming the path-P probe at `d557f06` gains, the forced times (55.7, 55.7, 27.7, 54.7 s, taken from EXT-v2 itself), and the decision rule: pass = EXT-v2 passes on all four tuning cases on both boards, and C2/C3/C4/C5b still pass. State in advance that the lag clause is satisfied by construction and is not evidence.
- **Planted faults that must go red:** (a) release off (the host-006 behaviour) must fail `release_speed` and `release_size`; (b) forced time + 250 ms must fail `congruence` (lag); (c) outward-only (no pull) must fail. The positive plant (`--plant`) must still pass.
- **What the result means.** Pass: the look is solved given timing, and the work moves to A07/A18/A24 plus the Captain's E1–E3 looks. Fail on `release_size` or `congruence`: the release shape needs A03/A06 before any detector work. Carte Blanche is spent and is reported but not claimed.

### Experiment 2 (host): how much drop does the mic's bass cut remove?

- **Question.** If the Titan's HPF really sits at 259 Hz, how much of the BOOM drop's sub-bass, level surge and release trigger does it remove compared with a 20 Hz DC block?
- **Already done here:** IC `hpf` (the model reproduces the manual's 100 Hz at 32 kHz; 259 Hz at 82.8 kHz; candidate 0x3FE7/0x3FF4 gives 20.1 Hz).
- **Run.** Filter `boom_source.s16` (40 kHz) through three analogue-equivalent first-order high-passes (259 Hz model, 20 Hz candidate, planted 400 Hz) and replay each through the same probe. Report `sub_rms`, `vu_level` and the release surge (fast/slow power) in the last 2 s of the build and the first 2 s of the drop.
- **Freeze before results.** Model check: 100 ± 2 Hz at 32 kHz. Effect: drop `sub_rms` with the 20 Hz filter is ≥ 6 dB above the 259 Hz model. Sensitivity: the planted 400 Hz filter must read ≥ 3 dB below the 259 Hz model. A k1 change of 1 LSB must move fc by > 0.5 Hz.
- **What the result means.** A large loss justifies the witness-lane device sweep (A01 part c, no Captain look needed) and moves A09 up. A small loss demotes A01. `boom_room` already passed through some microphone, so its low-band deficit against `boom_source` is reported as supporting evidence only (speaker and room roll-off confound it).

### Experiment 3 (device, one Captain look, no flash): close path P and settle colour speed on the image already on the Titan

- **Step 1, instrument (no Captain).** Take a wire snapshot of a 20 s BOOM passage on host-006 (`scripts/palette_wire_snapshot.py`). Freeze first: the snapshot reproduces the host replay of the same audio within the existing equivalence tolerance on both boards; packed zero during the silent cut. Planted: the host-005 gains or palette B replay must fail the comparison.
- **Step 2, one Captain look.** On the same image, use the live in-house Mood control (`setInhouseMood`, board 0, which sets colour for both boards) to show the same Sonic Samba 20 s passage at Mood 0.05 (default) and Mood 0.5, in four shuffled presentations with one identical pair. Freeze first: Mood readback equals the commanded value each time; the colour-offset step response on a click fixture is faster at 0.5 (host model, τ 80 → 20 ms). Planted: a presentation labelled 0.5 but commanded 0.05 must fail the readback.
- **Question for the Captain.** "Which colour feels more on the music without becoming nervous?" His answer settles the pending colour-speed question (E6) and A10's first half. Flash count: zero. Restore Mood 0.05 afterwards, and confirm by readback that music visualisation continued throughout.

---

## 10. Files read and checks run by the integrator

- Read in full: `SWARM-BRIEF.md`, `SYNTH-S1-opus.md`, `SYNTH-S2-grok/REPORT.md` (incl. §11), `SYNTH-S3-astra/REPORT.md`, `D10-hardware-leverage/VERIFY.md`.
- Read in part: D02 `vp_numbers.json` and `clamp_occupancy.json`; W:25-130 (header), W:934-1000; A:1-75, 225-310; AH:1-50; G:14-24, 98-134; host-006 receipt `:380-430`; `scripts/p6_ext.py`; `scripts/freeze_tolerance.py`; `tests/host/p6_effect_probe.cpp` argument parsing; `TOLERANCE-FROZEN-P6-EXT-v2.json`; `TOLERANCE-FROZEN-P6-PAL-HD-v2.json`; `P6-EXT-DESIGN-CANDIDATES-FROZEN.json`; `P6P/results/ext/ext-v2-{X1,X3,b5595c35,b5595c35-planted}.json`; `RUNS/STATUS.md:176-201`; `P6P/cost-host-006/{counters,info,report}.json`; MAN 211471, 212793-212836, 213200-213260, 213322-213330.
- Ran: `INTEGRATOR/settle_checks.py` (HPF response from the manual's structure and coefficients; in-house Goertzel state at guarded full scale across all 80 bins at 24 kHz; RMS truncation error). Output: `INTEGRATOR/settle_checks.json`. Git: `log`, `status --porcelain` only.
- Not done: no build, replay, flash, device read, or camera. No image was produced; this is a text deliverable, so the integrator re-read the summary section for plain English rather than inspecting a render.
