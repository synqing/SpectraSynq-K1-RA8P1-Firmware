# D03-s3-archaeology — adversarial verification

Verifier: Opus 5.5 subagent, 15 Oct 2026. Read-only. No device, serial, USB, flash or commit. The Titan must always be visualising music; nothing here changes a build.

## Plain-English verdict

The report is mostly accurate. 30 decision-relevant claims were checked against source; 25 hold. Four are wrong, and one of them changes a lever ranking:

1. **The 2,000-sample window cap does nothing on the Titan today.** No bin reaches 2,000 samples at either 12.8 kHz (largest window 978) or 24 kHz (largest 1,834). So lever L06 ("raise the cap", impact 4) does nothing until the bank is moved down to 55 Hz (L04), and its planted fault cannot go red. The real limit on bass sharpness is the ×2 divisor on bins 0–39 (L07).
2. **The 32-bit resonator wrap (L09) is worse than the report says.** The report calls loud-bin zeroing on the Titan an untested hypothesis. A host re-implementation of the exact Titan arithmetic zeroes the 110 Hz bin for a sine at amplitude 3,750 (about −19 dBFS), well below the loud guard's 30,000 ceiling. It also zeroes the 440 Hz bin at 15,000. The magnitude comes out wrong whenever the true magnitude passes about 46,341. This is a host model, not a device measurement, but it makes L09 a leading suspect for weak bass and drops that do not hit.
3. Smaller errors: "no window began in SensoryBridge 4.0.0" is wrong, because the window was already commented out in 3.2.0. The "1.9×" cost figure for 24 kHz is copied from a lesson whose own numbers give 2.65×. The "17 % of the hop" headroom figure uses 7.5 ms, but the measured deadline is 5 ms, which gives 26 %.

## Claims checked

| # | report claim | checked against | verdict |
|---|---|---|---|
| 1 | Titan `kNoteOffset=12`, `kX2CrossoverBin=40`, `kMaximumBlockSize=2000` | Titan `src/k1/core/audio/gdft_goertzel.cpp:18-20,120` @ `1ef53952` | CONFIRMED |
| 2 | Titan wraps coeff×q1 to int32 before `>>14`; magnitude also int32-wrapped; `>>6` present | `gdft_goertzel.cpp:52-56,61-63,71-80` | CONFIRMED |
| 3 | K1 shipping uses int64 recurrence + magnitude; quote "ZEROING those bins on loud audio" | K1 `platformio.ini:79-99` @ `68a95773` | CONFIRMED |
| 4 | C08/L06: cap 2000 limits resolution to fs/2000; "at 24 kHz the same cap is 12 Hz, so the better rate is coarser on capped bass bins"; L06 experiment plants "cap 2000 at 24 kHz" | Recomputed block sizes with the Titan formula (`gdft_goertzel.cpp:104-120`) | **REFUTED** for the current config. Max block 978 (12.8 kHz) and 1,834 (24 kHz), both at bin 0 (110 Hz). Zero bins capped. Binding limit is the ×2 divisor (bin 0 ≈ 13.1 Hz cells). Cap binds only with offset 0 at 24 kHz (55 Hz → 3,669 → capped). The 55/62 Hz test tones are not in the offset-12 bank |
| 5 | Rounded-k pulls A4 440 → ~420 Hz | K1 `config_types.h` true-centre comment; recomputed Titan: k=8, N=244 → 419.7 Hz (12.8 k), N=458 → 419.2 Hz (24 k) | CONFIRMED; it also persists at 24 kHz |
| 6 | ADR-0008 "7712 µs under 8000 µs gate", 48 kHz "Dead on arrival" | K1 `docs/architecture/ADR-0008-usb-audio-keeps-12800-ap.md:32,54` | CONFIRMED |
| 7 | 16 kHz/120 dropped AP cadence 133.33→123.15 Hz; GDFT "heavy but survivable" | K1 `docs/forensics/2026-06-15-16k120-ap-stage-profile-verdict.md:5-14` | CONFIRMED |
| 8 | `ae5d90a` DENSE 2 coords / 99.8 % frozen → 11 / 0 %; QUIET hold kept | commit message | CONFIRMED |
| 9 | `3776e777` 120 Hz Captain decision, no catch-up frames | commit message | CONFIRMED |
| 10 | `c2738b88` is "paint C1/C2 colour in 16-bit after device PASS", not the inventor of per-bin sampling | commit message | CONFIRMED with a nuance: the message names "palette_manual_colour on seven live modes", so the brief's link to per-bin call sites is partly right. The HD sampler itself is `d408433` (2026-06-11) |
| 11 | `d408433` "lost resolution three times" quote | SensoryBridge-k1-converge `d408433`; K1 `lightshow_modes.h:111-120` | CONFIRMED |
| 12 | `9294e03d` emit-hop tempo 2.6 ms vs 0.86 ms mean; `0aaaca46` reverted | commit messages | CONFIRMED. The revert reason is "bit-exact but no measured gain". The report's "device still spiked" is a paraphrase |
| 13 | 2026-05-20 smoothing rollback: symmetric 0.75 / 0.3, notes "~3× longer … milky/washed" | K1 `system/constants.h` (Change 6 block) | CONFIRMED |
| 14 | Static noise subtraction forced off after 2026-06-15 chroma-zero evidence | K1 `constants.h` (`K1_GDFT_STATIC_NOISE_SUBTRACTION_ENABLED 0`); Titan `inhouse_audio.h:31` | CONFIRMED |
| 15 | Three conflicting rate defaults: `audio_rate_config.h` calls 24 k production, `inhouse_audio.h:46` defaults 12800, profile `emotiscope_40k_asrc12800` | Titan `audio_rate_config.h:7-17`, `inhouse_audio.h:15,46`, `pdm_target.h:28-29` | CONFIRMED |
| 16 | Mode 11 shows 24 kHz hop as nearest-96 points | Titan `inhouse_audio.cpp:220-223` | CONFIRMED |
| 17 | Build script refuses boot brightness <100 ("user mandate"); `--inhouse-analysis24` flag | Titan `scripts/build_scalar.py:888-891,933-934` | CONFIRMED |
| 18 | Donor enum 128 px / 12800 / 100 Hz / 64 freqs / 96 tempi; in-house modes 7,8,11,18,32 | Titan `platform/ra8p1/emotiscope_port.h:9-16` | CONFIRMED |
| 19 | v9 palette default is `kPerPixelMapped` at `inhouse_waveform.h:46`; `sub_rms` at `:110` | Titan `inhouse_waveform.h` | CONFIRMED content; **line numbers wrong**: enum at 46, default at **50**; `sub_rms` at **113** |
| 20 | PL14-B12: "about 1.9×: p50 270k → 716k cycles, 1.29 ms peak vs 5 ms" | Kx-Titan `TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md:144-145` | **PARTLY REFUTED**: quoted faithfully, but 716/270 = 2.65×, not 1.9×. The source is internally inconsistent and the report repeats it unflagged |
| 21 | Section 5 preamble: "1.29 / 7.5 = 17 % of the hop" | Lesson says "against the 5 ms deadline"; P5 BUDGET is per 5 ms donor hop | **REFUTED** (minor): 1.29/5 = 26 %. §8.4 later uses 5 ms correctly |
| 22 | P5 budget: ap_cycles median 0.978 ms, p99 ~1.28, donor 0.285, in-house 0.693; render 0.197; pack 0.128; 47,505 frames, 0 skipped, render_hz 100 | Titan `docs/evidence/P5/device-budget/BUDGET.md:29-30,40,81` | CONFIRMED |
| 23 | L23 loud guard: ×16 (Q8 4096), ceiling 30000, no attack/release; `fa83ac0` 2026-09-19, `fe4c94b` 2026-09-20 | Titan `k1_pdm_sensitivity.h:9-23`, `pdm_target.c:614-636`, git | CONFIRMED. Knee = 30000/16 = 1875 raw ✓ |
| 24 | Brightness applied once in 16 bits (`value*b/255`); 24/255 → ~6,168 top codes; Emotiscope profile is RGB8×257 | Titan `hd_pixel16.h:28-31`, `palette_runtime.cpp:703-705,735-737` | CONFIRMED (65535×24/255 = 6167.6) |
| 25 | Donor `quantize_color` at `k1_dual_edge_donor.cpp:762`; donor 8-bit `raw_led_data` | Titan files | CONFIRMED |
| 26 | SensoryBridge rates: 2.0.0 18750; 3.0.0 12558/192/8192; 3.2.0 24400/4096; 4.0.0 12800 | `SensoryBridge-1` `ae67830`, `0c9a6b0`, `a27d5db`, `a17ec09` constants.h | CONFIRMED |
| 27 | "SensoryBridge 4.0.0 GDFT.h:91 has the window multiply commented out. That is where 'no window' began" | `a27d5db` (3.2.0) `GDFT.h:90` already has it commented out; `0c9a6b0` (3.0.0) `GDFT.h:91` uses `window_lookup` | **REFUTED**: the window was dropped between 3.0.0 and 3.2.0, not in 4.0.0. No stated reason either way |
| 28 | `interlace_flip` toggled but never read in SB 4.0 or K1 | `a17ec09` GDFT.h:65-66; K1 `k1_gdft_core.cpp:125-126`; K1 `diag/gdft_harness.h:72` "never read" | CONFIRMED. Missing context: it **was** read in 3.2.0 (`a27d5db` GDFT.h:80, low 16 bins every other frame), so it was a real S3-era budget cut that later went dead. K1 `docs/architecture/firmware-capability-overview.md:35` still claims "[LIVE] low bins interlaced → ~66 Hz", which is a stale doc |
| 29 | Emotiscope `microphone.h` stale "2 kHz sample rate" comment beside 12800; NUM_LEDS 128 / NUM_FREQS 64 / NOVELTY 50 Hz / 1024 / TEMPI 96 from 48 | Emotiscope `d0fef9a4` `src1/microphone.h:24,45`, `global_defines.h:13-35` | CONFIRMED |
| 30 | L09: "[HYPOTHESIS] Titan still zeroes loud bins; not measured" | Python re-implementation of `gdft_goertzel.cpp:49-98` (wrapToInt32 recurrence + magnitude), one cold-start block of a pure sine per bin | **STRENGTHENED** [INFERENCE, host model]: at 12.8 kHz, bin 0 (110 Hz, N=978) has its magnitude² wrap negative and is zeroed at sine amplitudes of 3,750, 7,500 and 15,000. At 30,000 it wraps positive with the wrong value (9.7e8 against the true 3.7e10). Bin 24 (440 Hz) is zeroed at 15,000 and 30,000. At 24 kHz, bins 0, 24, 39 and 40 zero at various amplitudes. Bins at 1 kHz and above are mostly intact at 12.8 kHz. Every input below the 30,000 guard ceiling can reach this. Not yet measured on the device |

Counts: 30 checked; 25 confirmed (2 with line or wording nuance, 1 strengthened); 4 refuted or partly refuted (#4, #20, #21, #27); 1 confirmed but missing context (#28).

## Origin claims asserted on thin evidence

| lever | asserted origin | evidence found | flag |
|---|---|---|---|
| L06 / C08 (cap 2000) | [INFERENCE] S3 budget | none at the line; present since SensoryBridge | Moot on Titan today (inactive). Origin unknown |
| L05 / C12 (tempo decimation 3) | S3 budget | the 16 kHz verdict names the tempo ACF as the wall; 9294e03d emit-hop 2.6 ms | No commit that *introduces* decimation 3 with a budget reason was cited. Budget is plausible but indirect. Treat it as [INFERENCE] |
| L07 / C09 (Cross40) | S3 budget | `e911f86d` "Promote Cross40+Lane-4 … under 8000 µs service"; handover cites the Cross0/40/80 matrix. Cross40 GDFT p99 ≈ 3.09 ms in `20260816T-g2-lane4-cross40/RESULT.json` | Supported indirectly: the selection was gate-bound. No source says in one line that "Cross0 failed the gate" |
| L19 / C20 (Hann off) | S3 budget | K1 `config_types.h` says default off for byte-identity and "Visual impact is unproven"; cost is mentioned, not given as the reason. Upstream removal (SB 3.0→3.2) has no stated reason | Mixed. The stated reason for staying off is parity plus no eyes-on, not cycles |
| L04 / C07 (110 Hz floor) | inherited v40102 profile; "[INFERENCE] budget-shaped" | `config_types.h` ChromaProfile comment | Inherited: confirmed. The budget claim has no evidence and is labelled inference, which is acceptable |
| L09 `>> 6` | "inherited headroom hack" | no stated reason anywhere | Unknown. Correctly hedged |
| L25 Q16 clamp | SQ15x16 soft-float budget | SB 4.0 introduced SQ15x16 (not re-checked); K1 "no new soft-float" comment | [INFERENCE]. Acceptable as labelled |
| L23 loud guard | inherited int16 storage | file comment: "Loud-guard … is what stops a snap railing int16" | Supported |

## Impact on lever ranking

- **Demote L06** to "conditional on L04": impact 0 on the current bank. Rewrite its experiment for offset 0 at 24 kHz, or drop it.
- **Promote L09** from hypothesis to the top AP suspect for "bass and drop do not hit". The smallest decisive test is unchanged and cheap: on the host, feed a 0 dBFS and a −19 dBFS 110 Hz sine through the Titan `analyseGdftRaw`. Freeze "bin 0 magnitude is monotonic in input level and non-zero" before running. The planted fault is the current `wrapToInt32`, which must go red. The fix is K1's existing int64 recurrence and magnitude (`platformio.ini:98-99`).
- L07 (×2 divisor) is the real bass-resolution lever, not L06.

## Not verified (out of time or out of scope)

- Which profile (8 or 9) and which `kInhouseV9Palette` value are on the silicon now. This needs a device identity readback, which this task did not allow.
- `667894a0` (OKLab LUT), `810846bc` (HD V2 stops), `44e5d36b`, `1be4930a`, K1.Lightwave `src/led_utilities.h:293`, K1.reinvented `goertzel.h`, SensoryBridge `6109a1e` body text beyond its subject, and `dec5fb53` beyond its message.
- Whether a real program at the Titan's ×16 sensitivity actually reaches the host-model amplitudes in #30. A per-hop `overflow_count` / peak log on the host replay corpus would settle it.
