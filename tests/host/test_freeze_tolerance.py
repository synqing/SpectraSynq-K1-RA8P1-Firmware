"""freeze_tolerance: the clock stamps the freeze, the file becomes read-only, and verify goes red on
an edited criterion, a writable file, or a result written before the freeze."""
import json
import os
import stat
import sys
import time
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
try:
    import freeze_tolerance as ft
finally:
    sys.path.pop(0)


def _tol(tmp):
    p = tmp / "TOLERANCE-FROZEN-x.json"
    p.write_text(json.dumps({"purpose": "x", "pass": "r >= 0.9"}))
    return p


def test_freeze_stamps_clock_hash_mode_and_status(tmp_path):
    p, status = _tol(tmp_path), tmp_path / "STATUS.md"
    out = ft.freeze(p, status, "P9")
    data = json.loads(p.read_text())
    assert data["frozen_utc"] == out["frozen_utc"] and data["frozen_sha256"] == ft.body_sha256(data)
    assert not os.access(p, os.W_OK)
    assert "freeze" in status.read_text() and out["body_sha256"][:16] in status.read_text()
    assert ft.verify(p, []) == []


def test_refreeze_is_refused(tmp_path):
    p = _tol(tmp_path)
    ft.freeze(p, None, "P9")
    p.chmod(stat.S_IWUSR | stat.S_IRUSR)
    with pytest.raises(SystemExit):
        ft.freeze(p, None, "P9")


def test_edited_criterion_is_red(tmp_path):
    p = _tol(tmp_path)
    ft.freeze(p, None, "P9")
    p.chmod(stat.S_IWUSR | stat.S_IRUSR)
    d = json.loads(p.read_text())
    d["pass"] = "r >= 0.5"
    p.write_text(json.dumps(d))
    p.chmod(stat.S_IRUSR)
    assert any("changed" in x for x in ft.verify(p, []))


def test_writable_file_is_red(tmp_path):
    p = _tol(tmp_path)
    ft.freeze(p, None, "P9")
    p.chmod(stat.S_IWUSR | stat.S_IRUSR)
    assert any("writable" in x for x in ft.verify(p, []))


def test_result_before_freeze_is_red(tmp_path):
    early = tmp_path / "result.json"
    early.write_text("{}")
    old = time.time() - 3600
    os.utime(early, (old, old))
    p = _tol(tmp_path)
    ft.freeze(p, None, "P9")
    assert any("before the freeze" in x for x in ft.verify(p, [early]))
