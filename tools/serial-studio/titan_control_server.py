#!/usr/bin/env python3
"""Restricted local IPC for SMC live-parameter patches. Does not open CDC."""
from __future__ import annotations

import json
import math
import os
import secrets
import socket
import threading
import time
from pathlib import Path
from typing import Callable

import emotiscope_control as donor
import live_protocol as proto

ALLOWED_OPS = frozenset({
    "hello", "get_config", "patch", "performance", "selection", "snapshot",
    "emotiscope_get", "emotiscope_patch",
})
ALLOWED_WRITE_OPS = frozenset({
    "patch", "performance", "selection", "get_config", "snapshot",
    "emotiscope_get", "emotiscope_patch",
})
APPROVAL_FILES = ("accepted-checkpoint.json", "accepted-candidate.json")
FORBIDDEN_CONFIG_SUBS = frozenset(
    {
        proto.CFG_SET_CONFIG,
        proto.CFG_BEGIN_SET,
        proto.CFG_APPEND_SET,
        proto.CFG_COMMIT_SET,
    }
)
MAX_LINE = 4096
MAX_OUTSTANDING = 1
WRITER_RECV_TIMEOUT = 30.0
LISTEN_ACCEPT_TIMEOUT = 0.2


def _parse_approval_file(path: Path) -> dict | None:
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError, UnicodeDecodeError):
        return None
    if not isinstance(data, dict) or not data:
        return None
    if not data.get("build") or not data.get("uid"):
        return None
    return data


def collect_approval_records(evidence: Path) -> tuple[list[dict], str | None]:
    evidence = Path(evidence)
    present = [evidence / name for name in APPROVAL_FILES if (evidence / name).is_file()]
    if not present:
        return [], "no_approval"
    parsed: list[dict] = []
    for path in present:
        rec = _parse_approval_file(path)
        if rec is None:
            return [], "malformed_approval"
        parsed.append(rec)
    if len(parsed) > 1:
        keys = ("uid", "build", "schema_sha256", "schema", "ops", "expires_at", "revoked", "fields", "targets")
        first = {key: parsed[0].get(key) for key in keys}
        for rec in parsed[1:]:
            if {key: rec.get(key) for key in keys} != first:
                return [], "ambiguous_approval"
    return [parsed[0]], None


def load_approval_record(evidence: Path) -> dict | None:
    records, error = collect_approval_records(evidence)
    if error or not records:
        return None
    return records[0]


def _require_str(value, name: str) -> str | None:
    if not isinstance(value, str) or not value:
        return f"{name}_type"
    return None


def hardware_writes_admitted(evidence: Path, bound: dict | None = None) -> tuple[bool, str]:
    if os.environ.get("TITAN_LIVE_CONTROL_ADMITTED") != "1":
        return False, "env"
    records, collect_error = collect_approval_records(evidence)
    if collect_error:
        return False, collect_error
    rec = records[0]
    bound = bound or {}
    for key in ("uid", "build"):
        err = _require_str(rec.get(key), key)
        if err:
            return False, err
        if rec.get(key) != bound.get(key):
            return False, f"{key}_mismatch"
    schema = rec.get("schema_sha256") or rec.get("schema")
    err = _require_str(schema, "schema")
    if err:
        return False, "schema_missing" if schema is None else err
    bound_schema = bound.get("schema_sha256") or bound.get("schema")
    if not isinstance(bound_schema, str) or not bound_schema:
        return False, "schema_missing"
    if schema != bound_schema:
        return False, "schema_mismatch"
    ops = rec.get("ops")
    if not isinstance(ops, list) or not ops:
        return False, "ops_missing"
    if not all(isinstance(op, str) and op for op in ops):
        return False, "ops_type"
    if any(op not in ALLOWED_WRITE_OPS for op in ops):
        return False, "ops_denied"
    if "patch" not in ops and "emotiscope_patch" not in ops:
        return False, "ops_denied"
    scoped = rec.get("fields") if rec.get("fields") is not None else rec.get("targets")
    if scoped is not None:
        if not isinstance(scoped, list) or not scoped or not all(isinstance(item, str) and item for item in scoped):
            return False, "fields_type"
    expires = rec.get("expires_at")
    if expires is not None:
        if isinstance(expires, bool) or (isinstance(expires, str) and not expires.strip()):
            return False, "expires_type"
        try:
            expiry = float(expires)
        except (TypeError, ValueError):
            return False, "expires_type"
        if not math.isfinite(expiry):
            return False, "expires_nonfinite"
        if time.time() > expiry:
            return False, "expired"
    revoked = rec.get("revoked")
    if revoked is True:
        return False, "revoked"
    if revoked not in (None, False):
        return False, "revoked_type"
    return True, "ok"


class RestrictedControlError(RuntimeError):
    def __init__(self, code: str, detail: str = ""):
        super().__init__(code)
        self.code = code
        self.detail = detail


class ControlSession:
    def __init__(self, bound: dict | None = None):
        self.token = secrets.token_urlsafe(18)
        self.bound = bound or {}
        self.pending: dict[str, float] = {}
        self.writer_id: str | None = None
        self.generation = 1

    def rotate(self) -> None:
        self.pending.clear()
        self.token = secrets.token_urlsafe(18)
        self.writer_id = None
        self.generation += 1


class RestrictedControlPlane:
    """JSON-line IPC. Translates named patches to opcode 25 PATCH only."""

    def __init__(self, transact: Callable[[int, bytes], dict] | None = None,
                 evidence: Path | None = None, bound: dict | None = None):
        self.transact = transact
        self.evidence = Path(evidence) if evidence else Path("/tmp/titan-control-offline")
        self.session = ControlSession(bound)
        self.forbidden_attempts = 0
        self.hw_writes = 0
        self.last_config: dict | None = None
        self.last_donor: dict | None = None
        self._io = threading.Lock()
        self._outstanding = 0

    def handle(self, request: dict, conn_id: str | None = None) -> dict:
        op = request.get("op")
        if op not in ALLOWED_OPS:
            self.forbidden_attempts += 1
            return {"ok": False, "error": "forbidden_op", "op": op}
        try:
            if op == "hello":
                return self._hello(conn_id)
            if op == "get_config":
                return self._get_config(request)
            if op == "snapshot":
                return self._snapshot(request)
            if op == "emotiscope_get":
                return self._emotiscope_get(request)
            if op == "emotiscope_patch":
                return self._emotiscope_patch(request, conn_id)
            return self._patch(request, conn_id)
        except RestrictedControlError as exc:
            return {"ok": False, "error": exc.code, "detail": exc.detail}

    def on_reconnect(self, bound: dict | None = None) -> None:
        self.session.bound = bound or self.session.bound
        self.session.rotate()
        self.last_config = None
        self.last_donor = None

    def _hello(self, conn_id: str | None) -> dict:
        if conn_id:
            if self.session.writer_id is None:
                self.session.writer_id = conn_id
            elif self.session.writer_id != conn_id:
                return {"ok": False, "error": "writer_busy", "session": self.session.token}
        admitted, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        return {
            "ok": True,
            "session": self.session.token,
            "bound": self.session.bound,
            "admitted": admitted,
            "admit_reason": reason,
            "generation": self.session.generation,
        }

    def _require_session(self, request: dict) -> str | None:
        token = request.get("session")
        if not token or token != self.session.token:
            return "stale_session"
        return None

    def _transact(self, op: int, payload: bytes) -> dict:
        if self.transact is None:
            raise RestrictedControlError("ADMISSION_HOLD", "no transport")
        with self._io:
            if self._outstanding >= MAX_OUTSTANDING:
                raise RestrictedControlError("busy", "outstanding")
            self._outstanding += 1
            try:
                return self.transact(op, payload)
            except RestrictedControlError:
                raise
            except Exception as exc:
                msg = str(exc).lower()
                if "quarantine" in msg or "header mismatch" in msg:
                    raise RestrictedControlError("quarantined", str(exc)) from exc
                raise RestrictedControlError("transport", str(exc)) from exc
            finally:
                self._outstanding -= 1

    def _get_config(self, request: dict) -> dict:
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        if self.transact is None:
            if self.last_config is None:
                return {"ok": False, "error": "ADMISSION_HOLD"}
            return {"ok": True, "config": self.last_config, "session": self.session.token}
        got = self._transact(25, proto.pack_config_sub(proto.CFG_GET_CONFIG))
        if not got.get("ok"):
            return {"ok": False, "error": "get_config_rejected", "status": got.get("status")}
        cfg = proto.unpack_config(got["body"])
        self.last_config = cfg
        return {"ok": True, "config": cfg, "session": self.session.token}

    def _snapshot(self, request: dict) -> dict:
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        if self.transact is None:
            return {"ok": False, "error": "ADMISSION_HOLD"}
        kind = request.get("kind", "live")
        if kind == "emotiscope_ml":
            got = self._transact(17, b"MLST")
            if not got.get("ok"):
                return {"ok": False, "error": "ml_witness_rejected", "status": got.get("status")}
            body = got.get("body")
            try:
                witness = donor.decode_ml_state(body)
            except (ValueError, TypeError):
                return {"ok": False, "error": "ml_witness_framing"}
            return {"ok": True, "witness": witness, "body_hex": bytes(body).hex(), "session": self.session.token}
        if kind != "live":
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": "snapshot.kind"}
        got = self._transact(23, b"")
        if not got.get("ok"):
            return {"ok": False, "error": "snapshot_rejected", "status": got.get("status")}
        return {"ok": True, "snapshot": proto.unpack_snapshot(got["body"]), "session": self.session.token}

    def _patch(self, request: dict, conn_id: str | None) -> dict:
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        if conn_id and self.session.writer_id not in (None, conn_id):
            return {"ok": False, "error": "writer_busy"}
        if conn_id and self.session.writer_id is None:
            self.session.writer_id = conn_id
        fields = request.get("fields") or {}
        selection = request.get("op") == "selection"
        if not isinstance(fields, dict) or (not fields and not selection) or (selection and len(fields)>8):
            return {"ok": False, "error": "empty_patch"}
        request_id = request.get("request_id")
        if request_id is None:
            return {"ok": False, "error": "request_id_required"}
        boot_id = request.get("boot_id")
        if boot_id is None:
            return {
                "ok": False,
                "error": "missing_boot_id",
                "request_id": int(request_id),
                "session": self.session.token,
            }
        try:
            if selection:
                wire = proto.pack_selection(request.get("revision"), request_id, boot_id,
                                            request.get("mode"), request.get("palette"), fields)
            elif request.get("op") == "performance":
                wire = proto.pack_performance(request.get("revision"), fields, request_id=request_id,
                                              boot_id=boot_id, palette=request.get("palette"), audition=request.get("audition",False))
            else:
                wire = proto.pack_patch(int(request.get("revision", 0)), fields, request_id=int(request_id),
                                        boot_id=int(boot_id))
        except (ValueError, TypeError, OverflowError) as exc:
            self.forbidden_attempts += 1
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": str(exc)}
        sub = proto._u32(wire, 0)
        if sub in FORBIDDEN_CONFIG_SUBS or sub not in ({12} if selection else {9, 10, 11} if request.get("op") == "performance" else {proto.CFG_PATCH}):
            self.forbidden_attempts += 1
            return {"ok": False, "error": "would_emit_whole_config"}
        if (self.evidence / "quiesce.flag").exists():
            return {
                "ok": False,
                "error": "hold",
                "request_id": int(request_id),
                "session": self.session.token,
                "queued_cleared": True,
            }
        admitted, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        if self.transact is None or not admitted:
            return {
                "ok": False,
                "error": "ADMISSION_HOLD",
                "admit_reason": reason,
                "request_id": int(request_id),
                "session": self.session.token,
                "queued_cleared": False,
            }
        rec = load_approval_record(self.evidence)
        if request.get("op") in {"performance", "selection"} and (rec is None or request["op"] not in rec.get("ops", [])):
            return {"ok": False, "error": "ops_denied", "admit_reason": "performance_not_admitted"}
        if request.get("op") == "patch" and (rec is None or "patch" not in rec.get("ops", [])):
            return {"ok": False, "error": "ops_denied", "admit_reason": "legacy_patch_not_admitted"}
        scoped = None
        if rec is not None:
            scoped = rec.get("fields") if rec.get("fields") is not None else rec.get("targets")
        requested_scope = set(fields) | ({"scene.palette_a"} if request.get("palette") is not None else set()) | ({"scene.mode"} if selection else set())
        if scoped is not None and requested_scope - set(scoped):
            return {
                "ok": False,
                "error": "ops_denied",
                "admit_reason": "field_scope",
                "request_id": int(request_id),
                "session": self.session.token,
            }
        got = self._transact(25, wire)
        if not got.get("ok"):
            if got.get("error") in {"hold", "ADMISSION_HOLD"}:
                return {
                    "ok": False,
                    "error": got.get("error"),
                    "admit_reason": got.get("admit_reason"),
                    "request_id": int(request_id),
                    "session": self.session.token,
                    "queued_cleared": got.get("error") == "hold",
                }
            status = got.get("status")
            if status == 6:
                error = "wrong_boot"
            elif status == 5:
                error = "stale"
            else:
                error = "patch_rejected"
            return {
                "ok": False,
                "error": error,
                "status": status,
                "request_id": int(request_id),
                "session": self.session.token,
            }
        cfg = proto.unpack_config(got["body"])
        self.last_config = cfg
        applied = {name: (proto.performance_value(cfg, name) if request.get("op") in {"performance", "selection"}
                          else _visual_get(cfg, name)) for name in fields}
        self.hw_writes += 1
        return {
            "ok": True,
            "revision": cfg.get("revision"),
            "applied": applied,
            "palette_a": cfg.get("palette_a"),
            "mode_a": cfg.get("mode_a"), "mode_b": cfg.get("mode_b"),
            "config": cfg if selection else None,
            "emit_on": cfg.get("emit_on"),
            "session": self.session.token,
            "request_id": int(request_id),
            "last_patch_request_id": cfg.get("last_patch_request_id"),
            "boot_id": cfg.get("boot_id"),
        }

    def _donor_ids(self, request: dict) -> tuple[int, int, int] | dict:
        request_id = request.get("request_id")
        boot_id = request.get("boot_id")
        revision = request.get("revision", 0)
        if request_id is None:
            return {"ok": False, "error": "request_id_required"}
        if boot_id is None:
            return {
                "ok": False,
                "error": "missing_boot_id",
                "request_id": int(request_id) if isinstance(request_id, int) else request_id,
                "session": self.session.token,
            }
        try:
            return int(request_id), int(boot_id), int(revision)
        except (TypeError, ValueError):
            return {"ok": False, "error": "forbidden_or_invalid_field"}

    def _donor_reply(self, got: dict, request_id: int, wrote: bool) -> dict:
        body = got.get("body") or b""
        state = _decode_donor_body(body) if isinstance(body, (bytes, bytearray)) else None
        status = int(got.get("status") or 0)
        if got.get("ok") and status == 0 and state is not None:
            self.last_donor = state
            if wrote:
                self.hw_writes += 1
            return {
                "ok": True,
                "state": state,
                "revision": state["revision"],
                "boot_id": state["boot_id"],
                "request_id": request_id,
                "accepted_request_id": state["accepted_request_id"],
                "rendered_request_id": state["rendered_request_id"],
                "emitted_request_id": state["emitted_request_id"],
                "dedup": state["dedup"],
                "changed": state["changed"],
                "mode_transition": state["mode_transition"],
                "session": self.session.token,
                "status": 0,
            }
        error = _DONOR_STATUS.get(status, "patch_rejected")
        if state is None and status == 0:
            error = "donor_framing"
        reply = {
            "ok": False,
            "error": error,
            "status": status,
            "request_id": request_id,
            "session": self.session.token,
        }
        if state is not None:
            self.last_donor = state
            reply["state"] = state
            reply["revision"] = state["revision"]
            reply["boot_id"] = state["boot_id"]
        return reply

    def _emotiscope_get(self, request: dict) -> dict:
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        ids = self._donor_ids(request)
        if isinstance(ids, dict):
            return ids
        request_id, boot_id, _revision = ids
        observed = self.last_donor or {}
        same_boot = observed.get("boot_id") == boot_id
        board = request.get("board", 0)
        # Extended readback is explicitly negotiated. A new broker does not
        # silently enlarge a legacy client's request or trust caller caps.
        requested_version = request.get("state_version", min(observed.get("version", 2), 4) if same_boot else 2)
        if type(requested_version) is not int or requested_version not in (1, 2, 3, 4, 5, 6, 7, 8):
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": "state_version"}
        try:
            wire = donor.encode_get(
                request_id, boot_id, board,
                state_version=requested_version,
                board_capabilities=(observed.get("board_capabilities", 0) if same_boot else 0) & (511 if requested_version>=8 else 255 if requested_version>=6 else 63 if requested_version==5 else 7 if requested_version==4 else 3),
            )
        except (ValueError, TypeError, OverflowError) as exc:
            self.forbidden_attempts += 1
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": str(exc)}
        if self.transact is None:
            if self.last_donor is None or not same_boot:
                return {"ok": False, "error": "ADMISSION_HOLD"}
            if self.last_donor.get("selected_board", 0) != board:
                return {"ok": False, "error": "ADMISSION_HOLD", "detail": "selected_board_not_cached"}
            return {"ok": True, "state": self.last_donor, "session": self.session.token, "cached": True}
        got = self._transact(25, wire)
        reply = self._donor_reply(got, request_id, wrote=False)
        if requested_version >= 5 and int(got.get("status") or 0) == 3:
            reply["error"] = "unsupported_state_version"
        return reply

    def _emotiscope_patch(self, request: dict, conn_id: str | None) -> dict:
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        if conn_id and self.session.writer_id not in (None, conn_id):
            return {"ok": False, "error": "writer_busy"}
        if conn_id and self.session.writer_id is None:
            self.session.writer_id = conn_id
        fields = request.get("fields")
        if not isinstance(fields, dict) or len(fields) > len(donor.FIELDS):
            return {"ok": False, "error": "empty_patch" if not isinstance(fields, dict) else "too_many_fields"}
        ids = self._donor_ids(request)
        if isinstance(ids, dict):
            return ids
        request_id, boot_id, revision = ids
        observed = self.last_donor or {}
        # Capability comes from this bound device's GET/reply, never the caller.
        same_boot = observed.get("boot_id") == boot_id
        requested_version = request.get("state_version", min(observed.get("version", 2), 4) if same_boot else 2)
        if type(requested_version) is not int or requested_version not in (1, 2, 3, 4, 5, 6, 7, 8):
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": "state_version"}
        try:
            wire = donor.encode_patch(
                request_id, boot_id, revision, fields,
                state_version=requested_version,
                board_capabilities=observed.get("board_capabilities", 0) if same_boot else 0,
            )
        except (ValueError, TypeError, OverflowError, KeyError) as exc:
            self.forbidden_attempts += 1
            return {"ok": False, "error": "forbidden_or_invalid_field", "detail": str(exc)}
        if int.from_bytes(wire[:4], "little") not in (donor.PATCH_SUB, donor.EXTENDED_PATCH_SUB, donor.INHOUSE_PATCH_SUB, donor.WAVEFORMS_PATCH_SUB):
            self.forbidden_attempts += 1
            return {"ok": False, "error": "would_emit_whole_config"}
        if (self.evidence / "quiesce.flag").exists():
            return {
                "ok": False,
                "error": "hold",
                "request_id": request_id,
                "session": self.session.token,
                "queued_cleared": True,
            }
        admitted, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        if self.transact is None or not admitted:
            return {
                "ok": False,
                "error": "ADMISSION_HOLD",
                "admit_reason": reason,
                "request_id": request_id,
                "session": self.session.token,
                "queued_cleared": False,
            }
        rec = load_approval_record(self.evidence)
        if rec is None or "emotiscope_patch" not in rec.get("ops", []):
            return {"ok": False, "error": "ops_denied", "admit_reason": "emotiscope_patch_not_admitted"}
        scoped = rec.get("fields") if rec.get("fields") is not None else rec.get("targets")
        if scoped is not None and set(fields) - set(scoped):
            return {
                "ok": False,
                "error": "ops_denied",
                "admit_reason": "field_scope",
                "request_id": request_id,
                "session": self.session.token,
            }
        got = self._transact(25, wire)
        return self._donor_reply(got, request_id, wrote=True)


def donor_public_state(state: dict) -> dict:
    flags = int(state["flags"])
    applicability = int(state["applicability"])
    public = {
        "version": state["version"],
        "boot_id": state["boot_id"],
        "revision": state["revision"],
        "request_id": state["request_id"],
        "brightness": state["brightness"],
        "softness": state["softness"],
        "color": state["color"],
        "effective_hue": state["effective_hue"],
        "warmth": state["warmth"],
        "color_range": state["color_range"],
        "speed": state["speed"],
        "saturation": state["saturation"],
        "background": state["background"],
        "mode": state["mode"],
        "mirror_mode": bool(flags & 1),
        "screensaver": bool(flags & 2),
        "temporal_dithering": bool(flags & 4),
        "auto_color_cycle": bool(flags & 8),
        "reverse_color_range": bool(flags & 16),
        "emission_enabled": bool(flags & 32),
        "output_precision": 1 if flags & donor.FLAG_OUTPUT_PRECISION else 0,
        "output_intensity_supported": state.get("output_intensity_supported", False),
        "output_intensity_a": state.get("output_intensity_a"),
        "output_intensity_b": state.get("output_intensity_b"),
        "board_capabilities": state.get("board_capabilities", 0),
        "primary_intensity_supported": state.get("primary_intensity_supported", False),
        "secondary_output_supported": state.get("secondary_output_supported", False),
        "primary_intensity": state.get("primary_intensity"),
        "secondary_intensity": state.get("secondary_intensity"),
        "secondary_output_enabled": state.get("secondary_output_enabled"),
        "selected_board": state.get("selected_board", 0),
        "independent_scenes_supported": state.get("independent_scenes_supported", False),
        "response_gain": state.get("response_gain"),
        "edge_response": state.get("edge_response"),
        "edge_depth": state.get("edge_depth"),
        "palette": state.get("palette", 0),
        "hardware_output_brightness": state["output_brightness"],
        "applicability": applicability,
        "speed_applicable": bool(applicability & (1 << 5)),
        "reverse_applicable": bool(applicability & (1 << 13)),
        "accepted_request_id": state["accepted_request_id"],
        "rendered_request_id": state["rendered_request_id"],
        "emitted_request_id": state["emitted_request_id"],
        "rendered_frame": state["rendered_frame"],
        "last_status": state["last_status"],
        "dedup": state["dedup"],
        "changed": state["changed"],
        "mode_transition": state["mode_transition"],
        "schema_sha256": state["schema"].hex(),
    }
    feature_keys = (
        "edge_colour_enabled", "edge_colour_mode", "edge_colour_strength", "edge_colour_spread", "edge_colour_spatial",
        "pair_link", "effective_pair_link", "pair_colour_enabled", "pair_colour_mode", "pair_colour_strength", "pair_colour_spread", "pair_colour_spatial",
        "musical_role", "musical_strength", "musical_attack_ms", "musical_release_ms", "ml_enabled", "ml_strength", "ml_valid", "ml_result_sequence",
        "ml_input_sequence", "ml_now_us", "ml_model_id", "ml_inference_count", "ml_last_cycles", "ml_max_cycles", "ml_rejected_results",
        "publication_sequence", "event_cursor", "conditioning_value",
    )
    feature_keys += tuple(donor.FIELDS[field][0] for field in range(45, 64)) + (
        "composed_scenes_supported", "active_scene", "scene_phase", "scene_pending",
        "scene_gesture_id", "scene_phrase_state", "scene_quiet",
        "renderer_family", "inhouse_waveform_supported", "latest_state_version",
        "supported_modes", "active_controls", "inhouse_mood", "inhouse_mood_supported",
    )
    for key in feature_keys:
        public[key] = state.get(key)
    return public



_DONOR_STATUS = {
    3: "invalid",
    5: "stale",
    6: "wrong_boot",
    7: "request_conflict",
    9: "reconcile",
    10: "busy",
}


def _decode_donor_body(body: bytes) -> dict | None:
    if len(body) not in (donor.STATE_BYTES, donor.TOP_STATE_BYTES, donor.DUAL_STATE_BYTES, donor.FEATURE_STATE_BYTES, donor.COMPOSED_STATE_BYTES, donor.INHOUSE_STATE_BYTES, donor.WAVEFORMS_STATE_BYTES):
        return None
    try:
        state = donor.decode_state(body)
    except (ValueError, TypeError):
        return None
    reserved_bytes = 12 if state["version"] >= 6 else 16 if state["version"] in (1, 4) else 8
    if state["magic"] != donor.STATE_MAGIC or state["reserved"] != b"\x00" * reserved_bytes:
        return None
    return donor_public_state(state)


def _visual_get(cfg: dict, name: str):
    channel, field = name.split(".", 1)
    vis = cfg.get(channel) or {}
    return vis.get(field)


class SocketInUseError(RuntimeError):
    pass


def unix_socket_has_listener(path: Path) -> bool:
    if not path.exists():
        return False
    probe = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    probe.settimeout(0.2)
    try:
        probe.connect(str(path))
        return True
    except OSError:
        return False
    finally:
        try:
            probe.close()
        except OSError:
            pass


class ControlServer:
    def __init__(self, plane: RestrictedControlPlane, socket_path: Path):
        self.plane = plane
        self.path = Path(socket_path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        try:
            os.chmod(self.path.parent, 0o700)
        except OSError:
            pass
        if unix_socket_has_listener(self.path):
            raise SocketInUseError(str(self.path))
        if self.path.exists():
            self.path.unlink()
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.bind(str(self.path))
        os.chmod(self.path, 0o600)
        self.sock.listen(1)
        self.sock.settimeout(LISTEN_ACCEPT_TIMEOUT)
        self.stop = False
        self.thread = threading.Thread(target=self._loop, daemon=True)
        self._conn_lock = threading.Lock()
        self._active = False
        self._active_conn_id: str | None = None

    def start(self) -> str:
        self.thread.start()
        return str(self.path)

    def close(self) -> None:
        self.stop = True
        try:
            self.sock.close()
        except OSError:
            pass
        try:
            if self.path.exists():
                self.path.unlink()
        except OSError:
            pass

    def _loop(self) -> None:
        while not self.stop:
            try:
                conn, _addr = self.sock.accept()
            except (TimeoutError, OSError):
                continue
            conn_id = secrets.token_hex(8)
            with self._conn_lock:
                if self._active:
                    try:
                        conn.sendall(b'{"ok":false,"error":"writer_busy"}\n')
                        conn.close()
                    except OSError:
                        pass
                    continue
                self._active = True
                self._active_conn_id = conn_id
            threading.Thread(target=self._serve, args=(conn, conn_id), daemon=True).start()

    def _serve(self, conn: socket.socket, conn_id: str) -> None:
        buf = b""
        conn.settimeout(WRITER_RECV_TIMEOUT)
        try:
            while not self.stop:
                chunk = conn.recv(MAX_LINE)
                if not chunk:
                    return
                if len(buf) + len(chunk) > MAX_LINE:
                    conn.sendall(b'{"ok":false,"error":"message_too_large"}\n')
                    return
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    if not line.strip():
                        continue
                    try:
                        request = json.loads(line.decode())
                        with self._conn_lock:
                            reply = self.plane.handle(request, conn_id=conn_id)
                    except (json.JSONDecodeError, UnicodeDecodeError, TypeError):
                        reply = {"ok": False, "error": "bad_json"}
                    conn.sendall((json.dumps(reply) + "\n").encode())
        except OSError:
            return
        finally:
            with self._conn_lock:
                if self._active_conn_id == conn_id:
                    self._active = False
                    self._active_conn_id = None
                if self.plane.session.writer_id == conn_id:
                    self.plane.session.writer_id = None
            try:
                conn.close()
            except OSError:
                pass


def main(argv: list[str] | None = None) -> int:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--socket", required=True)
    parser.add_argument("--evidence", required=True)
    args = parser.parse_args(argv)
    evidence = Path(args.evidence)
    evidence.mkdir(parents=True, exist_ok=True)
    plane = RestrictedControlPlane(transact=None, evidence=evidence)
    server = ControlServer(plane, Path(args.socket))
    server.start()
    print(json.dumps({"ok": True, "socket": str(server.path), "admitted": False}))
    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        return 0
    finally:
        server.close()


if __name__ == "__main__":
    raise SystemExit(main())
