---
phase: 3
slug: wp2-host-hygiene
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 3 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python 3 unittest / standalone scripts |
| **Quick run command** | `python3 tools/serial-studio/tests/test_identity_checkpoint.py && python3 -m unittest tests.host.test_mode32_real_audio_observation -v` |
| **Full suite command** | See 03-RESEARCH.md Validation Architecture full suite |
| **Estimated runtime** | ~90 seconds excluding full palette compile |

## Sampling Rate

- Per task: quick run
- Per wave: full suite except `test_palette_runtime.py` unless WP2-07/08 touched
- Before verify-work: full suite including palette if those files changed

## Per-Task Verification Map

Copied from 03-RESEARCH.md WP2-02 through WP2-10 and OBS-01–06. Wave 0: WP2-03 attach/poll/age tests; WP2-09 missing-counter / capture-exception / restore-failure.

## Wave 0 Requirements

- [ ] Broker identity-change / stale-validity tests
- [ ] F-06 remaining three negatives
- [ ] Autostart help-string assert (flags 5, not carousel)
- [ ] Recorder rotation / write-fail test

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Live SS-03 replay | OBS-06 | Needs device + current proof assets | Deferred; do not fake panels |

**Approval:** pending
