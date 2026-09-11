---
abstract: "Pinned donor files for the 2026-09-10 RA8P1 gold extraction. HOST-ONLY. Mechanisms adapted; not drop-in firmware."
---

# Donor file list — K1-RA8P1-002 gold extraction

HOST-ONLY. Commits from `docs/titan/RA8P1_Gold_Extraction_2026-09-10.md` in EdgeAI Lab. No hardware reproduced.

| Donor | Commit | Files extracted | What landed here |
| --- | --- | --- | --- |
| SenseGlow | `2e5abea350585dde19cecd8cd048ba680e7c6b8e` | `shared/senseglow_protocol.h`, `Integration_CPU0/src/lighting/senseglow_sk6812_waveform.h`, `senseglow_sk6812_waveform.c`, `senseglow_sk6812_dma.c` | CRC-last 8-slot latest-state ring; DMA/fault/age RAM counters; GPIO-PODR **cost** comparison. Not SK6812 RGBW. Not whole-port DMA. |
| SSIE Dataset Capture | `b74eb35885d663c23f05ed50cffd95dc5efd7e4b` | `docs/audio_protocol.md`, `python/receive_audio.py` | Exact hop packet + WAV/peak/RMS/clip/zero **without** DC removal or 16 kHz relabel. Contract is 12 800 Hz / 96 stereo / 7.5 ms. |
| Zephyr USB Audio | `c10299320b104c17d0e69e2ee32f23ab8bb9ce79` | `app/src/main.c`, `app/src/feedback.c` | RAM event history and occupancy counters. 120 ms prebuffer **not** copied. |
| EnvControl | `5ce85a7ac40bbe72d02697d6c72a914ce2b8f5ec` | `docs/ra8p1_esp32_link_failure_root_cause_summary.md` | Failure knowledge only: C-runtime init and oscillator flags before UART rewrite. No pin map copied. |
| ra8-firmware | `9c6b463f567f9c96d3e486c6cffd43bc898a737d` | `tools/ra8_emulator/src/periph/board_periph_npu.c` | Negative: NPU emulator is SE55 stand-in, not Vela. Not used for timing. |

Local artefacts:

- `platform/ra8p1/titan_ram_diag.{h,c}`
- `platform/ra8p1/k1_exact_stream.{h,c}`
- `platform/ra8p1/ws281x_waveform.{h,c}`
- `platform/ra8p1/k1_shared_snapshot.{h,c}`
- `platform/ra8p1/k1_crc32.h`

SenseGlow, SSIE Dataset Capture, EnvControl, and ra8-firmware are MIT for their own code. This extraction does not vendor their trees.

---
**Document Changelog**
| Date | Author | Change |
| --- | --- | --- |
| 2026-09-10 | agent:grok | Created pinned donor list for the first gold-extract work package. |
