# Phase 3: WP2 host hygiene and observation - Research

**Researched:** 2026-09-20
**Domain:** Titan Serial Studio broker identity, telemetry units, F-06 real-audio observation runner, F-05 metadata
**Confidence:** HIGH on copy-ready APIs (working-tree source). MEDIUM on dirty-vs-HEAD classification (source vs audited commit `4311408`, git porcelain not re-run in this SSA). NOT_VERIFIED on test pass/fail (tests scored by source, not executed).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Gold-extract path-based import, scheduler LED2 linkage, SConscript missing-K1 guard, bounded GPT mutations.

- `identity_ok()` today checks protocol, UID, nonempty build/source, parseable contract — not the selected checkpoint.
- Arbitrary `superseded-unbound-build` and `new-unverified-build` both returned `(True, 'ok')` for the expected UID at the audited commit.
- Qualification must bind accepted build/application checkpoint. Observation of another build may be reported as unaccepted.

- `titan_snapshot.py` currently assigns `last_emit_cycles` to `hop_max_us` — that is not AP-hop timing.
- `run_mode32_real_audio.py` `hop_max` is sample peak amplitude, not execution time.
- Renderer frame count ≠ completed transmission.

- Observation ≠ acceptance.
- Reuse `run_colour_integrity.py` ownership/restoration.
- Force of version-1 mode-32 settings and brightness 255 without restore is a defect.
- `authored_stats()` treating frames shorter than 480 bytes as black is a defect.
- Restore settings on every path including exceptions.

- Dated corrections, not rewriting historical outcomes.
- Do not untrack the historical PCM1808 stage tree in this phase.

### Claude's Discretion

- Exact Python APIs for expected-build vs identity_ok split, provided states remain distinct.
- Timeout values for remaining unbounded compile/run in `test_palette_runtime.py`.

### Deferred Ideas (OUT OF SCOPE)

Live SS-03 replay on device (needs hardware + WP5 image). Historian closes only when current proof assets support them.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WP2-01 | Do not re-implement completed gold-extract / LED2 / SConscript / GPT mutations | Treat as already done. No tasks. |
| WP2-02 | Broker qualification binds accepted checkpoint. `identity_ok()` must not return ok for `superseded-unbound-build` / `new-unverified-build` | Copy working-tree `identity_identified` + `identity_ok(checkpoint=)` + `bind_identity`. Untracked `test_identity_checkpoint.py` covers the two strings at function level only. |
| WP2-03 | Invalidate on disconnect, age expiry, restart, identity change. Tests: same-UID build change, stale-to-fresh, rejected opcode, failed cleanup, ownership handoff | `poll_once` already rebinds opcode 1. Age expiry does **not** clear `bound["ok"]`. Required tests are missing. |
| WP2-04 | One CDC owner. Read-only monitoring: no hidden config/flash/reset | Copy `cdc_lock`, `FORBIDDEN_OPS`, `OBSERVE_OPS={1,6,17}`. `test_ss03_transport.test_forbidden_opcode_does_not_write` exists. |
| WP2-05 | Do not map `last_emit_cycles` → `hop_max_us`. Missing measurements stay unavailable | Working tree already stores `last_emit_cycles` separately. Schema-3 `hop_max_us` stays absent. Dashboard still labels `identity_ok` as “Identified”. |
| WP2-06 | Record rotation/limits; preserve archive hash `ba47ecba0bf61a6b3aae8218e9cfac0e637bd3d143efb60b8b75d4fd752d478d` | `REC_MAX=20000`, `REC_MAX_BYTES=32*1024*1024`. Archive path already hashed. Do not recreate the old broker session. |
| WP2-07 | Remaining subprocess timeouts in `test_palette_runtime.py` | Working tree already uses `COMPILE_TIMEOUT=120`, `RUN_TIMEOUT=60` on ordinary compile/run. Verify; do not invent a second timeout scheme. |
| WP2-08 | `--palette-autostart` help is live mode 32, flags 5, not carousel | `build_scalar.py` help string already matches. Align tests/boot receipts; do not change behaviour to a carousel. |
| WP2-09 | F-06 observation runner: exact length, both rails, restore every path, honest failure. Negatives: short frame, missing counters, capture exception, restoration failure | Runner source already repaired. Untracked observation test covers **only** the short-frame negative. |
| WP2-10 | F-05 docs/metadata; no PCM1808 untrack | Several current-image fields already corrected. `CONTINUATION-LEDGER.md` historical “now resident” paragraph remains; keep dated correction, do not rewrite outcomes. |
| OBS-01 | Live audio health fields | Reuse opcode-6 `pdm_target`. Do not invent RMS/dBFS (`rms_unavailable()`). |
| OBS-02 | Cadence fields | AP hop compute is **not** `last_emit_cycles`. Leave `hop_max_us` UNAVAILABLE until AP owns it. |
| OBS-03 | Transmission counts separately | Palette `emitted` is latched/emit count, not DMA-complete. Opcode-22 GPT witness is a different runner, not broker observe-only. |
| OBS-04 | Visual state | Opcode-17 `musical`, `visual_path`, `effect_frames`, frame CRCs. Sparse USB dump ≠ 120 Hz smoothness. |
| OBS-05 | Run trust | Distinct identified vs accepted. Epoch, age, CDC owner, live vs replay. |
| OBS-06 | Historian replay closes only with current proof assets | Deferred. Do not shrink dashboard to four generic values. |
</phase_requirements>

## Summary

Phase 3 is host-only hygiene. The audited commit `431140853dd8b58af53240ef84d5fb1a08bd8b45` really did accept unbound build strings and mapped emitter DWT onto `hop_max_us`. The **working tree already contains the intended API split**. Plans must copy those signatures, land or complete their negatives, and must not re-invent a second identity function or a second real-audio runner.

Untracked tests do **not** close the brief. `test_identity_checkpoint.py` covers WP2-02 at the pure-function layer. It does not cover WP2-03 attach/poll/age-invalidation. `test_mode32_real_audio_observation.py` covers the 479-byte frame only. Missing-counter, capture-exception, and restore-failure negatives are still Wave 0 work.

**Primary recommendation:** Keep `identity_identified` vs `identity_ok(..., checkpoint=)` vs `bind_identity`. Treat recognised UID as observation. Treat `accepted-checkpoint.json` (or `TITAN_ACCEPTED_*`) as qualification. Copy the colour-integrity restore pattern into F-06 tests. Do not flash. Do not call `run_mode32_real_audio.py` an acceptance gate.

## Standard Stack

### Core

| Library / artefact | Version / identity | Purpose | Why Standard |
|--------------------|--------------------|---------|--------------|
| Python 3 unittest + standalone `if __name__` scripts | repo convention; no `pytest.ini` | Host negatives | Existing Serial Studio and `tests/host` runners |
| `tools/serial-studio/titan_snapshot.py` | working tree | Identity + schema-3 snapshot | SS-03 decode path |
| `tools/serial-studio/titan_broker.py` | working tree | One CDC owner, TCP `127.0.0.1:7778` | SS-03 observe/broker |
| `tools/serial-studio/titan_transport.py` | working tree | Observe-only opcodes | Forbids flash/config/reset |
| `scripts/cdc_lock.py` (imported as `cdc_lock`) | working tree | Exclusive CDC | WP2-04 |
| `scripts/run_colour_integrity.py` | working tree | Restore/settings ownership pattern | Brief F-06: copy this, do not hand-roll |
| `scripts/run_mode32_real_audio.py` | working tree | Observation runner | F-06 target |
| `scripts/build_scalar.py` | working tree | `--palette-autostart` help | WP2-08 |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `pyserial` / `serial.tools.list_ports` | already imported | USB enumerate VID:PID `045b:5310` | Live attach only; host tests use `FakeSerial` |
| `tools/serial-studio/titan_decode.py` | schema 3 | CSV cells, `UNAVAILABLE` | Do not invent panel numbers |
| Archived broker log | SHA-256 `ba47ecba0bf61a6b3aae8218e9cfac0e637bd3d143efb60b8b75d4fd752d478d` | WP2-06 preserve | Never recreate |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Working-tree `identity_identified` + `identity_ok(checkpoint=)` | New names (`qualify()`, `device_ok()`) | Discretion allows rename **only if** identified vs accepted stay distinct. Do not rename: tests and `attach()` already call these. |
| `accepted-checkpoint.json` | Env-only `TITAN_ACCEPTED_BUILD` | Env is already the fallback. Keep both. |
| Colour-integrity restore helpers | Ad-hoc opcode 16 in the music runner | Forbidden. Copy `SETTINGS` / `settings_payload` / `check_settings`. |

**Installation:** none. Use in-tree Python 3.

## Architecture Patterns

### Recommended Project Structure

```
tools/serial-studio/
├── titan_snapshot.py          # identity_identified / identity_ok / bind_identity / snapshot
├── titan_broker.py            # attach(), load_checkpoint(), poll_once(), Recorder
├── titan_transport.py         # OBSERVE_OPS, FORBIDDEN_OPS
├── titan_decode.py            # schema-3 CSV; UNAVAILABLE
├── telemetry-schema.json      # slot 8 identity_ok, slot 14 hop_max_us
└── tests/
    ├── test_identity_checkpoint.py   # WP2-02 function negatives (untracked, incomplete vs WP2-03)
    ├── test_ss03_broker.py           # replay, stale age field, freeze; not identity invalidation
    ├── test_ss03_transport.py        # rejected/forbidden opcodes
    ├── test_ss03_lock.py             # second CDC lock
    └── test_ss03_decode.py           # last_emit_cycles is not hop_max_us
scripts/
├── run_mode32_real_audio.py   # observation only
├── run_colour_integrity.py    # restore pattern to copy
└── test_palette_runtime.py    # COMPILE_TIMEOUT=120, RUN_TIMEOUT=60
tests/host/
└── test_mode32_real_audio_observation.py  # short-frame only
docs/evidence/K1-RA8P1-002/
├── external-receipts.json
├── combined-runtime-contract.json
├── titan-onboard-lmd2718-source-contract.json  (repo root: docs/titan-onboard-lmd2718-source-contract.json)
├── 001-002-EVIDENCE-MAP.md
└── ss-03-live-observability/BROKER-LOG-ARCHIVED.md
```

### Pattern 1: Identified vs accepted

**What:** Two predicates. `identity_identified` is UID/protocol/nonempty/parseable contract. `identity_ok` additionally requires a checkpoint dict whose `build` (and optional `source`/`contract`/`uid`) matches.
**When to use:** Broker attach, `info-once`, snapshot `identity_ok` flag, Serial Studio LED.
**Copy-ready signature (working tree):**

```python
# Source: tools/serial-studio/titan_snapshot.py
EXPECTED_UID = "545433931bd25436593630352d068363"
STALE_MS = 2000

def identity_identified(info: dict, expected_uid: str = EXPECTED_UID) -> tuple[bool, str]:
    ...
    return True, "identified"

def identity_ok(
    info: dict,
    expected_uid: str = EXPECTED_UID,
    checkpoint: dict | None = None,
) -> tuple[bool, str]:
    identified, reason = identity_identified(info, expected_uid)
    if not identified:
        return False, reason
    if not isinstance(checkpoint, dict) or not str(checkpoint.get("build") or "").strip():
        return False, "no accepted checkpoint bound"
    if info["build"] != checkpoint["build"]:
        return False, f"build not accepted {info['build']!r}"
    ...
    return True, "ok"

def bind_identity(info, expected_uid=EXPECTED_UID, checkpoint=None) -> dict:
    # keys: ok, identified, accepted, reason, uid, build, source, contract, ...
```

**Attach rule (working tree):** `Broker.attach()` calls `bind_identity`. It **releases hardware and raises** only when `not bound["identified"]`. An identified-unaccepted device stays attached for truthful observation. `cmd_info_once` returns process exit 0 if identified, even when `ok` is false.

**Checkpoint load:**

```python
# Source: tools/serial-studio/titan_broker.py load_checkpoint()
# 1. evidence / "accepted-checkpoint.json" with {"build": "..."} plus optional source/contract/uid
# 2. else TITAN_ACCEPTED_BUILD (+ optional TITAN_ACCEPTED_SOURCE, TITAN_ACCEPTED_CONTRACT)
# 3. else None → identity_ok reason "no accepted checkpoint bound"
```

STATUS 2026-09-20: live INFO build `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` is **identified, not accepted**. Do not write that build into `accepted-checkpoint.json` because the transmitter is not admitted.

### Pattern 2: Colour-integrity restore (copy into F-06)

**What:** Capture opcode-17, arm restore only after identity, restore opcode-16 in `finally`, verify with `check_settings`, always close CDC.
**When to use:** Any runner that mutates settings (`run_mode32_real_audio.py` already follows this in source; tests must prove the exception paths).

Copy these exact names from `scripts/run_colour_integrity.py`:

- `SETTINGS` — `active automatic_cycle emit_enabled palette_a palette_b mode_a mode_b brightness output_channel transition_ms travel_ms direction showcase`
- `settings_payload(settings)` — `struct.pack('<10I', 3, palette_a, palette_b, mode_a, mode_b, flags, brightness, output_channel, transition_ms, travel_ms)`
- flags bits: `active | automatic_cycle<<1 | emit_enabled<<2 | (direction=='edges_in')<<3 | showcase<<4`. Mode 32 active+emit no carousel = flags **5**.
- `--restore-settings` file, else captured pre-test settings
- `restore_armed` only after opcode-1 identity match
- artefacts: `original-settings.json`, `restored-settings.json`, `receipt['restore_verified']` / `restore_error`, `cdc_released`
- Host negative already exists: `tests/host/test_colour_integrity.py::ColourIntegrity.test_capture_error_and_restore_error_still_release_cdc`

Working-tree `run_mode32_real_audio.py` already: transact(1) identity vs `--build-id` + `PIN`; transact(17) capture; `restore_armed=True`; wanted `mode_a=mode_b=32, active=True, automatic_cycle=False, emit_enabled=True, showcase=False`; `finally` restore + `check_settings`; `write_failure` sets `label='OBSERVATION_FAIL'`, `accepted=False`.

### Pattern 3: Telemetry units

**What:** Schema-3 snapshot never fills `hop_max_us` from palette `last_emit_cycles`.
**When to use:** Always.

```python
# Source: tools/serial-studio/titan_snapshot.py snapshot()
# last_emit_cycles → row["last_emit_cycles"] only
# hop_max_us omitted unless AP metrics own it (they currently do not)
# timing_valid stays 0
# mic_a_valid / mic_b_valid stay 0; rms_unavailable() returns None
# latched_frames from palette["emitted"] if present else palette["frames"] — still not DMA-complete
```

Existing decode negative: `tools/serial-studio/tests/test_ss03_decode.py::test_last_emit_cycles_is_not_hop_compute`.

### Anti-Patterns to Avoid

- **Treating recognised UID as qualified build:** `identity_identified` True is not `identity_ok` True.
- **Mapping `last_emit_cycles` to `hop_max_us`:** emitter DWT, not AP hop.
- **Calling `run_mode32_real_audio.py` an acceptance gate:** receipt `accepted` is always False; label `OBSERVATION` / `OBSERVATION_FAIL`.
- **Inventing firmware numbers for Serial Studio panels:** missing fields stay `UNAVAILABLE`. Do not synthesise RMS, hop_max_us, or DMA-complete from palette frames.
- **Short RGB frames as black/silence:** `authored_stats` must raise `ValueError('incomplete native RGB frame: …')` when `len(frame) != 480`.
- **Dashboard LED labelled “Identified” bound to `identity_ok`:** that slot is now accepted-checkpoint. Relabel; expose `identity_identified` separately.
- **Live unaccepted device as `SCOPE_UNIDENTIFIED`:** `snapshot()` currently forces `SCOPE_UNIDENTIFIED` when `not ok`. Discretion: keep identified-unaccepted visible (uid/build already published when `identified`). Do not hide the build string.
- **Re-implementing gold-extract / LED2 / SConscript / GPT mutations.**
- **Untracking PCM1808 stage tree.**
- **Rewriting historical STATUS/ledger outcomes** instead of a dated correction.
- **Recreating the 1.8 GB broker log.**

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| CDC exclusive open | New lock files | `cdc_lock.acquire(device, OWNER)` OWNER=`ss03-broker` | Second lock must fail (`test_ss03_lock.py`) |
| Settings restore | Ad-hoc JSON | `SETTINGS` + `settings_payload` + `check_settings` | Colour-integrity already has capture+restore+CDC-release negatives |
| Expected-build gate | New `qualify()` | `identity_ok(..., checkpoint=)` | Untracked test and `attach()` already use it |
| Observe vs mutate | Broker opcode 16/7/22 | `FORBIDDEN_OPS = {2,3,4,5,7,8,9,10,11,12,13,14,16,18,19,22}` | Hidden config/flash/reset is WP2-04 fail |
| Hop compute panel | Convert emit cycles / 1000 | Leave `hop_max_us` absent | Units lie otherwise |
| Real-audio acceptance | New scorer in the observation runner | Keep `accepted: False`; WP5 uses the repaired runner later | F-06 |

**Key insight:** The defect at `4311408` was “recognised UID ⇒ ok”. The working tree already split that. Phase 3 work is **negatives + remaining invalidation + docs**, not a new identity architecture.

## Common Pitfalls

### Pitfall 1: Scoring untracked tests as WP2-02/03/09 complete

**What goes wrong:** Planner skips Wave 0 because files exist.
**Why it happens:** CONTEXT.md says they “may already exist”.
**How to avoid:** Score below. Default NOT_VERIFIED until orchestrator re-runs the commands.
**Warning signs:** Test file has two functions and a `print("…_ok")`.

| File | WP2-02 | WP2-03 | WP2-09 |
|------|--------|--------|--------|
| `tools/serial-studio/tests/test_identity_checkpoint.py` | **Partial PASS at function layer:** unbound strings → `identity_identified` True, `identity_ok()` False reason `no accepted checkpoint bound`; checkpoint mismatch `not accepted`; `bind_identity` keeps `build` visible with `accepted` False | **FAIL:** no attach, no poll identity_change, no age expiry invalidation, no disconnect, no failed Recorder cleanup, no ownership handoff | n/a |
| `tools/serial-studio/tests/test_ss03_broker.py` | no | **Partial:** `device_age_ms >= 2000` after 3 s, freeze flag; does not clear `bound["ok"]` | n/a |
| `tools/serial-studio/tests/test_ss03_transport.py` | no | **Partial:** forbidden opcode does not write (rejected opcode) | n/a |
| `tools/serial-studio/tests/test_ss03_lock.py` | no | **Partial:** second lock fails (ownership) | n/a |
| `tests/host/test_mode32_real_audio_observation.py` | n/a | n/a | **FAIL vs brief list:** only 479-byte frame + exact-length zeros. Missing: `require_counters`, capture exception, restoration failure, `accepted=False` failure receipt, hop_max-not-time |

### Pitfall 2: Age field mistaken for binding invalidation

**What goes wrong:** WP2-03 “age expiry” is marked done because `device_age_ms` advances.
**Why it happens:** `current_row()` reports age; `bound` stays `ok=True`.
**How to avoid:** On age > `STALE_MS` (2000), disconnect, restart, or identity change: set metric validity flags false (`timing_valid`/`rate_valid`/`led_valid`), bump `epoch` or clear `bound["ok"]`, require rebind before `SCOPE_CURRENT`. Test stale-to-fresh recovery.

### Pitfall 3: Forcing brightness 255 / version-1 settings without restore

**What goes wrong:** Live image left in a test configuration.
**Why it happens:** Audited F-06 runner did that.
**How to avoid:** Working-tree runner no longer forces brightness 255; it overlays mode 32 + flags 5 onto captured settings. Tests must still inject restore failure.

### Pitfall 4: CLI help vs behaviour drift on `--palette-autostart`

**What goes wrong:** Help says carousel; firmware boots mode 32 flags 5.
**How to avoid:** Copy current help: `'boot live WaveformK1 mode 32 with active+emit (flags 5); not a catalogue carousel'`. `docs/PALETTES.md` already says not carousel. Keep `K1_PALETTE_AUTOSTART=1`.

## Code Examples

### Broker CLI flags (copy)

```sh
# Source: tools/serial-studio/titan_broker.py main()
python3 tools/serial-studio/titan_broker.py run --evidence "$EVIDENCE_DIR"
python3 tools/serial-studio/titan_broker.py run --evidence "$EVIDENCE_DIR" --replay-only
python3 tools/serial-studio/titan_broker.py status
python3 tools/serial-studio/titan_broker.py stop
python3 tools/serial-studio/titan_broker.py info-once --evidence "$EVIDENCE_DIR"
```

Constants: `BIND=("127.0.0.1", 7778)`, `PUBLISH_HZ=5.0`, `POLL_HZ=1.0`, `OWNER="ss03-broker"`, poll ops 1 then optionally 6 and 17.

### Real-audio CLI flags (copy)

```sh
# Source: scripts/run_mode32_real_audio.py
# Observation, not acceptance. Fresh --output (exist_ok=False).
python3 scripts/run_mode32_real_audio.py \
  --output "$TITAN_NEW_REAL_AUDIO_DIR" \
  --build-id "$FULL_BUILD_ID" \
  --audio "$EXISTING_RECORDING" \
  --play-s 20 --after-s 8 --volume 0.5 --period-s 0.15
```

There is **no** `--qualification` on this runner. `NATIVE_RGB_BYTES=480`. `REQUIRED_PDM=('ap_hops','paired_slots','measured_hz','rate_locked','last_hop_peak')`. Peak field is `last_hop_peak`, not `hop_max`. Both rail signs: `sat_pos`/`sat_neg` per lane plus `gain_clip_pos`/`gain_clip_neg`.

### Colour-integrity restore CLI (copy for F-06 tests / later WP4)

```sh
python3 scripts/run_colour_integrity.py \
  --build "$TITAN_COLOUR_BUILD_DIR" \
  --output "$TITAN_NEW_COLOUR_RUN_DIR" \
  --phase-s 20 \
  --restore-settings "$TITAN_VERIFIED_RESTORE_SETTINGS_FILE"
```

### Palette timeouts (discretion: keep these)

```python
# Source: scripts/test_palette_runtime.py
COMPILE_TIMEOUT = 120
RUN_TIMEOUT = 60
# mutation subprocess.run(..., timeout=30)
```

WP2-07 at audited commit claimed ordinary compile/run were unbounded. Working tree already bounds them. Plan task = prove by running `python3 -u scripts/test_palette_runtime.py`, not rewrite.

### Recorder limits (WP2-06)

```python
REC_MAX = 20000
REC_MAX_BYTES = 32 * 1024 * 1024
# rotate: path.name + "." + %Y%m%dT%H%M%S
# write OSError → recorder.failed = True → Broker.loop sets stop → cmd_run returns 2
```

## Dirty-tree deltas plans must copy (working tree vs brief F-04/F-06)

Inspect these paths before editing. Do not reset unrelated dirty work. Do not assume HEAD still matches the F-04 reproduction.

| Path | Working-tree delta vs audited F-04/F-06 | Plan action |
|------|------------------------------------------|-------------|
| `tools/serial-studio/titan_snapshot.py` | `identity_identified`; `identity_ok(..., checkpoint=)`; `bind_identity`; `last_emit_cycles` not assigned to `hop_max_us` | Land + add remaining negatives. Do not revert to nonempty-string ok. |
| `tools/serial-studio/titan_broker.py` | `load_checkpoint`; `attach` uses `bind_identity`; poll opcode 1 rebind + `identity_change` epoch; `info-once` splits identified vs ok | Keep. Add WP2-03 tests around `poll_once` / Recorder / lock. |
| `scripts/run_mode32_real_audio.py` | Observation label; exact 480; `require_counters`; both rails; restore in `finally`; `write_failure` | Keep. Extend untracked test to the four brief negatives. |
| `scripts/build_scalar.py` | Autostart help already “mode 32, flags 5; not a catalogue carousel” | Align remaining help/tests/boot receipts only. |
| `tools/serial-studio/tests/test_identity_checkpoint.py` | Untracked | Keep as WP2-02 seed; **not** WP2-03. |
| `tests/host/test_mode32_real_audio_observation.py` | Untracked | Keep as short-frame seed; **not** WP2-09 complete. |
| `docs/evidence/K1-RA8P1-002/external-receipts.json` | Current board already `c7f6034a…` / HEX `3aa09139…` / image `live-audio-gpt-20260920-03` | Do not restore `f0205ef8` / `led-build-04` as current. |
| `docs/titan-onboard-lmd2718-source-contract.json` | `sincrng_register`: 10 | Keep. |
| `docs/evidence/K1-RA8P1-002/combined-runtime-contract.json` | GPT silicon note already mentions three terminal underruns | Keep DMA map labelled default PDM-first. |
| `docs/evidence/K1-RA8P1-002/001-002-EVIDENCE-MAP.md` | Table of thirteen deliverables exists | Fill remaining honest limitations; do not create empty 001 docs. |
| `docs/evidence/K1-RA8P1-002/CONTINUATION-LEDGER.md` | Dated 2026-09-20 correction exists; body still says historical “now resident” | Dated correction only. D1 still says “zero completions” — reconcile with colour-integrity three-failure note, do not delete history. |

## State of the Art

| Old Approach (4311408 / F-04) | Current Approach (working tree) | When Changed | Impact |
|-------------------------------|----------------------------------|--------------|--------|
| `identity_ok` = UID + nonempty strings | `identity_identified` then checkpointed `identity_ok` | Uncommitted host repair | Unbound builds no longer qualify at the function layer |
| `hop_max_us = last_emit_cycles` | `last_emit_cycles` separate; hop_max absent | Uncommitted | Stops unit lie |
| Real-audio forces brightness 255, no restore; short frame = black | Capture/restore; raise on `len != 480`; `accepted=False` | Uncommitted | F-06 source repair; tests incomplete |
| `--palette-autostart` help carousel | Help = mode 32 flags 5 | Working tree | WP2-08 source likely done |
| Palette compile unbounded | 120 s / 60 s timeouts | Working tree | WP2-07 likely done; verify by run |

**Deprecated/outdated:**

- Treating `identity_ok` CSV slot as “device recognised”.
- Documenting `hop_max` as AP execution time on the music runner (`last_hop_peak` is amplitude).
- SS-03 archive log as current identity (build `d13cd520…` ≠ live `c7f6034a…`).

## Open Questions

1. **Does age expiry clear `bound["ok"]`, or only paint stale?**
   - What we know: `STALE_MS=2000`; `device_age_ms` is published; `bound` is unchanged.
   - What's unclear: product preference for “unaccepted current” vs “stale identified”.
   - Recommendation: invalidate metric validity and `SCOPE_CURRENT` on age/disconnect/identity change; keep last uid/build visible as historical/unaccepted. Add the WP2-03 tests.

2. **Which build may enter `accepted-checkpoint.json`?**
   - What we know: live `c7f6034a…` is identified-not-accepted; transmitter not admitted.
   - What's unclear: Phase 3 cannot mint an accepted live image.
   - Recommendation: checkpoint file optional in Phase 3; tests use a fixture build string. Do not promote the live INFO build.

3. **Git dirty vs HEAD exact hunks**
   - What we know: STATUS says host repairs uncommitted at `4311408`.
   - What's unclear: this SSA did not re-run `git diff`.
   - Recommendation: Phase 3 Wave 0 starts with `git diff` on the six named scripts plus the two untracked tests. Orchestrator re-runs tests.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Python 3 `unittest` + standalone scripts (no pytest config in repo) |
| Config file | none — see Wave 0 |
| Quick run command | `python3 tools/serial-studio/tests/test_identity_checkpoint.py && python3 tools/serial-studio/tests/test_ss03_decode.py && python3 -m unittest tests.host.test_mode32_real_audio_observation -v` |
| Full suite command | `python3 tools/serial-studio/tests/test_identity_checkpoint.py && python3 tools/serial-studio/tests/test_ss03_broker.py && python3 tools/serial-studio/tests/test_ss03_transport.py && python3 tools/serial-studio/tests/test_ss03_lock.py && python3 tools/serial-studio/tests/test_ss03_decode.py && python3 -m unittest tests.host.test_mode32_real_audio_observation tests.host.test_colour_integrity -v && python3 -u scripts/test_palette_runtime.py` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WP2-01 | No reimplementation | manual-only (file list / git log) | n/a | n/a |
| WP2-02 | Unbound builds not ok | unit | `python3 tools/serial-studio/tests/test_identity_checkpoint.py` | ✅ seed; extend if `attach()` regresses |
| WP2-03 | Build change same UID | unit | `python3 tools/serial-studio/tests/test_ss03_broker.py` after new cases | ❌ Wave 0 — add FakeSerial poll identity_change |
| WP2-03 | Stale-to-fresh | unit | same | ❌ Wave 0 — age must drop validity, then recover |
| WP2-03 | Rejected opcode | unit | `python3 tools/serial-studio/tests/test_ss03_transport.py` | ✅ `test_forbidden_opcode_does_not_write` |
| WP2-03 | Failed cleanup | unit | broker Recorder full/OSError | ❌ Wave 0 |
| WP2-03 | Ownership handoff | unit | `python3 tools/serial-studio/tests/test_ss03_lock.py` | ✅ second lock; still need broker-vs-runner handoff |
| WP2-04 | No hidden mutate | unit | transport forbidden ops | ✅ |
| WP2-05 | emit cycles ≠ hop_max_us | unit | `python3 tools/serial-studio/tests/test_ss03_decode.py` | ✅ `test_last_emit_cycles_is_not_hop_compute` |
| WP2-06 | Rotation / write fail | unit | Recorder rotate + failed | ❌ Wave 0 (limits exist in source) |
| WP2-07 | Compile/run timeouts | smoke | `python3 -u scripts/test_palette_runtime.py` | ✅ timeouts in source; run to prove |
| WP2-08 | Autostart not carousel | unit/help | `python3 -u scripts/test_palette_runtime.py` + argparse help assert | ✅ behaviour; add help-string assert |
| WP2-09 | Short frame not black | unit | `python3 -m unittest tests.host.test_mode32_real_audio_observation.Mode32Observation.test_short_frame_is_not_black -v` | ✅ |
| WP2-09 | Missing counters | unit | same module, new test | ❌ Wave 0 |
| WP2-09 | Capture exception | unit | FakeSerial / mock transact | ❌ Wave 0 |
| WP2-09 | Restoration failure | unit | copy colour-integrity Port pattern | ❌ Wave 0 |
| WP2-10 | Docs current fields | smoke | `python3 -c` JSON current_board_build_id ≠ led-build-04 | ✅ partial; ledger D1 still stale |
| OBS-01–06 | No invented panels | unit | decode UNAVAILABLE when flags 0 | ✅ decode tests; do not add fake firmware |

### Sampling Rate

- **Per task commit:** quick run command above
- **Per wave merge:** full suite command above
- **Phase gate:** full suite green before `/gsd:verify-work`. No device flash. No physical PASS.

### Wave 0 Gaps

- [ ] Extend `test_ss03_broker.py` — same-UID build change, epoch bump, metrics cleared, stale validity, Recorder `failed` stops loop
- [ ] Extend `tests/host/test_mode32_real_audio_observation.py` — `require_counters`, capture exception, restore failure, `OBSERVATION_FAIL` / `accepted is False`
- [ ] Assert `build_scalar.py --help` contains `flags 5` and not carousel as current behaviour
- [ ] Recorder rotation unit (hit `REC_MAX_BYTES` with a temp file)
- [ ] Framework install: none — CPython 3 unittest only

## Sources

### Primary (HIGH confidence)

- `tools/serial-studio/titan_snapshot.py` — `identity_ok` / `identity_identified` / `snapshot`
- `tools/serial-studio/titan_broker.py` — `attach`, `load_checkpoint`, CLI
- `tools/serial-studio/titan_transport.py` — `OBSERVE_OPS`, `FORBIDDEN_OPS`
- `scripts/run_mode32_real_audio.py`, `scripts/run_colour_integrity.py`
- `scripts/test_palette_runtime.py`, `scripts/build_scalar.py` `--palette-autostart`
- `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md` §4 F-04/F-05/F-06, §5 WP2
- `.planning/REQUIREMENTS.md` WP2-* OBS-*
- `.planning/config.json` `workflow.nyquist_validation: true`

### Secondary (MEDIUM confidence)

- Untracked tests scored by source, not executed
- STATUS / `external-receipts.json` current-image fields (may themselves be dirty)
- Brief F-04 reproduction at `4311408` (historical; working tree has moved)

### Tertiary (LOW confidence)

- Exact `git diff` hunks vs HEAD — not re-run in this SSA. Orchestrator must `git diff -- scripts/run_mode32_real_audio.py scripts/build_scalar.py tools/serial-studio/titan_snapshot.py tools/serial-studio/titan_broker.py` before planning edits.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — in-tree modules, no new libraries
- Architecture: HIGH — signatures read from working tree
- Pitfalls: HIGH — brief anti-patterns + test-gap scoring
- Dirty-tree vs HEAD: MEDIUM — STATUS + source comparison, no porcelain

**Research date:** 2026-09-20
**Valid until:** 2026-10-20 for APIs; 7 days for dirty-tree inventory

## RESEARCH COMPLETE

**Phase:** 3 - WP2 host hygiene
**Confidence:** HIGH APIs / NOT_VERIFIED tests
**Copy:** `identity_ok(info, expected_uid=EXPECTED_UID, checkpoint=None) -> (bool, str)`; `attach()` binds identified, qualifies only with checkpoint; do not map `last_emit_cycles` to `hop_max_us`; F-06 CLI `--output --build-id --audio`; restore via `settings_payload`/`check_settings`.
**Gaps:** WP2-03 and three of four WP2-09 negatives still missing; do not treat untracked tests as complete.
