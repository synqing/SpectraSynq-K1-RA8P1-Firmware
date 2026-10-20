# Titan RA8P1 Takeover Implementation Plan

> **Correction, 2026-09-20 (Captain):** This index previously deferred `docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md` until transmitter admission. That reverses the purpose of the runtime plan. Transmitter admission is required for **combined physical-output acceptance**. It is not required for audio ownership, the K1 processing loop, controlled rendering, MIR observations, recording, replay, or an explicitly emit-disabled development image. The 20 September takeover brief is **historical context** and is not amended. Executable GSD tasks remain in `.planning/phases/**/*-PLAN.md`. Do not treat this index as a substitute for those files.

**Goal:** Deliver a usable live K1 **development runtime** on Titan Mini, with honest measurements. A hashed handback that still contains BLOCKED rows means the **handoff is complete**. It does not mean the firmware is shipped or the port is complete.

**Architecture:** Persistent integration owner plus bounded subagents (Option 1). Phase 1 checkpoint first. Then finish only the host negatives that are actually still red. Then execute the live-runtime plan through a named ARM-built candidate and Rearm under the established waiter. Q1–Q5 timing and P601/DIN diagnosis are **separate tracks** with their own prerequisites. Combined-image qualification, physical lanes, and K1-L latency follow **after** transmitter admission. Host-only work proceeds if hardware is absent; physical rows stay BLOCKED/NOT TESTED rather than green. Dirty-tree host repairs are **evidence to bind**, not a reason to rewind to audited commit `4311408`.

**Tech Stack:** RA8P1 / Titan Mini, Cortex-M85 scalar AP+VP, Python 3 host unittest/scripts, `programme_scalar.py` + Lab `titan_ra8p1_boot`, Serial Studio broker, GPT6/P601 WS281x DMA driver, DualMCU behavioural pin `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`.

**Specs:**

- Historical takeover: [docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md](../../Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md) — do not amend
- Live runtime (now in-scope): [docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md](../../Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md)

## Global Constraints

- M85 owns AP+VP. M33 parked. U55 is not an implicit production dependency. D-cache remains off unless a separately scoped experiment is reopened.
- Preserve `MEDIA_TIME_48K`, musical time, event vs availability time, affine mapping, epochs, bounded render scheduling.
- Production-candidate AP: 24,000 Hz, 180 samples, 7,500 µs hop, 80 bins, crossover 40.
- Geometry: two logical 160-pixel channels, centres 79/80; 128-pixel bench centres 63/64; centre-origin or edge-origin motion only.
- P601 = GTIOC6A. P004 has no GPT/serial-output route. P603 later only with Captain physical approval.
- DMA duty data in DMA-accessible SRAM, not DTCM.
- Shared K1 code must not include Renesas/FSP/RT-Thread headers.
- Do not modify DualMCU or Titan BSP working trees.
- No fourth DMA arbitration-map experiment as the next LED action. Never restore `DMCRA <= 2`.
- Rearm: start the prepared waiter first, reply exactly `WAITING`, consume `events.jsonl`. Events are `WAITING_FOR_IDENTIFIED_ROM` → `ROM_SEEN` → `ROM_IDENTIFIED_WRITING` → `WRITE_VERIFIED`. Do not invent `PROGRAMME_VERIFY_PASS`. Never reuse programme/run directories.
- USB `045b:5310` and `WRITE_VERIFIED` are not application identity. Bind opcode-1 INFO separately.
- Host/compile is not physical timing, fidelity, LED phase, or multi-K1 sync. Firmware emit is dump/score, not Captain eyes.
- No synthetic computer tones or white noise. No `git add -A`. No sibling worktree. No history rewrite.
- Do not self-stamp K1-C. Allowed ruling tokens only.
- Preserve the existing LED diagnostic configuration as investigation. Do not freeze independent runtime development behind it.
- Independent G8 remains parked. Ordinary implementation reviews are not G8.

## Recovery identity (two different assets)

Do not treat a hash prefix as a choice. Bind **full** SHA-256 from files on disk before any flash preparation.

| Role | Full SHA-256 | Path / name |
|------|----------------|-------------|
| Programmer safety fuse (`programme_scalar.py` refuse-if-changed) | `a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a` | EdgeAI `p3-2026-09-09/build-staged-v2/rtthread.hex` |
| Intended Titan return / current live-audio candidate | HEX `3aa0913950c5815c28c3ae06b37ee4462c5e2537adc738517cccd8b97b60eabd`; INFO build `c7f6034a902833e3f8a17f7c5792f90990f647bccfcf7f0cd211036ebaba824c` | `live-audio-gpt-20260920-03` (WRITE_VERIFIED via `live-audio-gpt-prog-20260920-03`) |

The safety fuse is **not** the Titan return image. HASHES.json must name both roles. If either file is missing or the hash differs, that role is FAILED and the waiter is not announced ready.

STATUS 20 September identity (`c7f6034a…`) is **identified-not-accepted**. Hash verification of the INFO/HEX receipts proves those files are intact. Runtime admission and transmitter admission each require their own tests; hashing does not confer either.

The September 19 DTCM tree `g4-uncapped-raw-hops-build-20260919-02` is **present** (HEX `9f12869a…`). Its raw run directory is **absent**. Do not label it empty-TCM.

---

## How to execute

Persistent lead owns checkpoint, CDC, programming, accepted results, and reviews after coherent change-sets (audio ownership, publication, controls, integration). Subagents receive bounded file ownership. Shared-tree edits are coordinated. CDC and programming always have one owner.

Do **not** start at LED diagnosis. Do **not** stop at an analyser-blocked handback.

| Order | Work | Completion condition |
|-------|------|----------------------|
| 1 | Reconcile checkout, resident image, recovery assets, existing test evidence | One accurate checkpoint; preserve completed work |
| 2 | Finish specifically identified missing host negatives | Exact remaining failures resolved; existing valid results retained |
| 3 | Execute the live-runtime plan: capture/clock contracts, single AP owner, controlled VP, complete controls, MIR visibility | Host-verified and ARM-built named candidate |
| 4 | Programme under Rearm and run real-music development checks | Demonstrated development baseline, with output limitations stated |
| Separate | Q1–Q5 timing campaign and P601/DIN diagnosis | Their own prerequisites and receipts; no blanket dependency on either |
| After transmitter admission | Integrate the admitted transmitter, qualify combined operation, then physical lanes and latency | Exact combined-image and physical evidence |

| GSD | Directory | Spec | Hardware |
|-----|-----------|------|----------|
| 1 | `.planning/phases/01-wp0-live-checkpoint/` | WP0 + WP7 SOP | Observe; flash only on Rearm |
| 2 | `02-wp1-capture-gate/` and `03-wp2-host-hygiene/` | **Remaining** F-01/F-04/F-06 negatives only | Host; do not restart green campaigns |
| 3 | `.planning/phases/10-live-k1-runtime/` | Live-runtime plan Phases 0–6 | Host + ARM build; emit-disabled OK |
| 4 | live-runtime Phase 7 campaign | Rearm + `run_live_k1` / real music | Rearm; development baseline |
| Separate | `04-wp3-uncapped-timing/` | WP3, F-03, Q1–Q5 | Own Rearm of empty-TCM images |
| Separate | `05-wp4-led-fault/` | WP4 P601/DIN | Analyser; WP4-11 BLOCKED if absent |
| After transmitter | `06-wp5-live-admission/` | Combined live candidate | After WP4 admission |
| After transmitter | `07-wp6-product-output/` physical rows, `08-latency-ruling/` | WP6 physical + K1-L | After combined image |
| Last | `09-docs-handoff/` | §7–9 | Records whatever actually happened |

`/clear` first for a fresh context window. Do not `--auto` through flashes.

---

### Task 1: Phase 1 — checkpoint and programming SOP

**Files:** `.planning/phases/01-wp0-live-checkpoint/01-01-PLAN.md`, `01-02-PLAN.md`

**Implements:** WP0-01…WP0-07, WP7-01…WP7-05.

- [x] **01-01** Inventory checkout (do not reset dirty host repairs). Run platform-memory `--check`. Inspect CDC before opening the port. Search named `g4-uncapped-raw-hops-build-20260919-02` (**present** as DTCM; raw run **ABSENT**). Bind INFO via `titan_broker.py info-once` only if CDC is free. Preserve opcode-22 before any image change. One dated STATUS/`external-receipts.json` correction; unknown stays unknown. `check_references.sh`. Bind existing host-test results to current files.
- [ ] **01-02** Prepare fresh `--output` directory, exact `programme_scalar.py --build … --wait-seconds 180 --execute` command, **both** recovery hashes with roles, bound runner. Dry-run without `--execute`. On Rearm: waiter first, `WAITING`. App identity is a separate INFO bind.

**Copy:** events `WRITE_VERIFIED` not `PROGRAMME_VERIFY_PASS`. UID `545433931bd25436593630352d068363`.

---

### Task 2: Remaining host negatives (do not restart green work)

Bind current files to passing tests. Execute only tests that are still red or still missing. Working-tree scorer already splits capture fields; identity already requires a checkpoint; tempo placement already names empty-tcm vs dtcm; real-audio runner already treats observation as non-acceptance.

Historical takeover plans `02-01`/`03-01`/`03-02` remain the file map for any **actual** remaining hole. Do not restore HEAD F-01 false PASS. No wire PASS in this slice.

Verify the bound set (orchestrator re-run):

```
python3 -m unittest tests.host.test_score_p601_capture tests.host.test_gpt_fault_witness -v
python3 scripts/score_p601_capture.py --self-test
```

Serial Studio tests are pytest-style; unittest dotted-path is not a failure of the product.

---

### Task 3: Live K1 runtime implementation

**Files:** `.planning/phases/10-live-k1-runtime/` plus `docs/Titan_Live_K1_Runtime_Implementation_Plan_2026-09-20.md`

Implement live-plan Phases 1–6: common clock, hop descriptor, atomic ASRC, no pending-hop mailbox, single AP owner, controlled VP, controls, MIR snapshots. First candidate may default physical emit **off**. Preserve LED diagnostic configuration as a separate investigation image.

Exit: `LIVE_RUNTIME_IMPLEMENTED` — host tests + ARM-built named candidate. This is not on-target proof.

---

### Task 4: Programme and real-music development checks

Use Phase 1 SOP. Fresh programme and run directories. Bind INFO after `WRITE_VERIFIED`. Run the live-plan Phase 7 normal sequence. State emit-disabled / transmitter-unadmitted limitations explicitly. This can reach `TITAN_LIVE_K1_DEV_READY` without four-lane wiring or optical KEEP.

---

### Separate track A: Q1–Q5

**Files:** `04-01-PLAN.md`, `04-02-PLAN.md`. Empty-TCM profile image, not DTCM labelled empty-TCM. Does not wait on the analyser. Does not block live-runtime.

### Separate track B: P601/DIN

**Files:** `05-01`…`05-03`. No analyser → WP4-11 BLOCKED ready-to-run task; continue independent work. No fourth arbitration map. Never restore `DMCRA <= 2`.

### After transmitter admission

Named combined candidate (old WP5), physical product output, then K1-L joins. Do not conceal a blocked transmitter as four-lane PASS.

### Handoff

Phase 9 records every row as PASS/FAIL/BLOCKED/NOT TESTED. Hashed BLOCKED handback = complete handoff ≠ ship.

---

## Findings F-01–F-06 → plans

| Finding | Plan | Note |
|---------|------|------|
| F-01 unsupported physical success | 02-01 | Bind existing split-field scorer; do not restart if host suite is green |
| F-02 lost frame 2030 contract | 02-02 | Historic frame immutable |
| F-03 DTCM vs empty-TCM | 04-01, 04-02 | Separate timing track |
| F-04 unbound build qualifies | 03-01 | Bind existing `identity_ok(..., checkpoint=)` |
| F-05 docs/receipts disagree | 03-03, 09-01 | Dated correction; do not rewrite historical paragraphs |
| F-06 real-audio not acceptance | 03-02, live-plan Phase 7 | Observation vs acceptance |

---

## Red team (plan attack)

| Attack | Result | Mitigation |
|--------|--------|------------|
| Treat STATUS 20 Sep identity as accepted | Would inherit live-audio `c7f6034a…` as qualified | identified-not-accepted; rehash receipts |
| Defer live-runtime until analyser | Stops at another blocked handback | Runtime un-deferred; analyser is a separate track |
| Execute a combined emit-on image while diagnosing P601 | Confounds LED diagnosis | Preserve diagnostic config; first live candidate may emit-disable |
| Confuse `a167833b…` with `3aa09139…` | Wrong recovery / wrong flash | HASHES.json names both roles with full hashes |
| Treat uncommitted as untested | Restart completed scoring | Bind tests to file SHAs; execute only remaining reds |
| Treat BLOCKED handback as incomplete | Fake “not done” | Handoff complete ≠ ship |
| Fourth DMA map | Repeats three failed policies | Forbidden in 05-02 |
| Restore `DMCRA <= 2` | False completion | Forbidden; mutation suite |
| Skip analyser, stamp WAVEFORM | Unsupported physical PASS | WP4-11 BLOCKED path; split scorer fields |
| Rearm without prepared waiter | Protocol failure | 01-02 must exist first; `exist_ok=False` |
| USB-read as K1-L event time | False 12 ms | LAT-01 negatives |
| Green aggregate | Fake ship | 09-01 no PASS over BLOCKED deps |
| Independent G8 self-review | Previously rejected | PARK-G8 ledger only |
| Reset dirty tree | Lose in-flight host repairs | WP0-01 preserve unrelated work |

## Second-order effects

| Decision | First order | Then | Then |
|----------|-------------|------|------|
| Runtime before transmitter | Usable development image | Risk of claiming combined/physical PASS | Split outcomes: DEV_READY ≠ PRODUCT_QUALIFIED |
| Preserve LED diagnostic config | P601 investigation stays coherent | Must not freeze AP/VP work | Separate images / emit-disabled candidate |
| Honest BLOCKED rows | Avoids fake PASS | Phase 9 looks “incomplete” | That is a complete handoff, not a ship stamp |
| Bind existing host greens | Calendar saved | A later dirty-tree edit can invalidate a SHA | Re-run only when the bound file hash moves |
| Empty-TCM as a named build | Honest Q1–Q5 | Requires Rearm over live-audio resident | Keep WP3 results safe; do not steal the live-runtime Rearm |

## Assumptions to re-check on execute

1. Working-tree scorer/broker/tempo-placement diffs still match their bound test SHAs.
2. External evidence root **has** directory `g4-uncapped-raw-hops-build-20260919-02` (DTCM). Its raw run is absent.
3. No logic analyser on USB until STATUS says otherwise.
4. Pytest is present for Serial Studio function tests; unittest dotted-path is not the product gate.
5. Programmer safety HEX `a167833b…` still exists at the p3 path; intended recovery remains `live-audio-gpt-20260920-03` until a newer live candidate is accepted.

---

## Completion language

| Token | Means | Does not mean |
|-------|--------|----------------|
| `LIVE_RUNTIME_IMPLEMENTED` | Source + host tests + ARM build of the named candidate | On-target behaviour |
| `TITAN_LIVE_K1_DEV_READY` | Phase 7 live development campaign on that image | Four-lane, optical KEEP, RT1062 ruling, empty-TCM Q1–Q5 |
| `PRODUCT_QUALIFIED` | Transmitter + physical + latency + Captain ruling | — |
| Hashed handback with BLOCKED rows | Handoff complete | Firmware shipped / port complete |

---

## Verification (planning correction)

- Live-runtime deferral removed from this index, ROADMAP, PROJECT, STATE.
- Takeover brief left unchanged.
- Recovery roles split; prefixes are not a choice.
- Host-test bind is a checkpoint artefact, not a restart order.
- Phase 1 bounded checkpoint proceeds immediately.

A plan is not completion. Unavailable hardware blocks the affected measurement only.

---
*Planning date: 2026-09-20. Spec commit cited by the brief: `431140853dd8b58af53240ef84d5fb1a08bd8b45`. Correction applied the same day under Captain order to un-defer the live runtime.*
