# K1 Bloom Return — canonical design reference v1

Date: 2026-09-10. Status: source investigation verified; host reference implemented and checked; product integration and optical quality unproven.

This is the entry point for subsequent Bloom/PRISM work in the Titan repository. Keep the source facts, compatibility baseline and new effect contract distinct. Changes to this reference must state which of those three they change. A passing host reference is not a firmware, realtime or physical-product acceptance result.

## 1. Authority and the material finding

The behavioural authority remains DualMCU commit `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`. Titan base inspected: `5fdde20a9f0f27177b8c7d12204c38bb002bbbdc`. The three imported files below match that pin byte for byte. Do not rewrite them to implement this proposal.

| Imported file | SHA-256 |
|---|---|
| `src/k1/core/visual/product_effect_renderer.cpp` | `3029f968231ed745baf03cc7443aadcc67f3d02d318b5059b68c9a1888b9befd` |
| `src/k1/core/visual/product_output_treatment.cpp` | `5f5c9df4f05c47ddaea33d0dabad526e4ccb5e2655e0f06ca16fa26bb818fe2e` |
| `src/k1/core/visual/channel_render_state.h` | `7683987320dbf5a462c9a9ec4f9794b0fc666335284e6bf22a3d6c437f04684c` |

Pinned K1 `applyPrismOverlay()` does not implement Sensory Bridge's image operation. Count 1 on black creates pixel 77 `(48,0,24)` and pixel 82 `(0,48,24)` on a 160-pixel channel. Count 0 and `vp_fix_prism_off=true` preserve black. `check_pinned_prism.cpp` compiles the actual imported treatment and verifies these cells. This is a compatibility fact, not proof that the pin should silently be corrected. The default prism count is zero.

Consequences: current PRISM can add light independent of input, bypass palette choice, and break RGB mirror equality. Exact Titan-to-pin parity does not imply parity with upstream Sensory Bridge. This is also separate from K1 mode 23 `renderPulsePrism`, an event-driven ring effect.

## 2. What the donor actually does

Source snapshot: Sensory Bridge commit `5f329261987144242be4c3729f6d344631d3be1e`. These are source-derived conclusions; no claim is made that a particular unseen recording has been reproduced.

- [Active Bloom, lines 398–499](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/lightshow_modes.h#L398-L499): move history right by `0.25 + 1.75*MOOD`, retain 0.99, replace centre seed, save history, fade the outer quarter, mirror the display. The earlier commented-out Bloom implementation is not the active path.
- [Sprite translation](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/led_utilities.h#L1247-L1290): fractional splatting softens a moving signal; out-of-range contributions are discarded. There is no reflective edge boundary.
- [PRISM](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/led_utilities.h#L1133-L1144): repeatedly add a half-scale, mirrored copy with gain 0.25. If P denotes that image operator, k iterations yield `(I + 0.25P)^k B`, before later limits. Two iterations give `B + 0.5PB + 0.0625P²B`. Compressed branches can look like returning waves and create interior origins; they are not triggered by reaching the physical edge.
- [BULB](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/led_utilities.h#L1076-L1093) applies a stationary repeating intensity mask. It can reveal node-like points but does not spawn travelling seeds.
- [Loop ordering](https://github.com/connornishijima/SensoryBridge/blob/5f329261987144242be4c3729f6d344631d3be1e/SENSORY_BRIDGE_FIRMWARE/SENSORY_BRIDGE_FIRMWARE.ino#L178-L231): PRISM and BULB are downstream of Bloom's saved history. Copying treated pixels into history would create a different feedback system.

The reusable aesthetic is continuous seeded motion, soft trails, attenuated overlapping returns and optional stationary texture. Donor PRISM's interior origins conflict with K1's centre-origin mandate. Preserve its mathematics as reference, and explicitly identify the new behaviour as **Bloom Return**, rather than claiming an exact donor port.

K1 already time-scales its Bloom transport and attenuation at a nominal 120 Hz. Time-based movement is not a newly discovered missing feature. Its Pixel8 history and repeated splatting can still lose low-level detail, and correcting displacement alone does not make resampling blur independent of cadence.

## 3. New effect contract

| Concern | Contract for Bloom Return v1 |
|---|---|
| Origin | New colour enters at the centre. Returns enter from the physical endpoints after the outward travel time. No interior origin grid. |
| Geometry | Render one radial half; write the same RGB to both halves. For N even, index pairs are `N/2-1-r` and `N/2+r`. Native N=160, bench N=128; normalize radius by `N/2-1`. |
| Musical drive | Continuous timestamped source colour, preserving a flowing wash. Do not replace Bloom with a beat-only comet generator. Existing chromagram/palette preparation is an adapter responsibility. |
| Return count | 0: centre→edge, then finish. 1: centre→edge→centre, then finish. 2: centre→edge→centre→edge→centre, then finish. Count is per source contribution; continuing audio continues creating contributions. |
| Colour | Store colour when injected. Old colour remains attached to old history; palette changes affect new input. Intentional global palette transitions are a separate, declared composition operation. |
| Precision | Reference uses float RGB in a declared working domain. It is not calibrated linear radiance. Production should preserve at least RGB16 precision through history/composition/output; a Pixel8 round trip defeats this aim. |
| Black | Zero source eventually yields exact zero after the finite history horizon. The enhanced path must not pass through the pinned fixed-colour PRISM overlay or an enabled additive base coat. |
| Time | MEDIA_TIME_48K content timestamps; source hop 360 ticks. Render calls do not advance simulation state. Preserve event time versus availability time and the existing affine scheduling contract. |
| State | Fixed history, no allocation or expanding event queue. Missing input resets the reference history; duplicate/older input is rejected. Epoch changes require explicit reset. |
| Compatibility | Modes 3/9 and pinned treatments remain byte-frozen. Integrate as a separately identified effect and treatment path. Check the current registry before assigning a numeric mode ID. |

## 4. Implementable translation: a source-history renderer

For fixed one-way travel time T, let S(t) be the centre's colour history and r the normalized radius (0 centre, 1 edge). Read these delayed copies:

| Leg | Source time read | Gain | Included |
|---|---|---|---|
| First outward | `t - rT` | 1 | Always |
| First inward | `t - (2-r)T` | edge_gain | Returns ≥1 |
| Second outward | `t - (2+r)T` | edge_gain × centre_gain | Returns =2 |
| Second inward | `t - (4-r)T` | edge_gain² × centre_gain | Returns =2 |

Multiply each contribution by `2^(-age/half_life)` and divide by the fixed sum of enabled leg gains. The reference therefore bounds channels to [0,1] for bounded source RGB without clipping. This normalization trades primary-wave brightness for headroom; brightness-matched optical comparison must decide whether it is the shipping rule. Do not use a frame-varying normalization that pumps brightness.

An isolated seed reaches the first edge at T, centre at 2T, second edge at 3T and final centre at 4T. No collision queue is needed: the delayed-source equations encode boundary arrival and preserve overshoot between render instants. They also retain continuous colour evolution rather than reducing music to discrete particles.

`bloom_return_reference.h` implements this construction without donor source code. It uses a causal hold for seed reconstruction: no interpolation can leak a later seed into time before that seed existed. This is a timing/topology reference, not the final smooth appearance. Smoother reconstruction must declare its delay; causal trailing filters or a one-hop delayed interpolation are candidates. Render timestamp selection must account for available input; the reference rejects rendering beyond the newest input and does not invent a scheduling policy.

Travel time is latched while history is active. Changing T directly in the equations would reposition existing waves. The reference's `configure()` clears history intentionally. Live mood/speed changes require a later phase-integrated travel coordinate or a bounded transition between instances; do not expose a teleporting speed knob.

Memory and cost:

- Reference maximum T is 2 seconds, so two full returns need up to 8 seconds of source history.
- 1,070 RGB float samples use **12,840 bytes per channel**; observed host `sizeof(BloomReturn)` is **12,896 bytes**. Two host instances are 25,792 bytes, excluding frames and other VP state. These are host layout measurements, not a Titan map result.
- An RGB16 history payload would use 6,420 bytes per channel, but that representation has not been implemented or compared for low-level tail quality.
- At N=160 and two returns, radial evaluation needs at most 80×4=320 delayed source reads per channel, then mirrored writes. The prototype evaluates decay with `exp2` per leg per radius; production can precompute fixed-T radius/leg delays, decay and gains. No timing headroom is claimed until actual target measurement.

## 5. Integration and improvement sequence

| Step | Concrete work | Evidence that answers a product question |
|---|---|---|
| 1 — Reference (done) | Pin donor/pinned-K1 facts; implement return equations; compile the actual K1 PRISM probe. | Mechanism, compatibility gap and bounded reference are reproducible. |
| 2 — Portable effect | Adapt actual timestamped K1 source colour and all 44 palette IDs into a separate renderer; add causal softness and radius-mapped optional BULB texture; preserve zero and RGB symmetry. | Actual palette vectors, irregular render calls, long gaps, epoch resets, and no pre-event light. |
| 3 — High-precision output | Feed a wide composition/output seam. Keep identity LUT by default; avoid early Pixel8 conversion and automatic force-saturation on authored palettes. | Dark tails and gradients survive to the output packer; compatibility path remains exact. |
| 4 — Titan render path | Run on-device at the real output cadence; precompute fixed kernels if profiling calls for it. | Incremental VP cost and memory map with the real AP heavy frames and actual output backend. Host loop speed is not acceptance. |
| 5 — Physical judgement | Match brightness and source audio; compare existing Bloom, new outward-only Bloom and one/two returns on real bars. | Centre/edge origin, convincing return, retained dark detail, no distracting node flicker, measured audio-to-light latency. |

Reuse the existing measurement mechanism for these checks. Do not create another generic soak programme. This effect does not solve the AP tempo/ACF deadline failure or justify the RA8P1 migration by itself. It needs no NPU or M33 partition.

The output-stage documents remain relevant: the 2026-08-28 G0 ruling selects WS2816 dual-DIN and cancels the residual-dither branch. A WS2812 bench remains useful but does not validate shipping depth or lane timing. The G0 four-lane concurrency target is a requirement, not proof of a Titan implementation. Keep emitter packing and lane ownership outside this renderer. Identity gamma remains the starting point. A 16-bit island after Pixel8 cannot recover colour history already lost to quantization.

Prioritized improvements after the reference:

1. Causal softness and finite dark tails at high precision, with a measured latency cost. This restores the organic quality without frame-rate-dependent blur.
2. Palette coherence and transitions: centre-born colour history plus a deliberate crossfade policy; no hardcoded red/green dots.
3. Physically meaningful returns with controllable damping and return count; optional stronger attenuation/broadening on later legs.
4. Optional rhythm-conditioned injection using existing validated musical features. Continuous chromatic drive remains available; confidence loss must have a deterministic fallback.

Reusable downstream result: the same bounded source-history mechanism can drive ribbons, tidal washes and centre/edge echoes. Keep any effect-specific source driver separate from geometry and output treatment. This supports shared quality improvements without requiring a wholesale VP rewrite.

## 6. Reproduce and limits

From the Titan repository root:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror docs/research/bloom-return/check_bloom_return.cpp -o /tmp/k1-bloom-return-check
/tmp/k1-bloom-return-check
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc/k1 docs/research/bloom-return/check_pinned_prism.cpp src/k1/core/visual/product_output_treatment.cpp -o /tmp/k1-pinned-prism-check
/tmp/k1-pinned-prism-check
```

The reference check covers return selection and gain, boundary arrival, read-only rendering, RGB channel preservation, exact mirror mapping for both geometries, finite impulse tail, fixed capacity after wrap, stale input, gaps, epoch reset and causal late-seed behaviour. The PRISM probe checks real imported code, not a rewritten model. See `verification.json` for the captured result.

Not yet proved: perceptual match to the donor, smoothness, actual palette adapter behaviour, Titan cycle cost, MCU structure layout, full AP/VP deadlines, output packing, physical light quality or end-to-end latency. No firmware was flashed for this reference. No new mode ID is allocated. Those are the remaining implementation questions, not reasons to repeat the already answered source investigation.
