#!/usr/bin/env python3
"""Lane P7: run a tempo tracker through the FROZEN synthetic gate.

  p7_run_gate.py verify GATE.json            regenerate every case, check PCM + truth hashes and tool hashes,
                                             build the probe; runs no tracker
  p7_run_gate.py run GATE.json OUT_DIR       verify, then probe every case (product tracker, 'baseline'),
                                             score with p7_gate.py, then freeze_tolerance.py verify the
                                             gate against the written results. Exit 0 only if all pass.

The gate must already be frozen by scripts/freeze_tolerance.py (refused otherwise). Probe input is the
generated 40 kHz s16 via k1_poly24, exactly as p7_tempo_probe.cpp documents. PCM is regenerated into the
job scratch (deterministic; hashes are frozen in the gate), never committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[5]
sys.path.insert(0, str(HERE))
import p7_gate  # noqa: E402
import p7_synth  # noqa: E402

PY = "/Users/spectrasynq/miniforge3/bin/python"
WORK = Path("/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p7/gate-run")
FREEZE = ROOT / "scripts/freeze_tolerance.py"


def verify(gate_path: Path) -> tuple[dict, Path]:
    gate = json.loads(gate_path.read_text())
    if "frozen_sha256" not in gate:
        raise SystemExit("REFUSED: gate not frozen")
    r = subprocess.run([PY, str(FREEZE), "verify", str(gate_path)], capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(f"REFUSED: freeze_tolerance verify failed: {r.stdout}")
    for role, rec in gate["tools"].items():
        if p7_gate.file_sha256(HERE / rec["file"]) != rec["sha256"]:
            raise SystemExit(f"REFUSED: {role} {rec['file']} changed after the freeze")
    pcm_dir = WORK / "pcm"
    truth_dir = WORK / "truth"
    pcm_dir.mkdir(parents=True, exist_ok=True)
    truth_dir.mkdir(parents=True, exist_ok=True)
    cases = {c["id"]: c for c in p7_synth.case_list()}
    if [c["id"] for c in gate["cases"]] != list(cases):
        raise SystemExit("REFUSED: generator case list differs from the frozen case list")
    for rec in gate["cases"]:
        x, truth = p7_synth.render(cases[rec["id"]])
        s16 = p7_synth.to_s16(x).tobytes()
        if hashlib.sha256(s16).hexdigest() != rec["pcm_sha256"]:
            raise SystemExit(f"REFUSED: PCM of {rec['id']} differs from the frozen hash")
        (pcm_dir / f"{rec['id']}.s16").write_bytes(s16)
        tp = truth_dir / f"{rec['id']}.truth.json"
        tp.write_text(json.dumps(truth) + "\n")
        if p7_gate.file_sha256(tp) != rec["truth_sha256"]:
            raise SystemExit(f"REFUSED: truth of {rec['id']} differs from the frozen hash")
    WORK.mkdir(parents=True, exist_ok=True)
    subprocess.run(["cc", "-std=c11", *gate["probe_build"]["flags"], "-c", "platform/ra8p1/k1_poly24.c", "-o",
                    str(WORK / "poly.o")], cwd=ROOT, check=True)
    exe = WORK / "p7_tempo_probe"
    subprocess.run(["c++", "-std=c++17", *gate["probe_build"]["flags"], str(HERE / gate["tools"]["probe"]["file"]),
                    *gate["probe_build"]["product_sources"], str(WORK / "poly.o"), "-o", str(exe)],
                   cwd=ROOT, check=True)
    return gate, exe


def run(gate_path: Path, out: Path) -> int:
    gate, exe = verify(gate_path)
    traces = out / "traces"
    traces.mkdir(parents=True, exist_ok=True)
    probe_log = {}
    for rec in gate["cases"]:
        r = subprocess.run([str(exe), str(WORK / "pcm" / f"{rec['id']}.s16"), str(p7_synth.FS),
                            str(traces / f"{rec['id']}.csv"), "baseline"], capture_output=True, text=True)
        probe_log[rec["id"]] = {"exit": r.returncode, "stdout": r.stdout.strip()}
        if r.returncode != 0:
            raise SystemExit(f"probe failed on {rec['id']}: {r.returncode} {r.stdout} {r.stderr}")
    (out / "probe-log.json").write_text(json.dumps(probe_log, indent=1) + "\n")
    rc = p7_gate.main(["score", str(gate_path), str(WORK / "truth"), str(traces), str(out / "gate-result.json")])
    fv = subprocess.run([PY, str(FREEZE), "verify", str(gate_path), str(out / "gate-result.json"),
                         str(out / "probe-log.json")], capture_output=True, text=True)
    (out / "freeze-verify.json").write_text(fv.stdout)
    if fv.returncode != 0:
        print(f"freeze verify failed: {fv.stdout}")
        return 2
    return rc


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    v = sub.add_parser("verify")
    v.add_argument("gate", type=Path)
    r = sub.add_parser("run")
    r.add_argument("gate", type=Path)
    r.add_argument("out", type=Path)
    a = ap.parse_args(argv)
    if a.cmd == "verify":
        verify(a.gate)
        print(json.dumps({"verified": True}))
        return 0
    return run(a.gate, a.out)


if __name__ == "__main__":
    sys.exit(main())
