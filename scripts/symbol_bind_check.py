#!/usr/bin/env python3
"""Offline symbol/source binding check for an existing ELF. Does not rebuild. Does not attach."""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REQUIRED_DWARF_FOR_SOURCE = (".debug_info", ".debug_line", ".debug_abbrev")
PROHIBITED_ATTACH = ("getAllRegisters", "target remote", "arm-none-eabi-gdb")


def readelf_sections(elf: Path) -> str:
    cmd = ["arm-none-eabi-readelf", "-S", str(elf)]
    run = subprocess.run(cmd, capture_output=True, text=True)
    if run.returncode != 0:
        raise RuntimeError(run.stderr or run.stdout or "readelf failed")
    return run.stdout


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--expected-build-id", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--attach-mode", default="none")
    args = parser.parse_args(argv)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    out: dict = {
        "task": "D3",
        "start": datetime.now(timezone.utc).isoformat(),
        "elf": str(args.elf),
        "expected_build_id": args.expected_build_id,
        "SYMBOL_BINDING": "BLOCKED",
    }
    try:
        mode = args.attach_mode.lower()
        for token in PROHIBITED_ATTACH:
            if token.lower() in mode:
                out["SYMBOL_BINDING"] = "FAIL"
                out["error"] = f"attach mode refused: {args.attach_mode}"
                return 2
        if not args.elf.is_file():
            out["SYMBOL_BINDING"] = "BLOCKED"
            out["error"] = "ELF missing"
            return 2
        blob = args.elf.read_bytes()
        build_id_present = args.expected_build_id.encode() in blob
        out["matched_executable_identity"] = bool(build_id_present)
        if not build_id_present:
            out["SYMBOL_BINDING"] = "FAIL"
            out["error"] = "ELF does not contain expected build_id; refuse symbol load"
            return 2
        sections = readelf_sections(args.elf)
        section_names = set()
        for line in sections.splitlines():
            # readelf -S rows look like: "  [29] .debug_info       PROGBITS ..."
            if "]" in line:
                after = line.split("]", 1)[1].strip()
                if after.startswith("."):
                    section_names.add(after.split()[0])
        out["sections_excerpt"] = [
            line.strip() for line in sections.splitlines() if "debug" in line.lower() or ".symtab" in line
        ]
        has_symtab = ".symtab" in section_names
        dwarf = {name: (name in section_names) for name in REQUIRED_DWARF_FOR_SOURCE}
        out["symtab"] = has_symtab
        out["dwarf"] = dwarf
        # Source-path binding requires DWARF info/line/abbrev, not merely .debug_frame.
        if all(dwarf.values()) and has_symtab:
            out["SYMBOL_BINDING"] = "PASS"
            out["note"] = "Executable identity matches and DWARF source sections are present"
            return 0
        out["SYMBOL_BINDING"] = "FAIL"
        out["error"] = (
            "Executable identity matches but DWARF source sections are incomplete; "
            "do not claim source-level binding. Do not substitute a different build_id ELF."
        )
        out["missing"] = [name for name, ok in dwarf.items() if not ok]
        return 1
    finally:
        out["end"] = datetime.now(timezone.utc).isoformat()
        args.output.write_text(json.dumps(out, indent=2) + "\n")
        print(json.dumps(out, indent=2))


if __name__ == "__main__":
    sys.exit(main())
