# Titan: why the DBS pilot did not produce a better show

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

Investigation dated 15 October 2026, Australia/Perth. This is a source and existing-evidence audit, with independent numerical reproductions. No firmware, controls, playback, camera session or physical device was changed by this investigation. The active P6 owner retains the device and their blind comparison.

## Finding

The evidence identifies a real loss of amplitude information in the current renderer, a proposed intervention that cannot repair that mapping, and measurement defects that make some of the rejection claims unreliable. The pilot has not demonstrated a better lightshow. It also has not demonstrated that Titan cannot produce one, that adaptive grouping is generally useless, or that every reported missing attack disappeared physically.

The recurring engineering failure is that changes to the reference behaviour, partial replay results and hardware health have been allowed to stand in for the complete musical performance. Earlier lane incidents independently document scoring one configuration and building another, and metrics that accepted the defect they were supposed to reject. Those incidents are documented history, not an assertion that their fixes are absent today.

My earlier translation contributed to the problem. It proposed a multiplier restricted to 0.75–1.0 while preserving the renderer's geometry and history. That can create contrast by reducing continuing light, but it cannot restore the level differences erased by the renderer's ceiling. Once the pilot located that boundary, the implementation objective needed to move to the composition repair. The adaptive-grouping hypothesis should no longer have driven the work.

## Source identity and limits

Product checkout: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`.

Inspected and rechecked HEAD: `1d0095f6662669e475ed43fb6ae3e770dcb97099`. Pilot receipt: commit `059a399`. The three existing tracked modifications belonged to the lane owner: `docs/emotiscope-port-001/CURRENT-STATE.md`, `docs/emotiscope-port-001/INHOUSE-WAVEFORM-MIGRATION-20261012.md`, and `docs/emotiscope-port-001/evidence/audio-windows-001/manifest.json`. Existing untracked work was also preserved. All relative product paths below resolve under this checkout.

Read evidence includes the receipt, both frozen criteria, reference qualification, original scores, rejected implementation patch, probe, preparation and scoring scripts, current renderer/runtime, source lineage, and current release-puppet evidence. I inspected the stage and payload figures. I did not independently observe the physical pilot, which never ran on Titan. Existing puppet timing is another run's reported measurement, not a new observation or a verdict on the owner's blind A/B.

## 1. The renderer makes different loud inputs identical at the deposit-amplitude boundary

`src/k1/core/visual/inhouse_waveform.h` defines a −9 dB level ceiling and mode-32 gain 0.580092. `renderV9()` maps the admitted VU value approximately as `level = clamp((20 log10(vu) + 33) / 24, 0, 1)`. `depositV9()` requests amplitude `0.580092 × level^1.5` for lower Hybrid.

| VU level at this renderer input | Normalised level | Requested deposit amplitude |
|---|---:|---:|
| −24 dB | 0.375 | 0.133212 |
| −18 dB | 0.625 | 0.286627 |
| −12 dB | 0.875 | 0.474798 |
| −9 dB | 1.000 | 0.580092 |
| −6 dB | 1.000 | 0.580092 |
| −3 dB | 1.000 | 0.580092 |

This is a flat control transfer, not evidence of processor overload or LED transport failure. The dB coordinate is the VU value consumed by the renderer; in the live path response gain is applied before it. Do not equate it unconditionally with raw microphone dBFS.

An independent calculation over 1,950 board-0 render frames in BOOM 55.5–75 s found level saturation in 31.74% of source frames and 15.49% of room-proxy frames. These use render-frame sampling, whereas the receipt's approximately 33%/16% came from its stage analysis. The nearby percentages support the same mechanism; they are not interchangeable denominators.

The stage evidence shows attack-related changes surviving in the analysis in inspected passages. That supports repairing the demonstrated downstream loss first. It does not establish that every musically important attack survives every upstream stage.

## 2. The proposed feature could only attenuate

The rejected integration uses a contrast multiplier bounded by 0.75 and 1.0. It never increases requested attack amplitude above baseline. For a flat baseline the greatest possible high/low amplitude ratio from this control alone is 1/0.75 = 1.333. Detector mistakes reduce wanted deposits too.

With identical deposition geometry and nonnegative advection/decay, reducing input amplitude cannot increase the scalar intensity history under the renderer's maximum-composition rule. Palette winners may change, so this statement is about scalar intensity, not a proof that every output RGB channel or perceived brightness decreases monotonically.

This design can be useful for contrast, but it does not restore the lost loudness slope. An adaptive frequency partition has no special ability to bypass that mathematical limitation. Its benefit would have to come from better timing of the dimming, and the cross-input results did not establish that benefit.

## 3. One clean pulse can become three scored attacks

`scripts/p6_dbs_score.py`, functions `births()` and `greedy()`, accept equal neighbouring maxima as separate births. There is no plateau consolidation. A 30 ms plateau at 100 Hz with amplitudes `[0.58, 0.58, 0.58]` above a 0.435 background produces births at 80, 90 and 100 ms. Against one onset at 80 ms, the exact functions match one birth and classify the other two as false.

This was executed using the scorer's actual function AST, not merely inferred from a description. The reproduction script accompanies this report.

I also independently reproduced the original BOOM counts before making a diagnostic comparison that collapses consecutive equal-amplitude plateau births into one centre event. No frozen result was altered or promoted.

| Oracle comparison | Original excess false births over baseline | Excess after diagnostic plateau consolidation |
|---|---:|---:|
| BOOM source | 261 − 236 = 25 | 228 − 222 = 6 |
| BOOM room proxy | 551 − 488 = 63 | 478 − 469 = 9 |

The duplicate-counting behaviour explains 54 of the 63 excess room-oracle counts under this diagnostic. Nine remain. Most real-arm results barely change. Therefore the finding invalidates a blanket interpretation of the oracle rejection; it does not turn the rejected candidates into passing candidates.

The receipt's suggestion that M3 is unattainable for any contrast design goes beyond the evidence. The implemented oracle failed this measurement. Neither that oracle nor this measurement establishes impossibility for all designs.

## 4. The reference is incomplete, and the event matching overstates certainty

The reference's own qualification records approximately 40–53% recall on planted weak BOOM attacks, particularly poor kick recall, and false activity on sustained negative material. Selecting 207 stronger events from 573 detections improves the reference's usefulness but does not make the remaining music exhaustively labelled.

A newly revealed real attack absent from that reference can be counted as false. The direction of that bias is unfavourable to a candidate, but calling it conservative does not make a failed result decisive. The bias is neither bounded nor necessarily equal across candidates.

One concrete room-proxy retention loss for full-bank change was near the onset at 41.40159 s. Baseline's event was +4.889 ms; the candidate's nearest event was −5.111 ms. It missed the −5 ms lead cutoff by 0.111 ms, below the reference's millisecond-scale timing uncertainty. Other losses reflect reduced prominence or larger peak shifts. They must not all be dismissed as matching artefacts.

The weak/recovery generated probes still matter because their planted timing is known: adaptive retained 9 of 12 versus baseline's 12, and static retained 11. Those are substantive warnings even after the measurement defects are acknowledged.

The term "baseline-visible attack" in this pilot means a peak in a scalar deposit observation. It does not mean an attack independently seen on the Titan plate. Sonic retention similarly uses baseline deposit births when the audio reference fails its timing qualification.

## 5. The replay does not traverse the live product path

The rejected patch's `tests/host/p6_dbs_probe.cpp` runs the 40 kHz input through the 24 kHz converter, 180-sample analysis and real core renderer at 100 Hz. That is meaningful core coverage. It explicitly omits donor admission, display processing and wire packing.

In `platform/ra8p1/palette_runtime.cpp`, `PaletteRuntime::renderInhouse()` additionally applies live admission, confirmed quiet, response gain and actual controls before rendering; then display brightness, warmth, white balance and a hue-preserving shoulder. Master/edge gains and native packing follow elsewhere.

The probe writes renderer `Pixel16` frames directly. Those files are not final GRB48 wire payloads. The scorer reads CSV deposit observations; it does not read the frame files for its event gate. The observed `deposit_peak` is requested amplitude before the triangle and history winner test, not proof of newly written visible pixels.

Consequently, a passing core replay would still leave the actual consumer and optical appearance unqualified. A failing scalar peak metric does not by itself prove that a physical gesture was lost.

## 6. The room fixture is an acoustic proxy, with introduced clipping

`scripts/p6_effect_mapping.py` prepares room input from a canonical iPhone movie's audio and applies a single gain to match Titan's previously recorded median raw RMS. This is not a recording of Titan's microphone PCM. Matching one median cannot match spectral response, temporal processing, noise, reverberation or individual attack levels.

The input manifest records BOOM room gain 86.9649, approximately +38.8 dB, and clipping fraction 0.00126810, approximately 0.1268%. The frozen fixture hash was independently verified. Its int16 samples include 4,584 full-scale rail samples among 3,616,427 pre-tail samples. The slight difference from the manifest clipping count arises from its pre-quantisation threshold and does not change the conclusion. Sonic has 32 rail samples among 1,558,187 pre-tail samples.

| Fixture | Verified SHA-256 |
|---|---|
| BOOM room | `95c9d9b8595295abbc15c08f93416c1666b9d338736cbe42239868600817cc27` |
| Sonic room | `dac2e8c06dce7fdad04244d30772256dabef275c79f7a87cad793629ace6a596` |

This proves a replay confound. It does not prove that clipping caused all regressions, or that Titan's actual microphone is clipping. The fixture remains useful when labelled honestly as a proxy.

## 7. Today's Hybrid is a redesigned renderer

The original K1 source at `/Users/spectrasynq/SpectraSynq_K1_Firmware`, commit `68a95773d41485fa14cb6dd71d89393d3aa25c73`, uses a two-pixel subpixel dot, additive RGB trail deposition and level-dependent decay. Relevant file: `SPECTRASYNQ_K1_FIRMWARE/effects/light_mode_waveform_hybrid_k1.cpp`. The local Lightwave reference independently preserves the two-pixel additive structure.

Current v9 uses a triangular footprint with up to ten pixels of continuous support, maximum-composition of scalar intensity, and a fixed 0.29 s mode-32 decay constant. These are meaningful changes to how attacks appear and interact; the current display is not an unchanged reference running on a different processor.

At 405 pixels/s, nominal travel in 109 ms is 44.145 pixels. The same-location decay factor is 0.687, but the light is also moving. Median deposition positions in the analysed passage leave only about 73 ms of nominal travel to the edge for source input and 95 ms for room proxy input. Thus "the trail is too long" is not yet a sufficient diagnosis. Position, width, winner selection and optical mixing must be considered together.

Restoring additive RGB wholesale could reintroduce the colour mixing that the permanent authored-palette requirement excludes. A narrow scalar-intensity deposit is a bounded later experiment if amplitude repair still loses gestures; source lineage guides the experiment without authorising a blind rollback.

## 8. What the results actually say

| Arm | BOOM source recovery / baseline retention | BOOM room recovery / baseline retention |
|---|---|---|
| Existing onset | 48/106; 96/97 | 5/46; 130/131 |
| Full-bank change | 86/106; 97/97 | 20/46; 122/131 |
| Static groups | 73/106; 95/97 | 14/46; 123/131 |
| Adaptive groups | 78/106; 94/97 | 14/46; 121/131 |

These are the original metric's results, with all its limitations. Existing onset passed the source-side M1/M2/M3 combination but did not transfer to the room proxy. Adaptive grouping did not earn adoption. The distinction between source success and transfer failure matters: "nothing works" hides where the failure changes.

The lane also contains a physical release-puppet experiment. Its README reports a strong centre release after a host-timed cue, with a calibrated timing estimate around −31 ms and explicit camera/alignment limitations. That is evidence worth retaining about available expressiveness, not proof of autonomous musical understanding. I did not inspect or reveal the owner's pending blind A/B order.

## 9. What other systems actually do

The comparison used primary implementations and maintainer documentation, not promotional footage.

| Reference | Verified behaviour | Transferable lesson |
|---|---|---|
| Scott Lawson audio-reactive-led-strip | Gain-normalised bands; scroll writes new colour at the centre, shifts and decays history; spectrum also uses change/common-mode signals. | Preserve the complete feature-to-gesture mapping when reproducing an effect. A named spectral front end is only one component. |
| LedFx melbank architecture | Multiple frequency ranges, automatic gain and temporal/change filtering, with effects choosing raw or filtered features. | Feature normalisation and effect-specific consumption are designed together. It does not imply adding a second AGC blindly to Titan. |
| WLED audio-reactive maintainers | Their end-to-end testing discussion explicitly investigates gain, scaling, preprocessing and inconsistent reactions. | Other projects have the same class of integration problems. Debug the received signal and the final effect together. |

Primary links: [Scott Lawson implementation](https://github.com/scottlawsonbc/audio-reactive-led-strip/blob/master/python/visualization.py), [LedFx architecture](https://docs.ledfx.app/en/latest/developer/melbanks.html), [WLED maintainer discussion](https://github.com/MoonModules/WLED-AudioReactive-Usermod/issues/7). Retrieved during this investigation. These sources do not establish a controlled perceptual advantage over Titan on the same music, microphone and optics.

## 10. Corrective direction and limits

First repair event measurement enough to make the next comparison interpretable. Then change the implicated amplitude transfer, preserving geometry initially. The bounded question is whether loud attacks remain different after actual composition and final output. An onset detector, adaptive grouping or new ML model is not required to test that mechanism.

A smooth monotone shoulder above the current −9 dB knee is one concrete candidate if the current authorised intensity envelope provides headroom. Preserve the current low-level law below the knee. For ceiling c, floor f, gain g and exponent gamma, an upper branch `A(d) = g + (Amax − g) × (1 − exp(−k(d − c)))`, with `k = g × gamma / ((c − f) × (Amax − g))`, matches the value and slope at c. This is a proposed experiment, not an installed correction. Resolve Amax from the real permitted output envelope; do not assume 1.0 is available simply because the representation allows it.

If no peak headroom exists, redesign allocation within the existing envelope and explicitly measure the trade-off. Do not present a global dimmer or brighter image as evidence of recovered detail. Preserve weak response, and test weak attacks after loud history: increased strong deposits can mask subsequent weak ones. If the amplitude difference survives but fresh pixels still fail, isolate footprint/winner composition as the next variable.

The finish line is a visibly improved dense passage with retained weak attacks on the actual Titan under a matched product configuration. Healthy transport, passing module checks, a frozen JSON file and a commit are useful intermediate facts; none is that finish line.

## Execution brief for the existing P6 lane owner

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

Complete the lower-Hybrid musical-detail repair in the existing P6 assignment. Produce distinct fresh attacks in dense BOOM while preserving weak Sonic and recovery; finish the authorised product sequence through physical observation. This brief corrects the DBS pilot's direction; it does not replace or shrink P1–P7, W0–W6 or M00–M24, reopen PRISM's native HOLD, or take another owner's hardware.

Worktree: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`. Audit baseline HEAD is `1d0095f6662669e475ed43fb6ae3e770dcb97099`; pilot commit is `059a399`. Read this investigation, `docs/evidence/P6/dbs/RECEIPT.md`, both frozen criteria, `scripts/p6_dbs_score.py`, current `src/k1/core/visual/inhouse_waveform.{h,cpp}`, and `platform/ra8p1/palette_runtime.cpp`. Reconcile only changes since that baseline. Preserve the original failed evidence and rejected patch; do not relabel them PASS.

1. Checkpoint your own changes and resolve current exclusive device ownership. Preserve all unrelated dirty/untracked files and the pending blind A/B. Load applicable AGENTS, workflow, lightshow-loop and, for hardware work, smc-titan-mixer and E2 Lite procedures. Preserve UID `545433931bd25436593630352d068363`, P601/GPT6 + P604/GPT8, mic DMA1/2, LED DMA0/3, continuous 160/DIN80+80 mapping, modes 32/18, current palette selections with path P, brightness default 100/255, output envelope and autonomous music. Use actual current settings, not host-007 assumptions.

2. Correct the event measurement in a new frozen version. Reproduce the three-frame plateau bug first. Represent one plateau/pulse episode as one event, while proving separate 75/100/200 ms planted attacks remain separate. Bound annotation, sampling and alignment uncertainty before freezing matching; do not widen a window after viewing candidate failures. Qualify one positive control against every gate and reject steady/no-attack, random-trigger and uniform-dimming controls. Treat unlabelled music events as unknown unless independently verified false. Use a short checked event set and exact planted probes rather than inventing a comprehensive new annotation programme. Keep >=95% retention of credible baseline events on room input and weak Sonic, but define that denominator at the measured output boundary. Preserve the existing quiet/recovery probe.

3. Make the comparison consume the actual product path: admission, response gain once, current controls, renderer, display transforms and final native packing. Bind configuration to results. Trace requested amplitude and the new material that actually wins against history; score the relevant final spatial gesture rather than substituting requested amplitude. Reuse existing harnesses and capture tools. Keep iPhone-derived fixtures labelled room proxies. For transfer qualification use a bounded current Titan PCM capture at the analysis boundary if supported; otherwise establish the missing capability explicitly and use live received-feature/final-output evidence without calling the phone Titan PCM. Do not block the host mechanism correction on building a new capture framework.

4. Implement the smallest amplitude-only correction first. Keep width, position, scroll, decay, authored palette coordinates and upper Tempo unchanged. Remove the flat high-level amplitude response with a monotone bounded mapping; preserve the established weak-level response where headroom allows. The shoulder described above is an initial candidate only after resolving its Amax within current authority. Do not globally turn down the show, apply output attenuation to stored history, add a silence floor, or reintroduce adaptive grouping as a prerequisite. Verify several loud admitted inputs produce distinct actual output gestures. Compare with baseline and a uniform-gain control so brightness alone cannot earn credit.

5. Test the named failure with natural lead-in: BOOM 0–75 s, detailed score 55.5–75 s and the existing broader 28–75 s window; weak Sonic; and weak-after-quiet/weak-after-loud probes, including 75/100/200 ms spacing. Reuse valid unaffected tests. Require credible-event retention >=95%, all 12 known recovery attacks retained, no sustained false gestures, intact multicolour/centre-origin behaviour, and no new deadline or transport faults. Treat source-file and room-proxy results separately. Freeze the candidate before the designated holdout and honour exposure history; do not call previously exposed Dopex unseen.

6. If amplitude discrimination is restored but deposits still merge or fail to win, run one separate narrow-footprint experiment informed by the original two-pixel dot, retaining scalar intensity and path-P palette semantics. Keep winner/composition changes separate so the cause remains identifiable. Do not blindly restore additive RGB or rebuild the analyser. Stop expanding a mechanism when its predicted distinguishing result is absent; inspect the next demonstrated loss and repair it within scope.

7. Before deployment, prove scored and built product configurations agree, including the real runtime/output transforms. Existing path-P tests alone do not qualify the previously bypassed stages. Plant the old knee/configuration to show the relevant checks fail. Build ARM from frozen sources, verify image/UID/settings using the authorised deployment route, and complete the physical comparison under existing standing authority once ownership permits. No new blanket image approval is needed; failed checks, unknown identity, native denial or another active owner stop the affected write.

8. Use the working camera-server route. Verify full-plate framing and actual recorded audio before the comparison; retain natural history and the complete release. Align by audio content, preserve offset/drift/uncertainty, and inspect both final payload and the plate's space–time view. Do not claim microphone equivalence from median calibration or optical timing finer than capture supports. Independently observe the result before requesting the Captain's taste verdict. Keep the release-puppet and autonomous drop/tempo questions separate from this microattack repair.

9. Deliver the implemented change, exact source/configuration/image identity, relevant checks and physical result in existing P6 records. Report PASS/FAIL/BLOCKED/NOT TESTED per claim. If a candidate regresses, retain its evidence and restore only through the owner's authorised recovery path. Leave the application autonomously visualising music. Do not close the work with another rejected-idea receipt and an optional suggestion to somebody else; complete executable repairs or name the precise blocking dependency and resumption action.

## Reproduction and evidence index

The accompanying `investigation-20261015/reproduce.py` extracts the original scorer functions and reproduces the plateau defect; it also calculates the source-derived transfer values. `diagnostics.json` preserves the independently reproduced original counts and the explicitly diagnostic plateau consolidation results. No original scorer or receipt was edited.

Key source anchors at the audited revision: `inhouse_waveform.h` constants near lines 38–53; `inhouse_waveform.cpp` decay/deposit near 800–869 and render mapping near 1102–1117; `palette_runtime.cpp` live runtime near 246–313; `p6_dbs_score.py` births/matching near 62–83; `p6_effect_mapping.py` room construction near 60–79 and 97–129. The rejected patch contains the pilot probe and observation instrumentation. Reference qualification is in `docs/evidence/P6/dbs/reference/README.md`; original results are `docs/evidence/P6/dbs/results/scores.json`.

Remaining unproven claims: perceptual benefit of the proposed mapping, exact loss on current live microphone input, optical contribution of footprint/history, and autonomous musical-drop competence. This investigation establishes mechanisms and measurement defects; those physical claims require the bounded owner execution above.
