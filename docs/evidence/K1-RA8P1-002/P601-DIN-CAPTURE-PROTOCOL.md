# P601 / first-LED DIN capture contract

Resident image is no longer `a3f37e8a…`. Captain Rearm 2026-09-19 wrote
uncapped profiler `32dd1f5f…` over it before this capture existed. That is a
Captain waiver of the preserve-resident hold, not a waveform receipt.

The predicted 3072-bit reference is still the retained all-zero colour-integrity
frame from `a3f37e8a…`. Do not stamp `WAVEFORM_CAPTURED` from that JSON, from
the profiler image, or from any host/compile-only result.

## Predicted reference (firmware accounting)

| Quantity | Value | Source |
| --- | --- | --- |
| Pixels on this stick | 128 | retained colour-integrity payload |
| Bits per pixel | 24 | GRB WS281x packing |
| Wire bits | 128 × 24 = **3072** | bit 0 .. bit 3071 |
| GPT6 pin | **P601 = GTIOC6A** | RA8P1 Table 1.17; not a PFS enum guess |
| DMA words | `bits - 2` = **3070** | two duty words preloaded; `ws281x_gpt_dma_hw.c` |
| Expected `DMCRA` at clean stop | 0 | remaining 1 is the known fault witness |
| Level shifter | 74HCT2G34GW | MCU pad → first LED DIN |
| Timing family | WS2812-class GPT profile on this image | do **not** mix WS2816C-1313-4P bench timing |

The retained payload is 384 all-zero GRB bytes. Reconstruct duty from that
payload; do not substitute a later host pixel command.

Machine-readable predicted reference and fail-closed scorer:

```
python3 scripts/score_p601_capture.py --self-test
python3 scripts/score_p601_capture.py --predict
python3 scripts/score_p601_capture.py --capture <new-p601-din.json>
```

Predicted receipt (not a waveform):
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/p601-predicted-a3f37e8a-20260919-01/predicted.json`

A missing capture file, the historical `ws2812-p601-live-01` diagnostic, a
3071-bit synthetic, and a synthetic object that claims `WAVEFORM_CAPTURED`
must all fail. Do not reuse that predicted directory for a later physical
capture.

## Probe points

1. MCU pad P601 (GTIOC6A), before the shifter.
2. First LED DIN, after the shifter, same ground reference.

Capture both ends of the **same** frame.

## Capture contract (must all be present)

- First pulse (bit 0): start, width, idle-low before it.
- Last pulse (bit 3071): start, width, not clipped.
- Bit count: 3072 legal bits, or the measured shortfall.
- Reset-low after the last bit: duration vs the profile `reset_us`.
- Same capture at P601 and DIN, hashed into a new EdgeAI receipt directory.
  Do not reuse an existing programme/run folder.

Label the receipt `WAVEFORM_CAPTURED` only when those fields exist.

## First-divergence tree

| Observation | Next action |
| --- | --- |
| 3072 legal bits, reset OK, `DMCRA==1` at GPT stop | Firmware accounting / GPT0 stop vs DMA. No fourth arbitration map. |
| Count short or last pulse clipped | GPT/ELC/DMA last request, now measured |
| Pad good, DIN bad | Level shifter / cable / ground |
| Both ends dead | Power/ground — stop firmware |

Repair **only** the first measured divergence. Then repeat zero-output, warm
low-load, and USB-stress with PDM guardrails; restore settings; release CDC.
Optical KEEP is later. Captain is not the plate validator.

## Stamp that closes this step

Hashed P601 + DIN capture of the retained `a3f37e8a` frame, filed under
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/`
with SHA-256 in `external-receipts.json`.
