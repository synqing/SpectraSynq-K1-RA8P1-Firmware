#!/usr/bin/env python3
"""Identified Titan single-pin LED test. Leaves the requested bounded colour latched."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib

from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

PROFILES = {"ws2812": 1, "ws2812-v5": 2, "ws2816c": 3}
PINS = {"P601": 0, "P004": 1}


def request_and_wire(profile, pin, pixels, lit_pixels, red, green, blue):
    if profile not in PROFILES or pin not in PINS:
        raise ValueError("unsupported profile or pin")
    if not 1 <= pixels <= 128 or not 0 <= lit_pixels <= pixels:
        raise ValueError("diagnostic requires 1..128 pixels and 0..pixels lit pixels")
    maximum = 65535 if profile == "ws2816c" else 255
    if any(not 0 <= channel <= maximum for channel in (red, green, blue)):
        raise ValueError(f"channels must be 0..{maximum}; implicit truncation is forbidden")
    width = 2 if profile == "ws2816c" else 1
    pixel = b"".join(channel.to_bytes(width, "big") for channel in (green, red, blue))
    wire = pixel * lit_pixels + bytes((pixels - lit_pixels) * 3 * width)
    request = struct.pack("<8I", 1, PROFILES[profile], PINS[pin], pixels, lit_pixels,
                          red, green, blue)
    return request, wire


def check_led_reply(reply, profile, pin, pixels, lit_pixels, wire, clock_hz):
    expected = {"op": 13, "version": 1, "profile": PROFILES[profile], "pin": PINS[pin],
                "pixels": pixels, "lit_pixels": lit_pixels, "bytes": len(wire),
                "crc": zlib.crc32(wire), "result": 0, "pin_config_error": 0}
    for key, value in expected.items():
        if reply.get(key) != value:
            raise RuntimeError(f"LED readback mismatch {key}: {reply.get(key)} != {value}")
    # PDR=bit 2, ASEL=bit 15, PMR=bit 16 in the pinned RA8P1 CMSIS header.
    if reply.get("pfs_after", 0) & ((1 << 2) | (1 << 15) | (1 << 16)) != 1 << 2:
        raise RuntimeError("pin readback is not GPIO output")
    if reply.get("emit_cycles", 0) <= 0 or reply.get("bit_period_min_cycles", 0) <= 0:
        raise RuntimeError("missing software emission markers")
    if reply.get("bit_period_max_cycles", 0) < reply["bit_period_min_cycles"]:
        raise RuntimeError("invalid software period range")
    if reply.get("latch_cycles", 0) * 1_000_000 < 300 * clock_hz:
        raise RuntimeError("reset-low software interval shorter than 300 us")
    if reply.get("wire_timing_measured") is not False or reply.get("photons") != "NOT_CLAIMED":
        raise RuntimeError("diagnostic overclaims its observation level")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--identify-only", action="store_true", help="INFO only; no pin configuration or LED command")
    parser.add_argument("--profile", choices=PROFILES)
    parser.add_argument("--pin", choices=PINS)
    parser.add_argument("--pixels", type=int)
    parser.add_argument("--lit-pixels", type=int)
    parser.add_argument("--red", type=int)
    parser.add_argument("--green", type=int, default=0)
    parser.add_argument("--blue", type=int, default=0)
    parser.add_argument("--off", action="store_true")
    args = parser.parse_args()
    request = wire = None
    build = None
    image_sha = None
    if not args.identify_only:
        if not args.build or not args.profile or not args.pin or args.pixels is None:
            parser.error("emission requires --build, --profile, --pin and --pixels")
        lit = 0 if args.off else (args.pixels if args.lit_pixels is None else args.lit_pixels)
        red = args.red if args.red is not None else (0x7A3C if args.profile == "ws2816c" else 128)
        try:
            request, wire = request_and_wire(args.profile, args.pin, args.pixels, lit,
                                             red, args.green, args.blue)
        except ValueError as error:
            parser.error(str(error))
        build = json.loads((args.build / "receipt.json").read_text())
        image_sha = hashlib.sha256((args.build / "rtthread.hex").read_bytes()).hexdigest()
        if build.get("pass") is not True or build.get("artifacts", {}).get("rtthread.hex") != image_sha:
            raise RuntimeError("build or image identity invalid; no serial opened")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {"pass": False, "start": datetime.now(timezone.utc).isoformat(),
               "operation": "INFO_ONLY" if args.identify_only else "WS281X_DIAGNOSTIC",
               "photons": "NOT_CLAIMED", "wire_timing_measured": False}
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
        rid = 0

        def transact(op, payload=b""):
            nonlocal rid
            rid += 1
            outgoing = packet(op, rid, payload=payload)
            if port.write(outgoing) != len(outgoing):
                raise RuntimeError("short CDC write")
            port.flush()
            header = read_exact(port, 32, 15)
            magic, status, received_id, sequence, size, cycles, crc, hcrc = struct.unpack("<4s7I", header)
            if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or received_id != rid or size > 19968:
                raise RuntimeError("response header/identity invalid")
            body = read_exact(port, size, 15)
            if zlib.crc32(body) != crc:
                raise RuntimeError("response payload CRC invalid")
            if status:
                raise RuntimeError(f"device rejected opcode {op}: status={status} body={body!r}")
            return json.loads(body)

        info = transact(1)
        receipt["runtime"] = info
        if info.get("uid") != UID or info.get("source") != PIN or info.get("protocol") != 1:
            raise RuntimeError("wrong UID/source/protocol; no LED command sent")
        if not info.get("cpp_initialised") or info.get("clock_hz", 0) <= 0:
            raise RuntimeError("runtime initialisation/clock invalid")
        print(f"IDENTIFIED_TITAN uid={info['uid']} build={info['build']}", flush=True)
        if not args.identify_only:
            if info.get("build") != build["build_id"]:
                raise RuntimeError("runtime build mismatch; no LED command sent")
            receipt.update(image_sha256=image_sha, request_hex=request.hex(),
                           expected_wire_hex=wire.hex(), pin=args.pin, profile=args.profile)
            result = transact(13, request)
            receipt["led"] = result
            check_led_reply(result, args.profile, args.pin, args.pixels, lit, wire, info["clock_hz"])
            receipt["latched_state"] = "off" if not lit or not (red or args.green or args.blue) else "bounded_colour"
            print(f"LED_COMMAND_COMPLETED pin={args.pin} profile={args.profile} pixels={args.pixels} lit={lit}; colour remains latched; photons not claimed", flush=True)
        receipt["pass"] = True
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
