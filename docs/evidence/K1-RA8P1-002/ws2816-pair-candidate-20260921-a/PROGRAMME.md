# WS2816-PAIR-CANDIDATE-20260921-A — programme command

Validated with `scripts/programme_scalar.py` help and a dry-run (no `--execute`).
Dry-run receipt: `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programme-ws2816-pair-dry-20260921-01` pass.

Do not run `--execute` until Captain says Rearm, and only after wiring confirmation.

## Candidate

- Identity: `fa54c7818a3fdf87344514854141dc06f7d000f5727e91b23a5fcc4245fe46b6`
- HEX: `f4d40e9f7ad2733ec2ba1a70122c795801ab29f9b43a4981148b9602c3b756e7`
- UID: `545433931bd25436593630352d068363`
- Backend: ws2816_gpt_pair, profile 3, emit off, brightness 24

## Rearm command (fresh output directory)

```sh
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ws2816-pair-candidate-20260921-a \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programme-ws2816-pair-20260921-01
```

`--execute` is added only by the Rearm waiter path. CDC exclusive ownership is inside `programme_scalar.py`: it quiesces the control-surface server, claims the lock, programmes, rebinds application INFO, then resumes the server. A reminder is not the handoff.

## Recovery command (WS2812 wiring restored first)

See `docs/evidence/K1-RA8P1-002/WS2816-PAIR-RECOVERY-AND-WIRING-20260921.md`. Resident HEX `cbcadb41…` from `live-k1-runtime-build-20260921-01`. Do not flash that HEX onto a WS2816 stick.
