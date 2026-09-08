# K1-RA8P1-001 — DualMCU Core Portability Probe

## 0. Mission

You are taking over a greenfield firmware repository for the SpectraSynq K1 on the Renesas RA8P1 Titan Mini.

Repository:

`/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware`

Your job is **not** to rewrite K1 for Renesas and **not** to declare RA8P1 the production MCU. Your job is to determine, by implementation and measurement, whether the platform-neutral compute architecture already proven in `SpectraSynq-K1-DualMCU-Firmware` can execute correctly and usefully on the Titan Mini.

The core hypothesis is:

> `MEDIA_TIME`, `MUSICAL_TIME`, MIR/DSP, peer-clock mapping and musical render scheduling are SpectraSynq architecture, not RT1062 architecture.

Prove or falsify that hypothesis.

Initial architecture for this lane:

```text
RA8P1 Cortex-M85 = AP + VP
RA8P1 Cortex-M33 = PARKED
Ethos-U55         = PARKED
ESP32-S3          = future RADIO-only companion; not required for first parity gate
```

First-pass objective: **correct scalar parity**.

Do not optimise with Helium/MVE. Do not move work to M33. Do not use the NPU. Those are follow-on experiments only after scalar parity exists.

---

## 1. Authority and source discipline

Read before doing anything:

- `AGENTS.md`
- `docs/REFERENCE-MANIFEST.md`
- `docs/PORTING-MATRIX.md`
- `platform/ra8p1/README.md`

Run:

```bash
./scripts/check_references.sh
```

Pinned behavioural authority:

```text
/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware
commit 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a
```

Pinned Titan BSP authority:

```text
/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini
commit 6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7
```

Both sibling repositories are **READ ONLY** unless the user explicitly authorises mutation.

Do not follow their current HEAD silently if it moves. Use the pinned commits for source comparison/import or explicitly document an authority update.

---

## 2. Greenfield rules

This repository has independent history and independent architecture ownership.

Do not:

- clone/copy the DualMCU `.git` directory;
- mass-copy the entire DualMCU tree;
- mass-copy the entire Titan BSP;
- create a giant `vendor/` dump as the first move;
- redesign algorithms while porting them;
- translate C++ to C merely because vendor examples are C;
- substitute RTOS timestamps for audio sample coordinates;
- conflate target compile success with target execution;
- conflate target execution with physical audio/timing proof.

Shared SpectraSynq code belongs under:

```text
src/k1/
```

RA8P1-specific code belongs under:

```text
platform/ra8p1/
```

Host tests belong under:

```text
tests/host/
```

Target/board tests belong under:

```text
tests/target/
```

Evidence belongs under:

```text
docs/evidence/<lane-or-gate>/
```

---

## 3. Do not disturb the user's current Titan bring-up

The Titan Mini has already arrived and the user is actively bringing it up.

Before flashing, erasing, regenerating FSP configuration, modifying the BSP checkout, changing debug settings or taking ownership of a serial/debug port:

1. inventory the current host state;
2. identify the board/debug probe/serial device by evidence;
3. inspect any currently running build/debug processes;
4. preserve current logs/configuration;
5. determine which Titan project/framework is presently being used.

Start read-only.

Useful commands may include:

```bash
ps aux | grep -Ei 'jlink|openocd|pyocd|rtt|renesas|e2studio|gdb' | grep -v grep
ls /dev/cu.* /dev/tty.* 2>/dev/null
system_profiler SPUSBDataType
```

Do not guess that the user's current bring-up is using RT-Thread Studio just because the local BSP is RT-Thread based.

Record the actual toolchain/framework/debug path in:

`docs/evidence/K1-RA8P1-001/00-environment-audit.md`

If a functioning bring-up project already exists elsewhere, reference it; do not hijack it.

---

## 4. Phase 0 — environment and toolchain audit

Determine and record:

- exact Titan Mini board identity/revision if visible;
- RA8P1 part identity if available from debug/board docs;
- active debug probe: J-Link, DAPLink or other;
- serial device(s);
- current working Titan example/project;
- compiler executable and version;
- C and C++ support;
- RTOS/framework: RT-Thread, FSP bare-metal, other;
- build system: SCons, Make, IDE-generated, CMake, other;
- current flash/debug command;
- whether M85 is currently booting;
- whether M33 is currently parked or running vendor firmware;
- whether caches/FPU are enabled by the working configuration;
- board clock configuration relevant to timing measurements;
- availability of a monotonic/free-running hardware timer.

Investigate the pinned BSP examples, especially:

```text
project/Titan_Mini_template
project/Titan_Mini_pdm
project/Titan_Mini_wavplayer
project/Titan_Mini_rpmsg
project/Titan_Mini_usb_pcdc
project/Titan_Mini_driver_all
```

Deliver:

`docs/evidence/K1-RA8P1-001/00-environment-audit.md`

No architecture decision may depend on an assumed toolchain.

---

## 5. Phase 1 — establish a minimal RA8P1 application scaffold

After the environment audit, instantiate the smallest proven Titan application under:

`platform/ra8p1/app/`

Prefer adapting the known-working Titan template rather than recreating startup/linker/FSP plumbing from memory.

However, import only what is needed for a reproducible M85 application.

The first target program must do nothing more than:

1. boot M85;
2. establish serial/log output;
3. report build identity;
4. report monotonic time;
5. report core clock if safely queryable;
6. prove a controlled reboot/restart path.

Expected evidence line shape:

```text
K1_RA8P1_BOOT=PASS CORE=M85 BUILD=<sha> MONOTONIC=<source> TOOLCHAIN=<version>
```

M33 remains parked.

Do not start audio here.

Gate:

`K1_RA8P1_BOOT=PASS`

Deliver:

- reproducible build command;
- reproducible flash command;
- serial/debug capture;
- board identity;
- negative proof: deliberately break the build identity or gate and show the checker turns red.

---

## 6. Phase 2 — import the smallest platform-neutral timing core

Import from the pinned DualMCU commit, preserving implementation semantics first:

```text
core/audio/media_time.h/.cpp
core/audio/musical_time.h/.cpp
core/audio/clock_affine.h/.cpp
core/audio/clock_exchange.h/.cpp
core/audio/peer_clock_sync.h/.cpp
core/system/media_time_correlation.h/.cpp
core/system/realtime_render_scheduler.h/.cpp
```

Destination should preserve clear include paths, e.g.:

```text
src/k1/core/audio/
src/k1/core/system/
```

Do not casually edit them during import.

First prove host parity in this greenfield repo using copied/adapted tests from the pinned authority.

At minimum preserve these properties:

- 48 kHz canonical `MEDIA_TIME` coordinate;
- 64-bit epoch/frame semantics;
- exact 48k↔24k and 48k↔12.8k mapping where applicable;
- Q32.32 fractional beat period;
- million-beat long-run prediction;
- event time distinct from result availability;
- peer affine map `N_peer ~= a*N_local+b`;
- four-stamp/peer-sync semantics;
- musical target scheduling with latency budget;
- target-neutral media↔monotonic correlation.

Do not weaken tests because RA8P1 uses a different compiler.

Gate:

```text
K1_RA8P1_SHARED_TIME_HOST=PASS
```

Then compile and execute the same core on M85.

Gate:

```text
K1_RA8P1_SHARED_TIME_M85=PASS
```

Where floating-point results are expected to differ by compiler/FPU, define measured tolerances before changing assertions and document why. Do not silently widen tolerances until green.

---

## 7. Phase 3 — import the MIR/DSP stack without optimisation

After shared-time parity is green, import the scalar AP components in dependency order:

```text
contract/audio_features_v1.h
core/audio/audio_rate_config.h
core/audio/gdft_goertzel.h/.cpp
core/audio/gdft_postprocess.h/.cpp
core/audio/chord_detect.h/.cpp
core/audio/onset_beat.h/.cpp
core/audio/tempo_acf.h/.cpp
core/audio/tempo_tracker.h/.cpp
core/audio/musical_saliency.h/.cpp
core/audio/audio_pipeline.h/.cpp
```

Only import additional dependencies when proven necessary.

Do not import RT1062 capture or FastLED implementation here.

Do not change GDFT math to use MVE yet.

Do not rewrite tempo/onset semantics.

Use the existing DualMCU fixtures/oracles as independent reference data. It is acceptable to copy fixture data into this repository if provenance and source commit are recorded. Do not make target output generate its own oracle.

Required parity classes:

- GDFT raw/output trajectory;
- GDFT postprocess trajectory;
- onset event IDs/flags/strength within defined FP tolerance;
- tempo/flywheel trajectory;
- chord outputs;
- musical saliency outputs;
- complete `AudioPipeline` output.

Gate:

```text
K1_RA8P1_AP_SCALAR_PARITY=PASS
```

A compile-only pass is insufficient. Run deterministic fixtures on M85 and compare outputs.

---

## 8. Phase 4 — benchmark the scalar M85 implementation

Before optimising anything, measure it.

For representative AP frames, capture:

- mean;
- median;
- p95;
- p99;
- max;
- CPU cycles if available;
- wall-time microseconds;
- stack high-water mark if available;
- heap allocation count/bytes;
- cache state;
- compiler optimisation level;
- code size;
- RAM usage.

Separate cheap frames from tempo/ACF update frames.

Compare to the known RT1062 scheduling contract, but do not fake a direct apples-to-apples comparison if capture/runtime context differs.

Current historical K1 concern to keep in view:

```text
12.8 kHz / 96-sample legacy contract: 7.5 ms hop
24 kHz / 180-sample production candidate: 7.5 ms hop
```

The RA8P1 lane is allowed to reveal substantially more headroom, but first demonstrate correctness.

Deliver:

`docs/evidence/K1-RA8P1-001/04-scalar-performance.md`

---

## 9. Phase 5 — establish the RA8P1 AUDIO_TIME hardware boundary

This is one of the most important hardware-specific parts of the port.

The target architecture is:

```text
physical audio sample/frame event
        ↓
SSIE/PDM + DMAC
        ↓
hardware capture boundary
        ↓
MEDIA_TIME_48K frame coordinate
        ↕
hardware monotonic timer
```

Do not reproduce the Teensy limitation where the best available observation is foreground dequeue/publication time unless the Titan hardware truly forces it.

Investigate the strongest available relation between:

- SSIE/TDM frame boundary;
- PDM/DMA boundary;
- DMAC transfer completion;
- GPT/AGT/free-running timer capture;
- interrupt entry;
- foreground processing.

The goal is an observation:

```cpp
(media_frame, monotonic_timestamp, uncertainty)
```

where uncertainty is measured/bounded rather than invented.

Preferred direction: hardware timer capture or the closest deterministic hardware event available to the sample/frame boundary.

Do not treat `uncertainty_us` as latency compensation.

Gate:

```text
K1_RA8P1_AUDIO_TIME_HW=PASS
```

Requires target execution evidence, not host simulation.

---

## 10. Phase 6 — audio capture bring-up

Use the Titan BSP `Titan_Mini_pdm` and `Titan_Mini_wavplayer` projects as references, not architecture authority.

First bring up the simplest known-good audio source already available on Titan, likely onboard PDM if that is what the user's current bench bring-up supports.

Prove:

- stable sample acquisition;
- sample rate measured/verified;
- bounded DMA buffering;
- no unbounded heap allocation in real-time path;
- frame/hop accounting;
- overrun detection;
- epoch/discontinuity behaviour;
- canonical media-frame advancement;
- media↔monotonic observation emitted from the capture adapter.

Do not yet require ADC6120/TAA5212 external TDM hardware to prove the software architecture.

After onboard PDM works, add a separate lane/subgate for the product-relevant external TDM contract:

```text
48 kHz
4 slots
32-bit slots
slot0 AUX-L
slot1 AUX-R
slot2 room mic
slot3 reserved
```

If the external ADC hardware is not connected to Titan, leave that as a clearly named pending physical gate.

Gate:

```text
K1_RA8P1_CAPTURE=PASS
```

---

## 11. Phase 7 — complete AP on live Titan audio

Compose:

```text
capture
→ canonical MEDIA_TIME
→ GDFT
→ postprocess
→ onset
→ chord
→ tempo/flywheel
→ MUSICAL_TIME
→ predicted BeatEvent
```

Preserve the existing architectural split:

- event timestamp = where event belongs in media time;
- result availability = when processing finished/published.

Do not stamp beats at CPU completion time.

Collect live evidence while also retaining deterministic fixture parity.

Gate:

```text
K1_RA8P1_AP_LIVE=PASS
```

---

## 12. Phase 8 — port K1-DM-142 musical render scheduling

The K1-DM-142 shared scheduler is already designed for this port.

Do not fork its policy.

The RA8P1 platform adapter must supply:

1. media/monotonic observation from capture;
2. `now` from the chosen monotonic timer;
3. conservative measured render cost;
4. physical LED output/latch cost;
5. target confidence and epoch semantics already supplied by shared code.

Preserve:

```text
predicted BeatEvent
→ MEDIA_TIME target
→ local media↔monotonic map
→ target event time
→ subtract render cost
→ subtract output/latch cost
→ kMusicalTarget
→ render-only beat edge
```

Do not copy the RT1062 values `1500 us uncertainty`, `200 us guard`, or `4800 us output budget` as Titan constants. Measure Titan.

Gate host/model first, then target.

```text
K1_RA8P1_MUSICAL_RENDER=PASS
```

Physical photon alignment must be reported separately:

```text
K1_RA8P1_PHOTON_PHASE=PASS|UNPROVEN
```

Never infer the second from the first.

---

## 13. Phase 9 — VP/render parity before physical LEDs

Do not begin by writing an RA8P1 WS2812 driver.

First import the platform-neutral visual engine needed for selected representative effects and prove pixel output parity in memory.

For identical:

- audio fixture;
- controls;
- tempo state;
- musical event;
- frame delta;

compare pixel-frame outputs/CRCs between reference and RA8P1 implementation.

Only after in-memory parity is green should you implement the Titan physical LED backend.

The physical backend must expose a measurable/bounded output completion/latch semantic suitable for K1-DM-142.

Gate:

```text
K1_RA8P1_VP_PARITY=PASS
K1_RA8P1_LED_OUTPUT=PASS|PENDING_HARDWARE
```

---

## 14. Phase 10 — do not add M33 or U55 yet

Cortex-M33 is parked for the entire first parity programme.

Study the vendor RPMsg example only to understand future options.

Do not introduce:

- RPMsg ownership;
- cross-core state replication;
- shared-memory AP/VP split;
- M33 watchdog/control partition;

until the M85-only stack is correct and measured.

Likewise, Ethos-U55 is parked.

Do not use the NPU for onset, classification, mood or instrument inference in K1-RA8P1-001.

The first decision must answer whether RA8P1 is a strong deterministic K1 processor **without requiring AI acceleration**.

---

## 15. Phase 11 — Helium/MVE optimisation is a separate experiment

Once scalar parity and scalar benchmark evidence are frozen, identify the hottest kernels.

Only optimise a kernel when:

1. its scalar baseline has deterministic fixture parity;
2. its runtime cost is measured;
3. the proposed MVE path has the same oracle;
4. a measurable improvement threshold is defined before implementation.

Likely candidates may include GDFT/spectral/vector operations, but do not assume.

Every optimised path must retain a scalar reference path or fixture differential sufficient to catch semantic drift.

Do not allow "faster" to redefine the algorithm.

---

## 16. Required failure semantics

Fail closed on:

- reference commit missing;
- toolchain identity unknown;
- target board identity ambiguous before flash;
- cross-epoch `AudioTime` comparison;
- sample-accounting loss;
- DMA overrun without epoch/gap handling;
- stale media↔monotonic mapping;
- invalid/low-confidence musical target;
- late musical target beyond policy tolerance;
- missing fixture/oracle;
- output fixture digest change without explicit update;
- target build that silently drops C++ source;
- heap use entering a bounded real-time path without explicit review;
- M33/U55 code appearing in first-lane runtime ownership.

---

## 17. Mutation / negative testing

For every major gate, prove at least one meaningful failure is observable.

Examples:

- corrupt a fixture digest;
- force media-rate mapping wrong;
- change Q32.32 period calculation;
- suppress epoch invalidation;
- force affine rate ratio to 1.0;
- disable musical target binding;
- inject an audio sample-accounting discontinuity;
- remove target build identity;
- force timestamp uncertainty to be omitted;
- introduce an M33 runtime ownership token and ensure policy gate rejects it.

Restore source after each mutation and prove green again.

Record receipts.

---

## 18. Evidence taxonomy

Every report must use explicit status classes:

```text
HOST_PASS
TARGET_COMPILE_PASS
TARGET_EXECUTION_PASS
PHYSICAL_PASS
UNPROVEN
BLOCKED
```

Do not report `PASS` without saying which class.

For timing results always state:

- clock source;
- sample rate;
- timer source;
- timestamp event;
- uncertainty/resolution;
- benchmark window/sample count;
- compiler flags;
- cache/FPU state;
- whether result is deterministic fixture, live board execution, or externally measured physical behaviour.

---

## 19. Commit strategy

Keep commits narrow and reviewable.

Recommended sequence:

```text
chore(ra8p1): establish Titan build and boot scaffold
feat(time): port shared media and musical time core
feat(audio): port scalar MIR pipeline to M85
feat(audio): add RA8P1 capture and audio-time adapter
feat(time): bind RA8P1 media clock to musical render scheduler
feat(visual): establish in-memory VP parity
feat(output): add measured RA8P1 LED backend
perf(mve): ... only after scalar baseline is frozen
```

Do not commit generated build outputs.

Do not use `git add -A` unless you have first enumerated every path and proven ownership.

Evidence/docs may be separate commits from executable changes.

---

## 20. Required deliverables

Create and maintain:

```text
docs/evidence/K1-RA8P1-001/
  00-environment-audit.md
  01-boot-gate.md
  02-shared-time-parity.md
  03-ap-scalar-parity.md
  04-scalar-performance.md
  05-audio-time-hardware.md
  06-capture-bringup.md
  07-live-ap.md
  08-musical-render.md
  09-vp-parity.md
  10-final-comparison.md
  mutation-receipts.txt
  final-validation.txt
```

Also produce:

`docs/RA8P1-vs-RT1062-RULING.md`

That final document must answer:

1. Does the existing K1 compute architecture port cleanly?
2. Which portions were genuinely platform-neutral?
3. Which portions required backend replacement?
4. Scalar M85 AP p50/p95/p99/max versus RT1062 evidence where comparable.
5. Memory/code-size headroom.
6. Audio capture/timestamp quality.
7. Musical render scheduling quality.
8. Toolchain/debug maturity.
9. BSP/driver friction.
10. Thermal/power observations if available.
11. What MVE could materially improve.
12. Whether M33 should remain parked.
13. Whether U55 has any justified K1 role yet.
14. Whether RA8P1 should be promoted to preferred DualMCU evaluation processor.

Allowed final rulings:

```text
PROMOTE_RA8P1_FOR_DUALMCU_EVAL
KEEP_RT1062_PRIMARY_DUALMCU_EVAL
CONTINUE_PARALLEL_EVALUATION
REJECT_RA8P1_FOR_K1
```

Do not rule on the single-S3 production architecture unless separately authorised. The DualMCU work remains an evaluation architecture unless the user changes that product-level decision.

---

## 21. First execution sequence

Execute in this order:

```text
A. verify this repo is clean and read AGENTS.md
B. ./scripts/check_references.sh
C. inspect current Titan bench/toolchain state read-only
D. write 00-environment-audit.md
E. establish reproducible M85 boot project under platform/ra8p1/app
F. commit the boot scaffold
G. import shared timing core from pinned DualMCU commit
H. establish host differential in this repo
I. compile + execute timing core on M85
J. import scalar AP modules and fixtures
K. establish M85 scalar AP parity
L. benchmark before optimising
M. build hardware AUDIO_TIME/capture adapter
N. compose live AP
O. bind K1-DM-142 scheduler using RA8P1 measurements
P. only then address VP and physical LED backend
Q. freeze final comparison/ruling
```

Do not skip directly to PDM or LED demos simply because vendor examples make them easy. The project goal is architectural parity, not a collection of peripheral demos.

---

## 22. Immediate first response expected from the CLI agent

Do not begin with a generic plan. Inspect the repository and host first, then report:

```text
REPO=
BRANCH=
BOOTSTRAP_COMMIT=
DUALMCU_REFERENCE_PIN=
TITAN_BSP_PIN=
TITAN_BOARD_VISIBLE=YES|NO
DEBUG_PROBE=
SERIAL_PORT=
CURRENT_BRINGUP_PROJECT=
TOOLCHAIN=
FRAMEWORK=
BUILD_SYSTEM=
M85_BOOT_STATUS=
M33_STATUS=
FIRST_EXECUTION_GATE=
BLOCKERS=
```

Then proceed end to end unless a genuinely destructive/ambiguous hardware action is required.

Do not ask the user to repeat information that can be discovered from the machine.

---

## 23. Central architectural rule

The most important rule in this whole lane is:

> Port the architecture; replace the backend.

If a source file contains Renesas/FSP/RT-Thread assumptions, it belongs in `platform/ra8p1/`.

If a source file expresses sample time, musical time, DSP semantics, clock mapping or scheduling policy, it should remain portable.

The success condition is not merely "K1 runs on Titan."

The success condition is:

> We can demonstrate with evidence that the same SpectraSynq compute semantics run on RT1062 and RA8P1, while hardware-specific capture/timer/output layers are replaceable backends.
