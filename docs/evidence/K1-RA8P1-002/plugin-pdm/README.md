---
abstract: "WP13 current-source PDM capture evidence. HOST units and CROSS_COMPILED object only. Physical capture is NOT_RUN. Not CURRENT_TARGET. Not production."
---

# plugin-pdm — current-source PDM capture

Corrected completion and conversion/submission units live in
`platform/ra8p1/pdm_capture.c`. The vendor Titan_Mini_pdm first-DATA-as-complete
and half-length stereo submission are not copied.

| Cell | Level | Result |
| --- | --- | --- |
| 16000/8000 stereo/mono unit fixtures | HOST | derived from the C module |
| First DATA interval | HOST | not full-buffer completion |
| Adapter bind | HOST | current `pdm_capture.c`/`.h` hashes; changed hash declines |
| Compile-only Arm object | CROSS_COMPILED | not linked, not executed |
| Identified-target capture | CURRENT_TARGET | **NOT_RUN** — live_target `NOT_VERIFIED` |

Last identified UID `545433931bd25436593630352d068363` and build 09 remain
HISTORICAL_TARGET. Re-enumeration, loaded-image identity and exclusive
ownership are still required before a physical cell.

G6 remains `NO_QUALIFYING_CANDIDATE`. E1 build-09 is not repeated.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:Sol-BENCH | Created WP13 plugin-pdm evidence index. |
