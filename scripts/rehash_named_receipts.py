#!/usr/bin/env python3
"""SHA-256 named receipt paths. Prints ABSENT; never guesses abbreviated hashes."""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description="Hash declared receipt paths or print ABSENT.")
    parser.add_argument("paths", nargs="*", help="Files to hash")
    parser.add_argument("--json-list", type=Path, help="JSON array of path strings")
    parser.add_argument("--help-only", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    declared = list(args.paths)
    if args.json_list:
        declared.extend(json.loads(args.json_list.read_text()))
    if not declared:
        parser.print_help()
        return 0
    rows = []
    for item in declared:
        path = Path(item)
        if not path.is_file():
            rows.append({"path": str(path), "sha256": "ABSENT"})
            print(f"{path}\tABSENT")
            continue
        digest = sha256_file(path)
        rows.append({"path": str(path), "sha256": digest})
        print(f"{path}\t{digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
