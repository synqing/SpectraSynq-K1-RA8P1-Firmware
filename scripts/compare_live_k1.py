#!/usr/bin/env python3
"""Same-input comparator with required negatives."""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path


def compare(a: dict, b: dict, label: str) -> list[str]:
    findings = []
    for key in ("hops", "generation", "peak", "tempo_bpm", "valid"):
        if a.get(key) != b.get(key):
            findings.append(f"{label}:{key}")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    replay = json.loads(args.replay.read_text())
    runs = replay.get("runs") or []
    if len(runs) < 2:
        raise SystemExit("need two replay runs")
    positives = compare(runs[0], runs[1], "repeat")
    mutated = copy.deepcopy(runs[1])
    mutated["peak"] = float(mutated.get("peak") or 0) + 1.0
    missing = copy.deepcopy(runs[1])
    missing["hops"] = int(missing.get("hops") or 0) + 8
    wrong = {"hops": 0, "generation": 0, "peak": 0, "tempo_bpm": 0, "valid": 0}
    negatives = {
        "value_mutation": compare(runs[0], mutated, "mutation"),
        "missing_hop": compare(runs[0], missing, "missing"),
        "wrong_input": compare(runs[0], wrong, "wrong"),
    }
    caught = all(len(v) > 0 for v in negatives.values())
    payload = {
        "positives_clean": positives == [],
        "negatives": negatives,
        "negatives_caught": caught,
    }
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "COMPARE.json").write_text(json.dumps(payload, indent=2) + "\n")
    print("COMPARE_OK" if caught and not positives else "COMPARE_FAIL", "caught", caught)
    return 0 if caught and positives == [] else 1


if __name__ == "__main__":
    raise SystemExit(main())
