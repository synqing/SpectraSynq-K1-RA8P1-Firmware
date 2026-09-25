---
abstract: "C3 host/broker closeout for SMC-TITAN-CONTROL-001. HOST_TEST only. Not an accepted checkpoint and not a programming authorisation."
---

# C3 host close — 26 September 2026

Status label: offline components implemented; target image built earlier; C3 transaction/identity gates closed on the simulated Adapter/Unix/Broker/Transport path; target application and optics not tested.

This file is HOST_TEST evidence. It is not `accepted-checkpoint.json`, not a flash record, and not optical proof.

## Before / after

| Case | Before | After |
|---|---|---|
| C3-A same-target external 100 with queued 240 | `before-fail.log`: `assert applied == []` failed; fake target overwritten to 240 | `test_c3a_same_target_external_edit_does_not_overwrite` keeps 100, `patch_applies` unchanged, pickup rearmed |
| C3-A unrelated revision | host `test_r2_1` / `test_stale_reply_keeps_final_gesture` red after first resync attempt | both pass; newest eligible 240 is retried without a new gesture |
| C3-B schema via `Broker.poll_once()` | control-plane identity stayed old; further PATCH accepted | INFO poll rotates session, `accepted=False`, zero backend PATCH |
| C3-C lost PATCH reply after apply | not closed on the real stack | apply count stays 1; second flush confirms via `last_patch_request_id` |
| C3-D approval without schema/ops or NaN expiry | schema/ops were skippable when absent | denied; competing records and wrong field scope submit zero PATCH |

## Commands and exits (HOST_TEST)

```
python3 tests/test_c3_close.py                         # 0  C3_CLOSE_PASS
python3 host/titan_control/run_tests.py                # 0  36 tests
python3 tests/test_titan_control_server.py             # 0  TITAN_CONTROL_SERVER_PASS
python3 tests/test_live_control_path.py                # 0  LIVE_CONTROL_PATH_PASS
python3 scripts/test_live_protocol.py                  # 0  LIVE_PROTOCOL_HOST_PASS
```

## Source hashes after repair

| Path | SHA-256 |
|---|---|
| `smc_titan_control/adapter.py` | `9fdc4e51a7dbfe2a04dcbd7cce001446f23c7bb95a36e40913589626416346c6` |
| `smc_titan_control/broker_client.py` | `7dccbaa7e274856ab86f1222939d2c6ac74a285f92fa26578869bab613d54212` |
| `titan_broker.py` | `6d150a649fcaff8b469a66953ede03f817018c3004fc9f33cfac37610c18e476` |
| `titan_control_server.py` | `5124b93bc7d9c91521c7e287589c5b146d95ee195f91745a84bfa61a0b94e530` |
| `titan_snapshot.py` | `d5804f49e2867f535d3ea70f1314cdd35d3f7b1ab3ca88c5ba60066971a54441` |
| `titan_live_device.py` | `fc7b0ecd0ae6ca28f463dcff6fc47c65e1da03c757f7a7f084f31858476c4b01` |
| `tests/test_c3_close.py` | `514903143a80cff53e87230d24ae61f5030cec8c50cebd8594a673f106f89460` |

Worktree HEADs at close: SMC `4935fff005768655f5800106616f7a0e86feee6a`; Titan `8427fccfb1c3b7fface2c2ba0a56a6017b785acf`. HEAD is not the candidate image.

Host Python: 3.14.7. Tests use the in-tree packages; no new pip install.

Current host contract digest (`docs/contracts/titan-live-v1.json`): `e3dca88cecbba56e562a01c18d8c02c0240f813dfbd438c7d185a739515c0b57`.

## Candidate selection (TARGET_BUILD, not programmed)

| Disposition | `build_id` | Mode / second lane | ELF | HEX | Schema in that receipt |
|---|---|---|---|---|---|
| Assess (later mode-8 emit-off) | `1afde371457324f6349226b9b0d71082a10da340e18e7a6817a1fbd1b115f421` | 8 / P604 GPT8 | `4019fb0897db5a73975986393f6f680a9e058502c395405e427d963a5ce84914` | `3a747235b323a08d997df1a368f573c7d8c26cbd55aeaaed8aa4d6331656e045` | `05198c7a0ad56c06a9ed29b13c8845a8a10da90d3dbff0f68098d2aa5d70f817` |
| Superseded / obsolete default | `72ee7bc44716d73cea7ba6040f59a8a15263990ba472c2e6fecb6a7e6488e2e0` | 32 / P603 GPT7 | `c2ff04a67c8c40ca6161bac38bd8310f86c1cc620cec0b5ef1a3d9d5843eca4e` | `4df453cf8cf3793b52c3b63021b1c57a0e4426ca1925ee737de8ba9ae6e0b3e5` | `05198c7a…` |

`1afde371…` is the configuration to assess. It is not permission to install it. It does not contain these C3 host repairs. Current dirty firmware protocol digest `e3dca88c…` differs from that image's `05198c7a…`; a protocol/default change needs a new TARGET_BUILD before any future write.

UID for a future admission: `545433931bd25436593630352d068363`. Input: `SINCO SMC-Mixer-Master` only. First field: fader 7 / pitchwheel channel 6 / mode 8 / `visual_a.vp_waveform_shift_rate` / emit-off. Scoped ops: `patch`, `get_config`, `snapshot`.

Recovery image: not named by this C3 host close. Keep the board's existing recovery/leave-state. Do not treat `1afde371…` or `72ee7bc4…` as recovery.

## Remaining gates

| Layer | Status |
|---|---|
| SOURCE_REVIEW | Host/broker C3-A–D repaired in the owned worktrees |
| HOST_TEST | Pass on FakeSerial Unix/Broker/Transport path |
| TARGET_BUILD | `1afde371…` exists PRE-SILICON; schema older than current dirty host contract |
| TARGET_RUNTIME | Open. No CDC attach, PATCH on silicon, or readback on the board |
| OPTICAL | Open. Emit-off cannot prove light |

Hardware leave-state: unchanged. No programming, reset, CDC open, emit-arm or fixture run.

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-26 | agent:grok | Recorded C3 HOST_TEST close and candidate dispositions without admitting hardware. |
