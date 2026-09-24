# TIT-2 portable-import receipt

Old reference commit (bootstrap pin, unchanged): `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`
New reference commit (`PIN_TIT2`, this delta): `16f70a9b2907c03c8262e79d05f5a05ed1795917`
Branch: DualMCU `claude/affectionate-euler-2k60bb`. Ancestry verified:
`git merge-base --is-ancestor <old> <new>` exits 0 (forward move, not a rewind).

## Mechanism

`scripts/import_tit2_delta.py --write` performs the copy and upserts
`docs/IMPORT-MANIFEST.tsv`; `--check` recomputes and diffs with no writes, so
the import is re-verifiable at any time. `scripts/verify_imports.py --slice
product --enforce` is the acceptance gate; it now accepts either pin per row
(`ALLOWED_PINS = {PIN, PIN_TIT2}`) instead of one global pin, so a mixed-pin
manifest still fails closed on any content it cannot account for.

## Exact changed/added files (61 total)

Full list with per-file `sha256`/`bytes`/`source_commit` is
`docs/IMPORT-MANIFEST.tsv` (source of truth; not duplicated here to avoid a
second copy going stale). Grouped by kind:

- **New — `core/visual/wide/**`** (19 files): the wide endpoint (types,
  colour, temporal, material, palette, field, endpoint, profile, bloom,
  descriptor).
- **New — `core/visual/modes/**`** (7 files): Liveiness v1 contract,
  per-family adapters (spectrum/waveform/rhythm/material), registry and the
  CTL v2 modulator hook.
- **New — `contract/tempo_field_v1.{h,cpp}` + `core/audio/{tempo_field,tempo_phase_tracks}.{h,cpp}`**
  (6 files): the TempoFieldV1 sidecar contract and its AP-side producer.
- **New — `contract/control_v2/**` + `core/control/v2/**`** (25 files): CTL
  v2 wire objects, codec, generated registry/profile, and the engine
  (binding math, response dynamics, persistence, profile compiler, registry
  view, control engine).
- **Updated (4 files, already present at the bootstrap pin)**:
  `core/audio/audio_pipeline.h` (adds a `const`, read-only
  `tempoTrackerState()` accessor -- observer-only, cannot feed back into the
  tracker), `core/visual/channel_render_state.h` (adds
  `modes::LiveinessInput liveiness{}` to `ChannelVisualControls`, default
  `{amount=kLiveinessNeutral, enabled=false}` -- stated by DualMCU to be
  exact identity in every mode), `core/visual/product_effect_renderer.cpp`
  (routes through `modes::liveinessEffective(...)` at every per-pixel
  velocity/drift site, reads `visual_audio_frame.tempo_field`, and gains the
  wide-route hook DualMCU's own commit message describes as "off by
  default"), `core/visual/visual_audio_frame.h` (adds
  `const contract::TempoFieldV1* tempo_field = nullptr` to
  `VisualAudioFrameView` -- additive, defaults null).

## Explicitly excluded

`core/audio/gdft_goertzel.{h,cpp}` also changed between the two commits
(DualMCU `321ccb6`, "run both PDM capsules and land the live pair" -- an
AP-owned PDM-capture change unrelated to this delta). Not imported; its
manifest rows stay on the bootstrap pin, matching `src/k1/core/audio/gdft_goertzel.{h,cpp}`
verbatim.

## Expected behavioural delta and compatibility proof

Claimed delta: **none observable under the existing `product`-slice host
harness with every new default left untouched.** `ChannelVisualControls`'s
new `liveiness` member defaults to the disabled/neutral `LiveinessInput`;
`VisualAudioFrameView`'s new `tempo_field` pointer defaults null;
`AudioPipeline::tempoTrackerState()` is additive and unused by the existing
fixture. Proof (host g++ 13.3, this container, 2026-09-24):

```
$ python3 scripts/import_tit2_delta.py --check
K1_TIT2_IMPORT=CHECK_PASS files=61 pin=16f70a9b2907c03c8262e79d05f5a05ed1795917

$ python3 scripts/verify_imports.py --slice product --enforce
{"slice": "product", "mode": "ACCEPTANCE", "required": 102, "checked": 102,
 "missing": 0, "divergent": 0, "errors": [], "pass": true}
```

Compiled the full `product`-slice `.cpp` set against
`tests/host/trajectory_main.cpp` twice -- once against `src/k1` (candidate),
once against a donor tree fetched per-row via `commit_for()` (mixed PIN /
PIN_TIT2, i.e. an independent reconstruction of exactly what the manifest
claims is pinned) -- with identical compiler flags
(`-std=c++17 -ffp-contract=off -fno-fast-math -O2`). The two binaries are
byte-identical, and running both against the 45 s synthetic corpus
`scripts/run_product_host.py` itself generates (silence / tone / tempo
switch / dropout / transients) produces byte-identical traces
(`diff -q` empty, 3,162,000 trace lines, `rendered=1` on 5,400 of 6,000
frames across mode IDs {3,7,8,9,11-16,18-29,32}).

**Negative-case proof (the comparator is not vacuously always-pass)**:
tampering one byte into an imported destination file
(`core/visual/wide/wide_bloom.cpp`) made `verify_imports.py --slice product
--enforce` correctly fail with `"divergent": 1,
"errors": ["destination differs from pinned source: core/visual/wide/wide_bloom.cpp"]`;
the file was then restored via `import_tit2_delta.py --write` (idempotent)
and re-verified clean. `run_product_host.py` itself cannot be run end-to-end
in this container (it unconditionally shells out to `ffmpeg -version` even
with no `--song` given, and `ffmpeg` is not installed and cannot be
installed here -- apt-get is blocked by this container's egress policy);
this is a pre-existing environment gap in that script, not caused by this
delta, and is reproduced identically on a clean checkout before this import.

**What this proof does NOT show**: whether `liveiness`/`tempo_field` change
behaviour once genuinely enabled (out of scope for TIT-2's import step;
belongs to whichever lane turns the switch on), any RA8P1 target-timing or
memory-budget claim (no toolchain/board here -- see TIT-SMOKE/TIT-3 handoff),
and the `docs/PALETTES.md`-style AP/VP numerical-parity claims that live
DualMCU's own test suite already carries for the bootstrap-pin files.

## Preserved compatibility vectors

`product` slice retains every entry it had before this delta (the slice
membership check in `verify_imports.py` -- `"product regression must retain
every accepted timing import"` -- still holds; `timing` slice is untouched).
No file outside the 61 listed above was written by
`scripts/import_tit2_delta.py`.

## Round 4 addendum: DUR-011 native-output interface (PIN_TIT2_ROUND4)

A second, further-scoped pin sits beside `PIN_TIT2`:
**`PIN_TIT2_ROUND4` = `5b34f98085e85cb6b58d8b61290a932d9e22e8fe`**, verified
ancestor-forward of `PIN_TIT2` (`git merge-base --is-ancestor 16f70a9... 5b34f98...`
exits 0). Scope: exactly `core/visual/wide/wide_native_output.{h,cpp}` (VP's
DUR-011 capture/FP32-treatment/resolve interface) -- grepped their own
`#include` lines to confirm they depend only on already-imported
`wide_bloom.h`/`wide_endpoint.h`/`wide_types.h`/`channel_render_state.h`, so
nothing else was pulled in. **Explicitly excluded**, though they changed in
the same DualMCU commit range: `contract/bridge_protocol.{h,cpp}` and
`core/control/bridge_command_processor.h` (a different lane's work, already
dirty in this shared checkout before this session started), CTL's own
`core/control/v2/control_engine.{h,cpp}` + the `control_v2` generated
JSON/`control_v2_types.h` (not asked for here), and
`core/visual/modes/mood_liveiness_migration_v1.h` +
`core/audio/tempo_field.cpp` (also changed; `wide_native_output` includes
neither). `docs/IMPORT-MANIFEST.tsv` rows for all of these stay on their
prior pin, matching their `src/k1` content verbatim. DUR-010's earlier
disposition ("no live caller; not implemented" -- see
`platform/ra8p1/hd_pixel16.h`'s own comment, which is the current source of
truth) is unaffected by this addendum: `quantiseHd16Exact`/
`quantiseHd16Selected` were removed, not imported alongside this round's
`wide_native_output` work.
