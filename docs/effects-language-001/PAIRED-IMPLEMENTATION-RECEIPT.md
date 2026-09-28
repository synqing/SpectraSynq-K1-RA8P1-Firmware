THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

# Paired material implementation — revision004, not visually accepted

Later additive execution: [paired hierarchy revision005](PAIRED-HIERARCHY-005.md). Preserve this earlier receipt as history.

28 September 2026. This additive receipt follows DUAL-EDGE-CORRECTION.md and preserves the original FX and VP/MOD/IVV assignments in full. No completion or optical acceptance is claimed.

Captain rejected the revised single-channel material as “Still too mechanical or repetitive”, and the first paired candidate as “Still unconvincing; the form or motion needs changing”. Both witnesses remain available. The current retained paired material is awaiting a new verdict.

## Playable result

http://127.0.0.1:8769/paired serves the actual C++ comparison. The original single-channel donor/incumbent/AP comparison remains at `/`. Whole-K1 scenes contain top160 and bottom160 arrays, each centred79/80. The viewer shows raw edge rows around an explicitly illustrative plate. Top-only/bottom-only isolation changes that illustration, not the exported data. Shared exposure maps code6168 to browser255; authored frames are not normalised to equal energy. Diagnostic energy matching is a separate selection.

Current executable: `out/paired-retained-verification-004/build/ribbon_replay`.
Default song-0 export: `out/paired-retained-verification-004/playback/a7345e2824f27903183b/`.
Offline audition: `out/paired-retained-verification-004/K1-paired-song-0.html`, with original PCM, actual final frames, rejected paired frames, identities and frozen settings embedded. Its sibling JSON records hashes. The current package was opened directly as a local file and browser-tested with working audio/seek and zero network requests (`paired-offline-final.json`). The earlier full package check also exercised isolation/snapshot and disabled rerender controls.

The three scenes are the pinned Cross-Strip Wave kernel, existing ribbon deliberately duplicated onto both edges, and the new retained paired candidate. The checkbox restores the rejected paired candidate for song-0 at the same timestamp/exposure. Song-1 has not been used for tuning.

## Source and implementation

`tests/reference/cross_strip_reference.h` adapts the exact pinned Cross-Strip kernel: phase offset0.375cycles, wavelength12pixels, drift0.12Hz, context speed50, intensity255, brightness24,120Hz calls and original safe-dt clamp. Original CRGB output is widened by257; this is not recovered precision. Source, licence, NOTICE and dependency provenance are retained under `reference/lightwave-cross-strip/`. The extracted original helpers/init/render match1800 frames, including parameter changes and dt-clamp cases. An intentional edge-collapse mutation fails. Full Lightwave renderer, film, calibration and hardware parity are not claimed.

`src/k1/core/visual/paired_wave_scene.{h,cpp}` owns both edges and advances once. The rejected repeating field was replaced with144 retained centre-deposited material knots. The Lorenz source's retained distance kernels, broad body and narrow same-colour core inform the material; Rose's different musical timescales inform response. The Lorenz trajectory controls formation, edge emphasis and a bounded common velocity variation. It does not set on-strip positions. This is a deliberate adaptation, not an original-kernel parity claim.

New positions start at the centre and move outward with positive physical velocity. Actual velocity is base × (0.65 +0.35 × envelope +0.15 × formation), so the transport control is labelled base velocity. The related other-edge deposit starts after a controlled delay. After complete departure, each edge waits before a reduced inward return. Isolated-attack final-frame tests verify a quiet interval and subordinate return. Dense overlapping material may keep other parts of the plate occupied. The candidate has high whole-excerpt occupancy; this remains visible and is not an aesthetic success metric.

Deposited RGB owns its colour. Palette/width edits affect new material. Velocity, diffusion and decay edits change future evolution without resetting current positions. Formation events occur at24Hz boundaries, independent of presentation. Decay is time-based, with smooth retirement before5.8s maximum age to bound storage. Excess deposits are omitted and counted, never allowed to erase travelling material. No rainbow cycling, white clipping camouflage, extra gamma, blur, atmosphere or copied edge-collapsing film pass is applied.

Settings/defaults and stage order are in FX-PROFILES.json. The authoring controls include width, transport, handoff delay, articulation, spread, half-life, quiet interval, return strength and selected palette. Invalid fractional palette IDs and out-of-range controls are rejected. No SMC mappings changed.

`ribbon_profile.{h,cpp}` adds the bounded mode adapter and an explicit post-master native-frame view. Liveiness0/0.5/1 gives0.5/1/2 times base transport, clamped to the physical range; base values, decay, width and AP data remain unchanged. Its producer-to-wire tests prevent a second brightness multiplier. `stagePaired` similarly stages two960-byte full-edge streams and tests every swap/reversal using asymmetric sentinels. The current split bench topology has not been redefined as the product.

## Executed verification

- `verify_ribbon.py --output out/paired-retained-verification-004`:45,370 final frames over ten permitted inputs, including nine full PCM fixtures and six deterministic feature studies. ASAN/UBSAN, fixed-capacity/no-allocation, retained palette ownership, same-time edits, isolated departure/gap/return, stale input, mirror geometry,24/255 ceiling and native staging pass. Song-0 repeats byte-for-byte.
- The two-second cadence fixture uses the same40Hz publications at30/60/120/240Hz presentation; final outputs differ by at most one native16 code. This is bounded host evidence, not a universal timing or optical claim.
- `paired-compatibility-004.json`:all five original single-channel output lanes and causal metadata are byte-identical to the earlier flow replay on all ten inputs. Protected AP/platform/diagnostic sources also remain byte-identical to the lane base.
- `verify_ribbon_arm.py --output evidence/arm-paired-retained-004`:scalar ARM objects build with warnings as errors and no allocation symbols. Paired state12440bytes, ribbon profile13976bytes. Paired render's compiler stack entry is2040bytes, not measured whole-call-chain high-water. Whole-image RAM, WCET and physical latency are not admitted.
- An ARM aggregate-initialisation failure was retained, repaired by an explicit `Knot{}` temporary, and rebuilt. The repair's song-0 visual/native bytes were unchanged.
- `paired-final-browser.json` and `paired-final-18.png`/`paired-final-23.01.png` record current viewer checks; the offline witnesses are `paired-offline-final.json` and `paired-retained-current-browser.json`.

Host execution receipt: `1aad7fc28ee4ff97653df92030e83e985223631baf4a3f02502c369173ce7702`.
ARM execution receipt: `80f291b5ab8104b6e3410bbe4fe4d9492f40b7c0642db5c53a2617eae5c4cdd5`.
Actual source hashes: `evidence/paired-source-manifest-004.json`; build/input identities are retained with each export. Passing commands do not admit the broad parent assignments.

## Ownership, admission and physical boundary

Primary native Codex is the implementation/integration writer for the new portable modules, tests and authoring files. No external agent launch succeeded: the native guard rejected the attempted independent reference worker as `UNINSPECTABLE_BRIEF`. Independent reference/IVV review therefore remains open.

Existing ORCH-0 now has an additive frozen documentary observer. Four real cases pass: retained inputs/base, full assignments/owners, actual native binding and wrong-source/session rejection. The original task IDs/text/dependencies remain unchanged. `workflow_accept` refuses `Incomplete candidate identity`; its source-coverage flag is false. The latest bound witness is `evidence/orch0-admission-obstacle-002.json`, referencing execution `9a5b9920ceca59a85892dda902330c24f42d08a746d95d08af66072d15f26878`. ORCH-0 is recorded as BLOCKED_EXTERNAL for formal admission only. The gate permits a blocked handback with zero completed task IDs; this is not completion. Read-only inspection identifies the installed snapshot's 64 MiB untracked-file budget (`surfaces/integration.py:132`). No acceptance overlay, invented PASS, session reset, ignore change, governance code change or unrelated root cleanup was used.

No firmware was flashed, reset or reconfigured by this continuation. The previously recorded resident target remains a separate build. Fresh physical execution would require a new current identity/admission check. The earlier read-only witness found clock-origin incompatibility and22/64 AP timings above7.5ms; the AP lane is separately paused. This is not a claim that the old white/blue optical defect returned.

Remaining ordered path:
1. Obtain the operator's current material verdict and repair the specifically rejected appearance; complete independent source/evaluation review when the native dispatch guard permits an inspectable assignment.
2. Resolve source-identity coverage for formal ORCH admission and complete applicable remaining VP/CTL/MOD/IVV contracts without compressing or relabelling their original assignments.
3. Platform owner reconciles the paused runtime/clock/admission work and actual two160-pixel physical mapping. Integration binds this post-master profile once, preserves compatibility/controls and proves full-image resource/timing admission.
4. Use the authorised identity-gated E2 Lite procedure for the exact full image, restore autonomous output on application return and perform the bounded optical comparison. Preserve UID545433931bd25436593630352d068363, brightness24/255, P601/GPT6, P604/GPT8, micDMA1/2, LEDDMA0/3 and probe target-powerOFF.
5. Record preference/rejection and the physical result. No optical acceptance or later effect-family completion is implied by this host work.
