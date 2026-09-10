"""WP15 HOST audio/NPU pair: mismatch reject, load-only refused, physical NOT_RUN."""
from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from ctypes import c_uint8

from run_audio_npu_pair import (  # noqa: E402
    ADAPTER_ID,
    ARM_GCC,
    DEADLINE_US,
    G4,
    G4_O2_MISSES,
    G4_O3_MISSES,
    G6,
    LOAD_ONLY_OUTPUT,
    NAMED_PHYSICAL_FACTS,
    P99_US,
    PDM_NOT_RUN,
    SAMPLE_RATE_HZ,
    SOURCE_C,
    TREATMENT_AUDIO_ONLY,
    TREATMENT_USEFUL_NPU,
    U55_NOT_RUN,
    bind_adapter,
    build_adapter_document,
    compile_host_library,
    cross_compile,
    current_target_not_run,
    frozen_pair_identity,
    host_fixture_events,
    independent_max,
    independent_misses,
    independent_percentile,
    load_host_pair,
    load_predecessor_not_run,
    make_receipt,
    matching_pair_sides,
    missing_current_target_fields,
    missing_named_physical_facts,
    observed_source_hashes,
    refuse_widened_deadline,
    run_host_treatment,
)


@pytest.fixture(scope="module")
def host_pair():
    probe, receipt = load_host_pair()
    assert receipt["qualification_level"] == "HOST"
    assert receipt["exit_code"] == 0
    assert receipt["arm_binary_executed"] is False
    return probe


@pytest.fixture(scope="module")
def pair_identity():
    identity = frozen_pair_identity()
    assert len(identity["input"]) == 64
    assert len(identity["profile"]) == 64
    assert len(identity["build"]) == 64
    return identity


def test_ac1_matching_pair_except_treatment_is_comparable(host_pair):
    audio, npu = matching_pair_sides()
    rc = host_pair.pair_comparable(audio, npu)
    reverse = host_pair.pair_comparable(npu, audio)
    assert rc == 0
    assert reverse == 0
    assert audio["input"] == npu["input"]
    assert audio["profile"] == npu["profile"]
    assert audio["build"] == npu["build"]
    assert audio["clock"] == npu["clock"]
    assert audio["treatment"] != npu["treatment"]
    honest = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": npu, "comparable": "OK", "expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True, "npu_role": "identified-platform-load-only"},
    )
    assert honest["ok"] is True
    assert honest["physical_pair"] == "NOT_RUN"


def test_ac1_mismatched_input_rejected(host_pair, pair_identity):
    audio, npu = matching_pair_sides()
    npu = dict(npu, input="aa" * 32)
    rc = host_pair.pair_comparable(audio, npu)
    assert rc == 1
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": npu},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "pair-mismatch-input" for item in receipt["errors"])
    assert pair_identity["input"] != npu["input"]


def test_ac1_mismatched_profile_rejected(host_pair):
    audio, npu = matching_pair_sides()
    npu = dict(npu, profile="bb" * 32)
    rc = host_pair.pair_comparable(audio, npu)
    assert rc == 2
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": npu},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "pair-mismatch-profile" for item in receipt["errors"])


def test_ac1_mismatched_build_rejected(host_pair):
    audio, npu = matching_pair_sides()
    npu = dict(npu, build="cc" * 32)
    rc = host_pair.pair_comparable(audio, npu)
    assert rc == 3
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": npu},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "pair-mismatch-build" for item in receipt["errors"])


def test_ac1_mismatched_clock_rejected(host_pair):
    audio, npu = matching_pair_sides()
    npu = dict(npu, clock=16000)
    rc = host_pair.pair_comparable(audio, npu)
    assert rc == 4
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": npu},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "pair-mismatch-clock" for item in receipt["errors"])
    assert audio["clock"] == SAMPLE_RATE_HZ


def test_ac1_same_treatment_is_not_declared_pair(host_pair):
    audio, npu = matching_pair_sides()
    same = dict(npu, treatment=TREATMENT_AUDIO_ONLY)
    rc = host_pair.pair_comparable(audio, same)
    assert rc == 5
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"audio": audio, "npu": same},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "pair-treatment-not-declared" for item in receipt["errors"])


def test_ac2_distributions_from_raw_events_match_independent(host_pair):
    audio = run_host_treatment(host_pair, TREATMENT_AUDIO_ONLY)
    npu = run_host_treatment(host_pair, TREATMENT_USEFUL_NPU)
    for fixture in (audio, npu):
        snap = fixture["snapshot"]
        durations = snap["durations_us"]
        assert snap["event_count"] == 8
        assert snap["window_end_us"] > snap["window_start_us"]
        assert snap["overhead_sum_us"] > 0
        assert fixture["independent"]["p50_us"] == independent_percentile(durations, 50) == snap["p50_us"]
        assert fixture["independent"]["p95_us"] == independent_percentile(durations, 95) == snap["p95_us"]
        assert fixture["independent"]["p99_us"] == independent_percentile(durations, 99) == snap["p99_us"]
        assert fixture["independent"]["max_us"] == independent_max(durations) == snap["max_us"]
        assert snap["has_distribution"] is True
        for event in snap["events"]:
            assert event["complete_us"] >= event["release_us"]
            assert event["duration_us"] == event["complete_us"] - event["release_us"]
            assert event["overhead_us"] > 0
    assert audio["snapshot"]["durations_us"] != npu["snapshot"]["durations_us"]


def test_ac2_deadline_misses_from_raw_and_7500(host_pair):
    audio = run_host_treatment(host_pair, TREATMENT_AUDIO_ONLY)
    npu = run_host_treatment(host_pair, TREATMENT_USEFUL_NPU)
    assert audio["snapshot"]["deadline_us"] == DEADLINE_US
    assert npu["snapshot"]["deadline_us"] == DEADLINE_US
    assert audio["snapshot"]["deadline_misses"] == 0
    assert independent_misses(audio["snapshot"]["durations_us"]) == 0
    assert npu["snapshot"]["deadline_misses"] == independent_misses(npu["snapshot"]["durations_us"])
    assert npu["snapshot"]["deadline_misses"] > 0
    for event in npu["snapshot"]["events"]:
        expected = 1 if event["duration_us"] > DEADLINE_US else 0
        assert event["missed"] == expected
    events = host_fixture_events(TREATMENT_USEFUL_NPU)
    assert any(complete - release > DEADLINE_US for release, complete, _overhead in events)


def test_ac2_p99_unscored_even_when_computed(host_pair):
    npu = run_host_treatment(host_pair, TREATMENT_USEFUL_NPU)
    assert npu["snapshot"]["p99_us"] is not None
    assert npu["snapshot"]["p99_scored"] is False
    assert npu["snapshot"]["p99_allocation_us"] == P99_US
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"raw_events": npu["snapshot"]["events"], "claims_distribution": True, "expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        p99_pass_fail="unscored",
    )
    assert receipt["ok"] is True
    assert receipt["p99_pass_fail"] == "unscored"
    scored = make_receipt(
        qualification_level="HOST",
        pair={"raw_events": npu["snapshot"]["events"], "claims_distribution": True},
        execution={"load_only": True},
        p99_pass_fail="PASS",
    )
    assert scored["ok"] is False
    assert any(item["code"] == "p99-invented-pass" for item in scored["errors"])
    assert "p99-pass-fail" in scored["blocked_cells"]


def test_ac2_missing_raw_events_cannot_claim_distribution():
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"claims_distribution": True, "raw_events": []},
        execution={"load_only": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "missing-raw-events" for item in receipt["errors"])


def test_ac3_load_only_cannot_satisfy_useful_npu_treatment(host_pair):
    rc = host_pair.admit(load_only=1, useful_u55_admitted=0, physical_pdm_complete=0)
    snap = host_pair.snapshot()
    assert rc == 6
    assert snap["last_decline_name"] == "LOAD_ONLY"
    assert snap["admitted"] is False
    expected = (c_uint8 * 3)(*LOAD_ONLY_OUTPUT)
    assert int(host_pair.lib.k1_coexist_is_load_only_output(expected, 3)) == 1
    claimed = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT, "claims_useful_inference": True},
        execution={"load_only": True, "npu_role": "identified-platform-load-only", "useful_npu_treatment": True},
    )
    assert claimed["ok"] is False
    assert any(item["code"] == "load-only-as-useful-npu-treatment" for item in claimed["errors"])
    honest = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT, "claims_useful_inference": False},
        execution={"load_only": True, "npu_role": "identified-platform-load-only", "useful_npu_treatment": False},
    )
    assert honest["ok"] is True
    assert "audio-npu-physical-pair" in honest["blocked_cells"]
    probe_rc = int(host_pair.lib.k1_coexist_probe())
    assert probe_rc == 6
    assert int(host_pair.lib.k1_coexist_acquire()) != 0


def test_ac3_physical_pair_not_run_because_predecessors_not_run():
    predecessors = load_predecessor_not_run()
    pdm = json.loads(PDM_NOT_RUN.read_text(encoding="utf-8"))
    u55 = json.loads(U55_NOT_RUN.read_text(encoding="utf-8"))
    assert pdm["physical_capture"] == "NOT_RUN"
    assert u55["u55_current_target"] == "NOT_RUN"
    assert predecessors["both_not_run"] is True
    assert predecessors["physical_pdm_complete"] is False
    assert predecessors["useful_u55_admitted"] is False
    receipt = current_target_not_run()
    assert receipt["physical_pair"] == "NOT_RUN"
    assert receipt["qualification_level"] == "HOST"
    assert receipt["identities"]["live_target"] == "NOT_VERIFIED"
    assert receipt["predecessors"]["wp13_physical_pdm"] == "NOT_RUN"
    assert receipt["predecessors"]["wp14_useful_u55"] == "NOT_RUN"
    assert receipt["identities"]["named_missing_facts"] == list(NAMED_PHYSICAL_FACTS)
    for fact in NAMED_PHYSICAL_FACTS:
        assert fact in receipt["identities"]["named_missing_facts"]
    current = make_receipt(
        qualification_level="CURRENT_TARGET",
        pair={"predecessors": predecessors, "expected_raw_output": LOAD_ONLY_OUTPUT},
        identities={},
        execution={"arm_binary_executed": False, "load_only": True},
    )
    assert current["status"] == "FAIL_CLOSED"
    messages = {item["message"] for item in current["errors"]}
    codes = {item["code"] for item in current["errors"]}
    assert "physical-pdm-not-run" in codes
    assert "useful-u55-not-admitted" in codes
    for fact in NAMED_PHYSICAL_FACTS:
        assert fact in messages


def test_ac3_changed_load_only_output_still_refused_as_useful(host_pair):
    audio, npu = matching_pair_sides()
    receipt = make_receipt(
        qualification_level="HOST",
        pair={
            "audio": audio,
            "npu": npu,
            "expected_raw_output": [1, 2, 3],
            "claims_useful_inference": True,
        },
        execution={"load_only": True, "npu_role": "identified-platform-load-only", "useful_npu_treatment": True},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "load-only-as-useful-npu-treatment" for item in receipt["errors"])
    u55 = host_pair.admit(load_only=0, useful_u55_admitted=0, physical_pdm_complete=0)
    assert u55 == 7
    pdm = host_pair.admit(load_only=0, useful_u55_admitted=1, physical_pdm_complete=0)
    assert pdm == 8
    assert host_pair.snapshot()["physical_not_run"] is True


def test_ac4_deadline_not_widened(host_pair):
    refusal = refuse_widened_deadline(8000)
    assert refusal["ok"] is False
    assert refusal["code"] == "deadline-widened"
    assert int(host_pair.lib.k1_coexist_refuse_widen(8000)) == 1
    assert int(host_pair.lib.k1_coexist_refuse_widen(DEADLINE_US)) == 0
    rc = host_pair.admit(load_only=0, useful_u55_admitted=1, physical_pdm_complete=1, claimed_deadline_us=10000)
    assert rc == 9
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        claimed_deadline_us=10000,
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "deadline-widened" for item in receipt["errors"])
    assert receipt["authorised_deadline_us"] == DEADLINE_US
    bind_rc = host_pair.bind("11" * 32, "22" * 32, "33" * 32, deadline_us=10000)
    assert bind_rc == 9


def test_ac4_g4_o2_o3_already_failed_not_unrun(host_pair):
    snap = host_pair.snapshot()
    assert int(host_pair.lib.k1_coexist_g4_o2_misses()) == G4_O2_MISSES == 2005
    assert int(host_pair.lib.k1_coexist_g4_o3_misses()) == G4_O3_MISSES == 2006
    assert host_pair.lib.k1_coexist_g4_status_text() in (b"FAILED", "FAILED")
    assert int(host_pair.lib.k1_coexist_g4_already_failed()) == 1
    assert snap["g4_already_failed"] is True
    unrun = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        g4_status="UNRUN",
    )
    assert unrun["ok"] is False
    assert any(item["code"] == "g4-rerun-as-unrun" for item in unrun["errors"])
    relabel = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        g4_status="PASS",
    )
    assert relabel["ok"] is False
    assert any(item["code"] == "g4-already-failed-relabelled" for item in relabel["errors"])
    rc = host_pair.admit(load_only=0, useful_u55_admitted=1, physical_pdm_complete=1, g4_as_unrun=1)
    assert rc == 10
    honest = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        g4_status=G4,
    )
    assert honest["ok"] is True
    assert honest["g4_status"] == G4
    assert honest["g4_o2_deadline_misses"] == 2005
    assert honest["g4_o3_deadline_misses"] == 2006


def test_ac4_invented_p99_pass_rejected(host_pair):
    rc = host_pair.admit(load_only=0, useful_u55_admitted=1, physical_pdm_complete=1, claimed_p99_pass=1)
    assert rc == 11
    assert int(host_pair.lib.k1_coexist_p99_scored()) == 0
    receipt = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True},
        p99_pass_fail="FAIL",
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "p99-invented-pass" for item in receipt["errors"])
    assert receipt["p99_us"] == P99_US


def test_ac5_current_target_fails_closed_with_named_missing_facts():
    predecessors = load_predecessor_not_run()
    receipt = make_receipt(
        qualification_level="CURRENT_TARGET",
        pair={"predecessors": predecessors, "expected_raw_output": LOAD_ONLY_OUTPUT},
        identities={},
        execution={"arm_binary_executed": False},
    )
    codes = {item["code"] for item in receipt["errors"]}
    messages = {item["message"] for item in receipt["errors"]}
    assert receipt["ok"] is False
    assert receipt["status"] == "FAIL_CLOSED"
    assert "current-target-identity-missing" in codes
    for field in (
        "target_uid",
        "loaded_image_sha256",
        "build_id",
        "ownership_record.exclusive_owner",
        "measurement_method_id",
        "input_identities",
        "runtime_identity_method",
    ):
        assert field in messages
    for fact in NAMED_PHYSICAL_FACTS:
        assert fact in messages
    identities = {
        "target_uid": "545433931bd25436593630352d068363",
        "build_id": "c4ceebe7f4d899d39a917fb12c385c0f743278fd53aa2a82045230f3490e87bb",
        "ownership_record": {"exclusive_owner": "nobody", "method": "re-enumeration"},
    }
    missing = missing_current_target_fields(identities)
    assert "loaded_image_sha256" in missing
    named = missing_named_physical_facts(identities, predecessors)
    assert "live_uid" in named
    assert "useful_u55_admission" in named
    assert "physical_pdm" in named


def test_ac5_levels_remain_distinct(host_pair):
    host = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"arm_binary_executed": False, "load_only": True},
    )
    current = make_receipt(
        qualification_level="CURRENT_TARGET",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        identities={},
        execution={"arm_binary_executed": False},
    )
    assert host["ok"] is True
    assert host["qualification_level"] == "HOST"
    assert current["ok"] is False
    assert current["status"] == "FAIL_CLOSED"
    assert current["qualification_level"] == "CURRENT_TARGET"
    assert current["qualification_level"] != host["qualification_level"]
    assert current["qualification_level"] != "CROSS_COMPILED"
    claimed_host = make_receipt(
        qualification_level="HOST",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        execution={"load_only": True, "arm_binary_executed": False},
        claims_physical_pair=True,
    )
    assert claimed_host["ok"] is False
    assert any(item["code"] == "host-events-as-physical-pair" for item in claimed_host["errors"])
    g6 = host_pair.lib.k1_coexist_g6_text()
    assert (g6.decode("ascii") if isinstance(g6, bytes) else str(g6)) == G6


def test_ac5_port_name_and_compiler_facts_fail_closed():
    identities = {
        "target_uid": "545433931bd25436593630352d068363",
        "loaded_image_sha256": "00" * 32,
        "build_id": "11" * 32,
        "measurement_method_id": "audio-npu-pair",
        "input_identities": {"fixture": "pair"},
        "runtime_identity_method": "uid-readback",
        "ownership_record": {"exclusive_owner": "agent", "method": "port-name"},
        "live_target": "VERIFIED",
        "port": "/dev/cu.usbmodem21401",
    }
    receipt = make_receipt(
        qualification_level="CURRENT_TARGET",
        pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
        identities=identities,
        execution={"arm_binary_executed": False, "compiler_facts_only": True},
    )
    codes = {item["code"] for item in receipt["errors"]}
    assert "port-name-is-not-identity" in codes
    assert "compiler-facts-as-current-target" in codes
    assert receipt["ok"] is False


def test_ac5_changed_source_hash_invalidates_old_adapter(tmp_path):
    adapter = build_adapter_document()
    assert adapter["id"] == ADAPTER_ID
    (tmp_path / "adapter.json").write_text(json.dumps(adapter), encoding="utf-8")
    observed = observed_source_hashes()
    assert bind_adapter(observed, adapter)["ok"] is True
    tampered = dict(observed)
    tampered["application"] = "ab" * 32
    declined = bind_adapter(tampered, adapter)
    assert declined["ok"] is False
    assert declined["declined"] == "unrecognised_source_hash"
    assert "application" in declined["mismatched"]


def test_ac5_probes_default_off_and_cdc_path_remains():
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
    scon = (ROOT / "platform/ra8p1/SConscript").read_text(encoding="utf-8")
    assert "#ifdef K1_COEXIST" in entry
    assert "#ifdef K1_ARM_NUMERIC" in entry
    assert "#ifdef K1_USEFUL_NPU" in entry
    assert "#ifdef K1_PDM_CAPTURE" in entry
    assert "R_USB_Open" in entry
    assert "R_USB_Read" in entry
    assert "R_USB_Write" in entry
    assert "k1_fixture_consume" in entry
    assert "K1_COEXIST=1" not in scon
    assert "-DK1_COEXIST" not in scon
    assert "K1_ARM_NUMERIC=1" not in scon
    assert "-DK1_ARM_NUMERIC" not in scon
    assert "K1_USEFUL_NPU=1" not in scon
    assert "-DK1_USEFUL_NPU" not in scon
    assert "K1_PDM_CAPTURE=1" not in scon
    assert "-DK1_PDM_CAPTURE" not in scon
    assert "Glob('*.c')" not in scon
    assert 'Glob("*.c")' not in scon
    assert "Glob('coexist_probe.c')" in scon
    assert "Glob('arm_numeric_probe.c')" in scon
    assert "Glob('useful_npu_probe.c')" in scon
    assert "Glob('pdm_capture.c')" in scon
    assert "Glob('npu_load.c')" in scon
    source = SOURCE_C.read_text(encoding="utf-8")
    assert "R_USB_" not in source
    assert "R_PDM_" not in source
    assert "RM_ETHOSU" not in source


def test_ac5_host_library_builds_with_werror(tmp_path):
    suffix = "dylib" if sys.platform == "darwin" else "so"
    receipt = compile_host_library(tmp_path / f"libcoexist.{suffix}")
    assert receipt["exit_code"] == 0
    assert Path(receipt["output"]).is_file()


def test_ac5_cross_compile_is_not_physical_pair(tmp_path):
    cross = cross_compile(tmp_path / "coexist_probe.o")
    if ARM_GCC.is_file():
        assert cross["qualification_level"] == "CROSS_COMPILED"
        assert cross["executed"] is False
        assert cross["linked"] is False
        assert cross["cannot_satisfy"] == "physical-audio-npu-pair"
        assert cross["physical_pair"] == "NOT_RUN"
        assert cross["qualification_level"] != "HOST"
        assert cross["qualification_level"] != "CURRENT_TARGET"
        claimed = make_receipt(
            qualification_level="CROSS_COMPILED",
            pair={"expected_raw_output": LOAD_ONLY_OUTPUT},
            execution={"compiler_facts_only": True, "arm_binary_executed": False, "load_only": True},
            claims_physical_pair=True,
        )
        assert claimed["ok"] is False
        assert any(item["code"] == "load-only-as-useful-npu-treatment" for item in claimed["errors"])
    else:
        assert cross["status"] == "NOT_RUN"
        assert cross["missing_tool"] == str(ARM_GCC)
