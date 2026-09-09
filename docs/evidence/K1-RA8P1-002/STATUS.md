# K1-RA8P1-002 current execution

Execution authorised by Captain, 10 September 2026. Owner: orchestrator.
Branch: `lane/k1-ra8p1-002` in the existing RA8P1 workspace. No children active;
the prior wave guard rejected the structured launch, so execution is sequential.

## Current result

Fourteen unchanged timing modules are imported and pass an enforced completeness
check. Eight import fault tests pass. Independently compiled donor/candidate
musical-time and scheduler tests pass, including the million-beat case and
90 independent fixture rows. Receipt: `timing-host-02.json`.
The scheduler's inherited photon-labelled output is a HOST model, not optical proof.

The first host run failed because the runner did not stage the donor CSV; no
source defect or tolerance change was involved. The runner now stages the pinned
fixtures with hashes and runs in their isolated directory. Preserve that failure
as a harness lesson rather than editing the oracle.

## Authority and environment

- RA8P1 bootstrap: `e21bad8`, clean before this branch.
- DualMCU: pin and current HEAD `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`;
  untracked `_to_delete/` is preserved. Read-only.
- BSP: pin and current HEAD `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`, clean.
- EdgeAI: `40b45ff`, existing untracked brief/strategic-plan/review/test-results
  preserved. Governing mission is its `docs/titan/K1-RA8P1-End-to-End-Execution-Brief.md`, revision 1.
- No harness init exists in RA8P1/EdgeAI. Supplied instructions and pinned
  DualMCU execution standard were read; no replacement harness invented.
- EdgeAI original operating brief was not located; current amendment delta,
  decisions and selection gate remain available. No missing path is fabricated.
- Proven platform reuse: pinned RT-Thread/FSP USB PCDC, GNU Arm 13.3.1,
  SCons. Base source discovery is C-only and architecture flags allow MVE;
  explicit C++ discovery and scalar build options are required.
- USB enumeration: one `045B:5310` Titan application at location `1-1`;
  no owner returned by `lsof` for its current CDC path. No runtime UID or image
  verification in this execution yet. Board unchanged. ROM programming requires
  the documented held USER/BOOT/reset sequence if no software route is present.

## Acceptance matrix

| Gate | State | Next proof |
| --- | --- | --- |
| G0 | IN_PROGRESS | C++ build/recovery route and slice matrix |
| G1 | HOST timing subset passed | Media mapping/full negative coverage; identified M85 execution |
| G2 | NOT_RUN | AP/VP import and whole-trajectory differential |
| G3/F1 | NOT_RUN | Identified M85 scalar parity |
| G4/F2 | NOT_RUN | Frozen real-workload target timing and failure campaign |
| G5 | NOT_RUN | Available capture/output/bridge paths |
| G6 | NOT_RUN | Existing source-share candidate gate |
| G7 | NOT_RUN | Qualifying frozen candidate required |
| G8 | NOT_RUN | Integrated evidence and recommendation |
| E1 | OPEN | Generic P4; orchestrator owns separate Lab campaign |
| E2 | OPEN | Selected causal implementation and deployment |
| K1-A | OPEN | Fresh production-S3 comparison; preserve protected source |
| K1-B | OPEN | Real bounded Titan/S3 transfer after E1 |
| K1-C | OPEN | Captain decision after prerequisites |

## Next action

Build the scalar M85 C++ test shell from the proven platform route, with parked
M33/U55, then import the actual AP/VP dependency closure. Physical gate failures
must not block independent HOST implementation.
