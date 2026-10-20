#!/usr/bin/env python3
"""M9 — Host workflow CLI: record, inspect, replay, control change, readback, save/load.

Never opens the real CDC broker. Never writes Titan NVM. Host/mock only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from mir_config_store import HostConfigStore  # noqa: E402
from mir_evidence_kinds import (  # noqa: E402
    EVIDENCE_DIGITAL_FIXTURE,
    EVIDENCE_OBSERVATION_REPLAY,
    EVIDENCE_PCM_RECOMPUTE,
    label_payload,
)
from mir_ingress_adapters import HostIngressAdapter, IngressFrame, SourceEpoch  # noqa: E402
from mir_mock_controls import HostControlClient, MockControlEndpoint  # noqa: E402
from mir_observation_recorder import BoundedObservationRecorder  # noqa: E402
from mir_peer_hmi import MockPeerHmi, PeerMessage  # noqa: E402

SWARM_BASE_ID = "cb33aab6c8fe327d782f8ad31c3966d00e8a7c1cd71e140dd03381e28eb8729a"
REAL_PCM_FIXTURE = "BLOCKED"


def cmd_record(args: argparse.Namespace) -> int:
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    rec = BoundedObservationRecorder(
        out / "observations.jsonl",
        max_items=args.max_items,
        max_bytes=args.max_bytes,
    )
    try:
        for i in range(args.count):
            rec.put(
                label_payload(
                    EVIDENCE_OBSERVATION_REPLAY if args.as_replay else EVIDENCE_DIGITAL_FIXTURE,
                    {"seq": i, "note": "host_mock_record"},
                )
            )
            if args.gap_every and i > 0 and i % args.gap_every == 0:
                rec.mark_gap(reason="injected_gap", after_seq=i)
            time.sleep(args.interval)
    finally:
        rec.close()
    stats = rec.stats()
    (out / "RECORD_STATS.json").write_text(json.dumps(stats, indent=2) + "\n")
    print("RECORD_OK", stats["path"], "gaps", stats["gaps"], "failed", stats["failed"])
    return 0 if not stats["failed"] else 1


def cmd_inspect(args: argparse.Namespace) -> int:
    path = Path(args.path)
    kinds: dict[str, int] = {}
    gaps = 0
    rows = 0
    with path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            row = json.loads(line)
            rows += 1
            kind = row.get("kind", "unknown")
            kinds[kind] = kinds.get(kind, 0) + 1
            if kind == "gap":
                gaps += 1
            ek = row.get("evidence_kind")
            if ek:
                kinds[f"evidence:{ek}"] = kinds.get(f"evidence:{ek}", 0) + 1
    report = {"path": str(path), "rows": rows, "gaps": gaps, "kinds": kinds}
    if args.output:
        Path(args.output).write_text(json.dumps(report, indent=2) + "\n")
    print("INSPECT_OK", json.dumps(report))
    return 0


def cmd_replay(args: argparse.Namespace) -> int:
    """Observation replay OR PCM recompute — never labelled as the same evidence."""
    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    if args.mode == "observations":
        src = Path(args.input)
        rows = []
        with src.open(encoding="utf-8") as fh:
            for line in fh:
                if not line.strip():
                    continue
                row = json.loads(line)
                if row.get("kind") == "gap":
                    rows.append(row)
                    continue
                if row.get("evidence_kind"):
                    # Preserve original evidence_kind; never silently relabel.
                    rows.append(row)
                else:
                    rows.append(label_payload(EVIDENCE_OBSERVATION_REPLAY, row))
        payload = label_payload(
            EVIDENCE_OBSERVATION_REPLAY,
            {
                "source": str(src),
                "rows": len(rows),
                "device_mutated": False,
                "REAL_PCM_FIXTURE": REAL_PCM_FIXTURE,
                "note": "Session is observation playback; per-row evidence_kind is preserved when already set",
            },
        )
        (out / "OBSERVATION_REPLAY.json").write_text(json.dumps(payload, indent=2) + "\n")
        print("REPLAY_OK mode=observations rows", len(rows), "evidence", EVIDENCE_OBSERVATION_REPLAY)
        return 0

    # PCM recompute path: hash-only stub when REAL_PCM blocked; still distinct label.
    pcm = Path(args.input)
    digest = hashlib.sha256(pcm.read_bytes()).hexdigest()
    payload = label_payload(
        EVIDENCE_PCM_RECOMPUTE if not args.digital else EVIDENCE_DIGITAL_FIXTURE,
        {
            "pcm": str(pcm),
            "pcm_sha256": digest,
            "device_mutated": False,
            "call_path": "declared_host_recompute_or_digital_fixture",
            "REAL_PCM_FIXTURE": REAL_PCM_FIXTURE,
            "note": "Host workflow labels only; full LiveAudioRuntime recompute remains scripts/replay_live_k1.py",
        },
    )
    (out / "PCM_RECOMPUTE.json").write_text(json.dumps(payload, indent=2) + "\n")
    print("REPLAY_OK mode=pcm evidence", payload["evidence_kind"], digest[:12])
    return 0


def cmd_control(args: argparse.Namespace) -> int:
    endpoint = MockControlEndpoint()
    client = HostControlClient(endpoint)
    if args.disconnect:
        endpoint.disconnect()
        try:
            client.change({"mode_a": args.mode_a})
        except Exception as exc:
            print("CONTROL_DISCONNECT_OK", type(exc).__name__, getattr(exc, "code", ""))
            return 0
        print("CONTROL_FAIL expected disconnect")
        return 1
    before = endpoint.readback()
    after = client.change({"mode_a": args.mode_a, "palette_a": args.palette_a})
    print(
        "CONTROL_OK rev",
        before["revision"],
        "->",
        after["revision"],
        "mode_a",
        after["mode_a"],
        "emit",
        after["emit_on"],
    )
    return 0


def cmd_readback(args: argparse.Namespace) -> int:
    endpoint = MockControlEndpoint()
    if args.seed:
        seed = json.loads(Path(args.seed).read_text())
        endpoint.apply(seed, expected_revision=endpoint.readback()["revision"])
    cfg = endpoint.readback()
    if args.output:
        Path(args.output).write_text(json.dumps(cfg, indent=2) + "\n")
    print("READBACK_OK rev", cfg["revision"], "emit", cfg["emit_on"])
    return 0


def cmd_save(args: argparse.Namespace) -> int:
    store = HostConfigStore(Path(args.store))
    cfg = json.loads(Path(args.config).read_text()) if args.config else MockControlEndpoint().readback()
    path = store.save(args.name, cfg, extra={"SWARM_BASE_ID": SWARM_BASE_ID})
    print("SAVE_OK", path, "live_nvm NOT_AUTHORIZED")
    return 0


def cmd_load(args: argparse.Namespace) -> int:
    store = HostConfigStore(Path(args.store))
    record = store.load(args.name)
    if args.output:
        Path(args.output).write_text(json.dumps(record, indent=2) + "\n")
    print("LOAD_OK schema", record["schema_version"], "scope", record["persistence_scope"])
    return 0


def cmd_status(args: argparse.Namespace) -> int:
    status = {
        "lane": "M",
        "SWARM_BASE_ID": SWARM_BASE_ID,
        "REAL_PCM_FIXTURE": REAL_PCM_FIXTURE,
        "live_nvm": "NOT_AUTHORIZED",
        "real_broker": "NOT_OPENED",
        "workflow": [
            "record",
            "inspect",
            "replay",
            "control",
            "readback",
            "save",
            "load",
        ],
    }
    print(json.dumps(status, indent=2))
    return 0


def cmd_demo_peer(args: argparse.Namespace) -> int:
    peer = MockPeerHmi(owner="mir-host")
    peer.accept(PeerMessage(version=1, owner="mir-host", sequence=1, kind="heartbeat"))
    peer.accept(
        PeerMessage(
            version=1,
            owner="mir-host",
            sequence=2,
            kind="handoff",
            payload={"new_owner": "hmi-display"},
        )
    )
    print("PEER_OK", json.dumps(peer.contract()))
    return 0


def cmd_demo_ingress(args: argparse.Namespace) -> int:
    adapter = HostIngressAdapter(SourceEpoch("digital-silence", 1))
    adapter.admit(IngressFrame("digital-silence", 1, 1, [0, 0, 0]))
    print("INGRESS_OK", json.dumps(adapter.note()))
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="cmd", required=True)

    r = sub.add_parser("record", help="bounded observation recording")
    r.add_argument("--output", type=Path, required=True)
    r.add_argument("--count", type=int, default=8)
    r.add_argument("--interval", type=float, default=0.0)
    r.add_argument("--gap-every", type=int, default=0)
    r.add_argument("--max-items", type=int, default=20000)
    r.add_argument("--max-bytes", type=int, default=32 * 1024 * 1024)
    r.add_argument("--as-replay", action="store_true")
    r.set_defaults(func=cmd_record)

    i = sub.add_parser("inspect", help="inspect observation jsonl")
    i.add_argument("--path", type=Path, required=True)
    i.add_argument("--output", type=Path)
    i.set_defaults(func=cmd_inspect)

    rp = sub.add_parser("replay", help="observation replay or PCM recompute (distinct labels)")
    rp.add_argument("--mode", choices=("observations", "pcm"), required=True)
    rp.add_argument("--input", type=Path, required=True)
    rp.add_argument("--output", type=Path, required=True)
    rp.add_argument("--digital", action="store_true", help="label PCM path as DIGITAL_FIXTURE")
    rp.set_defaults(func=cmd_replay)

    c = sub.add_parser("control", help="revision-checked control change on mock endpoint")
    c.add_argument("--mode-a", type=int, default=32)
    c.add_argument("--palette-a", type=int, default=0)
    c.add_argument("--disconnect", action="store_true")
    c.set_defaults(func=cmd_control)

    rb = sub.add_parser("readback", help="read back mock config")
    rb.add_argument("--seed", type=Path)
    rb.add_argument("--output", type=Path)
    rb.set_defaults(func=cmd_readback)

    s = sub.add_parser("save", help="host/mock config save")
    s.add_argument("--store", type=Path, required=True)
    s.add_argument("--name", required=True)
    s.add_argument("--config", type=Path)
    s.set_defaults(func=cmd_save)

    l = sub.add_parser("load", help="host/mock config load")
    l.add_argument("--store", type=Path, required=True)
    l.add_argument("--name", required=True)
    l.add_argument("--output", type=Path)
    l.set_defaults(func=cmd_load)

    st = sub.add_parser("status", help="workflow / fixture status")
    st.set_defaults(func=cmd_status)

    sub.add_parser("demo-peer", help="exercise mock peer/HMI contract").set_defaults(func=cmd_demo_peer)
    sub.add_parser("demo-ingress", help="exercise host ingress adapter").set_defaults(func=cmd_demo_ingress)
    return p


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
