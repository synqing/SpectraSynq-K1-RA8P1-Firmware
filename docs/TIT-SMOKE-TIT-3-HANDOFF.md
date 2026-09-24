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

- **Wide-native16 switch, off (default)**: `packNative16Lane` with
  `use_wide_native16=false` byte-matches `packBenchGrb48Lane` for every
  input tried host-side, including a produced-but-unused wide frame. On
  target: confirm the real WS2816 emit path in `fixture_app.cpp` (now
  calling `packNative16Lane` instead of `packBenchGrb48Lane` directly)
  produces identical optical/electrical output to the pre-TIT-2 image at
  the same `PaletteConfig` — no wire decoder sets `use_wide_native16` yet,
  so every live `SET_CONFIG` should still land on the legacy path.
- **Wide-native16 switch, on**: a producer now exists
  (`PaletteRuntime::step()` fills `wide_a_`/`wide_b_` from each channel's
  rendered Pixel8 frame via `core::visual::wide::quantiseUnorm16`), and is
  reachable end-to-end through the fixture protocol. **Important, proven
  host-side**: because the source is already Pixel8-quantised, this
  producer's packed output is currently byte-identical to the legacy path
  for any real render (65535/255 == 257 exactly, so no Pixel8 value is ever
  near this exact law's rounding boundary) — turning the switch on today
  changes nothing observable, on host or target. Genuine divergence needs
  the renderer to expose a working-domain F32 frame upstream of Pixel8
  quantisation (the wide endpoint's E1-E4 stages: finite/enabled check,
  intensity*master gain, device transfer, shared current-limit scale) —
  not attempted this session; that is the concrete next step for whichever
  lane wants the wide route to actually change light output. On-target,
  once that exists: bind FP32/working-domain reference to
  `hd_pixel16`/packing at the final conversion point (lane brief step 6);
  test after all brightness/treatment operations; verify independent A/B
  and centre mapping are preserved.
- **`quantiseHd16` vs `quantiseUnorm16`** (DUR-010, DualMCU `b21294a`):
  `quantiseHd16Exact` + `quantiseHd16Selected(colour, use_exact_law)`
  exist in `platform/ra8p1/hd_pixel16.h`, default off, host-proven
  byte-identical to `quantiseHd16` for every input tried. No call site
  passes `use_exact_law=true` yet — the switch exists but nothing reads it
  on the live pixel path (it sits beside `quantiseHd16`, not yet spliced
  into `packBenchGrb48Lane`/`packWideNative16Lane`). Confirmed divergence:
  `+Inf` (`quantiseHd16` -> 0 via its `isfinite` guard; the exact law ->
  65535). Both agree on NaN (-> 0). On-bench: drive both laws with the same
  vector including `+Inf`/half-way-boundary values through whichever call
  site eventually wires the switch in, and confirm the same host identity
  holds on target hardware (it is a pure host-observable numeric law, not a
  timing-dependent one, so no new bench-only failure mode is expected).
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
