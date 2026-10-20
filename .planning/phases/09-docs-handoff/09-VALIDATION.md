---
phase: 9
slug: docs-handoff
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 9 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python checklist over the handback + evidence map |
| **Quick run command** | Count §8 rows with PASS/FAIL/BLOCKED/NOT TESTED |
| **Full suite command** | Crosswalk has thirteen deliverable rows; parked §7 table present; remaining tasks have owner or unassigned |

## Wave 0 Requirements

- [ ] `scripts/` or inline checker: no PASS row whose prerequisite is BLOCKED
- [ ] Hash list of cited receipts

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Narrow commit grouping | DOC-03 | Git policy | Inspect staged paths; no git add -A |

**Approval:** pending
