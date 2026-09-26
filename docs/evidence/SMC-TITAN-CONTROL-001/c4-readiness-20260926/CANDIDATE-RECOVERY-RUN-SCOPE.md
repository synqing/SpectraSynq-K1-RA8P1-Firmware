---
abstract: "C4 admission packet for SMC-TITAN-CONTROL-001: candidate addc00a2, recovery 980eb3a9 held emit-on, run-scope fader 7 mode 8 emit-off. Not an accepted checkpoint and not a flash grant."
---

# C4 candidate / recovery / run-scope packet

Prepared 26 September 2026. Status: C4 readiness for the existing admission process. Hardware remains held. This file is not `accepted-checkpoint.json`.

C3 HOST_TEST stays closed. This packet does not reopen C3.

## 1. Candidate to assess — reuse, do not rebuild, do not install yet

| Field | Value |
|---|---|
| Directory | `docs/evidence/SMC-TITAN-CONTROL-001/offline-arm-build-20260926-r3/` |
| `build_id` | `addc00a22b67d3db2e010c96e66db81cdb8a209c9c9d193dcdaadb98d5d5e3a3` |
| ELF SHA-256 | `62859ec129e9367309df0a7decf4c57844c4b4b0017234a5d788cf145953758d` (file MATCH) |
| HEX SHA-256 | `ca3c671e9b7a647b425d99715e23e85bf67b0b32e8ffab22f44415a232ba3b93` (file MATCH) |
| Schema | `e3dca88cecbba56e562a01c18d8c02c0240f813dfbd438c7d185a739515c0b57` (present in ELF) |
| Defaults | `titan-live-defaults-smc-control-mode8-v1.json` `d1998017c3fb8263483f4b57191f293189dddfa08b11dabea3796d8b21641581` |
| Declared sources | 120, as in the r3 `receipt.json` `sources` map |
| Mode / emit | boot mode 8; `K1_LIVE_EMIT` absent; `live_emit` false |
| Pair | P601/GPT6/DMAC2 + P604/GPT8/DMAC3 |
| ELF strings | `P604` present, `P603` absent |
| Linked | `liveApplyPatch`, `liveNotePatchAccepted`, `liveNoteRendererConsumed` |
| Compiler | Arm GNU Toolchain 13.3.Rel1 `arm-none-eabi-g++` 13.3.1 |
| DualMCU pin (derivative, not identity) | `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` |
| BSP pin | `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7` |
| Size | text 259580, data 19864, bss 375380 |

Applicable: this schema matches the C3-closed host contract `e3dca88c`. Wiring in `host-repair-p604-20260924/BINDING.json` is already P601/P604, so this image follows the bound pins.

Do not install `1afde371…` (mode 8, schema `05198c7a`, older). Do not install `72ee7bc4…` (mode 32, P603). Do not downgrade the schema to those older receipts. Do not rebuild automatically.

Later dirty worktree schema `af180050…` and directory `offline-arm-build-20260926-r4` (`51b73cdb…`) exist. They are a later overlay. They are not this packet’s candidate.

A future admitted host/broker must present schema `e3dca88c` and build `addc00a2`. A dirty host at `af180050` would deny this image. That is a stop, not a reason to rebuild in this close.

## 2. Recovery image — locate, assess, do not auto-restore emit-on

| Field | Value |
|---|---|
| Archive | `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/swarm-20260922/integration/PIN12-P604-20260923/image/` |
| Programme receipt | `…/programme-rearm-20260923T1207/receipt.json` (`pass` true, rfp-cli SWD, probe power off) |
| `build_id` | `980eb3a9962e570437d7e1b6d7bae733b3a742284e4d45765742675409c90abe` |
| HEX | `029c582bad9adb78abb1c486ca9d02b080183d8420cd5c930ad5dea866f9c62f` (file MATCH) |
| ELF | `3141cfd0f4503b405ee0f2de98d63a36512d93883695e37ba3013a5b0ea6774b` (file MATCH) |
| Schema | `a77dbf9298dbfb4385623dcb66d4c41037a60c6b65dc734bb7a521449f9d0e09` |
| UID | `545433931bd25436593630352d068363` |
| Pins | P601 / P604 |
| Preflight 24 Sep | `emit_on`: 1, `mode_a`: 32, `runtime_kind`: live |
| Image receipt `live_emit` | true |

This is the saved P604 resident/recovery archive. Restoring it would bring back an emitting mode-32 show. That is a separate, later admission. It is **not** automatic recovery inside this emit-off-only session.

If the emit-off candidate write or probe fails: stop. Leave-state stays whatever the board then holds. Do not Rearm `980eb3a9…` to “put the show back” unless Captain names that HEX, emit-on, and mode 32 in a new admission.

## 3. Target resource configuration

| Resource | This candidate | Bound bench |
|---|---|---|
| DIN A | P601 GPT6 DMA2 | BINDING `din_a` P601 |
| DIN B | P604 GPT8 DMA3 | BINDING `din_b` P604 |
| Profile | pair profile 3, `GPT6\|GPT8` | keep current wiring |
| Audio | PDM target on, PCM1808 off, dcache disabled | unchanged |
| Emission | boot off; PATCH must not arm | hold |
| PATCH path | opcode 25 subcommand 8; no SET/BEGIN/APPEND/COMMIT | C3 host already refuses whole-config |
| `PaletteRuntime::configure` | symbol exists in ELF for boot; scalar PATCH must not call it | source review; runtime scorer watches emit/mode/revision |

Do not move a wire to match P603 images.

## 4. Run scope for the first admitted sequence

Input: `SINCO SMC-Mixer-Master` only.

Field: fader 7, pitchwheel channel 6, mode 8, `visual_a.vp_waveform_shift_rate`, range 0–240.

Ops: `patch`, `get_config`, `snapshot`. No clocks, pins, DMA, audio capture, safety limits, or emit-arm.

Two tests, in order, after a genuine admission and rfp-cli write of `addc00a2` HEX:

1. Direct protocol probe of decoded requests 40.0 and 200.0. Score apply + consume on snapshot receipts. Emit stays 0. Unrelated AP/history/values stay. Exact 40/200 are protocol values, not fader steps.
2. Physical fader from the real acknowledged start. Requested = `240 * coarse / 127`. Nearby grid points are about 39.6850 and 200.3150. Compare acknowledged to the float32-encoded request.

Emit-off can close control/application. Light needs a later optical admission.

## 5. Image-bound scorer

Command:

```
python3 tools/serial-studio/score_smc_live_control.py \
  --identity docs/evidence/SMC-TITAN-CONTROL-001/c4-readiness-20260926/CANDIDATE-IDENTITY.json \
  --snapshot <snapshot.json> --origin target|host_fake \
  --expect-request-id N --expect-value V --applied-value V
```

Host tests: `python3 tools/serial-studio/tests/test_c4_scorer.py` → `C4_SCORER_PASS`.

The scorer fails closed on wrong UID/build/schema, `emit_on != 0`, mode other than 8, apply without matching request/revision, consume not bound to the accepted request, and a target hop that is only `hop_sequence+1`. FakeSerial consume remains `host_model`. Optical is always `not_in_scope`.

## 6. Acceptance and stop (for a future Captain admission)

Accept only if: rfp-cli verifies HEX `ca3c671e…` on UID `545433931bd25436593630352d068363`; INFO build is `addc00a2…`; schema is `e3dca88c…`; emit 0; mode 8; scorer apply+consume PASS on the protocol probe; no 980eb3a9 auto-restore.

Stop if: identity drifts; emit arms; mode leaves 8; host schema is not `e3dca88c`; a second CDC writer appears; PATCH uses whole-config; operator flashes `1afde371` or `72ee7bc4`; recovery HEX is programmed without a new emit-on admission.

Programming route after genuine admission only: E2 Lite `5AS079228B`, SWD 1 MHz, probe power off, `-noquery -run`, range `02000000,02C9F01F`, no GDB, no `programme_scalar.py`.

## 7. Evidence classes still open

| Layer | Status |
|---|---|
| SOURCE_REVIEW | Candidate and recovery hashes re-derived; resource pins reconciled |
| HOST_TEST | C3 remains closed; C4 scorer negatives pass |
| TARGET_BUILD | `addc00a2` reused; not rebuilt |
| TARGET_RUNTIME | Open. No CDC, PATCH on silicon, or board readback |
| OPTICAL | Open |

---
**Document Changelog**
| Date | Author | Change |
|------|--------|--------|
| 2026-09-26 | agent:grok | Packed addc00a2 candidate, 980eb3a9 emit-on recovery hold, and image-bound scorer for C4 readiness. |
