# Phase 8: Latency, comparison, ruling - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §6; `docs/decisions/2026-09-10-audio-to-light-ceiling.md`; `docs/EXECUTION-BRIEF.md` §20)

<domain>
## Phase Boundary

Implement generation-tagged joins, run or honestly block K1-L, bind comparison baselines, write `docs/RA8P1-vs-RT1062-RULING.md` answering fourteen questions. Captain owns the decision.

</domain>

<decisions>
## Implementation Decisions

### Latency definition
- Newest-sample DMA-return estimate → transfer-complete of first frame carrying that AP generation
- p50 ≤ 12,000 µs; ≥ 1,000 joined Titan frames; real music through microphone; observer off
- Report analysis, publish-to-render wait, render, submission-to-transfer-complete
- Frontend/decimation/ASRC delay and LED latch reported separately
- p99 ≤ 15,444 µs is proposed, not an accepted hard gate without Captain stamp
- AP service p99 ≤ 8,000 µs distinct from 7,500 µs frozen resident contract
- Acoustic-to-photon is a separate physical/common-clock measurement

### Joins
- Capture / AP publication / VP use / submit / transfer-complete
- Missing joins, dropped frames, epoch changes cannot be silently discarded
- Never use USB-read time as event time

### Comparison
- Legacy S3: 43 joined frames, p50 11,576 µs — receipt unbound in inspected authority. Bind before using.
- Do not relabel S3 as RT1062.
- RA8P1 vs RT1062 needs comparable RT1062 evidence from its own lane. Do not modify or flash that lane from this task.

### Allowed recommendations only
- `PROMOTE_RA8P1_FOR_DUALMCU_EVAL`
- `KEEP_RT1062_PRIMARY_DUALMCU_EVAL`
- `CONTINUE_PARALLEL_EVALUATION`
- `REJECT_RA8P1_FOR_K1`

Brief recommendation: retain RT1062 as primary evaluation hardware while Titan closes measured gaps. Do not reject Titan compute. Do not change shipping-S3 architecture. Do not self-stamp production migration.

### Claude's Discretion
- Packet layout of the ruling document, provided all fourteen questions are answered or marked unavailable.

</decisions>

<specifics>
## Specific Ideas

Fourteen questions live in `docs/EXECUTION-BRIEF.md` §20 (environment, boot, shared time, scalar parity/performance, audio-time boundary, capture, live AP, musical render, VP parity, comparison, mutations, final validation, plus memory/timestamps/scheduling/tooling/BSP/power/MVE-M33-U55 as listed in the takeover brief).

</specifics>

<code_context>
## Existing Code Insights

- `docs/RA8P1-vs-RT1062-RULING.md` is currently a stub (~511 bytes)
- G6=`NO_QUALIFYING_CANDIDATE`, G7=`NOT_RUN_NO_CANDIDATE`

</code_context>

<deferred>
## Deferred Ideas

Full optical latency (parked). Independent G8 review (parked unless Captain reopens).

</deferred>
