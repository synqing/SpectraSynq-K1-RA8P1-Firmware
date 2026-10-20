THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

<!-- BEGIN TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->
**Captain directive — 27 September 2026; supersedes conflicting earlier operating instructions below.**

Titan must boot with visual output ON and run continuous, autonomous, audio-reactive music visualisation whenever powered and the application is running. Normal output must not wait for a host, broker, camera, recording session, approval, or diagnostic window. Old boot-off requirements and automatic timed disarm at the end of diagnostic/camera/host sessions are superseded. A diagnostic timeout ends the diagnostic activity; it must not gate or stop normal music visualisation. Do not invent another permission gate for this already-authorised behaviour. Put this mandate first in every new Titan-facing agent instruction, plan and handover.

Preserve the current maximum brightness **24/255**, mode/control constraints, Titan UID **545433931bd25436593630352d068363**, **P601/GPT6 and P604/GPT8**, microphone **DMA1/2**, and LED **DMA0/3** unless Captain separately changes them. Report genuine hardware faults immediately and truthfully; do not hide faults or claim repaired/continuous optical output from software counters alone. This instruction does not promise physically uninterrupted light during power loss, programming or reset: disclose unavoidable interruption and restore autonomous music visualisation when the application returns. Preserve existing programming identity checks and probe target-power OFF. Historical observations, receipts and original plan assignments below remain historical evidence; conflicting boot-off, timed-disarm or normal-output permission clauses are not current authority.
<!-- END TITAN CONTINUOUS MUSIC MANDATE 2026-09-27 -->

# Live K1 runtime — handback 20 September 2026

The running board still has the earlier live image. A replacement image with the new observation opcodes is built, hashed and dry-run programmed. It has not been flashed.

## What is true now

Work package A is bound. The current board identity is the 20 September live-runtime image whose INFO build starts `fde39fec`. That image does not speak the new snapshot, events, config or timing opcodes, so the long development campaign cannot pass on it.

Work package B is in source and in a new ARM image named `live-k1-runtime-build-20260920-04`. Host tests for clock, resampler, live owner, wire codec, lease exclusivity and scorer negatives are green. Offline replay of a silent 24 kHz fixture is repeatable and the comparator catches mutated, missing and wrong inputs. Named recorded music was not found, so the music-intelligence baseline is not claimed.

Work packages C to F need the replacement image on the board. Flashing waits on Captain saying Rearm.

Physical light remains off. No commit was made.

## What is left

1. Captain: say **Rearm**.
2. Agent: start the prepared waiter first, then reply exactly `WAITING`. Programme directory `live-k1-runtime-prog-20260920-02` (fresh; do not reuse the dry-run directory).
3. After write verified and reset: bind INFO to build `99e70c74…`, then a 5-second smoke, then the 13-minute campaign with emit off.
4. Score that campaign. Development-ready is only true if every D-row is PASS.
5. Captain: supply a named real recording if G06 must pass. Silent digital fixtures are already proven on the host path.

## Identities

| Role | Path / hash |
|---|---|
| Running image | `live-k1-runtime-build-20260920-02` build `fde39fec…` HEX `79d98d54…` |
| Replacement | `live-k1-runtime-build-20260920-04` build `99e70c74d978560353423a3ff478072545bc0e9370cd36bbdbae50359e3fd0b1` HEX `158439e5ee364d19146d00d836402e23ae1e3e1337df464cf7e2bd2d47d27b9b` |
| Failed unused compile | `live-k1-runtime-build-20260920-03` — do not reuse |
| Dry-run programme | `live-k1-runtime-prog-dry-20260920-01` pass |
| Fresh programme (Rearm) | `live-k1-runtime-prog-20260920-02` must not exist yet |
| Fresh campaign run | `live-k1-runtime-run-20260920-03` |
| Host G | `live-k1-runtime-host-g-20260920-01` G00–G05 PASS, G06 FAIL |
| Schema | `docs/contracts/titan-live-v1.json` SHA-256 `12a22efe…` |
| UID | `545433931bd25436593630352d068363` |

## Rearm command

```sh
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-build-20260920-04 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/live-k1-runtime-prog-20260920-02 \
  --wait-seconds 180 --execute
```

## Rulings

- No sibling worktree.
- No flash without Rearm.
- No commit unless asked.
- Failed build-03 is never reused; 04 is the replacement.
- Silent PCM is a permitted digital fixture, not named music.
- D17 PASS means the scorer wrote rows; it does not make DEV_READY true over NOT_TESTED rows.
