# D10-hardware-leverage — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

The Titan's main processor is only about a third busy. At the median it uses about 28% of each 10 ms frame (35% if every stage sits at its 99th percentile), which leaves about 6.5-7 ms of free time per frame. The ESP32-era reasons to compress colour, maths or bit depth no longer apply.
The biggest finding is in the microphone hardware. Its built-in bass-cut filter still uses the coefficients written for the 16 kHz demo (manual note: 100 Hz). The product runs the microphones about 2.6 times faster, and my calculation puts the cut near 259 Hz. That would weaken kick and bass by about 8 dB at 110 Hz and 14 dB at 55 Hz before any analysis. It is one register write to fix and is not yet measured on the device.
The microphone output also keeps only 16 of its 20 bits, multiplies by 16, and turns the gain down on the loudest 7 ms blocks, which are exactly the drops. The hardware already offers 20-bit output.
The second onboard microphone is captured and then thrown away. Because each microphone has its own filter settings, it can serve as a free side-by-side test channel for every input fix.
The LED wire sends a frame in 5.1 ms but frames go out only every 10 ms. The strip could run at 120-190 frames per second, and frames could be timed to the analysis instead of waiting a random 0-10 ms.
The WS2816 LEDs apply their own built-in gamma curve. The ESP32 firmware corrects for it; the Titan does not, which may explain part of the dark-palette trouble seen in P6. The original Emotiscope modes still send 8-bit colour by default.
Unused but already on the board or chip: a motion sensor with tap detection, a speaker amplifier and DAC (usable as a self-timed latency instrument), the M33 core, the NPU, the empty fast instruction memory, 32 MB of SDRAM, a hardware sound-detection interrupt and a clock-measurement circuit.
Recommended order: first check the second microphone matches the first, then fix the bass cut and 20-bit output on it. After that, run frames faster and locked to the analysis, then measure the LED gamma before the Captain judges it.
I wrote no report file: the harness refused the write. Everything is in this result.

## Levers

### HW-01 — RA8P1 PDMIF hardware high-pass filter (manual R01UH1064EJ0130 Rev1.30 §50.3.4.3 Fig 50.4); coefficients hal_data.c:617-619 (s0 0x3F61, k1 0x3EC1, h {0x4000,0xC000}); product copies g_pdm0_cfg.p_extend and overrides only sincdec/sincrng (platform/ra8p1/pdm_target.c:764-781)
- **container**: AP
- **controls**: Brightness and motion driven by kick and bass energy (drop impact); bass-band colour weight
- **current**: v8 and v9 both use the unchanged BSP/reset coefficients. [FACT] Manual §50.2.21 note: reset value means 'cut-off frequency = 100 Hz'. [FACT] The HPF is fed by the sinc output, and Fout = PDM_CLK/(2(SINCDEC+1)), so the HPF runs at 2·Fout. [INFERENCE] k1 = 0.98053: fc = 100.1 Hz at fs 32 kHz (BSP 16 kHz design), but 259 Hz at fs 82,816 Hz (product 41,408 Hz). That is -8.2 dB at 110 Hz, -13.7 dB at 55 Hz, -16.3 dB at 40 Hz.
- **info_loss**: About 8-16 dB of kick and sub-bass energy, plus phase lead, removed in hardware before the analysis sees it.
- **origin**: Accident or inheritance: BSP coefficients designed for SINCDEC 124 (16 kHz); the 40 kHz retune changed only SINCDEC/SINCRNG (pdm_target.h:21-24).
- **still_needed**: Only a DC blocker (about 20-30 Hz) is needed; the 259 Hz cut is not. Cost: 0 CPU cycles.
- **options**: Rewrite PDHFCK1R/PDHFCS0R for the real rate (fc 20 Hz: k1 about 0x3FE7, s0 about 0x3FF3); or bypass the HPF and remove DC in software; derive fc from the measured rate; also audit the compensation-filter and LPF reset designs (LPF note says 7 kHz at the reference rate, roughly 18 kHz now [INFERENCE]).
- **impact**: 5
- **novelty**: 5
- **effort**: 1
- **experiment**: Paired-lane A/B on the same air (uses HW-03): lane B HPF at fc 20 Hz, lane A unchanged; log sweep 30-1000 Hz. Frozen criterion (scripts/freeze_tolerance.py): B/A gain >= +6 dB at 45-65 Hz and within ±1.5 dB at 800-1000 Hz. Planted fault: lane B at fc about 1 kHz must read <= -6 dB at 45-65 Hz. A host model of the Table 50.12 HPF must first reproduce the 100 Hz note at 32 kHz; a 1-LSB k1 mutation must move fc by more than 0.5 Hz.

### HW-02 — PDM PCM width PDM_PCM_WIDTH_16_BITS_0_14 (hal_data.c:643; k1_pdm_fifo16.h), then ×16 sensitivity Q8 4096 and per-296-sample-slot loud guard (k1_pdm_sensitivity.h; pdm_target.c:614-636)
- **container**: AP
- **controls**: Brightness dynamic range build→drop, transient sharpness, onset contrast
- **current**: v8 and v9: 16-bit window [14:0] plus sign, then ×16 gain; gain is cut for any slot whose raw peak exceeds 1,875. [FACT] FSP offers PDM_PCM_WIDTH_20_BITS_0_18 (r_pdm_api.h:43), and the data buffer stores 20-bit signed data (manual p.3200). PLATFORM_MEMORY records the capsules reaching int16 rails.
- **info_loss**: About 4 bits of headroom. Quiet signals keep about 12 effective bits [INFERENCE]. Block-wise gain cuts land on the loudest 7.15 ms slots, i.e. on drops.
- **origin**: Inherited from the S3/DualMCU '×16 SENSITIVITY' analogue, plus a later fix for int16 railing.
- **still_needed**: No. The analysis runs in float, so 20-bit input is free [INFERENCE]. The int16 ASRC interface needs an int32 path.
- **options**: Switch to 20-bit width and scale in int32 or float; delete the loud guard; keep the clip counters as instruments; per-lane HDR (lane B low gain).
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Export capture_last_hop_gain_q8 (pdm_target.c:626). Frozen criterion: fraction of drop hops with gain < 4096 minus fraction of build hops <= 10 points; otherwise the guard flattens build→drop. Planted fault: ceiling 3,000 must push the drop fraction above 90%. Then a lane-B 20-bit A/B: crest factor at the BOOM drop >= lane A + 3 dB, with zero sat_pos/sat_neg on lane B.

### HW-03 — Second onboard mic U13/U14 LMD2718T261-OA1 (schematic PDM MICS sheet); K1_PDM_TARGET_MEASUREMENT_LANE is DMA-captured and metered, but only the programme lane is copied into capture_pcm (pdm_target.c:608-610)
- **container**: AP
- **controls**: Nothing in the light today. It is an input-quality, redundancy and paired-control instrument.
- **current**: Captured, metered, then discarded [FACT].
- **info_loss**: A complete independent acoustic channel.
- **origin**: Deliberate measurement-lane design, not an S3 budget limit.
- **still_needed**: DMA and polling are already paid. Analysing it would add about one ASRC (about 0.13 M cycles/hop, ~2.7%) [INFERENCE].
- **options**: (a) Permanent on-device A/B: PDM registers are per channel (HPF, width, shift), so the same air and clock serve as a paired control. (b) HDR loud/quiet lanes. (c) Capsule-fault failover, protecting always-visualising. (d) Averaging for SNR [HYPOTHESIS: noise may be correlated].
- **impact**: 3
- **novelty**: 4
- **effort**: 2
- **experiment**: Null A/B with identical configuration on both lanes. Frozen criterion: B/A 30-8000 Hz band power within ±1.5 dB over 60 s of trance. Planted fault: lane B with a 1-bit HFIS shift must read -6 ± 0.5 dB.

### HW-04 — PDMIFCLK comes from MOCO, not the crystal (clock-chain.json claim-wp12-pdmifclk-moco; manual §9.10.34); a software rate-tracking ASRC runs in k1_asrc_24k.c / k1_poly24.c
- **container**: AP
- **controls**: Timing (latency), tempo accuracy, bin-centre accuracy
- **current**: Measured 41,408 Hz (P5 BUDGET.md) and 41,417 Hz (PL14-B12) [FACT]: +3.5% and drifting. The ASRC costs about 133k cycles/hop plus a 47-sample (1.14 ms) delay.
- **info_loss**: 1.14 ms of filter delay. It also caused one alignment bug class (PL14-B2, 2.35 ms).
- **origin**: Hardware constraint, plus inherited fixed-rate analysis contracts.
- **still_needed**: Partly. The CAC (clock frequency accuracy measurement circuit) can measure MOCO against the crystal in hardware; rate-aware GDFT could delete the in-house ASRC [HYPOTHESIS].
- **options**: CAC rate truth (P4 plan item 5, deferred); rate-aware analysis kernels recomputed at boot and on drift; or keep the ASRC.
- **impact**: 2
- **novelty**: 3
- **effort**: 3
- **experiment**: Log the CAC count against the ASRC rate estimate for 10 minutes. Frozen criterion: |difference| < 0.01% after 5 s. Planted fault: a wrong CAC reference divider must read > 1%.

### HW-05 — PDMIF hardware sound-detection block (HPF plus moving average, then 20-bit upper/lower threshold, then interrupt; manual p.3200; r_pdm_api.h PDM_EVENT_SOUND_DETECTION, pdm_sound_detection_setting_t)
- **container**: AP
- **controls**: Timing of transient response (kick-onset flash)
- **current**: Not configured; grep found no sound_detection in pdm_target.c [FACT].
- **info_loss**: A sample-rate transient event with microsecond interrupt latency, versus hop-based detection (5-7.5 ms hops plus slot fill).
- **origin**: Never explored.
- **still_needed**: Opportunity only. Cost: one interrupt per threshold crossing.
- **options**: A transient pre-trigger that schedules an immediate LED frame (HW-08) or arms onset confirmation; thresholds re-programmed each hop from the AGC envelope; a per-channel low-passed kick detector on lane B [HYPOTHESIS].
- **impact**: 3
- **novelty**: 5
- **effort**: 3
- **experiment**: Log interrupt stamps against the published onset stamps on the P5 click track. Frozen criterion: median lead >= 3 ms, and >= 90% of truth clicks raise an interrupt within 2 ms of the click. Planted fault: a full-scale threshold must give < 5% hits.

### HW-06 — WS2816C internal 4-bit gamma (datasheet p.1, sha256 43d21c16…). Titan packs palette values directly after scale16 (palette_runtime.cpp:735-737). K1 S3 applies degamma 1/2.2 (SpectraSynq_K1_Firmware/SPECTRASYNQ_K1_FIRMWARE/visual/k1_ws2816_degamma.h).
- **container**: output
- **controls**: Colour (palette dark end, mid-tone saturation), brightness response, fade tails
- **current**: v8 and v9: no degamma on Titan; grep found 0 hits in platform/ and src/ [FACT]. K1 S3 has degamma (look library, commit 38428a82).
- **info_loss**: [HYPOTHESIS] Values authored for linear-PWM WS2812 render darker in the low mids on WS2816. That compresses the low end, which matches P6's recurring 'palettes' dark low end' blocker and variant B's range restriction.
- **origin**: Accident or inheritance: the Titan port never carried the K1 look library.
- **still_needed**: An appearance and parity decision. Cost: about 1-2% of render for a LUT lookup per channel.
- **options**: No degamma (current); K1 degamma at γ 2.2; an inverse curve measured from a camera ramp; degamma applied to palette sampling only; a per-board choice.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Step 1, measure the LED: iPhone 240 fps of a 16-step grey ramp per channel on one board. Frozen criterion: fitted internal γ in [1.6, 2.8] with R² >= 0.98 justifies degamma; γ in [0.9, 1.2] rejects the lever. Planted fault: a square-law pre-curve must shift the fitted γ by >= 0.8. Step 2: Captain A/B, v9 with and without degamma on the two boards.

### HW-07 — Emotiscope modes 0-6: emotiscopeWirePixel (RGB8×257, emotiscope_output.h) is chosen when output_precision_ == 0 (palette_runtime.cpp:411-419); the default 0 is set at emotiscope_control.h:299,352,396; the 16-bit candidate emotiscopeWirePixel16 ('donor_float_round16') exists
- **container**: output
- **controls**: Colour and brightness resolution of the Emotiscope modes
- **current**: RGB8 with temporal dither, expanded ×257, then scaled by brightness. In-house modes already author TRUE16 [FACT].
- **info_loss**: 256 code levels per channel instead of 6,168 at brightness 24 [INFERENCE]; dither noise the 16-bit part does not need.
- **origin**: S3 budget: 8-bit LEDs and transport.
- **still_needed**: No. Cost: nil; the float source k1_emotiscope_last_rgb16 already exists.
- **options**: Make output_precision = 1 the boot-composition default.
- **impact**: 2
- **novelty**: 1
- **effort**: 1
- **experiment**: Host replay of modes 0-6 at brightness 24. Frozen criterion: distinct non-zero 16-bit levels per channel over 60 s >= 4× the ×257 path, with mean absolute difference <= 1/255. Planted fault: forcing ×257 into the 16-bit path must fail the level count.

### HW-08 — kPalettePeriodUs = 10000 under K1_EMOTISCOPE_PORT (palette_runtime.h:39-40); due test palette_runtime.h:144; wire 80 px × 48 bit × 1.25 µs + 300 µs = 5.10 ms per lane, four lanes in parallel (BUDGET.md (e); ws281x_diag.h profile 3)
- **container**: output
- **controls**: Timing (latency, jitter), motion smoothness
- **current**: 100.01 fps, zero skipped frames [FACT]. Wait for the tick is modelled at 5 ms median and 9.9 ms p99.
- **info_loss**: Motion steps: a release front crossing 80 px in 100 ms moves 8 px per frame. Plus 0-10 ms of random latency.
- **origin**: Emotiscope donor cadence. The W4 STOP assumed 160 px per line (9.6 ms); canon §5 corrects this to 80 px per lane.
- **still_needed**: No. The datasheet allows about 5.08 ms per frame, so about 196 Hz is the ceiling [INFERENCE]. CPU cost is about +0.3% per extra 20 Hz.
- **options**: 120 Hz (canon); hop-locked render on the in-house 7.5 ms hop (133 Hz) to delete the random wait; 150-180 Hz free-run; event-triggered extra frames from HW-05. The renderer is dt-aware (inhouse_waveform.cpp:411,461); check mode 11's 120-basis decay.
- **impact**: 4
- **novelty**: 2
- **effort**: 2
- **experiment**: Device timing log with a per-frame publish→submit stamp. Frozen criterion: p50 publish→wire-end <= 6.0 ms and p99 <= 7.5 ms, with zero skipped_releases and emit_errors over 75 s BOOM. Planted fault: a fixed 4 ms pre-submit delay must fail p50. Captain judges 100 vs 133 Hz by eye.

### HW-09 — InhouseWaveformRenderer per-frame step (src/k1/core/visual/inhouse_waveform.cpp); one sample per frame, Q16 history
- **container**: VP
- **controls**: Motion: perceived speed and thrust of the explosive release, smoothness of fast fronts
- **current**: One sample per frame at 100 Hz.
- **info_loss**: The continuous trajectory between frames, so fast fronts strobe [INFERENCE].
- **origin**: S3 render budget (FastLED era).
- **still_needed**: No. 8× temporal supersampling costs about 1.5 ms per frame (~15% CPU), from about 570 cycles/px (182k ÷ 320 px) [INFERENCE].
- **options**: N sub-steps integrated with a box or shutter filter; applied only above a front-velocity threshold; combined with HW-08.
- **impact**: 4
- **novelty**: 4
- **effort**: 3
- **experiment**: Host replay of the BOOM drop at 1× and 8×. Frozen criterion: maximum per-frame front displacement <= 2 px, and the front reaches the edges no later than at 1× (±1 frame). Planted fault: dropping the last sub-step gives a one-frame lag and must fail. Then Captain A/B.

### HW-10 — 16-bit chain: scale16 brightness (hd_pixel16.h; palette_runtime.cpp:735), TRUE16 in-house authoring (palette_runtime.cpp:290-297), Q16 history (inhouse_waveform.cpp:8-40)
- **container**: output
- **controls**: Brightness, fade smoothness, true black, dim-pixel colour
- **current**: At brightness 24/255 each channel has 0…6,168 codes, against 24 on an 8-bit LED [INFERENCE arithmetic]. Kept on in-house modes; lost on Emotiscope modes (HW-07).
- **info_loss**: None on in-house modes today. This lever is a guard.
- **origin**: Upgrade already taken (v8 composition '16-bit', canon §5).
- **still_needed**: Yes; keep it and guard it.
- **options**: A guard test that no Pixel8 reaches the wire; deliberately long smooth fades to true black.
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: Product-path guard (PL14-B14 style). Frozen criterion: a 2 s fade at brightness 24 emits >= 1,000 distinct codes on the brightest channel and ends at exactly 0. Planted fault: inserting liftPixel8 must give <= 25 codes.

### HW-11 — GPT6/8 plus GPT7/12 transmitters, GPT0/3/4 event timers, DMAC0/3/6/7, ELC links (ws281x_gpt_dma_hw_top.c:1-45; pair driver); submission is software-initiated after render (fixture_app.cpp:663-668)
- **container**: output
- **controls**: Timing: jitter and inter-board skew
- **current**: Each submit happens when render ends; board 1 follows board 0 by its pack time.
- **info_loss**: Constant latency: jitter equals render plus poll variance.
- **origin**: Port design.
- **still_needed**: Opportunity. Cost: GPT compare and ELC configuration only.
- **options**: Pre-arm all four lanes and start them on one GPT compare event through ELC, at a declared present-time.
- **impact**: 2
- **novelty**: 4
- **effort**: 3
- **experiment**: P601 loopback sampler (--p601-loopback) stamps the first edge against the declared time. Frozen criterion: |actual - declared| p99 <= 20 µs over 10,000 frames. Planted fault: a 1 ms offset must read 1 ms ± 20 µs.

### HW-12 — U16 LSM6DS3TR-C IMU on I2C1 at 0x6A, INT1 on P412 (Titan_Mini_schematic_v1.0 IMU sheet); BSP example project/Titan_Mini_peripheral_imu
- **container**: system
- **controls**: Control surface (palette, composition) and geometry (mirror/orientation)
- **current**: Not initialised in the product tree [FACT].
- **info_loss**: Physical interaction and orientation of the unit.
- **origin**: Never in scope.
- **still_needed**: Opportunity under the asymmetric-capability doctrine. Embedded tap and 6D orientation engines [HYPOTHESIS: ST datasheet not read this session].
- **options**: Double-tap selects the next palette or composition; 6D orientation chooses the mirror automatically; vibration as a contact sub-bass sensor [HYPOTHESIS].
- **impact**: 2
- **novelty**: 3
- **effort**: 2
- **experiment**: Log tap interrupts only, with no action mapped, during 10 minutes of loud trance plus 20 deliberate double-taps. Frozen criterion: >= 19/20 detected and <= 1 false event per 10 minutes. Planted fault: a minimum threshold must give > 10 false events.

### HW-13 — U15 ES8156 DAC (I2C1 0x08, I2S, SDOUT) into U12 LPA2103AQVF class-D into the J4 speaker connector (schematic SPEAKER/ES8156 sheets); BSP Titan_Mini_wavplayer and drv_i2s.c
- **container**: system
- **controls**: No light; provides acoustic latency and sensitivity truth
- **current**: Unused. Whether a speaker is fitted is unknown. The ES8156 is a DAC only, so it is not a line-in (Everest product brief) [FACT].
- **info_loss**: The air, mic and PDM-filter latency segment, which BUDGET.md lists as unmeasured.
- **origin**: Board feature never used.
- **still_needed**: An instrument only. Costs an I2S DMA channel and the GPT2 MCLK timer (check for conflict with the LED GPTs).
- **options**: A boot calibration chirp; a periodic low-level self-test click; a swept tone to verify HW-01.
- **impact**: 2
- **novelty**: 4
- **effort**: 3
- **experiment**: With a speaker on J4, 100 DWT-stamped clicks detected on both mic lanes. Frozen criterion: p99 - p50 <= 0.5 ms. Planted fault: an inserted 2 ms delay must shift p50 by 2 ± 0.2 ms.

### HW-14 — Ethos-U55: npu_load.c (RM_ETHOSU_Open, PMU, sub_0001 smoke graph); useful_npu_probe.c admission gate; BSP example Titan_Mini_npu_ai_face_detection
- **container**: AP
- **controls**: Beat/downbeat phase and build/drop/section structure, feeding motion and anticipation
- **current**: Smoke load only; it never reaches lighting. G6 is NO_QUALIFYING_CANDIDATE (STATUS.md:262; g4-npu-coexist-profile.json) [FACT]. Vendor figure: 256 MAC/cycle at 500 MHz.
- **info_loss**: Not a loss; it is unused capacity. Targets the half-time tempo lock and the drop-detector misses.
- **origin**: Parked by the old operating contract (AGENTS.md item 5); superseded by canon §4/§5.
- **still_needed**: [INFERENCE] A causal TCN beat tracker of about 0.1 M parameters is about 10-20 M MAC/s, or 1-4% of the M85 with MVE. The NPU is needed only for larger CRNN or structure models.
- **options**: CPU-first learned beat/downbeat model; NPU for a larger structure model; or DSP-only fixes.
- **impact**: 4
- **novelty**: 4
- **effort**: 5
- **experiment**: Host first: run BeatNet or madmom online (on disk) over the P7 corpus. Frozen criterion: F-measure at ±70 ms >= 0.80, and BPM within 2% (not half or double) on >= 90% of 10 s windows. Planted fault: ×0.5 time-stretch must produce half-time BPM and fail.

### HW-15 — Cortex-M33 at 250 MHz, not started (hal_entry.c:258 'No SecondaryCoreStart'); BSP project/Titan_Mini_rpmsg master/remote
- **container**: system
- **controls**: Jitter isolation; robustness of always-visualising
- **current**: Off [FACT].
- **info_loss**: None.
- **origin**: Canon §5: M85 is the single core until it cannot hold 8.33 ms; the M33 may only do I/O, never render.
- **still_needed**: No measured need; the M85 is about 28% busy.
- **options**: Stay parked; an I/O split (about 6.5%) for jitter only; a supervisor that restarts the M85 application on fault.
- **impact**: 1
- **novelty**: 2
- **effort**: 4
- **experiment**: None until the frozen trigger fires: an idle-counter measurement showing M85 above 70% at the target cadence.

### HW-16 — 32 MiB SDRAM (W9825G6KH-6 by design, window 0x68000000, 0 bytes used per the P4-corrections rtthread.map) and 1.6 MB SRAM
- **container**: AP
- **controls**: Drop anticipation (tension over 10-60 s); section-aware palette and motion
- **current**: Short state only; DTCM 69 KB used.
- **info_loss**: Song-scale context: the renderer cannot know it is 16 bars into a riser.
- **origin**: S3 RAM budget.
- **still_needed**: No. 60 s of 80 bins × 100 Hz × int16 = 0.96 MB fits in SRAM [INFERENCE]; SDRAM is only needed for minutes or raw PCM.
- **options**: Self-similarity/novelty over 30-60 s; riser/build detection feeding the release envelope; a raw-PCM ring for on-device replay.
- **impact**: 4
- **novelty**: 4
- **effort**: 4
- **experiment**: Host: 30 s novelty build detector on the P6-DROP-DETECTOR annotations. Frozen criterion: >= 80% of drops preceded by a 'build' state >= 2 s earlier, with <= 1 false build per track-minute. Planted fault: a time-reversed track must fail.

### HW-17 — ITCM 128 KiB at 0x0, 0 bytes used (map); ASRC-in-ITCM image 2e550b62 did not boot (PL14-A4); canon §5 lists an un-ported 'ITCM startup copy repair' in frozen Kx-Titan
- **container**: system
- **controls**: Timing jitter (instruction-fetch determinism)
- **current**: Code runs through the I-cache from MRAM/flash.
- **info_loss**: None; this is a performance lever.
- **origin**: Un-ported repair.
- **still_needed**: Low priority at about 28% CPU. DTCM gave render -4.5% and analysis -2.4% (P4).
- **options**: Port the native ITCM startup copy, then place the GDFT and render hot loops there.
- **impact**: 1
- **novelty**: 1
- **effort**: 3
- **experiment**: Host/link test that the startup copy covers __itcm_from_flash$$, with a planted out-of-range section going red. Only then a device boot with the recovery image staged.

### HW-18 — In-house 24 kHz GDFT, about 0.70 M cycles per hop; GCC 13.3 did not vectorise k1_poly24 (0 q-register vfma; PL14-B5)
- **container**: AP
- **controls**: Affordable resolution and rate: shorter hop, 160 bins, sub-bass
- **current**: Scalar code with fp-contract on.
- **info_loss**: None directly. It gates finer analysis. Sub-bass windows are physically about 0.3 s at 55 Hz, so fast sub-bass needs a time-domain envelope instead [INFERENCE].
- **origin**: P4 receipt: 'vectorised GDFT is the next lever if headroom is needed'.
- **still_needed**: Only if the analysis dimension asks for more resolution or rate.
- **options**: MVE intrinsics GDFT; recursive Goertzel or sliding DFT; or leave as is.
- **impact**: 3
- **novelty**: 2
- **effort**: 3
- **experiment**: Device cost capture. Frozen criterion: in-house p50 <= 0.5 × 699,512 cycles, and host feature gate median r >= 0.999. Planted fault: skipping the last 4 taps must fail r.

### HW-19 — K1_PDM_TARGET_SLOT_ELEMENTS 296 = 7.15 ms at 41.4 kHz (pdm_target.h:16); the only hard rule is 'multiple of 8' (FSP interrupt threshold)
- **container**: AP
- **controls**: Timing (capture block fill; BUDGET stage (b) median 4.95 ms)
- **current**: 296 samples per slot.
- **info_loss**: Up to one slot of freshness (7.15 ms).
- **origin**: Port convenience. Canon §3 proposes 64-word blocks (about 1.6 ms).
- **still_needed**: No. Interrupt and poll rate rise about 4.6× at 64 samples; poll is about 5% of CPU now, so measure the overhead.
- **options**: 64, 96 or 128 samples per slot, with ASRC granularity kept independent.
- **impact**: 3
- **novelty**: 1
- **effort**: 2
- **experiment**: Device timing log. Frozen criterion: publish-capture p50 falls by >= 2.5 ms from the 6.03 ms BOOM median, with zero pair_skew_drops and asrc_starved over 75 s. Planted fault: a 296-sample control must show no change (±0.2 ms).

### HW-20 — USB: CDC 045b:5310 through R_USB_Open(g_basic0) (hal_entry.c:262); the RA8P1 family has USBHS (BSP fsp_features.h:104)
- **container**: system
- **controls**: No light; bit-exact on-device evaluation input
- **current**: CDC telemetry only.
- **info_loss**: The exact source PCM on the device clock (today: host replay or acoustic only).
- **origin**: Never in scope.
- **still_needed**: Instrument only; the product stays microphone-reactive.
- **options**: Composite CDC plus UAC2 behind a test-only flag.
- **impact**: 2
- **novelty**: 3
- **effort**: 4
- **experiment**: Frozen trigger only: do not start until the USB-CDC-NOATTACH defect (PL14-A11) is root-caused.

### HW-21 — RA8P1 MRAM 0.5/1 MB (vendor page); every flash darkens the plate 41-47 s (PL14-A6) and adaptive state starts cold
- **container**: system
- **controls**: First seconds after boot: level normalisation and tempo lock
- **current**: Cold start.
- **info_loss**: Learned room level and tempo prior across resets.
- **origin**: Never in scope.
- **still_needed**: Small value.
- **options**: Snapshot a few hundred bytes of adaptive state, restored at boot with an age limit and CRC.
- **impact**: 1
- **novelty**: 2
- **effort**: 3
- **experiment**: Frozen criterion: the VU normaliser is within 10% of steady state <= 1 s after reset with restore. Planted fault: a corrupted CRC must fall back to cold start.

### HW-22 — Rejected or no-value capabilities: WS2816 overclock; 24-bit output mode; extra data lanes; PCM1808 line-in; 2D GPU, MIPI and Ethernet
- **container**: output
- **controls**: n/a
- **current**: Not used.
- **info_loss**: n/a
- **origin**: Evidenced negatives.
- **still_needed**: No.
- **options**: Overclock: out of spec (datasheet p.4 requires T0H+T0L >= 1.25 µs). 24-bit mode: the WS2816C has only 48-bit frames. 2×40 px lanes: needs strip rework. PCM1808: U18 lacks an SSIE receive group (PLATFORM_MEMORY). GPU/MIPI/Ethernet: no light value now.
- **impact**: 1
- **novelty**: 1
- **effort**: 1
- **experiment**: None.

## Failures mapped

- Colour over-averaged to one colour per frame (fixed in v9 path P). It was an S3-budget compression; with about 28% CPU and a TRUE16 wire, no further colour compression is justified. The remaining output colour losses are HW-06 (internal WS2816 gamma) and HW-07 (Emotiscope RGB8×257).
- Drop does not hit harder than the build. HW-01: hardware HPF at about 259 Hz cuts kick and bass 8-14 dB. HW-02: the loud guard cuts gain on the loudest slots. HW-09: sub-frame motion for the explosive release. HW-08: cadence and latency. HW-16: build-up memory. HW-06: low-end gamma (P6 found drop luma falls ×0.80 in variant B).
- In-house analysis blind below 110 Hz; sub-bass 70-120 ms late. HW-01 (hardware bass cut plus phase lead), HW-18 (affordable sub-bass resolution or time-domain envelope), HW-05 (instant amplitude pre-trigger).
- Three drop detectors missed +50 ms. HW-01 and HW-02 (input truth), HW-16 (structure memory), HW-14 (learned detector).
- Tempo locks half-time (88 BPM prior). HW-14 (CPU-first learned beat/downbeat), HW-01 (kick salience restored).
- Latency about 16 ms typical (P5). HW-08 removes the 0-10 ms tick wait; HW-19 shrinks slot fill; HW-04 removes ASRC delay; HW-05 adds a pre-trigger; HW-11 gives a declared present-time.
- int16 railing of the onboard capsules (PLATFORM_MEMORY). HW-02 (20-bit PCM width).
- ITCM image did not boot (PL14-A4). HW-17.
- Product default did not select the fix (PL14-B14). The same pattern exists in HW-07 (16-bit donor path present, default 0); HW-10's guard test applies the PL14-B14 remedy to bit depth.
- Air, microphone and PDM-filter latency unmeasured (P5 BUDGET). HW-13 (on-board speaker self-test), HW-03 (paired lanes), HW-11 (stamped presentation).

## Contradictions

- C-1: The brief says '120 Hz logical render target'. The running product is 100 Hz: kPalettePeriodUs=10000 under K1_EMOTISCOPE_PORT (palette_runtime.h:39-40), measured 100.01 fps (P5 BUDGET.md). Canon §5 targets 8.33 ms.
- C-2: The brief says 'two boards, each a 160-pixel scene split 80/80 across P601/P604'. Code: board 0 uses P601/GPT6 + P604/GPT8 (DMA0/3); board 1 uses P603/GPT7A + P714/GPT12B (DMA6/7, ELC E-H) behind default-disabled --titan-top-output (ws281x_gpt_dma_hw_top.c:1-45). Canon §5 says 4 data lines × 80 px.
- C-3: The same AGENTS.md says 'maximum brightness 24/255' (continuous-music block) and 'default brightness 100/255 minimum' (29 Sep brightness mandate). Not resolved here, as instructed.
- C-4: Canon §3 offers '24-bit + dither' output to save wire time, but the WS2816C datasheet (p.4) defines only a 48-bit frame. No 24-bit mode exists on the fitted LED.
- C-5: The W4 WS2816-PROTOCOL-EVIDENCE.json assumes 160 px per line (9.6 ms, about 101 Hz ceiling, STOP). Canon §5 and the code use 80 px per lane (5.1 ms, about 196 Hz ceiling).
- C-6: AGENTS.md operating contract items 4-5 (scalar parity, no MVE, M33/U55 parked) and STATUS.md 'D-cache off' and 'G4/F2 failed' describe the hobbled profile. build_scalar.py titan-product (MVE, D-cache on, O3) and canon §4/§5 supersede them; canon declares G4/F2 VOID, but STATUS still headlines it.
- C-7: The mandate says mic DMA1/2 and LED DMA0/3. pdm_target.h defaults are RISE DMA0 / FALL DMA1, overridden to 1/2 only under the build flag dmac-lane-map pdm-first (build_scalar.py:850-852); led-first uses 0/1 (:863-865).
- C-8: Measured MOCO PDM rate is 41,408 Hz (P5 BUDGET) versus 41,417 Hz (PL14-B12): small, but evidence of drift.
- C-9: The BSP HPF coefficients (manual note: 100 Hz cut-off at the design rate) are reused at about 2.6× the design sample rate with no recorded retune. Inferred effective cut-off is about 259 Hz.
- C-10: Canon §5 says the v8 boot composition has 'both outputs ON', but the build flag help for --titan-top-output says 'optional default-disabled'. Whether the second board is on in the Captain's current image needs readback.

## Open questions

- Is the HPF sample rate exactly 2·Fout, giving fc about 259 Hz? This needs an on-device sweep with the HW-03 paired lane, or the exact manual statement (I did not locate it fully in §50).
- Do the PDM compensation-filter and LPF reset coefficients also assume the 16 kHz design? If so, what treble shaping happens at SINCDEC 49?
- What is the WS2816C internal 4-bit gamma curve? The datasheet gives no curve, so the HW-06 ramp measurement must decide.
- Is a speaker fitted to J4 on the Captain's Titan? (Decides HW-13.)
- Is the Titan USB port wired as USB high speed or full speed? (HW-20.)
- How much CPU do RT-Thread, USB-CDC telemetry and interrupt service take? An idle-loop counter would turn the ~65-72% free estimate into a measurement.
- Which brightness cap binds now, 24/255 or 100/255? (Captain.)
- Is the second board (top output) enabled in the currently installed image?
- Do the BSP I2S MCLK timer (GPT2) or a CAC configuration collide with the LED GPT/DMA/ELC resources?
- Does the Captain want any audible self-test (HW-13) or tap control (HW-12)? These are product decisions.
- Process: the harness refused writing REPORT.md from this subagent. The parent must persist this result into D10-hardware-leverage/REPORT.md (folder created, empty).
