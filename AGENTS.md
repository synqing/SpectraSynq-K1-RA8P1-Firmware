# Agent Operating Contract

1. This is a greenfield RA8P1 repository. Do not import the DualMCU Git history.
2. Treat the pinned DualMCU commit in `docs/REFERENCE-MANIFEST.md` as behavioural authority for portable K1 modules.
3. Treat the pinned Titan BSP commit as vendor/platform evidence, not SpectraSynq architecture authority.
4. First-pass objective is **scalar functional parity**, not Helium/MVE optimisation, NPU use, or M33 partitioning.
5. Cortex-M85 owns AP + VP for K1-RA8P1-001. M33 and U55 remain parked until parity and timing measurements justify reopening them.
6. Preserve `MEDIA_TIME_48K`, `MUSICAL_TIME`, event-time vs availability-time separation, affine clock mapping, and target-neutral render scheduling semantics.
7. Platform code belongs under `platform/ra8p1/`. Shared K1 code must not include Renesas/FSP/RT-Thread headers.
8. Never claim physical timing, audio fidelity, LED phase, or multi-K1 synchronization from a host/compile-only result.
9. Do not modify the sibling DualMCU or Titan BSP repositories unless the user explicitly authorises it. Read them; do not develop inside them.
10. Do not silently change frameworks. Audit the live Titan bring-up/toolchain first. Any RT-Thread/FSP/bare-metal choice must be evidence-backed and documented.
11. Keep commits narrow. Do not use `git add -A` without proving every path belongs to the current slice.
12. A green build is necessary, never sufficient. Each major gate needs at least one negative/mutation proof that it can fail.
