# Continuous body/detail Bloom — running correction and retained Revision 2 record

THE TITAN MUST ALWAYS BE VISUALISING MUSIC.

[Latest physical comparison and live-state update](CONTINUOUS-BLOOM-PHYSICAL-20261012.md): camera restored; paired footage captured; subsequent spacing run stopped for competing control activity.

## Retained deployed correction — 12 October 2026, 04:09 Perth

Source commit `7ae880ef05e13912bead58a73a520ad862507ca9`; resident build
`4c94653ddc3d9d2a6a56500e1422d953a48c0be50b0486d7deaec55f6e3a43e4`.
The body-level calculation now applies the selected attack/release once.
Previously it multiplied VU by an already smoothed transient and smoothed that
product again. The duplicated response weakened early material and could keep
the injected level rising after a short transient ended. The correction feeds
the raw transient target into the existing VU envelope. Spectral interpretation,
detail, palette, history transport and physical output limits are retained.

This is an implemented response correction; **physical appearance remains
UNVERIFIED**. The intended visible consequence is more immediate body-led material
from short musical hits. No preference or optical improvement is claimed.

The new step/impulse regression failed on the retained code and passed after
the correction. The existing donor-PCM-to-GRB48 run still passes fresh attack,
silence, stale-input and independent-control checks. Detail output matches the
retained production trace byte-for-byte; body output changes. Existing
conditioning checks pass. All 207 ARM receipt inputs match the source commit.

E2 Lite programming ran 04:09:06–04:09:31 Perth; application identity matched
at 04:09:41. Programming interrupted the show; photon interruption duration was
not measured. The first deployment invocation stopped before hardware access
because the default Python 3.14 XML dependency was broken. The supported
`TITAN_DEPLOY_PYTHON=/opt/homebrew/bin/python3.13` override passed its imports and
completed the unchanged deployment procedure.

SQ4 recall restored the full composition. Its documented exclusion of physical
controls required a separate restoration of the pre-flash settings: both board
gains 0.25, secondary enabled, master 100/255, emission ON, precision 0. All preset
fields and physical settings were then read back. The approved 24-second Sonic
Samba passage completed on the installed candidate. Four fresh existing
bottom-submission records and both boards' state were retained; these do not
constitute an optical comparison. Final readback identifies the new build,
both configured outputs remain enabled, and reported LED faults are zero.

Evidence: `../../_scratch/continuous-bloom-r3-single-attack/` contains
`red.log`, `green.log`, `targeted-result.json`, `arm-001/receipt.json`,
`candidate-admission.json`, `programme-001/receipt.json`,
`post-recall-status.json`, `preset-restoration.json`, `live-audio-run.json`
and `final-health.json`. The source changes are limited to
`platform/ra8p1/musical_conditioning.h` and
`harness/tests/test_continuous_bloom.cpp`.

Current EiP inventory still contains no product camera. This blocks the physical
comparison, not the completed source correction or normal autonomous output.
Do not repeat the closed smoothing repair, reflash this same candidate, or expand
profiling to fill that gap. Preserve this installed state and the retained image.
Next physical operation remains the original matched plate comparison once the
authorised product camera is connected and shows the actual K1 stack. All existing
Revision 2 permissions, bans and unfulfilled physical criteria remain in force.

## Retained Revision 2 delivery record — superseded running identity only

12 October 2026, Perth. **Installed engineering candidate; physical delivery incomplete. Aesthetic preference PENDING.** This executes the 11 October Revision 2 order only. No new effects, choreography, analyser, ML targets or UI were added.

The authorised EiP camera produced an indistinct brown surface with no identifiable K1 stack and stopped early with AVFoundation `-11808` (`RecordingStopped`). The owner opened and inspected its decoded frame and subsequently a contact sheet spanning the recording. The entire available view lacks an identifiable K1 plate. The file is truncated to 1.73 seconds; the error reports a finalised playable file, which does not establish a successful full-duration capture. No valid baseline/candidate plate comparison exists. Firmware and controls remained independently executable and were completed. There is no claim that this is a better physical lightshow.

## Running identity and recovery

- Firmware repair commit: `b32fde5` in the existing `titan-emotiscope-port-001` checkout.
- Installed build: `2b8daa2f45031b4041382373238370b977af1bac51d99d31c1bd7101e664e980`.
- UID: `545433931bd25436593630352d068363`.
- Wire schema: `6d68932e22c0796bcac290a233df9e235410b7af724ac71112525a86017b561b` (unchanged; new publication fields are internal).
- Actual info readback: [post-programme identity](../../_scratch/continuous-bloom-r2/programme-001/post-info/handoff-info.json). [Build receipt](../../_scratch/continuous-bloom-r2/arm-002/receipt.json), [programmer receipt](../../_scratch/continuous-bloom-r2/programme-001/receipt.json), [deployment log](../../_scratch/continuous-bloom-r2/deployment.log). Programming completed with the established UID-bound E2 Lite, 1 MHz, probe-power-OFF procedure. Application interruption was approximately 24 seconds during programming; autonomous output returned.
- All 207 build inputs match the current local files, including the external defaults file: [source readback](../../_scratch/continuous-bloom-r2/installed-source-readback.json).
- Retained image: build `47bc25b5f69c9e27c90dfe089343412569825beef07bed1a4f94e47718ef28d4`, [old receipt](../../_scratch/composed-scenes-arm-003/receipt.json), [old admission](../../_scratch/composed-scenes-live/accepted-candidate-final.json). Original running settings were preserved before changes in [baseline-status.json](../../_scratch/continuous-bloom-r2/baseline-status.json). No unrelated work was removed.

Configured routing is unchanged: bottom board uses P601/GPT6/DMA0 and P604/GPT8/DMA3, top uses P603/GPT7A/DMA6 and P714/GPT12B/DMA7. Each board is one 160-pixel field split into two 80-pixel DIN segments. Those segments are not separate optical channels. Microphone P502/P812 and DMA1/2 are unchanged. The assembled optical mapping was not freshly verified through the failed camera.

## What is running and saved

[Continuous Body/Detail Bloom preset](presets/continuous-body-detail-bloom.json): scene 3, unlinked, automatic phrase policy OFF, board-scoped quiet, body role 1 / detail role 2, interpretation strength 1 on both. Both use palette 1; ML influence, background and screensaver are OFF only in this comparison configuration. Existing inference and other presets remain available.

Travel is 1200 ms outward and 1800 ms return; widths 0.20 / 0.14, return strength 0.55, trail 120 ms, one return. These are retained settings, not an optical tuning result. Bloom remains continuous: ribbon separation, timed ribbon quiet and ribbon recovery do not impose an admission barrier on this preset. Silence comes from valid amplitude/capture evidence.

Master output remains **100/255**, both board intensities **0.25**, secondary output ON. No gamma, current limit, colour correction, LED count, pin or routing changes. Edge Response defaults OFF/depth 0 and is editable per board.

Saved as **SQ4** through the existing Surface authoring path. This is a complete **host-saved** two-board composition, not device nonvolatile storage. A live body Edge Response edit was read back, then SQ4 recall restored every authored field on both boards and their relationship: [recall and leave-state](../../_scratch/continuous-bloom-r2/recall-and-leave-state.json). Power cycling still follows the existing autonomous firmware boot defaults; SQ4 is recalled through the existing host controls.

## Exactly how audio becomes material

1. The existing donor consumes 64 PCM samples per analysis hop at 12.8 kHz (5 ms). No analyser was replaced. The current render period is 10 ms.
2. The producer now publishes its actual last PCM timestamp and bounded maxima of squared PCM amplitude, body-band energy and normalised raw VU since the preceding publication. A loud first hop is no longer overwritten by a quiet second hop before rendering. This is one bounded summary, not an event queue. Both boards receive the same immutable generation.
3. Body weights are 1 up to 180 Hz, falling linearly to 0 at 700 Hz. Detail weights rise from 0 at 250 Hz to 1 at 1400 Hz. Body consumes current spectrum plus the producer-observed body peak; detail consumes smoothed spectrum. These are frequency/temporal interpretations, not instrument classification.
4. Body target is `clamp(4 * positive_body_change * 0.01 / elapsed_seconds + novelty * body_energy)`. Detail target is weighted high-band energy. Each board owns its attack/release envelopes and event cursor. The preset uses 40 ms attack and 160 ms release. Followers integrate the timestamp difference since **their own last consumed audio generation**; duplicate render publications reuse cached features without advancing an event.
5. At full interpretation strength, the injected level uses the existing conditioned VU envelope. Body's VU input now includes the retained within-publication VU peak; detail uses the sustained VU view. The level is multiplied by that board's response gain and the existing scene injection factor. No cosmetic minimum level was introduced. Full-strength body output remains relatively low in the measured payload; this has not been declared optically satisfactory.
6. Silence compares **squared PCM peak against squared donor noise floor**, with the existing small squared-amplitude floor. It no longer compares normalised VU with an unnormalised squared floor. Invalid capture, producer age beyond 30 ms or below-floor input stops new injection. Existing colour history continues travelling and drains.
7. Bloom retains RGB in its existing 25 ms history samples. Every render can contribute a peak to the current unfinished sample, so a short attack between sample boundaries survives. Completed samples retain their colours. Missing render intervals receive empty samples, never backfilled copies of the latest attack. The first attack after a fully drained rest is retained too.
8. Colour selection remains the existing authored palette traversal over 1.6 seconds. It is **not** pitch identification or song-section recognition. Outward/return travel and retained RGB remain the existing Bloom transport. New music adds material while previous material travels; there is no five-second scene-busy admission barrier.
9. Edge Response runs once on the displayed frame, **after brightness and before tone mapping**, then existing warmth, white balance, quantisation and output adapters. It never modifies stored colour history. Its 40 ms response envelope and mode/depth belong to the selected board. Mode 1 uses novelty; mode 2 uses spatially mapped spectral energy. Gain is `1 - depth * level * (1 - response_envelope)`.

## Evidence and fixed criteria

| Criterion | Engineering result | Physical result |
|---|---|---|
| Fresh music during ongoing travel | PASS in actual donor PCM-to-GRB48 harness: later attacks at 0.8-second intervals within five seconds change centre material; a pulse wholly between the old 25 ms capture ticks survives | UNTESTED through stack |
| Independent body/detail | PASS: different temporal output; changing only body Edge Response leaves every detail payload byte identical under deterministic input | Optical distinction UNTESTED |
| Controls and silence | PASS: response changes body payload; zero PCM and stopped PCM both drain both histories; first pulse after rest survives; live control/recall readback matches | Physical silence/appearance UNTESTED |
| Optics preserve design | NOT ESTABLISHED | BLOCKED by camera view/capture failure |
| Timing | PASS for bounded observed workload: 10 ms render period; no added publication misses or skipped LED releases in either comparison window | Microphone-to-photon latency NOT MEASURED |
| Quality | Engineering evidence is scoped as above | Aesthetic PENDING; no preferred/accepted claim |

Timing used the same already-approved `sonic-samba-20-44s.wav` passage, normal host playback path and unchanged output controls. Audio SHA-256 `32b74fc59786a9696303415a011fa4c5fc39975032e969a36cd09ea6f118fe22`. Microphone capture is not bit-identical between acoustic replays.

| Sampled render duration | Retained image | Candidate |
|---|---:|---:|
| Minimum | 1.533 ms | 1.544 ms |
| Median | 1.548 ms | 1.567 ms |
| Maximum observed | 1.675 ms | 1.689 ms |
| Added audio-publication deadline misses | 0 | 0 |
| Added skipped LED releases | 0 | 0 |

These are 113/112 sampled firmware cycle observations at the current 1 GHz clock, not exhaustive worst-case or photon measurements. The audio-publication budget is separately 5 ms. Capture/publish timestamps are software observations (`physical_ts_valid=0`), not a measured microphone-to-photon path. [Timing comparison](../../_scratch/continuous-bloom-r2/timing-comparison.json), [baseline LED counters](../../_scratch/continuous-bloom-r2/baseline-led-timing.json), [candidate LED counters](../../_scratch/continuous-bloom-r2/candidate-led-timing.json). Baseline LED-window boundaries were reconstructed from the capture file completion time; records bracket the passage.

Actual bottom-board submission captures: [baseline](../../_scratch/continuous-bloom-r2/baseline-payload/capture.json), [candidate](../../_scratch/continuous-bloom-r2/candidate-payload/capture.json). Both contain 101 changing payloads. These taps cover the bottom board's two DIN segments, not both optical boards. Top-board isolation is covered by the deterministic production-path harness, not inferred from this tap. Buffer changes do not prove better light.

Minimal causal proof: [test source](../../harness/tests/test_continuous_bloom.cpp), [runner](../../harness/tests/test_continuous_bloom.py), [commands/output](../../_scratch/continuous-bloom-r2/host/commands.json), [result](../../_scratch/continuous-bloom-r2/host/result.json). It executes real donor PCM analysis, both render instances and final GRB48 packing with address/undefined-behaviour checks. Existing conditioning analytical tests and their gain-only mutation check passed. Existing motion/history tests passed; their deliberately shortened tail fault was rejected. The old sparse-render test expected invented deposits during 1.5 seconds without observations; it now checks valid 10/25 ms cadence equivalence and explicitly rejects such backfill, as Revision 2 B8 requires.

The owner inspected the changed source and consumer outputs. The existing motion test raster was opened: retained coloured material travels and returns, then clears in that host trace. This is only history-regression evidence, not a physical K1 preview or substitute for the required film-stack recording.

## Capture repair, failed attempts and remaining operation

The existing broker's payload read required a legacy checkpoint even though the running candidate already passed the live UID/build/schema admission. The scoped repair reuses that existing admission for this bounded read. Wrong UID/build/schema cases remain rejected; output never disarms. This adjacent broker change is necessary to use the existing capture seam, not a replacement capture framework.

Retained failures/limits:

- The first controls request used `mode` instead of the existing `current_mode` field and was rejected without changing settings; corrected through the existing API.
- The first timing ownership handoff raced mixer shutdown and received `writer_busy`. Waiting for shutdown before acquiring the broker connection corrected it; no ownership bypass was used.
- Four capture tests initially hit macOS AF_UNIX path length limits; a shorter temporary test root corrected the environment. The focused capture suite then passed, including identity mismatch rejection.
- The legacy payload scorer omits board intensity and reported 242 scaling-byte mismatches. Applying the unchanged actual 0.25 board-gain formula reproduced every captured byte. The failed legacy score is retained, not relabelled PASS.
- The optional Grok 4.7 xhigh worker produced no patch after 14 minutes and was stopped. The integrating owner implemented and reviewed this candidate. A 500k context allocation was not verified.
- Governance recorded the actual production-path command successfully. Formal `workflow_accept` refused because no task registry is bound to this session; no replacement registry or fabricated acceptance was created.
- [Camera failure log](../../_scratch/continuous-bloom-r2/camera-preflight.jsonl), [failed recording](../../_scratch/continuous-bloom-r2/camera-preflight.mov), [inspected frame](../../_scratch/continuous-bloom-r2/camera-preflight.png). This is failure evidence, **not the requested paired physical clip**.

Remaining delivery path:

1. Restore a usable EiP feed of the assembled K1 light-guide/film stack. No further capture retry is justified until that condition changes.
2. Record retained baseline and candidate with the same approved passage, cue, level, stack, position, locked exposure and white balance, alongside the existing payload/timing observations.
3. Inspect the actual plate footage; repair any observed defect within P1–P8, one explained mechanism per comparison. Do not raise output limits or substitute software evidence.
4. Deliver that concrete pair for Captain's aesthetic decision, then stop this cycle. All parked work remains parked.

Device leave-state: candidate image running, continuous autonomous audio-reactive output ON, both boards enabled at the unchanged operating gains, SQ4 settings recalled, broker and mixer healthy, LED faults zero. [Final health](../../_scratch/continuous-bloom-r2/leave-health.json). This is an installed engineering increment, not completion of the required physical performance.

## Continuation readback — 12 October, 03:31 Perth

The camera is now unavailable at device discovery: AVFoundation lists only the MacBook Pro Camera and no EiP device. Camera permission remains authorised (`3`). [Current inventory](../../_scratch/continuous-bloom-r2/camera-continuation-inventory.json), [inspected recording contact sheet](../../_scratch/continuous-bloom-r2/camera-preflight-contact.png). The exact blocked operation is opening persistent device `EB6E6241-CD5C-4917-91B3-7E6500000001` to record the assembled K1 stack. There is no connected product camera to repair or operate through the existing capture path. No blind recording retry or camera substitution was performed.

Fresh control readback confirms every field of the saved comparison preset matches both running boards, including unlinked scene 3, body/detail roles at strength 1, ML influence off and independent histories. Output remains enabled at master 100/255 and both board gains 0.25. [Live settings](../../_scratch/continuous-bloom-r2/continuation-status.json), [field comparison](../../_scratch/continuous-bloom-r2/continuation-preset-readback.json). Health reports 100 frames/s and zero LED faults. Firmware remains the installed repair build above. No physical acceptance is claimed; the remaining numbered delivery path above still applies.
