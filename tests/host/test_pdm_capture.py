"""WP13 HOST PDM capture: units, first-interval refusal, adapter bind, fail-closed current-target."""
from __future__ import annotations

import sys
import json
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from run_pdm_capture import (  # noqa: E402
    ADAPTER_ID,
    ARM_GCC,
    CORRECTED_UNITS,
    CURRENT_TARGET_IDENTITY,
    REQUIRED_PDM_FIELDS,
    SOURCE_C,
    SOURCE_H,
    VENDOR_16000_SUBMISSION_BYTES,
    bind_adapter,
    build_adapter_document,
    compile_host_library,
    cross_compile_pdm_capture,
    derive_units,
    load_adapter,
    load_host_pdm,
    make_receipt,
    missing_current_target_fields,
    observed_source_hashes,
    physical_capture_not_run,
    run_host_fixture,
    run_host_stream_fixture,
)


@pytest.fixture(scope="module")
def host_pdm():
    pdm, receipt = load_host_pdm()
    assert receipt["qualification_level"] == "HOST"
    assert receipt["exit_code"] == 0
    return pdm


@pytest.mark.parametrize(
    "frames,channels",
    [(16000, 2), (16000, 1), (8000, 2), (8000, 1)],
)
def test_ac1_host_fixtures_derive_corrected_units(host_pdm, frames, channels):
    result = run_host_fixture(host_pdm, frames, channels)
    expected = CORRECTED_UNITS[(frames, channels)]
    after_stop = result["after_stop"]
    units = derive_units(frames, channels)
    assert result["qualification_level"] == "HOST"
    assert after_stop["capture_bytes"] == expected["capture_bytes"] == units["capture_bytes"]
    assert after_stop["conversion_bytes"] == expected["conversion_bytes"] == units["conversion_bytes"]
    assert after_stop["submission_bytes"] == expected["submission_bytes"] == units["submission_bytes"]
    assert after_stop["submission_bytes"] == after_stop["conversion_bytes"]
    assert after_stop["callback_interval"] == expected["callback_interval"]
    assert after_stop["capture_channels"] == 1
    assert after_stop["capture_element_bytes"] == 4
    assert after_stop["output_element_bytes"] == 2
    assert after_stop["output_channels"] == channels
    if frames == 16000 and channels == 2:
        assert after_stop["submission_bytes"] != VENDOR_16000_SUBMISSION_BYTES


def test_ac1_8000_changes_bytes_from_16000(host_pdm):
    stereo_16k = run_host_fixture(host_pdm, 16000, 2)["after_stop"]
    stereo_8k = run_host_fixture(host_pdm, 8000, 2)["after_stop"]
    mono_16k = run_host_fixture(host_pdm, 16000, 1)["after_stop"]
    assert stereo_8k["requested_frames"] != stereo_16k["requested_frames"]
    assert stereo_8k["capture_bytes"] != stereo_16k["capture_bytes"]
    assert stereo_8k["callback_interval"] != stereo_16k["callback_interval"]
    assert mono_16k["conversion_bytes"] != stereo_16k["conversion_bytes"]
    assert mono_16k["submission_bytes"] == 16000 * 1 * 2
    assert stereo_8k["submission_bytes"] == 8000 * 2 * 2


def test_ac1_first_data_is_not_complete(host_pdm):
    host_pdm.configure(16000, 2)
    host_pdm.on_data(4000)
    snap = host_pdm.snapshot()
    assert snap["first_data_seen"] is True
    assert snap["received_elements"] == 4000
    assert snap["full_buffer_completion"] is False
    assert snap["first_data_does_not_prove_complete"] is True
    assert snap["ownership_transferred"] is False
    assert snap["final_stopped_count"] is None
    assert snap["stop_tail"] is None
    from ctypes import c_int16, c_int32

    capture = (c_int32 * 16000)()
    output = (c_int16 * 32000)()
    rc = host_pdm.lib.k1_pdm_convert(capture, 16000, output, 32000)
    assert rc != 0


def test_ac1_convert_refuses_until_stop_matches(host_pdm):
    from ctypes import c_int16, c_int32

    host_pdm.configure(8000, 1)
    host_pdm.on_data(2000)
    host_pdm.on_data(2000)
    host_pdm.on_data(2000)
    host_pdm.on_data(2000)
    assert host_pdm.snapshot()["full_buffer_completion"] is True
    capture = (c_int32 * 8000)()
    output = (c_int16 * 8000)()
    assert host_pdm.lib.k1_pdm_convert(capture, 8000, output, 8000) != 0
    host_pdm.stop(7999)
    assert host_pdm.snapshot()["ownership_transferred"] is False
    assert host_pdm.lib.k1_pdm_convert(capture, 8000, output, 8000) != 0


def test_ac2_changed_source_hash_invalidates_old_adapter():
    adapter = build_adapter_document()
    observed = observed_source_hashes()
    assert bind_adapter(observed, adapter)["ok"] is True
    tampered = dict(observed)
    tampered["application"] = "ab" * 32
    declined = bind_adapter(tampered, adapter)
    assert declined["ok"] is False
    assert declined["declined"] == "unrecognised_source_hash"
    assert "application" in declined["mismatched"]
    header = dict(observed)
    header["application_header"] = "cd" * 32
    declined_header = bind_adapter(header, adapter)
    assert declined_header["ok"] is False
    assert "application_header" in declined_header["mismatched"]


def test_ac2_adapter_file_matches_current_sources():
    on_disk = load_adapter()
    assert on_disk["id"] == ADAPTER_ID
    bind = bind_adapter(observed_source_hashes(), on_disk)
    assert bind["ok"] is True
    assert bind["declined"] is None


def test_ac2_levels_remain_distinct(tmp_path):
    host_receipt = make_receipt(
        qualification_level="HOST",
        pdm=_host_pdm_fields(),
        identities={"adapter_id": ADAPTER_ID},
        execution={"arm_binary_executed": False},
    )
    assert host_receipt["qualification_level"] == "HOST"
    assert host_receipt["ok"] is True
    assert "pdm-current-target" in host_receipt["blocked_cells"]
    assert host_receipt["physical_capture"] == "NOT_RUN"
    cross = cross_compile_pdm_capture(tmp_path / "pdm_capture.o")
    if ARM_GCC.is_file():
        assert cross["qualification_level"] == "CROSS_COMPILED"
        assert cross["ok"] is True
        assert cross["executed"] is False
        assert cross["linked"] is False
        assert cross["physical_capture"] == "NOT_RUN"
        assert cross["qualification_level"] != "HOST"
        assert cross["qualification_level"] != "CURRENT_TARGET"
    else:
        assert cross["status"] == "NOT_RUN"
        assert cross["missing_tool"] == str(ARM_GCC)
    current = make_receipt(
        qualification_level="CURRENT_TARGET",
        pdm=_host_pdm_fields(),
        identities={},
        execution={"arm_binary_executed": False},
    )
    assert current["ok"] is False
    assert current["status"] == "FAIL_CLOSED"
    assert current["qualification_level"] == "CURRENT_TARGET"
    assert current["qualification_level"] != host_receipt["qualification_level"]
    assert current["qualification_level"] != "CROSS_COMPILED"


def test_ac3_current_target_schema_fails_closed_when_absent():
    pdm = _host_pdm_fields()
    receipt = make_receipt(qualification_level="CURRENT_TARGET", pdm=pdm, identities={})
    codes = {item["code"] for item in receipt["errors"]}
    messages = {item["message"] for item in receipt["errors"]}
    assert receipt["ok"] is False
    assert receipt["status"] == "FAIL_CLOSED"
    assert "current-target-identity-missing" in codes
    assert "pdm-current-target-incomplete" in codes
    for field in CURRENT_TARGET_IDENTITY:
        assert field in messages
    for field in ("final_stopped_count", "stop_tail", "output_duration_s"):
        assert field in messages
    assert receipt["physical_capture"] == "NOT_RUN"


def test_ac3_current_target_partial_identity_still_fails():
    pdm = dict(_host_pdm_fields())
    pdm.update(
        {
            "full_buffer_completion": True,
            "final_stopped_count": 16000,
            "stop_tail": 0,
            "output_duration_s": 1.0,
        }
    )
    identities = {
        "target_uid": "545433931bd25436593630352d068363",
        "build_id": "c4ceebe7f4d899d39a917fb12c385c0f743278fd53aa2a82045230f3490e87bb",
        "ownership_record": {"exclusive_owner": "nobody", "method": "re-enumeration"},
    }
    missing = missing_current_target_fields(identities)
    assert "loaded_image_sha256" in missing
    assert "measurement_method_id" in missing
    assert "input_identities" in missing
    assert "runtime_identity_method" in missing
    receipt = make_receipt(qualification_level="CURRENT_TARGET", pdm=pdm, identities=identities)
    assert receipt["ok"] is False
    assert receipt["status"] == "FAIL_CLOSED"


def test_ac3_port_name_is_not_identity():
    pdm = dict(_host_pdm_fields())
    pdm.update(
        {
            "full_buffer_completion": True,
            "final_stopped_count": 16000,
            "stop_tail": 0,
            "output_duration_s": 1.0,
        }
    )
    identities = {
        "target_uid": "545433931bd25436593630352d068363",
        "loaded_image_sha256": "00" * 32,
        "build_id": "11" * 32,
        "measurement_method_id": "pdm-loopback",
        "input_identities": {"tone": "1khz"},
        "runtime_identity_method": "uid-readback",
        "ownership_record": {"exclusive_owner": "agent", "method": "port-name"},
        "port": "/dev/cu.usbmodem21401",
    }
    receipt = make_receipt(qualification_level="CURRENT_TARGET", pdm=pdm, identities=identities)
    assert receipt["ok"] is False
    assert any(item["code"] == "port-name-is-not-identity" for item in receipt["errors"])


def test_ac3_physical_capture_is_not_run_without_exclusive_target():
    receipt = physical_capture_not_run()
    assert receipt["physical_capture"] == "NOT_RUN"
    assert receipt["qualification_level"] == "HOST"
    assert receipt["identities"]["live_target"] == "NOT_VERIFIED"
    assert receipt["identities"]["last_identified_level"] == "HISTORICAL_TARGET"
    assert receipt["pdm"]["stop_tail"] is None
    assert receipt["pdm"]["final_stopped_count"] is None
    assert receipt["pdm"]["full_buffer_completion"] == "unknown"
    assert "pdm-current-target" in receipt["blocked_cells"]
    assert receipt["status"] != "PASS" or receipt["physical_capture"] == "NOT_RUN"


def test_ac3_compiler_facts_cannot_claim_current_target():
    receipt = make_receipt(
        qualification_level="CURRENT_TARGET",
        pdm=_host_pdm_fields(),
        identities={},
        execution={"compiler_facts_only": True, "arm_binary_executed": False},
    )
    assert receipt["ok"] is False
    assert any(item["code"] == "compiler-facts-as-current-target" for item in receipt["errors"])


def test_ac4_first_interval_claim_is_rejected():
    pdm = dict(_host_pdm_fields())
    pdm["first_data_does_not_prove_complete"] = False
    receipt = make_receipt(qualification_level="HOST", pdm=pdm)
    assert receipt["ok"] is False
    assert any(item["code"] == "pdm-first-interval-as-completion" for item in receipt["errors"])


def test_ac4_missing_required_pdm_field_fails():
    pdm = dict(_host_pdm_fields())
    del pdm["stop_tail"]
    receipt = make_receipt(qualification_level="HOST", pdm=pdm)
    assert receipt["ok"] is False
    assert any(item["code"] == "pdm-field-missing" for item in receipt["errors"])
    for field in REQUIRED_PDM_FIELDS:
        assert field in _host_pdm_fields()


def test_stream_ping_pong_transfers_exact_timestamped_ownership(host_pdm):
    host_pdm.stream_configure(8, 2)
    host_pdm.stream_start(100)
    assert host_pdm.stream_snapshot() == {
        "configured": True,
        "running": True,
        "halted": False,
        "active_slot": 0,
        "epoch": 1,
        "completed_slots": 0,
        "overflow_events": 0,
        "drop_events": 0,
        "recovery_count": 0,
        "slot_states": [1, 0],
        "slot_received": [0, 0],
    }
    for end_us in (110, 120, 130):
        assert host_pdm.stream_on_data(2, end_us) == 0
    assert host_pdm.stream_on_data(2, 140) == 1
    first = host_pdm.stream_acquire()
    assert first == {
        "slot": 0,
        "epoch": 1,
        "sequence": 1,
        "capture_start_us": 100,
        "capture_end_us": 140,
    }
    assert host_pdm.stream_release(first) == 0
    for end_us in (150, 160, 170):
        assert host_pdm.stream_on_data(2, end_us) == 0
    assert host_pdm.stream_on_data(2, 180) == 1
    second = host_pdm.stream_acquire()
    assert second == {
        "slot": 1,
        "epoch": 1,
        "sequence": 2,
        "capture_start_us": 140,
        "capture_end_us": 180,
    }
    assert host_pdm.stream_release(second) == 0
    assert host_pdm.stream_snapshot()["active_slot"] == 0


def test_stream_overflow_halts_without_overwriting_ready_or_consumer(host_pdm):
    host_pdm.stream_configure(4, 2)
    host_pdm.stream_start(1000)
    assert host_pdm.stream_on_data(2, 1010) == 0
    assert host_pdm.stream_on_data(2, 1020) == 1
    first = host_pdm.stream_acquire()
    assert first is not None
    assert host_pdm.stream_on_data(2, 1030) == 0
    assert host_pdm.stream_on_data(2, 1040) == -2
    snap = host_pdm.stream_snapshot()
    assert snap["running"] is False
    assert snap["halted"] is True
    assert snap["active_slot"] == 0xFFFFFFFF
    assert snap["slot_states"] == [3, 2]
    assert snap["overflow_events"] == 1
    assert snap["drop_events"] == 1
    second = host_pdm.stream_acquire()
    assert second is not None
    assert second["slot"] == 1
    assert second["capture_start_us"] == 1020
    assert second["capture_end_us"] == 1040
    assert host_pdm.lib.k1_pdm_stream_recover(2000) == -3
    assert host_pdm.stream_release(first) == 0
    assert host_pdm.stream_release(second) == 0
    assert host_pdm.lib.k1_pdm_stream_recover(2000) == 0
    recovered = host_pdm.stream_snapshot()
    assert recovered["running"] is True
    assert recovered["halted"] is False
    assert recovered["epoch"] == 2
    assert recovered["recovery_count"] == 1
    assert recovered["overflow_events"] == 1
    assert recovered["drop_events"] == 1
    assert host_pdm.stream_release(first) == -3


def test_stream_rejects_bad_geometry_and_nonmonotonic_timestamp(host_pdm):
    with pytest.raises(Exception, match="stream_configure_failed"):
        host_pdm.stream_configure(180, 16)
    host_pdm.stream_configure(32, 16)
    host_pdm.stream_start(500)
    assert host_pdm.stream_on_data(8, 510) == -1
    assert host_pdm.stream_on_data(16, 499) == -1
    snap = host_pdm.stream_snapshot()
    assert snap["slot_received"] == [0, 0]
    assert snap["completed_slots"] == 0


def test_stream_stop_discards_only_incomplete_producer_slot(host_pdm):
    host_pdm.stream_configure(4, 2)
    host_pdm.stream_start(10)
    assert host_pdm.stream_on_data(2, 20) == 0
    host_pdm.lib.k1_pdm_stream_stop()
    snap = host_pdm.stream_snapshot()
    assert snap["running"] is False
    assert snap["halted"] is False
    assert snap["slot_states"] == [0, 0]
    assert snap["slot_received"] == [0, 0]


def test_stream_receipt_fixture_captures_normal_overflow_and_recovery(host_pdm):
    result = run_host_stream_fixture(host_pdm)
    assert result["qualification_level"] == "HOST"
    assert result["physical_capture"] == "NOT_RUN"
    assert result["normal"]["owner"]["capture_start_us"] == 100
    assert result["normal"]["owner"]["capture_end_us"] == 140
    assert result["overflow"]["return_code"] == -2
    assert result["overflow"]["snapshot"]["slot_states"] == [3, 2]
    assert result["overflow"]["snapshot"]["overflow_events"] == 1
    assert result["overflow"]["recover_while_owned_return_code"] == -3
    assert result["recovered"]["running"] is True
    assert result["recovered"]["recovery_count"] == 1


def test_two_stream_contexts_keep_dual_microphone_ownership_independent(host_pdm):
    import ctypes

    class Slot(ctypes.Structure):
        _fields_ = [
            ("state", ctypes.c_uint32),
            ("received", ctypes.c_uint32),
            ("epoch", ctypes.c_uint32),
            ("sequence", ctypes.c_uint32),
            ("capture_start_us", ctypes.c_uint64),
            ("capture_end_us", ctypes.c_uint64),
        ]

    class Stream(ctypes.Structure):
        _fields_ = [
            ("slots", Slot * 2),
            ("elements_per_slot", ctypes.c_uint32),
            ("interval_elements", ctypes.c_uint32),
            ("active_slot", ctypes.c_uint32),
            ("epoch", ctypes.c_uint32),
            ("next_sequence", ctypes.c_uint32),
            ("completed_slots", ctypes.c_uint32),
            ("overflow_events", ctypes.c_uint32),
            ("drop_events", ctypes.c_uint32),
            ("recovery_count", ctypes.c_uint32),
            ("configured", ctypes.c_int),
            ("running", ctypes.c_int),
            ("halted", ctypes.c_int),
        ]

    lib = host_pdm.lib
    stream_pointer = ctypes.POINTER(Stream)
    lib.k1_pdm_stream_context_configure.argtypes = [stream_pointer, ctypes.c_uint32, ctypes.c_uint32]
    lib.k1_pdm_stream_context_start.argtypes = [stream_pointer, ctypes.c_uint64]
    lib.k1_pdm_stream_context_on_data.argtypes = [stream_pointer, ctypes.c_uint32, ctypes.c_uint64]
    lib.k1_pdm_stream_context_acquire.argtypes = [
        stream_pointer,
        ctypes.POINTER(ctypes.c_uint32),
        ctypes.POINTER(ctypes.c_uint32),
        ctypes.POINTER(ctypes.c_uint32),
        ctypes.POINTER(ctypes.c_uint64),
        ctypes.POINTER(ctypes.c_uint64),
    ]
    lib.k1_pdm_stream_context_release.argtypes = [
        stream_pointer, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32
    ]

    streams = [Stream(), Stream()]
    for stream in streams:
        assert lib.k1_pdm_stream_context_configure(ctypes.byref(stream), 120, 120) == 0
        assert lib.k1_pdm_stream_context_start(ctypes.byref(stream), 1000) == 0
    assert lib.k1_pdm_stream_context_on_data(ctypes.byref(streams[0]), 120, 8500) == 1
    assert streams[0].completed_slots == 1
    assert streams[1].completed_slots == 0
    assert lib.k1_pdm_stream_context_on_data(ctypes.byref(streams[1]), 120, 8504) == 1

    owners = []
    for stream in streams:
        slot = ctypes.c_uint32()
        epoch = ctypes.c_uint32()
        sequence = ctypes.c_uint32()
        start = ctypes.c_uint64()
        end = ctypes.c_uint64()
        assert lib.k1_pdm_stream_context_acquire(
            ctypes.byref(stream), ctypes.byref(slot), ctypes.byref(epoch),
            ctypes.byref(sequence), ctypes.byref(start), ctypes.byref(end)
        ) == 0
        owners.append((slot.value, epoch.value, sequence.value, start.value, end.value))
    assert owners == [(0, 1, 1, 1000, 8500), (0, 1, 1, 1000, 8504)]
    assert lib.k1_pdm_stream_context_release(ctypes.byref(streams[0]), *owners[0][:3]) == 0
    assert streams[0].slots[0].state == 0
    assert streams[1].slots[0].state == 3
    assert lib.k1_pdm_stream_context_release(ctypes.byref(streams[1]), *owners[1][:3]) == 0


def test_sconscript_does_not_glob_all_c():
    text = (ROOT / "platform/ra8p1/SConscript").read_text(encoding="utf-8")
    assert "Glob('*.c')" not in text
    assert 'Glob("*.c")' not in text
    assert "Glob('pdm_capture.c')" in text
    assert "Glob('pdm_target.c')" in text
    assert "Glob('npu_load.c')" in text


def test_pdm_probe_is_default_off_and_cdc_path_remains():
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
    scon = (ROOT / "platform/ra8p1/SConscript").read_text(encoding="utf-8")
    assert "#ifdef K1_PDM_CAPTURE" in entry
    assert "k1_pdm_configure" in entry
    assert "R_USB_Open" in entry
    assert "R_USB_Read" in entry
    assert "R_USB_Write" in entry
    assert "k1_fixture_consume" in entry
    assert "K1_PDM_CAPTURE=1" not in scon
    assert "-DK1_PDM_CAPTURE" not in scon
    assert "K1_PDM_TARGET=1" not in scon
    assert "-DK1_PDM_TARGET" not in scon
    assert not SOURCE_C.read_text(encoding="utf-8").count("R_PDM_")
    assert "r_pdm" not in SOURCE_C.read_text(encoding="utf-8")


def test_dual_pdm_starts_only_after_usb_configuration_is_observed():
    entry = (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
    usb_open = entry.index("R_USB_Open")
    configured = entry.index("case USB_STATUS_CONFIGURED")
    pdm_start = entry.index("if(attached && !k1_pdm_target_initialised())")
    pdm_poll = entry.index("k1_pdm_target_poll();", pdm_start)
    assert usb_open < configured < pdm_start < pdm_poll
    assert entry[:configured].count("k1_pdm_target_initialise()") == 0


def test_pdm_target_is_dual_edge_dmac_and_excludes_parallel_cpu_fifo_drain():
    target = (ROOT / "platform/ra8p1/pdm_target.c").read_text(encoding="utf-8")
    header = (ROOT / "platform/ra8p1/pdm_target.h").read_text(encoding="utf-8")
    build = (ROOT / "scripts/build_scalar.py").read_text(encoding="utf-8")
    assert ".activation_source = ELC_EVENT_PDM_DAT2" in target
    assert ".activation_source = ELC_EVENT_PDM_DAT0" in target
    assert ".channel = K1_PDM_TARGET_RISE_DMA_CHANNEL" in target
    assert ".channel = K1_PDM_TARGET_FALL_DMA_CHANNEL" in target
    assert "capture_rise_pdm_cfg.dat_irq = FSP_INVALID_VECTOR" in target
    assert "capture_fall_pdm_cfg.dat_irq = FSP_INVALID_VECTOR" in target
    assert "capture_rise_pdm_cfg.pcm_edge = PDM_INPUT_DATA_EDGE_RISE" in target
    assert "capture_fall_pdm_cfg.pcm_edge = PDM_INPUT_DATA_EDGE_FALL" in target
    assert target.count("interrupt_threshold = PDM_INTERRUPT_THRESHOLD_8") == 2
    assert "capture_rise_pdm_cfg.p_extend = &capture_rise_pdm_extend" in target
    assert "capture_fall_pdm_cfg.p_extend = &capture_fall_pdm_extend" in target
    assert "const int16_t sample = (int16_t) (raw << 1)" in target
    assert "K1_PDM_TARGET_PROGRAMME_LANE 0u" in header
    assert "K1_PDM_TARGET_MEASUREMENT_LANE 1u" in header
    assert "K1_PDM_TARGET_SLOT_ELEMENTS 120u" in header
    assert "K1_PDM_TARGET_SLOT_DURATION_US 7500u" in header
    assert "sample_rate_match\\\":false" in (ROOT / "platform/ra8p1/hal_entry.c").read_text(encoding="utf-8")
    assert "PDM_CFG_DMAC_ENABLE (1)" in build
    assert "VECTOR_NUMBER_DMAC1_INT" in build
    assert "VECTOR_NUMBER_PDM_ERR0" in build
    assert "generated_transfer_symbols_linked" in build


def test_dual_target_build_is_bound_to_working_firmware_contract():
    contract_path = ROOT / "docs/dual-im69d130-source-contract.json"
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
    build = (ROOT / "scripts/build_scalar.py").read_text(encoding="utf-8")
    assert contract["contract_id"] == "k1-dual-im69d130-working-source-v1"
    assert contract["electrical_contract"]["im1"]["role"] == "programme_ap_source"
    assert contract["electrical_contract"]["im2"]["role"] == "measurement_only"
    assert contract["working_capture_contract"]["sample_rate_hz"] == 12800
    assert contract["working_capture_contract"]["frames_per_slot"] == 96
    assert contract["titan_current_boundary"]["sample_rate_parity"] is False
    assert "PDM_SOURCE_CONTRACT" in build
    assert "source_contract_sha256" in build


def test_host_library_builds_with_werror(tmp_path):
    receipt = compile_host_library(tmp_path / ("libpdm_capture.dylib" if sys.platform == "darwin" else "libpdm_capture.so"))
    assert receipt["exit_code"] == 0
    assert Path(receipt["output"]).is_file()


def _host_pdm_fields() -> dict:
    return {
        "requested_frames": 16000,
        "capture_bytes": 64000,
        "conversion_bytes": 64000,
        "submission_bytes": 64000,
        "capture_channels": 1,
        "capture_element_bytes": 4,
        "output_element_bytes": 2,
        "first_data_does_not_prove_complete": True,
        "full_buffer_completion": "unknown",
        "final_stopped_count": None,
        "stop_tail": None,
    }
