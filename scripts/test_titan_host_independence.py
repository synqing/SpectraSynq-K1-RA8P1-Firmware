"""Offline checks for titan_host_independence.py (no hardware, no broker, no camera)."""
import copy
import json
import os
import stat
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import titan_host_independence as h  # noqa: E402

PROFILE = Path(__file__).resolve().parents[1] / "docs/emotiscope-port-001/presets/UNSELECTED-current-hybrid-tempo-crimson-v8-20261014.json"


def fake_camera(tmp: Path, body: str) -> Path:
    p = tmp / "fake_camera"
    p.write_text("#!/bin/bash\n" + body + "\n")
    p.chmod(p.stat().st_mode | stat.S_IEXEC)
    return p


def run_start(body: str, limit: float) -> str:
    with tempfile.TemporaryDirectory() as t:
        tmp = Path(t)
        h.c.CAMERA = fake_camera(tmp, body)
        h.CAMERA_START_LIMIT_S = limit
        run = h.Run(tmp)
        try:
            cam, _, _ = h.start_camera(run, 5)
            cam.wait(timeout=10)
            return "started"
        except RuntimeError as exc:
            return str(exc)


def test_quiet_stdout_longer_than_three_seconds_is_not_failure():
    # The 14 Oct defect: 3 s of quiet stdout raised queue.Empty.
    body = 'echo \'{"event":"camera_auto_acquisition"}\'; sleep 5; echo \'{"event":"recording_started","audio_connection":true}\'; sleep 1'
    assert run_start(body, 20) == "started"


def test_camera_exit_before_start_fails():
    assert "exited" in run_start("exit 4", 20)


def test_deadline_expiry_fails():
    assert "did not report" in run_start("sleep 30", 1.5)


def test_start_without_microphone_fails():
    assert "microphone" in run_start('echo \'{"event":"recording_started","audio_connection":false}\'', 20)


def boards_from(profile):
    boards = {}
    for b, row in profile["boards"].items():
        r = {("mode" if k == "current_mode" else k): v for k, v in row.items()}
        r.update(profile["pair"])
        r.update({k: (int(v) if isinstance(v, bool) else v) for k, v in profile["output"].items() if k != "period_us"})
        boards[b] = {"values": r}
    return boards


def palette_from(profile):
    return {"period_us": profile["output"]["period_us"]}


def test_profile_matches_itself():
    p = h.load_profile(PROFILE)
    assert h.profile_diff(boards_from(p), p, palette_from(p)) == {}
    assert h.compared_field_count(p) == 2 * (49 + 6 + 8) + 1


def test_missing_field_is_a_difference_not_a_skip():
    p = h.load_profile(PROFILE)
    boards = boards_from(p)
    del boards["1"]["values"]["palette"]
    assert h.profile_diff(boards, p, palette_from(p)) == {"1": {"palette": [h.MISSING, 39]}}


def test_empty_or_renamed_readback_goes_red():
    p = h.load_profile(PROFILE)
    empty = {"0": {"values": {}}, "1": {"values": {}}}
    diff = h.profile_diff(empty, p, palette_from(p))
    assert len(diff["0"]) == 63 and len(diff["1"]) == 63
    renamed = {b: {"values": {"x_" + k: v for k, v in r["values"].items()}} for b, r in boards_from(p).items()}
    assert len(h.profile_diff(renamed, p, palette_from(p))["0"]) == 63
    assert "0" in h.profile_diff({"1": boards_from(p)["1"]}, p, palette_from(p))


def test_pair_and_period_are_compared():
    p = h.load_profile(PROFILE)
    boards = boards_from(p)
    boards["0"]["values"]["pair_link"] = 1
    assert h.profile_diff(boards, p, palette_from(p)) == {"0": {"pair_link": [1, 0]}}
    assert h.profile_diff(boards_from(p), p, {"period_us": 8333}) == {"palette": {"period_us": [8333, 10000]}}
    assert "palette" in h.profile_diff(boards_from(p), p, {})


def test_profile_planted_faults_go_red():
    p = h.load_profile(PROFILE)
    for board, key, bad in (("1", "palette", 41), ("0", "mode", 1), ("1", "secondary_output_enabled", 0),
                            ("0", "primary_intensity", 1.0)):
        boards = boards_from(p)
        assert key in boards[board]["values"], key
        boards[board]["values"][key] = bad
        diff = h.profile_diff(boards, p)
        assert board in diff and len(diff[board]) == 1, (key, diff)
