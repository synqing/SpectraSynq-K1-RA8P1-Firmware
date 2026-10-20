---
phase: 1
slug: wp0-live-checkpoint
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 1 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python 3 + shell hashlib / git |
| **Config file** | none |
| **Quick run command** | `git rev-parse HEAD && python3 /Users/spectrasynq/Workspace_Management/Software/agent-skills/packages/ra8p1-titan-engineering/skills/ra8p1-titan-engineering/scripts/platform_memory.py --check` |
| **Full suite command** | Quick plus `bash scripts/check_references.sh` and SHA-256 of STATUS-cited receipt files |
| **Estimated runtime** | ~30 seconds host; INFO bind is a separate CDC step |

## Sampling Rate

- **After every task commit:** Quick run
- **After every plan wave:** Full suite
- **Before `$gsd-verify-work`:** Full suite green; identity either bound or explicitly BLOCKED
- **Max feedback latency:** 60 seconds for host checks

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|---------|-------------------|-------------|--------|
| 1-01-01 | 01 | 1 | WP0-01 | smoke | platform_memory --check | ✅ | ⬜ pending |
| 1-01-02 | 01 | 1 | WP0-02 | smoke | CDC/process inventory | ❌ W0 | ⬜ pending |
| 1-01-03 | 01 | 1 | WP0-03 | smoke | named-receipt glob + sha256 | search may be empty | ⬜ pending |
| 1-01-04 | 01 | 1 | WP0-04 | integration | titan_broker.py info-once | ✅ | ⬜ pending |
| 1-01-05 | 01 | 1 | WP0-05 | manual-if-no-flash | opcode-22 extract | ✅ decoder | ⬜ pending |
| 1-01-06 | 01 | 1 | WP0-06 | smoke | STATUS/receipts current fields vs hashes | ✅ | ⬜ pending |
| 1-01-07 | 01 | 1 | WP0-07 | smoke | check_references.sh | ✅ | ⬜ pending |
| 1-02-01 | 02 | 2 | WP7-01 | smoke | programme_scalar.py without --execute | ✅ | ⬜ pending |

## Wave 0 Requirements

- [ ] `scripts/check_cdc_owner.py` or documented `lsof`/`ps` recipe that fails closed if owner unknown
- [ ] Receipt rehash helper that prints ABSENT rather than guessing hashes

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Rearm WAITING reply | WP7-02 | Operator protocol | Captain says Rearm; agent starts waiter then replies WAITING |
| Finger reset instruction | WP7-03 | Human hands | Once only if not already done |

**Approval:** pending
