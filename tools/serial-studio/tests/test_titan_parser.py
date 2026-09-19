#!/usr/bin/env python3
"""Parser tests: unavailable stays unavailable, labels are human, no invented contract."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
PARSER = HERE / "titan_parser.js"
UNAVAILABLE = "UNAVAILABLE"
sys.path.insert(0, str(HERE))


def run_parse(frame) -> list:
    runner = f"""{PARSER.read_text()}
var _result = parse({json.dumps(frame)});
console.log(JSON.stringify(_result));
"""
    proc = subprocess.run(
        ["node", "-"],
        input=runner,
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr)
    return json.loads(proc.stdout)


def live_frame(**extra) -> dict:
    row = {
        "schema": 2,
        "mode": 3,
        "identity_ok": 1,
        "protocol": 1,
        "simulated": 1,
        "health_valid": 1,
        "mic_a_valid": 1,
        "mic_b_valid": 1,
        "uid": "545433931bd25436593630352d068363",
        "build": "77f77ecf2b449de7",
        "source": "6b1e7bc5c9f9871e",
        "contract": "sr24000.hop180.bins80.xover40",
        "hop_max_us": 5600,
        "late_starts": 0,
        "deadlines": 0,
        "crc_mismatches": 0,
        "dma_irqs": 607,
        "latched_frames": 607,
        "led_faults": 0,
        "mic_a_rms_dbfs": -28.0,
        "mic_b_rms_dbfs": -27.4,
        "capture_rate_hz": 41405.5,
        "hop_samples": 180,
        "admitted_rate_hz": 24000,
        "gate_result": 0,
        "sequence": 10,
    }
    row.update(extra)
    return row


def test_live_human_labels():
    out = run_parse(json.dumps(live_frame()))
    assert out[0] == "SIMULATED DATA — TITAN NOT CONNECTED"
    assert out[1] == "LIVE"
    assert out[2] == "NONE"
    assert out[3].startswith("54543393")
    assert out[8] == -28.0
    assert out[12] == 41405.5
    assert out[17] == "OK"


def test_fault_is_failed_not_a_number():
    out = run_parse(json.dumps(live_frame(mode=6, gate_result=3, deadlines=1, led_faults=1)))
    assert out[1] == "FAULT"
    assert out[2] == "FAILED"
    assert out[17] == "FAULT"
    assert out[15] == 1


def test_replay_label():
    out = run_parse(json.dumps(live_frame(mode=7, gate_result=3)))
    assert out[1] == "REPLAY"
    assert out[2] == "FAILED"


def test_missing_rms_with_valid_flag_is_rejected():
    row = live_frame()
    del row["mic_a_rms_dbfs"]
    assert run_parse(json.dumps(row)) == []


def test_invalid_mic_does_not_invent_full_scale():
    row = live_frame(mic_a_valid=0, mic_b_valid=0)
    del row["mic_a_rms_dbfs"]
    del row["mic_b_rms_dbfs"]
    out = run_parse(json.dumps(row))
    assert out[8] == UNAVAILABLE
    assert out[9] == UNAVAILABLE
    assert out[10] == 0
    assert 0 not in (out[8], out[9])


def test_invalid_health_does_not_invent_zero_faults_or_24k():
    row = live_frame(health_valid=0)
    for key in (
        "hop_max_us",
        "late_starts",
        "deadlines",
        "crc_mismatches",
        "dma_irqs",
        "latched_frames",
        "led_faults",
        "capture_rate_hz",
        "hop_samples",
        "admitted_rate_hz",
    ):
        del row[key]
    out = run_parse(json.dumps(row))
    assert out[13] == UNAVAILABLE
    assert out[14] == UNAVAILABLE
    assert out[16] == UNAVAILABLE
    assert out[12] == UNAVAILABLE
    assert out[21] == UNAVAILABLE
    assert out[22] == UNAVAILABLE
    assert 0 not in (out[13], out[14], out[16])
    assert 24000 not in out
    assert 180 not in out


def test_rejects_whitespace_as_zero():
    row = live_frame()
    row["late_starts"] = " "
    assert run_parse(json.dumps(row)) == []
    row = live_frame()
    row["late_starts"] = ""
    assert run_parse(json.dumps(row)) == []


def test_rejects_negative_counts_and_unknown_enums():
    assert run_parse(json.dumps(live_frame(late_starts=-1))) == []
    assert run_parse(json.dumps(live_frame(mode=99))) == []
    assert run_parse(json.dumps(live_frame(protocol=2))) == []
    row = live_frame()
    row["identity_ok"] = "yes"
    assert run_parse(json.dumps(row)) == []


def test_sequence_must_be_safe_int_or_string():
    assert run_parse(json.dumps(live_frame(sequence=1.5))) == []
    assert run_parse(json.dumps(live_frame(sequence=-4))) == []


def test_sequence_as_string_is_preserved():
    out = run_parse(json.dumps(live_frame(sequence="18446744073709551615")))
    assert out[23] == "18446744073709551615"


def test_identity_required_when_ok():
    row = live_frame()
    del row["uid"]
    assert run_parse(json.dumps(row)) == []


def test_corrupt_json():
    assert run_parse("{not json") == []
    assert run_parse("") == []


def test_identified_unidentified_when_not_ok():
    row = live_frame(identity_ok=0)
    del row["uid"]
    del row["build"]
    del row["source"]
    del row["contract"]
    out = run_parse(json.dumps(row))
    assert out[3] == "UNIDENTIFIED"


def test_python_oracle_matches_js():
    from titan_decode import decode

    out = run_parse(json.dumps(live_frame()))
    cells, _, _ = decode(live_frame())
    assert cells is not None
    assert [str(x) for x in out] == [str(x) for x in cells]


def test_accepts_csv_cells():
    out = run_parse(json.dumps(live_frame()))
    csv_line = ",".join(str(x) for x in out)
    assert run_parse(csv_line) == [str(x) for x in out]


def test_accepts_byte_array_frames():
    raw = json.dumps(live_frame()).encode("utf-8")
    out = run_parse(list(raw))
    assert out[0] == "SIMULATED DATA — TITAN NOT CONNECTED"
    assert out[3].startswith("54543393")


def test_invalid_health_ignores_leftover_numbers():
    row = live_frame(health_valid=0, hop_max_us=5600, hop_samples=180, admitted_rate_hz=24000)
    out = run_parse(json.dumps(row))
    assert out[13] == UNAVAILABLE
    assert out[21] == UNAVAILABLE
    assert out[22] == UNAVAILABLE
    assert 24000 not in (out[21], out[22], out[12])


def test_missing_contract_when_health_valid_is_rejected():
    row = live_frame()
    del row["hop_samples"]
    del row["admitted_rate_hz"]
    assert run_parse(json.dumps(row)) == []


if __name__ == "__main__":
    test_live_human_labels()
    test_fault_is_failed_not_a_number()
    test_replay_label()
    test_missing_rms_with_valid_flag_is_rejected()
    test_invalid_mic_does_not_invent_full_scale()
    test_invalid_health_does_not_invent_zero_faults_or_24k()
    test_rejects_whitespace_as_zero()
    test_rejects_negative_counts_and_unknown_enums()
    test_sequence_must_be_safe_int_or_string()
    test_sequence_as_string_is_preserved()
    test_identity_required_when_ok()
    test_corrupt_json()
    test_identified_unidentified_when_not_ok()
    test_python_oracle_matches_js()
    test_accepts_csv_cells()
    test_accepts_byte_array_frames()
    test_invalid_health_ignores_leftover_numbers()
    test_missing_contract_when_health_valid_is_rejected()
    print("parser_tests_ok")
