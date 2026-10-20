# Phase 2: WP1 capture gate - Research

**Researched:** 2026-09-20
**Domain:** P601/DIN capture scoring, provenance vs conformance
**Confidence:** HIGH on working-tree scorer field names (orchestrator grep). NOT_VERIFIED on unittest green.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Split outcomes: `capture_present`, `raw_files_verified`, `frame_correlated`, `waveform_conforms`, `first_divergence`.
- Synthetic/parser tests never create physical qualification.
- Historic `a3f37e8a` frame 2030 is immutable witness, not the only executable contract.
- Predicted bench wire: 128×24=3072 bits; DMA bits-2=3070 plus two preloaded words; P601=GTIOC6A.
- Payload equality is not correlation for all-black traffic.
- F-01 reproduction must not return physical PASS.
- Unit tests must not require a local-Mac witness in `setUpClass()`.

### Claude's Discretion
- Manifest JSON field names if WP1-02 contents are present.
- Correlation method: instrument trigger, segmented indexing, or reviewed marker with verified pin ownership. No invented spare pin.

### Deferred Ideas
- Armed live capture (WP4). This phase only makes the gate honest.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Research support |
|----|------------------|
| WP1-01 | Working tree `score_p601_capture.py` has `--admit-current-witness` and split fields. Plans must copy those names, not re-invent a second scorer. |
| WP1-02 | Manifest + SHA-256 of **actual** files. Still open if no manifest schema test exists. |
| WP1-03 | Dual-probe decode vs populated-part limits + instrument resolution. |
| WP1-04 | Correlation beyond payload hash. |
| WP1-05 | Distinct fields already present in working tree (`capture_present`, `waveform_conforms`, `physical_claims`, `first_divergence`). `qualified = raw_files_verified and frame_correlated and waveform_conforms`. |
| WP1-06 | Negatives listed in brief. Orchestrator confirmed `physical_claims` defaults False in several paths. Must still encode F-01 tamper case as executable test. |
| WP1-07 | Check `tests/host/test_score_p601_capture.py` `setUpClass` — remove Mac receipt requirement from unit tests. |
| WP1-08 | Protocol must describe a **new** run/boot/frame identity. |

</phase_requirements>

## Summary

At audited commit `4311408`, `complete_synthetic(..., source='physical')` with illegal pulse widths returned physical success (brief §10). The **working tree already splits fields** and has `--admit-current-witness`. Plans must: (1) copy those APIs, (2) add remaining negatives (manifest, non-hex hash, NaN, 3073 bits, clipped pulse, correlation), (3) prove F-01 is rejected, (4) keep unit tests Mac-free. No wire PASS.

**Primary recommendation:** Keep one scorer with split outcomes. Do not add a parallel “physical stamp” boolean that ORs parser success with `source='physical'`.

## Copy-ready contracts

Orchestrator-verified in working tree `scripts/score_p601_capture.py`:

- Fields: `physical_claims`, `capture_present`, `waveform_conforms`, `first_divergence`
- CLI: `--admit-current-witness`
- Qualification: `qualified = raw_files_verified and frame_correlated and waveform_conforms`

F-01 challenge (must FAIL physical qualification after repair) — copy from brief §10; do not treat as physical evidence.

Host commands:

```sh
python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness -v
python3 scripts/score_p601_capture.py --self-test
```

## Common Pitfalls

- Copying HEAD `4311408` scorer (F-01 false PASS) instead of working-tree split
- Stamping `WAVEFORM_CAPTURED` from synthetics
- Requiring frame 2030 as the only valid frame id
- Asking Captain to look at the plate (`instrument-not-captain-eyes`)
- Inventing a spare correlation pin

## Validation Architecture

| Property | Value |
|----------|-------|
| Framework | Python 3 unittest |
| Quick | `python3 -m unittest tests.host.test_score_p601_capture -v` |
| Full | that plus `test_gpt_fault_witness` and `--self-test` |

| Req ID | Test | Exists |
|--------|------|--------|
| WP1-06 F-01 | synthetic physical tamper rejected | must exist or Wave 0 |
| WP1-07 | tests run without external receipt | check setUpClass |
| WP1-02 | missing/tampered raw file | Wave 0 if missing |
| WP1-06 3071/3073 | bit count negatives | Wave 0 if missing |

### Wave 0
- [ ] Mac-free unit tests
- [ ] Manifest SHA-256 mismatch test
- [ ] Non-hex hash, NaN timings, 3071/3073, clipped pulse, insufficient reset
- [ ] F-01 reproduction as failing physical qualification

## Sources

- Working-tree `scripts/score_p601_capture.py` (grep 2026-09-20)
- `tests/host/test_score_p601_capture.py`, `scripts/gpt_fault_witness.py`
- Brief §4 F-01/F-02, §5 WP1, §10
- SSA R-WP0-WP1-WP7 (RESEARCH files were not persisted; claims not consumed as verified)

## RESEARCH COMPLETE

**Phase:** 2
**Copy:** split score fields; `--admit-current-witness`; unittest module `tests.host.test_score_p601_capture`
**Gaps:** remaining negatives and Mac-free setUpClass must be confirmed on execute, not assumed from STATUS “PASS host”.
