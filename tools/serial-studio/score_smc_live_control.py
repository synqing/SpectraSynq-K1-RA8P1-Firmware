#!/usr/bin/env python3
"""Image-bound SMC live-control scorer. HOST_TEST or TARGET_RUNTIME records only.

Does not open CDC. Does not programme. Optical PASS is unreachable here.
A FakeSerial hop that equals audio_hop+1 is a host model, not target consumption.
"""
from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve().parent
DEFAULT_IDENTITY = (
    HERE.parents[2]
    / "docs/evidence/SMC-TITAN-CONTROL-001/c4-readiness-20260926/CANDIDATE-IDENTITY.json"
)

APPLY = "applied"
CONSUME = "consumed"
EMIT = "emitted"
UNPROVEN = "unproven"
FAIL = "fail"


def load_identity(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text())
    if not isinstance(data, dict) or not data.get("build_id"):
        raise ValueError("identity missing build_id")
    return data


def _hex(value: Any) -> str:
    return str(value or "").strip().lower()


def _int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def score_record(
    *,
    identity: dict[str, Any],
    snapshot: dict[str, Any],
    config: dict[str, Any] | None = None,
    origin: str = "host_fake",
    expect_request_id: int | None = None,
    expect_value: float | None = None,
    applied_value: float | None = None,
) -> dict[str, Any]:
    reasons: list[str] = []
    evidence_class = "TARGET_RUNTIME" if origin == "target" else "HOST_TEST"
    want_uid = _hex(identity.get("uid"))
    want_build = _hex(identity.get("build_id"))
    want_schema = _hex(identity.get("schema_sha256"))
    got_uid = _hex(snapshot.get("uid"))
    got_build = _hex(snapshot.get("build_sha256") or snapshot.get("build"))
    got_schema = _hex(snapshot.get("schema_sha256") or snapshot.get("schema"))
    identity_ok = True
    if origin == "target":
        if got_uid and got_uid != want_uid:
            identity_ok = False
            reasons.append("uid_mismatch")
        if got_build and got_build != want_build:
            identity_ok = False
            reasons.append("build_mismatch")
        if got_schema and got_schema != want_schema:
            identity_ok = False
            reasons.append("schema_mismatch")
        if not got_uid or not got_build or not got_schema:
            identity_ok = False
            reasons.append("identity_incomplete")

    emit_on = _int(snapshot.get("emit_on"))
    if config is not None and "emit_on" in config:
        emit_on = _int(config.get("emit_on"), emit_on)
    emit_held = emit_on == 0
    if not emit_held:
        reasons.append("emit_armed")

    mode = _int(snapshot.get("mode_id") or snapshot.get("mode_a") or 0)
    if config is not None and config.get("mode_a") is not None:
        mode = _int(config.get("mode_a"), mode)
    mode_ok = mode == int(identity.get("mode_a") or 8)
    if not mode_ok:
        reasons.append("mode_mismatch")

    accepted_id = _int(snapshot.get("accepted_request_id"))
    accepted_rev = _int(snapshot.get("accepted_revision"))
    rendered_id = _int(snapshot.get("last_rendered_request_id"))
    rendered_hop = _int(snapshot.get("last_rendered_hop"))
    emitted_id = _int(snapshot.get("last_emitted_request_id"))
    hop_sequence = _int(snapshot.get("hop_sequence"))
    hops_consumed = _int(snapshot.get("hops_consumed"))

    applied = FAIL
    if expect_request_id is not None:
        if accepted_id == int(expect_request_id) and accepted_rev > 0:
            applied = APPLY
        else:
            reasons.append("apply_unconfirmed")
    elif accepted_id > 0 and accepted_rev > 0:
        applied = APPLY

    if expect_value is not None and applied_value is not None:
        if not math.isfinite(float(applied_value)) or abs(float(applied_value) - float(expect_value)) > 1e-4:
            applied = FAIL
            reasons.append("value_mismatch")

    consumed = UNPROVEN
    hop_is_model = origin != "target" and rendered_hop != 0 and (
        rendered_hop == hop_sequence or rendered_hop == hops_consumed
    )
    if rendered_id == 0:
        consumed = UNPROVEN
        reasons.append("consume_not_latched")
    elif expect_request_id is not None and rendered_id != int(expect_request_id):
        consumed = FAIL
        reasons.append("consume_wrong_request")
    elif rendered_id != accepted_id:
        consumed = FAIL
        reasons.append("consume_not_bound_to_accepted")
    elif origin == "target" and rendered_hop == hop_sequence + 1 and hops_consumed != rendered_hop:
        consumed = UNPROVEN
        reasons.append("consume_hop_is_fake_model")
    elif hop_is_model:
        consumed = "host_model"
        reasons.append("consume_host_model_not_target")
    else:
        consumed = CONSUME

    emitted = EMIT if emitted_id != 0 else "idle"
    if emitted_id != 0 and emit_held:
        reasons.append("emit_receipt_while_held_off")
        emitted = FAIL

    ok = identity_ok and emit_held and mode_ok and applied == APPLY and consumed in {CONSUME, "host_model"} and emitted != FAIL
    if origin == "target" and consumed != CONSUME:
        ok = False
    optical = "not_in_scope"
    return {
        "ok": ok,
        "evidence_class": evidence_class,
        "optical": optical,
        "identity_ok": identity_ok,
        "emit_held_off": emit_held,
        "mode_ok": mode_ok,
        "applied": applied,
        "consumed": consumed,
        "emitted": emitted,
        "reasons": reasons,
        "bound_build": want_build,
        "bound_schema": want_schema,
        "accepted_request_id": accepted_id,
        "rendered_request_id": rendered_id,
        "rendered_hop": rendered_hop,
        "hop_sequence": hop_sequence,
        "hops_consumed": hops_consumed,
        "emit_on": emit_on,
        "mode": mode,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--identity", default=str(DEFAULT_IDENTITY))
    parser.add_argument("--snapshot", required=True)
    parser.add_argument("--config")
    parser.add_argument("--origin", choices=("host_fake", "target"), default="host_fake")
    parser.add_argument("--expect-request-id", type=int)
    parser.add_argument("--expect-value", type=float)
    parser.add_argument("--applied-value", type=float)
    args = parser.parse_args(argv)
    identity = load_identity(Path(args.identity))
    snapshot = json.loads(Path(args.snapshot).read_text())
    config = json.loads(Path(args.config).read_text()) if args.config else None
    result = score_record(
        identity=identity,
        snapshot=snapshot,
        config=config,
        origin=args.origin,
        expect_request_id=args.expect_request_id,
        expect_value=args.expect_value,
        applied_value=args.applied_value,
    )
    print(json.dumps(result, indent=2))
    return 0 if result["ok"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
