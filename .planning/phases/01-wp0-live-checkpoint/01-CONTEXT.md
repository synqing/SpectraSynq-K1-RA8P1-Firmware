# Phase 1: WP0 checkpoint and WP7 programming SOP - Context

**Gathered:** 2026-09-20
**Status:** Ready for planning
**Source:** PRD Express Path (`docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md` §5 WP0, §5 WP7, §2)

<domain>
## Phase Boundary

Reconcile the live Mac checkout, CDC owner, named timing receipts, and application identity without resetting dirty work. Write one truthful STATUS/receipts current entry. Prepare the Rearm waiter artefacts (fresh directory, exact command, recovery hashes, bound runner) so a later phase can flash without discovering those after Captain says Rearm. This phase does not grant a new flash unless Captain issues Rearm with those artefacts already prepared.

</domain>

<decisions>
## Implementation Decisions

### Authority order
- Read `AGENTS.md`, then platform memory `--check`, then STATUS newest dated entry, then colour integration, mic/LED2 canon, remaining plan, latency decision, uncapped timing brief, original EXECUTION-BRIEF.
- DualMCU pin `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`. BSP pin `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`. Do not follow moving sibling HEAD.

### Checkout
- Firmware: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`
- Do not assume `c2d4dca + unpublished week`. Preserve unrelated dirty files. No `git add -A`. No sibling worktree.

### Device
- UID `545433931bd25436593630352d068363`. USB `045b:5310` is not identity.
- Do not kill unknown CDC owner. Historical D-state reports are not current process evidence.
- STATUS 20 September claims live INFO build `c7f6034a…` and no broker owner after `takeover-info-once-20260920-01`. Treat as hypothesis until receipts are re-read.

### Programming SOP
- Command shape: `python3 scripts/programme_scalar.py --build "$TITAN_FINAL_BUILD_DIR" --output "$TITAN_NEW_PROGRAMME_DIR" --wait-seconds 180 --execute`
- On Rearm: start waiter first, reply exactly `WAITING`. Consume `events.jsonl`. Never reuse directories.
- Programming success ≠ application startup, peripheral transaction, register state, or light.

### Claude's Discretion
- Exact STATUS prose wording for the dated correction entry, provided unknown fields stay unknown.
- Where to store the prepared waiter command file (prefer a fresh unused evidence directory, never an existing programme receipt dir).

</decisions>

<specifics>
## Specific Ideas

Suggested read-only checks from the brief:

```sh
git status --short
git branch --show-current
git rev-parse HEAD
git log -8 --oneline
git diff --stat
bash scripts/check_references.sh
```

Named timing asset: `g4-uncapped-raw-hops-build-20260919-02`. Search that name first.

External evidence root: `/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`

</specifics>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scripts/programme_scalar.py` — waiter
- Existing identity/INFO path (opcode 1)
- `docs/evidence/K1-RA8P1-002/STATUS.md`, `external-receipts.json`

### Established Patterns
- Rearm protocol in AGENTS.md
- Never reuse programme/run directories

### Integration Points
- Later WP3/WP4/WP5 flashes consume the SOP artefacts from this phase

</code_context>

<deferred>
## Deferred Ideas

WP1–WP6 implementation, latency campaign, architecture ruling, parked M33/U55/NPU work.

</deferred>
