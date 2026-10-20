#!/usr/bin/env python3
"""Build the Titan Mini Serial Studio project. Fail closed. Never steal UART."""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

SS = Path("/Users/spectrasynq/Serial-Studio")
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(SS / "tests" / "utils"))
sys.path.insert(0, str(HERE))

from api_client import APIError, SerialStudioClient  # noqa: E402
from source_ownership import (  # noqa: E402
    TITAN_TITLE,
    process_source_ok,
    should_leave_session_alone,
)

PARSER = (HERE / "titan_parser.js").read_text()
SAVE_PATH = HERE / "titan.ssproj"
REPLAY = HERE / "titan_fixture_replay.py"
STALE_MS = 2000
CONTROL = """var STALE_MS = 2000;

function loop() {
  var r = io.getLatestFrame();
  var frame = r && r.result ? r.result : r;
  var has = !!(frame && frame.hasData);
  var ageRaw = frame && frame.ageMs;
  var age = Number(ageRaw);
  var validAge = has && ageRaw !== null && ageRaw !== undefined && ageRaw !== "" && isFinite(age) && age >= 0;
  if (validAge) {
    tableSet("titan_watchdog", "last_valid_ms", Date.now() - age);
    tableSet("titan_watchdog", "host_stale", age > STALE_MS ? 1 : 0);
  } else {
    tableSet("titan_watchdog", "host_stale", 1);
  }
  refreshDashboard();
  delay(200);
}
"""

GOOD = [
    {"min": 0.5, "max": 2, "severity": 1, "label": "YES", "blink": False, "color": "#3DDC97"},
    {"min": 0, "max": 0.5, "severity": 3, "label": "NO", "blink": True, "color": "#FF5A5A"},
]
BAD = [
    {"min": 0.5, "max": 1e9, "severity": 3, "label": "ALARM", "blink": True, "color": "#FF5A5A"},
    {"min": 0, "max": 0.5, "severity": 1, "label": "OK", "blink": False, "color": "#3DDC97"},
]
MIC_BANDS = [
    {"min": -90, "max": -6, "severity": 1, "label": "ok", "blink": False, "color": "#3DDC97"},
    {"min": -6, "max": -0.5, "severity": 2, "label": "hot", "blink": False, "color": "#F5C542"},
    {"min": -0.5, "max": 3, "severity": 3, "label": "clip", "blink": True, "color": "#FF5A5A"},
]


class SetupError(RuntimeError):
    pass


def require(client: SerialStudioClient, name: str, params=None):
    try:
        return client.command(name, params)
    except APIError as exc:
        raise SetupError(f"{name}: {exc.code}: {exc.message}") from exc
    except TimeoutError as exc:
        raise SetupError(f"{name}: timeout {exc}") from exc


def sources_of(client: SerialStudioClient) -> list:
    listed = require(client, "project.source.list")
    return listed.get("sources") or []


def parser_loaded(client: SerialStudioClient) -> bool:
    got = require(client, "project.frameParser.getCode")
    code = got.get("code") or ""
    lang = got.get("language")
    return lang == 0 and "UNAVAILABLE" in code and "SIMULATED DATA" in code


def install_parser(client: SerialStudioClient, force: bool = False) -> None:
    require(client, "dashboard.setOperationMode", {"mode": 0})
    require(
        client,
        "project.frameParser.update",
        {
            "endSequence": "\n",
            "checksumAlgorithm": "",
            "operationMode": 0,
            "frameDetection": 0,
            "decoderMethod": 0,
        },
    )
    client.source_update(0, decoderMethod=0, frameStart="", frameEnd="\n", frameDetection=0)
    require(
        client,
        "project.frameParser.setTemplate",
        {
            "template": "delimited",
            "params": {"separator": ",", "quoteChar": "", "skipEmpty": False, "trimFields": False},
            "sourceId": 0,
        },
    )
    _ = force


def guard_session(client: SerialStudioClient) -> tuple:
    io = require(client, "io.getStatus")
    project = require(client, "project.getStatus")
    srcs = sources_of(client)
    proc = require(client, "io.process.getConfig")
    blocked = should_leave_session_alone(io, project, srcs, REPLAY, proc)
    if blocked:
        raise SetupError(blocked)
    return io, project, srcs, proc


def configure_process_io(client: SerialStudioClient, arguments: str | None = None) -> None:
    args = arguments or "--cycle --hold 4 --period 0.2"
    require(client, "io.setBusType", {"busType": 8})
    client.source_update(
        0,
        title="Titan host snapshot (Process I/O)",
        busType=8,
        frameStart="",
        frameEnd="\n",
        frameDetection=0,
        decoderMethod=0,
        checksumAlgorithm="",
    )
    require(client, "io.process.setExecutable", {"executable": str(REPLAY)})
    require(client, "io.process.setArguments", {"arguments": args})
    require(client, "io.process.setWorkingDir", {"workingDir": str(HERE)})
    require(client, "io.process.setMode", {"mode": 0})
    client.source_configure(
        0,
        {
            "executable": str(REPLAY),
            "arguments": args,
            "workingDir": str(HERE),
            "mode": 0,
        },
    )
    ok, reason = process_source_ok(
        require(client, "io.getStatus"),
        sources_of(client),
        REPLAY,
        require(client, "io.process.getConfig"),
    )
    if not ok:
        raise SetupError(f"Process I/O not configured: {reason}")


def add_dataset(client, gid, dataset_id, slot, title, alias, units="", **extra):
    patch = {
        "groupId": gid,
        "datasetId": dataset_id,
        "title": title,
        "alias": alias,
        "index": slot,
        "sourceId": 0,
        "graph": extra.get("graph", False),
    }
    if units:
        patch["units"] = units
    for key in ("wgtMin", "wgtMax", "pltMin", "pltMax", "ledHigh", "led", "widget"):
        if key in extra:
            patch[key] = extra[key]
    try:
        require(client, "project.dataset.update", patch)
    except SetupError as exc:
        if "timeout" not in str(exc).lower():
            raise
    options = extra.get("options")
    if options is not None:
        bits = options
        if isinstance(options, list):
            lookup = {"plot": 1, "fft": 2, "bar": 4, "gauge": 8, "compass": 16, "led": 32, "waterfall": 64}
            bits = 0
            for item in options:
                bits |= lookup[item]
        require(
            client,
            "project.dataset.setOptions",
            {"groupId": gid, "datasetId": dataset_id, "options": bits},
        )
    if extra.get("alarmBands"):
        require(
            client,
            "project.dataset.setAlarmBands",
            {"groupId": gid, "datasetId": dataset_id, "alarmBands": extra["alarmBands"]},
        )
    if extra.get("virtual"):
        require(client, "project.dataset.setVirtual", {"groupId": gid, "datasetId": dataset_id, "virtual": True})
    if extra.get("transform"):
        require(
            client,
            "project.dataset.setTransformCode",
            {
                "groupId": gid,
                "datasetId": dataset_id,
                "code": extra["transform"],
                "language": 0,
            },
        )


def pin(client, ws_by_title, unique, workspace, group, widget):
    wid = ws_by_title.get(workspace)
    gid = unique.get(group)
    if wid is None or gid is None:
        raise SetupError(f"cannot pin {widget} {group} onto {workspace}")
    require(
        client,
        "project.workspace.addWidget",
        {"workspaceId": wid, "groupId": gid, "widgetType": widget},
    )


def build_project(client: SerialStudioClient) -> None:
    require(client, "project.new")
    time.sleep(0.3)
    require(client, "project.setTitle", {"title": TITAN_TITLE})
    require(client, "dashboard.setFps", {"fps": 30})
    install_parser(client)
    require(client, "project.dataTable.add", {"name": "titan_watchdog"})
    require(client, "project.dataTable.addRegister", {"table": "titan_watchdog", "name": "last_valid_ms", "computed": True, "defaultValue": 0})
    require(client, "project.dataTable.addRegister", {"table": "titan_watchdog", "name": "host_stale", "computed": True, "defaultValue": 1})
    require(client, "controlScript.set", {"code": CONTROL})
    configure_process_io(client)

    spec = [
        ("Banner", 0, [
            (1, "SIMULATED / LIVE banner", "banner", ""),
            (2, "Operating state", "operating_state", ""),
            (3, "Test result", "test_result", ""),
        ]),
        ("Identity", 0, [
            (4, "UID", "uid", ""),
            (5, "Build", "build", ""),
            (6, "Source pin", "source", ""),
            (7, "Contract", "contract", ""),
        ]),
        ("Lane A", 10, [(9, "Lane A RMS", "lane_a_rms", "dBFS")]),
        ("Lane B", 10, [(10, "Lane B RMS", "lane_b_rms", "dBFS")]),
        ("Mic validity", 0, [
            (11, "Lane A live", "lane_a_valid", ""),
            (12, "Lane B live", "lane_b_valid", ""),
        ]),
        ("Capture clock", 4, [(13, "Measured capture rate", "capture_rate_hz", "Hz")]),
        # hop_max_us is AP hop compute. Palette last_emit_cycles must not fill it.
        ("Hop timing", 4, [(14, "Worst hop compute", "hop_max_us", "us")]),
        ("Fault counts", 4, [
            (15, "Late starts", "late_starts", ""),
            (16, "Deadlines", "deadlines", ""),
            (17, "CRC mismatches", "crc_mismatches", ""),
        ]),
        ("LED health", 0, [
            (18, "LED health", "led_health", ""),
            (19, "DMA IRQs", "dma_irqs", ""),
            (20, "Latched frames", "latched_frames", ""),
            (21, "LED faults", "led_faults", ""),
        ]),
        ("Flags", 0, [
            (8, "Identified", "identity_ok", ""),
            (25, "Health valid", "health_valid", ""),
            (26, "Simulated", "simulated", ""),
        ]),
        ("Hop config", 0, [
            (22, "Hop samples", "hop_samples", ""),
            (23, "Admitted rate", "admitted_rate_hz", "Hz"),
            (24, "Sequence", "sequence", ""),
            (27, "Mic mapping", "audio_mapping", ""),
        ]),
        ("Watchdog", 0, []),
    ]

    for title, widget, _fields in spec:
        require(client, "project.group.add", {"title": title, "widgetType": widget})
    groups = {g["title"]: g for g in require(client, "project.group.list").get("groups", [])}

    def gid(title):
        return groups[title]["groupId"]

    for title, widget, fields in spec:
        if not fields:
            continue
        require(
            client,
            "project.dataset.addMany",
            {"groupId": gid(title), "count": len(fields), "options": 0, "titlePattern": "{n}", "startNumber": 1, "startIndex": 1},
        )
        for i, field in enumerate(fields):
            slot, label, alias, units = field
            extra = {}
            if title in ("Capture clock", "Hop timing", "Fault counts"):
                extra.update(graph=True, options=["plot"])
            if title in ("Lane A", "Lane B"):
                extra.update(graph=True, options=["bar"], widget="bar", wgtMin=-90, wgtMax=0, pltMin=-90, pltMax=0, alarmBands=MIC_BANDS)
            if title == "Mic validity":
                extra.update(options=["led"], led=True, ledHigh=1, alarmBands=GOOD)
            if alias == "identity_ok":
                extra.update(options=["led"], led=True, ledHigh=1, alarmBands=GOOD)
            if alias == "health_valid":
                extra.update(options=["led"], led=True, ledHigh=1, alarmBands=GOOD)
            if alias == "simulated":
                extra.update(options=["led"], led=True, ledHigh=1, alarmBands=BAD)
            add_dataset(client, gid(title), i, slot, label, alias, units, **extra)

    require(client, "project.dataset.add", {"groupId": gid("Banner"), "title": "Packet freshness", "options": 0})
    add_dataset(
        client,
        gid("Banner"),
        3,
        0,
        "Packet freshness",
        "packet_freshness",
        virtual=True,
        transform=(
            "function transform(value) {\n"
            '  return Number(tableGet("titan_watchdog", "host_stale"))'
            ' ? "STALE — NO VALID PACKET" : "FRESH";\n'
            "}\n"
        ),
    )

    wgid = gid("Watchdog")
    require(client, "project.dataset.add", {"groupId": wgid, "title": "Host stale", "options": 32})
    add_dataset(
        client,
        wgid,
        0,
        0,
        "Host stale",
        "host_stale",
        options=["led"],
        led=True,
        ledHigh=1,
        alarmBands=BAD,
        virtual=True,
        transform=(
            "function transform(value) {\n"
            '  return Number(tableGet("titan_watchdog", "host_stale"));\n'
            "}\n"
        ),
    )
    require(client, "project.dataset.add", {"groupId": wgid, "title": "Last valid (ms)", "options": 0})
    add_dataset(
        client,
        wgid,
        1,
        0,
        "Last valid (ms)",
        "last_valid_ms",
        virtual=True,
        transform=(
            "function transform(value) {\n"
            '  return Number(tableGet("titan_watchdog", "last_valid_ms"));\n'
            "}\n"
        ),
    )

    require(client, "project.workspace.setCustomizeMode", {"enabled": True})
    require(client, "project.workspace.clearAll")
    workspaces = [
        ("Overview", "Image, source mode, mics, clock, timing, LEDs, named test result"),
        ("Audio", "Lane A/B levels and measured capture rate"),
        ("Timing", "Hop compute separate from fault counts"),
        ("LEDs", "LED health and counters"),
        ("Engineering", "Sequence, hop contract, mapping"),
    ]
    for title, desc in workspaces:
        added = require(client, "project.workspace.add", {"title": title})
        require(client, "project.workspace.update", {"id": added["id"], "title": title, "description": desc})
    listed = require(client, "project.workspace.list").get("workspaces", [])
    ws_by_title = {w["title"]: w["id"] for w in listed}
    unique = {g["title"]: g.get("uniqueId", g["groupId"]) for g in require(client, "project.group.list").get("groups", [])}
    pins = [
        ("Overview", "Banner", "datagrid"),
        ("Overview", "Identity", "datagrid"),
        ("Overview", "Lane A", "barpanel"),
        ("Overview", "Lane B", "barpanel"),
        ("Overview", "Mic validity", "led"),
        ("Overview", "Capture clock", "multiplot"),
        ("Overview", "Hop timing", "multiplot"),
        ("Overview", "LED health", "datagrid"),
        ("Overview", "Watchdog", "datagrid"),
        ("Audio", "Lane A", "barpanel"),
        ("Audio", "Lane B", "barpanel"),
        ("Audio", "Mic validity", "led"),
        ("Audio", "Capture clock", "multiplot"),
        ("Timing", "Hop timing", "multiplot"),
        ("Timing", "Fault counts", "multiplot"),
        ("LEDs", "LED health", "datagrid"),
        ("LEDs", "Flags", "led"),
        ("Engineering", "Hop config", "datagrid"),
        ("Engineering", "Identity", "datagrid"),
        ("Engineering", "Banner", "datagrid"),
    ]
    for item in pins:
        pin(client, ws_by_title, unique, *item)
    install_parser(client)


def export_project(client: SerialStudioClient) -> dict:
    exported = require(client, "project.exportJson")
    config = exported.get("config") if isinstance(exported, dict) else None
    if not isinstance(config, dict) or not config.get("groups"):
        raise SetupError("exportJson did not contain groups")
    if config.get("sources"):
        config["sources"][0]["frameStart"] = ""
        config["sources"][0]["startSequences"] = []
        config["sources"][0]["decoderMethod"] = 0
        config["sources"][0]["frameParserLanguage"] = 2
        config["sources"][0]["frameParserTemplate"] = "delimited"
        config["sources"][0]["frameParserParams"] = {
            "separator": ",",
            "quoteChar": "",
            "skipEmpty": False,
            "trimFields": False,
        }
    config["controlScriptCode"] = CONTROL
    SAVE_PATH.write_text(json.dumps(config, indent=2) + "\n")
    return config


def canonical_config(config: dict) -> dict:
    src = (config.get("sources") or [{}])[0]
    transforms = []
    for group in config.get("groups") or []:
        for ds in group.get("datasets") or []:
            code = ds.get("transformCode") or ds.get("code") or ""
            if code:
                transforms.append({"title": ds.get("title"), "virtual": ds.get("virtual"), "code": code})
    return {
        "title": config.get("title"),
        "groups": [g.get("title") for g in config.get("groups") or []],
        "workspaces": [w.get("title") for w in config.get("workspaces") or []],
        "busType": src.get("busType"),
        "frameStart": src.get("frameStart"),
        "frameEnd": src.get("frameEnd"),
        "decoderMethod": src.get("decoderMethod"),
        "language": src.get("frameParserLanguage"),
        "template": src.get("frameParserTemplate"),
        "separator": (src.get("frameParserParams") or {}).get("separator"),
        "control": "ageMs" in (config.get("controlScriptCode") or "")
        and "refreshDashboard" in (config.get("controlScriptCode") or "")
        and 'tableSet("titan_watchdog", "host_stale", 1)' in (config.get("controlScriptCode") or "")
        and "validAge" in (config.get("controlScriptCode") or ""),
        "transforms": transforms,
    }


def wait_connected(client: SerialStudioClient, seconds: float = 8.0) -> dict:
    deadline = time.monotonic() + seconds
    last = {}
    while time.monotonic() < deadline:
        last = require(client, "io.getStatus")
        if last.get("isConnected") and last.get("busTypeSlug") == "process":
            return last
        time.sleep(0.25)
    raise SetupError(f"Process I/O did not connect: {last}")


def connect_process(client: SerialStudioClient) -> dict:
    io = require(client, "io.getStatus")
    if io.get("isConnected") and io.get("busTypeSlug") == "process":
        return io
    try:
        require(client, "io.connect")
    except SetupError:
        pass
    return wait_connected(client)


def connect_if_ours(client: SerialStudioClient) -> dict:
    configure_process_io(client)
    install_parser(client)
    require(client, "dashboard.setOperationMode", {"mode": 0})
    return connect_process(client)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rebuild", action="store_true")
    parser.add_argument("--connect", action="store_true", default=True)
    parser.add_argument("--no-connect", action="store_false", dest="connect")
    args = parser.parse_args()
    REPLAY.chmod(REPLAY.stat().st_mode | 0o111)
    client = SerialStudioClient(timeout=60.0)
    client.connect()
    try:
        io, project, srcs, proc = guard_session(client)

        ours = process_source_ok(io, srcs, REPLAY, proc)[0] and project.get("title") == TITAN_TITLE
        if args.rebuild or not ours or int(project.get("groupCount") or 0) < 8:
            if io.get("isConnected"):
                if not process_source_ok(io, srcs, REPLAY, proc)[0]:
                    raise SetupError("refusing to disconnect a non-Process-I/O session")
                require(client, "io.disconnect")
                time.sleep(0.3)
            build_project(client)
        else:
            configure_process_io(client)
            install_parser(client)
        config = export_project(client)
        verification = {
            "project_file": str(SAVE_PATH),
            "project_sha256": hashlib.sha256(SAVE_PATH.read_bytes()).hexdigest(),
            "group_titles": [g.get("title") for g in config.get("groups", [])],
            "workspace_titles": [w.get("title") for w in config.get("workspaces", [])],
        }
        if args.connect:
            io = connect_if_ours(client)
            time.sleep(1.5)
            data = require(client, "dashboard.getData")
            banner = ""
            for group in (data.get("frame") or {}).get("groups") or []:
                for ds in group.get("datasets") or []:
                    if ds.get("alias") == "banner" or ds.get("title") == "SIMULATED / LIVE banner":
                        banner = str(ds.get("value") or "")
            if "SIMULATED DATA" not in banner:
                if io.get("isConnected"):
                    require(client, "io.disconnect")
                    time.sleep(0.3)
                require(client, "project.open", {"filePath": str(SAVE_PATH)})
                time.sleep(0.8)
                install_parser(client, force=True)
                io = connect_if_ours(client)
                time.sleep(2.0)
                data = require(client, "dashboard.getData")
            verification["io"] = io
            verification["dashboard_preview"] = json.dumps(data, default=str)[:5000]
        (HERE / "ss-02-verification.json").write_text(json.dumps(verification, indent=2) + "\n")
        print(json.dumps({k: verification[k] for k in verification if k != "dashboard_preview"}, indent=2))
        return 0
    finally:
        client.disconnect()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SetupError as exc:
        print("SETUP_FAILED", exc, file=sys.stderr)
        raise SystemExit(1)
