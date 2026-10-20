# D13-time-and-latency — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

1. The plumbing is quick; the smoothing is slow. Sample to the end of the LED wire takes about 11 ms; with the 10 ms frame-tick wait it is about 16 ms (P5 BUDGET.md:33-34) [FACT].
2. Most lateness is deliberate smoothing:
   - Extent (waveform length): two filters in a row reach half the new value after about 20 ms and 90 % after about 70 ms (inhouse_waveform.cpp:983-984).
   - Onsets: computed after an inherited roughly 2 Hz "Mood" low-pass (tau 80 ms, inhouse_audio.cpp:103-111, 323-367). Onset peaks land 37–83 ms late and are smeared over about 110 ms. The beat tracker and mode 18 both read this signal. [FACT code; INFERENCE model]
   - Path P colour: built on the same low-passed spectrum. Half its change takes 75–113 ms and 90 % takes 210–250 ms, so the first 200 ms sweep of a drop still carries the build's colour.
3. The broadcast lip-sync standard (ITU-R BT.1359) says light trailing sound is noticed beyond about 45 ms and unacceptable beyond about 90 ms.
   - Brightness: about 20 ms, OK. Extent: about 40 ms, borderline. Onsets: about 57–103 ms, past the limit.
   - A release triggered by the sub-bass lands about 100–145 ms after the musical drop. That reads as a late reaction, not a hit.
4. The sub-bass really does arrive 72–117 ms after the musical moment; the music causes this, not the firmware (P6 v2 DETECTOR.md:86-87). No causal sub-bass trigger can meet a 50 ms deadline. The fixes:
   - fire the release on a fast transient while armed by the build or cut, and let the sub-bass add body when it arrives; or
   - fire on a predicted downbeat advanced by the 16 ms pipeline delay.
   - Best is both: prediction picks the moment, the transient sets the exact instant, the sub-bass confirms.
5. Prediction is blocked today: the tracker locks to half time, needs 11.5 s to lock, and its beat phase inherits the onset delay above.
6. The AGC peak follower has a 30 ms attack, so a drop's surge in level is cancelled within about 3 frames. How much it stands out depends on how long the preceding cut lasted.
7. Cheapest wins: compute onsets before the low-pass, give extent an instant attack, render when audio arrives instead of on the tick, project beat phase forward to the moment light leaves the LED, and A/B the existing live Mood control (field 64; 0.05 now).
8. Realistic floor on the current boards is about 10–15 ms from sound to light for a hit. The 5.1 ms wire is fixed without more data lines.

## Levers

### L01 — AP: InhouseAudio::condition novelty, platform/ra8p1/inhouse_audio.cpp:341-367 (reads post-LPF spectrum from :323-327,351-358)
- **container**: AP
- **controls**: timing and sharpness of every onset-driven visual (mode 18 energy rate inhouse_waveform.cpp:1001-1002; v9 accent :744-750; tempo tracker input inhouse_audio.cpp:295-297)
- **current**: v8=v9: novelty from post-Mood-LPF knee'd spectrum; onset peak 37.5-82.5 ms after a step, half-width 105-120 ms at mood 0.05 [INFERENCE model, novelty_and_mood.json]
- **info_loss**: destroys onset time resolution (~LPF tau 80 ms) and attack shape; rhythm detail finer than ~100 ms smeared
- **origin**: inherited: K1 process_GDFT LPF (inhouse_audio.h:30-31, .cpp:103-107,314-316); Emotiscope also fed novelty from 12-frame averaged spectrogram (Emotiscope/src1/tempo.h:361, goertzel.h:52, d0fef9a). S3 flicker/budget trade [INFERENCE]
- **still_needed**: no for timing (yes for display spectrum); pre-LPF diff over 80 bins ~200 cycles/hop [INFERENCE]
- **options**: diff magnitude_average_ or raw normalised magnitudes; per-band log flux; keep LPF only for display
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: Click corpus host replay: onset->novelty-peak delay median <=15 ms, p95 <=25 ms (frozen). Planted: novelty re-pointed at post-LPF spectrum must fail. Risk: noisier novelty needs re-thresholding.

### L02 — AP: InhouseAudio::setMood inhouse_audio.cpp:97-112; live control field 64 emotiscope_control.h:594,1051; default 0.05 emotiscope_control.h:265
- **container**: AP
- **controls**: timing of displayed spectrum, chroma, path-P colour (linear_shape inhouse_audio.cpp:347-349)
- **current**: mood 0.05 -> effective cutoff 2.0 Hz, tau 79.6 ms (100 Hz coefficient applied per 7.5 ms hop); chain t50 75-113 ms, t90 210-248 ms
- **info_loss**: removes spectral motion above ~2 Hz (about the beat rate)
- **origin**: inherited SensoryBridge/K1 user 'Mood' feel control (K1 BLE-MIDI primary.mood); 0.05 from reconstruction
- **still_needed**: aesthetic only, not budget
- **options**: mood 0.25 (t50 -30 ms), 1.0 (tau 14 ms); correct coefficient basis; split slow-colour vs fast-shape LPF
- **impact**: 3
- **novelty**: 1
- **effort**: 1
- **experiment**: Host replay mood {0.05,0.25,0.5}: frozen (a) colour-position t50 after synthetic chord change <=60 ms at 0.25, (b) sustained-chord frame jitter <= 0.05-value + 0.002. Planted mood=0 must fail (a). Zero code: existing live control.

### L03 — VP: renderV9 extent cascade inhouse_waveform.cpp:982-986 (EMA 16 ms -> 23 ms, symmetric)
- **container**: VP
- **controls**: motion/space: how fast the waveform extends on a hit
- **current**: t50 20 ms, t90 70 ms both directions (v8 same at :203-204)
- **info_loss**: low-pass on peak dynamics; attack slope lost
- **origin**: inherited Emotiscope smoothing; aesthetic anti-flicker for 8-bit/60 fps [INFERENCE]
- **still_needed**: release half aesthetic; attack half contradicts 'explosive force'
- **options**: asymmetric follow (attack <=1 frame, release unchanged) using existing follow() :34-36
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: Frozen: extent >=0.9 final <=15 ms after synthetic step; sustained-noise per-frame extent RMS change <=1.5x current. Planted: symmetric cascade fails 15 ms clause. Then one Captain A/B on BOOM drop (taste).

### L04 — system: fixed-tick render palette_runtime.h:40 (10000 us under K1_EMOTISCOPE_PORT), :144 frameDue; fixture_app.cpp:574-580,2237,2494
- **container**: system
- **controls**: timing of all output
- **current**: 100 Hz polled tick; wait uniform 0-10 ms, mean 5, p99 9.9 (BUDGET.md:32,81); 100 Hz render beats against 133.3 Hz in-house cadence
- **info_loss**: adds delay only; latest-snapshot fields drop some hops (see L17)
- **origin**: inherited SYSTEM_FPS 100 reconstruction (inhouse_audio.h:41-43); non-Emotiscope build is 8333 us
- **still_needed**: no: wire allows 196 fps, render+pack 0.33 ms
- **options**: render on new in-house generation when DMA idle (coalesce); 200 Hz tick; onset express frame
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Device timing log fields: publish->submit p50 <=0.5 ms, p99 <=5.5 ms, 0 skipped frames (frozen). Planted 10 ms tick in host scheduler model fails p50. Risk: dt discretisation of all constants changes; mode 11 120-basis decay; new tolerance per PL14-B8.

### L05 — map/VP: path P colour applyShapeCentroidP inhouse_waveform.cpp:596-625 (input post-LPF linear_shape) and updateOffsetP :630-651; non-P follower :669-673
- **container**: map
- **controls**: colour timing: which colour a hit is painted
- **current**: P: t50 75-113 ms, t90 210-248 ms after harmonic change; drop's first ~198 ms centre-to-edge sweep carries pre-drop colour. Non-P: 0.6 s follow, 0.25 palette/s cap
- **info_loss**: harmony preserved but delayed (not a budget necessity)
- **origin**: inherited F3 LPF; non-P deliberate 'harmony not beats' (P6 it2/3 :661-667)
- **still_needed**: taste
- **options**: pre-LPF shape for colour; separate fast colour tau; colour snap on armed release (L14)
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: Synthetic chord-change: centre-pixel deposit offset reaches 90% of new centroid <=60 ms (frozen) while PAL-HD v2 'colour moves' second-fraction >= current. Planted frozen offset fails.

### L06 — AP: new short-window band-energy path beside GDFT (precedent src/k1/core/audio/sub_band.h)
- **container**: AP
- **controls**: onset/bass-hit timing; sub-110 Hz content (GDFT starts 110 Hz gdft_goertzel.cpp:26)
- **current**: only sub_rms (40-110 Hz, group delay 4.0-9.7 ms) is short-window; GDFT windows 1.7 ms (10.5 kHz) to 76.4 ms (110 Hz), bin40 crossover 8.0->15.2 ms
- **info_loss**: GDFT trades time for semitone resolution: necessary for pitch, wrong for timing
- **origin**: S3 budget: one bank served both [INFERENCE]
- **still_needed**: single bank: no; 8 biquad bands x2 sections ~5.4 Mcycles/s ~0.5% M85 [INFERENCE from P6 detector op counts]
- **options**: kick/snare/hat bands (P7 beat-stage draft 35-140/170-210/7k-11k Hz), 2 ms frame envelopes
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Click corpus onset-time |error| median <=3 ms, p95 <=8 ms (frozen). Planted: band routed through 76 ms GDFT bin fails. DTCM region must be declared (PL14-B4).

### L07 — AP: analysis hop audio_rate_config.h:17 (24000/180), inhouse_audio.h:22-23
- **container**: AP
- **controls**: timing quantisation of all AP features
- **current**: in-house 7.5 ms (mean +3.75 ms event age), donor 5.0 ms; not in P5 total (BUDGET.md:91)
- **info_loss**: envelope sampled at 133.3 Hz; content kept by observed_peak_scaled, timing rounded
- **origin**: inherited parity contract 'same 7.5 ms hop' (inhouse_audio.h:20-21)
- **still_needed**: unknown; halving doubles GDFT to ~1.43 ms per 3.75 ms (~38% core) unless sliding/recursive Goertzel or low bins every other hop
- **options**: leave GDFT at 7.5 ms and get fast timing from L06
- **impact**: 1
- **novelty**: 1
- **effort**: 3
- **experiment**: Deferred until L06 scored; then in-hop age p50 <=2 ms, planted 180-sample hop fails.

### L08 — AP: AGC peak follower inhouse_audio.cpp:247-253 (attack tau 30 ms eff 33.6; release tau 1.99 s); peak_scaled = drive/follower
- **container**: AP
- **controls**: extent/brightness contrast of drop vs build
- **current**: drop surge normalised within ~30-90 ms; surge ratio ~1.6-2.7x only if preceded by a 1-2 s cut [INFERENCE]
- **info_loss**: destroys absolute dynamics on a 30 ms scale
- **origin**: inherited K1 k1_agc_dt_clock (inhouse_audio.h:30-31)
- **still_needed**: levelling yes, its speed at the drop no
- **options**: armed hold: freeze/slow follower attack while tension/cut armed, release after sub-bass body; or drive extent from pre-AGC dB level (v9 already uses vu dB for amplitude :944-951)
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Host BOOM/Sonic/Carte: peak extent in [drop, +300 ms] >=1.3x max extent in [drop-4 s, drop-0.5 s] on >=5/6 drops, release-level extents elsewhere <= current (frozen). Planted hold-off fails >=2 drops.

### L09 — map/VP: updateReleaseV9 inhouse_waveform.cpp:755-796 (EXT, off by default inhouse_waveform.h:66-76)
- **container**: map
- **controls**: motion/space/brightness of the drop release ('explosive force')
- **current**: off; when on, single surge trigger; sub trigger lands 72-117 ms (signal) + ~9 ms band-pass + ~20 ms plumbing after drop
- **info_loss**: n/a decision layer
- **origin**: P6 frozen candidates X1-X3 failed (STATUS 14:45:39Z)
- **still_needed**: yes, new design
- **options**: two time layers: arm on slow evidence (dip ring :767-793, tension), FIRE on first strong broadband/hi-mid transient (L06) within arm, scale/sustain by sub_rms when it arrives, ease back (no hard cancel) if no body by 150 ms
- **impact**: 5
- **novelty**: 4
- **effort**: 3
- **experiment**: Unchanged v2/v3 frozen hit rule (+50 ms window, 0.5 s collar, 0 false): >=5/6 drops hit, 0 false. Planted 200 ms-late fire misses every drop (existing plant v2 DETECTOR.md:35). Holdout = Captain corpus (none unseen left).

### L10 — system: tempo_tracker.cpp + new bar/downbeat stage + renderV9
- **container**: system
- **controls**: timing of metric events at zero or chosen negative latency
- **current**: impossible: half-time lock (88 BPM prior tempo_tracker.cpp:23-24), beat error 26-380 ms; constrained reference clock 26-74 ms off on 4/6 drops and wrong downbeat (v3 DETECTOR.md:45-71)
- **info_loss**: n/a
- **origin**: inherited V2 tracker prior
- **still_needed**: yes, new
- **options**: fire at predicted downbeat minus measured plumbing (~16 ms); gate L09 causal fire to a +/-50 ms predicted slot (hybrid rejects pre-drop clicks at BOOM 55.30/55.52 s); pre-emptive tension cue before downbeat
- **impact**: 5
- **novelty**: 5
- **effort**: 4
- **experiment**: v3 frozen rule unchanged plus predicted-downbeat |error| <=25 ms on >=5/6 drops (frozen). Planted half-time clock and +1-beat downbeat offset both fail. Risk: confident wrong prediction; confidence-gate it.

### L11 — VP: mode 18 tempo transport inhouse_waveform.cpp:990-999; phase published at update tempo_tracker.cpp:421 / inhouse_audio.cpp:388-392
- **container**: VP
- **controls**: timing of beat-locked motion
- **current**: phase used unextrapolated: stale 0-22.5 ms (update period) + ~16 ms plumbing; PLL lock point also inherits L01's 37-83 ms novelty bias (onset phase tempo_tracker.cpp:324)
- **info_loss**: phase accuracy
- **origin**: accident/inherited
- **still_needed**: no
- **options**: phase += bpm/60*(t_render + wire - tempo_capture_us) plus calibrated novelty-delay offset; gate on confidence >=0.6
- **impact**: 2
- **novelty**: 2
- **effort**: 1
- **experiment**: Synthetic metronome: |phase error at photon time| <=0.03 beat median over 60 s after lock (frozen). Planted no-extrapolation fails.

### L12 — AP: tempo warm-up kConfidenceWarmupUpdates 512 tempo_tracker.cpp:39; ACF history 512 x 22.5 ms tempo_acf.h:11
- **container**: AP
- **controls**: how soon beat-locked visuals/prediction exist after music start or a cut
- **current**: 11.52 s by code; first lock 10.53 s measured, target 1.2-4.4 s (P7 RECEIPT:108-109,228)
- **info_loss**: n/a (availability)
- **origin**: inherited V2 tracker
- **still_needed**: no; 6 s window used by v3 reference clock
- **options**: evidence-proportional warm-up; 6 s ACF; IOI quick-lock (P7 beat-stage draft)
- **impact**: 3
- **novelty**: 2
- **effort**: 2
- **experiment**: Existing P7 GATE-SYNTHETIC-v1 LOCK criterion (frozen 12:25:06Z); existing planted late-lock fault red.

### L13 — AP/platform: PDM slot 296 pdm_target.h:16; resampler queue median 355 samples = 8.57 ms (BUDGET.md:64)
- **container**: AP
- **controls**: base latency of everything
- **current**: capture->publish p50 6.0 / p99 9.5 ms; two bands 3.8 ms (30%) and 7.0 ms (70%) (BUDGET.md:28,38)
- **info_loss**: none, delay only
- **origin**: FSP multiple-of-8 constraint plus poll granularity
- **still_needed**: 64-96 sample slot ~3-4x ISR rate, trivial at 1 GHz [INFERENCE]
- **options**: smaller slot; drain resampler as soon as one output hop exists
- **impact**: 1
- **novelty**: 1
- **effort**: 3
- **experiment**: Device timing log: capture->publish p50 <=3.5 ms, p99 <=6 ms, starved 0 (frozen). Planted 296 build fails p50. Risk: capture path most fragile (PL14-A4); back-projection maths pdm_target.c:1227-1243.

### L14 — map: deposit_offset_ inhouse_waveform.cpp:956-958
- **container**: map
- **controls**: colour of the hit
- **current**: drop light painted with lagged harmony colour (L05)
- **info_loss**: n/a
- **origin**: new
- **still_needed**: taste
- **options**: on release fire snap deposit offset to current pre-LPF centroid or to an authored palette 'release' position (not rainbow); centre-to-edge gradient across the 200 ms sweep [HYPOTHESIS]
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Host: centre-pixel offset within 1 frame of fire equals target |d|<=0.004 (frozen); planted no-snap fails; then Captain A/B.

### L15 — VP: integer scroll inhouse_waveform.cpp:1004-1010 (405 px/s = 4.05 px/frame)
- **container**: VP
- **controls**: motion smoothness (judder); ~2.5 ms of history per pixel
- **current**: integer shift, a 5 px step about every 20 frames
- **info_loss**: discards fractional scroll phase; 16-bit WS2816 can now render sub-pixel
- **origin**: S3 budget/8-bit; inherited millis() arithmetic :113-114
- **still_needed**: no (~2k cycles linear resample [INFERENCE])
- **options**: sub-pixel transport with 16-bit output
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Constant tone: trail centroid velocity coefficient of variation <=0.05 (frozen, baseline measured first); planted integer shift fails.

### L16 — VP: fadeV9 tau 290/125 ms inhouse_waveform.cpp:801; transport 405 px/s
- **container**: VP
- **controls**: visible musical memory: ~198 ms of time over 80 px
- **current**: fixed per mode (v9 ruling: constant release)
- **info_loss**: n/a (aesthetic mapping)
- **origin**: aesthetic, P6 ruling 1 (inhouse_waveform.h:17-19)
- **still_needed**: taste
- **options**: scroll period tied to beat/bar after lock; shorter fade in builds, longer after drop (time-domain tension shaping)
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Host: scroll period = k*60/bpm within +/-3% after lock (frozen); planted fixed 405 fails; Captain A/B for taste.

### L17 — system: InhouseAudio::acquire inhouse_audio.cpp:419-426
- **container**: system
- **controls**: transient visibility across 133.3 Hz AP vs 100 Hz render
- **current**: peak max-held over missed generations (good); vu_level/spectrum/novelty latest-only, about 25% of hops unseen by brightness [INFERENCE]
- **info_loss**: short loud hops between frames lost for width/brightness
- **origin**: inherited single-snapshot publication
- **still_needed**: no
- **options**: publish max vu and summed onset since last read; L04 removes most of it
- **impact**: 2
- **novelty**: 2
- **effort**: 1
- **experiment**: Synthetic 7.5 ms bursts at random phase: >=99% raise rendered level (frozen); planted latest-only fails.

### L18 — VP: freshness gate inhouse_waveform.cpp:124-128 (<=30 ms); tempo staleness 30+22.5 ms :990-993
- **container**: VP
- **controls**: behaviour on audio hiccups (drain vs hold)
- **current**: 30 ms
- **info_loss**: none when healthy
- **origin**: deliberate safety: drain, never replay loud frame (:119-121)
- **still_needed**: yes (always-visualising, no frozen frame)
- **options**: tighten to 20 ms after L13
- **impact**: 1
- **novelty**: 1
- **effort**: 1
- **experiment**: Only with L13.

### L19 — output: WS2816 profile 3, 80 px/DIN lockstep ws281x_diag.h:41-42; BUDGET.md:70-79
- **container**: output
- **controls**: fixed 5.1 ms wire, all pixels latch at reset; 196 fps ceiling
- **current**: 5.100 ms (3840 bits x 1.25 us + 300 us); 9.6 ms figure withdrawn (STATUS 16:49Z)
- **info_loss**: none; delay
- **origin**: hardware: 160 px per board on two DINs
- **still_needed**: yes on current boards
- **options**: 280 us reset saves 0.02 ms (risky: EEVblog reports units needing 4-5 ms); four DINs/board (hardware change) would give 2.7 ms
- **impact**: 1
- **novelty**: 1
- **effort**: 5
- **experiment**: None; record only.

### L20 — system/installation: acoustic propagation 2.92 ms/m
- **container**: system
- **controls**: perceived synchrony per listener seat
- **current**: unmodelled; cancels if mic and listener are equidistant from the speaker
- **info_loss**: n/a
- **origin**: physics
- **still_needed**: n/a
- **options**: disclose seat distance with any sync claim; placement guidance (Titan nearer speaker gives far listeners free budget, e.g. 4 m = 11.7 ms); possible lead offset for predicted events [HYPOTHESIS]
- **impact**: 2
- **novelty**: 3
- **effort**: 1
- **experiment**: None; budgeting disclosure.

### L21 — AP/platform: k1_poly24 filter design on device at reset/rate lock (PL14-D8, ~12k sinf)
- **container**: AP
- **controls**: one-off frame stall; poll high-water 8.67 ms (BUDGET.md:85)
- **current**: designed on device
- **info_loss**: possible missed frame at lock
- **origin**: accident (plan said offline)
- **still_needed**: no
- **options**: offline tables for 36-44 kHz or incremental design across hops
- **impact**: 1
- **novelty**: 1
- **effort**: 2
- **experiment**: Device log: poll high-water <2 ms over 10 relocks (frozen); planted on-device design fails.

### L22 — AP: Mood coefficient basis inhouse_audio.cpp:103-111 (quirk acknowledged :314-316)
- **container**: AP
- **controls**: every Mood value mislabelled: effective cutoff 1.33x nominal
- **current**: 100 Hz coefficient at 133.3 Hz -> 2.0 Hz not 1.5 Hz at mood 0.05
- **info_loss**: labelling honesty
- **origin**: inherited source quirk kept for oracle parity
- **still_needed**: parity only
- **options**: correct to hop dt or relabel; subsumed by L02
- **impact**: 1
- **novelty**: 1
- **effort**: 1
- **experiment**: Unit test: effective cutoff equals label within 1% (frozen); planted 100 Hz basis fails.

### L23 — GDFT window lengths gdft_goertzel.cpp:104-131 (rectangular, Cross40 halves resolution below bin 40)
- **container**: AP
- **controls**: time vs pitch resolution per bin; drop of temporal resolution at low notes
- **current**: bin0 76.4 ms (centroid 38 ms), bin11 40.5 ms, bin39 8.0 ms, bin40 15.2 ms, bin79 1.7 ms
- **info_loss**: low notes integrate 76 ms (time smeared); crossover makes bins 39->40 jump 8->15 ms
- **origin**: inherited K1/SensoryBridge semitone design; S3 budget for one bank
- **still_needed**: yes for pitch/chroma; no for timing (use L06)
- **options**: keep for chroma; windowed (Hann) multi-resolution if leakage matters; never use for onset timing
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Covered by L06 (timing) and D07 (pitch). No standalone proposal.

## Failures mapped

- The drop does not hit harder than the build (Captain). Time-domain causes [INFERENCE]: extent attack smoothing, t90 70 ms (L03); the AGC follower erases the drop surge within about 30-90 ms (L08); a sub-bass-triggered release lands about 100-145 ms after the drop, beyond the 90 ms acceptability limit (L09/L10); the release's first ~200 ms sweep is painted in the build's colour (L05/L14).
- P6 drop detectors v1-v3 all failed the +50 ms window. The sub-bass step is 72-117 ms after the annotations, a property of the signal (v2 DETECTOR.md:86-87), so no causal sub-bass rule can pass. v3 failed because its prediction came from a half-time tracker with an 11.5 s warm-up (L10/L12).
- Half-time tempo and 26-380 ms beat-phase error. The 88 BPM prior is the main cause (P7). In addition [INFERENCE], the flywheel locks onto novelty computed after the 80 ms low-pass, which builds a 37-83 ms late bias into the phase (L01/L11).
- The P5 camera could not measure latency: the v8 effect does not flash on each click. Onsets are smeared by the Mood low-pass (L01) and extent by the 16/23 ms cascade (L03).
- v8 colour moved in only 1-11% of seconds. Cause: the slow-layer follower (0.6 s, 0.25 palette units/s cap, harmony gate). Path P fixed the over-averaging but keeps the 75-113 ms low-pass lag (L05).
- PL14-B2: the two resamplers' sample indices named moments 2.35 ms apart. A time-alignment fault, fixed with a 94-zero pre-roll and enforced by poly24_clock_probe.
- PL14-D8: the 24 kHz path added 0.55 ms capture-to-publish, and filter design on the device risks a 3-6 ms stall (poll high-water 8.67 ms). See L21.
- PL14-C6: a 67 ms correlation peak was read as a latency. A measurement-validity failure; this map uses only stamped device stages and computed constants.

## Contradictions

- Render rate. The brief says '120 Hz logical render target', but the product K1_EMOTISCOPE_PORT image uses kPalettePeriodUs 10000 = 100 Hz (palette_runtime.h:40). The P5 capture reports render_hz 100 and a measured 100.01 fps (BUDGET.md:81). The 8333 us (120 Hz) period applies only to non-Emotiscope builds (palette_runtime.h:47); mode 11 still decays on a 120 basis (inhouse_waveform.cpp:458-460).
- Hop. The brief says the hop is 180 samples / 7.5 ms at 24 kHz. The P5 latency stamp is the donor's 64-sample 12.8 kHz hop of 5.0 ms (BUDGET.md:91; k1_asrc_24k.h:17-18). Both cadences exist (200 Hz and 133.3 Hz), and the P5 total is donor-stamped.
- 'Same 7.5 ms hop, so every time constant holds' (inhouse_audio.h:20-21) is not true of Mood: a 100 Hz coefficient applied at 133.3 Hz gives a 2.0 Hz effective cutoff instead of 1.5 Hz (inhouse_audio.cpp:103-111, quirk acknowledged at :314-316). The renderer converts its constants to elapsed time (inhouse_waveform.cpp:144-147); the analysis does not.
- Pipeline latency. P6's early '~40 ms pipeline' was withdrawn in favour of a '<=10 ms host step' (P6 RECEIPT:94-98). That step test exercised only the sub_rms/release path. The GDFT -> Mood low-pass -> novelty/colour path has a half-rise of 75-113 ms. Each figure is right for its own path; neither is 'the' pipeline latency.
- LED wire 9.6 ms vs 5.1 ms. Settled at 5.1 ms (STATUS.md 16:49Z); listed for completeness.
- Tempo lock: 11.52 s by code (512 x 22.5 ms) vs 10.53 s measured (P7 RECEIPT:108-109). [INFERENCE] Updates made before the music starts count toward the warm-up.
- Palette default: HEAD 1ef5395 has kInhouseV9Palette=kRestrictedRange (git show HEAD:src/k1/core/visual/inhouse_waveform.h:46), but the uncommitted working tree has kPerPixelMapped (:50). The path-P colour timing applies only to the working-tree selection.
- Brightness cap 24/255 (brief and global rule) vs 100/255 (Kx-Titan AGENTS.md, as the brief reports). Not resolved here; it does not affect timing.
- WS2816 reset: the datasheet minimum is 280 us (per a search summary; I could not text-extract the primary PDF), but field reports say some lots need 4-5 ms (EEVblog). This unit runs at 300 us with 0 skipped frames (BUDGET.md:81). That is a risk for other LED lots, not a conflict on this unit.

## Open questions

- Is the device's real onset-to-novelty delay the modelled 37-83 ms? Check on a host replay of the click corpus with unchanged InhouseAudio, comparing publication_.novelty against the click truth.
- What is the WS2816 emit-to-photon time and the PWM frequency? 10 kHz is unverified (secondary sources only). Measuring it needs P5 proposal 1 and the Captain's OK for one logged edge on top of the running show.
- Does the Captain see the build-coloured first 200 ms of the drop as a defect or as character? Needs a Captain A/B of L05/L14.
- What offset does the Captain perceive as 'on the hit'? Run a blind A/B at 0, +20, +45 and +90 ms with one identical-pair control (E5). The order and the 'late in at least 2 of 3' criterion are frozen before viewing.
- Can the current boards drive more than two DINs per board (spare GPT channels, routing)? Only that removes the 5.1 ms wire floor.
- Will the P7 beat stage (runs/P7-BEAT-STAGE, in progress) include a bar/downbeat stage? L10 needs it, not just a beat.
- Is there a device path that plays fed audio (not the room) for A/Bs while the show stays always-on?
- ITU-R BT.1359 thresholds were taken from secondary sources (NAB TV TechCheck 5 Oct 2009, https://nab.org/xert/scitech/2009/TVTechCheck/TV100509.asp; EE Times, https://www.eetimes.com/audio-video-synchronization/; both accessed 15 Oct 2026). Their application to music-to-light transients is [INFERENCE]; the 20-30 ms 'hit' target is [HYPOTHESIS].
- Delivery gap: the harness refused the Write of REPORT.md, so the parent or integrator must store this structured result if a file is required.
