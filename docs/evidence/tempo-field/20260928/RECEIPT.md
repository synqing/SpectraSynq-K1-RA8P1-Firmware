THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ALWAYS.

# Music intelligence candidate — 28 September 2026

Implemented AP-A and AP-B in the actual scalar AudioPipeline, connected the immutable field to the local AP→VP view, and built a playable host reference scene. This is an additive execution receipt. The original AP chapter, AP acceptance table and PERF assignments remain unchanged.

## Inspect the result

Open `audition/song-0.html` for the development song or `audition/song-1.html` for the held-out song. Both embed the exact frozen audio, actual C++ scene pixels and a causal feature timeline. `audition/clear120.html` and `audition/sparse_reentry.html` isolate onset/warm-up and silence/re-entry behaviour. Three switches independently remove observed accents, incumbent clock and candidate field. The display uses one fixed colour/transfer for both panels. The two panels have matched total linear output; the browser quantisation residual is disclosed.

The useful implemented capability is independent, causally available attack and rhythmic-field motion at matched level. The independent scorer reports an attributable shape change when phase is removed. This does not prove preferred appearance or more accurate beat detection. The incumbent estimator is deliberately unchanged. Real-song beat/attack annotations are absent; musical phase accuracy and operator preference remain open. The exact missing input is sample-indexed attack/beat annotations with metrical ambiguity for the two frozen30-second MUSDB excerpts, plus an operator judgement of the delivered pairs.

## Source and input identity

Worktree: `.worktrees/titan-music-intelligence-001`, branch `feat/titan-music-intelligence-001`. Implementation commit `950580c5548478180721a3e9658d6cfc111f888a`; replay CLI repair `27a2002`. Baseline inherited current control working source, including96 dirty/untracked files; checkpoint commit `d9fcf4d1fd55fcbb61f2091a9eac480a6bf2f5ec`. `source-checkpoint.json` accounts for every copied file and source hash. No root/control checkout was reset or cleaned. The pinned imported source is still separately identified; `docs/tempo-field-adaptation-v1.json` records the five authorised local adaptations rather than relabelling them byte-identical imports.

Frozen fixture identity: `tests/fixtures/tempo_field/v1/pcm/manifest.json`, SHA-256 `307638abc2933ea050fae002fc5e2c9a1135a7283413d2f54edbbaf58297dd78`. Ten fixtures: eight explicitly synthetic PCM cases and two previously authorised music excerpts. song-1 is held out from tuning. No generated novelty is presented as real-song evidence.

`baseline-build/` retains the actual pre-candidate executable and portable sources. `baseline-source.json`, `baseline-results.json`, `comparison-results.json` and `sliced-parity/` bind sources, binaries and traces. Full per-hop observations are retained in `baseline/` and `candidate/`; `completed-fields/` contains one row per completed generation. Event time, analysed support, availability and presentation are separate. Host replay uses a simulated clock advanced at hop release; this is causal functional evidence, not measured processing or physical latency.

## Verification and limitations

See `acceptance-review.md` for each AP-AC01…12 disposition, `oracle-freeze.md` for the predeclared convention/error bound, `oracle-results.json` for actual tests and killed mutations, and `replay-metrics.md` for per-excerpt results. Complete replay preserves every serialised incumbent field across all ten fixtures with field enabled, disabled and phase disabled. A separate effective sliced build compares matching sliced baseline/candidate paths. Rendering all eight option combinations does not mutate AP.

The independent oracle uses direct complex correlation and high-accuracy summation. Four fresh observations and eight slots are fixed capacities. Phase tracks preserve extended beat coordinates, separate actual running frequency from winner BPM, reject invalid/old epochs, and expose unknown uncertainty honestly. AP-B is qualified for continuous expressive motion only. Predicted-next-beat authority remains false.

Failed attempts remain visible: parity-wrap, invalid-field authority, finite activity overflow, generation exhaustion, missing availability validation, historical-energy phase refresh through silence, and mixed-namespace warm-up evidence. Tests and initial traces/builds are retained rather than replaced with successful receipts. Silence repair does not alter the incumbent. It exposes a remaining limitation: the incumbent silence decision reduces phase coverage sharply on the dense mixture. Exact tempo and half/double-family scores remain separate; syncopation and attack false positives remain weaknesses.

ARM snapshot size1184 bytes, alignment8; total LiveAudioRuntime growth9840 bytes. Known compiler stack paths fit the configured32KiB main thread, but library/interrupt depth and physical high-water remain unmeasured. See `sliced-parity/arm-memory-final.json`. The host runtime test detects steady-state C++ allocations and exercises completion timestamps, cached reads, disable, epoch reset and unchanged scheduling flags.

Final ARM candidate: `arm-tempo-field-epoch-repair-1/rtthread.hex`, SHA-256 `298c2312b4790c5420403d1591fe7d981f3035482b21c77007ab789c97235b61`. All134 build source hashes match. This HEX was not programmed. Final sliced comparison: `sliced-parity/epoch-repair-1/RESULTS.json`,30/30 matches across139200 hops;15380 warming hops and14435 silent hops had zero validity/lifecycle violations.

## Device and target admission

The first read-only INFO failed framing; the bounded follow-up preserved drained bytes and recovered identity without reset or configuration writes. `target-identity/resynchronised-result.json` binds actual UID545433931bd25436593630352d068363, resident build7767c349b9369db81ac0c6260435d1e4c4f19bd0c2b16770f6ff343056b1102d and schema229c40d80d70915acd2ef5645bbd687477889b40f807ec847c2d2a384130fb10. It reports emission ON, mode8/8, brightness24, P601/P604 and pair_fault0. The same sample reports hold, musical=false and363378us audio age. Counters are not optical evidence. No AP candidate was programmed and no reset/interruption or emission change was performed by this work.

AP-G3 remains OPEN: exact candidate combined-load start/completion lateness, publication age, capture losses, memory high-water and physical output have not passed. A successful ARM build cannot override the existing stale/hold observation. Current optical state remains UNKNOWN. The build preserves autonomous boot output and existing wiring/DMA assignments; the reference scene is host output and has not replaced constrained live mode8.

The workflow executed verification commands successfully, but `workflow_accept` refused command-only acceptance because the historical bound registry lacks the required observed case admission. `workflow_gate` retains original AP/ORCH/CTL/IVV dependencies. `governance-executions.json` records this without rewriting or closing those assignments.

## Reproduce

From this worktree:

```sh
python3 tools/audio/score_tempo_field.py
python3 scripts/test_k1_live_runtime.py
python3 tools/audio/score_tempo_replay.py
python3 tools/audio/run_tempo_comparison.py --output /tmp/titan-ap-replay-new
python3 scripts/replay_live_k1.py --pcm tests/fixtures/tempo_field/v1/pcm/song-0.pcm --output /tmp/titan-song-replay-new --scene --repeat 2
python3 tools/audio/build_tempo_audition.py --baseline docs/evidence/tempo-field/20260928/baseline/song-0.jsonl --candidate docs/evidence/tempo-field/20260928/candidate/song-0.jsonl --pcm tests/fixtures/tempo_field/v1/pcm/song-0.pcm --output /tmp/titan-song-audition.html --label 'Development song'
node docs/evidence/tempo-field/20260928/audition/inspect_auditions.cjs
```

These component commands were executed during this assignment; fresh output paths are required where the tool refuses overwrites. Build commands and exact HEX/effective-source identities are retained with the ARM receipts and sliced runner. No network/audio download, sibling firmware/vendor edit or radio protocol expansion was performed.

## Remaining numbered path and owners

1. Titan platform owner: reconcile the current7767c349 baseline's stale/hold behaviour against its frozen timing/capture budget, without stopping autonomous output; retain actual optical evidence. Then run paired full-load baseline/candidate admission using the committed AP publication and exact effective source. Preserve current clock authority, brightness24, UID, pins, DMA and probe powerOFF. Restore normal output after any unavoidable programming/reset.
2. Independent verifier and governance owner: attach the actual observed cases to the existing frozen registry acceptance mechanism; do not replace its full assignments or infer completion from exit0.
3. Operator: inspect the working same-audio auditions, especially sparse re-entry23.010s and real-song18s, and judge the added motion. Annotated musical phase qualification remains a separate measurable task.
4. Next MIR increment: qualify harmonic movement; then texture/build/breakdown/re-entry cues on held-out passages. Learned semantics and CPU/NPU optimisation require incremental utility/resource evidence.
