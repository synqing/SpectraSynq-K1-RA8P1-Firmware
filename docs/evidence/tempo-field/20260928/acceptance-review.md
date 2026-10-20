THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

Independent AP verifier review — 28 September 2026.
This is an evidence disposition, not a replacement execution plan or target approval.
No hardware, serial, platform or AP implementation file was changed by this verifier.

`python3 tools/audio/score_tempo_field.py` passed: the unmutated suite passed eleven
test methods initially; the final suite has twelve, including actual tracker
epoch/configuration recovery. Three separately compiled negative controls each failed assertions
as required. The negative controls reverse phase sign, shift the history reference
one sample, and remove the stale-projection refusal. Compiler errors do not count
as killed mutations. Raw logs and exact source hashes are in `oracle-results.json`.
The independent numerical bound was frozen before candidate scoring and was never
relaxed. Parent's support-interval clarification changes publication start to
512 intervals; it does not change the newest-sample phase reference or oracle.

| Row | Independent disposition | Evidence and remaining scope |
|---|---|---|
| AP-AC01 | PASS HOST FULL REPLAY | Independently verified final comparison-results.json: all 30 complete PCM candidate/field-off/phase-off rows pass and baseline/candidate trace hashes match (139200 hop comparisons). Final sliced-parity/epoch-repair-1/RESULTS.json also has 30 exact rows, 60 independently rehashed traces and baseline-tracer reconstruction proofs; receipt SHA-256 2a159e433946576bb73898ae1086a6c53f3215cbe9df074a441787f7e4bdead3. |
| AP-AC02 | PASS HOST SEAM | Exported comb and point values match injected completed ACF; stable identity, ties, maximum four fresh analyses and eight slots checked. Full PCM salience provenance remains with main replay. |
| AP-AC03 | PASS HOST METADATA | Cached generation/availability stay unchanged; delayed completion keeps captured endpoint; raw observation ages remain distinct; ring order, epoch/configuration during pending jobs, counter exhaustion and actual unsliced update cadence checked. Effective sliced complete-runtime parity now independently hash-verified. |
| AP-AC04 | PASS MATHEMATICAL | 9,216 impulse vectors plus 24 rational pulse trains, fractional BPM, exact reference/projection boundaries and independent direct-complex oracle. All within the predeclared conditioned double arithmetic bound. |
| AP-AC05 | METRICS REPORTED; QUALIFICATION NOT TESTED | replay-metrics.json/md now reports exact/family BPM errors, annotated attack misses/false events, acquisition/step delays and exploratory phase error. No registered musical beat-accuracy tolerance exists. Real songs are unannotated; song-1 remains held-out and was not tuned. |
| AP-AC06 | PASS HOST | Rank changes retain ID/generation; exact .08-cycle boundary retains generation and just-over restarts; positive and negative wrap corrections preserve extended beat anchor; overflow invalidates rather than recycling identity. |
| AP-AC07 | PASS HOST ARITHMETIC | Exact rational half/double/3:2 results at beat parity, wraps and integers beyond 2^53; denominator/numerator zero, epoch mismatch, NaN and overflow rejected. Derived labels remain a producer/consumer contract, not independent detection. |
| AP-AC08 | PASS EXERCISED HOST LIFECYCLE | Publication disable/re-enable, phase suspend/re-enable, epoch/configuration break, invalid time and repeated read isolation pass. Complete-runtime field-on/off and phase-on/off trajectories are identical across ten PCM inputs. Eight downstream scene masks read the same immutable AP samples. Hardware control/reconnect qualification remains outside this host receipt. |
| AP-AC09 | PASS TESTED HOST INPUTS | Zero/negative/NaN/Inf observation rejection; zero-activity coasting and exact 48k/96k age boundaries; finite-input activity overflow invalidation. Independent retained-energy/silence regression found and verified a repair. Final sparse PCM: last observation691560, coast cutoff739560, last valid sampled739440, first invalid739800, no fresh phase during declared silence. Actual onset parity passes AC01. |
| AP-AC10 | PARTIAL HOST/ARM STATIC | Fixed capacities, malformed count rejection and owned-copy nonmutation checked. ARM object receipt reports field1184 bytes/alignment8, publisher2296, phase-state1096 and runtime126008 versus116168 baseline. Static .su records are available; they do not prove actual stack high-water or satisfy an unrecorded target budget. Main's allocation instrumentation is a separate execution receipt. |
| AP-AC11 | NOT TESTED | No physical target, capture-loss, start-lateness, queue, workload or deadline measurement performed. |
| AP-AC12 | PASS HOST REFERENCE CONSUMER | Independently built and executed scene owner's consumer tests after source review. Repeated rendering leaves AP field unchanged; stale/epoch/schema/count/duplicate/future/unknown-availability inputs rejected; options and brightness/centre-outward reference behaviour checked. Physical production consumer/runtime admission remains open. |

Six actual defects were found and repaired by the AP implementation owner while
the verifier retained failing tests: (1) circular correction crossing wrap lost
beat parity; (2) invalid ACF retained old count/phase-authority; (3) finite FLT_MAX
novelty produced valid infinite activity after cast; (4) exhausted track generation
left stale authority. Exact first witnesses were lifecycle return 17, lifecycle
return 18, publication return 15 and lifecycle return 20 respectively. The fifth
was sustained fresh phase from historic energy during current declared silence,
found by full PCM scoring and reproduced as lifecycle return22. The sixth was
valid AP-A evidence from retained pre-discontinuity history; actual tracker tests
returned2 for both epoch and configuration changes. The repair keeps field and
activity unavailable through511 new novelty samples and admits at512; first
failure is retained in oracle-discontinuity-first-failure.log. All now
pass unchanged expectations. Source review also found unknown availability was
accepted by the reference consumer; its owner repaired that and added a fixture.

No screenshot/render was generated by this verifier. Main and the visual owner
retain audition/render evidence. No target or musical-accuracy PASS is inferred
from this result.

Full replay metrics expose remaining limitations: syncopation exact BPM MAE39.462;
90→135 BPM first correct bin6.5325s after the step; dense phase coverage5.5% and
zero measured phase contribution to its visible scene. Held-out song-1 phase
coverage40.6% and nonzero matched-level phase contribution establish bounded
expressiveness, not musical accuracy or preference. The initial faulty replay
metrics are preserved in replay-metrics-initial-failure.json.

Remaining numbered path: (1) main joins the host audition evidence and
governance admission; (2) target owner freezes actual workload/memory/deadline
budgets and runs the separately authorised image-bound target checks before any
target-admitted claim. AP-B mathematical expressiveness may proceed under the
explicit AC05 limitation with required host parity now passed.
