#!/usr/bin/env python3
"""Normalize live Titan observations onto the shared titan_decode path."""
from __future__ import annotations

import re
import time

from titan_decode import (
    ORIGIN_LIVE,
    ORIGIN_REPLAY,
    ORIGIN_SIM,
    SCOPE_CURRENT,
    SCOPE_HISTORICAL,
    SCOPE_UNIDENTIFIED,
    UNAVAILABLE,
    cells_to_csv,
    decode,
)

CONTRACT_RE = re.compile(r"^sr(\d+)\.hop(\d+)\.bins(\d+)\.xover(\d+)$")
EXPECTED_UID = "545433931bd25436593630352d068363"
STALE_MS = 2000


def parse_contract(text: str) -> dict | None:
    match = CONTRACT_RE.match(text or "")
    if not match:
        return None
    return {
        "admitted_rate_hz": int(match.group(1)),
        "hop_samples": int(match.group(2)),
        "bins": int(match.group(3)),
        "xover": int(match.group(4)),
    }


def identity_identified(info: dict, expected_uid: str = EXPECTED_UID) -> tuple[bool, str]:
    if not isinstance(info, dict):
        return False, "INFO is not an object"
    if info.get("protocol") != 1:
        return False, f"unsupported protocol {info.get('protocol')}"
    if info.get("uid") != expected_uid:
        return False, f"UID mismatch {info.get('uid')!r}"
    for key in ("build", "source", "contract"):
        if not isinstance(info.get(key), str) or not info[key].strip():
            return False, f"missing {key}"
    if parse_contract(info["contract"]) is None:
        return False, f"unparsed contract {info.get('contract')!r}"
    return True, "identified"


def identity_ok(
    info: dict,
    expected_uid: str = EXPECTED_UID,
    checkpoint: dict | None = None,
) -> tuple[bool, str]:
    identified, reason = identity_identified(info, expected_uid)
    if not identified:
        return False, reason
    if not isinstance(checkpoint, dict) or not str(checkpoint.get("build") or "").strip():
        return False, "no accepted checkpoint bound"
    if info["build"] != checkpoint["build"]:
        return False, f"build not accepted {info['build']!r}"
    if checkpoint.get("source") and info["source"] != checkpoint["source"]:
        return False, f"source not accepted {info['source']!r}"
    if checkpoint.get("contract") and info["contract"] != checkpoint["contract"]:
        return False, f"contract not accepted {info['contract']!r}"
    if checkpoint.get("uid") and info["uid"] != checkpoint["uid"]:
        return False, f"UID not accepted {info['uid']!r}"
    return True, "ok"


def bind_identity(
    info: dict,
    expected_uid: str = EXPECTED_UID,
    checkpoint: dict | None = None,
) -> dict:
    identified, id_reason = identity_identified(info, expected_uid)
    ok, reason = identity_ok(info, expected_uid, checkpoint)
    visible = identified
    contract = parse_contract(str(info.get("contract") or "")) if identified else None
    return {
        "ok": ok,
        "identified": identified,
        "accepted": ok,
        "reason": reason,
        "uid": info.get("uid") if visible else None,
        "build": info.get("build") if visible else None,
        "source": info.get("source") if visible else None,
        "contract": info.get("contract") if visible else None,
        "protocol": info.get("protocol"),
        "clock_hz": info.get("clock_hz"),
        "device_sequence": info.get("sequence"),
        "hop_samples": contract["hop_samples"] if contract else None,
        "admitted_rate_hz": contract["admitted_rate_hz"] if contract else None,
        "unsupported": False,
    }


def rms_unavailable() -> None:
    """Lifetime square-sum is not a hop RMS. Do not invent dBFS."""
    return None


def snapshot(
    *,
    origin: str,
    bound: dict,
    epoch: int,
    display_seq: int,
    device_age_ms: float,
    device_progress,
    frozen: int,
    metrics: dict | None,
    palette: dict | None,
    identity_scope: str | None = None,
    gate_result: int = 0,
    mode: int | None = None,
) -> dict:
    ok = bool(bound.get("ok"))
    identified = bool(bound.get("identified", ok))
    scope = identity_scope or (
        SCOPE_CURRENT if ok and origin == ORIGIN_LIVE
        else SCOPE_HISTORICAL if identified
        else SCOPE_UNIDENTIFIED
    )
    if origin == ORIGIN_REPLAY:
        scope = SCOPE_HISTORICAL
        mode = 7
    elif not ok:
        scope = SCOPE_UNIDENTIFIED
        mode = 1 if mode is None else mode
    elif mode is None:
        mode = 3 if origin == ORIGIN_LIVE else 3

    pdm = (metrics or {}).get("pdm_target") if isinstance(metrics, dict) else None
    rate = None
    rate_valid = 0
    if isinstance(pdm, dict) and pdm.get("measured_hz") and pdm.get("rate_locked"):
        try:
            rate = float(pdm["measured_hz"])
            if rate > 0:
                rate_valid = 1
        except (TypeError, ValueError):
            rate = None
            rate_valid = 0

    led_valid = 0
    led_faults = None
    latched = None
    if isinstance(palette, dict) and "emit_errors" in palette:
        try:
            led_faults = int(palette.get("emit_errors") or 0)
            latched = int(palette.get("emitted") if palette.get("emitted") is not None else palette.get("frames") or 0)
            led_valid = 1
        except (TypeError, ValueError):
            led_valid = 0

    row = {
        "schema": 3,
        "origin": origin,
        "identity_scope": scope,
        "protocol": 1,
        "mode": mode,
        "identity_ok": 1 if ok else 0,
        "identity_identified": 1 if identified else 0,
        "mic_a_valid": 0,
        "mic_b_valid": 0,
        "timing_valid": 0,
        "rate_valid": rate_valid,
        "led_valid": led_valid,
        "frozen_device": 1 if frozen else 0,
        "device_age_ms": int(device_age_ms) if device_age_ms >= 0 else 0,
        "connection_epoch": int(epoch),
        "sequence": str(display_seq),
        "gate_result": gate_result,
    }
    if identified:
        row.update(
            uid=bound["uid"],
            build=bound["build"],
            source=bound["source"],
            contract=bound["contract"],
            hop_samples=bound["hop_samples"],
            admitted_rate_hz=bound["admitted_rate_hz"],
        )
    if device_progress is None:
        row["device_progress"] = UNAVAILABLE
    else:
        row["device_progress"] = int(device_progress)
    if rate_valid:
        row["capture_rate_hz"] = rate
    if led_valid:
        row["led_faults"] = led_faults
        row["latched_frames"] = latched
        # Emitter DWT, not AP hop_max_us. Schema-3 hop_max stays absent unless
        # the AP metrics actually own hop_max_us.
        if palette.get("last_emit_cycles") is not None:
            try:
                row["last_emit_cycles"] = int(palette.get("last_emit_cycles"))
            except (TypeError, ValueError):
                pass
        try:
            row["frame_a_crc"] = int(palette.get("frame_a_crc") or 0)
            row["frame_b_crc"] = int(palette.get("frame_b_crc") or 0)
        except (TypeError, ValueError):
            pass
    if isinstance(pdm, dict) and pdm.get("last_hop_peak") is not None:
        try:
            row["last_hop_peak"] = int(pdm.get("last_hop_peak") or 0)
        except (TypeError, ValueError):
            pass
    return row


def encode(row: dict) -> str:
    cells, _, _ = decode(row, "", 0)
    if cells is None:
        return ""
    return cells_to_csv(cells)


def now() -> float:
    return time.monotonic()
