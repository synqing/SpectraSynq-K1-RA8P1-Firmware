"""Lane P6: the 40-110 Hz SubBandDetector passes its host test, and goes red with a planted wrong centre."""
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def build_and_run(tmp: Path, defines: list[str]) -> subprocess.CompletedProcess:
    exe = tmp / "test_p6_sub_band"
    subprocess.run(["c++", "-std=c++17", "-O2", "-fno-fast-math", *defines, "-I", str(ROOT / "src/k1"),
                    str(ROOT / "tests/host/test_p6_sub_band.cpp"), "-o", str(exe)], check=True)
    return subprocess.run([str(exe)], capture_output=True, text=True)


def test_sub_band_passes(tmp_path):
    r = build_and_run(tmp_path, [])
    assert r.returncode == 0, r.stdout


def test_sub_band_red_on_planted_wrong_centre(tmp_path):
    r = build_and_run(tmp_path, ["-DK1_SUBBAND_PLANT_WRONG_CENTRE=1"])
    assert r.returncode != 0, r.stdout
