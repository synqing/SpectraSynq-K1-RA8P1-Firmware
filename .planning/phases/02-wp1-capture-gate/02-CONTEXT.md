# Phase 2: WP1 capture gate - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §4 F-01, F-02; §5 WP1)

<domain>
## Phase Boundary

Repair `scripts/score_p601_capture.py`, `scripts/gpt_fault_witness.py`, `tests/host/test_score_p601_capture.py`, and `P601-DIN-CAPTURE-PROTOCOL.md` so a new acquisition can be scored honestly. No wire PASS in this phase.

</domain>

<decisions>
## Implementation Decisions

### Outcomes must split
- Distinct: `capture_present`, `raw_files_verified`, `frame_correlated`, `waveform_conforms`, `first_divergence`.
- One ambiguous `pass` flag is forbidden.
- Synthetic/parser tests never create physical qualification.
- Valid capture of bad pulses is retained failing evidence.

### New acquisition, old witness immutable
- Historic `a3f37e8a` frame 2030 cannot be recovered by replaying saved bytes.
- Preserve-resident hold was waived; that is not a wire-capture waiver.
- Reusing payload or seeing frame 2030 after another boot does not prove the same event.

### Predicted wire (bench)
- 128 × 24 = 3072 bits. DMA `bits-2` = 3070 plus two preloaded words.
- Pin P601 = GTIOC6A. Probe MCU pad and first LED DIN.
- All-black traffic repeats: payload equality is not correlation.

### Negatives required
- Missing/tampered raw file, non-hex hash, synthetic relabelling as physical, wrong build/session/frame, missing probe, invalid/non-finite timing, payload mismatch, wrong profile, 3071/3073 bits, clipped final pulse, insufficient reset.
- F-01 reproduction at audited commit must fail as physical qualification after repair.
- `setUpClass()` must not require an external Mac receipt for unit tests.

### Claude's Discretion
- Manifest JSON schema field names, provided required contents from WP1-02 are present.
- Correlation method among: instrument trigger, segmented frame indexing, or explicitly reviewed low-overhead marker with verified pin ownership. Do not invent a spare pin.

</decisions>

<specifics>
## Specific Ideas

F-01 reproduction (creates synthetic objects, no physical evidence) is in brief §10. After repair it must not return `pass=True` / `WAVEFORM_CAPTURED` / `physical_claims=True` / `first_divergence=None` for the tampered synthetic.

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scripts/score_p601_capture.py` (dirty tree may already have partial repairs — verify, do not assume)
- `scripts/gpt_fault_witness.py`
- Colour-integration immutable v2 diagnostic

### Established Patterns
- Host mutations already reject false completion — keep them
- Instrument-not-Captain-eyes: dump/score, do not ask Captain to look at the plate

</code_context>

<deferred>
## Deferred Ideas

Armed live capture is WP4. This phase only makes the gate executable and honest.

</deferred>
