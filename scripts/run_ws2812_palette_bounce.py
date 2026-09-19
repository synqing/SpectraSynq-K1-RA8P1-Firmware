#!/usr/bin/env python3
"""Palette-coloured centre-out bounce on the identified WS2812 transmitter.

Same ping-pong radius as run_ws2812_centre_out.py. Colours come from the
frozen 44 K1 palettes (FastLED16), not the Titan 100-103 effect engine.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import subprocess
import time
import zlib
from datetime import datetime, timezone
from pathlib import Path

from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

PIXELS = 128
REPO = Path(__file__).resolve().parents[1]
PALETTE_DATA = REPO / "src/k1/core/visual/product_palette_data.cpp"


def load_fastled16(path: Path) -> list[list[tuple[int, int, int]]]:
    text = path.read_text()
    starts = [m.start() for m in re.finditer(r"constexpr Pixel8 kFastLed16_(\d+)\[\] = \{", text)]
    palettes: list[list[tuple[int, int, int]]] = []
    for start in starts:
        chunk = text[start:start + 800]
        colours = [
            (int(r), int(g), int(b))
            for r, g, b in re.findall(r"\{(\d+)U, (\d+)U, (\d+)U\}", chunk)
        ][:16]
        if len(colours) != 16:
            raise RuntimeError("FastLED16 block does not have 16 slots")
        palettes.append(colours)
    if len(palettes) != 44:
        raise RuntimeError(f"expected 44 palettes, got {len(palettes)}")
    return palettes


def scale8(value: int, scale: int) -> int:
    return (value * (scale + 1)) >> 8


def blend(first: int, second: int, fraction: int) -> int:
    second_weight = (fraction << 4) & 255
    first_weight = 255 - second_weight
    return scale8(first, first_weight) + scale8(second, second_weight)


def apply_brightness(value: int, brightness: int) -> int:
    if brightness == 255:
        return value
    if brightness == 0 or value == 0:
        return 0
    return scale8(value, brightness + 1)


def sample(palette: list[tuple[int, int, int]], index: int, brightness: int) -> tuple[int, int, int]:
    slot = (index >> 4) & 15
    fraction = index & 15
    colour = palette[slot]
    if fraction:
        nxt = palette[0 if slot == 15 else slot + 1]
        colour = (
            blend(colour[0], nxt[0], fraction),
            blend(colour[1], nxt[1], fraction),
            blend(colour[2], nxt[2], fraction),
        )
    return (
        apply_brightness(colour[0], brightness),
        apply_brightness(colour[1], brightness),
        apply_brightness(colour[2], brightness),
    )


def mix(a: tuple[int, int, int], b: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    t = 0.0 if t < 0 else 1.0 if t > 1 else t
    s = 1.0 - t
    return (
        int(a[0] * s + b[0] * t + 0.5),
        int(a[1] * s + b[1] * t + 0.5),
        int(a[2] * s + b[2] * t + 0.5),
    )


def palette_bounce_frame(
    palettes: list[list[tuple[int, int, int]]],
    palette_a: int,
    palette_b: int,
    mix_t: float,
    radius: int,
    brightness: int,
) -> bytes:
    out = bytearray(PIXELS * 3)
    half = PIXELS // 2
    for i in range(PIXELS):
        radial = (half - 1 - i) if i < half else (i - half)
        if radial > radius:
            continue
        index = int(radial * 255 / 63)
        colour = mix(
            sample(palettes[palette_a], index, brightness),
            sample(palettes[palette_b], index, brightness),
            mix_t,
        )
        out[i * 3] = colour[1]
        out[i * 3 + 1] = colour[0]
        out[i * 3 + 2] = colour[2]
    return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cycles", type=int, default=20)
    parser.add_argument("--fps", type=float, default=30.0)
    parser.add_argument("--brightness", type=int, default=96)
    parser.add_argument("--transition-s", type=float, default=1.5)
    args = parser.parse_args()
    if not 1 <= args.cycles <= 40 or not 1 <= args.fps <= 60 or not 1 <= args.brightness <= 255:
        parser.error("cycles 1..40, fps 1..60, brightness 1..255")
    palettes = load_fastled16(PALETTE_DATA)
    build = json.loads((args.build / "receipt.json").read_text())
    image = args.build / "rtthread.hex"
    if build.get("pass") is not True or hashlib.sha256(image.read_bytes()).hexdigest() != build["artifacts"]["rtthread.hex"]:
        raise RuntimeError("build identity invalid")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "pass": False,
        "start": datetime.now(timezone.utc).isoformat(),
        "operation": "WS2812_PALETTE_BOUNCE_128",
        "photons": "NOT_CLAIMED",
        "engine": "host opcode-14 FastLED16 palettes + centre-out bounce",
        "not": "centre_palette_engine modes 100-103",
    }
    port = None
    try:
        import serial
        from serial.tools import list_ports
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045B, 0x5310)]
        if len(matches) != 1:
            raise RuntimeError("expected one Titan application CDC")
        device = matches[0].device
        owners = subprocess.run(
            ["lsof", "-t", device, device.replace("/cu.", "/tty.")],
            capture_output=True,
            text=True,
        )
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise RuntimeError("CDC has another owner")
        port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
        request_id = 0

        def transact(op: int, payload: bytes = b""):
            nonlocal request_id
            request_id += 1
            outgoing = packet(op, request_id, payload=payload)
            if port.write(outgoing) != len(outgoing):
                raise RuntimeError("short CDC write")
            port.flush()
            header = read_exact(port, 32, 15)
            magic, status, received_id, sequence, size, cycles, crc, hcrc = struct.unpack("<4s7I", header)
            body = read_exact(port, size, 15)
            if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or received_id != request_id:
                raise RuntimeError("bad envelope")
            if zlib.crc32(body) != crc:
                raise RuntimeError("bad crc")
            if status:
                raise RuntimeError(f"opcode {op} rejected {status}")
            return json.loads(body)

        info = transact(1)
        if info.get("uid") != UID or info.get("source") != PIN or info.get("build") != build["build_id"]:
            raise RuntimeError("target identity mismatch")
        receipt["runtime"] = {"uid": info["uid"], "build": info["build"]}
        radii = list(range(64)) + list(range(62, -1, -1))
        started = time.monotonic()
        deadline = time.perf_counter()
        frames = 0
        for cycle in range(args.cycles):
            for radius in radii:
                elapsed = time.monotonic() - started
                span = max(args.transition_s, 0.05)
                step = int(elapsed / span)
                palette_a = step % 44
                palette_b = (palette_a + 1) % 44
                mix_t = (elapsed / span) - step
                mix_t = mix_t * mix_t * (3.0 - 2.0 * mix_t)
                wire = palette_bounce_frame(
                    palettes, palette_a, palette_b, mix_t, radius, args.brightness
                )
                payload = struct.pack("<4I", 1, 1, 0, PIXELS) + wire
                reply = transact(14, payload)
                if reply.get("result") != 0 or reply.get("pixels") != PIXELS:
                    raise RuntimeError(f"frame {frames} rejected: {reply}")
                frames += 1
                deadline += 1.0 / args.fps
                delay = deadline - time.perf_counter()
                if delay > 0:
                    time.sleep(delay)
        receipt["frames"] = frames
        receipt["pass"] = True
        print(json.dumps({"pass": True, "frames": frames, "build": info["build"]}, indent=2))
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
