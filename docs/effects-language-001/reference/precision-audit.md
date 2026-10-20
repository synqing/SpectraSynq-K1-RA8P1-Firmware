# K1 colour precision and output architecture: source audit

Research snapshot: 24 September 2026. Read-only source investigation; no target build, flash, serial session, optical measurement or firmware modification. The checkout is not evidence of the resident image.

## Decision

Use **scalar FP32 as the reference representation for a new, optional wide working pipeline**, with explicitly declared colour intent, bounded headroom, isolated history and one final native UNORM16 quantisation. Keep the existing renderer as a versioned appearance profile and numerical oracle. Sensory Bridge's signed Q15.16 remains relevant as donor behaviour and a possible storage/optimisation choice; it is not the next missing capability by itself.

We already have some of its machinery on DualMCU. We do **not** yet have its equivalent consistently across both K1 lanes. More importantly, neither inspected normal musical output path is an end-to-end wide colour path. Both eventually funnel through RGB8 before the WS2816 words are prepared. A 16-bit wire word cannot reconstruct information already discarded upstream.

Fadecandy supplies valuable ideas about late conversion, explicit output mapping, independent display cadence and efficient LUT evaluation. Calling the entire old pipeline technically superior would be misleading: it accepts RGB8, deliberately stores LUT knots at 15-bit precision for a packed multiply, couples two interpolation decisions, and generates RGB8 output by temporal error distribution. Native WS2816C K1 should retain its native RGB16 route, with identity output correction by default and no external RGB8 residual dither on that route.

## Source identity and lane differences

The audit captured complete relevant source text and SHA-256 identities in `evidence/precision/source-capture.json`; `manifest.json` is the compact identity ledger and `source-excerpts.json` contains numbered decisive excerpts.

| Lane | Current checkout HEAD | Working-tree qualification |
|---|---|---|
| DualMCU | `67a1cd0b0ecc626b28af2df9b0d11f855b402379` | Repository dirty in HMI/control work; all 37 files captured for this numerical/output audit matched HEAD at capture |
| Titan | `431140853dd8b58af53240ef84d5fb1a08bd8b45` | Repository dirty; captured `fixture_app.cpp`, `palette_runtime.cpp/.h`, `k1_live_runtime.cpp` and AGENTS differ from HEAD or are untracked; exact file hashes are retained |

Roots are `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware` and `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`. Titan's `docs/REFERENCE-MANIFEST.md` still names DualMCU `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` as its behavioural import authority. Today's sibling DualMCU HEAD is not automatically Titan's authority. This explains some divergence; it does not justify silently replacing Titan's source.

| Boundary | DualMCU captured source | Titan captured source | Consequence |
|---|---|---|---|
| Position, timing, many controls | FP32 geometry, phase and envelopes | FP32 geometry, phase and envelopes | Continuous motion is already partly present; source colour and accumulation precision still matter |
| Palette HD sampler | FP32 interpolation between RGB8 stops | FP32 interpolation between RGB8 stops | Interpolation can create fractional values, but most callers return Pixel8 |
| Persistent colour for donor-derived modes | Twelve RGB signed SQ15x16 arrays of 160 pixels per channel | No equivalent SQ15x16 arrays in inspected imported renderer; Bloom transports RGB8 `previousFrame` | Histories are materially different; do not describe both as wide |
| Public working frame | `Pixel8`, three bytes/pixel | `Pixel8`, three bytes/pixel | Common composition/output seam is eight bit |
| Output treatment | Pixel8 incandescent/bulb/base/prism/order; legacy dither toggle | Pixel8 output treatment | Repeated multiplication and truncation lose dim detail |
| Native packing | Pixel8 ×257 after optional byte brightness | Separate Pixel16 staging/packing API preserves low bytes, but normal fixture render path stages Pixel8 ×257 | Wide transport is present; arbitrary wide authored musical pixels are not yet produced by the inspected path |
| Physical lane use in bounded image | `K1_WS2816_DUAL_DIN` packs logical A into two 80-pixel DINs; B has no connected output in this image | Selected logical channel staged into two 80-pixel DINs | Neither bounded bench route is proof of full independent A/B, four-half product output |

## What Q15.16 actually means here

Sensory Bridge's `CRGB16` contains **three `SQ15x16` channels**, not three `uint16_t` channels. Its adjacent comment saying “Unsigned Q8.8” is stale and contradicts the declarations. The signed fixed-point format uses a 32-bit storage word, 16 fractional bits, 15 magnitude integer bits and a sign bit. Nominal representable range is −32768 to 32767.9999847. A unit step is 1/65536.

The relevant colour convention is **normalised component values**: ordinary black is 0 and ordinary full channel is 1; values are multiplied by 255 or 254 at the output. It is not a 0–255 working code multiplied by an additional 65536. Headroom exists in the number format, though particular stages clamp or normalise it and some colour paths round through FastLED HSV bytes. The final LED transport is RGB8.

DualMCU's `Sq15x16` makes the same units explicit: constructors multiply a float by 65536, multiplication uses an int64 product shifted right 16, conversion to float divides by 65536, and `quantiseSqRounded` multiplies that normalised result by 255. `sqFromPixel8` first divides bytes by 255. Its donor Bloom keeps fractional RGB in `bloom_history`, stores that history before display edge fading/mirroring, and then separately quantises the displayed frame. This separation is worth preserving. Injection can still originate from Pixel8, so wide history does not mean every stage was wide.

| Representation | Meaning of 1.0/full scale | Spacing around normalised unit range | Bytes per RGB pixel | Appropriate role |
|---|---|---:|---:|---|
| RGB8 / UNORM8 | code 255 → 1 | 1/255 | 3 | Legacy compatibility, some bench transport |
| Signed SQ15x16 | raw 65536 → 1 | 1/65536, absolute | 12 | Existing donor history; bounded fixed-point alternative |
| RGB16 / UNORM16 | code 65535 → 1 | 1/65535 | 6 | Final native LED command, compact bounded storage |
| FP32 RGB | numeric 1 → declared reference level | 2^-23 immediately above 1; finer absolute spacing below 1 | 12 | New composition, continuous transforms, HDR headroom, scalar oracle |

FP32 has substantially finer arithmetic resolution near unit level and much wider dynamic range than Q15.16 at the same RGB storage size. That is a reason to prefer it for authoring/reference composition, **not** proof of a better image or faster execution on these exact builds. Fixed-point may still win for specific kernels or deliberate numerical parity. Do not confuse CMSIS `q15_t` (a signed fractional 16-bit type) with this 32-bit Q15.16 representation.

## Where current precision is lost

1. **Palette exit.** `sampleProductPaletteHd` computes floats but callers frequently invoke `quantiseLinearRgb` immediately. That quantiser truncates `255*x`; other SQ conversion paths round. These are distinct numerical contracts.
2. **Weighted geometry.** The common `render_primitives.cpp::addScaled` truncates each weighted contribution before addition into Pixel8. Two white contributions of 0.002 each become zero plus zero. Sum them in float first and their final native UNORM16 component is 262. This is derived arithmetic, not a claimed visual threshold.
3. **Titan history.** `transportOutward` reads Pixel8 history, distributes it with weighted byte operations and writes another Pixel8 snapshot. A fractional transport path can therefore lose energy through numerical truncation in addition to intentional decay and edge loss.
4. **Dual output treatment.** Incandescent and bulb operations quantise to bytes between stages. Base coat saturates those bytes. Edge policy also modifies bytes. Master/policy gain and current limiting then scale Pixel8 again. A high-resolution HMI brightness value does not make those output levels high resolution.
5. **Native packing.** DualMCU's `map8_to_16` repeats a byte into both halves of a word. Titan's normal `fixture_app` explicitly labels the same operation `REPLICATE8`. It can produce only 256 distinct pre-brightness component words. Titan's subsequent 16-bit brightness multiplication may create unequal high/low bytes; that does not prove a wide source renderer. A low-byte diversity test alone would give a false pass.
6. **Legacy dither location.** Current `product_output_treatment` exposes a four-phase byte dither toggle. It processes an already quantised byte scaled by 254/255; it cannot recover the discarded palette or geometry fractions. Its existence is not evidence that it is enabled in the resident image or an authorisation to put it into the native16 route.

`frame_blend.cpp` also uses Pixel8 history, but DualMCU's production runtime applies that helper only when the product renderer did not handle the channel. Do not attribute its EMA indiscriminately to every product effect.

## Colour-space truth: names are insufficient

`PaletteLinearRgb` currently normalises legacy RGB8 stops and interpolates their numbers. The inspected sampler contains no sRGB inverse transfer, LED calibration or optical measurement. Its name therefore establishes neither scene radiance nor measured linear light. Applying gamma because a variable says “linear” would repeat the very domain confusion this migration should remove.

The new contract should label inputs:

- `LegacyPaletteCodeV1`: existing authored RGB code relationships; preserve them through an explicit compatibility adapter.
- `WorkingRgbF32V1`: non-negative, device-primary light intention. 1.0 is a documented reference channel level. Mathematical additions and blends happen here. Until measured, a device-linear relationship remains a declared modelling assumption, not photometric truth.
- `DeviceDriveRgbF32V1`: bounded drive after the selected artistic tone/gain and device mapping.
- `DeviceRgb16V1`: rounded native UNORM16 command.

A future calibrated profile may establish a more physically meaningful working space. It must have a versioned asset/input conversion; it cannot silently reinterpret all existing palettes. Identity correction remains the default. Artistic warmth, saturation, tone compression and a measured device transfer are separate controls and stages.

## Proposed numerical and ownership contract

This is an implementation candidate, not a measured deployment result.

1. Keep event/presentation timestamps in the existing integer time domains. Use FP32 local elapsed seconds and bounded normalised/radial coordinates for rendering; do not convert an ever-growing absolute microsecond timestamp to float. Keep centre 79/80 and independent logical A/B.
2. Each source layer owns its state and writes finite FP32 RGB. Suggested initial authoring bound is 0–4 per component, at most eight simultaneously admitted layers, and 0–32 for a pre-tone accumulator. These limits are proposed resource/headroom limits, not natural limits of FP32. Validate parameters outside the hot loop; count/clamp anomalous output at the stage boundary. Do not permit NaN/Inf to propagate into integer conversions.
3. Define the blend operator. Additive light, weighted crossfade, premultiplied alpha-over and max-composition are different operations. Coverage is multiplied exactly once. Preserve headroom until the declared tone/gamut stage; a saturating byte add is not the reference operator for a wide layer.
4. History has a declared domain: scalar envelope recoloured now, stored RGB memory, geometric object state, or presentation persistence. Only the owning module reads/writes it. Never feed final brightness, current-limit gain, calibration or output quantisation back into authored history unless a particular effect explicitly requires that feedback.
5. Place artistic tone/gain and device mapping after composition. A optional LUT may be bypassed with exact identity. Frame interpolation, LUT interpolation and persistence have independent controls; disabling one must not change another's mathematics. Turning an artistic feature off is an identity operation at its own boundary; its reset/decay/freeze policy must be explicit.
6. Apply brightness with wide arithmetic once at its owned boundary. Preserve the current master versus channel intent; remove hidden duplicate scaling only through an explicit changed-profile evaluation. Keep current/thermal limits separate from optional appearance toggles. If a device LUT is nonlinear, determine power-limited drive consistently with that LUT instead of assuming a post-LUT code multiplier is a linear-light gain.
7. Quantise once at the native endpoint: `round(clamp(x,0,1)*65535)`. Saturate out-of-range finite values; handle non-finite values before conversion. Pack G16/R16/B16 without scaling bytes. Native16 output has no external RGB8 residual dither. A separate RGB8 bench adapter can have its own documented quantiser, without changing the wide scene.
8. Publication owns an immutable completed frame and generation. Display completion/elapsed display time governs presentation-history decisions, not an assumed render call count. Skipped or busy submissions must not advance a temporal quantiser as though photons were emitted.

Changing numeric representation can alter the appearance even when the formula is ostensibly unchanged: dim trails last longer, overlap grows brighter, quiet gaps fill, and palettes may seem less saturated as previously lost components survive. These are expected comparison targets, not automatic improvement. Preserve legacy golden vectors and add wide-profile acceptance separately.

## Bounded storage for both 160-pixel channels

The following is exact layout arithmetic for tightly packed RGB arrays, excluding object padding, DMA alignment, control/state objects, stack, audio and driver buffers. It is not a target linker result.

| Object | Bytes for 2×160 |
|---|---:|
| One RGB8 frame | 960 |
| One RGB16 frame | 1,920 |
| One RGB FP32 or SQ15x16 frame | 3,840 |
| One RGBA FP32 frame | 5,120 |
| Example: three RGB working frames + two RGB histories + two RGB16 publication buffers | **23,040** |
| Current DualMCU twelve full RGB SQ histories alone | **46,080** |

Thus float does not inherently require more RAM than existing SQ15x16 colour histories. The real cost is the number of simultaneously retained surfaces and effect states. Stream stateless layers through one scratch surface. Reserve state through fixed-capacity per-channel slots with declared maxima. A fade between two stateful effects may need both histories at once; an added persistence module needs another. Do not promise the 23,040-byte example covers every possible enabled graph. Admit configurations against their actual liveness and measured execution budget.

If the new path coexists with every existing DualMCU history, its example allocation adds 23,040 bytes; it does not magically replace the 46,080 bytes. Reclaiming mutually exclusive state requires a separate migration and reset/transition contract. No `malloc`/`new` in render. Baseline scalar timing must respect the existing RT1062 2 ms effect budget and actual audio scheduling; no speed claim follows from the small pixel count.

## Fadecandy: selective adoption

Pinned donor: `PimentNoir/fadecandy` at `36f616158f195a327f8486474af2956dad52881d`.

| Mechanism | Value for K1 | Required translation |
|---|---|---|
| Wide interpolation before final conversion | Retains fractions between authored frames | New K1 scenes should already be wide; avoid an RGB8 host ingress bottleneck |
| Explicit per-channel output LUT | Clear device mapping; replace costly per-pixel powers with a bounded table | Identity default, monotonic/finite/end-point checks, one declared colour domain and exact bypass |
| Packed `SMUADX` interpolation | Potential M7/M85 kernel optimisation | Benchmark against scalar reference; preserve 16-bit knots if accuracy matters, or explicitly qualify 15-bit knot loss |
| `USAT` saturation | Clear bounded conversion primitive | Use target compiler/CMSIS wrapper behind portable interface; keep scalar implementation |
| Refresh independent of host frames | Stable presentation cadence | K1 already has timestamped scheduling; evaluate analytic scenes at presentation time where possible |
| Error-feedback RGB16→RGB8 conversion | Useful concept for an RGB8-only device | Excluded from native WS2816C product output; not a prerequisite for smooth RGB16 |
| USB/server ecosystem | Authoring/replay and a comparison endpoint | Standard OPC is RGB8 and untimestamped; retain canonical wide/timed captures separately |
| Kinetis USB, timer and three-DMA engine | Historical reference | Not a portable RT1062/RA8P1 driver; preserve existing owned transmitter boundaries |

The old LUT uses a deliberately aliased/unaligned 32-bit load of neighbouring 16-bit values and `SMUADX`. A portable implementation should use safe loads or `memcpy`, with explicit alignment/endian and compiler assumptions; ARM instruction availability alone does not establish C++ correctness or performance. Current CMSIS documentation lists the SIMD intrinsic family for M7 and M85, but target options, memory placement and generated code still need inspection. Helium can be an additional backend only after the scalar contract works; it is not a reason to change the scene model.

Three donor details deserve regression cases:

- `FCP_INTERPOLATION` controls both temporal framebuffer interpolation **and LUT interpolation**. Prior host probe: an identity-mapped input byte 128 gives 32896 with interpolated LUT, 32768 with that switch off. K1 must not preserve this unwanted feature coupling.
- Fadecandy arrival interpolation incurs roughly one source interval of delay. At 60 Hz this is about 16.7 ms. Optional interpolation must not become an unconditional stage for prompt musical accents.
- The retained server-LUT probe reaches NaN for a non-default slope/cutoff/gamma combination. Validate a compiled LUT before publication. A selectable curve is not valid merely because a server accepted the parameters.

## Acceptance that distinguishes a real wide path

Use dim fractional coverage, two sub-byte additive contributions, overlapping HDR layers, abrupt palette changes, quiet gaps, long low-level decay, half-scale identity, native low-byte patterns, and final gain/current limiting. Compare captured pre-quantisation floats with the final intended word; do not accept low-byte diversity alone as proof. A dark-to-bright-to-dark case must demonstrate immediate-attack persistence independently of an EMA.

Toggle tests should show that disabled modules are exact identity at their boundary, A/B state does not alias, and enabling one feature does not alter another's parameter or history. Replaying the same timed input with different render cadences must preserve declared velocity/lifetime; any intentional material-diffusion difference belongs in the profile contract. Numeric and host tests cannot establish visual preference, measured PWM precision, diffuser response or physical latency.

## Decisive source locators

Paths below are relative to the explicit absolute roots above. Every cited captured file has SHA-256 and dirty status in `evidence/precision/manifest.json`; numbered source text is in `source-excerpts.json` or the adjacent full snapshot.

| Claim | Exact locator |
|---|---|
| Dual Pixel8 public seam | Dual `core/pixel.h:8–14`; `core/visual/channel_render_state.h` private frame/previous/scratch arrays |
| Dual SQ units and arithmetic | Dual `core/visual/product_effect_renderer.cpp:171–211,3513–3541` |
| Dual wide Bloom history before display operations | Dual `core/visual/product_effect_renderer.cpp:3607–3653` |
| Float palette numerics and byte exit | Dual `core/visual/product_palette.cpp:60–117,139–160` |
| Treatment/gain/limit order | Dual `firmware/rt1062/production_runtime.cpp:788–850` |
| Native byte replication and bounded bench A-only route | Dual `firmware/rt1062/ws2816_pack.h:35–78`; `fastled_parallel_output.cpp:111–122` |
| Titan byte history | Titan `src/k1/core/visual/product_effect_renderer.cpp:326–366,2029–2070` |
| Titan replicated normal frame versus wide packer | Titan `platform/ra8p1/fixture_app.cpp:359–386,400–410`; `palette_runtime.cpp:370–397` |
| Titan true16 helper versus byte lift | Titan `platform/ra8p1/hd_pixel16.h:10–42` |
| SB declaration and nominal full-scale convention | [constants.h, pinned 4.1.1](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/constants.h#L38-L42); [led_utilities.h](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/led_utilities.h#L208-L272) |
| Fadecandy order and residual conversion | [fc_pixel.cpp](https://github.com/PimentNoir/fadecandy/blob/36f616158f195a327f8486474af2956dad52881d/firmware/fc_pixel.cpp#L25-L90) |
| Fadecandy 15-bit LUT and coupled interpolation | [fc_pixel_lut.cpp](https://github.com/PimentNoir/fadecandy/blob/36f616158f195a327f8486474af2956dad52881d/firmware/fc_pixel_lut.cpp#L28-L86) |
| ARM intrinsics, not a speed claim | [CMSIS SIMD intrinsics](https://arm-software.github.io/CMSIS_6/latest/Core/group__intrinsic__SIMD__gr.html) |

The canonical Titan skill's platform-memory record and WS2816 recall were read and checked for entry compliance. Its historical receipts were not treated as current target state. This work establishes no new timeless silicon rule, and therefore changes no platform skill or firmware authority.
