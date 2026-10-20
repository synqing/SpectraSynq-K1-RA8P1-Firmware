"""Loopback status for the Emotiscope strategy. Page load does not write Titan."""
from __future__ import annotations

import hmac
import json
from http.server import BaseHTTPRequestHandler
import secrets
from urllib.parse import urlparse

from .dashboard_server import BoundedServer

PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Titan Emotiscope mixer</title>
<style>
body { font: 15px/1.4 sans-serif; margin: 24px; max-width: 920px; }
table { border-collapse: collapse; width: 100%; }
td, th { border-bottom: 1px solid #ccc; text-align: left; padding: 4px 8px; }
button { margin: 4px 4px 4px 0; }
.note { color: #333; }
</style>
</head>
<body>
<h1>Titan Emotiscope mixer</h1>
<p>Donor brightness and output brightness are different controls. This page shows what the board last accepted. It is not a picture of the lights.</p>
<table>
<tr><th>Control</th><th>MIDI</th><th>Does</th></tr>
<tr><td>Faders 1–8</td><td>ch 0, CC 40–47</td><td>Donor brightness, softness, speed, background, color, color range, saturation, warmth</td></tr>
<tr><td>Knob 1</td><td>ch 0, CC 30</td><td>Output brightness, 0–255</td></tr>
<tr><td>M1–M7</td><td>ch 0, CC 20–26</td><td>Analog, Spectrum, Octave, Metronome, Spectronome, Hype, Bloom</td></tr>
<tr><td>S1–S5</td><td>S1/S2 ch 0 CC 28/29; S3–S5 ch 1 CC 30–32</td><td>Mirror, reverse color, auto color, screensaver, dither</td></tr>
<tr><td>Play / Pause</td><td>ch 0, CC 52 / 53</td><td>Emission on / off</td></tr>
<tr><td>Previous / Next</td><td>ch 0, CC 57 / 58</td><td>Previous / next musical mode</td></tr>
<tr><td>SQ1–SQ8</td><td>see profile</td><td>Recall a saved scene. An empty slot does nothing.</td></tr>
</table>
<p class="note">Saturation fader position is squared for pickup and square-rooted once when sent. Auto color does not make the color fader chase the moving hue. Fader 1 and Fader 4 wait until each has moved on channel 0, so they cannot be confused with R5 and R8.</p>
<p>
<button id="defaults" type="button">Restore donor defaults</button>
<button id="save" type="button">Save scene SQ1</button>
</p>
<pre id="state">waiting</pre>
<script>
let token = "";
async function session() {
  const reply = await fetch("/api/session");
  token = (await reply.json()).token;
}
async function refresh() {
  const reply = await fetch("/api/status");
  const body = await reply.json();
  const lines = [
    "mode " + body.mode_name + " (" + body.mode + ")",
    "donor brightness " + body.donor_brightness,
    "output brightness " + body.output_brightness + "/255",
    "emission " + (body.emission_enabled ? "on" : "off"),
    "hue sample " + body.color_reference,
    "accepted " + body.accepted_request_id + "  rendered " + body.rendered_request_id + "  emitted " + body.emitted_request_id,
    "fader 1 and 4 writes armed: " + body.faders_armed,
    (body.applicability_notes || []).join("\\n")
  ];
  document.getElementById("state").textContent = lines.join("\\n");
}
async function post(path, payload) {
  if (!token) await session();
  await fetch(path, {method: "POST", headers: {"Content-Type": "application/json", "X-SMC-Token": token}, body: JSON.stringify(payload)});
}
document.getElementById("defaults").onclick = () => post("/api/defaults", {});
document.getElementById("save").onclick = () => post("/api/scene", {slot: "SQ1", action: "save"});
refresh();
setInterval(refresh, 400);
</script>
</body>
</html>
"""


def make_emotiscope_dashboard(status, engine, port: int):
    token = secrets.token_urlsafe(32)

    class Handler(BaseHTTPRequestHandler):
        def setup(self):
            super().setup()
            self.connection.settimeout(3)

        def log_message(self, *_args):
            return

        def allowed(self) -> bool:
            host = self.headers.get("Host", "")
            here = self.server.server_port
            if host not in {f"127.0.0.1:{here}", f"localhost:{here}"}:
                return False
            origin = self.headers.get("Origin")
            return origin is None or origin in {f"http://127.0.0.1:{here}", f"http://localhost:{here}"}

        def reply(self, value, code=200, kind="application/json"):
            data = json.dumps(value, allow_nan=False).encode() if kind == "application/json" else value
            self.send_response(code)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def do_GET(self):
            if not self.allowed():
                self.reply({"error": "Local origin required"}, 403)
                return
            path = urlparse(self.path).path
            if path in {"/status", "/api/status"}:
                self.reply(status())
            elif path == "/api/session":
                self.reply({"token": token})
            elif path == "/":
                self.reply(PAGE.encode(), kind="text/html; charset=utf-8")
            else:
                self.reply({"error": "Not found"}, 404)

        def do_POST(self):
            if not self.allowed() or not hmac.compare_digest(self.headers.get("X-SMC-Token", ""), token):
                self.reply({"error": "Local session token required"}, 403)
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 4096:
                    raise ValueError("body size")
                body = json.loads(self.rfile.read(length))
                path = urlparse(self.path).path
                if path == "/api/scene" and body.get("action") == "save":
                    engine.request_save(str(body.get("slot")))
                elif path == "/api/scene" and body.get("action") == "recall":
                    engine.request_recall(str(body.get("slot")))
                elif path == "/api/defaults":
                    engine.request_defaults()
                elif path == "/api/remap":
                    engine.request_remap(str(body.get("control")), body.get("field"))
                else:
                    self.reply({"error": "Not found"}, 404)
                    return
                self.reply({"state": "pending"}, 202)
            except (ValueError, TypeError, KeyError) as exc:
                self.reply({"error": str(exc)}, 400)

    return BoundedServer(("127.0.0.1", port), Handler)
