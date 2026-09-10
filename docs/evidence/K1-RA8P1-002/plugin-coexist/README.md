---
abstract: "WP15 paired audio/NPU gate. HOST mismatch reject and load-only refusal. CROSS_COMPILED object only. Physical pair CURRENT_TARGET is NOT_RUN because WP13 PDM and WP14 useful U55 are both NOT_RUN. Not production."
---

# plugin-coexist — paired audio/NPU bookkeeping

`platform/ra8p1/coexist_probe.c` freezes the pair identity (PCM input, NPU
coexist profile, build, 24 kHz clock) and allows only the declared treatment
to differ: audio-only versus useful-NPU-active. A changed input, profile,
build or clock is rejected. Distributions come from raw events. Independent
Python rank matches the C percentiles.

The load-only graph `[127, 117, 123]` cannot satisfy useful-NPU treatment.
WP13 physical PDM is `NOT_RUN`. WP14 useful U55 CURRENT_TARGET is `NOT_RUN`.
The physical pair is therefore `NOT_RUN`. Named missing facts: live UID,
exclusive owner, useful-U55 admission, physical PDM.

G4 O2/O3 stay FAILED (2,005 / 2,006 misses) and are not relabelled unrun.
7,500 µs is not widened. 6,000 µs p99 stays unscored.

| Cell | Level | Result |
| --- | --- | --- |
| Pair identity except treatment | HOST | comparable; mismatch rejects |
| HOST synthetic distributions | HOST | raw events; independent rank; not physical |
| Load-only as useful-NPU treatment | HOST | refused |
| Compile-only Arm object | CROSS_COMPILED | not linked, not executed |
| G4 O2/O3 | HISTORICAL_TARGET | FAILED; not repeated |
| Identified-target physical pair | CURRENT_TARGET | **NOT_RUN** |

Last identified UID `545433931bd25436593630352d068363` and build 09 remain
HISTORICAL_TARGET. `K1_COEXIST` is default off. PDM/ARM/NPU flags stay default
off. The USB CDC fixture is unchanged.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-10 | agent:Sol-BENCH | Created WP15 plugin-coexist evidence index. |
