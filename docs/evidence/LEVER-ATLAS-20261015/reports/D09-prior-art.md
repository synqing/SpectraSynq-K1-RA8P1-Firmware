# D09-prior-art — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

1. Almost every music-light product squeezes the music into a few numbers before drawing it. WLED uses 16 one-byte bands and builds a 16-slice "Spectrum" palette from them. Resolume uses Low/Mid/High. Consumer lamps (Nanoleaf, Govee, Twinkly) mostly react to loudness. The Titan's old averaged-colour fault belongs to this same family, and the industry never fixed it.
2. The Titan's tempo failures are standard for the field. LedFx issue #2058 (opened 3 Oct 2026) reports beats about 46 ms late, an off-beat lock (F-measure 0.642), tempo read 2–3 % high, and half or double tempo near 80–90 and 170–180 BPM. Published fixes exist: a zero-latency beat tracker that fires one frame early (Meier, Chiu & Müller, F1 74.68 % at 0 ms) and BeatNet/BeatNet+.
3. Only three kinds of system know where a build ends and a drop starts. Hue+Spotify reads song metadata, and Spotify closed that data to new apps on 27 Nov 2024. SoundSwitch and grandMA work from analysis or programming done before playback. Lightjams claims live drop detection but publishes no accuracy. No microphone-only device has published proof. That gap is the Captain's "the drop does not hit harder than the build".
4. The Titan's own gain stages likely flatten the drop [INFERENCE from source]. inhouse_audio.cpp:250-254 divides by a level follower that catches up with loud sound in about 30 ms. :330-339 adds per-band auto-gain with a time constant near 190 ms. WLED's own documentation recommends its slowest gain mode for music with strong dynamics.
5. Two Titan assets have no competitor match. One is a real 16 bits per colour on the wire (WS2816), against everyone else's 8 bits plus flicker dithering at 400+ Hz; at the 24/255 cap that is about 6,168 levels against about 24. The other is the M85 + Ethos-U55 headroom for small music models; Emotiscope's beta 64-36-8-36-64 network is the only on-device precedent found.
6. Research guidance: colour follows the music's emotional character (tempo and major/minor to saturation, lightness and yellow-blue, r 0.89–0.99; Palmer 2013); loudness goes with brightness; pitch goes with height. The best sound-to-light delay shrinks with tempo, about 100 ms at 60 BPM to 30 ms at 240 BPM (Arrighi et al.), and no product adapts to that.
7. Novelty, rated honestly: the individual techniques mostly exist somewhere. What nobody ships is the combination: microphone only, no cloud, full-resolution colour per bin, true 16-bit output at low brightness, and a drop release that matches the audio.

## Levers

### D09-L01 — platform/ra8p1/inhouse_audio.cpp:250-254 (follower; peak_scaled = drive/follower) and :330-339 (per-lane envelope and gain); alpha() at :26-27 gives tau = 0.010*(1-ref)/ref
- **container**: AP
- **controls**: brightness and deposit amplitude at the drop; the contrast between build and drop ('hits harder')
- **current**: [FACT] Follower attack tau ~30 ms, release ~1.99 s; peak_scaled smoothing attack ~5.4 ms, release ~57 ms. Per-lane gain target = clamp(0.25/envelope, 0.1–10), tau ~190 ms; lane envelope attack ~26 ms, release ~490 ms. Applies in both v8 and v9 (same file); the v8/v9 diff in these lines is unknown.
- **info_loss**: [INFERENCE] A build that ramps over 8–16 s pulls the follower up with it, so the level ratio at the drop stays near 1. Most of the build-to-drop level step is divided out before the visual stage sees it. The brief notes a raw 'donor level' rising x3.93 at the drop.
- **origin**: inherited from the S3 SPH/AGC source (comment at :24); a room-level-independence choice, not a compute budget
- **still_needed**: partly: independence from room level is needed; erasing contrast within a section is not
- **options**: (a) Hold or slow the gain while a tension estimate rises. (b) Publish a second, unnormalised long-window contrast level (tau 8–30 s). (c) Prior art: WLED 'Lazy' AGC is recommended for wide-dynamic music (kno.wled.ge/advanced/audio-reactive). Keep the clamp so loud rooms do not saturate.
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Frozen criterion (freeze_tolerance.py): on the annotated drop corpus, mean contrast signal in the 500 ms after the drop divided by the mean in the 2 s before it is >= 2.0 for >= 80% of drops. Planted fault: a synthetic track with constant level across the 'drop' must score < 1.2.

### D09-L02 — new stage after the GDFT bank and novelty (none exists today)
- **container**: AP->map
- **controls**: motion: contraction during the build, explosive extension at release; timing
- **current**: [FACT, brief] Three drop detectors failed to fire within +50 ms (runs/P6-DROP-DETECTOR/{,v2,v3}). There is no tension variable.
- **info_loss**: None of the cues Solberg 2014 lists as causing tension is published as a named signal: bass removal and reintroduction, risers (rising spectral centroid), drum-roll acceleration (rising onset rate), breakdown contrast.
- **origin**: never built; pro tools get structure offline (SoundSwitch phrases, grandMA timecode, Hue+Spotify metadata)
- **still_needed**: yes; this is the Captain's open complaint
- **options**: Hand-built features (low-band energy deficit, slope of spectral centroid, slope of onset rate, novelty density, level slope) feeding a hysteretic state machine (build -> peak tension -> release), outputting tension in [0,1] plus a release event with confidence. Alternatively a small model on the U55 (L08). Scale how strongly effects follow it by confidence (Emotiscope Spectronome pattern). Sources: Solberg 2014 Dancecult 6:61-82; Yadati 2014 ISMIR (two-stage, 'watershed moment'); Lightjams musicToDMX (claim only).
- **impact**: 5
- **novelty**: 4
- **effort**: 4
- **experiment**: Frozen criterion: release event within [-50,+80] ms of the annotated drop for >= 80% of drops, with <= 1 false release per 3 minutes on non-EDM controls. Planted fault: zeroing the low-band deficit feature must drop recall below 50%.

### D09-L03 — src/k1/core/audio/tempo_tracker.cpp:23 (kTactusBpm = 88.0F); range 60–155 BPM
- **container**: AP
- **controls**: timing; the cadence of pulses and strobes
- **current**: [FACT] Prior centred on 88 BPM; drum-and-bass is unreachable. The same failure class appears in aubio/LedFx (~120 BPM preference; octave errors at 80–90 and 170–180; github.com/LedFx/LedFx/issues/2058) and in Emotiscope's 48–144 BPM range (Emotiscope src1/global_defines.h:31,34).
- **info_loss**: loses the metrical level, so the visual pulse runs at half time
- **origin**: inherited or aesthetic; no evidence found for choosing 88
- **still_needed**: no, not at 88: human preference is near 120 BPM (McKinney & Moelants 2006)
- **options**: An explicit metrical-level stage reporting the kick/backbeat level; range 55–200; scale tempo-driven effects by confidence; adopt the LedFx/aubio fixes (unbiased period, fair phase choice, latency taken back).
- **impact**: 4
- **novelty**: 1
- **effort**: 3
- **experiment**: Use the existing frozen P7 gate. Planted fault: re-centring the prior at 60 BPM must turn the four-on-the-floor 120–140 BPM cases red.

### D09-L04 — beat-phase output -> effect trigger time
- **container**: map/system
- **controls**: timing; how tight the light feels against the beat
- **current**: [FACT, brief] Device budget ~16 ms from sample to wire end; predicted downbeats 26–380 ms off (driven by the half-time lock). No anticipation.
- **info_loss**: none; this adds future information
- **origin**: absent
- **still_needed**: yes, after L03
- **options**: Trigger beats early by pipeline latency plus a tempo-dependent perceptual offset. Meier/Chiu/Müller TISMIR reach F1 74.68% at 0 ms latency by firing one 11.61 ms frame early. Arrighi et al. put the best offset at ~100 ms at 1 Hz down to ~30 ms at 4 Hz. The Music Assistant Hue plugin uses a fixed 20 ms render-ahead. Scale by phase confidence.
- **impact**: 4
- **novelty**: 4
- **effort**: 3
- **experiment**: Frozen criterion: with scripts/p5_latency.py on click tracks at 90, 120 and 174 BPM, median (photon - onset) of beat-synced events in [-10,+20] ms, and p99-p50 <= 15 ms. Planted fault: lookahead forced to 0 must give a median >= +16 ms.

### D09-L05 — GDFT bank (lowest bin 110 Hz, per the brief); sub_rms under K1_INHOUSE_MAPPING_V9 (inhouse_audio.cpp:239-241)
- **container**: AP
- **controls**: drop timing (bass returning); weight of the kick
- **current**: [FACT, brief] No analysis below 110 Hz; sub-bass arrives 70–120 ms after the musical drop. [FACT] sub_rms exists, but only on V9 builds.
- **info_loss**: loses the cue Solberg names for the drop: bass and bass drum coming back in
- **origin**: [INFERENCE] an S3 budget choice: long low-frequency windows cost latency and cycles
- **still_needed**: no: [HYPOTHESIS] an IIR low band plus an envelope at 24 kHz costs under 1% of the M85
- **options**: Named Kick and Low channels, as TouchDesigner's Palette:audioAnalysis has; a low-band onset detector; low-band energy deficit as an input to L02
- **impact**: 4
- **novelty**: 1
- **effort**: 2
- **experiment**: Frozen criterion: on synthetic kick-only tracks at 40–100 Hz, >= 95% of onsets found within +/-20 ms. Planted fault: with the low band disabled, recall must fall to <= 20%.

### D09-L06 — inhouse_waveform.cpp path P; platform/ra8p1/palette_runtime.cpp:75-76 (sampleProductPaletteFastLed16 takes a uint8_t position)
- **container**: VP
- **controls**: colour across the strip and colour drift over time
- **current**: [FACT, brief] v9 reaches 9–16 palette positions across the strip, against 1 before. [FACT] The FastLED-compatible sampler takes an 8-bit position (256 steps).
- **info_loss**: [INFERENCE] 256 positions is fine across space but may visibly step during slow colour drift
- **origin**: inherited from FastLED
- **still_needed**: unknown; step visibility at the 24/255 cap needs measuring
- **options**: A 16-bit (Q16) palette position, interpolating between HD stops. WLED's 16-slice GEQ 'Spectrum' palette is the pattern to avoid.
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: Frozen criterion: on a slow-drift synthetic, any pixel's per-frame colour jump is <= 1.0 dE2000 in >= 99.9% of frames. Planted fault: forcing the position through uint8 must produce steps above 1.0 dE.

### D09-L07 — palette_runtime.cpp:92-94 (8-bit history shifted to 16 bits); :290-296 (16-bit wide path); wire packer; snapshot at fixture_app.cpp:350
- **container**: output
- **controls**: brightness gradation, fade smoothness, true black, colour at low level
- **current**: [FACT] The in-house path stays 16-bit to the wire ('History and actual wire output never pass through Pixel8'); the authored-history ageing path starts from 8-bit history.
- **info_loss**: [INFERENCE] At the 24/255 cap, any 8-bit stage leaves ~24 levels per channel, against ~6,168 at 16 bits
- **origin**: inherited from the S3's 8-bit LEDs
- **still_needed**: no: the WS2816 is natively 16-bit
- **options**: Audit every stage for narrowing to uint8; keep history in Q16 or float. Prior art needed temporal dithering at >= 400 Hz for the same effect (Fadecandy; Emotiscope src1/types.h:64, configuration.h:64-65); the Titan's ~5.1 ms wire time per frame caps it near 196 Hz, so native 16 bits is its route.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Frozen criterion: in the wire snapshot during a 3 s fade at cap 24, each channel takes >= 500 distinct 16-bit values. Planted fault: a build forcing >>8<<8 must give <= 25.

### D09-L08 — new: Ethos-U55 on the RA8P1
- **container**: AP
- **controls**: timing (beat and downbeat); structure (section, tension)
- **current**: absent on the Titan. [FACT] Emotiscope has a 64-36-8-36-64 network (src1/neural.h:1-5) used only in a beta mode.
- **info_loss**: none; it adds a learned representation that ignores how much percussion a track has (as BeatNet+ does)
- **origin**: n/a
- **still_needed**: unknown; depends on L02 and L03 results
- **options**: A quantised causal CRNN (like BeatNet) or a tiny tension/section classifier fed the existing GDFT and novelty features. Renesas application note on Ethos-U with RA8.
- **impact**: 4
- **novelty**: 4
- **effort**: 5
- **experiment**: Frozen criterion: pass the L02/L03 gates with inference <= 2 ms per hop on the U55. Planted fault: shuffled weights must fall below chance.

### D09-L09 — silence gate (inhouse_audio.cpp:255, rms_silent_ hysteresis); fade-to-black on cuts
- **container**: VP/system
- **controls**: brightness and contrast just before a drop
- **current**: [FACT, brief] The Captain noted fade-to-black on silent cuts as an improvement. LedFx forces zeros below min_volume.
- **info_loss**: preserved: the silence itself carries the 'gap before the drop' information
- **origin**: aesthetic; Captain approved
- **still_needed**: yes
- **options**: Treat a cut that follows rising tension as a held, loaded silence: stay at true black, then release from black for maximum contrast. Add no brightness floor. Light must resume the moment music returns.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Frozen criterion: (luminance in the first 200 ms after the drop) / (luminance in the last 500 ms before the cut) >= 4 on annotated cut-drops. Planted fault: a 10% brightness floor must fail.

### D09-L10 — 120 Hz logical render; 5.1 ms of wire time per frame on two lanes in lockstep
- **container**: output/system
- **controls**: motion smoothness; the shortest visible flash
- **current**: [FACT, brief] 120 Hz. Emotiscope runs 200–450 FPS with 128 LEDs.
- **info_loss**: small: a 75 ms flash spans 9 frames; the dj-ledfx issue #39 notes a 10 Hz light misses about a quarter of 75 ms flashes
- **origin**: Titan design
- **still_needed**: yes; [INFERENCE] about 196 Hz is the ceiling
- **options**: Compute motion positions as floats per frame so the release is not stepped at 8.3 ms; align frames to the predicted beat (L04)
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Frozen criterion: jitter of beat-event photon timestamps, p99-p50 <= 8.3 ms. Planted fault: rendering at 60 Hz must exceed 12 ms.

### D09-L11 — colour position, saturation and lightness applied after chroma
- **container**: map
- **controls**: colour (the slow layer)
- **current**: [FACT, brief/P6] Colour is the chroma centroid behind a tonality gate, with inertia. Emotiscope has key detection (src1/key_detection.h:42); whether the Titan uses it is unknown.
- **info_loss**: the major/minor and fast/slow dimension is not used
- **origin**: n/a
- **still_needed**: taste; the Captain decides
- **options**: Following Palmer et al. 2013 (PNAS; r 0.89–0.99), push fast or major music toward more saturated, lighter, yellower colour, and slow or minor toward the opposite, as a bounded bias. No rainbow defaults.
- **impact**: 3
- **novelty**: 3
- **effort**: 3
- **experiment**: Frozen criterion: on matched major/minor pairs, mean saturation moves in the predicted direction for >= 80% of pairs. Planted fault: a sign-flipped mapping must fail. The Captain's preference is a separate on-device A/B judged by eye.

### D09-L12 — effect parameter mapping in inhouse_waveform.cpp (decay and deposit)
- **container**: map/VP
- **controls**: motion
- **current**: [FACT, P6 ruling] Decay = 0.8 + 3.5 x peak, so louder music fades faster. The Captain ruled that level should drive deposit and width, not faster decay.
- **info_loss**: [INFERENCE] the current coupling removes sustain on loud passages
- **origin**: inherited
- **still_needed**: no, per the ruling
- **options**: Let audio drive how fast things move, not only how big they are (Resolume's speed mode; LedFx beat/bar oscillators). Map tension (L02) to outward velocity so the release reads as thrust.
- **impact**: 4
- **novelty**: 2
- **effort**: 2
- **experiment**: Frozen criterion: in the 300 ms after the drop, front velocity (px/s) is >= 2x the mean over the last 2 s of the build, for >= 80% of drops. Planted fault: a constant-velocity build must fail.

### D09-L13 — new phrase counter (8/16/32 bars) built on bar phase
- **container**: map/system
- **controls**: composition over minutes; avoids monotony
- **current**: [INFERENCE] absent
- **info_loss**: the phrase time scale is missing
- **origin**: n/a; prior art is SoundSwitch Autoloops (8–128 bars) and Lumi phrases
- **still_needed**: taste
- **options**: When a phrase boundary is detected with high confidence, change a secondary parameter (mirror mode, spread) with a transition aligned to the bar
- **impact**: 3
- **novelty**: 3
- **effort**: 3
- **experiment**: Frozen criterion: >= 70% of look changes land within +/-1 beat of annotated phrase starts. Planted fault: switching at random times must fall to <= 20%.

### D09-L14 — constraint layer after composition, before the wire
- **container**: output/system
- **controls**: brightness transitions; transitions into or out of saturated red
- **current**: [FACT, brief] 24/255 cap on this unit. No flash-safety check (ITU-R BT.1702 style) is known [INFERENCE].
- **info_loss**: none; it is a constraint, not a signal stage
- **origin**: n/a
- **still_needed**: yes, as product hygiene
- **options**: Count large luminance changes in a sliding one-second window: no more than 3 when the change exceeds a quarter of the darker state; treat saturated red specially. The Music Assistant Hue plugin only warns.
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: Frozen criterion: 0 windows over the limit across the corpus. Planted fault: a synthetic 8 Hz strobe must be flagged.

### D09-L15 — publication_ struct in inhouse_audio.cpp (:237-254, :340)
- **container**: AP/map interface
- **controls**: the vocabulary every effect draws on
- **current**: [FACT] Published values are raw_rms, vu_level, peak_scaled, sub_rms (V9), novelty and band sums.
- **info_loss**: the information is kept, but there are no named structural signals
- **origin**: inherited from SPH and Emotiscope
- **still_needed**: extend it
- **options**: Adopt named musical signals with confidence values, after TouchDesigner and Lightjams: kick, snare, centroid, partials density, bar phase, tension, release, energy level
- **impact**: 3
- **novelty**: 2
- **effort**: 3
- **experiment**: Frozen criterion: each named signal passes its own synthetic truth test >= 90%. Planted fault: swapping two signal slots must fail both tests.

## Failures mapped

- LedFx/aubio (issue #2058, 3 Oct 2026) and the Titan P7 failure are the same class. LedFx: beats ~46 ms late, off-beat lock (F 0.642), tempo 2–3% high, octave errors at 80–90 and 170–180 BPM. Titan: half-time lock from the 88 BPM prior, predicted downbeats 26–380 ms off.
- WLED's 16 one-byte GEQ bands and its 16-slice 'Spectrum' palette are the same over-compression the Titan had before v9 (16-slot averaged palette, one colour per frame).
- WLED says its peak detection 'is NOT beat detection', and Nanoleaf responds to 'volume and duration'. Titan: the Tempo board has no path from level to brightness (P6 ruling).
- A Nanoleaf reviewer wished it were 'better at identifying bass and treble'. Titan: no analysis below 110 Hz; sub-bass arrives 70–120 ms after the drop.
- Yadati 2014 calls drop detection 'challenging'. Titan: three drop detectors missed the +50 ms window.
- Fadecandy and Emotiscope need temporal dithering at 400+ Hz to look smooth at low brightness. Titan: any 8-bit stage at the 24/255 cap (authored-history seed at palette_runtime.cpp:92-94; 8-bit palette position at :75-76).
- WLED recommends its slowest AGC ('Lazy') for music with strong dynamics. Titan: a ~30 ms follower attack and ~190 ms per-lane auto-gain probably level the build and drop to the same height [INFERENCE].
- LedFx users report 'almost strobing with any effect' (#371). Titan: no flash-safety layer; matched-energy evaluation rule.
- Hue is 'a beat behind' and Wi-Fi jitter runs 20–80 ms; Govee's per-device microphones disagree. Titan has the structural opposite: a local microphone and one audio stage feeding both boards.

## Contradictions

- Lightjams presents real-time build-up and drop detection as a routine feature, but publishes no accuracy. Yadati et al. (ISMIR 2014) call drop detection challenging, and the Titan's own three detectors failed at +50 ms.
- Sources disagree on the tolerated sound-to-light delay because they answer different questions. The binding-window literature gives about ±100–200 ms; Arrighi et al. give a tempo-dependent best offset of 30–100 ms; Meier et al. call 46 ms unsuitable (an audio-to-audio case); Emotiscope claims 'without any visible latency whatsoever' with no measurement. These figures should not be pooled.
- WLED documentation says peak detection is 'NOT beat detection', yet Twinkly and Nanoleaf market BPM or beat accuracy with no published numbers.
- LedFx's delay_ms sounds like latency compensation, but issue #2058 says it is an output delay for Bluetooth speakers, unrelated to beat latency.
- Hue + Spotify is marketed as 'real time' analysis, but it reads pre-computed metadata. That is how it can anticipate beats, and Spotify closed audio_analysis and audio_features to new apps on 27 Nov 2024.
- Brightness cap: 24/255 on this owner's unit, against 100/255 as the Kx-Titan migration default (per the brief). Not resolved here.
- Line numbers: the brief cites inhouse_waveform.cpp:209-231 at 81ac031, but this worktree's HEAD is 1ef5395, so cited lines may have moved.

## Open questions

- On the v9 product build, which effects read peak_scaled and the per-lane gain (inhouse_audio.cpp:250-254, :338-339)? Does the waveform deposit see the normalised or the raw level?
- How large is the measured raw level step at annotated drops before and after each gain stage, and how much survives to the visual stage? (The brief cites x3.93 for the 'donor level'.)
- Does the microphone, through its port and PDM decimation, capture 40–110 Hz well enough for a kick or sub channel?
- Does anything in the v9 path narrow to 8 bits between the waveform trail and the wire, beyond the authored-history seed and the 8-bit palette position?
- How long does a ~100k-parameter causal CRNN take per hop on the RA8P1's Ethos-U55 at current clocks? No public figure was found.
- Which brightness cap governs the product: 24/255 or 100/255?
- Would the Captain want phrase-level look changes (L13) or tempo/mode colour bias (L11)? Both are taste calls; each needs a small on-device A/B.
- Training-data provenance and licensing for any NPU model (L08), and the GPL lineage of reused Emotiscope code.
- Three PDFs (Solberg 2014, Yadati 2014, Palmer 2013) could not be text-extracted (binary index, markitdown 406 or disconnect, PNAS 403). Their claims are cited only at the abstract level and should be read in full before any design decision rests on them.
