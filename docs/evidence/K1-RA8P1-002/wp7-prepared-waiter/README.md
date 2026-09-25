# WP7 prepared waiter — hold

Default: **hold**. Artefacts exist. No flash this checkpoint.

On Rearm: start `COMMAND.sh` first, before USB enumeration, rebuild, or plan recap. First chat line is exactly `WAITING`. Relay `ROM_SEEN`, `ROM_IDENTIFIED_WRITING`, `WRITE_VERIFIED` from `events.jsonl`. Do not invent `PROGRAMME_VERIFY_PASS`. Never reuse the dry-run directory or the live `--output` path.

After WRITE_VERIFIED: “Release USER and BOOT, then RESET” once only if Captain has not already done it. Then `BOUND-RUNNER.txt`.

This waiter returns to `live-audio-gpt-20260920-03` (HEX `3aa09139…`). Empty-TCM Q1–Q5 uses a different prepared command in `Q1-Q5-CAMPAIGN.md`. A future live-runtime candidate uses a new unused programme directory, not `…-prog-20260920-04` after that directory has been used.

A timed-out waiter that wrote nothing is not failed firmware execution. No alternative programmer.
