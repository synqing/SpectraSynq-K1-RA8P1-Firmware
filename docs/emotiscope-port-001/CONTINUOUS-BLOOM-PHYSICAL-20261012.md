# Continuous Bloom physical comparison — 12 October 2026

**Physical comparison captured and inspected; overall delivery remains incomplete.** The camera-access blocker is resolved. The later spacing correction stopped because unrelated controls changed during its recording. No aesthetic preference or complete engineering acceptance is claimed.

## Review the actual plate

- [Matched music comparison](../../_scratch/continuous-bloom-r2/physical-001/baseline-candidate.mp4): retained baseline above, candidate below. Same 24-second Sonic Samba excerpt, host playback path, master 255/255 and 0.25 board gains. One continuous EiP recording holds framing, exposure, white balance and focus across both passages and the intervening programming operation.
- [Causal probe](../../_scratch/continuous-bloom-r2/physical-001/causal-001/causal-review.mp4): five 82 Hz bursts 0.8 seconds apart, 2200 Hz sustain, their mixture, then eight seconds of digital silence. This is an engineering probe, not a review soundtrack.
- [Unedited paired original](../../_scratch/continuous-bloom-r2/physical-001/pair-original.mov), [unedited causal original](../../_scratch/continuous-bloom-r2/physical-001/causal-001/original.mov), [media hashes](../../_scratch/continuous-bloom-r2/physical-001/media-manifest.json).

The review movies crop the real camera image and add labels/source audio; no colour, exposure, contrast or brightness enhancement was applied. Source audio is aligned to the logged playback launch, not a measured microphone onset. Playback startup delay and camera callback offset are uncalibrated; these clips establish visible behaviour, not microphone-to-photon latency. Original video PTS and host/device timestamps are retained. Public macOS capture reports locked modes but does not expose numeric ISO/exposure duration; both music passages share one unchanged session.

## Source, configuration and physical identity

Baseline build `47bc25b5f69c9e27c90dfe089343412569825beef07bed1a4f94e47718ef28d4`, source `eda9bf4`.
Candidate build `4c94653ddc3d9d2a6a56500e1422d953a48c0be50b0486d7deaec55f6e3a43e4`, source `7ae880e`, including the prior `b32fde5` repairs. UID `545433931bd25436593630352d068363`, schema `6d68932e22c0796bcac290a233df9e235410b7af724ac71112525a86017b561b`.

The current candidate was identified from the actual device and current source record, replacing the superseded identity in the earlier session summary. No new firmware changes were needed to capture it. Both images were programmed through the existing E2 Lite procedure, probe power OFF, with successful UID/build/schema readback. Programming took about 24 seconds each. The candidate was restored after the baseline capture and output resumed.

The operator's live master setting was 255/255 at entry, so both comparison passages retain 255/255 rather than the older 100/255 audition default. Both board gains remain 0.25. Gamma, current limits, correction, pins and routing are unchanged. Each passage uses SQ4: scene 3 on both boards, unlinked, roles body/detail at full strength, ML OFF, background OFF, screensaver OFF, palette 1. [Complete settings and per-sample consistency](../../_scratch/continuous-bloom-r2/physical-001/configuration-integrity.json) confirms no configuration drift in either music passage or the causal probe.

The assembled plate is visible in the authorised EiP camera (`EB6E6241-CD5C-4917-91B3-7E6500000001`). Bottom/body optical response and top/detail response are distinguishable in the low-burst/high-sustain probe. Each physical edge contains two electrical DIN halves; they have not been relabelled as four optical channels. Configured routing remains bottom P601/GPT6/DMA0 plus P604/GPT8/DMA3; top P603/GPT7/DMA6 plus P714/GPT12/DMA7; microphone DMA1/2.

## Observed result and fixed criteria

| Criterion | Current evidence and disposition |
|---|---|
| Fresh music remains visible | Repeated bass bursts create new bottom-centre material within four seconds while older material travels. Production-path causal tests, event/conditioning readbacks, bottom final payload and physical sequence are retained. No scene-busy gate occurs in the continuous preset. |
| Independent behaviour | High sustain produces a stronger sustained upper contribution while lower material decays; repeated bass creates lower pulses. Existing deterministic peer-byte isolation check remains valid for the installed correction. Acoustic playback is not bit-identical input. |
| Controls and silence | Existing response/control tests and readbacks pass. The probe visibly drains to a dark plate after the constructed sound stops; later low-level births reappear with new conditioning events. Therefore the entire digital-silence interval is **not** claimed as verified acoustic silence. Ambient input/noise remains a limitation requiring a received-input observation if it is to be classified further. |
| Optics preserve design | Colour and travelling material are visible through the real stack. Much of the music passage remains filled; the intended darker separation needs improvement. The owner did not accept this as the finished physical candidate. |
| Timing | Earlier bounded timing belongs to build `2b8daa...`, not the current single-attack correction. Current physical captures show progressing 10 ms payload timestamps and no reported transport faults, but the required same-workload render-duration comparison for `4c946...` remains outstanding. No microphone-to-photon claim. |
| Honest quality | Engineering **INCOMPLETE**, aesthetic **PENDING**. Camera access is no longer the blocker. |

The owner inspected complete plate sequences from both music passages and the causal probe, plus the final review composite. Some emitter-adjacent camera pixels approach channel clipping: roughly 6.1% baseline / 5.6% candidate of the downsampled plate pixels reach 250 in at least one channel; all-channel clipping is 0% / 0.074%. This limits highlight-colour judgement and is not evidence of electrical clipping. The larger plate retains colour differences and darker regions. Neither brightness averages nor these fractions establish aesthetic improvement.

## One scoped correction attempted, then stopped

Observed defect: body material is blurred across roughly 360 ms by outgoing width 0.20 × 1200 ms plus 120 ms trail, reducing separation between nearby musical changes. The only proposed change was narrower body material spread: outgoing width 0.08, return width 0.08, trail 40 ms. Detail settings, musical envelopes, travel, return strength and physical gains stay unchanged. This uses existing controls; no new effect or renderer was added.

The before/after run is **invalid**. During its first passage, external control traffic changed palette repeatedly, mode, screensaver, ML enable, musical strength, choreography and both scene selections, finally selecting Ribbons. The root sent only the intended three body-width/trail fields at the end. That patch was rendered, but it cannot be assessed as a Bloom improvement on this contaminated run. [Configuration record](../../_scratch/continuous-bloom-r2/physical-001/configuration-integrity.json), [failed run log](../../_scratch/continuous-bloom-r2/physical-001/spacing-run.log), [retained recording](../../_scratch/continuous-bloom-r2/physical-001/spacing-001/original.mov).

Hardware writes stopped when the conflict was established; an asynchronous ownership question is pending. No service was killed or control input disabled to override the other operator. The installed candidate remains running with emission ON; [observed settings at the stop](../../_scratch/continuous-bloom-r2/physical-001/ownership-stop-status.json) are retained. The narrowed settings were not saved into SQ4 or committed as an accepted preset.

Other retained capture failures: the first paired request asked for 116 seconds inside a helper with a fixed 120-second deadline plus reserve; it refused before recording. Correcting the request to 110 seconds completed the pair. The local ffmpeg lacks drawtext; static title strips were used outside the plate image instead. Neither failure was hidden or counted as physical proof.

## Remaining authorised delivery

1. Establish that the competing Titan control activity has stopped. Resume the same candidate and restore the controlled Bloom preset; do not compete with active operator changes.
2. Repeat the spacing comparison after that changed condition, inspect it, retain or reject the three-field adjustment, and save/read back the complete chosen two-board preset.
3. Complete the current-image timing comparison and any required causal follow-up, with the same workload and retained baseline.
4. Deliver the final matched physical comparison with criterion-by-criterion engineering status for Captain's aesthetic decision. Park all banned follow-on work.
