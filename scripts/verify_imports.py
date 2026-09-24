#!/usr/bin/env python3
"""Check pinned source identity and explicit import-slice completeness."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT.parent / "SpectraSynq-K1-DualMCU-Firmware"
PIN = "6b1e7bc5c9f9871e6ea4e900455bcb37d756304a"
# TIT-2 explicit delta (wide endpoint, liveiness modes, TempoFieldV1 sidecar,
# CTL v2): a forward, ancestor-verified authority update from PIN, scoped to
# the exact files scripts/import_tit2_delta.py names. PIN itself is
# unchanged and still governs every other manifest row -- see
# docs/reference-import-receipt-tit2.md for why this is scoped rather than a
# blanket PIN bump (DualMCU changed core/audio/gdft_goertzel.* between the
# two commits for an unrelated, AP-owned reason that is explicitly NOT
# imported here).
PIN_TIT2 = "16f70a9b2907c03c8262e79d05f5a05ed1795917"
ALLOWED_PINS = {PIN, PIN_TIT2}


def commit_for(name: str, root: Path = ROOT) -> str:
    """The manifest's declared source_commit for one src/k1-relative path.

    Donor-fetch call sites (run_host.py, run_product_host.py, ...) use this
    instead of the bare PIN constant so a scoped authority update such as
    PIN_TIT2 fetches each file from the commit its own manifest row actually
    names, never silently from PIN for files that only exist at PIN_TIT2.
    Falls back to PIN when a path has no manifest row (donor-only fixtures
    such as test/... are not import-manifest entries).
    """
    with (root / "docs/IMPORT-MANIFEST.tsv").open() as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            if row["path"] == name:
                return row["source_commit"]
    return PIN


def safe_path(root: Path, name: str) -> Path:
    p = PurePosixPath(name)
    if not name or p.is_absolute() or ".." in p.parts or str(p) != name or "\\" in name:
        raise ValueError(f"invalid path: {name!r}")
    resolved = (root / name).resolve()
    if not resolved.is_relative_to(root.resolve()):
        raise ValueError(f"path escapes root: {name}")
    return resolved


def verify(root: Path, reference: Path, slice_name: str, enforce: bool) -> dict:
    result = dict(slice=slice_name, mode="ACCEPTANCE" if enforce else "INFORMATIONAL",
                  required=0, checked=0, missing=0, divergent=0, errors=[])
    with (root / "docs/IMPORT-MANIFEST.tsv").open() as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    entries = {}
    for row in rows:
        name = row["path"]
        safe_path(root / "src/k1", name)
        if name in entries:
            raise ValueError(f"duplicate manifest entry: {name}")
        if row["source_commit"] not in ALLOWED_PINS or not re.fullmatch(r"[0-9a-f]{64}", row["sha256"]):
            raise ValueError(f"invalid source identity: {name}")
        if int(row["bytes"]) <= 0:
            raise ValueError(f"invalid source size: {name}")
        entries[name] = row
    slices = json.loads((root / "docs/import-slices.json").read_text())
    if slice_name not in slices:
        raise ValueError(f"unknown slice: {slice_name}")
    required = slices[slice_name]
    if not isinstance(required, list) or not all(isinstance(p, str) for p in required):
        raise ValueError("required set must be a list of paths")
    if len(required) != len(set(required)):
        raise ValueError("duplicate required entry")
    if enforce and not required:
        raise ValueError("empty required set cannot pass acceptance")
    if slice_name == "product" and not set(slices.get("timing", [])).issubset(required):
        raise ValueError("product regression must retain every accepted timing import")
    result["required"] = len(required)
    for name in required:
        if name not in entries:
            raise ValueError(f"unknown required entry: {name}")
        row = entries[name]
        source = subprocess.run(["git", "-C", str(reference), "show", f"{row['source_commit']}:{name}"],
                                capture_output=True, check=True).stdout
        if len(source) != int(row["bytes"]) or hashlib.sha256(source).hexdigest() != row["sha256"]:
            result["divergent"] += 1
            result["errors"].append(f"source identity mismatch: {name}")
            continue
        destination = safe_path(root / "src/k1", name)
        if not destination.is_file():
            result["missing"] += 1
            result["errors"].append(f"missing: {name}")
            continue
        result["checked"] += 1
        if destination.read_bytes() != source:
            result["divergent"] += 1
            result["errors"].append(f"destination differs from pinned source: {name}")
    result["pass"] = enforce and not result["errors"] and result["checked"] == result["required"]
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--reference", type=Path, default=REFERENCE)
    parser.add_argument("--slice", default="timing")
    parser.add_argument("--enforce", action="store_true")
    args = parser.parse_args()
    try:
        result = verify(args.root, args.reference, args.slice, args.enforce)
    except (ValueError, KeyError, OSError, TypeError, subprocess.CalledProcessError) as error:
        print(json.dumps(dict(slice=args.slice, required=0, checked=0, missing=0,
                              divergent=0, errors=[str(error)], **{"pass": False})))
        return 2
    print(json.dumps(result, indent=2))
    return 0 if result["pass"] or not args.enforce else 2


if __name__ == "__main__":
    raise SystemExit(main())
