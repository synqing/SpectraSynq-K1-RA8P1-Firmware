#!/usr/bin/env python3
"""Drive the identified 160-pixel WS2816 stick from its centre to both edges."""
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

PROFILES = {"fastled": 4, "ws2816c": 3}
PIXELS_PER_LANE = 80
LANE_BYTES = PIXELS_PER_LANE * 6
P601 = 0
P004 = 1


def pack_grb48(red: int, green: int, blue: int) -> bytes:
    if any(not 0 <= channel <= 0xFFFF for channel in (red, green, blue)):
        raise ValueError("WS2816 channels must be 0..65535")
    return struct.pack(">HHH", green, red, blue)


def centre_out_lanes(radius: int, brightness: int) -> tuple[bytes, bytes]:
    """Return DIN-A/DIN-B wire buffers for logical centre pixels 79/80."""
    if not 0 <= radius < PIXELS_PER_LANE or not 1 <= brightness <= 0xFFFF:
        raise ValueError("radius must be 0..79 and brightness 1..65535")
    lane_a = bytearray(LANE_BYTES)
    lane_b = bytearray(LANE_BYTES)
    for tail in range(min(8, radius + 1)):
        level = brightness >> tail
        if not level:
            break
        a_index = PIXELS_PER_LANE - 1 - radius + tail
        b_index = radius - tail
        if 0 <= a_index < PIXELS_PER_LANE:
            lane_a[a_index * 6 : a_index * 6 + 6] = pack_grb48(level, 0, 0)
        if 0 <= b_index < PIXELS_PER_LANE:
            lane_b[b_index * 6 : b_index * 6 + 6] = pack_grb48(0, 0, level)
    return bytes(lane_a), bytes(lane_b)


def check_frame_reply(reply: dict, pin: int, wire: bytes, profile: int = PROFILES["fastled"]) -> None:
    expected = {
        "op": 14,
        "version": 1,
        "profile": profile,
        "pin": pin,
        "pixels": PIXELS_PER_LANE,
        "bytes": LANE_BYTES,
        "crc": zlib.crc32(wire),
        "result": 0,
        "pin_config_error": 0,
        "wire_timing_measured": False,
        "photons": "NOT_CLAIMED",
    }
    for key, value in expected.items():
        if reply.get(key) != value:
            raise RuntimeError(f"WS2816 frame readback mismatch {key}: {reply.get(key)} != {value}")
    # PDR=bit 2, ASEL=bit 15, PMR=bit 16 in the pinned RA8P1 CMSIS header.
    if reply.get("pfs_after", 0) & ((1 << 2) | (1 << 15) | (1 << 16)) != 1 << 2:
        raise RuntimeError("WS2816 pin readback is not GPIO output")
    if reply.get("emit_cycles", 0) <= 0 or reply.get("latch_cycles", 0) <= 0:
        raise RuntimeError("WS2816 emission markers missing")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cycles", type=int, default=2)
    parser.add_argument("--fps", type=float, default=20.0)
    parser.add_argument("--brightness", type=lambda value: int(value, 0), default=0x7A3C)
    parser.add_argument(
        "--profile",
        choices=PROFILES,
        default="fastled",
        help="fastled is the bring-up default; ws2816c retains the narrower C-1313 datasheet profile",
    )
    parser.add_argument("--leave-lit", action="store_true", help="leave the final edge pair latched instead of sending black")
    args = parser.parse_args()
    if not 1 <= args.cycles <= 20 or not 1 <= args.fps <= 60:
        parser.error("cycles must be 1..20 and FPS must be 1..60")
    centre_out_lanes(0, args.brightness)

    build = json.loads((args.build / "receipt.json").read_text())
    image_sha = hashlib.sha256((args.build / "rtthread.hex").read_bytes()).hexdigest()
    if build.get("pass") is not True or build.get("artifacts", {}).get("rtthread.hex") != image_sha:
        raise RuntimeError("build or image identity invalid; no serial opened")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "pass": False,
        "start": datetime.now(timezone.utc).isoformat(),
        "operation": "WS2816_CENTRE_OUT_DUAL_80",
        "execution_level": "ON_SILICON_COMMAND_PATH",
        "photons": "NOT_CLAIMED",
        "wire_timing_measured": False,
        "logical_pixels": 160,
        "pixels_per_lane": PIXELS_PER_LANE,
        "lane_a": {"pin": "P601", "logical_pixels": "1-80", "colour": "red"},
        "lane_b": {"pin": "P004", "logical_pixels": "81-160", "colour": "blue"},
        "centre_origin": "logical pixels 80/81 (zero-based 79/80)",
        "requested_fps": args.fps,
        "brightness_u16": args.brightness,
        "transport_profile": args.profile,
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
        print(f"IDENTIFIED_TITAN uid={info['uid']} build={info['build']}", flush=True)

        # A palette-capable image may otherwise overwrite P601 between frames.
        if build.get("palette_runtime"):
            stop_with_black = struct.pack("<8I", 1, 0, 1, 0, 0, 5, 0, 0)
            stop_disabled = struct.pack("<8I", 1, 0, 1, 0, 0, 0, 0, 0)
            transact(16, stop_with_black)
            time.sleep(0.05)
            receipt["palette_stop"] = transact(16, stop_disabled)
            if receipt["palette_stop"].get("active") or receipt["palette_stop"].get("emit_enabled"):
                raise RuntimeError("autonomous palette output did not stop")

        profile = PROFILES[args.profile]

        def emit(pin: int, wire: bytes) -> dict:
            payload = struct.pack("<4I", 1, profile, pin, PIXELS_PER_LANE) + wire
            reply = transact(14, payload)
            check_frame_reply(reply, pin, wire, profile)
            return reply

        black = bytes(LANE_BYTES)
        emit(P601, black)
        emit(P004, black)
        radii = list(range(PIXELS_PER_LANE)) + list(range(PIXELS_PER_LANE - 2, -1, -1))
        deadline = time.perf_counter()
        step = 0
        for cycle in range(args.cycles):
            for radius in radii:
                lane_a, lane_b = centre_out_lanes(radius, args.brightness)
                started = time.perf_counter()
                reply_a = emit(P601, lane_a)
                reply_b = emit(P004, lane_b)
                receipt["frames"].append({
                    "step": step,
                    "cycle": cycle,
                    "radius": radius,
                    "lane_a_crc": reply_a["crc"],
                    "lane_b_crc": reply_b["crc"],
                    "lane_a_emit_cycles": reply_a.get("emit_cycles"),
                    "lane_b_emit_cycles": reply_b.get("emit_cycles"),
                    "transaction_ms": (time.perf_counter() - started) * 1000.0,
                })
                step += 1
                deadline += 1.0 / args.fps
                remaining = deadline - time.perf_counter()
                if remaining > 0:
                    time.sleep(remaining)
        if args.leave_lit:
            final_a, final_b = centre_out_lanes(PIXELS_PER_LANE - 1, args.brightness)
            emit(P601, final_a)
            emit(P004, final_b)
            receipt["final_state"] = "edge pair latched"
        else:
            receipt["off"] = {"lane_a": emit(P601, black), "lane_b": emit(P004, black)}
            receipt["final_state"] = "both lanes black"
        receipt["completed_steps"] = step
        receipt["completed_wire_frames"] = step * 2 + 4
        receipt["pass"] = True
        print(
            f"WS2816_CENTRE_OUT_COMPLETED logical_pixels=160 steps={step} "
            f"wire_frames={receipt['completed_wire_frames']} build={info['build']}; photons not claimed",
            flush=True,
        )
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
