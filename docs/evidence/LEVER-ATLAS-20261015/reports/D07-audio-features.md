**D07 — Audio-feature levers for Titan**

[INFERENCE] Captain, the highest-value change is to preserve separate **fast event**, **musical context** and **visual smoothing** paths. The current tempo input has already lost much of the timing detail needed for a forceful drop.  
[FACT] The note bank starts nominally at **110 Hz**, but current v9 source already contains a separate **40–110 Hz** detector; its release consumer is **disabled by default**. [B:18–39; S:15–55; W:63–76]  
[FACT] The tempo tracker retains **60–155 BPM**, an **88 BPM prior**, three-hop aggregation and an **11.52-second nominal warm-up**. [T:17–39,500–518,568–595]  
[FACT] Existing host experiments identify the prior as a major cause of half-time; adding kick-band novelty alone did not repair it. [P7:103–147]  
[INFERENCE] Correct tempo, correct beat phase and correct bar-downbeat identity are three separate requirements. [D3:35–71]  
[FACT] The three detector studies failed their complete acceptance sets, but v1 and v2 each detected one drop; “none ever fired” would be wrong. [D1:54–69; D2:60–88; D3:83–93]  
[INFERENCE] More compute cannot reveal a bass arrival that has not happened yet; prediction must carry uncertainty and count premature visible releases as errors. [D2:82–88; D3:79–93]  
[FACT] BeatNet’s local streaming implementation documents **84 ms delay**, so it is not a ready-made ≤50 ms solution. [BN:154–158]  
[HYPOTHESIS] Prioritise unsmoothed multiband onsets, metrical-level selection, timestamped phase prediction and an uncertainty-aware downbeat model before semantic section or vocal classification. [L04–L08 below]

**Evidence identity and reading conventions**

[FACT] Titan source was inspected at HEAD `1ef53952ab7a50af38414321c8a91e515744098a`. Four relevant files were dirty; therefore HEAD alone does not identify the inspected implementation. The source snapshot hashes were:

| File | SHA-256 |
|---|---|
| `platform/ra8p1/inhouse_audio.cpp` | `98f0b748020992483b38caace1a65184aa8b102f81535175deeff4f5c7167d74` |
| `platform/ra8p1/inhouse_audio.h` | `309388a8f61f068c11c51d5abf7570768b221afa43788ce6c34cb131a1bdee5d` |
| `src/k1/core/visual/inhouse_waveform.cpp` | `0be2360eb0326cdf53c60742ba749b796a48ee1d7d325291d2e45f03aece82ae` |
| `src/k1/core/visual/inhouse_waveform.h` | `88cea47b90ccc528a855fefb1cad12208b880607e47adb49cec3e29b29fc2272` |

[FACT] Citation aliases below expand to these exact paths; line numbers refer to the inspected files:

| Alias | Path |
|---|---|
| A / AH | `platform/ra8p1/inhouse_audio.cpp` / `.h` |
| B | `src/k1/core/audio/gdft_goertzel.cpp` |
| S | `src/k1/core/audio/sub_band.h` |
| T / TH | `src/k1/core/audio/tempo_tracker.cpp` / `.h` |
| C | `src/k1/core/audio/tempo_acf.cpp` |
| V / W | `src/k1/core/visual/inhouse_waveform.cpp` / `.h` |
| R | `platform/ra8p1/k1_live_runtime.cpp` |
| P7 | `docs/evidence/P7/RECEIPT.md` |
| D1 | `_scratch/waveform-migration-001/runs/P6-DROP-DETECTOR/DETECTOR.md` |
| D2 / D3 | Same directory, `v2/DETECTOR.md` / `v3/DETECTOR.md` |
| LAT | `docs/evidence/P5/device-budget/BUDGET.md` |
| BRIEF | `_scratch/waveform-migration-001/runs/LEVER-SWARM-20261015/SWARM-BRIEF.md` |
| BN | `/Users/spectrasynq/Workspace_Management/Software/BeatNet/src/BeatNet/BeatNet.py` |

[FACT] Reference repositories inspected: BeatNet `40336132a0ce0371d0f1d601d08550094ec7c106`; madmom `27f032e8947204902c675e5e341a3faf5dc86dae`; hybrid-beat-tracker `31981ea795f43d4835758698efdb29036d65290f`; Kx-Titan `2654ff3a341c7406b54b9d04be171512f017646d`.

[FACT] “v8/v9 current setting” below means the inspected source profiles. It does **not** identify the presently resident image. Historical device results are attributed to their receipts; no proposed feature is product-qualified by this report. [R:48–55; W:16–20; LAT:5]

> 🧠 **From Hindsight memory (Canonical audio performance suite v0.1 — Titan BOOM improvement)** — [FACT, historical record dated 13–14 October 2026] The page distinguishes recorded agent claims from physical qualification. I used it for orientation; current source and dated receipts support the findings below.

**What Titan actually computes**

| Stage | Verified implementation and information consequence |
|---|---|
| Capture | [FACT] Nominal PDM rate **40,000 Hz**, **296 samples/block**, sinc decimation setting 49, SINCRNG 10. Programme audio uses one selected microphone lane. [ `platform/ra8p1/pdm_target.h:13–24,35–43` ] |
| Measured-rate correction | [FACT] A provisional rate estimate starts after **250 ms**, refined at **2 s**. The P5 capture reports **41,408 Hz**; another P4 receipt reports **41,417 Hz**. These are different observations, not a fixed silicon rate. [ `platform/ra8p1/pdm_target.c:533–565`; LAT:25; `docs/evidence/P4/corrections/RECEIPT-ITERATION-2.md:63` ] |
| Input gain | [FACT] Nominal digital gain **×16**, reduced blockwise to keep peak at approximately **30,000 int16 counts**. [ `platform/ra8p1/k1_pdm_sensitivity.h:9–33`; `pdm_target.c:614–635` ] |
| Resampling | [FACT] `k1_poly24`: **24 kHz**, **180-sample/7.5 ms hops**, **96 taps**, **128 phases plus endpoint**, **11.25 kHz cutoff**, Kaiser β=7. Causal content delay is **47 source samples**, approximately **1.135 ms** at 41,417 Hz. [ `platform/ra8p1/k1_poly24.h:16–26,68–74` ] |
| Profile selection | [FACT] `K1_INHOUSE_ANALYSIS24` configures 24 kHz. The class default remains **12.8 kHz/96 samples**, also 7.5 ms. Thus “v8” does not inherently mean 12.8 kHz: the later v8 P4 profile uses 24 kHz. [AH:15–46; R:48–55; P7:81–85] |
| Note bank | [FACT] **80 bins**, note-table offset **12**, nominal **110–10,548.08 Hz**. At 12.8 kHz, **71** target bins are below Nyquist; at 24 kHz, all **80** are. [B:18–39,102–131; A:52–69] |
| Analysis windows | [INFERENCE, source arithmetic] At 24 kHz, actual rectangular windows range **40–1,834 samples**, approximately **1.67–76.42 ms**. Bin 39 uses 192 samples/8 ms; bin 40 jumps to 364/15.17 ms because the resolution divisor changes. The **2,000-sample cap** and **4,096-sample history** are not the window used by every bin. [B:19–20,115–120; `gdft_goertzel.h:12–16`] |
| Actual bin centres | [INFERENCE, source arithmetic] The code rounds each target to an integer DFT bin before Q14 coefficient quantisation. The nominal 110 Hz bin first rounds to approximately **104.69 Hz**, then Q14 shifts its effective resonator frequency again. This is not an exact semitone analyser. [B:121–128] |
| Spectrum smoothing | [FACT] Raw magnitude first uses **0.3 new + 0.7 old**, then a Mood-dependent low-pass. Default Mood **0.05** gives a nominal **1.5 Hz** coefficient calculated at 100 Hz but applied at 133⅓ Hz. [AH:37,43; A:103–111,274,313–326] |
| Smoothing time | [INFERENCE, linear-filter arithmetic] Those two smoothers contribute approximately **17.5 + 75.9 = 93.4 ms low-frequency mean lag**, before the nonlinear novelty stage. This is not a measured onset delay, and must not simply be added to the physical latency receipt. [A:274,323–326] |
| Spectral normalisation | [FACT] Adaptive envelope/noise estimate, gate hysteresis **4× / 2.5× noise**, target **0.25**, gain range **0.1–10**, bass tilt **1.30 below 200 Hz**, treble tilt **0.85 above 3 kHz**, soft knee above **0.5**, then clamp **0–1**. [A:329–358] |
| Novelty | [FACT] Positive difference of that conditioned spectrum, averaged over **80 bins**, square-rooted and stored in Q16. No separate sub-band novelty enters this tempo path. [A:341–367,289–297] |
| Silence | [FACT] Raw-RMS entry **0.04**, exit **0.08**, **5 s dwell**, using the previous hop’s RMS. Tempo additionally has its own 128-update history-range silence detector. [AH:38–40; W:14; A:255–266; T:74–99] |
| Tempo | [FACT] **96 one-BPM bins, 60–155 BPM**; selection prior **88 BPM**, σ **0.75 octave**; confidence prior **120 BPM**, σ **0.9 octave**. Four ACF teeth weighted **1, 0.8, 0.7, 0.6**. [T:17–26,500–518; C:27–28,148–164] |
| Tempo chronology | [FACT] Three-hop maximum aggregation gives **44.44 Hz / 22.5 ms** updates. History and confidence warm-up are **512 updates / 11.52 s**. Challenger needs **10% advantage for five updates**. [T:170–186,242–252,568–595; `tempo_acf.h:11`] |
| Beat correction | [FACT] Onset peak confirmation waits for the following tempo update; phase correction runs only while locked. Flywheel correction cap **0.10 cycle**, frequency pull **±4%**, coast **8 beats**. No meter/downbeat field exists in the active `InhouseWaveformAudio` publication. [T:306–408; W:81–113] |
| New v9 sub-band | [FACT] Two cascaded RBJ band-pass sections, nominal **40–110 Hz**, followed by hop RMS. The release defaults are `enabled=false`, `trigger_sub=false`; accent boost and widening also default to zero. [S:15–55; W:53–76] |

**Algorithm and cost basis**

[INFERENCE] There is no defensible universal “best algorithm” for every requested feature under this microphone and latency constraint. The useful distinction is **causal execution**, **decision availability**, and **time needed to establish musical meaning**. A tracker can update every 7.5 ms while needing several beats of past context. [P7:103–109; D3:54–71]

[INFERENCE] Fifty milliseconds contains **1,200 samples at 24 kHz**, but only **two periods of 40 Hz**. Broad sub-bass energy is feasible; reliable low-note identity is a different problem. Two signals with identical pasts and different future drops cannot be distinguished causally before they diverge. [AH:22–27; D2:82–88]

[HYPOTHESIS] All proposed CPU costs below are planning estimates for static-buffer C/C++ at **1 GHz**, with suitable memory placement and optimised DSP—not device measurements. At a 7.5 ms hop, **7.5 million cycles are available**; 10k cycles is **0.13%**, 100k is **1.33%**. Shared frontend costs must be counted once.

[FACT] Existing device evidence reports roughly **0.70 ms in-house processing** on active hops, with combined AP maxima around **1.29 ms**. It does not benchmark the proposed additions. [LAT:40]

[FACT] Renesas documents the RA8P1 U55 as **256 MAC/cycle at up to 500 MHz**, or **128 GMAC/s theoretical peak**. This is capability documentation, not a measured active configuration. [Renesas datasheet](https://www.renesas.cn/zh/document/dst/ra8p1-group-datasheet), accessed **2026-10-15**.

[INFERENCE] For the small filters, reductions and state machines below, **U55: CPU** means keep the work on M85; there is no evidenced useful direct NPU implementation. For neural options, conversion, quantisation, operator support, memory traffic and CPU fallback must be measured. [Arm integration documentation](https://github.com/ARM-software/CMSIS-Ethos-U), accessed **2026-10-15**.

[FACT] Relevant research/reference families:

- **Onsets:** SuperFlux’s frequency-axis maximum filter suppresses vibrato-induced flux; aubio provides energy, spectral-flux and complex-domain alternatives. [SuperFlux paper](https://phenicx.upf.edu/system/files/publications/Boeck_DAFx-13.pdf), [aubio descriptors](https://aubio.org/doc/0.4.6/specdesc_8h.html), accessed **2026-10-15**.
- **Beat/downbeat/meter:** BeatNet combines causal neural activations with particle filtering. Its **local implementation**, however, documents 84 ms streaming delay. [BeatNet paper](https://arxiv.org/abs/2108.03576), accessed **2026-10-15**; [BN:154–158].
- **Causal inference reference:** madmom has forward beat inference, but framing must explicitly use the correct causal origin; an online decoder does not make a centred frontend causal. [`madmom/features/beats.py:1064–1095`; `madmom/audio/signal.py:1128–1141,1274–1277`, at the pinned madmom commit].
- **Tonal/timbral descriptors:** Essentia supplies HPCP, tuning, key, spectral contrast, centroid, flatness and related descriptors. These are reusable methods, not a qualified Titan implementation. [Essentia overview](https://essentia.upf.edu/documentation.html), [tonal extraction](https://essentia.upf.edu/tutorial_tonal_hpcpkeyscale.html), accessed **2026-10-15**.
- **Structural novelty:** self-similarity/checkerboard methods are useful references, but a centred comparison uses information on both sides of a boundary; it needs causal adaptation for this task. [FMP novelty segmentation](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C4/C4S4_NoveltySegmentation.html), accessed **2026-10-15**.
- **Vocal presence:** singing-voice classification is distinct from ordinary speech activity. YAMNet’s standard **0.96 s examples / 0.48 s hop** are unsuitable as a direct ≤50 ms vocal-onset detector. [YAMNet reference](https://github.com/tensorflow/models/blob/master/research/audioset/yamnet/README.md?plain=1), [singing-voice review](https://arxiv.org/abs/1806.01180), accessed **2026-10-15**.

**Lever records**

[HYPOTHESIS] Every experiment below is a **proposal to freeze before execution**, not a claim that a new criterion was frozen or tested. Use the existing `scripts/freeze_tolerance.py:1–15`; retain tuning/holdout separation and the existing P7 criteria. Scores are engineering prioritisation judgements, not measured taste.

**L01 — Preserve acoustic level and low-level precision**

- **id; container:** D07-L01; AP.
- **stage:** `k1_pdm_loud_guard_q8`, `platform/ra8p1/k1_pdm_sensitivity.h:12–33`; `InhouseAudio::analyse`, A:228–254.
- **final-light control:** [INFERENCE] Loudness, attack strength, width and the contrast available between build and drop. [V:934–949,970–977]
- **current v8/v9:** [FACT] Both receive ×16/limited PCM and Q16-derived RMS. v9 maps pre-spectral-AGC RMS through **−33 to −9 dBFS**, then level emphasis **1.5**; v8 also uses follower-normalised peak. [A:228–254; W:48–50; V:940–949]
- **information preserved/destroyed:** [INFERENCE] The block limiter starts reducing nominal gain above raw peak **1,875**. Per-sample Q16 squaring produces zero energy for `|sample|<128`, approximately **−48.2 dBFS amplitude**; all such samples disappear from RMS even though PCM is nonzero. [Sensitivity:9–22; A:233–237]
- **origin:** [FACT] Sensitivity comments explicitly describe restoring a DualMCU ×16 analogue; RMS rounding is retained source arithmetic. **No inspected evidence proves that the RMS dead zone was an intentional aesthetic choice.** [Sensitivity:5–8; A:233]
- **still needed / cost:** [HYPOTHESIS] Preserve input protection; replace early energy rounding with float or wide accumulation. **1–5k cycles/hop**; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Publish protected PCM level, limiter gain and gain-normalised level separately. Use crest factor and fast/slow power, without boosting silence.
- **interactions:** [INFERENCE] Every level, silence, sub-band and dynamics detector inherits the upstream limiter. [ `pdm_target.c:625–675`; A:228–267]
- **room robustness:** [HYPOTHESIS] Distance/gain variation requires relative dynamics alongside absolute level; limiter gain changes must not masquerade as musical compression.
- **visual impact / novelty / effort:** [HYPOTHESIS] **4 / 2 / 2**.
- **risk:** [INFERENCE] Removing protection causes clipping; retaining only normalised level erases meaningful dynamics.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze RMS error **≤1% for int16 sine amplitudes 64–16,000**, exact zero for zero PCM, and no saturation. Plant the current per-sample Q16 square operation: low-amplitude cases must fail.

**L02 — Sub-bass and kick fundamentals below 110 Hz**

- **id; container:** D07-L02; AP.
- **stage:** `SubBandDetector::process`, S:15–55; `updateReleaseV9`, V:752–795.
- **final-light control:** [HYPOTHESIS] Low-end weight and release strength independent of treble or chord brightness.
- **current v8/v9:** [FACT] v8 has no dedicated sub-band publication. v9 computes **40–110 Hz RMS**, but the release is off and does not select sub-band triggering by default. [AH:8–10; A:239–241; W:65–76]
- **information preserved/destroyed:** [FACT] Adds one scalar from frequencies absent as dedicated note targets. [B:18–26; S:17–18] [INFERENCE] It still collapses bass pitch, attack shape and several low-frequency sources into one number. “Blind below 110” means no dedicated bank coverage, not zero spectral leakage.
- **origin:** [FACT] Added explicitly for P6 release work, commit `1416cb5`; not an old S3 budget setting.
- **still needed / cost:** [HYPOTHESIS] **Yes.** Two biquads plus energy approximately **5–10k cycles/hop**; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Retain 40–110 Hz; compare overlapping **30–70 / 70–150 Hz** envelopes, their derivatives and broadband ratios. RBJ biquads are a suitable causal baseline. [Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Useful for L04/L05/L09; insufficient alone for L08 when bass arrives after the annotated drop. [D2:82–88]
- **room robustness:** [HYPOTHESIS] Vulnerable to room modes, speaker roll-off and low-frequency ringing; broad overlapping bands and relative rises are preferable to a single narrow resonator.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 3 / 2**.
- **risk:** [INFERENCE] More selective filtering increases settling delay; late sub-bass cannot meet an earlier semantic deadline.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze burst-response agreement with an independent filter oracle within **±0.5 dB**, release detection within **50 ms** on a declared bass-return fixture, and zero triggers on matched-level 1 kHz bursts. Plant the existing **660 Hz wrong-centre** mutation. [S:23–26]

**L03 — Separate transient windows from pitch windows**

- **id; container:** D07-L03; AP.
- **stage:** `makeGdftConfiguration`, B:102–131; `analyseInhouseScalar`, A:45–69.
- **final-light control:** [INFERENCE] Timing of attacks versus stability/detail of colour and harmonic texture.
- **current v8/v9:** [FACT] Same variable rectangular-window bank, integer-bin frequency rounding and Q14 coefficients. [B:115–128; A:268–271]
- **information preserved/destroyed:** [INFERENCE] Low bins integrate about **76 ms**, while high bins integrate under **2 ms**. The bank is a collection of differently aged summaries, not one simultaneous spectrum. Cross40 changes temporal support abruptly. [B:115–120]
- **origin:** [FACT] Inherited Cross40/rectangular configuration. [A:268–270] [INFERENCE] Budget motivation is plausible but unproved by the inspected source.
- **still needed / cost:** [HYPOTHESIS] Retain long support for tonal stability; add a shared short frontend. A **512-point/21.33 ms** or **1,024-point/42.67 ms** real FFT every 7.5 ms is approximately **50–200k cycles/hop**, including window/magnitude work; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Short Hann-window spectrum for transients; longer tonal analysis for harmony. Do not shorten all note windows indiscriminately.
- **interactions:** [INFERENCE] Supplies L04/L05/L11/L12 and prevents slow tonal smoothing from setting event latency.
- **room robustness:** [HYPOTHESIS] Short windows expose attacks but remain sensitive to reflections; long windows stabilise harmonics but smear events.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 3 / 3**.
- **risk:** [INFERENCE] A wider bandwidth or sharper note resolution changes the donor’s visual texture.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze separate transient and tonal tests: **p95 attack decision ≤25 ms**, and no more than **1 dB tonal-magnitude error** on the chosen steady-tone oracle. Plant a **100 ms spectrum smoother**; timing must turn red.

**L04 — Preserve unsmoothed, per-band spectral flux**

- **id; container:** D07-L04; AP.
- **stage:** `InhouseAudio::condition`, A:313–367.
- **final-light control:** [HYPOTHESIS] Crisp bass pulses, snare accents, hats and simultaneous independent movement.
- **current v8/v9:** [FACT] Both tempo paths receive one positive-flux scalar calculated after Mood smoothing, AGC, knee and clamp. v9’s optional accent uses only the lowest **12 note bins**, still after conditioning. [A:341–367; V:725–743]
- **information preserved/destroyed:** [INFERENCE] **80 changes → one scalar** loses frequency identity; positive-only differences lose offsets/cuts. Saturation can make different loud spectra produce similar or zero flux.
- **origin:** [FACT] Source `calculate_novelty` behaviour is explicitly preserved. [A:365–367]
- **still needed / cost:** [HYPOTHESIS] Keep the legacy scalar for compatibility; add pre-gain log flux for **4–8 bands**, separate positive/negative changes and adaptive whitening. **5–20k cycles/hop incremental** after shared spectrum; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Compare ordinary flux, SuperFlux frequency-max suppression and complex-domain novelty. Use past-only thresholds and bounded peak confirmation. [SuperFlux paper](https://phenicx.upf.edu/system/files/publications/Boeck_DAFx-13.pdf), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Directly supplies L05–L09; should not depend on each board’s Mood.
- **room robustness:** [HYPOTHESIS] SuperFlux helps tonal vibrato; reverberant secondary attacks still require refractory and decay-aware thresholds.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 4 / 3**.
- **risk:** [INFERENCE] Over-whitening can amplify microphone noise and create false rhythmic confidence.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze per-band onset recall **≥95% within +50 ms**, false events **≤1/minute** on declared sustained/vibrato controls. Plant an **all-band sum broadcast into every band**; band-isolation checks must fail.

**L05 — Distinguish kick, snare, hat and generic transients**

- **id; container:** D07-L05; AP.
- **stage:** Active publication W:81–113; reusable `src/k1/core/audio/onset_beat.h:10–37,59–96`.
- **final-light control:** [HYPOTHESIS] Different musical roles receive different spatial weight: kick thrust, snare articulation, restrained hat detail.
- **current v8/v9:** [FACT] Active in-house publication exposes novelty but no kick/snare/hat events. A separate `OnsetBeat` implementation contains these fields; the non-Emotiscope pipeline uses it. [W:81–113; `audio_pipeline.cpp:146–148`; R:271–319]
- **information preserved/destroyed:** [INFERENCE] One scalar cannot distinguish a snare roll from kick density when total flux is similar.
- **origin:** [FACT] Separate legacy pipeline capabilities exist, but are not automatically active in the in-house path. [R:246–319]
- **still needed / cost:** [HYPOTHESIS] **Yes, as confidence-weighted roles rather than hard instrument truth.** Band ratios, attack/decay, crest and spectral flatness: **10–40k cycles/hop** beyond shared features; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Start with causal multiband templates; only consider a small trained classifier if confusion remains. Reuse `OnsetBeat` concepts, not its assumptions unchecked.
- **interactions:** [INFERENCE] Helps choose tactus level and bar accents; avoids making every transient a drop.
- **room robustness:** [HYPOTHESIS] Kick/snare overlap and reverberation weaken rules; music-specific training with room responses may improve robustness.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 4 / 3**.
- **risk:** [INFERENCE] Incorrect instrument labels can make the show confidently emphasise the wrong event.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze a kick/snare/hat confusion matrix with **macro-F1 ≥0.85**, event decisions **≤50 ms**, and silent false-event limit. Plant a **kick/snare label swap**.

**L06 — Tactus range and octave/3:2 selection**

- **id; container:** D07-L06; AP.
- **stage:** `selectionScore`, `updateWinner`, `initialiseTempoTracker`; T:135–186,500–518.
- **final-light control:** [INFERENCE] Motion cadence and the grid on which release predictions occur.
- **current v8/v9:** [FACT] **60–155 BPM**, 88 BPM selection prior; no explicit metrical-level selection stage in this implementation. [T:17–26,135–186,508–518; P7:145–147]
- **information preserved/destroyed:** [FACT] Tempos **156–200** cannot be selected. P7 reports BOOM **138→69/92**, Sonic approximately **142→71**, Dopex approximately **150→75**. [P7:103–125]
- **origin:** [FACT] Portable source preserves S3 V2 confidence/flywheel behaviour from `c3e13ff…`. The reason for choosing 88 BPM is not established here. [TH:125–128]
- **still needed / cost:** [HYPOTHESIS] Continuity preference remains useful; this fixed prior/range does not. Maintain candidates at **½, 1, 2, ⅔, 3/2** with band evidence and metrical scoring. **20–100k incremental cycles/update**, amortised below approximately **0.5% CPU** at 44.44 Hz; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Extend range, flatten prior, retain multiple candidates, score kick/backbeat consistency. Do not simply double any low BPM.
- **interactions:** [INFERENCE] Depends on L04/L05; supplies L07/L08.
- **room robustness:** [FACT] P7’s flat-prior improvement appeared in both file and room replays, but was not universal. [P7:115–147]
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 3 / 4**.
- **risk:** [INFERENCE] Half-time can be musically legitimate; the annotation convention must define which level is wanted.
- **smallest decisive experiment:** [HYPOTHESIS] Reuse the existing **326-case P7 gate unchanged**—±2% asserting BPM, four-beat acquisition, two-beat step recovery, prohibited octave/ratio locks. Plant ×½ and ×⅔ outputs. [P7:170–213]

**L07 — Beat phase, event timestamps and prediction uncertainty**

- **id; container:** D07-L07; AP/system.
- **stage:** `computeOnsetPhase`, `advanceFlywheel`, T:306–408; acquisition A:388–400.
- **final-light control:** [INFERENCE] Whether thrust lands on the beat rather than merely travelling at the right average speed.
- **current v8/v9:** [FACT] Same 22.5 ms tempo cadence and delayed local-peak confirmation. Phase correction requires lock. v9 mixes tempo motion up to **70%**, controlled by confidence **0.30–0.60**. [T:316–367; V:980–992]
- **information preserved/destroyed:** [INFERENCE] Three-hop max hold discards the winning onset’s exact timestamp. One subsequent tempo update is needed to confirm a peak, adding **22.5 ms availability delay** before other smoothing effects.
- **origin:** [FACT] Inherited flywheel design. [TH:125–128]
- **still needed / cost:** [HYPOTHESIS] Keep prediction/continuity, but preserve onset time and uncertainty. Fractional-phase PLL or a small state estimator: **2–15k cycles/hop**; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Update phase on the fast event path; estimate sensor/filter delay separately; predict to intended render/emit time; publish phase variance.
- **interactions:** [INFERENCE] Correct BPM without correct phase cannot satisfy the drop window. [D3:58–71]
- **room robustness:** [HYPOTHESIS] Reflections and soft attacks increase phase uncertainty; do not compensate them with one arbitrary global offset.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 4 / 4**.
- **risk:** [INFERENCE] Retrospectively corrected event timestamps can look accurate while decisions still arrive late.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze **event-time error p95 ≤25 ms** and **decision availability ≤50 ms**, reporting both independently. Plant a **one-tempo-update delay** and a **half-beat phase shift**.

**L08 — Downbeat, bar phase and meter**

- **id; container:** D07-L08; AP.
- **stage:** Missing from active W:81–113; prospective consumer `updateReleaseV9`, V:752–795.
- **final-light control:** [HYPOTHESIS] Phrase-scale preparation and an edge-to-centre release on the intended bar boundary.
- **current v8/v9:** [FACT] Active publication carries BPM/beat phase/confidence, not meter or bar position. The research v3 clock chose the strongest of four beat positions and failed. [W:96–100; D3:54–69]
- **information preserved/destroyed:** [INFERENCE] A beat phase does not distinguish beat **1 from 2/3/4**. Assuming four beats also destroys meter uncertainty.
- **origin:** [INFERENCE] Missing feature, not an established compute-saving compression.
- **still needed / cost:** [HYPOTHESIS] **Yes for predictive drops.** Small beat-synchronous meter/accent HMM or particle filter: approximately **30–200k cycles per update**, depending on state count. U55: CPU for inference state; neural activations are a separate option.
- **manipulation options:** [HYPOTHESIS] Compare explicit **3/4 and 4/4** hypotheses, kick/snare accents, harmonic changes and phrase history. Permit “unresolved”; avoid selecting a bar by loudness alone. BeatNet provides a causal joint-model reference. [BeatNet paper](https://arxiv.org/abs/2108.03576), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Requires L06/L07; tension should arm a prediction, not fabricate a downbeat.
- **room robustness:** [HYPOTHESIS] Several bars of accumulated evidence can resist individual missed attacks, but genre/accent ambiguity remains.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 5 / 5**.
- **risk:** [FACT] v3 cancelled ten off-target provisional fires after 150 ms. [D3:79–93] [INFERENCE] If those fires had already produced light, cancellation could not erase the visual mistake.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze bar-label accuracy **≥95% after eight bars**, decision error in **[0,+50] ms** for every declared drop, and zero visible false releases in declared negatives. Count provisional visible fires. Plant a **one-beat bar rotation**.

**L09 — Multiscale dynamics, energy and tension**

- **id; container:** D07-L09; AP/map.
- **stage:** A:247–254; V:725–795; W:53–76.
- **final-light control:** [HYPOTHESIS] Gradual contraction during a build and release magnitude based on accumulated musical change.
- **current v8/v9:** [FACT] v8 peak drive is normalised by a follower. v9 has a disabled release using **30 ms / 2 s power followers**, **3–9 dB surge range**, **0.6 s decay**; optional accent uses **0.4 / 4 s** density. [A:247–254; W:53–76]
- **information preserved/destroyed:** [INFERENCE] Instantaneous or follower-relative amplitude cannot retain a **4–16-bar** build trajectory. The current release measures a surge, not semantic tension.
- **origin:** [FACT] Follower is inherited; v9 release is a P6 experiment. [A:248; W:63–64]
- **still needed / cost:** [HYPOTHESIS] **Yes.** Parallel **20–50 ms, 0.5 s, 2 s and beat/bar-scale** levels, crest, onset density and spectral-brightness slopes: **5–20k cycles/hop**; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Keep a vector of tension cues rather than one “energy” number; combine rising density, high-frequency growth and bass withdrawal.
- **interactions:** [INFERENCE] L08 determines when release is credible; L01 prevents AGC from flattening its strength.
- **room robustness:** [HYPOTHESIS] Relative trends resist level changes better than absolute thresholds, but microphone movement and room noise can create false trends.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 4 / 3**.
- **risk:** [INFERENCE] More light is not equivalent to more thrust; the mapping must express displacement/velocity, matching the Captain’s definition. [BRIEF:26]
- **smallest decisive experiment:** [HYPOTHESIS] Freeze paired build/drop and loud-non-drop passages: correct tension ordering in **≥90%**, no release on the labelled climax/riser negatives, and all accepted releases inside the existing timing window. Plant **broadband RMS alone** as the tension score.

**L10 — Separate silence, musical cuts and rhythmic uncertainty**

- **id; container:** D07-L10; AP/map.
- **stage:** A:255–266; T:74–99,568–569; V:138–142.
- **final-light control:** [INFERENCE] True black, cut contrast, immediate return and whether tempo can assert during quiet passages.
- **current v8/v9:** [FACT] Shared **0.04/0.08 RMS**, five-second silence state; source novelty is zeroed by it. v9 adds a level floor through its −33 dBFS transfer. [AH:38–40; T:569; V:948–949]
- **information preserved/destroyed:** [INFERENCE] **0.04 ≈ −28 dBFS RMS**, **0.08 ≈ −21.9 dBFS RMS**. A single flag conflates quiet music, silence and rhythm absence.
- **origin:** [FACT] Explicit reconstruction defaults, not transferred microphone calibration. [AH:29–40]
- **still needed / cost:** [HYPOTHESIS] True-black gating remains necessary; split **acoustic presence**, **abrupt cut**, **rhythm confidence** and **long silence**. **2–10k cycles/hop**; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Fast negative energy/flux detects cuts; slower noise-relative hysteresis controls presence; rhythm confidence can decay without forcing all musical features to zero.
- **interactions:** [FACT] P7 found false confidence on noise, persistent locks through silence and failures on quiet/hats-only inputs. [P7:233–237]
- **room robustness:** [HYPOTHESIS] Freeze tests across fan noise, reverberant tails and low-level music; do not calibrate to one absolute room level.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 2 / 3**.
- **risk:** [INFERENCE] Over-sensitive gating destroys quiet musical continuity; normalisation without a presence gate destroys true black.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze cut declaration **≤30 ms**, exact zero output after the existing authorised drain, and no forced-silence state on declared −20/−30 dBFS rhythmic fixtures. Plant a **constant nonzero presence floor** and the old fixed threshold.

**L11 — Harmonic content, chroma, chord and key**

- **id; container:** D07-L11; AP/map.
- **stage:** V:154–179,594–624; reusable `src/k1/core/audio/chord_detect.h:10–21`.
- **final-light control:** [HYPOTHESIS] Palette position, harmonic transitions and distinction between stable pads and changing harmony.
- **current v8/v9:** [FACT] v8 folds selected spectrum bins modulo **12**, then derives a circular centroid. Current v9 path P uses **pre-knee shape**, max normalisation and the same 12-class fold. No key field exists in the active publication. [V:154–179,594–624; W:81–113]
- **information preserved/destroyed:** [INFERENCE] **80→12→one angle** discards octave/register, voicing and multimodal pitch-class structure. A centroid is not a key or chord label.
- **origin:** [FACT] Inherited colour mapping; v9 pre-knee shape explicitly addresses loud-spectrum flattening. [A:346–349; V:594–595]
- **still needed / cost:** [HYPOTHESIS] Keep chroma for colour, preserve its full vector and register separately. **5–30k cycles/hop** incremental for folding/template scores; a longer shared tonal FFT adds approximately **100–400k cycles at a slower update rate**. U55: CPU.
- **manipulation options:** [HYPOTHESIS] HPCP with spectral peaks, tuning correction and whitening; causal chord templates; multi-second key posterior. Key is context, not a guaranteed ≤50 ms onset label. [Essentia tonal methods](https://essentia.upf.edu/tutorial_tonal_hpcpkeyscale.html), accessed **2026-10-15**.
- **interactions:** [INFERENCE] L03 controls pitch resolution; L02 preserves bass root information; L16 prevents renewed centroid collapse.
- **room robustness:** [HYPOTHESIS] Room coloration and harmonics bias pitch classes; whitening and confidence help, but cannot remove ambiguous harmony.
- **visual impact / novelty / effort:** [HYPOTHESIS] **4 / 4 / 4**.
- **risk:** [INFERENCE] False harmonic certainty can stall or jerk colour.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze transposed chord fixtures: correct pitch-class rotation, cosine similarity **≥0.95** under ±12 dB gain before limiting, and no confident key on noise. Plant **one-semitone indexing error** and post-knee input substitution.

**L12 — Timbre, spectral brightness and noisiness**

- **id; container:** D07-L12; AP.
- **stage:** A:343–364; active publication W:81–113.
- **final-light control:** [HYPOTHESIS] Sharp versus soft motion, narrow versus broad detail, texture density and restrained colour contrast.
- **current v8/v9:** [FACT] A fixed bass/treble tilt exists; low/mid/high sums are collapsed into one spectral-energy field. No explicit centroid/roll-off/flatness vector is published by this path. [A:345,359–364; W:81–113]
- **information preserved/destroyed:** [INFERENCE] Equal total energy can represent a bass drone, bright noise or a cymbal; the scalar cannot distinguish them.
- **origin:** [FACT] Three-sum accumulation preserves source statement order. [A:359] [INFERENCE] No evidence establishes a deliberate product decision to omit timbre.
- **still needed / cost:** [HYPOTHESIS] **Yes, cheaply.** Spectral centroid, roll-off, flatness, crest/contrast and 8–16 mel-band ratios: **5–25k cycles/hop** after shared FFT; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Calculate descriptors on pre-tilt/pre-knee magnitudes and retain explicit frequency weighting. [Essentia descriptor catalogue](https://essentia.upf.edu/documentation.html), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Supplies L05/L09/L13; should modulate existing authored palettes rather than introduce hue-wheel sweeps.
- **room robustness:** [HYPOTHESIS] Absolute brightness is strongly affected by microphone position and speaker response; relative change is safer.
- **visual impact / novelty / effort:** [HYPOTHESIS] **4 / 4 / 2**.
- **risk:** [INFERENCE] Excessive response to hats creates constant visual agitation.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze equal-RMS dark/bright/noisy stimuli; require correct descriptor ordering on **all declared pairs**, ±6 dB gain invariance, and no motion at zero input. Plant **reversed bin frequencies**.

**L13 — Structural novelty and section hypotheses**

- **id; container:** D07-L13; AP/system.
- **stage:** A:365–367; reusable `src/k1/core/audio/musical_saliency.h:16–44`; active W:81–113.
- **final-light control:** [HYPOTHESIS] Distinct behaviour for intro, verse-like repetition, build, drop and breakdown without switching scene on every transient.
- **current v8/v9:** [FACT] Current novelty is a framewise rise metric. The separate saliency module exposes harmonic/rhythmic/timbral/dynamic novelty, but not through the active in-house publication. [A:365–367; R:271–319]
- **information preserved/destroyed:** [INFERENCE] One-hop novelty lacks repetition, duration and several-bar context; “large change” does not identify “drop”.
- **origin:** [INFERENCE] Missing semantic/context layer, not demonstrated S3 budget debt.
- **still needed / cost:** [HYPOTHESIS] Useful after L04–L10. Past-only change detection over beat-synchronous feature vectors: **10–100k cycles per beat**. A small causal trained section model is a later, higher-cost option; U55 is potentially useful only for that model.
- **manipulation options:** [HYPOTHESIS] Causal change-point detection plus duration/state constraints; preserve multiple section hypotheses and “unknown”. Use self-similarity methods as references, without borrowing their future context. [FMP segmentation](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C4/C4S4_NoveltySegmentation.html), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Combines dynamics, rhythm, timbre, harmony and vocal presence.
- **room robustness:** [HYPOTHESIS] Beat-relative changes can resist fixed room coloration; conversation, movement and speaker changes remain confounds.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 5 / 5**.
- **risk:** [INFERENCE] Reliable semantic labels within 50 ms of an unprecedented boundary are not generally available.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze boundary detection separately from semantic labelling: **boundary recall ≥90% within 50 ms** on a declared abrupt-change subset, section macro-F1 **≥0.8 after a declared confirmation interval**. Plant a **future-frame dependency**; prefix-invariance must fail.

**L14 — Vocal presence**

- **id; container:** D07-L14; AP.
- **stage:** No vocal field in active W:81–113; prospective parallel analysis from admitted PCM, A:200–205.
- **final-light control:** [HYPOTHESIS] Leave visual space around vocals, distinguish instrumental drops, or give a restrained sustained layer to vocal phrases.
- **current v8/v9:** [FACT] No vocal-presence estimate in the inspected in-house feature/publication path. [A:200–311; W:81–113]
- **information preserved/destroyed:** [INFERENCE] Harmonic brightness cannot distinguish voice from a synthesiser lead.
- **origin:** [INFERENCE] Not implemented here; no evidence of a removed donor feature.
- **still needed / cost:** [HYPOTHESIS] **Optional, lower priority than timing repair.** Small causal log-mel CNN/TCN, approximately **0.1–1 MMAC per 20 ms**: **0.2–8 million M85 cycles/update** under an assumed 2–8 cycles/MAC. U55 arithmetic floor approximately **0.8–7.8 µs**, with actual invocation/memory/fallback cost unmeasured.
- **manipulation options:** [HYPOTHESIS] Train singing-vocal presence on music and room augmentation; preserve a probability. Do not use speech VAD as equivalent singing detection. [Singing-voice review](https://arxiv.org/abs/1806.01180), accessed **2026-10-15**.
- **interactions:** [INFERENCE] Can inform L13 and visual restraint; must never gate ordinary autonomous visualisation.
- **room robustness:** [HYPOTHESIS] Nearby speech, vocal-like synths and reverberation are decisive negatives.
- **visual impact / novelty / effort:** [HYPOTHESIS] **3 / 5 / 5**.
- **risk:** [INFERENCE] No inspected model establishes reliable vocal onset classification within 50 ms; fast updates are not proof of fast semantic recognition.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze voice/synth/instrumental/speech cases, **macro-F1 ≥0.9**, first-decision latency reported independently, and zero future dependence. Plant **future-context frames** and a **speech-only classifier**.

**L15 — Feature chronology, age and uncertainty**

- **id; container:** D07-L15; system/AP.
- **stage:** A:208–214,370–429; `platform/ra8p1/k1_poly24.h:57–74`.
- **final-light control:** [INFERENCE] Synchronisation between observed audio events, predicted beats and the emitted frame.
- **current v8/v9:** [FACT] Epoch, generation, sample interval, capture and publication times exist; tempo has a separate capture timestamp. Feature-specific window centres/delays and confidence intervals do not appear in the active structure. [W:96–113; A:388–400]
- **information preserved/destroyed:** [INFERENCE] One capture timestamp names the newest sample but does not describe the different effective ages of a 1.7 ms high bin, 76 ms low bin and smoothed novelty.
- **origin:** [FACT] Chronology was deliberately preserved; P4 corrected resampler alignment and later Q32 wrap behaviour. [ `k1_poly24.h:60–74`; `docs/evidence/P4/corrections/RECEIPT-ITERATION-2.md:65–68` ]
- **still needed / cost:** [HYPOTHESIS] **Yes.** Add support interval/event time/available time for event features and uncertainty for predictions. **1–5k cycles/hop**, small metadata footprint; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Share the existing epoch/sample clock; keep “detected at” separate from “event estimated at”; extrapolate only permitted predictors.
- **interactions:** [INFERENCE] Required for honest L07/L08 latency acceptance and comparisons between fast/slow paths.
- **room robustness:** [INFERENCE] Does not remove acoustic delay; it prevents software from concealing or double-counting it.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 2 / 3**.
- **risk:** [INFERENCE] A timestamp correction can improve a graph without changing late light.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze prefix-causal replay with known event times, event/availability ordering and epoch resets; every decision must use samples available by its declared time. Plant a **one-hop timestamp lead** and **cross-epoch history reuse**.

**L16 — Preserve parallel feature views through the AP→VP boundary**

- **id; container:** D07-L16; AP/map.
- **stage:** `InhouseWaveformAudio`, W:79–113; `condition`, A:313–367; `renderV9`, V:934–998.
- **final-light control:** [HYPOTHESIS] Independent motion, colour, brightness and spatial articulation instead of making one flattened statistic control everything.
- **current v8/v9:** [FACT] v8 receives conditioned spectrum and scalar dynamics. v9 adds sub-RMS and pre-knee shape, but tempo still consumes conditioned scalar novelty. Per-board Mood changes each board’s conditioning and therefore its tracker input. [AH:90–115; A:279–297,346–367]
- **information preserved/destroyed:** [INFERENCE] Current publication lacks a band-event vector, metrical candidates, bar phase, uncertainty and gain provenance. Different consumers cannot recover these after collapse.
- **origin:** [FACT] v9’s pre-knee colour path is an existing example of preserving a second view to avoid information loss. [A:346–349; V:594–624]
- **still needed / cost:** [HYPOTHESIS] **Yes.** A bounded shared feature struct with fast events, slow context and presentation views: approximately **1–10k cycles/hop**, usually under **1–2 KB** additional publication/state before model history; U55: CPU.
- **manipulation options:** [HYPOTHESIS] Keep analysis independent of board Mood; smooth at the consumer. Preserve current consumers while introducing explicit new fields. Share FFT/bands across boards.
- **interactions:** [INFERENCE] Enables every preceding lever without duplicating whole audio pipelines.
- **room robustness:** [HYPOTHESIS] Common calibrated evidence avoids each board interpreting the same room differently merely because its visual Mood differs.
- **visual impact / novelty / effort:** [HYPOTHESIS] **5 / 4 / 4**.
- **risk:** [INFERENCE] Publishing extra fields alone creates no visual value; each must have a tested real consumer.
- **smallest decisive experiment:** [HYPOTHESIS] Freeze an identical PCM replay with two Mood settings: **beat/onset event timestamps must remain identical**, while authorised visual smoothing differs. Plant a dependency from Mood into the rhythm branch; the independence test must fail.

**Neural option: what the existing references actually buy**

[FACT] The local BeatNet model uses a 1D convolution, a 150-unit projection, **two unidirectional 150-unit LSTM layers**, and a three-output head. Its particle filter defaults to **1,500 beat particles**, **250 downbeat particles**, **55–215 BPM** and **2–4 beats/bar**. [`BeatNet/src/BeatNet/model.py:18–30`; `particle_filtering_cascade.py:108–120`, pinned BeatNet commit]

[INFERENCE, architecture arithmetic] That network is approximately **405,010 MAC/frame**, or **20.25 MMAC/s at 50 Hz**, excluding feature extraction, nonlinearities and particle filtering. At an assumed **2–12 M85 cycles/MAC**, its matrix work is approximately **0.81–4.86 million cycles/frame**, or **4–24% CPU**. This is a feasibility estimate, not a benchmark.

[INFERENCE] U55’s theoretical arithmetic floor is approximately **3.2 µs/frame** for those MACs. That number is not an inference-time prediction: LSTM conversion/operator placement, state transfer, activation functions and particle filtering remain material. A causal convolutional replacement may map more cleanly, but would be a new trained model requiring validation. [BeatNet model source above; Renesas datasheet cited above]

[FACT] hybrid-beat-tracker contains useful C++ SuperFlux and phase-lock code, but the inspected SuperFlux batch path allocates vectors and its peak picker accepts post-context; the main detector’s `process_file` returns `UnsupportedFormat`. It is a source library to examine, not a demonstrated embedded drop detector. [`core/onset/superflux.cpp:42–59,92–126`; `core/beat_detector.cpp:135–145`, pinned hybrid commit]

[HYPOTHESIS] The smallest useful neural investigation is a **source-bound host comparison** of causal activations against L04/L06/L08, followed by an integer model/operator placement report. Do not start with a firmware transplant or assume the NPU eliminates frontend latency.

**Known failures mapped to levers**

| Failure | Explanation supported by evidence | Relevant levers |
|---|---|---|
| Drop lacks the intended thrust | [FACT] Captain defines impact through contraction/release geometry, not just brightness. [BRIEF:26] [INFERENCE] A correct energy increase can still produce the wrong motion. | L02, L08, L09, L16 |
| Bass-triggered release misses +50 ms | [FACT] Sonic bass steps were **72 ms and 117 ms late** in v2; BOOM’s bass body was outside the window. [D2:82–88] | L02, L04, L07, L08 |
| Half-time and 92 BPM lock | [FACT] Restricted range, 88 BPM prior and metrical ambiguity are directly supported by P7 interventions. [P7:103–147] | L04–L08 |
| “Correct BPM” still misses drop | [FACT] Reference-clock beat errors were **−74 to +65 ms** on several cases, with wrong/unresolved bar positions. [D3:60–69] | L07, L08, L15 |
| Noise asserts tempo; quiet hats fail | [FACT] P7 reports both failures and the five-second absolute-RMS interaction. [P7:233–237] | L01, L04, L10 |
| Loud music flattens colour information | [FACT] Current v9 explicitly bypasses knee/clamp for colour shape. [A:346–349; V:594–624] | L01, L11, L16 |
| Historical S3 dull/held colour | [FACT, documentary evidence] The forensics records loud/dense input, chroma flattening, gating and held-hue freezing, with S3 `ae5d90a` lineage. [`docs/evidence/P6/palette-forensics/FORENSICS.md:13–20,73–74`] | L11, L16 |
| S3-derived tempo assumptions persist on Titan | [FACT] Tracker source explicitly preserves the S3 V2 path. [TH:125–128] [INFERENCE] Titan’s faster processor does not alter inherited priors or discarded information. | L04, L06, L07 |
| Slow-looking light despite short transport budget | [FACT] P5’s approximately 16 ms figure is a sample/publication/render/wire budget with inferred frame wait. [LAT:9–17] [INFERENCE] It does not measure feature-estimator response or semantic recognition delay. | L03, L04, L07, L15 |
| Host success does not select product behaviour | [FACT] PL14-B14 records the wrong compile-time palette default despite passing host variants. [`Kx-Titan/docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md`, PL14-B14; W:42–47] | L16; every proposed consumer |

**Contradictions and corrections**

1. **[FACT] “Titan is blind below 110 Hz” is incomplete for current v9 source.** The bank starts at 110 Hz, but a separate 40–110 Hz detector exists. **[INFERENCE]** The meaningful unresolved questions are its response, activation and consumption—not its mere existence. [B:18–26; S:15–55; W:65–76]

2. **[FACT] “Three detectors failed to fire” conflates whole-set failure with no hits.** v1 hit BOOM file; v2 hit Carte Blanche; v3 hit none. All failed complete acceptance. [D1:54–69; D2:60–88; D3:83–93]

3. **[FACT] The brief’s “predicted downbeats 26–380 ms off” mixes different quantities.** Product nearest-beat errors included **−380 to +188 ms**; the separate reference clock had a **−26 ms nearest beat** and **−454 ms bar-downbeat error** on Carte Blanche. These are not one downbeat-error distribution. [D3:41–48,60–67]

4. **[FACT] The 120 Hz target conflicts with the measured P5 profile’s 100 Hz render cadence.** P5 reports 100.01 frames/s and a 10 ms scheduling period. **[INFERENCE]** Use the exact candidate profile when budgeting a future feature. [BRIEF:32; LAT:81]

5. **[FACT] “Pre-AGC” does not mean untouched microphone level.** The v9 RMS and sub-band inputs are before spectral AGC but after the PDM ×16/block limiter. [ `pdm_target.c:625–675`; A:228–240; V:940]

6. **[INFERENCE] Nominal note labels are not exact filter centres.** Integer-frequency rounding and Q14 coefficient quantisation intervene. “110 Hz bank boundary” is a design description, not an ideal brick-wall cutoff. [B:121–128]

7. **[FACT] v9 release infrastructure is present but disabled by default.** Its existence cannot be reported as an active drop detector. [W:65–76]

8. **[FACT] P7’s existing gate reports phase but does not gate it.** Passing that gate would not establish the requested predictive-downbeat accuracy. [P7:170–177,328]

9. **[FACT] BeatNet’s “online” research framing does not make this local streaming implementation meet 50 ms.** The source documents 84 ms delay; its file/realtime paths also require framing inspection. [BN:154–188]

10. **[FACT] The original palette forensics says no 80-bin historical fix was found; the later swarm brief cites `c2738b88` as that fix.** Treat the earlier conclusion as superseded/incomplete, not negative proof. [`FORENSICS.md:22`; BRIEF:14–21]

11. **[FACT] PL14 calls the Q32 wrap defect pending, but the later correction receipt and current helper describe the repair and planted-fault test.** This illustrates why lesson summaries must be checked against later source/receipts. [`k1_poly24.h:60–65`; `docs/evidence/P4/corrections/RECEIPT-ITERATION-2.md:65–68`]

12. **[FACT] Brightness authority conflicts remain explicitly documented:** the brief states **24/255 on the owner’s unit**, while migration instructions and detector documents retain **100/255 default**. This excavation does not resolve or change that setting. [BRIEF:32; D3:109]

**Highest-leverage order and remaining questions**

[HYPOTHESIS] The practical order is:

1. **Preserve a fast, pre-normalisation event path** and fix low-level energy arithmetic; reuse the existing sub-band rather than adding another equivalent detector.
2. **Repair tempo metrical selection and confidence** against the existing P7 gate.
3. **Add separately scored phase and downbeat prediction**, including uncertainty and visible false-fire accounting.
4. **Connect the accepted event to actual release geometry**, preserving authored multicolour palettes and true black.
5. Add **timbre and harmonic context** cheaply; pursue section/vocal models only if they improve a named visual decision.

[INFERENCE] The main unanswered questions are:

- Which exact **resident image, build flags and source hashes** correspond to Captain’s v9 verdict? Source inspection cannot answer live identity. [BRIEF:34; dirty-source identities above]
- Are drop annotations defined by **musical grid, first audible transient, bass arrival or intended release**? Their disagreement is already visible in the detector evidence. [D1:63; D2:82–88]
- What error budget is acceptable for **predictive early releases**, not just late ones? Existing acceptance counts only `[drop, drop+50 ms]` as a hit. [D3:19–21]
- What are the installed microphone/PDM frontend’s actual low-frequency response and room transfer? `pdm_target.c:764–785` copies additional BSP configuration; nominal sub-band code alone does not establish acoustic sensitivity.
- Should both boards share one musical clock while retaining independent visual Mood? The current per-board conditioned novelty can differ. [A:279–297]
- Which additional tracks and annotations will remain truly held out? Carte Blanche was already spent; v3 explicitly reports no unseen holdout. [D1:65–69; D3:11]
- Can a proposed neural model compile predominantly to U55, and does it improve **decision availability and room robustness**, rather than merely offline accuracy? [BN:154–158; Arm integration reference above]

**Execution and limits**

[FACT] No repository files were changed, and no device, serial, USB, probe, flash, playback or commit actions were taken. Commands were source/document reads, Git identity/status queries, web research and read-only arithmetic.

[FACT] The arithmetic check exited **0**, verifying hop/update timing, warm-up duration, smoother lag, the RMS dead zone, limiter threshold and BeatNet operation count. Receipt: [`9c7c3b…json`](/Users/spectrasynq/.local/state/spectrasynq-agents/verification/9c7c3b111229b15433ff8136df52b0bd3f29c21b10c6daf33d2722724a1d89b0.json). This is arithmetic evidence, not a new DSP or device test.

[FACT] Workflow gate reported **no bound task ledger; not a completion certificate**. The semantic evidence call rejected its reference schema (`id is required`); no semantic admission is claimed.

[FACT] The requested output is this textual excavation. Images are N/A because no visual candidate was produced; source lines, calculations and the report’s consumer implications were inspected. **[HYPOTHESIS]** The next decisive work is the bounded L04/L06/L07 comparison, with frozen criteria and planted faults, before any later authorised device experiment.