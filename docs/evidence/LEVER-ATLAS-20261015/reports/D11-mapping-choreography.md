# D11 — Mapping and choreography

**[FACT] Scope:** read-only research. No files changed, devices accessed, firmware built, tests executed or images installed. Preserve continuous autonomous music visualisation, the assignment’s **24/255 cap**, true black and no rainbow defaults. Source: `_scratch/waveform-migration-001/runs/LEVER-SWARM-20261015/SWARM-BRIEF.md:3–5,26–38`.

**[INFERENCE] Main finding:** Titan needs separate mappings for **immediate musical response, ordinary motion, tension/release geometry and section memory**. Increasing brightness, extending a trail or accelerating the existing scroll cannot independently express the Captain’s requested contraction followed by explosive **edges-to-centre** release. The current position mapping and outward transport are different mechanisms from that choreography. Sources: `src/k1/core/visual/inhouse_waveform.cpp:963–1023`; swarm brief:26.

**[HYPOTHESIS] Recommended direction:** retain v9’s intensity/colour separation, preserve a fast response that works without tempo or drop classification, and add bounded spatial choreography above it. Use prediction only after independent timing qualification; an uncertain prediction must never spend the full drop gesture.

## Evidence identity and reading conventions

**[FACT] Titan HEAD inspected:** `1ef53952ab7a50af38414321c8a91e515744098a`, with existing uncommitted changes in the renderer, audio producer and build script. These findings describe the inspected working source, **not a verified resident image**.

**[FACT] Source abbreviations below expand to these exact paths:**

| Abbreviation | Path |
|---|---|
| `W` | `src/k1/core/visual/inhouse_waveform.cpp` |
| `WH` | `src/k1/core/visual/inhouse_waveform.h` |
| `A` | `platform/ra8p1/inhouse_audio.cpp` |
| `AH` | `platform/ra8p1/inhouse_audio.h` |
| `PR` | `platform/ra8p1/palette_runtime.cpp` |
| `LR` | `platform/ra8p1/k1_live_runtime.cpp` |
| `SB` | `src/k1/core/audio/sub_band.h` |
| `TT` | `src/k1/core/audio/tempo_tracker.cpp` |
| `RUN` | `_scratch/waveform-migration-001/runs` |
| `PL14` | `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-PRISM/Kx-Titan/docs/migration/TITAN-PRODUCT-LANE-LESSONS-2026-10-14.md` |
| `S3` | `/Users/spectrasynq/SpectraSynq_K1_Firmware/SPECTRASYNQ_K1_FIRMWARE` |
| `LW` | `/Users/spectrasynq/Workspace_Management/Software/Lightwave-Ledstrip/firmware-v3` |
| `EMO` | `/Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL` |

**[FACT] Inspected working-file SHA-256 identities:**

```text
WH  2e0013b84d627db17945681d8764aa869417a6e45af9d75c0bf881e5e0ed1e94
W   0be2360eb0326cdf53c60742ba749b796a48ee1d7d325291d2e45f03aece82ae
A   98f0b748020992483b38caace1a65184aa8b102f81535175deeff4f5c7167d74
PR  29da26329fabcaa230629a378594c946977f2c058dd69b80bb3dc4035471ff8d
LR  90696e797c80f2514d33d301f46c003bdc0a4b65173612556caccb945dce3f3a
```

**[FACT] “v9” requires qualification:** profile 9 enables `K1_INHOUSE_MAPPING_V9`; it does not itself select path P. The inspected header still selects `kRestrictedRange`, despite an adjacent path-P comment. Accent boost and width boost are zero; extent release is disabled. Sources: `scripts/live_boot_composition.py:12–17`; `WH:34–76`; `W:585–590`.

**[FACT] Status terminology:** implementation findings below are source observations; historical measurements are explicitly attributed to their reports; all proposed functions, thresholds and ratings are hypotheses awaiting tests. No proposed number in this report is an already-frozen acceptance criterion.

## 1. Inventory of current mappings

### AP information arriving at the mapping boundary

| Mapping/function | Current behaviour and time constants | Information preserved or destroyed |
|---|---|---|
| **[FACT] `InhouseAudio::analyse` → RMS/VU** | Raw hop RMS; optional VU floor; `vu_average` averages current and previous VU. At 24 kHz, 180 samples = 7.5 ms. `A:228–246`; `AH:20–27`. | **[INFERENCE]** Preserves absolute hop energy separately from AGC. RMS discards waveform sign and within-hop timing; two-hop VU averaging adds another temporal reduction. |
| **[FACT] Peak drive → `peak_scaled`** | Subtract 350 PCM units by default. Adaptive follower uses rational coefficients derived from reference α=.25 rising/.005 falling; ratio follower uses .65/.15. `A:23–26,247–254`; `AH:33–40`. | **[INFERENCE]** Preserves relative attacks; destroys absolute-level interpretation. Equivalent rational time parameters are 30/1990 ms and 5.38/56.67 ms, respectively—not exponential time constants. |
| **[FACT] GDFT → conditioned spectrum** | Magnitude average `.3 new + .7 old`; Mood low-pass cutoff `1+10·mood`, calculated against 100 Hz but advanced every 7.5 ms; AGC; frequency tilt; knee above .5; clamp to 1. `A:97–111,268–278,313–367`. | **[INFERENCE]** At default Mood=.05, the first two smoothing stages have approximately 21.03 ms and 79.58 ms exponential time constants. The final clamp makes all pre-knee values ≥1.5 indistinguishable. These are not merely storage reductions. |
| **[FACT] Spectrum → novelty/energy** | Novelty is square root of mean positive bin difference, after AGC/knee/clamp. Spectral energy is the mean of 80 conditioned bins. `A:341–367`. | **[INFERENCE]** Reduces 80 changes to one scalar and loses their frequency identities. Saturated bins cannot report further positive change. |
| **[FACT] Pre-knee shape branch** | `linear_shape[80] = magnitude_last × tilt`, before gain, knee and unity clamp. `A:346–350,382–386`. | **[INFERENCE]** Preserves relative spectral structure lost by the knee, but retains upstream magnitude/Mood smoothing. “Pre-knee” does not mean raw or fast. |
| **[FACT] Sub-band branch** | v9 computes hop RMS through two cascaded band-pass sections, nominally 40–110 Hz. `SB:3–8,17–55`; `A:239–240`. | **[INFERENCE]** Supplies evidence below the note bank’s 110 Hz start; reduces that band to energy and adds filter response delay. It does not identify a drop. |
| **[FACT] Producer → renderer acquisition** | Latest feature publication plus maximum `peak_scaled` across unread generations; eight-generation bounded peak history. `AH:15–17`; `A:377–429`. | **[INFERENCE]** Approximately 60 ms of peak coverage preserves a missed peak amplitude, but not its exact occurrence time, count, novelty or corresponding spectrum. |

### Shared preparation and v9 mode 32/18 mappings

| Feature → visual parameter | Implemented function, setting and timing | Information loss / consequence |
|---|---|---|
| **[FACT] Publication → freshness** | Requires coherent ordering, valid sample interval and capture age ≤30 ms. Render `dt` uses integer milliseconds, clamped 1–50 ms. `W:105–141`. | **[INFERENCE]** Regressive/stale audio cannot sustain births. Equal generations remain fresh, so “fresh” is not equivalent to “new event.” Millisecond quantisation discards sub-ms scheduling precision. |
| **[FACT] Spectrum → chroma** | Spectrum mix `1−.25^(100dt)`; fold selected range, default 60 bins, into 12 classes. Adaptive chroma maximum uses .999 decay and .05 rise at 100 Hz. Flatness gate opens between .08 and .28. `W:144–175`; `WH:115–125`. | **[INFERENCE]** Spectrum smoother τ≈7.21 ms; chroma decay τ≈9.995 s. Folding removes octave identity; circular centroid further reduces 12 strengths to one angle and concentration. |
| **[FACT] VU → level** | `l=clip((20log10(vu)+33)/24)`: −33 dBFS→0, −9 dBFS→1. `W:940–950`; `WH:48–50`. | **[INFERENCE]** Values below the floor and above the ceiling lose distinguishability. Runtime Response has already multiplied and clipped VU, so this is not always an untouched acoustic dBFS measurement. `PR:265–272`. |
| **[FACT] Level → deposit amplitude/width** | `amplitude=g·l^1.5`; gains .395878/.591745 for modes 32/18. Width=.5+3.5l; actual triangular support radius is width+1, hence 1.5–5 px. `WH:34–50`; `W:812–845`. | **[INFERENCE]** Both brightness and occupied width increase with level. Integrated deposited light therefore changes through two coupled controls. |
| **[FACT] Peak → mode-32 position** | Two EMAs, 16 then 23 ms; `p=80+56·peak₂`. `W:970–977`. | **[INFERENCE]** With clamped peak, centre of deposition spans 80–136, not the whole half-strip. Louder audio moves the root towards the edge. The isolated two-EMA cascade reaches only 27.8% of a step at 20 ms and 91.4% at 80 ms. |
| **[FACT] Peak → mode-18 position** | Rounded `80+80·peak`, capped at 159; no added 16/23 ms pair. `W:995–998`. | **[INFERENCE]** Removes fractional position and retains upstream peak compression. |
| **[FACT] Speed → mode-32 transport** | Outward integer shifts at `405·motion_scale` px/s, maximum eight steps/frame. Runtime maps `motion_scale=2·speed`. `W:963–1008`; `PR:250`. | **[INFERENCE]** This is outward transport of history, not an inward drop front. At 100 Hz the step cap corresponds to 800 px/s delivered per ordinary frame. |
| **[FACT] Tempo/onset → mode-18 transport** | `w=.7·smoothstep(clip((confidence−.30)/.30))`; tempo rate `24·BPM/60·(1+.5cos(2πphase))`; fallback `30+90·clip(novelty/.12)` px/s while audible. `W:980–993`. | **[INFERENCE]** At least 30% remains on the energy path, but wrong tempo still modulates motion. Beat phase is not bar/downbeat identity. |
| **[FACT] Time → trail decay** | τ=290 ms in mode 32; 125 ms in mode 18. Confirmed silence adds 10 s⁻¹ to decay. `W:798–806`. | **[INFERENCE]** Quiet effective τ≈74.4/55.6 ms. Fixed release separates decay from loudness, but long tails can obscure closely spaced attacks. |
| **[FACT] New deposit → history ownership** | Replace intensity, colour and offset only when new intensity exceeds decayed stored intensity. `W:844–868`. | **[INFERENCE]** Avoids RGB summation and preserves the winning deposit’s attributes; weaker later events at the same pixel are discarded. |
| **[FACT] Harmony → default v9 colour** | Confidence multiplies tonality gate, centroid strength/.08 and level/.5. Follow 600 ms, velocity momentum 250 ms, anchor 15 s, fallback drift 8 s; caps .25/.03 palette units/s. `W:655–722`. | **[INFERENCE]** Several independent reasons can freeze colour. A fast harmonic change is intentionally suppressed even when the sampler itself has full precision. |
| **[FACT] Default palette transform** | Restricted-range triangle fold for IDs 31/38: `lo+(hi−lo)(1−|2c−1|)`. `W:574–580`. | **[INFERENCE]** Coordinates `c` and `1−c` become identical: a two-to-one mapping. Excluding dark stops also changes the authored palette’s dynamic structure. |
| **[FACT] Path P colour** | Max-normalised pre-knee shape → chroma centroid. Weak strength blends from held anchor by `strength/.08`; no downstream speed cap. Per-pixel output `HD(wrap(deposited_offset+d))`, `d=distance/80`. `W:593–653,872–901`. | **[INFERENCE]** Preserves spatial palette variation and avoids RGB-history mixing. Still collapses harmony to one offset; position on the strip is not an independently measured frequency-bin magnitude. |
| **[FACT] Saturation/mirror → output** | Saturation moves channels towards their maximum; mirror selects the opposite history index. `W:878–915`. | **[INFERENCE]** Saturation below 1 changes authored palette chromaticity. Mirror gives geometric equality, not proof of motion direction. |
| **[FACT] Low-band flux → optional accent** | Positive changes in bins 0–11; 400 ms fast and 4 s slow density; ratio 3 gives full accent. Default amplitude/width boosts=0. `W:725–749`; `WH:53–61`. | **[INFERENCE]** Disabled by default; if enabled, 400 ms density is a build/activity descriptor, not a ≤50 ms drop trigger. |
| **[FACT] Power surge → optional extent release** | Fast/slow power 30/2000 ms; +3 dB onset over baseline, 6 dB span; 600 ms decay. Optional sub input, decaying-new-peak and 6 s dip gates. Default disabled. `W:752–795`; `WH:65–76`. | **[INFERENCE]** Pulls the deposit towards centre; it does not reverse the outward-moving history or encode build/armed/release states. |

**[FACT] Output controls remain relevant:** `renderInhouse` applies `.3+.7·brightness`, a whole-pixel shoulder above .75, warmth and white balance after rendering; master/edge scaling follows elsewhere. Zero input remains zero through these multiplicative operations. `PR:281–294`.

**[FACT] Modes 8/7/11 retain the v8 path even in a v9 build.** Their followers use .08/.05 reference coefficients; mode 8 transports at 100 px/s and scales displacement by `.7/sensitivity`; modes 7/11 use `120·motion_scale`; mode 11 builds a 3–10 px radius from four PCM-history rows. They retain colour blends and distinct fade rules. `W:185–199,408–525`.

### What changed from v8

**[FACT] Mode-32 v8 combines two palette colours in RGB, smooths RGB for 80 ms, adds deposits and uses level-dependent decay `0.8+3.5·peak`, with quiet acceleration. v9 replaces that with bounded level emphasis, max-intensity ownership and constant decay. Sources: `W:201–285,798–868`.

**[INFERENCE] Those v9 changes address brightness ordering and colour contamination, but they cannot establish the requested tension/release language by themselves. A higher drop/build light ratio measures a different consequence from inward travel distance, arrival time and thrust. Sources: `docs/evidence/P6/RECEIPT.md:46–62`; swarm brief:26.

## 2. First-principles mapping design

### Container contract

**[HYPOTHESIS] Use four concurrent timescales inside the existing renderer/producer architecture:**

1. **Hop:** preserve attacks, level and spectral changes.
2. **Beat:** shape recurring motion when timing is credible.
3. **Build/bar-scale:** organise contraction and release.
4. **Section:** retain identity and context without slowing attacks.

**[INFERENCE] This requires additional state and clear consumers, not a new framework, NPU or second processor. The existing publication already carries RMS, peak, spectrum, pre-knee shape, novelty, sub RMS, timestamps and tempo fields. Sources: `WH:79–113`; `A:377–398`.

**[HYPOTHESIS] Update event logic once per `(epoch,generation)` at 7.5 ms chronology; interpolate visual state at render time. Carry transient maxima **with timestamps**, not only a maximum scalar. Keep `capture_us`, feature availability and render time distinct. Existing insertion points: `A:200–214,309,377–429`; `W:120–133`.

### Common functions and proposed units

**[HYPOTHESIS] Define `C(x)=min(1,max(0,x))` and an asymmetric follower:**

\[
F(y,x,\Delta t)=y+(x-y)(1-e^{-\Delta t/\tau}),
\quad
\tau=\begin{cases}\tau_a&x>y\\\tau_r&x\le y.\end{cases}
\]

**[HYPOTHESIS] A hold duration is explicit:** after a detected attack, keep its peak for \(h\) ms before release. A release constant is not a hold. Existing `follow()` is suitable for the two slopes but needs explicit event/hold state. Source seam: `W:31–35`.

**[HYPOTHESIS] Initial candidate features, calculated only on valid new audio:**

- \(L=C((20\log_{10}(\max(raw\_rms,10^{-9}))+45)/36)\). Initial range **−45 to −9 dBFS**, with the existing silence/admission veto retained.
- \(P_f=F(P_f,raw\_rms^2;8,120\text{ ms})\); \(P_s=EMA(raw\_rms^2;2000\text{ ms})\).
- \(J=C((10\log_{10}((P_f+\epsilon)/(P_{s,\mathrm{previous}}+\epsilon))-3)/9)\): transient contrast, not a drop probability.
- \(N=C(novelty/.12)\), retained as a secondary onset cue because its source is already compressed.
- \(H_{12}\): normalised pre-knee chroma vector; preserve the whole vector alongside its centroid.
- \(T\): tension state defined in §3; it never directly authorises extra light during silence.

**[HYPOTHESIS] The lower proposed level floor is a test candidate, not a recommendation to amplify room noise. Reject it if existing silence/weak-attack tests regress. Source baseline: `W:948–949`; admission: `PR:254–264`.

### Parameter mapping table

**[HYPOTHESIS] The following values are starting candidates to freeze before comparison, not tuned results.**

| Visual parameter | Driver and timescale | Transfer function / initial parameters | Attack / release / hold |
|---|---|---|---|
| **Extent** | Level plus transient during ordinary music; tension during build; explicit release event | Ordinary inward reach \(e=C(.35+.40L+.15J)\). Build/release overrides use §3. Extent means fraction of the **edge-to-centre half-distance**, not brightness or number of lit pixels. | Ordinary 20/180/30 ms; build 2–8 s; release 100–180 ms analytic travel |
| **Position** | Extent and event identity | Right front \(p_R=159-79e\); left front \(p_L=79e\). Ordinary reference mode may retain its bouncing root; drop gesture must use these inward coordinates. | Follow extent; no extra position EMA on release |
| **Velocity** | Fast onset, reliable beat phase, tension | Normal outward wake \(v=(1-q)(30+90N)+q[24b(1+.5\cos 2\pi\phi)]\), \(b=BPM/60\); initially \(q\le.35\). During build multiply by \(1-.65T\). Release velocity is \(dp/dt\), not this scroll rate. | 10/120/20 ms for activity; predictive contribution separately gated |
| **Brightness** | Absolute RMS and short attack contrast | New-deposit amplitude \(a=gL^{1.5}(1+.25J)\), capped at 1; retain existing mode gain initially. Keep final hardware cap ≤24/255. | 8/120/15 ms; silence overrides hold and forbids birth |
| **Colour position** | Pre-knee harmonic shape, per hop; section anchor only for ambiguity | Preserve path-P live offset. For strength \(s<.08\), \(o=\mathrm{wrap}(o_{\rm anchor}+C(s/.08)\Delta(c,o_{\rm anchor}))\). No multiplication by post-gate tonality or RMS. | No additional filter initially; any later filter must have τ≤20 ms and pass colour-motion tests |
| **Colour spread** | Pre-knee spectral diversity and section energy, 250 ms scale | \(p_i=\mathrm{wrap}(o_{\rm deposit}+w\,d_i)\). Candidate \(w=C(.75+.25E_{\rm section})\); baseline \(w=1\). Apply spread to new material, preserving old deposited attributes. | 100/500/250 ms; compare against fixed \(w=1\) |
| **Saturation** | Authored palette; no mandatory audio driver | Default \(s=1\). If a separately approved desaturation control is studied, restrict candidate to .9–1 and label it a palette transformation. | Default no envelope; optional 100/300/0 ms |
| **Trail decay** | Beat separation and scene role, not instantaneous loudness | Start with v9 τ=290/125 ms. Candidate beat-aware \(\tau=\mathrm{clip}(.35T_b,90,220)\) ms only when beat period reliable; otherwise retain fixed τ. | Change τ over 100 ms; silence adds existing 10 s⁻¹ drain |
| **Symmetry** | Explicit scene/control choice and release geometry | Mirror the inward front exactly about 79.5. Do not infer stereo placement from this mono feature publication. Keep boards’ histories independent. | No automatic per-hop switching; changes at a safe scene boundary |
| **Micro-detail** | Per-hop peak/novelty residual | Separate transient layer with ≤20 ms hold and 60–90 ms decay; it may alter width by ≤1 px without moving the section anchor. | 0–8/60–90/20 ms |

**[INFERENCE] This preserves the useful division in v9—intensity and palette coordinate as separate attributes—while removing the assumption that every visual parameter should consume the same normalised peak or the same smoothed scalar. Sources: `W:864–901`; `A:243–254,346–367`.

**[HYPOTHESIS] Compare three alternatives:** retain v9 with corrected path-P selection; add only fast transient/geometry separation; then add the full tension state machine. The second is the smallest useful candidate because it can improve response even when semantic drop detection remains wrong.

## 3. Tension-and-release state machine

### What “hits harder” means geometrically

**[FACT] The requested build leaves the waveform compressed towards the left/right edges; the release extends from those edges towards the centre. Source: swarm brief:26.

**[INFERENCE] Define the state in edge-to-centre coordinates to eliminate ambiguous language:**

- \(e=0\): compressed at physical/logical edges.
- \(e=1\): reaches indices 79/80.
- Build: \(de/dt<0\).
- Release: \(de/dt>0\).

**[INFERENCE] Merely moving the deposit inward while transporting its old history outward can increase visible span without producing an inward-moving front. Those outcomes need separate measurements. Source: `W:976–977,998,1004–1008`.

### Available evidence and missing evidence

**[FACT] Available now in v9 source:** raw RMS, normalised peak, pre-knee spectral shape, post-knee novelty, sub RMS, tempo phase/confidence and chronology. No bar number, downbeat confidence or section label appears in `InhouseWaveformAudio`. Source: `WH:79–113`.

**[FACT] The product tracker has an 88 BPM prior and 512-update confidence warm-up. Its P7 report records half-time failures and other false-lock behaviours. Sources: `TT:17–39,500–519`; `docs/evidence/P7/RECEIPT.md:19–43`.

**[FACT] The detector series did not pass its frozen corpus:** v1 chosen detector hit 1/5 tuning drops and missed the holdout; v2 hit 1/6; v3 hit 0/6. Sources: `RUN/P6-DROP-DETECTOR/DETECTOR.md:54–69`; `v2/DETECTOR.md:58–70`; `v3/DETECTOR.md:83–93`.

**[INFERENCE] Therefore full predictive release should initially be disabled. A confidence field produced by the failing tracker is insufficient evidence to enable it.**

### Proposed evidence variables

**[HYPOTHESIS] Build evidence should combine complementary, imperfect cues rather than call any one “tension”:**

- \(U\): upper-band energy proxy from squared pre-knee magnitudes for bins ≥220 Hz.
- \(D\): onset density, counting threshold crossings of \(N>.65\), with 90 ms event refractory.
- \(r_U=C((dB(EMA_{200ms}U)-dB(EMA_{2s}U)-1)/5)\).
- \(r_D=C((EMA_{400ms}D-EMA_{4s}D)/3)\), where density is events/s.
- \(B=.6r_U+.4r_D\), followed with 1000 ms attack and 2000 ms release.
- Sub-gap: \(dB(EMA_{30ms}(sub^2))\) ≥8 dB below its recent peak for ≥300 ms; recent peak decays at 1 dB/s and expires after 8 s.

**[HYPOTHESIS] These are build/gap candidates derived from available data, not musical understanding. A rising pad, snare roll or breakdown can satisfy them. Preserve their separate values and rejection reasons. Source seams: `A:239–240,346–367`; failed-detector comparison: `v3/DETECTOR.md:75–93`.

### States and transitions

| State | Entry/exit conditions | Geometry and permitted response |
|---|---|---|
| **[HYPOTHESIS] FLOW** | Default with valid audio. Enter BUILD only after \(B>.6\) for 750 ms. | Ordinary live mapping remains active. No semantic-drop claim. |
| **[HYPOTHESIS] BUILD** | Sustained build evidence; leave if \(B<.25\) for 1 s, or after 12 s without further evidence. | Slowly contract towards edges. Latch maximum tension; do not reverse contraction on every kick. |
| **[HYPOTHESIS] ARMED/GAP** | BUILD plus sub-gap, or a separately qualified section-boundary cue. Retain for ≤8 s. | Hold geometric tension. Actual silence produces black; stored tension emits nothing. |
| **[HYPOTHESIS] PROVISIONAL** | While armed, an audible broadband/onset return \(J>.6\) or \(N>.75\). | Immediate small inward accent: extra reach ≤.12, no global brightness gain. Await sub return for ≤150 ms. |
| **[HYPOTHESIS] RELEASE** | Armed plus broadband/onset return and sub recovery to within 6 dB of pre-gap level, with sub rise ≥3 dB over two hops. Future predictive entry requires qualified beat/downbeat evidence. | Full inward travelling front; initial extent inherited from build. A late confirmation is recorded as late, never backdated. |
| **[HYPOTHESIS] RECOVER** | After release traversal; refractory 1600 ms for another full gesture. | Return to ordinary extent over 400 ms; attacks remain active throughout refractory. |
| **[HYPOTHESIS] QUIET/INVALID** | Existing silence/admission or chronology rejection. | No new deposits; drain to black. Invalidate predictive phase immediately. Keep only bounded, non-emitting section/tension memory. |

**[HYPOTHESIS] Current-source seams:** place state updates beside `updateReleaseV9`, but separate semantic evidence, geometry and deposition. Reuse per-board state ownership; do not make one board’s render call advance another board’s state. Sources: `W:752–795`; `WH:160–163`; `PR:243–280`.

### Extent trajectories

**[HYPOTHESIS] Build contraction, using \(S(u)=3u^2-2u^3\):**

\[
e(t)=e_{\rm entry}-(e_{\rm entry}-.12)\,S(u),
\qquad
u=C((t-t_{\rm build})/T_{\rm build}).
\]

**[HYPOTHESIS] Start with \(T_{\rm build}=4000\) ms, or eight validated beats clamped to 2000–8000 ms. Advancing \(u\) requires continued build evidence; interrupted evidence pauses or recovers over 1000 ms. This avoids contracting indefinitely because an old build flag remains set.**

**[HYPOTHESIS] Confirmed release:**

\[
e(t)=e_0+(.98-e_0)\left[1-(1-u)^3\right],
\quad
u=C((t-t_r)/T_r),
\]

\[
T_r=180-80J_r\ {\rm ms}.
\]

**[HYPOTHESIS] This gives immediate maximum velocity followed by deceleration, with traversal in 100–180 ms. The derivative is explicit:**

\[
\frac{de}{dt}=\frac{3(.98-e_0)}{T_r}(1-u)^2.
\]

**[INFERENCE] The existing eight-pixel scroll cap can limit this gesture. Evaluate the front analytically and deposit over its swept interval rather than forcing the gesture through ordinary integer history shifts. Source: `W:967–968,1000–1018`.

### Anticipation and wrong-detection behaviour

**[HYPOTHESIS] Qualification before prediction:** over a separate annotated corpus, require p95 beat prediction error ≤20 ms, no octave-rate error in accepted windows, and explicit downbeat accuracy if bar release is used. A beat-only phase must not be labelled a downbeat. These are proposed criteria; the current tracker has not passed them. Sources: `v3/DETECTOR.md:35–69`.

**[HYPOTHESIS] Once qualified:** allow a ≤.05 extent preparation during the last 60 ms before the predicted event. Use a separately measured output-time estimate for scheduling; do not subtract P5’s approximate total and claim photon synchrony. P5 explicitly leaves microphone and LED-to-photon intervals unresolved. Source: `docs/evidence/P5/device-budget/BUDGET.md:9–17,87–95`.

**[HYPOTHESIS] False-positive cost exceeds false-negative cost for a full drop gesture.** A false full release spends the visual climax before the music; a missed classification can still receive the fast ordinary response. Initial policy:

- Full unconfirmed release: prohibited.
- Provisional false accent: reach ≤.12; recover within 150 ms.
- Missed drop: fast RMS/onset mapping continues; late confirmation may complete the gesture, labelled late.
- Timeout/invalid phase: return to FLOW without stopping music.
- Confirmation failure: count the provisional visible excursion as a false accent; never erase it from scoring.

**[FACT] v3’s report excludes cancelled fires from hit/false-fire counts. That is legitimate for its detector bookkeeping but insufficient for judging visible choreography once a provisional flash has been rendered. Source: `v3/DETECTOR.md:79–93`.

## 4. Section memory and palette dynamics

### Section memory

**[HYPOTHESIS] Retain compact descriptors, not an averaged replacement for the live signal:**

- Eight recent descriptors containing the 12-value pre-knee chroma distribution, mean/peak RMS, sub/broadband ratio and onset density.
- Update on validated beats; otherwise every 500 ms and call them **time windows**, not beats or bars.
- Keep a separate 15 s reference for section comparison.
- Store the last strong harmonic anchor, last confirmed release time, build maximum and pre-gap sub level.
- Reset transient chronology on epoch change; never replay old attacks.

**[INFERENCE] This is small bounded state compared with existing 160-pixel histories. Eight 16-float descriptors occupy 512 bytes before metadata; no measured M85 cycle cost is available. Source baseline: `WH:212–240`.

**[HYPOTHESIS] Detect a possible section change when both conditions hold for 1 s:**

1. Chroma-vector cosine distance from the reference >.25.
2. Either RMS changes ≥4 dB, sub/broadband ratio changes ≥6 dB, or onset density changes ≥2 events/s.

**[HYPOTHESIS] Use a 4 s minimum section dwell. A recognised return may restore a prior palette offset/spread, but not its past brightness or motion events. This creates continuity without scripting a song-specific timeline.**

**[INFERENCE] A full chroma vector is preferable to the current scalar anchor for this purpose: different chords can share a circular centroid, and an octave fold cannot identify voicing. Sources: `W:154–176,609–624`.

### Palette dynamics

**[HYPOTHESIS] Keep three separate palette concepts:**

1. **Selected authored palette:** operator identity; no autonomous rainbow or palette roulette.
2. **Live harmonic offset:** path-P rule, updated from pre-knee shape.
3. **Spatial spread/section anchor:** slow, bounded context; never a gate on the live harmonic branch.

**[HYPOTHESIS] During a drop, spend contrast mainly on geometry and occupied space. Do not force white, jump to the brightest stop or normalise every colour to equal luminance. Those operations would remove authored dark regions and confound the intended test.**

**[FACT] The P6 receipt already demonstrates this distinction: one restricted-range candidate raised its “light” metric while lowering luma from build to drop. Source: `docs/evidence/P6/RECEIPT.md:30–32,46–51`.

**[HYPOTHESIS] Keep deposited offset and spread with each trail element. Interpolate brightness spatially; sample colour from the authored curve afterwards. When two layers overlap, select one layer’s complete palette attributes with the winning intensity rather than taking channelwise maxima or averaging RGB. Source starting mechanism: `W:844–901`.

### Lineage comparison

| Lineage | Observed mechanism | Implication for Titan |
|---|---|---|
| **[FACT] K1 S3**, HEAD `68a95773d41485fa14cb6dd71d89393d3aa25c73` | `palette_manual_colour` evaluates authored stops with float coordinates and CRGB16 output; cold-cache fallback still uses FastLED16. `S3/visual/lightshow_modes.h:244–294`. Commit `c2738b88` replaces GDFT’s FastLED call. | **[INFERENCE]** Reuse direct stop evaluation and test fallback separately. HD sampling and per-pixel coordinate choice are distinct levers. |
| **[FACT] K1 vibrancy**, commit `ae5d90a37480ce859d3ae058d0b186b9c456935f` | Pre-gate chroma and proportional weak-centroid movement instead of hard freeze. Current helper: `S3/visual/lightshow_modes.h:297–317,333–337,420–428`. | **[INFERENCE]** Do not reinstate a post-gate confidence multiplier merely to make colour look “stable.” |
| **[FACT] K1 Hybrid** | 16/23 ms peak smoothing, 80 ms RGB smoothing, 405 px/s scroll and amplitude-dependent decay. `S3/effects/light_mode_waveform_hybrid_k1.cpp:47–77,132–154`. | **[INFERENCE]** These are documented effect-character choices and adaptations; no source inspected proves they were mandatory S3 compute-saving measures. |
| **[FACT] Lightwave**, HEAD `4d71bc012d18207c05493f28c6ddb5d04cc24817` | Bloom V2 transports greyscale, applies display-only warp, then colour at output. `LW/src/effects/ieffect/sensorybridge_reference/SbK1BloomV2Effect.cpp:219–275`. Hybrid instead retains RGB trails and a roughly 163 ms colour EMA. `.../SbK1WaveformHybridEffect.cpp:159–227`. | **[INFERENCE]** Lightwave contains competing designs, not one universal colour doctrine. Attribute-state transport is the relevant reusable mechanism. |
| **[FACT] Lightwave smoothing correction**, commit `5c9f4f7498d6e590f9b1cc623adc7cafedd8a55b` | Commit describes reducing cascaded smoothing; actual commit diff changes `firmware-v3/src/audio/AudioActor.cpp`. | **[INFERENCE]** Audit complete serial filter chains. Commit prose is not proof every named downstream coefficient changed. |
| **[FACT] SensoryBridge**, commit `6109a1eeb4fe6f6f14b1669995561ea5c90ac9dd` | Spectrum River maps bin \(k\) to pixel \(80+k\), palette coordinate \(k/79\), brightness from bin energy; outward transport and per-frame .90 persistence. `SENSORY_BRIDGE_FIRMWARE/light_mode_spectrum_river.cpp:26–29,40–69`. | **[INFERENCE]** Preserves bin identity spatially, unlike a waveform’s single harmonic offset. Its frame-dependent decay should not be copied unchanged. |
| **[FACT] Emotiscope.HIL**, HEAD `7f86939bd7c16c4a6dd5789813e9d551246ed134` | v1.1 Bloom computes scalar spatial intensity then colour; its chroma assignment overwrites the transported value. Waveform auto-scales by frame maximum and uses fixed 63/64 mirror indices. `EMO/v1.1_build/light_modes/active/bloom.h:8–28`; `.../beta/waveform.h:4–41`. | **[INFERENCE]** Reuse separation of intensity and colour, not the overwritten transport, per-frame normalisation or 128-pixel geometry. |
| **[FACT] Emotiscope beat tunnel** | Uses multiple tempo phases, a narrow phase window and frame-count animation. `EMO/src/light_modes/active/beat_tunnel.h:12–39`. | **[INFERENCE]** Multiple rhythm hypotheses are an available lineage idea; this does not establish reliable downbeats or justify importing its timing unchanged. |

## 5. Anti-over-smoothing and over-averaging guards

**[HYPOTHESIS] Freeze each new criterion before running its candidate. Keep existing frozen P6 criteria unchanged. Evaluate both boards, matched PCM, sparse attacks, sustained passages, cuts, build/drop transitions and the −6/0/+6 dB ladder.**

| Guard | Proposed or retained criterion | Planted fault that must fail |
|---|---|---|
| **[HYPOTHESIS] Fast branch latency** | After a valid hop-level step is available, fast deposit reaches 50% within 15 ms and 90% within 30 ms; report source-to-availability separately. | Insert 80 ms EMA in fast branch. |
| **[HYPOTHESIS] Attack separation** | Two equal 15 ms attacks separated by 120 ms produce two distinct peaks; intervening fast-layer trough ≤60% of smaller peak. | Replace fast envelope with 290 ms release-only max hold. |
| **[HYPOTHESIS] Event conservation** | Every accepted event ID consumed exactly once at 60/100/120 Hz rendering; no invented events on repeated generations. | Re-fire whenever a held publication is “fresh.” |
| **[HYPOTHESIS] Frame-rate independence** | At common timestamps, extent differs ≤.01 and integrated intensity ≤2% across 60/100/120 Hz replay. | Fixed per-frame fade or frame-count hold. |
| **[FACT] Existing HD fidelity guard** | Max-normalised RGB distance to the exact piecewise-linear palette curve ≤.004, with offending fraction ≤.001 for lit pixels ≥.01. Source: `RUN/P6-20261014T124826Z/permitted/TOLERANCE-FROZEN-P6-PAL-HD-v2.json:5–20`. | Existing `slot16` and `rgb_mix` faults must fail. |
| **[FACT] Existing colour-motion guard** | Moving 1 s windows span >1/32; fraction ≥v8−.02; room ladder must not fall by >.02 at increasing level. Source: `.../TOLERANCE-FROZEN-P6-PAL.json:13–25`. | Frozen offset; post-knee shape substituted for pre-knee shape. |
| **[FACT] Existing spatial coverage guard** | Median ≥4 recovered palette bins and circular span ≥.25 on lit right-half pixels. Same source:17. | One colour sample broadcast across all pixels. |
| **[HYPOTHESIS] Harmonic identity** | On unclipped gain-scaled copies, pre-knee normalised chroma cosine similarity ≥.99 and strong-centroid circular deviation ≤.01. | Hard-clamp all magnitudes before shape normalisation. |
| **[HYPOTHESIS] Drop geometry** | Full release starts only under allowed transition; reaches \(e≥.9\) within 180 ms; inward front velocity exceeds preceding build contraction velocity by ≥4×. | Increase brightness without changing geometry; reverse front direction. |
| **[HYPOTHESIS] Detector timing** | Retain existing [0,+50 ms] annotated trigger window and zero false full releases on the scored excerpts. Report provisional excursions separately. | 200 ms late trigger; always-on trigger; cancellation that deletes a visible false event. |
| **[HYPOTHESIS] Silence** | No births during admitted silence; exact all-zero output within 1 s after zero PCM, and no recolouring-induced nonzero pixel. | Add 1/1024 brightness floor. |
| **[HYPOTHESIS] Configuration identity** | Product configuration with host variants off must match scored configuration frame-for-frame. | Restore `kRestrictedRange` while scoring host path P. |

**[INFERENCE] The 16/23 ms peak pair, approximately 21/80 ms spectral pair and 600/250 ms colour pair serve different branches. They must not be indiscriminately added into one “total latency”; measure the actual feature-to-parameter path. Sources: `A:274,323–326`; `W:669–673,974–975`.

**[INFERENCE] An 80→12 chroma fold can be intentional harmonic abstraction. A 12→1 centroid, RGB blending, palette discretisation, temporal low-pass and clipping each lose different information. Calling all of them “compression” hides which loss is necessary. Sources: `W:154–176,233–239,574–580`; `A:351–358`.

## 6. Lever records

**[HYPOTHESIS] Ratings below estimate prospective value, not demonstrated preference. Impact, novelty and effort each use 1–5. M85 costs are unmeasured unless explicitly stated. Every experiment is proposed for later authorised execution.**

### D11-01 — Preserve events across AP/VP rates

- **Container:** [HYPOTHESIS] map.
- **Stage:** [FACT] `InhouseAudio::acquire`, `A:377–429`; renderer admission `W:120–133`.
- **Controls:** [HYPOTHESIS] Attack timing, count and velocity impulses.
- **Current setting v8/v9:** [FACT] Latest publication; eight-generation maximum-peak summary; same-generation freshness permitted.
- **Information preserved/destroyed:** [INFERENCE] Approximately 60 ms peak amplitude coverage, but event time/count and corresponding spectral shape are lost.
- **Origin:** [FACT] Bounded missed-generation defence documented at `A:410–425`; **[INFERENCE]** not established as a compute limitation.
- **Still needed on Titan:** [HYPOTHESIS] Bounded buffering yes; maximum-only semantics no. M85 cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Small timestamped event queue; per-hop state update; render interpolation.
- **Interactions:** [HYPOTHESIS] Fast envelope, beat phase, event refractory, stale-input handling.
- **Visual impact:** [HYPOTHESIS] 5.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 3.
- **Risk:** [HYPOTHESIS] Duplicate events, queue overflow or fabricated phase through gaps.
- **Smallest decisive experiment:** [HYPOTHESIS] Replay timestamped attacks at 133⅓ Hz AP against 60/100/120 Hz VP; require exact accepted-event count and ≤one-render-period availability delay. Plant latest-value-only loss and repeated-generation retriggering.

### D11-02 — Fast absolute-level branch

- **Container:** [HYPOTHESIS] map.
- **Stage:** [FACT] `analyse`, `A:228–254`; `renderV9`, `W:940–950`.
- **Controls:** [HYPOTHESIS] Attack brightness and width.
- **Current setting v8/v9:** [FACT] v8 uses peak/VU/chroma mixtures; v9 fixed −33…−9 dB map and exponent 1.5.
- **Information preserved/destroyed:** [INFERENCE] v9 restores level ordering but discards levels outside its 24 dB interval; adaptive peak is not absolute loudness.
- **Origin:** [FACT] P6 matched-light design, `WH:31–36`; **[INFERENCE]** aesthetic/experimental.
- **Still needed on Titan:** [HYPOTHESIS] Separate level and attack paths yes; current thresholds unknown. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Raw-RMS branch, 8/120/15 ms envelope, candidate −45…−9 dB interval.
- **Interactions:** [HYPOTHESIS] Admission, Response, output cap, deposit width.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] Quiet-room noise becomes visible; excessive width disguises level.
- **Smallest decisive experiment:** [HYPOTHESIS] Matched PCM level ladder plus quiet attacks; require monotone integrated intensity, fast-branch t90≤30 ms and unchanged silence gate. Plant adaptive normalisation into brightness and an 80 ms EMA.

### D11-03 — Explicit contraction and inward release

- **Container:** [HYPOTHESIS] map / VP.
- **Stage:** [FACT] `renderV9`, `W:963–1023`; optional `updateReleaseV9`, `W:752–795`.
- **Controls:** [HYPOTHESIS] Extent, direction, thrust and arrival.
- **Current setting v8/v9:** [FACT] Outward history transport; peak moves deposit towards edge. v9 optional release moves root inward but defaults off.
- **Information preserved/destroyed:** [INFERENCE] Current mapping has no retained build phase; a single peak cannot distinguish build climax from drop.
- **Origin:** [FACT] Inherited bouncing-dot character, `S3/effects/light_mode_waveform_hybrid_k1.cpp:10–26`.
- **Still needed on Titan:** [HYPOTHESIS] Preserve reference mode; add separately selectable choreography. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] §3 state machine and analytic edge-to-centre front.
- **Interactions:** [HYPOTHESIS] Drop confidence, history transport, cap, true black.
- **Visual impact:** [HYPOTHESIS] 5.
- **Novelty:** [HYPOTHESIS] 4.
- **Effort:** [HYPOTHESIS] 4.
- **Risk:** [HYPOTHESIS] False climax; direction confusion; skipped pixels during fast travel.
- **Smallest decisive experiment:** [HYPOTHESIS] First use fixed labelled event times to isolate renderer geometry: build reaches e≤.18, release e≥.9 within180 ms, speed contrast≥4×. Plant brightness-only accent and outward release. Then test causal detection separately.

### D11-04 — Prediction with bounded consequences

- **Container:** [HYPOTHESIS] map.
- **Stage:** [FACT] Tempo consumption `W:980–993`; tracker `TT:17–39`.
- **Controls:** [HYPOTHESIS] Beat preparation and release timing.
- **Current setting v8/v9:** [FACT] v8 tempo weight can reach1; v9 caps it at.7. No qualified downbeat field.
- **Information preserved/destroyed:** [INFERENCE] One winner suppresses tempo ambiguity; half-time and beat/bar phase remain conflated if treated as a drop clock.
- **Origin:** [FACT] Inherited tracker prior and confidence logic, `TT:500–519`.
- **Still needed on Titan:** [HYPOTHESIS] Rhythmic motion yes; unqualified predictive release no. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Prediction disabled initially; later use qualified timing with ≤.05 preparation extent.
- **Interactions:** [HYPOTHESIS] Detector, latency compensation, section memory.
- **Visual impact:** [HYPOTHESIS] 5.
- **Novelty:** [HYPOTHESIS] 3.
- **Effort:** [HYPOTHESIS] 4.
- **Risk:** [HYPOTHESIS] Confident early false release.
- **Smallest decisive experiment:** [HYPOTHESIS] Freeze p95 beat-error≤20 ms and zero octave errors on accepted windows; separately score downbeats. Plant doubled period and 80 ms phase bias; require prediction disabled rather than stronger smoothing.

### D11-05 — Use sub-band evidence without waiting exclusively for it

- **Container:** [HYPOTHESIS] AP / map.
- **Stage:** [FACT] `SubBandDetector::process`, `SB:40–55`; `updateReleaseV9`, `W:763–765`.
- **Controls:** [HYPOTHESIS] Gap evidence and release confirmation.
- **Current setting v8/v9:** [FACT] No v8 field; v9 computes sub RMS, optional release defaults to broadband and is disabled.
- **Information preserved/destroyed:** [INFERENCE] Adds 40–110 Hz energy missing from note bins; loses detailed bass waveform and timing through band filtering/RMS.
- **Origin:** [FACT] P6 EXT addition, `SB:3–8`.
- **Still needed on Titan:** [HYPOTHESIS] Yes as complementary evidence; not sufficient as a drop oracle. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Broadband immediate response, sub confirmation, explicit gap memory.
- **Interactions:** [HYPOTHESIS] False-positive policy and delayed bass body.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] All releases late when bass arrival is late.
- **Smallest decisive experiment:** [HYPOTHESIS] Preserve the existing +50 ms detector criterion while separately reporting bass-evidence arrival. Plant “wait for sub only”; it must fail timely-response cases rather than move annotations.

### D11-06 — Remove upstream smoothing from the wrong consumer

- **Container:** [HYPOTHESIS] AP / map.
- **Stage:** [FACT] `analyse`/`condition`, `A:274,313–350`; `applyShapeCentroidP`, `W:593–624`.
- **Controls:** [HYPOTHESIS] Spectral motion and harmonic responsiveness.
- **Current setting v8/v9:** [FACT] Both use magnitude/Mood smoothing; path P bypasses knee but not those filters.
- **Information preserved/destroyed:** [INFERENCE] At default Mood, serial spectral τ≈21+80 ms before visual filtering; rapid shape changes are attenuated.
- **Origin:** [FACT] Explicitly inherited source quirk, `A:313–317`; no demonstrated Titan necessity.
- **Still needed on Titan:** [HYPOTHESIS] A slow context branch yes; fast branch needs independent assessment. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Publish an unsmoothed or lightly smoothed shape alongside slow shape.
- **Interactions:** [HYPOTHESIS] Noise, weak chroma, novelty, colour-motion criteria.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 3.
- **Risk:** [HYPOTHESIS] Faster but unstable colour/noise.
- **Smallest decisive experiment:** [HYPOTHESIS] Alternating spectral patterns plus recorded sparse music; require ≥80% modulation retention at4 Hz and ≤20% rise in quiet-frame coordinate jitter. Plant 100 ms EMA into the fast shape.

### D11-07 — Preserve palette attributes through transport

- **Container:** [HYPOTHESIS] VP.
- **Stage:** [FACT] `depositV9`/`outputV9`, `W:844–915`.
- **Controls:** [HYPOTHESIS] Colour fidelity and spatial variety.
- **Current setting v8/v9:** [FACT] v8 RGB accumulation; v9 intensity winner plus stored colour; path P adds deposited offset and per-pixel HD sampling.
- **Information preserved/destroyed:** [INFERENCE] RGB sums lose source palette identity; path P preserves coordinate identity but still has one harmonic offset per deposit.
- **Origin:** [FACT] K1 HD/vibrancy lineage; `c2738b88`, `ae5d90a`; path-P contract `TOLERANCE-FROZEN-P6-PAL.json:5–9`.
- **Still needed on Titan:** [HYPOTHESIS] Yes; no need to average RGB history. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Carry intensity, offset, spread and event owner; shade once at output.
- **Interactions:** [HYPOTHESIS] Resampling, overlap, palette changes, saturation.
- **Visual impact:** [HYPOTHESIS] 5.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] Winner starvation of weaker events; attribute discontinuity.
- **Smallest decisive experiment:** [HYPOTHESIS] Existing PAL/HD-v2 fidelity, spread and motion gates; plant RGB blend, channelwise max, FastLED16 and frozen offset. Require each specified fault red.

### D11-08 — Separate section memory from live colour

- **Container:** [HYPOTHESIS] map.
- **Stage:** [FACT] `updateColourV9`, `W:668–719`; path-P anchor `W:638–650`.
- **Controls:** [HYPOTHESIS] Harmonic continuity and section identity.
- **Current setting v8/v9:** [FACT] v8 held centroid; default v9 15 s anchor plus 8 s drift; path P last strong anchor.
- **Information preserved/destroyed:** [INFERENCE] One angle cannot distinguish chord distributions sharing a centroid; long averaging erases section boundaries.
- **Origin:** [FACT] P6 colour-stability design, `W:655–667`.
- **Still needed on Titan:** [HYPOTHESIS] Context yes, scalar-only context insufficient. Proposed descriptors ≈512 bytes before metadata; cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Eight vector descriptors and confidence-qualified return memory.
- **Interactions:** [HYPOTHESIS] Build detection, palette spread and epoch reset.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 4.
- **Effort:** [HYPOTHESIS] 3.
- **Risk:** [HYPOTHESIS] False section recognition and persistent wrong anchor.
- **Smallest decisive experiment:** [HYPOTHESIS] A–B–A chroma sequence including two distributions with the same centroid; require distinct stored contexts and return-offset error≤.02 without delaying transient t90. Plant scalar-centroid-only memory.

### D11-09 — Keep trail persistence independent of level

- **Container:** [HYPOTHESIS] VP.
- **Stage:** [FACT] `fadeV9`, `W:798–806`; v8 `W:254–256`.
- **Controls:** [HYPOTHESIS] Temporal separation and occupied space.
- **Current setting v8/v9:** [FACT] v8 decay depends on peak; v9 τ290/125 ms with quiet drain.
- **Information preserved/destroyed:** [INFERENCE] v9 preserves stable temporal character; persistence can still merge rapid events.
- **Origin:** [FACT] P6 explicit attack/release separation; `W:798–800`.
- **Still needed on Titan:** [HYPOTHESIS] Yes; exact constants require musical comparison. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Fixed baseline plus optional beat-aware τ90–220 ms.
- **Interactions:** [HYPOTHESIS] Velocity controls trail length; width and max-hold affect overlap.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] Short tails become staccato; adaptive τ pumps.
- **Smallest decisive experiment:** [HYPOTHESIS] 120/180/240 ms attack spacing, equal integrated light; require distinguishable peaks and exact black≤1 s. Plant level-dependent decay or per-frame fade.

### D11-10 — Preserve fractional travel without colour averaging

- **Container:** [HYPOTHESIS] VP.
- **Stage:** [FACT] Integer scroll `W:1000–1018`; mode-18 position `W:995–998`.
- **Controls:** [HYPOTHESIS] Motion smoothness and velocity accuracy.
- **Current setting v8/v9:** [FACT] Fraction accumulated, whole-pixel history shifted; mode18 deposit rounded.
- **Information preserved/destroyed:** [INFERENCE] Average distance is partly retained; within-frame fractional position is not represented in history transport.
- **Origin:** [FACT] Source-preserving transport, `W:113–117,963–965`; budget justification unknown.
- **Still needed on Titan:** [HYPOTHESIS] No proven requirement for integer-only transport; cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Fractional coordinates or resample intensity/attributes before palette lookup.
- **Interactions:** [HYPOTHESIS] HD colour, high-speed release and frame cadence.
- **Visual impact:** [HYPOTHESIS] 3.
- **Novelty:** [HYPOTHESIS] 2.
- **Effort:** [HYPOTHESIS] 3.
- **Risk:** [HYPOTHESIS] Numerical diffusion or interpolating palette coordinates across a wrap incorrectly.
- **Smallest decisive experiment:** [HYPOTHESIS] 0.25/0.5/1.5 px-per-frame travelling pulse; centroid error≤.25 px, energy error≤2%, existing HD fidelity maintained. Plant RGB interpolation and forced integer rounding.

### D11-11 — Treat symmetry and board independence explicitly

- **Container:** [HYPOTHESIS] VP / system.
- **Stage:** [FACT] Per-board `renderInhouse`, `PR:243–280`; mirror `W:880–888`.
- **Controls:** [HYPOTHESIS] Spatial coherence, paired gestures and independence.
- **Current setting v8/v9:** [FACT] Separate renderer objects; v9 mirrors output lookup. v8 mode18 skips mirror during non-injecting drain. `W:516–519`.
- **Information preserved/destroyed:** [INFERENCE] Mirroring deliberately removes left/right difference; copying state between boards would additionally destroy independent scene history.
- **Origin:** [FACT] Explicit per-board ownership `WH:160–163`; inherited mode18 exception.
- **Still needed on Titan:** [HYPOTHESIS] Independence yes; symmetry is scene-specific. Cycles unknown.
- **Manipulation options:** [HYPOTHESIS] Shared event timestamp, independent artistic responses; exact mirrored release where selected.
- **Interactions:** [HYPOTHESIS] Palette identity, event queue and physical output mapping.
- **Visual impact:** [HYPOTHESIS] 4.
- **Novelty:** [HYPOTHESIS] 3.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] Shared mutable state or false universal symmetry checks.
- **Smallest decisive experiment:** [HYPOTHESIS] Change one board’s speed/palette; peer output hashes remain identical. For the new symmetric release, paired front coordinates sum159±.5. Plant shared history and reversed half mapping.

### D11-12 — Make the scored mapping the compiled mapping

- **Container:** [HYPOTHESIS] system.
- **Stage:** [FACT] `profile_defines`, `scripts/live_boot_composition.py:12–17`; `perPixelMapped`, `W:585–590`; `WH:47`.
- **Controls:** [HYPOTHESIS] Whether the real image uses the intended colour/motion design.
- **Current setting v8/v9:** [FACT] v8 adds no mapping flag; v9 enables renderer branch but inspected default remains restricted-range.
- **Information preserved/destroyed:** [INFERENCE] Host-only selector evidence does not describe the product configuration; a correct experiment can be entirely absent from the image.
- **Origin:** [FACT] Documented integration failure, `PL14:148–151`.
- **Still needed on Titan:** [HYPOTHESIS] Configuration equivalence always needed; runtime cycle cost essentially a build-selection concern, not measured.
- **Manipulation options:** [HYPOTHESIS] Product-config replay with host variants off and explicit selected-path witness.
- **Interactions:** [HYPOTHESIS] Every visual experiment and build receipt.
- **Visual impact:** [HYPOTHESIS] 5.
- **Novelty:** [HYPOTHESIS] 1.
- **Effort:** [HYPOTHESIS] 2.
- **Risk:** [HYPOTHESIS] Claiming corrected appearance from a non-product test binary.
- **Smallest decisive experiment:** [HYPOTHESIS] Require identical frames between scored path P and product-config replay; plant default back to B. Verify both boards and silent cuts.

## 7. Contradictions, open questions and handback

### Contradictions requiring explicit treatment

1. **[FACT] Brightness:** this assignment says24/255; `scripts/build_scalar.py:932–935` requires boot defaults100–255. **[INFERENCE]** Research can preserve the24 cap, but a later executable candidate must reconcile which control layer enforces it. No build or device change was attempted.

2. **[FACT] Path-P selection:** `PL14:150` instructs selecting P; inspected `WH:47` still selects B. **[INFERENCE]** The instruction and adjacent comment are not evidence that the executable default changed.

3. **[FACT] Rate:** brief:32 names120 Hz; `platform/ra8p1/palette_runtime.h:39–47` selects100 Hz under Emotiscope. P5’s capture reports100 Hz. **[INFERENCE]** Test both current cadence and proposed target; do not silently call them equivalent.

4. **[FACT] Bandwidth:** GDFT starts110 Hz (`src/k1/core/audio/gdft_goertzel.cpp:26`), but v9 has a separate40–110 Hz feature (`SB:17–55`). **[INFERENCE]** The note bank is blind below110 Hz; the entire inspected AP is not.

5. **[FACT] Palette history:** the first forensics report says no80-bin mapping bug was found; the second identifies the GDFT FastLED16 replacement at `c2738b88`. Sources: `docs/evidence/P6/palette-forensics/FORENSICS.md:22`; `palette-forensics-2/grok-k1-git.md:39–60`. **[INFERENCE]** These are separate failures: spatial palette resolution, chroma-coordinate freezing and temporal colour averaging.

6. **[FACT] Titan sampling:** current `W:41–70` already directly samples authored stops; the v8 fallback scans compiled16 entries at `W:223–231`. **[INFERENCE]** “Titan used a16-slot palette everywhere” is too broad. Path P additionally changes *where and how often* colour is sampled.

7. **[FACT] Direction:** detector-v1 prose describes contraction towards the middle, while the brief’s Captain quotation requires final compression towards the edges. Sources: `RUN/P6-DROP-DETECTOR/DETECTOR.md:7`; brief:26. **[HYPOTHESIS]** Use the explicit \(e=0\) edge / \(e=1\) centre convention.

8. **[FACT] Detector outcomes:** “all failed” means no complete frozen-corpus pass, not zero hits for every version. v1/v2 have isolated hits; v3 has none. Sources: detector reports cited in §3.

9. **[INFERENCE] Half-time does not mathematically prevent every drop from coinciding with a predicted beat; it loses alternate beat opportunities. Actual phase errors and unresolved bar phase are the evidence against prediction here. Source: `v3/DETECTOR.md:41–69`.

10. **[FACT] P5’s≈16.4 ms typical total includes a modelled frame wait and excludes unstamped acoustic/optical intervals. **[INFERENCE]** It is neither a direct photon-latency measurement nor a universal compensation constant. Source: `docs/evidence/P5/device-budget/BUDGET.md:15–17,81,87–95`.

### Open questions

- **[HYPOTHESIS] Artistic decision:** should ordinary mode32 retain the outward bouncing-dot character, with inward motion reserved for drops, or should the entire scene use edge-origin extension? Compare these two candidates with identical event times, palette and integrated light.
- **[HYPOTHESIS] Error tolerance:** is a small provisional inward accent acceptable when no drop follows? The proposed≤.12 reach is deliberately conservative, not an established preference.
- **[FACT] Holdout status:** Carte Blanche is already seen in detector v2/v3; those reports cannot support an unseen-holdout claim. `v2/DETECTOR.md:15`; `v3/DETECTOR.md:11`.
- **[INFERENCE] Drop annotations and room alignment need retained uncertainty bounds. A detector cannot be judged at50 ms precision while silently moving anchors to its own energy peaks. Source: v1 `DETECTOR.md:63`.
- **[INFERENCE] Harmony-driven colour can still cancel at the centroid. Test full-vector section memory before adding more centroid smoothing. Source: `W:609–624`.
- **[FACT] Target cycle cost and physical appearance of this design are unknown.** Existing P5 timing concerns another image and cannot certify the proposed state machine. Source: P5 `BUDGET.md:5`.

### Evidence limits and next executable path

> 🧠 **From Hindsight memory (Faithful in-house Waveform migration to assembled Titan K1)** — [FACT] The page retained14October migration and v8 state claims. They were treated as historical leads; this report uses the current source and named evidence instead of treating that remembered build as resident.

**[FACT] Memory health:** the session guard reported six failed refresh operations. Knowledge-page retrieval worked, but the standing-rules lookup required the separate mental-model endpoint. No memory repair or installation was attempted.

**[FACT] Verification performed:** source and git-history reads; inspected mapping/build diffs; SHA-256 reads; deterministic arithmetic for filter time constants and the16/23 ms step response. The platform-memory checker reported valid records with source identity unchecked. Governance reported no bound task ledger and explicitly no completion certificate.

**[FACT] Visual output:** no render, screenshot or LED output was produced. Images are N/A for this read-only source/design report; it makes no appearance-equivalence or physical-response claim.

**[HYPOTHESIS] Remaining path, in order:**

1. Prove product-config path P matches its scored host configuration, including a wrong-default planted fault.
2. Test the fast mapping and inward geometry using fixed event times, preserving24/255 and true black.
3. Test causal state detection independently; retain failed drops, false accents, timing errors and spent holdouts.
4. Measure added M85 cost and complete-image behaviour through the existing lane procedure.
5. Under the hardware owner’s applicable authority, compare matched physical output; only that comparison can establish whether the release is congruent with the music and “hits harder.”


---

## Appendix V — Opus verification of the Astra lane (15 Oct 2026)

Lane: GPT Astra (`gpt-6-astra`, high reasoning, Codex CLI 0.162.1, read-only sandbox), session `01a13b68-e9b3-7223-94d4-4ec7b4acf92e`, rc=0. This appendix was added by the verifying Opus agent; everything above it is Astra's text unchanged.

**Citation spot-checks (20 checked, 0 failed, 3 nuances).**

| # | Claim | Result |
|---|---|---|
| 1 | `WH:47` default palette is `kRestrictedRange`, not path P | [FACT] confirmed in the working tree (line 47) and in HEAD `1ef5395` (line 46). The colour fix is still not the compiled default. |
| 2 | Mode-32 root `p=80+56·peak₂` (`W:976`) | confirmed (`0.7F*80`) |
| 3 | Trail τ 290/125 ms, silence +10 s⁻¹ (`W:801`) | confirmed |
| 4 | Level −33…−9 dBFS (`W:948–949`, `WH:48`) | confirmed |
| 5 | Width `.5+3.5l`, reach width+1 (`W:826,841`) | confirmed |
| 6 | Colour follow 0.6 s / momentum 0.25 s / anchor 15 s / drift 8 s / caps .25,.03 (`W:668–673`) | confirmed |
| 7 | Mode-18 tempo weight capped 0.7 (`W:986–987`) | confirmed |
| 8 | Profile 9 only adds `-DK1_INHOUSE_MAPPING_V9=1` (`scripts/live_boot_composition.py:12–17`) | confirmed |
| 9 | Boot brightness must be 100..255 (`scripts/build_scalar.py:932–935`) | confirmed (lines 933–934); conflicts with the 24/255 cap |
| 10 | Note bank starts at 110 Hz (`gdft_goertzel.cpp:26`) | confirmed with nuance: table starts at 55 Hz, `kNoteOffset = 12` (line 18) makes the first used bin 110 Hz |
| 11 | `.3+.7·brightness`, shoulder above .75 (`PR:281–294`) | confirmed |
| 12 | 100 Hz under `K1_EMOTISCOPE_PORT` (`palette_runtime.h:39–47`) | confirmed; nuance: the non-port default is 8333 µs = 120 Hz |
| 13 | Sub-band 40–110 Hz, 4th order (`sub_band.h:3–8,17–18`) | confirmed |
| 14 | Tracker 88 BPM prior, 512-update warm-up (`tempo_tracker.cpp`) | confirmed (`kTactusBpm = 88.0F`, `kConfidenceWarmupUpdates = 512`) |
| 15 | PL14-B14 says select P as compile-time default (`PL14:148–151`) | confirmed; the source has not done it |
| 16 | Detector v1 1/5 + holdout miss; v2 1/6; v3 0/6 | confirmed in the three DETECTOR.md files |
| 17 | Detector-v1 prose says the build pulls "from the edges toward the middle" (`DETECTOR.md:7`) | confirmed; contradicts the Captain's quote |
| 18 | Mood LPF quirk and rational α (`A:23–26,313–317`) | confirmed; 30 / 1990 ms arithmetic reproduced |
| 19 | S3 Hybrid 16/23 ms, 405 px/s, decay scale 3.5 | confirmed; nuance: the S3 file labels 405 px/s "Captain-calibrated native speed 27" (lines 23, 54), so its origin is aesthetic, not budget |
| 20 | SensoryBridge `6109a1e` Spectrum River `hue=k/79`, persistence .90 | confirmed at `light_mode_spectrum_river.cpp:27,64` in `/Users/spectrasynq/k1-ap-evidence-lab` (Astra gave no repo path) |
| + | P6 receipt: variant B luma ×0.80 while "light" ×1.42 build→drop (`docs/evidence/P6/RECEIPT.md:32,48`) | confirmed |

Derived numbers re-computed: spectrum smoother τ = 10/ln 4 = 7.21 ms; chroma-max decay τ ≈ 9.995 s; quiet trail τ 74.4/55.6 ms; 16→23 ms cascade step response 27.8 % at 20 ms, 91.4 % at 80 ms. All match.

**One finding Astra implied but did not state, now made explicit.**
[INFERENCE, from `W:966–1019`] In mode 32 the history scrolls outward from index 80 and index 80 is cleared every step, so the lit span runs from the deposit root `p` to the edge. Louder relative peak moves `p` outward (`p` up to 136), so the lit span shrinks towards the edges: at `peak₂ = 1` only 23 of 80 pixels (≈29 % extent) can stay lit. A build that gets louder therefore contracts naturally, which matches the Captain's build description. But the drop is louder still, and `peak_scaled` is normalised against a follower that decays over ≈2 s (`A:23–26`), so after a breakdown the drop drives `peak₂` to its maximum. **The current mapping renders the drop as the most-contracted moment, which is the reverse of "hits harder".** The disabled `updateReleaseV9` (`W:977`: `position = 80 + (position-80)(1-release)`) is exactly the inversion this needs. That makes D11-03 the highest-value lever, and its first test needs no detector: fixed labelled drop times with release on vs off.

**Visual output:** none produced. Images are N/A because this is a read-only source and design report.
