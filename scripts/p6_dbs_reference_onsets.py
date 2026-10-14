#!/usr/bin/env python3
"""P6 DBS pilot: source-bound onset reference for BOOM 0-75 s and Sonic 6-30 s.

Host-only analysis. Reads audio, writes JSON/PNG under docs/evidence/P6/dbs/reference/.
Touches no device, firmware or source file.

Pinned run (system python lacks pandas; nothing here needs it):
    uv run --quiet --with numpy==2.1.3 --with scipy==1.14.1 --with matplotlib==3.9.2 \
        python scripts/p6_dbs_reference_onsets.py

Run scripts/p6_dbs_probes.py first: the negative_control_probes block reads its files.

Method: SuperFlux-style onset detection (Boeck & Widmer, DAFx 2013) with every
parameter declared in PARAMS and copied into the output JSON. The single free
constant, the threshold offset delta, is chosen ONCE on BOOM 0-28 s planted-event
development set A (never on the 28-75 s scored window) by DELTA_RULE (or, when no
grid value satisfies it, DELTA_FALLBACK), then frozen and checked on an
independent BOOM plant set B and on Sonic plants.
"""
from __future__ import annotations

import hashlib
import json
import wave
from pathlib import Path

import numpy as np
from scipy.ndimage import maximum_filter1d, uniform_filter1d
from scipy.signal import resample_poly

W = Path(__file__).resolve().parents[1]
OUT = W / "docs/evidence/P6/dbs/reference"

BOOM_WAV = W / "_scratch/waveform-migration-001/audio-boom-001/boom-0-75s-pcm16.wav"
BOOM_SHA = "09c9a02f439cbeb66f006cf5a973ae7988b5ef2e9ecdcf470c140d6918e63652"
REPLAY_S16 = Path("/tmp/p6-product-path-p-inputs/boom_source.s16")
REPLAY_SHA = "45edfd0948c8cc24f99bcde0a8f2d8b4c8fa70413d648c6a6e1cfaa3a4bfbd15"
REPLAY_FS = 40000
SONIC_WAV = W / "docs/emotiscope-port-001/evidence/audio-windows-001/sonic-samba-6-30s.wav"
SONIC_SHA = "8eb9b859dc5c073dc94dda49cc4556420d3bf9978762249f1d51fbb2efaeeb44"
# Full retained Sonic source used by docs/evidence/P6/drop-cue; read only to check the 6 s window origin.
SONIC_FULL_WAV = (W.parent / "titan-music-intelligence-001/tests/fixtures/tempo_field/v1/trance/sonic-samba.wav")
DROP_CUE_REF = W / "docs/evidence/P6/drop-cue/reference-onsets.json"

PARAMS = {
    "detector": "superflux-style log-filterbank spectral flux with frequency max-filtered reference",
    "downmix": "mono = mean of channels, float scale [-1, 1) (int16 / 32768)",
    "window": "Hann (scipy-style periodic), unnormalised",
    "win_s": 1024 / 44100,          # 1024 samples at 44.1 kHz; scaled per rate (rounded)
    "hop_target_s": 0.002,          # hop = round(0.002 * fs) samples; actual hop recorded per input
    "nfft_rule": "next power of two >= 4 * window samples (zero padding; interpolates bins, adds no resolution)",
    "frame_time_rule": "signal front-padded by win/2 zeros, so frame k is centred on sample k*hop",
    "filterbank": "triangular, log-spaced 24 bands/octave, area-normalised, duplicate FFT bins merged (madmom unique_filters)",
    "bands_per_octave": 24,
    "fmin_hz": 30.0,
    "fmax_rule_hz": "min(16000, 0.45 * fs)",
    "log_compression": "log(1 + lambda * |X|) on filterbank magnitude",
    "lambda": 1.0,
    "freq_maxfilter_bands": 3,
    "diff_lag_s": 0.010,            # mu = round(0.010 / hop_s) frames = 5
    "odf": "sum over bands of max(0, L[k] - maxfilt3(L[k - mu]))",
    "threshold": "moving mean of ODF over +-thr_avg_s, plus delta (absolute ODF units)",
    "thr_avg_s": 0.050,
    "peak_max_s": 0.030,            # ODF[k] must equal the max over +-30 ms
    "min_sep_s": 0.030,             # earlier peak kept when two are closer
    "delta": None,                  # frozen by choose_delta() on BOOM 0-28 s set A
    "band_groups_hz": {"sub": [30, 110], "low": [110, 250], "mid": [250, 2000], "high": [2000, 8000]},
    "band_group_rule": "filterbank band assigned by centre frequency; high group capped at fmax; ODF from bands above 8 kHz reported as other_gt8k",
    "context_rule": "dense if median IOI over onsets within +-0.5 s is <= 0.160 s (needs >= 1 IOI), else sparse",
    "over_sustain_rule": "full-band mean-square dB of [t-200 ms, t) >= that of [t, t+200 ms) - 6 dB",
    "prominence_rule": "ODF[k] - min(ODF over the preceding 60 ms)",
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_wav(path: Path) -> tuple[np.ndarray, int, int, int]:
    with wave.open(str(path)) as w:
        fs, ch, n, sw = w.getframerate(), w.getnchannels(), w.getnframes(), w.getsampwidth()
        assert sw == 2, f"{path} is not PCM16"
        raw = np.frombuffer(w.readframes(n), dtype="<i2").reshape(-1, ch)
    return raw.astype(np.float64).mean(axis=1) / 32768.0, fs, ch, n


def load_s16(path: Path) -> np.ndarray:
    return np.fromfile(path, dtype="<i2").astype(np.float64) / 32768.0


class Detector:
    """Rate-specific realisation of PARAMS. compute() is deterministic."""

    def __init__(self, fs: int):
        self.fs = fs
        self.win = int(round(PARAMS["win_s"] * fs))
        self.hop = int(round(PARAMS["hop_target_s"] * fs))
        self.hop_s = self.hop / fs
        self.mu = int(round(PARAMS["diff_lag_s"] / self.hop_s))
        self.nfft = 1 << int(np.ceil(np.log2(4 * self.win)))
        self.fmax = min(16000.0, 0.45 * fs)
        self.window = np.hanning(self.win + 1)[:-1]
        self._filterbank()

    def _filterbank(self) -> None:
        df = self.fs / self.nfft
        n_oct = np.log2(self.fmax / PARAMS["fmin_hz"])
        f = PARAMS["fmin_hz"] * 2.0 ** (np.arange(int(np.floor(n_oct * 24)) + 1) / 24)
        bins = np.unique(np.round(f / df).astype(int))
        bins = bins[bins >= 1]
        nb = self.nfft // 2 + 1
        fb, centres = [], []
        for i in range(1, len(bins) - 1):
            lo, c, hi = bins[i - 1], bins[i], bins[i + 1]
            tri = np.zeros(nb)
            tri[lo:c + 1] = np.linspace(0, 1, c - lo + 1)
            tri[c:hi + 1] = np.linspace(1, 0, hi - c + 1)
            tri /= tri.sum()
            fb.append(tri)
            centres.append(c * df)
        self.fb = np.array(fb).T  # (bins, bands)
        self.centres = np.array(centres)
        self.groups = {}
        for g, (lo, hi) in PARAMS["band_groups_hz"].items():
            self.groups[g] = (self.centres >= lo) & (self.centres < min(hi, self.fmax + 1))
        self.groups["other_gt8k"] = self.centres >= 8000

    def realised(self) -> dict:
        return {"fs": self.fs, "win_samples": self.win, "win_ms": 1000 * self.win / self.fs,
                "hop_samples": self.hop, "hop_ms": 1000 * self.hop_s, "mu_frames": self.mu,
                "lag_ms": 1000 * self.mu * self.hop_s, "nfft": self.nfft, "fmax_hz": self.fmax,
                "n_bands": int(self.fb.shape[1]),
                "band_count_per_group": {g: int(m.sum()) for g, m in self.groups.items()}}

    def compute(self, x: np.ndarray) -> dict:
        pad = np.concatenate([np.zeros(self.win // 2), x, np.zeros(self.win)])
        n_frames = (len(x) + self.hop - 1) // self.hop
        L = np.empty((n_frames, self.fb.shape[1]), dtype=np.float32)
        idx = np.arange(self.win)
        for k0 in range(0, n_frames, 2048):
            ks = np.arange(k0, min(n_frames, k0 + 2048))
            frames = pad[ks[:, None] * self.hop + idx[None, :]] * self.window
            mag = np.abs(np.fft.rfft(frames, n=self.nfft, axis=1))
            # numpy 2.x + Accelerate can raise spurious matmul FP warnings; finiteness is asserted below.
            with np.errstate(all="ignore"):
                L[ks] = np.log1p(PARAMS["lambda"] * (mag @ self.fb))
        assert np.isfinite(L).all(), "non-finite filterbank output"
        ref = maximum_filter1d(L, size=PARAMS["freq_maxfilter_bands"], axis=1)
        D = np.zeros_like(L)
        D[self.mu:] = np.maximum(0.0, L[self.mu:] - ref[:-self.mu])
        odf = D.sum(axis=1).astype(np.float64)
        t = np.arange(n_frames) * self.hop_s
        return {"t": t, "odf": odf, "D": D}

    def pick(self, odf: np.ndarray, delta: float) -> np.ndarray:
        avg_n = 2 * int(round(PARAMS["thr_avg_s"] / self.hop_s)) + 1
        max_n = 2 * int(round(PARAMS["peak_max_s"] / self.hop_s)) + 1
        thr = uniform_filter1d(odf, size=avg_n, mode="constant") + delta
        mx = maximum_filter1d(odf, size=max_n, mode="constant")
        cand = np.where((odf == mx) & (odf >= thr) & (odf > 0))[0]
        sep = int(round(PARAMS["min_sep_s"] / self.hop_s))
        keep: list[int] = []
        for k in cand:
            if not keep or k - keep[-1] >= sep:
                keep.append(int(k))
        return np.array(keep, dtype=int)


# --------------------------------------------------------------------------- timing refinement
REFINE = {
    "method": "R1 short-window envelope rise",
    "signal": "first difference of the mono signal (transient emphasis)",
    "envelope_win_s": 0.001,       # trailing mean-square window w
    "search_s": [-0.020, 0.015],   # relative to the ODF frame time
    "estimate": "argmax_n (E[n] - E[n - w]) in dB, minus w (the rise is maximal when the trailing window first lies wholly after the attack)",
    "accept_min_rise_db": 6.0,
    "accept_max_shift_s": 0.015,
    "fallback": "ODF frame time, flagged refined=false",
}


def refine_times(x: np.ndarray, fs: int, t_frame: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    y = np.diff(x, prepend=x[0])
    w = max(1, int(round(REFINE["envelope_win_s"] * fs)))
    c = np.concatenate([[0.0], np.cumsum(y * y)])
    out, ok = t_frame.copy(), np.zeros(len(t_frame), dtype=bool)
    lo_s, hi_s = REFINE["search_s"]
    for i, tc in enumerate(t_frame):
        a = int(round((tc + lo_s) * fs))
        b = int(round((tc + hi_s) * fs))
        a, b = max(a, 2 * w), min(b, len(x))
        if b - a < 3:
            continue
        n = np.arange(a, b + 1)
        e_now = (c[n] - c[n - w]) / w
        e_prev = (c[n - w] - c[n - 2 * w]) / w
        rise = 10 * np.log10((e_now + 1e-20) / (e_prev + 1e-20))
        j = int(np.argmax(rise))
        t_est = (n[j] - w) / fs
        if rise[j] >= REFINE["accept_min_rise_db"] and abs(t_est - tc) <= REFINE["accept_max_shift_s"]:
            out[i], ok[i] = t_est, True
    return out, ok


# --------------------------------------------------------------------------- planted-event validation
PLANT = {
    "types": {
        "burst": "10 ms white noise x exp(-t / 3 ms), instantaneous start",
        "kick": "5 ms sine glide 150 -> 60 Hz (exponential), phase 0 at start, x exp(-t / 2.5 ms)",
    },
    "levels_db": [-6, -12, -18, -24, -30],
    "level_rule": "plant RMS over its own duration relative to host mono RMS over [t-200 ms, t+200 ms)",
    "per_set": 50,
    "per_level": 10,
    "type_rule": "alternating burst/kick within each level",
    "min_spacing_s": 0.300,
    "host_exclusion_s": 0.040,
    "host_exclusion_rule": "no host-only detection within +-40 ms, host detections taken at the smallest delta in the grid (largest host set)",
    "match_tolerance_s": 0.025,
    "match_rule": "one-to-one, nearest detection within +-25 ms, error = detected - planted onset sample time",
    "seeds": {"boom_A_dev": 20261015, "boom_B_holdout": 20261016, "sonic": 20261017},
}
DELTA_GRID = [0.5, 1, 1.5, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40]
DELTA_RULE = ("largest delta in DELTA_GRID for which BOOM 0-28 s set A gives recall >= 0.90 over plants "
              ">= -18 dB AND p95 |error| <= 6 ms over those matched plants (raw ODF frame time); "
              "chosen once, before any 28-75 s analysis, then frozen")
DELTA_FALLBACK = ("declared after the first sweep showed no grid value passes: take the largest delta whose "
                  "set A recall(>= -18 dB) equals the grid maximum (most conservative delta at the best "
                  "achievable recall). The recall requirement is then reported as FAILED, not relaxed.")


def plant_wave(kind: str, fs: int, rng: np.random.Generator) -> np.ndarray:
    if kind == "burst":
        n = int(round(0.010 * fs))
        return rng.standard_normal(n) * np.exp(-np.arange(n) / (0.003 * fs))
    n = int(round(0.005 * fs))
    t = np.arange(n) / fs
    f = 150.0 * (60.0 / 150.0) ** (t / 0.005)
    return np.sin(2 * np.pi * np.cumsum(f) / fs - 2 * np.pi * f[0] / fs) * np.exp(-t / 0.0025)


def make_plants(host: np.ndarray, fs: int, span: tuple[float, float], host_det_t: np.ndarray,
                seed: int) -> list[dict]:
    rng = np.random.default_rng(seed)
    cand = rng.permutation(np.arange(int(span[0] * 1000), int(span[1] * 1000))) / 1000.0
    pos: list[float] = []
    for t in cand:
        if len(pos) >= PLANT["per_set"]:
            break
        if host_det_t.size and np.min(np.abs(host_det_t - t)) < PLANT["host_exclusion_s"]:
            continue
        if pos and np.min(np.abs(np.array(pos) - t)) < PLANT["min_spacing_s"]:
            continue
        pos.append(float(t))
    pos.sort()
    order = rng.permutation(len(pos))
    plants = []
    for rank, i in enumerate(order):
        lvl = PLANT["levels_db"][rank % len(PLANT["levels_db"])]
        kind = "burst" if (rank // len(PLANT["levels_db"])) % 2 == 0 else "kick"
        s = int(round(pos[i] * fs))
        w = plant_wave(kind, fs, rng)
        h = host[max(0, s - int(0.2 * fs)):s + int(0.2 * fs)]
        loc = float(np.sqrt(np.mean(h * h)))
        w = w * loc * 10 ** (lvl / 20) / np.sqrt(np.mean(w * w))
        plants.append({"sample": s, "t": s / fs, "type": kind, "level_db": lvl,
                       "host_local_rms_dbfs": 20 * np.log10(loc + 1e-12), "wave": w})
    plants.sort(key=lambda p: p["sample"])
    return plants


def apply_plants(host: np.ndarray, plants: list[dict]) -> np.ndarray:
    y = host.copy()
    for p in plants:
        y[p["sample"]:p["sample"] + len(p["wave"])] += p["wave"]
    return y


def match(plant_t: np.ndarray, det_t: np.ndarray, tol: float) -> np.ndarray:
    """Return signed error per plant (nan if unmatched); one-to-one, nearest first."""
    err = np.full(len(plant_t), np.nan)
    if det_t.size == 0:
        return err
    pairs = sorted((abs(d - p), i, j) for i, p in enumerate(plant_t) for j, d in enumerate(det_t)
                   if abs(d - p) <= tol)
    used_p, used_d = set(), set()
    for _, i, j in pairs:
        if i in used_p or j in used_d:
            continue
        used_p.add(i)
        used_d.add(j)
        err[i] = det_t[j] - plant_t[i]
    return err


def score(plants: list[dict], err: np.ndarray) -> dict:
    lv = np.array([p["level_db"] for p in plants])
    out = {"n": len(plants), "by_level": {}}
    for l in PLANT["levels_db"]:
        m = lv == l
        out["by_level"][str(l)] = {"n": int(m.sum()), "matched": int(np.isfinite(err[m]).sum()),
                                   "recall": float(np.isfinite(err[m]).mean()) if m.any() else None}
    for name, m in (("ge_minus18", lv >= -18), ("all", np.ones_like(lv, dtype=bool))):
        e = err[m & np.isfinite(err)] * 1000
        out[name] = {"n": int(m.sum()), "matched": int(e.size), "recall": float(e.size / max(1, m.sum())),
                     "err_ms_median": float(np.median(e)) if e.size else None,
                     "abs_err_ms_p95": float(np.percentile(np.abs(e), 95)) if e.size else None,
                     "err_ms_p05_p95": [float(np.percentile(e, 5)), float(np.percentile(e, 95))] if e.size else None}
    for kind in ("burst", "kick"):
        m = (lv >= -18) & np.array([p["type"] == kind for p in plants])
        e = err[m & np.isfinite(err)] * 1000
        out[f"ge_minus18_{kind}"] = {"n": int(m.sum()), "recall": float(e.size / max(1, m.sum())),
                                     "abs_err_ms_p95": float(np.percentile(np.abs(e), 95)) if e.size else None}
    return out


def plant_rows(plants: list[dict], err_raw: np.ndarray, err_ref: np.ndarray) -> list[dict]:
    return [{"sample": p["sample"], "t_s": p["t"], "type": p["type"], "level_db": p["level_db"],
             "host_local_rms_dbfs": round(p["host_local_rms_dbfs"], 2),
             "err_raw_ms": None if np.isnan(a) else round(1000 * a, 3),
             "err_refined_ms": None if np.isnan(b) else round(1000 * b, 3)}
            for p, a, b in zip(plants, err_raw, err_ref)]


def run_plant_set(det: Detector, host: np.ndarray, span, host_det_t, seed, delta, refine: bool):
    plants = make_plants(host, det.fs, span, host_det_t, seed)
    y = apply_plants(host, plants)
    r = det.compute(y)
    k = det.pick(r["odf"], delta)
    t_raw = r["t"][k]
    t_ref, _ = refine_times(y, det.fs, t_raw)
    pt = np.array([p["t"] for p in plants])
    e_raw = match(pt, t_raw, PLANT["match_tolerance_s"])
    e_ref = match(pt, t_ref, PLANT["match_tolerance_s"])
    return plants, y, e_raw, e_ref


# --------------------------------------------------------------------------- onset description
def describe(x: np.ndarray, fs: int, det: Detector, r: dict, k: np.ndarray, delta: float,
             scored_span: tuple[float, float]) -> list[dict]:
    t_frame = r["t"][k]
    t_ref, ok = refine_times(x, fs, t_frame)
    odf = r["odf"]
    avg_n = 2 * int(round(PARAMS["thr_avg_s"] / det.hop_s)) + 1
    thr = uniform_filter1d(odf, size=avg_n, mode="constant") + delta
    pre_n = int(round(0.060 / det.hop_s))
    c2 = np.concatenate([[0.0], np.cumsum(x * x)])

    def ms_db(a: float, b: float) -> float:
        i, j = max(0, int(round(a * fs))), min(len(x), int(round(b * fs)))
        return float(10 * np.log10((c2[j] - c2[i]) / max(1, j - i) + 1e-20))

    rows = []
    for n, (kk, tf, tr, okk) in enumerate(zip(k, t_frame, t_ref, ok)):
        flux = {g: float(r["D"][kk][m].sum()) for g, m in det.groups.items()}
        tot = float(r["D"][kk].sum())
        pre, post = ms_db(tr - 0.2, tr), ms_db(tr, tr + 0.2)
        rows.append({
            "i": n, "time_s": round(float(tr), 5), "time_frame_s": round(float(tf), 5),
            "refined": bool(okk), "strength": round(float(odf[kk]), 3),
            "threshold": round(float(thr[kk]), 3),
            # largest delta at which this peak still clears the threshold (min-separation interactions aside)
            "picked_up_to_delta": round(float(odf[kk] - (thr[kk] - delta)), 3),
            "prominence": round(float(odf[kk] - odf[max(0, kk - pre_n):kk + 1].min()), 3),
            "band_flux": {g: round(v, 3) for g, v in flux.items()},
            "band_fraction": {g: round(v / tot, 4) if tot > 0 else 0.0 for g, v in flux.items()},
            "pre200_db": round(pre, 2), "post200_db": round(post, 2),
            "over_sustain": bool(pre >= post - 6.0),
            "scored": bool(scored_span[0] <= tr < scored_span[1]),
        })
    times = np.array([r_["time_s"] for r_ in rows])
    for n, row in enumerate(rows):
        row["ioi_s"] = None if n == 0 else round(float(times[n] - times[n - 1]), 5)
        m = np.where(np.abs(times - times[n]) <= 0.5)[0]
        iois = np.diff(times[m])
        med = float(np.median(iois)) if iois.size else None
        row["median_ioi_pm0p5_s"] = None if med is None else round(med, 5)
        row["context"] = "dense" if (med is not None and med <= 0.160) else "sparse"
    return rows


def dense_mask(times: np.ndarray, grid: np.ndarray) -> np.ndarray:
    out = np.zeros(len(grid), dtype=bool)
    for i, g in enumerate(grid):
        m = times[np.abs(times - g) <= 0.5]
        if m.size >= 2 and np.median(np.diff(m)) <= 0.160:
            out[i] = True
    return out


# --------------------------------------------------------------------------- alignment
def bin_energy_db(x: np.ndarray, fs: int, dt: float, dur: float) -> np.ndarray:
    """Mean-square per exact dt-second bin on the true time axis (rate independent)."""
    nb = int(dur / dt)
    edges = np.round(np.arange(nb + 1) * dt * fs).astype(int)
    c = np.concatenate([[0.0], np.cumsum(x[:edges[-1]] ** 2)])
    e = (c[edges[1:]] - c[edges[:-1]]) / np.diff(edges)
    return 10 * np.log10(e + 1e-12)


def xcorr_lag(a: np.ndarray, b: np.ndarray, max_lag: int) -> tuple[float, float]:
    """Lag (in bins, sub-bin by parabola) that best aligns b to a; positive = b later."""
    a = (a - a.mean()) / a.std()
    b = (b - b.mean()) / b.std()
    lags = np.arange(-max_lag, max_lag + 1)
    cc = np.array([np.mean(a[max(0, -l):len(a) - max(0, l)] * b[max(0, l):len(b) - max(0, -l)]) for l in lags])
    j = int(np.argmax(cc))
    frac = 0.0
    if 0 < j < len(cc) - 1:
        y0, y1, y2 = cc[j - 1], cc[j], cc[j + 1]
        frac = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2)
    return float(lags[j] + frac), float(cc[j])


def boom_alignment(x44: np.ndarray, rep: np.ndarray, det44: Detector, onsets: list[dict], delta: float) -> dict:
    dt = 0.002
    a = bin_energy_db(x44, 44100, dt, 75.0)
    b = bin_energy_db(rep, REPLAY_FS, dt, 75.0)
    lag, cc = xcorr_lag(a, b, 125)
    segs = []
    for s0 in range(0, 75, 15):
        i0, i1 = int(s0 / dt), int((s0 + 15) / dt)
        l_, c_ = xcorr_lag(a[i0:i1], b[i0:i1], 125)
        segs.append({"span_s": [s0, s0 + 15], "offset_ms": round(l_ * dt * 1000, 3), "corr": round(c_, 4)})
    # waveform: resample the 44.1 k mono to 40 k and correlate 2 s segments at sample resolution
    x40 = resample_poly(x44, 400, 441)
    wav = []
    for s0 in (5.0, 30.0, 45.0, 60.0, 70.0):
        i0, i1 = int(s0 * REPLAY_FS), int((s0 + 2.0) * REPLAY_FS)
        ra, rb = x40[i0:i1], rep[i0:i1]
        best = None
        for l in range(-200, 201):
            seg_b = rep[i0 + l:i1 + l]
            c_ = float(np.dot(ra, seg_b) / np.sqrt(np.dot(ra, ra) * np.dot(seg_b, seg_b) + 1e-30))
            if best is None or c_ > best[1]:
                best = (l, c_)
        l = best[0]
        seg_b = rep[i0 + l:i1 + l]
        gain = float(np.dot(ra, seg_b) / np.dot(ra, ra))
        wav.append({"t_s": s0, "lag_samples_40k": l, "lag_ms": l / 40.0, "corr": round(best[1], 6),
                    "gain_replay_over_mono_db": round(20 * np.log10(abs(gain)), 3), "polarity": int(np.sign(gain))})
    # onset level: run the same frozen detector on the replay and match to the reference
    d40 = Detector(REPLAY_FS)
    r = d40.compute(rep[:int(75 * REPLAY_FS)])
    k = d40.pick(r["odf"], delta)
    t_rep, _ = refine_times(rep, REPLAY_FS, r["t"][k])
    ref_t = np.array([o["time_s"] for o in onsets if o["scored"]])
    e = match(ref_t, t_rep[(t_rep >= 28) & (t_rep < 75)], PLANT["match_tolerance_s"])
    em = e[np.isfinite(e)] * 1000
    return {
        "method": "log mean-square in exact 2 ms bins on each file's own time axis (rate independent), "
                  "normalised cross-correlation over +-250 ms, parabolic sub-bin peak; plus 40 kHz waveform "
                  "correlation (44.1 k mono resample_poly 400/441) and onset-level matching",
        "envelope_offset_ms_full": round(lag * dt * 1000, 3), "envelope_corr_full": round(cc, 4),
        "envelope_offset_ms_by_segment": segs,
        "sign": "positive offset = replay later than the 44.1 kHz reference",
        "waveform_segments": wav,
        "onset_level": {"reference_scored_onsets": int(ref_t.size),
                        "replay_onsets_28_75": int(((t_rep >= 28) & (t_rep < 75)).sum()),
                        "matched_within_25ms": int(em.size),
                        "match_fraction": round(em.size / max(1, ref_t.size), 4),
                        "signed_diff_ms_median": round(float(np.median(em)), 3) if em.size else None,
                        "abs_diff_ms_p95": round(float(np.percentile(np.abs(em), 95)), 3) if em.size else None},
        "replay_tail": {"total_s": len(rep) / REPLAY_FS,
                        "tail_from_75s_is_digital_zero": bool(np.all(rep[int(75 * REPLAY_FS):] == 0))},
    }


def sonic_alignment(s: np.ndarray, fs: int) -> dict:
    if not SONIC_FULL_WAV.exists():
        return {"status": "full Sonic source not present; window origin not checked"}
    full, ffs, _, _ = load_wav(SONIC_FULL_WAV)
    dt = 0.002
    a = bin_energy_db(full, ffs, dt, 74.0)
    b = bin_energy_db(s, fs, dt, 23.9)
    best = None
    for start in range(int(3.0 / dt), int(9.0 / dt)):
        seg = a[start:start + len(b)]
        c_ = float(np.corrcoef(seg, b)[0, 1])
        if best is None or c_ > best[1]:
            best = (start, c_)
    return {"full_source": str(SONIC_FULL_WAV), "full_source_sha256": sha256(SONIC_FULL_WAV),
            "full_source_rate": ffs,
            "window_origin_in_full_source_s": round(best[0] * dt, 4), "corr": round(best[1], 4),
            "method": "2 ms log mean-square envelope correlation over origins 3-9 s (2 ms resolution)"}


# --------------------------------------------------------------------------- annotation comparison
def runs_near(iois: np.ndarray, times: np.ndarray, target: float, tol: float = 0.15, min_len: int = 3) -> list:
    hit = np.abs(iois - target) <= tol * target
    out, i = [], 0
    while i < len(hit):
        if hit[i]:
            j = i
            while j + 1 < len(hit) and hit[j + 1]:
                j += 1
            if j - i + 1 >= min_len:
                out.append({"from_s": round(float(times[i]), 3), "to_s": round(float(times[j + 1]), 3),
                            "n_iois": int(j - i + 1), "median_ioi_ms": round(1000 * float(np.median(iois[i:j + 1])), 1)})
            i = j + 1
        else:
            i += 1
    return out


def boom_annotations(onsets: list[dict], stereo: np.ndarray, r: dict, det: Detector) -> dict:
    t = np.array([o["time_s"] for o in onsets])
    ioi = np.diff(t)
    tm = t[1:]  # IOI i ends at onset i+1
    res: dict = {}
    # 1. build 31-42 s
    m = (t[:-1] >= 31) & (t[1:] <= 42)
    seq = [{"t_s": round(float(a), 3), "ioi_ms": round(1000 * float(b), 1)} for a, b in zip(t[:-1][m], ioi[m])]
    res["build_31_42"] = {
        "annotation": "pulses become closer ~31-42 s, 434 -> 219 -> 125 -> 95 ms (provisional)",
        "n_onsets": int(((t >= 31) & (t < 42)).sum()),
        "runs_within_15pct": {str(v): runs_near(ioi[m], t[:-1][m], v / 1000) for v in (434, 219, 125, 95)},
        "median_ioi_ms_per_2s": [{"span_s": [a, a + 2],
                                  "median_ioi_ms": (round(1000 * float(np.median(ioi[(t[:-1] >= a) & (t[:-1] < a + 2)])), 1)
                                                    if ((t[:-1] >= a) & (t[:-1] < a + 2)).sum() else None),
                                  "n": int(((t[:-1] >= a) & (t[:-1] < a + 2)).sum())}
                                 for a in range(30, 44, 2)],
        "sequence": seq,
    }
    # sensitivity only (no re-tuning): the same runs over stronger subsets of the frozen onsets
    tiers = {}
    pu = np.array([o["picked_up_to_delta"] for o in onsets])
    for tier in (10, 20, 40, 80):
        tt = t[pu >= tier]
        ii = np.diff(tt)
        mt = (tt[:-1] >= 31) & (tt[1:] <= 42)
        tiers[f"picked_up_to_delta_ge_{tier}"] = {
            "n_onsets_31_42": int(((tt >= 31) & (tt < 42)).sum()),
            "runs_within_15pct": {str(v): runs_near(ii[mt], tt[:-1][mt], v / 1000) for v in (434, 219, 125, 95)},
            "sequence": [{"t_s": round(float(a_), 3), "ioi_ms": round(1000 * float(b_), 1)} for a_, b_ in zip(tt[:-1][mt], ii[mt])],
        }
    res["build_31_42"]["strength_tier_sensitivity"] = tiers
    # 2. recurrence after 55.5 s (ODF autocorrelation, lags 0.25-0.8 s)
    mm = (r["t"] >= 55.5) & (r["t"] < 75)
    o = r["odf"][mm] - r["odf"][mm].mean()
    lags = np.arange(int(0.25 / det.hop_s), int(0.8 / det.hop_s))
    ac = np.array([np.dot(o[:-l], o[l:]) / np.dot(o, o) for l in lags])
    j = int(np.argmax(ac))
    res["recurrence_after_55p5"] = {
        "annotation": "~435 ms / 138 BPM periodicity after 55.5 s (provisional)",
        "odf_autocorr_peak_lag_ms": round(1000 * lags[j] * det.hop_s, 1),
        "odf_autocorr_peak_value": round(float(ac[j]), 4),
        "odf_autocorr_at_435ms": round(float(ac[np.argmin(np.abs(lags * det.hop_s - 0.435))]), 4),
        "bpm_of_peak": round(60 / (lags[j] * det.hop_s), 2),
    }
    # 3. 107-110 ms accents
    sc = (t[:-1] >= 28) & (t[1:] < 75)
    hist, edges = np.histogram(1000 * ioi[sc], bins=np.arange(40, 500, 5))
    top = np.argsort(hist)[::-1][:6]
    res["ioi_107_110"] = {
        "annotation": "later accents ~107-110 ms, compatible with ~109 ms sixteenths (provisional)",
        "n_ioi_scored": int(sc.sum()),
        "n_ioi_100_118ms": int(((ioi[sc] >= 0.100) & (ioi[sc] <= 0.118)).sum()),
        "n_ioi_100_118ms_after_55p5": int(((ioi >= 0.100) & (ioi <= 0.118) & (t[:-1] >= 55.5) & (t[1:] < 75)).sum()),
        "top_ioi_bins_ms": [{"bin_ms": [int(edges[i]), int(edges[i + 1])], "count": int(hist[i])} for i in top],
    }
    # 4. 70-73 ms gaps near 55 s
    g = (t[:-1] >= 53) & (t[1:] <= 58) & (ioi >= 0.060) & (ioi <= 0.085)
    res["gaps_70_73_near_55"] = {
        "annotation": "brief 70-73 ms gaps around 55 s (provisional)",
        "iois_60_85ms_in_53_58s": [{"from_s": round(float(a), 3), "ioi_ms": round(1000 * float(b), 1)}
                                   for a, b in zip(t[:-1][g], ioi[g])],
    }
    # 5. decay 50-55 vs strong 55.5-75 (stereo RMS = sqrt of mean square over both channels)
    def srms(a, b):
        seg = stereo[int(a * 44100):int(b * 44100)]
        return 10 * np.log10(np.mean(seg * seg))
    d1, d2 = srms(50, 55), srms(55.5, 75)
    res["decay_vs_strong"] = {
        "annotation": "stereo RMS ~-19.81 (50-55 s) vs ~-6.69 dBFS (55.5-75 s), 13.12 dB (provisional; computed on the float MP3 decode)",
        "pcm16_stereo_rms_dbfs_50_55": round(d1, 2), "pcm16_stereo_rms_dbfs_55p5_75": round(d2, 2),
        "difference_db": round(d2 - d1, 2),
        "onset_rate_per_s_50_55": round(((t >= 50) & (t < 55)).sum() / 5, 2),
        "onset_rate_per_s_55p5_75": round(((t >= 55.5) & (t < 75)).sum() / 19.5, 2),
    }
    # 6. drop-cue sub-bass step
    ref = json.loads(DROP_CUE_REF.read_text())
    boom = next(d for d in ref["drops"] if d["name"].startswith("BOOM"))
    near = [o for o in onsets if 55.5 <= o["time_s"] <= 56.0]
    res["drop_cue_55p766"] = {
        "reference": {k: boom["bands"][k]["t"] for k in boom["bands"]},
        "reference_note": "drop-cue reference uses 10 ms band-energy half-rise per band, not spectral flux",
        "onsets_55p5_56p0": [{"time_s": o["time_s"], "strength": o["strength"],
                              "sub_fraction": o["band_fraction"]["sub"], "low_fraction": o["band_fraction"]["low"],
                              "context": o["context"], "over_sustain": o["over_sustain"]} for o in near],
    }
    return res


# --------------------------------------------------------------------------- negative control (probes)
PROBES = W / "_scratch/waveform-migration-001/runs/P6-DBS-20261015/probes"


def probe_characterisation(delta: float, boom_onsets: list[dict]) -> dict:
    """Characterise the frozen detector on the synthetic probes (written by p6_dbs_probes.py).

    Descriptive only: nothing here feeds back into delta or any other parameter.
    """
    idx_path = PROBES / "index.json"
    if not idx_path.exists():
        return {"status": "probe files absent; run scripts/p6_dbs_probes.py first"}
    index = json.loads(idx_path.read_text())
    det = Detector(40000)
    sig, res, events = {}, {}, {}
    for name, meta in index["files"].items():
        path = W / meta["file"]
        assert sha256(path) == meta["sha256"], f"probe hash mismatch: {path}"
        sig[name] = load_s16(path)
        res[name] = det.compute(sig[name])
        events[name] = np.array([e["t_s"] for e in json.loads(path.with_suffix(".json").read_text())["events"]])
    rows = []
    for dl in DELTA_GRID:
        row = {"delta": dl}
        t = res["sustain"]["t"][det.pick(res["sustain"]["odf"], dl)]
        row["sustain_false_births_per_s_steady_3_11s"] = round(float(np.sum((t >= 3) & (t < 11)) / 8.0), 3)
        for name in ("weak_alone", "weak_over_sustain", "spacing_200", "spacing_100", "spacing_75", "quiet_recovery"):
            t = res[name]["t"][det.pick(res[name]["odf"], dl)]
            et = events[name] if name != "quiet_recovery" else events[name][events[name] > 11.0]
            for tol in (0.010, 0.025):
                e = match(et, t, tol)
                row[f"{name}_recall_pm{int(tol * 1000)}ms"] = round(float(np.isfinite(e).mean()), 3)
        rows.append(row)
    # salience floor at the frozen delta: strongest false birth on the steady sustain span
    r = res["sustain"]
    k = det.pick(r["odf"], delta)
    avg_n = 2 * int(round(PARAMS["thr_avg_s"] / det.hop_s)) + 1
    thr = uniform_filter1d(r["odf"], size=avg_n, mode="constant") + delta
    steady = (r["t"][k] >= 3) & (r["t"][k] < 11)
    pu = r["odf"][k][steady] - (thr[k][steady] - delta)
    floor_ = float(pu.max()) if pu.size else None
    sc = [o for o in boom_onsets if o["scored"]]
    above = sum(o["picked_up_to_delta"] > floor_ for o in sc) if floor_ is not None else None
    return {
        "probe_index": str(idx_path.relative_to(W)),
        "probe_sha256": {k_: v["sha256"] for k_, v in index["files"].items()},
        "rule": "descriptive only; delta stays frozen at the BOOM 0-28 s choice",
        "match_rule": "one-to-one nearest within the stated tolerance; quiet_recovery scored on its 12 weak attacks only",
        "by_delta": rows,
        "sustain_false_birth_salience_floor": {
            "definition": "max picked_up_to_delta among detections on sustain.s16 steady span 3-11 s at the frozen delta",
            "value": None if floor_ is None else round(floor_, 3),
            "boom_scored_onsets_above_floor": above,
            "boom_scored_onsets_total": len(sc),
            "caution": "the probe layer (pink noise + chord at REF) is not BOOM's sustained material; this floor is an "
                       "engineering yardstick for noise-flux births, not a validated musical-salience threshold",
        },
    }


# --------------------------------------------------------------------------- plotting
DISPLAY_TIER = 10.0  # plot styling only: onsets with picked_up_to_delta below this are drawn as small grey dots


def plot_boom(path: Path, r: dict, onsets: list[dict], delta: float, det: Detector) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    t, odf = r["t"], r["odf"]
    avg_n = 2 * int(round(PARAMS["thr_avg_s"] / det.hop_s)) + 1
    thr = uniform_filter1d(odf, size=avg_n, mode="constant") + delta
    ot = np.array([o["time_s"] for o in onsets])
    of = np.array([o["time_frame_s"] for o in onsets])
    dense = np.array([o["context"] == "dense" for o in onsets])
    sus = np.array([o["over_sustain"] for o in onsets])
    strong = np.array([o["picked_up_to_delta"] >= DISPLAY_TIER for o in onsets])
    oval = odf[np.clip(np.round(of / det.hop_s).astype(int), 0, len(odf) - 1)]
    grid = np.arange(28, 75, 0.01)
    dm = dense_mask(ot, grid)

    fig, axes = plt.subplots(3, 1, figsize=(16, 12.5), gridspec_kw={"height_ratios": [1.2, 1, 1]})
    spans = [(28, 75), (36, 43), (55, 60)]
    for ax, (a, b) in zip(axes, spans):
        wide = (b - a) > 20
        m = (t >= a) & (t <= b)
        g = (grid >= a) & (grid <= b)
        ax.fill_between(grid[g], 0, 1, where=dm[g], transform=ax.get_xaxis_transform(),
                        color="#f6e7c8", alpha=0.6, lw=0, label="dense span (median IOI <= 160 ms)")
        ax.plot(t[m], odf[m], color="#36454f", lw=0.5 if wide else 0.7, label="ODF (log spectral flux)")
        ax.plot(t[m], thr[m], color="#c0392b", lw=0.6, ls="--", alpha=0.8,
                label=f"threshold = moving mean +-50 ms + delta {delta:g}")
        sel = (ot >= a) & (ot <= b)
        ax.plot(ot[sel & ~strong], oval[sel & ~strong], "o", ms=2.2 if wide else 3, color="#8a8a8a", ls="none",
                label=f"onset, low salience (survives delta < {DISPLAY_TIER:g})")
        for cond, col, lab in ((sel & strong & ~sus, "#1f6fb4", "onset rising from quieter material"),
                               (sel & strong & sus, "#c62828", "onset over sustained material")):
            ax.plot(ot[cond], oval[cond], "v", ms=4 if wide else 6, color=col, ls="none", label=lab)
            if not wide:
                ax.vlines(ot[cond], 0.0, 0.05, transform=ax.get_xaxis_transform(), color=col, lw=1.2)
        ax.set_ylim(0, float(odf[m].max()) * (1.32 if wide else 1.06))  # headroom keeps the legend off the data
        ax.set_title(f"BOOM {a}-{b} s: {int(sel.sum())} reference onsets, {int((sel & strong).sum())} with "
                     f"picked_up_to_delta >= {DISPLAY_TIER:g}  |  {int((sel & dense).sum())} dense, "
                     f"{int((sel & sus).sum())} over sustain", loc="left", fontsize=11)
        ax.set_xlim(a, b)
        ax.set_ylabel("ODF")
        ax.grid(alpha=0.25, lw=0.5)
        if not wide:
            ax.xaxis.set_major_locator(matplotlib.ticker.MultipleLocator(0.5))
    axes[0].legend(loc="upper left", fontsize=8.5, ncol=3, framealpha=0.95)
    axes[-1].set_xlabel("song time (s) = sample / 44100")
    fig.suptitle("P6 DBS onset reference - BOOM 0-75 s PCM16 (sha256 09c9a02f...), frozen SuperFlux-style detector, "
                 "markers at refined onset time; 0-28 s is unscored and not shown", fontsize=10)
    fig.tight_layout()
    fig.savefig(path, dpi=110)
    plt.close(fig)


def plot_sonic(path: Path, r: dict, onsets: list[dict], delta: float, det: Detector) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    t, odf = r["t"], r["odf"]
    avg_n = 2 * int(round(PARAMS["thr_avg_s"] / det.hop_s)) + 1
    thr = uniform_filter1d(odf, size=avg_n, mode="constant") + delta
    ot = np.array([o["time_s"] for o in onsets])
    sus = np.array([o["over_sustain"] for o in onsets])
    strong = np.array([o["picked_up_to_delta"] >= DISPLAY_TIER for o in onsets])
    of = np.array([o["time_frame_s"] for o in onsets])
    oval = odf[np.clip(np.round(of / det.hop_s).astype(int), 0, len(odf) - 1)]
    grid = np.arange(0, 24, 0.01)
    dm = dense_mask(ot, grid)
    fig, axes = plt.subplots(2, 1, figsize=(16, 8))
    for ax, (a, b) in zip(axes, ((0, 24), (19, 24))):
        m = (t >= a) & (t <= b)
        g = (grid >= a) & (grid <= b)
        ax.fill_between(grid[g], 0, 1, where=dm[g], transform=ax.get_xaxis_transform(), color="#f2d49b",
                        alpha=0.55, lw=0, label="dense span")
        ax.plot(t[m], odf[m], color="#36454f", lw=0.6, label="ODF")
        ax.plot(t[m], thr[m], color="#c0392b", lw=0.8, ls="--", label="threshold")
        sel = (ot >= a) & (ot <= b)
        ax.plot(ot[sel & ~strong], oval[sel & ~strong], "o", ms=2.5, color="#8a8a8a", ls="none",
                label=f"onset, low salience (survives delta < {DISPLAY_TIER:g})")
        ax.plot(ot[sel & strong & ~sus], oval[sel & strong & ~sus], "v", ms=5, color="#1f6fb4", ls="none",
                label="onset rising from quieter material")
        ax.plot(ot[sel & strong & sus], oval[sel & strong & sus], "v", ms=5, color="#c62828", ls="none",
                label="onset over sustained material")
        for edge in (0.5, 23.5):
            if a <= edge <= b:
                ax.axvline(edge, color="k", lw=0.8, ls=":")
        ax.set_ylim(0, float(odf[m].max()) * (1.32 if (b - a) > 10 else 1.06))
        ax.set_xlim(a, b)
        ax.set_ylabel("ODF")
        ax.grid(alpha=0.25, lw=0.5)
        ax.set_title(f"Sonic Samba window {a}-{b} s (0 = 6 s into retained source): {int(sel.sum())} onsets, "
                     f"{int((sel & strong).sum())} with picked_up_to_delta >= {DISPLAY_TIER:g}; "
                     "dotted = scored window edges 0.5/23.5 s", loc="left")
    axes[0].legend(loc="upper left", fontsize=8, ncol=3, framealpha=0.95)
    axes[-1].set_xlabel("window time (s); 12.8 kHz input, analysis bands capped at 5.76 kHz")
    fig.tight_layout()
    fig.savefig(path, dpi=110)
    plt.close(fig)


# --------------------------------------------------------------------------- main
def clean(o):
    if isinstance(o, dict):
        return {k: clean(v) for k, v in o.items() if k != "wave"}
    if isinstance(o, list):
        return [clean(v) for v in o]
    if isinstance(o, (np.floating,)):
        return float(o)
    if isinstance(o, (np.integer,)):
        return int(o)
    if isinstance(o, np.bool_):
        return bool(o)
    return o


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for p, h in ((BOOM_WAV, BOOM_SHA), (REPLAY_S16, REPLAY_SHA), (SONIC_WAV, SONIC_SHA)):
        got = sha256(p)
        assert got == h, f"sha256 mismatch for {p}: {got}"

    with wave.open(str(BOOM_WAV)) as w:
        braw = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").reshape(-1, w.getnchannels())
    stereo = braw.astype(np.float64) / 32768.0
    x, fs, ch, nfr = load_wav(BOOM_WAV)
    det = Detector(fs)

    # ---- 1. choose delta on BOOM 0-28 s set A (development only)
    dev = x[:int(28 * fs)]
    r_dev = det.compute(dev)
    host_t = r_dev["t"][det.pick(r_dev["odf"], DELTA_GRID[0])]
    plantsA = make_plants(dev, fs, (0.5, 27.5), host_t, PLANT["seeds"]["boom_A_dev"])
    yA = apply_plants(dev, plantsA)
    rA = det.compute(yA)
    ptA = np.array([p["t"] for p in plantsA])
    sweep = []
    for dl in DELTA_GRID:
        e = match(ptA, rA["t"][det.pick(rA["odf"], dl)], PLANT["match_tolerance_s"])
        s_ = score(plantsA, e)
        n_host = int(len(det.pick(r_dev["odf"], dl)))
        sweep.append({"delta": dl, "recall_ge_m18": s_["ge_minus18"]["recall"],
                      "abs_err_p95_ge_m18_ms": s_["ge_minus18"]["abs_err_ms_p95"],
                      "recall_all": s_["all"]["recall"], "host_only_onsets_0_28": n_host})
    passing = [s_ for s_ in sweep if s_["recall_ge_m18"] >= 0.90 and (s_["abs_err_p95_ge_m18_ms"] or 1e9) <= 6.0]
    if passing:
        delta, delta_basis = max(s_["delta"] for s_ in passing), "primary rule"
    else:
        best = max(s_["recall_ge_m18"] for s_ in sweep)
        delta = max(s_["delta"] for s_ in sweep if s_["recall_ge_m18"] == best)
        delta_basis = "fallback rule (primary rule unsatisfiable on the grid)"
    PARAMS["delta"] = delta

    # ---- 2. validate at the frozen delta: set A, set B, Sonic; raw vs refined timing
    plantsA2, yA2, eA_raw, eA_ref = run_plant_set(det, dev, (0.5, 27.5), host_t, PLANT["seeds"]["boom_A_dev"], delta, True)
    host_t_dev = r_dev["t"][det.pick(r_dev["odf"], delta)]
    plantsB, yB, eB_raw, eB_ref = run_plant_set(det, dev, (0.5, 27.5), host_t, PLANT["seeds"]["boom_B_holdout"], delta, True)

    s, sfs, sch, snfr = load_wav(SONIC_WAV)
    dets = Detector(sfs)
    r_s_host = dets.compute(s)
    host_ts = r_s_host["t"][dets.pick(r_s_host["odf"], DELTA_GRID[0])]
    plantsS, yS, eS_raw, eS_ref = run_plant_set(dets, s, (0.5, 23.5), host_ts, PLANT["seeds"]["sonic"], delta, True)

    # refinement choice is made on set A only
    sA_raw, sA_ref = score(plantsA2, eA_raw), score(plantsA2, eA_ref)
    use_refined = (sA_ref["ge_minus18"]["recall"] >= sA_raw["ge_minus18"]["recall"]
                   and (sA_ref["ge_minus18"]["abs_err_ms_p95"] or 1e9) < (sA_raw["ge_minus18"]["abs_err_ms_p95"] or 1e9))

    # red test: omit every other plant from set B, keep scoring against the full list
    half = [p for i, p in enumerate(plantsB) if i % 2 == 0]
    yH = apply_plants(dev, half)
    rH = det.compute(yH)
    tH = rH["t"][det.pick(rH["odf"], delta)]
    if use_refined:
        tH, _ = refine_times(yH, fs, tH)
    ptB = np.array([p["t"] for p in plantsB])
    eH = match(ptB, tH, PLANT["match_tolerance_s"])
    eB_sel = eB_ref if use_refined else eB_raw
    kept = np.arange(len(plantsB)) % 2 == 0
    lvB = np.array([p["level_db"] for p in plantsB])
    eHost = match(ptB, host_t_dev if not use_refined else refine_times(dev, fs, host_t_dev)[0], PLANT["match_tolerance_s"])
    rng = np.random.default_rng(1)
    rand_t = rng.uniform(0.5, 27.5, 2000)
    chance = float(np.mean([np.min(np.abs(host_t_dev - t_)) <= PLANT["match_tolerance_s"] for t_ in rand_t]))
    red = {
        "rule": "set B re-synthesised with every other plant omitted; detections scored against the FULL plant list",
        "full_set_matched": int(np.isfinite(eB_sel).sum()),
        "half_set_matched_total": int(np.isfinite(eH).sum()),
        "half_set_matched_on_present_plants": int(np.isfinite(eH[kept]).sum()),
        "half_set_matched_on_omitted_plants": int(np.isfinite(eH[~kept]).sum()),
        "omitted_plants": int((~kept).sum()),
        "omitted_ge_m18_matched_in_full_run": int(np.isfinite(eB_sel[~kept & (lvB >= -18)]).sum()),
        "omitted_ge_m18_matched_in_half_run": int(np.isfinite(eH[~kept & (lvB >= -18)]).sum()),
        "host_only_matched_to_plant_list": int(np.isfinite(eHost).sum()),
        "host_only_note": "zero by construction: plants were placed >= 40 ms from host detections",
        "chance_rate_random_times_within_25ms_of_a_host_onset": round(chance, 4),
        "chance_note": "fraction of 2000 uniformly random times in 0.5-27.5 s lying within +-25 ms of a frozen-delta host "
                       "onset: what an unexcluded plant could match by coincidence",
        "went_red": bool(np.isfinite(eH).sum() < np.isfinite(eB_sel).sum()
                         and np.isfinite(eH[~kept]).sum() == 0),
    }

    def strata(plants, e, host_frozen_t):
        lv = np.array([p["level_db"] for p in plants])
        ty = np.array([p["type"] for p in plants])
        pt = np.array([p["t"] for p in plants])
        dist = np.array([np.min(np.abs(host_frozen_t - t_)) for t_ in pt])
        loc = np.array([p["host_local_rms_dbfs"] for p in plants])
        out = {"host_onsets_for_distance": "host-only detections at the frozen delta"}
        for nm, m in (("burst", ty == "burst"), ("kick", ty == "kick")):
            mm = m & (lv >= -18)
            out[f"recall_ge_m18_{nm}"] = {"n": int(mm.sum()), "recall": float(np.isfinite(e[mm]).mean()) if mm.any() else None}
        for lo, hi in ((0.040, 0.060), (0.060, 0.100), (0.100, 10.0)):
            mm = (lv >= -18) & (dist >= lo) & (dist < hi)
            out[f"recall_ge_m18_host_dist_{int(lo*1000)}_{int(min(hi,9.999)*1000)}ms"] = {
                "n": int(mm.sum()), "recall": float(np.isfinite(e[mm]).mean()) if mm.any() else None}
        for lo, hi in ((-99, -15), (-15, 0)):
            mm = (lv >= -18) & (loc >= lo) & (loc < hi)
            out[f"recall_ge_m18_local_rms_{lo}_{hi}dbfs"] = {"n": int(mm.sum()),
                                                              "recall": float(np.isfinite(e[mm]).mean()) if mm.any() else None}
        return out

    def block(plants, e_raw, e_ref, label, host_frozen_t):
        sr, sf = score(plants, e_raw), score(plants, e_ref)
        sel = sf if use_refined else sr
        e_sel = e_ref if use_refined else e_raw
        st = strata(plants, e_sel, host_frozen_t)
        return {"set": label, "n_plants": len(plants), "timing_raw_frame": sr, "timing_refined": sf,
                "strata_selected_timing": st,
                "selected_timing": "refined" if use_refined else "raw_frame",
                "requirement": {"recall_ge_m18_ge_0p90": sel["ge_minus18"]["recall"] >= 0.90,
                                "recall_ge_m18_bursts_only_ge_0p90": (st["recall_ge_m18_burst"]["recall"] or 0) >= 0.90,
                                "abs_err_p95_ge_m18_le_6ms": (sel["ge_minus18"]["abs_err_ms_p95"] or 1e9) <= 6.0,
                                "abs_err_p95_all_matched_le_6ms": (sel["all"]["abs_err_ms_p95"] or 1e9) <= 6.0},
                "plants": plant_rows(plants, e_raw, e_ref)}

    plant_validation = {
        "plant_spec": PLANT, "delta_grid": DELTA_GRID, "delta_rule": DELTA_RULE, "delta_sweep_setA": sweep,
        "delta_frozen": delta, "delta_basis": delta_basis, "delta_fallback_rule": DELTA_FALLBACK,
        "refinement": REFINE,
        "refinement_choice_rule": "use refined times if, on set A at the frozen delta, refined recall(>=-18) is not lower "
                                  "and refined p95 |err|(>=-18) is strictly lower than raw frame times",
        "refinement_selected": use_refined,
        "boom_A_dev": block(plantsA2, eA_raw, eA_ref, "BOOM 0-28 s set A (development: delta + refinement chosen here)", host_t_dev),
        "boom_B_holdout": block(plantsB, eB_raw, eB_ref, "BOOM 0-28 s set B (independent seed; not used for any choice)", host_t_dev),
        "sonic": block(plantsS, eS_raw, eS_ref, "Sonic window 0.5-23.5 s (transfer; not used for any choice)",
                       r_s_host["t"][dets.pick(r_s_host["odf"], delta)]),
        "red_test": red,
    }

    # ---- 3. BOOM reference (frozen)
    r = det.compute(x)
    k = det.pick(r["odf"], delta)
    onsets = describe(x, fs, det, r, k, delta, (28.0, 75.0))
    if not use_refined:
        for o in onsets:
            o["time_s"], o["refined"] = o["time_frame_s"], False
    rep = load_s16(REPLAY_S16)
    align = boom_alignment(x, rep, det, onsets, delta)
    ann = boom_annotations(onsets, stereo, r, det)

    sc_ = [o for o in onsets if o["scored"]]
    summary = {"scored_window_s": [28.0, 75.0], "n_onsets_total": len(onsets), "n_scored": len(sc_),
               "n_scored_dense": sum(o["context"] == "dense" for o in sc_),
               "n_scored_over_sustain": sum(o["over_sustain"] for o in sc_),
               "n_scored_dense_and_over_sustain": sum(o["context"] == "dense" and o["over_sustain"] for o in sc_),
               "n_scored_refined": sum(o["refined"] for o in sc_)}
    boom_doc = {
        "schema": "p6-dbs-reference-onsets/1",
        "input": {"path": str(BOOM_WAV.relative_to(W)), "sha256": BOOM_SHA, "rate": fs, "channels": ch, "frames": nfr},
        "song_time_origin": "song time = sample / 44100 in this PCM16 file (sample 0 = 0.000 s); 0-28 s rows scored=false",
        "params": {**PARAMS, "realised": det.realised(), "refinement": REFINE if use_refined else None},
        "plant_validation": plant_validation,
        "alignment_check": {"replay": {"path": str(REPLAY_S16), "sha256": REPLAY_SHA, "rate": REPLAY_FS,
                                       "channels": 1, "frames": int(len(rep))}, **align},
        "annotation_comparison": ann,
        "negative_control_probes": probe_characterisation(delta, onsets),
        "summary": summary,
        "onsets": onsets,
    }
    (OUT / "boom-onsets.json").write_text(json.dumps(clean(boom_doc), indent=1))
    plot_boom(OUT / "boom-onsets.png", r, onsets, delta, det)

    # ---- 4. Sonic reference (frozen; same delta)
    rs = dets.compute(s)
    ks = dets.pick(rs["odf"], delta)
    son = describe(s, sfs, dets, rs, ks, delta, (0.5, 23.5))
    if not use_refined:
        for o in son:
            o["time_s"], o["refined"] = o["time_frame_s"], False
    ref = json.loads(DROP_CUE_REF.read_text())
    sd = next(d for d in ref["drops"] if d["name"] == "Sonic 27.7")
    salign = sonic_alignment(s, sfs)
    near = [o for o in son if 21.5 <= o["time_s"] <= 22.0]
    ss = [o for o in son if o["scored"]]
    sonic_doc = {
        "schema": "p6-dbs-reference-onsets/1",
        "input": {"path": str(SONIC_WAV.relative_to(W)), "sha256": SONIC_SHA, "rate": sfs, "channels": sch, "frames": snfr},
        "song_time_origin": "window time = sample / 12800 (0 = 6.000 s into the retained 75 s Sonic source); scored 0.5-23.5 s",
        "rate_limits": "12.8 kHz input: Nyquist 6.4 kHz, analysis fmax 5.76 kHz; the 'high' group covers 2.0-5.76 kHz only "
                       "and there is no >8 kHz content. Band fractions are not comparable one-to-one with BOOM.",
        "params": {**PARAMS, "realised": dets.realised(), "refinement": REFINE if use_refined else None},
        "plant_validation": {"see": "boom-onsets.json plant_validation.sonic (delta frozen on BOOM)",
                             "sonic": plant_validation["sonic"]},
        "alignment_check": {"window_origin": salign,
                            "drop_cue_sonic_27p7_source_time": {k_: v["t"] for k_, v in sd["bands"].items()},
                            "drop_cue_in_window_time": {k_: round(v["t"] - 6.0, 3) for k_, v in sd["bands"].items()},
                            "onsets_window_21p5_22p0": [{"time_s": o["time_s"], "strength": o["strength"],
                                                         "sub_fraction": o["band_fraction"]["sub"]} for o in near]},
        "summary": {"scored_window_s": [0.5, 23.5], "n_onsets_total": len(son), "n_scored": len(ss),
                    "n_scored_dense": sum(o["context"] == "dense" for o in ss),
                    "n_scored_over_sustain": sum(o["over_sustain"] for o in ss),
                    "n_scored_refined": sum(o["refined"] for o in ss)},
        "onsets": son,
    }
    (OUT / "sonic-6-30-onsets.json").write_text(json.dumps(clean(sonic_doc), indent=1))
    plot_sonic(OUT / "sonic-6-30-onsets.png", rs, son, delta, dets)

    print(json.dumps(clean({"delta": delta, "basis": delta_basis, "use_refined": use_refined,
                            "strata": {k_: plant_validation[k_]["strata_selected_timing"] for k_ in ("boom_A_dev", "boom_B_holdout", "sonic")},
                            "reqs": {k_: plant_validation[k_]["requirement"] for k_ in ("boom_A_dev", "boom_B_holdout", "sonic")},
                            "A": {k_: plant_validation["boom_A_dev"][k_] for k_ in ("requirement",)},
                            "A_raw": plant_validation["boom_A_dev"]["timing_raw_frame"]["ge_minus18"],
                            "A_ref": plant_validation["boom_A_dev"]["timing_refined"]["ge_minus18"],
                            "B_raw": plant_validation["boom_B_holdout"]["timing_raw_frame"]["ge_minus18"],
                            "B_ref": plant_validation["boom_B_holdout"]["timing_refined"]["ge_minus18"],
                            "S_raw": plant_validation["sonic"]["timing_raw_frame"]["ge_minus18"],
                            "S_ref": plant_validation["sonic"]["timing_refined"]["ge_minus18"],
                            "nS": len(plantsS), "red": red, "summary": summary,
                            "sonic_summary": sonic_doc["summary"],
                            "align": {k_: align[k_] for k_ in ("envelope_offset_ms_full", "onset_level")}}), indent=1))


if __name__ == "__main__":
    main()


