# Phase 4: WP3 uncapped timing - Research

**Researched:** 2026-09-20
**Domain:** Named empty-TCM vs DTCM scalar profiler, 6,000-hop raw trace, Q1–Q5, hop-136 delay negative
**Confidence:** HIGH on copy-ready build/run APIs and hop-136 vs hop-0 split. MEDIUM on whether empty-TCM HEX `22d405c1…` still exists on disk. NOT_VERIFIED on Q1–Q5 answers (campaign pack is a ready-to-Rearm procedure, not a measurement).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Timing brief requires empty ITCM/DTCM.
- `build_scalar.py` applies `apply_tempo_dtcm_overlay()` whenever `--resident-controls` is provided; `--stage-profile` requires those controls; overlay places two ACF arrays in `.dtcm`.
- Overlay-order regression intentionally preserves profiling and DTCM.
- A DTCM run answers questions about that configuration only.
- `run_scalar_schedule.py` decodes raw version 2 but writes `K1T1.v1` — fix the label.

- Raw timing support already exists. Compact `Distribution<8000>` is not the defect.

- Retain the same frozen input bytes. Do not regenerate both sides' goldens. Do not re-run the entire old corpus for document edits.

- Known 5 ms delay at hop 136 on the 6,000-hop fixture.
- Host one-hop fixture uses hop 0 — do not conflate.

- If resident image must change, prepare under WP7 (Phase 1 SOP). Do not consume an uncaptured new LED fault by switching back to the profiler carelessly.
- STATUS claims empty-TCM HEX `22d405c1…` built; resident still live-audio `c7f6034a…`. Verify hashes; do not expand abbreviated hashes by guessing.

### Claude's Discretion

- Flag/name for the empty-TCM build choice, provided it cannot be confused with the DTCM default.

### Deferred Ideas (OUT OF SCOPE)

Combined live timing (WP5), K1-L (Phase 8).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WP3-01 | Reuse any completed raw run from WP0. Compact 8,000 µs histograms are not percentiles | `Distribution<8000>` remains compact summary. Authority is raw hops when `K1_ENABLE_STAGE_PROBE`. STATUS: no empty-TCM raw-hop run yet. Named DTCM profiler `32dd1f5f…` / HEX `9f12869a…` cannot close empty-TCM Q1–Q5. |
| WP3-02 | Explicit configuration record: O3, frozen hashes, placement, cache/FPU, clock, observer, full build/HEX, ELF/map | Copy `build_scalar.py` receipt keys `tempo_placement`, `tempo_placement_evidence`, `stage_profile`, `optimisation`/`flags`. |
| WP3-03 | Preserve qualified DTCM image. Explicit empty-TCM build choice. Never call DTCM empty-TCM. Fix `K1T1.v1` label | Working tree already has `--tempo-placement empty-tcm\|dtcm` (required with `--resident-controls`) and receipt schema `K1T1.v2`. Untracked `test_tempo_placement.py` covers placement, not the schema string and not Q1–Q5. |
| WP3-04 | Same frozen bytes. Do not regenerate both goldens | Profile `docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json`: PCM `5970e8d2…`, header `50040332…`, 6000 hops. |
| WP3-05 | All 6,000 rows; decode after window; sequence vs target counters | `run_scalar_schedule.py` opcode 12 chunks of 128; `RAW_TRACE_VERSION=2`. |
| WP3-06 | Answer Q1–Q5 or a precise blocker each | Campaign pack lists commands. Measurement BLOCKED on Rearm. Map Q3 names to implemented stage ids, not the brief’s informal list. |
| WP3-07 | 5 ms delay at hop 136; negative not qualification; not host hop 0 | Firmware: `index==(K1_RESIDENT_HOPS>136U?136U:0U)`, flag `0x80`. Runner `--timing-mutation`. |
| WP3-08 | Ordinary too-slow vs late start from previous tempo | Raw `lateness_cycles` vs `total_cycles`; JSONL `completion_cycles = total + lateness`. Do not hide ordinary misses. |
| WP3-09 | Tempo p99 vs 22,500 µs is not a 7,500 µs release-to-completion pass | Report both. Slicing later is out of scope. |
| WP3-10 | Verified `run_scalar_schedule.py` interface; fresh dirs; separate `--timing-mutation`; `--qualification` is a different gate | Copy CLI below. `exist_ok=False`. Qualification loops = 40 from frozen profile. |
</phase_requirements>

## Summary

F-03 is a **configuration identity** bug, not missing raw-trace code. The timing brief wants empty ITCM/DTCM. At the audited commit, `--resident-controls` always applied `apply_tempo_dtcm_overlay()`. The working tree already names the choice `--tempo-placement empty-tcm|dtcm` and requires it with `--resident-controls`. Discretion: **keep that flag name**. Do not invent `--empty-tcm` beside it.

Q1–Q5 are not closed. `docs/evidence/K1-RA8P1-002/Q1-Q5-CAMPAIGN.md` is a Rearm/run pack with full hashes. Resident application remains live-audio `c7f6034a…`. Switching images is WP7. Do not consume an uncaptured LED fault. Compact `Distribution<8000>` stays; do not rewrite it. Never call a DTCM result empty-TCM. Never conflate host hop-0 (`K1_RESIDENT_HOPS=1`) with target hop 136.

**Primary recommendation:** Copy `--tempo-placement empty-tcm` + `--stage-profile` for the Q1–Q5 image, `--tempo-placement empty-tcm` without `--stage-profile` for the observer control, `--tempo-placement dtcm` only as the named historical overlay. Collect 6,000 `K1T1` v2 rows after the timing window. Run `--timing-mutation` in a **fresh** directory as `G4_RAW_TIMING_NEGATIVE`.

## Standard Stack

### Core

| Library / artefact | Version / identity | Purpose | Why Standard |
|--------------------|--------------------|---------|--------------|
| `scripts/build_scalar.py` | working tree | Stage BSP + overlays + SCons | Only ARM image builder |
| `scripts/tempo_dtcm_overlay.py` | working tree | `apply_tempo_placement` / `apply_tempo_dtcm_overlay` | Named empty-tcm vs dtcm |
| `scripts/stage_profile.py` | working tree | Disposable K1 probe insertion | Order: probes **then** placement |
| `scripts/run_scalar_schedule.py` | working tree | Identity-gated 6,000-hop run + raw decode | Existing runner; do not rewrite |
| `platform/ra8p1/fixture_app.cpp` | working tree | Raw hops, flag `0x80`, hop 136/0 | Target mutation site |
| `platform/ra8p1/stage_probe.h` | working tree | `K1StageId` enum | Q3 stage names |
| `platform/ra8p1/k1_double_probe.*` | working tree | `__aeabi_d*` wraps | Q4 |
| Frozen profile | `g4-scalar-workload-profile.json` | 6000 hops, hashes, qualification_loops=40 | Shared fixture bytes |
| Frozen header/PCM | `resident-controls-01` | header sha256 `5004033206cb9950374ec50ca0835481a21a03ea4059103862ff7bc29f7cccba`, PCM `5970e8d26787f5c9ccdda50b95077d3da503b3826495ed7232d2854fecb8d394` | Do not regenerate |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `tests/host/test_tempo_placement.py` | untracked | Placement/ELF/CLI negatives | WP3-03 host gate — not Q1–Q5 |
| `tests/host/test_stage_profile.py` | tracked + dirty extras | Probe order, raw decode, empty-tcm keeps probes | Overlay-order regression |
| `tests/host/test_schedule_protocol.cpp` | tracked | Host 1-hop, flag `0x80` at hop **0** | Do not use as hop-136 proof |
| `scripts/test_schedule_protocol.py` | tracked | Compiles scalar/npu/profile host fixtures | Includes `-DK1_ENABLE_STAGE_PROBE=1` |
| `docs/evidence/K1-RA8P1-002/Q1-Q5-CAMPAIGN.md` | untracked campaign pack | Copy-ready build/prog/run paths | Procedure, not a PASS |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `--tempo-placement empty-tcm\|dtcm` | New `--empty-tcm` / `--no-dtcm` | Discretion allows a new name **only if** it cannot be confused with DTCM. Working tree already has the unambiguous pair. **Keep it.** |
| Rewrite `Distribution<8000>` | Leave compact bins; use raw JSONL | Brief: compact bins are not the defect |
| Reuse DTCM run `g4-uncapped-raw-hops-build-20260919-02` as empty-TCM | Separate named empty-TCM image | Calling DTCM empty-TCM is F-03 |

**Installation:** none. ARM build uses existing `uvx --from scons scons` inside `build_scalar.py`.

## Architecture Patterns

### Recommended Project Structure

```
scripts/
├── build_scalar.py              # --tempo-placement required with --resident-controls
├── tempo_dtcm_overlay.py        # apply_tempo_dtcm_overlay, apply_tempo_placement, ELF verify
├── stage_profile.py             # instrument_stage_sources (disposable copies only)
└── run_scalar_schedule.py       # --timing-mutation, RAW_TRACE_VERSION=2, schema K1T1.v2
platform/ra8p1/
├── fixture_app.cpp              # delay at hop 136 or 0; Distribution<8000> compact only
├── stage_probe.h
└── k1_double_probe.c / .h
tests/host/
├── resident_controls.h          # K1_RESIDENT_HOPS 1U → mutation index 0
├── test_tempo_placement.py
├── test_stage_profile.py
└── test_schedule_protocol.cpp   # hop-0 host negative
docs/evidence/K1-RA8P1-002/
├── g4-scalar-workload-profile.json
└── Q1-Q5-CAMPAIGN.md
```

### Pattern 1: Named placement (F-03 repair already in tree)

**What:** `--resident-controls` no longer implies DTCM. Placement is an explicit identity input hashed into `build_id`.
**When to use:** Every resident scalar build.
**Copy-ready argparse (working tree `build_scalar.py`):**

```text
--resident-controls PATH
--stage-profile                  # requires --resident-controls
--tempo-placement {empty-tcm,dtcm}
  # required with --resident-controls and forbidden without it
--optimisation {o2,o3-unroll}    # Q1–Q5 uses o3-unroll
--dcache {disabled,enabled}      # Q1–Q5 uses disabled
```

Call-site order (do not reverse):

```python
# Source: scripts/build_scalar.py
if args.stage_profile:
    receipt['instrumented_sources'] = instrument_stage_sources(stage/'src/k1')
if args.resident_controls:
    placement = apply_tempo_placement(stage/'src/k1', args.tempo_placement)
    receipt['tempo_placement_sources'] = placement
    if args.tempo_placement == 'dtcm':
        receipt['tempo_dtcm_sources'] = placement['overlay']
    verify_tempo_placement_receipt(receipt)
# later, after nm:
receipt['tempo_placement_evidence'] = verify_tempo_elf_placement(symbols, args.tempo_placement)
```

`apply_tempo_dtcm_overlay(root)` still exists and **only** rewrites the two ACF arrays into `.dtcm`. `apply_tempo_placement(root, 'empty-tcm')` must **not** rewrite; it rejects staged text that already names `section(".dtcm")`. `apply_tempo_placement(root, 'dtcm')` calls the overlay.

ELF windows (CPU view): ITCM `0x00000000–0x00020000`, DTCM `0x20000000–0x20020000`. Hot arrays `(512+200)*4 = 2848` bytes. empty-tcm evidence: `residence='automatic-storage'`, `stack_delta_bytes=2848`, no exported `work`/`acf` statics in TCM. dtcm: `acf`/`work` addresses inside DTCM, not ITCM.

CONTEXT still describes the audited “always overlay” behaviour. Plans must copy the **working-tree** API above, then add a host test that `build_scalar.py --resident-controls …` without `--tempo-placement` exits nonzero (already in `test_tempo_placement.py::test_build_scalar_requires_named_placement`).

### Pattern 2: Raw trace v2 (do not rewrite)

**What:** After the schedule finishes, pull 6,000 records via opcode 12 in chunks of 128.
**Copy-ready constants (`run_scalar_schedule.py`):**

```python
RAW_TRACE_VERSION = 2
RAW_TRACE_CHUNK = 128
RAW_FLAG_TEMPO = 1
RAW_FLAG_RENDERED = 2
RAW_FLAG_DEADLINE = 4
RAW_FLAG_RELEASE_GUARD = 8
RAW_FLAG_INJECTED = 16
# decode_raw_trace_chunk: magic b'K1T1', version 2
# receipt['raw_stage_trace']['schema'] = 'K1T1.v2 little-endian; JSONL is the lossless decoded form'
```

CONTEXT / brief F-03: audited writer used descriptive `K1T1.v1` while decoding v2. Working tree already writes `K1T1.v2`. Plan: add a unit assert on that string; do not “fix” it again if already v2.

JSONL fields after decode (lossy flags expanded): `hop`, `total_cycles`, `lateness_cycles`, `injected_delay_cycles`, `double_calls`, `double_cycles`, `stages_cycles`, `tempo_updated`, `rendered`, `deadline_missed`, `release_guard_failed`, `timing_mutation`, `completion_cycles`.

WP3-08: `lateness_cycles` is release lateness (blocked by previous work). `total_cycles` is this hop’s work (includes injected delay on the mutated hop). Ordinary miss = `deadline_missed` and not `tempo_updated`. Do not relabel ordinary misses as “structural tempo only”.

### Pattern 3: Hop 136 vs hop 0

**What:** Same C expression, two fixtures.

```cpp
// Source: platform/ra8p1/fixture_app.cpp
if ((schedule.flags & 0x80U) && schedule.loop == 0U &&
    index == (K1_RESIDENT_HOPS > 136U ? 136U : 0U)) {
  const std::uint32_t delay_cycles = clock_hz / 200U; // declared 5 ms
  ...
  schedule.raw[index].injected_delay_cycles = k1_cycle_count() - delay_started;
}
```

| Fixture | `K1_RESIDENT_HOPS` | Mutation index | How to run |
|---------|--------------------|----------------|------------|
| Host schedule protocol | 1 (`tests/host/resident_controls.h`) | **0** | `python3 scripts/test_schedule_protocol.py` profile extra; `start(1, 0x80)` |
| Frozen Titan 6,000-hop | 6000 | **136** | `run_scalar_schedule.py --timing-mutation --loops 1 --mode scalar` |

Runner locate rule: `len(injected)==1 and hop==136 and injected_delay_cycles >= clock_hz//200 and timing_mutation`. Gate name `G4_RAW_TIMING_NEGATIVE`. A failed schedule result may still be a valid measurement — do not relabel it qualification.

`--timing-mutation` is rejected if combined with `--qualification`, `--mutation`, non-scalar `--mode`, `loops != 1`, or build without `stage_profile`.

Start flags: `flags = (3 if mutation else 1) | (mode<<4) | (0x40 if failure_campaign else 0) | (0x80 if timing_mutation else 0)`.

### Anti-Patterns to Avoid

- **Calling DTCM empty-TCM.** Named historical DTCM: profiler `32dd1f5f…`, HEX `9f12869a…` (`g4-uncapped-raw-hops-build-20260919-02`). Empty-TCM candidate (campaign, verify hashes): HEX `22d405c1d45871c45c0bfa5b88fc1868c79198de493efeef900a47f89cb81a8b`, build `8a78961b5ad0e821f7f20e13cc093b37e32b3be4643995ee12e4ccbb5a6878bd`.
- **Regenerating both goldens** to hide a behavioural change. Share fixture bytes.
- **Conflating hop 0 host fixture with hop 136 target mutation.**
- **Using compact `Distribution<8000>` p99 as Q1.** Those bins saturate (original brief: tempo p99 read 8,000 µs).
- **Calling `--qualification` the uncapped experiment.** Qualification is 40 loops / 240,000 hops / `G4_SCALAR_SUBPROFILE`. Uncapped Q1–Q5 is `--loops 1` on the stage-profile image.
- **Inheriting combined/product PASS** from a scalar timing run.
- **Flash without WP7.** Resident is live-audio `c7f6034a…`. Empty-TCM images are built not Rearmed.
- **Rewriting raw-trace support** because compact bins remain.
- **Mapping brief Q3 informal names** (`gdft_goertzel`, `tempo_acf`) onto new firmware stages. Use implemented `K1StageId` names.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Empty vs DTCM overlay | New rewriter | `apply_tempo_placement(root, 'empty-tcm'\|'dtcm')` | ELF + receipt verifiers exist |
| Raw decode | New parser | `decode_raw_trace_chunk` / `summarise_raw_trace` | Already reconciles counters |
| 5 ms negative | Host-only hop 0 claimed as 136 | `--timing-mutation` on 6000-hop image | Index is the proof |
| Q1–Q5 report | Hand averages from opcode-8 compact JSON | `raw-stage-trace.jsonl` + `summarise_raw_trace` | Recomputable from raw |
| Observer overhead | Guess instrumentation cost | Second image **without** `--stage-profile`, same frozen bytes | Q5 matched observer-off |
| Placement proof | `size` text section folklore | `verify_tempo_elf_placement` addresses | empty-tcm must not export TCM statics |

**Key insight:** Raw support is done. Phase 4 is **name the configuration, run or block honestly, keep hop 136 as a negative**.

## Common Pitfalls

### Pitfall 1: Untracked tests/campaign mistaken for WP3-03 / Q1–Q5 closed

**What goes wrong:** Planner skips host gates or claims Q1–Q5 answered.
**Score:**

| Artefact | WP3-03 | WP3-07 | Q1–Q5 |
|----------|--------|--------|-------|
| `tests/host/test_tempo_placement.py` | **Partial:** receipt mismatch, ELF TCM reject, empty-tcm does not rewrite, CLI requires `--tempo-placement`. Does **not** assert `K1T1.v2` schema string. Does **not** prove an ARM empty-TCM image. | n/a | n/a |
| `tests/host/test_stage_profile.py` | Partial: `test_empty_tcm_keeps_probes_without_dtcm`; overlay-order still requires probes **then** DTCM | n/a | decode/summary unit only (2 synthetic rows) |
| `tests/host/test_schedule_protocol.cpp` | n/a | **Host hop 0 only** (`K1_RESIDENT_HOPS=1`) | n/a |
| `Q1-Q5-CAMPAIGN.md` | Documents named builds | Documents `--timing-mutation` dir | **NOT a measurement.** STATUS: Q1–Q5 BLOCKED on Rearm. `g4-uncapped-raw-hops-prog-20260920-01` must not be adopted. |

### Pitfall 2: Expanding abbreviated hashes

**What goes wrong:** Inventing the rest of `22d405c1…`.
**How to avoid:** Copy full hashes from `Q1-Q5-CAMPAIGN.md` or receipts. If the file is absent, record unknown.

Full hashes from the campaign pack (verify on disk before use):

| Candidate | build_id | HEX sha256 | ELF sha256 |
|-----------|----------|------------|------------|
| empty-TCM profile `g4-empty-tcm-raw-hops-build-20260920-01` | `8a78961b5ad0e821f7f20e13cc093b37e32b3be4643995ee12e4ccbb5a6878bd` | `22d405c1d45871c45c0bfa5b88fc1868c79198de493efeef900a47f89cb81a8b` | `850c307805f1954fc2795857e5fcfc7138dafa99898cd3c14bc7cbd5be644e6e` |
| empty-TCM observer `g4-empty-tcm-observer-build-20260920-01` | `4ff1deea52d4f2e85a0630af78ac09e5243f66b69a9320d4e69b0588ad3eed43` | `28eda0d11083d100f770aca67a6dad67e2699c725e3bfabc28f04cdcd2e3d29c` | `878150270705ca5858e3901ec4482955bbd8513b42c71eb547b853f8342cdf6a` |
| DTCM named variant `g4-dtcm-raw-hops-build-20260920-01` | `ed3e75671158071c9317311ae669a2a89b412ab65ae70b56e5d34fef86c99107` | `b618cb887f07736a4ee8a333df0d6a8584378dab3f674b8d5b98d6446869fbb5` | `0835cc4caef4fb0b9220b6d2e92a09cb21dcfbab271241367d1127a8a32b3f63` |

DTCM variant size is claimed to match historical HEX `9f12869a…` (text/data/bss). That is overlay identity, not empty-TCM Q1–Q5.

### Pitfall 3: Q3 stage-name mismatch

**What goes wrong:** Plan invents `gdft_goertzel` probes because the 2026-09-10 brief listed them.
**How to avoid:** Implemented hop stages:

```python
PROFILE_FULL_HOP_STAGES = {
    'ap_total','gdft_raw','gdft_postprocess','features','chord_detect',
    'onset_beat','musical_saliency','tempo_total','musical_time','telemetry',
}
PROFILE_UPDATE_STAGES = {
    'tempo_history','tempo_acf_total','acf_prepare','acf_correlate','acf_comb',
    'acf_normalise','tempo_bank','tempo_flywheel','tempo_output',
}
PROFILE_UNUSED_STAGES = {'clock_affine'}  # count must be 0
# plus vp_render
```

Q3 “tempo_acf” → `tempo_acf_total` plus `acf_*`. Q3 “tempo_tracker” → `tempo_total` / `tempo_history` / `tempo_bank` / `tempo_flywheel` / `tempo_output`. `clock_affine` is instrumented but expected unused on the hop path; report zero only if the 6,000-row trace actually has count 0.

### Pitfall 4: Image change consumes a new LED fault

**What goes wrong:** Rearm onto profiler, transmitter faults, witness not captured.
**How to avoid:** WP7 waiter + recovery HEX `3aa0913950c5815c28c3ae06b37ee4462c5e2537adc738517cccd8b97b60eabd` (`live-audio-gpt-20260920-03`). Fresh programme dirs only. Preserve first fault.

## Code Examples

### Build flags (copy)

Empty-TCM Q1–Q5 profile (observer **on**):

```sh
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-raw-hops-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --stage-profile --tempo-placement empty-tcm
```

Empty-TCM observer control (no stage profile):

```sh
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-empty-tcm-observer-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --tempo-placement empty-tcm
```

Named DTCM variant (map proof only; **not** Q1–Q5):

```sh
python3 scripts/build_scalar.py \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/g4-dtcm-raw-hops-build-20260920-01 \
  --optimisation o3-unroll --dcache disabled \
  --resident-controls /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/resident-controls-01/resident_controls.h \
  --stage-profile --tempo-placement dtcm
```

If those output directories already exist, **do not reuse them**. `mkdir(..., exist_ok=False)`. Choose a new dated suffix.

### Run flags (copy)

```sh
python3 scripts/run_scalar_schedule.py \
  --build "$TITAN_PROFILE_BUILD_DIR" \
  --resident "$TITAN_RESIDENT_FIXTURE_DIR" \
  --profile "$TITAN_FROZEN_PROFILE_FILE" \
  --output "$TITAN_NEW_PROFILE_RUN_DIR" \
  --loops 1 --mode scalar
```

Timing negative (separate fresh directory):

```sh
python3 scripts/run_scalar_schedule.py \
  --build "$TITAN_PROFILE_BUILD_DIR" \
  --resident "$TITAN_RESIDENT_FIXTURE_DIR" \
  --profile /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json \
  --output "$TITAN_NEW_MUTATION_RUN_DIR" \
  --loops 1 --mode scalar --timing-mutation
```

Other flags (do **not** mix into Q1–Q5): `--qualification` (exact 40 loops), `--mutation` (CRC comparator negative), `--failure-campaign` (NPU scheduled only).

### Q1–Q5 mapping (copy)

| Q | Answer from | Blocker if missing |
|---|----------------|--------------------|
| Q1 true hop distribution | `summarise_raw_trace` `total.tempo` / `total.ordinary` min/p50/p95/p99/max **cycles and µs** | No 6,000-row JSONL, or compact opcode-8 only |
| Q2 miss cross-tab | `cross_tab`: tempo_missed/met, ordinary_missed/met | Incomplete trace or counts ≠ `deadline_misses` |
| Q3 stages | `stages[name][tempo\|ordinary]` using implemented names | Do not invent `gdft_goertzel` |
| Q4 doubles | `software_double.*.calls/cycles`; firmware `double_calls`/`double_cycles` | Observer image has no wraps — do not compare as like-for-like stage costs |
| Q5 observer | Compare empty-TCM profile vs empty-TCM observer ordinary means vs historical 3,872 µs baseline | Second Rearm not done → blocker, not a guessed delta |

Tempo p99 vs 22,500 µs is Q/WP3-09 only. 7,500 µs release-to-completion remains a separate contract (`g4-scalar-workload-profile.json` `release_to_completion_deadline_us: 7500`).

## Dirty-tree deltas plans must copy

| Path | Working-tree delta vs audited F-03 | Plan action |
|------|--------------------------------------|-------------|
| `scripts/tempo_dtcm_overlay.py` | `apply_tempo_placement`, ELF/receipt verifiers, `PLACEMENTS=('empty-tcm','dtcm')` | Copy; do not always-call overlay |
| `scripts/build_scalar.py` | `--tempo-placement` required with `--resident-controls`; `apply_tempo_placement` after probes | Keep flag name |
| `scripts/run_scalar_schedule.py` | schema string `K1T1.v2 little-endian; JSONL is the lossless decoded form` | Assert in host test; do not revert to v1 label |
| `tests/host/test_tempo_placement.py` | Untracked | Land as WP3-03 host gate |
| `tests/host/test_stage_profile.py` | `test_empty_tcm_keeps_probes_without_dtcm` | Keep overlay order: probes then DTCM |
| `docs/evidence/K1-RA8P1-002/Q1-Q5-CAMPAIGN.md` | Untracked procedure | Copy commands; never treat as Q1–Q5 PASS |

CONTEXT sentence “`build_scalar.py` applies `apply_tempo_dtcm_overlay()` whenever `--resident-controls` is provided” is the **audited defect**, not the working-tree contract. Plans implement/verify the named flag, not the old always-DTCM path.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `--resident-controls` ⇒ DTCM overlay | `--tempo-placement empty-tcm\|dtcm` | Uncommitted host repair | Empty-TCM can actually be built |
| Receipt label `K1T1.v1` while decode v2 | `K1T1.v2 …` | Uncommitted | Schema honesty |
| Compact 8,000 µs bins as p99 | Raw hops authority when probed | Already in `fixture_app.cpp` comments | Do not delete `Distribution<8000>` |
| Always-on DTCM called “brief configuration” | Named historical variant | Campaign + STATUS | Stops F-03 laundering |

**Deprecated/outdated:**

- Adopting `g4-uncapped-raw-hops-prog-20260920-01` (campaign: does not exist, not adopted).
- Using opcode-8 compact p99 for Q1.
- Claiming hop-0 host `0x80` run as hop-136 Titan negative.

## Open Questions

1. **Are the 2026-09-20 empty-TCM artefacts still on the EdgeAI_Artifacts disk?**
   - What we know: campaign pack lists full hashes.
   - What's unclear: this SSA did not stat those directories.
   - Recommendation: WP3-01/02 first action is hash-verify or rebuild into a **new** dated directory. Never reuse receipt dirs.

2. **Must Phase 4 Rearm?**
   - What we know: resident is live-audio `c7f6034a…`. Q1–Q5 needs the empty-TCM profile image.
   - What's unclear: Captain Rearm timing.
   - Recommendation: Host placement tests and schema assert proceed without flash. Target collection is BLOCKED pending WP7. Record the blocker per question, do not invent hop times.

3. **Historical G4 qualified DTCM image vs named 2026-09-20 DTCM rebuild**
   - What we know: qualified scalar HEX `619077a5…` / build `603e3f17…` is a different slim-status image. Profiler HEX `9f12869a…` is the uncapped DTCM overlay.
   - What's unclear: whether to rebuild DTCM for map proof (`b618cb88…`) or only cite `9f12869a…`.
   - Recommendation: Do not rebuild to “match” goldens. Preserve both identities; never present either as empty-TCM.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Python 3 `unittest` + `scripts/test_schedule_protocol.py` host C++ fixture |
| Config file | none |
| Quick run command | `python3 -m unittest tests.host.test_tempo_placement tests.host.test_stage_profile -v` |
| Full suite command | `python3 -m unittest tests.host.test_tempo_placement tests.host.test_stage_profile -v && python3 scripts/test_schedule_protocol.py` |

Target `run_scalar_schedule.py` is **not** a host unit test. It is an on-silicon runner. Do not put it in the per-commit quick command.

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WP3-01 | Compact bins not Q1 authority | unit | `tests.host.test_stage_profile.StageProfileTests.test_raw_trace_decode_and_summary` | ✅ synthetic; live 6,000-row is target |
| WP3-02 | Placement in receipt/ELF | unit | `python3 -m unittest tests.host.test_tempo_placement -v` | ✅ untracked |
| WP3-03 | empty-tcm CLI required; no overlay on empty-tcm; no DTCM overlay recorded | unit | `test_build_scalar_requires_named_placement` + receipt tests | ✅ partial; add schema `K1T1.v2` assert |
| WP3-03 | Never label DTCM empty-TCM | smoke/docs | grep receipts/STATUS | ❌ Wave 0 doc assert |
| WP3-04 | Frozen PCM/header hashes unchanged | unit | compare profile JSON to resident receipt | ✅ profile file; do not regenerate |
| WP3-05 | 6,000 rows + counter reconcile | on-silicon | `run_scalar_schedule.py --loops 1` | ❌ target; BLOCKED without Rearm |
| WP3-06 | Q1–Q5 | analysis of JSONL | none until raw file exists | ❌ BLOCKED |
| WP3-07 | Hop 136 5 ms | on-silicon negative | `--timing-mutation` fresh dir | ❌ target; host hop 0 exists separately |
| WP3-07 | Host hop 0 not claimed as 136 | unit | `python3 scripts/test_schedule_protocol.py` | ✅ different index |
| WP3-08 | Lateness vs total | unit + JSONL | decode fields | ✅ unit shape; live rows target |
| WP3-09 | 22500 vs 7500 distinct | manual report | n/a | n/a until Q1 |
| WP3-10 | CLI / exist_ok False / gates | unit | argparse + mutation exclusivity already in runner | ✅ source; add focused unit if missing |

### Sampling Rate

- **Per task commit:** quick unittest command
- **Per wave merge:** full host suite including `test_schedule_protocol.py`
- **Phase gate:** host suite green **and** either (a) 6,000-row JSONL with hashed receipt answering Q1–Q5 or (b) each Q has a precise Rearm/identity blocker. No inherited product PASS.

### Wave 0 Gaps

- [ ] Land/keep `tests/host/test_tempo_placement.py`
- [ ] Assert `run_scalar_schedule.py` schema string contains `K1T1.v2` and does not write `K1T1.v1`
- [ ] Doc/receipt grep that DTCM builds are labelled `dtcm` / `section .dtcm`
- [ ] Do **not** add a host test that claims hop 0 is hop 136
- [ ] Framework install: none

## Sources

### Primary (HIGH confidence)

- `scripts/build_scalar.py` `--tempo-placement`, overlay order
- `scripts/tempo_dtcm_overlay.py`
- `scripts/run_scalar_schedule.py` CLI, `RAW_TRACE_VERSION`, hop 136 locate
- `platform/ra8p1/fixture_app.cpp` `0x80` delay, `clock_hz/200U`
- `tests/host/resident_controls.h` `K1_RESIDENT_HOPS 1U`
- `docs/BRIEF-2026-09-10-uncapped-ap-timing.md` Q1–Q5
- `docs/evidence/K1-RA8P1-002/g4-scalar-workload-profile.json`
- `docs/evidence/K1-RA8P1-002/Q1-Q5-CAMPAIGN.md`
- `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md` F-03 / WP3
- `.planning/config.json` nyquist enabled

### Secondary (MEDIUM confidence)

- STATUS empty-TCM HEX claims and live resident `c7f6034a…`
- Campaign ARM hashes (need on-disk verify)

### Tertiary (LOW confidence)

- Exact `git diff` vs HEAD for `build_scalar.py` / `tempo_dtcm_overlay.py` / `run_scalar_schedule.py` — not re-run in this SSA

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — in-tree builders/runners
- Architecture: HIGH — placement API and hop-index expression read from source
- Pitfalls: HIGH — F-03 / hop-0 / golden regeneration
- Q1–Q5 numbers: N/A — not measured; default NOT_VERIFIED

**Research date:** 2026-09-20
**Valid until:** 2026-10-20 for APIs; immediately stale if a Rearm changes resident identity

## RESEARCH COMPLETE

**Phase:** 4 - WP3 uncapped timing
**Confidence:** HIGH APIs / NOT_VERIFIED Q1–Q5
**Copy:** `--tempo-placement empty-tcm|dtcm` (keep this name); `apply_tempo_placement` after `instrument_stage_sources`; schema `K1T1.v2`; hop 136 via `--timing-mutation` / flag `0x80`; host hop 0 is `K1_RESIDENT_HOPS==1`.
**Gaps:** Untracked placement test ≠ empty-TCM measurement; campaign pack ≠ PASS; Rearm is WP7.
