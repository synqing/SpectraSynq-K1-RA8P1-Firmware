# Phase 3: WP2 host hygiene and observation - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §4 F-04, F-05, F-06; §5 WP2)

<domain>
## Phase Boundary

Host identity/staleness/ownership, telemetry units, log rotation, F-06 runner, palette CLI help, and F-05 metadata alignment. No speculative LED flash.

</domain>

<decisions>
## Implementation Decisions

### Do not redo
- Gold-extract path-based import, scheduler LED2 linkage, SConscript missing-K1 guard, bounded GPT mutations.

### Expected-build vs recognised device
- `identity_ok()` today checks protocol, UID, nonempty build/source, parseable contract — not the selected checkpoint.
- Arbitrary `superseded-unbound-build` and `new-unverified-build` both returned `(True, 'ok')` for the expected UID at the audited commit.
- Qualification must bind accepted build/application checkpoint. Observation of another build may be reported as unaccepted.

### Units
- `titan_snapshot.py` currently assigns `last_emit_cycles` to `hop_max_us` — that is not AP-hop timing.
- `run_mode32_real_audio.py` `hop_max` is sample peak amplitude, not execution time.
- Renderer frame count ≠ completed transmission.

### F-06
- Observation ≠ acceptance.
- Reuse `run_colour_integrity.py` ownership/restoration.
- Force of version-1 mode-32 settings and brightness 255 without restore is a defect.
- `authored_stats()` treating frames shorter than 480 bytes as black is a defect.
- Restore settings on every path including exceptions.

### F-05
- Dated corrections, not rewriting historical outcomes.
- Do not untrack the historical PCM1808 stage tree in this phase.

### Claude's Discretion
- Exact Python APIs for expected-build vs identity_ok split, provided states remain distinct.
- Timeout values for remaining unbounded compile/run in `test_palette_runtime.py`.

</decisions>

<specifics>
## Specific Ideas

Archived broker log SHA-256 `ba47ecba0bf61a6b3aae8218e9cfac0e637bd3d143efb60b8b75d4fd752d478d` must be preserved, not recreated.

Observability must answer the five question tables in brief WP2 (audio health, cadence, transmission, visual state, run trust).

</specifics>

<code_context>
## Existing Code Insights

- `tools/serial-studio/titan_snapshot.py`, `titan_broker.py`
- Untracked `tools/serial-studio/tests/test_identity_checkpoint.py` may already exist — verify against WP2-02/03
- Untracked `tests/host/test_mode32_real_audio_observation.py` may already exist — verify against WP2-09

</code_context>

<deferred>
## Deferred Ideas

Live SS-03 replay on device (needs hardware + WP5 image). Historian closes only when current proof assets support it.

</deferred>
