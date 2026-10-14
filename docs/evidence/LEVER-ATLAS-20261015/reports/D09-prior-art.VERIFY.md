# D09 prior art: adversarial verification (15 Oct 2026)

## In brief

The D09 report was never written to disk. The harness blocked its Write. I recovered the full text from the producer's own StructuredOutput in its session transcript (`subagents/workflows/wf_2a73b8fd-675/agent-ad8bfd9a35cc2df4f.jsonl`) and checked it against the source at Titan worktree HEAD `1ef5395` (branch `lane/titan-lagfree-20261014`).

Most of its numbers hold up. The time constants, the 88 BPM prior, the 16-bit wire path, the Emotiscope citations, the wire-time and level arithmetic, and the LedFx and Spotify web claims all match their sources.

Four decision-relevant claims are wrong, and all four go wrong the same way: D09 read the v8 code paths or the preview code and did not read what v9 already does. Each of the four levers below describes, as missing, something v9 already has.

1. **L06 (8-bit palette position).** Path P does not use the uint8 FastLED sampler. It samples the HD palette with a float position quantised to Q16, so the proposed fix is already in place.
2. **L01 (gain stages flatten the drop).** In v9, deposit amplitude and width come from the pre-AGC level, not the follower. The follower controls the waveform's extent. That is exactly the "length" the Captain describes, so the lever survives with a corrected target.
3. **L02 (no tension or release stage was ever built).** v9 has a built release detector: a pre-AGC surge against a slow baseline, a dip gate and an optional sub-bass trigger. It also has an onset-density accent. Both are switched off by default.
4. **L12 (decay rises with peak).** That is v8 only. v9 already applies the Captain's ruling: a constant release time, with level driving width.

## Claims confirmed [FACT]

| # | D09 claim | Evidence |
|---|---|---|
| 1 | Worktree HEAD is 1ef5395 | `git rev-parse` → 1ef5395 |
| 2 | Follower attack about 30 ms, release about 1.99 s | `platform/ra8p1/inhouse_audio.cpp:250`, alpha(0.25 / 0.005); tau = 0.010(1−r)/r at `:25` |
| 3 | peak_scaled attack about 5.4 ms, release about 57 ms | `:253`, alpha(0.65 / 0.15) |
| 4 | Lane gain target clamp(0.25/env, 0.1–10), tau about 190 ms; lane envelope about 26 / 490 ms | `:330-339` |
| 5 | sub_rms published only on V9 builds | `:239-241` |
| 6 | Tempo prior 88 BPM | `src/k1/core/audio/tempo_tracker.cpp:23` (`kTactusBpm = 88.0F`); low bound 60 at `:17` |
| 7 | Authored-history ageing starts from 8-bit history | `platform/ra8p1/palette_runtime.cpp:92-94` |
| 8 | Wide path is 16-bit; "History and actual wire output never pass through Pixel8" | `palette_runtime.cpp:290-296` |
| 9 | The wire snapshot is the actual emitter input | `platform/ra8p1/fixture_app.cpp:350-352` |
| 10 | Emotiscope tempo range 48–144 BPM | `Emotiscope/src1/global_defines.h:31,34` |
| 11 | Emotiscope network 64-36-8-36-64 | `src1/neural.h:1-5` |
| 12 | Emotiscope temporal dithering | `src1/types.h:63-64`, `configuration.h:64-65` |
| 13 | Emotiscope key detection | `src1/key_detection.h:42` |
| 14 | 5.1 ms wire time per frame, ceiling about 196 Hz | 80 × 48 bits × 1.25 µs = 4.8 ms + 0.3 ms reset; 1 / 5.1 ms = 196 Hz |
| 15 | About 6,168 levels against about 24 at the 24/255 cap | 65535 × 24/255 = 6168; 8-bit gives 24 |
| 16 | Three drop detectors missed the +50 ms window | `runs/P6-DROP-DETECTOR/DETECTOR.md:9-11`, `v2/DETECTOR.md:11`, `v3/DETECTOR.md:7-9` |
| 17 | Half-time lock caused by the 88 BPM prior | `docs/evidence/P7/RECEIPT.md:19,105` |
| 18 | LedFx #2058, opened 3 Oct 2026: 46 ms, F 0.642, 2–3 %, 80–90 and 170–180 BPM | GitHub API, created 2026-10-03T05:46:54Z; all four figures present in the body (accessed 15 Oct 2026) |
| 19 | Spotify restricted audio-analysis and audio-features on 27 Nov 2024 | developer.spotify.com/blog/2024-11-27-changes-to-the-web-api, HTTP 200, both terms present (accessed 15 Oct 2026) |
| 20 | Decay 0.8 + 3.5 × peak exists | `src/k1/core/visual/inhouse_waveform.cpp:254`, and `docs/evidence/P6/RECEIPT.md:280`. **v8 path only; see R4.** |

## Claims refuted

**R1. D09-L06: "path P; palette_runtime.cpp:75-76 (sampleProductPaletteFastLed16 takes a uint8_t position)".**
- [FACT] `palette_runtime.cpp:66-77` is the `preview()` function, not path P.
- [FACT] Path P calls `sampleProductPaletteHd(controls.palette_id, position, 1.0F)` with a float position (`inhouse_waveform.cpp:886-890`). The sampler quantises its coordinate to Q16 (`:43`) and interpolates linearly between stops (`:59-66`).
- [FACT] The uint8 FastLED16 path appears in v9 only as the planted host fault `kSlot16` (`:895-898`).
- Correction: the lever "use a Q16 palette position" is already implemented. Remaining issue [INFERENCE]: palette stop positions are uint8 (`stops[i].position/255`). That does not limit drift smoothness.

**R2. D09-L01: the gain stages control "brightness and deposit amplitude at the drop".**
- [FACT] In v9, the level that sets deposit amplitude and width is a "fixed dB transfer of the pre-AGC RMS" taken from `vu_level` (`inhouse_waveform.cpp:940-949`).
- [FACT] `vu_level` is not divided by the follower (`inhouse_audio.cpp:243-244`).
- [FACT] The follower-normalised `peak_scaled` sets the waveform's extent, meaning where the deposit lands (`inhouse_waveform.cpp:970-977, 995-998`).
- [FACT] The per-lane auto-gain scales the spectrum (`inhouse_audio.cpp:351`). That spectrum feeds colour and accent, not deposit amplitude.
- Correction: L01 is still the strongest lever, but its target is extent, the "length" in the Captain's words, not amplitude. Its experiment should measure extent at the drop, not a generic "contrast signal".

**R3. D09-L02: "There is no tension variable", "never built", "None of the cues … is published".**
- [FACT] v9 contains `updateReleaseV9` (`inhouse_waveform.cpp:752-796`). It compares a pre-AGC power surge (30 ms fast follower against a 2 s baseline, 3–9 dB span) and has:
  - an optional new-peak gate;
  - an optional dip gate over [t−6 s, t−0.5 s] that requires a cut or breakdown before the surge;
  - an option to trigger on the 40–110 Hz `sub_rms` instead of broadband level (`inhouse_waveform.h:63-76`).
- [FACT] Its output pulls the deposit inward to release the waveform's extent (`:977, :998`).
- [FACT] `updateAccentV9` (`:728-750`) computes fast and slow onset density from low-band spectral flux.
- [FACT] Both stages are off by default: `enabled = false` at `h:66`, and accent boost and widen are 0 at `h:55`.
- [FACT] Detector v1 also has a pure-sub path (`P6-DROP-DETECTOR/DETECTOR.md:27,48`).
- Correction: what exists is a built but unqualified release and accent stage, not an absence. The next step is to qualify and enable it, and to add the missing tension estimator (centroid slope, onset-rate slope), not to build from zero.

**R4. D09-L12: current setting "Decay = 0.8 + 3.5 × peak" (louder fades faster), "still needed: no, per the ruling".**
- [FACT] That line is in v8 `render()` (`inhouse_waveform.cpp:254`).
- [FACT] v9 `fadeV9` uses a constant release, 1/0.29 s or 1/0.125 s, plus a drain only on silence (`:798-803`). The comment reads "independent of level".
- [FACT] Width is 0.5 + 3.5 × level (`:826`). This is the Captain's P6 ruling 1, and v9 already implements it.
- What remains new in L12 is "map tension to outward velocity".

## Minor citation drift (not decision-relevant)

- `alpha()` is at `inhouse_audio.cpp:23-27`, not `:26-27`.
- The silence gate is at `:256`, not `:255`.
- The renderer accepts 60–156 BPM (`inhouse_waveform.cpp:983-984`). The 155 BPM upper bound in the tracker was not located.
- L05 says "no analysis below 110 Hz" and also "sub_rms exists". Those statements contradict each other. `sub_rms` is the 40–110 Hz band (`h:73`).

## Origins asserted without evidence

- **L01**: "a room-level-independence choice, not a compute budget". The source comment (`inhouse_audio.cpp:24`) only names the origin file `k1_agc_dt_clock.h`. "Inherited" is supported; the motive is not.
- **L05**: "S3 budget choice". Labelled [INFERENCE], but no S3 commit or document is cited.
- **L07**: "inherited from the S3's 8-bit LEDs". Plausible, but no evidence is cited.
- **L03**: "inherited or aesthetic". `tempo_tracker.cpp:15-16` states the constants mirror the production V2 source, which supports "inherited". "Aesthetic" is unsupported.
- **L06**: "inherited from FastLED". Moot, because path P does not use that sampler (R1).

## Not verifiable in this pass

These were not checked against primary text. Three of the PDFs had already failed to extract for D09 itself.

- Meier, Chiu & Müller: F1 74.68 % at 0 ms.
- Palmer 2013: r 0.89–0.99.
- Arrighi: best offset falls from 100 ms to 30 ms as tempo rises.
- Solberg 2014 and Yadati 2014.
- McKinney & Moelants: preferred tempo near 120 BPM.
- WLED documentation recommending "Lazy" AGC.
- Lightjams' drop claim.
- dj-ledfx #39.
- Fadecandy dithering at 400 Hz or more.
- That Emotiscope's network is used only in a beta mode.
- Ethos-U55 inference cost.

## Verdict

- Checked: 24.
- Confirmed: 20.
- Refuted: 4 (R1–R4).

The prior-art survey and the arithmetic are sound. The lever table is not safe to rank as written: L01, L02, L06 and L12 must be re-scored against the v9 code paths (`renderV9`, `outputV9`, `updateReleaseV9`, `fadeV9`), not the v8 paths.
