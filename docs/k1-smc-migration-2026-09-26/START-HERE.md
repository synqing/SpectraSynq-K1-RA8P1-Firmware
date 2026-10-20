# Start here — SMC-Mixer migration supersedes the old surface-first order

Read `GROK-SMC-MIGRATION-ADDENDUM.md` first. The immediate implementation lane is SMC-Mixer → USB Master → Mac host adapter → single-owner Titan broker → non-resetting partial parameter updates. K1.Sliders and DualMCU are outside this assignment's write scope.

This is a migration/precedence addendum, not new firmware or target authorisation. It preserves the complete parent programme and previous defect evidence; it does not mark retired-surface tasks complete.

## Reading order

1. `GROK-SMC-MIGRATION-ADDENDUM.md` — controlling scope, current source correction, continuation tasks, acceptance and STOP/GO.
2. `evidence/SOURCE-RECHECK.md` — what was actually reread, current source hashes, newer first path and evidence limits.
3. In the existing SMC worktree: full `docs/execution/SMC-TITAN-CONTROL-001.md`, `...-DISPATCH.md` and `...-GATES.md`. These raw files are not bundled; their exact location is in the addendum.
4. `reference/Pasted text(20260925-182715).txt` — preserved older SMC handback; source has advanced beyond some missing-feature rows.
5. `reference/K1-Implementation-Handoff.md` — full original technical programme, with the addendum's explicit applicability changes.

`reference/previous-direction/` preserves the earlier 2.41-directed briefs and source recheck. Read them as historical defect evidence, not the active task order. Do not resume them merely because they were written first or say “P1-CLOSE”.

## Current first path

The latest reread profile selects physical fader 7 (pitchwheel channel 6, zero-based) → `visual_a.vp_waveform_shift_rate` in Titan effect mode 8. The old fader-6/hue example is not the current selected first test. A live mode change is not authorised by this statement; revalidate the applicable scene and admission.

The existing host source already contains a live CLI, a rate loop and Unix IPC. Verify and finish these components rather than recreating them. They were not executed for this package.

## Send this to Grok

Execute GROK-SMC-MIGRATION-ADDENDUM.md under existing SMC-TITAN-CONTROL-001. It supersedes the earlier K1.Sliders P1-CLOSE critical path. Do not edit/flash K1.Sliders or DualMCU. Continue in the existing SMC and Titan feature worktrees. USB ingress is SINCO SMC-Mixer-Master only. Read the fresh source recheck: live CLI/Unix server now exist, and the selected first path is fader 7 / mode-8 motion, not the old hue example. Preserve all previous evidence and full programme obligations with explicit scope dispositions. Complete the real broker/adapter lifecycle, application identity and uncertainty handling, frame-safe PATCH integration and candidate build; then perform the admitted one-fader target session, expand only demonstrated controls, and add acknowledged-look save/recall. Respect actual existing hardware holds; continue offline work while held. Do not report simulated ACKs, schema-writable fields or software dumps as physical completion.

## Package validation

`SHA256SUMS.json` lists every other file in this package. It proves packaged-byte identity only, not behavioural correctness or target admission.
