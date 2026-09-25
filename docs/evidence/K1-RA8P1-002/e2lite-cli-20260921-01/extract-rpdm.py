#!/usr/bin/env python3
from __future__ import annotations

import io
import lzma
import os
import tarfile
import zipfile
from pathlib import Path

PLUGINS = Path("/Applications/Renesas e2 studio with RA FSP v6.6.0/Renesas e2 studio with RA FSP 6.6.0.app/Contents/eclipse/plugins")
rpdm = PLUGINS / "com.renesas.ide.supportfiles.ra.rpdm.macosx.aarch64_10.6.0.v20260707-035422.jar"
out = Path("/tmp/renesas-rpdm-extract")
out.mkdir(exist_ok=True)

print("=== rpdm tar listing ===")
with zipfile.ZipFile(rpdm) as z:
    raw = z.read("gdb_server.tar.xz")
data = lzma.decompress(raw)
tf = tarfile.open(fileobj=io.BytesIO(data), mode="r:")
names = tf.getnames()
print("count", len(names))
for n in names:
    print(n)
    if any(k in n.lower() for k in ("ra8", "r7k", "xml", "xmi", "device", "e2l")):
        pass

print("=== extract rpdm tar ===")
tf.extractall(out)
print("extracted to", out)

print("=== search extracted + packs for R7KA8 / DeviceList ===")
needles = (b"R7KA8", b"R7KA8P1KF", b"RA8P1", b"<DeviceList", b"E2LITE")
packs = Path("/Applications/Renesas e2 studio with RA FSP v6.6.0/fsp/internal/projectgen/ra/packs")
scan_roots = [out, packs, Path("/Users/spectrasynq/Library"), Path("/Users/spectrasynq/Workspace_Management/Tools/Renesas")]
# Library walk is huge - only search e2studio-ish
for root in [out, packs]:
    for dirpath, _, filenames in os.walk(root):
        for fn in filenames:
            p = Path(dirpath) / fn
            try:
                if p.stat().st_size > 30_000_000:
                    continue
                data = p.read_bytes()
            except Exception:
                continue
            found = [n.decode() for n in needles if n in data]
            if found:
                print("HIT", p, p.stat().st_size, found)

print("=== E2RFW pack listing ===")
for pack in packs.glob("*RA8P1*"):
    print("PACK", pack.name, pack.stat().st_size)
    if zipfile.is_zipfile(pack):
        with zipfile.ZipFile(pack) as z:
            for n in z.namelist()[:80]:
                print(" ", n)
            for info in z.infolist():
                if info.file_size > 20_000_000:
                    continue
                blob = z.read(info)
                found = [n.decode() for n in needles if n in blob]
                if found:
                    print("  HIT", info.filename, info.file_size, found)
                    if b"R7KA8" in blob:
                        i = blob.find(b"R7KA8")
                        print("   ctx", blob[max(0,i-60):i+80])
