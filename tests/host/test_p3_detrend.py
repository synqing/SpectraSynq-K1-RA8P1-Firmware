"""P3 iteration 3: the Q3 detrend must not create edge spikes (review N1).

A camera-like trace (a steady level of 0.5 with a small wobble) is detrended. The
first and last half-second must stay within the trace's own scale. The zero-padded
form used before iteration 3 is checked the same way and must FAIL, so this test is
shown to go red on the defect it guards.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import p3_q3_alignment as q3  # noqa: E402

FS = 30.0


def zero_padded(x, fs):
    k = max(1, int(fs))
    y = x - np.convolve(x, np.ones(k) / k, mode="same")
    return y - y.mean()


def trace():
    rng = np.random.default_rng(3)
    return 0.5 + 0.013 * rng.standard_normal(600)


def edges_ok(y):
    k = int(FS)
    mid = y[k:-k].std()
    return np.abs(np.r_[y[:k // 2], y[-(k // 2):]]).max() < 5.0 * mid


def test_detrend_keeps_length_and_edges():
    y = q3.detrend(trace(), FS)
    assert len(y) == 600
    assert edges_ok(y)


def test_zero_padded_form_is_red():
    assert not edges_ok(zero_padded(trace(), FS))


def test_detrend_removes_slow_drift():
    t = np.arange(600) / FS
    y = q3.detrend(0.5 + 0.2 * t / t[-1] + 0.01 * np.sin(2 * np.pi * 2.3 * t), FS)
    assert np.abs(y).max() < 0.03
