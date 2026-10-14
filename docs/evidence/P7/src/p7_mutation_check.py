#!/usr/bin/env python3
"""Lane P7: show the host tests can fail. Each mutant edits one rule in a scratch copy of src/ and runs
test_p7_gate.py there; a mutant must make pytest exit non-zero ("killed"). Writes JSON to argv[1].
"""
from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).parent
PY = "/Users/spectrasynq/miniforge3/bin/python"
SCRATCH = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p7/mutants")
MUTANTS = {
    "flip_detection_disabled": ("p7_gate.py", 'RELATIVES = {"x1/2": 0.5,', 'RELATIVES = {} and {"x1/2": 0.5,'),
    "acquire_grace_8_beats": ("p7_gate.py", 'GRACE_BEATS = {"acquire": 4.0', 'GRACE_BEATS = {"acquire": 8.0'),
    "step_grace_4_beats": ("p7_gate.py", '"step": 2.0', '"step": 4.0'),
    "release_grace_20_beats": ("p7_gate.py", '"release": 2.0', '"release": 20.0'),
    "assert_confidence_0.6": ("p7_gate.py", "ASSERT_CONFIDENCE = 0.30", "ASSERT_CONFIDENCE = 0.60"),
    "tolerance_widened_10pct": ("p7_gate.py", "correct = np.abs(bpm - tr) <= tol * tr",
                                "correct = np.abs(bpm - tr) <= 1.1 * tol * tr"),
    "lowconf_ignored": ("p7_gate.py", "            if bad.any():\n                lowconf_ok = False",
                        "            if False:\n                lowconf_ok = False"),
    "missing_trace_passes": ("p7_gate.py", 'results.append({"id": case["id"], "pass": False,',
                             'results.append({"id": case["id"], "pass": True,'),
    "truth_and_audio_shift_10ms": ("p7_synth.py", "return [start + k * p for k", "return [0.010 + start + k * p for k"),
    "audio_only_shift_2ms": ("p7_synth.py", "    i = int(round(at_s * FS))", "    i = int(round((at_s + 0.002) * FS))"),
}


def main(out: Path) -> int:
    shutil.rmtree(SCRATCH, ignore_errors=True)
    res = {}
    for name, (f, old, new) in MUTANTS.items():
        d = SCRATCH / name
        shutil.copytree(HERE, d, ignore=shutil.ignore_patterns("__pycache__", ".pytest_cache"))
        s = (d / f).read_text()
        assert s.count(old) == 1, name
        (d / f).write_text(s.replace(old, new))
        r = subprocess.run([PY, "-m", "pytest", "-q", "-p", "no:warnings", "-p", "no:cacheprovider", "test_p7_gate.py"],
                           cwd=d, capture_output=True, text=True)
        res[name] = {"pytest_exit": r.returncode, "killed": r.returncode != 0,
                     "summary": r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr[-200:]}
    r0 = subprocess.run([PY, "-m", "pytest", "-q", "-p", "no:warnings", "-p", "no:cacheprovider", "test_p7_gate.py"],
                        cwd=HERE, capture_output=True, text=True)
    doc = {"unmutated": {"pytest_exit": r0.returncode, "summary": r0.stdout.strip().splitlines()[-1]},
           "mutants": res, "all_killed": all(v["killed"] for v in res.values())}
    out.write_text(json.dumps(doc, indent=1) + "\n")
    print(json.dumps({"unmutated_exit": r0.returncode, "killed": sum(v["killed"] for v in res.values()),
                      "mutants": len(res)}))
    return 0 if doc["all_killed"] and r0.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main(Path(sys.argv[1])))
