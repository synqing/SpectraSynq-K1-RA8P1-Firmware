# Phase 10 — Live K1 runtime

**Spec:** `docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md` (do not amend the 20 September takeover brief).

**Depends on:** Phase 1 checkpoint; remaining host negatives bound.

**Does not depend on:** analyser, P601/DIN WAVEFORM, empty-TCM Q1–Q5, four-lane wiring, optical KEEP, RT1062 ruling.

**First candidate:** physical emit may default off. Preserve the existing LED diagnostic configuration as a separate investigation image.

**Owner:** persistent integration lead. Subagents get exclusive files. CDC and programming stay on the lead.

**Exit:** `LIVE_RUNTIME_IMPLEMENTED` — host tests + ARM-built named candidate with hashes. Not on-target proof. Not PRODUCT_QUALIFIED.

## Change-sets (live-plan §18)

1. Common clock, hop descriptor, ASRC atomicity, source ownership.
2. Live AP owner, reset/publication, main-loop dispatch.
3. Renderer freshness / composition / control ownership.
4. Protocol, broker, controls, dashboard, recording.
5. Build isolation and host/target gates.
6. Named target results (Phase 11).

## Protected

- DualMCU and Titan BSP trees
- `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md`
- Historical P601 receipts / frame 2030
- Empty-TCM campaign hashes and unused programme directories
- Independent G8
