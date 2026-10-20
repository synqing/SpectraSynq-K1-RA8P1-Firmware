#!/usr/bin/env python3
"""Score a live-K1 development campaign. Green aggregate over a missing row is a fail."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROWS = [f"D{i:02d}" for i in range(18)] + [f"G{i:02d}" for i in range(7)]


def load(path: Path):
    return json.loads(path.read_text()) if path.is_file() else None


def score(run_dir: Path) -> dict:
    run_dir = Path(run_dir)
    candidate = load(run_dir / "CANDIDATE-FOR-TEST.json") or load(run_dir / "BIND.json")
    info = load(run_dir / "INFO.json")
    schema = load(run_dir / "SCHEMA.json")
    snapshot = load(run_dir / "SNAPSHOT.json")
    config = load(run_dir / "CONFIG.json")
    events = load(run_dir / "EVENTS.json")
    timing = load(run_dir / "TIMING.json")
    campaign = load(run_dir / "CAMPAIGN.json")
    replay = load(run_dir / "REPLAY.json")
    compare = load(run_dir / "COMPARE.json")
    fixture = load(run_dir / "FIXTURE.json")
    rows = {}

    def put(rid, status, reason, evidence):
        rows[rid] = {"id": rid, "status": status, "reason": reason, "evidence": evidence}

    if candidate and candidate.get("identity_bound") and candidate.get("build_id"):
        if candidate.get("accepted_checkpoint"):
            put("D00", "FAIL", "fabricated accepted checkpoint", str(run_dir))
        else:
            put("D00", "PASS", "candidate-for-test bound, not accepted", str(run_dir / "CANDIDATE-FOR-TEST.json"))
    else:
        put("D00", "FAIL", "missing candidate binding", str(run_dir))

    owner = load(run_dir / "CDC-OWNER.json")
    if owner and owner.get("owner"):
        put("D01", "PASS", "named CDC owner", str(run_dir / "CDC-OWNER.json"))
    else:
        put("D01", "FAIL", "no CDC owner record", str(run_dir))

    if snapshot and snapshot.get("publish_time_us") != snapshot.get("capture_time_us"):
        put("D02", "PASS", "publish and capture remain distinct", "SNAPSHOT.json")
    else:
        put("D02", "FAIL" if snapshot else "NOT_TESTED", "clocks not evidenced on this run", "SNAPSHOT.json")

    if snapshot and snapshot.get("hops_consumed", 0) > 0 and snapshot.get("hop_sequence", 0) > 0:
        put("D03", "PASS", "hop sequence and consume counters present", "SNAPSHOT.json")
    else:
        put("D03", "NOT_TESTED" if not snapshot else "FAIL", "no complete AP accounting", "SNAPSHOT.json")

    if snapshot and snapshot.get("live_origin") == 1:
        put("D04", "PASS", "live_origin set", "SNAPSHOT.json")
    else:
        put("D04", "NOT_TESTED" if not snapshot else "FAIL", "live origin missing", "SNAPSHOT.json")

    groups = None
    if snapshot:
        from importlib.util import spec_from_loader
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/serial-studio"))
        import live_protocol as proto
        groups = proto.groups_present(snapshot)
    if groups and all(groups.values()) and snapshot.get("spectrum") and max(abs(x) for x in snapshot["spectrum"]) > 0:
        put("D05", "PASS", "C1 groups present with non-zero spectrum", "SNAPSHOT.json")
    elif snapshot:
        put("D05", "FAIL", "MIR groups incomplete or zero-filled", str(groups))
    else:
        put("D05", "NOT_TESTED", "no snapshot", "")

    if config and config.get("mode_a") is not None and config.get("emit_on") == 0:
        put("D06", "PASS" if campaign else "NOT_TESTED", "config present; channel campaign may still be short", "CONFIG.json")
    else:
        put("D06", "NOT_TESTED", "no controlled-channel campaign", "CONFIG.json")

    if config and config.get("revision") is not None and config.get("emit_on") == 0:
        put("D07", "PASS" if campaign and campaign.get("controls_ok") else "NOT_TESTED",
            "config blob read; atomic mutation evidence in campaign", "CONFIG.json")
    else:
        put("D07", "NOT_TESTED", "no validated SET/readback", "")

    if events and events.get("count", 0) > 0 and not events.get("empty"):
        put("D08", "PASS", "event history returned records", "EVENTS.json")
    else:
        put("D08", "NOT_TESTED" if not events else "FAIL", "event history empty or missing", "EVENTS.json")

    if timing and timing.get("count", 0) > 0:
        put("D09", "PASS", "timing history returned records", "TIMING.json")
    else:
        put("D09", "NOT_TESTED" if not timing else "FAIL", "timing history empty or missing", "TIMING.json")

    if campaign and campaign.get("deadline_misses", 1) == 0:
        put("D10", "PASS", "zero deadline misses", "CAMPAIGN.json")
    else:
        put("D10", "NOT_TESTED", "no campaign service bounds", "CAMPAIGN.json")

    put("D11", "NOT_TESTED", "stack/heap reserves not scored from this host pack", "")

    if snapshot and "asrc_starved" in snapshot:
        put("D12", "PASS", "starved/gain counters present including zeros", "SNAPSHOT.json")
    else:
        put("D12", "NOT_TESTED", "no source-quality counters", "")

    if snapshot and snapshot.get("injection_start_us") and snapshot.get("stale"):
        put("D13", "PASS", "stale markers present", "SNAPSHOT.json")
    elif campaign and campaign.get("stale_ok"):
        put("D13", "PASS", "campaign recorded stale recovery", "CAMPAIGN.json")
    else:
        put("D13", "NOT_TESTED", "stale-source test not run", "")

    if campaign and campaign.get("observer_off_ok"):
        put("D14", "PASS", "observation independence recorded", "CAMPAIGN.json")
    else:
        put("D14", "NOT_TESTED", "observer off/on not run", "")

    if campaign and campaign.get("restore_ok") and (config or {}).get("emit_on") == 0:
        put("D15", "PASS", "restoration readback emit false", "CAMPAIGN.json")
    else:
        put("D15", "NOT_TESTED", "no restoration evidence", "")

    if candidate and candidate.get("dcache") == "disabled" and candidate.get("m33") == "parked":
        put("D16", "PASS", "declared runtime configuration", "CANDIDATE-FOR-TEST.json")
    else:
        put("D16", "FAIL", "runtime configuration not declared", "")

    put("D17", "PASS", "this scorer wrote row-by-row output", str(run_dir / "SCORE.json"))

    if fixture and fixture.get("sha256") and fixture.get("rate_hz") == 24000:
        put("G00", "PASS", "hashed 24 kHz fixture identity", "FIXTURE.json")
        put("G02", "PASS", "frozen PCM asset", "FIXTURE.json")
    else:
        put("G00", "NOT_TESTED", "no frozen fixture", "")
        put("G02", "NOT_TESTED", "no 24 kHz S16LE asset", "")

    if replay and replay.get("device_mutated") is False:
        put("G01", "PASS", "offline replay did not open the device", "REPLAY.json")
    else:
        put("G01", "NOT_TESTED", "no replay receipt", "")

    if replay and replay.get("repeatable"):
        put("G03", "PASS", "repeatable host replay", "REPLAY.json")
    else:
        put("G03", "NOT_TESTED", "replay not proven repeatable", "")

    if replay and replay.get("delay_case"):
        put("G04", "PASS", "delay/reset/rate cases recorded", "REPLAY.json")
    else:
        put("G04", "NOT_TESTED", "no delay/reset case", "")

    if compare and compare.get("negatives_caught"):
        put("G05", "PASS", "comparator failed mutated inputs", "COMPARE.json")
    else:
        put("G05", "NOT_TESTED", "no comparator negatives", "")

    if fixture and fixture.get("named_music") and fixture.get("commands"):
        put("G06", "PASS", "named music and developer commands", "FIXTURE.json")
    else:
        put("G06", "FAIL" if fixture and not fixture.get("named_music") else "NOT_TESTED",
            "named music or developer commands missing", "FIXTURE.json")

    required_d = [rows[r] for r in ROWS if r.startswith("D")]
    required_g = [rows[r] for r in ROWS if r.startswith("G")]
    d_pass = all(r["status"] == "PASS" for r in required_d)
    g_pass = all(r["status"] == "PASS" for r in required_g)
    if any(r["status"] == "FAIL" for r in required_d):
        d_pass = False
    if any(r["status"] in {"FAIL", "NOT_TESTED", "BLOCKED"} for r in required_d):
        d_pass = False
    if any(r["status"] in {"FAIL", "NOT_TESTED", "BLOCKED"} for r in required_g):
        g_pass = False
    return {
        "rows": rows,
        "TITAN_LIVE_K1_DEV_READY": d_pass,
        "MUSIC_INTELLIGENCE_BASELINE_READY": d_pass and g_pass,
        "scope": "emit_off_logical_ab",
        "physical_admission": "unproven",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-dir", required=True, type=Path)
    args = parser.parse_args()
    result = score(args.run_dir)
    (args.run_dir / "SCORE.json").write_text(json.dumps(result, indent=2) + "\n")
    print("DEV_READY" if result["TITAN_LIVE_K1_DEV_READY"] else "DEV_NOT_READY",
          "MIR_READY" if result["MUSIC_INTELLIGENCE_BASELINE_READY"] else "MIR_NOT_READY")
    if result["TITAN_LIVE_K1_DEV_READY"] and any(v["status"] != "PASS" for k, v in result["rows"].items() if k.startswith("D")):
        raise SystemExit("scorer contradiction")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
