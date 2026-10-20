#!/usr/bin/env python3
"""Titan live control latency: command -> device readback, through the running mixer API.

Atlas Experiment 3, step 2. Reads CONTROL-LATENCY-FROZEN.json; does not widen it.
Board 0 speed only, alternating 0.50 / 0.55, restored to 0.50. No flash.

  python3.13 titan_control_latency.py --out DIR [--n 20]
"""
from __future__ import annotations

import argparse
import json
import time
import urllib.request
from pathlib import Path

API = "http://127.0.0.1:8776"


def get(path: str) -> dict:
    return json.load(urllib.request.urlopen(API + path, timeout=2))


def post(path: str, body: dict, token: str) -> int:
    req = urllib.request.Request(API + path, data=json.dumps(body).encode(), method="POST",
                                 headers={"Content-Type": "application/json", "X-SMC-Token": token})
    with urllib.request.urlopen(req, timeout=2) as r:
        return r.status


def speeds(s: dict) -> tuple[float | None, float | None]:
    b = s.get("board_states") or {}
    g = lambda k: (b.get(k) or b.get(int(k)) or {}).get("speed")  # noqa: E731
    return g("0"), g("1")


def one(value: float, token: str, b1_seen: list) -> dict:
    t0 = time.perf_counter()
    rendered0 = get("/api/status").get("rendered_request_id")
    t0 = time.perf_counter()
    status = post("/api/controls", {"fields": {"speed": value}, "board": 0}, token)
    confirmed = rendered = None
    while time.perf_counter() - t0 < 2.0:
        s = get("/api/status")
        s0, s1 = speeds(s)
        b1_seen.append(s1)
        now = time.perf_counter() - t0
        if confirmed is None and s0 is not None and abs(s0 - value) < 1e-4:
            confirmed = now
        if rendered is None and s.get("rendered_request_id") not in (None, rendered0):
            rendered = now
        if confirmed is not None and rendered is not None:
            break
        time.sleep(0.005)
    return {"value": value, "http": status, "confirm_s": confirmed, "rendered_s": rendered}


def pct(xs: list[float], q: float) -> float:
    xs = sorted(xs)
    k = max(0, min(len(xs) - 1, int(round(q / 100 * (len(xs) - 1)))))
    return xs[k]


def verdict(rows: list[dict], b1: list, restored: bool, add_s: float = 0.0) -> dict:
    lat = [r["confirm_s"] + add_s for r in rows if r["confirm_s"] is not None]
    all_ok = len(lat) == len(rows)
    p50, p95 = (pct(lat, 50), pct(lat, 95)) if lat else (None, None)
    b1_ok = all(v is not None and abs(v - 0.5) < 1e-4 for v in b1)
    return {"n": len(rows), "confirmed": len(lat), "p50_s": p50, "p95_s": p95,
            "max_s": max(lat) if lat else None, "jitter_s": (p95 - p50) if lat else None,
            "all_confirmed": all_ok, "p95_ok": bool(p95 is not None and p95 <= 0.150),
            "board1_ok": b1_ok, "restored": restored,
            "pass": bool(all_ok and p95 is not None and p95 <= 0.150 and b1_ok and restored)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--n", type=int, default=20)
    a = ap.parse_args()
    token = get("/api/session")["token"]
    s0, s1 = speeds(get("/api/status"))
    if s0 is None or abs(s0 - 0.5) > 1e-4 or s1 is None or abs(s1 - 0.5) > 1e-4:
        raise SystemExit(f"REFUSED: start state is not speed 0.5/0.5 (got {s0}, {s1})")
    rows, b1 = [], []
    for i in range(a.n):
        rows.append(one(0.55 if i % 2 == 0 else 0.50, token, b1))
        time.sleep(0.25)
    fin = one(0.50, token, b1)
    restored = fin["confirm_s"] is not None
    res = {"rows": rows, "restore": fin, "measured": verdict(rows, b1, restored),
           "plant_late_poll": verdict(rows, b1, restored, add_s=0.200),
           "plant_board1_command": verdict(rows, b1 + [0.55], restored)}
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "control-latency.json").write_text(json.dumps(res, indent=2) + "\n")
    for k in ("measured", "plant_late_poll", "plant_board1_command"):
        print(k, json.dumps(res[k]))
    rend = [r["rendered_s"] for r in rows if r["rendered_s"] is not None]
    if rend:
        print("rendered p50 %.3f p95 %.3f (n=%d)" % (pct(rend, 50), pct(rend, 95), len(rend)))
    return 0 if res["measured"]["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
