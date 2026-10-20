---
phase: 2
slug: wp1-capture-gate
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 2 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python 3 unittest |
| **Config file** | none |
| **Quick run command** | `python3 -m unittest tests.host.test_score_p601_capture -v` |
| **Full suite command** | `python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness -v && python3 scripts/score_p601_capture.py --self-test` |
| **Estimated runtime** | ~20 seconds |

## Sampling Rate

- After every task commit: quick run
- After every plan wave: full suite
- Before verify-work: full suite green; no WAVEFORM physical PASS

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|---------|-------------------|-------------|--------|
| 2-01-01 | 01 | 1 | WP1-05 | unit | test_score_p601_capture | ✅ | ⬜ pending |
| 2-01-02 | 01 | 1 | WP1-06 | unit | F-01 synthetic physical rejected | ❌ W0 if missing | ⬜ pending |
| 2-01-03 | 01 | 1 | WP1-07 | unit | suite runs without Mac receipt | check setUpClass | ⬜ pending |
| 2-02-01 | 02 | 2 | WP1-02 | unit | manifest sha mismatch | ❌ W0 | ⬜ pending |
| 2-02-02 | 02 | 2 | WP1-03 | unit | timing/reset/bit-count negatives | ❌ W0 | ⬜ pending |
| 2-02-03 | 02 | 2 | WP1-04 | unit | payload-equal not correlated | ❌ W0 | ⬜ pending |

## Wave 0 Requirements

- [ ] Mac-free unit tests
- [ ] F-01 reproduction test
- [ ] Manifest / non-hex / NaN / 3071 / 3073 / clipped / reset negatives
- [ ] Correlation negative (all-black payload equality)

## Manual-Only Verifications

None for this host phase. Live armed capture is Phase 5.

**Approval:** pending
