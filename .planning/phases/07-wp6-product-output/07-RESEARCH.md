<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Order: effects → blend → treatment → edge policy → gain → joint current limit → stage/show
- No double brightness/treatment
- Both logical channels through the full chain; F1 renderer pixels alone are insufficient
- TRUE16 independent of Pixel8 renderer
- `packBenchGrb48Lane()` expands Pixel8 by ×257 and does not claim extra precision
- Test `0x12AB` through wide submit/packer without double packing, scaling, or 8-bit dither residue
- 160×48 bits at 800 kbit/s = 9.6 ms payload > 8.333 ms/120 Hz
- Each 160-pixel edge needs parallel 80/80; two edges need four concurrent lanes
- 80-pixel lane 4.8 ms payload plus reset
- P004 cannot run selected GPT backend. P603 later only with Captain physical approval
- Do not silently reduce pixel count, colour depth, or render rate to obtain a pass
- Memory (32-bit duty-word): one 80×48-bit lane 15,360 bytes; four lanes 61,440 per bank; two banks 122,880 before other buffers
- Duty data in DMA-accessible SRAM, not DTCM

### Claude's Discretion
- Named slice identifier, provided DualMCU manifest is not silently rewritten to fake byte-identity

### Deferred Ideas (OUT OF SCOPE)
- Captain-authorised P603/DIN-B physical move and second edge (parked §7).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WP6-01 | Admit local modules as named reviewed slice; do not fake DualMCU byte-identity | `PRODUCT_OUTPUT_CHAIN_FILES` staged with `--palette-runtime`; **not** in `import-slices.json` |
| WP6-02 | Full order both channels; no double brightness/treatment | Host `composeLogicalChannels`; live `PaletteRuntime::step` **incomplete** |
| WP6-03 | TRUE16 `0x12AB` wide path; ×257 is not precision | `packPixel` / `splitChannel160`; `packBenchGrb48Lane` |
| WP6-04 | WS2816C-1313-4P on proven single GPT lane after WP4 | Profile 3 vs 1; known-good-strip table |
| WP6-05 | Topology before multi-lane; 9.6 ms vs 8.333 ms | Four concurrent 80-pixel lanes |
| WP6-06 | P004 no GPT; Captain owns DIN move; no silent reduce | `k1_ws281x_gpt_pin_can_pwm`; `K1_TITAN_DIN_B` |
| WP6-07 | SRAM duty budget 15360 / 61440 / 122880 | `k1_ws281x_gpt_buffer_bytes` |
| WP6-08 | Halves/edges latch; centre; joint limit; timestamps; GPIO diagnostic only | 5.412 ms IRQ-off GPIO |
| WP6-09 | Physical status PASS/FAIL/BLOCKED/NOT TESTED at actual scope | Do not conceal blocked transmitter |
</phase_requirements>

# Phase 7: WP6 product output — Research

**Researched:** 2026-09-20
**Domain:** DualMCU-order output composition, TRUE16 packing, WS2816/multi-lane topology on RA8P1
**Confidence:** HIGH for host packer/order APIs and pin/timing facts; MEDIUM for ARM staging vs live `step()`; **NOT_VERIFIED** for physical WS2816/four-lane
**Default verdict:** NOT_VERIFIED

## Summary

Host composition **exists** (`tests/host/test_output_composition.cpp`) and prints `physical_lanes=OUTSTANDING p004_gpt=UNSUPPORTED`. Local `frame_blend.*` and `product_runtime_policy.*` are **named DualMCU derivatives** (must stay non-byte-identical). They are **absent** from `docs/import-slices.json`. Current `scripts/build_scalar.py` **does** stage them as `PRODUCT_OUTPUT_CHAIN_FILES` when `--palette-runtime` is set — the takeover brief’s “not in the build local-file list” sentence is **stale relative to this checkout**. `RA8P1_LOCAL_K1_FILES` still only lists `ws2816_pack.h` plus tempo slice.

`PaletteRuntime::step` still only calls `applyProductOutputTreatment` then pack-time brightness. It does **not** call `applyFrameBlending`, `applyProductEdgePolicy`, `applyProductOutputGain`, or `applyProductCurrentLimit`. Do not treat staging or the host fixture as live-path admission.

TRUE16 test pattern is **`0x12AB`**. `packBenchGrb48Lane` uses Pixel8×**257** and must not be used as the wide proof. Physical four-lane/P603 stay parked. WP6 host work may proceed while hardware is absent; it must not conceal a blocked WP4 transmitter.

**Primary recommendation:** Name a reviewed derivative slice without rewriting `import-slices.json` to fake identity; wire the DualMCU order into the live palette path once, with double-gain/treatment negatives; keep physical rows BLOCKED/NOT TESTED until WP4 wire + Captain topology.

## Standard Stack

### Core

| Artefact | DualMCU sha256 (pin `6b1e7bc5…`) | Local status | Purpose |
|----------|-----------------------------------|--------------|---------|
| `src/k1/core/visual/frame_blend.{h,cpp}` | h `610519b4…` / cpp `03b9f47d…` | derivative, not in import-slices | `applyFrameBlending` |
| `src/k1/core/visual/product_runtime_policy.{h,cpp}` | h `ec1a5375…` / cpp `543fc91a…` | derivative, not in import-slices | edge / gain / joint limit |
| `src/k1/core/visual/product_output_treatment.{h,cpp}` | in import-slices `product` | imported | `applyProductOutputTreatment` |
| `src/k1/core/visual/ws2816_pack.h` | local (`RA8P1_LOCAL_K1_FILES`) | `packPixel`, `splitChannel160` | TRUE16 GRB48 |
| `src/k1/core/visual/product_effect_renderer.*` | in import-slices | `renderProductChannel` | effects |

### Supporting

| Artefact | When to use |
|----------|-------------|
| `scripts/test_output_composition.py` | Compile host chain + prove local ≠ DualMCU byte-identity |
| `scripts/test_gold_extract.py` | `k1_ws281x_gpt_buffer_bytes` 30720/61440/122880 |
| `scripts/test_ws281x_gpt_dma.py` | P004 cannot PWM |
| Profile 3 | WS2816C-1313-4P 250/650/1250 ns, 6 B/px, 80 px/lane |
| Profile 1 | Bench WS2812 250/875/1250, 3 B/px, 128 px — **does not qualify WS2816** |

### Alternatives considered (rejected)

| Instead of | Could use | Tradeoff |
|------------|-----------|----------|
| Silent `import-slices.json` rewrite | Named `local-derivative` receipt (already in `build_scalar.py`) | Faking byte-identity |
| `packBenchGrb48Lane` as TRUE16 | `packPixel`/`splitChannel160` with `Pixel16{0x12AB,…}` | ×257 destroys LSBs (`0x12*257 ≠ 0x12AB`) |
| Sequential 160×48 on one wire at 120 Hz | Four concurrent 80-pixel GPT lanes | 9.6 ms > 8.333 ms |
| P004 GPT or invented pin | P603 later + Captain | P004 has no GPT/serial-output |
| Shrink pixels/depth/Hz | Honest BLOCKED | Forbidden pass-hunting |
| GPIO `k1_ws281x_diag_emit` | GPT/DMA backend | 5.412 ms IRQ-off |

**Host commands:**

```bash
python3 scripts/test_output_composition.py
python3 scripts/test_gold_extract.py
python3 scripts/test_ws281x_gpt_dma.py
```

Expected host line: `K1_OUTPUT_COMPOSITION=PASS chain=PASS true16=PASS double_gain_rejected=PASS repeated_treatment_detected=PASS physical_lanes=OUTSTANDING p004_gpt=UNSUPPORTED`.

## Architecture Patterns

### Current vs required path

```
DualMCU production_runtime (~500–570 at pin):
  renderProductChannel → applyFrameBlending → applyProductOutputTreatment
  → applyProductEdgePolicy → applyProductOutputGain → applyProductCurrentLimit
  → PixelPort::submit / show

Titan PaletteRuntime::step today:
  renderProductChannel (or preview/bounce/centre) → applyProductOutputTreatment
  → packBenchGrb (Pixel8 GRB24 × brightness) OR packBenchGrb48Lane (×257)

Titan host fixture (test_output_composition.cpp composeLogicalChannels):
  blend → treatment → edge → gain → joint limit → splitChannel160(Pixel16)
  (effects assumed already in the Pixel8 buffers)
```

### Pattern 1: Named slice admission (WP6-01)

Do **not** add blend/policy to `docs/import-slices.json` to look imported. Keep:

```python
# build_scalar.py
PRODUCT_OUTPUT_CHAIN_FILES = [
    'core/visual/frame_blend.h', 'core/visual/frame_blend.cpp',
    'core/visual/product_runtime_policy.h', 'core/visual/product_runtime_policy.cpp',
]
# staged only if args.palette_runtime
receipt['product_output_chain_sources'] = {
    'status': 'local-derivative', 'dualmcu_pin': PIN, 'byte_identical': False, ...
}
```

`test_output_composition.py` **fails** if local SHA equals DualMCU SHA. Discretion: name the slice string; do not rewrite the DualMCU manifest.

`palette_runtime.cpp` does not `#include` blend/policy. Staging without `step()` calls is **not** linked-use evidence of the full order. Planner must add calls **or** an explicit “staged but unused” BLOCKED for live composition.

Session-start git snapshot listed only the takeover brief as untracked; completeness gap is the **live step()**, not an assumed dirty patch.

### Pattern 2: Copy-ready composition APIs

| Function | Header | Notes |
|----------|--------|-------|
| `renderProductChannel` | `product_effect_renderer.h` | Effects; F1 320 Pixel8 is **pre-treatment** |
| `applyFrameBlending` | `frame_blend.h` | A 180 / B 120 DualMCU moods; host uses mood 0, dt 0.0075F |
| `applyProductOutputTreatment` | `product_output_treatment.h` | Dither `{64,128,192,255}` when enabled; already in `step()` |
| `applyProductEdgePolicy` | `product_runtime_policy.h` | Both 160-pixel spans |
| `applyProductOutputGain` | same | master Q0.16 + policy brightness; **once** |
| `applyProductCurrentLimit` | same | **Joint** both channels |
| `packPixel` | `ws2816_pack.h` | G_hi,G_lo,R_hi,R_lo,B_hi,B_lo |
| `splitChannel160` | same | 80/80 halves → two 480-byte lanes |
| `PaletteRuntime::packBenchGrb` | `palette_runtime.cpp` | 128 GRB24 bench resample + brightness |
| `PaletteRuntime::packBenchGrb48Lane` | same | Pixel8×257; **not** TRUE16 proof |

TRUE16: `Pixel16{0x12AB,0,0}` → `packPixel` bytes `[2]=0x12,[3]=0xAB`. Host asserts `0x12*257 != 0x12AB`. Fixture opcode 11 TRUE16 path is **blocked** when `K1_PALETTE_GPT_DMA` (error 10) — do not relabel opcode 11 as this chain.

Centres: `kCentreLeft=79`, `kCentreRight=80`. Brightness in `configure()` is forced to **255 inside ChannelVisualControls** so physical brightness is applied **once** later — if you then also `applyProductOutputGain` **and** `packBenchGrb` scale, that is double brightness.

### Pattern 3: Topology, pins, memory (do not invent)

| Fact | Value | Source |
|------|-------|--------|
| Logical geometry | 2×160, centres 79/80 | `pixel_topology.h` |
| Bench WS2812 | 128, centres 63/64 | brief + bounce |
| DIN-A | P601 `K1_TITAN_DIN_A 0x0601` GTIOC6A GPT6 | `titan_led_pins.h` |
| DIN-B | P004 `0x0004` **no GPT** | `k1_ws281x_gpt_pin_can_pwm` false |
| Later candidate | P603 GTIOC7A | Captain physical only |
| 160×48 @ 800 kbit/s | **9.6 ms** payload | FASTLED-PORT-DESIGN |
| 80×48 | **4.8 ms** + reset | same |
| Duty cap | `80 * 48` words = 3840×4 = **15360** bytes | `K1_WS281X_GPT_DUTY_CAP` |
| Four lanes one bank | **61440** | `k1_ws281x_gpt_buffer_bytes(2,160,3,1)==61440` (equivalent) |
| Two banks | **122880** | `(2,160,3,2)` |
| Placement | DMA SRAM, **not DTCM** | AGENTS.md / brief |

GPIO diagnostic: LED-HANDOVER measured IRQ-disabled span **5.412115 ms**. Cannot replace concurrent live-audio GPT.

WS2816 qualification: change **profile+packing** on the **already proven** single P601 engine; known-good WS2812 control first (`FASTLED-PORT-DESIGN.md` control table). Profile 1 T1H 875 ns is outside WS2816C-1313-4P’s 520–800 ns one-high window — use **profile 3** (650 ns). Profile 4 is GRB48 over the WS2812 875 ns transport; keep 3 and 4 distinct.

### Anti-patterns

- Rewriting import-slices so derivatives look imported.
- Calling F1 renderer pixels “output parity”.
- Using ×257 or opcode 11 GPT-blocked path as TRUE16 silicon proof.
- Silent 80-pixel or 24-bit or 60 Hz shrink to beat 9.6 ms.
- Fourth DMA map, P004 PWM, unapproved P603.
- Concealing WP4 BLOCKED behind host PASS.

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---------|-------------|-------------|-----|
| GRB48 pack | New byte swap | `packPixel` | Wire order already G then R then B, 16-bit |
| 80/80 split | Ad-hoc indices | `splitChannel160` | Rejects count ≠ 160 |
| Duty size | Spreadsheet guess | `k1_ws281x_gpt_buffer_bytes` | Host gold already locked |
| FastLED `WS2816Controller` wrap | Double pack | Raw bytes into GPT submit | Port design forbids wrapping packed TRUE16 |
| Joint current | Per-channel clip only | `applyProductCurrentLimit` both spans | Coupled scaling is the product rule |

## Common Pitfalls

### Pitfall 1: Staged but unused
**What goes wrong:** ARM image contains blend.o but `step()` never calls it.  
**How to avoid:** Linked-use evidence = disassembly symbols **and** a frame comparison through the live path.  
**Warning signs:** Receipt `product_output_chain_sources` treated as WP6-02 PASS.

### Pitfall 2: Double brightness / treatment
**What goes wrong:** `controls.brightness=255` plus pack scale plus `applyProductOutputGain`, or two dither steps.  
**How to avoid:** Host already detects double gain and repeated treatment — port those asserts onto the live path.

### Pitfall 3: ×257 as TRUE16
**What goes wrong:** Low byte `0xAB` never appears.  
**How to avoid:** Wide `Pixel16` through `packPixel` without 8-bit expand.

### Pitfall 4: WS2812 stamp on WS2816
**What goes wrong:** Illegal T1H / 24-bit payload on 48-bit part.  
**How to avoid:** Profile 3 after WP4 single-lane proof; control-strip table.

### Pitfall 5: Pass by shrinking
**What goes wrong:** 80 px or 24-bit or 60 Hz labelled product.  
**How to avoid:** BLOCKED/NOT TESTED at true scope.

### Pitfall 6: Dirty-file assumption
**What goes wrong:** Planning as if an uncommitted complete chain exists.  
**How to avoid:** `step()` read shows treatment-only; host test is a **fixture**, not the runtime.

## Code Examples

### Host TRUE16 (copy)

```cpp
Pixel16 wide[kPixelsPerChannel]{};
wide[kCentreLeft] = {0x12AB, 0, 0};
wide[kCentreRight] = {0, 0, 0x12AB};
assert(splitChannel160(wide, kPixelsPerChannel, show.lane_a, show.lane_b));
assert(show.lane_a[kCentreLeft * 6U + 2U] == 0x12 &&
       show.lane_a[kCentreLeft * 6U + 3U] == 0xAB);
```

### Not TRUE16

```cpp
// palette_runtime.cpp packBenchGrb48Lane
return uint16_t((uint32_t(value) * 257U * config_.brightness) / 255U);
```

### Memory helper

```c
/* tests/host/test_gold_extract.cpp */
assert(k1_ws281x_gpt_buffer_bytes(2u, 80u, 3u, 1u) == 30720u);  /* 2×80×48-bit */
assert(k1_ws281x_gpt_buffer_bytes(2u, 160u, 3u, 1u) == 61440u); /* four 80s worth */
assert(k1_ws281x_gpt_buffer_bytes(2u, 160u, 3u, 2u) == 122880u);
```

### Pin

```c
int k1_ws281x_gpt_pin_can_pwm(uint32_t pin) {
    return pin == K1_WS281X_GPT_PIN_P601;
}
```

## State of the Art

| Old | Current | Impact |
|-----|---------|--------|
| Brief: blend/policy not in local-file list | `PRODUCT_OUTPUT_CHAIN_FILES` staged with palette | Update plans to “staged derivative / unused in step()” |
| Opcode 11 first light | Blocked under GPT_DMA | Do not use as composition proof |
| Pixel8 wire | TRUE16 packer exists | Live submit still Pixel8 bench GRB24 |
| One 160×48 chain | Physically too long at 120 Hz | Four lanes required; P603 parked |

**Parked:** P603/DIN-B and second edge; M33/U55/NPU; K1-L (Phase 8); fourth DMA map.

## Open Questions

1. **When to call blend/edge/gain/limit in `step()`** — required for WP6-02 live admission; host fixture is not enough. Recommend one explicit integration in palette runtime with double-apply negatives, after WP5 candidate is named.
2. **Linked-use evidence** — `build_scalar.py` asserts palette symbols; it does **not** currently assert `applyFrameBlending` / `applyProductCurrentLimit` in the ELF. Add those asserts when the live path uses them.
3. **Physical WS2816** — BLOCKED on WP4 wire + populated part + profile 3; NOT TESTED is honest if instrument/strip absent.
4. **Second 160-pixel edge** — physical dependency; Captain; do not invent routing.

## Validation Architecture

> Nyquist validation enabled.

### Test framework

| Property | Value |
|----------|-------|
| Framework | C++17 host fixture via `scripts/test_output_composition.py`; gold-extract; GPT pin model |
| Config file | none |
| Quick run | `python3 scripts/test_output_composition.py` |
| Full suite (this phase) | quick + `python3 scripts/test_gold_extract.py` + `python3 scripts/test_ws281x_gpt_dma.py` |

### Phase requirements → test map

| Req ID | Behaviour | Test type | Automated command | File exists? |
|--------|-----------|-----------|-------------------|--------------|
| WP6-01 | Named derivative; not byte-identical; not in import-slices | unit | `python3 scripts/test_output_composition.py` (`PRODUCT_OUTPUT_DERIVATIVE=PASS`) | ✅ |
| WP6-02 | Full order both channels; no double apply | unit (host fixture) | same `chain=PASS double_gain_rejected repeated_treatment` | ✅ fixture; ❌ live `step()` Wave 0 |
| WP6-03 | `0x12AB` wide pack | unit | same `true16=PASS`; `packPixel` | ✅ host; ❌ live GPT submit |
| WP6-04 | WS2816C profile on proven lane | target | profile 3 after WP4 | ❌ physical |
| WP6-05 | 9.6 ms / four 80s | analysis + cost | `k1_ws281x_gpt_cost` / gold-extract | ✅ host numbers |
| WP6-06 | P004 no GPT | unit | `test_ws281x_gpt_dma.py` | ✅ |
| WP6-07 | 15360/61440/122880 | unit | `test_gold_extract.py` buffer_bytes | ✅ |
| WP6-08 | Concurrent latch/centre/limit/timestamps | target | none for four lanes | ❌ Wave 0 / parked P603 |
| WP6-09 | Honest physical stamp | docs/receipt | host already prints OUTSTANDING | ✅ pattern |

### Sampling rate

- **Per task commit:** `test_output_composition.py`.
- **Per wave merge:** + gold-extract + P004 PWM negative.
- **Phase gate:** host suite green; live-path order either implemented+tested **or** explicit unused-in-step BLOCKED; physical rows not green without WP4+instrument.

### Wave 0 gaps

- [ ] Live `PaletteRuntime::step` (or successor) test that both channels traverse blend→treatment→edge→gain→joint limit→pack **once**.
- [ ] ELF/symbol assert for those functions when `--palette-runtime`.
- [ ] TRUE16 `0x12AB` through `k1_ws281x_gpt_dma_hw_submit` (not ×257) on host model / later silicon.
- [ ] Profile-3 WS2816C bound test distinct from profile-1 (HOST timing already in `k1_ws281x_diag_profile`; physical after WP4).
- [ ] Four-lane concurrency tests — **blocked** on Captain P603/second edge; do not invent pins.

## Sources

### Primary (HIGH)

- `docs/import-slices.json` (no blend/policy)
- `scripts/build_scalar.py` `PRODUCT_OUTPUT_CHAIN_FILES` / `RA8P1_LOCAL_K1_FILES`
- `scripts/test_output_composition.py`, `tests/host/test_output_composition.cpp`
- `palette_runtime.cpp` `step` / `packBenchGrb` / `packBenchGrb48Lane`
- `ws2816_pack.h`, `frame_blend.h`, `product_runtime_policy.h`, `product_output_treatment.h`
- `titan_led_pins.h`, `ws281x_gpt_dma.c` `k1_ws281x_gpt_pin_can_pwm`
- `ws281x_waveform.h` `K1_WS281X_GPT_DUTY_CAP`, `k1_ws281x_gpt_buffer_bytes`
- `docs/evidence/K1-RA8P1-002/OUTPUT-PATH-COMPOSITION.md`
- `docs/evidence/K1-RA8P1-002/FASTLED-PORT-DESIGN.md`
- Takeover brief §5 WP6

### Secondary (MEDIUM)

- OUTPUT-PATH changelog 2026-09-20 “admitted to palette staging” = staging, not `step()` integration
- Gold-extract 2×160×profile3 as byte-equivalent to four 80-pixel lanes

### Tertiary (NOT_VERIFIED)

- Physical WS2816C, four concurrent lanes, analogue strip identity, photons

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — files/SHAs/functions read
- Architecture: HIGH for host order/pins; MEDIUM for “admitted” vs unused `step()`
- Pitfalls: HIGH
- Physical product output: NOT_VERIFIED

**Research date:** 2026-09-20
**Valid until:** 7 days or a live `step()` integration that changes the unused-path claim

## RESEARCH COMPLETE
