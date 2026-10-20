"""Lane P4 corrections (runs/P4-20261014T085434Z/permitted/TOLERANCE-FROZEN-corrections.json,
frozen 2026-10-14T12:11:59Z before any result):
  H1  offline-window design == previous on-device design (<= 2e-6) at 36000/40000/41417/44000 Hz;
  H4  capture-lag arithmetic exact across 2^32/2^33/2^40 source indices; a planted
      non-modular copy of the product function goes red;
  H5  the generated window table matches the Kaiser formula; a planted edit goes red.
Host oracle only; not M85 execution."""
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "platform/ra8p1"
H = ROOT / "tests/host"
sys.path.insert(0, str(ROOT / "scripts"))
import gen_poly24_window  # noqa: E402

ZEROS = 94  # K1_POLY24_ALIGN_ZEROS for 96 taps


def build(tmp, name, poly_c=P / "k1_poly24.c", include=P):
    exe = tmp / name
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-fast-math",
                    "-I" + str(include), "-I" + str(P), str(H / "poly24_corrections_probe.c"),
                    str(H / "poly24_design_reference.c"), str(poly_c), "-lm", "-o", str(exe)], check=True)
    return exe


def call(exe, *args):
    return json.loads(subprocess.run([str(exe), *map(str, args)], check=True, capture_output=True,
                                     text=True).stdout)


@pytest.fixture(scope="module")
def probe(tmp_path_factory):
    return build(tmp_path_factory.mktemp("p4c"), "probe")


@pytest.fixture(scope="module")
def planted_nonmodular(tmp_path_factory):
    """The product file with the F9 fix reverted to the pre-fix comparison."""
    tmp = tmp_path_factory.mktemp("p4c-plant")
    src = (P / "k1_poly24.c").read_text()
    body = ("    const uint64_t content_abs = (stream_base_index << 32) + content_q32;\n"
            "    const uint64_t newest_q32 = newest_source_index << 32;\n"
            "    if (newest_q32 < content_abs) {\n        return 0;\n    }\n"
            "    const uint64_t lag = newest_q32 - content_abs;\n")
    planted, n = re.subn(r"    const uint64_t newest_q32 = newest_source_index << 32;\n"
                         r"    const uint64_t lag = .*?;\n    if \(\(int64_t\)lag < 0\) \{\n"
                         r"        return 0;\n    \}\n", body, src, flags=re.S)
    assert n == 1, "plant did not apply: the guarded code changed shape"
    (tmp / "k1_poly24.c").write_text(planted)
    shutil.copy2(P / "k1_poly24_window.h", tmp / "k1_poly24_window.h")
    return build(tmp, "planted", poly_c=tmp / "k1_poly24.c", include=tmp)


@pytest.mark.parametrize("hz", [36000, 40000, 41417, 44000])
def test_h1_design_matches_previous_formula(probe, hz):
    r = call(probe, "design", hz)
    assert r["step_equal"] == 1
    assert r["max_abs_diff"] <= 2e-6, r


def exact_lag(newest, base, content_q32):
    return (newest << 32) - (base << 32) - content_q32


LAG_CASES = [
    # (newest, base, content_q32): content a few hundred samples behind newest
    (1_000, 0, (700 << 32) + 0x80000000),
    ((1 << 32) + 5, 0, ((1 << 32) - 300) << 32),           # newest past 2^32, content before it
    ((1 << 32) + 400, 10, ((1 << 32) + 100) << 32),        # both past 2^32
    ((1 << 33) + 7, (1 << 32), ((1 << 32) - 200) << 32 & ((1 << 64) - 1)),
    ((1 << 40) + 12345, (1 << 40), (12000 << 32) + 0x1234),
]


@pytest.mark.parametrize("newest,base,content", LAG_CASES)
def test_h4_lag_exact_across_wrap(probe, newest, base, content):
    true = exact_lag(newest, base, content)
    assert true >= 0
    r = call(probe, "lag", newest, base, content & ((1 << 64) - 1))
    assert r["ok"] == 1 and r["lag_q32"] == true, (r, true)


def test_h4_content_after_newest_rejected(probe):
    newest, base = (1 << 32) + 50, 0
    content = (((1 << 32) + 60) << 32) & ((1 << 64) - 1)  # 10 samples AFTER newest, wrapped
    assert call(probe, "lag", newest, base, content)["ok"] == 0


@pytest.mark.parametrize("consumed", [ZEROS + 1000, (1 << 32) - 50, (1 << 32) + 1000, (1 << 41) + 3])
def test_h4_long_stream_chain(probe, consumed):
    base = 777
    r = call(probe, "chain", consumed, base, 0)  # newest=0 only to read pos/step
    content_true = (consumed << 32) + r["pos_q32"] - r["step_q32"] - (ZEROS << 32)
    newest = base + (content_true >> 32) + 3
    r = call(probe, "chain", consumed, base, newest)
    assert r["ok"] == 1 and r["lag_q32"] == exact_lag(newest, base, content_true), r


def test_h4_planted_nonmodular_goes_red(planted_nonmodular):
    newest, base, content = LAG_CASES[1]
    r = call(planted_nonmodular, "lag", newest, base, content & ((1 << 64) - 1))
    assert not (r["ok"] == 1 and r["lag_q32"] == exact_lag(newest, base, content)), \
        "planted pre-fix arithmetic still passed: the long-index test cannot fail"


def test_h5_window_table_matches_formula():
    assert gen_poly24_window.check() == []


def test_h5_planted_window_edit_goes_red(tmp_path):
    text = (P / "k1_poly24_window.h").read_text()
    first = re.search(r"\{\{?\s*([0-9.e+-]+)f", text.split("= {", 1)[1])
    value = first.group(1)
    planted = text.replace(value + "f", f"{float(value) + 1e-4:.9e}f", 1)
    assert planted != text
    (tmp_path / "w.h").write_text(planted)
    assert gen_poly24_window.check(tmp_path / "w.h"), "planted window edit was not detected"
