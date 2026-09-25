#!/usr/bin/env python3
"""Find e2-server-gdb CPU names for RA8P1 / E2 Lite."""
from __future__ import annotations

import os
import re
import zipfile
from pathlib import Path

PAT = re.compile(rb"R7KA8[A-Z0-9_]*|R7FA8[A-Z0-9_]*|RA8P1[A-Z0-9_]*")
E2L = re.compile(rb"E2LITE.{0,80}R7K|R7K.{0,80}E2LITE", re.DOTALL)

ROOTS = [
    Path("/Applications/Renesas e2 studio with RA FSP v6.6.0"),
    Path("/Users/spectrasynq/Workspace_Management/Tools/Renesas"),
    Path("/tmp/renesas-host-check-e4rag280"),
    Path("/Users/spectrasynq/Applications/Renesas"),
]


def scan_bytes(label: str, data: bytes) -> None:
    names = sorted({m.decode("ascii", "ignore") for m in PAT.findall(data)})
    if names:
        print(f"{label}: {len(names)}")
        for n in names[:80]:
            print(f"  {n}")


print("=== english.xml ===")
ex = Path("/tmp/renesas-host-check-e4rag280/english.xml")
print("size", ex.stat().st_size)
scan_bytes("english.xml", ex.read_bytes())

print("=== e2-server-gdb binary ===")
binp = Path("/tmp/renesas-host-check-e4rag280/e2-server-gdb")
b = binp.read_bytes()
scan_bytes("binary", b)
i = b.find(b"No matching device found")
print("msg", b[max(0, i - 120) : i + 200] if i >= 0 else "absent")
# device table nearby strings containing E2LITE
idx = 0
pairs = []
while True:
    j = b.find(b"E2LITE", idx)
    if j < 0:
        break
    window = b[max(0, j - 64) : j + 96]
    if b"R7" in window or b"RA" in window:
        pairs.append(window.replace(b"\x00", b" ").decode("ascii", "ignore"))
    idx = j + 6
    if len(pairs) > 30:
        break
print("E2LITE nearby windows", len(pairs))
for p in pairs[:20]:
    print(" ", repr(p)[:200])

print("=== walk plugins for gdbserver jars ===")
jars: list[Path] = []
for root in ROOTS:
    if not root.exists():
        print("missing", root)
        continue
    for dirpath, dirnames, filenames in os.walk(root):
        dn = dirpath.lower()
        if any(x in dn for x in (".fseventsd", "node_modules", ".git")):
            dirnames[:] = []
            continue
        for fn in filenames:
            low = fn.lower()
            if "gdbserver" in low or "debuginfo" in low or "deviceinfo" in low:
                jars.append(Path(dirpath) / fn)
            elif low.endswith(".jar") and ("supportfiles.ra" in low or "gdb" in low):
                jars.append(Path(dirpath) / fn)

print("files", len(jars))
for p in jars[:60]:
    print(" ", p)

print("=== scan jars/xml for RA8 names ===")
for p in jars:
    try:
        if p.suffix == ".jar" or zipfile.is_zipfile(p):
            with zipfile.ZipFile(p) as z:
                for info in z.infolist():
                    name = info.filename.lower()
                    if info.file_size > 20_000_000:
                        continue
                    if any(x in name for x in ("xml", "json", "csv", "txt", "device", "cpu")):
                        data = z.read(info)
                        if PAT.search(data) or b"R7KA8" in data:
                            print("HIT", p.name, info.filename)
                            scan_bytes(f"{p.name}:{info.filename}", data)
        else:
            data = p.read_bytes()
            if PAT.search(data):
                print("HIT file", p)
                scan_bytes(str(p), data)
    except Exception as e:
        print("skip", p, e)
