#!/usr/bin/env python3
"""In-memory Titan live device on FakeSerial. Does not open CDC."""
from __future__ import annotations

import json
import struct
import time
import zlib
from typing import Any

import live_protocol as proto
from titan_transport import EXPECTED_UID, FakeSerial

SCHEMA = proto.SCHEMA_SHA256
BUILD = "test-build-live-device"
SNAPSHOT_BYTES = proto.SNAPSHOT_BYTES


def pack_snapshot_min(*, hop_sequence: int = 0, hops_consumed: int = 0,
                      config_revision: int = 0, emit_on: int = 0, mode_id: int = 8,
                      receipt: dict | None = None) -> bytes:
    buf = bytearray(SNAPSHOT_BYTES)
    struct.pack_into("<I", buf, 0, proto.SNAP_MAGIC)
    struct.pack_into("<H", buf, 4, 1)
    struct.pack_into("<H", buf, 6, 1)
    struct.pack_into("<Q", buf, 8, 1)
    struct.pack_into("<Q", buf, 24, int(hop_sequence))
    struct.pack_into("<H", buf, 596, int(mode_id))
    struct.pack_into("<I", buf, 604, int(config_revision))
    struct.pack_into("<I", buf, 608, int(hops_consumed))
    buf[670] = int(emit_on)
    rec = receipt or {}
    struct.pack_into("<I", buf, 812, int(rec.get("rendered_request_id") or 0))
    struct.pack_into("<I", buf, 816, int(rec.get("rendered_revision") or 0))
    struct.pack_into("<I", buf, 820, int(rec.get("rendered_frame") or 0))
    struct.pack_into("<I", buf, 824, int(rec.get("rendered_hop") or 0))
    struct.pack_into("<I", buf, 828, int(rec.get("superseded_request_id") or 0))
    struct.pack_into("<I", buf, 832, int(rec.get("accepted_request_id") or 0))
    struct.pack_into("<I", buf, 836, int(rec.get("accepted_revision") or 0))
    struct.pack_into("<I", buf, 840, int(rec.get("emitted_request_id") or 0))
    return bytes(buf)


class LiveDeviceSerial(FakeSerial):
    """Answers K1S1 opcodes 1/6/17/23/25 from Transport.write()."""

    def __init__(self, chunk: int = 32, mode_a: int = 8, emit_on: int = 0, revision: int = 2):
        super().__init__(chunk=chunk, timeout=0.05)
        self.uid = EXPECTED_UID
        self.build = BUILD
        self.schema = SCHEMA
        self.sequence = 1
        self.audio_hop = 0
        self.consume_events: list[dict[str, Any]] = []
        self.drop_next = False
        self.drop_before = False
        self.delay_s = 0.0
        self.wrong_rid_next = False
        self.duplicate_next = False
        self.stale_body_once = False
        self.malformed_next = False
        self.whole_config_attempts = 0
        self.patch_applies = 0
        self.cfg = {
            "version": 2,
            "revision": int(revision),
            "emit_on": int(emit_on),
            "flags": 1,
            "mode_a": int(mode_a),
            "mode_b": int(mode_a),
            "last_patch_request_id": 0,
            "visual_a": {
                "vp_waveform_shift_rate": 0.0,
                "hue_position": 0.0,
                "flags": proto.VIS_ENABLED | proto.VIS_MIRROR | proto.VIS_CHROMATIC,
            },
            "visual_b": {
                "vp_waveform_shift_rate": 0.0,
                "hue_position": 0.0,
                "flags": proto.VIS_ENABLED | proto.VIS_MIRROR | proto.VIS_CHROMATIC,
            },
        }
        self._rx_acc = bytearray()
        self.closed_by_test = False
        self.forbidden_subs = 0
        self.render_frames = 0
        self.receipt = {
            "accepted_request_id": 0,
            "accepted_revision": 0,
            "pending_request_id": 0,
            "pending_revision": 0,
            "rendered_request_id": 0,
            "rendered_revision": 0,
            "rendered_frame": 0,
            "rendered_hop": 0,
            "superseded_request_id": 0,
            "emitted_request_id": 0,
        }

    def restart(self) -> None:
        """Same-image reboot: counters and config roll back, build/schema stay."""
        self.sequence = 1
        self.audio_hop = 0
        self.render_frames = 0
        self.consume_events = []
        self.receipt = {key: 0 for key in self.receipt}
        self.cfg["revision"] = 0
        self.cfg["last_patch_request_id"] = 0
        vis = dict(self.cfg.get("visual_a") or {})
        vis["vp_waveform_shift_rate"] = 0.0
        self.cfg["visual_a"] = vis
        self._rx_acc = bytearray()

    def render_frame(self) -> None:
        """Latch renderer consumption. Snapshot does not do this by itself."""
        pending = int(self.receipt.get("pending_request_id") or 0)
        self.render_frames += 1
        self.audio_hop += 1
        if pending:
            self.receipt["rendered_request_id"] = pending
            self.receipt["rendered_revision"] = int(self.receipt.get("pending_revision") or 0)
            self.receipt["rendered_frame"] = self.render_frames
            self.receipt["rendered_hop"] = self.audio_hop
            self.receipt["pending_request_id"] = 0
            self.receipt["pending_revision"] = 0
            self.consume_events.append({
                "request_id": pending,
                "hop_sequence": self.audio_hop,
                "frame": self.render_frames,
                "revision": self.receipt["rendered_revision"],
            })

    def write(self, data: bytes) -> int:
        n = super().write(data)
        self._rx_acc.extend(data)
        self._drain()
        return n

    def close(self) -> None:
        self.closed_by_test = True
        super().close()

    def bump_revision(self) -> None:
        self.cfg["revision"] = int(self.cfg["revision"]) + 1

    def set_mode(self, mode: int) -> None:
        self.cfg["mode_a"] = int(mode)
        self.cfg["mode_b"] = int(mode)

    def set_schema(self, schema: str | None) -> None:
        self.schema = schema

    def apply_external(self, field: str, value: float) -> None:
        channel, short = field.split(".", 1)
        vis = dict(self.cfg.get(channel) or {})
        vis[short] = float(value)
        self.cfg[channel] = vis
        self.cfg["revision"] = int(self.cfg["revision"]) + 1

    def restart_same_image(self) -> None:
        self.sequence = 1
        self.audio_hop = 0
        self.cfg["revision"] = 0
        self.cfg["last_patch_request_id"] = 0
        vis = dict(self.cfg.get("visual_a") or {})
        vis["vp_waveform_shift_rate"] = 0.0
        self.cfg["visual_a"] = vis

    def _info_body(self) -> bytes:
        payload = {
            "protocol": 1,
            "uid": self.uid,
            "build": self.build,
            "source": "live-device",
            "contract": "sr24000.hop180.bins80.xover40",
            "clock_hz": 1000000000,
            "sequence": self.sequence,
            "schema_sha256": self.schema,
            "schema": self.schema,
            "runtime_kind": "live",
        }
        return json.dumps(payload).encode()

    def _drain(self) -> None:
        while True:
            if len(self._rx_acc) < 32:
                return
            magic, op, req, sequence, size, crc, proto_ver, hcrc = struct.unpack(
                "<4s7I", bytes(self._rx_acc[:32])
            )
            if magic != b"K1S1":
                del self._rx_acc[:1]
                continue
            if zlib.crc32(self._rx_acc[:28]) != hcrc:
                del self._rx_acc[:1]
                continue
            need = 32 + size
            if len(self._rx_acc) < need:
                return
            payload = bytes(self._rx_acc[32:need])
            del self._rx_acc[:need]
            if zlib.crc32(payload) != crc:
                self._respond(3, req, b"")
                continue
            self._handle(op, req, payload)

    def _respond(self, status: int, req: int, body: bytes) -> None:
        if self.drop_next:
            self.drop_next = False
            return
        if self.malformed_next:
            self.malformed_next = False
            self.feed(b"????not-a-frame????")
            return
        if self.delay_s:
            time.sleep(float(self.delay_s))
            self.delay_s = 0.0
        if self.wrong_rid_next:
            self.wrong_rid_next = False
            req = int(req) ^ 0xFFFF
        self.sequence += 1
        self.feed_response(status, req, self.sequence, body)
        if self.duplicate_next:
            self.duplicate_next = False
            self.feed_response(status, req, self.sequence, body)

    def _handle(self, op: int, req: int, payload: bytes) -> None:
        if op == 1:
            self._respond(0, req, self._info_body())
            return
        if op == 6:
            self._respond(0, req, json.dumps({"pdm_target": {"ap_hops": self.audio_hop, "measured_hz": 24000, "rate_locked": 1}}).encode())
            return
        if op == 17:
            self._respond(0, req, json.dumps({
                "mode_a": self.cfg["mode_a"],
                "emit_on": self.cfg["emit_on"],
                "din_b": "P604",
            }).encode())
            return
        if op == 23:
            self.audio_hop += 1
            body = pack_snapshot_min(
                hop_sequence=self.audio_hop,
                hops_consumed=self.audio_hop,
                config_revision=int(self.cfg["revision"]),
                emit_on=int(self.cfg["emit_on"]),
                mode_id=int(self.cfg["mode_a"]),
                receipt=self.receipt,
            )
            self._respond(0, req, body)
            return
        if op == 25:
            self._handle_config(req, payload)
            return
        self._respond(3, req, b"")

    def _handle_config(self, req: int, payload: bytes) -> None:
        if len(payload) < 4:
            self._respond(3, req, b"")
            return
        sub = proto._u32(payload, 0)
        if sub in (proto.CFG_SET_CONFIG, proto.CFG_BEGIN_SET, proto.CFG_COMMIT_SET,
                   proto.CFG_APPEND_SET):
            self.whole_config_attempts += 1
            self.forbidden_subs += 1
            self._respond(3, req, b"")
            return
        if sub == proto.CFG_GET_CONFIG:
            self._respond(0, req, proto.pack_config(self.cfg))
            return
        if sub != proto.CFG_PATCH:
            self._respond(3, req, b"")
            return
        if self.drop_before:
            self.drop_before = False
            return
        try:
            patch = proto.unpack_patch(payload)
        except ValueError:
            self._respond(3, req, b"")
            return
        if int(patch["revision"]) != int(self.cfg["revision"]):
            self._respond(5, req, b"")
            return
        if self.stale_body_once:
            self.stale_body_once = False
            stale = dict(self.cfg)
            vis = dict(stale.get("visual_a") or {})
            vis["vp_waveform_shift_rate"] = 40.0
            stale["visual_a"] = vis
            stale["revision"] = int(self.cfg["revision"]) + 1
            stale["last_patch_request_id"] = 1
            self._respond(0, req, proto.pack_config(stale))
            return
        channel = "visual_a" if patch["channel"] == 0 else "visual_b"
        vis = dict(self.cfg.get(channel) or {})
        vis.update({name.split(".", 1)[1]: float(value) for name, value in patch["fields"].items()})
        self.cfg[channel] = vis
        self.cfg["revision"] = int(self.cfg["revision"]) + 1
        self.cfg["last_patch_request_id"] = int(patch["request_id"])
        self.patch_applies += 1
        rid = int(patch["request_id"])
        if self.receipt["pending_request_id"] and self.receipt["pending_request_id"] != rid:
            self.receipt["superseded_request_id"] = self.receipt["pending_request_id"]
        self.receipt["accepted_request_id"] = rid
        self.receipt["accepted_revision"] = int(self.cfg["revision"])
        self.receipt["pending_request_id"] = rid
        self.receipt["pending_revision"] = int(self.cfg["revision"])
        self._respond(0, req, proto.pack_config(self.cfg))
