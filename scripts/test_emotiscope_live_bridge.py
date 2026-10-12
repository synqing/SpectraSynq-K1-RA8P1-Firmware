#!/usr/bin/env python3
"""Exercise the native Emotiscope live bridge and three deliberate regressions."""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import subprocess
import resource

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--compiler", default="/usr/bin/clang++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    platform = root / "platform/ra8p1"
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    runtime = platform / "k1_live_runtime.cpp"
    test = root / "tests/host/test_emotiscope_live_bridge.cpp"
    source = runtime.read_text()
    observed = [runtime, platform/"k1_live_runtime.h", test, Path(__file__),
                platform/"emotiscope_port.cpp", platform/"emotiscope_port.h",
                platform/"emotiscope_quantize.inc",
                platform/"inhouse_audio.cpp", platform/"inhouse_audio.h",
                root/"src/k1/core/visual/inhouse_waveform.h",
                root/"src/k1/core/audio/gdft_goertzel.cpp", root/"src/k1/core/audio/gdft_goertzel.h",
                platform/"k1_live_clock.c", platform/"k1_live_clock.h",
                platform/"k1_live_protocol.cpp", platform/"k1_live_protocol.h",
                platform/"k1_live_schema.inc", platform/"emotiscope_edge_response.h",
                root/"src/k1/core/visual/product_palette.cpp",
                root/"src/k1/core/visual/product_palette_data.cpp"]
    observed += [platform/"pdm_target.h"]
    observed += [p for p in (platform/"emotiscope_donor").rglob("*") if p.is_file()]
    observed += [p for p in (platform/"emotiscope_shims").rglob("*") if p.is_file()]
    observed = sorted(set(observed))
    before = {str(p.relative_to(root)): sha(p) for p in observed}
    receipt = {"scope": "Host bridge boundaries with actual donor; not target timing or optical admission",
               "clock": "AP and VP share the current execution clock; no future render timestamp",
               "sources_sha256": before, "cases": []}
    compiler = subprocess.run([args.compiler, "--version"], capture_output=True, text=True)
    receipt["compiler"] = compiler.stdout
    passed = True
    variants = [
        ("normal", None, None),
        ("capture_snapshot", None, None),
        ("missing_sample_contract", "hop.samples != K1_EMOTISCOPE_CHUNK ||", "false ||"),
        ("legacy_deadline", "fields->deadline_us = 5000U", "fields->deadline_us = 7500U"),
        ("wrong_media_rate", "last_hop_.hop_sequence * 240U", "last_hop_.hop_sequence * 360U"),
    ]
    for name, old, new in variants:
        variant = output / (name + ".cpp")
        if old:
            if source.count(old) != 1:
                raise RuntimeError("Mutation does not bind exactly once: " + name)
            variant.write_text(source.replace(old, new))
        else:
            variant.write_text(source)
        binary = output / name
        depfile = output / (name + ".d")
        command = [
            args.compiler, "-std=c++17", "-O1", "-g", "-Wno-narrowing",
            "-ffp-contract=off", "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer", "-DK1_EMOTISCOPE_PORT",
            "-I"+str(root/"src/k1"), "-I"+str(root/"src"),
            "-I"+str(platform), "-I"+str(platform/"emotiscope_donor"),
            "-I"+str(platform/"emotiscope_shims"),
            str(platform/"emotiscope_port.cpp"), str(variant),
            str(platform/"inhouse_audio.cpp"), str(root/"src/k1/core/audio/gdft_goertzel.cpp"),
            str(root/"src/k1/core/visual/product_palette.cpp"),
            str(root/"src/k1/core/visual/product_palette_data.cpp"),
            str(platform/"k1_live_clock.c"), str(platform/"k1_live_protocol.cpp"),
            str(test), "-o", str(binary),
        ]
        if name == "capture_snapshot":
            command.insert(1, "-DK1_PDM_TARGET")
        build = subprocess.run(command, capture_output=True, text=True, timeout=120)
        (output/(name+"-build.log")).write_text(build.stdout + build.stderr)
        case = {"name": name, "build_command": command, "build_exit": build.returncode,
                "variant_sha256": sha(variant)}
        if build.returncode == 0:
            run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=120)
            (output/(name+"-run.log")).write_text(run.stdout + run.stderr)
            if old:
                observed_result = run.returncode != 0 and (
                    "Assertion failed" in run.stderr or "Assertion" in run.stderr and "failed" in run.stderr)
            else:
                observed_result = run.returncode == 0 and "EMOTISCOPE_LIVE_BRIDGE_PASS" in run.stdout
            case.update(exit=run.returncode, expected_result_observed=observed_result,
                        observation=(run.stdout + run.stderr).strip(),
                        binary_sha256=sha(binary))
        else:
            case.update(expected_result_observed=False, observation=build.stderr)
        passed &= case["expected_result_observed"]
        receipt["cases"].append(case)
    after = {str(p.relative_to(root)): sha(p) for p in observed}
    receipt["sources_unchanged_during_run"] = before == after
    passed &= receipt["sources_unchanged_during_run"]
    receipt["host_checks_satisfied"] = passed
    (output/"test_emotiscope_live_bridge.cpp").write_bytes(test.read_bytes())
    (output/"receipt.json").write_text(json.dumps(receipt, indent=2)+"\n")
    print(json.dumps({"host_checks_satisfied": passed, "receipt": str(output/"receipt.json"),
                      "cases": [{"name": c["name"], "exit": c.get("exit"),
                                 "expected_result_observed": c["expected_result_observed"]}
                                for c in receipt["cases"]]}, indent=2))
    return 0 if passed else 1

if __name__ == "__main__":
    raise SystemExit(main())
