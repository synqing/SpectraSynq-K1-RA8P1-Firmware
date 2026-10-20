#!/usr/bin/env python3
from __future__ import annotations

import io
import os
import tarfile
import zipfile
from pathlib import Path

EV = Path("/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/e2lite-cli-20260921-01")
PLUGINS = Path("/Applications/Renesas e2 studio with RA FSP v6.6.0/Renesas e2 studio with RA FSP 6.6.0.app/Contents/eclipse/plugins")
NEEDLES = (b"R7KA8", b"RA8P1", b"R7KA8P1KF", b"R7FA8P1")


def hits(data: bytes) -> list[str]:
    found = []
    for n in NEEDLES:
        if n in data:
            found.append(n.decode())
    return found


print("=== gdb_server.tar.xz listing ===")
jar = PLUGINS / "com.renesas.ide.supportfiles.ra.gdbserver.macosx.aarch64_10.6.0.v20260707-035422.jar"
with zipfile.ZipFile(jar) as z:
    print("jar members", z.namelist()[:40], "count", len(z.namelist()))
    for name in z.namelist():
        if "tar" in name.lower() or name.endswith(".xz") or "device" in name.lower():
            print(" member", name, z.getinfo(name).file_size)
            raw = z.read(name)
            if name.endswith(".xz") or "tar" in name:
                import lzma
                data = lzma.decompress(raw)
                tf = tarfile.open(fileobj=io.BytesIO(data), mode="r:")
                names = tf.getnames()
                print("  tar files", len(names))
                for n in names:
                    if any(k in n.lower() for k in ("xml", "xmi", "device", "cpu", "ra8", "r7k", "arm")):
                        print("   ", n)
                # also dump first 80 names
                print("  first80:")
                for n in names[:80]:
                    print("   ", n)

print("=== scan key jars for RA8 bytes ===")
key = [
    "com.renesas.ide.supportfiles.ra.debug_2.0.200.v20260612-1041.jar",
    "com.renesas.ide.supportfiles.ra.rpdm.macosx.aarch64_10.6.0.v20260707-035422.jar",
    "com.renesas.ide.supportfiles.ra.gdbserver.macosx.aarch64_10.6.0.v20260707-035422.jar",
    "com.renesas.ide.supportfiles.ra.debug.gdb.macosx.aarch64_2.0.200.v20260707-1008.jar",
]
for kn in key:
    p = PLUGINS / kn
    print("JAR", kn, "size", p.stat().st_size if p.exists() else "MISSING")
    if not p.exists():
        continue
    with zipfile.ZipFile(p) as z:
        for info in z.infolist():
            if info.file_size > 80_000_000:
                print("  skip huge", info.filename, info.file_size)
                continue
            data = z.read(info)
            h = hits(data)
            if h:
                print("  HIT", info.filename, info.file_size, h)
                # extract unique nearby tokens
                for needle in NEEDLES:
                    i = 0
                    c = 0
                    while c < 8:
                        j = data.find(needle, i)
                        if j < 0:
                            break
                        win = data[max(0, j - 40) : j + 80]
                        printable = "".join(chr(b) if 32 <= b < 127 else "|" for b in win)
                        print("   ", printable)
                        i = j + len(needle)
                        c += 1

print("=== plugin names containing ra8 / r7ka / device ===")
for p in sorted(PLUGINS.iterdir()):
    n = p.name.lower()
    if any(k in n for k in ("ra8", "r7ka", "device", "debuginfo", "pack", "svd", "pdsc")):
        print(p.name)
