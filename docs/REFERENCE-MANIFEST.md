# Reference Manifest — K1-RA8P1-001

## SpectraSynq behavioural/reference source

- Local path: `/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-DualMCU-Firmware`
- Remote: `https://github.com/synqing/SpectraSynq-K1-DualMCU-Firmware.git`
- Required branch at bootstrap: `lane/k1-dm-142-beat-render-scheduler`
- Pinned authority commit: `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`
- K1-DM-142 code commit beneath it: `dbdff67fcaa00c6264d4f8ad4e0202cbb1d07677`
- Role: behavioural, algorithmic and timing-semantics reference. READ ONLY.

## Titan Mini vendor/platform reference

- Local path: `/Users/spectrasynq/Workspace_Management/Software/sdk-bsp-ra8p1-titan-mini`
- Remote: `https://github.com/RT-Thread-Studio/sdk-bsp-ra8p1-titan-mini.git`
- Pinned bootstrap commit: `6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7`
- Role: board support, startup, FSP/RT-Thread integration, drivers and known-working Titan examples. READ ONLY.

## Especially relevant BSP examples

- `project/Titan_Mini_template` — minimum project scaffold
- `project/Titan_Mini_pdm` — board PDM/audio capture reference
- `project/Titan_Mini_wavplayer` — audio dataflow/playback reference
- `project/Titan_Mini_rpmsg` — M85/M33 multicore reference; **study only, M33 remains parked**
- `project/Titan_Mini_usb_pcdc` — USB CDC bring-up reference
- `project/Titan_Mini_driver_all` — peripheral inventory/reference

The CLI agent must run `scripts/check_references.sh` before importing or comparing source. If a reference head has moved, do not silently follow it; use the pinned commit or document an explicit authority update.

`scripts/check_references.sh` accepts `--dualmcu-root`/`--bsp-root` (or
`K1_DUALMCU_ROOT`/`K1_TITAN_BSP_ROOT`) to read the two reference repos from a
non-Mac checkout path (e.g. a container where they sit as siblings of this
repository); this widens where the repos are read from, never what is
accepted -- every pin/ancestor check is unchanged and still exits non-zero on
a genuine miss (see `scripts/import_tit2_delta.py`'s own header comment and
`docs/reference-import-receipt-tit2.md` for a worked run of both the old and
new call forms). `scripts/verify_imports.py` similarly defaults `REFERENCE`
to `ROOT.parent / "SpectraSynq-K1-DualMCU-Firmware"` (a sibling-directory
convention that already resolves without an override wherever the two repos
are checked out side by side).

## TIT-2 scoped authority update — 2026-09-24

A second, narrower pin sits beside the bootstrap pin above:

- **`PIN_TIT2` = `16f70a9b2907c03c8262e79d05f5a05ed1795917`**, on DualMCU
  branch `claude/affectionate-euler-2k60bb`. `6b1e7bc5c9f9871e6ea4e900455bcb37d756304a`
  (the bootstrap pin) is a verified ancestor of it (forward move, not a
  rewind) -- `git -C <dualmcu> merge-base --is-ancestor 6b1e7bc5c9f9871e6ea4e900455bcb37d756304a 16f70a9b2907c03c8262e79d05f5a05ed1795917` exits 0.
- **Scope**: exactly the files `scripts/import_tit2_delta.py` names --
  `core/visual/wide/**`, `core/visual/modes/**`, `contract/tempo_field_v1.*`,
  `core/audio/{tempo_field,tempo_phase_tracks}.*`, `contract/control_v2/**`,
  `core/control/v2/**`, plus four existing `product`-slice files DualMCU
  changed on the way from the bootstrap pin to this one:
  `core/audio/audio_pipeline.h`, `core/visual/channel_render_state.h`,
  `core/visual/product_effect_renderer.cpp`, `core/visual/visual_audio_frame.h`.
- **Explicitly excluded**: `core/audio/gdft_goertzel.{h,cpp}` also changed
  between the two commits (DualMCU commit `321ccb6`, "run both PDM capsules
  and land the live pair") -- an AP-owned PDM-capture change with no
  relationship to this delta. Importing it was never asked for and would be
  an unexplained compatibility change, so it stays on the bootstrap pin;
  `src/k1/core/audio/gdft_goertzel.{h,cpp}` and their `docs/IMPORT-MANIFEST.tsv`
  rows are untouched.
- **Mechanism**: `scripts/import_tit2_delta.py --write` copies the pinned
  bytes into `src/k1/` and upserts `docs/IMPORT-MANIFEST.tsv`;
  `--check` recomputes every hash from the reference commit and diffs
  against both the manifest and the on-disk destination with no writes, so
  the import is independently re-verifiable at any later date without
  re-running `--write`. `scripts/verify_imports.py`'s single global `PIN`
  check became a `row["source_commit"] in {PIN, PIN_TIT2}` check (see that
  file's own comment) so a mixed-pin manifest still fails closed on any
  genuine divergence; `scripts/commit_for(name)` looks up each row's own
  pin for donor-side fetches in `run_host.py`/`run_product_host.py`.
- **Receipt**: `docs/reference-import-receipt-tit2.md`.
