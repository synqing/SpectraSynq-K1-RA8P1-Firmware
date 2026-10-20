# K1 appearance transfer — implementation and evaluation handoff

23 September 2026. Companion to the source-pinned Connor/Fadecandy teardown. This is the next-work specification produced by the research; it is not a claim that the changes or physical comparisons below have run.

## Objective

Establish which complete rendering relationships explain the desired appearance, then transfer one visibly useful profile into the existing K1 engine. Finish with a reproducible reference, a clearly named candidate, a matched comparison and an adoption decision grounded in actual appearance and timing.

The initial candidate should preserve fractional history and expose its transport, spread and decay deliberately. Native high precision is necessary where fractions would otherwise be lost; it does not by itself define the look.

## Authority and fixed context

| Reference | Scope |
|---|---|
| Sensory Bridge 3.2 `a27d5db…` and 4.1.1 `5f32926…` | Two materially different audio/visual profiles; full SHAs and source paths are in `sensory/refs-inventory.json`. |
| Emotiscope 1.1.0 `634b7ed8eefa2f1913ba1a1e7b29c116f22714d4` | Released scalar-VU Bloom, swept dots, float composition and complete output chain. Main has identical firmware. |
| Emotiscope 2.0 `92e14bdd36d96f1da59f852a4a769af0639b7116` | Unfinished experimental reference for separate colour control, phosphor and preview; inactive/incorrectly advertised paths are explicitly documented. |
| SuperPixie `cacbe615aa31a3c8d5a7d7e66286c67a7a7779ce` | Continuous geometry, semantic scene transitions and immediate-attack persistence; not a music analyzer. |
| Fadecandy `36f616158f195a327f8486474af2956dad52881d` | Arithmetic/server reference; retain its exact source attribution and distinguish the historical Kinetis backend from portable maths. |
| Current K1 authority | Each lane's active instructions, behavioral reference, control contracts and actual source/image identity. The earlier 23 September target audit is a dated source map, not a substitute for rechecking the affected files when implementation begins. |

Preserve current 24 kHz/180-sample/80-bin AP semantics; corrected K1 tempo behavior; centre-origin geometry with 79/80 as the centre pair; independent A/B controls; established capture and output ownership; true-black lifecycle; and identity colour correction by default. Carry the scoped WS2816C/four-half-line product decision and cancelled residual-RGB8-dither work. Current bench topology remains a separate evidence scope.

Use existing lane isolation and change registration. Preserve the current compatibility path and named 23-mode renderer reference. Do not retune frozen 100 Hz/10 ms artistic parameters merely because the AP interval is 7.5 ms.

## Work package 1 — executable appearance references

**Why:** A mode name and a final RGB dump omit the history meaning, clock, palette ownership and output transforms that determine the appearance.

Create a host reference for the smallest relevant official functions and their actual call order. Record source blob hashes, settings, logical geometry, state initialization, reference update cadence and every omitted stage. Keep original behavior and any correctness repair as separate profiles. The evidence packet already supplies small numerical probes; reuse them where they answer the question.

For Bloom, distinguish:

- Sensory Bridge's chroma-derived colour history.
- Emotiscope's scalar VU history recoloured using current hue controls.
- K1's existing authored/history behavior.

A reference must declare whether palette changes recolour existing trails or only new deposits. Preserve the fractional resampling kernel, origin replacement, decay, edge policy and quantization points. A pure shift with the same centroid is not the same reference.

**Output:** one machine-readable appearance descriptor per admitted reference, deterministic timed input and state/frame traces. Verify the original source path before calling a result “upstream parity.” Correctness repairs must be listed, not hidden behind the same label.

## Work package 2 — matched diagnostic scenes

**Why:** Whole-song comparisons confound capture, AGC, gesture, brightness and optics. Small scenes identify what changes; music determines whether the change is useful.

Use host-generated feature sequences for a single impulse, two nearby impulses, a sustained level, a gradual swell, a palette change during a trail, and a rapid control interruption. Then use the same admitted recorded music through the relevant complete AP profile. Keep direct feature replay separate from full PCM replay.

Hold geometry, scene timing, palette, brightness/current ceiling and compared output stage fixed. Compare:

1. Repeated fractional transport versus one-time translation at matched centroid motion and retained sum.
2. Original coupled speed/spread/decay versus an explicitly separated candidate.
3. Swept constant-peak, constant-energy and bounded velocity-emphasis drawing.
4. Existing temporal chain versus prompt accent plus controlled persistence.
5. Incumbent precision versus wide state through the final meaningful operation.

Start with the first comparison that distinguishes the observed shortcoming. Do not run an exhaustive permutation of all parameters. Preserve exact darkness where the scene or product state requests it.

**Output:** common inputs, per-stage traces and side-by-side visualizations. Metrics describe centroid, spread, retained sum, peak, rise/tail time, gap depth, hue change, clipping and presentation age. They are not an automatic aesthetic score.

## Work package 3 — one complete K1 profile

**Likely source seams, recheck before editing:** `core/visual/product_effect_renderer.*`, `product_palette.*`, `channel_render_state.*`, `product_output_treatment.*`, and the existing pixel/output boundary. On Titan, corresponding derivative paths are under `src/k1/`; `platform/ra8p1/palette_runtime.*`, `hd_pixel16.h` and the `fixture_app.cpp` lift bridge compose the live path. Use the declared derivative/reference update process.

Implement one wide, centre-origin profile alongside compatibility output. Store the selected history representation at sufficient precision, apply palette/geometry/treatment in a declared domain and quantize only where the physical path requires it. An adapter that multiplies Pixel8 by 257 is still a compatibility path; it cannot restore earlier fractions.

For the new independent-control profile, define velocity in pixels/s, diffusion in pixels²/s and decay in seconds or distance. Choose the discrete update from those semantics and prove its boundedness. Preserve the legacy coupled profile for comparison. Do not claim that a new transport equation is an exact donor reproduction.

Keep artistic intensity/colour distinct from device calibration and aggregate output limiting. Preserve identity correction unless a separate colour candidate is being evaluated. Ensure the final gain/limiter sees all displayed contributions and channels. Apply blackout after any history/persistence path capable of retaining light, while preserving authored state only where the accepted lifecycle requires it.

**Output:** one selectable profile, complete producer-to-wire vectors, and a reference selector that reproduces established behavior. No capture driver, tempo replacement or LED-backend rewrite belongs in this work package.

## Work package 4 — host authoring and replay utility

**Why:** Fadecandy's most useful adjacent capability may be shorter iteration time, provided the observer accurately names what it displays.

First use existing K1 replay and recording seams. If a Fadecandy/OPC preview adds value, put it behind a host adapter. Keep canonical wide frames, timestamps and source identity in the recorded reference; derive RGB8 previews afterward. Ordinary OPC packets are not a lossless record of wide output or proof of physical presentation.

Evaluate the actual native macOS build before depending on legacy fcserver. The source has old dependencies and 32-bit assumptions on one build route. A small contemporary preview/replay adapter is an alternative when only mapping and visualization are needed. Do not introduce a PC dependency into normal autonomous K1 playback or stream pixels through the S3 control SPI contract.

Separate frame interpolation, LUT interpolation and output quantization. Preserve immediate onset response; use local trajectory evaluation at intended presentation time where the renderer owns the motion. Copy no transfer-curve generator without finite, monotonic, endpoint and identity checks—the packet reproduces a non-default Fadecandy NaN case.

**Output:** a usable offline comparison loop and a statement of its precision/timestamp boundaries. A UI that displays only pre-output keyframes must be labelled accordingly.

## Work package 5 — physical comparison and decision

Perform physical work within the current lane assignment and authorization. Target identity, mapping, output faults and evidence scoring must be reliable for the claim being made; unrelated later qualifications do not block independent host work.

Compare the selected profile on the same actual LEDs, geometry and optical stack. Fix camera position, exposure and white balance when video is used. Show brightness-matched comparisons as well as equal-setting comparisons. Include direct viewing, dim moving patterns, closely spaced musical events and sparse/held material. Measure actual output cadence and event-to-visible timing where responsiveness is claimed.

Characterize a single-pixel spatial response through the physical cover at a few drive levels if optical blur is a plausible explanation. Treat this as setup-specific characterization, not absolute calibrated photometry. Native LED PWM and camera integration must remain distinct from software frame counters.

**Output:** the preferred profile and why; demonstrated timing/cost; the rejected alternatives; and which apparent improvements disappear after brightness or optics are controlled. If no useful visual difference remains, retain the incumbent and close the candidate honestly.

## Acceptance and genuine dependencies

| Claim | Required evidence |
|---|---|
| Exact reference behavior | Source/configuration pins and deterministic vectors including retained state, not only final frame shape |
| Correct new transport | Declared mathematical semantics; conserved/decaying sum as intended; bounded values; centre/direction and boundary checks |
| Genuine wide colour | Distinct fractional source values survive the complete treatment and packer; no hidden Pixel8 round trip |
| Improved appearance | Controlled direct comparison with stated preference and competing explanations; numerical descriptors alone cannot pass this claim |
| Preserved response | Explicit event, availability, render, submission and presentation times; interpolation delay included |
| Runtime suitability | Fixed storage, no steady-state allocation, actual loaded target timing, correct DMA ownership and no required fault regressions |
| Product compatibility | Existing profile vectors and control/blackout behavior preserved; admitted exceptions named |

Stop the affected action when its source/profile or target identity is unknown, the reference itself fails its declared invariants, the active lane has a conflicting owner, or required physical evidence is invalid. Preserve the failure and resume after that specific condition is repaired. Source comparison, host mathematics and documentation can continue independently.

Deliver changed paths and identities, reference/candidate traces, the actual comparison result, remaining untested claims and the next concrete action. Keep a complete appearance reference available to future agents so this work becomes cumulative.
