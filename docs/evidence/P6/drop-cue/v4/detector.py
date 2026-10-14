#!/usr/bin/env python3
"""Causal sub-bass return detector, host reference. PROPOSAL — NOT APPROVED.

The Titan visualiser must keep showing music. A drop is the moment the
sub-bass comes back after a gap. This file is research on the host. It does
not change product firmware and it does not claim what the lights look like.

Stream: 24 kHz mono, hops of 180 samples (7.5 ms). The output of hop n uses
only samples at indices < (n+1)*180. Every constant is a field on the
configs below or a planted-fault field in DETECTOR-CRITERIA-v4.json.

Run from this directory:
  python detector.py --stage planted
  python detector.py --stage score
  python detector.py --stage figure
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import subprocess
from collections import deque
from dataclasses import asdict, dataclass
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy.signal import butter, resample_poly, sosfilt

HERE = Path(__file__).resolve().parent
CRITERIA_PATH = HERE / "DETECTOR-CRITERIA-v4.json"
V1_CACHE = HERE.parent / "cache"
CACHE = HERE / "cache"
RESULTS = HERE / "results"

FS = 24000
HOP = 180
HOP_S = HOP / FS  # 0.0075 s, exact in decimal; comparisons use FLOAT_SLACK_S
SCORE_LO = 0.0
SCORE_HI = 75.0
HIT_AFTER_S = 0.050
EXCLUSION_S = 0.5
FLOAT_SLACK_S = 1e-9
RESAMPLE_HALF_LEN_MULT = 10

# One biquad sample: 5 multiplies, 4 adds, plus 5 cycles for the state
# loads, stores and the scalar loop. Same count the v1 note used.
BIQUAD_CYCLES = 14
BUDGET_CYCLES = 50000


def load_criteria() -> dict:
    return json.loads(CRITERIA_PATH.read_text())


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def alpha(tau_s: float) -> float:
    return 1.0 - math.exp(-HOP_S / tau_s)


def asym(state: float, target: float, tau_up: float, tau_down: float) -> float:
    tau = tau_up if target > state else tau_down
    return state + alpha(tau) * (target - state)


def band_sos(lo_hz: float, hi_hz: float, order: int) -> np.ndarray:
    return butter(int(order), (float(lo_hz), float(hi_hz)), btype="bandpass", fs=FS, output="sos")


class BandFront:
    """One causal bandpass. Broadband is the raw hop, computed by the caller."""

    def __init__(self, lo_hz: float, hi_hz: float, order: int) -> None:
        self.sos = band_sos(lo_hz, hi_hz, order)
        self.zi = np.zeros((self.sos.shape[0], 2), dtype=np.float64)

    @property
    def n_sections(self) -> int:
        return int(self.sos.shape[0])

    def hop_rms(self, x: np.ndarray) -> float:
        y, self.zi = sosfilt(self.sos, x, zi=self.zi)
        return float(math.sqrt(float(np.mean(y * y)) + 1e-18))


class TrailingMin:
    """Causal sliding minimum. Hop n sees only values from hops >= n-win+1."""

    def __init__(self, hops: int) -> None:
        self.hops = int(hops)
        self.values: deque[tuple[int, float]] = deque()
        self.index = 0

    def push(self, value: float) -> float:
        while self.values and self.values[0][0] <= self.index - self.hops:
            self.values.popleft()
        while self.values and self.values[-1][1] >= value:
            self.values.pop()
        self.values.append((self.index, float(value)))
        self.index += 1
        return self.values[0][1]


def db_ratio(now: float, prev: float) -> float:
    return 20.0 * math.log10((now + 1e-18) / (prev + 1e-18))


@dataclass
class FloorStepConfig:
    """Fast sub-bass envelope minus its trailing minimum.

    The floor is the minimum of the fast envelope over `floor_window_s`.
    The envelope has sat low when `step_db` stays at or under `slack_db`.
    After that has lasted `hold_s`, the next hop that leaves the floor fires
    if the step and the 3-hop rise both clear their gates. `decision_hops`
    is 1, which is inside the allowed 1–3 hops: the whole rise has to be
    present on the hop that leaves the floor. Two extra hops let a climax
    whose rise arrives late still cross the gate.
    """

    name: str = "floor_step"
    sub_lo_hz: float = 30.0
    sub_hi_hz: float = 90.0
    sub_order: int = 2
    tau_fast_attack_s: float = 0.012
    tau_fast_release_s: float = 0.180
    floor_window_s: float = 2.0
    slack_db: float = 18.0
    hold_s: float = 1.0
    step_db: float = 14.0
    rise_db: float = 12.0
    rise_hops: int = 3
    decision_hops: int = 1
    warmup_s: float = 1.0
    refractory_s: float = 1.6


@dataclass
class RatioGateConfig(FloorStepConfig):
    """floor_step, and the sub-bass / broadband ratio must be rising."""

    name: str = "ratio_gate"
    share_rise_db: float = 6.0


@dataclass
class TensionConfig(FloorStepConfig):
    """floor_step released only when a non-sub build has been held by the gap.

    The build is the rise, over `build_window_s`, of a slow envelope of the
    energy outside the sub-bass band. The gap holds that tension. A pure
    sub-bass recording never builds it, so this path stays quiet there.
    """

    name: str = "tension_release"
    tau_rest_attack_s: float = 0.40
    tau_rest_release_s: float = 1.40
    build_window_s: float = 1.0
    tension_min: float = 0.50
    tension_decay_s: float = 2.0
    tension_cap: float = 8.0


class FloorStep:
    """Sub-bass return. State: 0 idle, 1 gap, 2 armed, 3 refractory."""

    def __init__(self, cfg: FloorStepConfig | None = None) -> None:
        self.cfg = cfg or FloorStepConfig()
        c = self.cfg
        self.front = BandFront(c.sub_lo_hz, c.sub_hi_hz, c.sub_order)
        self.floor = TrailingMin(max(1, int(round(c.floor_window_s / HOP_S))))
        self.t = 0.0
        self.fast = 0.0
        self.started = False
        self.low_time = 0.0
        self.armed_time = 0.0
        self.since = 0
        self.refractory = 0.0
        self.hist: list[float] = []
        self.share_hist: list[float] = []
        self.rest_hist: list[float] = []
        self.rest_slow = 0.0
        self.rest_started = False
        self.tension = 0.0
        self.last_sub = 0.0
        self.last_broad = 0.0
        self.last_fast = 0.0
        self.last_floor = 0.0
        self.last_step_db = 0.0
        self.last_rise_db = 0.0
        self.last_share_rise_db = 0.0
        self.last_tension = 0.0
        self.last_fire_kind = ""

    @property
    def n_sections(self) -> int:
        return self.front.n_sections

    def _extra_ok(self) -> bool:
        return True

    def _after_low(self, rest: float) -> None:
        return None

    def push(self, hop: np.ndarray) -> bool:
        cfg = self.cfg
        sub = self.front.hop_rms(hop)
        broad = float(math.sqrt(float(np.mean(hop * hop)) + 1e-18))
        self.last_sub = sub
        self.last_broad = broad
        if not self.started:
            self.fast = sub
            self.started = True
        else:
            self.fast = asym(self.fast, sub, cfg.tau_fast_attack_s, cfg.tau_fast_release_s)
        floor = self.floor.push(self.fast)
        self.last_fast = self.fast
        self.last_floor = floor
        step = db_ratio(self.fast, floor)
        self.hist.append(self.fast)
        while len(self.hist) > cfg.rise_hops + 1:
            self.hist.pop(0)
        # hist[0] is exactly rise_hops ago once the buffer is full, else the oldest hop.
        rise = db_ratio(self.fast, self.hist[0])
        self.last_step_db = step
        self.last_rise_db = rise
        share_db = 10.0 * math.log10((sub * sub) / (broad * broad) + 1e-18)
        self.share_hist.append(share_db)
        while len(self.share_hist) > cfg.rise_hops + 1:
            self.share_hist.pop(0)
        self.last_share_rise_db = share_db - self.share_hist[0]
        rest = math.sqrt(max(broad * broad - sub * sub, 0.0))
        self._track_rest(rest)
        self.t += HOP_S
        fired = False
        if step <= cfg.slack_db:
            self.low_time += HOP_S
            self.armed_time = self.low_time
            self.since = 0
            self._after_low(rest)
        else:
            self.since += 1
            if (
                self.refractory <= 0.0
                and self.t >= cfg.warmup_s
                and 1 <= self.since <= cfg.decision_hops
                and self.armed_time >= cfg.hold_s
                and step >= cfg.step_db
                and rise >= cfg.rise_db
                and self._extra_ok()
            ):
                fired = True
                self.last_fire_kind = cfg.name
                self.refractory = cfg.refractory_s
                self.low_time = 0.0
                self.armed_time = 0.0
                self.since = 99
                self.tension = 0.0
            elif self.since > cfg.decision_hops:
                self.low_time = 0.0
                self.armed_time = 0.0
        if self.refractory > 0.0:
            self.refractory = max(0.0, self.refractory - HOP_S)
        self.last_tension = self.tension
        return fired

    def _track_rest(self, rest: float) -> None:
        return None

    @property
    def state_code(self) -> int:
        if self.refractory > 0.0:
            return 3
        if self.armed_time >= self.cfg.hold_s and self.since == 0:
            return 2
        if self.low_time > 0.0 or self.since > 0:
            return 1
        return 0


class RatioGate(FloorStep):
    def __init__(self, cfg: RatioGateConfig | None = None) -> None:
        super().__init__(cfg or RatioGateConfig())

    def _extra_ok(self) -> bool:
        return self.last_share_rise_db >= float(self.cfg.share_rise_db)  # type: ignore[attr-defined]


class TensionRelease(FloorStep):
    def __init__(self, cfg: TensionConfig | None = None) -> None:
        super().__init__(cfg or TensionConfig())

    def _track_rest(self, rest: float) -> None:
        cfg: TensionConfig = self.cfg  # type: ignore[assignment]
        if not self.rest_started:
            self.rest_slow = rest
            self.rest_started = True
        else:
            self.rest_slow = asym(self.rest_slow, rest, cfg.tau_rest_attack_s, cfg.tau_rest_release_s)
        self.rest_hist.append(self.rest_slow)
        keep = int(round(cfg.build_window_s / HOP_S)) + 2
        if len(self.rest_hist) > keep:
            self.rest_hist.pop(0)

    def _after_low(self, rest: float) -> None:
        cfg: TensionConfig = self.cfg  # type: ignore[assignment]
        hops = int(round(cfg.build_window_s / HOP_S))
        if len(self.rest_hist) > hops:
            build = db_ratio(self.rest_hist[-1], self.rest_hist[-1 - hops])
            if build > 0.0:
                self.tension = min(cfg.tension_cap, self.tension + build * HOP_S)
        # The gap holds tension: no decay on this branch.

    def push(self, hop: np.ndarray) -> bool:
        before = self.tension
        fired = super().push(hop)
        cfg: TensionConfig = self.cfg  # type: ignore[assignment]
        # Outside the gap, tension leaks. The fire test already saw the held value.
        if self.last_step_db > cfg.slack_db and not fired:
            self.tension = before * math.exp(-HOP_S / cfg.tension_decay_s)
            self.last_tension = self.tension
        return fired

    def _extra_ok(self) -> bool:
        cfg: TensionConfig = self.cfg  # type: ignore[assignment]
        return self.tension >= cfg.tension_min


class BroadbandSurge:
    """Planted fault. Constants come from the frozen file and are not fitted."""

    def __init__(self, spec: dict | None = None) -> None:
        spec = spec or load_criteria()["planted_faults"]["broadband_loudness"]
        self.tau_fast = float(spec["tau_fast_s"])
        self.tau_slow = float(spec["tau_slow_s"])
        self.surge_db = float(spec["surge_db"])
        self.warmup = float(spec["warmup_s"])
        self.refractory_s = float(spec["refractory_s"])
        self.eps = float(spec["power_eps"])
        self.fast = 0.0
        self.slow = 0.0
        self.t = 0.0
        self.refractory = 0.0
        self.started = False
        self.last_sub = 0.0
        self.last_broad = 0.0
        self.last_fast = 0.0
        self.last_step_db = 0.0
        self.last_rise_db = 0.0
        self.state_code = 0

    def push(self, hop: np.ndarray) -> bool:
        power = float(np.mean(hop * hop))
        self.last_broad = float(math.sqrt(power + 1e-18))
        self.last_sub = self.last_broad
        self.last_fast = self.last_broad
        if not self.started:
            self.fast = power
            self.slow = power
            self.started = True
        else:
            self.fast = self.fast + alpha(self.tau_fast) * (power - self.fast)
            self.slow = self.slow + alpha(self.tau_slow) * (power - self.slow)
        surge = 10.0 * math.log10((self.fast + self.eps) / (self.slow + self.eps))
        self.last_step_db = surge
        self.last_rise_db = surge
        self.t += HOP_S
        fired = False
        if self.refractory > 0.0:
            self.refractory = max(0.0, self.refractory - HOP_S)
        elif self.t >= self.warmup and surge >= self.surge_db:
            fired = True
            self.refractory = self.refractory_s
            self.slow = max(self.slow, self.fast)
        self.state_code = 3 if self.refractory > 0.0 else 0
        return fired


def measure_resample_delay(source_rate: int) -> int:
    g = math.gcd(int(source_rate), FS)
    up, down = FS // g, int(source_rate) // g
    pos = 2000
    guard = 4 * (2 * RESAMPLE_HALF_LEN_MULT * max(up, down) + 1)
    imp = np.zeros(pos + guard, dtype=np.float64)
    imp[pos] = 1.0
    y = resample_poly(imp, up, down)
    peak = int(np.argmax(np.abs(y)))
    expected = pos * up / down
    return max(int(round(peak - expected)), 0)


def to_24k(mono: np.ndarray, source_rate: int) -> tuple[np.ndarray, int]:
    if int(source_rate) == FS:
        return np.ascontiguousarray(mono, dtype=np.float64), 0
    g = math.gcd(int(source_rate), FS)
    up, down = FS // g, int(source_rate) // g
    y = resample_poly(np.ascontiguousarray(mono, dtype=np.float64), up, down)
    delay = measure_resample_delay(int(source_rate))
    if delay:
        y = y[delay:]
    return y, delay


def load_wav_24k(path: Path) -> tuple[np.ndarray, dict]:
    audio, rate = sf.read(str(path), always_2d=True, dtype="float64")
    mono = audio.mean(axis=1)
    y, delay = to_24k(mono, int(rate))
    return y, {
        "path": str(path),
        "source_rate": int(rate),
        "channels": int(audio.shape[1]),
        "resample_delay_samples": delay,
        "samples_24k": int(len(y)),
        "sha256": sha256_file(path),
        "duration_s": float(len(y) / FS),
    }


def load_room(spec: dict) -> tuple[np.ndarray, dict]:
    """Read the v1 decoded cache. Do not write it. Decode into v2/cache only if needed."""
    path = Path(spec["path"])
    digest = sha256_file(path)
    if digest != spec["sha256"]:
        raise RuntimeError(f"{spec['id']} hash {digest} != {spec['sha256']}")
    name = spec["id"]
    meta_v1 = V1_CACHE / f"{name}.json"
    npy_v1 = V1_CACHE / f"{name}.npy"
    if meta_v1.exists() and npy_v1.exists():
        meta = json.loads(meta_v1.read_text())
        if meta.get("sha256") == digest:
            pcm = np.load(npy_v1).astype(np.float64)
            info = dict(meta)
            info["duration_s"] = float(len(pcm) / FS)
            info["loaded_from"] = str(npy_v1)
            return pcm, info
    CACHE.mkdir(parents=True, exist_ok=True)
    npy = CACHE / f"{name}.npy"
    meta_path = CACHE / f"{name}.json"
    if npy.exists() and meta_path.exists():
        meta = json.loads(meta_path.read_text())
        if meta.get("sha256") == digest:
            pcm = np.load(npy).astype(np.float64)
            meta["duration_s"] = float(len(pcm) / FS)
            return pcm, meta
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", str(path), "-ac", "1", "-ar", "48000", "-f", "f32le", "-"],
        check=True, capture_output=True).stdout
    mono = np.frombuffer(raw, dtype="<f4").astype(np.float64)
    y, delay = to_24k(mono, 48000)
    meta = {
        "path": str(path), "source_rate": 48000, "channels": 1,
        "resample_delay_samples": delay, "samples_24k": int(len(y)), "sha256": digest,
        "duration_s": float(len(y) / FS),
        "decode": "ffmpeg mono 48 kHz float, then delay-compensated resample to 24 kHz",
    }
    np.save(npy, y.astype(np.float32))
    meta_path.write_text(json.dumps(meta, indent=2) + "\n")
    return y, meta


def song_time_at_hop(index: int, song_start: float, movie_offset: float, movie_scale: float) -> float:
    t_stream = (index + 1) * HOP_S
    return song_start + (t_stream - movie_offset) / movie_scale


def in_hit(t: float, drop: float) -> bool:
    dt = t - drop
    return -FLOAT_SLACK_S <= dt <= HIT_AFTER_S + FLOAT_SLACK_S


def in_exclusion(t: float, drop: float) -> bool:
    return abs(t - drop) <= EXCLUSION_S + FLOAT_SLACK_S


def score_fires(fires: list[float], drops: list[float], scored_lo: float, scored_hi: float) -> dict:
    """Frozen v1 rule, not widened. Fires outside the scored span are ignored."""
    required = [d for d in drops if scored_lo - FLOAT_SLACK_S <= d < scored_hi]
    inside = [t for t in fires if scored_lo - FLOAT_SLACK_S <= t < scored_hi]
    hit_flags = []
    hit_times = []
    for d in required:
        matches = [t for t in inside if in_hit(t, d)]
        hit_flags.append(bool(matches))
        hit_times.append(matches[0] if matches else None)
    false = [t for t in inside if not any(in_exclusion(t, d) for d in required)]
    hit_set = set()
    for d in required:
        for t in inside:
            if in_hit(t, d):
                hit_set.add(t)
    collar = [t for t in inside if t not in hit_set and t not in false]
    seconds = max(scored_hi - scored_lo, FLOAT_SLACK_S)
    return {
        "required_drops_s": required,
        "hits": hit_flags,
        "hit_count": int(sum(hit_flags)),
        "hit_times_s": hit_times,
        "misses_s": [d for d, ok in zip(required, hit_flags) if not ok],
        "false_fires_s": false,
        "false_count": len(false),
        "false_per_minute": len(false) / (seconds / 60.0),
        "collar_fires_s": collar,
        "collar_count": len(collar),
        "scored_lo_s": scored_lo,
        "scored_hi_s": scored_hi,
        "scored_seconds": seconds,
        "pass": bool(hit_flags) and all(hit_flags) and len(false) == 0,
    }


def full_song_false_rate(fires: list[float], drops: list[float], duration_s: float) -> dict:
    """Report-only. The whole file, not the [0, 75) span. Does not decide the pass."""
    false = [t for t in fires if 0.0 <= t <= duration_s + FLOAT_SLACK_S and not any(in_exclusion(t, d) for d in drops)]
    return {
        "role": "report_only",
        "duration_s": duration_s,
        "false_count": len(false),
        "false_per_minute": len(false) / (duration_s / 60.0) if duration_s > 0 else 0.0,
        "false_fires_s": false,
    }


def synthetic_times(span: tuple[float, float], drops: list[float], kind: str) -> list[float]:
    lo, hi = span
    if kind == "always_on":
        n0 = math.ceil(lo / HOP_S - 1e-12)
        n1 = math.floor((hi - FLOAT_SLACK_S) / HOP_S)
        return [k * HOP_S for k in range(n0, n1 + 1)]
    if kind == "delayed":
        return [d + 0.200 for d in drops if lo <= d + 0.200 < hi]
    raise ValueError(kind)


@dataclass
class Window:
    id: str
    role: str
    pcm: np.ndarray
    drops: list[float]
    song_start: float
    movie_offset: float
    movie_scale: float
    content_hi: float
    climax: list[list[float]]
    cut: list[list[float]]
    info: dict
    is_song_file: bool

    def time_of(self, index: int) -> float:
        return song_time_at_hop(index, self.song_start, self.movie_offset, self.movie_scale)

    @property
    def scored_lo(self) -> float:
        return max(SCORE_LO, self.song_start)

    @property
    def scored_hi(self) -> float:
        return min(SCORE_HI, self.content_hi)


def _window_from_spec(spec: dict, pcm: np.ndarray, info: dict, content_hi: float,
                      role: str, is_song_file: bool,
                      movie_offset: float = 0.0, movie_scale: float = 1.0) -> Window:
    return Window(
        id=spec["id"], role=role, pcm=pcm, drops=list(spec["drops_s"]),
        song_start=float(spec.get("song_start_s", 0.0)),
        movie_offset=movie_offset, movie_scale=movie_scale,
        content_hi=content_hi, climax=[list(p) for p in spec.get("climax_s", [])],
        cut=[list(p) for p in spec.get("cut_s", [])],
        info=info, is_song_file=is_song_file,
    )


def load_scored_windows() -> list[Window]:
    crit = load_criteria()
    out: list[Window] = []
    for spec in crit["tuning_windows"]:
        if spec["id"] in ("boom_room", "sonic_room"):
            pcm, info = load_room(spec)
            content_hi = 6.0 + 24.0 if spec["id"] == "sonic_room" else 75.0
            out.append(_window_from_spec(
                spec, pcm, info, content_hi, role="tuning", is_song_file=False,
                movie_offset=float(spec["movie_offset_s"]), movie_scale=float(spec["movie_scale"]),
            ))
        else:
            pcm, info = load_wav_24k(Path(spec["path"]))
            if info["sha256"] != spec["sha256"]:
                raise RuntimeError(f"{spec['id']} hash {info['sha256']} != {spec['sha256']}")
            out.append(_window_from_spec(spec, pcm, info, 75.0, role="tuning", is_song_file=True))
    for spec in crit["seen_windows"]:
        pcm, info = load_wav_24k(Path(spec["path"]))
        if info["sha256"] != spec["sha256"]:
            raise RuntimeError(f"{spec['id']} hash {info['sha256']} != {spec['sha256']}")
        out.append(_window_from_spec(spec, pcm, info, 75.0, role="seen", is_song_file=True))
    return out


def run_detector(det, window: Window, keep_trace: bool) -> dict:
    fires: list[float] = []
    n = len(window.pcm) // HOP
    trace = None
    if keep_trace:
        trace = {
            "t": np.empty(n),
            "sub": np.empty(n),
            "broad": np.empty(n),
            "fast": np.empty(n),
            "step": np.empty(n),
            "rise": np.empty(n),
            "state": np.empty(n, dtype=np.int8),
            "fired": np.zeros(n, dtype=np.int8),
            "low": np.empty(n),
        }
    for i in range(n):
        edge = det.push(window.pcm[i * HOP:(i + 1) * HOP])
        t = window.time_of(i)
        if edge:
            fires.append(t)
        if trace is not None:
            trace["t"][i] = t
            trace["sub"][i] = det.last_sub
            trace["broad"][i] = det.last_broad
            trace["fast"][i] = getattr(det, "last_fast", det.last_sub)
            trace["step"][i] = getattr(det, "last_step_db", 0.0)
            trace["rise"][i] = getattr(det, "last_rise_db", 0.0)
            trace["state"][i] = int(det.state_code)
            trace["fired"][i] = 1 if edge else 0
            trace["low"][i] = float(getattr(det, "armed_time", 0.0))
    scored = score_fires(fires, window.drops, window.scored_lo, window.scored_hi)
    duration = float(window.info.get("duration_s", n * HOP_S))
    # Song time of the last hop can run past the file label on a room recording.
    # The song-file rate uses the file duration on the song clock, which starts at 0.
    full = full_song_false_rate(fires, window.drops, duration if window.movie_offset == 0.0 else float(window.time_of(n - 1)))
    out = {
        "id": window.id,
        "role": window.role,
        "detector": type(det).__name__,
        "is_song_file": window.is_song_file,
        **scored,
        "full_song": full,
    }
    if trace is not None:
        out["trace"] = trace
        out["fires_s"] = fires
    return out


def cycle_estimate(det) -> dict:
    """[INFERENCE] Operation count. Not a measurement on the Cortex-M85."""
    sections = int(det.n_sections)
    biquad = sections * HOP * BIQUAD_CYCLES
    energies = 2  # sub-bass band, raw broadband
    energy = energies * HOP * 2
    # Naive trailing minimum over the declared window, one compare per stored hop.
    floor_hops = max(1, int(round(det.cfg.floor_window_s / HOP_S)))
    logic = 80
    extra = 0
    if isinstance(det, RatioGate):
        extra += 20
    if isinstance(det, TensionRelease):
        extra += 40
    total = biquad + energy + floor_hops + logic + extra
    return {
        "label": "[INFERENCE] from operation counts, not timed on the M85",
        "n_biquad_sections": sections,
        "cycles_per_sample_per_section": BIQUAD_CYCLES,
        "biquad_cycles": biquad,
        "energy_cycles": energy,
        "trailing_min_compares_upper_bound": floor_hops,
        "logic_cycles": logic + extra,
        "total_cycles_per_hop": total,
        "budget_cycles": BUDGET_CYCLES,
        "within_budget": total <= BUDGET_CYCLES,
        "hop_s": HOP_S,
    }


def make(name: str):
    if name == "floor_step":
        return FloorStep()
    if name == "ratio_gate":
        return RatioGate()
    if name == "tension_release":
        return TensionRelease()
    if name == "broadband_loudness":
        return BroadbandSurge()
    raise KeyError(name)


def _public(row: dict) -> dict:
    return {k: v for k, v in row.items() if k not in ("trace", "fires_s")}


def margins_from_trace(row: dict, window: Window, cfg: FloorStepConfig) -> dict:
    """How far the rise sat above the gate at a hit, and how close a non-drop came."""
    tr = row["trace"]
    t = tr["t"]
    rise = tr["rise"]
    step = tr["step"]
    fired = tr["fired"].astype(bool)
    per_drop = []
    for d, hit_t in zip(row["required_drops_s"], row["hit_times_s"]):
        window_m = (t >= d - FLOAT_SLACK_S) & (t <= d + HIT_AFTER_S + FLOAT_SLACK_S)
        collar_m = (np.abs(t - d) <= EXCLUSION_S + FLOAT_SLACK_S)
        item = {
            "drop_s": d,
            "hit_time_s": hit_t,
            "max_rise_db_in_hit_window": round(float(rise[window_m].max()) if np.any(window_m) else 0.0, 2),
            "max_step_db_in_hit_window": round(float(step[window_m].max()) if np.any(window_m) else 0.0, 2),
            "max_rise_db_in_collar": round(float(rise[collar_m].max()) if np.any(collar_m) else 0.0, 2),
        }
        if hit_t is not None:
            i = int(np.argmin(np.abs(t - hit_t)))
            item["rise_db_at_fire"] = round(float(rise[i]), 2)
            item["step_db_at_fire"] = round(float(step[i]), 2)
            item["margin_above_rise_gate_db"] = round(float(rise[i] - cfg.rise_db), 2)
            item["margin_above_step_gate_db"] = round(float(step[i] - cfg.step_db), 2)
        else:
            item["short_of_rise_gate_in_hit_window_db"] = round(
                float(cfg.rise_db - item["max_rise_db_in_hit_window"]), 2)
        per_drop.append(item)
    regions = []
    for kind, pairs in (("climax", window.climax), ("cut", window.cut)):
        for a, b in pairs:
            m = (t >= a) & (t <= b)
            if not np.any(m):
                continue
            j = int(np.argmax(np.where(m, rise, -1e9)))
            regions.append({
                "kind": kind,
                "span_s": [a, b],
                "max_rise_db": round(float(rise[j]), 2),
                "at_s": round(float(t[j]), 3),
                "short_of_rise_gate_db": round(float(cfg.rise_db - rise[j]), 2),
            })
    # Closest non-drop: largest 3-hop rise outside every exclusion, on the whole file.
    outside = np.ones(len(t), dtype=bool)
    for d in window.drops:
        outside &= np.abs(t - d) > EXCLUSION_S
    if np.any(outside):
        j = int(np.argmax(np.where(outside, rise, -1e9)))
        nearest = {
            "at_s": round(float(t[j]), 3),
            "rise_db": round(float(rise[j]), 2),
            "step_db": round(float(step[j]), 2),
            "short_of_rise_gate_db": round(float(cfg.rise_db - rise[j]), 2),
            "fired": bool(fired[j]),
        }
    else:
        nearest = None
    return {"drops": per_drop, "labelled_non_drops": regions, "closest_non_drop_rise": nearest}


def stage_planted(windows: list[Window] | None = None) -> dict:
    windows = windows or load_scored_windows()
    rows = []
    for w in windows:
        span = (w.scored_lo, w.scored_hi)
        required = [d for d in w.drops if span[0] <= d < span[1]]
        for kind in ("always_on", "delayed"):
            fires = synthetic_times(span, required, kind)
            scored = score_fires(fires, w.drops, w.scored_lo, w.scored_hi)
            rows.append({"id": w.id, "fault": kind, **scored})
        row = run_detector(BroadbandSurge(), w, keep_trace=False)
        row["fault"] = "broadband_loudness"
        rows.append(_public(row))
        # v4: a detector that fires exactly on the old hand labels must now fail.
        labels = load_criteria()["anchors"]["label_drops_s"][w.id]
        fires = [d for d in labels if span[0] <= d < span[1]]
        scored = score_fires(fires, w.drops, w.scored_lo, w.scored_hi)
        rows.append({"id": w.id, "fault": "label_anchored", **scored})
    verdict = {
        "label_red": all((not r["pass"]) and r["hit_count"] == 0
                         for r in rows if r["fault"] == "label_anchored"),
        "always_on_red": all(r["fault"] == "always_on" and (not r["pass"]) and r["false_count"] > 0
                             for r in rows if r["fault"] == "always_on"),
        "delayed_red": all(r["fault"] == "delayed" and (not r["pass"]) and r["false_count"] == 0 and r["hit_count"] == 0
                           for r in rows if r["fault"] == "delayed"),
        "broadband_red": all((not r["pass"]) for r in rows if r["fault"] == "broadband_loudness"),
    }
    verdict["scorer_red_on_all_three"] = all(verdict[k] for k in ("always_on_red", "delayed_red", "broadband_red", "label_red"))
    payload = {"proposal": "PROPOSAL — NOT APPROVED", "verdict": verdict, "rows": rows}
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "planted.json").write_text(json.dumps(payload, indent=2) + "\n")
    print(json.dumps(verdict, indent=2))
    return payload


def stage_score(windows: list[Window] | None = None) -> dict:
    planted_path = RESULTS / "planted.json"
    if not planted_path.exists():
        raise RuntimeError("planted faults have not been run")
    planted = json.loads(planted_path.read_text())
    if not planted["verdict"]["scorer_red_on_all_three"]:
        raise RuntimeError("refusing to score candidates: planted faults did not all go red")
    windows = windows or load_scored_windows()
    names = ("floor_step", "ratio_gate", "tension_release")
    rows = []
    traces = {}
    margin_doc = {}
    for name in names:
        for w in windows:
            det = make(name)
            row = run_detector(det, w, keep_trace=(name == "floor_step"))
            if "trace" in row:
                traces[w.id] = row["trace"]
                margin_doc[w.id] = margins_from_trace(row, w, det.cfg)
            pub = _public(row)
            rows.append(pub)
            print(name, w.id, "hits", row["hit_count"], "/", len(row["required_drops_s"]),
                  "false", row["false_count"], "collar", row["collar_count"],
                  "full", f"{row['full_song']['false_per_minute']:.3f}/min",
                  "PASS" if row["pass"] else "FAIL")
    by_name: dict[str, list[dict]] = {}
    for row in rows:
        by_name.setdefault(row["detector"], []).append(row)
    class_to_id = {"FloorStep": "floor_step", "RatioGate": "ratio_gate", "TensionRelease": "tension_release"}
    summary = []
    for cls, group in by_name.items():
        cid = class_to_id[cls]
        hits = sum(g["hit_count"] for g in group)
        required = sum(len(g["required_drops_s"]) for g in group)
        false = sum(g["false_count"] for g in group)
        collar = sum(g["collar_count"] for g in group)
        passed = all(g["pass"] for g in group)
        cycles = cycle_estimate(make(cid))
        summary.append({
            "id": cid, "pass": passed, "hits": hits, "required": required,
            "false": false, "collar": collar,
            "cycles_per_hop": cycles["total_cycles_per_hop"],
        })
    # Frozen tie-break. None of these pass the whole set, so: most hits,
    # fewest false fires, fewest collar fires. A remaining tie uses the
    # smaller operation count, which is the pass-case clause of the same rule.
    summary.sort(key=lambda s: (not s["pass"], -s["hits"], s["false"], s["collar"], s["cycles_per_hop"]))
    chosen = summary[0]
    tied = [s for s in summary if (s["pass"], s["hits"], s["false"], s["collar"]) ==
            (chosen["pass"], chosen["hits"], chosen["false"], chosen["collar"])]
    constants = {
        "floor_step": asdict(FloorStepConfig()),
        "ratio_gate": asdict(RatioGateConfig()),
        "tension_release": asdict(TensionConfig()),
    }
    cycles = cycle_estimate(make(chosen["id"]))
    payload = {
        "proposal": "PROPOSAL — NOT APPROVED",
        "no_unseen_holdout": True,
        "carte_role": "seen",
        "summary": summary,
        "tied_on_the_written_rule": [s["id"] for s in tied],
        "residual_tie_break": "smaller operation count, the pass-case clause of the frozen rule",
        "chosen_id": chosen["id"],
        "rows": rows,
        "margins_floor_step": margin_doc,
        "constants": constants,
        "cycles": {cid: cycle_estimate(make(cid)) for cid in names},
    }
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "score.json").write_text(json.dumps(payload, indent=2) + "\n")
    chosen_doc = {
        "status": "chosen on the seen set; there is no unseen holdout",
        "proposal": "PROPOSAL — NOT APPROVED",
        "id": chosen["id"],
        "tied_with": [s["id"] for s in tied if s["id"] != chosen["id"]],
        "residual_tie_break": payload["residual_tie_break"],
        "summary": chosen,
        "constants": constants[chosen["id"]],
        "selection_rule": load_criteria()["selection_rule"],
    }
    (HERE / "CHOSEN.json").write_text(json.dumps(chosen_doc, indent=2) + "\n")
    np.savez_compressed(RESULTS / "traces.npz", **{
        f"{wid}_{key}": arr for wid, tr in traces.items() for key, arr in tr.items()
    })
    print("CHOSEN", chosen["id"], chosen, "tied", payload["tied_on_the_written_rule"])
    print("cycles", cycles["total_cycles_per_hop"])
    return payload


def stage_figure() -> Path:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    trace = np.load(RESULTS / "traces.npz")
    doc = json.loads((RESULTS / "score.json").read_text())
    score_by_id = {r["id"]: r for r in doc["rows"] if r["detector"] == "FloorStep"}
    order = ["boom_file", "boom_room", "sonic_file", "sonic_room", "carte_file"]
    titles = {
        "boom_file": "BOOM, song file",
        "boom_room": "BOOM, room recording",
        "sonic_file": "Sonic Samba, song file",
        "sonic_room": "Sonic Samba, room (6–30 s)",
        "carte_file": "Carte Blanche, seen file (not a holdout)",
    }
    crit = load_criteria()
    regions = {}
    for w in crit["tuning_windows"]:
        regions[w["id"]] = w
    regions[crit["seen_windows"][0]["id"]] = crit["seen_windows"][0]
    cfg = doc["constants"]["floor_step"]
    fig, axes = plt.subplots(len(order), 1, figsize=(16.5, 18.5), dpi=130, sharex=False)
    fig.suptitle(
        "Drop detector v2 — PROPOSAL, NOT APPROVED\n"
        f"sub-bass return {cfg['sub_lo_hz']:.0f}–{cfg['sub_hi_hz']:.0f} Hz, "
        f"step ≥ {cfg['step_db']:.0f} dB, 3-hop rise ≥ {cfg['rise_db']:.0f} dB "
        f"after the floor sat within {cfg['slack_db']:.0f} dB for {cfg['hold_s']:.1f} s\n"
        "Grey is broadband, blue is sub-bass, green is detector state, red is a scored fire. "
        "A black cross is a full-song false fire. Amber is a climax, purple is a cut, "
        "the dashed line is the annotated drop.",
        fontsize=12, y=0.995,
    )
    for ax, wid in zip(axes, order):
        t = trace[f"{wid}_t"]
        sub = trace[f"{wid}_sub"]
        broad = trace[f"{wid}_broad"]
        state = trace[f"{wid}_state"]
        fired = trace[f"{wid}_fired"]
        sub_db = 20.0 * np.log10(sub + 1e-18)
        broad_db = 20.0 * np.log10(broad + 1e-18)
        lo = float(np.percentile(sub_db, 1)) - 3
        hi = float(np.percentile(sub_db, 99)) + 8
        ax.set_ylim(lo, hi)
        meta = regions[wid]
        for a, b in meta.get("cut_s", []) or []:
            ax.axvspan(a, b, color="#6b4c9a", alpha=0.16)
        for a, b in meta.get("climax_s", []) or []:
            ax.axvspan(a, b, color="#e09f3e", alpha=0.28)
        for d in meta.get("drops_s", []):
            ax.axvline(d, color="black", ls="--", lw=1.1)
            ax.axvspan(d, d + HIT_AFTER_S, color="#1b7f3a", alpha=0.35)
        ax.plot(t, broad_db, color="#9aa0a6", lw=0.7)
        ax.plot(t, sub_db, color="#1f4e79", lw=1.35)
        band = lo + (hi - lo) * (0.02 + 0.10 * (state.astype(float) / 3.0))
        ax.plot(t, band, color="#0b6e4f", lw=2.0)
        sc = score_by_id[wid]
        ft = t[fired.astype(bool)]
        if len(ft):
            fy = np.interp(ft, t, sub_db)
            ax.scatter(ft, fy, s=64, c="#c0392b", zorder=5)
            for ts, ys in zip(ft, fy):
                ts = float(ts)
                if any(in_hit(ts, d) for d in sc["required_drops_s"]):
                    kind = "HIT"
                elif any(in_exclusion(ts, d) for d in sc["required_drops_s"]):
                    kind = "collar"
                else:
                    kind = "FALSE"
                ax.annotate(
                    f"{kind} {ts:.2f}s",
                    (ts, float(ys)),
                    textcoords="offset points", xytext=(8, -16),
                    fontsize=9, color="#7b1e14", ha="left",
                    bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="#c0392b", alpha=0.92),
                )
        full_false = sc["full_song"]["false_fires_s"]
        if full_false:
            fx = np.array(full_false, dtype=float)
            # Keep marks that sit on this panel's time axis.
            shown = fx[(fx >= float(t[0]) - 0.05) & (fx <= float(t[-1]) + 0.05)]
            if len(shown):
                fy = np.interp(shown, t, sub_db)
                ax.scatter(shown, fy, s=80, marker="x", c="black", linewidths=1.4, zorder=6)
        note_y = lo + 0.18 * (hi - lo)
        for i, d in enumerate(sc["misses_s"]):
            ax.annotate(
                f"miss {d:.1f}s",
                (d, note_y + (i % 2) * 0.08 * (hi - lo)),
                fontsize=9, color="#1a1a1a", ha="left", va="bottom",
                bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="#888888", alpha=0.92),
            )
        ax.set_xlim(float(sc["scored_lo_s"]), float(sc["scored_hi_s"]))
        verdict = "PASS" if sc["pass"] else "FAIL"
        full = sc["full_song"]
        full_bit = ""
        if sc["is_song_file"]:
            full_bit = f"   full-song false {full['false_count']} ({full['false_per_minute']:.2f}/min over {full['duration_s']:.1f}s)"
        ax.set_title(
            f"{titles[wid]}  —  {verdict}   hits {sc['hit_count']}/{len(sc['required_drops_s'])}"
            f"   false fires {sc['false_count']} ({sc['false_per_minute']:.2f}/min)"
            f"   collar {sc['collar_count']}{full_bit}",
            loc="left", fontsize=10,
        )
        ax.set_ylabel("level, dBFS")
        ax.grid(True, axis="x", alpha=0.25)
    axes[-1].set_xlabel("song time, seconds")
    fig.tight_layout(rect=(0, 0, 1, 0.955))
    path = HERE / "detector.png"
    fig.savefig(path)
    plt.close(fig)
    print("wrote", path, path.stat().st_size)
    return path


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--stage", required=True, choices=["planted", "score", "figure", "all"])
    args = p.parse_args(argv)
    if args.stage in ("planted", "all"):
        stage_planted()
    if args.stage in ("score", "all"):
        stage_score()
    if args.stage in ("figure", "all"):
        stage_figure()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
