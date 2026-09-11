#!/usr/bin/env python3
"""Interactive, identified Titan WS2812 console with K1-style hotkeys."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import select
import struct
import subprocess
import sys
import termios
import time
import tty
import zlib

from run_led_smoke import UID, packet, read_exact
from run_ws2812_centre_out import centre_out_frame
from verify_imports import PIN

BUILD_DEFAULT = Path("/Users/spectrasynq/Workspace_Management/Edts/Titan/k1-ra8p1-002/ws281x-build-05")
OUTPUT_ROOT = Path("/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002")
COLOURS = {
    "red": (1, 0, 0),
    "green": (0, 1, 0),
    "blue": (0, 0, 1),
    "white": (1, 1, 1),
}


@dataclass
class ConsoleState:
    pattern: str = "centre"
    colour: str = "red"
    brightness: int = 96
    fps: int = 30
    paused: bool = False
    radius: int = 0
    direction: int = 1


def solid_frame(pixels: int, colour: str, brightness: int) -> bytes:
    red, green, blue = (channel * brightness for channel in COLOURS[colour])
    return bytes((green, red, blue)) * pixels


def frame_for_state(state: ConsoleState, pixels: int = 128) -> bytes:
    if state.pattern == "off":
        return bytes(pixels * 3)
    if state.pattern == "solid":
        return solid_frame(pixels, state.colour, state.brightness)
    if state.pattern == "centre":
        return centre_out_frame(pixels, state.radius, state.brightness)
    raise ValueError(f"unknown pattern: {state.pattern}")


def advance(state: ConsoleState, pixels: int = 128) -> None:
    if state.pattern != "centre" or state.paused:
        return
    state.radius += state.direction
    edge = pixels // 2 - 1
    if state.radius >= edge:
        state.radius = edge
        state.direction = -1
    elif state.radius <= 0:
        state.radius = 0
        state.direction = 1


def apply_typed(command: str, state: ConsoleState) -> str:
    fields = command.strip().split()
    if not fields:
        return "no command"
    if fields == ["help"]:
        return "help"
    if fields == ["status"]:
        return "status"
    if fields == ["off"]:
        state.pattern = "off"
        return "off"
    if fields == ["centre"]:
        state.pattern = "centre"
        state.paused = False
        return "centre"
    if fields == ["quit"]:
        return "quit"
    if len(fields) == 2 and fields[0] == "colour" and fields[1] in COLOURS:
        state.colour = fields[1]
        if state.pattern == "off":
            state.pattern = "solid"
        return f"colour={state.colour}"
    if len(fields) == 2 and fields[0] == "brightness":
        value = int(fields[1])
        if not 1 <= value <= 128:
            raise ValueError("brightness must be 1..128")
        state.brightness = value
        return f"brightness={value}"
    if len(fields) == 2 and fields[0] == "fps":
        value = int(fields[1])
        if not 1 <= value <= 60:
            raise ValueError("fps must be 1..60")
        state.fps = value
        return f"fps={value}"
    if len(fields) == 2 and fields[0] == "solid" and fields[1] in COLOURS:
        state.pattern = "solid"
        state.colour = fields[1]
        return f"solid={state.colour}"
    raise ValueError("unknown command; type :help")


def apply_hotkey(key: str, state: ConsoleState) -> str:
    if key in "rgbw":
        state.pattern = "solid"
        state.colour = {"r": "red", "g": "green", "b": "blue", "w": "white"}[key]
        return f"solid={state.colour}"
    if key == "c":
        state.pattern = "centre"
        state.paused = False
        return "centre"
    if key in ("0", " "):
        state.pattern = "off"
        return "off"
    if key == "p":
        state.paused = not state.paused
        return f"paused={state.paused}"
    if key == "]":
        state.brightness = min(128, state.brightness + 8)
        return f"brightness={state.brightness}"
    if key == "[":
        state.brightness = max(1, state.brightness - 8)
        return f"brightness={state.brightness}"
    if key == ";":
        return "status"
    if key in ("h", "?"):
        return "help"
    if key in ("q", "\x03"):
        return "quit"
    return "ignored"


def help_text() -> str:
    return (
        "\nTitan LED keys (immediate):\n"
        "  c  centre-out motion    r/g/b/w  solid colour\n"
        "  [/] brightness -/+      p        pause/resume\n"
        "  0/Space off             ;        status\n"
        "  :  typed command mode   h/?      help\n"
        "  q/Ctrl-C quit (sends off first)\n"
        "Typed commands: :centre, :solid red|green|blue|white, :off,\n"
        "  :brightness 1..128, :fps 1..60, :status, :help, :quit\n"
    )


def state_text(state: ConsoleState) -> str:
    return (f"pattern={state.pattern} colour={state.colour} brightness={state.brightness} "
            f"fps={state.fps} paused={state.paused} radius={state.radius}")


class TitanClient:
    def __init__(self, build: dict):
        self.build = build
        self.port = None
        self.request_id = 0

    def open(self, wait_seconds: float) -> dict:
        import serial
        from serial.tools import list_ports
        deadline = time.monotonic() + wait_seconds
        print("WAITING_FOR_IDENTIFIED_TITAN_CDC: normal RESET is safe; do not hold USER/BOOT")
        while True:
            ports = list(list_ports.comports())
            matches = [p for p in ports if (p.vid, p.pid) == (0x045B, 0x5310)]
            if len(matches) > 1:
                seen = [(p.device, f"{p.vid or 0:04x}:{p.pid or 0:04x}") for p in ports]
                raise RuntimeError(f"expected exactly one Titan application CDC 045b:5310; seen={seen}")
            if len(matches) == 1:
                device = matches[0].device
                alias = device.replace("/cu.", "/tty.")
                owners = subprocess.run(["lsof", "-t", device, alias], capture_output=True, text=True)
                if owners.returncode not in (0, 1) or owners.stdout.strip():
                    raise RuntimeError("Titan CDC has another owner; close its serial monitor")
                try:
                    self.port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
                    break
                except serial.SerialException:
                    self.port = None
                    # Reset can expose a short-lived application node before the
                    # final CDC enumeration. Keep listening until the deadline.
            if time.monotonic() >= deadline:
                seen = [(p.device, f"{p.vid or 0:04x}:{p.pid or 0:04x}") for p in ports]
                raise RuntimeError(f"Titan application CDC 045b:5310 did not settle; seen={seen}")
            time.sleep(0.25)
        info = self.transact(1)
        if info.get("uid") != UID or info.get("source") != PIN or info.get("protocol") != 1:
            raise RuntimeError("wrong Titan UID, source or protocol")
        if info.get("build") != self.build.get("build_id"):
            raise RuntimeError(f"runtime build mismatch: {info.get('build')} != {self.build.get('build_id')}")
        return info

    def transact(self, op: int, payload: bytes = b"") -> dict:
        if self.port is None:
            raise RuntimeError("Titan client is not open")
        self.request_id += 1
        outgoing = packet(op, self.request_id, payload=payload)
        if self.port.write(outgoing) != len(outgoing):
            raise RuntimeError("short CDC write")
        self.port.flush()
        header = read_exact(self.port, 32, 15)
        magic, status, received_id, sequence, size, cycles, crc, hcrc = struct.unpack("<4s7I", header)
        if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or received_id != self.request_id or size > 19968:
            raise RuntimeError("response header or identity invalid")
        body = read_exact(self.port, size, 15)
        if zlib.crc32(body) != crc:
            raise RuntimeError("response CRC invalid")
        if status:
            raise RuntimeError(f"Titan rejected opcode {op}: status={status} body={body!r}")
        return json.loads(body)

    def frame(self, wire: bytes) -> dict:
        payload = struct.pack("<4I", 1, 1, 0, 128) + wire
        reply = self.transact(14, payload)
        if reply.get("op") != 14 or reply.get("pixels") != 128 or reply.get("bytes") != 384:
            raise RuntimeError("frame readback shape mismatch")
        if reply.get("crc") != zlib.crc32(wire) or reply.get("result") != 0:
            raise RuntimeError("frame CRC or emission result mismatch")
        return reply

    def close(self) -> None:
        if self.port is not None:
            self.port.close()
            self.port = None


def read_key(command_buffer: list[str]) -> tuple[str, str | None]:
    key = os.read(sys.stdin.fileno(), 1).decode("utf-8", errors="ignore")
    if command_buffer:
        if key in ("\r", "\n"):
            command = "".join(command_buffer[1:])
            command_buffer.clear()
            sys.stdout.write("\n")
            return "command", command
        if key in ("\x7f", "\b"):
            if len(command_buffer) > 1:
                command_buffer.pop()
                sys.stdout.write("\b \b")
                sys.stdout.flush()
            return "editing", None
        if key == "\x1b":
            command_buffer.clear()
            sys.stdout.write("\ncommand cancelled\n")
            return "editing", None
        if key.isprintable():
            command_buffer.append(key)
            sys.stdout.write(key)
            sys.stdout.flush()
        return "editing", None
    if key == ":":
        command_buffer.append(":")
        sys.stdout.write("\n:")
        sys.stdout.flush()
        return "editing", None
    return "hotkey", key


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=BUILD_DEFAULT)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--wait-seconds", type=float, default=120.0)
    args = parser.parse_args()
    if not sys.stdin.isatty():
        parser.error("interactive console requires a TTY")
    build = json.loads((args.build / "receipt.json").read_text())
    image_sha = hashlib.sha256((args.build / "rtthread.hex").read_bytes()).hexdigest()
    if build.get("pass") is not True or build.get("artifacts", {}).get("rtthread.hex") != image_sha:
        raise RuntimeError("build or image identity invalid")
    output = args.output or OUTPUT_ROOT / f"titan-led-console-{datetime.now().strftime('%Y%m%d-%H%M%S')}"
    output.mkdir(parents=True, exist_ok=False)
    receipt = {"pass": False, "start": datetime.now(timezone.utc).isoformat(),
               "operation": "TITAN_LED_INTERACTIVE_CONSOLE", "events": [],
               "photons": "NOT_CLAIMED", "image_sha256": image_sha}
    state = ConsoleState()
    client = TitanClient(build)
    old_terminal = None
    graceful = False
    try:
        info = client.open(args.wait_seconds)
        receipt["runtime"] = info
        old_terminal = termios.tcgetattr(sys.stdin.fileno())
        tty.setcbreak(sys.stdin.fileno())
        print(f"IDENTIFIED_TITAN uid={info['uid']} build={info['build']}")
        print(help_text())
        command_buffer: list[str] = []
        dirty = True
        next_frame = time.monotonic()
        while True:
            animated = state.pattern == "centre" and not state.paused
            timeout = max(0.0, next_frame - time.monotonic()) if animated else None
            ready, _, _ = select.select([sys.stdin], [], [], timeout)
            if ready:
                mode, value = read_key(command_buffer)
                if mode == "editing":
                    continue
                try:
                    action = apply_typed(value or "", state) if mode == "command" else apply_hotkey(value or "", state)
                except (ValueError, TypeError) as error:
                    print(f"ERROR: {error}")
                    continue
                if action == "help":
                    print(help_text())
                    continue
                if action == "status":
                    print(state_text(state))
                    continue
                if action == "quit":
                    graceful = True
                    break
                if action != "ignored":
                    print(action)
                    receipt["events"].append({"at": datetime.now(timezone.utc).isoformat(), "action": action})
                    dirty = True
            now = time.monotonic()
            if dirty or (animated and now >= next_frame):
                wire = frame_for_state(state)
                reply = client.frame(wire)
                receipt["last_frame"] = {"crc": reply["crc"], "emit_cycles": reply.get("emit_cycles"),
                                         "state": state_text(state)}
                advance(state)
                dirty = False
                next_frame = now + 1.0 / state.fps
        off = bytes(384)
        receipt["off_reply"] = client.frame(off)
        receipt["pass"] = graceful
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        if old_terminal is not None:
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, old_terminal)
        client.close()
        receipt["end"] = datetime.now(timezone.utc).isoformat()
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(f"\nTitan console closed; receipt={output / 'receipt.json'}")


if __name__ == "__main__":
    main()
