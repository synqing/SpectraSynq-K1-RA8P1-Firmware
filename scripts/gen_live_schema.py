#!/usr/bin/env python3
"""Regenerate platform/ra8p1/k1_live_schema.inc from titan-live-v1.json."""
from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "docs/contracts/titan-live-v1.json"
DST = ROOT / "platform/ra8p1/k1_live_schema.inc"


def main() -> int:
    data = SRC.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    lines = [
        "/* generated from docs/contracts/titan-live-v1.json — do not edit by hand */",
        "#pragma once",
        f"static const unsigned char kTitanLiveSchemaJson[{len(data)}] = {{",
    ]
    row = []
    for i, byte in enumerate(data):
        row.append(f"{byte:3d}")
        if len(row) == 16 or i == len(data) - 1:
            comma = "," if i != len(data) - 1 else ""
            lines.append("    " + ", ".join(row) + comma)
            row = []
    lines.append("};")
    lines.append(f"static const unsigned kTitanLiveSchemaLen = {len(data)};")
    lines.append(f'static const char kTitanLiveSchemaSha256[] = "{digest}";')
    lines.append("")
    DST.write_text("\n".join(lines))
    print("SCHEMA_INC", digest, len(data))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
