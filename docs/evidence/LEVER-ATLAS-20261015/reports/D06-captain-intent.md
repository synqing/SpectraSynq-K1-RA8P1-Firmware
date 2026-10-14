# D06-captain-intent — dimension report (written by the lane owner from the excavator's structured result)

The excavator's own Write was refused by the harness ("Subagents should return findings as text"); this file is its returned result, verbatim fields, unedited.

Lane: opus

## Summary

1. What he wants: a smooth, continuous field of rich, saturated palette colour that "understands music". It should be "fast, snappy, it captivates", "alive", "ORGANIC, FLOW, AESTHETICS and IMPACT". It should never twitch, step, strobe or switch on and off. It goes totally dark when the music cuts for effect.
2. His definition of "hits harder" (14 Oct, verbatim in transcript c8f0d616 at 14:32:18Z): the waveform shrinks toward the left and right edges through the build. At the drop it must shoot back to full length (edges toward the centre) with "an appropriate amount" of "explosive force", congruent with the audio. This is about extent and timing, not only brightness.
3. Why Titan cannot do this today [FACT code, INFERENCE cause]: in Waveform Hybrid K1, both v8 and v9 place each new deposit at 80 + level·0.7·80 (inhouse_waveform.cpp:268 and :976). Louder sound lands nearer the edge, and in v8 it also fades faster (:254). The drop is the loudest moment, so it produces the most contraction rather than a release. A fix has to change the position map and the decay at the drop event. Retuning gain will not do it.
4. Colour rules in his words: the palette must be the main colour source and a single colour is banned. Rainbows are banned. White may be about 5–10% at most, and full white never. He wants the whole palette range shown with nuance. He rated Hybrid K1 7.5–8/10, saying a 10 needs "more colour variation, more nuance of the colour palette".
5. Titan verdicts (14 Oct): v9 b5595c35 is "overall superior to v8" and "do not revert". The fade to black on silent cuts improved. The drop is "still a no", with only "SOME semblance of tension release".
6. I collected a verbatim quote ledger (May–Oct 2026), turned it into 15 testable requirements with candidate frozen criteria and planted faults, and wrote 16 lever records. I also found 11 contradictions between sources and propose 9 small side-by-side clip comparisons for the questions only his eye can settle.
7. Sources: claude-mem is degraded and holds his prompts only up to 9 Oct. I read its database read-only. His 14 Oct words come from the session transcript; Hindsight worked.
8. REPORT.md was not written because the harness refused the report-file write; everything is in this result. Nothing was built, flashed, committed or played. Images do not apply because this dimension produces text only.

## Levers

### L06-01 — VP: deposit position. v8 src/k1/core/visual/inhouse_waveform.cpp:268 position=min(159,80+peak_stage2*0.7*80); v9 :976 (same) plus an experimental release_ pull toward 80 at :977 (working tree, not on silicon). The scroll runs 80->159 (:265, :1005) and the mirror copies 159-i (:290, :519).
- **container**: VP
- **controls**: Space and timing: the extent of the lit band and whether the drop releases. The lit band runs from the deposit point to the edge.
- **current**: v8 and v9: louder sound puts the deposit nearer the edge, so the band is shorter and hugs the edges [FACT]. On b5595c35 the waveform 'hugs the edges and the centre stays dark' (P6 RECEIPT.md:158).
- **info_loss**: Level is kept, but because louder means nearer the edge, the loudest instant (the drop) is the most contracted. Any monotone level map makes the drop look like 'more build' [INFERENCE].
- **origin**: Inherited aesthetic from the Sensory Bridge/K1 Waveform: 'loud pushes the trace far from centre, quiet hugs it' (SpectraSynq_K1_Firmware docs/architecture/waveform-mode-design-rationale.md:46-49). Not an S3 budget item. Captain verbatim, 14 Oct 2026 14:32Z: 'at the peak of the tension build up, the length of the waveform is basically compressed heavily towards the left and right edges, so naturally, when the drop hits, this tension MUST be released with "an appropriate amount" of "explosive force"/thrust from the origin to full extension (in this case, left/right edges towards the centre/middle) "Hits hard/harder" is when the visual impact of that release is congruent with the audio. SIMPLE.'
- **still_needed**: The sign is needed during the build, because he values the contraction. It must be overridden at the drop. M85 cost is negligible: one branch per frame.
- **options**: (a) Invert the map when a drop is detected: sweep the deposit point from the edge toward the centre over a release time T, then hand back to the normal map. (b) Add a tension state that stores how far the build contracted and releases it. (c) Make extent a separate channel from brightness. (d) [HYPOTHESIS] Sweeping the deposit point inward may read as 'edges toward the centre' better than the K3/X3 behaviour of lighting the centre first.
- **impact**: 5
- **novelty**: 3
- **effort**: 3
- **experiment**: R1 EXT-release + R2 build contraction, frozen together. Extent E(t) = innermost lit pixel's distance from the edge / 80, where lit means emitted Y >= 25% of the frame maximum, scored on the device LED payload. At each annotated drop t_d (true onsets at 10 ms resolution, not anchors quantised to 0.1 s), E must reach >= 0.9 with T90 <= TBD (candidate 150 ms), and the first rise must start in [t_d-20 ms, t_d+TBD (candidate 60 ms)]. Also: median E over the final 1.5 s before the cut <= 0.6 x median E over the first 25% of the build, and no single rise of 0.3 or more in E within 150 ms (a release) inside the build-climax window. Must be red: b5595c35 and v8 (he said 'still a no'), a planted release 250 ms late, a planted release fired at the riser, and the X1-X3 level-surge candidates (climax extent 0.47->0.69). Must be green: a planted instant release at t_d. Reuse TOLERANCE-FROZEN-P6-EXT-v2.json with onsets re-anchored. The bars come from the Captain's ladder (Q1).

### L06-02 — VP: trail decay. v8 inhouse_waveform.cpp:254 decay=0.8+3.5*peak+10*(1-quiet); v9 :801 constant 1/0.29 s (mode 32) or 1/0.125 s (mode 18), plus 10/s when silent.
- **container**: VP
- **controls**: Persistence and extent length (the 'alive' breathing), and how long the drop stays extended.
- **current**: v8: louder means a shorter trail. v9: constant release (P6 constraint a).
- **info_loss**: v8 ties loudness to a shorter extent, the same direction as L06-01, so the contraction is doubled at the drop [INFERENCE].
- **origin**: K1 doctrine: 'loud passages fade faster (punchy, snappy) ... short and aggressive in a drop' (waveform-mode-design-rationale.md:59-66). This is aesthetic, and it conflicts with the Captain's 14 Oct drop definition (contradiction X3).
- **still_needed**: Yes for everyday breathing. No at the drop. Cost negligible.
- **options**: Keep v9's constant release, and add a slow release gated by the drop so that the extended trail persists for 300 ms or more. Alternatively, invert the loudness-to-decay relation at the drop only.
- **impact**: 4
- **novelty**: 2
- **effort**: 2
- **experiment**: Extent sustain: E >= 0.7 for >= 300 ms after t_d (TBD from his ladder). The planted v8 decay law must be red; a planted held release must be green. Run it together with R6 (no stepping) so that the release does not become a hard cut.

### L06-03 — AP: drop-event feature. The lowest analysis bin is 110 Hz. There is an experimental 40-110 Hz detector in src/k1/core/audio/sub_band.h (v9-only, uncommitted).
- **container**: AP
- **controls**: Timing: when the release fires.
- **current**: The analysis is blind below 110 Hz, and sub-bass arrives 70-120 ms after the musical drop. Three detectors failed (runs/STATUS.md 15:48, 16:13, 16:50). The product tempo tracker locks to half time.
- **info_loss**: Energy below 110 Hz is lost, and that is where the drop is: +17 dB on BOOM (P6 RECEIPT.md:167) [FACT].
- **origin**: Inherited GDFT bin layout (S3 budget) [INFERENCE]. Captain's requirement: 'congruent with the audio'. He gave no millisecond figure.
- **still_needed**: No. The M85 can afford bins below 110 Hz. The cost is not measured here.
- **options**: Use a broadband transient or flux edge as the trigger and sub-bass as confirmation; extend the bins below 110 Hz. Do not substitute predicted beats for reactive timing, which the canon prohibits (TITAN-CANON-2026-10-13.md:52).
- **impact**: 5
- **novelty**: 3
- **effort**: 3
- **experiment**: R1 timing window on >= 6 annotated drops, including a held-out track from his corpus (P7 CORPUS-PROPOSAL.md needs 7 tracks from him). Planted: a detector delayed by 100 ms must be red, and a riser-only clip must not fire.

### L06-04 — New state (not built), sitting between the AP features and the VP extent: a tension integrator that tells the build climax and the riser apart from the drop.
- **container**: map
- **controls**: Whether a release is armed. Prevents false releases at the climax or riser.
- **current**: Absent. Candidates X1-X3 released on the build's own loudness surge (climax extent 0.47->0.69) and on the riser 0.19 s early (P6 RECEIPT.md:164-166).
- **info_loss**: The build's trajectory is thrown away every frame [INFERENCE].
- **origin**: Never built (unknown). Captain verbatim: 'During the build up phase, you can see the waveform length slow and surely contracting from it's state of full extension (edges to middle)'.
- **still_needed**: Yes. It is the missing piece for 'hits harder'. Cost is a few floats per frame.
- **options**: Integrate the contraction depth (1-E) over the build and arm on sustained contraction plus rising density. Release on cut->re-entry or on a sub-bass onset. Songs with no build must still visualise music continuously (the Titan mandate).
- **impact**: 5
- **novelty**: 4
- **effort**: 3
- **experiment**: R2 (no release in the climax) and R1 (release at the drop) on BOOM, Sonic@27.7 and Sonic@54.7, plus a sealed holdout. Planted: arming that is always on must fail R2.

### L06-05 — VP: level transfer and deposit gain. v9 inhouse_waveform.cpp:949 l=clamp01((20log10 vu+33)/(ceiling+33)); gains in inhouse_waveform.h:35-36 (0.395878 / 0.591745).
- **container**: map
- **controls**: Brightness and contrast at the drop.
- **current**: l reaches 1 at -9 dBFS. 33.6% of BOOM drop frames sit at that ceiling, and the deposit uses about 40% of the renderer's range (REVIEW-ITERATION-1 F3).
- **info_loss**: Loudness above -9 dBFS is lost [FACT].
- **origin**: Matched-average-light calibration (P6 constraint b), chosen by the agent.
- **still_needed**: Unknown. Taste decides.
- **options**: Raise the ceiling (-3 dBFS was tried; DI stayed red), or reserve headroom that only a drop event can use.
- **impact**: 3
- **novelty**: 1
- **effort**: 1
- **experiment**: R3: DI-Impact I = p95 L* over [t_d, t_d+1 s] divided by p95 L* over the build climax, scored on the payload. The bar comes from his ladder (x1.0/1.25/1.5/2/3). b5595c35 is the known negative. A planted flat deposit must be red; a planted x2 accent must be green.

### L06-06 — VP output: per-pixel HD palette sample (path P). inhouse_waveform.cpp:879-897 sampleProductPaletteHd. The selector inhouse_waveform.h:47 is now kPerPixelMapped in the working tree; the build on silicon (b5595c35) is variant B.
- **container**: VP
- **controls**: Colour nuance across the strip.
- **current**: The old path mixed the RGB of two FastLED 16-slot samples (inhouse_waveform.cpp:216-231). Path P passes on the host (host-004 49308b9a, STATUS 17:09).
- **info_loss**: The old path collapsed each frame to one averaged colour. Path P shows 9-16 palette positions against 1 (brief).
- **origin**: FastLED 16-slot palette (S3 budget plus inheritance). Captain verbatim (14 Oct): 'The palette was not being "accurately mapped" to each of the 80bins, as a result of "over-averaging"'. Also (11 Jul): 'For a full 10, needs more colour variation, more nuance of the colour palette to be displayed.'
- **still_needed**: The 16-slot copy is no longer needed: the HD sample on the M85 is cheap, and v9 render cost fell 7%.
- **options**: Ship path P with a test proving the product's compile-time default selects it (lesson PL14-B14).
- **impact**: 5
- **novelty**: 2
- **effort**: 1
- **experiment**: R7: off-palette lit pixels <= 0.1% and per-pixel error <= 0.004 (TOLERANCE-FROZEN-P6-PAL-HD-v2.json). R8: >= 4 distinct palette positions (>= 1/32 apart) in >= 90% of lit frames. The slot16 and rgb_mix plants must be red. A product-path bit-equivalence check must also be red on host-004 (variant B default).

### L06-07 — VP colour layer, v9 inhouse_waveform.cpp:657-720: follow at speed x confidence / 0.6 s, momentum 0.25 s, cap 0.25 palette/s, drift 0.03 palette/s, luma hold :685-715.
- **container**: VP
- **controls**: How lively the colour is over time.
- **current**: b5595c35 colour moves in 1% of seconds, against 95% for v8 (STATUS 14:46). The ruling 'every frame (K1 rule) vs slow but never frozen' has been pending since 14 Oct.
- **info_loss**: Harmonic change is suppressed when confidence is low; the colour freezes.
- **origin**: P6 constraint (c), proposed by the agent. The Hybrid's 'heavy colour EMA' (claude-mem #79149). Captain verbatim (17 Aug): 'waveform-fast and waveform-tempo are EXTREMELY lively in its variation and rendering of the entire range within each palette whereas waveform hybrid k1 or waveform seem more reluctant.'
- **still_needed**: Taste decides (Q3). Cost negligible.
- **options**: Adopt the K1 per-frame rule (ae5d90a). Or keep the colour slow but never frozen. Or rely on per-pixel deposited positions (path P carries a position per pixel).
- **impact**: 4
- **novelty**: 2
- **effort**: 1
- **experiment**: R9: circular range of the palette coordinate >= 1/32 in >= 90% of music seconds. A frozen-colour plant must be red. Pair it with R6 so jitter ('flips on a dime') is also red. The upper rate bound comes from his A/B.

### L06-08 — Map: harmony (chroma centroid) to palette coordinate, against palettes with dark ends: 32 starts at black, 39 is black at both ends (P6 RECEIPT.md:688).
- **container**: map
- **controls**: Perceived brightness of the drop.
- **current**: Before the BOOM drop the colour moves gold->red and the lower board's luma is x0.63 (MUSIC-VISUAL-VALUE-TAKEOVER-2026-10-08.md:1873). The luma-hold candidate scores at best DI 0.904, still red.
- **info_loss**: Harmony is kept, but it cancels the brightness rise at the drop [FACT].
- **origin**: P3 finding: 'dim drop = effect mapping (colour from harmony switches gold->red)' (STATUS 09:28).
- **still_needed**: Taste decides. Must not tint (the palette must stay the colour source).
- **options**: Luma-compensated position, palette re-authoring, or a luma hold only for non-confident moves.
- **impact**: 3
- **novelty**: 2
- **effort**: 2
- **experiment**: R3 scored on luma or L*, not channel sum. The C1 lesson applies: channel sum hid a darker drop. A planted dark-end jump at t_d must be red.

### L06-09 — VP silence and cut gate: drain +10/s (:254, :801), silence_scale_ and hold_ (:201, :251-253).
- **container**: VP
- **controls**: Dark on deliberate cuts, and a snappy re-entry.
- **current**: Improved in v9: 'There's definitely a noticible improvement it the fade to back on silent drop cut passages'. Some 'lingering' remains, and his view of it is ambiguous.
- **info_loss**: None: this lever adds information.
- **origin**: Sensory Bridge silence design; the K1 go-dark redesign (claude-mem #78222). Captain verbatim (11 Jun 2026): 'it's critical that the effects ACTUALLY TOTALLY CUT and go dark to match that phase of the song for impact - failure to actually cut signals poor control'.
- **still_needed**: Yes.
- **options**: Add a cut detector (envelope falls >= X dB within Y ms) that triggers a fast drain only on cuts. Re-entry is instant.
- **impact**: 4
- **novelty**: 2
- **effort**: 2
- **experiment**: R4: for cuts of >= 300 ms, emitted mean Y <= 2% of the pre-cut p95 within 200 ms (TBD), and every channel 0 within the 5 s dwell (C3b). R5: first visible change within one render frame of the published re-entry. A planted brightness floor and a planted 1 s hold must be red. Rule M20: weak attacks in quiet music must not be cut.

### L06-10 — VP intensity field: y = max(x, y*exp(-dt/tau)), instant attack (P6 RECEIPT.md:767-770).
- **container**: VP
- **controls**: Smoothness: no on/off and no stepping.
- **current**: Instant attack with a constant exponential release.
- **info_loss**: None.
- **origin**: Captain verbatim (11 Jul): 'Professionally rendered effects have "easing" built in, not insane amounts of easing but focus needs to be given to how they "decay"/fade out - NEVER a hard cut or an immediate drop out.' And (18 Jul): 'steppy brightness MUST NEVER EVER FUCKING HAPPEN EVER'.
- **still_needed**: Yes.
- **options**: Ease the attack over 1-2 frames only on very large jumps. Keep transients instant so the snappiness oracle is preserved.
- **impact**: 3
- **novelty**: 1
- **effort**: 1
- **experiment**: R6: outside annotated cuts, the share of frames where a board's mean L* falls > 40% in one frame while the input falls < 6 dB must be <= 0.5%, and luma flicker power at 8-25 Hz must be <= the input envelope's. A planted binary deposit gate and a planted 8 Hz square wave must be red.

### L06-11 — Palettes (product_palette_data.cpp), additive deposit, display stage and gamma (platform/ra8p1/palette_runtime.cpp:281-294).
- **container**: output
- **controls**: Vibrancy and saturation; the white budget.
- **current**: Not measured on the Titan payload; no LED payload capture exists (F4).
- **info_loss**: Unknown until measured.
- **origin**: Captain verbatim (17 Aug): 'SOME (extremely minimal) white is fine, maybe 5-10% AT MOST ... The objective is to render RICHNESS, RICH VIBRANT, SATURATED COLOURS'. 8 Oct: 'IF YOU EVER CAUSER MY TITAN TO DISPLAY FULL WHITE LEDS I WILL FUCKING KILL YOU'. 24 Jun: 'I FUCKING DESPISE rainbows'.
- **still_needed**: Yes.
- **options**: A clamp that preserves saturation instead of clipping each channel; use the WS2816 16-bit headroom.
- **impact**: 4
- **novelty**: 2
- **effort**: 2
- **experiment**: R11: lit pixel energy with HSV saturation < 0.25 must be <= 10% per song, and full-scale white pixels must be 0. Planted additive overflow and a planted white stop must be red.

### L06-12 — Output mapping: DIN halves (palette_runtime.cpp / fixture_app.cpp, commit 99c0cf44) and the renderer mirror.
- **container**: output
- **controls**: Symmetry and direction.
- **current**: The top halves were swapped until the 13 Oct fix, and a scene-override defect was then repaired. Both are agent claims; the Captain has not accepted them (Hindsight).
- **info_loss**: A swap destroys the direction meaning.
- **origin**: Captain verbatim (19 Jul): 'all motion propagation either originate from the middle of the strip and move outwards towards the edges, or they begin at the edges and move towards the centre. These are the ONLY 2 permitted motion directions'. 14 Aug: 'the led 80 and led 81 MUST FUCKING HAVE THE SAME FUCKING COLOUR'.
- **still_needed**: Yes.
- **options**: Add a payload byte test for symmetry and direction.
- **impact**: 4
- **novelty**: 1
- **effort**: 1
- **experiment**: R12: in mirror mode, logical pixels i and 159-i must be byte-identical every frame, with velocity outward or inward only. A planted half swap and a planted one-sided render must be red.

### L06-13 — System: two-board composition. Lower Hybrid K1 32/32, upper Waveform Tempo 18 with Crimson Gold 39, gains 0.25/0.25 (TITAN-CANON-2026-10-13.md:63).
- **container**: system
- **controls**: How the light guide blends the two edges. Possible split roles at the drop.
- **current**: Two independent renderers sharing one audio observation.
- **info_loss**: Unknown.
- **origin**: Captain verbatim (8 Oct): 'make the RGBIC leds dance with captivation motion while producing the most visually stimulating palette of colours and synergistically mix between the top and bottom channels within the LGFS.'
- **still_needed**: Yes. The form is taste (Q7).
- **options**: Both boards release together; or complementary roles (one releases, one shifts colour); or a timed hand-off between edges.
- **impact**: 4
- **novelty**: 4
- **effort**: 3
- **experiment**: On-device A/B Q7 with two clips. Inter-board payload correlation is report-only until his ruling.

### L06-14 — Output brightness ceiling: master and board gains.
- **container**: output
- **controls**: Absolute headroom for impact.
- **current**: Sources conflict: 24/255 (global Titan mandate and this brief); 100 with gains .25/.25 (Hindsight captain-standing-rules, 11/14 Oct); master 255 with gains 0.25 (TITAN-CANON §5); hardware_output_brightness 255 in the presets (REVIEW-ITERATION-1 F13).
- **info_loss**: A cap limits the drop's peak-to-build ratio.
- **origin**: Owner rule (conflicting records).
- **still_needed**: Owner decides. Do not change it.
- **options**: None by agents. He must make one statement.
- **impact**: 3
- **novelty**: 1
- **effort**: 1
- **experiment**: None. This needs a ruling (Q5).

### L06-15 — VP mode 18 motion: w*tempo + (1-w)*energy, w = 0.7*smoothstep((conf-0.30)/0.30) (P6 RECEIPT.md:775-779; inhouse_waveform.cpp:988).
- **container**: map
- **controls**: Motion speed, modulation and variation on the upper board ('galloping').
- **current**: w sits at the 0.7 cap in 53% of BOOM frames while the tracker reads 93 BPM against roughly 138 (REVIEW-ITERATION-1 F8).
- **info_loss**: A confident wrong lock drives the motion.
- **origin**: P6 constraint (d). Captain verbatim (11 Jul): 'lacks variation in motion ... no modulation, no novelty, no variation'. 27 Jun: 'a sense of "spirited galloping" ... appears to be completely/totally abscent'.
- **still_needed**: Yes, after P7.
- **options**: Fix P7 first, then let confidence detect a wrong lock. Use energy and onset density to modulate speed.
- **impact**: 3
- **novelty**: 2
- **effort**: 2
- **experiment**: R13: per song, the gap between the p10 and p90 of extent E must be >= 0.6, and no single periodic line may carry > 50% of the deposit-speed power. A planted constant-extent render and a planted fixed-period pulse must be red. This is an [INFERENCE] proxy that needs his A/B.

### L06-16 — System: the evaluation instrument (frozen metrics C1, DI, EXT, PAL and the Captain ladder).
- **container**: system
- **controls**: Whether the team optimises what he actually sees.
- **current**: C1 passed (x1.25-1.73) while his eye said no (F1/F2). No device payload has been captured (F4). He could not use the graph-heavy page.
- **info_loss**: Channel-sum medians hid a darker drop.
- **origin**: Captain verbatim (14 Oct 14:54Z): 'Right now, it's just a bunch of graphs and a FUCK TONNE of words.' Also 14:32Z: 'You need to show me visually.' Standing rule: the instrument, not the Captain's eyes.
- **still_needed**: Yes.
- **options**: Score on the LED payload. Calibrate the bars once on a synthetic ladder he judges. Present every question as side-by-side clips. Apply a known-negative rule: b5595c35 must fail any 'hits harder' metric before that metric is trusted.
- **impact**: 5
- **novelty**: 2
- **effort**: 2
- **experiment**: Before any design pass: b5595c35's BOOM payload must score below every drop bar, and a planted x2 accent at t_d must score above.

## Failures mapped

- Titan v8 and v9 b5595c35: the drop does not hit harder than the build (Captain 14 Oct 'still a no'). Cause [INFERENCE]: louder sound puts the deposit nearer the edge and shortens the trail (inhouse_waveform.cpp:268/:976/:254), so the drop is the point of greatest contraction. Requirements R1/R3.
- Titan donor path: every frame collapsed to one averaged colour, from a per-channel RGB mix of the 16-slot palette (inhouse_waveform.cpp:216-231). R7/R8.
- Titan v9 b5595c35: colour moves in 1% of seconds against 95% for v8 (STATUS 14:46). R9.
- Titan: the drop renders dimmer because harmony moves the colour gold->red into the palettes' dark ends (P3; MUSIC-VISUAL-VALUE-TAKEOVER-2026-10-08.md:1873). R3.
- Titan P6: the frozen C1 metric passed while the Captain's eye said no; it used channel-sum medians (REVIEW-ITERATION-1 F1/F2). Lever L06-16.
- Titan: a host fix was not selected by the product compile-time default (PL14-B14; inhouse_waveform.h:46-47). Delivery of R7/R8.
- Titan, 8 Oct: half the strip went full white. Captain: 'IF YOU EVER CAUSER MY TITAN TO DISPLAY FULL WHITE LEDS I WILL FUCKING KILL YOU'. R11.
- Titan, 13 Oct: the top DIN halves were swapped and a scene override changed the top effects; the Captain saw 'as if a rotational ... algorithm were applied' (Hindsight). R12.
- Titan: three drop detectors were late or silent. The analysis is blind below 110 Hz and the tracker locks to half time (STATUS 15:48/16:13/16:50). R1 timing.
- S3/K1: washed-out white palettes (prompts 58401, 113956, 119141, 119583). R11.
- S3/K1: rainbow used despite the ban (prompts 115139, 119307).
- S3/Lightwave ports: twitching, strobing, on/off stutter and steppy brightness (prompts 116465, 116478, 116504, 116827, 116832). R6.
- S3: Waveform lit only about 40 of 160 LEDs and lacked 'spirited galloping' (prompts 115310, 115329). R13.
- S3: tempo/onset output 'basically corrupted'; the effect was sound-reactive but not music-reactive (prompt 69611). R14.
- S3: the main unit was 'dull ... not as snappy' as the bench because of noise calibration (prompt 119683; claude-mem #74080). R5.
- S3: changes were never flashed, giving 'same ol boring performance' (prompt 113867; #63250). Delivery.
- Emotiscope on K1-S3: 'not behaving like a music visualiser anymore' after changes (prompt 122384).

## Contradictions

- X1 Brightness ceiling. Four sources disagree: (1) 24/255 in the global Titan mandate and the swarm brief. (2) Hindsight captain-standing-rules: 'Hardware brightness 100 and board gain .25/.25 (2026-10-11; clarified 2026-10-14) ... overrides older 24'. (3) TITAN-CANON-2026-10-13.md:63: boot composition 'master 255, gains 0.25'. (4) The P6 presets show hardware_output_brightness 255 (REVIEW-ITERATION-1.md F13). Left unresolved as the brief instructs.
- X2 'NEVER a hard cut or an immediate drop out' (11 Jul, prompt 116465) vs 'it's critical that the effects ACTUALLY TOTALLY CUT and go dark' (11 Jun, prompt 113960). [INFERENCE] These reconcile: there should be no on/off response to triggers while music plays, and total dark when the music itself cuts. R4 and R6 encode both.
- X3 The K1 Waveform doctrine says 'loud passages fade faster ... short and aggressive in a drop' (waveform-mode-design-rationale.md:59-66), and the Titan map sends loud deposits toward the edge. His 14 Oct definition requires the drop to reach full extension. His newer, drop-specific words should govern the drop.
- X4 Origin. K1 canon 'draws the trace from the heart outward' vs his 14 Oct 'from the origin ... (left/right edges towards the centre)'. His 19 Jul rule allows both directions. Whether the K3/X3 light, born at the centre and streaming outward, reads as his edges-to-centre thrust is open (P6 RECEIPT.md:214).
- X5 The slow colour layer (P6 constraint c; Hybrid 'heavy colour EMA', claude-mem #79149) conflicts with his 'more colour variation', with 'EXTREMELY lively' (17 Aug) and with the K1 per-frame rule (ae5d90a). Yet he preferred v9, whose colour moved in only 1% of seconds. [INFERENCE] The v9 preference was driven by intensity and fade, not colour.
- X6 Variant B narrows palettes to their bright sub-ranges, in effect the 'brighter palette' that constraint (e) bars (REVIEW-ITERATION-1 F6). It is nonetheless the build kept on the device, and he said 'I have no fucking idea what you're trying to say' about it. It is admissible for testing only.
- X7 The host C1 passed at x1.25-1.73 light against his 'still a no'. The metric was withdrawn as a proxy; on luma only 2 of 4 cases pass.
- X8 White at '5-10% AT MOST' (17 Aug) vs his approved promo-clip copy 'release in clean warm-white blooms' (29 Jul, prompt 118072). Low weight: the copy describes a rendered marketing clip, not firmware.
- X9 The ban on on/off response vs his clip taxonomy 'STACCATO: Sharp, discontinuous bursts with instant attack and no smooth fade' (prompt 118072). Low weight: a promo clip category.
- X10 The +/-50 ms 'congruence' bar used by the P6 detectors was chosen by the agent. The Captain gave no number; the canon cites ITU-R BT.1359 detectability at about +45 ms of light lag (TITAN-CANON-2026-10-13.md:30). The bar should be calibrated on his ladder.
- X11 His recollection of 'over-averaging ... each of the 80 bins ... fixed in SpectraSynq_K1_Firmware' has 0 claude-mem hits in any year (palette-forensics-2/mem-2025.md:27). The mechanism is confirmed in source: K1 c2738b88 HD per-bin sampling, ae5d90a chroma that never freezes, and Lightwave Feb 2026 per-pixel output colour. His wording is memory, not record.

## Open questions

- Q1 How much force is 'appropriate'? Show a synthetic drop ladder on the device: release T90 30/60/120/250 ms crossed with peak x1/1.5/2/3 on one build->drop. His smallest 'harder' becomes the frozen bar for R1 and R3.
- Q2 Does the release read right? Compare (a) light born at the centre and streaming outward (K3/X3 behaviour) with (b) the deposit point sweeping from the edge toward the centre. These are two side-by-side clips; freeze both definitions first.
- Q3 Colour speed: the K1 per-frame colour rule vs slow-but-never-frozen, both on path P. This ruling has been pending since STATUS 14:46 on 14 Oct.
- Q4 Is the 'lingering' he saw in the quiet cut liked, disliked or neutral? Show a 0.2 s drain beside a 1 s drain on the BOOM cut.
- Q5 Which brightness ceiling binds on this Titan unit: 24, 100 or 255 out of 255 (X1)? This needs a ruling, not an A/B.
- Q6 Variant B fold vs path P as the product colour: show the same 20 s side by side. This is taste.
- Q7 Two-board drop: both boards release together, or the lower board releases while the upper shifts colour ('synergistically mix between the top and bottom channels').
- Q8 Corpus: he needs to name the 7 missing tracks from P7 CORPUS-PROPOSAL.md so that a sealed holdout drop exists. The Carte Blanche holdout is spent.
- Q9 White budget: authored palette vs a saturation-preserving clamp. Also: is there a reference clip of 'spirited galloping'? And can the S3 Waveform-Fast/Tempo render-stage payload (usbmodem11301, per Hindsight 13 Oct) serve as the colour-liveliness oracle for R9 and R10?
- Process: claude-mem is degraded (prompts stored only to 2026-10-09; liveness watchdog stale). Captain statements from 10–15 Oct exist only in session transcripts and Hindsight. Should they be reconciled into durable memory?
