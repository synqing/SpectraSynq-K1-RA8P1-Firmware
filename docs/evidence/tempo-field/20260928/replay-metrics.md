THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

Independent replay scoring; descriptive host evidence, 28 September 2026.

All 30 candidate / field-off / phase-off parity receipts match their current trace, baseline and frozen PCM identities. This preserves the incumbent detector trajectory; it does not improve detector accuracy.

| Excerpt | Exact BPM MAE | Half/exact/double-family MAE | Matched / missed / false attacks | Phase authority coverage | Phase shape contribution |
|---|---:|---:|---|---:|---:|
| clear120 (development) | 0.763 | 0.763 | 51 / 13 / 40 | 68.0% | 0.060 |
| syncopation (development) | 39.462 | 20.010 | 51 / 13 / 28 | 68.0% | 0.070 |
| half_time (development) | 1.146 | 0.867 | 72 / 24 / 46 | 68.0% | 0.076 |
| sparse_reentry (development) | 1.020 | 1.020 | 37 / 7 / 26 | 49.7% | 0.049 |
| dense (development) | 0.880 | 0.880 | 64 / 0 / 291 | 5.5% | 0.000 |
| tempo_step (development) | 10.444 | 5.512 | 53 / 7 / 27 | 68.0% | 0.066 |
| silence (development) | unannotated | unannotated | 0 / 0 / 0 | 0.0% | unannotated |
| fractional12743 (development) | 0.821 | 0.821 | 61 / 7 / 33 | 68.0% | 0.059 |
| song-0 (development) | unannotated | unannotated | unannotated | 45.7% | 0.039 |
| song-1 (held-out) | unannotated | unannotated | unannotated | 40.6% | 0.042 |

BPM errors use annotated active spans and include acquisition. The family metric allows only half/exact/double references; a 2:3 answer is not silently counted as exact. Blank numeric truth is unavailable, never a perfect zero. The attack window is the frozen causal 0..150 ms rule using unique detected event IDs and nearest unused annotations.

The 90→135 BPM step first reaches the 135 bin after 6.5325 s and first locks there after 6.6000 s. These are first occurrences, not a new stability threshold. Syncopation remains a substantial tempo-estimation weakness, and the attack counts retain misses and false positives.

All eight scene masks were scored. Maximum paired total-output mismatch is 2.346e-05 level units. Phase shape contribution is mean total-variation distance between the normalised full candidate and the same candidate with phase disabled; 0 means identical distribution and 1 means disjoint distribution. Normalisation removes brightness as an explanation. Nonzero values prove an independent phase contribution reaches the fixed scene. They do not establish attractive motion or listener preference.

The dense fixture has only 5.5% phase-authority coverage and mean phase shape contribution 0.000000; there is no measured phase contribution to its visible scene. The conservative inherited silence decision suppresses phase observations and the scene envelope. This is a recorded availability limitation, not a successful dense-music result. Held-out song-1 retains 40.6% phase-authority coverage. 

Sparse-gap authority coverage during 12..22 s is 34.108%; the detector declares silence for 75.712% of that span. Silence lifecycle checks across all excerpts: PASS. Exact last-observed/coast/stale frame coordinates are retained below and in JSON.

| Sparse silence start/end frames | Last direct observation | Coast cutoff | Last authoritative frame | First no-authority frame | Fresh-phase hops |
|---|---:|---:|---:|---:|---:|
| 360 / 96120 | 0 | None | None | 360 | 0 |
| 692640 / 1056240 | 691560 | 739560 | 739440 | 739800 | 0 |

The first attempt kept phase authority throughout the sparse silent gap by correlating historic energy. Its scored result is preserved in replay-metrics-initial-failure.json. The repair prevents fresh phase observations when the current field declares silence, preserving the last observation for bounded coasting. AP-A historical salience remains available.

AP-AC05 musical phase qualification remains NOT TESTED: exploratory phase-to-annotation error distributions are reported without inventing an acceptance tolerance. song-0 and held-out song-1 have no attack/beat truth and receive no beat-accuracy result. No held-out tuning occurred. Physical target timing, optical output and musical preference are outside this scorer.

Scorer SHA-256: `b9358a23759c27b091c474e8274c5234d28a2d6ddaaddab28eddd74ba34763bf`. Manifest SHA-256: `307638abc2933ea050fae002fc5e2c9a1135a7283413d2f54edbbaf58297dd78`.
