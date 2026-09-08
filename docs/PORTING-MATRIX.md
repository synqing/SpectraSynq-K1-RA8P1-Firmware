# K1 RA8P1 Porting Matrix

| Subsystem | Initial RA8P1 treatment | Gate |
|---|---|---|
| `MEDIA_TIME_48K` | import unchanged | host + M85 parity |
| `MUSICAL_TIME` / Q32.32 | import unchanged | vectors + long-run parity |
| affine peer clock map | import unchanged | deterministic skew/offset fixtures |
| clock exchange / peer sync | import unchanged | request/response host gate |
| media↔monotonic correlation | import unchanged | target-neutral host + M85 compile |
| musical render scheduler | import unchanged | target-neutral host + M85 compile |
| GDFT | import scalar first | host differential + target fixture |
| GDFT postprocess | import scalar first | trajectory parity |
| onset detector | import scalar first | event differential |
| tempo ACF/tracker | import scalar first | BPM/phase trajectory parity |
| musical saliency | import scalar first | fixture parity |
| `AudioPipeline` | import after dependencies | complete AP parity |
| capture HAL | replace | RA8P1 SSIE/PDM/DMAC adapter |
| local timestamp source | replace | hardware timer correlated to capture boundary |
| LED backend | replace | RA8P1 output driver + measured latch cost |
| render/effect catalogue | later shared import | pixel CRC parity |
| S3 radio bridge | later adapter | not required for K1-RA8P1-001 core parity |
| Cortex-M33 | PARKED | no implementation in first lane |
| Ethos-U55 | PARKED | no implementation in first lane |
| Helium/MVE optimisation | PARKED until scalar parity | benchmarked follow-on only |
