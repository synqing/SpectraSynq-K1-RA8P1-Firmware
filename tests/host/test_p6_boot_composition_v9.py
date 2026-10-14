"""Lane P6: boot composition profile 9 selects the v9 effect mapping; profile 8 stays v8.

The v8 guard preprocesses inhouse_waveform.cpp without K1_INHOUSE_MAPPING_V9 and compares it
with the same file at 52c1c21 (the last commit before P6): any edit to the v8 path goes red.
"""
import copy
import json
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from live_boot_composition import profile_defines, render_composition, validate_composition  # noqa: E402

SCHEMA = ROOT / "docs/contracts/titan-emotiscope-v1.json"
PRESETS = ROOT / "docs/emotiscope-port-001/presets"
V8 = PRESETS / "UNSELECTED-current-hybrid-tempo-crimson-v8-20261014.json"
V9 = PRESETS / "UNSELECTED-current-hybrid-tempo-crimson-v9-20261014.json"
BASE = "52c1c21"


def validate(defaults):
    raw = SCHEMA.read_bytes()
    return validate_composition(defaults, json.loads(raw), raw)


def test_v8_profile_adds_no_define():
    c = validate(json.loads(V8.read_text()))
    assert c["profile_version"] == 8 and profile_defines(c) == []


def test_v9_profile_selects_mapping_and_same_setters():
    c8, c9 = validate(json.loads(V8.read_text())), validate(json.loads(V9.read_text()))
    assert profile_defines(c9) == ["-DK1_INHOUSE_MAPPING_V9=1"]
    assert render_composition(c8) == render_composition(c9)
    a, b = copy.deepcopy(c8), copy.deepcopy(c9)
    b["profile_version"] = 8
    assert a == b  # the presets differ only in the profile number


@pytest.mark.parametrize("version", [7, 10, "9", 9.0])
def test_other_profiles_refused(version):
    d = json.loads(V9.read_text())
    d["composition"]["profile_version"] = version
    with pytest.raises(RuntimeError):
        validate(d)


def git_show(path: str) -> str:
    return subprocess.run(["git", "show", f"{BASE}:{path}"], cwd=ROOT, capture_output=True, text=True, check=True).stdout


def v8_unit(cpp_text: str, header_text: str, tmp: Path) -> str:
    """The renderer translation unit as the v8 build sees it (no K1_INHOUSE_MAPPING_V9)."""
    inc = tmp / "inc" / "core/visual"
    inc.mkdir(parents=True, exist_ok=True)
    (inc / "inhouse_waveform.h").write_text(header_text)
    src = tmp / "tu.cpp"
    src.write_text(cpp_text)
    return subprocess.run(["c++", "-std=c++17", "-E", "-P", "-I", str(tmp / "inc"), "-I", str(ROOT / "src/k1"),
                           str(src)], capture_output=True, text=True, check=True).stdout


def guard(cpp_text: str, header_text: str, tmp: Path) -> bool:
    base = v8_unit(git_show("src/k1/core/visual/inhouse_waveform.cpp"),
                   git_show("src/k1/core/visual/inhouse_waveform.h"), tmp / "base")
    return base == v8_unit(cpp_text, header_text, tmp / "candidate")


def test_v8_renderer_translation_unit_unchanged(tmp_path):
    cpp = (ROOT / "src/k1/core/visual/inhouse_waveform.cpp").read_text()
    h = (ROOT / "src/k1/core/visual/inhouse_waveform.h").read_text()
    assert guard(cpp, h, tmp_path)


def test_v8_guard_goes_red_on_planted_edits(tmp_path):
    cpp = (ROOT / "src/k1/core/visual/inhouse_waveform.cpp").read_text()
    h = (ROOT / "src/k1/core/visual/inhouse_waveform.h").read_text()
    planted_cpp = cpp.replace("0.8F + 3.5F*peak", "0.8F + 3.6F*peak")
    planted_h = h.replace("  float decay_rate = 0.0F;\n", "  float decay_rate = 0.0F;\n  float planted = 0.0F;\n")
    assert planted_cpp != cpp and planted_h != h
    assert not guard(planted_cpp, h, tmp_path / "a")
    assert not guard(cpp, planted_h, tmp_path / "b")


def audio_unit(cpp_text: str, header_text: str, tmp: Path) -> str:
    """inhouse_audio.cpp as the v8 build sees it (no K1_INHOUSE_MAPPING_V9); the v9-only
    sub-band detector (P6 EXT trigger) must vanish from it."""
    inc = tmp / "inc"
    inc.mkdir(parents=True, exist_ok=True)
    (inc / "inhouse_audio.h").write_text(header_text)
    src = tmp / "audio.cpp"
    src.write_text(cpp_text)
    return subprocess.run(["c++", "-std=c++17", "-E", "-P", "-DK1_EMOTISCOPE_PORT=1", "-I", str(inc),
                           "-I", str(ROOT / "platform/ra8p1"), "-I", str(ROOT / "src/k1"), "-I", str(ROOT / "src"),
                           str(src)], capture_output=True, text=True, check=True).stdout


def test_v8_inhouse_audio_translation_unit_unchanged(tmp_path):
    base = audio_unit(git_show("platform/ra8p1/inhouse_audio.cpp"), git_show("platform/ra8p1/inhouse_audio.h"),
                      tmp_path / "base")
    now = audio_unit((ROOT / "platform/ra8p1/inhouse_audio.cpp").read_text(),
                     (ROOT / "platform/ra8p1/inhouse_audio.h").read_text(), tmp_path / "now")
    assert base == now
    planted = (ROOT / "platform/ra8p1/inhouse_audio.cpp").read_text().replace(
        "  publication_.raw_rms = rms;\n", "  publication_.raw_rms = rms * 1.0001F;\n")
    assert planted != (ROOT / "platform/ra8p1/inhouse_audio.cpp").read_text()
    assert audio_unit(planted, (ROOT / "platform/ra8p1/inhouse_audio.h").read_text(), tmp_path / "plant") != base
