#!/usr/bin/env python3
"""Restricted local IPC for SMC live-parameter patches. Does not open CDC."""
from __future__ import annotations

import json
import hashlib
import math
import os
import secrets
import socket
import struct
import threading
import time
from pathlib import Path
from typing import Callable

import emotiscope_control as donor
import live_protocol as proto

NATIVE_OPS = frozenset({
    "native_identity", "native_health", "native_status",
    "native_scene", "native_global", "native_durable", "native_durable_read",
})
NATIVE_OPCODES = {
    "native_identity": 0x50524200,
    "native_status": 0x50524201,
    "native_scene": 0x50524203,
    "native_global": 0x50524204,
    "native_health": 0x50524205,
    "native_durable": 0x50524206,
    "native_durable_read": 0x50524207,
}
BIND_OPCODE = 0x50524202
TARGET_UID = b"545433931bd25436593630352d068363"
ALLOWED_OPS = frozenset({
    "hello", "native_isolated_capture", "native_fixture_status", "legacy_health", "get_config", "patch", "performance", "selection", "snapshot",
    "emotiscope_get", "emotiscope_patch",
}) | NATIVE_OPS
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
OWNER_LEASE_PATH = Path('/tmp/k1-show-20261018-TITAN.lock/owner.json')
READ_OPS = frozenset({'legacy_health', 'get_config', 'snapshot', 'emotiscope_get',
                      'native_identity', 'native_health', 'native_status', 'native_durable_read'})


def maintenance_lease_digest():
    """Only the fixed canonical owner file grants a temporary maintenance lane."""
    try:
        raw = OWNER_LEASE_PATH.read_bytes()
        lease = json.loads(raw)
        if type(lease.get('pid')) is not int or lease['pid'] <= 0:
            return None
        os.kill(lease['pid'], 0)
        return hashlib.sha256(raw).hexdigest()
    except (OSError, ValueError, TypeError):
        return None


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


def _native_writes_admitted(rec: dict, bound: dict) -> tuple[bool, str]:
    """Admit the replacement opcodes actually advertised by the current image."""
    if rec.get("controls_persistent") is not False or bound.get("controls_persistent") is not False:
        return False, "persistence"
    if rec.get("durable_commit") != "explicit-v1" or bound.get("durable_commit") != "explicit-v1":
        return False, "durable_commit"
    for key in ("uid", "build"):
        err = _require_str(rec.get(key), key)
        if err:
            return False, err
        if rec.get(key) != bound.get(key):
            return False, f"{key}_mismatch"
    schema = rec.get("schema_sha256") or rec.get("schema")
    if schema != "replacement-health-v1":
        return False, "schema_mismatch"
    bound_schema = bound.get("schema_sha256") or bound.get("schema")
    if bound_schema != schema:
        return False, "schema_mismatch"
    ops = rec.get("ops")
    if not isinstance(ops, list) or not {"native_identity", "native_health"} <= set(ops) <= NATIVE_OPS:
        return False, "ops_denied"
    if set(ops) & {"patch", "emotiscope_get", "emotiscope_patch", "performance", "selection"}:
        return False, "ops_denied"
    return True, "ok"


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
    if rec.get("protocol") == "titan-native":
        return _native_writes_admitted(rec, bound)
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
                 evidence: Path | None = None, bound: dict | None = None, isolated_execute=None,
                 fixture_status_read=None):
        self.transact = transact
        self.isolated_execute = isolated_execute
        self.fixture_status_read = fixture_status_read
        self.evidence = Path(evidence) if evidence else Path("/tmp/titan-control-offline")
        self.session = ControlSession(bound)
        self.forbidden_attempts = 0
        self.hw_writes = 0
        self.last_config: dict | None = None
        self.last_donor: dict | None = None
        self._io = threading.Lock()
        self._outstanding = 0
        self.maintenance_id = None
        self.maintenance_token = None
        self.maintenance_digest = None

    def handle(self, request: dict, conn_id: str | None = None) -> dict:
        if request.get('op') == 'native_fixture_status' and not (conn_id and conn_id == self.maintenance_id):
            return {'ok': False, 'error': 'maintenance_required'}
        if request.get('op') == 'hello' and 'maintenance' in request:
            proof = request.get('maintenance')
            digest = maintenance_lease_digest()
            if (not conn_id or not isinstance(proof, dict) or set(proof) != {'owner_lease_sha256'}
                    or not digest or proof.get('owner_lease_sha256') != digest):
                return {'ok': False, 'error': 'maintenance_lease_invalid'}
            if self.maintenance_id not in (None, conn_id):
                return {'ok': False, 'error': 'maintenance_busy'}
            self.maintenance_id, self.maintenance_digest = conn_id, digest
            self.maintenance_token = secrets.token_urlsafe(24)
            reply = self._hello(None)
            return dict(reply, session=self.maintenance_token, maintenance=True)
        if conn_id and conn_id == self.maintenance_id:
            if (maintenance_lease_digest() != self.maintenance_digest
                    or request.get('session') != self.maintenance_token):
                return {'ok': False, 'error': 'maintenance_lease_revoked'}
            # Existing command admission and framing remain the only dispatch path.
            row = dict(request, session=self.session.token)
            reply = self._handle(row, conn_id=None)
            if 'session' in reply:
                reply = dict(reply, session=self.maintenance_token)
            return reply
        if request.get('op') != 'hello':
            if conn_id and self.session.writer_id not in (None, conn_id):
                return {'ok': False, 'error': 'writer_busy'}
            if request.get('op') not in READ_OPS and OWNER_LEASE_PATH.exists():
                return {'ok': False, 'error': 'maintenance_hold', 'queued_cleared': True}
        return self._handle(request, conn_id)

    def _handle(self, request: dict, conn_id: str | None = None) -> dict:
        op = request.get("op")
        if op not in ALLOWED_OPS:
            self.forbidden_attempts += 1
            return {"ok": False, "error": "forbidden_op", "op": op}
        try:
            if op == "hello":
                return self._hello(conn_id)
            if op == 'native_isolated_capture':
                return self._isolated_capture(request)
            if op == 'native_fixture_status':
                return self._fixture_status(request)
            if op == "legacy_health":
                return self._legacy_health(request)
            if op == "get_config":
                return self._get_config(request)
            if op == "snapshot":
                return self._snapshot(request)
            if op == "emotiscope_get":
                return self._emotiscope_get(request)
            if op == "emotiscope_patch":
                return self._emotiscope_patch(request, conn_id)
            if op in NATIVE_OPS:
                return self._native(request)
            return self._patch(request, conn_id)
        except RestrictedControlError as exc:
            return {"ok": False, "error": exc.code, "detail": exc.detail}

    def on_reconnect(self, bound: dict | None = None) -> None:
        self.session.bound = bound or self.session.bound
        self.session.rotate()
        self.maintenance_id = self.maintenance_token = self.maintenance_digest = None
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

    def _fixture_status(self, request):
        """Fresh fixture STATUS on the sole transport. No ARM, drain or CANCEL."""
        err = self._require_session(request)
        if err:
            return {'ok': False, 'error': err}
        admitted, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        rec = load_approval_record(self.evidence) or {}
        if (not admitted or rec.get('protocol') != 'titan-native'
                or 'native_status' not in rec.get('ops', [])):
            return {'ok': False, 'error': 'ADMISSION_HOLD', 'admit_reason': reason}
        boot, epoch = request.get('boot'), request.get('epoch')
        if (type(boot) is not int or boot <= 0 or type(epoch) is not int or epoch < 0
                or self.fixture_status_read is None):
            return {'ok': False, 'error': 'fixture_status_request'}
        ident = self._native_identity()
        if ident.get('ok') is not True:
            return ident
        control = self._native_status()
        if control.get('ok') is not True or control.get('boot') != boot:
            return {'ok': False, 'error': 'fixture_boot_changed'}
        if control.get('bound') is not True:
            self._bound_boot = None
        failed = self._ensure_bound()
        if failed:
            return failed
        try:
            with self._io:
                raw = self.fixture_status_read(boot, epoch)
            if not isinstance(raw, bytes) or len(raw) not in (640, 768):
                raise ValueError('fixture_status_length')
            version, actual_boot, actual_epoch, state = struct.unpack_from('<4I', raw)
            # v7 (21 Oct 2026): the v6 field map with 3,840-bit lanes; same 768-byte length.
            if (version, len(raw)) not in ((4, 640), (5, 768), (6, 768), (7, 768)) or (actual_boot, actual_epoch) != (boot, epoch):
                raise ValueError('fixture_status_identity')
            if epoch == 0:
                if state != 0 or struct.unpack_from('<I', raw, 16)[0] != 0:
                    raise ValueError('fixture_not_clean_bootstrap')
            elif state not in (6, 7, 8):
                raise ValueError('fixture_still_active_or_uninitialised')
            if any(struct.unpack_from('<I', raw, word * 4)[0] & 0x80 for word in range(72, 76)):
                raise ValueError('fixture_dma_act')
        except (ValueError, RuntimeError, TimeoutError, OSError) as exc:
            return {'ok': False, 'error': str(exc), 'replayed': False}
        return {'ok': True, 'identity': ident['identity'], 'control': control,
                'raw_hex': raw.hex(), 'cached': False, 'replayed': False,
                'opcode': 0x50524602, 'physical_qualified': False,
                'session': self.session.token}

    def _isolated_capture(self, request):
        err = self._require_session(request)
        if err:
            return {'ok': False, 'error': err, 'capture_finished': True}
        allowed, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        if not allowed:
            return {'ok': False, 'error': 'ADMISSION_HOLD', 'admit_reason': reason, 'capture_finished': True}
        records, error = collect_approval_records(self.evidence)
        if error or self.isolated_execute is None:
            return {'ok': False, 'error': 'isolated_capture_unavailable', 'capture_finished': True}
        import hashlib
        source = next((self.evidence / name for name in APPROVAL_FILES if (self.evidence / name).is_file()), None)
        if source is None or hashlib.sha256(source.read_bytes()).hexdigest() != request.get('admission_sha256'):
            return {'ok': False, 'error': 'isolated_admission_hash_mismatch', 'capture_finished': True}
        # Separate exact isolated grant required; ordinary native controls grant
        # neither this collector nor a caller-selected raw opcode tunnel.
        if not isinstance(records[0].get('isolated_capture_grant'), dict):
            return {'ok': False, 'error': 'isolated_capture_grant_missing', 'capture_finished': True}
        try:
            result = self.isolated_execute(request, records[0], dict(self.session.bound))
            return {'ok': True, 'result': result, 'capture_finished': True, 'session': self.session.token}
        except Exception as exc:
            return {'ok': False, 'error': str(exc), 'capture_finished': getattr(exc, 'collector_quiesced', False), 'session': self.session.token}

    def _legacy_health(self, request: dict) -> dict:
        """Fresh identity and output telemetry. No cached state, writes or reset."""
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        if self.session.bound.get("protocol") == "titan-native":
            return {"ok": False, "error": "wrong_protocol"}
        previous = None
        for index in range(2):
            if index:
                time.sleep(0.05)
            documents = []
            for opcode in (1, 17):
                got = self._transact(opcode, b"")
                if not got.get("ok"):
                    return {"ok": False, "error": "legacy_health_rejected", "status": got.get("status")}
                try:
                    documents.append(json.loads(got["body"]))
                except (ValueError, TypeError, KeyError):
                    return {"ok": False, "error": "legacy_health_json"}
            info, output = documents
            if not isinstance(info, dict) or not isinstance(output, dict):
                return {"ok": False, "error": "legacy_health_json"}
            if any(info.get(key) != self.session.bound.get(key) for key in ("uid", "build")):
                return {"ok": False, "error": "identity_mismatch"}
            if info.get("schema_sha256") != (self.session.bound.get("schema_sha256") or self.session.bound.get("schema")):
                return {"ok": False, "error": "schema_mismatch"}
            count = output.get("emitted", output.get("frames"))
            if output.get("emit_errors") != 0 or type(count) is not int or count <= 0:
                return {"ok": False, "error": "legacy_output_fault_or_missing"}
            boot = info.get("boot_id")
            if type(boot) is not int or boot <= 0:
                return {"ok": False, "error": "legacy_boot_missing"}
            if previous and (previous[0] != boot or count <= previous[1]):
                return {"ok": False, "error": "legacy_output_not_advancing_or_reboot"}
            previous = (boot, count)
        return {"ok": True, "info": info, "faults": 0, "completed_count": count,
                "session": self.session.token, "physical_qualified": False}

    def _native(self, request: dict) -> dict:
        """Replacement opcodes on the broker transport. Live scene replies are not saves."""
        err = self._require_session(request)
        if err:
            return {"ok": False, "error": err}
        admitted, reason = hardware_writes_admitted(self.evidence, self.session.bound)
        if not admitted:
            return {"ok": False, "error": "ADMISSION_HOLD", "admit_reason": reason, "session": self.session.token}
        rec = load_approval_record(self.evidence) or {}
        op = request["op"]
        if op not in rec.get("ops", []):
            return {"ok": False, "error": "ops_denied", "admit_reason": op}
        if op == "native_identity":
            return self._native_identity()
        if op == "native_status":
            return self._native_status()
        if op == "native_health":
            return self._native_health()
        if op == "native_scene":
            return self._native_scene(request)
        if op == "native_global":
            return self._native_global(request)
        if op == "native_durable":
            return self._native_durable(request)
        if op == "native_durable_read":
            return self._native_durable_read(request)
        return {"ok": False, "error": "forbidden_op", "op": op}

    def _native_result(self, got: dict) -> dict | None:
        if got.get("ok"):
            return None
        status = got.get("status")
        if status == 5:
            error = "stale"
        elif status == 6:
            error = "wrong_boot"
        elif status == 3:
            error = "rejected"
        else:
            error = "rejected"
        return {"ok": False, "error": error, "status": status, "replayed": False, "session": self.session.token}

    def _native_identity(self) -> dict:
        got = self._transact(NATIVE_OPCODES["native_identity"], b"")
        failed = self._native_result(got)
        if failed:
            return failed
        try:
            identity = json.loads(got["body"].decode())
        except (UnicodeDecodeError, json.JSONDecodeError, AttributeError):
            return {"ok": False, "error": "identity_json", "session": self.session.token}
        if identity.get("uid") != self.session.bound.get("uid") or identity.get("durable_commit") != "explicit-v1":
            return {"ok": False, "error": "identity_mismatch", "session": self.session.token}
        if identity.get("controls_persistent") is not False:
            return {"ok": False, "error": "persistence", "session": self.session.token}
        caps = identity.get("capabilities") or {}
        if caps.get("controls") != "replacement-control-v1" or caps.get("health") != "replacement-health-v1":
            return {"ok": False, "error": "capabilities", "session": self.session.token}
        self.session.bound = dict(self.session.bound, boot=None)
        return {"ok": True, "identity": identity, "saved": False, "session": self.session.token}

    def _native_status(self) -> dict:
        got = self._transact(NATIVE_OPCODES["native_status"], b"")
        failed = self._native_result(got)
        if failed:
            return failed
        body = got.get("body") or b""
        if len(body) != 112:
            return {"ok": False, "error": "status_length", "session": self.session.token}
        version, boot, bound, target = struct.unpack_from("<4I", body)
        if version != 1 or bound not in (0, 1) or target not in (0, 1):
            return {"ok": False, "error": "status_flags", "session": self.session.token}
        global_revision, brightness, output_limit = struct.unpack_from("<QIf", body, 48)
        scenes = []
        for offset in (64, 88):
            revision, mode, palette, gain, response_ms = struct.unpack_from("<QIIff", body, offset)
            scenes.append({"revision": revision, "mode": mode, "palette": palette, "gain": gain, "response_ms": response_ms})
        self._boot = boot
        return {"ok": True, "boot": boot, "bound": bool(bound), "target_matches": bool(target),
                "global_revision": global_revision, "brightness": brightness, "output_limit": output_limit,
                "scenes": scenes, "saved": False, "session": self.session.token}

    def _read_health(self) -> dict:
        got = self._transact(NATIVE_OPCODES["native_health"], b"")
        failed = self._native_result(got)
        if failed:
            return failed
        try:
            snapshot = json.loads(got["body"].decode())
        except (UnicodeDecodeError, json.JSONDecodeError, AttributeError):
            return {"ok": False, "error": "health_json", "session": self.session.token}
        output = snapshot.get("output") if isinstance(snapshot, dict) else None
        startup = snapshot.get("startup") if isinstance(snapshot, dict) else None
        if (not isinstance(snapshot, dict) or snapshot.get("schema") != "replacement-health-v1"
                or snapshot.get("physical_qualified") is not False
                or not isinstance(startup, dict) or startup.get("ready") != 1
                or startup.get("failures") != 0 or startup.get("last_driver_error") != 0
                or not isinstance(output, dict) or output.get("first_fault") != 0 or output.get("failures") != 0
                or type(output.get("completed_count")) is not int or output.get("completed_count") <= 0):
            return {"ok": False, "error": "health_rejected", "session": self.session.token}
        now = snapshot.get("snapshot_us")
        last = output.get("last_completed_us")
        if type(now) is not int or type(last) is not int or last <= 0 or now < last or now - last > 10_000_000:
            return {"ok": False, "error": "stale_output", "session": self.session.token,
                    "snapshot_us": now, "last_completed_us": last,
                    "completion_age_us": now - last if type(now) is int and type(last) is int else None,
                    "completed_count": output["completed_count"], "boot": snapshot.get("boot_id")}
        return {"ok": True, "snapshot": snapshot, "completed_count": output["completed_count"]}

    def _native_health(self) -> dict:
        first = self._read_health()
        if not first.get("ok"):
            return first
        # A complete native wire pair takes at least 10.334 ms. Allow two
        # pairs between observations; consecutive USB replies can be <1 ms.
        time.sleep(0.025)
        second = self._read_health()
        if not second.get("ok"):
            return second
        boot = first["snapshot"].get("boot_id")
        if type(boot) is not int or boot <= 0 or second["snapshot"].get("boot_id") != boot:
            return {"ok": False, "error": "health_boot_changed", "session": self.session.token}
        if second["completed_count"] <= first["completed_count"]:
            return {"ok": False, "error": "output_not_advancing", "session": self.session.token,
                    "completed_count": [first["completed_count"], second["completed_count"]]}
        self.last_health_snapshot = second["snapshot"]  # full record for the broker's minute log
        return {"ok": True, "completed_count": second["completed_count"], "faults": 0, "boot": boot,
                "physical_qualified": False, "saved": False, "session": self.session.token}

    def _require_boot(self, request: dict) -> dict | None:
        if request.get("boot") != getattr(self, "_boot", None):
            return {"ok": False, "error": "wrong_boot", "replayed": False, "session": self.session.token}
        return None

    def _ensure_bound(self) -> dict | None:
        """Opcode 0x50524202. Status 6 means unbound, so this must precede every write and read."""
        boot = getattr(self, "_boot", None)
        if not boot:
            return {"ok": False, "error": "unbound", "status": 6, "replayed": False, "session": self.session.token}
        if getattr(self, "_bound_boot", None) == boot:
            return None
        payload = struct.pack("<II32s", 1, int(boot), TARGET_UID)
        if len(payload) != 40 or len(TARGET_UID) != 32:
            return {"ok": False, "error": "bind_length", "replayed": False, "session": self.session.token}
        got = self._transact(BIND_OPCODE, payload)
        failed = self._native_result(got)
        if failed:
            failed["replayed"] = False
            failed["saved"] = False
            return failed
        body = got.get("body") or b""
        if len(body) != 112:
            return {"ok": False, "error": "status_length", "replayed": False, "saved": False, "session": self.session.token}
        version, read_boot, bound, target = struct.unpack_from("<4I", body)
        if version != 1 or bound != 1 or target != 1 or read_boot != boot:
            return {"ok": False, "error": "unbound", "status": 6, "replayed": False, "saved": False, "session": self.session.token}
        self._bound_boot = boot
        return None

    def _native_scene(self, request: dict) -> dict:
        refused = self._require_boot(request)
        if refused:
            return refused
        unbound = self._ensure_bound()
        if unbound:
            return unbound
        scene = request.get("scene")
        if scene not in (0, 1) or request.get("mode") not in (1, 7):
            return {"ok": False, "error": "scene_fields", "session": self.session.token}
        payload = struct.pack("<IIIQIIffI", 1, request["boot"], scene, int(request["expected_revision"]),
                              request["mode"], int(request["palette"]), float(request["gain"]),
                              float(request["response_ms"]), int(bool(request.get("reset", 0))))
        got = self._transact(NATIVE_OPCODES["native_scene"], payload)
        failed = self._native_result(got)
        if failed:
            return failed
        return {"ok": True, "scene": scene, "saved": False, "disposition": "acknowledged", "session": self.session.token}

    def _native_global(self, request: dict) -> dict:
        refused = self._require_boot(request)
        if refused:
            return refused
        unbound = self._ensure_bound()
        if unbound:
            return unbound
        brightness = request.get("brightness")
        if type(brightness) is not int or not 0 <= brightness <= 255:
            return {"ok": False, "error": "brightness", "session": self.session.token}
        payload = struct.pack("<IIQIf", 1, request["boot"], int(request["expected_revision"]), brightness, float(request["output_limit"]))
        got = self._transact(NATIVE_OPCODES["native_global"], payload)
        failed = self._native_result(got)
        if failed:
            return failed
        return {"ok": True, "saved": False, "disposition": "acknowledged", "session": self.session.token}

    def _native_durable(self, request: dict) -> dict:
        refused = self._require_boot(request)
        if refused:
            return refused
        try:
            unbound = self._ensure_bound()
        except RestrictedControlError as exc:
            return {"ok": False, "error": "ambiguous_not_saved", "detail": exc.code, "replayed": False, "saved": False, "session": self.session.token}
        if unbound:
            return unbound
        scenes = request.get("expected_scene_revisions")
        if not isinstance(scenes, list) or len(scenes) != 2:
            return {"ok": False, "error": "revisions", "session": self.session.token}
        payload = struct.pack("<IIQQQ", 1, request["boot"], int(scenes[0]), int(scenes[1]), int(request["expected_global_revision"]))
        try:
            got = self._transact(NATIVE_OPCODES["native_durable"], payload)
        except RestrictedControlError as exc:
            return {"ok": False, "error": "ambiguous_not_saved", "detail": exc.code, "replayed": False, "session": self.session.token}
        failed = self._native_result(got)
        if failed:
            failed["saved"] = False
            failed["replayed"] = False
            return failed
        body = got.get("body") or b""
        if len(body) != 16:
            return {"ok": False, "error": "durable_length", "saved": False, "replayed": False, "session": self.session.token}
        version, disposition, generation, stored = struct.unpack("<IIII", body)
        if version != 1 or disposition != 0:
            return {"ok": False, "error": "not_durable", "saved": False, "replayed": False, "session": self.session.token}
        return {"ok": True, "saved": True, "disposition": "durable", "generation": generation,
                "stored_brightness": stored, "replayed": False, "session": self.session.token}

    def _native_durable_read(self, request: dict) -> dict:
        """Opcode 0x50524207. Eight-byte request, 64-byte version-2 snapshot. Not live status and not a save."""
        refused = self._require_boot(request)
        if refused:
            return refused
        try:
            unbound = self._ensure_bound()
        except RestrictedControlError as exc:
            return {"ok": False, "error": "ambiguous_not_saved", "detail": exc.code, "replayed": False,
                    "saved": False, "not_live_status": True, "session": self.session.token}
        if unbound:
            unbound.update(saved=False, replayed=False, not_live_status=True)
            return unbound
        try:
            got = self._transact(NATIVE_OPCODES["native_durable_read"], struct.pack("<II", 1, request["boot"]))
        except RestrictedControlError as exc:
            return {"ok": False, "error": "ambiguous_not_saved", "detail": exc.code, "replayed": False,
                    "saved": False, "not_live_status": True, "session": self.session.token}
        failed = self._native_result(got)
        if failed:
            failed.update(saved=False, replayed=False, not_live_status=True)
            return failed
        body = got.get("body") or b""
        if len(body) != 64:
            return {"ok": False, "error": "durable_read_length", "saved": False, "replayed": False,
                    "not_live_status": True, "session": self.session.token}
        version, present, generation = struct.unpack_from("<III", body, 0)
        if version != 2 or present not in (0, 1):
            return {"ok": False, "error": "durable_read_rejected", "saved": False, "replayed": False,
                    "not_live_status": True, "session": self.session.token}
        scenes, brightness, limit = [], None, None
        if present:
            magic, snap_version = struct.unpack_from("<II", body, 12)
            if magic != 0x50534331 or snap_version != 2:
                return {"ok": False, "error": "durable_composition", "saved": False, "replayed": False,
                        "not_live_status": True, "session": self.session.token}
            brightness, limit = struct.unpack_from("<If", body, 52)
            if isinstance(brightness, bool) or not isinstance(brightness, int) or not 0 <= brightness <= 255 or not math.isfinite(limit) or not 0 <= limit <= 1:
                return {"ok": False, "error": "durable_read_rejected", "saved": False, "replayed": False,
                        "not_live_status": True, "session": self.session.token}
            for index in range(2):
                mode, palette, gain, response_ms = struct.unpack_from("<IIff", body, 20 + index * 16)
                if mode not in (1, 7) or isinstance(palette, bool) or not isinstance(palette, int) or not 0 <= palette <= 255 or not math.isfinite(gain) or not 0 <= gain <= 1 or not math.isfinite(response_ms) or not 1 <= response_ms <= 1000:
                    return {"ok": False, "error": "durable_read_rejected", "saved": False, "replayed": False,
                            "not_live_status": True, "session": self.session.token}
                scenes.append({"mode": mode, "palette": palette, "gain": gain, "response_ms": response_ms})
        return {"ok": True, "disposition": "durable_read", "present": bool(present), "generation": generation,
                "stored_brightness": brightness, "output_limit": limit, "scenes": scenes,
                "not_live_status": True, "saved": False, "replayed": False, "session": self.session.token}

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
        "inhouse_sensitivity", "inhouse_square_iter", "inhouse_injection_photons", "waveform_family_supported",
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
        self._connections = set()

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
                # Two connections maximum: one persistent controller and one
                # canonical-lease maintenance handshake. Hello still admits both.
                if len(self._connections) >= 2:
                    try:
                        conn.sendall(b'{"ok":false,"error":"writer_busy"}\n')
                        conn.close()
                    except OSError:
                        pass
                    continue
                self._connections.add(conn_id)
            threading.Thread(target=self._serve, args=(conn, conn_id), daemon=True).start()

    def _serve(self, conn: socket.socket, conn_id: str) -> None:
        buf = b""
        conn.settimeout(2.0)
        admitted_connection = False
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
                            if not admitted_connection and request.get('op') != 'hello':
                                reply = {'ok': False, 'error': 'hello_required'}
                            else:
                                reply = self.plane.handle(request, conn_id=conn_id)
                            if request.get('op') == 'hello' and reply.get('ok') is True:
                                admitted_connection = True
                                conn.settimeout(WRITER_RECV_TIMEOUT)
                    except (json.JSONDecodeError, UnicodeDecodeError, TypeError):
                        reply = {"ok": False, "error": "bad_json"}
                    conn.sendall((json.dumps(reply) + "\n").encode())
                    if not admitted_connection:
                        return
        except OSError:
            return
        finally:
            with self._conn_lock:
                self._connections.discard(conn_id)
                if self.plane.maintenance_id == conn_id:
                    self.plane.maintenance_id = self.plane.maintenance_token = self.plane.maintenance_digest = None
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
