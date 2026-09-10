"""WP14 HOST Arm fixtures and useful-U55 admission. CURRENT_TARGET cells NOT_RUN."""
from __future__ import annotations

import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from run_arm_numeric_probe import (  # noqa: E402
    ADAPTER_ID as ARM_ADAPTER,
    ARM_GCC,
    PREDECLARED_ABS_TOL,
    SOURCE_C as ARM_SOURCE,
    bind_disassembly,
    campaign_vectors,
    compile_host_library as compile_arm_host,
    cross_compile as cross_compile_arm,
    current_target_not_run as arm_current_target_not_run,
    independent_dot_f64,
    load_host_arm,
    make_receipt as make_arm_receipt,
    python_f32_accum,
    refuse_widened_tolerance,
    run_host_fixtures,
    score_finite,
)
from run_useful_npu_probe import (  # noqa: E402
    G6,
    LOAD_ONLY_OUTPUT,
    LOAD_ONLY_TFLITE_SHA256,
    SOURCE_C as NPU_SOURCE,
    acquisition_request,
    compile_host_library as compile_npu_host,
    cross_compile as cross_compile_npu,
    current_target_not_run as npu_current_target_not_run,
    load_host_npu,
    locate_original_artefacts,
    make_receipt as make_npu_receipt,
)


@pytest.fixture(scope="module")
def host_arm():
    probe, receipt = load_host_arm()
    assert receipt["qualification_level"] == "HOST"
    assert receipt["exit_code"] == 0
    assert receipt["arm_binary_executed"] is False
    return probe


@pytest.fixture(scope="module")
def host_npu():
    probe, receipt = load_host_npu()
    assert receipt["qualification_level"] == "HOST"
    assert receipt["exit_code"] == 0
    return probe


@pytest.fixture(scope="module")
def host_fixtures(host_arm):
    return run_host_fixtures(host_arm)


def test_ac1_campaign_fixture_uses_independent_reference(host_arm, host_fixtures):
    left, right = campaign_vectors()
    reference = independent_dot_f64(left, right)
    campaign = host_fixtures["campaign"]
    assert campaign["count"] == 256
    assert campaign["independent_f64"] == reference
    assert campaign["qualification_level"] == "HOST"
    assert campaign["arm_binary_executed"] is False
    assert campaign["independent_ref_recorded"] is True
    assert campaign["independent_ref_not_executed_arm"] is True
    assert campaign["score"]["within_predeclared"] is True
    assert campaign["score"]["scored_as_executed_arm"] is False
    assert campaign["score"]["predeclared_abs_tol"] == PREDECLARED_ABS_TOL
    assert abs(campaign["host_scalar"] - campaign["host_chunked"]) <= PREDECLARED_ABS_TOL
    assert campaign["python_f32_accum"] == python_f32_accum(left, right)
    assert campaign["python_f32_accum"] != reference or campaign["python_vs_ref"] >= 0.0
    assert campaign["input_sha256"]


def test_ac1_cancellation_magnitude_tails_prepared(host_fixtures):
    for name in ("cancellation", "magnitude", "tail"):
        item = host_fixtures[name]
        assert item["count"] > 0
        assert item["arm_binary_executed"] is False
        assert item["score"]["scored"] is False
        assert item["score"]["not_widened"] is True
        assert item["independent_f64"] is not None
    assert host_fixtures["tail"]["count"] == 5
    assert host_fixtures["tail"]["count"] % 4 != 0
    assert host_fixtures["cancellation"]["host_scalar"] != host_fixtures["magnitude"]["host_scalar"]
    assert host_fixtures["cancellation"]["input_sha256"] != host_fixtures["campaign"]["input_sha256"]


def test_ac1_specials_classified_not_scored_with_abs_tol(host_fixtures):
    specials = {item["name"]: item for item in host_fixtures["specials"]}
    assert specials["nan_times_one"]["observed_class"] == 1
    assert specials["pos_inf_times_two"]["observed_class"] == 2
    assert specials["neg_inf_times_two"]["observed_class"] == 3
    assert specials["pos_inf_times_zero"]["observed_class"] == 1
    assert specials["pos_zero_times_one"]["class_match"] is True
    assert specials["neg_zero_times_one"]["class_match"] is True
    assert specials["denormal_times_two"]["class_match"] is True
    for item in host_fixtures["specials"]:
        assert item["score_with_abs_tol"] is False
        assert item["arm_binary_executed"] is False
        assert item["class_match"] is True


def test_ac1_changed_input_changes_host_output(host_arm, host_fixtures):
    left, right = campaign_vectors()
    left[0] = left[0] + 0.5
    changed = host_arm.dot_scalar(left, right)
    assert changed != host_fixtures["campaign"]["host_scalar"]


def test_ac2_host_accumulation_cannot_pass_as_arm(host_fixtures):
    receipt = make_arm_receipt(
        qualification_level="HOST",
        numerical=host_fixtures["campaign"],
        execution={"arm_binary_executed": False, "host_language": "c", "python_numerics": False},
        claims_executed_arm=True,
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "python-numerics-as-executed-arm" for item in receipt["errors"])
    assert "arm-executed-numerics" in receipt["blocked_cells"]
    assert receipt["arm_current_target"] == "NOT_RUN"


def test_ac2_python_numerics_cannot_pass_as_arm(host_fixtures):
    receipt = make_arm_receipt(
        qualification_level="HOST",
        numerical={"python_f32_accum": host_fixtures["campaign"]["python_f32_accum"]},
        execution={"arm_binary_executed": False, "python_numerics": True, "host_language": "python"},
        claims_executed_arm=True,
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "python-numerics-as-executed-arm" for item in receipt["errors"])


def test_ac2_cross_compiled_disassembly_cannot_pass_arm(tmp_path):
    scalar = cross_compile_arm("scalar", tmp_path / "scalar.o", tmp_path / "scalar.disasm")
    mve = cross_compile_arm("mve", tmp_path / "mve.o", tmp_path / "mve.disasm")
    if ARM_GCC.is_file():
        assert scalar["qualification_level"] == "CROSS_COMPILED"
        assert scalar["executed"] is False
        assert scalar["linked"] is False
        assert scalar["arm_binary_executed"] is False
        assert scalar["ok"] is True
        assert scalar["bind"]["required"] == "k1_arm_dot_scalar"
        assert scalar["bind"]["has_mve_vfma_q"] is False
        assert mve["ok"] is True
        assert mve["bind"]["required"] == "k1_arm_dot_mve"
        assert mve["bind"]["has_mve_vfma_q"] is True
        assert scalar["qualification_level"] != "HOST"
        assert scalar["qualification_level"] != "CURRENT_TARGET"
        claimed = make_arm_receipt(
            qualification_level="CROSS_COMPILED",
            numerical={"bind": mve["bind"]},
            execution={"arm_binary_executed": False, "compiler_facts_only": True, "cross_compiled": True},
            claims_executed_arm=True,
        )
        assert claimed["ok"] is False
        assert any(item["code"] == "disassembly-as-executed-arm" for item in claimed["errors"])
        wrong = bind_disassembly(Path(mve["disassembly"]).read_text(encoding="utf-8"), "not_the_benchmark_function", True)
        assert wrong["ok"] is False
        assert wrong["code"] == "wrong-function-disassembly"
    else:
        assert scalar["status"] == "NOT_RUN"


def test_ac2_levels_remain_distinct(host_fixtures):
    host = make_arm_receipt(
        qualification_level="HOST",
        numerical=host_fixtures["campaign"],
        execution={"arm_binary_executed": False},
    )
    current = make_arm_receipt(
        qualification_level="CURRENT_TARGET",
        numerical=host_fixtures["campaign"],
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


def test_ac3_load_only_refused_as_useful(host_npu):
    rc = host_npu.admit(load_only=1, constant_output=1, expected_raw=LOAD_ONLY_OUTPUT, model_sha256=LOAD_ONLY_TFLITE_SHA256)
    snap = host_npu.snapshot()
    assert rc == 1
    assert snap["last_decline_name"] == "LOAD_ONLY"
    assert snap["admitted"] is False
    assert snap["invoked"] is False
    assert snap["g6"] == G6
    claimed = make_npu_receipt(
        qualification_level="HOST",
        npu={"expected_raw_output": LOAD_ONLY_OUTPUT, "g6": G6, "licence": "UNKNOWN"},
        execution={"load_only": True, "npu_role": "identified-platform-load-only"},
        claims_useful_inference=True,
    )
    assert claimed["ok"] is False
    assert any(item["code"] == "load-only-as-useful-inference" for item in claimed["errors"])
    honest = make_npu_receipt(
        qualification_level="HOST",
        npu={"expected_raw_output": LOAD_ONLY_OUTPUT, "g6": G6, "licence": "UNKNOWN", "original_goldens_located": False},
        execution={"load_only": True, "npu_role": "identified-platform-load-only"},
        claims_useful_inference=False,
    )
    assert honest["ok"] is True
    assert "u55-useful-inference" in honest["blocked_cells"]


def test_ac3_constant_output_and_changed_identity(host_npu):
    constant = host_npu.admit(load_only=0, constant_output=0, expected_raw=LOAD_ONLY_OUTPUT)
    assert constant == 2
    model_a = "11" * 32
    model_b = "22" * 32
    weights = "33" * 32
    inp_a = "44" * 32
    inp_b = "55" * 32
    ident_a = host_npu.bind(model_a, weights, inp_a)
    ident_b = host_npu.bind(model_b, weights, inp_a)
    ident_c = host_npu.bind(model_a, weights, inp_b)
    assert ident_a != ident_b
    assert ident_a != ident_c
    assert ident_b != ident_c


def test_ac3_different_student_refused(host_npu):
    rc = host_npu.admit(different_student=1, has_same_compilation_model=1, has_golden_inputs=1, has_golden_outputs=1)
    assert rc == 10
    receipt = make_npu_receipt(
        qualification_level="HOST",
        npu={"g6": G6, "licence": "UNKNOWN", "different_student": True, "original_goldens_located": False},
        execution={"load_only": False},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "different-student-selected" for item in receipt["errors"])


def test_ac4_missing_goldens_block_only_u55(host_npu, host_fixtures):
    inspection = locate_original_artefacts()
    assert inspection["exists"] is True
    assert inspection["missing"]["same_compilation_tflite"] is True
    assert inspection["missing"]["golden_inputs"] is True
    assert inspection["missing"]["golden_outputs"] is True
    assert inspection["missing"]["allocation_report"] is True
    assert inspection["licence"] == "UNKNOWN"
    assert inspection["generated_c_extents_observation"]["is_original_allocation_report"] is False
    rc = host_npu.admit(
        has_same_compilation_model=0,
        has_allocation_report=0,
        has_operator_report=0,
        has_golden_inputs=0,
        has_golden_outputs=0,
        licence_cleared=0,
    )
    snap = host_npu.snapshot()
    assert rc == 3
    assert snap["g6"] == G6
    assert snap["licence_unknown"] is True
    assert snap["admitted"] is False
    assert int(host_npu.lib.k1_useful_npu_invoke()) != 0
    assert host_npu.snapshot()["invoked"] is False
    request = acquisition_request(inspection)
    assert request["g6"] == G6
    assert request["licence"] == "UNKNOWN"
    assert request["do_not_pick_a_different_student"] is True
    arm = make_arm_receipt(
        qualification_level="HOST",
        numerical=host_fixtures["campaign"],
        execution={"arm_binary_executed": False},
    )
    assert arm["ok"] is True
    assert host_fixtures["campaign"]["score"]["within_predeclared"] is True
    npu = make_npu_receipt(
        qualification_level="HOST",
        npu={"g6": G6, "licence": "UNKNOWN", "original_goldens_located": False},
        execution={"load_only": True},
    )
    assert "u55-useful-inference" in npu["blocked_cells"]
    assert npu["u55_current_target"] == "NOT_RUN"


def test_ac4_g6_and_licence_cannot_be_rewritten():
    rewritten = make_npu_receipt(
        qualification_level="HOST",
        npu={"g6": "PASS", "licence": "Apache-2.0", "original_goldens_located": False},
        execution={"load_only": False},
    )
    codes = {item["code"] for item in rewritten["errors"]}
    assert "g6-rewritten-without-goldens" in codes
    assert "licence-unknown-cleared" in codes


def test_ac4_current_target_arm_and_u55_not_run(host_fixtures):
    arm = arm_current_target_not_run(host_fixtures)
    npu = npu_current_target_not_run({"g6": G6, "licence": "UNKNOWN"})
    assert arm["arm_current_target"] == "NOT_RUN"
    assert arm["qualification_level"] == "HOST"
    assert arm["identities"]["live_target"] == "NOT_VERIFIED"
    assert npu["u55_current_target"] == "NOT_RUN"
    assert npu["g6"] == G6
    empty = make_arm_receipt(
        qualification_level="CURRENT_TARGET",
        numerical=host_fixtures["campaign"],
        identities={},
        execution={"arm_binary_executed": False},
    )
    assert empty["status"] == "FAIL_CLOSED"
    assert any(item["code"] == "current-target-identity-missing" for item in empty["errors"])
    npu_empty = make_npu_receipt(
        qualification_level="CURRENT_TARGET",
        npu={"g6": G6, "licence": "UNKNOWN", "original_goldens_located": False},
        identities={},
        execution={"arm_binary_executed": False},
    )
    assert npu_empty["status"] == "FAIL_CLOSED"


def test_ac4_port_name_and_compiler_facts_fail_closed():
    identities = {
        "target_uid": "545433931bd25436593630352d068363",
        "loaded_image_sha256": "00" * 32,
        "build_id": "11" * 32,
        "measurement_method_id": "arm-numeric",
        "input_identities": {"fixture": "campaign"},
        "runtime_identity_method": "uid-readback",
        "ownership_record": {"exclusive_owner": "agent", "method": "port-name"},
        "port": "/dev/cu.usbmodem21401",
    }
    receipt = make_arm_receipt(
        qualification_level="CURRENT_TARGET",
        numerical={"arm_binary_executed": False},
        identities=identities,
        execution={"arm_binary_executed": False, "compiler_facts_only": True},
    )
    codes = {item["code"] for item in receipt["errors"]}
    assert "port-name-is-not-identity" in codes
    assert "compiler-facts-as-current-target" in codes
    assert receipt["ok"] is False


def test_ac5_tolerance_not_widened():
    refusal = refuse_widened_tolerance(1e-04)
    assert refusal["ok"] is False
    assert refusal["code"] == "tolerance-widened-after-mismatch"
    scored = score_finite(2e-05, claimed_tol=1e-04)
    assert scored["widened"] is True
    assert scored["within_predeclared"] is False
    receipt = make_arm_receipt(
        qualification_level="HOST",
        numerical={"absolute_error": 2e-05},
        execution={"arm_binary_executed": False},
        claimed_tol=1e-04,
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "tolerance-widened-after-mismatch" for item in receipt["errors"])
    assert receipt["p99_pass_fail"] == "unscored"


def test_ac5_probes_default_off_and_cdc_path_remains():
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
    scon = (ROOT / "platform/ra8p1/SConscript").read_text(encoding="utf-8")
    assert "#ifdef K1_ARM_NUMERIC" in entry
    assert "#ifdef K1_USEFUL_NPU" in entry
    assert "#ifdef K1_PDM_CAPTURE" in entry
    assert "R_USB_Open" in entry
    assert "R_USB_Read" in entry
    assert "R_USB_Write" in entry
    assert "k1_fixture_consume" in entry
    assert "K1_ARM_NUMERIC=1" not in scon
    assert "-DK1_ARM_NUMERIC" not in scon
    assert "K1_USEFUL_NPU=1" not in scon
    assert "-DK1_USEFUL_NPU" not in scon
    assert "K1_PDM_CAPTURE=1" not in scon
    assert "-DK1_PDM_CAPTURE" not in scon
    assert "Glob('*.c')" not in scon
    assert 'Glob("*.c")' not in scon
    assert "Glob('arm_numeric_probe.c')" in scon
    assert "Glob('useful_npu_probe.c')" in scon
    assert "Glob('pdm_capture.c')" in scon
    assert "Glob('npu_load.c')" in scon


def test_ac5_host_libraries_build_with_werror(tmp_path):
    suffix = "dylib" if sys.platform == "darwin" else "so"
    arm = compile_arm_host(tmp_path / f"libarm.{suffix}")
    npu = compile_npu_host(tmp_path / f"libnpu.{suffix}")
    assert arm["exit_code"] == 0
    assert npu["exit_code"] == 0
    assert Path(arm["output"]).is_file()
    assert Path(npu["output"]).is_file()


def test_ac5_npu_cross_compile_is_not_useful(tmp_path):
    cross = cross_compile_npu(tmp_path / "useful_npu_probe.o")
    if ARM_GCC.is_file():
        assert cross["qualification_level"] == "CROSS_COMPILED"
        assert cross["executed"] is False
        assert cross["linked"] is False
        assert cross["cannot_satisfy"] == "useful-inference"
        assert ARM_SOURCE.is_file()
        assert NPU_SOURCE.is_file()
        assert ARM_ADAPTER == "k1-ra8p1-arm-numeric-v1"
    else:
        assert cross["status"] == "NOT_RUN"


def test_ac5_probe_entry_does_not_invoke(host_npu, host_arm):
    assert int(host_npu.lib.k1_useful_npu_probe()) == 1
    assert int(host_npu.lib.k1_useful_npu_invoke()) != 0
    assert host_npu.snapshot()["invoked"] is False
    assert int(host_arm.lib.k1_arm_numeric_prepare()) == 0
    assert int(host_arm.lib.k1_arm_binary_executed()) == 0
