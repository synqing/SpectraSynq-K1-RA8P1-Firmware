THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS. IT MUST NEVER NOT BE VISUALISING MUSIC EVER!

# K1 dual-edge effects — correction and source-grounded execution addendum

28 September 2026. Lane: titan-effects-language-001. Scope of this receipt: source inspection and correction of the existing assignment; no new effect implementation or physical acceptance is claimed.

## Current authority and correction

Captain: “There are 2 (that's what a K1 is built with) led strips with 160 leds each edge lighting the top and bottom long edges of a light guide.”

The product is ONE light-guide plate driven by TWO 160-pixel edge fields, 320 pixels total. Each edge has a centre pair at 79/80. Within-edge left/right symmetry and the relationship between the top and bottom edges are separate design decisions. Identical edges are a legitimate authored choice; they are not a compulsory topology. Physical wiring orientation must be mapped separately from logical scene coordinates.

This supersedes START-HERE.md's instruction to establish a single-channel material before developing the top/bottom preview, and its queue item deferring top/bottom choreography until fourth. It also supersedes any older skill shorthand saying the product is 160 total / two 80-pixel edges. A split bench channel does not redefine the product. Preserve the full original assignment and parent VP/MOD/IVV requirements; this addendum changes the geometry, source priority and immediate execution order.

The initial brief was my error: it allowed the central product affordance to become later work. Keep the existing ribbon implementation as useful material work and a comparison baseline, but it is not yet a complete K1 effect. Do not reopen AP-A/AP-B or the resolved historical LED fault as a substitute for effects development.

## What was actually inspected

GitHub main resolved to **60009324ec3e7785c11db010343189d34e7314e0**, synqing/Lightwave-Ledstrip. The references below are implementation sources, not effect-name speculation or claims that every current version has passed an optical audition.

The supplied ribbon_audition.html (SHA-256 e7ba6008b616df9adc856492c1faee32f84a715507926a5d355cc1ee2b46d7a3) explicitly renders one 160-pixel row per comparison lane. Its backend, RGB frame files and audio are not embedded. Inspection of this attachment was source inspection, not complete musical playback.

Fresh worktree inspection found uncommitted ribbon_material.{h,cpp}, replay/tests and audition/server files. RibbonFrame and RibbonFrame16 each contain kPixelsPerChannel pixels; render() produces one internally mirrored field. Outgoing/returning objects both inhabit that same field. Their temporal relationship does not supply a top/bottom relationship. Preserve these active files and their author's work.

## Existing visual language to reuse

Paths in this table are relative to firmware-v3/ in the pinned repository. Read the complete named kernel and its output path before adapting it.

| Source | Actual mechanism | What to carry into Titan |
|---|---|---|
| [CrossStripWaveInterferenceEffect.cpp](https://github.com/synqing/Lightwave-Ledstrip/blob/60009324ec3e7785c11db010343189d34e7314e0/firmware-v3/src/effects/ieffect/CrossStripWaveInterferenceEffect.cpp) | Centre-mirrored triangular fields on each edge; independent cyan/amber outputs, 8–24 pixel wavelengths, selectable relative phase, bounded contrast and drift. Requests and declares dual-channel output. | Explicit control of edge relationship, spacing, dark intervals and colour identity. The default 0.375-cycle offset is an inspectable starting preset, not an artistic verdict. |
| [LGPMoireCurtainsEffect.cpp](https://github.com/synqing/Lightwave-Ledstrip/blob/60009324ec3e7785c11db010343189d34e7314e0/firmware-v3/src/effects/ieffect/LGPMoireCurtainsEffect.cpp) | Slightly different radial spatial frequencies on the two edges, plus a palette offset. | Compositional change through relative band spacing; the plate is driven by related fields. Preserve declared cadence when adapting its per-frame phase step. |
| [LGPRoseBloomAREffect.cpp](https://github.com/synqing/Lightwave-Ledstrip/blob/60009324ec3e7785c11db010343189d34e7314e0/firmware-v3/src/effects/ieffect/LGPRoseBloomAREffect.cpp) | Radial contour and Gaussian bands; evolving lobe/opening state, localized impact and slower colour state. Writes equal centre-paired edge patterns. | Recognizable shape with distinct musical timescales. Equal edges can still be an intentional whole-plate composition. Its experimental overlays are not automatically improvements. |
| [Lorenz Ribbon in LGPHolyShitBangersPack.cpp](https://github.com/synqing/Lightwave-Ledstrip/blob/60009324ec3e7785c11db010343189d34e7314e0/firmware-v3/src/effects/ieffect/LGPHolyShitBangersPack.cpp#L364-L453) | Retained radial positions, age-weighted distance kernels, broad body and narrower highlight; bounded colour variation and equal edges. | Material character and memory. This original kernel is autonomous, not proof of musical response; its per-render integration and target-specific storage need deliberate adaptation. |

Sensory Bridge/Emotiscope remain references for musical responsiveness and material history. Lightwave supplies the direct K1 visual and geometric baseline. Do not replace this existing library with another generic catalogue of pulses or six new demonstration effects.

## Output-path findings that affect transfer

The Lightwave renderer supports distinct edges. RendererActor.cpp:2633–2638 copies different halves of the unified 320-pixel buffer into separate strips; its “mirror memcpy” comment does not mean copying strip A over strip B. Direct strip writes require both the request and DUAL_CHANNEL metadata role. These are Lightwave APIs; Titan must use its current portable interfaces.

Some individual paths intentionally collapse the edges. LGPFilmPost.cpp:61–139 reads the first half, applies treatment and writes the same result into the second half. Do not import this into a differentiated-edge scene. SET_CENTER_PAIR also writes the same colour onto both edges. Trace actual assignments; a preceding fade call that is overwritten for every pixel is not retained framebuffer history.

Use names such as “interference”, “holographic” and “time reversal” as authored effect names. Their source does not prove those physical optical mechanisms. Browser plate rendering must be labelled illustrative unless its light-transfer response has been measured. Several source comments and historical audit conclusions overstate their mathematical guarantees; preserve the visual mechanism, inspect the math, and measure the resulting movement.

## Immediate implementation sequence

1. **Make the existing authoring/replay path pair-aware now.** Each scene output is top[160] and bottom[160], with explicit physical-orientation mapping and separate centre-pair geometry. Use one coordinated scene owner with persistent state; avoid advancing one shared stateful effect twice to obtain two edges. Keep the old single-channel diagnostic available as a diagnostic.

2. **Bring one existing paired form into the comparison before more ribbon tuning.** Use the pinned Cross-Strip Wave kernel as the first source-grounded reference. Preserve its authored field equations, colour identity, phase settings and declared time units in a reference selection. Show the current ribbon on both edges as the explicitly identical-edge baseline. Do not claim complete Lightwave firmware parity from an isolated adapter.

3. **Author one musical paired candidate from that form.** Keep a recognizable spatial structure across attacks. Give available musical inputs distinct bounded roles: slower energy/timbre can shape field width, contrast or drift; salient attacks can create localized articulation; admitted rhythm may organize relative timing. Use only actually available validated inputs and retain useful response without phase lock. Preserve hue ownership, material memory and negative space. A uniformly louder copy or independent random movement on each edge is not enough.

4. **Make the comparison show one whole K1 per candidate.** Display top raw pixels, one plate view and bottom raw pixels together. Expose top-only, bottom-only and combined contributions; clearly mark independent-edge timing/phase and whether edges are identical. Use actual C++ final frames, consistent display exposure and the shared 24/255 ceiling. Keep authored dynamics intact; any aggregate energy matching is a separately labelled diagnostic. An illustrative plate view helps authoring but does not pass optical acceptance.

5. **Package playable evidence.** Include audio, rendered pairs, metadata/settings and a static playback entry, or a verified reachable serving arrangement. A lone HTML file requiring absent /config, /render and frame assets is not a portable audition. Retain the existing fixed music, sparse re-entry at 23.010 s, phase-absent passage and held-out discipline. After the first paired candidate, use Moire Curtains, Rose Bloom and Lorenz material mechanisms to guide targeted improvement, not as simultaneous prerequisites.

6. **Complete the existing target/optical handoff when the host candidate is ready.** The platform owner reconciles active runtime work and actual output mapping through the established admission procedure. Do not claim product geometry is physically exercised on a differently wired bench. Preserve continuous autonomous music output, brightness24/255, UID545433931bd25436593630352d068363, P601/GPT6 and P604/GPT8, microphone DMA1/2 and LED DMA0/3. Hardware constraints and target checks remain those of the original assignment.

## Acceptance specific to this correction

| Criterion | Required evidence |
|---|---|
| Correct geometry | Both 160-pixel final edge arrays present; 79/80 symmetry and travel direction checked independently of physical orientation. |
| Edge relationship survives | Deliberately unequal source fields remain unequal after treatment/staging; identical-edge preset remains deliberate. A swap/mirror witness catches accidental mapping errors. |
| Useful comparison | Source reference, current ribbon baseline and one musical candidate can be played with synchronized audio as whole-K1 views; edge isolation works. |
| Visual and musical value | Operator can identify form, articulation, spatial contrast and musical relevance without watching analysis graphs. Failures remain visible; a build or numerical difference does not pass this criterion. |
| Optical claim | Actual intended dual-edge light-guide observation at declared settings. Host inspection alone leaves this NOT TESTED. |

The effects owner continues the existing implementation lane from this correction. This review did not notify a separate native agent, change renderer code, run new effect tests or flash Titan. The original assignments remain open; this document is a corrected execution input, not an effects-completion receipt.
