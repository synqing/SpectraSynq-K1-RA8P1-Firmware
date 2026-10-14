#!/usr/bin/env python3
"""P6 DBS pilot: synthetic mechanism probes (engineering probes, not review music).

Host-only. Writes 40 kHz mono s16le files (+8 s digital-zero tail, the replay
convention of /tmp/p6-product-path-p-inputs/boom_source.s16) and one JSON per file
listing every event's exact sample index, type and level, plus the file sha256.

Pinned run:
    uv run --quiet --with numpy==2.1.3 --with scipy==1.14.1 \
        python scripts/p6_dbs_probes.py

Level convention: REF = median of 100 ms non-overlapping RMS windows (raw int16
counts) of the BOOM replay input over 28-75 s. The sustained layer is scaled to
RMS = REF; an attack at L dB has RMS over its first 20 ms = REF * 10^(L/20).
Every component is deterministic (fixed seeds), and files are built by summing
identical components, so e.g. weak_over_sustain - sustain == the attacks exactly
(before integer rounding).
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

import numpy as np
from scipy.signal import butter, sosfilt

W = Path(__file__).resolve().parents[1]
OUT = W / "_scratch/waveform-migration-001/runs/P6-DBS-20261015/probes"
REPLAY = Path("/tmp/p6-product-path-p-inputs/boom_source.s16")
REPLAY_SHA = "45edfd0948c8cc24f99bcde0a8f2d8b4c8fa70413d648c6a6e1cfaa3a4bfbd15"
FS = 40000
TAIL_S = 8.0
FULL_SCALE = 32768.0

SPEC = {
    "rate": FS, "format": "s16le mono", "tail": f"{TAIL_S:g} s digital zero appended to every file",
    "ref_rule": "REF = median RMS of 100 ms non-overlapping windows of the BOOM replay (raw int16 counts), 28-75 s",
    "attack_level_rule": "attack RMS over its first 20 ms (from the event sample) = REF * 10^(level_db/20)",
    "event_sample_rule": "sample index where the attack envelope starts (envelope = 0 at that sample, so the first "
                         "non-zero attack sample is index + 1); t_s = sample / 40000",
    "floor": "pink noise at -60 dBFS RMS, present for the whole content span except declared digital-zero spans",
    "pink_noise": "Gaussian white spectrum shaped 1/sqrt(f) (flat below 20 Hz), DC removed, unit RMS, fixed seed",
    "sustained_layer": "pink noise + chord sines 220/277/330 Hz (equal amplitude, phases 0/0.7/1.9 rad); "
                       "pink and chord at equal RMS; total RMS = REF (x2 for the +6 dB loud section); "
                       "tremolo 0.2 Hz, +-0.5 dB (1 dB peak-to-peak) gain",
    "layer_fade": "raised cosine; 1.0 s in/out in sustain/weak/spacing files, 0.25 s in after the digital-zero gap",
    "kick": "80 ms sine, frequency glide 60 -> 45 Hz exponential, 1 ms linear attack, exp decay tau 30 ms, "
            "5 ms cosine taper at end; plus a 5 ms click (first-differenced white noise, exp decay tau 1 ms) "
            "at -6 dB peak relative to the body peak; one frozen realisation reused for every kick",
    "snare": "40 ms white noise, causal Butterworth band-pass 300-8000 Hz order 2, 0.5 ms linear attack, "
             "exp decay tau 12 ms, 5 ms cosine taper; one frozen realisation reused for every snare",
    "seeds": {"floor": 101, "layer_pink": 202, "kick_click": 303, "snare": 404},
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pink(n: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    spec = np.fft.rfft(rng.standard_normal(n))
    f = np.fft.rfftfreq(n, 1 / FS)
    spec *= 1 / np.sqrt(np.maximum(f, 20.0))
    spec[0] = 0
    x = np.fft.irfft(spec, n)
    return x / np.sqrt(np.mean(x * x))


def raised_cos(n: int) -> np.ndarray:
    return 0.5 - 0.5 * np.cos(np.pi * np.arange(n) / max(1, n))


def ref_rms() -> tuple[float, dict]:
    assert sha256(REPLAY) == REPLAY_SHA, "replay input hash mismatch"
    x = np.fromfile(REPLAY, dtype="<i2").astype(np.float64)
    seg = x[28 * FS:75 * FS]
    n = int(0.1 * FS)
    m = len(seg) // n
    r = np.sqrt((seg[:m * n].reshape(m, n) ** 2).mean(axis=1))
    ref = float(np.median(r))
    return ref, {"replay": str(REPLAY), "replay_sha256": REPLAY_SHA, "window_s": 0.1, "span_s": [28, 75],
                 "n_windows": int(m), "ref_rms_counts": round(ref, 3),
                 "ref_rms_dbfs": round(20 * np.log10(ref / FULL_SCALE), 3)}


def make_kick() -> np.ndarray:
    n = int(0.080 * FS)
    t = np.arange(n) / FS
    f = 60.0 * (45.0 / 60.0) ** (t / 0.080)
    body = np.sin(2 * np.pi * np.cumsum(f) / FS - 2 * np.pi * f[0] / FS)
    env = np.exp(-t / 0.030) * np.minimum(1.0, t / 0.001)
    env[-int(0.005 * FS):] *= raised_cos(int(0.005 * FS))[::-1]
    body *= env
    rng = np.random.default_rng(SPEC["seeds"]["kick_click"])
    nc = int(0.005 * FS)
    click = np.diff(rng.standard_normal(nc + 1)) * np.exp(-np.arange(nc) / (0.001 * FS))
    click *= 0.5 * np.abs(body).max() / np.abs(click).max()
    body[:nc] += click
    body[0] = 0.0  # envelope starts at zero on the declared event sample
    return body


def make_snare() -> np.ndarray:
    n = int(0.040 * FS)
    rng = np.random.default_rng(SPEC["seeds"]["snare"])
    sos = butter(2, [300, 8000], btype="bandpass", fs=FS, output="sos")
    x = sosfilt(sos, rng.standard_normal(n))
    t = np.arange(n) / FS
    env = np.exp(-t / 0.012) * np.minimum(1.0, t / 0.0005)
    env[-int(0.005 * FS):] *= raised_cos(int(0.005 * FS))[::-1]
    return x * env


def scaled(w: np.ndarray, ref: float, level_db: float) -> np.ndarray:
    head = w[:int(0.020 * FS)]
    return w * ref * 10 ** (level_db / 20) / np.sqrt(np.mean(head * head))


class Builder:
    def __init__(self, content_s: float, ref: float):
        self.n = int(round(content_s * FS))
        self.ref = ref
        self.x = np.zeros(self.n)
        self.events: list[dict] = []
        self.spans: list[dict] = []
        self.zero: list[tuple[int, int]] = []

    def floor(self) -> None:
        self.x += pink(self.n, SPEC["seeds"]["floor"]) * FULL_SCALE * 10 ** (-60 / 20)
        self.spans.append({"component": "floor_pink_-60dBFS", "from_s": 0.0, "to_s": self.n / FS})

    def layer(self, a_s: float, b_s: float, fade_in_s: float, fade_out_s: float, gain_db: float = 0.0) -> None:
        a, b = int(round(a_s * FS)), int(round(b_s * FS))
        n = b - a
        t = np.arange(self.n) / FS  # chord/tremolo phase referenced to file time: identical across files
        pk = pink(self.n, SPEC["seeds"]["layer_pink"])[a:b]
        chord = sum(np.sin(2 * np.pi * f * t[a:b] + ph) for f, ph in ((220, 0.0), (277, 0.7), (330, 1.9)))
        chord /= np.sqrt(np.mean(chord * chord))
        y = (pk + chord) / np.sqrt(2.0)
        y *= 10 ** ((0.5 * np.sin(2 * np.pi * 0.2 * t[a:b])) / 20)
        y *= self.ref * 10 ** (gain_db / 20) / np.sqrt(np.mean(y * y))
        fi, fo = int(round(fade_in_s * FS)), int(round(fade_out_s * FS))
        if fi:
            y[:fi] *= raised_cos(fi)
        if fo:
            y[n - fo:] *= raised_cos(fo)[::-1]
        self.x[a:b] += y
        self.spans.append({"component": "sustained_layer", "gain_db_re_ref": gain_db, "from_s": a_s, "to_s": b_s,
                           "fade_in_s": fade_in_s, "fade_out_s": fade_out_s,
                           "steady_from_s": a_s + fade_in_s, "steady_to_s": b_s - fade_out_s})

    def attack(self, t_s: float, kind: str, level_db: float, wave: np.ndarray) -> None:
        s = int(round(t_s * FS))
        w = scaled(wave, self.ref, level_db)
        end = min(self.n, s + len(w))
        self.x[s:end] += w[:end - s]
        head = w[:int(0.020 * FS)]
        self.events.append({"sample": s, "t_s": s / FS, "type": kind, "level_db_re_ref": level_db,
                            "rms_first20ms_counts": round(float(np.sqrt(np.mean(head * head))), 2),
                            "peak_counts": round(float(np.abs(w).max()), 1), "duration_samples": len(w)})

    def digital_zero(self, a_s: float, b_s: float) -> None:
        a, b = int(round(a_s * FS)), int(round(b_s * FS))
        self.zero.append((a, b))
        self.spans.append({"component": "digital_zero", "from_s": a_s, "to_s": b_s})

    def finish(self, name: str, purpose: str, notes: list[str], transitions: list[dict]) -> dict:
        for a, b in self.zero:
            self.x[a:b] = 0.0
        y = np.concatenate([self.x, np.zeros(int(TAIL_S * FS))])
        clip_idx = np.where((np.round(y) > 32767) | (np.round(y) < -32768))[0]
        clipped = int(clip_idx.size)
        pcm = np.clip(np.round(y), -32768, 32767).astype("<i2")
        path = OUT / f"{name}.s16"
        pcm.tofile(path)
        ev = sorted(self.events, key=lambda e: e["sample"])
        for i, e in enumerate(ev):
            nxt = ev[i + 1]["sample"] if i + 1 < len(ev) else None
            e["spacing_to_next_samples"] = None if nxt is None else nxt - e["sample"]
            e["overlap_next_samples"] = 0 if nxt is None else max(0, e["sample"] + e["duration_samples"] - nxt)
        content = pcm[:self.n].astype(np.float64)
        doc = {
            "schema": "p6-dbs-probe/1", "file": str(path.relative_to(W)), "sha256": sha256(path),
            "rate": FS, "channels": 1, "format": "s16le",
            "frames": int(len(pcm)), "duration_s": len(pcm) / FS, "content_duration_s": self.n / FS,
            "tail_s": TAIL_S, "tail_is_digital_zero": bool(np.all(pcm[self.n:] == 0)),
            "purpose": purpose, "notes": notes,
            "clipped_samples": clipped,
            "clipped_at_s": sorted({round(i / FS, 3) for i in clip_idx.tolist()}),
            "clip_note": None if not clipped else "hard-clipped to int16 range; levels follow the brief and were not "
                                                   "re-tuned to avoid it",
            "peak_abs_counts": int(np.abs(pcm.astype(np.int32)).max()),
            "content_rms_dbfs": round(float(20 * np.log10(np.sqrt(np.mean(content ** 2)) / FULL_SCALE + 1e-12)), 3),
            "components": self.spans,
            "non_attack_transitions": transitions,
            "n_events": len(ev), "events": ev,
        }
        (OUT / f"{name}.json").write_text(json.dumps(doc, indent=1))
        return doc


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    ref, ref_doc = ref_rms()
    kick, snare = make_kick(), make_snare()
    waves = {"kick": kick, "snare": snare}
    docs = {}

    # Shared timeline for files 1-3: floor 0-16 s; layer 2-12 s (1 s fades); attacks at 4.0 + 0.6 k s.
    weak_times = [4.0 + 0.6 * k for k in range(12)]
    weak_kinds = ["kick" if k % 2 == 0 else "snare" for k in range(12)]
    layer_tr = [{"t_s": 2.0, "what": "sustained layer fade-in starts (1 s raised cosine)"},
                {"t_s": 11.0, "what": "sustained layer fade-out starts (1 s raised cosine)"}]

    b = Builder(16.0, ref)
    b.floor()
    b.layer(2.0, 12.0, 1.0, 1.0)
    docs["sustain"] = b.finish(
        "sustain", "No attacks. Any onset/birth here is a false birth (fade spans declared for exclusion if desired).",
        ["2 s quiet, 10 s sustained layer (incl. 1 s fades), 4 s quiet"], layer_tr)

    b = Builder(16.0, ref)
    b.floor()
    for t_, k_ in zip(weak_times, weak_kinds):
        b.attack(t_, k_, -12.0, waves[k_])
    docs["weak_alone"] = b.finish(
        "weak_alone", "12 weak attacks (-12 dB re REF) alone over the -60 dBFS floor.",
        ["first attack at 4.0 s, not 2.0 s: attack times are identical to weak_over_sustain, whose layer needs "
         "its 1 s fade-in plus 1 s steady lead; 0-4 s is quiet floor here",
         "alternating kick (even index) / snare (odd index), 600 ms apart"], [])

    b = Builder(16.0, ref)
    b.floor()
    b.layer(2.0, 12.0, 1.0, 1.0)
    for t_, k_ in zip(weak_times, weak_kinds):
        b.attack(t_, k_, -12.0, waves[k_])
    docs["weak_over_sustain"] = b.finish(
        "weak_over_sustain", "Same attacks/levels/times as weak_alone, over the sustained layer of sustain.",
        ["floor and layer are sample-identical to sustain.s16; attacks sample-identical to weak_alone.s16"], layer_tr)

    for ms in (200, 100, 75):
        b = Builder(14.0, ref)
        b.floor()
        b.layer(2.0, 12.0, 1.0, 1.0)
        step = ms * FS // 1000
        s0 = 4 * FS
        k = 0
        while s0 + k * step < 10 * FS:
            b.attack((s0 + k * step) / FS, "kick", -6.0, kick)
            k += 1
        docs[f"spacing_{ms}"] = b.finish(
            f"spacing_{ms}", f"Train of identical kick attacks at -6 dB re REF, {ms} ms spacing, over the sustained layer.",
            [f"train spans 4.0-10.0 s ({k} attacks, spacing exactly {step} samples)",
             "attack type: kick (80 ms body + 5 ms click); at 75 ms spacing each 80 ms kick tail overlaps the next "
             "attack by 200 samples (its last 5 ms taper)", "layer steady 3-11 s; quiet floor to 14 s"], layer_tr)

    b = Builder(22.5, ref)
    b.floor()
    b.layer(0.0, 8.0, 0.0, 0.02, gain_db=6.0)
    t_ = 0.5
    i = 0
    while t_ < 8.0 - 0.1 + 1e-9:
        b.attack(t_, "kick" if i % 2 == 0 else "snare", 0.0, waves["kick" if i % 2 == 0 else "snare"])
        i += 1
        t_ = round(0.5 + 0.110 * i, 6)
    n_loud = i
    b.digital_zero(8.0, 11.0)
    b.layer(11.0, 20.5, 0.25, 1.0)
    for k in range(12):
        b.attack(11.6 + 0.6 * k, "kick" if k % 2 == 0 else "snare", -12.0, waves["kick" if k % 2 == 0 else "snare"])
    docs["quiet_recovery"] = b.finish(
        "quiet_recovery",
        "8 s loud dense section, 3 s digital zero, then 12 weak attacks over the normal layer: recovery after loud history.",
        [f"loud: layer +6 dB re REF from 0 s (abrupt start), {n_loud} attacks at 0 dB every 110 ms from 0.5 s, "
         "alternating kick/snare; 20 ms fade-out into the gap",
         "8.0-11.0 s digital zero (floor removed too)",
         "11.0 s: floor resumes and normal layer fades in over 0.25 s; weak attacks at 11.6 + 0.6 k s (k = 0..11), "
         "-12 dB, alternating kick/snare; layer fades out 19.5-20.5 s; floor to 22.5 s",
         "weak-attack levels/types match weak_over_sustain; delay from loud end to first weak attack = 3.6 s"],
        [{"t_s": 0.0, "what": "file starts with loud layer (abrupt)"},
         {"t_s": 7.98, "what": "loud layer 20 ms fade-out begins"},
         {"t_s": 8.0, "what": "digital zero begins"},
         {"t_s": 11.0, "what": "floor resumes; normal layer 0.25 s fade-in begins"},
         {"t_s": 19.5, "what": "layer fade-out begins (1 s)"}])

    index = {"schema": "p6-dbs-probe-index/1", "spec": SPEC, "reference_level": ref_doc,
             "files": {k: {"file": d["file"], "sha256": d["sha256"], "duration_s": d["duration_s"],
                           "n_events": d["n_events"], "clipped_samples": d["clipped_samples"],
                           "peak_abs_counts": d["peak_abs_counts"]} for k, d in docs.items()}}
    (OUT / "index.json").write_text(json.dumps(index, indent=1))
    print(json.dumps(index["reference_level"]), json.dumps(index["files"], indent=1))


if __name__ == "__main__":
    main()
