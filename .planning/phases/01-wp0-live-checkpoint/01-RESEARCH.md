# Phase 1: WP0 checkpoint and WP7 programming SOP - Research

**Researched:** 2026-09-20
**Domain:** Titan live identity, CDC ownership, named timing receipts, Rearm waiter
**Confidence:** HIGH on programmer/INFO APIs (orchestrator-read). MEDIUM on STATUS receipt hashes (indexed, bytes not rehashed). NOT_VERIFIED on live CDC/INFO this planning session.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Authority order: AGENTS.md → platform memory `--check` → STATUS newest dated entry → colour integration → mic/LED2 canon → remaining plan → latency decision → uncapped brief → EXECUTION-BRIEF.
- DualMCU pin `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`. BSP pin `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`.
- Preserve unrelated dirty work. No `git add -A`. No sibling worktree. Do not kill unknown CDC owner.
- UID `545433931bd25436593630352d068363`. USB `045b:5310` is not identity.
- On Rearm: start waiter first, reply exactly `WAITING`. Consume `events.jsonl`. Never reuse programme directories.
- Programming success ≠ application startup, peripheral transaction, register state, or light.
- STATUS 20 September identity PASS is a **hypothesis** until receipts are rehashed.

### Claude's Discretion
- STATUS dated-entry wording if unknown fields stay unknown.
- Prepared waiter command-file location: a **new** unused evidence directory.

### Deferred Ideas
- WP1–WP6 implementation, latency campaign, architecture ruling, parked M33/U55/NPU.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Research support |
|----|------------------|
| WP0-01 | Run `python3 …/platform_memory.py --check` then `--topic pins` / `dma` / `ws2816`. Suggested git commands in brief §5 WP0. Dirty tree already exists — inventory, do not reset. |
| WP0-02 | `titan_broker.py` uses `cdc_lock.acquire`. `programme_scalar.py` opens ROM via Lab `titan_ra8p1_boot`. Do not kill unknown owners. `lsof`/`ps` for `titan_broker` / `programme_scalar` before opening CDC. |
| WP0-03 | Named asset `g4-uncapped-raw-hops-build-20260919-02`. Orchestrator glob of `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/**/g4-uncapped*` returned **zero files**. Plan must search then record ABSENT, not reconstruct campaigns. |
| WP0-04 | Copy `python3 tools/serial-studio/titan_broker.py info-once --evidence <fresh-dir>`. Opcode 1 INFO. `identity_identified` ≠ `identity_ok(..., checkpoint=)`. |
| WP0-05 | Opcode 22 v2 snapshot before any image change. Empty opcode-22 = v1/728 B; LE version 2 = 1420 B. |
| WP0-06 | One dated STATUS entry + `external-receipts.json` current fields. Unknown stays unknown. |
| WP0-07 | `bash scripts/check_references.sh` — pin ancestry only; dirty DualMCU/BSP is not a reset licence. |
| WP7-01–05 | Copy `scripts/programme_scalar.py` flags and event names below. Output dir `mkdir(..., exist_ok=False)`. |

</phase_requirements>

## Summary

Phase 1 is observe-and-prepare. The working tree already has a 20 September STATUS identity row (live build `c7f6034a…`, UID matched, broker claimed absent). Plans must **re-read and hash** the named receipts rather than copying STATUS prose. The first researcher for this phase did not persist RESEARCH.md; this file is the orchestrator replacement after verifying `programme_scalar.py` and `titan_broker.py info-once`.

**Primary recommendation:** Treat `takeover-info-once-20260920-01` as identified-not-accepted. Prepare a **fresh** programme directory and waiter command file. Do not flash in this phase unless Captain issues Rearm after those artefacts exist.

## Standard Stack

| Artefact | Purpose |
|----------|---------|
| `scripts/programme_scalar.py` | UID-gated ROM programme wrapper |
| `tools/serial-studio/titan_broker.py info-once` | Opcode-1 application identity |
| `docs/evidence/K1-RA8P1-002/STATUS.md` | Concise current index |
| Lab `titan_ra8p1_boot` at `/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/scripts` | Programmer backend (read, do not develop) |

## Copy-ready contracts

### Programmer CLI

```sh
python3 scripts/programme_scalar.py \
  --build "$TITAN_FINAL_BUILD_DIR" \
  --output "$TITAN_NEW_PROGRAMME_DIR" \
  --wait-seconds 180 --execute
```

Dry run: omit `--execute` (receipt `dry_run=True`). `--output` must not already exist (`exist_ok=False`).

Hard-coded UID in `programme_scalar.py`: `545433931bd25436593630352d068363`.
App USB wait uses VID/PID `(0x045b, 0x5310)` and sets `identity_verified=False`.

### events.jsonl names (not `PROGRAMME_VERIFY_PASS`)

`WAITING_FOR_IDENTIFIED_ROM` → `ROM_SEEN` → `ROM_IDENTIFIED_WRITING` → `WRITE_VERIFIED` or `PROGRAMME_FAILED`.
Optional `APP_CDC_SEEN` if `--wait-app-seconds > 0`. Application identity is a **separate** INFO bind.

### INFO CLI

```sh
python3 tools/serial-studio/titan_broker.py info-once --evidence "$FRESH_HANDOFF_DIR"
```

### STATUS hypothesis to re-verify

| Claim | Pointer | Orchestrator check |
|-------|---------|-------------------|
| Identity identified | `takeover-info-once-20260920-01/handoff-info.json` sha256 `c3fb0fe01fdd185073e9637165c6de0a4eec66641019d412d4f8dacb575d0dbb` | Recompute SHA-256 of the actual file; do not expand abbreviated hashes by guessing |
| Live build | `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` | Must match INFO `build` field |
| Accepted | NOT TESTED | Do not promote identified → accepted |
| Named profiler receipt | `g4-uncapped-raw-hops-build-20260919-02` | Glob at planning time returned zero under the external evidence root — record ABSENT or bind if found on execute |

### Restore image (programmer)

`programme_scalar.py` checks known-good restore HEX sha256 `a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a` at EdgeAI_Artifacts `p3-2026-09-09/build-staged-v2/rtthread.hex`. Plans must verify that file still hashes before announcing ready.

### Retired image

HEX `42bd1d6d8f2ece9882ec3cfbd64778535cdd1e33aa10b04c024549f3bc2ee75e` is refused as known-failed GPT candidate.

## Common Pitfalls

- Treating USB enumeration or `WRITE_VERIFIED` as app identity
- Reusing a programme/run directory
- Killing a CDC owner that is not proven stale
- Resetting dirty host repairs that already implement F-01/F-04/F-06
- Expanding `32dd1f5f…` / `9f12869a…` by guessing missing hex digits
- Replying with a plan instead of `WAITING` after Rearm once the waiter is prepared
- Inventing `PROGRAMME_VERIFY_PASS` — that string is not in `programme_scalar.py`

## Validation Architecture

| Property | Value |
|----------|-------|
| Framework | Shell + Python 3 hashlib / unittest |
| Quick run | `git status --short; git rev-parse HEAD; python3 -c "from pathlib import Path; print(Path('scripts/programme_scalar.py').exists())"` |
| Full | Rehash STATUS-cited receipt files; `bash scripts/check_references.sh`; platform_memory `--check` |

| Req ID | Automated command | File exists |
|--------|-------------------|-------------|
| WP0-01 | `platform_memory.py --check` | ✅ skill scripts |
| WP0-02 | process/CDC inventory script (Wave 0 if missing) | ❌ leave as execute task |
| WP0-03 | named-path glob + sha256 | search may be empty — that is a valid BLOCKED |
| WP0-04 | `titan_broker.py info-once` | ✅ |
| WP7-01 | dry-run `programme_scalar.py` without `--execute` | ✅ |

### Wave 0
- [ ] Small host script that refuses to open CDC if lock file / broker PID held
- [ ] Receipt rehash checklist file template (not filled with guessed hashes)

## Sources

- `scripts/programme_scalar.py` (orchestrator read, 2026-09-20)
- `tools/serial-studio/titan_broker.py` `info-once` / `attach`
- `docs/evidence/K1-RA8P1-002/STATUS.md`, `external-receipts.json`
- Takeover brief §5 WP0 / WP7
- SSA R-WP0-WP1-WP7 return (files missing; claims treated as hypotheses)

## RESEARCH COMPLETE

**Phase:** 1
**Copy:** `programme_scalar.py --build --output --wait-seconds 180 --execute`; events `WAITING_FOR_IDENTIFIED_ROM`…`WRITE_VERIFIED`; `titan_broker.py info-once`
**Gaps:** Named g4-uncapped glob empty at plan time; live CDC unread this session; first researcher did not persist this file.
