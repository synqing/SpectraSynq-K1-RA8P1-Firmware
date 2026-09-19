# Titan Mini × Serial Studio Pro

Serial Studio Pro is the instrument panel. One host process owns Titan CDC (`045b:5310`). Serial Studio never opens the USB serial device.

## SS-02 (accepted, do not reopen)

Offline fixtures and Historian replay. Project `titan.ssproj` SHA-256 `b9a8562e4259a741bb9e2261ee6826d9df73d85d72ce01e57f8237d4848d3a84`. Receipts in `ss-02-proofs/`. Archived copy: `docs/evidence/K1-RA8P1-002/ss-03-live-observability/ss-02-accepted/`.

## SS-03 live observability

Broker: TCP `127.0.0.1:7778`, newline CSV, native comma-delimited parser. Live project: `titan-live.ssproj`.

### Host tests (no Titan open)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/tools/serial-studio
python3 tests/test_ss03_decode.py
python3 tests/test_ss03_transport.py
python3 tests/test_ss03_lock.py
python3 tests/test_ss03_broker.py
python3 prove_ss03.py --host-only
```

### Start / status / stop / handoff

```bash
python3 titan_broker.py run --evidence /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ss-03-live-observability/broker-live
python3 titan_broker.py status
python3 titan_broker.py stop
python3 titan_broker.py info-once --evidence /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ss-03-live-observability/handoff
```

Replay-only (no serial open or write):

```bash
python3 titan_broker.py run --replay-only --evidence /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ss-03-live-observability/broker-replay-only
```

Live dashboard: Serial Studio Pro, open `titan-live.ssproj`, Network TCP client `127.0.0.1:7778`, newline framing. Do not point this project at CDC.

Full campaign (starts the owned Serial Studio instance if needed):

```bash
python3 prove_ss03.py --run-id run --baseline-seconds 60 --dashboard-seconds 300
```

Admitted hardware polls: opcode 1 once per attach, then 1 Hz staggered opcode 6 and 17 when the identified image answers them. Opcodes 2–5, 7–14, 16, 18–19, 22 are refused.

**CDC close hazard:** on this Mac, closing the application CDC handle left Titan on `045b:0261` RA USB Boot. The broker must stay open across Serial Studio restart. Do not close/reopen CDC to “recover”. Do not flash from that event. Application return is a RESET without USER/BOOT.

## Do not

- Share CDC with Serial Studio or a second runner.
- Send ASCII into the binary protocol.
- Flash, reset, change gain, start firmware tests, or submit pixels for prettier plots.
- Treat a green dashboard as a G4 pass.
- Overwrite `titan.ssproj` or SS-02 receipts.
