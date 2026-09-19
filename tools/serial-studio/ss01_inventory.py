#!/usr/bin/env python3
"""SS-01: record the installed Serial Studio Pro binary and Titan CDC protocol.

Read-only against Serial Studio (port 7777). Optional exclusive opcode-1 identity
query against Titan application CDC, then the port is closed. Does not flash,
does not start a runner, and does not leave Serial Studio holding CDC.
"""
from __future__ import annotations

import hashlib
import json
import os
import platform
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
FW = HERE.parents[1]
SS = Path("/Users/spectrasynq/Serial-Studio")
sys.path.insert(0, str(SS / "tests" / "utils"))
sys.path.insert(0, str(FW / "scripts"))

from api_client import APIError, SerialStudioClient  # noqa: E402

APP = Path("/Applications/Serial Studio Pro.app")
BIN = APP / "Contents/MacOS/Serial-Studio-Pro"
OUT = HERE / "ss-01-capability-record.json"
CATALOG = HERE / "ss-01-api-catalog.json"
TITAN_VIDPID = (0x045B, 0x5310)


def plist(key: str) -> str:
    r = subprocess.run(
        ["defaults", "read", str(APP / "Contents/Info"), key],
        capture_output=True,
        text=True,
        check=False,
    )
    return r.stdout.strip() if r.returncode == 0 else ""


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def port_open(port: int) -> bool:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.4)
    try:
        s.connect(("127.0.0.1", port))
        return True
    except OSError:
        return False
    finally:
        s.close()


def try_cmd(c: SerialStudioClient, name: str, params=None):
    try:
        return {"ok": True, "result": c.command(name, params)}
    except APIError as e:
        return {"ok": False, "code": e.code, "message": e.message}
    except Exception as e:
        return {"ok": False, "code": type(e).__name__, "message": str(e)}


def titan_ports():
    from serial.tools import list_ports

    rows = []
    for p in list_ports.comports():
        if (p.vid, p.pid) != TITAN_VIDPID:
            continue
        owners = subprocess.run(
            ["lsof", "-nP", "-t", p.device, p.device.replace("/cu.", "/tty.")],
            capture_output=True,
            text=True,
        )
        rows.append(
            {
                "device": p.device,
                "serial_number": p.serial_number,
                "location": p.location,
                "manufacturer": p.manufacturer,
                "product": p.product,
                "owners": owners.stdout.strip().split() if owners.stdout.strip() else [],
            }
        )
    return rows


def opcode1_identity(device: str) -> dict:
    import serial
    import struct
    import zlib
    from run_led_smoke import packet, read_exact

    port = serial.Serial(device, 115200, timeout=0.5, write_timeout=2, exclusive=True)
    try:
        req = 1
        outgoing = packet(1, req)
        if port.write(outgoing) != len(outgoing):
            return {"ok": False, "error": "short_write"}
        port.flush()
        header = read_exact(port, 32, 15)
        magic, status, rid, sequence, size, cycles, crc, hcrc = struct.unpack(
            "<4s7I", header
        )
        if magic != b"K1R1" or zlib.crc32(header[:28]) != hcrc or rid != req or size > 19968:
            return {
                "ok": False,
                "error": "header_invalid",
                "magic": magic.decode("latin1", "replace"),
                "status": status,
                "size": size,
            }
        body = read_exact(port, size, 15)
        if zlib.crc32(body) != crc:
            return {"ok": False, "error": "body_crc"}
        if status:
            return {"ok": False, "error": "rejected", "status": status, "body": body.decode("utf-8", "replace")}
        info = json.loads(body)
        return {"ok": True, "info": info, "cycles": cycles, "sequence": sequence}
    finally:
        port.close()


def main() -> int:
    record = {
        "ticket": "SS-01",
        "captured_at": datetime.now(timezone.utc).isoformat(),
        "label": "HOST-AND-OPTIONAL-IDENTITY",
        "resident_image_claimed": False,
        "macos": {
            "product": platform.mac_ver()[0],
            "release": os.uname().release,
            "machine": platform.machine(),
            "sw_vers": subprocess.check_output(["sw_vers"], text=True),
        },
        "serial_studio_pro": {
            "bundle": str(APP),
            "binary": str(BIN),
            "cfbundle_short_version": plist("CFBundleShortVersionString"),
            "cfbundle_version": plist("CFBundleVersion"),
            "cfbundle_id": plist("CFBundleIdentifier"),
            "binary_sha256": sha256(BIN) if BIN.exists() else None,
            "arch": subprocess.check_output(["lipo", "-archs", str(BIN)], text=True).strip(),
            "help_flags_present": {
                "headless": True,
                "api_server": True,
                "project": True,
                "csv_export": "--csv-export" in subprocess.check_output([str(BIN), "--help"], text=True, stderr=subprocess.STDOUT),
                "session_export": "--session-export" in subprocess.check_output([str(BIN), "--help"], text=True, stderr=subprocess.STDOUT),
            },
        },
        "listeners": {
            "api_7777": port_open(7777),
            "broker_7778": port_open(7778),
            "grpc_8888": port_open(8888),
        },
        "docs_example_client": {
            "serial_studio_checkout": str(SS),
            "mcp_client_dir": str(SS / "examples" / "MCP Client"),
            "mcp_client_present": (SS / "examples" / "MCP Client" / "client.py").is_file(),
            "api_client": str(SS / "tests" / "utils" / "api_client.py"),
        },
        "titan_protocol_from_source": {
            "firmware_tree": str(FW),
            "branch": subprocess.check_output(
                ["git", "-C", str(FW), "branch", "--show-current"], text=True
            ).strip(),
            "head": subprocess.check_output(
                ["git", "-C", str(FW), "rev-parse", "HEAD"], text=True
            ).strip(),
            "fixture_app": "platform/ra8p1/fixture_app.cpp",
            "request_magic": "K1S1",
            "response_magic": "K1R1",
            "header_bytes": 32,
            "application_usb": "045b:5310",
            "baud": 115200,
            "opcode_1_info_json_keys": [
                "protocol",
                "uid",
                "build",
                "source",
                "contract",
                "clock_hz",
                "cpu1_actcsr",
                "u55_opened",
                "cpp_initialised",
                "rejected",
                "sequence",
                "trajectory_bytes",
                "trace_bytes",
                "status_led",
            ],
            "contract": "sr24000.hop180.bins80.xover40",
            "opcodes": {
                "1": "INFO JSON identity (always)",
                "2": "180-sample hop, text trace (360 bytes)",
                "3": "RESET trajectory epoch",
                "4": "time probe",
                "5": "180-sample hop, binary trace (360 bytes)",
                "6": "platform metrics JSON",
                "7": "start resident schedule (compile-gated)",
                "8": "schedule status (compile-gated)",
                "9": "P4 start (K1_P4_LOAD)",
                "10": "P4 status (K1_P4_LOAD)",
                "11": "WS2816 packed-lane smoke; rejected with status 10 when K1_PALETTE_GPT_DMA",
                "12": "stage-probe raw chunk (K1_ENABLE_STAGE_PROBE)",
                "13": "WS281X diagnostic pack/emit; rejected when GPT DMA owns P601",
                "14": "WS281X submitted frame; rejected when GPT DMA owns P601",
                "15": "palette catalogue if K1_PALETTE_RUNTIME, else PCM1808 metrics if K1_PCM1808_TARGET",
                "16": "palette configure (K1_PALETTE_RUNTIME)",
                "17": "palette status",
                "18": "palette frame bytes",
                "19": "palette wire snapshot (WS2816 compile)",
                "20": "status LED / LED2 PHY subcommands",
                "22": "GPT/DMA diagnostic snapshot (K1_PALETTE_GPT_DMA)",
            },
            "do_not": [
                "Send ASCII/JSON into CDC; host requests are K1S1 binary.",
                "Treat STATUS.md as the resident image.",
                "FFT 5 Hz health fields and call that an audio spectrum.",
                "Let Serial Studio and a runner share 045b:5310.",
            ],
        },
    }

    help_text = subprocess.check_output([str(BIN), "--help"], text=True, stderr=subprocess.STDOUT)
    record["serial_studio_pro"]["cli_help"] = help_text
    record["titan_cdc"] = titan_ports()

    c = SerialStudioClient(timeout=8.0)
    c.connect()
    catalog = try_cmd(c, "api.getCommands")
    record["api"] = {"getCommands": {"ok": catalog["ok"]}}
    names = []
    if catalog["ok"]:
        commands = catalog["result"].get("commands", [])
        CATALOG.write_text(json.dumps(catalog["result"], indent=2) + "\n")
        names = [row.get("name") for row in commands if isinstance(row, dict)]
        record["api"]["command_count"] = len(names)
        record["api"]["command_names"] = names
        prefixes = {}
        for n in names:
            prefixes[n.split(".")[0]] = prefixes.get(n.split(".")[0], 0) + 1
        record["api"]["modules"] = prefixes
        record["api"]["pro_markers"] = {
            "sessions": any(n.startswith("sessions.") for n in names),
            "process_io": any(n.startswith("io.process.") for n in names),
            "plot3d": "project.workspace.addWidget" in names,
            "grpc": any("grpc" in n.lower() for n in names),
            "mcp": any("mcp" in n.lower() for n in names),
            "webview": any("web" in n.lower() for n in names),
            "reports": any("report" in n.lower() for n in names),
            "csv": any(n.startswith("csv") for n in names),
        }
    probes = [
        "io.getStatus",
        "project.getStatus",
        "dashboard.getStatus",
        "dashboard.getOperationMode",
        "project.source.list",
        "project.workspace.list",
        "project.group.list",
        "io.uart.listPorts",
        "io.process.getConfig",
        "csvExport.getStatus",
        "app.getVersion",
        "app.getAbout",
        "licensing.getStatus",
        "license.getStatus",
        "lemonSqueezy.getStatus",
        "commercial.getStatus",
        "sessions.getStatus",
        "session.getStatus",
        "reports.getStatus",
        "grpc.getStatus",
        "mcp.getStatus",
    ]
    record["readonly_probes"] = {}
    for name in probes:
        if names and name not in names and not name.startswith("licensing"):
            if name not in names:
                record["readonly_probes"][name] = {"ok": False, "code": "ABSENT"}
                continue
        record["readonly_probes"][name] = try_cmd(c, name)

    widget_cmds = [n for n in names if "widget" in n.lower() or n.startswith("project.workspace")]
    record["api"]["workspace_and_widget_commands"] = widget_cmds
    c.disconnect()

    identity = {"attempted": False}
    free = [p for p in record["titan_cdc"] if not p["owners"]]
    if len(record["titan_cdc"]) == 1 and free:
        identity["attempted"] = True
        try:
            identity.update(opcode1_identity(free[0]["device"]))
            if identity.get("ok"):
                record["resident_image_claimed"] = True
                record["resident_identity_source"] = "opcode_1_exclusive_cdc"
        except Exception as e:
            identity = {"attempted": True, "ok": False, "error": str(e)}
    elif record["titan_cdc"] and not free:
        identity = {
            "attempted": False,
            "ok": False,
            "error": "cdc_has_owner",
            "owners": record["titan_cdc"][0]["owners"],
        }
    else:
        identity = {"attempted": False, "ok": False, "error": "titan_cdc_not_unique_or_absent"}
    record["opcode1_identity"] = identity

    OUT.write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({
        "wrote": str(OUT),
        "app": record["serial_studio_pro"]["cfbundle_short_version"],
        "api_commands": record["api"].get("command_count"),
        "titan_cdc": record["titan_cdc"],
        "identity_ok": identity.get("ok"),
        "build": (identity.get("info") or {}).get("build"),
        "uid": (identity.get("info") or {}).get("uid"),
        "listeners": record["listeners"],
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
