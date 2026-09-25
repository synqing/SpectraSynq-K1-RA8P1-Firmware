#!/usr/bin/env python3
"""Titan K1-RA8P1 control surface — local HTTP server.

One-shot config: reads current LiveConfigBlob (opcode 25 / GET_CONFIG) and
writes a new one (opcode 25 / BEGIN_SET + APPEND_SET + COMMIT_SET). Reuses the
proven transport in tools/serial-studio. Stdlib only — no new framework.

Run:  python3 tools/control-surface/server.py
Open: http://localhost:8765
"""
from __future__ import annotations

import json
import struct
import sys
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SS = ROOT / "tools" / "serial-studio"
sys.path.insert(0, str(SS))
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(HERE))

import cdc_lock  # noqa: E402
import live_lease  # noqa: E402
import live_protocol as proto  # noqa: E402
import programme_handoff as handoff  # noqa: E402
import titan_live_control as tlc  # noqa: E402
from titan_broker import close_serial, find_titan, open_serial  # noqa: E402
from titan_transport import CAMPAIGN_OPS, Transport  # noqa: E402

PORT = 8765
# Short run_dir: live_lease binds an AF_UNIX socket at run_dir/campaign.sock,
# which has a 104-char path limit on macOS. The deep docs/evidence path
# exceeds that, so keep run_dir under /tmp.
RUN_ROOT = Path("/tmp/titan-cs-runs")
CONFIG_FIELDS = (
    "palette_a", "palette_b", "mode_a", "mode_b", "flags", "brightness",
    "output_channel", "transition_ms", "travel_ms", "emit_on",
    "focus_a", "focus_b", "visual_a", "visual_b",
)
PRESET_DIR = HERE / "presets"
programme_quiesced = False


def _finite_unit(value, lo: float, hi: float, name: str) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"{name} is not a number") from exc
    if number != number or number < lo or number > hi:  # noqa: PLR0124 NaN
        raise ValueError(f"{name} out of range {lo}..{hi}")
    return number


def validate_config(desired: dict) -> None:
    if "focus_a" in desired and desired["focus_a"] is not None:
        _validate_focus(desired["focus_a"], "focus_a")
    if "focus_b" in desired and desired["focus_b"] is not None:
        _validate_focus(desired["focus_b"], "focus_b")
    for key in ("visual_a", "visual_b"):
        if isinstance(desired.get(key), dict):
            _validate_visual(desired[key], key)
    for key in ("palette_a", "palette_b"):
        if key in desired and desired[key] is not None:
            pal = int(desired[key])
            if pal < 0 or pal >= 44:
                raise ValueError(f"{key} out of range")
    for key in ("brightness",):
        if key in desired and desired[key] is not None:
            _finite_unit(desired[key], 0, 255, key)


def _validate_focus(arr, name: str) -> None:
    if not isinstance(arr, list) or len(arr) != 105:
        raise ValueError(f"{name} must be 105 floats")
    for i, value in enumerate(arr):
        _finite_unit(value, 0.0, 4.0, f"{name}[{i}]")


def _validate_visual(vis: dict, name: str) -> None:
    for key in proto._VIS_FLOATS:
        if key in vis and vis[key] is not None:
            _finite_unit(vis[key], -8.0, 8.0, f"{name}.{key}")
    if "hue_position" in vis and vis["hue_position"] is not None:
        _finite_unit(vis["hue_position"], 0.0, 1.0, f"{name}.hue_position")
    if "flags" in vis and vis["flags"] is not None:
        flags = int(vis["flags"])
        if flags < 0 or flags > 0xFFFFFFFF:
            raise ValueError(f"{name}.flags invalid")


def _readback_delta(requested: dict, applied: dict) -> dict:
    keys = ("palette_a", "palette_b", "mode_a", "mode_b", "brightness", "emit_on")
    mismatch = {k: {"requested": requested.get(k), "applied": applied.get(k)}
                for k in keys if requested.get(k) is not None and requested.get(k) != applied.get(k)}
    return {"ok": not mismatch, "mismatch": mismatch, "revision": applied.get("revision")}


def _new_run_dir() -> Path:
    d = RUN_ROOT / time.strftime("%Y%m%d-%H%M%S")
    d.mkdir(parents=True, exist_ok=True)
    return d


def _acquire_cdc_with_retry(device: str, owner: str, tries: int = 12, step: float = 0.12):
    """Acquire the CDC flock with short backoff.

    cdc_lock.acquire is non-blocking (LOCK_NB): any concurrent holder — a
    direct diagnostic script, a slow prior request, or an overlapping
    browser poll — makes it raise 'CDC lock busy' immediately, which used
    to surface as an HTTP 500 and look like a hung device. Retry briefly
    (~1.4 s worst case) so transient contention is absorbed instead of
    being shown to the operator as a dead surface.
    """
    last = None
    for _ in range(tries):
        try:
            return cdc_lock.acquire(device, owner)
        except RuntimeError as exc:
            last = exc
            time.sleep(step)
    raise last


def with_transport(fn, run_dir: Path | None = None):
    """Attach to the Titan, run fn(transport, info), release. One transaction.

    We own the CDC flock lifecycle ourselves (not tlc.attach) so a failure in
    open_serial cannot leak the flock — the 'stale broker holds CDC lock' symptom
    is exactly a leaked flock from tlc.attach raising after acquire.
    """
    if programme_quiesced or handoff.claimed():
        raise RuntimeError("programmer owns CDC")
    run_dir = run_dir or _new_run_dir()
    run_dir.mkdir(parents=True, exist_ok=True)
    usb = find_titan(inventory_lsof=False)
    handle = _acquire_cdc_with_retry(usb["device"], "titan-live-campaign")
    port = None
    lease = None
    try:
        port = open_serial(usb["device"], handle)
        transport = Transport(port, allowed={1}, campaign=True)
        info = transport.info(timeout=2.0)
        if info["info"].get("uid") != tlc.UID:
            raise RuntimeError(f"uid mismatch: got {info['info'].get('uid')}")
        transport.allow(set(CAMPAIGN_OPS))
        lease = live_lease.acquire(run_dir, "titan-live-campaign")
        return fn(transport, info)
    finally:
        try:
            if lease is not None:
                live_lease.release(run_dir, lease["token"])
        except Exception:
            pass
        try:
            if port is not None:
                close_serial(port, handle)
        except Exception:
            pass
        try:
            cdc_lock.release(handle)
        except Exception:
            pass


def get_state() -> dict:
    def body(transport, info):
        cfg = proto.unpack_config(
            transport.transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))["body"]
        )
        ps = transport.transact(17, b"")
        pal_status = {}
        if ps["ok"]:
            try:
                pal_status = json.loads(ps["body"])
            except Exception:
                pal_status = {"raw": ps["body"].decode(errors="replace")[:400]}
        return {"ok": True, "info": info["info"], "config": cfg, "palette_status": pal_status}
    return with_transport(body)


def apply_config(desired: dict) -> dict:
    validate_config(desired)
    def body(transport, info):
        cur = transport.transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))
        if not cur["ok"]:
            return {"ok": False, "error": f"GET_CONFIG status {cur['status']}"}
        current = proto.unpack_config(cur["body"])
        merged = dict(current)
        for k in CONFIG_FIELDS:
            if k in desired and desired[k] is not None:
                if k in ("visual_a", "visual_b") and isinstance(desired[k], dict):
                    base = dict(merged.get(k) or proto._VIS_DEFAULT)
                    base.update(desired[k])
                    merged[k] = base
                else:
                    merged[k] = desired[k]
        merged["revision"] = current["revision"]
        if current.get("visual_capable"):
            merged["version"] = 2
            merged["visual_capable"] = True
        else:
            merged["version"] = 1
            merged["visual_capable"] = False
            merged.pop("visual_a", None)
            merged.pop("visual_b", None)
        blob = proto.pack_config(merged)
        got = tlc._stage_set(transport, blob)
        if not got["ok"]:
            return {"ok": False, "error": f"COMMIT status {got['status']}"}
        applied = proto.unpack_config(got["body"])
        return {
            "ok": True,
            "config": applied,
            "info": info["info"],
            "readback": _readback_delta(desired, applied),
            "visual_capable": bool(applied.get("visual_capable")),
        }
    return with_transport(body)


def get_snapshot() -> dict:
    def body(transport, info):
        got = transport.transact(23, b"")
        if not got["ok"]:
            return {"ok": False, "error": f"snapshot status {got['status']}"}
        snap = proto.unpack_snapshot(got["body"])
        return {"ok": True, "snapshot": snap, "info": info["info"]}
    return with_transport(body)


def get_frame(channel: int = 0) -> dict:
    def body(transport, info):
        got = transport.transact(18, struct.pack("<I", int(channel)))
        if not got["ok"]:
            return {"ok": False, "error": f"frame status {got['status']}"}
        pixels = got["body"]
        rgb = [{"r": pixels[i], "g": pixels[i + 1], "b": pixels[i + 2]}
               for i in range(0, len(pixels), 3)]
        lit = sum(1 for p in rgb if p["r"] or p["g"] or p["b"])
        return {
            "ok": True,
            "channel": int(channel),
            "bytes": len(pixels),
            "pixels": rgb,
            "lit": lit,
            "info": info["info"],
        }
    return with_transport(body)


def list_presets() -> list:
    PRESET_DIR.mkdir(parents=True, exist_ok=True)
    names = sorted(p.stem for p in PRESET_DIR.glob("*.json"))
    return names


def save_preset(name: str, payload: dict) -> dict:
    PRESET_DIR.mkdir(parents=True, exist_ok=True)
    safe = "".join(ch if ch.isalnum() or ch in "-_" else "_" for ch in name)[:64]
    if not safe:
        raise ValueError("empty preset name")
    path = PRESET_DIR / f"{safe}.json"
    path.write_text(json.dumps(payload, indent=2) + "\n")
    return {"ok": True, "name": safe, "presets": list_presets()}


def load_preset(name: str) -> dict:
    safe = "".join(ch if ch.isalnum() or ch in "-_" else "_" for ch in name)[:64]
    path = PRESET_DIR / f"{safe}.json"
    if not path.is_file():
        raise FileNotFoundError(safe)
    return json.loads(path.read_text())


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, data, ctype="application/json"):
        b = data if isinstance(data, bytes) else json.dumps(data).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(b)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(b)

    def _err(self, msg, code=500):
        self._send(code, {"ok": False, "error": msg})

    def _read_json(self):
        n = int(self.headers.get("Content-Length", 0))
        return json.loads(self.rfile.read(n) or "{}")

    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            f = HERE / "index.html"
            if f.exists():
                self._send(200, f.read_bytes(), "text/html; charset=utf-8")
            else:
                self._err("index.html missing", 404)
            return
        if self.path == "/palette_data.js":
            f = HERE / "palette_data.js"
            if f.exists():
                self._send(200, f.read_bytes(), "application/javascript")
            else:
                self._err("palette_data.js missing", 404)
            return
        if self.path == "/api/state":
            try:
                self._send(200, get_state())
            except RuntimeError as e:
                if "programmer owns CDC" in str(e):
                    self._err(str(e), 503)
                else:
                    self._err(f"{type(e).__name__}: {e}")
            except SystemExit as e:
                self._err(str(e))
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}")
            return
        if self.path == "/api/snapshot":
            try:
                self._send(200, get_snapshot())
            except RuntimeError as e:
                self._err(str(e), 503 if "programmer owns CDC" in str(e) else 500)
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}")
            return
        if self.path.startswith("/api/frame"):
            try:
                channel = 0
                if "channel=" in self.path:
                    channel = int(self.path.split("channel=", 1)[1].split("&", 1)[0] or "0")
                self._send(200, get_frame(channel))
            except RuntimeError as e:
                self._err(str(e), 503 if "programmer owns CDC" in str(e) else 500)
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}")
            return
        if self.path == "/api/presets":
            self._send(200, {"ok": True, "presets": list_presets()})
            return
        self._err("not found", 404)

    def do_POST(self):
        global programme_quiesced
        if self.path == "/api/programme/quiesce":
            programme_quiesced = True
            self._send(200, {"ok": True, "quiesced": True})
            return
        if self.path == "/api/programme/resume":
            programme_quiesced = False
            self._send(200, {"ok": True, "quiesced": False})
            return
        if self.path == "/api/config":
            try:
                desired = self._read_json()
            except Exception as e:
                self._err(f"bad json: {e}", 400)
                return
            try:
                self._send(200, apply_config(desired))
            except ValueError as e:
                self._err(str(e), 400)
            except RuntimeError as e:
                self._err(str(e), 503 if "programmer owns CDC" in str(e) else 500)
            except SystemExit as e:
                self._err(str(e))
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}")
            return
        if self.path == "/api/presets":
            try:
                body = self._read_json()
                name = str(body.get("name") or "")
                cfg = body.get("config") or {}
                self._send(200, save_preset(name, {"config": cfg, "saved_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}))
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}", 400)
            return
        if self.path.startswith("/api/presets/"):
            name = self.path.rsplit("/", 1)[-1]
            try:
                payload = load_preset(name)
                applied = apply_config(payload.get("config") or {})
                applied["preset"] = name
                self._send(200, applied)
            except FileNotFoundError:
                self._err("preset not found", 404)
            except Exception as e:
                self._err(f"{type(e).__name__}: {e}")
            return
        self._err("not found", 404)

    def log_message(self, fmt, *args):
        sys.stderr.write(f"[{time.strftime('%H:%M:%S')}] {self.address_string()} {fmt % args}\n")


def main() -> int:
    RUN_ROOT.mkdir(parents=True, exist_ok=True)
    h = HTTPServer(("127.0.0.1", PORT), Handler)
    print(f"Titan control surface on http://localhost:{PORT}", file=sys.stderr)
    print(f"  serving {HERE}", file=sys.stderr)
    try:
        h.serve_forever()
    except KeyboardInterrupt:
        print("\nstop", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
