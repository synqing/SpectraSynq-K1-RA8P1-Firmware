"""Python oracle for schema-2 Titan snapshots. Same fail-closed rules as titan_parser.js."""
from __future__ import annotations

from typing import Any

UNAVAILABLE = "UNAVAILABLE"
MAX_SAFE = 9007199254740991
MODE_LABEL = [
    "DISCONNECTED",
    "IDENTIFYING",
    "READY",
    "LIVE",
    "TESTING",
    "COMPLETED",
    "FAULT",
    "REPLAY",
    "DEBUG HALTED",
    "STALE",
]
RESULT_LABEL = ["NONE", "RUNNING", "PASSED", "FAILED", "NOT SUPPORTED"]
MAPPING = "Lane A / Lane B — U13/U14 mapping unresolved"
ORIGIN_LIVE = "LIVE OBSERVATION"
ORIGIN_SIM = "SIMULATED"
ORIGIN_REPLAY = "REPLAY"
ORIGINS = (ORIGIN_LIVE, ORIGIN_SIM, ORIGIN_REPLAY)
SCOPE_CURRENT = "CURRENT"
SCOPE_HISTORICAL = "HISTORICAL"
SCOPE_UNIDENTIFIED = "UNIDENTIFIED"
SCHEMA3_EXTRA = (
    "origin",
    "identity_scope",
    "device_age_ms",
    "device_progress",
    "connection_epoch",
    "timing_valid",
    "rate_valid",
    "led_valid",
    "frozen_device",
    "last_hop_peak",
    "frame_a_crc",
    "frame_b_crc",
)


def _own(obj: dict, key: str) -> bool:
    return key in obj


def _is_bool01(v: Any) -> bool:
    return v is True or v is False or v == 0 or v == 1


def _as01(v: Any) -> int:
    return 1 if v is True or v == 1 else 0


def _is_int(v: Any) -> bool:
    return isinstance(v, int) and not isinstance(v, bool)


def _is_safe_int(v: Any) -> bool:
    return _is_int(v) and 0 <= v <= MAX_SAFE


def _req_flag(obj: dict, key: str) -> int | None:
    if not _own(obj, key) or not _is_bool01(obj[key]):
        return None
    return _as01(obj[key])


def _req_enum(obj: dict, key: str, max_v: int) -> int | None:
    if not _own(obj, key) or not _is_int(obj[key]) or obj[key] < 0 or obj[key] > max_v:
        return None
    return obj[key]


def _req_count(obj: dict, key: str) -> int | None:
    if not _own(obj, key):
        return None
    if not _is_int(obj[key]) or obj[key] < 0 or obj[key] > MAX_SAFE:
        return None
    return obj[key]


def _req_finite(obj: dict, key: str) -> float | None:
    if not _own(obj, key):
        return None
    v = obj[key]
    if isinstance(v, bool) or not isinstance(v, (int, float)):
        return None
    if v != v or v in (float("inf"), float("-inf")):
        return None
    return float(v)


def _req_string(obj: dict, key: str) -> str | None:
    if not _own(obj, key) or not isinstance(obj[key], str):
        return None
    s = obj[key].strip()
    return None if s == "" else s


def _seq_string(obj: dict) -> str | None:
    if not _own(obj, "sequence"):
        return None
    v = obj["sequence"]
    if isinstance(v, str) and v.strip() != "":
        return v.strip()
    if _is_safe_int(v):
        return str(v)
    return None


def _measured(obj: dict, valid: int, key: str, kind: str):
    if not valid:
        return UNAVAILABLE
    if kind == "count":
        return _req_count(obj, key)
    if kind == "finite":
        return _req_finite(obj, key)
    return None


def decode(obj: dict, last_seq: str = "", same_seq: int = 0) -> tuple[list | None, str, int]:
    """Return (cells, last_seq, same_seq). cells is None when the snapshot is rejected."""
    if not isinstance(obj, dict):
        return None, last_seq, same_seq
    schema = _req_enum(obj, "schema", 100)
    if schema == 3:
        return _decode_schema3(obj, last_seq, same_seq)
    mode = _req_enum(obj, "mode", 9)
    identity_ok = _req_flag(obj, "identity_ok")
    protocol = _req_enum(obj, "protocol", 1)
    simulated = _req_flag(obj, "simulated")
    health_valid = _req_flag(obj, "health_valid")
    mic_a_valid = _req_flag(obj, "mic_a_valid")
    mic_b_valid = _req_flag(obj, "mic_b_valid")
    if (
        schema != 2
        or mode is None
        or identity_ok is None
        or protocol != 1
        or simulated is None
        or health_valid is None
        or mic_a_valid is None
        or mic_b_valid is None
    ):
        return None, last_seq, same_seq

    uid = _req_string(obj, "uid")
    build = _req_string(obj, "build")
    source = _req_string(obj, "source")
    contract = _req_string(obj, "contract")
    if identity_ok:
        if not uid or not build or not source or not contract:
            return None, last_seq, same_seq
    else:
        uid = uid or "UNIDENTIFIED"
        build = build or "UNIDENTIFIED"
        source = source or "UNIDENTIFIED"
        contract = contract or "UNIDENTIFIED"

    hop_max = _measured(obj, health_valid, "hop_max_us", "finite")
    late = _measured(obj, health_valid, "late_starts", "count")
    deadlines = _measured(obj, health_valid, "deadlines", "count")
    crc = _measured(obj, health_valid, "crc_mismatches", "count")
    dma = _measured(obj, health_valid, "dma_irqs", "count")
    latched = _measured(obj, health_valid, "latched_frames", "count")
    led_faults = _measured(obj, health_valid, "led_faults", "count")
    if None in (hop_max, late, deadlines, crc, dma, latched, led_faults):
        return None, last_seq, same_seq
    if isinstance(hop_max, (int, float)) and hop_max < 0:
        return None, last_seq, same_seq

    rms_a = _measured(obj, mic_a_valid, "mic_a_rms_dbfs", "finite")
    rms_b = _measured(obj, mic_b_valid, "mic_b_rms_dbfs", "finite")
    if rms_a is None or rms_b is None:
        return None, last_seq, same_seq

    rate = _measured(obj, health_valid, "capture_rate_hz", "finite")
    if rate is None:
        return None, last_seq, same_seq
    if isinstance(rate, (int, float)) and rate <= 0:
        return None, last_seq, same_seq

    hop_samples = _measured(obj, health_valid, "hop_samples", "count")
    admitted = _measured(obj, health_valid, "admitted_rate_hz", "finite")
    if hop_samples is None or admitted is None:
        return None, last_seq, same_seq
    if isinstance(hop_samples, (int, float)) and hop_samples <= 0:
        return None, last_seq, same_seq
    if isinstance(admitted, (int, float)) and admitted <= 0:
        return None, last_seq, same_seq

    gate_result = _req_enum(obj, "gate_result", 4) if _own(obj, "gate_result") else 0
    if gate_result is None:
        return None, last_seq, same_seq

    sequence = _seq_string(obj)
    if not sequence:
        return None, last_seq, same_seq

    if sequence == last_seq:
        same_seq += 1
    else:
        same_seq = 0
        last_seq = sequence

    frozen = same_seq >= 8
    led_health = "FAULT" if health_valid and led_faults > 0 else ("OK" if health_valid else UNAVAILABLE)
    banner = "SIMULATED DATA — TITAN NOT CONNECTED" if simulated else "LIVE TITAN"
    if frozen:
        banner += " / FROZEN SEQUENCE"

    cells = [
        banner,
        MODE_LABEL[mode],
        RESULT_LABEL[gate_result],
        uid,
        build,
        source,
        contract,
        identity_ok,
        _js_number(rms_a),
        _js_number(rms_b),
        mic_a_valid,
        mic_b_valid,
        _js_number(rate),
        _js_number(hop_max),
        late,
        deadlines,
        crc,
        led_health,
        dma,
        latched,
        led_faults,
        hop_samples,
        _js_number(admitted),
        sequence,
        health_valid,
        simulated,
        MAPPING,
    ]
    return cells, last_seq, same_seq


def _js_number(v):
    if isinstance(v, float) and v.is_integer():
        return int(v)
    return v


def _token(obj: dict, key: str, allowed: tuple[str, ...]) -> str | None:
    got = _req_string(obj, key)
    if got is None or got not in allowed:
        return None
    return got


def _decode_schema3(obj: dict, last_seq: str, same_seq: int) -> tuple[list | None, str, int]:
    mode = _req_enum(obj, "mode", 9)
    identity_ok = _req_flag(obj, "identity_ok")
    protocol = _req_enum(obj, "protocol", 1)
    origin = _token(obj, "origin", ORIGINS)
    scope = _token(obj, "identity_scope", (SCOPE_CURRENT, SCOPE_HISTORICAL, SCOPE_UNIDENTIFIED))
    mic_a_valid = _req_flag(obj, "mic_a_valid")
    mic_b_valid = _req_flag(obj, "mic_b_valid")
    timing_valid = _req_flag(obj, "timing_valid")
    rate_valid = _req_flag(obj, "rate_valid")
    led_valid = _req_flag(obj, "led_valid")
    frozen = _req_flag(obj, "frozen_device")
    device_age = _req_finite(obj, "device_age_ms")
    epoch = _req_count(obj, "connection_epoch")
    if None in (
        mode,
        identity_ok,
        protocol,
        origin,
        scope,
        mic_a_valid,
        mic_b_valid,
        timing_valid,
        rate_valid,
        led_valid,
        frozen,
        device_age,
        epoch,
    ):
        return None, last_seq, same_seq
    if protocol != 1 or device_age < 0:
        return None, last_seq, same_seq
    if origin == ORIGIN_REPLAY:
        mode = 7
        scope = SCOPE_HISTORICAL
    simulated = 1 if origin == ORIGIN_SIM else 0
    uid = _req_string(obj, "uid")
    build = _req_string(obj, "build")
    source = _req_string(obj, "source")
    contract = _req_string(obj, "contract")
    if identity_ok:
        if not uid or not build or not source or not contract:
            return None, last_seq, same_seq
        if scope == SCOPE_UNIDENTIFIED:
            return None, last_seq, same_seq
    else:
        uid = uid or "UNIDENTIFIED"
        build = build or "UNIDENTIFIED"
        source = source or "UNIDENTIFIED"
        contract = contract or "UNIDENTIFIED"

    hop_max = (
        _measured(obj, 1, "hop_max_us", "finite")
        if _own(obj, "hop_max_us")
        else UNAVAILABLE
    )
    late = _measured(obj, timing_valid, "late_starts", "count")
    deadlines = _measured(obj, timing_valid, "deadlines", "count")
    crc = _measured(obj, timing_valid, "crc_mismatches", "count")
    dma = (
        UNAVAILABLE
        if led_valid and not _own(obj, "dma_irqs")
        else _measured(obj, led_valid, "dma_irqs", "count")
    )
    latched = _measured(obj, led_valid, "latched_frames", "count")
    led_faults = _measured(obj, led_valid, "led_faults", "count")
    rms_a = _measured(obj, mic_a_valid, "mic_a_rms_dbfs", "finite")
    rms_b = _measured(obj, mic_b_valid, "mic_b_rms_dbfs", "finite")
    rate = _measured(obj, rate_valid, "capture_rate_hz", "finite")
    hop_samples = _measured(obj, identity_ok, "hop_samples", "count")
    admitted = _measured(obj, identity_ok, "admitted_rate_hz", "finite")
    if None in (hop_max, late, deadlines, crc, dma, latched, led_faults, rms_a, rms_b, rate, hop_samples, admitted):
        return None, last_seq, same_seq
    if isinstance(hop_max, (int, float)) and hop_max < 0:
        return None, last_seq, same_seq
    if isinstance(rate, (int, float)) and rate <= 0:
        return None, last_seq, same_seq
    if isinstance(hop_samples, (int, float)) and hop_samples <= 0:
        return None, last_seq, same_seq
    if isinstance(admitted, (int, float)) and admitted <= 0:
        return None, last_seq, same_seq

    progress = obj.get("device_progress", UNAVAILABLE)
    if identity_ok and "device_progress" in obj:
        if _req_count(obj, "device_progress") is None and obj.get("device_progress") != UNAVAILABLE:
            return None, last_seq, same_seq
        if _req_count(obj, "device_progress") is not None:
            progress = _req_count(obj, "device_progress")
    else:
        progress = UNAVAILABLE

    gate_result = _req_enum(obj, "gate_result", 4) if _own(obj, "gate_result") else 0
    if gate_result is None:
        return None, last_seq, same_seq
    sequence = _seq_string(obj)
    if not sequence:
        return None, last_seq, same_seq

    health_valid = 1 if timing_valid or rate_valid or led_valid else 0
    led_health = "FAULT" if led_valid and led_faults > 0 else ("OK" if led_valid else UNAVAILABLE)
    if origin == ORIGIN_REPLAY:
        banner = ORIGIN_REPLAY
        mode_label = "REPLAY"
    elif origin == ORIGIN_SIM:
        banner = "SIMULATED DATA — TITAN NOT CONNECTED"
        mode_label = MODE_LABEL[mode]
    else:
        banner = ORIGIN_LIVE
        mode_label = MODE_LABEL[mode]
    if frozen:
        banner += " / FROZEN DEVICE"

    cells = [
        banner,
        mode_label,
        RESULT_LABEL[gate_result],
        uid,
        build,
        source,
        contract,
        identity_ok,
        _js_number(rms_a),
        _js_number(rms_b),
        mic_a_valid,
        mic_b_valid,
        _js_number(rate),
        _js_number(hop_max),
        late,
        deadlines,
        crc,
        led_health,
        dma,
        latched,
        led_faults,
        hop_samples,
        _js_number(admitted),
        sequence,
        health_valid,
        simulated,
        MAPPING,
        origin,
        scope,
        _js_number(device_age),
        progress if progress == UNAVAILABLE else _js_number(progress),
        epoch,
        timing_valid,
        rate_valid,
        led_valid,
        frozen,
        _optional_count(obj, "last_hop_peak"),
        _optional_count(obj, "frame_a_crc"),
        _optional_count(obj, "frame_b_crc"),
    ]
    return cells, sequence, same_seq


def _optional_count(obj: dict, key: str):
    if not _own(obj, key) or obj.get(key) == UNAVAILABLE:
        return UNAVAILABLE
    got = _req_count(obj, key)
    return UNAVAILABLE if got is None else got


def cells_to_csv(cells: list) -> str:
    out = []
    for cell in cells:
        if cell is None:
            return ""
        text = str(cell)
        if "," in text or "\n" in text or "\r" in text:
            return ""
        out.append(text)
    return ",".join(out)
