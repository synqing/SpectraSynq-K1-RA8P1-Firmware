#!/usr/bin/env python3
"""Exclusive live-K1 campaign client. Emit stays off. Does not programme."""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "scripts"))

import cdc_lock  # noqa: E402
import live_lease  # noqa: E402
import live_protocol as proto  # noqa: E402
from titan_broker import close_serial, find_titan, open_serial  # noqa: E402
from titan_transport import CAMPAIGN_OPS, Transport  # noqa: E402

UID = "545433931bd25436593630352d068363"


def attach(run_dir: Path) -> tuple:
    usb = find_titan(inventory_lsof=False)
    handle = cdc_lock.acquire(usb["device"], "titan-live-campaign")
    port = open_serial(usb["device"], handle)
    transport = Transport(port, allowed={1}, campaign=True)
    info = transport.info(timeout=2.0)
    if info["info"].get("uid") != UID:
        close_serial(port, handle)
        raise SystemExit("uid mismatch")
    transport.allow(set(CAMPAIGN_OPS))
    lease = live_lease.acquire(run_dir, "titan-live-campaign")
    return usb, handle, port, transport, info, lease


def dump(path: Path, name: str, payload) -> None:
    path.mkdir(parents=True, exist_ok=True)
    if isinstance(payload, (bytes, bytearray)):
        (path / f"{name}.bin").write_bytes(payload)
        return
    (path / f"{name}.json").write_text(json.dumps(payload, indent=2) + "\n")


def cmd_schema(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        chunks = []
        offset = 0
        total = 1
        while offset < total:
            got = transport.transact(25, proto.pack_config_sub(proto.CFG_GET_SCHEMA, __import__("struct").pack("<II", offset, 4000)))
            if not got["ok"]:
                raise SystemExit(f"GET_SCHEMA status {got['status']}")
            page = proto.unpack_schema_page(got["body"])
            total = page["total"]
            chunks.append(page["json"])
            offset += page["take"]
            if page["take"] == 0:
                break
        text = b"".join(chunks).decode()
        dump(run_dir, "SCHEMA", json.loads(text))
        dump(run_dir, "INFO", info["info"])
        print("SCHEMA_OK", proto.SCHEMA_SHA256)
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def cmd_snapshot(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        got = transport.transact(23, b"")
        if not got["ok"]:
            raise SystemExit(f"snapshot status {got['status']}")
        snap = proto.unpack_snapshot(got["body"])
        dump(run_dir, "SNAPSHOT", snap)
        (run_dir / "SNAPSHOT.bin").write_bytes(got["body"])
        print("SNAPSHOT_OK gen", snap["publication_generation"], "stale", snap["stale"], "emit", snap["emit_on"])
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def cmd_config_get(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        got = transport.transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))
        if not got["ok"]:
            raise SystemExit(f"GET_CONFIG status {got['status']}")
        cfg = proto.unpack_config(got["body"])
        dump(run_dir, "CONFIG", cfg)
        print("CONFIG_OK emit", cfg["emit_on"], "mode", cfg["mode_a"], "rev", cfg["revision"])
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def _stage_set(transport, blob: bytes) -> dict:
    begin = transport.transact(25, proto.pack_config_sub(proto.CFG_BEGIN_SET, __import__("struct").pack("<I", len(blob))))
    if not begin["ok"]:
        return begin
    offset = 0
    while offset < len(blob):
        take = min(256, len(blob) - offset)
        got = transport.transact(
            25,
            proto.pack_config_sub(proto.CFG_APPEND_SET, __import__("struct").pack("<I", offset) + blob[offset:offset + take]),
        )
        if not got["ok"]:
            return got
        offset += take
    return transport.transact(25, proto.pack_config_sub(proto.CFG_COMMIT_SET))


def cmd_config_set(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        current = proto.unpack_config(transport.transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))["body"])
        desired = dict(current)
        if args.mode_a is not None:
            desired["mode_a"] = args.mode_a
        if args.mode_b is not None:
            desired["mode_b"] = args.mode_b
        if args.palette_a is not None:
            desired["palette_a"] = args.palette_a
        if args.palette_b is not None:
            desired["palette_b"] = args.palette_b
        desired["emit_on"] = 0
        desired["flags"] = int(desired.get("flags") or 1) & ~4
        blob = proto.pack_config(desired)
        got = _stage_set(transport, blob)
        if not got["ok"]:
            raise SystemExit(f"SET_CONFIG status {got['status']}")
        cfg = proto.unpack_config(got["body"])
        dump(run_dir, "CONFIG", cfg)
        print("SET_OK rev", cfg["revision"], "mode_a", cfg["mode_a"], "mode_b", cfg["mode_b"], "emit", cfg["emit_on"])
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def cmd_events(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        got = transport.transact(24, proto.pack_cursor(args.after))
        if not got["ok"]:
            raise SystemExit(f"events status {got['status']}")
        events = proto.unpack_history(got["body"], proto.EVENT_BYTES, proto.unpack_event)
        dump(run_dir, "EVENTS", events)
        print("EVENTS_OK count", events.get("count"), "newest", events.get("newest_seq"))
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def cmd_timing(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        got = transport.transact(26, proto.pack_cursor(args.after))
        if not got["ok"]:
            raise SystemExit(f"timing status {got['status']}")
        timing = proto.unpack_history(got["body"], proto.TIMING_BYTES, proto.unpack_timing)
        dump(run_dir, "TIMING", timing)
        print("TIMING_OK count", timing.get("count"), "newest", timing.get("newest_seq"))
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def cmd_test_stale(args) -> int:
    run_dir = Path(args.run_dir)
    usb, handle, port, transport, info, lease = attach(run_dir)
    try:
        uid = info["info"]["uid"]
        build = info["info"]["build"]
        got = transport.transact(25, proto.pack_stale_test(uid, build))
        if not got["ok"]:
            raise SystemExit(f"TEST_STALE status {got['status']}")
        dump(run_dir, "STALE", {"ok": True, "body": got["body"].decode(errors="replace")})
        print("STALE_OK")
        return 0
    finally:
        live_lease.release(run_dir, lease["token"])
        close_serial(port, handle)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="cmd", required=True)
    for name in ("schema", "snapshot", "config-get", "test-stale"):
        p = sub.add_parser(name)
        p.add_argument("--run-dir", required=True)
    setter = sub.add_parser("config-set")
    setter.add_argument("--run-dir", required=True)
    setter.add_argument("--mode-a", type=int)
    setter.add_argument("--mode-b", type=int)
    setter.add_argument("--palette-a", type=int)
    setter.add_argument("--palette-b", type=int)
    for name in ("events", "timing"):
        p = sub.add_parser(name)
        p.add_argument("--run-dir", required=True)
        p.add_argument("--after", type=int, default=0)
    args = parser.parse_args()
    if args.cmd == "schema":
        return cmd_schema(args)
    if args.cmd == "snapshot":
        return cmd_snapshot(args)
    if args.cmd == "config-get":
        return cmd_config_get(args)
    if args.cmd == "config-set":
        return cmd_config_set(args)
    if args.cmd == "events":
        return cmd_events(args)
    if args.cmd == "timing":
        return cmd_timing(args)
    if args.cmd == "test-stale":
        return cmd_test_stale(args)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
