# D08 — Perception levers for Titan

**THE TITAN MUST ALWAYS BE VISUALISING MUSIC.**

[INFERENCE] The strongest opportunities are preserving 16-bit precision through attenuation, separating colour from intensity, controlling the timing of visible acceleration, and shaping expansion against the preceding build. Increasing frame rate alone is a smaller timing lever than the existing attack smoothing. These conclusions follow from the source and calculations below; they are not claims of demonstrated optical improvement.

[FACT] This excavation made no repository changes and used no device, serial, USB, broker or programming access. All experiments below are proposals.

## Evidence identity and notation

[FACT] The inspected worktree was:

`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001`

Its reported HEAD was `1ef53952ab7a50af38414321c8a91e515744098a`. The tree contains uncommitted work, so HEAD alone does not identify the inspected implementation. Final source hashes:

| File | SHA-256 |
|---|---|
| `src/k1/core/visual/inhouse_waveform.h` | `88cea47b90ccc528a855fefb1cad12208b880607e47adb49cec3e29b29fc2272` |
| `src/k1/core/visual/inhouse_waveform.cpp` | `0be2360eb0326cdf53c60742ba749b796a48ee1d7d325291d2e45f03aece82ae` |

[FACT] The following abbreviations denote exact paths relative to that root. A citation such as **W:800–807** means the corresponding file and numbered source lines.

| Key | Exact path |
|---|---|
| W | `src/k1/core/visual/inhouse_waveform.cpp` |
| H | `src/k1/core/visual/inhouse_waveform.h` |
| A | `platform/ra8p1/inhouse_audio.cpp` |
| R | `platform/ra8p1/palette_runtime.cpp` |
| RH | `platform/ra8p1/palette_runtime.h` |
| HD | `platform/ra8p1/hd_pixel16.h` |
| PACK | `src/k1/core/visual/ws2816_pack.h` |
| FIX | `platform/ra8p1/fixture_app.cpp` |
| LIVE | `platform/ra8p1/k1_live_runtime.cpp` |
| BUDGET | `docs/evidence/P5/device-budget/BUDGET.md` |
| FORENSICS | `docs/evidence/P6/palette-forensics/FORENSICS.md` |
| LINEAGE | `docs/evidence/P6/palette-forensics-2/grok-k1-git.md` |
| FREEZE | `scripts/freeze_tolerance.py` |

[FACT] **PL14** denotes the separately inspected document:

`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/Kx-Titan/docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md`

[FACT] “Current v8/v9” below means the inspected source implementations, not verified resident firmware. The final header selects **v9 path P**, `kPerPixelMapped`, at **H:40–47**. Its deployment was not checked.

## Corrections that materially affect this investigation

1. **[FACT] The requested AV asymmetry is reversed relative to BT.1359.** Define \(\Delta=t_\text{light}-t_\text{sound}\): positive means light lags. BT.1359-1 gives average detection boundaries approximately **−125 to +45 ms**, and acceptability boundaries **−185 to +90 ms**. Its cited conditions therefore tolerate more **light leading** than light lagging. These are broadcasting results, not a Titan musical-impact acceptance contract. [ITU-R BT.1359-1, considering g, Appendix 1 Figure 2 and Appendix 2](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.1359-1-199811-I!!PDF-E.pdf), accessed 2026-10-15.

2. **[FACT] The approximately 16 ms budget already includes the 5.1 ms wire time.** The historical BOOM stage-stack is **16.452 ms**, including an assumed 5 ms frame wait. The **24.972 ms** figure adds stage percentiles; it is not an observed end-to-end p99. Adding another 5.1 ms double-counts transmission. **BUDGET:13–17, 28–34**.

3. **[FACT] This Emotiscope configuration specifies 100 Hz, not 120 Hz.** `K1_EMOTISCOPE_PORT` selects a **10,000 µs** period; the alternate branch selects **8,333 µs**. **RH:39–47**. Historical P5 measurements also describe 100 Hz. **BUDGET:70–81**.

4. **[FACT] A universal 24/255 firmware cap was not found in this path.** `setHardwareBrightness()` accepts values through 255; the default macro is 100 with a compile-time minimum of 100. **R:226–227; RH:18–23**. The swarm brief separately specifies a 24/255 owner envelope and explicitly asks that its conflict with the migration default be reported. **SWARM-BRIEF.md:32**. The calculations below use **24/255 as the requested analysis envelope**, without changing or resolving authority.

5. **[FACT] The waveform path already carries true 16-bit channels to GRB48 packing.** Its RGB8 frame is a preview; the Emotiscope build bypasses the apparent RGB8 lifting block in `fixture_app.cpp`. **R:273–298; FIX:657–666; R:735–737; PACK:28–34**.

6. **[FACT] Current palette handling is more specific than “Titan used a 16-slot palette.”** The v8 sampler already interpolates authored stops; its fallback scans 16 compiled entries. V8 also blends RGB contributions. Path P instead samples the HD palette for each output pixel. **W:41–70, 205–239, 885–901**. The later lineage investigation distinguishes these issues. **LINEAGE:21–27, 84–95**.

7. **[FACT] Two-board routing is not simply P601/P604 repeated.** The packer contains a separate upper-board lane mapping and orientation treatment. **R:687–740**. Board geometry and simultaneous optical presentation must be checked independently; mirror symmetry does not prove either.

## Quantitative foundation

### Precision beneath a 24/255 envelope

[INFERENCE, calculated] Let \(c=24/255\). For a full-range linear source followed by integer attenuation:

| Processing path | Maximum output code | Distinct possible levels, including black |
|---|---:|---:|
| Native 8-bit output, then attenuation | 24 | **25** |
| Native 16-bit output, then attenuation | 6,168 | **6,169** |
| 8-bit source lifted by ×257, then 16-bit attenuation | 6,168 | **256** |
| 8-bit attenuation first, then lifting | 6,168 | **25** |

[INFERENCE] Thus 16-bit transport preserves approximately **12.59 bits of code capacity** under this envelope, versus **4.64 bits** for attenuated 8-bit output. These are numerical capacities, not measured perceptual bits. Board gains, palette channels and optical noise can reduce usable resolution. Formula and implementation: **HD:22–31; R:735–737**.

[INFERENCE, calculated] For the illustrative transfer \(y=\lfloor Mcx^{2.2}\rfloor\):

- With \(M=255\), inputs below approximately **23.58%** remain zero.
- With \(M=65535\), that threshold falls to approximately **1.893%**.
- An 8-bit input grid produces **250** distinct 16-bit output values under this particular transfer; a full 16-bit grid can produce all **6,169**.
- Applying attenuation **before** that exponent gives only **2** possible 8-bit levels or **362** possible 16-bit levels over the entire capped range.

[FACT] This is a hypothetical gamma example, **not the implemented output transfer**. The inspected path uses Q16 arithmetic, level emphasis, a max-channel shoulder, white balance and integer output attenuation. Its exponent 1.5 shapes musical level; it is not a measured LED display gamma. **W:16–39, 934–960; H:48–50; R:281–298; HD:28–31**.

### Time and motion

[INFERENCE, calculated from RH:39–47 and BUDGET:28–34]

- 100 Hz: **10 ms/frame**.
- 120 Hz: **8.333 ms/frame**.
- Changing 100→120 Hz reduces uniformly distributed scheduling wait by only **0.833 ms on average**, or **1.667 ms at the worst phase**.
- The 5.1 ms transmission occupies **51%** of a 100 Hz period or **61.2%** of a 120 Hz period.
- \(45-16.452=28.548\) ms is merely the numerical distance between the historical stage-stack and one broadcast detection boundary. It is **not spare delay that should be consumed**.

[INFERENCE, calculated] The v8/v9 mode-32 positional cascade uses **16 ms and 23 ms** time constants. Its continuous-time step approximation reaches 10%, 50% and 90% at approximately **10.24, 32.50 and 76.23 ms**. Actual sampled behaviour depends on frame timing. **W:203–204, 963–978**.

[INFERENCE, calculated] At the default motion factor 1, **405 pixels/s** means **4.05 pixels/frame at 100 Hz**, **3.375 at 120 Hz**, and approximately **195 ms** to traverse 79 pixels. **W:258–267, 963–978**.

## Perception evidence used

All fetched sources below were accessed **2026-10-15**. Abstract-only evidence supports the stated finding, not unexamined experimental details.

| Ref | Evidence and relevance |
|---|---|
| **L1** | **[FACT]** ITU-R, **BT.1359-1 (1998)**, *Relative timing of sound and vision for broadcasting*, considering g, Appendix 1 Fig. 2, Appendix 2. Full standard fetched. Its timing asymmetry and stimulus limitations are stated above. [Official PDF](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.1359-1-199811-I!!PDF-E.pdf). |
| **L2** | **[FACT]** Davis, J., Hsieh, Y.-H. & Lee, H.-C. **(2015)**, “Humans perceive flicker artifacts at 500 Hz,” *Scientific Reports* **5**, 7861. Spatial edges and eye movements can expose temporal artefacts far above a conventional uniform-field fusion threshold; their result does not establish that Titan needs 500 FPS. Figs. 1–3. [Article](https://www.nature.com/articles/srep07861). |
| **L3** | **[FACT]** Braddick, O. **(1974)**, “A short-range process in apparent motion,” *Vision Research* **14(7)**, 519–527. Random-dot experiments demonstrate displacement-dependent motion correspondence; their approximately 15-arcmin limit is stimulus-specific, not an LED design rule. Publisher abstract fetched. [DOI](https://doi.org/10.1016/0042-6989(74)90041-8). |
| **L4** | **[FACT]** Vatakis, A. & Spence, C. **(2006)**, “Audiovisual synchrony perception for music, speech, and object actions,” *Brain Research* **1111(1)**, 134–142. Synchrony judgements depend on the stimulus; speech tolerances cannot simply become music-impact tolerances. Abstract fetched. [Article](https://www.sciencedirect.com/science/article/pii/S0006899306015903). |
| **L5** | **[FACT]** Su, Y.-H. **(2014)**, “Peak velocity as a cue in audiovisual synchrony perception of rhythmic stimuli,” *Cognition* **131(3)**, 330–344. Visual peak velocity influenced perceived synchrony in rhythmic motion. Abstract/highlights fetched. [Article](https://www.sciencedirect.com/science/article/abs/pii/S0010027714000298). |
| **L6** | **[FACT]** Marks, L. E. **(1987)**, “On cross-modal similarity: auditory–visual interactions in speeded discrimination,” *Journal of Experimental Psychology: Human Perception and Performance* **13(3)**, 384–394. Loudness corresponded with emitted brightness; pitch also interacted with visual brightness/angularity. Abstract fetched. [PubMed](https://pubmed.ncbi.nlm.nih.gov/2958587/). |
| **L7** | **[FACT]** Marks, L. E. **(1989)**, “On cross-modal similarity: the perceptual structure of pitch, loudness, and brightness,” *JEP: Human Perception and Performance* **15(3)**, 586–602. Correspondences depend on relative context and vary between observers. Abstract fetched. [PubMed](https://pubmed.ncbi.nlm.nih.gov/2527964/). |
| **L8** | **[FACT]** Evans, K. K. & Treisman, A. **(2010)**, “Natural cross-modal mappings between visual and auditory features,” *Journal of Vision* **10(1)**, 6, 1–12. Experiments support pitch relationships with vertical position, size and spatial frequency; they do not establish a universal pitch-to-hue wheel. Abstract/figure material fetched. [PubMed](https://pubmed.ncbi.nlm.nih.gov/20143899/). |
| **L9** | **[FACT]** Hunt, R. W. G. **(1952)**, “Light and dark adaptation and the perception of color,” *JOSA* **42(3)**, 190–199. Colour appearance changes with adaptation and luminance; reduced adapting illumination can reduce perceived saturation. Abstract/table material fetched. [Publisher](https://opg.optica.org/josa/abstract.cfm?uri=josa-42-3-190). |
| **L10** | **[FACT]** Corney, D., Haynes, J.-D., Rees, G. & Lotto, R. B. **(2009)**, “The Brightness of Colour,” *PLoS ONE* **4(3)**, e5091. The Helmholtz–Kohlrausch effect means equally luminous colours need not appear equally bright, with saturation contributing to appearance. Search-accessible article text/abstract fetched; direct repository PDF fetch failed. [Article](https://pmc.ncbi.nlm.nih.gov/articles/PMC2659800/). |
| **L11** | **[FACT]** Pridmore, R. W. **(1999)**, “Bezold–Brücke hue-shift as a function of luminance level, luminance ratio, interstimulus interval and adapting white for aperture and object colors,” *Vision Research* **39(23)**, 3873–3891. Hue appearance changes with luminance and viewing conditions. Abstract fetched. [PubMed](https://pubmed.ncbi.nlm.nih.gov/10748922/). |
| **L12** | **[FACT]** Coia, A. J. & Shevell, S. K. **(2018)**, “Chromatic induction in space and time,” *JOSA A* **35(4)**, B223–B230. Spatial and temporal surrounds alter colour appearance. Article search text fetched. [Article](https://pmc.ncbi.nlm.nih.gov/articles/PMC6022834/). |
| **L13** | **[FACT-LIT, not fetched]** Fechner, G. T. **(1860)**, *Elemente der Psychophysik*, Vol. II, Leipzig: Breitkopf & Härtel. The logarithmic formulation relates sensation to stimulus under Weber-law assumptions; it is not a universal near-black LED transfer curve. No figure/table relied upon. |
| **L14** | **[FACT-LIT, not fetched]** Stevens, S. S. **(1957)**, “On the psychophysical law,” *Psychological Review* **64(3)**, 153–181, DOI **10.1037/h0046162**. Power-law sensation models depend on modality and experimental conditions; they do not prescribe gamma 2.2 for Titan. Bibliographic metadata fetched, full paper unavailable. |
| **L15** | **[FACT-LIT, not fetched]** Stevens, S. S. & Guirao, M. **(1963)**, “Subjective scaling of length and area and the matching of length to loudness and brightness,” *Journal of Experimental Psychology* **66(2)**, 177–186. Cross-modal magnitude matching supports investigating extent alongside brightness; no numerical exponent is imported here. No figure/table relied upon. |

## Common experiment contract

[HYPOTHESIS] Every experiment below should freeze its source/image identity, exact input excerpt, analysis windows, metric definitions, thresholds, viewing geometry and planted fault **before candidate results** using **FREEZE:1–15, 37–68**. No tolerance file was created or frozen during this read-only excavation.

[HYPOTHESIS] Shared acceptance conditions:

- Exact zero input must create no new light; after the specified drain, every output channel must be zero. Proposed maximum drain gate: **1 second**, unless a particular experiment freezes a stricter value.
- Compare identical approved trance/EDM excerpts at the same authorised output envelope. Keep Dopex held out from tuning.
- Faults are host counterfactuals, not instructions to flash deliberately broken firmware.
- Separate numerical, electrical and optical measurements. A code-value difference is not a perceptual JND.
- For taste, use four randomised A/B pairs of one **10–15 second** passage. A predeclared **3/4 preference** can select Captain’s candidate; a tie remains undecided. This is an individual design decision, not population-level perceptual proof.
- Use fixed exposure for recordings and suitable optical instrumentation for millisecond timing. **PL14:163–176** identifies timing/capture interpretation failures.

[HYPOTHESIS] Scores below estimate **visual impact / novelty / effort**, each 1–5. They are prioritisation judgements, not measured outcomes. Per-lever M85 cycle costs are unknown unless explicitly stated. The historical aggregate render cost—about **197,000 cycles at 1 GHz**—cannot be assigned to individual levers. **BUDGET:29**.

---

## Lever records

### D08-01 — Align the visible event, not just the render invocation

- **Container; stage:** [FACT] **System / VP**. `render()` timestamp/freshness processing, **W:113–141**; `renderV9()`, **W:934–1024**; capture/publication clock handling, **LIVE:325–340, 443–480**.
- **What it controls:** [INFERENCE] Timing of the visible onset, peak expansion velocity and maximum extent relative to sound. These can occur at different times. **L4–L5; W:963–999**.
- **Current setting, v8/v9:** [FACT] Both have frame scheduling and feature-age constraints; mode 32 retains the 16/23 ms positional smoothing cascade. No inspected source establishes listener-position optical synchrony. **W:120–141, 203–204, 963–978; BUDGET:87–95**.
- **Information preserved/destroyed:** [INFERENCE] Timestamps preserve event ordering, but smoothing shifts perceptual landmarks. The approximately 76 ms modelled time to 90% positional response is larger than one frame.
- **Origin:** [INFERENCE] Inherited filtering and scheduling; an S3 compute-budget justification is unproved.
- **Still needed on Titan?:** [INFERENCE] Timestamp discipline **yes**; present perceptual timing **unknown**. Incremental M85 cost unknown.
- **Manipulation options:** [HYPOTHESIS] Compare added visual delay **0/10/20/40 ms** only to measure sensitivity; compare causal attack settings separately. Do not use prediction to conceal an unreliable onset detector.
- **Interactions:** [INFERENCE] Attack, cadence, extent, detector delay, speaker/listener geometry.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 3 / 3**. Risk: optimising a software timestamp while the optical event remains late.
- **Smallest decisive experiment:** [HYPOTHESIS] On a fixed passage, measure acoustic onset and optical expansion velocity. Freeze **p95 absolute landmark error ≤30 ms**, separately reporting onset and velocity. Inject **+60 ms** into the host replay; the timing gate must fail. Then run the common A/B for which landmark feels congruent.

### D08-02 — Frame cadence, jitter and flicker are separate controls

- **Container; stage:** [FACT] **System / output**. `PaletteRuntime::step()` scheduling, **R:356–361**; period selection, **RH:39–47**.
- **What it controls:** [INFERENCE] Temporal sampling of motion and exposure to repeated or irregular frames; not necessarily the LED driver’s internal PWM.
- **Current setting, v8/v9:** [FACT] Both use the **100 Hz Emotiscope branch**. Late scheduling increments the skipped-frame count and advances to a future tick. **RH:39–47; R:356–361**.
- **Information preserved/destroyed:** [INFERENCE] 100→120 Hz gains temporal samples but saves only **0.833 ms average scheduling wait**. It does not remove a 16/23 ms filter or prove flicker fusion. **L2**.
- **Origin:** [INFERENCE] Configuration/inheritance; evidence that 100 Hz remains a processor limit is absent.
- **Still needed on Titan?:** [INFERENCE] Stable cadence **yes**; exactly 100 Hz **unknown**. Total render cost historically about 0.197 ms, but transmission and two-board scheduling also constrain cadence. **BUDGET:29–31**.
- **Manipulation options:** [HYPOTHESIS] **100 versus 120 Hz** first; investigate higher rates only after measuring transport and PWM behaviour.
- **Interactions:** [INFERENCE] Integer scrolling, dithering, wire occupancy and board skew.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **3 / 2 / 3**. Risk: more submissions without smoother photons.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze **zero skipped frames**, optical inter-frame p99 deviation **≤1 ms**, and unchanged traversal time within **2%** over 60 seconds. Plant one duplicated frame every second; the cadence gate must fail. A/B motion at the intended seat.

### D08-03 — Fractional transport instead of integer scroll jumps

- **Container; stage:** [FACT] **VP**. V8 scrolling in `render()`, **W:258–267**; v9 `renderV9()`, **W:1000–1018**.
- **What it controls:** [INFERENCE] Apparent continuity and velocity of outward travel.
- **Current setting, v8/v9:** [FACT] Both accumulate travel then shift stored samples by whole pixels. Mode-32 rate is **405 × motion** pixels/s; v8 limits shifts per invocation to eight. **W:258–267, 963–978, 1000–1018**.
- **Information preserved/destroyed:** [INFERENCE] Average velocity is represented, but subpixel position is not continuously displayed. At motion 1 the field jumps roughly four pixels per 100 Hz frame.
- **Origin:** [INFERENCE] Inherited implementation; budget versus aesthetic intent is undocumented.
- **Still needed on Titan?:** [INFERENCE] Outward transport **yes**; integer-only presentation **unknown**. Fractional intensity sampling costs additional operations; no M85 measurement exists.
- **Manipulation options:** [HYPOTHESIS] Compare integer transport against fractional resampling at **150/300/405/600 pixels/s**, preserving the same physical travel time.
- **Interactions:** [INFERENCE] Interpolate intensity/position carefully so colour is not averaged back into muddy RGB. Viewing distance and pixel pitch determine angular displacement. **L3; W:885–901**.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 3 / 3**. Risk: excessive blur or energy gain.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze travelled-centroid error **≤0.5 pixel**, integrated-intensity drift **≤1%**, and exact-black preservation over a synthetic travelling impulse. Plant nearest-neighbour rounding in the candidate; the subpixel-error gate must fail.

### D08-04 — Attack sharpness and peak expansion velocity

- **Container; stage:** [FACT] **VP**. V8 peak followers, **W:201–204**; v9 mode-32 position, **W:963–978**.
- **What it controls:** [INFERENCE] Whether an audio attack produces an immediate thrust, a soft swell or a late maximum. **L5–L6**.
- **Current setting, v8/v9:** [FACT] Mode 32 uses the **16 ms / 23 ms** cascade in both paths. V8 additionally uses a held peak and RGB smoothing. **W:201–204, 233–239, 963–978**.
- **Information preserved/destroyed:** [INFERENCE] The cascade suppresses rapid variation and spreads a step over approximately **66 ms from 10–90%** in the continuous model.
- **Origin:** [INFERENCE] Inherited/aesthetic smoothing; not demonstrated to be an S3 computational necessity.
- **Still needed on Titan?:** [INFERENCE] Some noise rejection **yes**; these attack constants **unknown**. Two scalar followers are already implemented; changing constants need not add substantial algorithmic work, but cycles were not measured.
- **Manipulation options:** [HYPOTHESIS] Attack pairs **4/6, 8/12, 16/23 ms**; independently retain longer release. Compare peak-velocity timing rather than only maximum extent.
- **Interactions:** [INFERENCE] Faster attacks expose AP noise and interact with width, clipping and brightness.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 3 / 2**. Risk: nervous motion or false impacts.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze step-response **t90 ≤35 ms** for a fast candidate, overshoot **≤2%**, and quiescent position jitter **≤0.5 pixel RMS**. Plant an extra **80 ms** follower; the attack gate must fail. Captain judges the common A/B.

### D08-05 — Separate musical persistence from silent-cut drainage

- **Container; stage:** [FACT] **VP**. V8 decay, **W:251–256**; v9 `fadeV9()`, **W:800–807**.
- **What it controls:** [INFERENCE] Trail memory, separation between notes and the visibility of silence.
- **Current setting, v8/v9:** [FACT] V8 decay depends on peak and quiet state. V9 uses time constants **290 ms for mode 32**, **125 ms for mode 18**, plus an additional **10/s** decay rate in silence. **W:251–256, 800–807**.
- **Information preserved/destroyed:** [INFERENCE, calculated] Normal v9 trails lose 95% in approximately **869/374 ms**; with the extra silent decay, approximately **223/166 ms**. Fixed-point truncation determines eventual exact zero.
- **Origin:** [FACT] Different behaviour is explicitly implemented in the v9 path. [INFERENCE] It is a perceptual design choice, not merely resolution compression.
- **Still needed on Titan?:** [INFERENCE] Silence-specific drainage **yes**; musical trail duration **taste-dependent**. Current arithmetic already provides the mechanism.
- **Manipulation options:** [HYPOTHESIS] Mode-32 decay **150–400 ms**, mode-18 **60–200 ms**; silent added rate **10–25/s**.
- **Interactions:** [INFERENCE] Motion speed determines spatial trail length; darker palette stops change apparent persistence.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 2 / 1**. Risk: blurred rhythm or abrupt disappearance.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze measured exponential time constant within **±5%**, and exact zero by **1 s** after admitted silence. Plant a nonzero floor or disabled silent decay; the corresponding gate must fail.

### D08-06 — Preserve native 16-bit precision through every attenuation

- **Container; stage:** [FACT] **Output**. `outputChannel()`, **W:37–39**; `scale16()`, **HD:28–31**; `packBoardGrb48Lane()`, **R:687–740**.
- **What it controls:** [INFERENCE] Dark gradients, trail endings and low-level colour ratios.
- **Current setting, v8/v9:** [FACT] Both waveform paths produce Pixel16; preview truncation does not feed their wire output. Master attenuation operates on 16-bit values before GRB48 packing. **R:273–298, 735–737; PACK:28–34**.
- **Information preserved/destroyed:** [INFERENCE] Under the stated 24/255 envelope, true16 allows **6,169 code levels**; an early 8-bit attenuation permits only **25**. Lifting an 8-bit source cannot recreate missing source precision.
- **Origin:** [FACT] The present path deliberately preserves wide history/output. **R:296–297**. [INFERENCE] Retaining it is an existing capability, not a new architecture.
- **Still needed on Titan?:** [INFERENCE] **Yes.** Current cost is already paid; no additional conversion is needed.
- **Manipulation options:** [HYPOTHESIS] Treat native16 as the reference; compare deliberate 8-bit quantisation only as a diagnostic control.
- **Interactions:** [INFERENCE] Gamma placement, board gains, white balance and palette channel values.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 2 / 1**. Risk: mistaking code capacity for visible precision.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze an exhaustive channel ramp at gain 1 and brightness 24: **6,169 unique codes**, monotonic output and exact zero. Plant `value >> 8` before attenuation; the level-count gate must fail. Then inspect a slow dark fade optically.

### D08-07 — Calibrated luminance transfer, not assumed gamma

- **Container; stage:** [FACT] **Map / output**. Level mapping in `renderV9()`, **W:934–960**; display treatment in `renderInhouse()`, **R:281–298**.
- **What it controls:** [INFERENCE] How audio magnitude and stored intensity become emitted and perceived brightness.
- **Current setting, v8/v9:** [FACT] V9 maps **−33 to −9 dBFS** into level and uses emphasis **1.5**. Both pass through a max-channel shoulder beginning at **0.75**, warmth and fixed white balance. **H:48–50; W:934–960; R:283–294**.
- **Information preserved/destroyed:** [INFERENCE] Values outside the level interval collapse at its endpoints; the shoulder compresses bright excursions. These operations are distinct from display gamma.
- **Origin:** [FACT] Header comments tie v9 gains to the frozen P6 comparison. **H:31–36**. [INFERENCE] A universal perceptual transfer is not established by Weber–Fechner or Stevens. **L13–L14**.
- **Still needed on Titan?:** [INFERENCE] Bounded mapping **yes**; exact curve **unknown**. A lookup-table alternative has unmeasured cost.
- **Manipulation options:** [HYPOTHESIS] Level emphasis **1.0/1.25/1.5/2.0**; separately fit a monotonic optical transfer with **f(0)=0**. Never apply an arbitrary exponent after the brightness cap.
- **Interactions:** [INFERENCE] Precision, saturation, extent and thermal/electrical envelope.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 3 / 3**. Risk: crushed shadows or accidental brightness inflation.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze a target curve before a held-out optical ramp; require monotonicity, zero intercept and **≤5% normalised RMS error** above the instrument noise floor. Plant cap-before-gamma ordering; the curve-error gate must fail.

### D08-08 — Dither only if an observed quantisation problem remains

- **Container; stage:** [FACT] **Output**. `dither16to8()`, **HD:34–42**; active wide packing, **R:735–737**.
- **What it controls:** [INFERENCE] Time-averaged fractional output at the possible cost of temporal sparkle.
- **Current setting, v8/v9:** [FACT] A four-phase 16→8 helper exists, but the inspected Inhouse GRB48 path does not call it. Both waveform paths already retain 16-bit channels. **HD:34–42; R:273–298, 735–737**.
- **Information preserved/destroyed:** [INFERENCE] Dither can trade instantaneous precision for average precision; a four-frame cycle at 100/120 Hz repeats at **25/30 Hz**, so invisibility cannot be assumed. **L2**.
- **Origin:** [INFERENCE] Existing helper/inheritance, not an established requirement for this path.
- **Still needed on Titan?:** [INFERENCE] **Unknown; no evidence currently justifies adding it.** Cost and LED internal PWM interaction are unmeasured.
- **Manipulation options:** [HYPOTHESIS] First compare no dither with an ideal high-precision reference. Only if a deficit is measured, test bounded **±1 output-code** error diffusion.
- **Interactions:** [INFERENCE] Frame cadence, low-duty PWM, eye movement, neighbouring-pixel correlation.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **2 / 3 / 3**. Risk: visible flicker introduced while repairing an invisible error.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze mean error **≤0.25 code** over 256 frames, exact-black output, and optical modulation attributable to dither below **1% of the tested patch mean**. Plant a coherent four-phase pattern; the modulation gate must reject it if detectable.

### D08-09 — Preserve palette structure at each pixel

- **Container; stage:** [FACT] **VP / map**. `sample()`, **W:41–70**; `outputV9()`, **W:872–915**.
- **What it controls:** [INFERENCE] Spatial colour articulation and whether simultaneous colours remain authored palette colours.
- **Current setting, v8/v9:** [FACT] V8 blends RGB contributions and smooths the resulting colour. V9 path P stores intensity and deposited offset, then samples HD colour per pixel. The current header selects P. **W:205–239, 885–901; H:40–47**.
- **Information preserved/destroyed:** [INFERENCE] V8 collapses multiple current contributions into a deposited RGB mixture; historical trail pixels can still differ. P retains spatially distinct samples. It does **not** reconstruct 80 independent spectral amplitudes merely because its coordinate has 80 positions.
- **Origin:** [FACT] Source comments identify K1 per-bin HD mapping; the lineage report identifies K1 commit `c2738b881d87ce53b04879332344ac272c4797d1`. **H:40–41; LINEAGE:39–50**.
- **Still needed on Titan?:** [INFERENCE] Per-pixel palette fidelity **yes** for this design. Exact incremental M85 cost unknown; existing offset storage is **640 bytes per renderer**. **H:237**.
- **Manipulation options:** [HYPOTHESIS] Preserve authored palettes; vary spatial palette coverage **0.5/0.75/1.0** and offset span without hue-wheel cycling.
- **Interactions:** [INFERENCE] Adjacent contrast, low-luminance discrimination and deposited-offset movement.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 4 / 2**. Risk: using a host-only switch that the product build omits.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze a ramp spanning an authored stop transition; require agreement with the HD reference within **2 Q16 codes/channel** and at least **16 distinct sampled colours**. Plant `kSlot16` and `kRgbMixPalette`; reference-error gates must fail. Repeat with product compile flags.

### D08-10 — Colour motion from spectral shape before compression

- **Container; stage:** [FACT] **AP / map**. Pre-knee shape capture, **A:345–356**; `applyShapeCentroidP()` and `updateOffsetP()`, **W:596–652**.
- **What it controls:** [INFERENCE] Whether colour continues responding when loud spectral values approach compression or clipping.
- **Current setting, v8/v9:** [FACT] V8 uses the earlier chroma/centroid route. Path P normalises pre-knee shape, smooths it, folds into 12 pitch classes and updates offset without the older v9 colour-speed cap. **W:154–196, 596–652**.
- **Information preserved/destroyed:** [INFERENCE] P preserves relative pre-knee shape while discarding absolute magnitude for this colour calculation. Folding **80 bins into 12 classes**, then a centroid, still discards octave and multimodal detail.
- **Origin:** [FACT] This is the source implementation of the loud-end colour-motion repair described in the brief. **A:348–355; W:596–652**.
- **Still needed on Titan?:** [INFERENCE] Avoiding post-knee colour stalls **yes**; centroid-only mapping **unknown**. The extra shape array is **320 bytes per renderer**. **H:258**.
- **Manipulation options:** [HYPOTHESIS] Shape smoothing **4–20 ms**; weak-evidence anchoring threshold **0.02–0.10**; compare one centroid against two dominant spectral regions.
- **Interactions:** [INFERENCE] Palette continuity, noise, register mapping and colour slew.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 4 / 2**. Risk: uncontrolled colour jitter when spectral evidence is weak.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze two different spectral shapes at three input gains. Require same-shape offset variation **≤0.01** across gain and between-shape separation **≥0.10**. Plant post-knee clipping before shape extraction; the separation gate must fail.

### D08-11 — Saturation, hue and apparent brightness require optical separation

- **Container; stage:** [FACT] **VP / output**. Saturation treatment, **W:912–915**; warmth/white balance, **R:283–294**.
- **What it controls:** [INFERENCE] Apparent brightness and hue as intensity and saturation change.
- **Current setting, v8/v9:** [FACT] Both apply saturation-related treatment; runtime output uses white factors **1, 0.9375, 0.84** and warmth-dependent channel factors. V9’s optional luma hold is disabled. **W:242–249, 912–915; R:284–294; H:62**.
- **Information preserved/destroyed:** [INFERENCE] Channel scaling changes chromaticity; equal computed luminance does not ensure equal apparent brightness because of Helmholtz–Kohlrausch effects. Luminance changes can also shift perceived hue. **L10–L11**.
- **Origin:** [INFERENCE] Inherited display treatment; calibration provenance for the attached optical assembly remains unknown.
- **Still needed on Titan?:** [INFERENCE] Assembly-specific colour calibration **yes**; present factors **unknown**. Current operations are already paid for.
- **Manipulation options:** [HYPOTHESIS] Saturation **0.7/0.85/1.0**, warmth **0/0.25/0.5**, calibrated channel ratios within the authorised envelope. Do not enable a silence luma floor.
- **Interactions:** [INFERENCE] Low-duty precision, palette contrast and perceived loudness.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 3 / 3**. Risk: “equalising brightness” erases intentional palette character.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze three authored colours at three nonzero intensities; measure chromaticity and repeatability. Require calibrated-reference chromaticity error **Δu′v′ ≤0.01** where measurement uncertainty permits. Plant swapped green/blue calibration; the gate must fail. Captain separately judges perceived balance.

### D08-12 — Palette separation at room luminance and across neighbours

- **Container; stage:** [FACT] **Map / output**. HD sampling and spatial coordinate, **W:885–901**.
- **What it controls:** [INFERENCE] Whether neighbouring colours remain recognisable in the room after diffusion and adaptation.
- **Current setting, v8/v9:** [FACT] V8 supplies blended deposit colours; P samples a coordinate progressing by **1/80 per pixel** across each half. **W:205–239, 886–890**.
- **Information preserved/destroyed:** [INFERENCE] Distinct digital colours can become optically or perceptually indistinguishable. Adaptation changes colour appearance, and neighbouring colours induce contrast effects. **L9, L12**.
- **Origin:** [INFERENCE] Authored palette structure plus inherited spatial mapping; no source establishes a room-specific discrimination threshold.
- **Still needed on Titan?:** [INFERENCE] Curated multicolour structure **yes**; every authored transition being visible at every level **unproved**. No new processor feature is necessary.
- **Manipulation options:** [HYPOTHESIS] Compare colour-region widths **2/4/8/16 pixels**, and existing palettes with different chromatic separations; retain black stops and avoid rainbow sequencing.
- **Interactions:** [INFERENCE] Viewing distance, plate diffusion, ambient illumination, saturation and spatial speed.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 3 / 2**. Risk: replacing nuanced colours with exaggerated contrast.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze the seat, ambient light and three brightness levels. In ten randomised engineering presentations, require **≥8/10 correct identification** of the intended two-region distinction. Plant identical colours in both regions; the distinction must fail. This is discrimination testing, separate from preference.

### D08-13 — Allocate loudness to extent, width and intensity independently

- **Container; stage:** [FACT] **Map / VP**. `depositV9()`, **W:812–870**; position and level, **W:934–999**.
- **What it controls:** [INFERENCE] Perceived magnitude through spatial size and brightness rather than one overloaded amplitude control. **L6, L15**.
- **Current setting, v8/v9:** [FACT] V8 mode 32 deposits near **80 + 56 × smoothed peak**. V9 width is **0.5 + 3.5 × level**, with level emphasis **1.5** and mode gains **0.395878 / 0.591745**. **W:268–278, 812–870; H:31–36, 48–50**.
- **Information preserved/destroyed:** [INFERENCE] V9’s triangle has approximately **3–10 pixels** of full support. Coupling width and amplitude makes a loud event larger and brighter simultaneously, obscuring which produces the impression.
- **Origin:** [FACT] Gains were selected by a frozen P6 matched-energy rule, according to source comments. **H:31–36**.
- **Still needed on Titan?:** [INFERENCE] A bounded magnitude mapping **yes**; present coupling **unknown**. Existing constant-peak/constant-energy variants already support separation. **H:21–25**.
- **Manipulation options:** [HYPOTHESIS] Width half-range **0.5–6 pixels**; amplitude exponent **1–2**; extent span **24–79 pixels**.
- **Interactions:** [INFERENCE] Saturation, clipping, decay, direction and event timing.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 4 / 2**. Risk: brightness wins an unfair A/B.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze two candidates with integrated optical energy matched within **5%**, but at least **2× peak extent change**. Plant a fixed-width candidate; the extent criterion must fail. Captain judges whether the larger release better matches the sound.

### D08-14 — Centre symmetry and inward release are different properties

- **Container; stage:** [FACT] **VP**. Mirroring, **W:287–295, 880–889**; `updateReleaseV9()`, **W:755–795**; release application, **W:963–999**.
- **What it controls:** [INFERENCE] Motion origin, bilateral coherence and edge-to-centre release.
- **Current setting, v8/v9:** [FACT] Both can mirror around indices **79/80**. V9’s optional extent-release mechanism pulls deposition inward, but is **disabled by default**. **H:63–76; W:963–999**.
- **Information preserved/destroyed:** [INFERENCE] Mirroring reproduces geometry but does not establish travel direction. Moving new deposition inward while older samples continue outward can create competing motion cues.
- **Origin:** [FACT] The release structure explicitly describes an inward pull based on a pre-AGC surge. **H:63–74**. [INFERENCE] Whether it matches Captain’s specified build/release remains unproved.
- **Still needed on Titan?:** [INFERENCE] Origin discipline **yes**; this specific release mechanism **unknown**. Scalar envelope processing already exists.
- **Manipulation options:** [HYPOTHESIS] Release envelope **150–600 ms**; inward span **20–79 pixels**; compare inward deposition versus an explicitly inward-travelling front.
- **Interactions:** [INFERENCE] Attack, persistence, extent and detector timing.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 5 / 3**. Risk: a symmetric field being mistaken for a correctly directed release.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze a synthetic release: bilateral position error **≤0.5 pixel**, inward front travel **≥60 pixels within 150 ms**, and correct signed velocity on **≥95%** of active front samples. Plant outward travel with perfect mirroring; direction must fail.

### D08-15 — Preserve pitch height separately from pitch class

- **Container; stage:** [FACT] **AP / map**. Chroma folding and centroid, **W:154–174, 596–625**.
- **What it controls:** [INFERENCE] Whether musical register can alter location or scale independently of harmonic colour.
- **Current setting, v8/v9:** [FACT] The inspected mappings fold spectral bins using **modulo 12** before centroid-based colour selection. P changes its spectral input but retains this reduction. **W:154–174, 596–625**.
- **Information preserved/destroyed:** [INFERENCE] Octave-separated energy can become identical pitch-class evidence. One centroid also merges distinct multi-peak distributions.
- **Origin:** [INFERENCE] Harmonic/aesthetic representation; not established as an unavoidable S3 resource compromise.
- **Still needed on Titan?:** [INFERENCE] Pitch class may remain useful; discarding all register information is **not technically necessary**, but product value is unknown. An additional weighted centroid over 80 bins has unmeasured M85 cost.
- **Manipulation options:** [HYPOTHESIS] Retain chroma for palette offset while a separate log-frequency centroid controls **0–20 pixels** of extent or a **0–25%** width adjustment. Use vertical position only if actual mounting supplies a meaningful vertical axis.
- **Interactions:** [INFERENCE] Pitch and loudness can covary; avoid a false pitch-to-rainbow rule. **L7–L8**.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **3 / 4 / 2**. Risk: movement reflects spectral brightness rather than intended musical register.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze octave-separated equal-level tones: chroma remains within **0.01**, register output differs by **≥0.2 normalised units**. Plant modulo-12 folding in the register path; separation must fail. Musical usefulness then requires A/B.

### D08-16 — Build-to-drop contrast and attack congruence

- **Container; stage:** [FACT] **AP / map / VP**. `updateAccentV9()`, **W:728–749**; `updateReleaseV9()`, **W:755–795**.
- **What it controls:** [INFERENCE] A release’s contrast against the preceding build, rather than simply its absolute brightness.
- **Current setting, v8/v9:** [FACT] V8 has no corresponding v9 accent mechanism. V9 accent defaults have **boost 0**, **widen 0**, **0.4 s fast / 4 s slow** density followers and ratio-full **3**. Release defaults are disabled, with **30 ms / 2 s** power followers and **3 dB** onset plus **6 dB** span. **H:51–76**.
- **Information preserved/destroyed:** [INFERENCE] Short/long comparisons retain contextual change but compress the preceding history into a few state values. The accent uses the lowest 12 spectral bins, not an unrestricted sub-bass detector. **W:728–749**.
- **Origin:** [FACT] Source ties these controls to frozen P6 design candidates. **H:51, 63**. [INFERENCE] They are implemented options, not demonstrated solutions.
- **Still needed on Titan?:** [INFERENCE] Context sensitivity is a credible opportunity; these detectors remain **unknown**.
- **Manipulation options:** [HYPOTHESIS] First isolate VP using manually annotated host events. Compare contraction over **2–8 s**, release over **30–150 ms**, widening **0–4 pixels** and bounded boost **0–0.5**.
- **Interactions:** [INFERENCE] Detector lateness can invalidate a good VP design; preserve true black and the output envelope.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **5 / 5 / 3**. Risk: tuning visual impact around wrongly timed detector events.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze annotated events and require release peak velocity within **±30 ms**, with peak expansion velocity **≥3×** the preceding two-second median contraction speed. Plant a **100 ms delayed trigger**; timing must fail. Captain judges congruence using the common A/B.

### D08-17 — Two-board coherence and intentional hierarchy

- **Container; stage:** [FACT] **System / output**. `packBoardGrb48Lane()`, **R:687–740**; primary and secondary submission, **FIX:669–676, 695–702**.
- **What it controls:** [INFERENCE] Whether the two 160-pixel scenes read as one instrument, a deliberate call-and-response, or accidental disagreement.
- **Current setting, v8/v9:** [FACT] Both use board-specific packing and separate submissions. Each scene is split into **80-pixel / 480-byte** lanes; the upper-board mapping has its own orientation. **R:687–740; FIX:639–645, 669–676, 695–702**.
- **Information preserved/destroyed:** [INFERENCE] Correct logical frames do not establish simultaneous optical latching. Board timing differences can alter the apparent direction or hierarchy of one event.
- **Origin:** [INFERENCE] Hardware/output architecture. Whether any inter-board skew is intentional is unknown.
- **Still needed on Titan?:** [INFERENCE] Correct board identity and orientation **yes**; strict synchrony versus deliberate offset is a product choice. Cost of tighter scheduling is unmeasured.
- **Manipulation options:** [HYPOTHESIS] Establish a synchronous baseline; then compare deliberate offsets **0/10/20 ms** or different extent responses while retaining coherent musical timing.
- **Interactions:** [INFERENCE] AV timing, motion direction, room placement and brightness balance.
- **Visual impact / novelty / effort; risk:** [HYPOTHESIS] **4 / 4 / 3**. Risk: accidental skew being rationalised as creative behaviour.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze matching board landmarks with optical skew **≤2 ms**, identical generation identity and correct centre orientation. Plant a one-frame delay on one board; skew must fail. Only after this baseline should Captain compare deliberate hierarchy.

---

## Contradictions and unresolved questions

| Issue | Finding and required resolution |
|---|---|
| **24 versus 100 brightness** | **[FACT]** The brief specifies 24/255; current source permits 255 and defaults to 100. **RH:18–23; R:226–227; SWARM-BRIEF.md:32**. **[INFERENCE]** The actual authorised composite envelope—including board gains—must be frozen before physical comparison. No change was made here. |
| **120 Hz target versus 100 Hz implementation** | **[FACT]** Emotiscope selects 100 Hz. **RH:39–47**. **[INFERENCE]** A target is not evidence of the resident cadence. |
| **AV asymmetry** | **[FACT]** BT.1359’s sign convention contradicts “light may lag more than lead.” **L1**. **[INFERENCE]** Titan needs music-specific timing judgement rather than a copied broadcast tolerance. |
| **Latency double counting** | **[FACT]** The approximately 16.4 ms stack already contains wire time and a modelled scheduling wait. It excludes some physical delays. **BUDGET:13–17, 28–34, 87–95**. |
| **Path P default** | **[FACT]** PL14 documents a host/product default mismatch; the final inspected header now selects P. **PL14:148–152; H:40–47**. **[INFERENCE]** Current source selection does not establish the installed image. |
| **“16 slots” versus one mixed deposit colour** | **[FACT]** Authored-stop sampling, fallback slot scanning, RGB mixing and per-pixel sampling are distinct operations. **W:41–70, 205–239, 885–901; LINEAGE:84–95**. **[INFERENCE]** “16 slots” does not mean only 16 possible interpolated colours. |
| **Earlier colour forensics** | **[FACT]** The initial report and later lineage investigation reach different descriptions of the inherited colour fault. **FORENSICS:22–29; LINEAGE:3–27**. **[INFERENCE]** Use the later source-specific distinction rather than treating the initial report as settled authority. |
| **Digital brightness versus luminance** | **[FACT]** Source values specify arithmetic and channel codes, not room luminance. **R:281–298, 735–737**. **[INFERENCE]** Driver response, optics, ambient light and adaptation remain required measurements. |
| **Gamma** | **[FACT]** Musical emphasis 1.5, the output shoulder and a hypothetical gamma 2.2 are different transformations. **H:48–50; R:291–294**. **[INFERENCE]** Adding gamma without calibration could destroy the low-level resolution being sought. |
| **Symmetry versus direction** | **[FACT]** Mirroring and transport are separate code paths. **W:260–267, 880–889, 1000–1018**. **[INFERENCE]** A symmetric image can still move in the wrong direction. |
| **Actual spatial scale** | **[INFERENCE]** Pixel pitch, emitting area, plate diffusion, mounting orientation and normal viewing distance are needed to translate pixels/frame into angular motion and adjacent-colour discrimination. **L3, L9, L12**. |
| **Internal PWM and low-duty behaviour** | **[INFERENCE]** GRB48 packing proves transmitted code width, not effective optical precision or flicker performance. Those require the attached strip’s optical response. **PACK:28–34; L2**. |
| **Taste and impact** | **[FACT]** The brief reports v9 as overall superior while the drop still lacks the requested impact. **SWARM-BRIEF.md:33–39**. **[INFERENCE]** Neither more colours nor lower latency alone proves the requested build-to-release experience. |

## Recommended first comparisons

[HYPOTHESIS] Prioritise these bounded comparisons:

1. **D08-06 + D08-09:** prove that the actual product build preserves native16 and selects per-pixel HD mapping. These protect existing capability.
2. **D08-04:** compare faster positional attack at matched energy and unchanged transport. Its timing scale is substantially larger than the 100→120 Hz scheduling gain.
3. **D08-14 + D08-16:** use annotated host events to isolate the requested contraction/release behaviour before judging automatic drop detection.
4. **D08-11 + D08-12:** evaluate the surviving candidates at the actual room luminance, seat and optical assembly.

[INFERENCE] These comparisons reuse existing wide pixels, separated intensity/colour state, release controls and timing instrumentation. No NPU, new registry, new front-panel control set or additional orchestration layer is required to establish their value. **H:21–29, 53–76; W:812–915; LIVE:443–480**.

## Verification and handback

- **[FACT] Files changed:** none.
- **[FACT] Commands/checks:** source and evidence reads, targeted searches, diff/identity inspection, SHA-256 calculation, literature retrieval and deterministic arithmetic verification.
- **[FACT] Arithmetic:** completed successfully through `workflow_verify`; receipt `7151d65a0b4a7f343542f3ebab942792d834412771101b6370802a372a9c9599.json`.
- **[FACT] Firmware/physical validation:** not performed. No proposed perceptual experiment has been run or frozen.
- **[FACT] Visual inspection:** images are not applicable to this source/literature report. Source/diff and calculation outputs were inspected; no visual candidate or optical qualification is claimed.
- **[FACT] Governance:** admission was unavailable because this assignment had no bound task ledger. The final gate allowed normal handback but explicitly returned **“not a completion certificate.”**
- **[HYPOTHESIS] Next execution path:** select one comparison → freeze its exact criterion and input → demonstrate the planted fault fails → verify the product build’s actual path → conduct the bounded optical/on-device A/B → retain Captain’s decision separately from numerical and hardware evidence.
---

## Opus verification (15 Oct 2026, after the Astra run)

**Lane:** GPT Astra (`gpt-6-astra`, high reasoning, Codex CLI, read-only), exit code 0. Verified by Opus 5.5.

### Spot-check results: 18 of 18 [FACT] citations match the source

All of these were re-read at the same file hashes Astra recorded (`inhouse_waveform.h` 88cea47b…, `.cpp` 0be2360e…):

- W:201–204 (16/23 ms cascade)
- W:800–807 (290/125 ms fade, +10/s in silence)
- W:37–39 (`outputChannel`)
- HD:28–31 (`scale16`)
- R:735–737 (`scale16` then `attenuate` into GRB48)
- RH:39–47 (10,000 µs Emotiscope / 8,333 µs other)
- R:226–227 (`setHardwareBrightness` ≤255)
- RH:18–23 (boot default 100, `static_assert` ≥100)
- R:356–361 (skipped-frame logic)
- H:237 / H:258 (offset 160 floats, shape 80 floats)
- W:885–901 (per-pixel HD sample)
- W:963–978 (mode-32 transport, 405 px/s, position ceiling)
- BUDGET:29 (render 0.197 ms)
- R:296–297 (Pixel8 is preview only)
- A:345–356 (pre-knee `linear_shape`)
- LINEAGE:39 (K1 `c2738b881d87…`)
- PL14:148–152 (B14)

**Arithmetic re-run independently:**
- Cascade 10/50/90 % at 10.23 / 32.50 / 76.22 ms.
- 6,169 levels at 24/255.
- Gamma-2.2 zero thresholds 23.58 % and 1.893 %.
- 95 % decay at 869 / 374 ms, and 223 / 166 ms in silence.

**Literature:** the bibliographic details of L1–L15 match my knowledge of those works.

The BT.1359-1 sign convention is correct: detectability runs from +45 ms (sound early, so light late) to −125 ms (sound late, so light early). That correction overturns the premise in the dispatch prompt, which said light could lag more than it leads.

### Concurrent-edit warning
- [FACT] `inhouse_waveform.h` was modified at 01:19:45 on 15 Oct, during this run, by another agent. It is uncommitted relative to HEAD 1ef5395: 7 lines added, 1 removed.
- [FACT] Before that edit, line 46 read `kInhouseV9Palette = InhouseV9Palette::kRestrictedRange`. It now reads `kPerPixelMapped` (H:47).
- Line numbers in other swarm reports that cite this header may therefore be off by one.

### Additions Astra missed

1. **[FACT] Brightness authority is a four-way conflict, not two-way.**
   - The global Titan mandate in `~/.claude/CLAUDE.md` says "Preserve the current maximum brightness 24/255".
   - `Kx-Titan/AGENTS.md:12` says "default brightness100/255 supersedes historical24".
   - `palette_runtime.h:23` has `static_assert(kDefaultPaletteBrightness >= 100U …)`.
   - `scripts/build_scalar.py:933` raises if the boot brightness is below 100.
   - A 24/255 owner cap can therefore exist only as a runtime `setHardwareBrightness(24)`. It cannot be a build default.
   - This is reported, not resolved.
2. **[FACT] Extent ceiling (new lever D08-18).**
   - Mode-32 deposit position is `min(159, 80 + peak_stage2 × 0.7 × 80)` (W:976). The fresh deposit can therefore reach at most 56 of the 80 pixels in a half, which is 70 % of the half-length. Only trails that scroll outward reach the outer 24 pixels.
   - The width bound is 0.5 + 3.5 × level (W:826). It adds at most about 5 pixels of reach.
   - [INFERENCE] For the Captain's "full extension to the edges" release, the 0.7 factor is a hard spatial cap on how far the release can reach. It sits beside the 16/23 ms cascade, which caps how fast it gets there.
