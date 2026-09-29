<!-- BEGIN DEFAULT BRIGHTNESS MANDATE 2026-09-29 -->
**Latest user directive, 29 September 2026:** Normal default LED brightness must
remain **100/255 minimum**. Current boot and audition default is **100/255**.
This supersedes all earlier 24/255 brightness ceilings and default instructions
in this workspace, skills and historical handovers. Do not silently restore24
when arming emission, changing mode, reconnecting a host or rebuilding firmware.
Explicit operator dimming and emission-off controls remain available; this is a
minimum DEFAULT, not a prohibition on requested dimming or fault shutdown.
Preserve the corrected continuous160 / DIN80+80 mapping and autonomous music.
Evidence: docs/effects-language-001/evidence/captain-brightness100-20260929-01/.
<!-- END DEFAULT BRIGHTNESS MANDATE 2026-09-29 -->

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

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
