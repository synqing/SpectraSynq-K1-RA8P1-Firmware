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

from verify_imports import ROOT, REFERENCE, PIN, verify, commit_for


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, timeout=180, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{command}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--slice", choices=["timing", "product"], default="timing")
    parser.add_argument("--profile", choices=["strict_scalar", "pinned_native"], default="strict_scalar")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit("refusing to overwrite an existing receipt")
    imported = verify(ROOT, REFERENCE, args.slice, True)
    if not imported["pass"]:
        raise SystemExit(json.dumps(imported))
    names = json.loads((ROOT / "docs/import-slices.json").read_text())[args.slice]
    tests = ["test/test_musical_time/test_main.cpp", "test/test_musical_render_scheduler/test_main.cpp"]
    if args.slice == "product":
        tests += [f"test/{name}/test_main.cpp" for name in ["test_gdft_goertzel_parity", "test_gdft_postprocess_rate", "test_onset_v2_parity", "test_tempo_acf_parity", "test_musical_saliency_parity"]]
    receipt = dict(label="HOST", source_commit=PIN, candidate_commit=run(["git", "rev-parse", "HEAD"], cwd=ROOT).strip(),
                   compiler=run(["c++", "--version"]).splitlines()[0], profile=args.profile, imports=imported, tests=[])
    with tempfile.TemporaryDirectory(prefix="k1-host-") as temp:
        directory = Path(temp)
        donor = directory / "donor"
        test_root = directory / "tests"
        fixtures = run(["git", "-C", str(REFERENCE), "ls-tree", "-r", "--name-only", PIN,
                        "test/fixtures/musical_time"]).splitlines()
        if args.slice == "product":
            fixtures += ["test/fixtures/gdft_production_shell.golden.jsonl", "test/fixtures/tempo_v2_flywheel.golden.jsonl"]
        receipt["fixtures"] = {}
        for name in names + tests + fixtures:
            data = subprocess.check_output(["git", "-C", str(REFERENCE), "show", f"{commit_for(name)}:{name}"])
            destination = (test_root if name in tests + fixtures else donor) / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            if name in fixtures:
                receipt["fixtures"][name] = hashlib.sha256(data).hexdigest()
        for name in tests:
            outputs = {}
            executions = {}
            flags = ["-O2", "-ffp-contract=off", "-fno-fast-math"]
            if args.profile == "pinned_native":
                flags = ["-O0", "-ffast-math", "-fno-finite-math-only"]
                if "/test_tempo_acf_parity/" in name:
                    flags.append("-fno-unsafe-math-optimizations")
            for label, source_root in [("reference", donor), ("candidate", ROOT / "src/k1")]:
                executable = directory / (Path(name).parent.name + "-" + label)
                sources = [str(source_root / p) for p in names if p.endswith(".cpp")]
                command = ["c++", "-std=gnu++17", *flags, "-I" + str(source_root),
                           str(test_root / name), *sources, "-o", str(executable)]
                run(command)
                result = subprocess.run([str(executable)], cwd=test_root, capture_output=True, text=True, timeout=180)
                outputs[label] = result.stdout
                executions[label] = dict(exit=result.returncode, stderr=result.stderr, command=command)
            receipt["tests"].append(dict(name=name, fixture_sha256=hashlib.sha256((test_root / name).read_bytes()).hexdigest(),
                                         **{"pass": outputs["reference"] == outputs["candidate"] and all(row['exit'] == 0 for row in executions.values())}, flags=flags, outputs=outputs, executions=executions))
            print(outputs["candidate"], end="")
    receipt["pass"] = all(row['pass'] for row in receipt['tests'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"K1_PINNED_HOST={'PASS' if receipt['pass'] else 'FAIL'} slice={args.slice} profile={args.profile} receipt={args.output}")
    if not receipt['pass']: raise SystemExit(2)


if __name__ == "__main__":
    main()
