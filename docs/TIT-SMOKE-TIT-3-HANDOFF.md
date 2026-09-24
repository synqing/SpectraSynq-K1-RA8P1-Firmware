# TIT-SMOKE / TIT-3 local handoff — exact next execution step

Written by TIT (host-only session, no ARM toolchain/SCons/sibling BSP/E2 Lite
router/rfp-cli/board present). Everything below is either a command that ran
successfully in this container, or a command whose argv is resolved from
this repository's own source but was never executed here — each is marked.
Source: `scripts/build_scalar.py`, `scripts/programme_scalar.py`,
`scripts/check_references.sh`, `docs/REFERENCE-MANIFEST.md`,
`platform/ra8p1/README.md`, and the prior TIT-1/TIT-DEPLOY0 pass captured in
DualMCU `docs/evidence/k1-programme-2026-09-24/platform/{titan-inventory.json,deployment-command-manifest.titan.json}`.
This document supersedes nothing in that pass; it adds the TIT-2 delta
(`docs/reference-import-receipt-tit2.md`) to the build inputs and gives the
concrete next command a bench/CI runner executes.

## 0. What changed since the TIT-1 pass that affects this build

`src/k1/` now also carries the TIT-2 delta (61 files: `core/visual/wide/**`,
`core/visual/modes/**`, `contract/tempo_field_v1.*`,
`core/audio/{tempo_field,tempo_phase_tracks}.*`, `contract/control_v2/**`,
`core/control/v2/**`, plus 4 updated `product`-slice files). `platform/ra8p1/palette_runtime.{h,cpp}`
gained `packWideNative16Lane`/`packNative16Lane`, a step()-driven
`DeviceRgb16Frame` producer (`wideFrame()`), and `PaletteConfig::use_wide_native16`
(default `false`; no current wire decoder sets it, and the producer is
proven a no-op on packed bytes even when forced on host-side -- see
`docs/reference-import-receipt-tit2.md` and commit `080b75b` -- so this
build is behaviourally identical to the pre-TIT-2 build at every existing
opcode). `platform/ra8p1/hd_pixel16.h` gained `quantiseHd16Exact`/
`quantiseHd16Selected` (DUR-010, default off, unwired to any pixel path
yet). `platform/ra8p1/ctl_capability.h` is new (fixture opcode 21,
read-only, always compiled in). `scripts/build_scalar.py`'s own
`SConscript` walk of `platform/ra8p1/k1/` (which it stages from `src/k1/`
before SCons runs) picks up the new files automatically — no wrapper-argv
change is needed for the new source.

## 1. Build (RESOLVED_NOT_EXECUTED here — no sibling BSP/toolchain)

```
python3 scripts/build_scalar.py --output <new-external-directory>
```

Preconditions this container cannot satisfy: a sibling checkout at
`../sdk-bsp-ra8p1-titan-mini` (pinned `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`,
verified absent here), `arm-none-eabi-g++`/SCons on `PATH` (verified absent
here — `pip3 show scons` -> not found, `which arm-none-eabi-g++` -> nothing).
The wrapper's `--help` (21 flags) was captured by the prior TIT-1 pass;
nothing in this session's changes adds or removes a flag. Recommended flags
for the first scalar candidate that includes the TIT-2 delta, matching this
repository's existing scalar-only, no-optimisation-experiment posture:
`--optimisation o2 --dcache enabled --palette-runtime --palette-autostart`.
Add `--palette-ws2816` only once the wide-native16 switch is actually turned
on for a run (see step 5) — it is off by default and needs no build flag by
itself, since it is a `PaletteConfig` runtime field, not a compile switch.

`scripts/check_references.sh --dualmcu-root /home/user/SpectraSynq-K1-DualMCU-Firmware`
now checks both `DUAL_PIN` and `DUAL_PIN_TIT2`; on a bench host that also has
the BSP sibling checked out, run it with both roots before `build_scalar.py`
and treat a non-zero exit as a hard stop, per AGENTS.md item 15.

**Required postcondition** (unresolved, needs the bench/CI host): staged
BSP+K1 image under `<output>` with map/disassembly/compiler-macro/stack-usage
files and source/build hashes retained (`platform/ra8p1/README.md:20-26`).
Capture ELF/map/BIN sha256 into the run's own receipt directory; do not
reuse a prior run's receipt directory (AGENTS.md item 19: "Never reuse an
existing programme/run receipt directory").

## 2. Host-only checks that DID run here, successfully, this session

```
$ python3 scripts/import_tit2_delta.py --check
K1_TIT2_IMPORT=CHECK_PASS files=61 pin=16f70a9b2907c03c8262e79d05f5a05ed1795917

$ python3 scripts/verify_imports.py --slice product --enforce
{"slice": "product", ..., "pass": true}   # 102/102

$ python3 scripts/test_palette_runtime.py
... PALETTE_COMPATIBILITY_PASS independent_executables=2 overlay_mutation_rejected=true live_audio_differs=true
```

Run these three again on the bench/CI host before trusting the build step;
they are cheap (host g++, no board) and catch a source-identity or
compile-time regression before spending a programming cycle.

## 3. Programmer route (UNRESOLVED here — no E2 Lite/rfp-cli/board)

```
<load the E2 Lite router skill's current rfp-cli invocation>   # identify-target
python3 scripts/programme_scalar.py --build <build-directory> --output <new-run-directory> --execute   # program
```

Both are UNRESOLVED, not merely unexecuted: `scripts/programme_scalar.py`
cannot even import here (`ModuleNotFoundError: No module named
'titan_ra8p1_boot'` — that module ships with the `ra8p1-titan-engineering`
skill package named in AGENTS.md item 13, which is not installed in this
container; grep-confirmed the module is real, not a typo — it is imported at
`scripts/programme_scalar.py:28` and stubbed only inside
`scripts/test_programme_scalar_events.py`'s own test harness). No `rfp-cli`
binary exists anywhere in this container (filesystem search). **Do not**
invent an argv for either step; the bench operator must resolve them from
the live skill package and the router's own `--help`, per AGENTS.md items
13/19-25 (Rearm protocol: prepare image+hashes+output dir+exact command
first, start the programmer waiter, reply exactly `WAITING`).

Preconditions once both resolve, unchanged from `platform/ra8p1/README.md:47-53`
and this repo's own `AGENTS.md`/`docs/agent`-equivalent operator protocol:
exact canonical UID match before any write; USER/BOOT held through reset and
programming; release only after `PROGRAMME_VERIFY_PASS`; never kill an IDE
attached to the target out of band; a pinmap edit is not a wire move.

## 4. Readback / verify (UNRESOLVED here, downstream of step 3)

Bind the receipt to target UID, probe, address/range and image hash
(`platform/ra8p1/README.md:47-53`). If no image change is required (i.e. the
new build's source hashes match the currently-installed image's recorded
source hashes), retain the identified installed bytes and do not reflash —
this is not a blanket no-flash policy, it only applies when the source truly
did not change.

## 5. What to measure once a candidate carrying the TIT-2 delta is on target

All of these are `NOT_TESTED (needs bench)` from this session; each needs
the acceptance evidence the 09-titan-platform lane brief's table already
specifies (rate identity, AP timing/loss, VP/output timing, GPT/DMA
completion, native precision/black, memory, G3/G4 loaded runs, progress).
Specific to this delta, add:

All four items below are now IMPLEMENTED + HOST_PASS (commits `cfdaf6d`,
`98e1bb1`, `e89e7c6`, `080b75b`); this section says what an on-target run
adds beyond the host proof already in each commit message and
`docs/reference-import-receipt-tit2.md`.

- **DUR-010 disposition**: **no live caller; not implemented.**
  `quantiseHd16Exact`/`quantiseHd16Selected` were removed (Captain ruling:
  "a switch nothing reads is not a feature"). `quantiseHd16` itself is
  unchanged in behaviour except a real fix: its finiteness guard is now
  `isFiniteBits()` (bit-level), not `std::isfinite()`, because `-ffast-math`
  licenses the compiler to fold `isfinite()` to `true`, silently flipping
  the `+Inf` case from 0 to 65535. Host-proven both ways
  (`scripts/test_hd_pixel16.py` runs normal flags and `-ffast-math` and
  asserts identical output). Divergence vectors (`+Inf` diverges 0-vs-65535;
  NaN agrees at 0) remain as evidence in `tests/host/test_hd_pixel16.cpp`
  for if/when a caller appears. No on-target check needed: this is a pure
  host-observable numeric law with no live call site.
- **DUR-011 native-output path, off (`use_wide_native16=false`, default)**:
  `PaletteRuntime::step()` calls `resolveNativeOutputV1` (VP's
  `core/visual/wide/wide_native_output.h`, DualMCU pin `5b34f98`) only when
  the flag is set; `packNative16Lane` with the flag off byte-matches
  `packBenchGrb48Lane` for every case tried host-side (`WIDE_ROUTE_A1_PASS`,
  `WIDE_ROUTE_A4_PASS` — the latter also covers `use_wide_route` on/off with
  native16 off, per ORCH's A4 scope). On target: confirm the real WS2816
  emit path in `fixture_app.cpp` produces identical optical/electrical
  output to the pre-DUR-011 image at the same `PaletteConfig` — no wire
  decoder sets either flag yet, so every live `SET_CONFIG` still lands on
  the legacy path.
- **DUR-011 native-output path, on**: genuinely wired end to end now (the
  round-3 Pixel8-lift producer is deleted). Capture happens before
  `applyProductOutputTreatment` mutates `frame()`; the legacy Pixel8
  treatment still runs unconditionally (packBenchGrb48Lane/status
  CRCs/dwell path need it), so `resolveNativeOutputV1`'s internal FP32
  treatment is given a snapshot-and-restore of `ProductOutputTreatmentState`
  rather than the real one, so only one treatment's dither-phase advance
  persists per frame. Brightness maps to `EndpointConfigV1::master`
  (`brightness/255`) once, in the FP32 drive domain — the 16-bit words are
  **not** rescaled again in the pack (`packWideNative16Lane` no longer
  multiplies by `config_.brightness`). **Host-proven on live render output**
  (`WIDE_ROUTE_A2_PASS`, mode 3/Bloom, 90 `step()` calls with a genuinely
  time-varying `chroma_a_origin` audio vector — a left-zeroed vector renders
  exactly black regardless of `peak_scaled`/`vu_level`, which only gate
  musical-presence/`keep_live`, not colour): native words are **not**
  confined to the legacy lift's x257 lattice, and at least one pair of
  distinct pixels whose Pixel8 (8-bit) codes are equal have distinct native
  16-bit words — genuine sub-8-bit precision recovery, not a relabelled
  lift. `WIDE_ROUTE_A3_MUTATION_PASS` proves this isn't vacuous: forcing
  capture to fall back to `kLiftedPixel8` (mode 0/preview, outside
  `kWideRouteAdmittedModesV1`) makes every native word land back on the
  x257 lattice, exactly as the lifted case should. On target: WS2816
  capture with mode 3 routed + native16 on, confirm the captured wire bytes
  are off-lattice and that two same-Pixel8-code pixels carry distinct
  native words (the A2/A3 host proof, now on real hardware); confirm
  default-off (`use_wide_native16=false`) still matches the pre-DUR-011
  image byte for byte at the wire.
- **Current limiting**: Titan has **no incumbent current limiter**
  (grep-confirmed: nothing under `platform/ra8p1/` mentions
  `current_ma`/`max_current`/`current_limit`). `PaletteRuntime::endpointConfig()`
  reproduces "no limiting" by setting `EndpointConfigV1::max_current_ma` to
  `1e6` (the top of its declared valid range `[100, 1e6]`) — the largest
  physically possible two-channel draw (160 px x 3 components x 20 mA x 2
  channels = 19,200 mA) sits far below it, so `resolveWideEndpointV1`'s
  limiter is mathematically inert. Host-proven: `WIDE_ROUTE_A2_PASS`'s
  near-saturated centre-injected render still reports `limiter_scale == 1.0`
  and zero `nonfinite` counts on both channels (`runtime.endpointReport()`).
  On target: confirm the same at real full-white/full-brightness (the
  genuine physical worst case, not just this host proxy).
- **Edge policy**: Titan never calls `applyProductEdgePolicy` anywhere in
  `platform/ra8p1/` (grep-confirmed), so there is no coverage gap on Titan
  for this path — nothing to reconcile on target.
- **A5 (no allocation in step())**: by construction — `wide_workspace_`,
  `wide_a_`/`wide_b_`, `wide_route_a_`/`wide_route_b_` are all
  `PaletteRuntime` members, never step()-local. Host cost figure (labelled
  host, not a target timing claim): ~20 us/`step()` call on host g++ O2,
  both channels, `use_wide_native16` on (`WIDE_ROUTE_A5_HOST_COST` in the
  test output) — includes the legacy Pixel8 render/treatment this cycle
  still runs too, not just the wide path in isolation.
- **CTL_CAPABILITY (fixture opcode 21)**: implemented, read-only, host-proven
  through the real K1S1-framed wire protocol (not an in-process call) —
  `contract/control_v2/generated/control_registry_v2.generated.h`'s own
  `PlatformProfile{Platform::kTitan, ...}` row reports `max_pages=0` (no
  pages), `persistence_slot_bytes=0` (no persistence),
  `service_mask` carrying only `kServiceRouteFixture` (no BLE, no profile
  transfer) — truthful by construction, not asserted by the response code.
  On target: send the real opcode-21 packet over actual USB-CDC and confirm
  the same JSON. Note the correction to the originally proposed opcode: 20
  was already `status_command`; 21 is the first free slot (1-20 and 22 all
  reserved; `K1_WS281X_GPT_DIAG_OPCODE`=22).

## 6. Recovery

Unresolved here (no board). Use the saved compatible image plus matching
config/schema (lane brief step 8): after any rollback, verify bytes,
application identity, restored readback and continuing output. Preserve
failed-run evidence; start a new run directory after any repair; release
leases only with a truthful target/output leave-state.
