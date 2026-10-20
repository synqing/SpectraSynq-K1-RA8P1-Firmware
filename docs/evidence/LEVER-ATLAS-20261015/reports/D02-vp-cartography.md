# D02-vp-cartography — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

PLAIN ENGLISH (sources: lane/titan-lagfree-20261014 HEAD 1ef5395 plus uncommitted path-P edits; no device access):
1) [FACT] Spare compute: drawing both boards takes about 0.2 ms of each 10 ms frame and packing 0.13 ms (P5 BUDGET.md). So none of the remaining visual compressions is forced by the chip. They are choices, inheritances or accidents. On the S3's 8-bit wire, the same mode-32 chain at a 24/255 cap left about 9.5 brightness steps; the Titan's 16-bit wire leaves 2,442.
2) [FACT, existing replay CSV] Why the drop does not hit harder: the waveform head sits at almost the same place before and after the drop (BOOM board 0: median 126 px in the last 2 s of the build, 130 px in the first 2 s of the drop; the maximum is 136). The audio rises 8.4 dB over that span. The length comes from an AGC-flattened peak capped at 1, and louder means further out, so a drop contracts rather than releases. Brightness does respond (2.6 times more light), but 35% of drop frames hit the -9 dBFS level ceiling.
3) Path P (per-pixel palette) is real, and it is still not selected as the product default in the tree I read (kRestrictedRange, inhouse_waveform.h:46). Most of the '9-16 colours' come from a fixed palette ramp laid along the strip. The music still contributes one harmony number per frame: 80 bins become 12 notes, then 1 angle.
4) On board 0's palette (id 31), 17.5% of positions are near-black and the ends meet as a hard black/yellow edge. Path P sweeps the whole palette along each half of the strip, so part of the strip always masks the deposited light.
5) The trail is beaded: it scrolls 4.05 px per frame while a quiet deposit is 3 px wide (ripple at least 1.7:1 even at full level). The scroll is capped at 8 px per frame (100 ms centre to edge), and scroll speed is never used musically. Brightness, position and colour lag by about 0, 44 and 108 ms respectively.
6) Warmth (board 0: green ×0.67, blue ×0.50) and the white balance change palette hues after sampling. The palette-accuracy metrics measure before this step, so they cannot see it. Mirror off in mode 32 leaves the left half dark (a porting hazard).
The 25 levers are below. Ship path: the P6 lane owner freezes and runs the chosen host experiments, builds profile 9 with product defaults and the PL14-B14 equivalence test, installs, then scores wire snapshots on the Titan; the Captain judges the taste A/Bs.

## Levers

### D02-L01 — Frame tick and AP->VP sampling: palette_runtime.h:39-40 (kPalettePeriodUs 10000 under K1_EMOTISCOPE_PORT); palette_runtime.cpp:356-361; inhouse_audio.cpp:377-430 (acquire = latest hop; peak = max over up to 8 missed hops); fixture_app.cpp:573-579
- **container**: system / VP boundary
- **controls**: timing and motion smoothness
- **current**: v8 and v9: 100 Hz render tick; in-house publication 133.3 Hz (96 samples @ 12.8 kHz or 180 @ 24 kHz, a 7.5 ms hop)
- **info_loss**: DESTROYS: about 25% of in-house hops are never rendered for spectrum, vu and novelty (peak is kept by the max summary). The tick adds a 0-10 ms wait (p50 5 ms; BUDGET.md stage f).
- **origin**: inherited: 'Explicit 100 Hz reconstructed presentation profile' (inhouse_audio.h:41-43); K1 per-frame constants rebased to 100 Hz (inhouse_waveform.cpp:144-147)
- **still_needed**: no. The wire allows about 196 Hz (5.1 ms per frame); a render costs 0.2 ms.
- **options**: Render on each fresh hop (133 Hz, event-driven), use a 200 Hz tick, or phase-lock the tick to the hop. The v9 time constants are already in seconds; keep v8 bit-identical by gating on v9.
- **impact**: 3
- **novelty**: 2
- **effort**: 2
- **experiment**: Host replay at 133 vs 100 Hz. FROZEN: median publish-to-render-start <= 2.5 ms; 0% of fresh hops unrendered; PAL-HD-v2 and C2 still pass. PLANTED: a fixed 100 Hz tick must stay red.

### D02-L02 — Spectrum time smoothing: inhouse_audio.cpp:107-111 (mood LPF cutoff 1+10*mood Hz at a 100 Hz reference), :274 (0.3/0.7 average per hop), :323-326; inhouse_waveform.cpp:147-153 and :598-608 (0.75 per 10 ms EMA)
- **container**: AP boundary -> map
- **controls**: colour (how fast harmony reaches the palette)
- **current**: mood 0.05 on both boards (preset), so tau 79.6 ms + 21.0 ms + 7.2 ms: colour lags about 108 ms (computed, vp_numbers.json)
- **info_loss**: DESTROYS colour detail faster than about 10 Hz
- **origin**: inherited K1 low_pass_array(...,SYSTEM_FPS,1+10*MOOD) (K1 k1_gdft_core.cpp:442). It is evaluated at a fixed 100 Hz reference while hops run at 133 Hz, so it is about 33% faster than nominal (deliberate quirk, inhouse_audio.cpp:313-317). Aesthetic 'Mood'.
- **still_needed**: taste. It is already a live per-board control (inhouse_mood).
- **options**: Use a separate mood for colour and for the motion/flux spectrum; shorten it at drops; set it from phrase state.
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: Host chord-change step. FROZEN: the 63% delay of the path-P offset equals the analytic sum ±10 ms at mood {0, 0.05, 0.5}. PLANTED: mood forced to 1.0 must give a delay below 30 ms (red against the 0.05 expectation).

### D02-L03 — 80 bins -> 12 notes -> 1 centroid: inhouse_waveform.cpp:154-182 (v8 / v9-B); :596-625 (path P, uncommitted applyShapeCentroidP); chroma_range 60 at inhouse_waveform.h:122
- **container**: map
- **controls**: colour
- **current**: v8, v9-B and path P all reduce the spectrum to 1 circular angle and 1 strength per frame. Bins 60-79 (above 3.3 kHz) are ignored.
- **info_loss**: DESTROYS (x2): about 80 magnitudes become 1 number per frame. Octave, timbre, multiple pitch centres and high-band content are lost. Path P's b_spread (TOLERANCE-FROZEN-P6-PAL.json) is produced by the spatial ramp, not by extra audio information.
- **origin**: inherited design, not an S3 budget limit: 'Bloom/Waveform pick ONE palette position per frame by design' (claude-mem #54824 via FORENSICS.md). The fold is cheap on any chip.
- **still_needed**: unknown (taste)
- **options**: (a) A per-deposit spectral signature, e.g. 3 band energies or the top-2 chroma peaks, rendered to per-pixel hue in palette-position space, never RGB. (b) Frequency laid inside each blob (inner = low, outer = high; the K1 GDFT map at blob scale). (c) Spectral centroid (timbre) as a second palette axis. (d) chroma_range 80. Cost about 26 µs per frame [INFERENCE].
- **impact**: 4
- **novelty**: 4
- **effort**: 3
- **experiment**: 10-input host replay. FROZEN: |Spearman rho| >= 0.3, across music frames, between the deposited per-pixel hue spread and the frame's 3-band energy ratio on >= 8/10 inputs, with PAL-HD-v2 fidelity unchanged. PLANTED: a constant signature must fall to rho about 0 (red).

### D02-L04 — Path P spatial ramp: inhouse_waveform.cpp:885-902 (palette position = wrap(offset_[s] + d), d = distance from centre / 80). Product default NOT P: inhouse_waveform.h:46 kRestrictedRange (PL14-B14).
- **container**: VP
- **controls**: colour across space and over a deposit's age
- **current**: v8: one colour per frame. v9-B: one colour per deposit, frozen as it travels. P (host-variant only): ramp gain 1.0, so each half-strip spans a full palette cycle.
- **info_loss**: KEEPS the 16-bit HD gradient per pixel. ADDS a ramp that does not depend on the audio. Because d grows as the trail scrolls at 405 px/s, hue also encodes deposit age: a deposit at px 108 sweeps 64% of the palette in 0.126 s before the edge.
- **origin**: transplanted from K1 GDFT freq_prog = LED/80 (c2738b88), where pixel = frequency. In a waveform, pixel = time.
- **still_needed**: taste
- **options**: Ramp gain k in p = o + k*d (k = 0 gives one hue per deposit); ramp as explicit age ('cooling'); ramp direction; k tied to the release (narrow in the build, full palette burst at the drop). No rainbow defaults.
- **impact**: 4
- **novelty**: 4
- **effort**: 1
- **experiment**: Host first. FROZEN: hue-band count per frame at k = 0.25 is <= 50% of k = 1, and offset motion M (PAL criterion c) is unchanged. PLANTED: k = 0 must make PAL b_spread red. Then a Captain A/B on the device of k in {0.25, 1.0} (taste).

### D02-L05 — product_palette.cpp:79-86 (past the last stop, sampleProductPaletteHd clamps to the last colour; no last->first blend); palette data product_palette_data.cpp:876-884 (id 31), :1084-1095 (id 38)
- **container**: VP / palette data
- **controls**: colour and brightness
- **current**: Board 0 id 31 'BlacK_Red_Magenta_Yellow': 17.5% of positions below 5% luma, ends black -> yellow (RGB L1 jump 2.0). Board 1 id 38: cyclic, 6.2% near-black. Only 13 of 44 palettes have matching ends; 23 contain a near-black stop (vp_numbers.json scan).
- **info_loss**: DESTROYS: under path P about 14 of every 80 px sit on near-black palette positions, so the deposited intensity there is invisible [INFERENCE from 17.5% × 80]. A hard colour edge sits at the wrap pixel.
- **origin**: accident of transplanting: palettes were authored for single-position sampling (black-start FastLED gradients); the it4 variants A/B (inhouse_waveform.cpp:532-581) were earlier dark-end workarounds
- **still_needed**: no
- **options**: Wrap-aware sampling; a 'hue from palette, luma from intensity' mode; a one-to-one sub-range p = lo + (hi-lo)*c (FORENSICS step 4); P allowed only for cyclic palettes. Never add a brightness floor (true black).
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: Host replay. FROZEN: masked fraction (lit-intensity pixels >= 0.01 whose output max channel is < 5% of intensity × palette max) <= 1%. PLANTED: palette 31 unchanged under P must be red (about 17%).

### D02-L06 — Deposit vs scroll stride: inhouse_waveform.cpp:826 (width 0.5+3.5*l), :841-869 (triangle, reach = width+1, max-hold), :966-1019 (405 px/s integer scroll)
- **container**: VP
- **controls**: motion smoothness and trail continuity
- **current**: 4.05 px between deposits at 100 Hz. The triangle is 3.0 px wide at l = 0 and 10 px at l = 1. v8 deposits 2 px per stride (:268-279).
- **info_loss**: DESTROYS (spatial aliasing): for l < 0.15 there are dark gaps between beads. The brightness valley between beads is 0 / 0.15 / 0.38 / 0.60 of the peak at l = 0 / 0.25 / 0.5 / 1, so the trail ripples at least 1.7:1. [HYPOTHESIS] Whether the light-guide plate blurs this is unmeasured.
- **origin**: inherited firmware-v3 0x1313 / K1 dot-and-scroll (K1 light_mode_waveform_hybrid_k1.cpp:11-23, 55). 405 px/s is Captain-calibrated; the beading is a frame-rate side-effect.
- **still_needed**: no
- **options**: Swept deposit (fill the stride between this frame's and the last frame's deposit, interpolating level and offset); fractional sub-pixel scroll; higher render rate (L01); particles (L23). Cost under 10 µs.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: Host steady tone at 3 levels. FROZEN: trail continuity R = median over lit trail px of (5-px window min / max) >= 0.8 at all levels. PLANTED: the current deposit must be red (R <= 0.6). Then wire-snapshot plus camera check of whether the light-guide plate hides it (instrument, not Captain eyes).

### D02-L07 — Scroll speed and step ceiling: inhouse_waveform.cpp:966-1003 (maximum_steps 8 for mode 32, 30 for mode 18; integer steps); motion_scale = speed*2 (palette_runtime.cpp:250)
- **container**: VP
- **controls**: motion speed: the 'explosive thrust'
- **current**: Fixed 405 px/s (preset speed 0.5). The ceiling is 800 px/s, so centre to edge takes >= 100 ms. No musical input drives speed.
- **info_loss**: An unused channel. The ceiling DESTROYS any release faster than 100 ms.
- **origin**: inherited WFHYB_MAX_SCROLL_STEP 8 'per-frame clamp' (K1 light_mode_waveform_hybrid_k1.cpp:56); S3 cost vs aesthetic is unknown
- **still_needed**: no for cost (a few µs per memmove)
- **options**: Speed = base × (1 + thrust × release envelope); a one-frame shockwave deposit from the centre to the head; raise the ceiling together with the swept fill (L06).
- **impact**: 5
- **novelty**: 4
- **effort**: 2
- **experiment**: FROZEN: EXT-v2 (TOLERANCE-FROZEN-P6-EXT-v2.json) plus a new thrust clause: after the annotated drop the wavefront reaches >= 90% of half-length within 60 ms. PLANTED: constant 405 px/s must be red; a thrust 200 ms late must be red.

### D02-L08 — Motion extent: inhouse_audio.cpp:247-254 (peak follower attack 30 ms, release 1.99 s; peak_scaled = drive/follower, attack 5.4 ms, release 57 ms); palette_runtime.cpp:268-269 (clamped to 1 for modes 32/18); inhouse_waveform.cpp:970-977 (EMAs 16 and 23 ms; position = 80 + 56*peak_stage2)
- **container**: map
- **controls**: motion extent: the Captain's waveform 'length'
- **current**: Same in v8 and v9. M2 (v9_final4.boom_source, sha256 16639250…3bd5): head median 126.4 px in the last 2 s of the build vs 129.8 px in the first 2 s of the drop. Room recording: 114.8 -> 129.3 px (outward at the drop).
- **info_loss**: DESTROYS: the follower removes absolute level and slow trends; the clamp erases headroom; polarity louder = further out means the drop contracts instead of releasing.
- **origin**: inherited 0x1313 'louder pushes the dot further from centre' (K1 :10-12); the clamp is the Titan's (palette_runtime.cpp:266-268). [INFERENCE] the AGC is an S3-era room/8-bit need.
- **still_needed**: partly (some room normalisation)
- **options**: Extent from a two-timescale contrast (fast vs 4-8 s baseline) with a reserved range (build uses 0-70% of travel, release resets to the centre); soft knee instead of the clamp; a phrase-state driver (the existing composed_scene_runtime.h:41-69 Direction machine is authored and time-based, so only a candidate to reuse).
- **impact**: 5
- **novelty**: 4
- **effort**: 3
- **experiment**: FROZEN: median head distance from centre in drop +0.5 s <= 0.5 × that in the last 2 s of the build, on >= 3/4 corpus drops, with no such release in the 2 s before. PLANTED: the current mapping must be red (M2: 49.8 vs 46.4 px).

### D02-L09 — Level transfer: inhouse_waveform.h:35-36 (gains 0.395878 / 0.591745), :48-49 (ceiling -9 dBFS, emphasis 1.5); cpp:940-950, :826-840
- **container**: map
- **controls**: brightness and deposit width
- **current**: v9: -33 to -9 dBFS maps to 0-1 on a raw 7.5 ms hop RMS with no ballistics; amplitude = gain × l^1.5. M2: level median 0.61 in the build, 0.96 in the drop; 35.4% of drop frames sit at l = 1.
- **info_loss**: DESTROYS everything above -9 dBFS (clipped)
- **origin**: P6 design (gains match v8's mean light, inhouse_waveform.h:31-33); not the S3
- **still_needed**: the ceiling is arbitrary
- **options**: A ceiling relative to a slow loudness reference with headroom; a soft knee; emphasis from the fast/slow ratio; use the L10 headroom.
- **impact**: 4
- **novelty**: 3
- **effort**: 2
- **experiment**: FROZEN: <= 5% of music frames at the ceiling on every input; drop/build median deposit ratio >= 2.6 on BOOM and >= 1.5 on every corpus drop; mean light within ±10%. PLANTED: a -30 dBFS ceiling must be red.

### D02-L10 — Brightness headroom under the cap: gains inhouse_waveform.h:35-36; display palette_runtime.cpp:283; pack :706-737; scale16 at hd_pixel16.h:28-32
- **container**: output
- **controls**: brightness range available to the drop
- **current**: Top deposit is 0.396 (mode 32) or 0.592 (mode 18) of the post-cap range. At cap 24: top code 2,442 or 3,650. At preset 255 × board 0.25: 6,486.
- **info_loss**: KEEPS precision (11.3+ bits) but leaves 60% (mode 32) or 41% (mode 18) of the cap's allowed light unused at every moment
- **origin**: P6 mean-light matching to v8; the cap is the owner's safety rule (binding)
- **still_needed**: the cap: yes, binding. How the range under it is shared out: free.
- **options**: Keep the build <= 0.4 and let the release reach 1.0 within the same cap. Never a floor (true black).
- **impact**: 5
- **novelty**: 3
- **effort**: 1
- **experiment**: FROZEN: wire-snapshot drop peak code >= 2× the build peak; song mean light within ±10% of current; nothing above the cap. PLANTED: current gains must be red; a cap+1 write must be red on the cap check.

### D02-L11 — Fade: inhouse_waveform.cpp:800-807
- **container**: VP
- **controls**: trail length and decay
- **current**: v9 constant tau 0.29 s (mode 32) and 0.125 s (mode 18); silence adds 10/s (tau 74 and 56 ms). v8 mode 32: tau 1.25 s down to 0.23 s with peak (:254).
- **info_loss**: KEEPS; deliberately removed v8's 'loud = short tail' coupling. Visible length 405 × 0.29 = 117 px, more than 80 px, so tails always reach the edge.
- **origin**: P6 Captain ruling 1 (constant release); silent drain inherited (WFHYB_SILENCE_DECAY); Captain liked fade-to-black on cuts
- **still_needed**: yes (aesthetic)
- **options**: A longer afterglow in the release window; tau set by phrase; account for the silicon gamma (L15).
- **impact**: 3
- **novelty**: 2
- **effort**: 1
- **experiment**: FROZEN: DI (TOLERANCE-FROZEN-P6-DI.json) unchanged; a silent cut reaches black within 100 ms. PLANTED: a silent drain of 0 must be red.

### D02-L12 — Max-hold deposit: inhouse_waveform.cpp:864-868
- **container**: VP
- **controls**: brightness and colour where deposits overlap
- **current**: v9: intensity = max(old, new); the winner's colour or offset replaces the old one
- **info_loss**: DESTROYS the energy of repeated hits (no summation); KEEPS hue purity
- **origin**: P6, from the K1 lesson 'never add palette colours'
- **still_needed**: yes for colour; no for intensity
- **options**: Soft-knee summed intensity while keeping the winner's offset; a circular mean of offsets weighted by intensity (in palette-position space, never RGB).
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: FROZEN: PAL-HD-v2 stays at 0 off-palette pixels. PLANTED: the existing kRgbBlend fault (:857-861) stays red.

### D02-L13 — Mirror and centre origin: inhouse_waveform.cpp:973 (mode 32 zeroes source 0..79 every frame), :876, :881; palette_runtime.cpp:251
- **container**: VP / space
- **controls**: space
- **current**: Mirror on for both boards: 80 independent px per board, 160 of 320 across the product
- **info_loss**: DESTROYS 50% of spatial freedom (redundancy). HAZARD: in mode 32 with mirror_mode false, output 0..79 reads zeroed source, so the left half is dark (v8 :257 too).
- **origin**: centre-origin topology from K1 and the light-guide plate (product identity); the hazard is a porting accident
- **still_needed**: symmetry: taste; the hazard: must be fixed
- **options**: Asymmetric halves (low bands left, high right) keeping the centre origin; a small time offset between halves; board 1 as a complementary channel (pair features exist but are off in the preset).
- **impact**: 3
- **novelty**: 4
- **effort**: 2
- **experiment**: FROZEN: with mirror off, lit px in output 0..79 > 0 on music frames. PLANTED: the current code is the fault and must be red now.

### D02-L14 — Display treatment: palette_runtime.cpp:281-295 (brightness 0.3+0.7b; shoulder >0.75 tanh; warmth {1, 1-w+w*sqrt(.1982), 1-w+w*sqrt(.0244)}; white balance {1, .9375, .84})
- **container**: output
- **controls**: colour (per-channel) and brightness
- **current**: Board-0 warmth 0.5906: green ×0.672, blue ×0.502, then white balance. Board 1 warmth 0. The shoulder is inert under v9 gains (top 0.396/0.592 < 0.75).
- **info_loss**: DESTROYS authored hue, e.g. yellow -> (1, 0.63, 0). PAL metrics tap the renderer output (TOLERANCE-FROZEN-P6-PAL.json 'lit_pixel: renderer output'), so they cannot see this.
- **origin**: inherited donor (Emotiscope) display controls
- **still_needed**: white balance may be needed (WS2816 'RGB colour balance 3:6:1', K1 S2-chip-truth.md:71-72); warmth is aesthetic
- **options**: Score palette fidelity after treatment; make warmth a palette-space operation; measure the white-balance need optically.
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: FROZEN: post-treatment fidelity (PAL-HD-v2 method on wire bytes ÷ gains) <= 0.001 off-palette at warmth 0. PLANTED: warmth 0.59 must be red.

### D02-L15 — Code-to-light transfer: linear scale16 at hd_pixel16.h:28-32 and palette_runtime.cpp:720-737; WS2816C built-in gamma (K1 docs/forensics/ws2816-degamma-audit-2026-08-18/S2-chip-truth.md:53-56)
- **container**: output / silicon
- **controls**: brightness transfer of fades, amplitude ratios and colour balance
- **current**: No degamma on the Titan; codes are treated as linear light. Datasheet: '16bit gray scale, 4bit gamma correction inside … 20bit display effect', no enable bit. No exponent is published; the bit-budget bound is gamma <= 1.25.
- **info_loss**: unknown. If gamma > 1, a fade e^-t/tau becomes e^-gamma*t/tau in light, and board gain 0.25 gives 0.25^gamma.
- **origin**: unmeasured
- **still_needed**: measure
- **options**: One optical ramp measurement (camera or photometer, instrument), then define each VP stage in a stated domain. S2 warns a 2.2 inverse over-corrects mid-tones by 35-46%.
- **impact**: 3
- **novelty**: 3
- **effort**: 3
- **experiment**: (Needs light capture; outside this swarm.) FROZEN: fitted exponent 95% CI width <= 0.2 from a 32-step ramp. PLANTED: a synthetic linear ramp must fit gamma = 1.0 ± 0.05.

### D02-L16 — Pack and taps: palette_runtime.cpp:687-741; fixture_app.cpp:640-720 (wire snapshot: 8-bit preview at bytes 64..543, from >>8 at palette_runtime.cpp:297; 16-bit packed lanes at 544+ and 1024+)
- **container**: output / instrument
- **controls**: brightness (<= 1 LSB) and measurement fidelity
- **current**: In-house output is TRUE16 end to end (wide_ is written by the renderer, :273). output_precision_ affects donor modes only (:411-420).
- **info_loss**: KEEPS on the wire; DESTROYS 8 bits in any tool that scores the preview bytes
- **origin**: diagnostic preview inherited from the donor
- **still_needed**: yes, as a preview
- **options**: Require scoring tools to read the packed 48-bit lanes and divide by board/lane gain to recover renderer values.
- **impact**: 2
- **novelty**: 2
- **effort**: 1
- **experiment**: FROZEN: the tool reconstructs wide_ from the packed lanes within 1 LSB on a host replay. PLANTED: reading the preview bytes must fail the 1 LSB check.

### D02-L17 — Wire cadence: ws281x_diag.h:41-43 (profile 3: T0H 250 ns, T1H 650 ns, bit 1250 ns, reset 300 µs); fixture_app.cpp:640-713 (pair engine for board 0, top engine for board 1, same tick)
- **container**: output
- **controls**: timing
- **current**: 5.1 ms of wire per 10 ms frame; DINs run in lockstep (BUDGET.md stage e)
- **info_loss**: KEEPS
- **origin**: WS2816 48-bit protocol
- **still_needed**: yes
- **options**: Up to about 196 Hz; phase-offset the two boards to halve sampling jitter between plates. The DMA0/3 and P601/P604 ownership rules are binding.
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: On-device existing counters. FROZEN: skipped_releases = 0 and no lane_fault at the candidate rate. PLANTED: a 4 ms period must produce BUSY or skips (red).

### D02-L18 — Silence and admission: palette_runtime.cpp:258-264; composed_scene_runtime.h:24-37 (peak^2 opens > 0.0048 = -23.2 dBFS, closes <= 0.0034 = -24.7 dBFS, no dwell); inhouse_waveform.cpp:122-141 (fresh <= 30 ms), :801 (silent +10/s)
- **container**: map / system
- **controls**: brightness at cuts and at low volume
- **current**: Peak hysteresis with no dwell, plus AP RMS silence (enter 0.04, exit 0.08, dwell 5 s; inhouse_audio.h:34-40)
- **info_loss**: DESTROYS quiet music below about -24 dBFS peak (never lights the plate)
- **origin**: Titan measured-room admission; the Captain judged the fast fade on cuts an improvement
- **still_needed**: yes for cuts; a risk at low listening volume (always-visualising mandate)
- **options**: A threshold relative to the tracked room noise floor, keeping the fast fade on cuts.
- **impact**: 3
- **novelty**: 2
- **effort**: 2
- **experiment**: Corpus host replay at -12 and -18 dB. FROZEN: >= 95% of music frames lit, and each annotated cut still reaches black within 150 ms. PLANTED: admission at -10 dBFS must be red.

### D02-L19 — Renderer selection: inhouse_waveform.h:16-75 (all v9 knobs compile-time), :172-209 (host-only setters); scripts/live_boot_composition.py:14 (profile 9 = -DK1_INHOUSE_MAPPING_V9)
- **container**: system
- **controls**: all v9 behaviour, and how fast it can be iterated
- **current**: The working tree still has kInhouseV9Palette = kRestrictedRange (:46), so the product would run the v9 colour layer that FORENSICS showed moves in 1% of seconds vs 95% for v8. Accent and release are compiled off.
- **info_loss**: n/a (a delivery failure class: PL14-B14)
- **origin**: P6 lane process
- **still_needed**: n/a
- **options**: Move the v9 knobs into the existing per-board live schema (owner-only, hash-bound defaults) so A/Bs need no reflash; keep the product-equivalence pytest as the gate.
- **impact**: 3
- **novelty**: 3
- **effort**: 3
- **experiment**: FROZEN: the existing PL14-B14 equivalence pytest (product build frames bit-identical to the scored host config). PLANTED: restoring kRestrictedRange must turn it red.

### D02-L20 — Board 1 (mode 18) transport: inhouse_waveform.cpp:979-999
- **container**: VP
- **controls**: motion (beat pulse in scroll speed) and head stepping
- **current**: Tempo speed 24*(bpm/60)*(1+0.5cos 2*pi*phase) px/s with weight <= 0.7 (zero below confidence 0.3); energy speed 30+90*(novelty/0.12); BPM window 60-156; head position std::floor to whole pixels (:996)
- **info_loss**: DESTROYS sub-pixel head motion; the P7 half-time tempo lock halves the pulse rate
- **origin**: inherited K1 tempo waveform plus P6 blend
- **still_needed**: the pulse: taste; the whole-pixel floor: no
- **options**: Sub-pixel head; a pulse from corrected tempo; contraction ahead of a predicted downbeat, only after the tempo is fixed.
- **impact**: 3
- **novelty**: 3
- **effort**: 2
- **experiment**: FROZEN: fraction of frames with an exact-integer head position <= 10%. PLANTED: the current floor must be red (100%).

### D02-L21 — Drop-release driver: inhouse_waveform.cpp:728-796 (accent and release), inhouse_waveform.h:52-75 (compiled off); sub_rms 40-110 Hz (inhouse_audio.cpp:239-241)
- **container**: map
- **controls**: motion and brightness at the drop
- **current**: Off in the product. The best kick candidate K3 releases at BOOM and Sonic drops (T90 0.09-0.10 s) but misses ±50 ms (STATUS.md:177). Surge drivers fire on climaxes and risers (STATUS.md:174).
- **info_loss**: the drop event never reaches the light
- **origin**: P6 research in progress; input limited by the 110 Hz lowest bin (AP)
- **still_needed**: yes
- **options**: One trigger driving three outputs together: extent reset (L08), thrust (L07) and headroom brightness (L10). Today the release only moves position.
- **impact**: 5
- **novelty**: 4
- **effort**: 3
- **experiment**: FROZEN: EXT-v2 plus the L07 thrust clause. PLANTED: the existing 200 ms-late and fire-every-hop faults (P6-DROP-DETECTOR/DETECTOR.md:21).

### D02-L22 — Cross-channel latency: level inhouse_waveform.cpp:942-949 (raw hop RMS); position inhouse_audio.cpp:253 plus inhouse_waveform.cpp:974-975 (5.4 + 16 + 23 ms); colour L02 (21 + 80 + 7 ms)
- **container**: map
- **controls**: timing coherence of one musical event across channels
- **current**: Brightness about 0 ms, position about 44 ms, colour about 108 ms (sums of first-order time constants; [INFERENCE] mean group delay)
- **info_loss**: DESTROYS onset coherence: a drop's colour lands about 0.1 s after its light
- **origin**: inherited ballistics (0x1313 tau 0.016/0.023; K1 mood LPF)
- **still_needed**: taste (align vs a deliberate afterglow)
- **options**: Shorter position EMAs; colour from the fast spectrum at onsets; or a deliberate stagger.
- **impact**: 3
- **novelty**: 3
- **effort**: 1
- **experiment**: Host step and impulse probe. FROZEN (aligned candidate): per-channel 63% delays within 20 ms of each other. PLANTED: an added 50 ms delay on one channel must be red.

### D02-L23 — Architecture: renderV9 transport and deposit (inhouse_waveform.cpp:963-1023) as a 160-entry shift register of (intensity, colour, offset) shifted by whole pixels
- **container**: VP / system
- **controls**: all motion, plus colour per deposit
- **current**: Frame-buffer scroll
- **info_loss**: DESTROYS through coupling: the shift register ties together stride, beading, the thrust ceiling, whole-pixel motion and colour = age (L04, L06, L07, L20)
- **origin**: [INFERENCE] S3 budget and memory: K1 even reused the previous finalised frame as the trail ('No per-effect PSRAM buffer is needed', light_mode_waveform_hybrid_k1.cpp:26-30)
- **still_needed**: no (VP cost is under 1 ms; BUDGET.md)
- **options**: Deposits as objects (birth time, origin, velocity curve, level, palette descriptor) rendered analytically at sub-pixel accuracy each frame; thrust easing, per-deposit spectra and continuous trails come free. Keep v8 on the old path, bit-identical.
- **impact**: 5
- **novelty**: 4
- **effort**: 4
- **experiment**: FROZEN: configured identically (constant speed, integer positions), the particle renderer matches current v9 path-P frames within PAL-HD-v2 and C2 on all 10 inputs. PLANTED: a one-pixel transport offset must be red.

### D02-L24 — Dead controls on in-house modes: palette_runtime.cpp:236-251, :283-285; InhouseWaveformControls defaults at inhouse_waveform.h:115-126
- **container**: system / controls
- **controls**: none (that is the finding)
- **current**: [FACT] renderInhouse reads only current_mode, palette, speed, mirror_mode, brightness, warmth and response_gain. saturation (fixed 1.0), colour_offset (0.05) and chroma_range (60) are fixed. Donor fields softness, background, color, color_range, reverse_color_range and temporal_dithering have no effect on modes 32/18.
- **info_loss**: n/a
- **origin**: port boundary
- **still_needed**: n/a
- **options**: Map a few to v9 levers (a range control to the L04 ramp gain k, softness to the L11 tau). Never map background to a floor (true black).
- **impact**: 2
- **novelty**: 2
- **effort**: 2
- **experiment**: FROZEN: each exposed control changes the host-replay frame hash. PLANTED: an unwired control must be red.

### D02-L25 — Compute headroom: P5 BUDGET.md rows c-e; Kx-Titan PL14-B12
- **container**: system
- **controls**: what every other lever can afford
- **current**: VP render p50 0.197 ms / p99 0.277 ms plus pack 0.128 ms per 10 ms frame; AP about 1.0-1.29 ms per 5 ms hop
- **info_loss**: n/a (enabler)
- **origin**: S3 limits no longer apply (S3 8-bit at cap 24 gave about 9.5 levels vs 2,442 on the Titan)
- **still_needed**: n/a
- **options**: Spend VP time on L03, L06 and L23. Ethos-U55 and M33 are unexplored for the VP [HYPOTHESIS: not needed].
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: On device. FROZEN: vp_cycles p99 < 2 ms after adopting L03 + L06. PLANTED: a 9 ms busy-loop must produce skipped releases.

## Failures mapped

- K1 'over-averaging' palette (16-slot, uint8 index; K1 c2738b88): NOT on the main Titan colour path. sample() is an HD stop interpolation (inhouse_waveform.cpp:41-71). The 16-slot compiled_fastled16 is used only in near-black fallbacks (v8 mode 32 :220-232; family modes :393-404). Levers L05, L03.
- v9 colour freeze (post-gate confidence × strength × level, 0.25 u/s cap, 15 s anchor; FORENSICS.md: 1% moving seconds vs v8 95%): stage updateColourV9 :659-723. Path P replaces it, but the product default is still kRestrictedRange. L19, L02.
- Path P loud-end stall (knee'd chroma collapses when loud): fixed by a pre-knee shape centroid (applyShapeCentroidP :596-625 and linear_shape inhouse_audio.cpp:347-350), which is UNCOMMITTED in the worktree. L03.
- PL14-B14: the product compile-time default did not select the fix (inhouse_waveform.h:46). L19.
- Drop does not hit harder than the build. Measured on the existing replay (M2): the head/extent stays at about 126-130 px across the drop because of AGC normalisation plus the clamp at 1 (palette_runtime.cpp:268-269), and polarity louder = further out; 35% of drop frames clip the -9 dBFS level ceiling; the scroll speed channel is unused and capped at 800 px/s; accent/EXT is compiled off. L07, L08, L09, L10, L21.
- Detectors blind below 110 Hz / sub-bass 70-120 ms late: AP boundary (gdft_goertzel.cpp:26, the first bin at 110 Hz); sub_rms exists only in v9. Compounded by VP channel lags (colour about 108 ms). L21, L22.
- Tempo half-time lock (P7): drives board 1's mode-18 pulse speed (:988-992) at half rate. L20.
- S3-era banding: an 8-bit wire at a 24/255 cap with a 0.396 deposit gives about 9.5 levels (3.2 bits), which explains the historical smoothing and single-colour compressions. Absent on the Titan (11.3 bits). L10, L25.
- K1 WS2816 'looked 8-bit' (REPLICATE8): not present on the Titan in-house path, which is TRUE16; only the 8-bit preview in the wire snapshot is a measurement trap. L16.
- Palette dark low end (it4 variants A lifted / B restricted fold): re-emerges under path P as near-black strip segments and a black/yellow seam on palette 31 (17.5% of positions). L05.
- Trail beading (not previously recorded): a 4.05 px stride vs a 3-10 px max-held triangle; inherited from 0x1313 at 100 Hz. L06.
- Mirror-off hazard (not previously recorded): mode 32 zeroes source 0..79 every frame, so mirror_mode false gives a dark left half. L13.

## Contradictions

- Render rate: the brief says '120 Hz logical render target'. Source: kPalettePeriodUs = 10000 under K1_EMOTISCOPE_PORT (palette_runtime.h:39-40), and P5 BUDGET.md reports render_hz 100 with 100.01 fps measured. 8333 µs (120 Hz) is only the non-Emotiscope branch (:47).
- Brightness: the brief and the CLAUDE.md Titan mandate give a 24/255 maximum. On the other side: platform/ra8p1/README.md:1-11 and Kx-Titan AGENTS.md:12 say default >= 100/255 'supersedes historical 24'; palette_runtime.h:23 static_asserts >= 100; live_boot_composition.py:74 requires 100..255; the v9 preset uses hardware 255 with board intensity 0.25. These are different layers (cap / default / board gain). The light actually emitted on the owner's unit cannot be derived from source. Not resolved.
- Pins: the brief says 'two boards × 160 px split 80/80 over P601/P604'. Source: board 0 uses the pair engine P601/GPT6 + P604/GPT8 (ws281x_gpt_dma_hw_pair.h:12-14). Board 1 uses a separate 'top' engine on P603/P714 (ws281x_gpt_dma_hw_top.h pfs_p603/pfs_p714; palette_runtime.cpp:725-733 swaps its lanes).
- The brief says the Titan fault at inhouse_waveform.cpp:209-231 'used the 16-slot palette'. In v8 mode 32 the main colour is sample(), an HD stop interpolation; compiled_fastled16 appears only in the near-black fallback. One sample per frame is the K1 waveform design (#54824), and FORENSICS.md says 'Titan v8 already follows the fixed K1 rule'. The real averaging there is the RGB blend of the chroma and fallback colours (:233-236) and the RGB EMA (:237-239).
- '9-16 palette positions vs 1' (path P): PAL b_spread counts recovered positions along the strip, and the deterministic d ramp alone produces them. Music-driven colour information is still one offset per frame. True, but it is evidence of a gradient, not of a per-bin mapping.
- Revision identity: the brief cites HEAD 81ac031. The worktree HEAD is 1ef5395, with UNCOMMITTED path-P edits in inhouse_waveform.{h,cpp} and inhouse_audio.{h,cpp}, so cited line numbers differ.
- palette_runtime.h:152-155 says '0 = reference RGB8 (boot)', but this applies to donor modes only. In-house modes 32/18 write TRUE16 directly (palette_runtime.cpp:273, 411-420); status reports source_precision per_renderer_rgb16.

## Open questions

- Which composition and brightness are installed on the owner's Titan right now (24/255 cap vs 100/255 vs 255 × 0.25)? This needs a read-only status query by the lane owner; none was made here.
- WS2816C transfer exponent and direction on this light-guide plate (S2 bound gamma <= 1.25; no datasheet exponent). It needs an instrument ramp capture, not the Captain's eyes.
- Does the light-guide plate's spatial blur hide the 4-px trail beading (L06)? Wire snapshot plus one camera frame of a static bead pattern.
- Was firmware-v3 0x1313's MAX_SCROLL_STEP 8 an S3 cost limit or an aesthetic choice? This needs the Lightwave source (another dimension).
- What was the K1 S3 LED_FPS for the waveform? If it was above 100 Hz, the Titan beads more than the K1 did.
- Will host-005 (path P as the compile-time default plus the PL14-B14 equivalence test) land before synthesis? STATUS.md 17:09Z says it was in progress.
- Taste decisions for the Captain, each needing the smallest on-device A/B: path-P ramp gain k (L04), symmetric vs asymmetric halves (L13), colour-lag alignment vs afterglow (L22), thrust profile (L07).
- The REPORT.md write was refused by the harness for subagents. Should the orchestrator write REPORT.md from this structured output so the integrator has a file?
