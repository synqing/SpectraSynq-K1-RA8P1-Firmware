#!/usr/bin/env python3
"""Durable host ML admission and actual fixture-clock regression; no device I/O."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

# Verbatim broken callback at 776b376; this is a historical negative, not an oracle.
OLD_CALLBACK = """std::uint32_t donor_ml_clock_us(){
  return static_cast<std::uint32_t>(palette_clock.sample(k1_cycle_count(),palette_clock_hz));
}"""


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_ml_tests(root, output, compiler="/usr/bin/clang++"):
    started = time.perf_counter()
    output.mkdir(parents=True, exist_ok=False)
    platform = root / "platform/ra8p1"
    fixture = (platform / "fixture_app.cpp").read_text()
    match = re.search(r"std::uint32_t donor_ml_clock_us\(\)\s*\{.*?\n\}", fixture, re.S)
    if not match:
        raise RuntimeError("actual fixture ML callback not found")
    if not re.search(r"k1_emotiscope_set_ml_clocks\(\s*&k1_cycle_count\s*,\s*&donor_ml_clock_us\s*\)", fixture):
        raise RuntimeError("actual fixture does not bind the challenged ML callback")
    callback = match.group()
    paths = [platform / name for name in (
        "fixture_app.cpp", "emotiscope_ml.cpp", "emotiscope_ml.h",
        "emotiscope_port.cpp", "emotiscope_port.h", "musical_conditioning.h",
        "k1_live_runtime.cpp", "k1_live_runtime.h", "k1_live_clock.c",
        "k1_live_clock.h", "palette_clock.h")]
    paths += [root / "tests/host/test_emotiscope_ml.cpp",
              root / "tests/host/test_emotiscope_ml_clock.cpp",
              Path(__file__).resolve(), root / "scripts/test_emotiscope_controls.py",
              root / "src/k1/core/visual/product_palette.cpp",
              root / "src/k1/core/visual/product_palette_data.cpp"]
    paths += sorted((platform / "emotiscope_donor").rglob("*.h"))
    bindings = {str(path.relative_to(root)): sha(path) for path in paths}
    observations = {"scope": "host admission/actual callback regression; no device or physical ML claim",
                    "clock_hz": 1000000000, "synthetic_origin_difference_us": 20000,
                    "observed_device_origin_difference": "not measured",
                    "source_sha256": bindings, "commands": [], "variants": []}

    def execute(argv, name):
        argv = [str(value) for value in argv]
        command_started = time.perf_counter()
        completed = subprocess.run(argv, capture_output=True, text=True)
        (output / (name + ".log")).write_text(completed.stdout + completed.stderr)
        observations["commands"].append({"argv": argv, "exit_code": completed.returncode,
                                         "host_elapsed_seconds": time.perf_counter() - command_started,
                                         "log": name + ".log"})
        return completed

    def build(argv, name):
        completed = execute(argv, name)
        if completed.returncode:
            raise RuntimeError("compile failed: " + str(output / (name + ".log")))

    strict = [compiler, "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
              "-fno-fast-math", "-ffp-contract=off", "-fsanitize=address,undefined",
              "-ffunction-sections", "-fdata-sections", "-I" + str(platform),
              "-I" + str(root / "src"), "-I" + str(root / "src/k1")]
    try:
        focused = output / "focused-ml"
        build(strict + [root / "tests/host/test_emotiscope_ml.cpp",
                        platform / "emotiscope_ml.cpp", "-o", focused], "focused-build")
        focused_run = execute([focused], "focused-run")
        if focused_run.returncode:
            raise RuntimeError("focused ML assertions failed")
        clock_object = output / "live-clock.o"
        build(["/usr/bin/clang", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
               "-fsanitize=address,undefined", "-ffunction-sections", "-fdata-sections",
               "-c", platform / "k1_live_clock.c", "-o", clock_object], "clock-build")
        runtime_object = output / "live-runtime.o"
        build(strict + ["-DK1_EMOTISCOPE_PORT", "-DK1_RA8P1_TARGET", "-c",
                        platform / "k1_live_runtime.cpp", "-o", runtime_object], "runtime-build")
        # Link the real donor status provider used by LiveAudioRuntime::initialise.
        # Dead-strip unused AP/protocol functions; no fabricated clock implementation.
        donor_objects = []
        for name, source in (
                ("port", platform / "emotiscope_port.cpp"),
                ("palette", root / "src/k1/core/visual/product_palette.cpp"),
                ("palette-data", root / "src/k1/core/visual/product_palette_data.cpp")):
            obj = output / (name + ".o")
            build([compiler, "-std=c++17", "-O2", "-Wno-narrowing", "-fno-fast-math",
                   "-fsanitize=address,undefined", "-ffunction-sections", "-fdata-sections",
                   "-I" + str(platform), "-I" + str(root / "src/k1"),
                   "-I" + str(platform / "emotiscope_donor"),
                   "-I" + str(platform / "emotiscope_shims"),
                   "-c", source, "-o", obj], name + "-build")
            donor_objects.append(obj)
        linker = "-Wl,-dead_strip" if sys.platform == "darwin" else "-Wl,--gc-sections"
        for name, body, expected in (("current", callback, 0), ("old-origin", OLD_CALLBACK, 2)):
            variant = output / name
            variant.mkdir()
            (variant / "fixture_ml_clock_callback.inc").write_text(body + "\n")
            binary = variant / "clock-test"
            build(strict + ["-DK1_EMOTISCOPE_PORT", "-I" + str(variant),
                            root / "tests/host/test_emotiscope_ml_clock.cpp",
                            platform / "emotiscope_ml.cpp", clock_object, runtime_object,
                            *donor_objects, linker, "-o", binary], name + "-build")
            run = execute([binary], name + "-run")
            observations["variants"].append({"variant": name,
                "callback_sha256": hashlib.sha256(body.encode()).hexdigest(),
                "callback": body, "binary_sha256": sha(binary),
                "run_exit": run.returncode, "stdout": run.stdout, "stderr": run.stderr})
            if run.returncode != expected:
                raise RuntimeError(name + " unexpected exit: " + str(run.returncode))
            if name == "old-origin" and (
                    "FAIL result.valid actual shared-clock expectation" not in run.stderr
                    or "executions=1" not in run.stdout or "cycles=600000" not in run.stdout):
                raise RuntimeError("old callback did not fail after actual measured inference")
        after = {str(path.relative_to(root)): sha(path) for path in paths}
        observations["sources_unchanged"] = bindings == after
        if bindings != after:
            raise RuntimeError("challenged sources changed during test")
    finally:
        observations["host_elapsed_seconds"] = time.perf_counter() - started
        (output / "result.json").write_text(json.dumps(observations, indent=2) + "\n")
    return {"result": str(output / "result.json"), "focused_exit": focused_run.returncode,
            "current_callback_exit": observations["variants"][0]["run_exit"],
            "historical_callback_exit": observations["variants"][1]["run_exit"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--compiler", default="/usr/bin/clang++")
    args = parser.parse_args()
    result = run_ml_tests(Path(__file__).resolve().parents[1], args.output.resolve(), args.compiler)
    print(json.dumps(result))


if __name__ == "__main__":
    main()
