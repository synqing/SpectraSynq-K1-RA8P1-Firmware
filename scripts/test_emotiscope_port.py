#!/usr/bin/env python3
"""Pinned Emotiscope donor AP comparison, sanitizers, and deliberate negative variants."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

PINNED = {
    "goertzel.h": "cfac207ae4ec63ffe90aa28fad30ece1879093cb04db99642a77f82f4078b15d",
    "vu.h": "9ff6a9420e3e814e48214824965fe17323c3d16f6f64f698ac548f66722ec28e",
    "tempo.h": "1d51915799c72ba050cd62f82f5411e08e7e04dd255be0081e531cb1a428e3ee",
}
OLD = "((NOVELTY_HISTORY_LENGTH - 1) - block_size) + i"
NEW = "(NOVELTY_HISTORY_LENGTH - block_size) + i"

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def execute(cmd, destination):
    p = subprocess.run([str(x) for x in cmd], capture_output=True, text=True)
    destination.write_text(p.stdout + p.stderr)
    return {"command": [str(x) for x in cmd], "exit": p.returncode,
            "log": str(destination), "stdout": p.stdout, "stderr": p.stderr}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", required=True, type=Path)
    ap.add_argument("--donor-root", type=Path,
        default=Path("/Users/spectrasynq/Workspace_Management/Software/Emotiscope.HIL/v1.1_build"))
    ap.add_argument("--pcm", action="append", default=[], type=Path,
        help="Explicit mono 12800 Hz signed little-endian int16 fixture; no playback occurs")
    ap.add_argument("--compiler", default="/usr/bin/clang++")
    args = ap.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    oracle = output/"oracle"
    oracle.mkdir()
    original_hashes = {}
    for name in ["types.h", "goertzel.h", "vu.h", "tempo.h"]:
        source = args.donor_root/name
        original_hashes[name] = sha(source)
        if name in PINNED and original_hashes[name] != PINNED[name]:
            raise RuntimeError("Donor source differs from audited identity: "+name)
        text = source.read_text()
        if name == "tempo.h":
            if text.count(OLD) != 2:
                raise RuntimeError("Unexpected donor tempo window expressions")
            text = text.replace(OLD, NEW)
        (oracle/("reference_"+name)).write_text(text)
    platform = root/"platform/ra8p1"
    observed_paths = [platform/"emotiscope_port.cpp", platform/"emotiscope_port.h",
                      platform/"emotiscope_quantize.inc",
                      root/"tests/host/test_emotiscope_port.cpp",
                      root/"tests/host/emotiscope_ap_reference.h"]
    observed_paths += sorted((platform/"emotiscope_donor").rglob("*.h"))
    observed_paths += sorted((platform/"emotiscope_shims").rglob("*"))
    observed_paths = [p for p in observed_paths if p.is_file()]
    sources_before = {str(p.relative_to(root)): sha(p) for p in observed_paths}
    results = {"donor": str(args.donor_root), "pristine_ap_sources": original_hashes,
               "comparison": "Original AP compiled independently; only explicit tempo array-bound repair shared",
               "input_contract": "normalized float from s16le / 32768; 64 samples per 5000 us",
               "render_contract": "100 Hz port schedule; first delta=0; AP reads last GPU-published clock",
               "source_sha256": sources_before, "fixtures": [], "cases": []}

    def build(platform_dir, name):
        binary = output/name
        command = [args.compiler, "-std=c++17", "-O1", "-g", "-Wno-narrowing",
                   "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                   "-I"+str(platform_dir), "-I"+str(platform_dir/"emotiscope_donor"),
                   "-I"+str(platform_dir/"emotiscope_shims"), "-I"+str(oracle),
                   "-I"+str(root/"tests/host"), str(root/"tests/host/test_emotiscope_port.cpp"),
                   str(platform_dir/"emotiscope_port.cpp"), "-o", str(binary)]
        result = execute(command, output/(name+"-build.log"))
        results["cases"].append({"name": name+"-build", **result})
        if result["exit"] != 0:
            raise RuntimeError("Build failed: "+str(output/(name+"-build.log")))
        return binary

    success = True
    try:
        binary = build(platform, "port-ap")
        fixture_args = [("synthetic", ["synthetic"])]
        for path in args.pcm:
            path = path.resolve()
            if "song-0" in str(path).lower():
                raise RuntimeError("Banned soundtrack is not an eligible fixture")
            results["fixtures"].append({"path": str(path), "sha256": sha(path),
                                         "format": "s16le", "rate_hz": 12800, "channels": 1,
                                         "bytes": path.stat().st_size})
            fixture_args.append((path.stem, ["pcm", str(path)]))
        for name, argv in fixture_args:
            result = execute([binary]+argv, output/(name+"-run.log"))
            case = {"name": name+"-AP-parity-and-sanitizers", **result}
            if result["exit"] == 0:
                case["observed"] = json.loads(result["stdout"])
            results["cases"].append(case)
            success &= result["exit"] == 0

        for mutation in ["restored-tempo-underflow", "omitted-goertzel"]:
            mutated = output/mutation
            mutated.mkdir()
            for name in ["emotiscope_port.cpp", "emotiscope_port.h", "emotiscope_quantize.inc"]:
                shutil.copy2(platform/name, mutated/name)
            shutil.copytree(platform/"emotiscope_donor", mutated/"emotiscope_donor")
            shutil.copytree(platform/"emotiscope_shims", mutated/"emotiscope_shims")
            if mutation == "restored-tempo-underflow":
                p = mutated/"emotiscope_donor/tempo.h"
                text = p.read_text()
                if text.count(NEW) != 2:
                    raise RuntimeError("Cannot locate exact bounds mutation")
                p.write_text(text.replace(NEW, OLD))
            else:
                p = mutated/"emotiscope_port.cpp"
                text = p.read_text()
                text, count = re.subn(r"(?:[A-Za-z_][A-Za-z_0-9]*::)?calculate_magnitudes\(\);",
                    "/* deliberately omitted donor Goertzel calculation */", text)
                if count != 1:
                    raise RuntimeError("Cannot isolate exactly one AP mutation")
                p.write_text(text)
            mutant_binary = build(mutated, mutation+"-binary")
            result = execute([mutant_binary, "synthetic"], output/(mutation+"-run.log"))
            marker = "AddressSanitizer" if mutation=="restored-tempo-underflow" else "AP_PARITY_MISMATCH"
            rejected = result["exit"] != 0 and marker in (result["stdout"]+result["stderr"])
            results["cases"].append({"name": mutation, "expected_rejection_observed": rejected, **result})
            success &= rejected
        sources_after = {str(p.relative_to(root)): sha(p) for p in observed_paths}
        results["sources_unchanged_during_test"] = sources_before == sources_after
        success &= results["sources_unchanged_during_test"]
    except Exception as error:
        results["error"] = str(error)
        success = False
    # Case observations and actual process results, not a platform or appearance admission.
    results["host_checks_satisfied"] = success
    for case in results["cases"]:
        case.pop("stderr", None)
        case.pop("stdout", None)
    (output/"result.json").write_text(json.dumps(results, indent=2)+"\n")
    print(json.dumps({"host_checks_satisfied": success, "result": str(output/"result.json"),
                      "cases": [{"name": c["name"], "exit": c["exit"]} for c in results["cases"]],
                      "error": results.get("error")}, indent=2))
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
