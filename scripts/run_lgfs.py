#!/usr/bin/env python3
"""Select an LGFS optical test card on the Titan (opcode 28) and optionally verify the wire.

  python3 scripts/run_lgfs.py --sel status
  python3 scripts/run_lgfs.py --sel impulse,top,80 --verify
  python3 scripts/run_lgfs.py --sel off

The broker owns the Titan CDC while it runs. For a bench session:
  launchctl bootout gui/$(id -u)/com.spectrasynq.titan-broker      # before
  launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.spectrasynq.titan-broker.plist   # after
`off` hands the wire back to the live renderer (autonomous music) on the next frame.

--verify reads the wire snapshot of both boards (opcode 19, board 0 and 1) and compares
every packed pixel with the shared kit header's expected frame (lgfs_host from the kit).
A match proves what was submitted to the wire, not photons.
Exit 1 on a refusal, an identity mismatch or a wire mismatch.
"""
from __future__ import annotations
import argparse
import json
import struct
import subprocess
import sys
import time
import zlib
from pathlib import Path

import serial
from serial.tools import list_ports

UID = '545433931bd25436593630352d068363'
VIDPID = (0x045B, 0x5310)
OPCODE = 28
KIT = Path('/Users/spectrasynq/Workspace_Management/Software/K1-System-R3/outputs/lgfs-bench-20261016')


def packet(op, request, payload=b''):
    header = struct.pack('<4s6I', b'K1S1', op, request, 0, len(payload), zlib.crc32(payload), 1)
    return header + struct.pack('<I', zlib.crc32(header)) + payload


def read_exact(port, size, timeout):
    data = bytearray()
    deadline = time.monotonic() + timeout
    while len(data) < size and time.monotonic() < deadline:
        data.extend(port.read(size - len(data)))
    if len(data) != size:
        raise RuntimeError(f'USB timeout {len(data)}/{size}')
    return bytes(data)


class Titan:
    def __init__(self, device=None):
        if device is None:
            ports = [p.device for p in list_ports.comports() if (p.vid, p.pid) == VIDPID]
            if len(ports) != 1:
                raise SystemExit(f'expected one Titan 045b:5310, found {len(ports)}')
            device = ports[0]
        try:
            self.port = serial.Serial(device, 115200, timeout=0.2, exclusive=True)
        except serial.SerialException as exc:
            raise SystemExit(f'cannot open {device} ({exc}). If the broker holds it: '
                             'launchctl bootout gui/$(id -u)/com.spectrasynq.titan-broker')
        self.port.reset_input_buffer()
        self.request = int(time.time()) & 0xFFFF

    def transact(self, op, payload=b'', timeout=5.0):
        self.request += 1
        data = packet(op, self.request, payload)
        self.port.write(data)
        self.port.flush()
        header = read_exact(self.port, 32, timeout)
        magic, status, rid, _seq, size, _cycles, crc, header_crc = struct.unpack('<4s7I', header)
        if magic != b'K1R1' or zlib.crc32(header[:28]) != header_crc or rid != self.request:
            raise RuntimeError('response header invalid')
        body = read_exact(self.port, size, timeout)
        if zlib.crc32(body) != crc:
            raise RuntimeError('response body CRC invalid')
        return status, body


def expected(sel, frame=0):
    host = KIT / 'host'
    exe = Path('/tmp/lgfs_host_mac')
    src = host / 'lgfs_host.c'
    if not exe.exists() or exe.stat().st_mtime < src.stat().st_mtime:
        subprocess.run(['cc', '-std=c99', '-O2', '-I', str(KIT / 'shared'), '-o', str(exe), str(src)], check=True)
    out = subprocess.run([str(exe), sel, str(frame)], capture_output=True, text=True)
    rec = json.loads(out.stdout)
    if not rec.get('ok'):
        raise RuntimeError(f'host harness refused {sel}: {rec.get("error")}')
    return rec


def lanes_for(edge_pixels, board):
    """Same split as lgfs_pack_lane: board 1 DIN0 carries the right half."""
    def pack(p):
        r, g, b = p
        return bytes([g >> 8, g & 255, r >> 8, r & 255, b >> 8, b & 255])
    halves = [b''.join(pack(p) for p in edge_pixels[:80]), b''.join(pack(p) for p in edge_pixels[80:])]
    return (halves[1], halves[0]) if board == 1 else (halves[0], halves[1])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--sel', default='status')
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--port')
    a = ap.parse_args()
    t = Titan(a.port)
    status, body = t.transact(1)
    info = json.loads(body)
    if info.get('uid') != UID:
        raise SystemExit(f'UID mismatch: {info.get("uid")}')
    status, body = t.transact(OPCODE, struct.pack('<I', 1) + a.sel.encode() + b'\0')
    if status != 0:
        print(json.dumps({'op': 28, 'ok': False, 'status': status,
                          'hint': 'status 10 = image without the LGFS opcode; 3 = bad request'}))
        return 1
    reply = json.loads(body)
    reply['build'] = info.get('build', '')[:16]
    print(json.dumps(reply))
    if not reply.get('ok'):
        return 1
    if a.verify and a.sel not in ('status', 'off'):
        exp = expected(a.sel)
        results = {}
        for _attempt in range(40):          # re-read until the card has reached both snapshots
            results = {}
            for board, key in ((0, 'top'), (1, 'bottom')):
                status, snap = t.transact(19, struct.pack('<I', board))
                if status != 0 or len(snap) < 64 + 480 + 960:
                    results[board] = (key, -1)
                    continue
                lane0, lane1 = snap[544:1024], snap[1024:1504]
                want0, want1 = lanes_for(exp[key], board)
                same = sum(lane0[i*6:i*6+6] == want0[i*6:i*6+6] for i in range(80)) + \
                       sum(lane1[i*6:i*6+6] == want1[i*6:i*6+6] for i in range(80))
                results[board] = (key, same)
            if all(v[1] == 160 for v in results.values()):
                break
        bad = 0
        for board, (key, same) in results.items():
            if same < 0:
                print(f'board {board} ({key} edge): snapshot unavailable'); bad += 1
            else:
                print(f'board {board} ({key} edge): WIRE {"MATCH" if same == 160 else "MISMATCH"} {same}/160')
                bad += same != 160
        return 1 if bad else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
