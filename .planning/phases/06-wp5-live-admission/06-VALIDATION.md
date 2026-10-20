---
phase: 6
slug: wp5-live-admission
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 6 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Host palette/PDM tests + bound target runners |
| **Quick run command** | `python3 -u scripts/test_palette_runtime.py && python3 scripts/test_pdm_sensitivity.py && python3 scripts/test_pdm_sincrng.py` |
| **Full suite command** | Quick plus repaired `run_mode32_real_audio.py` observation (not acceptance) and existing 45 s combined preflight on the **named** image |
| **Estimated runtime** | Host ~2 min; target preflight ~45 s plus programming |

## Sampling Rate

- Host after every source change
- Target only on the named integration candidate after WP4 admission
- Do not extend soak without a stated remaining risk

## Wave 0 Requirements

- [ ] Autostart assert mode 32 / flags 5 / not carousel
- [ ] Brightness recorded from actual boot (working tree fixture uses brightness 24, not 255)
- [ ] `last_live_valid_` zero-time and wrap tests
- [ ] Both-rail observation negatives (F-06 must already be green)

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Acoustic identity | WP5-05 | Controlled observation | Else `ACOUSTIC_IDENTITY_UNRESOLVED` |
| Optical smoothness | WP5-09 | 8 Hz dump insufficient | Full-cadence/optical evidence |

**Approval:** pending
