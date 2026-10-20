# Phase 7: WP6 product output - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §5 WP6)

<domain>
## Phase Boundary

Admit local product-output modules as a named reviewed slice; prove composition order and TRUE16 on host; state physical WS2816/multi-lane honestly. Host work may proceed while hardware is unavailable; it must not conceal a blocked transmitter prerequisite.

</domain>

<decisions>
## Implementation Decisions

### Order
- effects → blend → treatment → edge policy → gain → joint current limit → stage/show
- No double brightness/treatment
- Both logical channels through the full chain; F1 renderer pixels alone are insufficient

### TRUE16
- Independent of Pixel8 renderer
- `packBenchGrb48Lane()` expands Pixel8 by ×257 and does not claim extra precision
- Test `0x12AB` through wide submit/packer without double packing, scaling, or 8-bit dither residue

### Topology
- 160×48 bits at 800 kbit/s = 9.6 ms payload > 8.333 ms/120 Hz
- Each 160-pixel edge needs parallel 80/80; two edges need four concurrent lanes
- 80-pixel lane 4.8 ms payload plus reset
- P004 cannot run selected GPT backend. P603 later only with Captain physical approval
- Do not silently reduce pixel count, colour depth, or render rate to obtain a pass

### Memory (32-bit duty-word design)
- One 80×48-bit lane: 15,360 bytes
- Four lanes: 61,440 per bank; two banks 122,880 before other buffers
- Duty data in DMA-accessible SRAM, not DTCM

### Claude's Discretion
- Named slice identifier, provided DualMCU manifest is not silently rewritten to fake byte-identity

</decisions>

<specifics>
## Specific Ideas

Known-good-strip/control comparison from `FASTLED-PORT-DESIGN.md`. WS2812 qualification does not qualify WS2816C-1313-4P.

GPIO bit-bang that disables interrupts ~5.4 ms remains diagnostic.

`frame_blend.*`, `product_runtime_policy.*` exist but are not in `import-slices.json` or the build local-file list.

</specifics>

<code_context>
## Existing Code Insights

- `docs/evidence/K1-RA8P1-002/OUTPUT-PATH-COMPOSITION.md`
- `scripts/test_output_composition.py`, `tests/host/test_output_composition.cpp`
- `ws2816_pack.h`

</code_context>

<deferred>
## Deferred Ideas

Captain-authorised P603/DIN-B physical move and second edge (parked §7).

</deferred>
