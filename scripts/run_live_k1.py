#!/usr/bin/env python3
"""Bound Titan live-K1 runner. Does not programme. Physical claims stay separate."""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools/serial-studio"
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(ROOT / "scripts"))

import live_protocol as proto  # noqa: E402


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bind(build: Path, expected: dict | None = None) -> dict:
    receipt_path = build / "receipt.json"
    hex_path = build / "rtthread.hex"
    elf_path = build / "rtthread.elf"
    for path in (receipt_path, hex_path, elf_path):
        if not path.is_file():
            raise SystemExit(f"missing {path}")
    receipt = json.loads(receipt_path.read_text())
    actual = {
        "name": build.name,
        "build_id": receipt.get("build_id"),
        "hex_sha256": sha256(hex_path),
        "elf_sha256": sha256(elf_path),
        "receipt_sha256": sha256(receipt_path),
        "live_runtime": receipt.get("live_runtime"),
        "live_emit": receipt.get("live_emit"),
    }
    if expected:
        errors = [name for name in ("build_id", "hex_sha256", "elf_sha256", "receipt_sha256")
                  if expected.get(name) and actual.get(name) != expected.get(name)]
        if errors:
            raise SystemExit("bind failed: " + ", ".join(errors))
    return actual


def campaign(output: Path, duration_s: float) -> dict:
    import cdc_lock
    import live_lease
    from titan_broker import close_serial, find_titan, open_serial
    from titan_transport import CAMPAIGN_OPS, Transport

    usb = find_titan(inventory_lsof=False)
    handle = cdc_lock.acquire(usb["device"], "run-live-k1-campaign")
    port = None
    lease = live_lease.acquire(output, "run-live-k1-campaign")
    record = []
    try:
        port = open_serial(usb["device"], handle)
        transport = Transport(port, allowed={1}, campaign=True)
        info = transport.info(timeout=2.0)
        (output / "INFO.json").write_text(json.dumps(info["info"], indent=2) + "\n")
        transport.allow(set(CAMPAIGN_OPS))
        deadline = time.monotonic() + duration_s
        after_events = 0
        after_timing = 0
        while time.monotonic() < deadline:
            live_lease.renew(output, lease["token"])
            snap_got = transport.transact(23, b"")
            if snap_got["ok"]:
                snap = proto.unpack_snapshot(snap_got["body"])
                record.append({"t": time.time(), "snap": {
                    "generation": snap["publication_generation"],
                    "hop": snap["hop_sequence"],
                    "stale": snap["stale"],
                    "emit_on": snap["emit_on"],
                    "peak": snap["peak_scaled"],
                }})
                (output / "SNAPSHOT.json").write_text(json.dumps(snap, indent=2) + "\n")
            ev = transport.transact(24, proto.pack_cursor(after_events))
            if ev["ok"]:
                events = proto.unpack_history(ev["body"], proto.EVENT_BYTES, proto.unpack_event)
                after_events = events.get("newest_seq") or after_events
                (output / "EVENTS.json").write_text(json.dumps(events, indent=2) + "\n")
            tm = transport.transact(26, proto.pack_cursor(after_timing))
            if tm["ok"]:
                timing = proto.unpack_history(tm["body"], proto.TIMING_BYTES, proto.unpack_timing)
                after_timing = timing.get("newest_seq") or after_timing
                (output / "TIMING.json").write_text(json.dumps(timing, indent=2) + "\n")
            time.sleep(0.2)
        cfg = transport.transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))
        if cfg["ok"]:
            (output / "CONFIG.json").write_text(json.dumps(proto.unpack_config(cfg["body"]), indent=2) + "\n")
        return {
            "usb": usb["device"],
            "samples": len(record),
            "duration_s": duration_s,
            "controls_ok": False,
            "stale_ok": False,
            "observer_off_ok": False,
            "restore_ok": False,
            "deadline_misses": 0,
            "record": record[-20:],
        }
    finally:
        live_lease.release(output, lease["token"])
        close_serial(port, handle)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--programme", type=Path)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--duration", type=float, default=5.0)
    parser.add_argument("--expect-build-id")
    args = parser.parse_args()
    expected = {"build_id": args.expect_build_id} if args.expect_build_id else None
    bound = bind(args.build.resolve(), expected)
    if args.output.exists():
        raise SystemExit(f"refusing to reuse existing run directory {args.output}")
    args.output.mkdir(parents=True)
    (args.output / "BIND.json").write_text(json.dumps({
        "bound": bound,
        "programme": str(args.programme) if args.programme else None,
        "execute": bool(args.execute),
        "physical_admission": "unproven",
    }, indent=2) + "\n")
    if not args.execute:
        print("LIVE_K1_RUNNER_BOUND", bound["name"], "execute=false")
        return 0
    result = campaign(args.output, args.duration)
    (args.output / "CAMPAIGN.json").write_text(json.dumps(result, indent=2) + "\n")
    print("LIVE_K1_CAMPAIGN", result["samples"], "duration", args.duration)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
