#!/usr/bin/env python3
"""Scorer must fail a missing hop and refuse a green aggregate."""
from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / "scripts"))
import score_live_k1  # noqa: E402


def write(path: Path, name: str, payload: dict) -> None:
    (path / name).write_text(json.dumps(payload) + "\n")


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-score-") as temp:
        run = Path(temp)
        write(run, "CANDIDATE-FOR-TEST.json", {
            "identity_bound": True,
            "build_id": "x",
            "accepted_checkpoint": False,
            "dcache": "disabled",
            "m33": "parked",
        })
        write(run, "CDC-OWNER.json", {"owner": "test"})
        write(run, "SNAPSHOT.json", {
            "publish_time_us": 10,
            "capture_time_us": 10,
            "hops_consumed": 0,
            "hop_sequence": 0,
            "live_origin": 1,
            "spectrum": [0.0] * 80,
            "chroma": [0.0] * 12,
            "asrc_starved": 0,
        })
        result = score_live_k1.score(run)
        if result["TITAN_LIVE_K1_DEV_READY"]:
            raise SystemExit("missing hop must not be DEV_READY")
        if result["rows"]["D02"]["status"] != "FAIL":
            raise SystemExit("identical publish/capture must fail D02")
        if result["rows"]["D03"]["status"] != "FAIL":
            raise SystemExit("zero hops must fail D03")
        mutated = dict(result)
        mutated["TITAN_LIVE_K1_DEV_READY"] = True
        if mutated["TITAN_LIVE_K1_DEV_READY"] and any(
            v["status"] != "PASS" for k, v in mutated["rows"].items() if k.startswith("D")
        ):
            print("LIVE_SCORE_HOST_PASS contradiction_detected")
            return 0
        raise SystemExit("scorer failed to keep missing rows visible")


if __name__ == "__main__":
    raise SystemExit(main())
