#!/usr/bin/env python3
"""Host-only Emotiscope VP checks and three deliberately broken builds.
Does not access serial, launch music playback, or program a device.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
FILES = [
    "platform/ra8p1/emotiscope_port.h", "platform/ra8p1/emotiscope_port.cpp",
    "platform/ra8p1/emotiscope_quantize.inc",
]
for folder in ["platform/ra8p1/emotiscope_donor", "platform/ra8p1/emotiscope_shims"]:
    FILES += [str(p.relative_to(ROOT)) for p in sorted((ROOT/folder).rglob("*")) if p.is_file()]
TESTS = [
    "test_emotiscope_visual_port.cpp",
    "test_emotiscope_visual_oracle.cpp",
    "test_emotiscope_visual_gap.cpp",
    "test_emotiscope_clock_wrap.cpp",
]
FILES += ["tests/host/"+name for name in TESTS]
FILES += ["tests/host/test_emotiscope_visual_port.py"]
def hashes():
    return {rel: hashlib.sha256((ROOT/rel).read_bytes()).hexdigest() for rel in FILES}
def run_case(root, test, output):
    output.mkdir(parents=True)
    exe = output/"test"
    cmd = ["c++", "-std=c++17", "-O1", "-g", "-fsanitize=address,undefined",
           "-fno-omit-frame-pointer", "-Wno-narrowing", "-Wno-vla-cxx-extension",
           "-Iplatform/ra8p1/emotiscope_donor", "-Iplatform/ra8p1/emotiscope_shims",
           "platform/ra8p1/emotiscope_port.cpp", "tests/host/"+test, "-o", str(exe)]
    c = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    (output/"compile.log").write_text(c.stdout+c.stderr)
    result = {"command": cmd, "compile_exit": c.returncode}
    if c.returncode == 0:
        r = subprocess.run([str(exe)], cwd=root, capture_output=True, text=True)
        (output/"run.log").write_text(r.stdout+r.stderr)
        result.update(exit=r.returncode, stdout=r.stdout, stderr=r.stderr)
    return result
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    before = hashes()
    results = {"source_sha256": before, "positive": {}, "negative": {}}
    ok = True
    for name in TESTS:
        result = run_case(ROOT, name, output/name.removesuffix(".cpp"))
        results["positive"][name] = result
        ok &= result.get("compile_exit") == 0 and result.get("exit") == 0
    mutations = [
      ("gamma-removed", "platform/ra8p1/emotiscope_donor/gpu_core.h",
       "\t\tapply_gamma_correction();", "\t\t/* deliberately omitted gamma */",
       "test_emotiscope_visual_oracle.cpp", "diff<2e-6f"),
      ("dither-bypassed", "platform/ra8p1/emotiscope_port.cpp",
       "quantize_color_error(configuration.temporal_dithering);",
       "quantize_color_error(false);",
       "test_emotiscope_visual_oracle.cpp", "byte_errors==0"),
      ("history-wiped", "platform/ra8p1/emotiscope_port.cpp",
       "++audio_gaps;fresh_hops=0;gap_rewarming=true;",
       "++audio_gaps;fresh_hops=0;gap_rewarming=true; std::memset(leds_last,0,sizeof(leds_last));",
       "test_emotiscope_visual_gap.cpp", "std::fabs"),
      ("unsigned-deadline-restored", "platform/ra8p1/emotiscope_donor/tempo.h",
       "if (uint32_t(t_now_us - next_update) < 0x80000000u) {",
       "if (t_now_us >= next_update) {",
       "test_emotiscope_clock_wrap.cpp", "deadline_mismatches==0"),
      ("millis-wrap-restored", "platform/ra8p1/emotiscope_port.cpp",
       "uint32_t millis(){return static_cast<uint32_t>(port_extended_us/1000u);}",
       "uint32_t millis(){return port_clock_us/1000u;}",
       "test_emotiscope_clock_wrap.cpp", "millis_mismatches==0"),
    ]
    for name, rel, old, new, test, expected in mutations:
        mutant = output/("source-"+name)
        for src in FILES:
            target = mutant/src
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT/src, target)
        q = mutant/rel
        s = q.read_text()
        assert s.count(old) == 1, (rel, old, s.count(old))
        q.write_text(s.replace(old, new))
        r = run_case(mutant, test, output/name)
        rejected = (r.get("compile_exit") == 0 and r.get("exit") not in (None, 0)
                    and expected in r.get("stderr", ""))
        r["expected_assertion"] = expected
        r["rejected_by_named_assertion"] = rejected
        results["negative"][name] = r
        ok &= rejected
    results["source_unchanged"] = before == hashes()
    ok &= results["source_unchanged"]
    results["passed"] = bool(ok)
    (output/"result.json").write_text(json.dumps(results, indent=2)+"\n")
    print(json.dumps({"passed": bool(ok), "source_unchanged": results["source_unchanged"],
          "positives": {k: v.get("exit") for k,v in results["positive"].items()},
          "negatives": {k: v.get("rejected_by_named_assertion") for k,v in results["negative"].items()},
          "receipt": str(output/"result.json")}, indent=2))
    return 0 if ok else 1
if __name__ == "__main__":
    sys.exit(main())
