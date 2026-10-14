# D13 time and latency: adversarial verification

Verifier: Opus 5.5, 15 Oct 2026. Read-only. Titan worktree HEAD `1ef5395`, branch `lane/titan-lagfree-20261014`.

## What happened

The D13 `REPORT.md` was never written to disk; its text went only to the parent agent. So this check covers the claims D13 left on disk: every source constant cited by file:line in `compute_timeconstants.py` and `novelty_and_mood.py`, and the two JSON outputs those scripts produced. I checked 18 claims against source. 14 hold. 2 hold with corrections that matter. 1 is refuted as a FACT (it is model output). 1 cannot be checked. I could not check the report's per-lever "origin" calls (S3 budget versus aesthetic) because I could not see them. None of the constants I traced carries evidence of an S3 budget origin; the source comments call them inherited reconstructions.

## Claims checked

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| 1 | GDFT bin 0 = 110 Hz, block 1834 at 24 kHz, 76.42 ms window; bin 79 = 1.67 ms; median 11 ms | CONFIRMED | `src/k1/core/audio/gdft_goertzel.cpp:102-134` (cap 2000 `:20`, crossover 40 `:19`); 24000/180 `audio_rate_config.h:17`; 24 kHz is active only under `K1_INHOUSE_ANALYSIS24` (`k1_live_runtime.cpp:49-55`, set at `scripts/build_scalar.py:891`). At 12.8 kHz the window durations are the same because block size scales with rate. |
| 2 | AGC alpha is rational: tau = 0.010(1-ref)/ref, alpha = dt/(tau+dt) | CONFIRMED | `platform/ra8p1/inhouse_audio.cpp:23-27` |
| 3 | Follower 0.25/0.005, peak_scaled 0.65/0.15 | CONFIRMED | `inhouse_audio.cpp:250,253`; dt = 96/12800 = 7.5 ms (`:11`), first call 10 ms (`:249`) |
| 4 | Envelope 0.28/0.02, noise 0.001, gain 0.05 | CONFIRMED | `inhouse_audio.cpp:331,333,339` |
| 5 | magnitude_average 0.3/0.7 per hop (about 21 ms effective tau) | CONFIRMED | `inhouse_audio.cpp:274` |
| 6 | Mood LPF cutoff 1+10*mood Hz on a 100 Hz basis but run every 7.5 ms hop; default mood 0.05 | CONFIRMED (default only) | `inhouse_audio.cpp:103-111,313-317`; `inhouse_audio.h:37,41-43`. The in-house mood can be set at runtime (`k1_live_runtime.h:88`). The value on a running unit was not checked. `fixture_app.cpp:2299` sets a donor palette mood of 0.35, which is a separate control. |
| 7 | Renderer spectrum mix tau 7.2 ms; chroma max release about 10 s, rise 195 ms | CONFIRMED | `src/k1/core/visual/inhouse_waveform.cpp:147,158,159` |
| 8 | Extent cascade 16 ms then 23 ms EMA (t50 20 ms, t90 70 ms at 100 Hz) | CONFIRMED, wrong line | `inhouse_waveform.cpp:203-204` and `:974-975` (the script cites `:983-984`); `ema()` is exponential (`:31-33`) |
| 9 | v9 fade tau 290 ms (mode 32) or 125 ms (mode 18), plus 10/s when silent (74.4 ms) | CONFIRMED | `inhouse_waveform.cpp:801` |
| 10 | Scroll 405 px/s (mode 32); centre-to-edge transit 197.5 ms | QUALIFIED | 405 is at `:967`, not `:970`. It is `405 * motion`, where `motion_scale` is clamped to 0–4 (`:938-939`). So 197.5 ms holds only at motion_scale = 1. |
| 11 | Freshness gate 30 ms | CONFIRMED | `inhouse_waveform.cpp:128` |
| 12 | Sub-band 40–110 Hz: RBJ band-pass, f0 = sqrt(40*110), Q = f0/70, two sections cascaded | CONFIRMED | `src/k1/core/audio/sub_band.h:3-18`. The 3–10 ms group delays are the script's own scipy output. |
| 13 | poly24 has 96 taps and a 47-sample delay (1.135 ms at 41,408 Hz) | CONFIRMED | `platform/ra8p1/k1_poly24.h:19,69`; `docs/evidence/P4/corrections/poly24-measured-corrections.json:17` (1.1348 ms) |
| 14 | PDM slot 296 samples = 7.148 ms | CONFIRMED | `platform/ra8p1/pdm_target.h:16`; `docs/evidence/P5/device-budget/BUDGET.md:25` |
| 15 | Novelty every 22.5 ms; tempo ACF history 512 updates = 11.52 s | CONFIRMED (history); UNVERIFIED (warm-up) | `audio_rate_config.h:27,35-39` (decimation 3); `tempo_acf.h:11`. I did not trace whether the tracker really needs all 512 updates before it outputs. |
| 16 | LED wire 5.1 ms per frame | CONFIRMED | `BUDGET.md:13`, `compute_budget.py` WIRE_MS |
| 17 | Frame wait averages 5 ms at 100 Hz | CONFIRMED | `platform/ra8p1/palette_runtime.h:39-40` (10,000 µs under `K1_EMOTISCOPE_PORT`); `BUDGET.md:81` (100.01 fps measured). This conflicts with the brief's "120 Hz logical render target": the current build renders at 100 Hz. |
| 18 | Novelty peak timing versus mood (`novelty_and_mood.json`) | REFUTED as FACT | Source novelty (`inhouse_audio.cpp:341-367`) is the square root of the mean positive per-bin difference of the spectrum, taken after gain, tilt, soft knee and clamp. The script uses the positive difference of the low-pass output and leaves out gain, knee and square root. Its timings are [INFERENCE]-grade model numbers, not measurements. |

## Corrections that matter for decisions

1. [FACT] The 16 ms "typical" device budget runs from the sample stamp to the end of the wire. The stamp is the newest sample of the hop (`BUDGET.md:91`). It does not include the time it takes a Goertzel window to fill. [INFERENCE] For content at 110 Hz, the acoustic onset reaches full magnitude up to 76 ms later (about 38 ms at the centroid), before the 0.3 EMA and the mood low-pass are added on top. D13's main point, that the analysis chain rather than the budget sets the latency, holds up.
2. [FACT] The mode-32 extent, which is the release "thrust", comes from `peak_scaled`. That signal is built from the raw PCM peak (`inhouse_audio.cpp:247-253`), not from the GDFT chain. So the 112 ms t50 of the GDFT, EMA and low-pass chain does not set extent latency. Extent latency is the peak follower (about 8.6 ms attack) plus the 16/23 ms cascade plus the release term (`inhouse_waveform.cpp:977`). [FACT] Extent is also capped at 80 + 0.7·80·peak_stage2 (`:976`), which is 56 px at most from the centre. Any D13 lever that blames GDFT smoothing for a slow drop extent is aimed at the wrong stage.
3. Line numbers drift in `inhouse_waveform.cpp`; fix them before anyone cites them again (items 8 and 10).

## Origin claims (S3 budget versus aesthetic)

I could not see D13's lever origins, so none can be confirmed. [FACT] The constants I traced are marked in source as inherited or reconstructed, not as S3 budget limits:
- rational AGC: "Source k1_agc_dt_clock.h" (`inhouse_audio.cpp:24`)
- 100 Hz SYSTEM_FPS mood basis: "reconstructed presentation profile" (`inhouse_audio.h:41-43`, `inhouse_audio.cpp:314-317`)
- 0.75 per-render spectrum smoothing: "Reference get_smooth_spectrogram" (`inhouse_waveform.cpp:144-146`)
- GDFT block cap 2000 and the ×2 crossover at bin 40 (`gdft_goertzel.cpp:19-20`): no origin evidence found.

Treat any lever that D13 tags "S3 budget" as unsupported until it cites a primary source.

## Not checked

D13 REPORT.md prose, its lever records, cycle costs and any quoted Captain words. These could not be checked because the file does not exist on disk.

Images: N/A. This is a non-visual verification of source and data.
