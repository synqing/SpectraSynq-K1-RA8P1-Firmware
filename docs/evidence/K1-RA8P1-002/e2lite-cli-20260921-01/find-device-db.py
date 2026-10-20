#!/usr/bin/env python3
from __future__ import annotations

import os
import zipfile
from pathlib import Path

NEEDLE = b"R7FA0E105"
NEEDLE2 = b"R7KA8"
NEEDLE3 = b"SkipSelectDeviceCheck"

roots = [
    Path("/Applications/Renesas e2 studio with RA FSP v6.6.0"),
    Path("/Users/spectrasynq/Workspace_Management/Tools/Renesas"),
    Path("/tmp/renesas-host-check-e4rag280"),
]

print("=== locate DebugComp / devices.xml / R7FA0E105 files ===")
hits_files = []
for root in roots:
    if not root.exists():
        continue
    for dirpath, dirnames, filenames in os.walk(root):
        dn = dirpath.lower()
        if any(x in dn for x in (".fseventsd", "node_modules", ".git", "toolchains")):
            dirnames[:] = [d for d in dirnames if d not in ("arm-gnu-toolchain-13.2.Rel1-darwin-arm64-arm-none-eabi",)]
            if "toolchains" in dn:
                dirnames[:] = []
                continue
        base = os.path.basename(dirpath)
        if base in ("DebugComp", "debugcomp", "DeviceSupport", "devices"):
            print("DIR", dirpath)
        for fn in filenames:
            low = fn.lower()
            if any(k in low for k in ("devices.xml", "device.xmi", "r7ka8", "ra8p1", "r7fa0e105")):
                p = os.path.join(dirpath, fn)
                print("FILE", p)
                hits_files.append(p)

print("=== gdb binary path-like strings ===")
b = Path("/tmp/renesas-host-check-e4rag280/e2-server-gdb").read_bytes()
import re
strs = re.findall(rb"[\x20-\x7e]{8,120}", b)
keys = (b"xml", b"xmi", b"Device", b"DebugComp", b"RPDM", b"CPU", b"SkipSelect", b".xml", b"devices")
for s in strs:
    if any(k.lower() in s.lower() for k in keys) and not s.startswith(b"python"):
        if any(k in s for k in (b"xml", b"xmi", b"Device", b"Debug", b"Skip", b"RPDM", b"CPU =", b"matching")):
            print(s.decode("ascii", "ignore"))

print("=== rpdm jar members ===")
rpdm = Path("/Applications/Renesas e2 studio with RA FSP v6.6.0/Renesas e2 studio with RA FSP 6.6.0.app/Contents/eclipse/plugins/com.renesas.ide.supportfiles.ra.rpdm.macosx.aarch64_10.6.0.v20260707-035422.jar")
with zipfile.ZipFile(rpdm) as z:
    for n in z.namelist():
        print(n, z.getinfo(n).file_size)

print("=== arm.common.debug jar members containing xml/xmi ===")
dbg = Path("/Applications/Renesas e2 studio with RA FSP v6.6.0/Renesas e2 studio with RA FSP 6.6.0.app/Contents/eclipse/plugins/com.renesas.e2studio.device.arm.common.debug_8.1.1.v20251120-1514.jar")
if dbg.exists():
    with zipfile.ZipFile(dbg) as z:
        for n in z.namelist():
            data = z.read(n)
            if NEEDLE in data or NEEDLE2 in data or b"R7FA" in data[:200] or n.lower().endswith((".xml", ".xmi")):
                print(n, z.getinfo(n).file_size, "R7KA8" if NEEDLE2 in data else "", "R7FA0" if NEEDLE in data else "")
