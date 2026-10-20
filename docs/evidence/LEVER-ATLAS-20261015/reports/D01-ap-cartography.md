# D01-ap-cartography — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

The Titan's sound analysis is not short of power. It uses about 0.7 ms of every 7.5 ms on one M85 core; the whole audio stage uses 1 ms of every 5 ms. What flattens the drop is inherited smoothing and loudness levelling, not the budget.

CHAIN (24 kHz v9 build; working tree at HEAD 1ef5395, uncommitted v9 edits; line numbers from that tree):
A1 microphone, LMD2718T261 (titan_onboard_mic.h:4). PDM clocked by MOCO, 41.41 kHz measured.
A2 PDM hardware filters: sinc4 (sincdec 49, SINCRNG 10), compensation filter, half-band LPF, then an HPF whose coefficients (s0 0x3F61, k1 0x3EC1) come from the configurator's 16 kHz setup (hal_data.c:617-619; configuration.xml:2262). Output is a 16-bit window of bits [14:0].
A3 DMA, 296-sample slots (7.15 ms). Only microphone U14 is kept; U13 is discarded (pdm_target.c:607-609).
A4 Gain x16 plus a per-block loud guard that cuts the block's gain to keep its peak at or below 30000 (k1_pdm_sensitivity.h; pdm_target.c:612-636).
B1 Donor ASRC to 12.8 kHz, 95-tap filter with a 5.2 kHz cutoff. Feeds the donor analysis and the admission gate.
B2 k1_poly24 to 24 kHz / 180 samples: 96-tap Kaiser filter, 1.14 ms delay.
C1 4096-sample history window (171 ms).
C3 raw_rms / vu_level: unsmoothed, the best-preserved level signal.
C4 sub_rms: 66 Hz band-pass, one RMS per hop, v9 only.
C5 peak follower: tau 30 ms up, 1.99 s down, ratio smoothed 5.4/57 ms. Halves the dB range (BOOM 19.3 to 8.6 dB, P3).
C6 silence flag: RMS 0.04 to enter, 0.08 to exit, 5 s dwell.
C7 GDFT, 80 bins, 110 Hz to 10.5 kHz. Rectangular window, input shifted right by 6 bits, Cross40 halves bass resolution, k rounded to an integer. Windows run from 76 ms (bass) to 1.7 ms (top); 35,251 MACs per hop.
C8 hop EMA 0.3, tau 21 ms.
C9 mood low-pass: coefficient designed for 100 Hz, applied at 133 Hz, so tau is 79.6 ms.
C10 spectral AGC: envelope 26 ms up / 490 ms down; gain tau 190 ms, target 0.25, range 0.1 to 10; noise gate.
C11 tilt, soft knee at 0.5, clamp at 1.
C12 linear_shape (path P): taken before C10/C11.
C13 novelty: computed after C8 to C11.
C14 spectral energy.
C15 tempo tracker: 60 to 155 BPM, 88 BPM prior, 11.5 s history, 512-update warm-up.
D1 capture to publish: 6.0 ms p50 (P5).
D2 the 100 Hz renderer reads only the newest 133 Hz hop; only the peak has a maximum-since-last-read summary.
D3 donor peak-squared admission gate (0.0048 / 0.0034); peak and VU clamped to 1.
D4 renderer chroma fold and tonality gate (VP boundary).

PROCESSING TIME vs FEATURE SPEED. A sample reaches publication in about 6 ms. The features then need about 112 ms to show half of a new bass level (90 % at 248 ms) because of C8 and C9.

KEY MEASURED FACTS (P3 BOOM trace):
- Build to drop: raw_rms rises 0.118 to 0.205 (+4.8 dB), but spectral_energy rises only 0.660 to 0.709 (+7 %).
- Novelty is lower in the drop than the build: 0.0345 vs 0.0437.
- Median spectral energy is 0.63, so the median bin sits inside the knee.
- The first hop after a step carries only 2.7 % of the jump into novelty.

NEW FAULTS IN CODE THE TITAN INHERITED:
1. Rounded k: bins 0 to 39 are about 85 cents flat, bins 40 to 79 about 20 cents sharp. The same flaw was confirmed on K1 hardware on 2026-06-21. Fixing it costs nothing.
2. Treble windows: 27 bins at 2.35 kHz and above cover on average 50 % of each hop; bin 79 covers 22 %.
3. HPF (hypothesis): if it is a one-pole filter at the 41.4 kHz output rate, its corner moved from about 50 Hz to about 128 Hz. That would attenuate the 40 to 110 Hz drop band by 4 to 10 dB.

ONLY CLEARLY BUDGET-DRIVEN STAGES: the 6-bit input shift, the rectangular window and Cross40 (S3 integer LANE4 path).

IDLE CAPABILITIES:
- 20-bit PDM capture.
- The second microphone.
- The PRISM AudioPipeline (kick, snare, hi-hat onsets), compiled out under K1_EMOTISCOPE_PORT.
- The donor VU, which rises x3.93 at the drop against x1.52 in-house.
- Helium (enabled, unused by AP code) and the Ethos-U55 (unused).

## Levers

### L01-gdft-exact-tuning — C7 makeGdftConfiguration rounded_bin, src/k1/core/audio/gdft_goertzel.cpp:121-128 (same in emotiscope_donor/goertzel.h:81)
- **container**: AP
- **controls**: colour (chroma fold, centroid, path-P centroid), indirectly tonality gate → colour motion
- **current**: v8=v9: bins 0-39 centred -79..-86 cents, bins 40-79 +19..+42 cents (one -48); bin0 resonates 104.7 Hz not 110 [FACT computed from code; K1 device-verified 2026-06-21: 440 Hz lit B4 bin, gdft-bin-center-honesty.md]
- **info_loss**: low register votes the pitch class one semitone up vs high register → ~30 deg disagreement on the chroma circle; [INFERENCE] depresses centroid_strength/flatness (gate>=0.5 only 3-23% of frames, inhouse_waveform.cpp:662)
- **origin**: inherited (Emotiscope/SensoryBridge integer-k Goertzel)
- **still_needed**: no; fractional k = same recurrence, 0 extra cycles
- **options**: exact k=N*f/fs; or relabel chroma fold to each bin's true pitch class; or choose N so k is integral near target
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: Host: chromatic scale + C/F/G triads at 24 kHz through InhouseAudio+renderer chroma. Frozen: >=95% of frames assign each pure tone 110 Hz-3.3 kHz to its own pitch class (baseline ~50%), triad median centroid_strength +>=25%. Planted fault: re-insert integer rounding → red.

### L02-bass-resolution-cross40 — C7 kX2CrossoverBin=40, gdft_goertzel.cpp:115-119
- **container**: AP
- **controls**: colour (bass chroma), spatial bass detail
- **current**: bins 0-39 block = fs/(2*spacing): main-lobe nulls at +-2 semitones (~4-semitone overlap); bass window 76 ms (153 ms without halving)
- **info_loss**: bass pitch resolution halved; adjacent bass notes indistinguishable [INFERENCE]
- **origin**: S3 budget/latency [INFERENCE]: removes 29.4k of 64.7k MAC/hop
- **still_needed**: latency part yes, budget part no (64.7k MAC ≈1.3 ms scalar, far less with L17)
- **options**: dual-resolution bass (fast 76 ms level + slow 153 ms pitch); better window (L03); decimated constant-Q bass pass
- **impact**: 2
- **novelty**: 3
- **effort**: 2
- **experiment**: Host A2+A#2 two-tone. Frozen: slow pitch path shows >=6 dB valley between peaks. Planted: collapse to fast window → red.

### L03-window-shape — C7 kernel, platform/ra8p1/inhouse_audio.cpp:45-71 (no window)
- **container**: AP
- **controls**: colour purity, spectrum shape across strip
- **current**: rectangular, -13 dB sidelobes
- **info_loss**: leakage of loud bass into neighbours flattens chroma [INFERENCE]
- **origin**: S3 budget: K1 LANE4 exact int path forbids windowing (k1_gdft_core.cpp:65-67); Emotiscope used Gaussian sigma 0.8; K1 has gated Hann
- **still_needed**: no; ~35k extra multiplies/hop
- **options**: Hann (K1_HANN_COHERENT_GAIN compensation exists), Gaussian, Blackman-Harris for chroma bins
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Host single 220 Hz tone. Frozen: energy outside +-2 bins drops >=10 dB. Planted: rectangular stays red.

### L04-goertzel-input-precision — C7 q0=(x>>6)+..., inhouse_audio.cpp:39, gdft_goertzel.cpp:54-56
- **container**: AP
- **controls**: brightness/colour in quiet passages, fades, noise floor
- **current**: 10-bit effective input; P3 median rms ≈61 LSB post-shift, quiet cut ≈31, post-roll room noise ≈9
- **info_loss**: ≈6 bits discarded; low impact loud, moderate in tails [INFERENCE]
- **origin**: S3 budget (int32 recurrence headroom)
- **still_needed**: no (float or int64 on M85)
- **options**: float32 recurrence (MVE-friendly), or >>2 with int64 state
- **impact**: 1
- **novelty**: 2
- **effort**: 2
- **experiment**: Host -50 dBFS tone in -70 dBFS noise. Frozen: in-bin SNR +>=6 dB. Planted: restore >>6 → red.

### L05-pdm-20bit-no-loud-guard — A2/A4 pcm_width PDM_PCM_WIDTH_16_BITS_0_14 (hal_data.c:643), x16 + per-block guard (k1_pdm_sensitivity.h:9-34, pdm_target.c:612-636)
- **container**: AP
- **controls**: brightness/motion on loudest hits
- **current**: 16-bit window [14:0], software x16, block gain cut so peak <=30000 (7.15 ms steps), int16 saturation
- **info_loss**: block-wise gain steps and clipping at peaks; host replay: 2.3% of hops >=32000, 8% of drop hops >=29000 (device fraction unknown)
- **origin**: inherited (DualMCU x16 analogue) + BSP default width
- **still_needed**: no; ASYMMETRIC: hardware offers PDM_PCM_WIDTH_20_BITS_0_18 (r_pdm_api.h:43)
- **options**: 20-bit FIFO into int32/float pipeline; or 16_BITS_4_18 headroom window
- **impact**: 3
- **novelty**: 4
- **effort**: 3
- **experiment**: Host model guard vs none on BOOM. Frozen: 0 drop-window hops with >1 dB hop-to-hop gain change, drop/build raw_rms ratio moves >=1 dB toward the source file. Planted: guard on → red. Device proof later via 20-bit MIR contract string.

### L06-pdm-hpf-coefficients — A2 HPF s0 0x3F61 k1 0x3EC1 h{0x4000,0xC000} (hal_data.c:617-619) designed at pdm_output_sampling_freq 16000 (configuration.xml:2262); pdm_target.c:764-781 overrides only sincdec/sincrng/threshold
- **container**: AP
- **controls**: brightness/timing of bass and sub (sub_rms v9 trigger, bins 0-5, drop release)
- **current**: [INFERENCE] one-pole DC blocker k1=0.9805: corner ~50 Hz at 16 kHz → ~128 Hz at 41.4 kHz
- **info_loss**: [HYPOTHESIS] -10.5 dB at 40 Hz, -6.8 dB at 66 Hz, -4.2 dB at 110 Hz, exactly where the drop's new energy lives (DETECTOR.md:21)
- **origin**: accident [INFERENCE]: rate changed sincdec 124→49, HPF not re-derived
- **still_needed**: a DC blocker yes; a 128 Hz corner no
- **options**: recompute s0/k1 for ~20 Hz at measured rate; or software DC block
- **impact**: 4
- **novelty**: 5
- **effort**: 1
- **experiment**: (1) Desk: compute |H(f)| at 41,408 Hz from the RA8P1 HPF equation and position. Frozen: attenuation at 66 Hz >=3 dB decides go; planted 16 kHz-rate evaluation must give <=3 dB. (2) Device (separate authorisation): 30-300 Hz sweep, shipped vs recomputed coefficients. Frozen: 50/200 Hz response ratio rises >=4 dB; swapping back must fail.

### L07-second-microphone — A3 programme-lane-only copy, pdm_target.c:607-609
- **container**: AP
- **controls**: brightness at noise floor, quiet detail
- **current**: U13 captured, reduced to metrics, discarded
- **info_loss**: [INFERENCE] ~3 dB SNR from summing uncorrelated self-noise; possible handling-noise rejection
- **origin**: inherited (diagnostic lane)
- **still_needed**: discard not needed; DMA already runs
- **options**: sum; difference as noise reference; room feature
- **impact**: 1
- **novelty**: 4
- **effort**: 2
- **experiment**: Host from a dual-lane capture (or device). Frozen: summed noise floor >=2.5 dB below single mic in silence. Planted: duplicated lane shows 0 dB.

### L08-fast-slow-layers — C8 EMA 0.3 (inhouse_audio.cpp:272-278) + C9 mood LPF (97-113, 323-326)
- **container**: AP
- **controls**: motion and timing of every spectrum consumer
- **current**: tau 21 ms then 79.6 ms (mood 0.05, coefficient for 100 Hz run at 133 Hz); bass step 50% at 112 ms, 90% at 248 ms
- **info_loss**: onsets inside ~100 ms smeared; mood off by 25%
- **origin**: aesthetic (SensoryBridge mood) + accident (rate quirk admitted at :314-316)
- **still_needed**: as a layer yes, as the only path no
- **options**: publish raw per-hop spectrum + mood-smoothed; instant attack / mood release; fix coefficient to 133.3 Hz
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Host 150 ms burst + 220 Hz onset. Frozen: fast layer reaches 50% within 15 ms (baseline 112 ms), slow layer within +-2% of baseline. Planted: slow routed into fast field → red.

### L09-section-preserving-loudness — C10 spectral AGC (inhouse_audio.cpp:329-340) + C5 peak follower (247-254)
- **container**: AP
- **controls**: brightness; build-vs-drop contrast (hits harder)
- **current**: target 0.25, env 26/490 ms, gain tau 190 ms, 0.1-10x; follower 30 ms/1.99 s
- **info_loss**: BOOM 19.3→8.6 dB; in-house x1.52 vs donor VU x3.93 at drop; raw +4.8 dB → spectral_energy +7%; lifts room noise to music level (peak_scaled 0.68 at rms 0.018)
- **origin**: inherited (K1 AGC)
- **still_needed**: minute-scale room normalisation yes; sub-second phrase normalisation no
- **options**: slow room normaliser (20-60 s) + published section_gain/loudness feature; freeze gain in detected cuts; publish unnormalised envelope beside normalised
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Host BOOM+Sonic. Frozen: published drop/build energy ratio >=60% of the source-file RMS drop/build ratio in dB (baseline ≈13%); post-roll room noise yields 0 admitted frames. Planted: tau 190 ms → red.

### L10-no-knee-in-published-spectrum — C11 tilt/knee/clamp, inhouse_audio.cpp:343-363
- **container**: AP
- **controls**: colour (tonality gate, chroma) and brightness at loud levels
- **current**: knee at 0.5, pre-knee >=1.5 → 1.0; median spectral_energy 0.63 (inside knee); chroma energy 0.017 build → 0.006 drop (P3)
- **info_loss**: top of range flattened; gate closes in loud passages
- **origin**: aesthetic/inherited
- **still_needed**: display maybe; analysis no (path P already proved pre-knee value for centroid)
- **options**: publish linear_shape (C12) as the general spectrum; knee per VP consumer; feed tonality gate from C12
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Host BOOM. Frozen: share of drop frames with gate>=0.5 at least doubles vs knee'd baseline, quiet cut <= baseline+5 points. Planted: knee'd vector → red.

### L11-raw-novelty-flux — C13 novelty, inhouse_audio.cpp:357,365-367
- **container**: AP
- **controls**: timing/motion (onsets, tempo input, drop detection)
- **current**: flux on post-EMA/LPF/AGC/knee spectrum; first hop sees 2.7% of a step; novelty drop 0.0345 < build 0.0437
- **info_loss**: a +23 dB click at the BOOM drop (DETECTOR.md:63) is diluted to a few percent
- **origin**: inherited (K1 calculate_novelty)
- **still_needed**: no
- **options**: log-magnitude (superflux) per band on C7 output; per-band novelty with max-since-cursor (L15)
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Host BOOM. Frozen: novelty peak within +-30 ms of the 55.7 s drop exceeds the build-window (45-50 s) p95 by >=2x (baseline below p95). Planted: compute on C11 output → red.

### L12-peak-headroom — D3 response clamp, platform/ra8p1/palette_runtime.cpp:266-271
- **container**: map
- **controls**: brightness/motion of loudest hits
- **current**: peak_scaled, vu and observed_peak clamped to 1 for modes 32/18; P3 peak_scaled p95 1.029, max 1.83
- **info_loss**: top ~5% of peaks identical
- **origin**: inherited
- **still_needed**: analysis no; display maybe
- **options**: soft-saturate in VP after mapping; publish an overshoot feature
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: Host. Frozen: >=3 distinct output levels in the top 5% of peak hops (baseline 1). Planted: re-clamp → red.

### L13-sub-bass-first-class — C4 sub_band.h:15-56 (v9 only) + C7 lowest bin 104.7 Hz
- **container**: AP
- **controls**: timing/brightness/motion of drop release
- **current**: v8 blind below ~100 Hz; v9 one band-pass RMS/hop, no envelope or onset, input possibly cut by L06; sub arrives 70-120 ms after the musical drop
- **info_loss**: the kick/sub body that 'hits' is absent from spectrum, colour, novelty
- **origin**: inherited (note bank starts at notes[12]) + budget (55 Hz semitone bin needs ~150 ms)
- **still_needed**: no
- **options**: 3-4 band sub filterbank (30-50/50-80/80-110) with fast envelopes and log-flux; coarse 12-note downward extension; sub-onset with max-since-cursor
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Host: DETECTOR.md:48 synthetic pure-sub return + BOOM 55.7 s. Frozen: sub-onset within 25 ms of synthetic bass return, no fire on 3 kHz snare bursts. Planted: existing K1_SUBBAND_PLANT_WRONG_CENTRE (660 Hz) → red.

### L14-tempo-range-prior-warmup — C15 tempo_tracker.cpp:17-48,64-100; tempo_acf.h:11
- **container**: AP
- **controls**: timing/motion (tempo-mode rate, inhouse_waveform.cpp:374-384,980-992)
- **current**: 60-155 BPM, 88 BPM tactus prior sigma 0.75 oct, 11.5 s history, 512-update warm-up (>=10.5 s), silence zeroing; 0/326 synthetic cases (P7)
- **info_loss**: half time / two-thirds time; predicted downbeats 26-380 ms off
- **origin**: inherited + aesthetic (88 prior)
- **still_needed**: no (P7: re-centring gives 138 BPM on BOOM)
- **options**: P7 step-3 design; L11 novelty input; range to 200 BPM; beat-level stage
- **impact**: 4
- **novelty**: 2
- **effort**: 3
- **experiment**: Existing frozen P7/gate/GATE-SYNTHETIC-v1.json (326 cases, planted faults proven red).

### L15-lossless-event-handoff — D2 acquire, inhouse_audio.cpp:377-430; fixture_app.cpp:572-577
- **container**: map
- **controls**: timing (events lost between frames)
- **current**: only peak_scaled has max-since-cursor; novelty/sub_rms/spectrum/tempo newest-hop only; 133 Hz hops vs 100 Hz frames → ~25% of hops never read [INFERENCE]
- **info_loss**: single-hop events in ~1 hop in 4 vanish
- **origin**: inherited
- **still_needed**: no; cursor and peaks_ ring already exist
- **options**: extend the peaks_ ring to novelty, sub-onset, per-band flux
- **impact**: 2
- **novelty**: 3
- **effort**: 1
- **experiment**: Host 1000 random single-hop impulses. Frozen: share reaching the renderer rises from ~75% to 100%. Planted: newest-only → red.

### L16-idle-existing-machinery — k1_live_runtime.h:133-134 (AudioPipeline only #ifndef K1_EMOTISCOPE_PORT); audio_features_v1.h:61-80; donor K1EmotiscopeStatus (emotiscope_port.h); npu_load.h
- **container**: system
- **controls**: timing/colour (drum events, better level)
- **current**: PRISM onset_beat (kick/snare/hihat/bass onset, saliency, tempo field) compiled out; donor VU/chromagram/96 tempi used only for admission; Ethos-U55 idle (probe only); Emotiscope ML autoencoder runs on CPU over donor 64 bins (ml_enabled, strength 0.4)
- **info_loss**: donor VU follows the drop x3.93 vs in-house x1.52 (P3 RECEIPT:42)
- **origin**: accident of migration (Emotiscope port replaced the PRISM pipeline)
- **still_needed**: donor already runs; its VU is free
- **options**: publish donor VU into InhouseWaveformAudio; port onset_beat onto the 24 kHz path; [HYPOTHESIS] NPU onset/beat model (BeatNet, madmom on disk)
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Host P3 recording. Frozen: donor VU published through the in-house struct reproduces drop/build x3.93 +-10%. Planted: wire in-house vu instead → ~x1.5 → red. Licence check first (GPL-lineage BENCH donor).

### L17-helium-float-gdft — C7 kernel, inhouse_audio.cpp:45-71
- **container**: system
- **controls**: enabler for L02/L03/L04/L21
- **current**: 0.70 ms per 7.5 ms (P4, f2f1983a), ≈20 cycles/MAC [INFERENCE]; scalar int64 with per-sample overflow branch; +mve.fp enabled (build_scalar.py:40) but unused by AP
- **info_loss**: none itself; headroom lever
- **origin**: inherited (K1 LANE4 exactness contract)
- **still_needed**: scalar not needed
- **options**: float32 Goertzel with 4-8 bins per Q register sharing the prefix; sliding DFT/FFT for treble
- **impact**: 1
- **novelty**: 3
- **effort**: 3
- **experiment**: Device cost capture (separate authorisation). Frozen: in-house p50 -40% vs f2f1983a with per-bin r>=0.999 on the P4 gate. Planted: 5% coefficient error → red. CMSIS precedent was slower (PL14-B5): measure, do not assume.

### L18-darkness-gates-calibrated — C6 silence (inhouse_audio.cpp:255-266), C10 noise gate (329-340), D3 BloomAdmission (composed_scene_runtime.h:24-37), tempo silence detector
- **container**: map
- **controls**: brightness (true black), always-visualising
- **current**: four gates; admission reads the 5.2 kHz-limited donor (B1), so hats-only content may fail [INFERENCE]
- **info_loss**: possible hats-only blackout
- **origin**: aesthetic/safety (true black mandatory)
- **still_needed**: yes; DO NOT add a floor
- **options**: admission on the 24 kHz path; publish which gate darkened each frame
- **impact**: 2
- **novelty**: 2
- **effort**: 1
- **experiment**: Host. Frozen: hats-only clip at -25 dBFS opens admission >=90% of frames, post-roll room noise 0 admitted frames. Planted: admission forced open → post-roll red.

### L19-chroma-register-range — D4 renderer chroma fold i%12, chroma_range 60, normaliser (inhouse_waveform.cpp:154-163)
- **container**: map
- **controls**: colour
- **current**: 110 Hz-3.3 kHz; fold assumes bin = semitone (broken by L01); normaliser rise ≈0.2 s, decay ≈10 s; new 6.4-10.5 kHz bins never reach chroma
- **info_loss**: [INFERENCE] the bass register dominates at tilt x1.3
- **origin**: inherited
- **still_needed**: unknown (taste)
- **options**: register weighting; 150 Hz-2 kHz chroma band; fold by true bin pitch
- **impact**: 2
- **novelty**: 2
- **effort**: 1
- **experiment**: Run the L01 tone test per register.

### L20-timbre-features — not implemented ('centroid' in code = chroma circular centroid, not spectral centroid)
- **container**: AP
- **controls**: colour (saturation/temperature within the palette), motion
- **current**: absent
- **info_loss**: the bright/dark timbre axis is never published
- **origin**: never implemented
- **still_needed**: ≈160 MAC/hop
- **options**: spectral centroid, rolloff and flatness on C7 or C12; map within the palette, never hue cycling (no rainbow default)
- **impact**: 3
- **novelty**: 4
- **effort**: 1
- **experiment**: Host BOOM. Frozen: spectral centroid separates riser (49.7-54 s) from cut by >=20% with Spearman >=0.7 vs the source file's centroid. Planted: shuffled bins → red.

### L21-treble-time-coverage — C7 bins 53-79 (window < 180-sample hop)
- **container**: AP
- **controls**: timing/brightness of hi-hats and transients
- **current**: 27 bins >=2.35 kHz see on average 50% of each 7.5 ms hop; bin 79 sees 22% (40 of 180 samples) [FACT computed from code]
- **info_loss**: about half of treble events are never analysed [INFERENCE]
- **origin**: inherited (hop fixed by the 12.8 kHz/96 cadence; windows sized for frequency resolution)
- **still_needed**: no; repeat short windows ceil(180/N) times per hop with peak-hold (~+12k MAC/hop [INFERENCE])
- **options**: sub-hop evaluation with max/energy sum; high-band PCM envelope follower
- **impact**: 3
- **novelty**: 5
- **effort**: 2
- **experiment**: Host 1000 random-position 2 ms 8 kHz clicks. Frozen: detection in bins 70-79 >=95% (baseline expected ≈30%). Planted: newest-window-only → red.

## Failures mapped

- The drop does not hit harder than the build. AP evidence: the sound rises 4.8 dB but published spectral energy rises only 7 %, and novelty is lower in the drop than the build (P3 trace). Levers L09, L10, L11, L13, L06, L08, L12.
- Palette over-averaging (fixed by v9 path P). It belongs to the same compression class as L10. The path-P centroid still inherits the detuning (L01) and the 80 ms smear (L08).
- Drop detectors v1 to v3 missed or fired late (P6-DROP-DETECTOR). Sub-bass blind or late, transients diluted, events dropped between frames: L13, L06, L11, L15.
- Tempo locks to half time and two-thirds time (P7): 88 BPM prior, 155 BPM ceiling, 512-update warm-up, and novelty fed from smoothed data. L14 and L11.
- The AGC lifts room noise to music level (P3-RECORD:107). L09 and L18.
- Hi-hat bands were zero at 12.8 kHz. Fixed in P4; the remaining time-coverage gap is L21.
- PL14-B14 (a host variant was not the product default). Every lever here needs a product-default equivalence test before any flash.

## Contradictions

- Brightness. The global CLAUDE.md (Captain, 27 Sep) says preserve maximum 24/255. Kx-Titan/AGENTS.md:12 says '100/255 supersedes historical 24'. scripts/build_scalar.py rejects boot brightness outside 100-255 ('user mandate'). The v9 preset UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json sets hardware_output_brightness 255 with primary_intensity 0.25. P6 RECEIPT:316 speaks of 'the 24/255 cap'. Reported, not resolved.
- Render rate. The swarm brief says a 120 Hz logical render target. The code and evidence say 100 Hz: period_us 10000 (live_boot_composition.py), render_hz 100, and 100.01 fps measured (P5 BUDGET.md stage f).
- The lowest bin is labelled 110 Hz but the Goertzel resonates at 104.7 Hz. The whole low half is about 85 cents flat.
- K1's gdft-bin-center-honesty.md says k=8 for every bin. That describes an older, non-Cross40 profile. The Titan's Cross40 configuration gives k=8 for bins 0-39 and k=17 (+20 cents) for bins 40-79.
- Mood time constant. The source intended tau ≈106 ms (100 Hz call rate). On the Titan it is 79.6 ms because the filter runs at 133 Hz; the code calls this a quirk and keeps it (inhouse_audio.cpp:314-316).
- The P4 receipt says the new hi-hat bands are 'real input to the colour (chroma)'. The renderer chroma folds only chroma_range=60 bins (up to 3.3 kHz), so the 6.4-10.5 kHz bins never reach the chroma. They do reach novelty and spectral energy.
- Process note: the assignment required REPORT.md in the run folder, but the harness refused subagent report files. The content is returned here instead.

## Open questions

- Where does the RA8P1 PDM high-pass filter sit, and what is its equation? Is the corner at 41.4 kHz really about 128 Hz? This decides L06 and needs the hardware manual.
- What is the LMD2718T261-OA1 microphone's low-frequency response, and has its acoustic identity been proven?
- Did K1 ever run feat/gdft-true-center-ab (exact-frequency coefficients), and what did the Captain conclude?
- On the device, how often does the loud guard engage, and how often does int16 clip during drops? The host replay omits the guard.
- Can U13 and U14 be summed without comb filtering in the 110 Hz-10 kHz band, given their spacing on the board?
- What does a vectorised float Goertzel actually cost on the M85? CMSIS was slower in P4, so measure rather than assume.
- Is the GPL-lineage donor's VU admissible as a product feature (L16), or must it be re-derived clean-room?
- Is the Cortex-M33 core used at all by the product image? Not checked.
- The P3 trace predates P4 (12.8 kHz analysis). The build/drop compression numbers should be re-run on the 24 kHz v9 replay before freezing criteria.
