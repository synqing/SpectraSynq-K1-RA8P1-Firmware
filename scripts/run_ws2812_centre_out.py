#!/usr/bin/env python3
"""Run an identified 128-pixel centre-out WS2812 motion diagnostic on Titan."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib

from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN


def centre_out_frame(pixels: int, radius: int, brightness: int) -> bytes:
    if pixels < 2 or pixels % 2 or not 0 <= radius < pixels // 2 or not 1 <= brightness <= 255:
        raise ValueError("centre-out requires an even pixel count, valid radius and brightness 1..255")
    values = bytearray(pixels * 3)
    left = pixels // 2 - 1 - radius
    right = pixels // 2 + radius
    for tail in range(min(6, radius + 1)):
        level = brightness >> tail
        if not level:
            break
        for index in (left + tail, right - tail):
            if 0 <= index < pixels:
                values[index * 3 + 1] = max(values[index * 3 + 1], level)  # GRB red
    return bytes(values)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--pixels", type=int, default=128)
    parser.add_argument("--cycles", type=int, default=3)
    parser.add_argument("--fps", type=float, default=30.0)
    parser.add_argument("--brightness", type=int, default=96)
    args = parser.parse_args()
    if args.pixels != 128 or not 1 <= args.cycles <= 20 or not 1 <= args.fps <= 60:
        parser.error("this identified diagnostic requires 128 pixels, 1..20 cycles and 1..60 FPS")
    centre_out_frame(args.pixels, 0, args.brightness)

    build = json.loads((args.build / "receipt.json").read_text())
    image_sha = hashlib.sha256((args.build / "rtthread.hex").read_bytes()).hexdigest()
    if build.get("pass") is not True or build.get("artifacts", {}).get("rtthread.hex") != image_sha:
        raise RuntimeError("build or image identity invalid; no serial opened")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "pass": False,
        "start": datetime.now(timezone.utc).isoformat(),
        "operation": "WS2812_CENTRE_OUT_128",
        "photons": "NOT_CLAIMED",
        "wire_timing_measured": False,
        "requested_fps": args.fps,
        "frames": [],
    }
    port = None
    try:
        import serial
        from serial.tools import list_ports
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045B, 0x5310)]
        if len(matches) != 1:
            raise RuntimeError("expected exactly one Titan application CDC")
        device = matches[0].device
        alias = device.replace("/cu.", "/tty.")
        owners = subprocess.run(["lsof", "-t", device, alias], capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise RuntimeError("Titan CDC has another owner; no multiplexing")
        port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
        receipt["usb"] = {"path": device, "location": matches[0].location}
        request_id = 0

        def transact(op: int, payload: bytes = b"") -> dict:
            nonlocal request_id
            request_id += 1
            outgoing = packet(op, request_id, payload=payload)
            if port.write(outgoing) != len(outgoing):
                raise RuntimeError("short CDC write")
            port.flush()
            header = read_exact(port, 32, 15)
            magic, status, received_id, sequence, size, cycles, crc, hcrc = struct.unpack("<4s7I", header)
            if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or received_id != request_id or size > 19968:
                raise RuntimeError("response header or identity invalid")
            body = read_exact(port, size, 15)
            if zlib.crc32(body) != crc:
                raise RuntimeError("response payload CRC invalid")
            if status:
                raise RuntimeError(f"device rejected opcode {op}: status={status} body={body!r}")
            return json.loads(body)

        info = transact(1)
        receipt["runtime"] = info
        if info.get("uid") != UID or info.get("source") != PIN or info.get("protocol") != 1:
            raise RuntimeError("wrong UID, source or protocol; no LED frames sent")
        if info.get("build") != build.get("build_id"):
            raise RuntimeError("runtime build mismatch; no LED frames sent")
        receipt["image_sha256"] = image_sha
        radii = list(range(64)) + list(range(62, -1, -1))
        deadline = time.perf_counter()
        frame_number = 0
        for cycle in range(args.cycles):
            for radius in radii:
                wire = centre_out_frame(args.pixels, radius, args.brightness)
                payload = struct.pack("<4I", 1, 1, 0, args.pixels) + wire
                started = time.perf_counter()
                reply = transact(14, payload)
                expected = {"op": 14, "profile": 1, "pin": 0, "pixels": args.pixels,
                            "bytes": len(wire), "crc": zlib.crc32(wire), "result": 0,
                            "pin_config_error": 0}
                for key, value in expected.items():
                    if reply.get(key) != value:
                        raise RuntimeError(f"frame {frame_number} readback mismatch {key}")
                if reply.get("pfs_after", 0) & ((1 << 2) | (1 << 15) | (1 << 16)) != 1 << 2:
                    raise RuntimeError(f"frame {frame_number} pin readback is not GPIO output")
                receipt["frames"].append({
                    "frame": frame_number,
                    "cycle": cycle,
                    "radius": radius,
                    "crc": reply["crc"],
                    "emit_cycles": reply.get("emit_cycles"),
                    "transaction_ms": (time.perf_counter() - started) * 1000.0,
                })
                frame_number += 1
                deadline += 1.0 / args.fps
                remaining = deadline - time.perf_counter()
                if remaining > 0:
                    time.sleep(remaining)
        receipt["completed_frames"] = frame_number
        receipt["final_state"] = "last centre-out frame remains latched"
        receipt["pass"] = True
        print(f"WS2812_CENTRE_OUT_COMPLETED pixels={args.pixels} frames={frame_number} build={info['build']}")
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        if port is not None:
            port.close()
        receipt["end"] = datetime.now(timezone.utc).isoformat()
        (args.output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
