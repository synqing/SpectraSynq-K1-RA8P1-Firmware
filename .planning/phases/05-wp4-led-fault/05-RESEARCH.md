<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Do not write a fourth arbitration-map experiment.
- Three failures: fixed PDM-first `d9700b14…` frame 3468; round-robin `49c582db…` frame 2063; fixed LED-first `a3f37e8a…` frame 2030. Each: 1 DMA word remaining; 384 all-zero GRB bytes; 3072 bits; 3070 DMA words plus two preloaded.
- Keep corrected nonzero DMCRA fault. Never restore `DMCRA <= 2`. Never manufacture a completion callback.
- Keep independent DMA/waveform/latch completion semantics.
- Arm P601 and first-LED DIN before reproducing. A fault-latched transmitter is stopped; attaching afterwards cannot retrieve the preceding waveform.
- Fresh run directory. v2 opcode-22: empty remains v1/728 bytes; LE version 2 requests v2/1420.
- Hypothesis branches (test, do not preselect):
  1. All 3072 bits and payload legal, reset valid, terminal DMCRA 1 → trace preload/final request/stop/completion accounting.
  2. Missing/wrong final data or clipped pulse at P601 → GPT event count, buffering, ELC stop ordering, last DMA request.
  3. P601 conforms, DIN does not → shifter, cable, ground, levels, supply (logic analyser ≠ analogue margins).
  4. Both probes lack valid signal → supply, continuity, ownership, trigger before editing render code.
  5. Wire conforms but colours fail → part/payload order, strip/control comparison, optical.
- `python3 scripts/run_colour_integrity.py --build ... --output ... --phase-s 20 --restore-settings ...`
- Restore file is the actual intended checkpoint, not a guessed default.
- Preserve first failure. Restore settings and release CDC on every exit. No reset/retry inside the scorer.
- If hardware absent: report exact missing acquisition capability; candidate, wiring/probe instructions, decoder ready. Continue independent work. Do not invent wire evidence.

### Claude's Discretion
- Which retained guarded image to prefer **if** artefacts and recovery path verify. Do not combine speculative driver cure with new gain/render.

### Deferred Ideas (OUT OF SCOPE)
- Live-audio integration (WP5), product four-lane (WP6), visual tuning while transmission unproven.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WP4-00 | Prerequisites: WP0 identity, WP1 capture path, instrument, selected image, WP7 transition; keep WP3 results safe | Use Phase 1 waiter contract and Phase 2 scorer; never reuse programme/run dirs |
| WP4-01 | Source-bound diagnostic candidate; prefer retained guarded image if artefacts verify; no speculative driver+gain combo | Three failed builds below; default `pdm-first` / LED `DMAC2` |
| WP4-02 | Verify strip, supply, ground, shifter, pin ownership. 74HCT2G34GW is present | `FASTLED-PORT-DESIGN.md`; `titan_led_pins.h` P601/P004 |
| WP4-03 | Arm P601 and first-LED DIN before reproduce; same-frame + pre-trigger | `P601-DIN-CAPTURE-PROTOCOL.md` |
| WP4-04 | Fresh dir; opcode-22 empty=v1/728, LE `2`=v2/1420 | `k1_ws281x_gpt_dma_hw_snapshot` / `_snapshot_v2`; `decode_v2` |
| WP4-05 | Identify first divergence via five branches; no fourth arbitration map | Branch table in Architecture Patterns |
| WP4-06 | Smallest surface; keep nonzero DMCRA; never `DMCRA <= 2`; never fake completion | Guard at `dmac_done` and `gpt0_overflow` |
| WP4-07 | Host + mutation tests + one measured-defect negative; cross-build; same-frame compare | `test_ws281x_gpt_dma_hw.py` + mutations; Wave 0 gap until cause known |
| WP4-08 | Repeat zero-output / warm low-load / warm USB-stress at `--phase-s 20` | `run_colour_integrity.py` phases |
| WP4-09 | AP/PDM progress, both completion events, no PDM/DMA transport regressions, both rail signs | `score_phase()` + opcode 6 |
| WP4-10 | Visible-corruption ticket needs optical evidence **plus** wire; dumps not Captain eyes | Instrument-not-Captain; optical row separate |
| WP4-11 | No instrument: exact missing capability, candidate, wiring, decoder ready | Protocol § Missing acquisition capability |
</phase_requirements>

# Phase 5: WP4 LED first divergence — Research

**Researched:** 2026-09-20
**Domain:** RA8P1 GPT6/GPT0/DMAC WS281x transmitter, P601/DIN capture, colour-integrity controls
**Confidence:** HIGH for copy-ready APIs and host tests; MEDIUM for repository-recorded silicon history; **NOT_VERIFIED** for physical first-divergence / photons
**Default verdict:** NOT_VERIFIED

## Summary

WP4 copies the existing production GPT/DMA driver, opcode-22 witness, colour-integrity runner, and five hypothesis branches. It must not invent a fourth DMA arbitration map, a new pin, or a `DMCRA <= 2` waiver. Remaining-count is the live `DMCRA` register (`dmac_ctrl.p_reg->DMCRA`), snapshotted as `dmac_count`. Completions require **both** DMA IRQ (`dma_complete`) and GPT0 hardware-stop (`waveform_complete`) plus reset time (`reset_ready`); latch is a third event.

Three on-silicon policies already failed with the same terminal one-word remainder. Host tests already reject unfinished DMA and the waiver mutation. Physical cause is open until a new same-frame P601+DIN capture exists. Optical KEEP is not this phase's PASS.

**Primary recommendation:** Arm both probes first; score a **new** acquisition with the WP1 gate; patch only the first measured divergence; keep the nonzero-DMCRA guard; rerun the three `--phase-s 20` controls in a fresh directory.

## Standard Stack

### Core

| Library / artefact | Version / pin | Purpose | Why standard |
|--------------------|---------------|---------|--------------|
| `platform/ra8p1/ws281x_gpt_dma_hw.c` | current checkout | Production transmitter | GPT6 PWM + DMAC2 + GPT0 stop on P601 |
| `platform/ra8p1/ws281x_gpt_dma.c` | current | Host state model | `dma_remaining`; P004 cannot PWM |
| `platform/ra8p1/ws281x_waveform.c` | current | Duty encoder | `k1_ws281x_build_gpt_duty` |
| DualMCU behavioural pin | `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` | Portable K1 behaviour | `docs/REFERENCE-MANIFEST.md` |
| Titan BSP pin | `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7` | FSP/register evidence | Vendor only |
| FastLED HOST oracle | 3.10.4 `adedfc40…` | Packing oracle | Not a RA8P1 driver |

### Supporting

| Artefact | Purpose | When to use |
|----------|---------|-------------|
| `scripts/gpt_fault_witness.py` `decode_v2` | Strict v2 layout | Every opcode-22 body |
| `scripts/run_colour_integrity.py` | Three bounded controls | After candidate is programmed |
| `scripts/score_p601_capture.py` | Provenance vs conformance | After WP1 repair; never stamp PASS from synthetic `source=physical` |
| `scripts/programme_scalar.py` | WP7 Rearm waiter | Prepared before Rearm; fresh `--output` |
| 74HCT2G34GW | Pad → DIN | Documented present; do not re-open absence |

### Alternatives considered (rejected)

| Instead of | Could use | Tradeoff |
|------------|-----------|----------|
| Fourth DMAC map | Already ran PDM-first / RR / LED-first | Same 1-word remainder; **forbidden** |
| Restore `DMCRA <= 2` | Mutation `incomplete_dma_waiver` | Host tests must keep rejecting it |
| GPIO `k1_ws281x_diag_emit` | Diagnostic only | ~5.412 ms IRQ-off; not live backend |
| P004 / invented pin | No GPT/serial-output | P603 later only with Captain physical approval |
| Captain looking at the plate | Dumps / analyser | Instrument-not-Captain-eyes |

**Installation / run (host; no flash from this research):**

```bash
PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_ws281x_gpt_dma_hw.py
PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_ws281x_gpt_dma.py
PYTHONDONTWRITEBYTECODE=1 python3 scripts/test_ws281x_gpt_dma_mutations.py
python3 -m pytest -q tests/host/test_colour_integrity.py tests/host/test_gpt_fault_witness.py tests/host/test_dmac_priority_stage.py
```

## Architecture Patterns

### Recommended surfaces (do not invent siblings)

```
platform/ra8p1/ws281x_gpt_dma_hw.c/.h   # silicon driver; remaining count = DMCRA
platform/ra8p1/ws281x_gpt_dma.c/.h      # host model; dma_remaining
platform/ra8p1/ws281x_waveform.c/.h     # k1_ws281x_build_gpt_duty
platform/ra8p1/titan_led_pins.h         # K1_TITAN_DIN_A 0x0601, DIN_B 0x0004
platform/ra8p1/fixture_app.cpp          # opcode 22 v1/v2 branch
scripts/run_colour_integrity.py
scripts/gpt_fault_witness.py
tests/test_ws281x_gpt_dma_hw.c
tests/host/test_ws281x_gpt_dma.cpp
```

### Pattern 1: Remaining-count and completion (copy these names)

**What:** Remaining DMA count is **`dmac_ctrl.p_reg->DMCRA`**. Snapshot field is **`dmac_count`**. Host model field is **`dma_remaining`**. DMA length is `bits - 2` after two preloads into GTCCRA/GTCCRC. For the 128×24 bench: **3072 bits, 3070 DMA words**.

**When to use:** Every fault witness, every colour-integrity GPT JSON, every first-divergence branch.

Copy-ready APIs:

| Function | File | Role |
|----------|------|------|
| `k1_ws281x_gpt_dma_hw_init` | `ws281x_gpt_dma_hw.c` | Open GPT6/GPT0/DMAC2, PFS P601, ELC |
| `k1_ws281x_gpt_dma_hw_submit` | same | Pack duty, preload 2, `R_DMAC_Reset(..., &duty[2], GTCCRC, dma_count)` |
| `k1_ws281x_gpt_dma_hw_poll` | same | Timeout; latch when `dma_complete && waveform_complete` and reset elapsed |
| `k1_ws281x_gpt_dma_hw_busy` / `_ready` / `_abort_low` | same | Ownership |
| `k1_ws281x_gpt_dma_hw_snapshot` | same | v1 728-byte `k1_ws281x_hw_diag_t` |
| `k1_ws281x_gpt_dma_hw_snapshot_v2` | same | v2 1420-byte `k1_ws281x_hw_diag_v2_t` |
| `k1_ws281x_gpt_dma_hw_completions` / `_errors` / `_last_fault` / `_last_bits` | same | Counters |
| `k1_ws281x_build_gpt_duty` | `ws281x_waveform.c` | MSB-first duty words |
| `k1_ws281x_gpt_pin_can_pwm` | `ws281x_gpt_dma.c` | **P601 only**; P004 false |
| `k1_ws281x_diag_profile` | `ws281x_diag.h` | Profiles 1–4 |

Guard (do not weaken):

```c
/* dmac_done and gpt0_overflow */
if (dmac_ctrl.p_reg->DMCRA != 0u) { set_fault(K1_WS281X_FAULT_DMA); return; }
```

Fault 6 = `K1_WS281X_FAULT_DMA`. Historical frames: 3468 / 2063 / 2030, remaining **1**.

### Pattern 2: Opcode 22 sizes (do not break clients)

| Request | Size | Function | Layout |
|---------|------|----------|--------|
| empty (`size == 0`) | **728** | `k1_ws281x_gpt_dma_hw_snapshot` | v1 `K1_WS281X_GPT_DIAG_VERSION=1` |
| LE uint32 **2** | **1420** | `k1_ws281x_gpt_dma_hw_snapshot_v2` | v2 header + 3 summary words + 680-byte witness |
| any other payload | reject status 3 | `fixture_app.cpp` | keep |

Host: `transact(22, struct.pack('<I', 2))` then `decode_v2(body)` (`V1_BYTES=728`, `V2_BYTES=1420`, `PAYLOAD_CAP=480`).

Witness fields to retain: `valid`, `frame_id`, `profile`, `payload_bytes`, `payload_fnv1a32`, `duty_fnv1a32`, `expected_dma_words`, `dma_source_start` (`&duty[2]`), `dmsts`, `dmctl`, `terminal.dmac_count`.

### Pattern 3: Colour-integrity CLI (copy)

```sh
python3 scripts/run_colour_integrity.py \
  --build "$TITAN_COLOUR_BUILD_DIR" \
  --output "$TITAN_NEW_COLOUR_RUN_DIR" \
  --phase-s 20 \
  --restore-settings "$TITAN_VERIFIED_RESTORE_SETTINGS_FILE"
```

- `--output` must not exist (`mkdir(..., exist_ok=False)`).
- `--phase-s` finite and **10–20**; documented comparability is **20**.
- Restore file is the **intended leave-state**, not guessed brightness 255. Historical LED-first restore used 255; current autostart default is **24** (see Phase 6). If unavailable, runner keeps captured pre-test settings.
- Phases: `zero-output` brightness 0; `warm-low-load` 24 + 40 ms pause; `warm-usb-stress` 24 continuous.
- On first GPT fault: write `*-first-fault-gpt.bin`, fail, restore, **release CDC**. No reset/retry inside scorer.
- `score_phase` requires GPT `attempts`/`dma_irqs`/`stop_irqs`/`frames` all advance; `frames` not greater than min(dma,stop) IRQs; PDM `ap_hops`/`paired_slots`/`last_capture_end_us` advance; zero deltas on `rearm_denied pair_skew_drops asrc_starved gain_clip_pos gain_clip_neg` and per-lane `error_callbacks overflow_events drop_events recovery_count packing_mismatch`. Rails retained, not acoustic qualification.

### Pattern 4: First-divergence branches (test; do not preselect)

| Captured observation | Bounded next work | Stay parked |
|----------------------|-------------------|-------------|
| 3072 legal bits, payload legal, reset valid, terminal **DMCRA==1** | Trace preload consumption, final DMA request, GPT0 overflow / ELC stop, completion accounting vs timeline. Explain discrepancy **before** changing the guard | Fourth arbitration map |
| Missing/wrong final data or clipped last pulse at **P601** | GPT event count, buffering, ELC stop ordering, last DMA request from **measured** edges | Render/palette edits |
| P601 conforms, **DIN** does not | 74HCT2G34GW, cable, ground, analogue levels/supply. Logic analyser ≠ margins | New pin |
| Both probes lack valid signal | Supply, continuity, peripheral ownership, trigger | Editing render code |
| Wire conforms, colours still fail | Part/payload order, known-good-strip/control (`FASTLED-PORT-DESIGN.md`), optical dump | Closing visible ticket from bit count alone |

### Anti-patterns

- **Fourth DMA map:** PDM-first `d9700b14…`, round-robin `49c582db…`, LED-first `a3f37e8a…` already failed the same way.
- **`DMCRA <= 2` waiver:** mutation `incomplete_dma_waiver` must remain rejected.
- **Crediting a frame from one IRQ:** `check_gpt` forbids `frames > min(dma_irqs, stop_irqs)`.
- **Attaching the analyser after latch:** transmitter is stopped; waveform is gone.
- **Reusing frame 2030 / predicted dir** as the new event.
- **Inventing a pin** or treating P004 as GPT.
- **Captain as plate validator.**

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---------|-------------|-------------|-----|
| Duty encoding | New bit-bang math | `k1_ws281x_build_gpt_duty` | Quantisation and preload already tested |
| Remaining count | Software guess | `DMCRA` / `dmac_count` / `dma_remaining` | Silicon remainder is the fault |
| v2 decode | Ad-hoc struct | `decode_v2` | Layout `_Static_assert` 728/1420 |
| Colour soak | New USB script | `run_colour_integrity.py` | Restore + CDC + PDM rails |
| Packing oracle | New FastLED port | Existing `packPixel` / profile table | Port design already chose GPT |
| Arbitration | New DMCTL experiment | Existing three receipts | Same terminal remainder |

**Key insight:** The guard already tells the truth (one word left). WP4 measures **where the stream diverges**, then patches that surface.

## Common Pitfalls

### Pitfall 1: Fourth arbitration map
**What goes wrong:** Another DMAC channel shuffle, same remainder.  
**Why:** Three policies already isolated that hypothesis.  
**How to avoid:** WP4-05 branches only.  
**Warning signs:** Plan text mentioning `led-first` / round-robin as the next flash.

### Pitfall 2: Weakening DMCRA
**What goes wrong:** Incomplete frames credited; mutation suite goes red.  
**How to avoid:** Keep `!= 0u` in **both** `dmac_done` and `gpt0_overflow`.  
**Warning signs:** `DMCRA <= 2` or “ignore last two words”.

### Pitfall 3: Analyser after fault
**What goes wrong:** Empty capture, invented waveform.  
**How to avoid:** Arm P601 + DIN **before** Rearm/reproduce.  
**Warning signs:** “We’ll attach after it stops.”

### Pitfall 4: Impersonating frame 2030
**What goes wrong:** WP1 F-01/F-02 false physical PASS.  
**How to avoid:** New boot/run/frame identity; old witness immutable.  
**Warning signs:** Scorer still hard-coded to `a3f37e8a` frame 2030 as the only legal event.

### Pitfall 5: Brightness 255 restore guess
**What goes wrong:** Leave-state is not current boot.  
**How to avoid:** Bind the actual restore JSON. Autostart morph default is 24.  
**Warning signs:** Hard-coded 255 without a checkpoint file.

### Pitfall 6: Optical as wire PASS
**What goes wrong:** Visible ticket closed from bit count or Captain eyes.  
**How to avoid:** Separate rows; dumps for emit; optical only with wire.

## Code Examples

Verified from current source (not silicon proof):

### Remaining-count guard

```c
/* Source: platform/ra8p1/ws281x_gpt_dma_hw.c dmac_done / gpt0_overflow */
if (dmac_ctrl.p_reg->DMCRA != 0u) {
    set_fault(K1_WS281X_FAULT_DMA);
    return;
}
```

### Preload vs DMA length

```c
dma_count = bits > 2u ? bits - 2u : 0u;
/* GTCCR[0]=duty[0], GTCCR[2]=duty[1]; DMA from &duty[2] */
```

### Opcode 22 in fixture

```cpp
/* platform/ra8p1/fixture_app.cpp */
if (command == K1_WS281X_GPT_DIAG_OPCODE && size == 4 &&
    get32(rx + 32) == K1_WS281X_GPT_DIAG_V2_VERSION) {
  k1_ws281x_gpt_dma_hw_snapshot_v2(&diag);
  respond(..., sizeof(diag)); /* 1420 */
}
if (command == K1_WS281X_GPT_DIAG_OPCODE && size == 0) {
  k1_ws281x_gpt_dma_hw_snapshot(&diag); /* 728 */
}
```

### Mutation that must stay rejected

```python
# scripts/test_ws281x_gpt_dma_mutations.py
'incomplete_dma_waiver': (
    '    if (dmac_ctrl.p_reg->DMCRA != 0u) {\n',
    '    if (dmac_ctrl.p_reg->DMCRA > 2u) {\n'),
```

## State of the Art

| Old approach | Current approach | When changed | Impact |
|--------------|------------------|--------------|--------|
| `DMCRA==1/2` credited | Nonzero DMCRA → fault 6 | Colour-integrity guard | False completion gone; visible corruption open |
| Historic frame 2030 as live contract | New acquisition identity | F-02 / protocol 2026-09-20 | Do not impersonate lost event |
| Scorer `source=physical` PASS | WP1 split provenance vs conformance | F-01 | Prerequisite for WP4 wire stamp |
| Three DMA maps | Stop; measure wire | 2026-09-16 | Next discriminator is P601/DIN |

**Deprecated:** treating round-robin or LED-first as an untried cure; GPIO bit-bang as product backend; P004 GPT.

**Parked (brief §7; do not schedule as WP4 work):** M33, U55, NPU/F2, Helium/MVE/cache-on, PCM1808, Titan–S3 radio, **P603/DIN-B**, power/thermal, musical-feature promotion, full optical latency, independent G8.

## Open Questions

1. **What is the first physical divergence?**
   - Known: three policies, terminal DMCRA=1, 384 zero GRB bytes, no waveform.
   - Unclear until armed capture.
   - Plan both instrument-present and WP4-11 BLOCKED paths.

2. **Which retained image if artefacts verify?**
   - Discretion: prefer a **guarded** image whose HEX/recovery path still hashes.
   - STATUS later profiler/`live-audio` writes superseded residence; do not flash a speculative gain/render combo as the diagnostic.
   - Recommendation: diagnostic candidate = current driver + GPT DMA + PDM, **no** new sensitivity/render.

3. **Instrument on the Mac?**
   - Protocol (2026-09-20): no Saleae/DSLogic/PicoScope/sigrok enumerated at that checkpoint.
   - Re-check at execution; if still absent, WP4-11 only.

## Validation Architecture

> `workflow.nyquist_validation` is true in `.planning/config.json`.

### Test framework

| Property | Value |
|----------|-------|
| Framework | Python 3 wrappers compiling C11/C++17 host tests; pytest for selected `tests/host` (no `pytest.ini`) |
| Config file | none — wrapper scripts |
| Quick run | `python3 scripts/test_ws281x_gpt_dma_hw.py && python3 scripts/test_ws281x_gpt_dma_mutations.py` |
| Full suite (this phase) | quick run + `python3 scripts/test_ws281x_gpt_dma.py` + `python3 -m pytest -q tests/host/test_colour_integrity.py tests/host/test_gpt_fault_witness.py tests/host/test_dmac_priority_stage.py` |

### Phase requirements → test map

| Req ID | Behaviour | Test type | Automated command | File exists? |
|--------|-----------|-----------|-------------------|--------------|
| WP4-00 | Identity/capture/WP7 prereqs | manual/gate | Phase 1–2 artefacts | ✅ process |
| WP4-01 | Source-bound candidate, no 4th map | review + host | mutations + build receipt `dmac_lane_map` | ✅ |
| WP4-02 | P004 cannot PWM; P601 can | unit | `python3 scripts/test_ws281x_gpt_dma.py` (`k1_ws281x_gpt_pin_can_pwm`) | ✅ |
| WP4-03 | Dual-probe arm-before | manual + WP1 scorer | `python3 scripts/score_p601_capture.py --self-test` after WP1 | ✅ scorer; ❌ live files |
| WP4-04 | v1 728 / v2 1420 | unit | `python3 scripts/test_ws281x_gpt_dma_hw.py`; `pytest tests/host/test_gpt_fault_witness.py` | ✅ |
| WP4-05 | Five branches; no 4th map | analysis + capture | WP1 scorer `first_divergence` | ❌ until capture |
| WP4-06 | Nonzero DMCRA fault; no `<=2` | unit + mutation | hw test remaining=1..3; `incomplete_dma_waiver` | ✅ |
| WP4-07 | Defect-specific negative | unit | **Wave 0** after cause known | ❌ Wave 0 |
| WP4-08 | Three phases `--phase-s 20` | target soak | `run_colour_integrity.py` | ✅ runner; ❌ this candidate |
| WP4-09 | PDM/AP/GPT progress + rails | target + host mocks | `score_phase` in `test_colour_integrity.py` | ✅ host; ❌ silicon |
| WP4-10 | Optical separate | dump/score | native RGB opcode 18 dumps; not Captain | ✅ dump path |
| WP4-11 | Missing instrument report | docs | protocol § Missing acquisition | ✅ |

### Sampling rate

- **Per task commit:** quick run (`test_ws281x_gpt_dma_hw.py` + mutations).
- **Per wave merge:** full suite above.
- **Phase gate:** full suite green **and** either same-frame capture scored (PASS/FAIL distinguished) **or** WP4-11 BLOCKED with decoder ready. No wire PASS from host.

### Wave 0 gaps

- [ ] Defect-specific host negative matching the **measured** first divergence (after WP4-05).
- [ ] Physical P601+DIN hashed files in a **new** EdgeAI directory (not predicted-2030).
- [ ] Restore-settings path bound to a verified JSON (not brightness 255 guess).

None of these gaps authorise a fourth DMA map.

## Sources

### Primary (HIGH confidence)

- `platform/ra8p1/ws281x_gpt_dma_hw.c` remaining count, guard, snapshot, submit
- `platform/ra8p1/ws281x_gpt_dma_hw.h` opcode 22 structs, 728/1420 comments
- `platform/ra8p1/fixture_app.cpp` opcode 22 branch
- `scripts/gpt_fault_witness.py` `V1_BYTES`/`V2_BYTES`/`decode_v2`
- `scripts/run_colour_integrity.py` CLI, phases, `score_phase`
- `tests/test_ws281x_gpt_dma_hw.c` `_Static_assert` 728/1420; remaining 1–3
- `docs/Titan_RA8P1_Takeover_Execution_Brief_2026-09-20.md` §5 WP4, §3 colour table
- `docs/evidence/K1-RA8P1-002/TITAN-COLOUR-INTEGRATION.md`
- `docs/evidence/K1-RA8P1-002/FASTLED-PORT-DESIGN.md` pins/profiles
- `docs/evidence/K1-RA8P1-002/P601-DIN-CAPTURE-PROTOCOL.md`
- Canonical Titan recall: `PLATFORM_MEMORY.md` topics `pins`, `dma`, `ws2816`

### Secondary (MEDIUM confidence)

- Repository-recorded silicon: frames 3468/2063/2030, HEX hashes in colour-integration table — history, not a 2026-09-20 re-measurement
- `combined-runtime-contract.json` DMA map is **PDM-first default**, not every candidate

### Tertiary (LOW / NOT_VERIFIED)

- Physical root cause, analogue margins, photons, current resident image

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — files and function names read in-tree
- Architecture: HIGH — driver + protocol; MEDIUM — which candidate is still recoverable
- Pitfalls: HIGH — three failed maps + mutation names in source
- Physical divergence: NOT_VERIFIED

**Research date:** 2026-09-20
**Valid until:** 7 days or next contradictory capture/driver identity

---

## Appendix — facts Phase 8 (latency) and Phase 9 (docs) will need

Do **not** write Phase 8/9 RESEARCH files here. Copy these into those plans later.

### From `docs/EXECUTION-BRIEF.md` §20 (Phase 8 LAT-04 + Phase 9 DOC-01)

Thirteen original deliverable names under `docs/evidence/K1-RA8P1-001/` (complete the **001↔002 crosswalk**, do not recreate empty 001 docs): `00-environment-audit.md`, `01-boot-gate.md`, `02-shared-time-parity.md`, `03-ap-scalar-parity.md`, `04-scalar-performance.md`, `05-audio-time-hardware.md`, `06-capture-bringup.md`, `07-live-ap.md`, `08-musical-render.md`, `09-vp-parity.md`, `10-final-comparison.md`, `mutation-receipts.txt`, `final-validation.txt`.

Fourteen ruling questions for `docs/RA8P1-vs-RT1062-RULING.md` (today a stub waiting on G4 Q2 / later joins): (1) compute architecture port cleanliness, (2) platform-neutral portions, (3) backend replacements, (4) scalar M85 p50/p95/p99/max vs RT1062, (5) memory/code-size headroom, (6) audio capture/timestamp quality, (7) musical render scheduling, (8) toolchain/debug maturity, (9) BSP/driver friction, (10) thermal/power if available, (11) what MVE could improve, (12) whether M33 stays parked, (13) whether U55 has a justified K1 role, (14) whether RA8P1 is preferred DualMCU eval processor.

Allowed stamps only: `PROMOTE_RA8P1_FOR_DUALMCU_EVAL` | `KEEP_RT1062_PRIMARY_DUALMCU_EVAL` | `CONTINUE_PARALLEL_EVALUATION` | `REJECT_RA8P1_FOR_K1`. Captain owns K1-C. Do not rule shipping-S3 architecture.

K1-L (Phase 8, not WP4): newest-sample DMA-return → transfer-complete of first frame with that AP generation; p50 ≤12,000 µs; ≥1,000 joined Titan frames; real music; observer off; report analysis / wait / render / wire; frontend+latch separate; p99 15,444 µs proposed; AP service p99 8,000 µs ≠ 7,500 µs frozen hop. Never USB-read time as event time. S3 43-frame p50 11,576 µs unbound until receipt hashed. Do not relabel S3 as RT1062.

### From `OUTPUT-PATH-COMPOSITION.md` (Phase 8 joins + Phase 9 product row)

Order: **effects → blend → treatment → edge policy → gain → joint current limit → stage/show**. F1 320 Pixel8 is pre-treatment. Opcode 11 is not this chain. TRUE16 `0x12AB` via `packPixel`/`splitChannel160`; Pixel8 ×257 is not precision. DualMCU SHAs for blend/policy stay **named derivatives**, not silent manifest rewrites. Physical four-lane / P004 GPT remain outstanding stamps: PASS/FAIL/BLOCKED/NOT TESTED.

### From `FASTLED-PORT-DESIGN.md` (Phase 8 wire term + Phase 9 topology)

P601=GTIOC6A; P004 no GPT/serial-output; P603=GTIOC7A later only with Captain. WS2816C-1313-4P: T0H 250 / T1H 650 / period ≥1250 ns / reset 300 µs (>280). Profile 1 WS2812 250/875/1250 is **not** that part. 160×48 bits at 800 kbit/s = **9.6 ms** > 8.333 ms/120 Hz; 80 px = 4.8 ms + reset. Duty SRAM: **15,360 bytes** per 80×48-bit lane (`80*48*4`); four lanes 61,440/bank; two banks 122,880. Function: `k1_ws281x_gpt_buffer_bytes`. Known-good-strip/control table (WS2812 vs Titan vs WS2816) is the optical/transport discriminator after WP4 wire. Do not wrap already-packed TRUE16 in FastLED `WS2816Controller`. GPIO IRQ-off **5.412115 ms** stays diagnostic.

### Phase 9 parked ledger (brief §7)

Resume-when only: M33, U55/E2/G7, F2 NPU, Helium/MVE/FPU/cache, PCM1808, radio bridge, P603/DIN-B, power/thermal, musical feature promotion, full optical latency, G8. Handback five points: commit/hashes/UID; every §8 row PASS/FAIL/BLOCKED/NOT TESTED; finding+negative+unmeasured; CDC/fault/settings/waiter; remaining tasks owner or **unassigned**.

## RESEARCH COMPLETE
