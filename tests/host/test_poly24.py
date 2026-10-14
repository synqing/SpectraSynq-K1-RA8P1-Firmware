"""Lane P4: k1_poly24 (measured-rate polyphase resampler to 24 kHz) against the
requirements frozen in runs/P4-20261014T085434Z/permitted/TOLERANCE-FROZEN-poly24.json
before any output existed. Host oracle only; not M85 execution."""
import subprocess
from pathlib import Path

import numpy as np
import pytest

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "platform/ra8p1"
TRUE_HZ = 41417
AMP = 8000.0
SECONDS = 4.0
OUT_HZ = 24000
SKIP = int(0.25 * OUT_HZ)


def build(tmp, name, extra=()):
    exe = tmp / name
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-fast-math", *extra,
                    "-I" + str(P), str(ROOT / "tests/host/poly24_probe.c"), str(P / "k1_poly24.c"),
                    "-lm", "-o", str(exe)], check=True)
    return exe


def run(exe, tmp, tone, told=TRUE_HZ, chunk=296):
    out = tmp / f"o-{exe.name}-{tone}-{told}-{chunk}.s16"
    subprocess.run([str(exe), str(TRUE_HZ), str(told), str(tone), str(AMP), str(SECONDS), str(chunk), str(out)],
                   check=True, capture_output=True)
    y = np.fromfile(out, dtype="<i2").astype(np.float64)
    return y


def gain_db(y):
    seg = y[SKIP:]
    return 20 * np.log10(np.sqrt(np.mean(seg ** 2)) / (AMP / np.sqrt(2)))


def peak_hz(y):
    seg = y[SKIP:]
    w = np.hanning(len(seg))
    spec = np.abs(np.fft.rfft(seg * w, n=4 * len(seg)))
    k = int(np.argmax(spec))
    a, b, c = np.log(spec[k - 1:k + 2] + 1e-12)
    delta = 0.5 * (a - c) / (a - 2 * b + c)
    return (k + delta) * OUT_HZ / (4 * len(seg))


@pytest.fixture(scope="module")
def tools(tmp_path_factory):
    tmp = tmp_path_factory.mktemp("poly24")
    return tmp, build(tmp, "probe"), build(tmp, "probe16", ["-DK1_POLY24_TAPS=16"])


def passband_ok(exe, tmp):
    return all(abs(gain_db(run(exe, tmp, f))) <= 0.5 for f in (110, 1000, 5000, 9000))


def top_bin_ok(exe, tmp):
    return gain_db(run(exe, tmp, 10548)) >= -3.0


def alias_ok(exe, tmp):
    return all(gain_db(run(exe, tmp, f)) <= -60.0 for f in (13000, 15000, 17000, 20000))


def freq_ok(exe, tmp, told=TRUE_HZ):
    return abs(peak_hz(run(exe, tmp, 1000, told)) - 1000.0) <= 0.0005 * 1000.0


def test_passband_flat(tools):
    tmp, exe, _ = tools
    gains = {f: round(gain_db(run(exe, tmp, f)), 3) for f in (110, 1000, 5000, 9000)}
    assert all(abs(g) <= 0.5 for g in gains.values()), gains


def test_top_gdft_bin_usable(tools):
    tmp, exe, _ = tools
    g = gain_db(run(exe, tmp, 10548))
    assert g >= -3.0, g


def test_alias_rejection(tools):
    tmp, exe, _ = tools
    gains = {f: round(gain_db(run(exe, tmp, f)), 1) for f in (13000, 15000, 17000, 20000)}
    assert all(g <= -60.0 for g in gains.values()), gains


def test_rate_correct(tools):
    tmp, exe, _ = tools
    assert freq_ok(exe, tmp), peak_hz(run(exe, tmp, 1000))


def test_chunk_invariance(tools):
    tmp, exe, _ = tools
    a, b, c = (run(exe, tmp, 3000, chunk=k) for k in (296, 64, 1))
    n = min(len(a), len(b), len(c))
    assert n > OUT_HZ and np.array_equal(a[:n], b[:n]) and np.array_equal(a[:n], c[:n])


def test_planted_wrong_rate_fails(tools):
    tmp, exe, _ = tools
    assert not freq_ok(exe, tmp, told=40000)


def test_planted_short_filter_fails(tools):
    tmp, _, short = tools
    assert not (alias_ok(short, tmp) and top_bin_ok(short, tmp))


def _clock(tmp, extra=()):
    exe = tmp / ("clock" + "".join(e.replace("=", "_").replace("-", "") for e in extra))
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-fast-math", *extra,
                    "-I" + str(P), str(ROOT / "tests/host/poly24_clock_probe.c"), str(P / "k1_poly24.c"),
                    "-lm", "-o", str(exe)], check=True)
    return subprocess.run([str(exe)], capture_output=True, text=True)


def test_stream_clock_names_the_source_sample(tmp_path):
    r = _clock(tmp_path)
    assert r.returncode == 0, r.stdout + r.stderr


def test_stream_clock_red_when_delay_constant_is_wrong(tmp_path):
    # Planted fault: the declared delay disagrees with the pre-roll by one sample.
    src = (P / "k1_poly24.h").read_text().replace(
        "#define K1_POLY24_ALIGN_ZEROS (2u * K1_POLY24_DELAY_SOURCE_SAMPLES)",
        "#define K1_POLY24_ALIGN_ZEROS (2u * K1_POLY24_DELAY_SOURCE_SAMPLES + 1u)")
    d = tmp_path / "planted"
    d.mkdir()
    (d / "k1_poly24.h").write_text(src)
    # Both translation units must see the planted header (quoted includes
    # resolve beside the source file first), so the source is copied too.
    (d / "k1_poly24.c").write_text((P / "k1_poly24.c").read_text())
    exe = tmp_path / "clock_planted"
    subprocess.run(["cc", "-std=c11", "-O2", "-fno-fast-math", "-I" + str(d),
                    str(ROOT / "tests/host/poly24_clock_probe.c"), str(d / "k1_poly24.c"), "-lm", "-o", str(exe)],
                   check=True)
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    assert r.returncode == 1, r.stdout
