# D02 VP cartography: adversarial verification (15 Oct 2026)

Verifier: Opus 5.5 subagent. Read-only on every repository. Nothing was built, flashed or committed.

## In plain English

The numbers from D02 mostly match the source. Re-running its two scripts gives byte-identical JSON. Three problems need fixing before anyone acts on them:

1. **The brightness gains changed while this check was running.** The working tree now has v9 deposit gains of 0.580092 (mode 32) and 0.830949 (mode 18), not 0.395878 and 0.591745. The header was rewritten at 01:38:14 local time. So every D02 amplitude and output-code number based on the old gains is now about 1.465 times too low.
2. **The drop time is cited to the wrong line.** The scripts use 55.73 s and cite `DETECTOR.md:63`. That line does not say 55.73. The annotated drop is 55.7025 s (`results/planted.json:20`). This barely moves the 2 s and 8 s window statistics, but the citation is wrong.
3. **Some figures are not source facts.** The "S3 would have 3.25 bits" figure is a hypothetical, because the S3 does not use this gain chain. The output-code figures leave out stages that are in the output path. Treat both as estimates.

The committed HEAD still sets the v9 palette default to `kRestrictedRange`. Only the uncommitted working tree selects the per-pixel path P. So lesson PL14-B14 ("the product default did not select the fix") is still true of committed source.

## Scope limitation (read first)

The D02 REPORT.md was never written; the harness refused it. The orchestrator says the report exists only as structured output, and that output was not passed to this verifier. So I could not read the report's lever records or check its quoted Captain words.

I verified the decision-relevant [FACT] constants that D02 encoded in its on-disk measurement scripts instead. `vp_numbers.py` says "every constant is cited in REPORT.md". Those constants carry the report's numbers.

Default rule applied: unsupported equals refuted.

## Source identity

- Worktree: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`.
- HEAD: `1ef53952ab7a50af38414321c8a91e515744098a`, not the 81ac031 cited in the brief.
- The working tree is dirty and is being edited concurrently. The following files differ from HEAD:
  - `inhouse_waveform.{h,cpp}`
  - `inhouse_audio.{h,cpp}`
  - `build_scalar.py`
  - `p6_effect_mapping.py`
  - `p6_effect_probe.cpp`

  `inhouse_waveform.h` mtime: 2026-10-15 01:38:14. Line numbers below refer to the working tree at about 01:38 unless they say HEAD.

## Reproduction

[FACT] I copied both D02 scripts to /tmp/d02v and ran them there. They regenerated both output files with identical hashes:

| File | sha256 |
|---|---|
| `vp_numbers.json` | `1c59e26e9ad97d5008efad6069e6533049c5195385d7b19bd3bcb7802a0fc9c3` |
| `clamp_occupancy.json` | `9d74c15bea01626bf12bc87a68258f50b5569001b5b29f17942d5345a54e973c` |

[FACT] The input CSV hashes match the hashes recorded in the JSON:

| CSV | sha256 |
|---|---|
| `v9_final4.boom_source` | `16639250…3bd5` |
| `v8_pre.boom_source` | `b909500b…1a46` |
| `v9_final4.boom_room` | `d3dcacd9…2d94` |

These files are under `runs/P6-20261014T124826Z/permitted/csv/`.

## Claim-by-claim

| # | D02 claim | Verdict | Evidence |
|---|---|---|---|
| 1 | Palette 31 has 7 stops, black → (255,255,0) | CONFIRMED | `product_palette_data.cpp:876-884`, descriptor `:1294` |
| 2 | Palette 38 has 10 stops; first and last are both (12,0,10), so there is no seam jump | CONFIRMED | `product_palette_data.cpp:1084-1095`, `:1301` |
| 3 | The HD sampler clamps beyond the last stop and does not blend across the wrap | CONFIRMED | `product_palette.cpp:60-109` (if `first+1>=stop_count`, select the last stop) |
| 4 | The AGC alpha is rational, with tau = 0.01(1-r)/r (not an EMA), at `inhouse_audio.cpp:23-27` | CONFIRMED | Exact lines; the comment cites the source `k1_agc_dt_clock.h` |
| 5 | AGC references: follower 0.25/0.005, peak_scaled 0.65/0.15, envelope 0.28/0.02, noise 0.001, gain 0.05 | CONFIRMED | `inhouse_audio.cpp:250,253,331,333,339` |
| 6 | The mood LPF cutoff is 1+10·mood Hz, against a 100 Hz reference. It advances once per 96-sample hop, so tau is 119 ms at mood 0 and 80 ms at mood 0.05 | CONFIRMED | `inhouse_audio.cpp:107-111`; `reference_presentation_hz=100` (`inhouse_audio.h:43`); default `mood=0.05` (`inhouse_audio.h:37`). Caveat: `fixture_app.cpp:2299` sets channel-0 mood to 0.35, so the live tau may be about 27 ms, not 80 ms |
| 7 | Magnitude average uses 0.3 new / 0.7 old (tau 21 ms) | CONFIRMED | `inhouse_audio.cpp:274` |
| 8 | VP spectrum smoother mixes 1-0.25^(dt·100), which is 0.75 per 10 ms (tau 7.2 ms) | CONFIRMED | `inhouse_waveform.cpp:147`, `:598` |
| 9 | v9 release tau is 0.29 s (mode 32) or 0.125 s (mode 18), plus 10/s when silent (74 ms or 56 ms) | CONFIRMED | `inhouse_waveform.cpp:801` |
| 10 | v8 decay rate = 0.8 + 3.5·peak (+10·(1-quiet)) | CONFIRMED | `inhouse_waveform.cpp:254` |
| 11 | Mode-32 scroll is 405 px/s, so 4.05 px per frame | PARTLY | 405 is at `inhouse_waveform.cpp:967` and `:259`. "Per frame" assumes 100 Hz. That holds for host replay (CSV t_s steps of 10 ms; `fixture_app.cpp:1965 "render_hz":100`; dt==10 ms special cases at `:411` and `:461`). The brief says the device logical target is 120 Hz; at 120 Hz it is 3.375 px/frame and the beading threshold moves. Device frame rate was not verified |
| 12 | Deposit reach = 1.5 + 3.5·level | CONFIRMED | `width=0.5+3.5*level` (`:826`), `reach=width+1` (`:838`) |
| 13 | Level transfer: -33 dBFS → 0, ceiling -9 dBFS → 1; amplitude = g·l^1.5 | CONFIRMED | `inhouse_waveform.cpp:949`; `inhouse_waveform.h` `kInhouseV9LevelCeilingDb=-9`, `kInhouseV9Emphasis=1.5`, variant `kLevelEmphasis` |
| 14 | v9 gains are 0.395878 (mode 32) and 0.591745 (mode 18) | **REFUTED for the current tree** | True at HEAD 1ef5395 (`git show HEAD:…inhouse_waveform.h:35-36`). The working tree now has 0.580092 and 0.830949 (`inhouse_waveform.h:38-39`, "path P matched-average-light calibration"). See the next table for the derived numbers that are now stale |
| 15 | Warmth sqrt(.1982) and sqrt(.0244); white balance (1, .9375, .84); board-0 warmth 0.5905 | CONFIRMED | `palette_runtime.cpp:284-286`. 0.5905511975288391 is a device readback (`runs/P1-20261014T061642Z/permitted/before-deploy-readback/before.json:481`), so it is historical, not a source default |
| 16 | Board gain 0.25 (`max_code_mode32_board_gain0.25`) | UNVERIFIABLE | `setBoardIntensity` (`palette_runtime.cpp:659-665`) takes a runtime argument. I found no 0.25 default in source |
| 17 | Mode-32 output at cap 24 is 11.25 bits, against 3.25 bits on an 8-bit S3 | **REFUTED as [FACT]** | The formula leaves out the display brightness factor 0.3+0.7·b (`palette_runtime.cpp:282`), the tanh shoulder, warmth and white balance. I also could not find where the 24/255 cap is applied. The "S3" figure applies Titan v9 gains to a hypothetical 8-bit path, and the S3 product never ran that chain. Both are at best [INFERENCE] upper bounds |
| 18 | Wire time 5.1 ms per frame (196 Hz maximum) | CONFIRMED | Arithmetic matches the brief (80×48×1.25 µs + 300 µs). This is not a device measurement |
| 19 | AP publication rate 133.3 Hz (12800/96); tempo tracker every 3 hops | CONFIRMED | `inhouse_audio.h:15-16`; `inhouse_audio.cpp:12-14` |
| 20 | Drop annotation 55.73 s (`DETECTOR.md:63`) | **REFUTED citation** | Line 63 does not contain 55.73. `DETECTOR.md:56` says "55.7 s … hit at 55.74 s, 40 ms later". The annotation is 55.7025 (`P6-DROP-DETECTOR/results/planted.json:20`). The windows in `clamp_occupancy.py` are therefore shifted by +27.5 ms. The effect on 2 s and 8 s medians is small, but "build_last_2s" contains 27 ms of the drop |
| 21 | Level saturation: level=1 in 35.4 % of fresh frames in the first 2 s of the drop on BOOM source, and 0 % in the last 2 s of the build. Room: 0 % and 0 % | CONFIRMED (reproduced) | `clamp_occupancy.json`. Note: the peak_scaled and position columns are identical in v8 and v9 because the AP is shared. They are not evidence of a v9 change |
| 22 | Palette 31 is dark (luma < 5 % of maximum) over 17.5 % of positions; palette 38 over 6.2 % | CONFIRMED as computed | Rec.709 weights on code values with no gamma, so this is a relative measure only |

## Stale numbers from claim 14

Every number derived from the old gains is low by a factor of 0.580092/0.395878 = **1.4653**:

| Derived number | D02 value | Rescaled to the current tree |
|---|---|---|
| `amp32_at_*dBFS` | as computed | ×1.4653 |
| `max_code_mode32_cap24` | 2442 | about 3578 |
| `max_code_mode18_cap24` | 3650 | about 5125 |
| `bits_mode32_cap24` | 11.25 | about 11.80 |

Any claim that path P "matches v8 mean light" must name which gain pair was measured.

## Additional findings

- [FACT] At HEAD, `kInhouseV9Palette = kRestrictedRange` (`git show HEAD:src/k1/core/visual/inhouse_waveform.h:46`).
  - The per-pixel path P (`kPerPixelMapped`) exists only in the uncommitted working tree (`inhouse_waveform.h:51`).
  - So PL14-B14 still holds for committed source: the default does not select the fix.
  - Any lever record whose "current setting on Titan v9" says "per-pixel mapped" must add "working tree only, uncommitted".
- [FACT] The working tree changed during verification. Any D02 line citation into `inhouse_waveform.{h,cpp}` may now be off by a few lines (+3 to +5 in the header).

## Origin audit (S3 budget or aesthetic)

The source comments for the VP/AP constants D02 used give these origins:

| Constant | Stated origin in source | Location |
|---|---|---|
| AGC rational alpha | Inherited K1 source | "Source k1_agc_dt_clock.h", `inhouse_audio.cpp:24` |
| Mood LPF | Inherited, with a quirk of the reconstructed SYSTEM_FPS=100 | "Source utilities.h low_pass_filter … SYSTEM_FPS=100", `inhouse_audio.cpp:103-106`, `:317-320` |
| 0.75 spectrum smoother | Inherited Emotiscope | "Reference get_smooth_spectrogram", `inhouse_waveform.cpp:144-146` |
| v9 gains | Calibration to v8 mean light | `inhouse_waveform.h:31-39` |
| -9 dB ceiling, emphasis 1.5, 0.29 s / 0.125 s release | P6 frozen design iterations | Header and `fadeV9` comments |
| 405 px/s scroll | Inherited from v8; origin unstated | `inhouse_waveform.cpp:965` ("v8's outward scroll") |

[FACT] No comment on any of these constants attributes it to an ESP32-S3 compute or RAM budget. Any D02 lever whose origin is "S3 budget" is therefore unsupported by the source I read. It should be relabelled "inherited (origin unknown)" unless the report cites a specific S3 commit or measurement. I could not see the report's lever records, so I cannot name which levers carry that label.

Only "inherited" is evidenced for the Emotiscope/K1 constants. Whether they were budget-driven or aesthetic is not evidenced here.

## Unverifiable here

- Quoted Captain words and the lever records themselves: the report was not supplied to the verifier.
- Board gain 0.25.
- Where the 24/255 brightness cap is applied.
- The device render rate: 100 Hz in the host replay against 120 Hz in the brief.
- Any measured M85 cycle counts: none appear in the on-disk D02 files.
