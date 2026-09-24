#!/usr/bin/env python3
"""TIT-2 explicit portable-import delta: wide endpoint, liveiness modes,
TempoFieldV1 sidecar and CTL v2 (contract + engine), plus the four existing
product-slice files DualMCU changed underneath them.

This is Titan's own reference process (see scripts/verify_imports.py,
docs/REFERENCE-MANIFEST.md) applied to one explicit, named delta. It is
mechanical and re-runnable:

  --write   copy the pinned bytes into src/k1/ and upsert docs/IMPORT-MANIFEST.tsv
  --check   recompute every row this delta owns from the reference commit and
            diff against both the manifest and the on-disk destination, with
            no writes; exit non-zero on any mismatch

Only the paths listed in NEW_FILES/BUMPED_FILES are touched. Every other
IMPORT-MANIFEST.tsv row (e.g. core/audio/gdft_goertzel.{h,cpp}, which also
changed between OLD_PIN and NEW_PIN for an unrelated AP-owned reason -- see
docs/reference-import-receipt-tit2.md) is left exactly as it is: importing
it was never asked for and would be an unexplained compatibility change.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT.parent / "SpectraSynq-K1-DualMCU-Firmware"
OLD_PIN = "6b1e7bc5c9f9871e6ea4e900455bcb37d756304a"
NEW_PIN = "16f70a9b2907c03c8262e79d05f5a05ed1795917"
MANIFEST = ROOT / "docs/IMPORT-MANIFEST.tsv"

# Files already present in IMPORT-MANIFEST.tsv at OLD_PIN whose content this
# delta updates (DualMCU changed them on the way from OLD_PIN to NEW_PIN, and
# the TIT-2 assignment names them explicitly).
BUMPED_FILES = [
    "core/audio/audio_pipeline.h",
    "core/visual/channel_render_state.h",
    "core/visual/product_effect_renderer.cpp",
    "core/visual/visual_audio_frame.h",
]

# Files new to the manifest, all pinned at NEW_PIN.
NEW_FILES = [
    # contract/tempo_field_v1 + core/audio tempo sidecar
    "contract/tempo_field_v1.h",
    "contract/tempo_field_v1.cpp",
    "core/audio/tempo_field.h",
    "core/audio/tempo_field.cpp",
    "core/audio/tempo_phase_tracks.h",
    "core/audio/tempo_phase_tracks.cpp",
    # core/visual/wide/**
    "core/visual/wide/wide_types.h",
    "core/visual/wide/wide_colour.h",
    "core/visual/wide/wide_colour.cpp",
    "core/visual/wide/wide_temporal.h",
    "core/visual/wide/wide_temporal.cpp",
    "core/visual/wide/wide_material.h",
    "core/visual/wide/wide_material.cpp",
    "core/visual/wide/wide_palette.h",
    "core/visual/wide/wide_palette.cpp",
    "core/visual/wide/wide_field.h",
    "core/visual/wide/wide_field.cpp",
    "core/visual/wide/wide_endpoint.h",
    "core/visual/wide/wide_endpoint.cpp",
    "core/visual/wide/wide_profile.h",
    "core/visual/wide/wide_profile.cpp",
    "core/visual/wide/wide_bloom.h",
    "core/visual/wide/wide_bloom.cpp",
    "core/visual/wide/wide_descriptor.h",
    "core/visual/wide/wide_descriptor.cpp",
    # core/visual/modes/**
    "core/visual/modes/liveiness_contract.h",
    "core/visual/modes/spectrum_liveiness.h",
    "core/visual/modes/waveform_liveiness.h",
    "core/visual/modes/rhythm_liveiness.h",
    "core/visual/modes/material_liveiness.h",
    "core/visual/modes/liveiness_registry.h",
    "core/visual/modes/liveiness_control_hook.h",
    # contract/control_v2/**
    "contract/control_v2/control_v2_types.h",
    "contract/control_v2/control_v2_objects.h",
    "contract/control_v2/control_v2_draft.h",
    "contract/control_v2/control_v2_codec.cpp",
    "contract/control_v2/schema/control_profile_v2.schema.json",
    "contract/control_v2/schema/control_registry_v2.schema.json",
    "contract/control_v2/registry/control_registry_v2.json",
    "contract/control_v2/profiles/default_profile_v2.json",
    "contract/control_v2/platforms/capacity_profiles_v2.json",
    "contract/control_v2/generated/control_registry_v2.generated.h",
    "contract/control_v2/generated/control_profile_default_v2.generated.h",
    "contract/control_v2/generated/control_bundle_manifest.json",
    "contract/control_v2/generated/capability_matrix_v2.json",
    # core/control/v2/**
    "core/control/v2/binding_math.h",
    "core/control/v2/binding_math.cpp",
    "core/control/v2/registry_view.h",
    "core/control/v2/registry_view.cpp",
    "core/control/v2/response_dynamics.h",
    "core/control/v2/response_dynamics.cpp",
    "core/control/v2/persistence.h",
    "core/control/v2/persistence.cpp",
    "core/control/v2/profile_compiler.h",
    "core/control/v2/profile_compiler.cpp",
    "core/control/v2/control_engine.h",
    "core/control/v2/control_engine.cpp",
]

ALL_FILES = BUMPED_FILES + NEW_FILES


def reference_bytes(path: str, commit: str = NEW_PIN) -> bytes:
    return subprocess.run(
        ["git", "-C", str(REFERENCE), "show", f"{commit}:{path}"],
        capture_output=True, check=True,
    ).stdout


def load_manifest() -> tuple[list[str], dict[str, dict]]:
    with MANIFEST.open() as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        fieldnames = reader.fieldnames
        rows = {row["path"]: row for row in reader}
    return fieldnames, rows


def write_manifest(fieldnames: list[str], rows: dict[str, dict]) -> None:
    ordered = sorted(rows.values(), key=lambda r: r["path"])
    with MANIFEST.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, delimiter="\t", fieldnames=fieldnames)
        writer.writeheader()
        for row in ordered:
            writer.writerow(row)


def do_write() -> int:
    fieldnames, rows = load_manifest()
    for path in ALL_FILES:
        data = reference_bytes(path, NEW_PIN)
        destination = ROOT / "src/k1" / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        rows[path] = {
            "path": path,
            "sha256": hashlib.sha256(data).hexdigest(),
            "bytes": str(len(data)),
            "source_commit": NEW_PIN,
        }
        print(f"WROTE {path} bytes={len(data)}")
    write_manifest(fieldnames, rows)
    print(f"K1_TIT2_IMPORT=WRITE files={len(ALL_FILES)} pin={NEW_PIN}")
    return 0


def do_check() -> int:
    _, rows = load_manifest()
    errors = []
    for path in ALL_FILES:
        data = reference_bytes(path, NEW_PIN)
        expected_sha = hashlib.sha256(data).hexdigest()
        row = rows.get(path)
        if row is None:
            errors.append(f"missing manifest row: {path}")
            continue
        if row["source_commit"] != NEW_PIN:
            errors.append(f"manifest source_commit stale: {path} has {row['source_commit']}")
        if row["sha256"] != expected_sha or int(row["bytes"]) != len(data):
            errors.append(f"manifest hash/size stale: {path}")
        destination = ROOT / "src/k1" / path
        if not destination.is_file():
            errors.append(f"destination missing: {path}")
            continue
        if destination.read_bytes() != data:
            errors.append(f"destination diverges from pinned reference: {path}")
    if errors:
        for error in errors:
            print(f"ERROR {error}")
        print(f"K1_TIT2_IMPORT=CHECK_FAIL files={len(ALL_FILES)} errors={len(errors)} pin={NEW_PIN}")
        return 2
    print(f"K1_TIT2_IMPORT=CHECK_PASS files={len(ALL_FILES)} pin={NEW_PIN}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="copy pinned bytes and upsert the manifest")
    mode.add_argument("--check", action="store_true", help="recompute and diff only; no writes")
    args = parser.parse_args()
    if not (REFERENCE / ".git").is_dir():
        print(f"K1_TIT2_IMPORT=BLOCKED reason=missing_reference path={REFERENCE}")
        return 1
    return do_write() if args.write else do_check()


if __name__ == "__main__":
    raise SystemExit(main())
