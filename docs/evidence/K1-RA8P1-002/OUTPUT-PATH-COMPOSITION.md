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

Import or re-implement edge / gain / current-limit against DualMCU bytes, then a wide 16-bit submit. Second 160-stick when wired.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:grok | Created from DualMCU production_runtime.cpp vs Titan visual import. |
| 2026-09-13 | agent:grok | D3 HOST port of blend/edge/gain/joint-limit; physical admission still after D2. |
