#!/usr/bin/env python3
"""Image-bound C4 scorer: apply vs consume vs emit. No CDC."""
from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))

from score_smc_live_control import load_identity, score_record  # noqa: E402

IDENTITY = (
    HERE.parents[1]
    / "docs/evidence/SMC-TITAN-CONTROL-001/c4-readiness-20260926/CANDIDATE-IDENTITY.json"
)


def _snap(**kwargs):
    base = {
        "uid": "545433931bd25436593630352d068363",
        "build_sha256": "addc00a22b67d3db2e010c96e66db81cdb8a209c9c9d193dcdaadb98d5d5e3a3",
        "schema_sha256": "e3dca88cecbba56e562a01c18d8c02c0240f813dfbd438c7d185a739515c0b57",
        "emit_on": 0,
        "mode_id": 8,
        "accepted_request_id": 7,
        "accepted_revision": 3,
        "last_rendered_request_id": 7,
        "last_rendered_hop": 9,
        "last_rendered_frame": 3,
        "last_emitted_request_id": 0,
        "hop_sequence": 9,
        "hops_consumed": 9,
    }
    base.update(kwargs)
    return base


def test_identity_loads_addc00a2():
    ident = load_identity(IDENTITY)
    assert ident["build_id"].startswith("addc00a2")
    assert ident["admitted"] is False
    assert ident["emit_on"] == 0


def test_host_fake_apply_and_model_consume():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(),
        origin="host_fake",
        expect_request_id=7,
        expect_value=40.0,
        applied_value=40.0,
    )
    assert got["applied"] == "applied"
    assert got["consumed"] == "host_model"
    assert got["emit_held_off"] is True
    assert got["optical"] == "not_in_scope"
    assert got["ok"] is True
    assert got["evidence_class"] == "HOST_TEST"


def test_target_consume_requires_bound_hop_not_plus_one_model():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(last_rendered_hop=11, hop_sequence=10, hops_consumed=4),
        origin="target",
        expect_request_id=7,
    )
    assert got["ok"] is False
    assert "consume_hop_is_fake_model" in got["reasons"]


def test_target_matching_consume_passes_without_optical():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(hops_consumed=9, hop_sequence=40, last_rendered_hop=9),
        origin="target",
        expect_request_id=7,
        expect_value=200.0,
        applied_value=200.0,
    )
    assert got["consumed"] == "consumed"
    assert got["ok"] is True
    assert got["optical"] == "not_in_scope"
    assert got["evidence_class"] == "TARGET_RUNTIME"


def test_emit_on_fails_and_recovery_emit_cannot_pass():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(emit_on=1, last_emitted_request_id=7),
        origin="target",
        expect_request_id=7,
    )
    assert got["ok"] is False
    assert got["emit_held_off"] is False
    assert "emit_armed" in got["reasons"]


def test_wrong_build_fails_closed():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(build_sha256="1afde371457324f6349226b9b0d71082a10da340e18e7a6817a1fbd1b115f421"),
        origin="target",
        expect_request_id=7,
    )
    assert got["ok"] is False
    assert "build_mismatch" in got["reasons"]


def test_unlatched_consume_is_unproven():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(last_rendered_request_id=0, last_rendered_hop=0),
        origin="target",
        expect_request_id=7,
    )
    assert got["consumed"] == "unproven"
    assert got["ok"] is False


def test_value_mismatch_fails_apply():
    ident = load_identity(IDENTITY)
    got = score_record(
        identity=ident,
        snapshot=_snap(),
        origin="host_fake",
        expect_request_id=7,
        expect_value=40.0,
        applied_value=240.0,
    )
    assert got["applied"] == "fail"
    assert "value_mismatch" in got["reasons"]


if __name__ == "__main__":
    test_identity_loads_addc00a2()
    test_host_fake_apply_and_model_consume()
    test_target_consume_requires_bound_hop_not_plus_one_model()
    test_target_matching_consume_passes_without_optical()
    test_emit_on_fails_and_recovery_emit_cannot_pass()
    test_wrong_build_fails_closed()
    test_unlatched_consume_is_unproven()
    test_value_mismatch_fails_apply()
    print("C4_SCORER_PASS")
