---
phase: 5
slug: wp4-led-fault
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 5 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Python host driver tests + colour-integrity runner + capture scorer |
| **Quick run command** | `python3 scripts/test_ws281x_gpt_dma_hw.py && python3 scripts/test_ws281x_gpt_dma_mutations.py` |
| **Full suite command** | Quick plus Phase 2 scorer tests plus `run_colour_integrity.py --phase-s 20` on a **fresh** dir when instrument+image authorised |
| **Estimated runtime** | Host ~60 s; each colour phase 20 s plus programming |

## Sampling Rate

- Per driver patch: mutation suite must go red for the measured defect
- Never reset/retry inside the scorer
- Optical is a separate row

## Per-Task Verification Map

| Task ID | Requirement | Test | Exists |
|---------|-------------|------|--------|
| 5-01 | WP4-01–03 | Rearm checkpoint + capture arm checklist | protocol |
| 5-02 | WP4-05–07 | first_divergence + defect-specific negative | Wave 0 after measurement |
| 5-03 | WP4-08–10 | three controls `--phase-s 20` | run_colour_integrity.py ✅ |

## Wave 0 Requirements

- [ ] Ready-to-run acquisition task if no analyser (WP4-11) — BLOCKED is valid
- [ ] Defect-specific mutation **after** first divergence is known — do not invent it now
- [ ] Confirm `incomplete_dma_waiver` still rejected; never restore `DMCRA <= 2`

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Probe attachment | WP4-03 | Physical | P601 and first-LED DIN armed before reproduce |
| Optical KEEP | WP4-10 | Distinct boundary | Dump/score first; Captain product taste only if asked |

**Approval:** pending
