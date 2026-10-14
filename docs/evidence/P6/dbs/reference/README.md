# P6 DBS onset reference: BOOM 0–75 s and Sonic 6–30 s

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This is host-only analysis evidence. It touched no device, firmware, mixer, broker or camera.

## What this is

This folder lists where fresh musical attacks ("onsets") occur in the canonical BOOM passage and the weak-transfer Sonic window. Every onset has a time, a strength, a share of each frequency band, and two labels: dense or sparse context, and whether it sits over sustained material. The Titan spectral-detail pilot uses this list as the "what actually happened in the music" side when it scores lower Hybrid K1 (mode 32).

It is a measured reference from one declared detector. It is not hand-annotated ground truth. Use it with the accuracy limits below.

## How accurate it is (plain answer)

- **Timing of the attacks it finds is good on BOOM.** Planted test attacks were found within 3.5 ms (95th percentile) on the development set and 2.9 ms on an independent set. The median error is about 0 ms. The ≤ 6 ms requirement passes on BOOM.
- **Timing on Sonic fails the 6 ms requirement.** The 95th percentile is 8.6 ms with raw frame times and 13.6 ms with the refinement chosen on BOOM. Sonic is 12.8 kHz material, and the refinement step does not transfer to it. Every Sonic row also carries the unrefined `time_frame_s`.
- **It misses many weak attacks. The recall requirement fails on every set.** Required: at least 90 % of planted attacks at −18 dB or louder are found. Measured: 53 % (BOOM development), 40 % (BOOM independent) and 79 % (Sonic). No threshold value fixes this, because recall stays flat across the whole threshold range. Two specified design features cause the misses:
  1. A strong attack within about 50 ms raises the moving-mean threshold above a weak neighbour. The ±30 ms peak window also suppresses the neighbour. Recall is 37–48 % for plants 40–60 ms from a host onset; the few plants more than 100 ms away were all found.
  2. Short low-frequency "kick" attacks barely move the spectral-flux sum. Only 20 of 160 analysis bands lie below 250 Hz. Kick recall is 13–33 % on BOOM; noise-burst recall is 67–73 %.
- **It fires on sustained noisy material.** On the synthetic `sustain` probe (pink noise plus a held chord, no attacks) it produces about 16.8 detections per second at the frozen threshold. On the same probe material it finds at most about half of a train of −6 dB kicks at ±10 ms. Many of the low-strength BOOM onsets are probably this kind of noise-flux artefact, not musical attacks.

### Practical use

- Treat strong onsets as reliable arrivals. Each row has `picked_up_to_delta`, the largest threshold at which that onset would still be picked. The strongest false detection on the sustain probe reached 29.2. **207 of the 573 scored BOOM onsets lie above that yardstick** (`negative_control_probes.sustain_false_birth_salience_floor`). The probe layer is not BOOM's material, so this yardstick is a guide, not a validated threshold.
- Do not use this list to decide that a weak attack close to a strong one (under about 60 ms), or a kick-only attack over a pad, is absent. The detector cannot see those reliably.
- The `context` field is nearly uninformative at the frozen threshold: 571 of 573 scored BOOM onsets are "dense", because the low threshold admits about 12 onsets per second. `over_sustain` is true for 530 of 573.

## Inputs (hash-checked on every run)

| Input | sha256 | Format |
|---|---|---|
| `_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav` | `09c9a02f…3e63652` | 44.1 kHz stereo PCM16, 3,307,500 frames |
| `/tmp/p6-product-path-p-inputs/boom_source.s16` (replay actually fed to the probes) | `45edfd09…bfbd15` | 40 kHz mono s16, 75 s + 8 s zero tail |
| `docs/emotiscope-port-001/evidence/audio-windows-001/sonic-samba-6-30s.wav` | `8eb9b859…eaeeb44` | 12.8 kHz mono, 24 s |

Song time for BOOM is sample / 44100. Rows from 0–28 s are present but marked `scored=false`; the scored window is 28–75 s. Sonic time is window time (0 = 6.000 s into the retained source); the scored window is 0.5–23.5 s.

## Alignment

- **The BOOM replay is exactly the reference, resampled.** Waveform correlation is 1.000000 at 0-sample lag in five 2-second segments, at +0.99 dB gain with normal polarity. The envelope offset is 0.000 ms overall and in every 15 s segment. The replay's 8 s tail is digital zero.
- Running the same frozen detector on the replay recovers 564 of 573 scored reference onsets within ±25 ms. The median difference is 0.0 ms and the 95th percentile 2.4 ms.
- **The Sonic window origin is confirmed at 6.000 s** in the full retained source (envelope correlation 0.999). The drop-cue "Sonic 27.7" high-band rise at 21.726 s in window time coincides exactly with a reference onset at 21.726 s, strength 41.2. That onset is 16 ms before the drop-cue sub-bass half-rise.

## Comparison with the provisional annotations (CANONICAL-AUDIO-PERFORMANCE-SUITE.md §3 and drop-cue)

| Annotation | What the reference shows | Verdict |
|---|---|---|
| Build pulses closer, ~31–42 s, 434 → 219 → 125 → 95 ms | One **continuous, smooth acceleration**: 432, 406, 384, 364 … 246, 220, 209 … 154, 144 … 124, 115, 109, 104, 99 ms from 31.8 s to 39.9 s, then about 70–90 ms to 42 s. All four quoted values occur in passing, but they are not discrete steps. At the frozen threshold, weak detections in between break up the IOI runs; the sequence appears cleanly once onsets weaker than `picked_up_to_delta` 10 are set aside (`strength_tier_sensitivity`). | Agrees on direction and range. **Disagrees on "steps"**: it is a continuous accelerando. |
| ~435 ms / 138 BPM recurrence after 55.5 s | The ODF autocorrelation peaks at exactly 435.0 ms (137.9 BPM, value 0.75). | Agrees. |
| Accents ~107–110 ms (~109 ms sixteenths) | The most common IOI bin over 28–75 s is 105–110 ms (109 intervals); 116 of 149 intervals between 100 and 118 ms fall after 55.5 s. | Agrees. |
| Brief 70–73 ms gaps around 55 s | Twelve intervals of 65–76 ms between 53 and 58 s (for example 55.661 → 71.0 ms, 55.842 → 71.4 ms). They sit among many other short intervals (40–60 ms are common at this threshold), so the reference cannot call them a regular feature. | Present; regularity not established (same as the annotation's own caveat). |
| Decay 50–55 s vs strong 55.5–75 s: −19.81 vs −6.69 dBFS, 13.12 dB | The PCM16 fixture measures −24.30 vs −11.17 dBFS stereo RMS. The **difference is identical, 13.12 dB**. The absolute values are 4.49 dB lower, consistent with the fixture's constant attenuation from the float decode [INFERENCE]. The reference also finds more onsets per second in the quiet decay (14.2/s) than in the strong section (10.8/s), which points to noise-flux detections in the decay tail. | Contrast agrees. The decay-section onset density is suspect. |
| Drop-cue BOOM sub-bass step at 55.766 s (band-energy half-rise) | No reference onset at 55.766 s. The drop appears as a broadband flux onset at 55.732 s (strength 202) with almost no sub-band share, followed by one at 55.842 s. The two methods measure different things: a smooth sub-bass level step is not a spectral-flux peak. | **Disagrees in kind.** Use drop-cue for the sub step, and this reference for the broadband attack 34 ms earlier. |

## Method (frozen; every value is also in each JSON `params`)

SuperFlux-style detector: mono downmix, Hann window 23.2 ms (1024 samples at 44.1 kHz, 297 at 12.8 kHz), hop about 2 ms (88 / 26 samples), zero-padded FFT, triangular log filterbank at 24 bands per octave from 30 Hz to min(16 kHz, 0.45·fs), log(1 + |X|). The reference frame is max-filtered across 3 bands, then positive differences are taken against the frame 5 hops (about 10 ms) earlier and summed. Threshold = moving mean over ±50 ms + δ. Peaks must be the maximum within ±30 ms; minimum separation is 30 ms.

**δ = 3**, chosen once on BOOM 0–28 s development plants. The primary rule (the largest δ with recall ≥ 0.9 and p95 ≤ 6 ms) could not be met by any grid value. The declared fallback took the largest δ at the best achievable recall, and the recall requirement is reported as failed. Onset times are refined to the sample using a 1 ms envelope rise on the first-differenced signal. Refinement was selected on the development set only; un-refined frame times are kept in `time_frame_s`.

Plant tests: 50 attacks per set (10 ms noise bursts and 5 ms kicks at −6/−12/−18/−24/−30 dB relative to local RMS). They are placed at least 40 ms from host detections and 300 ms apart, and matched one-to-one within ±25 ms. **The matching test can fail:** with every other plant omitted, matched count fell from 18 to 10, and 0 of the 25 omitted plants were matched. Random times without the 40 ms exclusion would sit within ±25 ms of a host onset 50 % of the time, which is why the exclusion is needed.

Sonic limits: Nyquist is 6.4 kHz, so analysis stops at 5.76 kHz. The "high" group covers only 2–5.76 kHz, and there is no content above 8 kHz. Band fractions are not comparable one-to-one with BOOM.

## Files

| File | sha256 |
|---|---|
| `boom-onsets.json` | `36f85517…0ae56b8` |
| `boom-onsets.png` (ODF and onsets 28–75 s, zooms 36–43 s and 55–60 s) | `529f3b01…19c7f7cf` |
| `sonic-6-30-onsets.json` | `21915afe…19c5df9d` |
| `sonic-6-30-onsets.png` | `fc8fa2ee…d619aec7` |

Producers: `scripts/p6_dbs_reference_onsets.py` and `scripts/p6_dbs_probes.py`. Run the probes first, then the reference:

```
uv run --quiet --with numpy==2.1.3 --with scipy==1.14.1 python scripts/p6_dbs_probes.py
uv run --quiet --with numpy==2.1.3 --with scipy==1.14.1 --with matplotlib==3.9.2 python scripts/p6_dbs_reference_onsets.py
```

Two consecutive full runs produced byte-identical probe audio, probe JSON and reference JSON.

Mechanism probes (engineering probes, not review music) are in `_scratch/waveform-migration-001/runs/P6-DBS-20261015/probes/`, with `index.json` holding the level convention, the specification and each file's hash. One probe, `quiet_recovery.s16`, has 22 hard-clipped samples in its loud section. The specified levels (+6 dB layer plus 0 dB attacks) reach full scale there. The levels were not changed to avoid it, and the clip times are listed in its JSON.
