# WP0 checkpoint — 2026-09-20 (afternoon correction)

Dated live checkpoint. This file is the receipt the STATUS row points at. It does not ship firmware and does not admit the transmitter.

## Authority

1. Current Captain order: live-runtime un-deferred; analyser is a separate track.
2. `AGENTS.md` (M85 AP+VP, Rearm protocol, no DualMCU/BSP edits, no sibling worktree).
3. Platform memory `--check`: `valid_records`, `records_sha256=fd316def0e4e9ac5ebff549de48c306442fe968b8a758ff00f9b6127c9366ea0`, `hardware_validated=false`.
4. Historical takeover brief left unchanged.

## Checkout

| Field | Value |
|-------|--------|
| Branch | `lane/k1-ra8p1-002` |
| HEAD | `431140853dd8b58af53240ef84d5fb1a08bd8b45` |
| Upstream | same SHA |
| Dirty | 27 modified tracked files, 15 untracked (including `.planning/`, live-runtime plan, takeover brief, host tests). **Not reset.** |
| DualMCU pin | `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` (`REFERENCE_DUALMCU=PASS`, `ancestor=1`; sibling current `cc90ea6bfa2dd43f490f6bd2d5d508f75e70248d`) |
| BSP pin | `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7` (`REFERENCE_TITAN_BSP=PASS`, current equals pin) |

## CDC

Titan `045b:5310` is enumerated at `/dev/cu.usbmodem00000000000011`.

`scripts/check_cdc_owner.py` exited 2 (`CDC_OWNED_OR_UNKNOWN`):

- `lsof` on both `cu` and `tty` aliases timed out (2 s). That is fail-closed, not proof of a live holder.
- Lock file `tools/serial-studio/.locks/cdc-tty.usbmodem00000000000011.lock` still says `owner=ss03-info-once pid=18445`. **PID 18445 is absent.** The lock is stale.
- No `titan_broker.py` or `programme_scalar.py` process.
- Serial was **not** opened. The stale lock was **not** unlinked. No owner was killed.

**INFO this session: BLOCKED.** Reused hashed `takeover-info-once-20260920-01/handoff-info.json` only.

## Application identity (reused hashed INFO)

File SHA-256 `c3fb0fe01fdd185073e9637165c6de0a4eec66641019d412d4f8dacb575d0dbb` (matches the 20 September claim).

| Field | Value |
|-------|--------|
| uid | `545433931bd25436593630352d068363` |
| build | `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` |
| source | `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` |
| contract | `sr24000.hop180.bins80.xover40` |
| cpu1_actcsr | `0` |
| u55_opened | `false` |
| clock_hz | `1000000000` |
| `identity_identified` | True / `identified` |
| `identity_ok(..., checkpoint=None)` | False / `no accepted checkpoint bound` |

USB `045b:5310` is not identity. WRITE_VERIFIED of HEX `3aa09139…` is programme evidence, not a live INFO bind this afternoon.

## Recovery identity (two assets, both hashed from files)

| Role | Full SHA-256 | Path |
|------|----------------|------|
| Programmer safety fuse | `a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a` | `EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex` |
| Intended Titan return / current live-audio HEX | `3aa0913950c5815c28c3ae06b37ee4462c5e2537adc738517cccd8b97b60eabd` | `live-audio-gpt-20260920-03/rtthread.hex` |
| Matching INFO build | `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` | same image receipt `f2808973dab087b2664385fcfcb7167cacf025307c59bd3f88ace69449a7f546` |
| ELF | `355626f10ee96bf8db4feee2642e7d62b50ff9a71f8c12fd47c78595f21ae374` | `live-audio-gpt-20260920-03` artifacts |

Neither prefix alone is a choice. The fuse is **not** the Titan return image.

## Named timing asset

`g4-uncapped-raw-hops-build-20260919-02` is **PRESENT** (planning-time glob was empty; execute found the directory).

- receipt.json SHA-256 `15bc804a3ee72a3163015d68175f64ea5df18222b0ee1e390e7faa541adfe574`
- build_id `32dd1f5f6ba784dadc1b42b82f94e86d928d289fc969730e1cb1e0f0c22bc062`
- HEX `9f12869a807f87b3081bc635cb945af9bfceb80c290595df55f75ec5f7fedd85`
- Scope: **DTCM profiler**. Must not be labelled empty-TCM. Cannot close Q1–Q5 empty-TCM.
- `g4-uncapped-raw-hops-run-*`: **ABSENT**
- Empty-TCM profiler remains built: HEX `22d405c1d45871c45c0bfa5b88fc1868c79198de493efeef900a47f89cb81a8b`, build `8a78961b5ad0e821f7f20e13cc093b37e32b3be4643995ee12e4ccbb5a6878bd`

## Opcode 22

NOT EXTRACTED this phase. Broker FORBIDDEN_OPS includes 22. No on-disk v2/1420 witness hashed under `docs/evidence/K1-RA8P1-002/` this afternoon. Empty remains v1/728; LE 2 remains v2/1420. No image change.

## Host-test bind (orchestrator re-run)

Do not restart these campaigns while the listed file SHAs are unchanged.

| Suite | Result |
|-------|--------|
| `test_score_p601_capture` + `test_gpt_fault_witness` | 12 OK |
| `score_p601_capture.py --self-test` | PASS, `waveform=NOT_CAPTURED` |
| Serial Studio identity/broker/decode (pytest) | 13 passed |
| `test_tempo_placement` | 5 OK |
| `test_mode32_real_audio_observation` | 2 OK |
| `test_colour_integrity` (including restore-error CDC release) | 8 OK |

See `HOST-NEGATIVES-REMAINING.md`. Palette host compile (~6 min) was run by a subagent; not re-run this afternoon. Physical PASS is not claimed.

## Programmer SOP (01-02)

Dry-run **without** `--execute`:

- build: `live-audio-gpt-20260920-03`
- output: `…/wp7-dry-live-audio-20260920-01` (consumed; never reuse)
- receipt: `dry_run=true`, `pass=true`, HEX `3aa09139…`

Live waiter command prepared for unused `live-audio-gpt-prog-20260920-04`. **Hold — no Rearm this checkpoint.**

## Separate tracks (not blocked by this file)

- Q1–Q5: needs Rearm of empty-TCM images, not the analyser.
- P601/DIN: BLOCKED on attached analyser.
- Live K1 runtime: **in-scope**; transmitter admission is not a prerequisite.

## Completion language

This checkpoint is a complete **handoff of current facts**. BLOCKED CDC INFO and BLOCKED P601 do not mean the port is complete and do not mean firmware is shipped.
