# K1-RA8P1-002 current execution

Captain authorised sequential end-to-end implementation after delegation was
rejected. No new agent launches or guard changes were attempted. The two existing
repositories remain on `lane/k1-ra8p1-002`; no worktree was created.

## What is true now

- **F1 passes for the covered scalar slice.** The identified Titan executed all
  14,000 frozen hops and matched 7,364,000 typed fields exactly, including all
  320 pixels per hop. Epoch/time semantics and the million-beat probe passed.
- **G4/F2 is implemented and failed.** O2 and O3 scalar schedules missed 2,005
  and 2,006 of 6,000 frozen 7.5 ms deadlines. The identified NPU-alone,
  scheduled, saturation and semantic-failure cells all executed; no concurrent
  mode qualified for soak.
- **Generic P4/E1 is implemented through a target image but remains ON-SILICON
  NOT_RUN.** Its host kernel comparison and target-scheduler tests pass. This is
  separate from K1 F1/F2 and cannot imply K1-A.
- **G6 is `NO_QUALIFYING_CANDIDATE`; G7 is `NOT_RUN_NO_CANDIDATE`.** E2,
  semantic F3 and K1-C remain unpassed. The U55 smoke graph is load only.
- **Physical capture, LED output and Titan-S3 transport remain open.** USB
  enumeration alone is not physical-path proof.

## Implementation commits

| Repository / commit | Material result |
| --- | --- |
| RA8P1 `2d4c73e`, `11c4b6d` | Enforced explicit nonempty import slices, cumulative timing retention and disposable negative fixtures |
| RA8P1 `c9acaef`, `1123522` | Actual pinned time/AP/VP import, independent host replay, native tests and platform-math isolation |
| RA8P1 `f4aa77b`, `e02525f` | C++ M85 shell, identity-gated USB/recovery, scalar startup and full target instrumentation |
| RA8P1 `e218e7f`, `ab17385` | Lossless resident 6,000-hop replay and frozen scalar workload profile |
| RA8P1 `a79ff5e`, `1a93de6` | Observer-distortion repair and byte-exact O3 scalar profile |
| RA8P1 `bf5185e` | Real generated U55 command stream and alternating exact inputs scheduled beside actual K1 |
| RA8P1 `346646b` | Capacity-one semantic seam with loss/delay/stale/order/identity/finite/pressure/error/timeout recovery |
| RA8P1 `1a0f62b` | Generic P4 target scheduler for rFFT, bin-56 Goertzel and symmetric Hann-rFFT under NPU load |
| RA8P1 `b95e999` | P4 deadline guard, unused-flag rejection and explicit unresolved 6 ms authority |
| RA8P1 `195c935` | Generated-NPU closure, measured NPU duty, observer-free qualification and fail-closed heap-growth acceptance |
| EdgeAI `d79591d` | Completed bounded candidate admission and correct no-candidate branch |
| EdgeAI `0b575f3` | Independent host implementation/comparison of the actual generic P4 kernels |

## F1 evidence

`target-corpus-02/receipt.json` is ON-SILICON PASS on UID
`545433931bd25436593630352d068363`, build
`a07b6bba8f5479cdaa3aa75a6d65e4a25de787c7b2ab1e52590e5e938f58f0ec`,
source `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a` and the
`sr24000.hop180.bins80.xover40` contract.

The three cases are 6,000 synthetic-control hops and two 4,000-hop MUSDB
official-test extracts. Each hop compares 526 fields. The complete target result
is exact against the independent Arm-newlib-profile host reference. Stack
untouched fell from 27,712 to 24,920 bytes; heap used/maximum remained 40,560
bytes. M33 and U55 were parked and C++ constructor startup was witnessed.

The same image's USB-paced diagnostics measured tempo-frame maxima around
9.71 ms and ordinary-frame maxima around 4.40 ms. Those transfers were not the
resident deadline test and do not themselves pass or fail G4.

## G4 scalar result and corrected images

The corrected O2 resident run `scalar-schedule-01` completed all 6,000 hops with
zero correctness errors, zero render-budget misses, queue high-water one, zero
drops/coalesces and no heap growth. It failed the frozen schedule:

| Measurement | O2 ON-SILICON result |
| --- | ---: |
| Deadline misses | 2,005 / 6,000 |
| Release-guard failures | 2,008 / 6,000 |
| Total mean / max | 5,637.503 / 9,684 us |
| Tempo mean / max | 9,171.905 / 9,684 us |
| Ordinary mean / max | 3,871.627 / 4,370 us |
| Dual-channel render mean / max | 225.978 / 657 us |
| Comparator telemetry mean / max | 245.866 / 250 us |
| Lateness mean / max | 647.755 / 5,107 us |

The preserved target mutation run reports exactly one correctness failure, so
the full-output CRC comparator does go red. The dominant failure is actual
tempo/ACF compute, not render, telemetry, queue growth or heap use.

The O3 scalar preflight kept all 6,000 outputs exact but missed 2,006 deadlines.
Tempo mean/max improved to 8,903.676/9,391 us and ordinary mean/max to
4,080.121/4,553 us, which is still insufficient for the 7.5 ms contract. It was
not extended into the frozen 30-minute soak.

The identified NPU cells produced these target results:

| Mode | Correct NPU calls | NPU duty | Deadline misses | Backlog high-water | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| NPU-alone, cold | 900 / 900 | 2.382% | 0 | 0 | FAIL: 264-byte first-use allocation |
| NPU-alone, warm diagnostic | 900 / 900 | 2.382% | 0 | 0 | Diagnostic PASS; no recurring heap growth |
| Scheduled K1 + NPU | 900 / 900 | 2.383% | 2,069 | 1 | FAIL |
| Saturation K1 + NPU | 18,000 / 18,000 | 47.647% | 6,000 | 1,730 | FAIL |
| Scheduled + semantic failures | 900 / 900 | 2.383% | 2,068 | 1 | FAIL |

All K1 outputs remained exact and every NPU output matched. No scheduled or
saturation mode qualified for soak. The fresh O3 and NPU boots each exposed the
same one-time 264-byte increase before later runs stabilised; this is consistent
with first-use status/formatting infrastructure rather than recurring workload
allocation. The runners now prime one idle status response before the resource
baseline while preserving zero-growth acceptance for the measured schedule.

Three source-bound images were produced:

| Image | Build ID | text / data / BSS | HEX SHA-256 | State |
| --- | --- | --- | --- | --- |
| O3 scalar | `52c1cf2c…de361a` | 425,852 / 18,104 / 351,812 | `06d12585…a80a9` | Flashed, verified, preflight failed |
| K1 + identified U55 + failure seam | `690f3e20…8664fc` | 636,676 / 18,128 / 607,992 | `0417f481…74eac` | Flashed, verified, all bounded cells executed |
| Generic P4 + K1 + identified U55 | `0a154a78…6bb907` | 666,500 / 42,408 / 657,172 | `549fb6dd…13a04` | Built; P4 ROM entry not observed |

All use Arm GNU 13.3.1, scalar M85 FP, `-ffp-contract=off`, no fast-math,
disabled vectorisers and no MVE. M33 remains parked. The NPU images bind the
same external generated graph sources and two alternating INT8 inputs; raw PMU
cycle, NPU_ACTIVE and MAC_ACTIVE counters are recorded without a factor-of-two
conversion.

The semantic seam never feeds smoke output to lighting. Its target campaign
accepted 309 controlled updates, fired each required rejection/error cell once,
used exactly one capacity-one pressure replacement, recorded nine fallback hops
and nine recoveries, and finished valid. The enclosing coexistence cell still
failed on 2,068 deadlines, so semantic recovery correctness does not pass F2.

## Generic P4/E1 boundary

The profile `e1-p4-workload-profile.json` was frozen before target capture. It
binds 16 kHz, 2,048 samples, seed 0, bin 56 (437.5 Hz), all 1,025 output bins,
the actual C kernels, packed fixture, compiler/BSP and identified NPU inputs.
It uses a 128 ms non-overlap release/deadline, 50 ms NPU cadence, a 2 ms NPU
admission guard and capacity-one scheduling. Concurrent and saturation
qualification each require 14,063 releases (1,800.064 seconds). The comparator
uses a predeclared two-ULP FFT limit and 1e-6 Goertzel absolute limit; a final-bin
mutation must fail exactly one release.

The host runtime test passes numerical comparison, mutation, scheduling, NPU
accounting and 32-bit DWT wrap. The linked target image exists, but no generic
P4 target number or E1 PASS is claimed until it is flashed and run.

## Semantic selection

The completed scope is the existing ShareStudent checkpoint with the existing
one-second non-overlapping frontend/evaluation schedule. Exact weights loaded;
four-source dependence was checked on 48 cached official-test windows. The 1 Hz
schedule fails both admitted transport envelopes, and exact zero PCM emits
nonzero shares `[0.131588, 0.329106, 0.228604, 0.310702]`.

That is a completed necessary-condition failure for this bounded tuple, not a
missing prerequisite. `NO_QUALIFYING_CANDIDATE` and
`NOT_RUN_NO_CANDIDATE` therefore apply. No student I/O freeze, export, Titan
semantic deployment, new ontology or commercial clearance is claimed.

## Physical inventory and board state

Current enumeration shows:

- Titan application USB: `045B:5310`, `/dev/cu.usbmodem00000000000011`, location `1-1`.
- Espressif USB JTAG/serial endpoint: `303A:1001`, serial `B4:3A:45:A5:87:90`, location `0-1.4`.
- Neither endpoint had a process owner at inventory time.

The second endpoint is not accepted as production-S3 firmware identity or a
Titan-S3 link. No connected PDM path, LED backend, bridge framing, clock mapping,
power or thermal instrument has been established. PDM remains dependency-blocked
by E1. G5/K1-B stay open.

The Titan currently runs identified NPU build
`690f3e205e1ae1123a644ce4057dfec9900633fbc60c0960e687543cd38664fc`
with M33 parked and U55 enabled. O3 and NPU programming completed with full
readback verification. The first P4 programming attempt observed zero ROM
devices and exited before opening, erasing or writing the target. No programmer
remains running.

## G8 review and decision packet

The independent-review dispatch was rejected by the delegation guard. Per the
Captain's instruction it was not retried and the guard was not changed. The
following is an orchestrator self-red-team, not independent acceptance:

| False-PASS risk challenged | Current disposition |
| --- | --- |
| Stale board or source identity | Target runners require the exact UID, build, source pin, contract, M33 state, C++ startup and expected U55 state |
| Partial-output comparison | F1 checks all 526 typed fields per hop; P4 checks all 1,025 bins of both FFT outputs plus Goertzel; end-field mutations go red |
| Mismatched NPU graph/public inputs | Profiles and runners bind all nine generated source/header files, source ONNX, same-compilation TFLite, bundle/run identity and two packed inputs |
| Dead-code workload | Build receipts require linked call symbols. Identified NPU target runs now prove exact invocation counts, raw PMU activity and measured wall duty; generic P4 execution remains pending |
| Counter wrap or clock fiction | 32-bit wrap extension is host-tested; runners require DWT/tick agreement within 2%. Long target qualification remains pending |
| Hidden fast-math or MVE | Compiler flags disable contraction, fast-math and vectorisers; ELF attributes and disassembly reject MVE |
| Missed heavy frames | The 6,000-hop scalar corpus includes every heavy tempo update. O2 reports 2,005 deadline misses rather than averaging them away |
| Queue or observer false pass | Capacity, backlog, drops and coalesces are checked; P4 polling-on/off preflights are separate and qualification is frozen polling-off |
| Heap activity hidden by free-space reserve | Found in self-review and fixed: heap-pool, live-use and high-water growth fail. Six negative tests cover the helper; idle observer priming separates first-use reporting infrastructure from the measured workload |
| Event/availability or failure recovery confusion | Event and availability timestamps are distinct; wrong identity, non-finite, future, stale, ordering, pressure, error, timeout and recovery cells pass on HOST and remain required on target |
| Future-context or candidate-generated semantic goldens | No semantic candidate qualified. G7 is not run; the smoke graph cannot pass E2 or feed lighting |
| Physical claims inferred from HOST/USB | Capture, LED, S3 transport, power and thermal cells remain explicitly open |

The decision comparison is therefore:

| Factor | RA8P1 evidence | Production comparison / decision effect |
| --- | --- | --- |
| Correctness and deadlines | F1 exact on 14,000 hops; O2/O3 miss 2,005/2,006 deadlines; scheduled NPU misses 2,069 and saturation misses all 6,000 | No fresh comparable S3 campaign exists, so K1-A stays separate and open. Measured RA8P1 coexistence fails and cannot justify migration |
| Memory | F1 retains 24,920 stack bytes. NPU runs retain at least 25,144 bytes after all cells; first-use allocations stabilise and do not recur | No fresh cross-platform memory receipt; measured schedules enforce reserve and zero post-prime heap growth |
| Transfer and physical latency | Identified USB fixture transport works; the actual Titan-S3/capture/output path is unproved | K1-B and physical F3 stay open; host traffic is not substituted |
| Tooling and recovery | The proven route programmed and verified O3 and NPU images in this campaign; the first P4 attempt found no ROM device and made no write | Recovery is source-bound but physical ROM entry remains intermittent |
| Power and thermal | No identified measurement exists | No production power/thermal conclusion is allowed |
| Implementation complexity | Actual K1 M85, optional U55 load, failure seam and separate P4 workload now build reproducibly | A production move adds unqualified M85/U55/S3 integration and recovery surfaces while the current platform remains the mature choice |

**Recommendation:** retain the existing production platform and keep RA8P1 as a
bench candidate. Reconsider an AP/VP migration only after O3 scalar and the
identified NPU campaigns pass their frozen preflights/qualifications, generic E1
passes separately, a fresh comparable production-S3 campaign closes K1-A, the
identified Titan-S3/capture/output path closes K1-B/F3, and power/thermal evidence
is measured. The no-candidate outcome keeps E2, semantic F3 and K1-C unpassed.

## Gate matrix

| Gate | State | Remaining proof |
| --- | --- | --- |
| G0 | PASS | Preserve source/import authorities |
| G1 | PASS, HOST + identified target | Preserve timing/identity regression |
| G2 | PASS for frozen HOST AP/VP slice | Preserve profile and scope |
| G3 / F1 | PASS for covered modules | Physical capture/output explicitly separate |
| G4 / F2 | FAILED O2, O3 and identified NPU cells | No soak: no required mode passed its bounded preflight |
| G5 / physical F3 | OPEN / dependencies identified | E1, identified capture/output/bridge hardware and measured paths |
| G6 | `NO_QUALIFYING_CANDIDATE` | New material evidence would be required to reopen |
| G7 | `NOT_RUN_NO_CANDIDATE` | Correct terminal state for this candidate scope |
| G8 | PARTIAL | Generic P4 silicon, physical cells and independent review unavailable |
| E1 | HOST + target build PASS; ON-SILICON NOT_RUN | P4 preflights and 30-minute passing concurrent modes |
| E2 / semantic F3 | UNPASSED | No qualifying selected/deployed candidate |
| K1-A | OPEN, independent of F2 | E1 plus fresh comparable production-S3 campaign |
| K1-B | OPEN | E1 plus identified real Titan-S3 transport |
| K1-C | UNPASSED | E1/E2/K1-A/K1-B, then Captain decision |

Current recommendation remains: retain the existing production platform and
treat RA8P1 as a bench candidate. F1 makes the port real; the O2 deadline failure,
unrun coexistence/P4 images, missing physical evidence and no semantic candidate
do not support production migration.

## Exact continuation

External root:
`/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002`.
`external-receipts.json` binds decisive receipts. Failed receipts are preserved.

Next unused programming receipt is `programming-p4-02`. From this repo:

```sh
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/p4-structural-build-06 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programming-p4-02 \
  --wait-seconds 120 --execute
```

Physical entry: arm the programmer first, hold USER/BOOT, press and release
RESET, keep USER/BOOT held until `PROGRAMME_VERIFY_PASS`, then release and reset
normally. Run P4 DSP-alone, NPU-alone, concurrent and saturation preflights with
polling on and off, plus the final-bin mutation. Only passing required concurrent
P4 modes receive their frozen 30-minute qualification; failed preflights are
preserved and not extended.

Host regression:

```sh
python3 -m unittest discover -s tests/host -v
python3 scripts/verify_imports.py --slice timing --enforce
python3 scripts/verify_imports.py --slice product --enforce
python3 scripts/test_fixture_protocol.py
python3 scripts/test_schedule_protocol.py
python3 scripts/test_semantic_sidecar.py
python3 scripts/test_p4_runtime.py \
  --kernels /Users/spectrasynq/SpectraSynq-EdgeAI-Lab/deployment/ra8p1/titan_p4 \
  --fixture /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/p4-fixture-01/p4_fixture.h
```

DualMCU remains at `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`; its `_to_delete/`
is untouched. BSP remains clean at `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`.
The Lab's unrelated untracked strategic plan, brief, review and `test-results/`
remain unstaged. No retired cadence run, audible loop, worktree or production-S3
mutation occurred.
