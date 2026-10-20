# Phase 4: WP3 uncapped timing - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (brief §4 F-03; §5 WP3; `docs/BRIEF-2026-09-10-uncapped-ap-timing.md`)

<domain>
## Phase Boundary

Name empty-TCM vs DTCM configurations, collect or recover 6,000 raw hops, answer Q1–Q5 or blockers, keep the hop-136 delay as a negative. No inherited combined/product PASS.

</domain>

<decisions>
## Implementation Decisions

### F-03
- Timing brief requires empty ITCM/DTCM.
- `build_scalar.py` applies `apply_tempo_dtcm_overlay()` whenever `--resident-controls` is provided; `--stage-profile` requires those controls; overlay places two ACF arrays in `.dtcm`.
- Overlay-order regression intentionally preserves profiling and DTCM.
- A DTCM run answers questions about that configuration only.
- `run_scalar_schedule.py` decodes raw version 2 but writes `K1T1.v1` — fix the label.

### Do not rewrite
- Raw timing support already exists. Compact `Distribution<8000>` is not the defect.

### Frozen inputs
- Retain the same frozen input bytes. Do not regenerate both sides' goldens. Do not re-run the entire old corpus for document edits.

### Mutation
- Known 5 ms delay at hop 136 on the 6,000-hop fixture.
- Host one-hop fixture uses hop 0 — do not conflate.

### Image change
- If resident image must change, prepare under WP7 (Phase 1 SOP). Do not consume an uncaptured new LED fault by switching back to the profiler carelessly.
- STATUS claims empty-TCM HEX `22d405c1…` built; resident still live-audio `c7f6034a…`. Verify hashes; do not expand abbreviated hashes by guessing.

### Claude's Discretion
- Flag/name for the empty-TCM build choice, provided it cannot be confused with the DTCM default.

</decisions>

<specifics>
## Specific Ideas

```sh
python3 scripts/run_scalar_schedule.py \
  --build "$TITAN_PROFILE_BUILD_DIR" \
  --resident "$TITAN_RESIDENT_FIXTURE_DIR" \
  --profile "$TITAN_FROZEN_PROFILE_FILE" \
  --output "$TITAN_NEW_PROFILE_RUN_DIR" \
  --loops 1 --mode scalar
```

Fresh directory with `--timing-mutation` for the negative. Never reuse receipt directories. `--qualification` is a different gate.

Q1–Q5: uncapped tempo/ordinary distributions; labelled miss cross-tab; measured stage attribution; dynamic double emulation calls/cost; matched observer overhead.

</specifics>

<code_context>
## Existing Code Insights

- `fixture_app.cpp`, `stage_probe.h`, `k1_double_probe.*`, `stage_profile.py`, `tempo_dtcm_overlay.py`, `build_scalar.py`, `run_scalar_schedule.py`
- Untracked `tests/host/test_tempo_placement.py` and `docs/evidence/K1-RA8P1-002/Q1-Q5-CAMPAIGN.md` — verify, do not assume complete

</code_context>

<deferred>
## Deferred Ideas

Combined live timing (WP5), K1-L (Phase 8).

</deferred>
