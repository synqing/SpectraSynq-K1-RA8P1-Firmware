#!/usr/bin/env python3
"""Bounded A8 current-image experiment entry.

Dry-run is the default. Emit-off observation reads identity, metrics and
palette status. It does not enable the paired transmitter. The sequence
command watches an image that is already emitting on P604. It does not
enable light, does not send the mapping opcode, and does not change volume.

This is not scripts/run_mode32_real_audio.py. That observation script opens
the serial port directly, runs an unbounded lsof check, and forces emission
on. It is not an acceptance scorer and must not be used on the paired A8 image.

This entry does not add an emission switch to titan_live_control.py. A missing
metric stays UNKNOWN. A measured zero stays zero. No test in the companion
file opens the real CDC device.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
import sys
import time
from pathlib import Path
from typing import Any, Callable, Mapping

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "scripts"))

import live_lease  # noqa: E402
import live_protocol as proto  # noqa: E402
from a8_platform_metrics import UNKNOWN, extract_a8_counters  # noqa: E402

OP_METRICS = 6
OP_PALETTE_STATUS = 17
OP_SNAPSHOT = 23
OP_CONFIG = 25
OP_PAIR_FIXTURE = 27
CONFIG_WRITE_SUBS = {3, 4, 5, 6}
CONFIG_READ_SUBS = {1, 2}
EMIT_OFF_READ_OPS = {1, OP_METRICS, OP_PALETTE_STATUS, OP_SNAPSHOT, OP_CONFIG}
PAIR_LANE_BYTES = 480
PAIR_FIXTURE_BYTES = 24 + (2 * PAIR_LANE_BYTES)
KIND_MAP = 1
KIND_BLACK = 2
FIXTURE_OK = 0
FIXTURE_SHAPE = 1
FIXTURE_PARTIAL = 2
FIXTURE_PROFILE = 3
FIXTURE_GENERATION = 4
FIXTURE_EPOCH = 5
FIXTURE_STALE = 6
FIXTURE_IN_FLIGHT = 7
FIXTURE_KIND = 8
FIXTURE_NOT_BLACK = 9
PAIR_FIELDS = (
    "configured_backend",
    "output_backend",
    "emit_enabled",
    "wire_profile",
    "din_a",
    "din_b",
    "pixels_per_din",
    "physical_lanes",
    "bench_pixels",
    "mode_a",
    "mode_b",
    "direction",
    "brightness",
    "pair_backend",
    "submitted_generation",
    "completed_generation",
    "pair_completions",
    "pending_replacements",
    "lane_a0_dma",
    "lane_a1_dma",
    "lane_a0_stop",
    "lane_a1_stop",
    "lane_a0_fault",
    "lane_a1_fault",
    "pair_fault",
    "emitted",
    "emit_errors",
)
COUNTER_FIELDS = (
    "stream_epoch",
    "pair_epoch_drops",
    "asrc_consumed",
    "asrc_discarded",
    "push_rejected",
    "stale_discards",
    "paired_slots",
    "pair_skew_drops",
    "startup_discard_pairs",
    "ap_hops",
    "measured_hz",
    "rate_locked",
    "gain_clip_pos",
    "gain_clip_neg",
)
SNAPSHOT_FIELDS = (
    "publication_generation",
    "stream_epoch",
    "hop_sequence",
    "emit_on",
    "config_revision",
    "hops_consumed",
    "hops_rejected",
    "asrc_starved",
    "measured_hz",
    "build_sha256",
    "uid",
    "valid",
    "stale",
)


class ExperimentError(RuntimeError):
    def __init__(self, reason: str, *, before_write: bool = True, detail: Any = None) -> None:
        super().__init__(reason)
        self.reason = reason
        self.before_write = before_write
        self.detail = detail


class RecordError(Exception):
    pass


def config_sub(payload: bytes) -> int | None:
    if len(payload) < 4:
        return None
    return struct.unpack_from("<I", payload)[0]


def is_control_write(op: int, payload: bytes) -> bool:
    if op == OP_CONFIG and config_sub(payload) in CONFIG_WRITE_SUBS:
        return True
    return False


def present(mapping: Mapping[str, Any] | None, names: tuple[str, ...]) -> dict[str, Any]:
    if not isinstance(mapping, Mapping):
        return {name: UNKNOWN for name in names}
    return {name: mapping[name] if name in mapping else UNKNOWN for name in names}


def decode_json_reply(got: Mapping[str, Any]) -> tuple[dict | None, dict]:
    body = got.get("body") if isinstance(got.get("body"), (bytes, bytearray)) else b""
    raw = {
        "ok": bool(got.get("ok")),
        "status": got.get("status"),
        "body_len": len(body),
        "sha256": hashlib.sha256(body).hexdigest(),
        "prefix_hex": body[:64].hex(),
    }
    if not got.get("ok"):
        raw["kind"] = "rejected"
        return None, raw
    try:
        parsed = json.loads(body.decode("utf-8"))
    except (UnicodeError, json.JSONDecodeError) as exc:
        raw["kind"] = "truncated_or_invalid"
        raw["error"] = type(exc).__name__
        return None, raw
    if not isinstance(parsed, dict):
        raw["kind"] = "truncated_or_invalid"
        raw["error"] = "not_object"
        return None, raw
    raw["kind"] = "json"
    return parsed, raw


def load_packet(path: Path) -> dict:
    packet = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(packet, dict):
        raise ExperimentError("packet is not an object")
    image = packet.get("image")
    if not isinstance(image, dict):
        raise ExperimentError("packet.image is required")
    for key in ("uid", "build", "schema_sha256", "protocol", "source", "contract"):
        if not image.get(key) and image.get(key) != 0:
            raise ExperimentError(f"packet.image.{key} is required")
    stages = packet.get("stages")
    if not isinstance(stages, dict) or "emit_off_observe" not in stages:
        raise ExperimentError("packet.stages.emit_off_observe is required")
    return packet


def identity_denial(packet: Mapping[str, Any], info: Mapping[str, Any]) -> str | None:
    image = packet["image"]
    checks = (
        ("uid", info.get("uid"), image.get("uid")),
        ("build", info.get("build"), image.get("build")),
        ("protocol", info.get("protocol"), image.get("protocol")),
        ("schema_sha256", info.get("schema_sha256"), image.get("schema_sha256")),
        ("source", info.get("source"), image.get("source")),
        ("contract", info.get("contract"), image.get("contract")),
    )
    for name, got, want in checks:
        if got != want:
            return f"{name} mismatch"
    return None


def profile_denial(packet: Mapping[str, Any], status: Mapping[str, Any]) -> str | None:
    expect = packet.get("expected_profile") or {}
    if not isinstance(expect, dict):
        return "expected_profile is not an object"
    for key, want in expect.items():
        if key not in status:
            return f"profile {key} missing"
        if status.get(key) != want:
            return f"profile {key} mismatch"
    return None


def revision_denial(packet: Mapping[str, Any], config: Mapping[str, Any]) -> str | None:
    changes = packet.get("admitted_changes") or []
    if not changes:
        return None
    expected = packet.get("expected_revision")
    if expected is None:
        return "admitted change without expected_revision"
    if config.get("revision") != expected:
        return "stale config revision"
    return None


def emit_change_denied(packet: Mapping[str, Any]) -> str | None:
    for change in packet.get("admitted_changes") or []:
        if not isinstance(change, dict):
            return "admitted change is not an object"
        if change.get("field") == "emit_on" or change.get("emit_on") not in (None, 0, False):
            return "emission enable is not a write of this entry"
        if change.get("value") not in (None, 0, False) and change.get("field") == "emit_on":
            return "emission enable is not a write of this entry"
    return None


def physical_blockers(packet: Mapping[str, Any]) -> list[str]:
    """External and image gaps that must be true before any output write.

    Returned in the order an operator should read them. Device open is refused
    while any item remains.
    """
    blockers: list[str] = []
    injection = packet.get("fixture_injection") or {}
    if not injection.get("supported"):
        blockers.append(
            injection.get("absence_reason")
            or "pair fixture injection command is absent on this image"
        )
    wiring = packet.get("wiring") or {}
    p603 = wiring.get("din_b_on_p603") is True
    p604 = wiring.get("din_b_on_p604") is True
    broken = wiring.get("break_80_81_verified") is True
    if p603 and p604:
        blockers.append("wiring names both P603 and P604; this image uses one DIN-B pin")
    elif p604 and not broken:
        blockers.append("P604 packet still needs the LED 80 to 81 break confirmed")
    elif p604 and broken:
        expect = packet.get("expected_profile") or {}
        if expect.get("din_b") not in (None, "P604"):
            blockers.append("P604 wiring does not match expected din_b")
    elif not (p603 and broken):
        blockers.append(
            "power-off DIN-B move from P004/U18 pin 16 to P603/U18 pin 33, "
            "with the LED 80 to 81 data connection confirmed broken"
        )
    if packet.get("output_permission") != "AUTHORISED_THIS_PAIR_PACKET":
        blockers.append(
            "explicit authorisation of this pair-output packet "
            "(AUTHORISED_THIS_PAIR_PACKET)"
        )
    limit = packet.get("current_limit") or {}
    milliamps = limit.get("milliamps")
    if not isinstance(milliamps, (int, float)) or isinstance(milliamps, bool):
        blockers.append(
            "measured supply-current limit for this WS2816 assembly is not recorded; "
            "brightness 24 is only the recorded boot configuration"
        )
    elif limit.get("meter") is not True:
        blockers.append(
            "supply-current limit is not a bench measurement for this assembly; "
            "a host-test milliamp value does not admit live mapping"
        )
    blockers.extend(mapping_shape_blockers(packet))
    return blockers


def _finite(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def window_blocker(stage: Mapping[str, Any]) -> str | None:
    samples = stage.get("samples") if "samples" in stage else 2
    interval = stage.get("interval_s") if "interval_s" in stage else 1.0
    duration = stage.get("max_duration_s")
    cleanup = stage.get("cleanup_allowance_s", 5)
    if not isinstance(samples, int) or isinstance(samples, bool) or samples < 1:
        return "samples must be a positive integer"
    if not _finite(interval) or interval < 0:
        return "interval_s must be a finite non-negative number"
    if not _finite(duration) or duration <= 0 or duration > 120:
        return "emit-off max_duration_s must be within 1..120"
    if not _finite(cleanup) or cleanup <= 0 or cleanup > 30:
        return "cleanup_allowance_s must be a finite positive number within 30s"
    if (samples - 1) * float(interval) > float(duration) + 1e-9:
        return "sample spacing exceeds max_duration_s"
    return None


def emit_off_blockers(packet: Mapping[str, Any]) -> list[str]:
    stage = (packet.get("stages") or {}).get("emit_off_observe") or {}
    blockers: list[str] = []
    if stage.get("host_runnable") is not True:
        blockers.append("emit-off stage is not marked host_runnable")
    why = window_blocker(stage)
    if why:
        blockers.append(why)
    denied = emit_change_denied(packet)
    if denied:
        blockers.append(denied)
    return blockers


def clip_fields(pdm: Mapping[str, Any] | None) -> dict[str, Any]:
    if not isinstance(pdm, Mapping):
        return {
            "gain_clip_pos": UNKNOWN,
            "gain_clip_neg": UNKNOWN,
            "sat_pos": UNKNOWN,
            "sat_neg": UNKNOWN,
        }
    lanes = pdm.get("lanes")
    sat_pos: Any = UNKNOWN
    sat_neg: Any = UNKNOWN
    if isinstance(lanes, list):
        sat_pos = [lane.get("sat_pos") if isinstance(lane, Mapping) and "sat_pos" in lane else UNKNOWN for lane in lanes]
        sat_neg = [lane.get("sat_neg") if isinstance(lane, Mapping) and "sat_neg" in lane else UNKNOWN for lane in lanes]
    return {
        "gain_clip_pos": pdm["gain_clip_pos"] if "gain_clip_pos" in pdm else UNKNOWN,
        "gain_clip_neg": pdm["gain_clip_neg"] if "gain_clip_neg" in pdm else UNKNOWN,
        "sat_pos": sat_pos,
        "sat_neg": sat_neg,
    }


def counter_view(metrics: Mapping[str, Any] | None) -> dict[str, Any]:
    extracted = extract_a8_counters(metrics)
    pdm = metrics.get("pdm_target") if isinstance(metrics, Mapping) else None
    view = dict(extracted)
    if not isinstance(pdm, Mapping):
        for name in COUNTER_FIELDS:
            view.setdefault(name, UNKNOWN)
        view["clip"] = clip_fields(None)
        view["sample_rate_match"] = UNKNOWN
        return view
    for name in COUNTER_FIELDS:
        if name not in view:
            view[name] = pdm[name] if name in pdm else UNKNOWN
    view["clip"] = clip_fields(pdm)
    view["sample_rate_match"] = pdm["sample_rate_match"] if "sample_rate_match" in pdm else UNKNOWN
    return view


def pcm_s16le_report(data: bytes, *, provenance: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Separate a nonzero sample from permission to call it music.

    A magnitude of 1 is non-silent. It does not name a recording and it does
    not grant playback permission. named_music stays false unless the caller
    supplies an identity and an already-recorded permit.
    """
    if len(data) % 2:
        return {
            "aligned": False,
            "sample_count": UNKNOWN,
            "nonzero_samples": UNKNOWN,
            "peak_abs": UNKNOWN,
            "non_silent": UNKNOWN,
            "named_music": False,
        }
    count = len(data) // 2
    samples = struct.unpack("<" + str(count) + "h", data) if count else ()
    nonzero = sum(1 for sample in samples if sample)
    peak = max((abs(sample) for sample in samples), default=0)
    non_silent = nonzero > 0
    named = False
    if isinstance(provenance, Mapping):
        named = bool(provenance.get("permitted")) and bool(provenance.get("identity")) and non_silent
    return {
        "aligned": True,
        "sample_count": count,
        "nonzero_samples": nonzero,
        "peak_abs": peak,
        "non_silent": non_silent,
        "named_music": named,
    }


def delta_value(start: Any, end: Any) -> Any:
    if start is UNKNOWN or end is UNKNOWN or start == UNKNOWN or end == UNKNOWN:
        return UNKNOWN
    if isinstance(start, bool) or isinstance(end, bool):
        return UNKNOWN
    if isinstance(start, (int, float)) and isinstance(end, (int, float)):
        return end - start
    return UNKNOWN


def account_window(samples: list[Mapping[str, Any]]) -> dict[str, Any]:
    """Start/end deltas for counters that were actually present.

    asrc_consumed counts samples. ap_hops counts hops. paired_slots counts
    capture slot pairs. Those units are not required to match.
    """
    if not samples:
        return {"samples": 0, "deltas": {}, "pair_join": {"pair_complete": UNKNOWN}}
    start = samples[0]
    end = samples[-1]
    start_counters = start.get("counters") if isinstance(start.get("counters"), Mapping) else {}
    end_counters = end.get("counters") if isinstance(end.get("counters"), Mapping) else {}
    names = sorted(set(start_counters) | set(end_counters))
    deltas = {name: delta_value(start_counters.get(name, UNKNOWN), end_counters.get(name, UNKNOWN)) for name in names}
    palette = end.get("palette") if isinstance(end.get("palette"), Mapping) else {}
    pair = pair_join(palette, epoch=end_counters.get("stream_epoch", UNKNOWN))
    return {
        "samples": len(samples),
        "deltas": deltas,
        "units": {
            "asrc_consumed": "samples",
            "ap_hops": "hops",
            "paired_slots": "slot_pairs",
            "stream_epoch": "epoch",
        },
        "equality_not_required": ["asrc_consumed", "ap_hops", "paired_slots"],
        "pair_join": pair,
        "sample_rate_match": end_counters.get("sample_rate_match", UNKNOWN),
    }


def progression_verdict(samples: list[Mapping[str, Any]], account: Mapping[str, Any]) -> str:
    """PASS only when a second sample shows the capture or publication moving.

    A missing delta stays a failure to demonstrate progression. A measured
    zero delta is a real stall, not an unknown.
    """
    if len(samples) < 2:
        return "FAIL"
    hop = account.get("deltas", {}).get("ap_hops")
    start = samples[0].get("snapshot") if isinstance(samples[0].get("snapshot"), Mapping) else {}
    end = samples[-1].get("snapshot") if isinstance(samples[-1].get("snapshot"), Mapping) else {}
    publication = delta_value(start.get("hop_sequence", UNKNOWN), end.get("hop_sequence", UNKNOWN))
    moved = []
    for value in (hop, publication):
        if isinstance(value, bool) or value is UNKNOWN or value == UNKNOWN:
            continue
        if isinstance(value, (int, float)) and value > 0:
            moved.append(value)
    if not moved:
        if hop is UNKNOWN and publication is UNKNOWN:
            return "FAIL"
        return "FAIL"
    return "PASS"


def pair_join(status: Mapping[str, Any], *, epoch: Any = None) -> dict[str, Any]:
    """A completed pair needs a real generation, the same completion, and a live epoch.

    submitted == completed == 0 is not a transmission. Missing lane or epoch
    evidence stays incomplete rather than being treated as zero.
    """
    submitted = status.get("submitted_generation", UNKNOWN)
    completed = status.get("completed_generation", UNKNOWN)
    completions = status.get("pair_completions", UNKNOWN)
    epoch_value = status.get("stream_epoch", UNKNOWN if epoch is None else epoch)
    fault_a = status.get("lane_a0_fault", UNKNOWN)
    fault_b = status.get("lane_a1_fault", UNKNOWN)
    base = {
        "pair_complete": False,
        "partial_pair_counted_complete": False,
        "submitted_generation": submitted,
        "completed_generation": completed,
        "pair_completions": completions,
        "stream_epoch": epoch_value,
        "lane_a0_fault": fault_a,
        "lane_a1_fault": fault_b,
        "pending_replacements": status.get("pending_replacements", UNKNOWN),
    }
    if submitted is UNKNOWN or completed is UNKNOWN:
        base["reason"] = "generation field missing"
        return base
    if not _whole(submitted) or not _whole(completed) or submitted <= 0 or completed != submitted:
        base["reason"] = "no observed generation completion"
        return base
    if not _whole(completions) or completions <= 0:
        base["reason"] = "completion count missing"
        return base
    if not _whole(epoch_value) or epoch_value <= 0:
        base["reason"] = "epoch missing"
        return base
    if fault_a is UNKNOWN or fault_b is UNKNOWN:
        base["reason"] = "lane fault state missing"
        return base
    if fault_a != 0 or fault_b != 0:
        base["reason"] = "lane fault"
        return base
    base["pair_complete"] = True
    base["reason"] = "generation completed on both lanes"
    return base


HEALTH_COUNTERS = (
    "stream_epoch",
    "pair_epoch_drops",
    "asrc_consumed",
    "ap_hops",
    "asrc_discarded",
    "push_rejected",
    "stale_discards",
)
LOSS_COUNTERS = ("asrc_discarded", "push_rejected", "stale_discards", "pair_epoch_drops")
HEALTH_PALETTE = (
    "emitted",
    "emit_errors",
    "pair_fault",
    "lane_a0_fault",
    "lane_a1_fault",
    "din_b",
)
SHOW_FIELDS = ("din_b", "emit_enabled", "wire_profile", "brightness", "mode_a", "mode_b")
CONTROL_FIELDS = ("palette_a", "palette_b", "mode_a", "mode_b")
CONTROL_AT_S = 30.0
STABILITY_SAMPLE_S = 10.0
MAPPING_HOLD = "AUTHORISED_MAPPING_HOLD"


def _absent(value: Any) -> bool:
    return value is UNKNOWN or value == UNKNOWN or value is None


def _positive_progress(delta: Any) -> bool:
    return isinstance(delta, (int, float)) and not isinstance(delta, bool) and delta > 0


def _negative_delta(delta: Any) -> bool:
    return isinstance(delta, (int, float)) and not isinstance(delta, bool) and delta < 0


def evaluate_run_health(
    rows: list[Mapping[str, Any]],
    *,
    require_audio_progress: bool = True,
    require_pair_complete: bool = True,
    din_b: str = "P604",
    expected_show: Mapping[str, Any] | None = None,
    min_window_s: float | None = None,
) -> dict[str, Any]:
    """Shared functional/stability health. A missing field is not a zero.

    Progress is scored on declared, sufficiently spaced observation windows.
    Immediate boundary polls are not required to increment.
    """
    window_s = STABILITY_SAMPLE_S if min_window_s is None else float(min_window_s)
    if len(rows) < 2:
        return {"verdict": "FAIL", "reason": "health window needs two samples", "deltas": {}}
    first_epoch = None
    prev: Mapping[str, Any] | None = None
    for index, row in enumerate(rows):
        counters = row.get("counters") if isinstance(row.get("counters"), Mapping) else {}
        palette = row.get("palette") if isinstance(row.get("palette"), Mapping) else {}
        for name in HEALTH_COUNTERS:
            if name not in counters or _absent(counters.get(name)):
                return {"verdict": "FAIL", "reason": f"{name} missing", "sample": index, "deltas": {}}
        for name in HEALTH_PALETTE:
            if name not in palette or _absent(palette.get(name)):
                return {"verdict": "FAIL", "reason": f"{name} missing", "sample": index, "deltas": {}}
        if palette.get("din_b") != din_b:
            return {"verdict": "FAIL", "reason": "din_b left P604", "sample": index, "deltas": {}}
        if palette.get("emit_errors") not in (0, 0.0):
            return {"verdict": "FAIL", "reason": "emit errors not zero", "sample": index, "deltas": {}}
        if palette.get("pair_fault") not in (0, 0.0):
            return {"verdict": "FAIL", "reason": "pair fault", "sample": index, "deltas": {}}
        if palette.get("lane_a0_fault") not in (0, 0.0) or palette.get("lane_a1_fault") not in (0, 0.0):
            return {"verdict": "FAIL", "reason": "lane fault", "sample": index, "deltas": {}}
        if first_epoch is None:
            first_epoch = counters.get("stream_epoch")
        elif counters.get("stream_epoch") != first_epoch:
            return {"verdict": "FAIL", "reason": "stream epoch changed", "sample": index, "deltas": {}}
        if expected_show:
            for name, want in expected_show.items():
                if name not in palette or _absent(palette.get(name)):
                    return {"verdict": "FAIL", "reason": f"{name} missing", "sample": index, "deltas": {}}
                if palette.get(name) != want:
                    return {"verdict": "FAIL", "reason": f"{name} mismatch", "sample": index, "deltas": {}}
        if prev is not None:
            prev_c = prev.get("counters") if isinstance(prev.get("counters"), Mapping) else {}
            prev_p = prev.get("palette") if isinstance(prev.get("palette"), Mapping) else {}
            for name in HEALTH_COUNTERS:
                delta = delta_value(prev_c.get(name), counters.get(name))
                if _negative_delta(delta):
                    return {"verdict": "FAIL", "reason": f"{name} discontinuity", "sample": index, "deltas": {}}
                if name in LOSS_COUNTERS and _positive_progress(delta):
                    return {"verdict": "FAIL", "reason": f"{name} increased", "sample": index, "deltas": {}}
            for name in ("emitted", "emit_errors"):
                delta = delta_value(prev_p.get(name), palette.get(name))
                if _negative_delta(delta):
                    return {"verdict": "FAIL", "reason": f"{name} discontinuity", "sample": index, "deltas": {}}
        prev = row
    first_c = rows[0]["counters"]
    last_c = rows[-1]["counters"]
    first_p = rows[0]["palette"]
    last_p = rows[-1]["palette"]
    deltas = {
        "ap_hops": delta_value(first_c.get("ap_hops"), last_c.get("ap_hops")),
        "asrc_consumed": delta_value(first_c.get("asrc_consumed"), last_c.get("asrc_consumed")),
        "asrc_discarded": delta_value(first_c.get("asrc_discarded"), last_c.get("asrc_discarded")),
        "push_rejected": delta_value(first_c.get("push_rejected"), last_c.get("push_rejected")),
        "stale_discards": delta_value(first_c.get("stale_discards"), last_c.get("stale_discards")),
        "emitted": delta_value(first_p.get("emitted"), last_p.get("emitted")),
        "emit_errors": delta_value(first_p.get("emit_errors"), last_p.get("emit_errors")),
        "stream_epoch": delta_value(first_c.get("stream_epoch"), last_c.get("stream_epoch")),
        "pair_epoch_drops": delta_value(first_c.get("pair_epoch_drops"), last_c.get("pair_epoch_drops")),
    }
    if require_audio_progress:
        saw_progress = False
        anchor = 0
        for index in range(1, len(rows)):
            t0 = rows[anchor].get("t")
            t1 = rows[index].get("t")
            if not _finite(t0) or not _finite(t1) or (t1 - t0) + 1e-9 < window_s:
                continue
            hops = delta_value(rows[anchor]["counters"].get("ap_hops"), rows[index]["counters"].get("ap_hops"))
            asrc = delta_value(rows[anchor]["counters"].get("asrc_consumed"), rows[index]["counters"].get("asrc_consumed"))
            emitted = delta_value(rows[anchor]["palette"].get("emitted"), rows[index]["palette"].get("emitted"))
            moved = _positive_progress(hops) and _positive_progress(asrc) and _positive_progress(emitted)
            if not moved:
                reason = "progress then freeze" if saw_progress else "ap_hops did not advance"
                if not _positive_progress(hops):
                    reason = "progress then freeze" if saw_progress else "ap_hops did not advance"
                elif not _positive_progress(asrc):
                    reason = "progress then freeze" if saw_progress else "asrc_consumed did not advance"
                elif not _positive_progress(emitted):
                    reason = "progress then freeze" if saw_progress else "frames did not advance"
                return {"verdict": "FAIL", "reason": reason, "sample": index, "deltas": deltas}
            saw_progress = True
            anchor = index
        if not saw_progress:
            return {"verdict": "FAIL", "reason": "no spaced observation window", "deltas": deltas}
    if require_pair_complete:
        joined = pair_join(last_p, epoch=last_c.get("stream_epoch"))
        if joined.get("pair_complete") is not True:
            return {
                "verdict": "FAIL",
                "reason": joined.get("reason") or "pair incomplete",
                "deltas": deltas,
                "pair_join": joined,
            }
    return {"verdict": "PASS", "reason": None, "deltas": deltas}


def _whole(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


class EvidenceWriter:
    def __init__(self, directory: Path, max_bytes: int) -> None:
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=False)
        self.max_bytes = int(max_bytes)
        self.used = 0

    def write_json(self, name: str, payload: Any) -> Path:
        if "/" in name or name.startswith("."):
            raise RecordError(f"refusing record name {name}")
        text = json.dumps(payload, indent=2, sort_keys=True) + "\n"
        encoded = text.encode("utf-8")
        if self.used + len(encoded) > self.max_bytes:
            raise RecordError("record size limit")
        path = self.directory / name
        path.write_text(text, encoding="utf-8")
        self.used += len(encoded)
        return path

    def write_bytes(self, name: str, payload: bytes) -> Path:
        if "/" in name or name.startswith(".") or not name:
            raise RecordError(f"refusing record name {name}")
        if self.used + len(payload) > self.max_bytes:
            raise RecordError("record size limit")
        path = self.directory / name
        path.write_bytes(payload)
        self.used += len(payload)
        return path


def _store_body(writer: EvidenceWriter, name: str, body: bytes, raw: Mapping[str, Any], *, op: int, payload: bytes, clock: Callable[[], float]) -> dict:
    data = bytes(body or b"")
    path = writer.write_bytes(name, data)
    stored = dict(raw)
    stored["body_file"] = path.name
    stored["body_len"] = len(data)
    stored["sha256"] = hashlib.sha256(data).hexdigest()
    stored["request"] = {
        "op": op,
        "payload_sha256": hashlib.sha256(bytes(payload)).hexdigest(),
        "monotonic_s": clock(),
        "wall_time_unix": time.time(),
    }
    return stored


def _read_metrics(session, writer: EvidenceWriter, index: int, clock: Callable[[], float]) -> tuple[dict | None, dict, dict]:
    got = session.transact(OP_METRICS, b"")
    parsed, raw = decode_json_reply(got)
    raw = _store_body(writer, f"sample-{index:04d}-metrics.bin", got.get("body") or b"", raw, op=OP_METRICS, payload=b"", clock=clock)
    if parsed is None:
        raise ExperimentError(f"metrics reply {raw.get('kind')}", before_write=True, detail=raw)
    return parsed, raw, counter_view(parsed)


def _read_status(session, writer: EvidenceWriter, index: int, clock: Callable[[], float]) -> tuple[dict | None, dict]:
    got = session.transact(OP_PALETTE_STATUS, b"")
    parsed, raw = decode_json_reply(got)
    raw = _store_body(writer, f"sample-{index:04d}-palette.bin", got.get("body") or b"", raw, op=OP_PALETTE_STATUS, payload=b"", clock=clock)
    return parsed, raw


def _read_snapshot(session, writer: EvidenceWriter, index: int, clock: Callable[[], float]) -> tuple[dict | None, dict]:
    got = session.transact(OP_SNAPSHOT, b"")
    body = got.get("body") if isinstance(got.get("body"), (bytes, bytearray)) else b""
    if not got.get("ok"):
        raw = {"kind": "rejected", "status": got.get("status")}
        return None, _store_body(writer, f"sample-{index:04d}-snapshot.bin", body, raw, op=OP_SNAPSHOT, payload=b"", clock=clock)
    if len(body) < proto.SNAPSHOT_BYTES:
        raw = {"kind": "truncated_or_invalid", "body_len": len(body)}
        return None, _store_body(writer, f"sample-{index:04d}-snapshot.bin", body, raw, op=OP_SNAPSHOT, payload=b"", clock=clock)
    try:
        snap = proto.unpack_snapshot(body)
    except ValueError as exc:
        raw = {"kind": "truncated_or_invalid", "error": str(exc), "body_len": len(body)}
        return None, _store_body(writer, f"sample-{index:04d}-snapshot.bin", body, raw, op=OP_SNAPSHOT, payload=b"", clock=clock)
    # The codec zero-fills publication_deadline_misses when the buffer is short.
    # This experiment does not report that field.
    snap.pop("publication_deadline_misses", None)
    raw = {"kind": "snapshot", "body_len": len(body)}
    return present(snap, SNAPSHOT_FIELDS), _store_body(writer, f"sample-{index:04d}-snapshot.bin", body, raw, op=OP_SNAPSHOT, payload=b"", clock=clock)


def _read_config(session) -> dict:
    got = session.transact(OP_CONFIG, proto.pack_config_sub(proto.CFG_GET_CONFIG))
    parsed_body = got.get("body") or b""
    if not got.get("ok") or len(parsed_body) < proto.CONFIG_BYTES_V1:
        raise ExperimentError("config read failed", before_write=True)
    return proto.unpack_config(parsed_body)


def _emission_gap(packet: Mapping[str, Any], status: Mapping[str, Any] | None) -> str | None:
    """Missing required profile or emission evidence. A rejected snapshot is not this gap."""
    if not isinstance(status, Mapping):
        return "emission state missing"
    why = profile_denial(packet, status)
    if why and why.endswith("mismatch"):
        return why
    if why:
        return why
    if "emit_enabled" not in status:
        return "emission state missing"
    if status.get("emit_enabled") is not False:
        return "emission state is not explicit emit-off"
    return None


def run_emit_off(
    packet: Mapping[str, Any],
    session,
    writer: EvidenceWriter,
    *,
    samples: int,
    interval_s: float,
    sleeper: Callable[[float], None],
    clock: Callable[[], float],
    deadline_s: float,
) -> dict:
    """Read a bounded emit-off window. Raises before any config write on denial.

    The deadline is absolute monotonic time across the reads and the spacing.
    Sleep duration is not used as a substitute for that clock.
    """
    if emit_change_denied(packet):
        raise ExperimentError(emit_change_denied(packet) or "emit denied", before_write=True)
    origin = clock()

    def guard(where: str, rows: list[Mapping[str, Any]]) -> None:
        elapsed = clock() - origin
        if elapsed > float(deadline_s):
            raise ExperimentError(
                "acquisition deadline exceeded",
                before_write=False,
                detail={
                    "where": where,
                    "elapsed_s": elapsed,
                    "limit_s": float(deadline_s),
                    "samples_retained": len(rows),
                    "counter_progression": progression_verdict(rows, account_window(rows)) if len(rows) >= 2 else "FAIL",
                    "runtime_acceptance": "FAIL",
                },
            )

    info_wrap = session.info()
    guard("identity", [])
    info = info_wrap.get("info") if isinstance(info_wrap, Mapping) and "info" in info_wrap else info_wrap
    if not isinstance(info, Mapping):
        raise ExperimentError("INFO missing", before_write=True)
    denial = identity_denial(packet, info)
    writer.write_json("identity.json", {"info": info, "denial": denial})
    if denial:
        raise ExperimentError(denial, before_write=True)
    changes = packet.get("admitted_changes") or []
    if changes:
        config = _read_config(session)
        guard("config", [])
        writer.write_json("config-initial.json", config)
        why = revision_denial(packet, config)
        if why:
            raise ExperimentError(why, before_write=True)
        raise ExperimentError("admitted changes are not applied by the emit-off path", before_write=True)
    rows: list[dict[str, Any]] = []
    for index in range(samples):
        if index:
            sleeper(interval_s)
            guard("spacing", rows)
        metrics, raw_metrics, counters = _read_metrics(session, writer, index, clock)
        guard("metrics", rows)
        status, raw_status = _read_status(session, writer, index, clock)
        guard("palette", rows)
        snapshot, raw_snapshot = _read_snapshot(session, writer, index, clock)
        guard("snapshot", rows)
        gap = _emission_gap(packet, status)
        if gap and gap.endswith("mismatch"):
            raise ExperimentError(gap, before_write=True)
        if isinstance(status, Mapping) and status.get("emit_enabled") is True:
            raise ExperimentError("emit already enabled; stopped without a control write", before_write=True)
        if isinstance(snapshot, Mapping) and snapshot.get("emit_on") not in (0, UNKNOWN, None):
            raise ExperimentError("snapshot emit_on was not 0; stopped without a control write", before_write=True)
        palette = present(status, PAIR_FIELDS) if status is not None else {name: UNKNOWN for name in PAIR_FIELDS}
        row = {
            "index": index,
            "counters": counters,
            "palette": palette,
            "snapshot": snapshot if snapshot is not None else {name: UNKNOWN for name in SNAPSHOT_FIELDS},
            "metrics_raw": raw_metrics,
            "palette_raw": raw_status,
            "snapshot_raw": raw_snapshot,
            "metrics_sha256": raw_metrics.get("sha256"),
            "emission_gap": gap,
        }
        writer.write_json(f"sample-{index:04d}.json", row)
        rows.append(row)
        if raw_status.get("kind") != "json":
            break
    account = account_window(rows)
    writer.write_json("account.json", account)
    progression = progression_verdict(rows, account)
    runtime = "PASS"
    if not rows or any(row.get("emission_gap") for row in rows):
        runtime = "FAIL"
    if rows and rows[-1]["palette"].get("emit_enabled") is not False:
        runtime = "FAIL"
    verdict = "PASS" if runtime == "PASS" and progression == "PASS" else "FAIL"
    return {
        "verdict": verdict,
        "runtime_acceptance": runtime,
        "counter_progression": progression,
        "scope": "emit_off_observe",
        "samples": len(rows),
        "control_writes": 0,
        "emit_enabled_by_this_run": False,
        "physical_observation": False,
        "account": account,
        "reason": next((row.get("emission_gap") for row in rows if row.get("emission_gap")), None),
    }


def finish_lease(run_dir: Path | None, token: str | None) -> None:
    if run_dir is None or not token:
        return
    try:
        live_lease.release(run_dir, token)
    except live_lease.LeaseError:
        return


def _write_result(writer: EvidenceWriter, payload: dict) -> dict:
    try:
        writer.write_json("result.json", payload)
    except RecordError:
        payload["record"] = "FAILED"
    return payload


def _transport_faults() -> tuple[type[BaseException], ...]:
    from titan_transport import FramingError, ObserveDenied  # noqa: WPS433

    return (FramingError, ObserveDenied, TimeoutError)


def _close_session(session, run_dir: Path | None, token: str | None, payload: dict | None) -> None:
    try:
        finish_lease(run_dir, token)
        if session is not None:
            session.close()
    except Exception as exc:
        if isinstance(payload, dict):
            payload["cleanup_error"] = f"{type(exc).__name__}: {exc}"


def execute_emit_off(
    packet: Mapping[str, Any],
    evidence: Path,
    open_session: Callable[[], Any],
    *,
    sleeper=time.sleep,
    clock: Callable[[], float] | None = None,
) -> dict:
    clock = clock or time.monotonic
    blockers = emit_off_blockers(packet)
    writer = EvidenceWriter(evidence, int(packet.get("max_record_bytes") or 2_000_000))
    if blockers:
        return _write_result(writer, {
            "verdict": "BLOCKED",
            "blockers": blockers,
            "device_opened": False,
            "physical_observation": False,
            "emit_enabled_by_this_run": False,
        })
    stage = packet["stages"]["emit_off_observe"]
    samples = int(stage["samples"]) if "samples" in stage else 2
    interval = float(stage["interval_s"]) if "interval_s" in stage else 1.0
    duration = float(stage["max_duration_s"])
    cleanup = float(stage.get("cleanup_allowance_s", 5))
    session = None
    token = None
    payload: dict | None = None
    try:
        session = open_session()
        token = getattr(session, "lease_token", None)
        result = run_emit_off(
            packet, session, writer,
            samples=samples, interval_s=interval, sleeper=sleeper,
            clock=clock, deadline_s=duration,
        )
        result["device_opened"] = True
        result["cleanup_allowance_s"] = cleanup
        result["acquisition_limit_s"] = duration
        payload = _write_result(writer, result)
        return payload
    except ExperimentError as exc:
        payload = {
            "verdict": "DENIED" if exc.before_write else "FAULT",
            "reason": exc.reason,
            "detail": exc.detail,
            "before_write": exc.before_write,
            "device_opened": session is not None,
            "emit_enabled_by_this_run": False,
            "physical_observation": False,
            "cleanup_allowance_s": cleanup,
            "acquisition_limit_s": duration,
        }
        if isinstance(exc.detail, dict):
            for key in ("counter_progression", "runtime_acceptance", "samples_retained"):
                if key in exc.detail:
                    payload[key] = exc.detail[key]
        payload = _write_result(writer, payload)
        return payload
    except (KeyboardInterrupt, RecordError, OSError, *_transport_faults()) as exc:
        payload = {
            "verdict": "FAULT",
            "reason": type(exc).__name__,
            "detail": str(exc),
            "emit_enabled_by_this_run": False,
            "device_opened": session is not None,
            "physical_observation": False,
            "runtime_acceptance": "FAIL",
        }
        payload = _write_result(writer, payload)
        return payload
    finally:
        _close_session(session, evidence, token, payload)


def mapping_shape_blockers(packet: Mapping[str, Any]) -> list[str]:
    injection = packet.get("fixture_injection") or {}
    if not injection.get("supported"):
        return []
    mapping = packet.get("mapping") or {}
    fixtures = mapping.get("fixtures")
    if not isinstance(fixtures, list) or not fixtures:
        return ["mapping fixture payload is absent"]
    blockers: list[str] = []
    seen: set[int] = set()
    for index, item in enumerate(fixtures):
        if not isinstance(item, Mapping):
            blockers.append(f"fixture {index} is not an object")
            continue
        generation = item.get("generation")
        if generation in (0, None):
            pass
        elif not _whole(generation) or generation < 0:
            blockers.append(f"fixture {index} generation is missing")
        elif generation in seen:
            blockers.append(f"fixture {index} repeats generation {generation}")
        else:
            seen.add(int(generation))
        try:
            lane0 = Path(item["lane0"]).read_bytes()
            lane1 = Path(item["lane1"]).read_bytes()
        except (OSError, KeyError, TypeError) as exc:
            blockers.append(f"fixture {index} unreadable ({type(exc).__name__})")
            continue
        if len(lane0) != PAIR_LANE_BYTES or len(lane1) != PAIR_LANE_BYTES:
            blockers.append(f"fixture {index} is not two 480-byte lanes")
    return blockers


def pair_fixture_request(lane0: bytes, lane1: bytes, generation: int, epoch: int, kind: int) -> bytes:
    if len(lane0) != PAIR_LANE_BYTES or len(lane1) != PAIR_LANE_BYTES:
        raise ExperimentError("pair fixture lanes must be 480 bytes", before_write=True)
    return struct.pack("<IIIIII", 1, 3, int(generation), int(epoch), int(kind), 0) + bytes(lane0) + bytes(lane1)


def host_admit(gate: Mapping[str, Any], request: bytes) -> int:
    """Same admission order as platform/ra8p1/pair_fixture_admit.c. No transmitter call."""
    if not request:
        return FIXTURE_SHAPE
    if len(request) == 24 + PAIR_LANE_BYTES:
        return FIXTURE_PARTIAL
    if len(request) != PAIR_FIXTURE_BYTES:
        return FIXTURE_SHAPE
    version, profile, generation, epoch, kind, reserved = struct.unpack_from("<IIIIII", request)
    if version != 1 or reserved != 0:
        return FIXTURE_SHAPE
    if profile != 3:
        return FIXTURE_PROFILE
    if generation == 0:
        return FIXTURE_GENERATION
    expected = gate.get("expected_epoch") or 0
    if epoch == 0 or expected == 0 or epoch != expected:
        return FIXTURE_EPOCH
    if kind not in (KIND_MAP, KIND_BLACK):
        return FIXTURE_KIND
    if generation <= int(gate.get("last_generation") or 0):
        return FIXTURE_STALE
    if gate.get("in_flight"):
        return FIXTURE_IN_FLIGHT
    if kind == KIND_BLACK and any(request[24:]):
        return FIXTURE_NOT_BLACK
    return FIXTURE_OK


def _load_fixtures(packet: Mapping[str, Any]) -> list[dict[str, Any]]:
    loaded = []
    for item in (packet.get("mapping") or {}).get("fixtures") or []:
        loaded.append({
            "lane0": Path(item["lane0"]).read_bytes(),
            "lane1": Path(item["lane1"]).read_bytes(),
            "generation": int(item["generation"]),
            "kind": KIND_MAP,
            "name": item.get("name"),
            "expect": item.get("expect"),
        })
    return loaded


def allocate_fixture_generations(fixtures: list[Mapping[str, Any]], last_generation: int) -> list[dict[str, Any]]:
    nxt = int(last_generation) + 1
    allocated = []
    for item in fixtures:
        allocated.append({
            "lane0": item["lane0"],
            "lane1": item["lane1"],
            "kind": item.get("kind", KIND_MAP),
            "name": item.get("name"),
            "expect": item.get("expect"),
            "packet_generation": item.get("generation"),
            "generation": nxt,
        })
        nxt += 1
    return allocated


def _led_restore_evidence(
    first: Mapping[str, Any] | None,
    second: Mapping[str, Any] | None,
    expected_profile: Mapping[str, Any],
) -> tuple[bool, str | None]:
    for status in (first, second):
        if not isinstance(status, Mapping):
            return False, "restore LED status missing"
        if status.get("emit_enabled") is not True:
            return False, "restore emit is not on"
        for name in ("din_a", "din_b"):
            if name in expected_profile and status.get(name) != expected_profile.get(name):
                return False, f"restore {name} mismatch"
        for name in ("wire_profile", "brightness"):
            if name in expected_profile and status.get(name) != expected_profile.get(name):
                return False, f"restore {name} mismatch"
        for name in ("lane_a0_fault", "lane_a1_fault", "pair_fault", "emit_errors"):
            if name not in status or _absent(status.get(name)):
                return False, f"{name} missing"
            if status.get(name) not in (0, 0.0):
                return False, "restore LED not healthy"
    emitted = delta_value(first.get("emitted") if isinstance(first, Mapping) else UNKNOWN, second.get("emitted") if isinstance(second, Mapping) else UNKNOWN)
    completions = delta_value(
        first.get("pair_completions") if isinstance(first, Mapping) else UNKNOWN,
        second.get("pair_completions") if isinstance(second, Mapping) else UNKNOWN,
    )
    if not _positive_progress(emitted):
        return False, "restored LED output did not advance"
    if not _positive_progress(completions):
        return False, "restored LED completion did not advance"
    return True, None


def _explicit_zero(status: Mapping[str, Any], name: str) -> bool:
    if name not in status or _absent(status.get(name)):
        return False
    return status.get(name) in (0, 0.0)


def _fixture_completion(status: Mapping[str, Any] | None, generation: int, epoch: int) -> tuple[bool, str | None]:
    """Requested generation, matching epoch, both lanes, explicit zero faults."""
    if not isinstance(status, Mapping):
        return False, "status missing"
    for name in ("lane_a0_fault", "lane_a1_fault", "pair_fault"):
        if name not in status or _absent(status.get(name)):
            return False, f"{name} missing"
        if status.get(name) not in (0, 0.0):
            return False, "lane fault after fixture submit" if "lane" in name else "pair fault"
    if status.get("completed_generation") != generation:
        return False, "generation mismatch"
    got_epoch = status.get("stream_epoch")
    if _absent(got_epoch) or got_epoch != epoch:
        return False, "epoch mismatch"
    for name in ("lane_a0_dma", "lane_a1_dma", "lane_a0_stop", "lane_a1_stop"):
        if status.get(name) != 1:
            return False, "partial lane"
    joined = pair_join(status, epoch=epoch)
    if joined.get("pair_complete") is not True:
        return False, str(joined.get("reason") or "pair incomplete")
    return True, None


def _black_complete(status: Mapping[str, Any] | None, generation: int, epoch: int) -> bool:
    ok, _reason = _fixture_completion(status, generation, epoch)
    return ok


def _lane_fault(status: Mapping[str, Any] | None) -> bool:
    if not isinstance(status, Mapping):
        return False
    if any(name not in status or _absent(status.get(name)) for name in ("lane_a0_fault", "lane_a1_fault", "pair_fault")):
        return True
    return bool(status.get("lane_a0_fault") or status.get("lane_a1_fault") or status.get("pair_fault"))


def _pair_in_flight(status: Mapping[str, Any] | None) -> bool:
    """Occupancy is submitted vs completed. A cumulative replacement count is not a queue."""
    if not isinstance(status, Mapping):
        return True
    submitted = status.get("submitted_generation")
    completed = status.get("completed_generation")
    if not _whole(submitted) or not _whole(completed):
        return True
    return int(submitted) != int(completed)


def _wait_pair_complete(
    session,
    writer: EvidenceWriter,
    generation: int,
    epoch: int,
    clock: Callable[[], float],
    sleeper,
    guard,
    *,
    timeout_s: float,
    interval_s: float,
) -> tuple[dict[str, Any] | None, str | None]:
    start = clock()
    status: dict[str, Any] | None = None
    why: str | None = None
    for poll in range(32):
        status, _raw = _read_status(session, writer, generation * 100 + poll, clock)
        guard("fixture-status")
        ok, why = _fixture_completion(status, generation, epoch)
        if why in {"lane fault after fixture submit", "pair fault", "pair_fault missing", "lane_a0_fault missing", "lane_a1_fault missing"}:
            return status, why
        if ok:
            return status, None
        if clock() - start > float(timeout_s):
            return status, why or "fixture completion timeout"
        sleeper(max(0.0, float(interval_s)))
        guard("fixture-wait")
    return status, why or "fixture completion timeout"


def mapping_hold_authorised(packet: Mapping[str, Any]) -> bool:
    lifecycle = packet.get("mapping_lifecycle") if isinstance(packet.get("mapping_lifecycle"), Mapping) else {}
    return lifecycle.get("hold") == MAPPING_HOLD


def hold_emit_off_blob(current: Mapping[str, Any]) -> bytes:
    desired = dict(current)
    desired["emit_on"] = 0
    desired["brightness"] = int(current.get("brightness", 24))
    desired["flags"] = int(current.get("flags") or 0)
    blob = proto.pack_config(desired)
    parsed = proto.unpack_config(blob)
    if parsed["emit_on"] != 0:
        raise ExperimentError("hold would leave emit on", before_write=True)
    return blob


def config_fields(config: Mapping[str, Any]) -> dict[str, Any]:
    keys = ("palette_a", "palette_b", "mode_a", "mode_b", "emit_on", "brightness", "flags")
    return {key: config.get(key) for key in keys}


def pack_grb48(red: int, green: int, blue: int) -> bytes:
    if any(not 0 <= channel <= 0xFFFF for channel in (red, green, blue)):
        raise ValueError("WS2816 channels must be 0..65535")
    return struct.pack(">HHH", green, red, blue)


def sparse_pair_lanes(*, a: Mapping[int, tuple[int, int, int]], b: Mapping[int, tuple[int, int, int]]) -> tuple[bytes, bytes]:
    lane0 = bytearray(PAIR_LANE_BYTES)
    lane1 = bytearray(PAIR_LANE_BYTES)
    for index, rgb in a.items():
        if not 0 <= int(index) < 80:
            raise ValueError("lane A pixel must be 0..79")
        lane0[int(index) * 6 : int(index) * 6 + 6] = pack_grb48(*rgb)
    for index, rgb in b.items():
        if not 0 <= int(index) < 80:
            raise ValueError("lane B pixel must be 0..79")
        lane1[int(index) * 6 : int(index) * 6 + 6] = pack_grb48(*rgb)
    return bytes(lane0), bytes(lane1)


def _physical_result(**fields: Any) -> dict[str, Any]:
    payload = {
        "scope": "mapping_stop",
        "physical_observation": False,
        "emit_enabled_by_this_run": False,
        "host_can_switch_led_supply": False,
        "operator_led_supply_fallback": "not controlled by this host",
        "darkness_verified": False,
        "darkness_unverified": True,
    }
    payload.update(fields)
    return payload


def execute_physical(
    packet: Mapping[str, Any],
    evidence: Path,
    open_session: Callable[[], Any],
    *,
    sleeper=time.sleep,
    clock: Callable[[], float] | None = None,
) -> dict:
    """Submit an admitted two-lane fixture and a black pair. Does not enable emit.

    A black pair is a real all-zero submission. Turning future output off is a
    different act and is not treated as darkness. On a fault, further submits
    stop and darkness stays unverified. This host cannot switch an LED supply.
    """
    clock = clock or time.monotonic
    writer = EvidenceWriter(evidence, int(packet.get("max_record_bytes") or 2_000_000))
    blockers = physical_blockers(packet)
    if blockers:
        return _write_result(writer, _physical_result(
            verdict="BLOCKED", blockers=blockers, device_opened=False,
        ))
    stage = (packet.get("stages") or {}).get("physical_mapping") or {}
    duration = stage.get("max_duration_s", 30)
    interval = stage.get("interval_s", 0)
    if not _finite(duration) or duration <= 0 or not _finite(interval) or interval < 0:
        return _write_result(writer, _physical_result(
            verdict="BLOCKED",
            blockers=["physical window limits are not finite and positive"],
            device_opened=False,
        ))
    session = None
    token = None
    payload: dict | None = None
    try:
        session = open_session()
        token = getattr(session, "lease_token", None)
        origin = clock()

        def guard(where: str) -> None:
            elapsed = clock() - origin
            if elapsed > float(duration):
                raise ExperimentError(
                    "acquisition deadline exceeded",
                    before_write=False,
                    detail={"where": where, "elapsed_s": elapsed, "limit_s": float(duration)},
                )

        info_wrap = session.info()
        guard("identity")
        info = info_wrap.get("info") if isinstance(info_wrap, Mapping) and "info" in info_wrap else info_wrap
        if not isinstance(info, Mapping):
            raise ExperimentError("INFO missing", before_write=True)
        denial = identity_denial(packet, info)
        writer.write_json("identity.json", {"info": info, "denial": denial})
        if denial:
            raise ExperimentError(denial, before_write=True)
        _metrics, _raw_metrics, counters = _read_metrics(session, writer, 0, clock)
        guard("metrics")
        status, _raw_status = _read_status(session, writer, 0, clock)
        guard("palette")
        epoch = counters.get("stream_epoch")
        if not _whole(epoch) or epoch <= 0:
            raise ExperimentError("stream epoch missing", before_write=True)
        profile = profile_denial(packet, status if isinstance(status, Mapping) else {})
        if profile:
            raise ExperimentError(profile, before_write=True)
        hold_ok = mapping_hold_authorised(packet)
        if hold_ok:
            if not isinstance(status, Mapping) or "emit_enabled" not in status:
                raise ExperimentError("emission state missing", before_write=True)
        else:
            gap = _emission_gap(packet, status if isinstance(status, Mapping) else None)
            if gap:
                raise ExperimentError(gap, before_write=True)
        if not isinstance(status, Mapping) or not _whole(status.get("submitted_generation")):
            raise ExperimentError("submitted generation missing", before_write=True)
        last = int(status["submitted_generation"])
        in_flight = _pair_in_flight(status)
        saved_config = None
        lifecycle = packet.get("mapping_lifecycle") if isinstance(packet.get("mapping_lifecycle"), Mapping) else {}
        restore_on_success = lifecycle.get("restore") is True
        hold_origin = None
        budget_s = float(lifecycle.get("blackout_budget_s") or 0.0) if _finite(lifecycle.get("blackout_budget_s")) else 0.0
        restore_reserve_s = float(lifecycle.get("restore_reserve_s") or 3.0)
        if restore_on_success or hold_ok:
            saved_config = _read_config(session)
            writer.write_json("mapping-saved-config.json", saved_config)
            rev_why = revision_denial(packet, saved_config) if packet.get("expected_revision") is not None else None
            if hold_ok and packet.get("expected_revision") is not None and saved_config.get("revision") != packet.get("expected_revision"):
                raise ExperimentError("stale config revision", before_write=True)
            if rev_why and not hold_ok:
                raise ExperimentError(rev_why, before_write=True)
        drain_timeout = float(lifecycle.get("completion_timeout_s") or 2.0)
        hold_timeline: dict[str, Any] = {}
        planned = _load_fixtures(packet) if hold_ok or restore_on_success else []
        if hold_ok:
            preflight = {
                "submitted_generation": last,
                "completed_generation": status.get("completed_generation"),
                "in_flight": in_flight,
                "epoch": int(epoch),
                "emit_enabled": status.get("emit_enabled"),
            }
            writer.write_json("preflight-observation.json", preflight)
            for item in planned:
                packet_gen = item.get("generation")
                if not _whole(packet_gen) or int(packet_gen) <= 0:
                    continue
                request = pair_fixture_request(item["lane0"], item["lane1"], int(packet_gen), int(epoch), KIND_MAP)
                code = host_admit({"last_generation": last, "expected_epoch": int(epoch), "in_flight": False}, request)
                if code != FIXTURE_OK:
                    raise ExperimentError(
                        f"fixture rejected before transmit ({code})",
                        before_write=True,
                        detail={"admit": code, "generation": packet_gen},
                    )
            if _lane_fault(status):
                raise ExperimentError("lane fault before fixture submit", before_write=True)
            hold_origin = clock()
            hold_timeline["staging_start_s"] = hold_origin

            def budget_remaining(*, reserving: bool) -> float:
                if budget_s <= 0:
                    return 2.0
                remaining = budget_s - (clock() - hold_origin)
                need = restore_reserve_s if reserving else 0.0
                return remaining - need

            def budget_check(where: str, *, reserving: bool = True) -> None:
                guard(where)
                if budget_s <= 0:
                    return
                left = budget_remaining(reserving=reserving)
                if left <= 1e-9:
                    raise ExperimentError(
                        "hold budget exhausted",
                        before_write=False,
                        detail={
                            "where": where,
                            "elapsed_s": clock() - hold_origin,
                            "budget_s": budget_s,
                            "remaining_s": budget_s - (clock() - hold_origin),
                            "reserve_s": restore_reserve_s if reserving else 0.0,
                        },
                    )

            def before_hold_tx(label: str) -> float:
                budget_check(f"stage-{label}", reserving=True)
                usable = budget_remaining(reserving=True)
                return min(2.0, max(0.01, usable))

            held = _stage_config(session, hold_emit_off_blob(saved_config), before_tx=before_hold_tx)
            if not held.get("ok"):
                raise ExperimentError("mapping hold commit failed", before_write=False)
            hold_timeline["hold_commit_s"] = clock()
            status, _raw_status = _read_status(session, writer, 1, clock)
            if not isinstance(status, Mapping) or status.get("emit_enabled") is not False:
                raise ExperimentError("hold did not clear emit", before_write=False)
            drain_origin = clock()
            while _pair_in_flight(status) and clock() - drain_origin <= drain_timeout:
                sleeper(float(interval) if float(interval) > 0 else 0.0)
                status, _raw_status = _read_status(session, writer, 2, clock)
                guard("drain")
            if _pair_in_flight(status):
                raise ExperimentError("outstanding pair work did not drain", before_write=False)
            in_flight = False
            last = int(status["submitted_generation"]) if _whole(status.get("submitted_generation")) else last
            epoch = status.get("stream_epoch", epoch)
            if not _whole(epoch) or int(epoch) <= 0:
                raise ExperimentError("stream epoch missing", before_write=False)
            epoch = int(epoch)
            allocated = allocate_fixture_generations(planned, last)
            for item in allocated:
                request = pair_fixture_request(item["lane0"], item["lane1"], int(item["generation"]), int(epoch), KIND_MAP)
                code = host_admit({"last_generation": last, "expected_epoch": int(epoch), "in_flight": False}, request)
                if code != FIXTURE_OK:
                    raise ExperimentError(
                        f"allocated fixture rejected before transmit ({code})",
                        before_write=False,
                        detail={"admit": code, "generation": item["generation"]},
                    )
            writer.write_json("generation-allocation.json", {
                "preflight": preflight,
                "final_last_generation": last,
                "final_epoch": int(epoch),
                "allocated": [
                    {"name": item.get("name"), "packet_generation": item.get("packet_generation"), "generation": item["generation"]}
                    for item in allocated
                ],
                "payload_preserved": True,
            })
        else:
            allocated = None

            def budget_check(where: str, *, reserving: bool = True) -> None:
                guard(where)

        if _lane_fault(status):
            payload = _physical_result(
                verdict="FAULT",
                reason="lane fault before fixture submit",
                device_opened=True,
                fixture_submits=0,
            )
            payload = _write_result(writer, payload)
            return payload
        submits = []
        fixtures = allocated if allocated is not None else _load_fixtures(packet)
        for fixture in fixtures:
            budget_check("fixture")
            gate = {"last_generation": last, "expected_epoch": int(epoch), "in_flight": in_flight}
            request = pair_fixture_request(fixture["lane0"], fixture["lane1"], fixture["generation"], int(epoch), KIND_MAP)
            code = host_admit(gate, request)
            if code != FIXTURE_OK:
                raise ExperimentError(
                    f"fixture rejected before transmit ({code})",
                    before_write=True,
                    detail={"admit": code, "generation": fixture["generation"]},
                )
            got = session.transact(OP_PAIR_FIXTURE, request)
            budget_check("fixture-submit")
            body = got.get("body") if isinstance(got.get("body"), (bytes, bytearray)) else b""
            _store_body(writer, f"fixture-{fixture['generation']:04d}.bin", body, {"ok": bool(got.get("ok")), "status": got.get("status")}, op=OP_PAIR_FIXTURE, payload=request, clock=clock)
            parsed = None
            try:
                parsed = json.loads(body.decode("utf-8")) if body else None
            except (UnicodeError, json.JSONDecodeError):
                parsed = None
            accepted = bool(got.get("ok") and isinstance(parsed, dict) and parsed.get("accepted") is True)
            submits.append({"generation": fixture["generation"], "kind": "map", "accepted": accepted})
            if not accepted:
                payload = _physical_result(
                    verdict="FAULT",
                    reason="fixture submit was not accepted",
                    device_opened=True,
                    fixture_submits=len(submits),
                    submits=submits,
                )
                payload = _write_result(writer, payload)
                return payload
            last = int(fixture["generation"])
            in_flight = True
            completion_timeout = float(lifecycle.get("completion_timeout_s") or 2.0)
            dwell_s = float(lifecycle.get("fixture_dwell_s") or 0.0)
            status, wait_reason = _wait_pair_complete(
                session, writer, last, int(epoch), clock, sleeper, guard,
                timeout_s=completion_timeout, interval_s=float(interval),
            )
            if wait_reason:
                payload = _physical_result(
                    verdict="FAULT",
                    reason=wait_reason,
                    device_opened=True,
                    fixture_submits=len(submits),
                    submits=submits,
                    restore_attempted=False,
                )
                payload = _write_result(writer, payload)
                return payload
            in_flight = False
            if dwell_s > 0:
                sleeper(dwell_s)
                budget_check("fixture-dwell")
        budget_check("black")
        black_generation = last + 1
        black = bytes(PAIR_LANE_BYTES)
        black_request = pair_fixture_request(black, black, black_generation, int(epoch), KIND_BLACK)
        code = host_admit(
            {"last_generation": last, "expected_epoch": int(epoch), "in_flight": in_flight},
            black_request,
        )
        if code != FIXTURE_OK:
            raise ExperimentError(
                f"black pair rejected before transmit ({code})",
                before_write=True,
                detail={"admit": code, "generation": black_generation},
            )
        got = session.transact(OP_PAIR_FIXTURE, black_request)
        budget_check("black-submit")
        body = got.get("body") if isinstance(got.get("body"), (bytes, bytearray)) else b""
        _store_body(writer, "black-pair.bin", body, {"ok": bool(got.get("ok"))}, op=OP_PAIR_FIXTURE, payload=black_request, clock=clock)
        submits.append({"generation": black_generation, "kind": "black", "accepted": bool(got.get("ok"))})
        darkness = False
        black_reason = "darkness unverified"
        polls = 0
        while polls < 8:
            status, _raw_status = _read_status(session, writer, 1000 + polls, clock)
            guard("black-poll")
            polls += 1
            ok, why = _fixture_completion(status, black_generation, int(epoch))
            if why and ("fault" in why or "missing" in why):
                black_reason = why
                break
            if ok:
                darkness = True
                black_reason = None
                break
            sleeper(float(interval))
            guard("black-spacing")
        restored = False
        restore_verified = False
        if (not darkness) or _lane_fault(status):
            payload = _physical_result(
                verdict="FAULT",
                reason=black_reason or "darkness unverified",
                darkness_verified=False,
                darkness_unverified=True,
                device_opened=True,
                fixture_submits=len(submits),
                submits=submits,
                black_generation=black_generation,
                restore_attempted=False,
                restore_applied=False,
                restore_verified=False,
                unresolved=config_fields(saved_config) if saved_config else None,
            )
            payload = _write_result(writer, payload)
            return payload
        if restore_on_success and saved_config is not None:
            budget_check("restore", reserving=False)
            blob = proto.pack_config(saved_config)

            def before_restore_tx(label: str) -> float:
                budget_check(f"restore-{label}", reserving=False)
                if budget_s <= 0:
                    return 2.0
                remaining = budget_s - (clock() - hold_origin) if hold_origin is not None else 2.0
                return min(2.0, max(0.01, remaining))

            staged = _stage_config(session, blob, before_tx=before_restore_tx)
            if not staged.get("ok"):
                raise ExperimentError("mapping restore commit failed", before_write=False)
            restored = True
            again = _read_config(session)
            if config_fields(again) != config_fields(saved_config):
                payload = _physical_result(
                    verdict="FAULT",
                    reason="restore readback mismatch",
                    darkness_verified=True,
                    darkness_unverified=False,
                    device_opened=True,
                    fixture_submits=len(submits),
                    submits=submits,
                    black_generation=black_generation,
                    restore_attempted=True,
                    restore_applied=True,
                    restore_verified=False,
                    unresolved=config_fields(again),
                )
                payload = _write_result(writer, payload)
                return payload
            first_metrics, _raw_m, first_c = _read_metrics(session, writer, 2000, clock)
            first_status, _raw_s = _read_status(session, writer, 2001, clock)
            observe_s = float(lifecycle.get("restore_observe_s") or max(float(interval), 0.0))
            sleeper(observe_s)
            _second_metrics, _raw_m2, second_c = _read_metrics(session, writer, 2002, clock)
            second_status, _raw_s2 = _read_status(session, writer, 2003, clock)
            budget_check("restore-progress", reserving=False)
            if saved_config.get("emit_on") == 1:
                ok_led, led_why = _led_restore_evidence(
                    first_status if isinstance(first_status, Mapping) else None,
                    second_status if isinstance(second_status, Mapping) else None,
                    packet.get("expected_profile") if isinstance(packet.get("expected_profile"), Mapping) else {},
                )
                if not ok_led:
                    payload = _physical_result(
                        verdict="FAULT",
                        reason=led_why or "restored LED output did not advance",
                        darkness_verified=True,
                        darkness_unverified=False,
                        device_opened=True,
                        fixture_submits=len(submits),
                        submits=submits,
                        black_generation=black_generation,
                        restore_attempted=True,
                        restore_applied=True,
                        restore_verified=False,
                        ap_hops_delta=delta_value(first_c.get("ap_hops"), second_c.get("ap_hops")),
                    )
                    payload = _write_result(writer, payload)
                    return payload
            restore_verified = True
            hold_timeline["restore_verified_s"] = clock()
        payload = _physical_result(
            verdict="SEQUENCED" if darkness else "FAULT",
            reason=None if darkness else black_reason,
            darkness_verified=darkness,
            darkness_unverified=not darkness,
            physical_observation=False,
            device_opened=True,
            fixture_submits=len(submits),
            submits=submits,
            black_generation=black_generation,
            restore_attempted=bool(restore_on_success and darkness),
            restore_applied=restored,
            restore_verified=restore_verified,
            hold_elapsed_s=(clock() - hold_origin) if hold_origin is not None else None,
            hold_budget_s=budget_s or None,
            hold_timeline=hold_timeline or None,
            generation_allocation=allocated and [
                {"packet_generation": item.get("packet_generation"), "generation": item["generation"]}
                for item in allocated
            ],
        )
        payload = _write_result(writer, payload)
        return payload
    except ExperimentError as exc:
        payload = _physical_result(
            verdict="DENIED" if exc.before_write else "FAULT",
            reason=exc.reason,
            detail=exc.detail,
            before_write=exc.before_write,
            device_opened=session is not None,
        )
        payload = _write_result(writer, payload)
        return payload
    except (KeyboardInterrupt, RecordError, OSError, *_transport_faults()) as exc:
        payload = _physical_result(
            verdict="FAULT",
            reason=type(exc).__name__,
            detail=str(exc),
            device_opened=session is not None,
        )
        payload = _write_result(writer, payload)
        return payload
    finally:
        _close_session(session, evidence, token, payload)


RATE_HZ = 24000
QUIET_S = 15.0
MUSIC_S = 60.0
PAUSE_S = 15.0
RESUME_S = 30.0
CLIP_S = 30.0
CLIP_SAMPLES = int(RATE_HZ * CLIP_S)
STABILITY_S = 600.0


def _clip_blockers(clip: Mapping[str, Any], label: str) -> list[str]:
    blockers: list[str] = []
    if not isinstance(clip, Mapping):
        return [f"{label} is not an object"]
    if clip.get("permitted") is not True or not clip.get("identity"):
        blockers.append(f"{label} has no recorded permit and identity")
    path = clip.get("path")
    if not isinstance(path, str) or not path:
        return blockers + [f"{label} path is missing"]
    try:
        data = Path(path).read_bytes()
    except OSError as exc:
        return blockers + [f"{label} unreadable ({type(exc).__name__})"]
    report = pcm_s16le_report(data, provenance={"permitted": True, "identity": clip.get("identity")})
    if report.get("sample_count") != CLIP_SAMPLES:
        blockers.append(f"{label} is not a {CLIP_S:.0f}s 24 kHz clip")
    if report.get("named_music") is not True:
        blockers.append(f"{label} is not named music")
    want = clip.get("sha256")
    digest = hashlib.sha256(data).hexdigest()
    if not isinstance(want, str) or digest != want:
        blockers.append(f"{label} hash does not match the packet")
    return blockers


def sequence_blockers(packet: Mapping[str, Any]) -> list[str]:
    """Refuse a music sequence that is not this image, this pin, or real clips."""
    blockers: list[str] = []
    expect = packet.get("expected_profile") or {}
    if expect.get("din_b") != "P604":
        blockers.append("sequence requires din_b P604")
    if expect.get("emit_enabled") is not True:
        blockers.append("sequence requires emit to already be on; this entry does not enable it")
    stage = (packet.get("stages") or {}).get("group_b") or {}
    wanted = {"quiet_s": QUIET_S, "music_s": MUSIC_S, "pause_s": PAUSE_S, "resume_s": RESUME_S}
    for name, duration in wanted.items():
        if stage.get(name) != duration:
            blockers.append(f"group_b {name} must be {duration:.0f}")
    clips = (packet.get("sequence") or {}).get("music_clips")
    resume = (packet.get("sequence") or {}).get("resume_clip")
    if not isinstance(clips, list) or len(clips) != 2:
        blockers.append("music window needs two distinct 30s clips")
        clips = []
    digests = []
    for index, clip in enumerate(clips):
        blockers.extend(_clip_blockers(clip, f"music clip {index}"))
        if isinstance(clip, Mapping) and isinstance(clip.get("sha256"), str):
            digests.append(clip["sha256"])
    blockers.extend(_clip_blockers(resume, "resume clip") if isinstance(resume, Mapping) else ["resume clip is missing"])
    resume_sha = resume.get("sha256") if isinstance(resume, Mapping) else None
    if len(set(digests)) != len(digests):
        blockers.append("music clips repeat one another")
    if resume_sha in digests:
        blockers.append("resume clip repeats a music-window clip")
    return blockers


def functional_blockers(packet: Mapping[str, Any]) -> list[str]:
    """Refuse a functional run that cannot apply an in-window admitted control."""
    blockers = sequence_blockers(packet)
    controls = packet.get("controls")
    if not isinstance(controls, list) or not controls:
        blockers.append("functional controls are not declared")
    else:
        for index, change in enumerate(controls):
            if not isinstance(change, Mapping):
                blockers.append(f"control {index} is not an object")
                continue
            unknown = [key for key in change if key not in CONTROL_FIELDS]
            if unknown:
                blockers.append(f"control {index} field is not admitted: {unknown[0]}")
            if "emit_on" in change:
                blockers.append(f"control {index} would change emission")
            for key, value in change.items():
                if key in CONTROL_FIELDS and not _whole(value):
                    blockers.append(f"control {index} {key} is not an integer")
    if packet.get("expected_revision") is None:
        blockers.append("functional expected_revision is missing")
    at = packet.get("controls_at_s", CONTROL_AT_S)
    if not _finite(at) or not (QUIET_S < float(at) < QUIET_S + MUSIC_S):
        blockers.append("controls_at_s is not strictly inside the music interval")
    return blockers


def stability_tracks(playlist: Mapping[str, Any]) -> tuple[list[str], list[dict[str, Any]]]:
    """Admit tracks from their sample counts. A declared second count is not enough."""
    tracks = playlist.get("tracks")
    if not isinstance(tracks, list) or not tracks:
        return ["stability tracks are not declared; a length check is not an execution"], []
    blockers: list[str] = []
    seen: list[str] = []
    ready: list[dict[str, Any]] = []
    total = 0
    for index, track in enumerate(tracks):
        if not isinstance(track, Mapping):
            blockers.append(f"stability track {index} is not an object")
            continue
        try:
            data = Path(str(track.get("path"))).read_bytes()
        except (OSError, TypeError):
            blockers.append(f"stability track {index} is unreadable")
            continue
        digest = hashlib.sha256(data).hexdigest()
        if digest != track.get("sha256"):
            blockers.append(f"stability track {index} hash mismatch")
        if digest in seen:
            blockers.append(f"stability track {index} repeats an earlier recording")
        seen.append(digest)
        samples = len(data) // 2
        if samples != track.get("samples"):
            blockers.append(f"stability track {index} sample count does not match the file")
        report = pcm_s16le_report(
            data,
            provenance={"permitted": track.get("permitted") is True, "identity": track.get("identity")},
        )
        if report.get("named_music") is not True:
            blockers.append(f"stability track {index} is not named music")
        total += samples
        ready.append({
            "path": str(track.get("path")),
            "samples": samples,
            "sha256": digest,
            "identity": track.get("identity"),
        })
    seconds = total / RATE_HZ
    if seconds + 1e-9 < STABILITY_S:
        blockers.append(
            f"stability tracks sum to {seconds:.3f}s, "
            f"{STABILITY_S - seconds:.3f}s short of 600s, and must not be looped"
        )
    declared = playlist.get("seconds")
    if _finite(declared) and abs(float(declared) - seconds) > (1.0 / RATE_HZ):
        blockers.append("stability seconds do not match the sample counts")
    return blockers, ready


def stability_blockers(packet: Mapping[str, Any]) -> list[str]:
    playlist = packet.get("stability_playlist") or {}
    seconds = playlist.get("seconds")
    if playlist.get("loop") is True:
        return ["stability must not loop a short clip"]
    if not _finite(seconds):
        return ["stability playlist length is missing"]
    if float(seconds) + 1e-6 < STABILITY_S:
        short = STABILITY_S - float(seconds)
        return [f"stability playlist is {short:.3f}s short of 600s and must not be looped"]
    track_blockers, _ready = stability_tracks(playlist)
    return track_blockers


class PcmPlayer:
    """Play raw 24 kHz mono s16le through afplay. Does not set the volume."""

    def __init__(self, directory: Path) -> None:
        self.directory = Path(directory)
        self._proc: Any = None

    def playing(self) -> bool:
        return self._proc is not None and self._proc.poll() is None

    def failed(self) -> bool:
        proc = self._proc
        if proc is None:
            return False
        code = proc.poll()
        return code not in (None, 0)

    def stop(self) -> None:
        proc = self._proc
        self._proc = None
        if proc is not None and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except Exception:
                proc.kill()

    def start(self, path: Path) -> None:
        import subprocess
        self.stop()
        data = Path(path).read_bytes()
        header = struct.pack(
            "<4sI4s4sIHHIIHH4sI",
            b"RIFF", 36 + len(data), b"WAVE", b"fmt ", 16,
            1, 1, RATE_HZ, RATE_HZ * 2, 2, 16, b"data", len(data),
        )
        wav = self.directory / "playing.wav"
        wav.write_bytes(header + data)
        self._proc = subprocess.Popen(["afplay", str(wav)])


def _sequence_sample(session, writer: EvidenceWriter, index: int, clock: Callable[[], float], phase: str) -> dict[str, Any]:
    _metrics, _raw_metrics, counters = _read_metrics(session, writer, index, clock)
    status, _raw_status = _read_status(session, writer, index, clock)
    palette = status if isinstance(status, Mapping) else {}
    row = {"phase": phase, "t": clock(), "counters": counters, "palette": present(palette, PAIR_FIELDS)}
    row["musical"] = palette["musical"] if "musical" in palette else UNKNOWN
    return row


def show_control_blob(current: Mapping[str, Any], change: Mapping[str, Any]) -> bytes:
    """Palette or mode change that keeps emit on and keeps the brightness."""
    if "emit_on" in change and int(change["emit_on"]) != 1:
        raise ExperimentError("control would clear emit", before_write=True)
    allowed = {"palette_a", "palette_b", "mode_a", "mode_b"}
    unknown = [key for key in change if key not in allowed]
    if unknown:
        raise ExperimentError(f"control field is not admitted: {unknown[0]}", before_write=True)
    desired = dict(current)
    for key in allowed:
        if key in change:
            desired[key] = int(change[key])
    desired["emit_on"] = 1
    desired["brightness"] = int(current.get("brightness", 24))
    desired["flags"] = int(current.get("flags") or 0) | 4
    blob = proto.pack_config(desired)
    parsed = proto.unpack_config(blob)
    if parsed["emit_on"] != 1 or int(parsed["brightness"]) != int(desired["brightness"]):
        raise ExperimentError("control would change the power envelope", before_write=True)
    return blob


def _config_transact(session, payload: bytes, timeout: float | None) -> dict:
    if timeout is None:
        return session.transact(OP_CONFIG, payload)
    try:
        return session.transact(OP_CONFIG, payload, timeout=timeout)
    except TypeError:
        return session.transact(OP_CONFIG, payload)


def _stage_config(session, blob: bytes, *, before_tx=None) -> dict:
    """BEGIN/APPEND/COMMIT. before_tx(label) runs before each write and returns the transport timeout."""

    def tx(label: str, payload: bytes) -> dict:
        timeout = before_tx(label) if before_tx is not None else None
        return _config_transact(session, payload, timeout)

    begin = tx("begin", proto.pack_config_sub(proto.CFG_BEGIN_SET, struct.pack("<I", len(blob))))
    if not begin.get("ok"):
        return begin
    offset = 0
    while offset < len(blob):
        take = min(256, len(blob) - offset)
        got = tx(
            "append",
            proto.pack_config_sub(proto.CFG_APPEND_SET, struct.pack("<I", offset) + blob[offset:offset + take]),
        )
        if not got.get("ok"):
            return got
        offset += take
    return tx("commit", proto.pack_config_sub(proto.CFG_COMMIT_SET))


def apply_show_controls(session, changes: list, *, expected_revision: Any, clock: Callable[[], float]) -> dict:
    got = session.transact(OP_CONFIG, proto.pack_config_sub(proto.CFG_GET_CONFIG))
    body = got.get("body") if isinstance(got.get("body"), (bytes, bytearray)) else b""
    if not got.get("ok") or len(body) < proto.CONFIG_BYTES_V1:
        raise ExperimentError("control read failed", before_write=True)
    current = proto.unpack_config(body)
    if current.get("revision") != expected_revision:
        raise ExperimentError("stale config revision", before_write=True)
    applied = []
    for change in changes:
        if not isinstance(change, Mapping):
            raise ExperimentError("control is not an object", before_write=True)
        before = current.get("revision")
        changed = False
        for key, value in change.items():
            if key in CONTROL_FIELDS and int(current.get(key)) != int(value):
                changed = True
        if not changed:
            raise ExperimentError("control does not change a palette or mode value", before_write=True)
        blob = show_control_blob(current, change)
        commit_at = clock()
        staged = _stage_config(session, blob)
        if not staged.get("ok"):
            raise ExperimentError("control commit failed", before_write=False)
        again = session.transact(OP_CONFIG, proto.pack_config_sub(proto.CFG_GET_CONFIG))
        raw = again.get("body") if isinstance(again.get("body"), (bytes, bytearray)) else b""
        if not again.get("ok") or len(raw) < proto.CONFIG_BYTES_V1:
            raise ExperimentError("control readback failed", before_write=False)
        readback = proto.unpack_config(raw)
        for key, value in change.items():
            if int(readback.get(key)) != int(value):
                raise ExperimentError("control readback mismatch", before_write=False)
        if readback.get("emit_on") != 1:
            raise ExperimentError("control readback cleared emit", before_write=False)
        if readback.get("brightness") != current.get("brightness"):
            raise ExperimentError("control readback changed brightness", before_write=False)
        if readback.get("revision") == before:
            raise ExperimentError("control revision did not advance", before_write=False)
        for key in CONTROL_FIELDS:
            if key in change:
                continue
            if readback.get(key) != current.get(key):
                raise ExperimentError(f"control changed untouched {key}", before_write=False)
        current = readback
        applied.append({key: int(change[key]) for key in change})
    return {
        "applied": applied,
        "revision": current.get("revision"),
        "commit_monotonic_s": commit_at,
        "readback_monotonic_s": clock(),
    }


def execute_sequence(
    packet: Mapping[str, Any],
    evidence: Path,
    open_session: Callable[[], Any],
    player,
    *,
    sleeper=time.sleep,
    clock: Callable[[], float] | None = None,
    apply_controls: bool = False,
) -> dict:
    """15s quiet, 60s named music, 15s pause, 30s resume. Does not enable emit."""
    clock = clock or time.monotonic
    writer = EvidenceWriter(evidence, int(packet.get("max_record_bytes") or 8_000_000))
    blockers = functional_blockers(packet) if apply_controls else sequence_blockers(packet)
    if blockers:
        return _write_result(writer, {
            "verdict": "BLOCKED",
            "scope": "functional_sequence" if apply_controls else "group_b_sequence",
            "blockers": blockers,
            "device_opened": False,
            "emit_enabled_by_this_run": False,
            "physical_observation": False,
        })
    session = None
    token = None
    payload: dict | None = None
    clips = packet["sequence"]["music_clips"]
    resume = packet["sequence"]["resume_clip"]
    try:
        session = open_session()
        token = getattr(session, "lease_token", None)
        info_wrap = session.info()
        info = info_wrap.get("info") if isinstance(info_wrap, Mapping) and "info" in info_wrap else info_wrap
        if not isinstance(info, Mapping):
            raise ExperimentError("INFO missing", before_write=True)
        denial = identity_denial(packet, info)
        if denial:
            raise ExperimentError(denial, before_write=True)
        _metrics, _raw, counters = _read_metrics(session, writer, 0, clock)
        status, _raw_status = _read_status(session, writer, 0, clock)
        if not isinstance(status, Mapping):
            raise ExperimentError("emission state missing", before_write=True)
        gap = profile_denial(packet, status)
        if gap:
            raise ExperimentError(gap, before_write=True)
        if status.get("emit_enabled") is not True:
            raise ExperimentError("emission is not already on; this entry does not enable it", before_write=True)
        rows: list[dict[str, Any]] = []
        index = 1

        def take(phase: str) -> dict[str, Any]:
            nonlocal index
            row = _sequence_sample(session, writer, index, clock, phase)
            index += 1
            rows.append(row)
            return row

        origin = clock()
        schedule_t = 0.0
        control_report = None
        control_at = float(packet.get("controls_at_s", CONTROL_AT_S)) if apply_controls else None
        stage_bounds: dict[str, Any] = {}

        def elapsed() -> float:
            return clock() - origin

        def hold(seconds: float) -> None:
            nonlocal schedule_t
            started = clock()
            sleeper(seconds)
            schedule_t += float(seconds)
            if apply_controls and clock() - started + 1e-9 < float(seconds):
                raise ExperimentError("monotonic clock did not cover the stage", before_write=False)

        def maybe_apply_controls() -> None:
            nonlocal control_report
            if not apply_controls or control_report is not None:
                return
            stamp = elapsed()
            if control_at is None or stamp + 1e-9 < control_at:
                return
            if stamp <= QUIET_S or stamp >= QUIET_S + MUSIC_S:
                raise ExperimentError("control time left the music interval", before_write=True)
            control_report = apply_show_controls(
                session,
                list(packet.get("controls") or []),
                expected_revision=packet.get("expected_revision"),
                clock=clock,
            )
            control_report["schedule_s"] = stamp
            control_report["elapsed_s"] = stamp

        def wait_playback(seconds: float) -> None:
            """Sleep the declared window. An early clean exit is not quiet time."""
            started = clock()
            left = float(seconds)
            while left > 1e-9:
                step = min(1.0, left)
                hold(step)
                maybe_apply_controls()
                if player.failed():
                    raise ExperimentError("playback failed", before_write=False)
                if not player.playing() and left - step > 0.25:
                    raise ExperimentError(
                        "playback ended before its declared duration",
                        before_write=False,
                    )
                left -= step
            if apply_controls and clock() - started + 1e-9 < float(seconds):
                raise ExperimentError("monotonic clock did not cover the stage", before_write=False)

        player.stop()
        quiet_t0 = elapsed()
        quiet_start = take("quiet")
        if player.playing():
            raise ExperimentError("quiet window started playback", before_write=False)
        hold(QUIET_S)
        quiet_end = take("quiet")
        stage_bounds["quiet"] = {"start_s": quiet_t0, "end_s": elapsed(), "duration_s": elapsed() - quiet_t0}
        player.start(Path(clips[0]["path"]))
        if not player.playing():
            raise ExperimentError("first music clip did not start", before_write=False)
        music_t0 = elapsed()
        music_start = take("music")
        wait_playback(CLIP_S)
        player.start(Path(clips[1]["path"]))
        if not player.playing():
            raise ExperimentError("second music clip did not start", before_write=False)
        wait_playback(CLIP_S)
        if apply_controls and control_report is None:
            raise ExperimentError("controls were not applied inside the music interval", before_write=False)
        music_end = take("music")
        stage_bounds["music"] = {"start_s": music_t0, "end_s": elapsed(), "duration_s": elapsed() - music_t0}
        player.stop()
        pause_t0 = elapsed()
        pause_start = take("pause")
        if player.playing():
            raise ExperimentError("pause window left playback running", before_write=False)
        hold(PAUSE_S)
        take("pause")
        stage_bounds["pause"] = {"start_s": pause_t0, "end_s": elapsed(), "duration_s": elapsed() - pause_t0}
        player.start(Path(resume["path"]))
        if not player.playing():
            raise ExperimentError("resume clip did not start", before_write=False)
        resume_t0 = elapsed()
        resume_start = take("resume")
        wait_playback(RESUME_S)
        resume_end = take("resume")
        stage_bounds["resume"] = {"start_s": resume_t0, "end_s": elapsed(), "duration_s": elapsed() - resume_t0}
        player.stop()
        quiet_rows = [row for row in rows if row["phase"] == "quiet"]
        music_rows = [row for row in rows if row["phase"] == "music"]
        pause_rows = [row for row in rows if row["phase"] == "pause"]
        resume_rows = [row for row in rows if row["phase"] == "resume"]
        musical = sum(1 for row in music_rows if row.get("musical") is True)
        quiet_musical = sum(1 for row in quiet_rows if row.get("musical") is True)
        pause_end_musical = pause_rows[-1].get("musical") if pause_rows else UNKNOWN
        resume_musical = sum(1 for row in resume_rows if row.get("musical") is True)
        emitted_delta = delta_value(music_start["palette"].get("emitted"), music_end["palette"].get("emitted"))
        error_delta = delta_value(quiet_start["palette"].get("emit_errors"), resume_end["palette"].get("emit_errors"))
        reason = None
        verdict = "PASS"
        if resume_end["palette"].get("din_b") != "P604":
            verdict, reason = "FAIL", "din_b left P604"
        elif error_delta not in (0, 0.0):
            verdict, reason = "FAIL", "emit errors changed"
        elif not isinstance(emitted_delta, (int, float)) or isinstance(emitted_delta, bool) or emitted_delta <= 0:
            verdict, reason = "FAIL", "frames did not advance during music"
        elif musical < 1:
            verdict, reason = "FAIL", "recording played but the board did not mark the window musical"
        elif quiet_musical:
            verdict, reason = "FAIL", "quiet window was marked musical, so the clip is not distinguished"
        # Mode 32 may stay musical for 5 s after the sound stops. The 15 s
        # pause is longer than that dwell, so only the last pause sample must
        # be quiet. The first pause sample is allowed to stay musical.
        functional, functional_reason = verdict, reason
        expect = packet.get("expected_profile") if isinstance(packet.get("expected_profile"), Mapping) else {}
        expected_show = {name: expect[name] for name in SHOW_FIELDS if name in expect}
        health = evaluate_run_health(
            rows,
            expected_show=expected_show or None,
            min_window_s=1.0 if apply_controls else STABILITY_SAMPLE_S,
        )
        if functional == "PASS" and pause_end_musical is not False:
            functional, functional_reason = "FAIL", "pause did not become quiet"
        elif functional == "PASS" and resume_musical < 1:
            functional, functional_reason = "FAIL", "resume did not wake"
        elif functional == "PASS" and apply_controls and health.get("verdict") != "PASS":
            functional, functional_reason = "FAIL", str(health.get("reason") or "health failed")
        elif functional == "PASS" and apply_controls:
            stamp = None
            if isinstance(control_report, Mapping):
                stamp = control_report.get("elapsed_s", control_report.get("schedule_s"))
                commit_t = control_report.get("commit_monotonic_s")
                if _finite(commit_t):
                    stamp = float(commit_t) - origin
            if not isinstance(control_report, Mapping):
                functional, functional_reason = "FAIL", "controls were not applied"
            elif not _finite(stamp) or not (QUIET_S < float(stamp) < QUIET_S + MUSIC_S):
                functional, functional_reason = "FAIL", "control commit was not inside the music interval"
            else:
                music_lo = stage_bounds.get("music", {}).get("start_s")
                music_hi = stage_bounds.get("music", {}).get("end_s")
                readback_t = control_report.get("readback_monotonic_s")
                readback_elapsed = (float(readback_t) - origin) if _finite(readback_t) else None
                if not _finite(music_lo) or not _finite(music_hi) or not (music_lo < float(stamp) < music_hi):
                    functional, functional_reason = "FAIL", "control commit was not inside the music interval"
                elif readback_elapsed is not None and not (music_lo < readback_elapsed < music_hi):
                    functional, functional_reason = "FAIL", "control readback was not inside the music interval"
        elif functional == "PASS" and not apply_controls:
            functional, functional_reason = "BLOCKED", "read-only sequence does not apply controls"
        if apply_controls:
            verdict, reason = functional, functional_reason
        payload = {
            "verdict": verdict,
            "reason": reason,
            "functional_verdict": functional,
            "functional_reason": functional_reason,
            "health": health,
            "scope": "group_b_sequence",
            "device_opened": True,
            "emit_enabled_by_this_run": False,
            "physical_observation": False,
            "musical_samples": musical,
            "pause_end_musical": pause_end_musical,
            "resume_musical_samples": resume_musical,
            "dwell_allowance_s": 5,
            "controls_applied": control_report,
            "stage_bounds": stage_bounds,
            "emitted_delta_during_music": emitted_delta,
            "emitted_delta_unit": "cumulative emitted frames, not a frame gap",
            "stages": ["quiet", "music", "pause", "resume"],
            "samples": len(rows),
            "stream_epoch": counters.get("stream_epoch", UNKNOWN),
            "quiet_emitted": quiet_end["palette"].get("emitted"),
            "resume_started": True,
            "field_units": {
                "emitted": "cumulative frames the pair transmitter accepted",
                "emit_errors": "cumulative transmitter errors",
                "musical": "mode-32 musical flag after the 5s dwell rule",
                "stream_epoch": "audio stream generation",
                "asrc_consumed": "cumulative input samples accepted by the resampler",
                "ap_hops": "cumulative analysis hops",
                "pair_epoch_drops": "cumulative pairs dropped when the stream epoch changed",
                "pair_fault": "pair fault code; a missing value is UNKNOWN, not zero",
            },
        }
        payload = _write_result(writer, payload)
        return payload
    except ExperimentError as exc:
        player.stop()
        payload = {
            "verdict": "DENIED" if exc.before_write else "FAULT",
            "reason": exc.reason,
            "detail": exc.detail,
            "before_write": exc.before_write,
            "scope": "group_b_sequence",
            "device_opened": session is not None,
            "emit_enabled_by_this_run": False,
            "physical_observation": False,
        }
        payload = _write_result(writer, payload)
        return payload
    except (KeyboardInterrupt, RecordError, OSError, *_transport_faults()) as exc:
        player.stop()
        payload = {
            "verdict": "FAULT",
            "reason": type(exc).__name__,
            "detail": str(exc),
            "scope": "group_b_sequence",
            "device_opened": session is not None,
            "emit_enabled_by_this_run": False,
            "physical_observation": False,
        }
        payload = _write_result(writer, payload)
        return payload
    finally:
        _close_session(session, evidence, token, payload)


def execute_stability(
    packet: Mapping[str, Any],
    evidence: Path,
    open_session: Callable[[], Any] | None = None,
    player=None,
    *,
    sleeper=time.sleep,
    clock: Callable[[], float] | None = None,
) -> dict:
    """Ten-minute run. A short or looped playlist never opens the board."""
    clock = clock or time.monotonic
    writer = EvidenceWriter(evidence, int(packet.get("max_record_bytes") or 8_000_000))
    blockers = stability_blockers(packet)
    if blockers:
        return _write_result(writer, {
            "verdict": "BLOCKED",
            "scope": "stability_preflight",
            "completed": False,
            "blockers": blockers,
            "device_opened": False,
            "emit_enabled_by_this_run": False,
            "looped": False,
            "physical_observation": False,
        })
    _ignored, tracks = stability_tracks(packet.get("stability_playlist") or {})
    if open_session is None:
        open_session = open_live_session_factory(evidence, pair_fixture=False)
    if player is None:
        player = PcmPlayer(evidence)
    session = None
    token = None
    payload: dict | None = None
    try:
        session = open_session()
        token = getattr(session, "lease_token", None)
        info_wrap = session.info()
        info = info_wrap.get("info") if isinstance(info_wrap, Mapping) and "info" in info_wrap else info_wrap
        if not isinstance(info, Mapping):
            raise ExperimentError("INFO missing", before_write=True)
        denial = identity_denial(packet, info)
        if denial:
            raise ExperimentError(denial, before_write=True)
        _metrics, _raw, counters = _read_metrics(session, writer, 0, clock)
        status, _raw_status = _read_status(session, writer, 0, clock)
        if not isinstance(status, Mapping):
            raise ExperimentError("emission state missing", before_write=True)
        gap = profile_denial(packet, status)
        if gap:
            raise ExperimentError(gap, before_write=True)
        if status.get("emit_enabled") is not True:
            raise ExperimentError("emission is not already on; this entry does not enable it", before_write=True)
        rows: list[dict[str, Any]] = []
        index = 1
        origin = clock()
        deadline = origin + STABILITY_S
        sample_interval = STABILITY_SAMPLE_S
        stage = (packet.get("stages") or {}).get("stability") or {}
        if _finite(stage.get("interval_s")) and float(stage["interval_s"]) > 0:
            sample_interval = float(stage["interval_s"])
        last_sample_at = origin
        health_fault: dict[str, Any] | None = None

        def take(phase: str) -> dict[str, Any]:
            nonlocal index, last_sample_at, health_fault
            row = _sequence_sample(session, writer, index, clock, phase)
            index += 1
            last_sample_at = clock()
            rows.append(row)
            expect = packet.get("expected_profile") if isinstance(packet.get("expected_profile"), Mapping) else {}
            expected_show = {name: expect[name] for name in SHOW_FIELDS if name in expect}
            health = evaluate_run_health(rows, expected_show=expected_show or None) if len(rows) >= 2 else None
            if health_fault is None and health and health.get("verdict") != "PASS":
                health_fault = health
            return row

        def wait_playback(seconds: float) -> None:
            left = float(seconds)
            while left > 1e-9:
                if clock() >= deadline:
                    return
                step = min(1.0, left, max(0.0, deadline - clock()))
                if step <= 0:
                    return
                sleeper(step)
                left -= step
                if clock() - last_sample_at >= sample_interval - 1e-9:
                    take("stability")
                    if health_fault is not None:
                        return
                if player.failed():
                    raise ExperimentError("playback failed", before_write=False)
                if not player.playing() and left > 0.25 and clock() < deadline:
                    raise ExperimentError("playback ended before its declared duration", before_write=False)

        started = []
        remaining = STABILITY_S
        source_duration = sum(track["samples"] for track in tracks) / RATE_HZ
        for track in tracks:
            if health_fault is not None or remaining <= 1e-9 or clock() >= deadline:
                break
            play_s = min(track["samples"] / RATE_HZ, remaining)
            player.start(Path(track["path"]))
            if not player.playing():
                raise ExperimentError("stability track did not start", before_write=False)
            started.append(track["identity"])
            take("stability")
            wait_playback(play_s)
            remaining = STABILITY_S - (clock() - origin)
            if player.playing() and remaining <= 1e-9:
                player.stop()
            elif player.playing() and play_s + 1e-9 < track["samples"] / RATE_HZ:
                player.stop()
        player.stop()
        if health_fault is None:
            take("stability")
        elapsed = clock() - origin
        expect = packet.get("expected_profile") if isinstance(packet.get("expected_profile"), Mapping) else {}
        expected_show = {name: expect[name] for name in SHOW_FIELDS if name in expect}
        health = health_fault or evaluate_run_health(rows, expected_show=expected_show or None)
        deltas = health.get("deltas") if isinstance(health.get("deltas"), Mapping) else {}
        reason = health.get("reason")
        verdict = "PASS" if health.get("verdict") == "PASS" else "FAIL"
        if verdict == "PASS" and elapsed + 1e-6 < STABILITY_S:
            verdict, reason = "FAIL", f"elapsed acquisition is {elapsed:.3f}s, not 600s"
        payload = {
            "verdict": verdict,
            "reason": reason,
            "scope": "stability_run",
            "completed": verdict == "PASS",
            "preflight": False,
            "device_opened": True,
            "emit_enabled_by_this_run": False,
            "looped": False,
            "physical_observation": False,
            "tracks_started": started,
            "samples": len(rows),
            "duration_s": elapsed,
            "source_duration_s": source_duration,
            "playlist_trimmed": source_duration > STABILITY_S + 1e-9,
            "health": health,
            "emitted_delta": deltas.get("emitted"),
            "emitted_delta_unit": "cumulative emitted frames, not a frame gap",
            "stream_epoch_delta": deltas.get("stream_epoch"),
            "pair_epoch_drops_delta": deltas.get("pair_epoch_drops"),
            "ap_hops_delta": deltas.get("ap_hops"),
            "asrc_consumed_delta": deltas.get("asrc_consumed"),
            "stream_epoch_start": counters.get("stream_epoch", UNKNOWN),
        }
        payload = _write_result(writer, payload)
        return payload
    except ExperimentError as exc:
        player.stop()
        payload = {
            "verdict": "DENIED" if exc.before_write else "FAULT",
            "reason": exc.reason,
            "scope": "stability_run",
            "completed": False,
            "device_opened": session is not None,
            "emit_enabled_by_this_run": False,
            "looped": False,
            "physical_observation": False,
        }
        payload = _write_result(writer, payload)
        return payload
    except (KeyboardInterrupt, RecordError, OSError, *_transport_faults()) as exc:
        player.stop()
        payload = {
            "verdict": "FAULT",
            "reason": type(exc).__name__,
            "detail": str(exc),
            "scope": "stability_run",
            "completed": False,
            "device_opened": session is not None,
            "emit_enabled_by_this_run": False,
            "looped": False,
            "physical_observation": False,
        }
        payload = _write_result(writer, payload)
        return payload
    finally:
        _close_session(session, evidence, token, payload)


def execute_dry_run(packet: Mapping[str, Any], evidence: Path) -> dict:
    writer = EvidenceWriter(evidence, int(packet.get("max_record_bytes") or 2_000_000))
    payload = {
        "verdict": "DRY_RUN",
        "device_opened": False,
        "emit_off_blockers": emit_off_blockers(packet),
        "physical_blockers": physical_blockers(packet),
        "emit_enabled_by_this_run": False,
        "forbidden_command": "scripts/run_mode32_real_audio.py",
    }
    writer.write_json("dry-run.json", payload)
    return payload


def dispatch(
    command: str,
    packet_path: Path,
    evidence: Path,
    open_session: Callable[[], Any] | None = None,
    *,
    sleeper=time.sleep,
    clock: Callable[[], float] | None = None,
    player=None,
) -> dict:
    packet = load_packet(packet_path)
    clock = clock or time.monotonic
    if command == "dry-run":
        return execute_dry_run(packet, evidence)
    if command == "emit-off":
        if open_session is None:
            open_session = open_live_session_factory(evidence, pair_fixture=False)
        return execute_emit_off(packet, evidence, open_session, sleeper=sleeper, clock=clock)
    if command == "physical":
        if open_session is None:
            lifecycle = packet.get("mapping_lifecycle") if isinstance(packet.get("mapping_lifecycle"), Mapping) else {}
            open_session = open_live_session_factory(
                evidence,
                pair_fixture=True,
                allow_controls=lifecycle.get("restore") is True or mapping_hold_authorised(packet),
            )
        return execute_physical(packet, evidence, open_session, sleeper=sleeper, clock=clock)
    if command == "sequence":
        if open_session is None:
            open_session = open_live_session_factory(evidence, pair_fixture=False)
        if player is None:
            player = PcmPlayer(evidence)
        return execute_sequence(packet, evidence, open_session, player, sleeper=sleeper, clock=clock)
    if command == "functional":
        if open_session is None:
            open_session = open_live_session_factory(evidence, pair_fixture=False, allow_controls=True)
        if player is None:
            player = PcmPlayer(evidence)
        return execute_sequence(
            packet, evidence, open_session, player,
            sleeper=sleeper, clock=clock, apply_controls=True,
        )
    if command == "stability":
        return execute_stability(
            packet, evidence, open_session, player, sleeper=sleeper, clock=clock,
        )
    raise ExperimentError(f"unknown command {command}")


def session_may_send(
    op: int,
    payload: bytes,
    *,
    pair_fixture: bool,
    allow_controls: bool = False,
) -> str | None:
    """Read-only allowlist for this entry. Opcode 27 is not on the emit-off list."""
    if is_control_write(op, payload) and not allow_controls:
        return "control write refused by the live session"
    if op == OP_PAIR_FIXTURE:
        if not pair_fixture:
            return "pair fixture opcode refused on the read-only session"
        return None
    if op not in EMIT_OFF_READ_OPS:
        return f"opcode {op} is outside the read-only allowlist"
    if op == OP_CONFIG and config_sub(payload) not in CONFIG_READ_SUBS:
        if not (allow_controls and config_sub(payload) in CONFIG_WRITE_SUBS):
            return "config subcommand is not on the read-only allowlist"
    return None


class LiveSession:
    def __init__(
        self,
        usb,
        handle,
        port,
        transport,
        info,
        lease,
        run_dir: Path,
        *,
        pair_fixture: bool = False,
        allow_controls: bool = False,
    ) -> None:
        self.usb = usb
        self.handle = handle
        self.port = port
        self.transport = transport
        self._info = info
        self.lease_token = lease["token"]
        self.run_dir = run_dir
        self.closed = False
        self.pair_fixture = pair_fixture
        self.allow_controls = allow_controls

    def info(self) -> dict:
        return self._info

    def transact(self, op: int, payload: bytes = b"", timeout: float = 2.0) -> dict:
        why = session_may_send(
            op, payload, pair_fixture=self.pair_fixture, allow_controls=self.allow_controls,
        )
        if why:
            raise ExperimentError(why, before_write=True)
        return self.transport.transact(op, payload, timeout=timeout)

    def close(self) -> None:
        if self.closed:
            return
        self.closed = True
        from titan_transport import close_serial  # noqa: WPS433
        import cdc_lock  # noqa: WPS433

        close_serial(self.port, self.handle)
        cdc_lock.release(self.handle)
        finish_lease(self.run_dir, self.lease_token)
        self.lease_token = None


def acquire_cdc_without_lsof(device: str, owner: str) -> dict:
    """Same flock file as cdc_lock.acquire, without lsof on the busy path."""
    import cdc_lock  # noqa: WPS433
    import fcntl

    path = cdc_lock.lock_path(device)
    fd = os.open(str(path), os.O_RDWR | os.O_CREAT, 0o644)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError as exc:
        os.close(fd)
        raise ExperimentError(f"CDC lock busy for {device}; not stolen ({exc})") from exc
    payload = f"owner={owner}\npid={os.getpid()}\ndevice={device}\n"
    os.lseek(fd, 0, os.SEEK_SET)
    os.ftruncate(fd, 0)
    os.write(fd, payload.encode())
    os.fsync(fd)
    return {"fd": fd, "path": str(path), "device": device, "owner": owner, "pid": os.getpid()}


def open_locked_port(device: str, owner: str, *, acquire, open_port, release):
    """Release a flock when the serial open fails. The original error is kept."""
    handle = acquire(device, owner)
    try:
        port = open_port(device, handle)
    except BaseException:
        release(handle)
        raise
    return handle, port


def open_live_session_factory(
    run_dir: Path,
    *,
    pair_fixture: bool = False,
    allow_controls: bool = False,
) -> Callable[[], LiveSession]:
    def open_session() -> LiveSession:
        from titan_broker import find_titan  # noqa: WPS433
        from titan_transport import Transport, close_serial, open_serial  # noqa: WPS433
        import cdc_lock  # noqa: WPS433

        usb = find_titan(inventory_lsof=False)
        handle, port = open_locked_port(
            usb["device"],
            "a8-bench-experiment",
            acquire=acquire_cdc_without_lsof,
            open_port=open_serial,
            release=cdc_lock.release,
        )
        transport = Transport(port, allowed={1}, campaign=True)
        try:
            info = transport.info(timeout=2.0)
            transport.allow(set(EMIT_OFF_READ_OPS))
            if pair_fixture:
                transport.admit_pair_fixture()
            lease = live_lease.acquire(run_dir, "a8-bench-experiment")
        except Exception:
            close_serial(port, handle)
            cdc_lock.release(handle)
            raise
        return LiveSession(
            usb, handle, port, transport, info, lease, run_dir,
            pair_fixture=pair_fixture, allow_controls=allow_controls,
        )

    return open_session


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        nargs="?",
        default="dry-run",
        choices=("dry-run", "emit-off", "physical", "sequence", "functional", "stability"),
    )
    parser.add_argument("--packet", required=True, type=Path)
    parser.add_argument("--evidence", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        result = dispatch(args.command, args.packet, args.evidence)
    except ExperimentError as exc:
        print(f"EXPERIMENT_DENIED {exc.reason}")
        return 2
    print(json.dumps({
        "verdict": result.get("verdict"),
        "device_opened": result.get("device_opened"),
        "emit_enabled_by_this_run": result.get("emit_enabled_by_this_run", False),
        "physical_observation": result.get("physical_observation", False),
        "runtime_acceptance": result.get("runtime_acceptance"),
        "counter_progression": result.get("counter_progression"),
        "darkness_verified": result.get("darkness_verified", False),
        "reason": result.get("reason"),
        "functional_verdict": result.get("functional_verdict"),
        "functional_reason": result.get("functional_reason"),
        "scope": result.get("scope"),
        "completed": result.get("completed"),
        "evidence": str(args.evidence),
    }))
    if result.get("verdict") in {"PASS", "DRY_RUN", "SEQUENCED"}:
        return 0
    if result.get("verdict") == "DENIED":
        return 4
    return 3


if __name__ == "__main__":
    raise SystemExit(main())
