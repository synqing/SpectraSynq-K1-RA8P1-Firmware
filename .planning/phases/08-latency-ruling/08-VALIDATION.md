---
phase: 8
slug: latency-ruling
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 8 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Host join-set unit tests + campaign receipt schema |
| **Quick run command** | Host join negatives (Wave 0) |
| **Full suite command** | K1-L campaign only after WP5; else BLOCKED |
| **Estimated runtime** | Host ~20 s; campaign is gated |

## Wave 0 Requirements

- [ ] Dropped-frame and epoch-change cannot vanish from the join set
- [ ] USB-read timestamp rejected as event time
- [ ] Ruling document checklist for EXECUTION-BRIEF.md §20

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Captain architecture stamp | LAT-04 | Captain owns K1-C | Present recommendation; do not self-stamp |

**Approval:** pending
