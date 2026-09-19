#!/usr/bin/env python3
"""Offline Titan host-snapshot stream for Serial Studio Process I/O.

Always simulated. Does not open CDC. Each scenario is held for --hold seconds
while sequence advances. --scenario NAME runs one case. --cycle walks the
catalogue. Silence emits nothing so the host watchdog can go stale.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

from titan_decode import cells_to_csv, decode

HERE = Path(__file__).resolve().parent
PAUSE_FLAG = HERE / "pause.flag"
FREEZE_FLAG = HERE / "freeze.flag"
SCENARIO_FLAG = HERE / "scenario.flag"

SCHEMA = 2
IDENTITY = {
    "uid": "545433931bd25436593630352d068363",
    "build": "77f77ecf2b449de754f469ac75f56b95d2b2d5a31684e8014824e9addaf7a118",
    "source": "6b1e7bc5c9f9871e6ea4e900455bcb37d756304a",
    "contract": "sr24000.hop180.bins80.xover40",
}

HEALTH = {
    "hop_max_us": 5600.0,
    "late_starts": 0,
    "deadlines": 0,
    "crc_mismatches": 0,
    "dma_irqs": 607,
    "latched_frames": 607,
    "led_faults": 0,
    "capture_rate_hz": 41405.5,
    "hop_samples": 180,
    "admitted_rate_hz": 24000,
}

MICS = {"mic_a_rms_dbfs": -28.0, "mic_b_rms_dbfs": -27.4}


def envelope(**extra) -> dict:
    row = {
        "schema": SCHEMA,
        "simulated": 1,
        "protocol": 1,
        "mode": 3,
        "identity_ok": 1,
        "health_valid": 1,
        "mic_a_valid": 1,
        "mic_b_valid": 1,
        "gate_result": 0,
        **IDENTITY,
        **HEALTH,
        **MICS,
    }
    row.update(extra)
    return row


SCENARIOS = {
    "identifying": envelope(
        mode=1,
        identity_ok=0,
        health_valid=0,
        mic_a_valid=0,
        mic_b_valid=0,
        uid="",
        build="",
        source="",
        contract="",
        **{k: None for k in list(HEALTH) + list(MICS)},
    ),
    "ready": envelope(mode=2, gate_result=0),
    "live": envelope(mode=3, gate_result=0),
    "testing": envelope(mode=4, gate_result=1, hop_max_us=7100.0),
    "pass": envelope(mode=5, gate_result=2),
    "fault": envelope(
        mode=6,
        gate_result=3,
        deadlines=1,
        late_starts=4,
        led_faults=1,
        hop_max_us=8900.0,
    ),
    "unsupported": envelope(mode=5, gate_result=4),
    "wrong_image": envelope(
        mode=3,
        uid="deadbeefdeadbeefdeadbeefdeadbeef",
        build="0000000000000000ffffffffffffffff",
        source="not-the-dualmcu-pin",
    ),
    "missing_measurement": envelope(mode=3, health_valid=1),
    "disconnected": envelope(
        mode=0,
        identity_ok=0,
        health_valid=0,
        mic_a_valid=0,
        mic_b_valid=0,
        **{k: None for k in list(HEALTH) + list(MICS)},
    ),
    "replay": envelope(mode=7, gate_result=3),
    "frozen_sequence": envelope(mode=3),
    "silent": None,
}

for name in ("identifying", "disconnected"):
    row = SCENARIOS[name]
    assert row is not None
    for key in list(HEALTH) + list(MICS):
        row.pop(key, None)
    if name == "identifying":
        for key in ("uid", "build", "source", "contract"):
            row.pop(key, None)

SCENARIOS["missing_measurement"].pop("hop_max_us", None)
SCENARIOS["missing_measurement"].pop("capture_rate_hz", None)

CYCLE = [
    "identifying",
    "ready",
    "live",
    "testing",
    "pass",
    "fault",
    "unsupported",
    "wrong_image",
    "missing_measurement",
    "frozen_sequence",
    "silent",
    "replay",
    "disconnected",
]


def requested_scenario(default: str) -> str:
    if SCENARIO_FLAG.exists():
        name = SCENARIO_FLAG.read_text().strip()
        if name in SCENARIOS:
            return name
    return default


def emit(row: dict, sequence: int, last_seq: str, same_seq: int) -> tuple[str, int]:
    payload = dict(row)
    payload["sequence"] = str(sequence)
    cells, last_seq, same_seq = decode(payload, last_seq, same_seq)
    if cells is None:
        return last_seq, same_seq
    line = cells_to_csv(cells)
    if not line:
        return last_seq, same_seq
    sys.stdout.write(line + "\n")
    sys.stdout.flush()
    return last_seq, same_seq


def run_scenario(name: str, sequence: int, hold: float, period: float, last_seq: str, same_seq: int) -> tuple[int, str, int]:
    deadline = None if hold <= 0 else time.monotonic() + hold
    frozen_seq = sequence

    def keep_going() -> bool:
        return deadline is None or time.monotonic() < deadline

    while keep_going():
        if PAUSE_FLAG.exists():
            time.sleep(period)
            continue
        current = requested_scenario(name)
        if current == "silent" or SCENARIOS.get(current) is None:
            time.sleep(period)
            continue
        row = SCENARIOS[current]
        freeze = FREEZE_FLAG.exists() or current == "frozen_sequence"
        if freeze:
            last_seq, same_seq = emit(row, frozen_seq or sequence, last_seq, same_seq)
        else:
            sequence += 1
            frozen_seq = sequence
            last_seq, same_seq = emit(row, sequence, last_seq, same_seq)
        time.sleep(period)
    return sequence, last_seq, same_seq


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cycle", action="store_true")
    parser.add_argument("--once", action="store_true")
    parser.add_argument("--scenario", choices=sorted(k for k in SCENARIOS))
    parser.add_argument("--hold", type=float, default=4.0)
    parser.add_argument("--period", type=float, default=0.2)
    args = parser.parse_args()
    sequence = 0
    last_seq = ""
    same_seq = 0
    names = [args.scenario] if args.scenario else CYCLE
    while True:
        for name in names:
            sequence, last_seq, same_seq = run_scenario(
                name, sequence, args.hold, args.period, last_seq, same_seq
            )
        if args.once or args.scenario or not args.cycle:
            return 0


if __name__ == "__main__":
    raise SystemExit(main())
