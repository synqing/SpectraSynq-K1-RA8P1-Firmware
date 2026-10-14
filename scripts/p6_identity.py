#!/usr/bin/env python3
"""Lane P6 C4: v8 stays bit-identical after the v9 change.

  p6_identity.py host  --before DIR --after DIR                  # replay.json frame hashes
  p6_identity.py arm   --before BUILD --after BUILD [--plant]    # loadable bytes + renderer object
  p6_identity.py selftest --before BUILD --after BUILD --host-before DIR --host-red DIR

ARM rule (frozen): the objcopy -O binary images may differ only inside each image's own
K1_BUILD_ID string and inside RT-Thread's __TIME__ string ("HH:MM:SS" after the banner);
inhouse_waveform.o disassembly must match exactly. Exit status is the verdict.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TC = ROOT.parents[2] / "toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin"  # build_scalar TOOLCHAIN


def host(before: Path, after: Path) -> dict:
    a = json.loads((before / "replay.json").read_text())["hashes"]
    b = json.loads((after / "replay.json").read_text())["hashes"]
    rows = {k: {"before": a[k]["frames_sha256"], "after": b.get(k, {}).get("frames_sha256")} for k in sorted(a)}
    ok = sorted(a) == sorted(b) and all(r["before"] == r["after"] for r in rows.values())
    return {"pass": ok, "inputs": rows}


def image(build: Path) -> bytes:
    out = build / "p6-loadable.bin"
    subprocess.run([str(TC / "arm-none-eabi-objcopy"), "-O", "binary", str(build / "rtthread.elf"), str(out)], check=True)
    data = out.read_bytes()
    out.unlink()
    return data


def allowed_regions(data: bytes, build: Path) -> list[tuple[int, int, str]]:
    regions = []
    build_id = json.loads((build / "receipt.json").read_text())["build_id"].encode()
    for m in re.finditer(re.escape(build_id), data):
        regions.append((m.start(), m.end(), "K1_BUILD_ID"))
    banner = data.find(b"build %s %s")
    if banner >= 0:
        for m in re.finditer(rb"\d\d:\d\d:\d\d\x00", data[banner:banner + 96]):
            regions.append((banner + m.start(), banner + m.start() + 8, "__TIME__"))
    return regions


def disassembly(build: Path) -> str:
    obj = build / "stage/src/k1/core/visual/inhouse_waveform.o"
    text = subprocess.run([str(TC / "arm-none-eabi-objdump"), "-d", str(obj)], capture_output=True, text=True, check=True).stdout
    return "\n".join(line for line in text.splitlines() if "file format" not in line)


def arm(before: Path, after: Path, plant: bool = False) -> dict:
    a, b = image(before), image(after)
    if plant:  # planted fault: one byte outside every allowed region
        b = bytearray(b)
        b[0x400] ^= 0x01
        b = bytes(b)
    regions = allowed_regions(a, before) + allowed_regions(b, after)
    diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
    outside = [i for i in diffs if not any(lo <= i < hi for lo, hi, _ in regions)]
    dis_equal = disassembly(before) == disassembly(after)
    ok = len(a) == len(b) and not outside and dis_equal
    return {"pass": ok, "bytes": [len(a), len(b)], "differing_bytes": len(diffs), "outside_allowed": outside[:20],
            "allowed_regions": [{"start": lo, "end": hi, "what": w} for lo, hi, w in regions],
            "renderer_object_disassembly_equal": dis_equal}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    h = sub.add_parser("host"); h.add_argument("--before", type=Path, required=True); h.add_argument("--after", type=Path, required=True)
    r = sub.add_parser("arm"); r.add_argument("--before", type=Path, required=True); r.add_argument("--after", type=Path, required=True)
    r.add_argument("--plant", action="store_true")
    s = sub.add_parser("selftest")
    for k in ("--before", "--after", "--host-before", "--host-red"):
        s.add_argument(k, type=Path, required=True)
    a = ap.parse_args(argv)
    if a.cmd == "host":
        res = host(a.before, a.after)
    elif a.cmd == "arm":
        res = arm(a.before, a.after, a.plant)
    else:  # both planted faults must go red
        red_arm = arm(a.before, a.after, plant=True)
        red_host = host(a.host_before, a.host_red)
        res = {"pass": (not red_arm["pass"]) and (not red_host["pass"]),
               "planted_arm_byte_red": not red_arm["pass"], "planted_host_v9_vs_v8_red": not red_host["pass"]}
    print(json.dumps(res, indent=2))
    return 0 if res["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
