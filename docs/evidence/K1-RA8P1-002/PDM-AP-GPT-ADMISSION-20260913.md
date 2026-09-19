---
abstract: "2026-09-13 combined 40 kHz PDM → 24 kHz/180 AP + GPT image. Clipping is sparse negative rails, not gain. Not programmed until Rearm."
---

# Combined microphone admission image — 2026-09-13

## Clipping (do not cut gain)

The 16 kHz diagnostic did **not** show two-sided acoustic overload. Both lanes: min −32768, max ≈ +3500–3900. Almost all energy is a handful of negative full-scale hits. That is packing/saturation-counter work, not a gain knob.

The FIFO helper now inspects bits 31:20, sign nibble 19:16 vs bit 15, and counts sat_neg / sat_pos / packing_mismatch / first_sat_raw. No analog or digital gain was reduced.

## Image

- Build `4f30da6bedaff23d12c43aaa70193c4541b1085c4520bba646db7d9b55153a5f`
- HEX `e9c4d655b383c669380ef119076897dd0a44fb01e9f3a6ea64a876feca9b409c`
- First combined flash (`4f30da6b…`) landed; PDM Start returned FSP_ERR_INVALID_SIZE (23) because 300 is not a multiple of 8.
- Fixed slot 296. Build `21ef1ed8…` HEX `524a941d…` ready, not programmed.
- ASRC into unchanged 24 kHz / 180
- GPT renderer on P601; PDM uses DMAC0/1, GPT uses DMAC2
- Acoustic identity still unproven; programme lane U14 expected
- G4 frozen receipt not rerun. Waveform not captured.

Prepared commands: `pdm-ap-gpt-source-20260913-01/rearm-ready.json`. Not programmed.

### Update — 2026-09-14: SINCDEC 49 without SINCRNG

The 40 kHz path copies BSP `g_pdm0_cfg_extend` (SINCDEC 124, SINCRNG 5, 4th-order) then overwrites only `sincdec = 49`. FSP writes both fields into `PDSFCRCHn` as given. Manual Table 50.7 (p3240–3242) lists legal (SFMD, CKDIV, SINCDEC, SINCRNG) tuples; **M = 50 is not a row**. Other tuples “might” overflow/underflow (0x7FFFF / 0x80000).

ON-SILICON on resident `d245b3c4…`: 40 kHz soak peak 857 / 879, sat_neg 0, packing_mismatch 0. 16 kHz diagnostic peak 32768 both lanes. 32768/857 = 38.2, vs (125/50)^4 = 39.06. Live lifetime after the snap: peak 6406 / 6553, still sat 0. Firmware does not dump PDSFCR; values are source-derived for the resident image.

Channel/edge and 16-bit packing match the manual. One discarded pair (7.4 ms) covers Table 50.16 filter settle (~0.67 ms at M=50). Image `b700a591…` HEX `9788ecd0…` sets SINCRNG 10 (between M=62/9 and M=42/11). **Not an accepted flash.** Each SINCRNG step is 2×. 5→10 is 32×. Recorded snap peaks 6406/6553 × 32 ≈ 204992/209696, which saturates int16. SINCRNG 7 is 4× (25624/26212, still unclipped on that snap) and is also off-table. Need a controlled tone before any range flash. Resident `77f77ecf…` still inherits SINCRNG 5.

### Update — 2026-09-14: snap freeze

A loud snap lit WaveformK1 then froze (LED dump max 255, CRC unchanged, GPT still emitting). DualMCU production clears the frame before render; Titan did not. `transportOutward` adds, so the picture saturates in place. Pinned DualMCU renderer left intact. Caller clear is in `palette_runtime.cpp`. Host `SNAP_FREEZE_MUTATION_PASS`. Freeze-fix image `dc07d97d…` HEX `55004b35…` at `pdm-ap-gpt-build-20260914-03`.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-14 | agent | SINCRNG 10 not accepted: 32x clips recorded snap. Rate-lock image 48727875 built unflashed. |
| 2026-09-14 | agent | SINCRNG 10 image b700a591 built; leftover-5 compile guard PASS; not flashed. |
| 2026-09-14 | agent | SINCDEC 49 leaves BSP SINCRNG 5; 16 kHz vs 40 kHz peak ratio 38.2 vs 39.06. |
| 2026-09-14 | agent | Snap freeze: missing DualMCU caller clear. Freeze-fix image dc07d97d built. |
| 2026-09-13 | agent | Created. Combined admission image ready; clipping counters; no flash. |
