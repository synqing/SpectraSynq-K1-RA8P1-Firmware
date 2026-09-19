#!/usr/bin/env python3
"""Derive titan-live.ssproj from the accepted SS-02 project. Do not modify titan.ssproj."""
from __future__ import annotations

import copy
import hashlib
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, "/Users/spectrasynq/Serial-Studio/tests/utils")

from api_client import APIError, SerialStudioClient  # noqa: E402

BASE = HERE / "titan.ssproj"
LIVE = HERE / "titan-live.ssproj"
ACCEPTED = "b9a8562e4259a741bb9e2261ee6826d9df73d85d72ce01e57f8237d4848d3a84"
NETWORK_BUS = 1
TCP_INDEX = 0
LIVE_TITLE = "Titan Mini Live Observability"
CONTROL = """var STALE_MS = 2000;
var lastStale = -1;

function loop() {
  var r = io.getLatestFrame();
  var frame = r && r.result ? r.result : r;
  var has = !!(frame && frame.hasData);
  var ageRaw = frame && frame.ageMs;
  var age = Number(ageRaw);
  var validAge = has && ageRaw !== null && ageRaw !== undefined && ageRaw !== "" && isFinite(age) && age >= 0;
  var values = (frame && frame.values) || [];
  var deviceAgeRaw = values[29];
  var deviceAge = Number(deviceAgeRaw);
  var validDevice = deviceAgeRaw !== null && deviceAgeRaw !== undefined && deviceAgeRaw !== "" && isFinite(deviceAge) && deviceAge >= 0;
  var stale = 1;
  if (validAge && validDevice) {
    var total = age + deviceAge;
    tableSet("titan_watchdog", "last_valid_ms", Date.now() - total);
    stale = total > STALE_MS ? 1 : 0;
  }
  tableSet("titan_watchdog", "host_stale", stale);
  if (stale !== lastStale) {
    lastStale = stale;
    refreshDashboard();
  }
  delay(500);
}
"""

BANNER_TRANSFORM = """function transform(value) {
  var o = tableGet("titan_watchdog", "origin_override");
  if (String(o) === "REPLAY") return "REPLAY";
  return value;
}
"""

STATE_TRANSFORM = """function transform(value) {
  var o = tableGet("titan_watchdog", "origin_override");
  if (String(o) === "REPLAY") return "REPLAY";
  return value;
}
"""

EXTRA = [
    (28, "Acquisition origin", "origin", ""),
    (29, "Identity scope", "identity_scope", ""),
    (30, "Device age (ms)", "device_age_ms", "ms"),
    (31, "Device progress", "device_progress", ""),
    (32, "Connection epoch", "connection_epoch", ""),
    (33, "Timing valid", "timing_valid", ""),
    (34, "Rate valid", "rate_valid", ""),
    (35, "LED valid", "led_valid", ""),
    (36, "Frozen device", "frozen_device", ""),
]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_accepted() -> None:
    got = sha256(BASE)
    if got != ACCEPTED:
        raise SystemExit(f"refusing to derive live project: titan.ssproj is {got}, expected {ACCEPTED}")


def clone_dataset(template: dict, unique: int, index: int, title: str, alias: str, units: str) -> dict:
    ds = copy.deepcopy(template)
    ds["uniqueId"] = unique
    ds["datasetId"] = index
    ds["index"] = index
    ds["title"] = title
    ds["alias"] = alias
    ds["units"] = units
    ds["virtual"] = False
    ds.pop("transformCode", None)
    ds.pop("code", None)
    return ds


def derive() -> dict:
    require_accepted()
    config = json.loads(BASE.read_text())
    config["title"] = LIVE_TITLE
    config["controlScriptCode"] = CONTROL
    src = config["sources"][0]
    src["busType"] = NETWORK_BUS
    src["title"] = "Titan live broker (TCP 127.0.0.1:7778)"
    src["connection"] = {
        "address": "127.0.0.1",
        "socketTypeIndex": TCP_INDEX,
        "tcpPort": 7778,
    }
    src["frameStart"] = ""
    src["frameEnd"] = "\n"
    src["frameDetection"] = 0
    src["decoderMethod"] = 0
    src["frameParserLanguage"] = 2
    src["frameParserTemplate"] = "delimited"
    src["frameParserParams"] = {
        "separator": ",",
        "quoteChar": "",
        "skipEmpty": False,
        "trimFields": False,
    }
    tables = config.setdefault("tables", [])
    if tables:
        names = {r["name"] for r in tables[0].get("registers", [])}
        if "origin_override" not in names:
            tables[0]["registers"].append({"name": "origin_override", "type": "computed", "value": "LIVE"})
    unique = int(config.get("nextUniqueId") or 43)
    hop = next(g for g in config["groups"] if g.get("title") == "Hop config")
    template = hop["datasets"][0]
    existing = {ds.get("index") for ds in hop["datasets"]}
    for index, title, alias, units in EXTRA:
        if index in existing:
            continue
        hop["datasets"].append(clone_dataset(template, unique, index, title, alias, units))
        unique += 1
    banner = next(g for g in config["groups"] if g.get("title") == "Banner")
    used_ids = {int(ds.get("datasetId", -1)) for ds in banner["datasets"]}
    for ds in banner["datasets"]:
        if ds.get("alias") in ("banner", "operating_state"):
            ds["transformCode"] = BANNER_TRANSFORM if ds.get("alias") == "banner" else STATE_TRANSFORM
            ds["transformLanguage"] = 0
    template = banner["datasets"][0]
    display_id = 0
    while display_id in used_ids:
        display_id += 1
    display = clone_dataset(template, unique, display_id, "Display origin", "display_origin", "")
    display["virtual"] = True
    display["index"] = 0
    display["transformCode"] = BANNER_TRANSFORM
    display["transformLanguage"] = 0
    banner["datasets"].append(display)
    unique += 1
    config["nextUniqueId"] = unique
    config["actions"] = []
    LIVE.write_text(json.dumps(config, indent=2) + "\n")
    return config


def configure_network(client: SerialStudioClient) -> dict:
    def cmd(name, params=None):
        try:
            return client.command(name, params)
        except APIError as exc:
            raise RuntimeError(f"{name}: {exc.code}: {exc.message}") from exc

    buses = cmd("io.listBuses")
    types = cmd("io.network.listSocketTypes")
    cmd("io.setBusType", {"busType": NETWORK_BUS})
    cmd("io.network.setSocketType", {"socketTypeIndex": TCP_INDEX})
    cmd("io.network.setRemoteAddress", {"address": "127.0.0.1"})
    cmd("io.network.setTcpPort", {"port": 7778})
    cfg = cmd("io.network.getConfig")
    cmd(
        "project.source.update",
        {
            "sourceId": 0,
            "title": "Titan live broker (TCP 127.0.0.1:7778)",
            "busType": NETWORK_BUS,
            "frameStart": "",
            "frameEnd": "\n",
            "frameDetection": 0,
            "decoderMethod": 0,
        },
    )
    return {"buses": buses, "socketTypes": types, "config": cfg}


def verify_live_source(io: dict, net: dict) -> tuple[bool, str]:
    slug = str(io.get("busTypeSlug") or "")
    bus = io.get("busType")
    if bus not in (None, NETWORK_BUS) and slug not in ("", "network"):
        return False, f"active bus is {slug or bus}, not Network"
    cfg = net or {}
    addr = str(cfg.get("address") or cfg.get("remoteAddress") or "")
    port = int(cfg.get("tcpPort") or cfg.get("port") or 0)
    sock = cfg.get("socketTypeIndex")
    if addr not in ("127.0.0.1", "localhost"):
        return False, f"address {addr!r} is not loopback"
    if port != 7778:
        return False, f"tcpPort {port} is not 7778"
    if sock not in (None, 0, "TCP", "Tcp"):
        return False, f"socketType {sock!r} is not TCP"
    return True, "network tcp 7778"


def main() -> int:
    config = derive()
    print(json.dumps({"path": str(LIVE), "sha256": sha256(LIVE), "title": config["title"], "busType": config["sources"][0]["busType"]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
