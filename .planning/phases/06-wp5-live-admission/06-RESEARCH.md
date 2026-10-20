<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Mode 32 both logical channels, active+emit, no four-second carousel.
- Record actual brightness/palettes/direction. Do not silently substitute historical brightness 255.
- SINCDEC 49 / SINCRNG 10, nominal ×16 with loud guard. Need matching target evidence.
- Changing gain is not the default next task.
- Preserve reserve-before-rearm/FILLING and programme/measurement separation.
- Do not silently feed 16 kHz diagnostic to 24 kHz AP or fall back to fixture audio.
- Microphones are LinkMems LMD2718T261-OA1, not IM69D130. U14 LOW/ch2 RISE programme; U13 HIGH/ch0 FALL measurement. Acoustic identity unresolved unless measured.
- Two-sample linear interpolation in `k1_asrc_24k.c`; `pdm_target.c` locks initial measured rate after about two seconds. That is not anti-alias, fidelity, or later-drift proof.
- Historical probes: no bins below 110 Hz; low-bin tone/label disagreement; 160→80 half-time tempo. Reproduce silently against current contract before calling defects.
- `last_live_valid_` at zero time and wrap.
- Five-second dwell rendering uninterrupted.
- Five-second quiet/black tail without history reinit or min-one residuals.
- Travelling structure ≠ centroid of a filling half-strip.
- Centre-origin: 160 px centres 79/80; 128 bench 63/64.
- 45 s combined runner is preflight, not G4 240,000-hop qualification.
- Observer-off/on: dense 20 Hz USB froze emission.

### Claude's Discretion
- Named candidate string, provided all sources/config are bound.
- Whether acoustic identity stays `ACOUSTIC_IDENTITY_UNRESOLVED` if no controlled observation.

### Deferred Ideas (OUT OF SCOPE)
- Product four-lane, K1-L physical latency stamp, NPU/MVE.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WP5-00 | Prerequisite: repaired transmitter admitted at bench scope | Phase 5 exit; no inherited four-lane or K1-L |
| WP5-01 | One named integration candidate; bind sources/config | `--pdm-target --palette-runtime --palette-autostart --palette-morph --palette-gpt-dma` |
| WP5-02 | Autostart mode 32 both channels, flags 5, no carousel; record real settings | `kLiveAudioBootMode`; `config.flags = 5U`; brightness **24** not 255 |
| WP5-03 | Read back SINCDEC/SINCRNG, DMAC ownership, rates, ASRC lock, cadence | Compile-time 49/10; opcode 6 **lacks** sinc fields today — gap |
| WP5-04 | FILLING reserve-before-rearm; no 16 kHz→24 kHz silent feed | `k1_pdm_stream_context_filling_slot`; `k1_pdm_target_pull_ap_hop` |
| WP5-05 | Acoustic identity or `ACOUSTIC_IDENTITY_UNRESOLVED` | Opcode 6 hard-codes `"unproven"` |
| WP5-06 | Both rail signs, clips, ASRC characterisation; historical probes vs contract | `sat_pos`/`sat_neg`, `gain_clip_*`; `k1_asrc24_*` |
| WP5-07 | Real music via repaired `run_mode32_real_audio.py --audio … --build-id …` | F-06 is Phase 3; WP5 consumes the repaired runner |
| WP5-08 | Presence / injection / decay; `last_live_valid_` zero/wrap; 5 s dwell/tail | `applyK1PresencePolicy`; `kTitanSilenceDwellUs`; `ageAuthoredHistory` |
| WP5-09 | Travelling structure ≠ centroid; native frames vs 8 Hz dump | opcode 18 480-byte native RGB; centre-origin mandate |
| WP5-10 | Combined preflight 45 s then named coverage; not G4 240k | `run_pdm_ap_gpt.py --pause-s 45` |
| WP5-11 | Observer on/off; dense 20 Hz USB must not freeze emit | bounded USB; not a production data path |
</phase_requirements>

# Phase 6: WP5 live image admission — Research

**Researched:** 2026-09-20
**Domain:** Live PDM → ASRC → 24 kHz/180 AP → palette VP on Titan, after WP4 transmitter admission
**Confidence:** HIGH for APIs, boot flags, PDM/ASRC constants; MEDIUM for F-06 runner completeness vs Phase 3; **NOT_VERIFIED** for acoustic identity, live sinc registers, combined qualification
**Default verdict:** NOT_VERIFIED

## Summary

WP5 admits **one named** image that already contains the **accepted WP4 driver** plus current intentional live behaviour. Copy existing PDM/ASRC/palette APIs; do not change gain (`K1_PDM_SENSITIVITY_Q8=4096`, loud guard 30000). Autostart is mode **32**, **flags 5** (active+emit), no carousel. Default morph brightness is **24**, not historical 255.

Opcode 6 already reports measured rate, lock, hops, both rail signs, clips, DMA channels, and hardcoded `acoustic_identity":"unproven"`. It does **not** currently print SINCDEC/SINCRNG. WP5-03 needs **target evidence** of 49/10, not another source-only `#define`.

`last_live_valid_` exists and is used. Host isolation covers hold-then-dwell; dedicated wrap/zero-time cases are incomplete. Travelling ribbons cannot be inferred from opcode-18 centroid on a filling half-strip. The 45 s `run_pdm_ap_gpt.py` is preflight only.

**Primary recommendation:** Bind one candidate; prove boot/readback/FILLING/dwell with dumps; keep acoustic identity unresolved unless a controlled observation exists; do not inherit product-output or K1-L.

## Standard Stack

### Core

| Artefact | Constant / version | Purpose | Why standard |
|----------|-------------------|---------|--------------|
| `pdm_target.c/.h` | SINCDEC **49**, SINCRNG **10**, 40 kHz, slot **296** | Dual LMD2718 capture | Current live frontend |
| `k1_pdm_sensitivity.h` | Q8 **4096** (×16), ceiling 30000 | Loud-guard scale | Restored DualMCU analogue; do not retune |
| `k1_asrc_24k.c/.h` | out 24000/180, in 36–44 kHz | Measured-rate linear interp | Admits 40 kHz (and ~41.4 kHz history) into AP |
| `palette_runtime.cpp/.h` | mode 32, flags 5, dwell 5 s | VP + presence | Current live visual |
| AP contract | 24000 Hz / 180 / 7500 µs / 80 bins / xover 40 | Qualified scalar shape | Distinct from 12.8 kHz/96 and 16 kHz diagnostic |

### Supporting

| Artefact | When to use |
|----------|-------------|
| `scripts/run_pdm_ap_gpt.py --pause-s 45` | Combined preflight after identity bind |
| `scripts/run_mode32_real_audio.py --audio … --build-id …` | Observation after Phase 3 F-06 repair |
| `scripts/run_music_baseline.py` | **Must keep refusing** synthetic room playback (`SystemExit` at `main`) |
| `scripts/test_pdm_sincrng.py` / `test_pdm_sensitivity.py` / `test_asrc_24k.py` / `test_pdm_rearm.py` | Host gates |
| `scripts/test_palette_runtime.py` | Autostart mutation, snap-freeze, centre |
| `scripts/run_quiet_gap.py` + `tests/host/test_mode32_wake.cpp` | Dwell/path isolation |
| Capsules | LinkMems **LMD2718T261-OA1**; U14 LOW/ch2 RISE programme; U13 HIGH/ch0 FALL measurement |

### Alternatives considered (rejected)

| Instead of | Could use | Tradeoff |
|------------|-----------|----------|
| Gain change | Historical quieter Q8 | **Forbidden default**; loud-guard already exists |
| 16 kHz diagnostic image | Current 40 kHz+ASRC | Wrong AP contract |
| Fixture PCM fallback | Live PDM hops | Silent lie |
| `run_music_baseline.py` playback | Real recording path | Hard-fails computer tones |
| Centroid as ribbon | Native travelling / occupancy scores | Filling half-strip moves centroid |
| 45 s soak as G4 | Historical 240,000-hop slim scalar | Different image/contract |

**Host commands:**

```bash
python3 scripts/test_pdm_sincrng.py
python3 scripts/test_pdm_sensitivity.py
python3 scripts/test_asrc_24k.py
python3 scripts/test_pdm_rearm.py
python3 -u scripts/test_palette_runtime.py
python3 scripts/test_mode32_wake.py   # SKIP_RESIDENT=1 as documented where needed
```

Target preflight (fresh output dir, after WP4 admit + WP7):

```bash
python3 scripts/run_pdm_ap_gpt.py --build "$TITAN_LIVE_BUILD_DIR" --output "$TITAN_NEW_PDM_AP_GPT_DIR" --pause-s 45
python3 scripts/run_mode32_real_audio.py --audio "$IDENTIFIED_RECORDING" --build-id "$FULL_BUILD_ID" --output "$TITAN_NEW_REAL_AUDIO_DIR"
```

## Architecture Patterns

### Recommended surfaces

```
platform/ra8p1/pdm_target.c/.h
platform/ra8p1/pdm_capture.c/.h
platform/ra8p1/k1_asrc_24k.c/.h
platform/ra8p1/k1_pdm_sensitivity.h
platform/ra8p1/palette_runtime.cpp/.h
platform/ra8p1/fixture_app.cpp          # K1_PALETTE_AUTOSTART flags 5, mode 32
platform/ra8p1/hal_entry.c              # opcode 6 JSON
src/k1/core/visual/pixel_topology.h     # 79/80
```

PDM owns **DMAC0/DMAC1** (default pdm-first). LED owns **DMAC2**. Combined image must not enable OSPI DMAC0 (`combined-runtime-contract.json`). Do not propose a fourth map.

### Pattern 1: Boot / autostart (copy)

```cpp
/* fixture_app.cpp K1_PALETTE_AUTOSTART */
config.flags = 5U;  /* bit0 active | bit2 emit; NOT bit1 carousel */
config.mode_a = k1::titan::kLiveAudioBootMode; /* 32 */
config.mode_b = k1::titan::kLiveAudioBootMode;
/* morph: version 3, transition 1500, palettes 0/1, brightness 24 */
```

`PaletteConfig` default brightness is **24**. WS2816 autostart branch sets brightness **128** and palettes 33/43 — that path is mutually exclusive with `K1_PALETTE_GPT_DMA`. Protocol test asserts `"mode_a":32`, `"emit_enabled":true`, `"automatic_cycle":false`, morph `"brightness":24`. Build flag `--palette-autostart` help: live WaveformK1 mode 32 flags 5, **not** a carousel.

Record actual INFO/status (opcode 1 + 17). Do not write 255 because an old restore JSON did.

### Pattern 2: PDM / ASRC APIs (copy)

| Function | Role |
|----------|------|
| `k1_pdm_target_initialise` / `_poll` | Open both lanes, start, pair |
| `k1_pdm_target_dmac_isr` | Invalidate; `k1_pdm_stream_context_on_data`; **`k1_pdm_stream_context_filling_slot`**; `xfer->reset` into FILLING dest |
| `k1_pdm_target_pull_ap_hop(int16_t out180[180])` | Consume 24 kHz hop |
| `k1_pdm_target_measured_hz` / `_rate_locked` | Lock after **≥2 s** and samples>0 via `k1_asrc24_set_rate` |
| `k1_pdm_target_ap_hops` / `_last_hop_dt_us` / `_asrc_starved` | Cadence |
| `k1_pdm_target_sample_min` / `_max` / `_sat_pos` / `_sat_neg` | **Both rail signs** |
| `k1_pdm_target_gain_clip_pos` / `_neg` / `_last_hop_gain_q8` | Post-gain |
| `k1_pdm_target_rearm_denied` / `_overflow_events` / `_drop_events` / `_recovery_count` / `_packing_mismatch` / `_pair_skew_drops` | Guardrails |
| `k1_pdm_loud_guard_q8` / `k1_pdm_scale_sample` | Sensitivity |
| `k1_asrc24_reset` / `_set_rate` / `_push` / `_pull180` / `_phase_increment_q16` | Linear interp; starve increments `starved` |

Constants: `K1_PDM_TARGET_SINCDEC_40K 49u`, `K1_PDM_TARGET_SINCRNG_40K 10u`, `K1_PDM_TARGET_SAMPLE_RATE_HZ 40000u`, `K1_PDM_TARGET_SLOT_ELEMENTS 296u` (multiple of 8), profile `"ap_40k_asrc24"`. Compile `#error` if SINCRNG 5 with SINCDEC 49.

Expected hop rate: **24000/180 = 133.333… /s**. History ~41,406–41,411 Hz measured, 133.3 hops/s. Nominal 40 kHz ≠ measured 40 kHz.

**WP5-03 gap:** opcode 6 prints `sample_rate_hz` from the **macro**, plus `measured_hz`/`rate_locked`, DMA channel numbers, **not** live SINCDEC/SINCRNG. Plan a bounded readback (register or extended JSON) rather than another gain change.

### Pattern 3: Presence / dwell (copy)

```cpp
/* palette_runtime.h */
kLiveAudioBootMode = 32
kTitanSilenceDwellUs = 5_000_000
kTitanDwellTauS = 1.17F
k1MusicalPresence: peak_scaled>0.25 || vu_level>0.25
applyK1PresencePolicy(..., last_live_us, last_live_valid)
last_live_valid_ member; configure() sets last_live_us_=0, last_live_valid_=false
```

`step()`: `keep_live = musical || in_dwell`; dwell uses `ageAuthoredHistory` Q8.8 elapsed-time, **no min-1 floor**; `dwell_armed_` prevents repeated reinit. Status JSON: `visual_path` effect/hold/dwell, `live_age_us`, `dwell_reinit`, `effect_frames`, `dwell_frames`.

Geometry: `kCentreLeft=79`, `kCentreRight=80`, `kPixelsPerHalf=80`. Bench 128 maps via `benchToNative`; bounce uses 63/64. Centre-origin: motion from centre out or edges in; **mirror symmetry is not origin**.

### Pattern 4: Observation vs acceptance

- `run_mode32_real_audio.py`: observation; `accepted: False` unless Phase 3 added an honest gate. CLI: `--audio`, `--build-id`, `--output` (must not exist), `--play-s`, `--after-s`. Reuses `settings_payload` / restore from colour runner. Opcode 18 frames must be **exactly 480** bytes. Do not treat short frames as black.
- `run_music_baseline.py`: `main()` immediately `SystemExit('HARD FAIL: never play white noise or computer-generated tones into the room')`. Do not bypass.
- `run_pdm_ap_gpt.py`: default `--pause-s 45`; requires combined PDM+GPT build receipt; `g4: NOT_RUN_THIS_IMAGE`; photons/waveform not claimed.
- No synthetic computer tones / white noise; no default same-song loop.

### Anti-patterns

- Feeding 16 kHz diagnostic PCM into 24 kHz AP.
- Falling back to fixture audio when PDM fails.
- Calling 45 s preflight a G4 qualification.
- Inferring ribbons from centroid (`authored_stats` uses pixels 80–159 only — half-strip bias).
- Changing SINCRNG/gain because historical probes lacked bins below 110 Hz — **reproduce first**.
- Fourth DMA map or OSPI+PDM both on DMAC0.

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---------|-------------|-------------|-----|
| 40→24 kHz hop | New resampler | `k1_asrc24_pull180` | Host 36–44 kHz coverage exists |
| CIC window | Guess BSP 5 | SINCRNG 10 + compile guard | Table 50.7 M=125 vs M=50 |
| Presence window | Chop renderer on 0.25 | `last_live_valid_` + 5 s dwell | Staggered-frame stutter |
| Dwell fade | Pixel8×0.99875 min-1 | `ageAuthoredHistory` Q8.8 | Stuck-at-1 scar |
| Soak | New 240k hop campaign | 45 s preflight + named coverage | Different firmware |
| Acoustic labels | Guess from room correlation | `ACOUSTIC_IDENTITY_UNRESOLVED` | Discretion if no controlled test |

## Common Pitfalls

### Pitfall 1: Brightness 255
**What goes wrong:** Boot claim mismatches device.  
**How to avoid:** Read opcode 17; morph GPT path is 24.  
**Warning signs:** Copying colour-integrity historical restore as “current default”.

### Pitfall 2: Macro sinc as live evidence
**What goes wrong:** WP5-03 closed from `pdm_target.h`.  
**How to avoid:** Read back what the PDM channel actually has. Opcode 6 does not yet.

### Pitfall 3: Rate lock is once
**What goes wrong:** Drift after 2 s untracked.  
**How to avoid:** Characterise; repair tracking **only if** measurement requires it.  
**Warning signs:** New ASRC feature chain without a measured defect.

### Pitfall 4: 8 Hz dump vs 120 Hz glitch
**What goes wrong:** Smoothness claimed from sparse USB.  
**How to avoid:** Native frames for algorithm; full-cadence/optical for physical smoothness.

### Pitfall 5: Observer freeze
**What goes wrong:** Dense 20 Hz USB froze emission historically.  
**How to avoid:** Observer-off/on pair; dashboard not a production path.

### Pitfall 6: Using unrepaired real-audio runner
**What goes wrong:** F-06 truncated frames, unrestored settings, hop_max = sample peak.  
**How to avoid:** Phase 3 repair is a **prerequisite**; current file already has 480-byte check and restore — still not an acceptance gate.

## Code Examples

### FILLING rearm

```c
/* pdm_target.c k1_pdm_target_dmac_isr */
if (k1_pdm_stream_context_filling_slot(&capture_stream[lane], &filling) != K1_PDM_STREAM_OK) {
    capture_rearm_denied++;
    k1_pdm_target_request_restart((int32_t)FSP_ERR_OVERFLOW);
    return;
}
error = xfer->p_api->reset(xfer->p_ctrl, (void *)&pdm->p_reg->PDDRR,
                           &capture_buffer[lane][filling][0], blocks);
```

### ASRC two-sample interpolator

```c
/* k1_asrc_24k.c k1_asrc24_pull180 */
y = a * (65536 - frac_q16) + b * frac_q16;
out180[o] = sat16((y + 32768) >> 16);
```

### Presence policy

```cpp
inline void applyK1PresencePolicy(..., bool& last_live_valid) noexcept {
  if (k1MusicalPresence(features)) { last_live_us = now_us; last_live_valid = true; }
  if (last_live_valid && now_us - last_live_us < kTitanSilenceDwellUs)
    features.event_flags &= ~kEventSilence;
}
```

## State of the Art

| Old | Current | Impact |
|-----|---------|--------|
| SINCRNG 5 / 16 kHz | 49/10 + 40 kHz + ASRC | Need live readback; no 16 kHz reflash |
| Timestamp 0 = never armed | `last_live_valid_` | Dwell across t=0 |
| Carousel autostart | flags 5, mode 32 | CLI help must match |
| Combined 45 s 2026-09-14 | Historical other HEX | No inheritance to this candidate |

**Parked:** product four-lane, K1-L, NPU/MVE, M33, U55, P603, PCM1808, gain retune, fourth DMA map.

## Open Questions

1. **Live SINCDEC/SINCRNG evidence path** — not in opcode 6 JSON today. Recommend a bounded diagnostic field or register dump on the named image.
2. **Acoustic identity** — default `ACOUSTIC_IDENTITY_UNRESOLVED` / JSON `"unproven"` unless a controlled capsule test runs.
3. **F-06 residual** — Phase 3 owns remaining runner honesty; WP5 must not treat observation receipts as qualification.
4. **Historical frontend probes** — no bins <110 Hz, tone/label mismatch, 160→80 half-time tempo: reproduce on **this** contract before calling defects.

## Validation Architecture

> Nyquist validation enabled.

### Test framework

| Property | Value |
|----------|-------|
| Framework | Python wrappers + C/C++ host binaries; pytest for quiet-gap |
| Config file | none |
| Quick run | `python3 scripts/test_pdm_sincrng.py && python3 scripts/test_pdm_sensitivity.py && python3 scripts/test_pdm_rearm.py && python3 scripts/test_asrc_24k.py` |
| Full suite (this phase) | quick + `python3 -u scripts/test_palette_runtime.py` + mode32 wake/quiet-gap host tests |

### Phase requirements → test map

| Req ID | Behaviour | Test type | Automated command | File exists? |
|--------|-----------|-----------|-------------------|--------------|
| WP5-00 | Transmitter admitted | gate | Phase 5 receipts | ❌ until WP4 |
| WP5-01 | Named bound candidate | build receipt | `build_scalar.py` flags above | ✅ tool |
| WP5-02 | Autostart 32 / flags 5 / brightness 24 | unit | `test_palette_runtime.py` AUTOSTART; bounce mutation | ✅ |
| WP5-03 | Live sinc + rate + lock | target | opcode 6 today: rate/lock/DMA **only** | ❌ sinc fields Wave 0 |
| WP5-04 | FILLING rearm; no 16 kHz feed | unit + review | `python3 scripts/test_pdm_rearm.py` | ✅ rearm; 16 kHz is config review |
| WP5-05 | Acoustic identity | manual/target | JSON `unproven` | ✅ field; ❌ measurement |
| WP5-06 | Rails/clips/ASRC | unit + target | `test_asrc_24k.py`; opcode 6 sat/clip | ✅ host; ❌ live char |
| WP5-07 | Real recording observation | target | repaired `run_mode32_real_audio.py` | ✅ script; F-06 Phase 3 |
| WP5-08 | Dwell / last_live_valid_ | unit | `test_mode32_wake.cpp` isolation; **wrap/t=0 incomplete** | ⚠️ Wave 0 |
| WP5-09 | Travel ≠ centroid | host + dumps | opcode 18; quiet-gap scores | ⚠️ centroid helper biased |
| WP5-10 | 45 s preflight | target | `run_pdm_ap_gpt.py --pause-s 45` | ✅ runner |
| WP5-11 | Observer on/off | target | bounded USB pair | ❌ campaign |

### Sampling rate

- **Per task commit:** quick PDM/ASRC/rearm/sincrng.
- **Per wave merge:** + palette runtime/autostart/snap-freeze.
- **Phase gate:** named image identity + host suite + 45 s preflight **or** explicit BLOCKED; no four-lane/K1-L stamp.

### Wave 0 gaps

- [ ] Opcode 6 (or equivalent) **live** SINCDEC/SINCRNG readback test.
- [ ] Dedicated `last_live_valid_` tests: `now_us=0` with prior live, and uint64 wrap.
- [ ] Travelling-structure scorer that is not centroid-of-pixels-80–159.
- [ ] Historical probe reproduction fixtures against 80-bin/xover-40 contract.
- [ ] Observer-on/off target pair on the named image.

## Sources

### Primary (HIGH)

- `pdm_target.c/.h`, `k1_asrc_24k.c/.h`, `k1_pdm_sensitivity.h`, `pdm_capture.h`
- `palette_runtime.cpp/.h`, `fixture_app.cpp` autostart, `hal_entry.c` opcode 6
- `pixel_topology.h` centres 79/80
- `scripts/run_pdm_ap_gpt.py`, `run_mode32_real_audio.py`, `run_music_baseline.py`
- `tests/host/test_palette_protocol.cpp`, `test_mode32_wake.cpp`, `test_pdm_rearm.c`, `test_pdm_sincrng.c`
- Takeover brief §5 WP5; session canon for LMD2718 / FILLING

### Secondary (MEDIUM)

- Historical 41.4 kHz / 133.3 hops/s / 45 s soak on **other** HEX
- F-06 remaining vs current runner (480-byte + restore already present)

### Tertiary (NOT_VERIFIED)

- Acoustic capsule identity, live sinc registers, combined qualification of **this** candidate, physical smoothness

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — headers/constants read
- Architecture: HIGH for boot/FILLING/ASRC; MEDIUM for sinc readback gap
- Pitfalls: HIGH — brief + source
- Live admission: NOT_VERIFIED until WP4 + target

**Research date:** 2026-09-20
**Valid until:** 7 days or identity/sinc/telemetry change

## RESEARCH COMPLETE
