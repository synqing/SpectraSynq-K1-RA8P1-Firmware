---
abstract: "WP14 prepared scalar/MVE fixtures and useful-U55 admission. HOST C plus CROSS_COMPILED objects. Arm and U55 CURRENT_TARGET cells are NOT_RUN. G6 remains NO_QUALIFYING_CANDIDATE. Not production."
---

# plugin-compute — prepared Arm fixtures and useful-U55 admission

Scalar and chunked C kernels plus an independent binary64 reference live in
`platform/ra8p1/arm_numeric_probe.c`. The campaign fixture reuses seed
`0x8A8F1` / 256 samples and the frozen 1e-05 absolute bound. Cancellation,
magnitude, tails and specials are prepared and classified. Host C and Python
accumulation are not executed Arm.

Useful-U55 admission lives in `platform/ra8p1/useful_npu_probe.c`. The
load-only graph `[127, 117, 123]` is refused. The BSP face detector has
generated C only; original same-compilation model, goldens and allocation
report were not found. G6 stays `NO_QUALIFYING_CANDIDATE`. Licence UNKNOWN
stays UNKNOWN. No other student was selected.

| Cell | Level | Result |
| --- | --- | --- |
| Campaign / cancellation / magnitude / tail / special fixtures | HOST | prepared; independent reference recorded |
| Host C versus chunked, frozen 1e-05 | HOST | within bound; not Arm pass |
| Scalar and MVE objects + function-bound disassembly | CROSS_COMPILED | compile-only, not executed |
| Identified-target Arm numerics | CURRENT_TARGET | **NOT_RUN** — live_target `NOT_VERIFIED` |
| Load-only graph as useful inference | HOST | refused |
| Face-detector original goldens | HOST | not located; acquisition request filed |
| Identified-target useful U55 | CURRENT_TARGET | **NOT_RUN** |
| G6 | HOST | `NO_QUALIFYING_CANDIDATE` |

Last identified UID `545433931bd25436593630352d068363` and build 09 remain
HISTORICAL_TARGET. Re-enumeration, loaded-image identity and exclusive
ownership are still required before a current-target cell.

`K1_ARM_NUMERIC` and `K1_USEFUL_NPU` are default off. The USB CDC fixture
is unchanged. p99 6000 µs is unscored.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:Sol-BENCH | Created WP14 plugin-compute evidence index. |
