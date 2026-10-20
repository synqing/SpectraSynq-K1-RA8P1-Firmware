#!/usr/bin/env python3
"""Dump the installed framebuffer (opcode 18) after two palette applies.

Does not use CRC as proof. Scores lit pixels against the selected palette LUT.
Music, time and history still move the plate — this only asks whether the
dumped colours sit closer to the requested palette than to the other.
Does not flash. Target idle-preview remains untested.
"""
from __future__ import annotations

import json
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/evidence/K1-RA8P1-002/palette-dump-20260921-01"
PALETTE_JS = ROOT / "tools/control-surface/palette_data.js"
BASE = "http://127.0.0.1:8765"


def http_json(path: str, payload: dict | None = None) -> dict:
    data = None if payload is None else json.dumps(payload).encode()
    req = urllib.request.Request(
        BASE + path,
        data=data,
        headers={"Content-Type": "application/json"} if data else {},
        method="GET" if data is None else "POST",
    )
    with urllib.request.urlopen(req, timeout=20) as r:
        return json.loads(r.read())


def load_luts() -> list:
    text = PALETTE_JS.read_text()
    start = text.index("[[")
    end = text.index("]];") + 2
    return json.loads(text[start:end])


def rgb_hue(p: dict) -> float | None:
    r, g, b = p["r"] / 255.0, p["g"] / 255.0, p["b"] / 255.0
    mx, mn = max(r, g, b), min(r, g, b)
    if mx < 4 / 255.0 or mx == mn:
        return None
    span = mx - mn
    if mx == r:
        h = (g - b) / span
    elif mx == g:
        h = 2.0 + (b - r) / span
    else:
        h = 4.0 + (r - g) / span
    return (h / 6.0) % 1.0


def mean_hue(pixels: list) -> float | None:
    hues = [rgb_hue(p) for p in pixels]
    hues = [h for h in hues if h is not None]
    if not hues:
        return None
    # circular mean
    import math
    x = sum(math.cos(h * 6.283185307179586) for h in hues) / len(hues)
    y = sum(math.sin(h * 6.283185307179586) for h in hues) / len(hues)
    ang = math.atan2(y, x) / 6.283185307179586
    return ang % 1.0


def lut_mean_hue(lut: list) -> float | None:
    return mean_hue([{"r": r, "g": g, "b": b} for r, g, b in lut])


def hue_dist(a: float | None, b: float | None) -> float:
    if a is None or b is None:
        return 1.0
    d = abs(a - b)
    return min(d, 1.0 - d)


def dump_after(palette: int) -> dict:
    applied = http_json("/api/config", {"palette_a": palette, "palette_b": palette})
    time.sleep(0.7)
    state = http_json("/api/state")
    frame = http_json("/api/frame?channel=0")
    ps = state.get("palette_status") or {}
    return {
        "requested_palette": palette,
        "applied_ok": bool(applied.get("ok")),
        "applied_palette": (applied.get("config") or {}).get("palette_a"),
        "status_palette": ps.get("palette_a") or ps.get("palette_id"),
        "readback": applied.get("readback"),
        "visual_path": ps.get("visual_path"),
        "musical": ps.get("musical"),
        "lit": frame.get("lit"),
        "bytes": frame.get("bytes"),
        "pixels": frame.get("pixels") or [],
    }


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    luts = load_luts()
    hue5 = lut_mean_hue(luts[5])
    hue33 = lut_mean_hue(luts[33])
    first = dump_after(5)
    second = dump_after(33)
    h_first = mean_hue(first["pixels"])
    h_second = mean_hue(second["pixels"])
    first_closer = hue_dist(h_first, hue5) < hue_dist(h_first, hue33)
    second_closer = hue_dist(h_second, hue33) < hue_dist(h_second, hue5)
    pixels_moved = sum(
        1 for a, b in zip(first["pixels"], second["pixels"])
        if a != b
    )
    both_effect = first["visual_path"] == "effect" and second["visual_path"] == "effect"
    receipt = {
        "installed_image": True,
        "flashed": False,
        "target_idle_preview": "untested",
        "lut_hue_5": hue5,
        "lut_hue_33": hue33,
        "palette_5": {
            "visual_path": first["visual_path"],
            "musical": first["musical"],
            "lit": first["lit"],
            "mean_hue": h_first,
            "hue_dist_to_5": hue_dist(h_first, hue5),
            "hue_dist_to_33": hue_dist(h_first, hue33),
            "closer_to_requested": first_closer,
            "applied_palette": first["applied_palette"],
            "status_palette": first.get("status_palette"),
            "mean_rgb": None,
        },
        "palette_33": {
            "visual_path": second["visual_path"],
            "musical": second["musical"],
            "lit": second["lit"],
            "mean_hue": h_second,
            "hue_dist_to_33": hue_dist(h_second, hue33),
            "hue_dist_to_5": hue_dist(h_second, hue5),
            "closer_to_requested": second_closer,
            "applied_palette": second["applied_palette"],
            "status_palette": second.get("status_palette"),
        },
        "pixels_changed_between_dumps": pixels_moved,
        "both_dumps_during_effect": both_effect,
        "pass": bool(
            first["applied_ok"] and second["applied_ok"]
            and first["lit"] and second["lit"]
            and first_closer and second_closer
            and pixels_moved >= 8
            and first.get("status_palette") == 5
            and second.get("status_palette") == 33
        ),
        "note": (
            "Framebuffer dump scored by circular hue against palette LUTs, "
            "not CRC. Target idle-preview remains untested. "
            "A hold-path dump is not a music-effect proof."
        ),
    }
    (OUT / "dump-palette-5.json").write_text(json.dumps({
        "visual_path": first["visual_path"], "pixels": first["pixels"],
        "mean_hue": h_first,
    }) + "\n")
    (OUT / "dump-palette-33.json").write_text(json.dumps({
        "visual_path": second["visual_path"], "pixels": second["pixels"],
        "mean_hue": h_second,
    }) + "\n")
    (OUT / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    printable = {k: receipt[k] for k in receipt if k != "note"}
    print(json.dumps(printable, indent=2))
    print("path", OUT / "receipt.json")
    return 0 if receipt["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
