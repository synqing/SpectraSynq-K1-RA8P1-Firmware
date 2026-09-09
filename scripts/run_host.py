#!/usr/bin/env python3
"""Run pinned donor assertions against isolated donor and candidate builds."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_imports import ROOT, REFERENCE, PIN, verify


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, timeout=180, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{command}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--slice", choices=["timing"], default="timing")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("refusing to overwrite an existing receipt")
    imported = verify(ROOT, REFERENCE, args.slice, True)
    if not imported["pass"]:
        raise SystemExit(json.dumps(imported))
    names = json.loads((ROOT / "docs/import-slices.json").read_text())[args.slice]
    tests = ["test/test_musical_time/test_main.cpp", "test/test_musical_render_scheduler/test_main.cpp"]
    receipt = dict(label="HOST", source_commit=PIN, candidate_commit=run(["git", "rev-parse", "HEAD"], cwd=ROOT).strip(),
                   compiler=run(["c++", "--version"]).splitlines()[0], imports=imported, tests=[])
    with tempfile.TemporaryDirectory(prefix="k1-host-") as temp:
        directory = Path(temp)
        donor = directory / "donor"
        test_root = directory / "tests"
        fixtures = run(["git", "-C", str(REFERENCE), "ls-tree", "-r", "--name-only", PIN,
                        "test/fixtures/musical_time"]).splitlines()
        receipt["fixtures"] = {}
        for name in names + tests + fixtures:
            data = subprocess.check_output(["git", "-C", str(REFERENCE), "show", f"{PIN}:{name}"])
            destination = (test_root if name in tests + fixtures else donor) / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            if name in fixtures:
                receipt["fixtures"][name] = hashlib.sha256(data).hexdigest()
        for name in tests:
            outputs = {}
            for label, source_root in [("reference", donor), ("candidate", ROOT / "src/k1")]:
                executable = directory / (Path(name).parent.name + "-" + label)
                sources = [str(source_root / p) for p in names if p.endswith(".cpp")]
                command = ["c++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-I" + str(source_root),
                           str(test_root / name), *sources, "-o", str(executable)]
                run(command)
                outputs[label] = run([str(executable)], cwd=test_root)
            if outputs["reference"] != outputs["candidate"]:
                raise RuntimeError(f"differential mismatch in {name}: {outputs}")
            receipt["tests"].append(dict(name=name, fixture_sha256=hashlib.sha256((test_root / name).read_bytes()).hexdigest(),
                                         **{"pass": True}, outputs=outputs))
            print(outputs["candidate"], end="")
    receipt["pass"] = True
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"K1_SHARED_TIME_HOST=PASS receipt={args.output}")


if __name__ == "__main__":
    main()
