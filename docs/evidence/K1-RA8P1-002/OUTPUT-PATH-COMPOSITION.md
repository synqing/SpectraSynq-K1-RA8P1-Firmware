---
abstract: "HOST comparison of DualMCU post-renderer output order vs Titan RA8P1 import. Blocks calling LED first light VP done. F1 renderer pixels remain valid and pre-treatment."
---

# DualMCU vs Titan output-path composition

Date: 2026-09-10. Authority: DualMCU `firmware/rt1062/production_runtime.cpp` (~500–570) at pin `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`. Titan import is the RA8P1 `src/k1/core/visual/` tree on `lane/k1-ra8p1-002`. Opcode 11 does **not** run this chain.

Operating objective: preserve K1 musical/visual **behaviour**; this note is the execution/output gap list, not a new renderer.

## Order DualMCU actually uses

`effects → blend → treatment → edge → gain → current limit → stage/show`

Cited:

- `renderProductChannel` then fallback `drawDot`
- `applyFrameBlending` (A 180, B 120) when the product renderer did not fill the frame
- `applyProductOutputTreatment` (incandescent, bulb cover, base coat, dither `{64,128,192,255}`, prism, reverse)
- `applyProductEdgePolicy`
- `applyProductOutputGain` (master brightness Q0.16)
- `applyProductCurrentLimit` (both channels together)
- `led_output_.stage` / `led_output_.show` behind `PixelPort::submit(ConstPixelSpan Pixel8)`

F1's 320 Pixel8 values are the **effect renderer before** this chain. They are not packed-wire goldens.

Local RA8P1 copies of `frame_blend.*` and `product_runtime_policy.*` are
**named DualMCU derivatives** at pin `6b1e7bc5…`, not byte-identical imports.
They are staged into palette images only (`--palette-runtime`). Frozen timing
images do not compile them. DualMCU SHAs:

| File | DualMCU sha256 |
| --- | --- |
| `frame_blend.h` | `610519b4942f88e5e8ddf0f171e554b20a4befc5715b2a7cb69a04ca4d74e497` |
| `frame_blend.cpp` | `03b9f47d30d8410e93dd381650d495e5047f5a222fac6e1b533dc517abe9d8e6` |
| `product_runtime_policy.h` | `ec1a5375d561673701310dc6437e11d1f3917a37c8d0c42e8ca4e12d70cff7a3` |
| `product_runtime_policy.cpp` | `543fc91ab37aac87335b047807672ff930c2dd17cea68abd4c8b469a849d092f` |

Host verifies `effects → blend → treatment → edge → gain → joint current limit → stage/show`
on two 160-pixel logical channels, plus TRUE16 `0x12AB` through `packPixel` /
`splitChannel160`. Pixel8 ×257 is not that path. Physical four-lane WS2816
qualification and P004 GPT routing remain outstanding.

## Titan import today vs this job

| Step | DualMCU | Titan import today | Opcode 11 first light |
| --- | --- | --- | --- |
| Effects renderer | yes | yes (F1 320 Pixel8 PASS) | not used |
| Frame blend | `applyFrameBlending` | HOST port `frame_blend.*` (2026-09-13); not default palette-step | not claimed |
| `applyProductOutputTreatment` | yes, Pixel8 dither on | yes, Pixel8 dither branch exists | **off** — packed u16, G0.1 cancels residual dither |
| Edge policy | `applyProductEdgePolicy` | HOST port `product_runtime_policy.*`; not default palette-step | host tests pass; physical admission after D2 |
| Output gain | `applyProductOutputGain` | HOST port, unity/zero/half checks | host tests pass |
| Current limit | `applyProductCurrentLimit` | HOST joint-limit both channels | host tests pass |
| Stage / show | `PixelPort::submit(Pixel8)` | no physical port | packed-lane submit, TRUE16 fixture `0x12AB` |
| Wire | G0.1 four concurrent lines | two lines wired (P601/P004) | channel-A subset, 2 of 4 |

Do **not** bind `ChannelRenderState` Pixel8 through `×257` onto the wire. `PixelPort` cannot carry RGB16 LSBs.

## What first light may claim

CRC match + DWT emit/bit-period = **emission** of a packed TRUE16 fixture. Not photon PASS. Not VP output parity. Not `K1_RA8P1_LED_OUTPUT=PASS`.

## Next (not this flash)

Physical four-lane WS2816 qualification, P004 GPT routing, and packing on
silicon. The logical host chain is implemented. Combined live preflight and
K1-L wait on transmitter admission.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:grok | Created from DualMCU production_runtime.cpp vs Titan visual import. |
| 2026-09-13 | agent:grok | D3 HOST port of blend/edge/gain/joint-limit; physical admission still after D2. |
| 2026-09-20 | agent:grok | Named DualMCU derivatives admitted to palette staging; host TRUE16 packing and full logical chain; physical lanes still outstanding. |
