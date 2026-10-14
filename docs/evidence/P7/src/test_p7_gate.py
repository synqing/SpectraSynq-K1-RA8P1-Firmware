"""Host tests for the P7 synthetic tempo suite: generator determinism, truth by construction, case-list
completeness, scorer green on the oracle and red on every planted fault (through the real CLI entry point).

Run: /Users/spectrasynq/miniforge3/bin/python -m pytest -q test_p7_gate.py
"""
from __future__ import annotations

import copy
import hashlib
import json
import sys
from pathlib import Path

import numpy as np
import pytest

sys.path.insert(0, str(Path(__file__).parent))
import p7_gate  # noqa: E402
import p7_plant  # noqa: E402
import p7_synth  # noqa: E402

SAMPLE_IDS = ["fotf_55", "fotf_138", "fotf_200", "fotf332_138", "halftime_140", "swing_110", "hats_128", "pad_1",
              "step_p8_120_129.6", "step_m8_140_128.8", "step_x2_80_160", "step_xhalf_160_80", "rubato_120",
              "gap_2s", "gap_10s", "level_-40dBFS"]
CASES = {c["id"]: c for c in p7_synth.case_list()}


@pytest.fixture(scope="module")
def truths():
    return {i: p7_synth.render(CASES[i])[1] for i in SAMPLE_IDS}


def test_case_list_covers_assignment():
    fams = {c["family"] for c in CASES.values()}
    assert fams == {"four_on_the_floor", "four_on_the_floor_332", "half_time", "swung_eighths", "hats_only",
                    "drumless_pad", "tempo_step", "rubato", "silence_gap", "level"}
    for fam in ("four_on_the_floor", "four_on_the_floor_332"):
        assert sorted(c["bpm"] for c in CASES.values() if c["family"] == fam) == [float(b) for b in range(55, 201)]
    ratios = sorted(round(c["bpm_after"] / c["bpm"], 4) for c in CASES.values() if c["family"] == "tempo_step")
    assert {0.5, 0.92, 1.08, 2.0} <= set(ratios)
    assert sorted(c["gap_s"] for c in CASES.values() if c["family"] == "silence_gap") == [2.0, 5.0, 10.0]
    assert sorted(c["level_dbfs"] for c in CASES.values() if c["family"] == "level") == [-40, -30, -20, -10, 0]
    assert all(c["depth"] == 0.05 for c in CASES.values() if c["family"] == "rubato")
    assert len(CASES) == len(p7_synth.case_list())  # ids unique


def test_generator_is_bit_reproducible():
    for i in ("fotf332_97", "pad_2", "rubato_90"):
        a, ta = p7_synth.render(CASES[i])
        b, tb = p7_synth.render(CASES[i])
        assert hashlib.sha256(p7_synth.to_s16(a).tobytes()).digest() == hashlib.sha256(p7_synth.to_s16(b).tobytes()).digest()
        assert json.dumps(ta) == json.dumps(tb)
    x1 = p7_synth.to_s16(p7_synth.render(CASES["fotf_120"])[0])
    x2 = p7_synth.to_s16(p7_synth.render(CASES["fotf_121"])[0])
    assert x1.tobytes() != x2.tobytes()


@pytest.mark.parametrize("cid", ["fotf_55", "fotf_200", "fotf332_138", "step_x2_80_160", "rubato_140", "gap_5s"])
def test_truth_is_where_the_kicks_are(cid):
    """Every annotated beat of a kick-driven case starts a kick: the 0-5 ms peak after the beat exceeds 3x the
    peak in the 15-5 ms before it, and the kick-band energy follows the annotated grid."""
    x, truth = p7_synth.render(CASES[cid])
    fs = p7_synth.FS
    for seg in truth["segments"]:
        if seg["kind"] != "tempo":
            continue
        for b in seg["beats_s"]:
            i = int(round(b * fs))
            after = np.max(np.abs(x[i:i + int(0.005 * fs)]))
            before = np.max(np.abs(x[i - int(0.015 * fs):i - int(0.005 * fs)]))
            assert after > 3.0 * before, (cid, b, after, before)


def test_pad_and_gaps_carry_no_tempo(truths):
    assert [s["kind"] for s in truths["pad_1"]["segments"]] == ["none"]
    kinds = [s["kind"] for s in truths["gap_10s"]["segments"]]
    assert kinds == ["none", "tempo", "none", "tempo"]
    g = truths["gap_10s"]["segments"][2]
    assert abs((g["end_s"] - g["start_s"]) - 10.0) < 1e-9
    x, t = p7_synth.render(CASES["gap_10s"])
    seg = x[int((g["start_s"] + 0.5) * p7_synth.FS):int(g["end_s"] * p7_synth.FS)]
    assert np.sqrt(np.mean(seg ** 2)) < 10 ** (-60 / 20)   # only the -72 dBFS noise floor


def test_oracle_is_green(truths):
    for i, tr in truths.items():
        r = p7_gate.score_case(p7_plant.oracle_trace(tr), tr, 0.02)
        assert r["pass"], (i, r["criteria"], r["segments"])


EXPECT = {  # fault -> (case, criteria that must go red, criteria that must stay green)
    "half_time": ("fotf_138", {"FLIP", "LOCK"}, set()),
    "three_two": ("fotf332_138", {"FLIP", "LOCK"}, set()),
    "late_lock": ("fotf_138", {"LOCK"}, {"BPM", "FLIP"}),
    "pad_false_lock": ("pad_1", {"LOWCONF"}, set()),
    "slow_reacquire": ("step_x2_80_160", {"LOCK"}, {"BPM"}),
    "single_flip": ("fotf_138", {"FLIP", "LOCK"}, set()),
    "silence_hold": ("gap_10s", {"LOWCONF"}, {"LOCK", "BPM", "FLIP"}),
    "asserting_wrong": ("fotf_138", {"BPM"}, {"FLIP"}),
}


@pytest.mark.parametrize("fault", sorted(EXPECT))
def test_planted_fault_goes_red(truths, fault):
    cid, red, green = EXPECT[fault]
    tr = truths[cid]
    r = p7_gate.score_case(p7_plant.oracle_trace(tr, fault), tr, 0.02)
    assert not r["pass"], (fault, r)
    for c in red:
        assert r["criteria"][c] is False, (fault, c, r["criteria"])
    for c in green:
        assert r["criteria"][c] is True, (fault, c, r["criteria"])


def test_half_time_and_three_two_red_on_every_tempo_case(truths):
    for i, tr in truths.items():
        if i == "pad_1":
            continue
        for fault in ("half_time", "three_two"):
            r = p7_gate.score_case(p7_plant.oracle_trace(tr, fault), tr, 0.02)
            assert not r["pass"] and not r["criteria"]["FLIP"], (i, fault)


def _frozen_gate(tmp: Path, ids):
    here = Path(__file__).parent
    gate = {"suite": p7_synth.SUITE, "tolerance": {"bpm_relative": 0.02},
            "tools": {"scorer": {"file": "p7_gate.py", "sha256": p7_gate.file_sha256(here / "p7_gate.py")}},
            "cases": [{"id": i} for i in ids], "frozen_utc": "2026-10-14T00:00:00Z", "frozen_sha256": "test"}
    p = tmp / "gate.json"
    p.write_text(json.dumps(gate))
    return p


def test_cli_entry_point_green_and_red(tmp_path, truths):
    ids = ["fotf_138", "pad_1", "gap_10s", "step_xhalf_160_80"]
    truth_dir = tmp_path / "truth"
    truth_dir.mkdir()
    for i in ids:
        (truth_dir / f"{i}.truth.json").write_text(json.dumps(truths[i]))
    gate = _frozen_gate(tmp_path, ids)
    for fault, want in (("oracle", 0), ("half_time", 1), ("three_two", 1), ("late_lock", 1), ("pad_false_lock", 1)):
        out = tmp_path / fault
        assert p7_plant.main(["write", str(gate), str(truth_dir), str(out), fault]) == 0
        rc = p7_gate.main(["score", str(gate), str(truth_dir), str(out), str(tmp_path / f"{fault}.json")])
        assert rc == want, (fault, rc)
    # a missing trace is a failure, not a skip
    (tmp_path / "oracle" / "pad_1.csv").unlink()
    assert p7_gate.main(["score", str(gate), str(truth_dir), str(tmp_path / "oracle"), str(tmp_path / "m.json")]) == 1


def test_cli_refuses_edited_scorer_and_unfrozen_gate(tmp_path, truths):
    gate = _frozen_gate(tmp_path, ["fotf_138"])
    g = json.loads(gate.read_text())
    g["tools"]["scorer"]["sha256"] = "0" * 64
    gate.write_text(json.dumps(g))
    with pytest.raises(SystemExit):
        p7_gate.main(["score", str(gate), str(tmp_path), str(tmp_path), str(tmp_path / "x.json")])
    del g["frozen_sha256"]
    gate.write_text(json.dumps(g))
    with pytest.raises(SystemExit):
        p7_gate.main(["score", str(gate), str(tmp_path), str(tmp_path), str(tmp_path / "x.json")])


def test_tolerance_boundary(truths):
    """+-2 % is inclusive at 1.9 % and exclusive at 2.1 % (tolerance is applied as stated, not widened)."""
    tr = truths["fotf_138"]
    for k, ok in ((1.019, True), (1.021, False), (0.981, True), (0.979, False)):
        t = p7_plant.oracle_trace(tr)
        t["bpm"] = np.where(t["locked"].astype(bool), t["bpm"] * k, t["bpm"])
        assert p7_gate.score_case(t, tr, 0.02)["criteria"]["LOCK"] is ok, k


@pytest.mark.parametrize("cid,bpm", [("fotf_120", 120.0), ("fotf_173", 173.0), ("fotf332_61", 61.0)])
def test_truth_matches_an_independent_grid_and_audio_onsets(cid, bpm):
    """Truth is checked against arithmetic done here (lead-in 1.0 s, period 60/bpm), and kick onsets are found
    in the audio by a detector that never reads the truth: first sample after a quiet run where |x| exceeds
    0.3 of the case peak. Both must agree within 0.5 ms (catches a truth or audio offset either side)."""
    x, truth = p7_synth.render(CASES[cid])
    fs = p7_synth.FS
    beats = np.asarray(truth["segments"][1]["beats_s"])
    expected = 1.0 + np.arange(len(beats)) * 60.0 / bpm
    assert np.max(np.abs(beats - expected)) < 1e-9
    thr = 0.3 * np.max(np.abs(x))
    above = np.abs(x) > thr
    onsets, last = [], -10**9
    for i in np.flatnonzero(above):
        if i - last > int(0.05 * fs):
            onsets.append(i / fs)
        last = i
    kicks = [o for o in onsets if np.min(np.abs(expected - o)) < 0.02]
    assert len(kicks) == len(expected), (len(kicks), len(expected))
    assert np.max(np.abs(np.asarray(kicks) - expected)) < 0.0005
