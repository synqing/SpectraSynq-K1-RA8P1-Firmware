# Phase 8: Latency, comparison, ruling - Research

**Researched:** 2026-09-20
**Domain:** Generation-joined audio-to-light latency and RA8P1 vs RT1062 ruling packet
**Confidence:** HIGH on the accepted latency definition. NOT_VERIFIED on any joined campaign.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Traced audio-to-light: newest-sample DMA-return estimate → transfer-complete of first frame carrying that AP generation; p50 ≤ 12,000 µs; ≥ 1,000 joined frames; real music; observer off.
- Report analysis, publish-to-render wait, render, submission-to-transfer-complete. Frontend/ASRC delay and LED latch separate.
- p99 ≤ 15,444 µs is **not** a hard gate without Captain stamp.
- AP service p99 ≤ 8,000 µs ≠ 7,500 µs frozen resident contract.
- Acoustic-to-photon is a separate common-clock measurement.
- Never use USB-read time as event time. Missing joins cannot be silently discarded.
- Bind S3 baseline (43 frames, p50 11,576 µs) before using it. Do not relabel S3 as RT1062. Do not flash RT1062 from this task.
- Allowed recommendation tokens only. Captain owns K1-C. Brief recommendation: keep RT1062 primary eval.

### Deferred
- Full optical latency; independent G8.

</user_constraints>

## Summary

`docs/RA8P1-vs-RT1062-RULING.md` is a stub. Joins do not exist as a closed campaign. Phase 8 cannot inherit a PASS from WP3 compact histograms or from USB observer timestamps.

**Primary recommendation:** Implement generation tags first as host-visible fields with negatives for dropped/epoch frames. Run K1-L only after WP5 admission. If joins cannot be formed, row is BLOCKED, not green.

## Copy-ready contracts

- Authority: `docs/decisions/2026-09-10-audio-to-light-ceiling.md`
- Questions: `docs/EXECUTION-BRIEF.md` §20 (fourteen questions — answer or mark unavailable)
- Recommendation enum: `PROMOTE_RA8P1_FOR_DUALMCU_EVAL` | `KEEP_RT1062_PRIMARY_DUALMCU_EVAL` | `CONTINUE_PARALLEL_EVALUATION` | `REJECT_RA8P1_FOR_K1`
- Historical gates: G6=`NO_QUALIFYING_CANDIDATE`, G7=`NOT_RUN_NO_CANDIDATE`

Join points: capture / AP publication / VP use / submit / transfer-complete.

## Validation Architecture

| Req | Test | Exists |
|-----|------|--------|
| LAT-01 | Host test: dropped frame and epoch change cannot be dropped from the join set silently | Wave 0 |
| LAT-02 | Campaign receipt with n≥1000 or explicit BLOCKED | hardware |
| LAT-03 | S3 receipt path bound or comparison NOT TESTED | docs |
| LAT-04 | Ruling file contains all §20 answers or UNAVAILABLE | docs gate |

## RESEARCH COMPLETE

**Phase:** 8
**Gaps:** No existing join implementation identified as production-complete; do not invent USB event time.
