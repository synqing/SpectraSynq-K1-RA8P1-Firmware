# D12-creative-vision — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

1. Why the drop cannot hit harder today. In mode 32 the deposit moves further from the centre as the music gets louder (inhouse_waveform.cpp:976), so the build climax and the drop draw the same picture. A drop that releases from the origin needs a separate state, held apart from loudness. That release code exists (updateReleaseV9) but is off, and every trigger tried so far has failed. [INFERENCE]
2. The light has spare power it never spends: level saturates at -9 dBFS in 34% of drop frames and about 60% of the range is unused. The strongest idea is a "drop bundle": hold brightness and extent back during the build, then spend them at the drop as a fast front from the centre, carrying the new colour with it (concepts C01, C02, C12, C15).
3. The cheapest high-value ideas each change one existing setting: colour on a circle of fifths instead of a chromatic circle (C04); harmonic tension deciding how much of the palette shows (C05); sub-bass that moves the light instead of flashing it (C07); kicks pull light inward, snares bloom it outward (C19); deposit shape following timbre (C18).
4. Features already built are switched off: the product build compiles out the kick, snare, hi-hat, chord, saliency and tempo-field pipeline (k1_live_runtime.cpp:165/271/319). An 8-dimension timbre latent also exists and is unused.
5. The output curve is linear (inhouse_waveform.cpp:37-40), so the 16-bit LEDs are under-used. Long smooth tails that still end at true zero are possible.
6. There are 19 concepts. Each has a frozen host criterion, a planted fault that must turn it red, and the smallest by-eye A/B for the Captain. Nothing has been built or run on the device. The sketch PNGs are a synthetic model, not renderer output.
7. The harness refused the REPORT.md write, so the full report content is carried in the levers below.

## Levers

### D12-L01 — renderV9 extent map, inhouse_waveform.cpp:976-977 (mode 32: position = 80 + 0.7*peak_stage2*80), :995-998 (mode 18)
- **container**: VP
- **controls**: space: lit length and centre gap
- **current**: v8/v9: louder input sits closer to the edge. A release scale (1-release_) exists, with release off by default (inhouse_waveform.h:75).
- **info_loss**: [INFERENCE] Loses the difference between build climax and drop: after AGC both have peak about 1 and draw the same extent. The synthetic model reproduces it: the lit fraction falls from 0.8 to 0.5 as the build gets louder.
- **origin**: inherited aesthetic (K1 Waveform Hybrid K1); not an S3 budget limit
- **still_needed**: The mapping is a valid look and matches the Captain's build description. What is missing is a second state that can override it at the release.
- **options**: Drive extent from a tension charge instead of level; invert at the release; hold full extension 1-2 s after the release.
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: See concept C01 (EXT-v2 plus no-climax-false-release; planted fault: charge forced to 1).

### D12-L02 — updateReleaseV9 inhouse_waveform.cpp:755-796; config inhouse_waveform.h:64-75 (enabled=false)
- **container**: VP
- **controls**: space and timing of the release
- **current**: Off. Release candidates X1-X3 fired on the climax or the riser. Kick candidates K1-K3: sub return 70-120 ms late. Drop detectors v1-v3 failed (STATUS 14:45Z, 14:56Z, 16:13Z, 16:50Z).
- **info_loss**: The trigger is one-shot with no memory of charge; section memory is only a 6 s dip ring.
- **origin**: P6 EXT iteration
- **still_needed**: Yes. It is the only release mechanism. Cost is under 1k cycles per frame. [INFERENCE]
- **options**: Add a charge filled only by build evidence: sub absent, rising high band or centroid, rising onset density. The release may fire only when charged, with size equal to the charge. Either trigger may fire it: sub return, or the first broadband onset after a dip.
- **impact**: 5
- **novelty**: 3
- **effort**: 3
- **experiment**: C01: EXT-v2 on all tuning tracks, plus zero releases within 2 s of annotated non-drop climaxes. Planted fault: charge forced to 1 must fire on climaxes and turn it red.

### D12-L03 — deposit gain, level ceiling and emphasis, inhouse_waveform.h:35-49; depositV9 :828-840
- **container**: VP
- **controls**: brightness; contrast between build and drop
- **current**: Gain32 0.396, ceiling -9 dBFS, emphasis 1.5. Saturates in 34% of drop frames; about 60% of the renderer range is unused (P6 erratum in STATUS).
- **info_loss**: The drop cannot be brighter than the late build.
- **origin**: Matched to v8's mean light as a test-fairness rule (P6 frozen rule); aesthetic and inherited, not a budget limit.
- **still_needed**: No as a product rule.
- **options**: A section-level iris that closes with charge during the build and opens fully at the release. It reopens slowly if no release comes within N bars. Never raises the floor.
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: C12: drop/build ratio of p95 luma >= 1.5 on every tuning track (luma, not channel-sum median). Planted fault: iris held at a constant 0.5 must fail.

### D12-L04 — transport inhouse_waveform.cpp:966-1019
- **container**: VP
- **controls**: motion
- **current**: Mode 32 scrolls outward at a constant 405 px/s in whole-pixel steps, at most 8 per frame (about 4 px jumps at 100 fps). Mode 18 blends tempo with 30+90*onset.
- **info_loss**: Sub-pixel position and all velocity expression; mode 32 motion ignores the music apart from the deposit.
- **origin**: S3-era integer millis() scroll arithmetic kept on purpose (:113-116), so S3 budget and inherited.
- **still_needed**: No for whole-pixel steps. Fractional anti-aliased transport is about 160 lerps per board per frame (<0.1% of a 1 GHz frame). [INFERENCE]
- **options**: Fractional transport; ballistic release fronts (C02); onset-momentum flywheel (C06); riser accelerando (C17).
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: C02: the front crosses 90% extent within 150 ms of the release. Planted fault: a constant-405 px/s front must fail.

### D12-L05 — chroma centroid inhouse_waveform.cpp:167-176 (v8 stage) and applyShapeCentroidP :614-624
- **container**: map
- **controls**: colour
- **current**: Chromatic circle (i/12*tau).
- **info_loss**: [INFERENCE] Keys a fifth apart, which are closely related, sit 210 degrees apart. A consonant triad gives a low centroid strength while dissonant clusters give a high one.
- **origin**: inherited from SensoryBridge/K1 chroma
- **still_needed**: unknown; taste decides
- **options**: Re-index the chroma as i -> 7i mod 12 before the centroid. One line at each of two places; zero cost.
- **impact**: 3
- **novelty**: 4
- **effort**: 1
- **experiment**: C04: share of frames with centroid strength >= 0.08 rises by 1.5x or more on tonal tracks, with colour still moving in >= 90% of seconds. Planted fault: a random fixed permutation must not reach the rise.

### D12-L06 — path P per-pixel position = offset + d, outputV9 inhouse_waveform.cpp:885-890
- **container**: VP
- **controls**: colour and space: how much of the palette is visible across the strip
- **current**: k = 1 fixed (one palette turn per half-strip) when path P is selected. The product default is still variant B (inhouse_waveform.h:46).
- **info_loss**: A fixed k cannot show harmonic clarity. A full turn crosses the black ends of palettes 31 and 38 (product_palette_data.cpp:876-884, 1084-1095), giving an accidental dark band.
- **origin**: K1 c2738b88 freq_prog mapping, inherited
- **still_needed**: Per-pixel sampling: yes (it is the over-averaging fix). A fixed k: no.
- **options**: k = f(fifths-circle dispersion or spectral flatness), clamped to [0.15, 1] so the strip is never a single colour. Offset the turn so the dark end lands at the edges.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: C05: count of distinct palette positions follows the annotated build ramp (Spearman >= 0.5) and never falls below 4. Planted fault: k = 0 must fail the floor of 4.

### D12-L07 — outputChannel inhouse_waveform.cpp:37-40; 16-bit gains palette_runtime.cpp:714-737; ws2816_pack.h:28-35
- **container**: output
- **controls**: brightness; dynamic range of tails
- **current**: Linear Q16 written to 16-bit, with no perceptual curve.
- **info_loss**: [INFERENCE] Under a 24/255 cap, 16-bit gives about 6100 levels against 24 at 8-bit. With a linear curve, perceived steps are crowded at the low end.
- **origin**: 8-bit FastLED-era linear assumption, inherited
- **still_needed**: No. A lookup table is about 480 operations per board per frame.
- **options**: A perceptual (L*-like) curve before the colour multiply; ember tails 1-3 s that still decay to exactly 0.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: C11: exact 0 reached after silence; distinct non-zero codes in a fade >= 10x the linear path. Planted fault: 8-bit quantisation (>>8<<8) must fail.

### D12-L08 — fadeV9 inhouse_waveform.cpp:800-807 (+10/s when silent); colour anchor tau 15 s at :671; silence_dwell_ms 5000 (inhouse_audio.h:40)
- **container**: VP
- **controls**: brightness and timing during cuts
- **current**: Fade-to-black on cuts was improved and approved by the Captain on 14 Oct.
- **info_loss**: The drain discards where and in what colour the music was; only the colour anchor survives.
- **origin**: P6 design
- **still_needed**: yes
- **options**: Implosion fade (light collapses toward the centre as it dies); remember extent and colour; relaunch them on return.
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: C03: every pixel exactly 0 within 400 ms of confirmed silence, with the centroid moving inward monotonically. Planted fault: kBrightnessFloor must fail.

### D12-L09 — NEW output warp stage after the field and before colour (outputV9); input sub_rms (inhouse_waveform.h:109, core/audio/sub_band.h)
- **container**: VP
- **controls**: space: light moves rather than brightens
- **current**: sub_rms only feeds the release trigger, which is off. Every visual is blind below 110 Hz.
- **info_loss**: Sub-bass (40-110 Hz) is invisible.
- **origin**: n/a (new)
- **still_needed**: New capability: about 160 interpolations per board per frame.
- **options**: x' = x - A*s(t)*sin(pi*x/80), with s a high-passed sub envelope so a sustained 808 does not hold the warp.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: C07: centroid displacement vs sub envelope r >= 0.5 AND total luma vs sub r <= 0.2. Planted fault: the warp implemented as a brightness gain must fail the luma clause.

### D12-L10 — boot composition (lower mode 32 / palette 32, upper mode 18 / palette 39; canon v8) with independent per-board renders; shared audio publication
- **container**: system
- **controls**: vertical space across the shared diffuser
- **current**: Independent boards; no musical coupling.
- **info_loss**: The diffuser's vertical axis carries nothing musical.
- **origin**: Captain's 11 Oct independent-state ruling (MUSIC-VISUAL-VALUE-TAKEOVER:342-379)
- **still_needed**: unknown; depends on the diffuser optics
- **options**: Register split, bass on the bottom board and treble on the top (C08); antiphony (C09); vertical alignment as consonance (C10).
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: C08: bottom/top luma ratio vs low/high energy ratio r >= 0.6. Planted fault: swapped bands give a negative r. Needs a one-sided on-plate capture first.

### D12-L11 — depositV9 width and shape inhouse_waveform.cpp:826, :845
- **container**: VP
- **controls**: space and texture
- **current**: Width = 0.5 + 3.5*level, triangular.
- **info_loss**: Width repeats level; timbre is never expressed.
- **origin**: P6 iteration 1
- **still_needed**: no as a level copy
- **options**: Width from spectral flatness or spread and from attack sharpness.
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: C18: width vs flatness r >= 0.5 and width vs level |r| <= 0.3. Planted fault: width = level must fail.

### D12-L12 — mode 18 energy_rate = 30 + 90*onset, inhouse_waveform.cpp:990-992; tempo blend :986-993
- **container**: VP
- **controls**: motion and timing
- **current**: Direct onset-to-speed with no inertia. The tempo half follows a tracker that locks to half time (P7: 88 BPM prior; weight at its 0.7 cap in 53% of BOOM frames).
- **info_loss**: Groove continuity; locked to half time.
- **origin**: P6 constraint d
- **still_needed**: Partly.
- **options**: An onset-impulse flywheel with drag tau 0.4-0.8 s; no phase needed.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: C06: scroll-velocity spectrum peaks within 3% of the annotated beat rate (or 2x) on BOOM and Sonic. Planted fault: a 69 BPM synthetic impulse clock must peak at half time and fail.

### D12-L13 — AudioPipeline compiled out under K1_EMOTISCOPE_PORT (k1_live_runtime.cpp:165 #ifdef, :271 #else, :298 pipeline_.process, :319 #endif; build_scalar.py:887); EmotiscopeMl 8-D latent (emotiscope_ml.h:20-35)
- **container**: AP
- **controls**: any: percussion roles, chord, saliency, texture
- **current**: Documented and implemented, but not on the product music path. The ML palette consumer is bounded to +/-1/16 turn.
- **info_loss**: Kick, snare, hi-hat, chord type, 4-axis saliency, multi-candidate TempoFieldV1 with ratios.
- **origin**: build configuration
- **still_needed**: Unknown. M85 cost not measured; the in-house AP maximum is 1.29M cycles of a 5 ms hop (P4).
- **options**: Enable alongside the donor, or derive equivalent features from the 80 bins plus sub_rms.
- **impact**: 3
- **novelty**: 4
- **effort**: 3
- **experiment**: C19 and C13 (below).

### D12-L14 — colour anchor inhouse_waveform.cpp:671; deposit_offset_ :957; colour_offset control
- **container**: VP
- **controls**: colour over time
- **current**: v9 slowly follows harmony; path P sets the offset every frame (K1 ae5d90a). P3: at about 49.5 s the drop switched from gold to red, so the drop rendered dimmer.
- **info_loss**: No section-level colour structure; the drop can land on a darker colour.
- **origin**: P6 / K1 ae5d90a
- **still_needed**: yes
- **options**: Commit a new colour chapter only at a release, carried in by the release front, with luma at least the build's.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: C15: palette change inside the 150 ms front window and drop-chapter luma >= build-chapter luma. Planted fault: a chapter forced to the palette's dark end must fail.

### D12-L15 — NEW negative-space layer; input top note bins (real at 24 kHz since P4: hi-hat bins r 0.79, STATUS 10:33Z)
- **container**: VP
- **controls**: texture in dark regions
- **current**: High bins only feed the chroma fold.
- **info_loss**: Hi-hat and shaker detail is not visible.
- **origin**: n/a
- **still_needed**: new
- **options**: Glints of about 30 ms, placed only where intensity < epsilon, with density from high-band flux.
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: C14: glints per second vs HF flux r >= 0.5, overlapping the main field < 5% of the time. Planted fault: uniform placement must fail the overlap clause.

### C01-Spring — Levers L01, L02, L03, L08. Charge = sub absent x rising HF or centroid x rising onset density. Release on sub return or the first broadband onset after a dip; size = charge.
- **container**: VP concept
- **controls**: SEES: through the build the waveform is pressed further into the outer edges and the centre darkens. At the drop the compression lets go: light fills from the origin to full extension and holds 1-2 s, then settles.
- **origin**: Captain's 'hits harder' definition (STATUS 14:33Z)
- **options**: Cost: about 20 floats and under 1k cycles per frame. Prior art: LGPStarBurstNarrative REST/BUILD/HOLD/RELEASE; Song-Aware Director build/drop states (claude-mem #50859). New here: the charge is the visible extent, and the charge gate prevents climax false releases (the X1-X3 failure). Gimmick risk: medium if it fires every chorus.
- **impact**: 5
- **novelty**: 3
- **effort**: 3
- **experiment**: Captain A/B: one image with v9 and v9+Spring selectable, BOOM drop twice each; question: 'does the drop hit harder than the build?'. Frozen: EXT-v2 plus no release within 2 s of non-drop climaxes. Planted fault: charge always 1. Depends on drop timing (70-120 ms late sub).

### C02-ThrustFront — Levers L04 (fractional transport), L07, L02. Drivers: release size from C01; drag constant (taste).
- **container**: VP concept
- **controls**: SEES: a bright, anti-aliased 2-3 px front leaves the centre at 500-1000 px/s, slows under drag, and leaves a glowing wake; it reaches the edges in 100-150 ms. Speed scales with the stored tension (drop_zoom.png panel B).
- **origin**: new
- **options**: Cost: about 200 operations per frame. Prior art: BeatPulseVoid, LGPWaveCollision rings. New here: front speed and drag encode the release size, with sub-pixel precision that 16-bit keeps smooth when dim. Gimmick risk: low if it fires on releases only.
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: Spring with an instant fill vs Spring with the front. Frozen: front centroid at >= 90% extent within 150 ms, and front luma >= 1.5x the build's p95 pixel luma. Planted fault: a constant-405 px/s front fails.

### C03-BlackWithMemory — Levers L08, L14, L02
- **container**: VP concept
- **controls**: SEES: in a silent cut, light collapses toward the centre as it dies, reaching true zero. When the music returns, the first light relaunches in the remembered colour and at the remembered extent.
- **origin**: Captain approved the fade-to-black improvement (14:06Z)
- **options**: Negligible cost. Prior art: LGPTimeReversalMirror replays history; an implosion into black as the sign of a cut was not found in the trees read. Gimmick risk: low. Must not treat quiet music as silence.
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: BOOM silent cut, current drain vs implosion drain. Frozen: every pixel exactly 0 within 400 ms; centroid moves inward monotonically. Planted fault: kBrightnessFloor.

### C04-FifthsCircle — Lever L05 (inhouse_waveform.cpp:172, :618)
- **container**: map concept
- **controls**: SEES: related chords and keys get related colours; a change to the dominant moves colour a small step and a distant key moves it far. Tonal passages hold a clearer colour.
- **origin**: new to this product
- **options**: Zero cost. Prior art: circle-of-fifths colouring is known in music visualisation; LGPChordGlow maps roots chromatically; the K1 and Titan lineages are chromatic. Gimmick risk: very low.
- **impact**: 3
- **novelty**: 4
- **effort**: 1
- **experiment**: Captain A/B: Sonic Samba and BOOM, chromatic vs fifths. Frozen: strength >= 0.08 in >= 1.5x as many frames on tonal tracks, and colour moves in >= 90% of seconds. Planted fault: a random permutation.

### C05-ConsonanceSpread — Levers L06, L05; path P must be the product default first (PL14-B14)
- **container**: VP concept
- **controls**: SEES: in a stable chord the strip is one colour family; as tension rises (riser, dissonance, noise) colour fans out across the whole palette from centre to edge; on resolution it draws back in (kymographs.png panel D).
- **origin**: new
- **options**: Cost: about 10 operations per frame. Not found in any of the three lineages read: colour diversity as a musical variable rather than hue. Gimmick risk: low. Floor k >= 0.15 keeps the never-a-single-colour rule.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Captain A/B: a build, path P k=1 vs k=f(tension). Frozen: distinct palette positions vs build ramp Spearman >= 0.5, never below 4. Planted fault: k = 0.

### C06-GrooveFlywheel — Levers L12, L04
- **container**: VP concept
- **controls**: SEES: each onset kicks the outward flow and drag slows it between kicks. A steady groove gives a steady stream, a drum fill speeds it up, a breakdown coasts to a stop. Cannot lock to half time.
- **origin**: answers P7 half-time lock
- **options**: Negligible cost. Nearest prior art: KuramotoTransport. Gimmick risk: low.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: upper board on BOOM, current vs flywheel. Frozen: velocity spectrum peak within 3% of the annotated beat rate or 2x. Planted fault: a 69 BPM impulse clock.

### C07-SubPush — Lever L09 (sub_rms)
- **container**: VP concept
- **controls**: SEES: on each sub hit the whole light pattern is shoved outward a few pixels like a speaker cone and springs back. Brightness stays the same; the light moves (kymographs.png panel C).
- **origin**: new; uses the existing v9 sub_rms
- **options**: Cost: about 160 interpolations per board per frame. Prior art: LGPBassBreath brightens and expands; displacement by sub-bass was not found. Gimmick risk: medium (wobble on sustained 808s); the envelope high-pass is mandatory.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Captain A/B: kick-heavy passage, warp off vs on. Frozen: displacement r >= 0.5 and luma r <= 0.2 against the sub envelope. Planted fault: warp as a gain.

### C08-PitchHeight — Levers L10, L13: bottom board renders low bins plus sub_rms; top board renders mid and high bins
- **container**: system concept
- **controls**: SEES: bass lives on the bottom board and treble on the top. The diffuser's vertical glow tilts with the spectral balance: a bass drop floods from the floor up; a hi-hat break hangs from the ceiling.
- **origin**: new
- **options**: Low cost. Pitch-to-height is a strong cross-modal association; that it reads on this plate is [HYPOTHESIS]. Lightwave dual-strip effects use phase offsets, not register. Gimmick risk: low.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: Dopex, current roles vs register split. Frozen: luma ratio vs energy ratio r >= 0.6. Planted fault: swapped bands. Needs a one-sided on-plate capture first.

### C09-Antiphony — Lever L10; reuse of PairedWaveScene (paired_wave_scene.h:14-75)
- **container**: system concept
- **controls**: SEES: a phrase on one board answers on the other a beat or a bar later, mirrored in colour.
- **origin**: prior art in tree
- **options**: Cost: a delay ring. Prior art: PairedWaveScene edge hand-off 0.18 s, beat_handoff; InterStripPhaseDelay. Gimmick risk: medium.
- **impact**: 3
- **novelty**: 2
- **effort**: 3
- **experiment**: Frozen: cross-board lagged correlation peaks at the authored delay +/-1 frame. Planted fault: delay 0.

### C10-AlignmentConsonance — Levers L10, L05
- **container**: system concept
- **controls**: SEES: consonance puts light on both boards at the same x, giving full-height columns across the plate; tension staggers the boards into a tooth or zigzag pattern.
- **origin**: extends Lightwave
- **options**: Prior art: CrossStripWaveInterference uses a fixed 3pi/4 offset (Captain's K1v2 preflight on 5 May found it most visible). New here: the offset follows the music. Gimmick risk: medium.
- **impact**: 3
- **novelty**: 4
- **effort**: 3
- **experiment**: Frozen: column overlap vs centroid strength Spearman >= 0.5. Planted fault: offset driven by a random walk. Diffuser optics unknown.

### C11-EmberTails — Levers L07, L08
- **container**: output concept
- **controls**: SEES: fades fall in perceptually even steps and can linger as faint embers for 1-3 s before exactly zero, so quiet passages keep a memory of the last phrase and the drop's contrast is larger.
- **origin**: asymmetric use of WS2816 16-bit
- **options**: Cost: a 480-operation lookup table per board per frame. Prior art: Lightwave PerceptualJND, K1 Lever-2. Gimmick risk: low. WS2816 behaviour at its lowest codes is unmeasured.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: quiet passage in a dark room. Frozen: exact 0 after silence and >= 10x distinct fade codes. Planted fault: 8-bit quantise.

### C12-LuminanceIris — Levers L03, L02
- **container**: VP concept
- **controls**: SEES: as a build intensifies the light gets denser (motion, spread, compression) but not brighter; at the release the iris opens fully, so the drop is the brightest moment. Never darkens the floor.
- **origin**: P6 erratum: 60% headroom unused
- **options**: Negligible cost. Standard lighting-design practice, absent from every lineage read. Gimmick risk: low; a slow reopen handles builds with no drop.
- **impact**: 5
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: drop bundle (C01+C02+C12+C15) vs v9 at matched maximum brightness. Frozen: drop/build p95 luma >= 1.5. Planted fault: iris also applied to the drop.

### C13-LatentTexture — Levers L13, L11 (EmotiscopeMl latent)
- **container**: VP concept
- **controls**: SEES: when the sound's character changes (a new synth or instrument enters), edge softness and grain change while colour stays the same.
- **origin**: embedded capability
- **options**: Existing inference. Gimmick risk: high; an unlabelled latent may move arbitrarily.
- **impact**: 2
- **novelty**: 4
- **effort**: 3
- **experiment**: Prove on host first. Frozen: latent-distance peaks hit annotated instrument entries >= 60% within +/-0.5 s. Planted fault: shuffled-time latent.

### C14-NegativeSpaceGlints — Lever L15
- **container**: VP concept
- **controls**: SEES: hi-hats and shakers appear as fine, brief pinpricks only in the dark gaps, never on top of the main waveform.
- **origin**: new
- **options**: Low cost. Prior art: ConfettiEffect. New here: placed only in dark space and driven by real HF flux. Gimmick risk: high (glitter).
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: Sonic Samba off vs on. Frozen: HF flux r >= 0.5 and overlap with the field < 5%. Planted fault: uniform placement.

### C15-ColourChapters — Levers L14, L02, L04, L06
- **container**: VP concept
- **controls**: SEES: the drop's new colour is carried in by the release front from the centre to the edges, never faded in, and is at least as bright in luma as the build's colour.
- **origin**: P3 finding
- **options**: Negligible cost. Prior art: LGPStarBurstNarrative phrase-gated chord commits. New here: the colour is committed spatially by the front. Fixes P3's gold-to-red dim drop.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Captain A/B: drop bundle with vs without chapter carry. Frozen: colour change inside the front window and luma >= the build's. Planted fault: chapter forced to the dark end.

### C16-MetricHierarchy — Levers L01, L12
- **container**: VP concept
- **controls**: SEES: downbeats reach full extent, beats reach half, off-beats a quarter; the bar structure becomes visible as nested reach.
- **origin**: depends on the beat stage
- **options**: BLOCKED on P7: the tracker locks to half time and phase is 26-380 ms off.
- **impact**: 3
- **novelty**: 3
- **effort**: 4
- **experiment**: Frozen: downbeat/off-beat reach >= 2. Planted fault: half-time clock.

### C17-RiserAccelerando — Levers L04, L02. Driver: centroid slope x onset density.
- **container**: VP concept
- **controls**: SEES: during a riser or snare roll the outward flow speeds up in proportion to the rise and stops dead at the cut.
- **origin**: new
- **options**: Negligible cost. Gimmick risk: medium (false risers).
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Captain A/B: BOOM build. Frozen: velocity vs annotated risers r >= 0.5, with false speed-ups <= 20%. Planted fault: speed driven by loudness.

### C18-TimbreWidth — Lever L11
- **container**: VP concept
- **controls**: SEES: a pure tone or sub draws a razor-thin line, a snare or noise a wide soft body, a pad a broad slow glow.
- **origin**: new
- **options**: Cost: about 80 operations per frame. Gimmick risk: low.
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: Captain A/B: mixed track. Frozen: width vs flatness r >= 0.5 and width vs level |r| <= 0.3. Planted fault: width = level.

### C19-PushPull — Levers L09, L13, L01. Kick proxy: sub_rms + low-bin flux. Snare proxy: broadband mid flux. Or the compiled-out AudioPipeline kick and snare.
- **container**: VP concept
- **controls**: SEES: kicks pull the light inward for a moment and snares bloom it outward, so a four-on-the-floor groove makes the plate breathe in time.
- **origin**: new
- **options**: Low cost. Opposite gestures for kick and snare were not found in the trees read. Gimmick risk: medium.
- **impact**: 4
- **novelty**: 4
- **effort**: 2
- **experiment**: Captain A/B: BOOM groove. Frozen: inward step on kicks and outward step on snares, each with a hit rate >= 70% within +/-30 ms. Planted fault: swapped proxies.

### RANKING — impact x novelty / effort
- **container**: system
- **controls**: priority
- **options**: C04 12.0; C18 9.0; C05, C07, C15, C19 8.0; C02, C12 7.5; C06, C08, C11, C17 6.0; C01 5.0; C03, C14 4.5; C10 4.0; C13 2.7; C16 2.3; C09 2.0. Correction: C01+C02+C12+C15 form one 'drop bundle' that answers the Captain's only standing negative verdict and has the highest combined impact. Proposed on-device A/B order: (1) C04+C18, (2) drop bundle, (3) C07+C19, (4) C05 on path P, (5) C08 after an on-plate capture.
- **impact**: 5
- **novelty**: 4
- **effort**: 2
- **experiment**: Each A/B is one image with both variants selectable at runtime if an existing control field allows it [HYPOTHESIS: the integrator must check the 127-field control surface]. Criteria are frozen with scripts/freeze_tolerance.py before results. The holdout must be a Captain corpus; Carte Blanche is spent.

## Failures mapped

- F1: drop not harder than build (Captain, 14 Oct 14:06Z). Structural cause: loud input maps to a compressed extent (inhouse_waveform.cpp:976), and about 60% of headroom is unused. Addressed by C01, C02, C12, C15, C17.
- F2: the host metric disagreed with the eye (C1 channel-sum median). All D12 criteria use luma or extent instead.
- F3: drop detectors v1-v3 failed; analysis is blind below 110 Hz; the sub arrives 70-120 ms late. C01's charge gate narrows how right the trigger must be. C07 uses sub_rms for sustained displacement, which tolerates the lateness.
- F4: tempo locks to half time and phase is 26-380 ms off. C06 needs no tracker; C16 is explicitly blocked on P7.
- F5: colour over-averaging (16-slot palette, RGB mix). Path P fixes it but is not the product default (inhouse_waveform.h:46, PL14-B14). C05 and C15 require path P.
- F6: the drop colour fell into the palette's dark end (P3: gold to red at about 49.5 s). Answered by the C15 luma guard and the L06 turn offset (palettes 31 and 38 have black ends).
- F7: level map saturates in 34% of drop frames and the gain leaves about 60% of range unused. Answered by C12.
- F8: v9 colour froze (moved in 1% of seconds). C04 may raise centroid strength on tonal music [HYPOTHESIS].
- F9: trails lingered on silent cuts (now improved). C03.
- F10: Captain rule 'never a single colour' (11 Oct). C05 floor k >= 0.15, guarded by a planted fault.
- F11: features built but not running (AudioPipeline compiled out under K1_EMOTISCOPE_PORT). C19 and C13 name the activation path.
- F12: P5 camera found no consistent per-click flash. C02 gives a crisp optical event per release but not per click, so it does not solve P5 [INFERENCE].

## Contradictions

- Brightness cap. The global CLAUDE.md Titan mandate says preserve 24/255. Kx-Titan AGENTS.md:12 says the default 100/255 supersedes 24. Canon v8 boot says master 255 with gains 0.25 (TITAN-CANON-2026-10-13.md:51). Not resolved here.
- Frame rate. The brief says a 120 Hz render target. Source sets a 100 Hz reconstruction cadence (inhouse_waveform.cpp:144-147; inhouse_audio.h:41-43 reference_presentation_hz=100). The device runs 99.7-99.9 fps (STATUS 13:08Z, 14:40Z).
- Palette default. Path P is the agreed over-averaging fix, but kInhouseV9Palette = kRestrictedRange (inhouse_waveform.h:46), so the device runs variant B, which halves colour variety.
- AudioFeaturesV1 advertises kick, snare, hi-hat, chord and beat phase, but the product build compiles out the AudioPipeline that produces them (k1_live_runtime.cpp:165/271/319, build_scalar.py:887). Documented and implemented, but not running.
- The Captain's build description (contraction toward the edges) is already what the level-to-extent mapping does as loudness rises. What is missing is the release, not the contraction [INFERENCE].
- paired_wave_scene.h:8-9 says it does not bind the two-80-pixel bench outputs to a 320-pixel topology. Canon section 5 fixes 2 strips x 160 = 4 lanes x 80. This affects whether PairedWaveScene can be reused as-is.
- The REPORT.md write required by the computed task was refused by the harness, which does not let subagents write report files. The content is delivered in this structured result instead.

## Open questions

- Is a release landing 70-120 ms after the musical drop seen as the hit, or as late? The cheapest test is one track with deliberately delayed releases at 0, 50 and 100 ms, judged by the Captain. This decides whether detection must be predictive.
- How much do the two boards' light fields overlap vertically in the shared diffuser? This decides C08 and C10 and needs a one-sided on-plate capture.
- How does WS2816 behave at its lowest 16-bit codes under the active cap (flicker, colour shift)? This decides C11.
- Which brightness cap binds: 24/255, 100/255, or master 255 x gain 0.25?
- Can the AudioPipeline run alongside the Emotiscope donor and the in-house analysis on the M85, at what cycle cost, and who owns the tempo prior?
- Does an existing runtime control field allow switching variants without a reflash? This sets how cheap every A/B is.
- Taste, for the Captain only: should colour move less under related-key modulation (C04), and what drag and front speed read as 'explosive' (C02)?
