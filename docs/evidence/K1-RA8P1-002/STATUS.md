# K1-RA8P1-002 current execution

Captain explicitly authorised sequential implementation after delegation failed.
No further launch attempts or guard changes followed that instruction. No children
were launched. The orchestrator is the sole hardware operator. Both repositories use their existing
checkout on `lane/k1-ra8p1-002`.

## Implemented

| Commit | Material result |
| --- | --- |
| RA8P1 `2d4c73e` | Enforced nonempty import slices; 14 unchanged timing files; independent timing/scheduler assertions, million-beat and 90-vector tests |
| RA8P1 `11c4b6d` | Cumulative product slice cannot omit timing files; ten disposable-fixture rejection tests |
| RA8P1 `c9acaef` | 52 byte-exact cumulative imports; real AP/VP; 14,000-hop independent scalar HOST replay; all 526 observable fields/hop, including all 320 pixels, match |
| RA8P1 `f4aa77b` | Scalar C++ M85 shell; bounded CRC/sequence USB fixture protocol; UID/build/source-gated client; epoch reset/time probe; proven ROM programmer wrapper |
| RA8P1 `1123522` | Seven pinned native test entry points pass; tempo golden 6,400 records; compiler-profile sensitivity isolated and retained |
| EdgeAI `d79591d` | Bounded existing semantic-tuple admission check and missing-prerequisite regression tests |

HOST trajectory: 45 s synthetic controls and two 30 s MUSDB official-test song
extracts; 7,364,000 field comparisons. Seven comparator corruptions rejected.
AP release is 7.5 ms; render cadence is 120 Hz. No physical timing is implied.
Thirteen HOST unit tests pass, including ten import tests. The actual C++ fixture
parser passes seven chunk sizes, reset/repeat, null/non-finite AP input handling,
CRC, sequence, oversized payload, timeout wrap and reconnect checks.

## Build and board

Accepted release: external `scalar-build-04`.
Build ID: `4412a2743fc7fc5f7da1d2046d6bf73769d8c6e52adf53a0cde33f06d21d3592`.
HEX SHA-256: `bfe574628bb82a0c8f60044147ef5555d60788591d2b3281f8c4dd4b41ff9bac`.
GNU Arm 13.3.1: text 184,756, data 18,096, BSS 195,540 bytes.
Separate debug build: `scalar-debug-01`. Both builds pass C++ inclusion,
constructor-table, compiler-macro, ELF-attribute and generated-instruction checks.
MVE, automatic vectorisation, fast-math and application contraction are disabled.
Precompiled scalar newlib uses double-precision FMA internally; see NUMERICAL-PROFILE.md.
No U55-open or secondary-core-start call is linked. D-cache follows P2/P3's
disabled policy. Runtime constructor/M33 checks passed on the identified board.

Main-thread stack is 32 KiB. Compiler frame reports: AP process 3,232 bytes,
trajectory 984, renderer 328, time probe 1,552. These are individual static frames,
not runtime high-water or whole-call-chain measurements.

Programming-02 completed full read-back verification of scalar-build-04 on UID
`545433931bd25436593630352d068363`. Normal reset returned application USB
045B:5310 at location 1-1. Runtime build/source/contract, C++ constructor witness,
M33 parked and U55 unopened all matched. The target million-beat/epoch/time probe
and five transport rejection cases passed. Two complete AP/VP hops matched all
1,052 fields exactly. These establish real execution, not complete F1.

The old native-math diagnostic replay was deliberately stopped after 3,280 complete
hops to replace verbose transfer with a lossless compact protocol. Its failed/
incomplete receipt and raw trace remain. First native-HOST difference: onset at
hop 337. The independently isolated Arm logf/expf/log2f profile explains every
collected difference: 1,725,280 fields match exactly under that diagnostic profile.
This is not retrospective qualification. USB was closed before programming-03.

The next scalar image is scalar-build-05, build ID
a07b6bba8f5479cdaa3aa75a6d65e4a25de787c7b2ab1e52590e5e938f58f0ec,
HEX bf0c6a4ff7a9645c094d41489f19b29fabe285f9704ebac27d62056292d08945.
It adds lossless compact full traces and stack/heap/clock-consistency readouts.
Its size is text 184,436 / data 18,104 / BSS 195,532 bytes. Both native and explicit
Arm-math HOST profiles pass all 14,000 hops plus complete compact round-trips.
Twenty-three unit tests pass, including 30,000 independent math-function checks.
Programming-03 timed out after 180 s without ROM entry and before opening/writing
the device. No programming process remains. A bounded read-only drain discarded
10,192 bytes of the interrupted response; a subsequent fresh INFO again matched
UID, scalar-build-04, pinned source and parked M33/U55. USB is closed and unowned.
The board has not been flashed with scalar-build-05 yet.

G4 preparation: the exact 45-second controls deduplicate to 346 unique hops.
Input dictionary, 6,000 indices, full-output CRCs and lengths occupy 172,560 bytes.
All 6,000 reconstructed inputs and donor-output round-trips pass. This prepares
resident replay without making USB bandwidth a soak-test dependency; no scheduler
or NPU qualification is implied. The generated data remain external.

Separate generic E1 preparation in the Lab now implements actual rFFT, symmetric
Hann-rFFT and bin-56 Goertzel C kernels against the existing 16 kHz/2,048/seed-0
fixture. Both 1,025-bin arrays and the Goertzel magnitude match exactly on HOST.
Fourteen comparator/candidate-gate tests pass. No P4 target, NPU-load or deadline
result follows from these HOST kernels.

## Failures and evidence boundaries

1. Initial timing harness omitted the pinned CSV. Fixed staging; no source or
   golden change. Passing receipts: timing-host-02.json and timing-host-03.json.
2. First build's scanner matched ASCII q7 in a literal pool. It now checks
   instruction forms, with MVE-positive and scalar/data-negative unit controls.
3. Stack reports followed BSP symlinks: six generated .su files appeared in the
   reference checkout. Only those files were moved, recoverably, to
   scalar-build-01/vendor-stack-usage/. BSP is clean. Staging now copies inputs.
   scalar-build-02 retains the dirty-reference refusal.
4. Strict -O2 reference tempo replay first fails its golden at step 12:
   winner_bin 5 versus 4. The four-profile probe shows pinned native -O0 passes
   6,400 rows; pinned native -O2 first differs at step 6027: confidence 0.45670
   versus 0.45669. Disabling contraction first differs at step 12. No source,
   golden or tolerance changed. Native test acceptance and scalar release
   acceptance are separate. Cross-profile/M85 numerical acceptance remains open;
   the target client stops on the first exact mismatch. Never borrow P3 tolerance.
5. Inherited onset/saliency tests explicitly do not establish captured legacy
   parity. The new long comparison is pinned-source HOST parity, not silicon.
6. K1-DM-112 explicitly freezes AGC's 10 ms tuning clock and mood's 100 Hz
   parameter, distinct from 7.5 ms AP releases. Those values remain unchanged.
7. No independent specialist review/model diversity is claimed. The prior
   launch was rejected with MISSING_WAVE_SIZE; Captain authorised sequential work.

## Semantic result

Completed scope: existing ShareStudent checkpoint + existing one-second,
non-overlapping frontend/evaluation schedule; no alternative search or training.
Exact checkpoint loaded; four-source input dependence checked on 48 cached
official-test windows. Its 1 Hz schedule fails both admitted transport envelopes.
Exact zero PCM yields shares [0.131588, 0.329106, 0.228604, 0.310702], not zeros.
These are actual necessary-condition failures, not missing prerequisites.

`NO_QUALIFYING_CANDIDATE` applies only to this tuple; G7 is
`NOT_RUN_NO_CANDIDATE`. The script's --enforce run exits 2. Missing prerequisites
instead produce BLOCKED, tested separately. No I/O freeze, export, Titan model,
new ontology, cadence/C1 replay or commercial clearance is claimed. Full
nine-criterion acceptance and semantic-dependent product acceptance remain unpassed.

## Acceptance and decision

| Gate | State | Next proof |
| --- | --- | --- |
| G0 | PASS, scoped build/implementation route | Preserve authorities |
| G1 | HOST and identified target PASS | Preserve scalar identity and timing regression |
| G2 | HOST parity in named profiles | Preserve profile limitations; no target inference |
| G3 / F1 | OPEN, scalar startup passed | Fresh complete M85 replay under frozen platform-math profile; resource bounds |
| G4 / F2 | NOT_RUN, depends on G3 | Frozen scheduled actual-K1 workload, NPU loads, bounded queues and recovery |
| G5 / physical F3 | OPEN | Real capture/output/bridge after prerequisites |
| G6 | NO_QUALIFYING_CANDIDATE, bounded tuple | New material evidence to reopen selection |
| G7 | NOT_RUN_NO_CANDIDATE | No smoke-graph substitute |
| G8 | Partial decision packet | Scalar/coexistence/physical evidence and independent review absent |
| E1 | HOST kernels PASS / P4 silicon NOT_RUN | Separate generic P4 integration and load/failure campaign; orchestrator owns it |
| E2 / semantic F3 | UNPASSED | No qualifying selected/deployed candidate |
| K1-A | OPEN, separate from G4/F2 | Fresh comparable S3 campaign plus E1 |
| K1-B | OPEN | Real bounded Titan/S3 transport after E1 |
| K1-C | UNPASSED | Prerequisites, then Captain's production-role decision |

Recommendation now: retain the existing production platform; RA8P1 remains a
bench candidate. The actual port and recovery-ready image exist, but scalar M85
execution, deadlines, physical output, power and thermal advantages are unproved.
Neither a build nor NPU smoke can justify migration. This is not campaign closure.

## Evidence and continuation

External root:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`.
external-receipts.json binds decisive receipts; build receipts bind sources,
artefacts, maps and disassembly. Large PCM/traces/binaries remain external.
No screenshots/renders were needed at this memory-sink boundary: full pixel
arrays are in the traces. Physical PDM/LED/bridge remain separate; the BSP PDM
example is 16 kHz, not proof of 24 kHz K1 capture. PDM waits for E1. S3 USB
enumeration is not evidence of Titan/S3 wiring.

From this repository, arm the next unused programming run:
```sh
k1_artifacts=/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/programme_scalar.py --build "$k1_artifacts/scalar-build-05" --output "$k1_artifacts/programming-04" --wait-seconds 180 --execute
```

Physical input: hold USER/BOOT, press/release RESET, keep USER/BOOT held through
PROGRAMME_VERIFY_PASS; release and reset normally. No new approval is required.
Then:
```sh
/Users/spectrasynq/SpectraSynq-EdgeAI-Lab/.venv/bin/python scripts/run_scalar_target.py --build "$k1_artifacts/scalar-build-05" --output "$k1_artifacts/target-smoke-02" --corpus "$k1_artifacts/host-product-arm-math-01" --stage smoke --binary --metrics
```
If smoke passes, run --stage corpus --binary --metrics --collect-mismatches into
target-corpus-02. Diagnose any mismatch
without moving a threshold. Remaining path: (1) scalar target acceptance;
(2) frozen actual-K1 NPU coexistence and separate E1; (3) available physical
paths and fresh K1-A/K1-B; (4) complete G8/K1-C packet. G4/F2 does not wait for
S3 comparison and cannot imply K1-A closure.

HOST regression:
```sh
python3 -m unittest discover -s tests/host -v
python3 scripts/verify_imports.py --slice timing --enforce
python3 scripts/verify_imports.py --slice product --enforce
python3 scripts/test_fixture_protocol.py
python3 scripts/run_host.py --slice product --profile pinned_native --output /tmp/k1-pinned-next.json
```

DualMCU stays at 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a; its existing
_to_delete/ remains untouched. BSP stays clean at
6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7. EdgeAI's existing untracked brief,
strategic plan, review and test-results remain unstaged. No worktrees,
production-S3 changes, retired cadence runs or audible loops. Missing old
operating-brief/harness paths were recorded, not invented; current amendment
and selection authorities were read.
