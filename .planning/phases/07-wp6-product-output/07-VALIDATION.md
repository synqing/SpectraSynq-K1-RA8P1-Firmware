---
phase: 7
slug: wp6-product-output
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-09-20
---

# Phase 7 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Host composition tests (`scripts/test_output_composition.py`, `tests/host/test_output_composition.cpp`) |
| **Quick run command** | `python3 scripts/test_output_composition.py` |
| **Full suite command** | Quick plus C++ host composition test if the tree's usual wrapper is used |
| **Estimated runtime** | ~30 seconds host |

## Wave 0 Requirements

- [ ] `0x12AB` TRUE16 vector already cited in research — keep and extend both channels
- [ ] Admission into `import-slices.json` **without** silently rewriting DualMCU pin
- [ ] Physical WS2816/multi-lane status token required even if BLOCKED/NOT TESTED

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| P603 / second edge | WP6-06 | Captain physical approval | Do not implement the move |
| Four concurrent lanes | WP6-05 | Hardware topology | BLOCKED until authorised |

**Approval:** pending
