THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

> 28 September correction: read [DUAL-EDGE-CORRECTION.md](DUAL-EDGE-CORRECTION.md) first. K1 is one light guide with top and bottom 160-pixel edges (320 total). That addendum supersedes the single-channel-first/deferred-top-bottom sequencing below and makes the existing Lightwave library the direct K1 effects baseline. The full original assignment remains preserved.

# Titan effects development — lane opening and implementation assignment
28 September 2026 · Australia/Perth
Lane: titan-effects-language-001
Branch: feat/titan-effects-language-001
Base: a96278c3cb1ab5346a0010412438cb7ecb29be03
Scope of this opening: create an isolated lane, preserve the existing decomposition and executable input material, diagnose the reference scene's artistic limits, and assign the first implementation. This opening is not a new-effects implementation or physical acceptance receipt.

## Objective

Deliver one compelling, stateful, centre-origin K1 appearance derived from the complete Sensory Bridge and Emotiscope mechanisms. The first release is Bloom/ribbon material: visible outward travel, intentional colour history, a quiet spatial/temporal interval and a weaker return. Demonstrate musical relevance and visual impact against the donor references and the existing K1 appearance, using the same recorded music and actual C++ output pipeline.

Use the implemented AP-A/AP-B as input. Do not reopen that implementation as the effects lane's main job. Read its limitations and support fresh/uncertain/missing rhythm evidence gracefully. The effects must have useful immediate musical response before tempo lock and while phase evidence is unavailable.

An automated geometric difference, a pretty browser glow, additional mode names, higher numeric precision or green tests cannot establish visual success. The operator has rejected the current audition as an effects direction. That is an appearance failure even though the AP/interface checks pass.

## Source authority and current state

Repository root:
/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware

This worktree:
.worktrees/titan-effects-language-001

AP source:
.worktrees/titan-music-intelligence-001
Implementation 950580c5548478180721a3e9658d6cfc111f888a
CLI repair 27a20028b48794bd3a6fa667ce0ef5886c5b87b4
Evidence a96278c3cb1ab5346a0010412438cb7ecb29be03

The music-intelligence tree currently also has uncommitted edits in platform/ra8p1/fixture_app.cpp, platform/ra8p1/stage_probe.h, scripts/run_scalar_schedule.py and scripts/stage_profile.py. This new lane starts from the named committed AP result. It does not copy or overwrite those active platform edits. The platform owner reconciles them before target integration; do not assume this base is the latest deployable runtime.

AP-A/AP-B, the immutable AP-to-VP view and host reference scene exist. Read docs/evidence/tempo-field/20260928/RECEIPT.md and acceptance-review.md. Host mathematical/causal evidence and ARM compilation do not close AP-G3 target admission. The dense-mixture phase limitation remains visible; effect fallback must handle it.

The former white/blue corruption status was stale. The later control-lane release-20260927-rc1/RESULT.md records an86.652s optical run with2796 camera frames, zero flash candidates and reviewed normal output; POST-QUALIFICATION-RESTORATION.md records changing continuous output restored. Do not reopen the old driver defect merely from historical closure text. The MIR receipt's later stale-audio/hold observation concerns a different sampled baseline and is not proof that white flashes returned. Recheck only the actual target prerequisites before target work.

Preserve original AP, VP, MOD, CTL, PERF and IVV assignments, task IDs and complete text. This is an additive execution focus. The existing full handoff remains docs/k1-smc-migration-2026-09-26/reference/K1-Implementation-Handoff.md; especially the VP chapter and VP-01…05, plus MOD's mode semantics. Full recovered reports and original appearance-transfer assignment are retained under docs/effects-language-001/reference/. Do not replace them with this addendum.

## Why the diagnostic audition is not an effects baseline

Inspected: src/k1/core/visual/tempo_reference_scene.{h,cpp}, tools/audio/tempo_audition_template.html, the packaged song-0 manifest and saved18s screenshot. This inspection did not play the complete song or establish an optical verdict.

1. renderTempoReferencePair is stateless. Each call regenerates scalar triangular pulses; it stores no deposited RGB, transported material, object lifecycle or history.
2. It maps the latest onset's age over600ms to radius. Closely spaced events replace the current onset description instead of preserving independently travelling gestures.
3. It maps each admitted phase directly to another similar triangular pulse. More musical evidence produces more similar marks rather than distinct compositional roles.
4. A constant0.10 spatial base and common VU envelope supply the body. They are not designed spectral material, harmonic colour, or phrase-dependent contrast.
5. The scene uses one fixed teal. It cannot demonstrate either donor's colour-history behavior or palette inertia.
6. Every frame is rescaled to identical baseline/candidate total output. That is useful for the AP shape comparison, but using it as the authored production transfer can flatten intended accent/background relationships.
7. The18s screenshot reports all displayed candidate phases unavailable. Weak motion there cannot be attributed solely to a renderer ignoring valid phase; the new appearance must remain meaningful in this exact condition.

My preceding brief deliberately constrained the reference scene to isolate AP contributions. Preserve it unchanged as a diagnostic. Do not keep polishing that triangular-pulse demonstration and call the effects problem solved.

## The recovered donor baseline

| Reference | Pin and distinct appearance mechanism |
|---|---|
| Sensory Bridge4.1.1 | 5f329261987144242be4c3729f6d344631d3be1e. Bloom deposits chroma-derived RGB at the centre; the colour lives in retained history. Fractional movement changes diffusion as well as position; retention and outer fade shape disappearance. |
| Emotiscope1.1.0 | 634b7ed8eefa2f1913ba1a1e7b29c116f22714d4. Bloom transports scalar VU history and colours it using the current hue. Its existing trail can recolour. Swept drawing, float composition and output treatment are part of the reference. |
| Emotiscope1.1 Spectronome | A spectral body remains visible while rhythmic concentration changes foreground/background emphasis. Metronome uses contribution m_i²/sum(m), sinusoidal position, square-root contribution for excursion and contribution for opacity. This is not normalized probability. |
| Sensory Bridge4.x Kaleidoscope | Low/mid/high spectral sums move separate smooth-noise coordinates; slower envelopes control brightness. Musical timbre moves a persistent material without requiring a stable beat. |
| SuperPixie | cacbe615aa31a3c8d5a7d7e66286c67a7a7779ce. Optional reference for continuous geometry and immediate-attack persistence. It is not an MIR donor. |

Use the report's full version distinctions. Sensory Bridge3.2 is a separate appearance, not a interchangeable implementation. Emotiscope2.0 is unfinished experimental material. The HIL fork's overwritten Bloom history is not the official Bloom. Hype odd/even grouping means tempo-bin parity, not kick versus snare.

Local donor copies have execution-day SHA-256 provenance in the lane input manifest. Their directory names alone do not prove correspondence to the upstream commit. Verify relevant function/blob correspondence before making upstream-parity claims. Preserve licenses. Do not redownload and reteardown all versions when a named local source and the existing analysis answer the question.

## The first implementation: one complete Bloom/ribbon profile

### FX-01 — reproduce the two material references

Build executable reference adapters around the smallest official Bloom functions and necessary call-order dependencies. Keep Sensory Bridge deposited-RGB history and Emotiscope scalar-history/current-colour behavior as separate named selections.

Record source/settings, logical geometry, reference cadence, initialization, injection, interpolation kernel, retention, edge extinction and omitted output stages. Use deterministic input sequences in the donor's declared feature units. A K1-AP adapter is a separate layer; using identical PCM with different analysis pipelines does not establish donor-frame parity.

Compare against the existing K1 Bloom/history path as well as the AP diagnostic. Reuse the retained research probes where they answer the question. Show a single impulse, two nearby impulses, a swell, a held value, a palette change while a trail is travelling, and an interrupted parameter change. The palette-change example must visibly distinguish the two history semantics.

GO: source-grounded reference behavior and a playable comparison. A candidate cannot proceed under a misleading donor-fidelity claim; repair only the mismatched mechanism and retain the failing witness.

### FX-02 — author K1 ribbon material

Use a bounded persistent representation. An attack deposits or shapes material at the centre; already travelling content continues evolving when a later event arrives. Define what merges, survives, is attenuated or is omitted when events are dense. Never overwrite all visual identity on every detected onset.

Keep the legacy coupled donor profile. Add an explicitly named K1 profile with controllable physical transport velocity, spread and lifetime. State whether decay is time-based or distance-based. Changing speed must not accidentally change all other material properties unless the selected profile explicitly couples them.

Design the two-ribbon grammar as interacting moving material: outgoing body, controlled separation, reduced return weight and an actual quiet interval. Width, front/tail shape, acceleration, launch spacing, occupancy and extinction matter together. A sequence of three global brightness ramps does not implement it.

Colour has ownership and memory. Declare whether a palette edit recolours retained material or affects only new deposits. Use the user's selected palette, restrained accents and slower bounded colour evolution. No automatic rainbow cycling; no white peaks used to conceal clipping.

Immediate musical accents and slower material evolution have different roles. Use validated onset/energy/chroma/saliency information already available. AP phase may coordinate timing or texture; it must not be the sole source of life. Do not call heuristic kick/snare fields isolated instruments or claim chroma is chord recognition.

Store state per logical visual channel. Use centre79/80 for160-pixel logical geometry through the actual geometry abstraction. Two physical80-pixel DIN halves of one160-pixel channel are not the top and bottom product channels. Top/bottom handoff may be developed in a clearly labelled dual-channel preview after this material is established; do not invent a second live product channel.

### FX-03 — preserve the appearance through the complete pipeline

Retain fractional geometry, colour and history until the actual quantization boundary. Inspect current source for narrowing; do not assume the23September audit still describes every path. Expanding Pixel8 by257 cannot recover discarded fractions.

Declare stage order per profile. For deposited-RGB history, sample colour before depositing and transporting the material. For scalar/live-colour history, transport intensity before colour mapping. Then declare composition, appearance treatment, master/aggregate limit, quantization and native staging. Preserve existing compatibility output and identity correction. Do not blindly apply donor gamma on top of another transfer.

Judge gap depth, impact and tails after persistence, blur, atmosphere and final treatment. A gap in a primitive that the final pipeline fills is a failure. Smoothing must not erase attacks or make every passage continuously luminous.

Keep bounded storage and no steady-state heap allocation. Use the actual Titan portable C++ APIs; do not import RenderContext/modifier-slot APIs from an unrelated older fork. The k1-effect-development skill explicitly corrects that old API. Current repository authority supersedes its historical board/flashing examples.

### FX-04 — put art direction into the existing authoring loop

Extend the current replay and audition packaging to consume actual C++ full-pipeline RGB frames. The browser is a viewer/editor for the same renderer, not an independently coded visual imitation.

Provide a small focused comparison: official RGB-history reference, official scalar-history reference, incumbent K1 and the new K1 profile. Use the same input and declared mapping. A plain LED view should remain available beside any illustrative diffuser view. Do not use aggressive CSS bloom or independent normalization to manufacture impact.

Expose the meaningful parameters with units, reset, snapshot and fast keyboard focus: transport, spread, lifetime, injection response, gap, return strength, palette behavior and optional treatment. Reuse existing canonical controls; do not create conflicting ownership or alter physical SMC mappings during this host work.

Keep two comparison views:
- Diagnostic matching for isolating mechanisms, explicitly labelled.
- Authored playback with a fixed shared master/peak ceiling and measured whole-excerpt energy, occupancy and clipping. Preserve intended within-phrase dynamics; do not force every authored frame to the same energy.

Reuse song-0 and the existing sparse re-entry passage at23.010s, plus the other fixed PCM fixtures. song-1 has now been delivered to the operator; preserve its original role and do not tune on it. If it becomes an iterative art-direction input, record that change and reserve another excerpt before claiming held-out generalisation.

The authoring loop must make weak results obvious. No dashboard expansion, new framework or exhaustive parameter sweep before one recognizable donor material and one complete K1 candidate are visible.

### FX-05 — physical judgement and adoption

Host reference fidelity, causal playback and property tests permit target integration; they do not prove visual impact on the LGP. The platform owner reconciles active runtime changes and current admission, builds the exact candidate and uses the existing authorized deployment procedure.

Preserve continuous autonomous output, brightness24/255, UID545433931bd25436593630352d068363, P601/GPT6 and P604/GPT8, microphone DMA1/2 and LED DMA0/3. No backend or capture rewrite belongs to the effects owner. A genuine target problem stops only its affected operation; it does not erase completed host development.

Compare on the actual intended LEDs/optics with stable viewing and, when recording, fixed exposure/white balance. Show equal-setting and matched-exposure comparisons; disclose software cadence and preview transfer. Do not claim calibrated optical brightness from a channel sum or camera value.

The operator supplies the appearance verdict once there are concrete playable candidates. Ask targeted comparisons at prepared timestamps: Does the outgoing material read as travelling? Is the gap actually quiet? Is the return subordinate? Does a sparse passage stay expressive? Does dense material preserve hierarchy? Is the change musically relevant without watching the analysis graph?

Report the preferred and rejected profiles with reasons. A disliked result stays visually unaccepted despite passing numerical gates. Repair the specific appearance defect against the chosen reference, not random combinations of unrelated features.

## Following effects, in order

1. Complete and judge the Bloom/ribbon profile above.
2. Spectronome-derived composition: meaningful spectral body plus limited phase-family foreground, with spatial hierarchy, distinct visual roles, continuous motion and graceful fallback. Do not use all candidates as equally prominent rings.
3. Kaleidoscope-derived timbral material: smooth continuous fields controlled by relative spectral activity and slower envelopes, giving held/non-beat music a different expression.
4. Build staged top/bottom choreography from the accepted material and established channel topology. Reuse the previous six-study work; do not restart six generic demos.

These are the onward queue, not simultaneous prerequisites for finishing the first profile.

## Acceptance and ownership

| Requirement | Evidence and decision |
|---|---|
| Reference fidelity | Pinned functions/settings, retained-state vectors and declared call order; original and adaptations visibly distinguished. |
| Coherent material | Nearby attacks do not erase all history; parameter edits do not teleport or reset unrelated state; cadence variation has bounded declared behavior. |
| True negative space | Quiet interval and weaker return remain after complete treatment. Measure descriptors, then judge actual output. |
| Musical relevance | Fixed full-PCM comparisons and causal timing; valid/absent phase cases both meaningful; event heuristics labelled honestly. |
| Useful visual impact | Operator chooses against concrete reference/current/candidate views and states why. No metric substitutes for this. |
| Real output | Actual C++ pipeline, preserved fractional values, target resource admission, mapped native frames and physical observation before optical claims. |
| Regression containment | Existing AP trajectories, compatibility profile, controls, true-black behavior and hardware ownership preserved. |

Effects owner: new portable material/primitives, mode adapter and authoring integration.
Reference/evaluation owner: donor correspondence and comparison design, independently reviewed.
Integration owner: one writer for shared renderer/state/output-treatment surfaces.
Titan platform owner: runtime/driver/capture/target integration.
Operator: artistic preference and direct physical judgement.
Named native implementing agent: unassigned at lane opening; no external agent launch is claimed.

Use existing VP/MOD/IVV requirements and registry when implementation is bound. Never rewrite the parent plan or manufacture an all-PASS receipt. Record actual changed source, build/profile/input identity, outcomes, failures and remaining gates.

## Immediate handoff

From this worktree, read this file, reference/appearance-transfer-original.md, the retained Connor teardown and existing VP chapter. Start FX-01/FX-02 in the existing C++ renderer/replay path and produce the first concrete material comparison. Continue through full-pipeline audition and the applicable physical handoff; do not return another proposed architecture as implementation.

The lane-opening turn created the worktree and this assignment, copied the established reference/input material with hashes and corrected the stale fault assessment. It did not change render code, run new effect tests, flash Titan or claim a new effect is implemented.
