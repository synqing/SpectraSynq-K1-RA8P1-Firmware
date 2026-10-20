# D05-failure-corpus — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

I collected 41 recorded visual or musical failures: 24 on the Titan and 17 on the ESP32-S3 K1 and its SensoryBridge and Lightwave predecessors. Each is traced to its stage, lever and fix, and they fall into 13 recurring patterns. The five that matter most for the light are: too much averaging; reading a signal after it has been squashed by a gate, AGC or soft knee; louder giving less light; nothing below 110 Hz; and the host model not being the product.

The biggest finding: the K1's August "sluggish, short leash" fault was blamed partly on a peak follower that rises 50 times faster than it falls (0.25 attack / 0.005 release). The Titan uses the same constants (platform/ra8p1/inhouse_audio.cpp:250), and that follower sets how far the Hybrid K1 waveform reaches (inhouse_waveform.cpp:973-977). After a loud build the follower is already high, so the drop cannot reach further than the build did. [INFERENCE] This is the most likely untested reason the drop still does not hit harder.

The Titan's spectral AGC is the same single whole-spectrum gain that made the K1 go darker as music got louder (inhouse_audio.cpp:330-339). Its gain freezes while gated, and onsets and tempo are computed after that gain and the soft knee. That fits the P6 finding that BOOM's drop has less onset energy than its build (0.014 against 0.038).

Three more probable hiding places:
- The display stage changes red, green and blue by different amounts (warmth, plus white balance 1 / 0.9375 / 0.84) after the colour test samples the frame (palette_runtime.cpp:281-294). The palette-accuracy pass may therefore not hold at the LEDs.
- The WS2816 chip applies its own gamma, and nothing on the Titan compensates, while the 0.25 gain pushes every colour value into the chip's dark region.
- Path P's palette position follows distance from the centre, not frequency, so some of its "9–16 palette positions" come from geometry rather than music.

The sources disagree in eight places. The most important:
- the "16-colour copy" explanation is overstated (the Titan sampler was already the K1 HD sampler);
- "1–11% before" refers to v9, not v8 (v8 moved in 95% of seconds);
- the render rate is 100 Hz, not 120;
- the wire time is settled at 5.1 ms against an older 9.6 ms figure;
- the brightness figures conflict (24/255 against gains 0.25 at master 255 against 100/255).

16 levers are listed below, each with a frozen criterion and a planted fault. The cheapest high-impact ones to try first:
- L01: re-time the peak follower;
- L03 and L04: read onsets and colour before the knee and the AGC;
- L08: score colour at the packer output.

Images are N/A because this is source and receipt research. The report file was not written because the harness refuses subagent report writes; everything is in this result.

## Levers

### L01 — peak follower platform/ra8p1/inhouse_audio.cpp:247-253 (follower_ attack r0.25 ~30ms, release r0.005 ~1.99s; peak_scaled = drive/follower, smoother 0.65/0.15); consumed inhouse_waveform.cpp:973-977 (mode32 extent via peak_stage1/2 EMAs 16+23 ms) and :996-999 (mode18 position)
- **container**: AP->map
- **controls**: motion/space (waveform extent, the Captain's 'hits harder'), timing
- **current**: v8 and v9 identical; constants are the same 0.25/0.005 50:1 asymmetry that the K1 Aug-18 audit named the 'short leash' (claude-mem #93761)
- **info_loss**: Destroys absolute dynamics: extent = loudness relative to the last ~2 s, so a loud build leaves no headroom for the drop [INFERENCE]; adds ~40 ms of series smoothing on extent [FACT tau sum]
- **origin**: inherited Emotiscope/K1 donor constants; the same constants were implicated in the K1 'sluggish, short leash' fault
- **still_needed**: unknown/aesthetic only; cost trivial (a few flops per 7.5 ms hop)
- **options**: dual-timescale: a fast ratio for transients plus a slow 8-15 s section reference; tension memory (the build integrates contraction, the drop releases against the stored build peak); drive extent from section-relative level
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Host replay BOOM file/room, Sonic, Carte. Frozen: EXT v2 extent at drop+150 ms >= build-climax extent + 0.25 half-strip on >=3/4 tuning cases, with build contraction kept. Planted fault: follower release forced equal to attack (tau 30 ms) must turn it red.

### L02 — spectral AGC platform/ra8p1/inhouse_audio.cpp:329-341 (single broadband scalar: target 0.25/envelope, clamp 0.1-10, attack .28/release .02, noise tracker .001, gate open 4x / close 2.5x, gain frozen while gated)
- **container**: AP
- **controls**: colour shape (chroma), spectral brightness, onset strength, tempo novelty
- **current**: v8=v9; identical structure to the K1 broadband AGC v2 that caused 'louder music -> dimmer effects' (#72835) and froze at its floor (#93761)
- **info_loss**: Destroys inter-section loudness (chroma energy x0.35 build->drop, agc-trace.html) and band balance under broadband load; freezes gain when the gate closes
- **origin**: S3 lineage (K1 AGC v2, 20 May 2026), replacing an unstable per-band AGC
- **still_needed**: the function yes; the broadband-only form no. Per-band (4 bands) is negligible against 716k cycles p50 in-house [FACT P4]
- **options**: per-band AGC (K1 K1_AGC_PERBAND_V1 exists, default OFF); publish pre-AGC features alongside post-AGC ones; slower release for section dynamics
- **impact**: 4
- **novelty**: 2
- **effort**: 3
- **experiment**: Frozen: BOOM drop/build 110-400 Hz flux ratio >= 1.2 (now ~0.37), with C3 silence/true-black still passing. Planted: gain pinned at its 0.1 floor must go red.

### L03 — soft knee inhouse_audio.cpp:352-355 (value>0.5 -> 0.5+e/(1+e))
- **container**: AP
- **controls**: colour shape and brightness of loud bins; onsets
- **current**: v8=v9; path P already reads pre-knee linear_shape for colour only (inhouse_audio.cpp:347-350)
- **info_loss**: Compresses the top half of every bin; caused the path-P loud-end colour stall
- **origin**: inherited K1 VP_FIX_AGC_SOFT_KNEE
- **still_needed**: only as a display safety stage, not for analysis consumers
- **options**: move novelty, chroma, sub and tempo taps to pre-knee; keep the knee for display only
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: Frozen: pre-knee novelty at the BOOM drop onset >= 1.5x build-climax novelty. Planted: tap moved back post-knee must go red.

### L04 — novelty inhouse_audio.cpp:356-362 (sqrt mean positive diff of the AGC'd+kneed spectrum, 110 Hz+); feeds mode-18 energy_rate inhouse_waveform.cpp:990 and the tempo tracker
- **container**: AP->map
- **controls**: timing (beat motion), mode-18 speed, tempo input
- **current**: v8=v9
- **info_loss**: Loses sub-bass and the loudness step; BOOM drop flux 0.014 < build 0.038 (P6 it6)
- **origin**: inherited Emotiscope
- **still_needed**: no reason to compute on the compressed spectrum on the M85
- **options**: log-magnitude flux pre-AGC, split sub/low/mid/high, kept as distinct observed-vs-predicted signals (TL14-069)
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Frozen DETECTOR-v2 rule: fire within +50 ms on >=4/6 annotated drops, 0 false fires. Planted: the broadband-surge fault must stay red.

### L05 — Goertzel bank 110 Hz-10.5 kHz (gdft_goertzel.cpp:102-134 per agc-trace.html); v9-only 40-110 Hz sub_band.h:44-53 (uncommitted)
- **container**: AP
- **controls**: drop timing, motion
- **current**: v8 blind below 110 Hz; v9 sub_rms exists but is only used by off-by-default release candidates
- **info_loss**: Drops 20-110 Hz, which carries +17 dB of the BOOM drop; sub arrives 72-117 ms after the musical drop anchor
- **origin**: S3 budget [INFERENCE]: long low-frequency windows at 12.8 kHz were expensive
- **still_needed**: no; a decimated bass branch is cheap on the M85 (TL14-068)
- **options**: decimated 20-110 Hz bank or biquad bands; publish sub_rms and sub flux to the product contract
- **impact**: 5
- **novelty**: 2
- **effort**: 2
- **experiment**: Frozen EXT re-anchored to 10 ms audio onsets: release lag <= +60 ms on BOOM file and Sonic@27.7. Planted: band moved to 110-200 Hz must go red.

### L06 — updateReleaseV9 inhouse_waveform.cpp:756-795 (InhouseV9Release, disabled by default)
- **container**: map
- **controls**: motion/space ('hits harder' extent release)
- **current**: off; X1-X3 (level surge) and K1-K3 (sub kick) candidates all failed EXT; v3 predicted-downbeat detector 0/6
- **info_loss**: n/a (feature absent)
- **origin**: new
- **still_needed**: yes
- **options**: tension-armed release: the build integrator (contraction + riser + sub hole) arms it; sub return OR predicted downbeat fires it; rollback if sub not back in 150 ms. Depends on L01, L05, L09
- **impact**: 5
- **novelty**: 4
- **effort**: 4
- **experiment**: Captain corpus holdout (unspent), frozen EXT v3 before scoring. Planted: climax-fire fault must go red; b5595c35 must stay red.

### L07 — amplitude->position inhouse_waveform.cpp:973-977 (mode32: 80+peak_stage2*0.7*80, max 136) and :996-999 (mode18: 80+peak*80)
- **container**: map
- **controls**: space
- **current**: v8=v9; on b5595c35 'the waveform hugs the edges and the centre stays dark' (P6 EXT)
- **info_loss**: Louder pushes the deposit outward, so the lit area shrinks (inverse polarity); the K1 canon calls amplitude->position the 'oscilloscope leak' (#81769)
- **origin**: inherited Hybrid K1 look
- **still_needed**: aesthetic; Captain decides
- **options**: flip polarity at the drop (loud -> centre deposit, scroll fills outward); map amplitude to width/brightness and section tension to position
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Frozen: EXT build-contraction AND drop-release pass on BOOM file. Planted: position frozen at 80 must fail contraction. Taste: smallest on-device A/B is BOOM 40-65 s, v9 vs polarity-flipped.

### L08 — display stage platform/ra8p1/palette_runtime.cpp:281-294 (brightness .3+.7b; hue-preserving shoulder >0.75 tanh; per-channel warmth; per-channel white {1,.9375,.84})
- **container**: output
- **controls**: colour, brightness
- **current**: v8=v9; applied after the renderer frames that P6 PAL/PAL-HD score
- **info_loss**: Shifts every palette hue (B -16%, G -6%, plus warmth) after the colour metrics; the shoulder compresses drop peaks
- **origin**: inherited donor display stage
- **still_needed**: the shoulder may be a safety stage; white balance should be a measured LED calibration, not a constant
- **options**: fold white balance into a measured per-LED matrix; run PAL-HD on post-display/packer frames; reserve shoulder headroom for drops
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: Frozen: PAL-HD-v2 on post-display frames, dE00<=3 against the palette curve passed through the declared calibration. Planted: scoring against an undeclared {1,.9375,.84} shift must go red.

### L09 — tempo prior/warm-up src/k1/core/audio/tempo_tracker.cpp:23 (kTactusBpm 88), :39 (512-update warm-up), :515-518 (priors)
- **container**: AP
- **controls**: timing; mode-18 motion weight (inhouse_waveform.cpp:986-996); any downbeat-predicted drop
- **current**: v8=v9; BOOM reads 69/92 (true 138), Sonic 71, Dopex 75; 0/326 on GATE-SYNTHETIC-v1; confident 106 BPM on silence
- **info_loss**: metrical identity lost; ~10.5 s minimum lock
- **origin**: inherited ESV11 lineage; the same octave family as K1 (#86123 Beauty 74 BPM)
- **still_needed**: no
- **options**: flat/120 prior (P7: BOOM 100% intro, 82-92% drop); explicit beat-level stage; confidence decay on silence/pads
- **impact**: 4
- **novelty**: 2
- **effort**: 3
- **experiment**: GATE-SYNTHETIC-v1 (frozen 2026-10-14T12:25:06Z) plus the Captain corpus; existing planted half-time/3:2/late-lock faults must stay red.

### L10 — wire packer palette_runtime.cpp:695-720 (edge gain x0.25 Q16) into WS2816C (datasheet: 4-bit in-silicon gamma, no disable)
- **container**: output
- **controls**: brightness, dark trail tail, palette dark ends
- **current**: linear codes, gain 0.25, no compensation (grep: no degamma anywhere on Titan)
- **info_loss**: [INFERENCE] darks lose more than linear arithmetic predicts at <=0.25 full scale; unmeasured on Titan
- **origin**: accident/unknown
- **still_needed**: n/a
- **options**: measure first (K1 R50 locked-exposure test, #93763), then a non-uniform de-gamma before the gain within the current budget
- **impact**: 3
- **novelty**: 2
- **effort**: 3
- **experiment**: Locked-exposure camera on a step ramp: R50<=0.66 confirms the chip curve, >=0.85 kills it. Planted: a synthetic x0.5 code scale must move R50.

### L11 — colour offset applyShapeCentroidP/updateOffsetP inhouse_waveform.cpp:593-657 (80->12 i%12 fold, one circular centroid per frame)
- **container**: map/VP
- **controls**: colour
- **current**: v9 path P: one musical scalar per frame (v8 also one)
- **info_loss**: polyphony (bass + lead) collapses to one hue per frame
- **origin**: K1 Waveform-family design (#54824 'one palette position per frame by design')
- **still_needed**: no compute reason on the M85
- **options**: offset from the dominant pitch class of the band that set the deposit; separate core/halo offsets
- **impact**: 3
- **novelty**: 4
- **effort**: 3
- **experiment**: Frozen: two-voice synthetic (A2 bass + E5 lead) yields two palette positions >=1/12 apart in >=80% of frames. Planted: a single-centroid build must go red.

### L12 — path P spatial term inhouse_waveform.cpp:888-890 (d = |i-centre|/80 added to the deposited offset)
- **container**: VP
- **controls**: colour x space
- **current**: v9 path P default (working tree)
- **info_loss**: adds geometric colour; for a scrolling trail distance ~ age, not frequency; inflates the '9-16 positions across strip' metric
- **origin**: K1 GDFT freq_prog analogy
- **still_needed**: aesthetic
- **options**: expose a d-scale control (0 = purely musical colour); derive d from deposit-time spectral content
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: Frozen: colour coverage with d=0 still >=9 positions/strip on BOOM. Planted: frozen offset with d on must fail a musical-coverage term.

### L13 — darkness gates: silence flag 5 s dwell (palette_runtime.h:51), spectral noise gate (inhouse_audio.cpp:335-336), tonality gate (inhouse_waveform.cpp:156-158), donor admission PCM peak^2 0.0048/0.0034 (palette_runtime.cpp:258)
- **container**: system
- **controls**: brightness/true black, always-visualising
- **current**: 4 gates; the admission gate is untraced (Dopex dark at 76.3 s before the silence flag at 80.7 s)
- **info_loss**: any gate can darken quiet music unattributed
- **origin**: inherited
- **still_needed**: yes, but each needs telemetry
- **options**: one telemetry word per gate per frame naming which gate closed
- **impact**: 2
- **novelty**: 1
- **effort**: 1
- **experiment**: Frozen: every dark frame in the P3 Dopex replay attributed to exactly one gate. Planted: an unlabelled fifth zeroing must report 'unattributed'.

### L14 — render cadence palette_runtime.h:40 (kPalettePeriodUs 10000 under K1_EMOTISCOPE_PORT) and wire 5.1 ms (80 px/DIN lockstep)
- **container**: output/system
- **controls**: timing
- **current**: 100 Hz; ~16.4 ms typical sample->wire end (P5 device budget, evidence 57d0ec5)
- **info_loss**: mean 5 ms cadence wait
- **origin**: donor 100 Hz reconstruction
- **still_needed**: no for compute (VP p50 ~182k cycles ~0.18 ms)
- **options**: 200 Hz latest-wins render (wire caps ~185 Hz/DIN); onset-triggered immediate frame
- **impact**: 2
- **novelty**: 2
- **effort**: 3
- **experiment**: Device-side capture->wire-end stamps: p50 <= 11 ms. Planted: an added 10 ms sleep must go red; wire bits decoded, not counted (TL-65).

### L15 — v9 level transfer renderV9 inhouse_waveform.cpp:943-949 (-33..-9 dBFS -> 0..1) and deposit amplitude 0.396*l^1.5 (kInhouseV9Gain32)
- **container**: map
- **controls**: brightness, width
- **current**: v9 b5595c35
- **info_loss**: ceiling saturates in 34% of song-file drop frames; ~60% of renderer range unused (STATUS erratum l.155)
- **origin**: matched optical energy to v8 (P6 it4)
- **still_needed**: aesthetic
- **options**: section-relative ceiling using L01's slow reference; reserve headroom for the drop
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: Frozen DI (14:18:24Z) on luma, not channel sum: b5595c35 must stay red and planted x3 accent green.

### L16 — instrument: product-path equivalence pytest (P6 host-005) and scoring point
- **container**: system
- **controls**: trust in every lever result
- **current**: PL14-B14 caught a host-variant default mismatch; metrics score renderer frames, not the packer
- **info_loss**: host map != product territory
- **origin**: process
- **still_needed**: yes
- **options**: every scored variant bit-equal to the compiled product default; score at the packer output
- **impact**: 4
- **novelty**: 1
- **effort**: 1
- **experiment**: The equivalence test must go red when kInhouseV9Palette is planted back to kRestrictedRange.

## Failures mapped

- T01 Titan palette 'over-averaging' (Captain): one sample per frame + RGB channel mix + RGB EMA + RGB trail (inhouse_waveform.cpp@81ac031 ~:204-240) -> VP colour -> path P (fix partly generalises: blend in position domain yes; distance-as-frequency is a stand-in)
- T02 v9 colour parked 1%/9%/4% of seconds; gate holds 29/61/79% at -6/0/+6 dB (post-gate confidence, 0.25 u/s cap) -> path P K1 ae5d90a rule
- T03 v9 kRestrictedRange fold maps c and 1-c to one colour (halves variety) -> path P default
- T04 path P loud-end colour stall: centroid read post-knee -> pre-knee linear_shape
- T05 PL14-B14 colour fix not in the built image (compile default) -> equivalence pytest
- T06 'drop still not harder than build' (Captain): v8 decay 0.8+3.5*peak; v9 ceiling saturation 34%; gain 0.396; gold->red luma; follower asymmetry [INFERENCE] -> unfixed
- T07 Captain 'hits harder' = extent release; b5595c35 hugs edges, centre dark -> amplitude->position polarity -> unfixed
- T08 release fires on build climax/riser (broadband surge) -> unfixed
- T09 drop detectors v1 1/5, v2 1/6, v3 0/6 within +50 ms -> unfixed
- T10 blind below 110 Hz; BOOM drop flux 0.014 < build 0.038 -> v9-only sub_band (uncommitted)
- T11 tempo half-time / 3:2 (69/92 vs 138; Sonic 71; Dopex 75): 88 BPM prior -> unfixed in product
- T12 tempo warm-up 10.5 s; confident 106 BPM on silence -> unfixed
- T13 mode-18 gate open at wrong tempo; weight capped 0.7 in 53% of frames -> unfixed
- T14 drop renders dimmer (upper luma x0.24): harmony colour gold->red + louder fades faster -> partial (v9 release)
- T15 lingering colour in silent cut -> v9 drain ('noticeable improvement', Captain)
- T16 Dopex boards dark at 76.3 s before the silence flag: untraced admission gate
- T17 peak AGC lifts room noise to music level (0.68 at RMS 0.018)
- T18 beat barely reaches the plate (camera reach ~13%); host light carries the beat via level, not tracker [INFERENCE]
- T19 native first boot dark -> half white -> full white (UID order, MOCO polarity, GTIOC8A/B, transmitter never completed a frame)
- T20 Sep 27 'severe random white flashes' then 'strip COMPLETELY STUCK': ELC stop vs DMA remaining-1 bind (#108789)
- T21 'exact same unacceptable visual' after deploy claim: retained scene override substituted Bloom (Hindsight)
- T22 top channel 'looks corrupted… rotational': DIN halves exchanged + scene override (99c0cf44)
- T23 two boards showed one copied scene (TL-17); scene-alias fault 005 still falsely green (TL14-059)
- T24 v9 indistinguishable from v8 by readback 127/0
- K01 2025 '12 colours that repeat… 81.25% wasted' (Captain): note_colors[i%12] -> per-bin hue_from_mapping
- K02 palettes parked at LUT index 128 -> clamp removed
- K03 'flat sheet of yellow', 'all white', Snapwave white: summed palette RGB -> dominant/one weighted sample (de4fc48)
- K04 11 Jun 2026 'tending towards white… not the whole range of the palette': CRGBPalette16 16-slot + uint8 -> HD sampler; GDFT c2738b88
- K05 'palettes progressively duller', worse louder: post-gate chroma + held-hue freeze -> ae5d90a
- K06 Lightwave 9 Feb 'unable to display the palette colours accurately': RGB trails -> brightness-only trails, colour at output (K1 only on a side branch 7b3bb992)
- K07 'louder music -> dimmer effects': single broadband AGC scalar (#72835) -> K1_AGC_PERBAND_V1 prepared, default off
- K08 Main RPL '30-40% dimmer, sluggish, very short leash' (Captain 18 Aug): gamma blamed then reversed to 3.5 mA drive + AGC frozen at floor + 0.25/0.005 follower (#93755 -> #93763)
- K09 agc_envelope dead on hardware, gate always closed (#78105)
- K10 bench mic starved -51.9 dBFS crest 7.6 dB (#93761, #81644)
- K11 Melodic Bloom 'twitchy, jumpy, flips on a dime': instant-rise ratio denominator (#81785)
- K12 Bloom BT 'twitchy… crackhead-like': speed floor 22 -> 35 px/s (#79324)
- K13 PULSE BLOOM strobe -> THE STROBE LAW: never global brightness on the beat (#58407)
- K14 Beauty half-time lock 74 vs 128 BPM (#86123/#86132)
- K15 dithered 8-bit flicker at the dim tail; 16-bit WS2816 wins (#85628)
- K16 harness PASS while device dark (ctx.leds), rainbow test palette (#67412)
- K17 waveform speed <27 'sluggish and out of sync' (#49345)

## Contradictions

- Colour root cause: the brief and Hindsight say inhouse_waveform.cpp:209-231 'used the 16-colour averaged copy'. [FACT] At 81ac031, sample() is already the K1 HD stop sampler (:41-71; grok-k1-git agrees), and compiled_fastled16 is only a near-black fallback (~:213-224). The collapse came from one sample per frame, RGB channel mixing, the RGB EMA and the RGB trail.
- Colour-movement baseline: the brief says 'colour moves… vs 1-11% of seconds before'. FORENSICS.md: v8 moved in 94.8% of seconds; 1/9/4% is v9 b5595c35 (variant B). 'Before' means v9, not v8.
- FORENSICS (1st dig) says the record holds no 80-bin per-bin palette bug. The 2nd dig (grok-k1-git) proves GDFT's 80 LEDs used a 16-slot uint8 ColorFromPalette until c2738b88 (22 Aug 2026).
- Render rate: the brief and TL14-037 say a 120 Hz logical render. The product lane compiles kPalettePeriodUs 10000 under K1_EMOTISCOPE_PORT (palette_runtime.h:40), measured 99.7-99.9 fps. 120 Hz (8333 us) is the other compile branch / Kx-Titan native contract.
- Wire geometry and time: TL14-035/036 say two 160-pixel lines and 9.6 ms, and P6 RECEIPT's latency table still says 9.6 ms. STATUS 16:49Z settles it at 80 px per DIN, P601/P604 lockstep, 5.1 ms (palette_runtime.cpp:756). The two lanes describe different transmitters.
- Brightness: the always-visualising mandate and the brief say cap 24/255. Runtime readback (STATUS/Hindsight) is master 255 with edge gains 0.25 (~64/255 effective). Kx-Titan AGENTS says 100/255. Reported, not resolved.
- K1 'short leash': claude-mem #93755 said WS2816C in-silicon gamma was the root cause; #93763 reversed this (3.5 mA LED drive + AGC frozen at loud-guard floor + 0.25/0.005 follower). Memory-only citations of 'gamma double-crush' as settled are wrong.
- P6: the ten-line report said C1 'passes every frozen host test'; the erratum says C1 measured channel-sum medians (wrong) and luma passes only 2/4. The P6 kick section said the pipeline adds ~40 ms; this was withdrawn (host onset->release <= 10 ms).

## Open questions

- Does the drop's extent release require breaking the AGC-relative peak follower (L01), or only a release envelope? A host replay sweeping the follower release from 2 s to 15 s on BOOM/Sonic answers it.
- What is the device default config.warmth, and how far do warmth plus white balance {1,.9375,.84} move palettes 31/38 off their curves at the packer? Readback plus arithmetic answers it.
- Does the Titan's own microphone capture BOOM's sub-bass? The iPhone room recording shows only +6 dB at the drop. This needs a device PCM capture in an authorised window.
- Which brightness figure is authoritative: 24/255, master 255 with gains 0.25, or Kx-Titan's 100/255?
- Is path P's spatial term d (colour by distance from the centre, which is age in a scroll) wanted as an aesthetic, or should colour be purely musical? This is a taste A/B for the Captain: d=1 vs d=0 on BOOM 40-65 s.
- Which of the four darkness gates darkened Dopex at 76.3 s, before the silence flag?
- Does the WS2816C in-silicon gamma measurably darken the trail tail and the palettes' dark ends at gain 0.25 (K1 R50 locked-exposure test)?
- Does the Titan spectral AGC ever lock at its floor with the gate closed on a quiet room input, as the K1 did (#93761)? Telemetry for gain_q16 and gated is not exported.
