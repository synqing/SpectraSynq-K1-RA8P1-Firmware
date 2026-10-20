---
phase: 4
slug: wp3-uncapped-timing
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 4 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python 3 unittest + on-silicon schedule runner |
| **Quick run command** | `python3 -m unittest tests.host.test_tempo_placement tests.host.test_stage_profile -v` |
| **Full suite command** | Quick plus `python3 scripts/run_scalar_schedule.py` on a **fresh** output dir when hardware/image authorised |
| **Estimated runtime** | Host ~30 s; 6000-hop target run is a gated campaign |

## Sampling Rate

- Per host task: quick run
- Hardware wave: new output directory only; never reuse receipts
- Mutation is a **negative**, not qualification

## Per-Task Verification Map

| Task ID | Plan | Requirement | Test Type | Command | Exists |
|---------|------|-------------|-----------|---------|--------|
| 4-01-01 | 01 | WP3-03 | unit | test_tempo_placement | ✅ untracked |
| 4-01-02 | 01 | WP3-03 | unit | empty-tcm receipt must not record DTCM overlay | ✅ source |
| 4-02-01 | 02 | WP3-05 | on-silicon | run_scalar_schedule.py fresh dir | gated WP7 |
| 4-02-02 | 02 | WP3-07 | on-silicon | --timing-mutation hop 136 | gated |
| 4-02-03 | 02 | WP3-06 | analysis | Q1–Q5 or BLOCKED per question | Q1-Q5-CAMPAIGN.md exists |

## Wave 0 Requirements

- [ ] Land `tests/host/test_tempo_placement.py` if still untracked
- [ ] Assert `K1T1.v2` schema label
- [ ] Host hop 0 fixture must not be cited as hop 136 proof

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Rearm empty-TCM image | WP3-10 / WP7 | Operator | Prepared waiter from Phase 1 |

**Approval:** pending
