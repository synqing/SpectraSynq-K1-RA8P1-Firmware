# SDD ledger — plan: docs/Titan_Live_Runtime_Acceptance_and_MIR_Execution_Brief_2026-09-20.md

Ruling: no sibling worktree — repo contract forbids it. Work in the existing SpectraSynq-K1-RA8P1-Firmware checkout.
Ruling: no commit unless Captain asks — user git rule.
Ruling: CDC/programming one owner (this lead). Subagents may write host/firmware source but must not open CDC or flash.
Ruling: do not Rearm unless a replacement image must be programmed. Current fde39fec remains the running identity until a new named build exists and Captain Rearms.
Ruling: emit stays false for this campaign.

Preflight: opcodes 1,3,4,6,7,8,11,12,15-22 occupied; 23-26 free. Assign 23 snapshot, 24 events, 25 config, 26 timing.

Ruling: failed ARM directory live-k1-runtime-build-20260920-03 is unused forever; replacement is live-k1-runtime-build-20260920-04 — fixture_app live_handle_config had to move after respond/error.
Ruling: silent 24 kHz PCM is a permitted digital fixture, not named music. G06 stays FAIL until Captain names a real recording.
Ruling: C–F on silicon wait on Rearm. Host G00–G05 are not a silicon campaign pass.

WP A: complete on live-k1-runtime-run-20260920-02 (bind, CDC-OWNER, CAPABILITY-RECORD, CONFIG-SNAPSHOT, SCHEMA).
WP B: host surfaces + firmware opcodes 23–26 + ARM 04 + programme dry-run.
WP C–F: BLOCKED Rearm.
WP G: host silent replay/compare PASS; named music FAIL.
WP H: FEATURE-COVERAGE + LIVE-RUNTIME-HANDBACK-2026-09-20.md

