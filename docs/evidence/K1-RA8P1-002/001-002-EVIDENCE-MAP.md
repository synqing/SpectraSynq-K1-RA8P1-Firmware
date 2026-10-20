# 001 ↔ 002 evidence map

Not a flash grant and not a substitute for STATUS. Issue stamps only for
silicon already proved on a named UID and build.

| 001 deliverable | 002 current source-bound evidence | Limitation |
| --- | --- | --- |
| Environment / identity | UID `545433931bd25436593630352d068363`; checkout `4311408`; BSP pin `6dd0a705`; DualMCU pin `6b1e7bc5`; INFO bind `takeover-info-once-20260920-01` | Identified, not accepted |
| Boot | Autostart source is live WaveformK1 mode 32, flags 5; live INFO build `c7f6034a…` | 5 s observation is not a soak |
| Shared time | Affine/media-time contracts retained in imported K1 | No new Titan timing qualification this session |
| Scalar parity | Historical scalar image `603e3f17…` / HEX `619077a5…` remains scoped PASS | Does not transfer to later GPT/PDM combined images |
| Scalar performance | Empty-TCM profiler HEX `22d405c1…` / build `8a78961b…` built; observer HEX `28eda0d1…`; DTCM `9f12869a…` remains the named historical overlay | Q1–Q5 not closed: images not yet Rearm'd; no raw-hop run |
| Audio-time boundary | 24 kHz / 180 / 7.5 ms AP contract; 12.8 kHz and 16 kHz paths separate | Measured PDM rate is not the AP hop |
| Capture | Predicted 3072-bit scorer `scripts/score_p601_capture.py`; historic frame 2030 witness immutable | `WAVEFORM_CAPTURED` still unstamped; no analyser attached |
| Live AP | Dual PDM 40 kHz + ASRC 24 kHz host tests; SINCRNG 10 in source | Acoustic identity of U13/U14 unproven |
| Musical render | Palette morph/runtime host PASS; centre-origin checks in palette tests | Not a live music acceptance |
| VP parity / product output | Host chain + TRUE16 `0x12AB` packing PASS; blend/policy are named DualMCU derivatives on palette staging | Physical four-lane WS2816 and P004 GPT routing outstanding |
| Comparison vs RT1062 | Ruling deferred | No comparable Titan p50/p99 join this session |
| Mutations | GPT mutations, SINC leftover-5, palette snap-freeze/autostart bounce, P601 physical-without-files | LED DMA mutation binaries that hang in D-state are not current evidence |
| Final validation | Combined-runtime contract is HOST_SOURCE_BOUND | Does not inherit GPT engine or colour-integrity receipts |

The live colour-integrity fault on `a3f37e8a…` (one DMA word remaining) is
closed by a hashed P601+DIN capture of a verified new or historic witness,
not by this map.
