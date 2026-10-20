# Phase 5: WP4 LED first divergence - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §5 WP4; colour table in §3)

<domain>
## Phase Boundary

Capture the LED terminal-word failure with both probes armed, identify the first measured divergence, patch the smallest implicated surface, repeat zero-output / warm low-load / warm USB-stress. Optical stays separate.

</domain>

<decisions>
## Implementation Decisions

### Not the next action
- Do not write a fourth arbitration-map experiment.
- Three failures: fixed PDM-first `d9700b14…` frame 3468; round-robin `49c582db…` frame 2063; fixed LED-first `a3f37e8a…` frame 2030. Each: 1 DMA word remaining; 384 all-zero GRB bytes; 3072 bits; 3070 DMA words plus two preloaded.

### Guard
- Keep corrected nonzero DMCRA fault. Never restore `DMCRA <= 2`. Never manufacture a completion callback.
- Keep independent DMA/waveform/latch completion semantics.

### Capture
- Arm P601 and first-LED DIN before reproducing. A fault-latched transmitter is stopped; attaching afterwards cannot retrieve the preceding waveform.
- Fresh run directory. v2 opcode-22: empty remains v1/728 bytes; LE version 2 requests v2/1420.

### Hypothesis branches (test, do not preselect)
1. All 3072 bits and payload legal, reset valid, terminal DMCRA 1 → trace preload/final request/stop/completion accounting.
2. Missing/wrong final data or clipped pulse at P601 → GPT event count, buffering, ELC stop ordering, last DMA request.
3. P601 conforms, DIN does not → shifter, cable, ground, levels, supply (logic analyser ≠ analogue margins).
4. Both probes lack valid signal → supply, continuity, ownership, trigger before editing render code.
5. Wire conforms but colours fail → part/payload order, strip/control comparison, optical.

### Controls
- `python3 scripts/run_colour_integrity.py --build ... --output ... --phase-s 20 --restore-settings ...`
- Restore file is the actual intended checkpoint, not a guessed default.
- Preserve first failure. Restore settings and release CDC on every exit. No reset/retry inside the scorer.

### Hardware absent
- Report exact missing acquisition capability; candidate, wiring/probe instructions, decoder ready. Continue independent work. Do not invent wire evidence.

### Claude's Discretion
- Which retained guarded image to prefer **if** artefacts and recovery path verify. Do not combine speculative driver cure with new gain/render.

</decisions>

<specifics>
## Specific Ideas

74HCT2G34GW is documented present — do not spend a new investigation assuming it is absent.

Centre-origin mandate still applies to any visual check, but optical KEEP is not this phase's PASS.

</specifics>

<code_context>
## Existing Code Insights

- `platform/ra8p1/ws281x_gpt_dma_hw.c`
- `scripts/run_colour_integrity.py`, `scripts/test_ws281x_gpt_dma_hw.py`, `scripts/test_ws281x_gpt_dma_mutations.py`
- `docs/evidence/K1-RA8P1-002/TITAN-COLOUR-INTEGRATION.md`, `FASTLED-PORT-DESIGN.md`

</code_context>

<deferred>
## Deferred Ideas

Live-audio integration (WP5), product four-lane (WP6), visual tuning while transmission unproven.

</deferred>
