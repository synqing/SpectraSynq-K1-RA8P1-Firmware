#!/usr/bin/env python3
"""Lane P6 (effect mapping, Captain ruling 1): host replay of boot composition v8 against v9.

Offline, host only; touches no device. Agent cannot listen; the visual verdict is the Captain's.

  p6_effect_mapping.py build   --profile v8|v9 --exe PATH
  p6_effect_mapping.py prepare --exe V8EXE --out RUN/inputs        # P3-method level calibration
  p6_effect_mapping.py run     --exe EXE --inputs RUN/inputs --out RUN/replay/NAME [--v9 VARIANT FAULT G32 G18]
                               [--only NAME ...]
  p6_effect_mapping.py hash    RUN/replay/NAME                     # per-input SHA-256 of renderer frames

Inputs (the P3 inputs): the BOOM song file (P2 raw, 40 kHz) and the room recordings of BOOM,
Sonic Samba 6-30 and Dopex (iPhone audio of each canonical-audio-v01 capture, on the movie
timeline). Each is scaled by ONE gain so the host's in-house raw RMS median over the P3 Q4
window equals the device's recorded median (P3 agc-trace method), then 8 s of digital zeros
are appended. The room recordings are also written at -6 and +6 dB for the loudness ladder.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
CAP = ROOT / "_scratch/waveform-migration-001/canonical-audio-v01"
SRC = ROOT / "_scratch/waveform-migration-001/runs/P2-20261014T071112Z/permitted/raw/boom-40k-mono.s16"
P3 = ROOT / "_scratch/waveform-migration-001/runs/P3-20261014T080935Z/permitted/analysis/p3-data.json"
FS = 40000
TAIL_S = 8.0
ROOMS = {"boom": "boom-admission-fix-candidate-001", "sonic": "sonic-6-30-admission-fix-candidate-001",
         "dopex": "dopex-admission-fix-holdout-001"}
WINDOW = {"boom": (28.0, 75.0), "sonic": (0.5, 23.5), "dopex": (28.0, 75.0)}  # P3 Q4 windows (song time)
LADDER_DB = (-6.0, 0.0, 6.0)
FIXTURE = {"boom": 75.0, "sonic": 24.0, "dopex": 75.0}  # audio_window duration_s of each capture
SOURCES = ["tests/host/p6_effect_probe.cpp", "platform/ra8p1/inhouse_audio.cpp", "src/k1/core/visual/inhouse_waveform.cpp",
           "src/k1/core/audio/gdft_goertzel.cpp", "src/k1/core/audio/tempo_tracker.cpp", "src/k1/core/audio/tempo_acf.cpp",
           "src/k1/core/visual/product_palette.cpp", "src/k1/core/visual/product_palette_data.cpp"]


def build(profile: str, exe: Path) -> None:
    exe.parent.mkdir(parents=True, exist_ok=True)
    flags = ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc", "-Isrc/k1"]
    if profile == "v9":
        flags += ["-DK1_INHOUSE_MAPPING_V9=1", "-DK1_INHOUSE_V9_HOST_VARIANTS=1"]
    obj = exe.with_suffix(".poly24.o")
    subprocess.run(["cc", "-std=c11", *flags, "-c", "platform/ra8p1/k1_poly24.c", "-o", str(obj)], cwd=ROOT, check=True)
    subprocess.run(["c++", "-std=c++17", *flags, *SOURCES, str(obj), "-o", str(exe)], cwd=ROOT, check=True)


def room_pcm(key: str) -> tuple[np.ndarray, float, float]:
    d = CAP / ROOMS[key]
    av = json.loads((d / "observation.json").read_text())["av_validation"]
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(d / "original.mov"), "-vn", "-ac", "1", "-ar", str(FS),
                          "-f", "f32le", "-"], capture_output=True, check=True).stdout
    # File time 0 = movie time 0; song time = (file time - offset) / scale.
    return np.frombuffer(raw, dtype="<f4").astype(np.float64), av["source_to_movie_offset_s"], av["source_to_audio_scale"]


def device_median(key: str) -> float:
    lo, hi = WINDOW[key]
    s = json.loads(P3.read_text())[key]["samples"]
    return float(np.median([x["inhouse_raw_rms"] for x in s if x.get("inhouse_raw_rms") is not None and lo <= x["t_mir"] <= hi]))


def write_s16(x: np.ndarray, gain: float, path: Path) -> float:
    y = x * gain * 32768.0
    clipped = float(np.mean(np.abs(y) > 32767.0))
    tail = np.zeros(int(TAIL_S * FS))
    np.clip(np.round(np.concatenate([y, tail])), -32768, 32767).astype("<i2").tofile(path)
    return clipped


def run_probe(exe: Path, s16: Path, csv: Path, frames: Path, v9: list[str] | None) -> None:
    argv = [str(exe), str(s16), str(csv), str(frames)] + (v9 or [])
    subprocess.run(argv, check=True)


def host_median(csv: Path, offset: float, scale: float, key: str) -> float:
    import pandas as pd
    d = pd.read_csv(csv, usecols=["t_s", "board", "raw_rms"])
    d = d[d.board == 0]
    t = (d.t_s - offset) / scale
    lo, hi = WINDOW[key]
    return float(d.raw_rms[(t >= lo) & (t <= hi)].median())


def prepare(exe: Path, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    tmp = out / "_cal"
    tmp.mkdir(exist_ok=True)
    manifest = {"fs": FS, "tail_s": TAIL_S, "method": "one gain per recording: host raw RMS median over the P3 Q4 "
                "window = device median (p3-data.json); then 8 s digital zeros appended", "inputs": {}}
    plan = [("boom_source", np.fromfile(SRC, dtype="<i2").astype(np.float64) / 32768.0, 0.0, 1.0, "boom")]
    for key in ROOMS:
        x, off, scale = room_pcm(key)
        plan.append((f"{key}_room", x, off, scale, key))
    for name, x, off, scale, key in plan:
        target = device_median(key)
        lo, hi = WINDOW[key]
        a, b = int((lo * scale + off) * FS), int((hi * scale + off) * FS)
        gain = 0.12 / max(float(np.sqrt(np.mean(x[a:b] ** 2))), 1e-9)
        for _ in range(4):
            write_s16(x, gain, tmp / "cal.s16")
            run_probe(exe, tmp / "cal.s16", tmp / "cal.csv", tmp / "cal.bin", None)
            gain *= target / host_median(tmp / "cal.csv", off, scale, key)
        rows = {}
        ladder = LADDER_DB if name.endswith("_room") else (0.0,)
        for db in ladder:
            tag = name if db == 0.0 else f"{name}{int(db):+d}dB"
            p = out / f"{tag}.s16"
            clipped = write_s16(x, gain * 10 ** (db / 20.0), p)
            rows[tag] = {"file": p.name, "sha256": hashlib.sha256(p.read_bytes()).hexdigest(), "ladder_db": db,
                         "clipped_fraction": clipped}
        run_probe(exe, out / f"{name}.s16", tmp / "cal.csv", tmp / "cal.bin", None)
        manifest["inputs"][name] = {"song": key, "gain": gain, "offset_s": off, "scale": scale,
                                    "fixture_s": FIXTURE[key] if name.endswith("_room") else None,
                                    "file_seconds": len(x) / FS, "device_raw_rms_median": target,
                                    "host_raw_rms_median": host_median(tmp / "cal.csv", off, scale, key),
                                    "window_song_s": list(WINDOW[key]), "files": rows}
        print(name, json.dumps(manifest["inputs"][name]["host_raw_rms_median"]), target, flush=True)
    for p in tmp.iterdir():
        p.unlink()
    tmp.rmdir()
    (out / "inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")


def run(exe: Path, inputs: Path, out: Path, v9: list[str] | None, only: list[str] | None) -> None:
    out.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((inputs / "inputs.json").read_text())
    hashes = {}
    for name, spec in manifest["inputs"].items():
        for tag, f in spec["files"].items():
            if only and tag not in only:
                continue
            frames = out / f"{tag}.frames.bin"
            run_probe(exe, inputs / f["file"], out / f"{tag}.csv", frames, v9)
            hashes[tag] = {"frames_sha256": hashlib.sha256(frames.read_bytes()).hexdigest(),
                           "frames_bytes": frames.stat().st_size,
                           "csv_sha256": hashlib.sha256((out / f"{tag}.csv").read_bytes()).hexdigest()}
            frames.unlink()  # the hash is the record; bytes are reproducible from the inputs
            print(tag, hashes[tag]["frames_sha256"][:16], flush=True)
    meta = {"exe": str(exe), "exe_sha256": hashlib.sha256(exe.read_bytes()).hexdigest(), "v9": v9, "hashes": hashes}
    (out / "replay.json").write_text(json.dumps(meta, indent=2) + "\n")


def prepare_it5(exe: Path, base: Path, out: Path, holdout_wav: Path) -> None:
    """Iteration 5 inputs (TOLERANCE-FROZEN-P6-DI.json): BOOM file + room at 0 dB copied from the
    P6 inputs unchanged, plus the Carte Blanche holdout (24 kHz -> 40 kHz, one gain so the host
    raw RMS median over 1-74 s equals the device BOOM median 0.118652, 8 s zeros appended)."""
    import shutil
    import wave
    from scipy.signal import resample_poly
    out.mkdir(parents=True, exist_ok=True)
    src = json.loads((base / "inputs.json").read_text())
    manifest = {"fs": FS, "tail_s": TAIL_S, "method": "iteration 5: BOOM inputs copied; Carte Blanche holdout added",
                "inputs": {}}
    for name in ("boom_source", "boom_room"):
        spec = dict(src["inputs"][name])
        spec["files"] = {name: spec["files"][name]}
        shutil.copyfile(base / spec["files"][name]["file"], out / spec["files"][name]["file"])
        manifest["inputs"][name] = spec
    w = wave.open(str(holdout_wav))
    if w.getframerate() != 24000 or w.getnchannels() != 1:
        raise SystemExit("holdout must be 24 kHz mono")
    x24 = np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(np.float64) / 32768.0
    x = resample_poly(x24, 5, 3)
    target = 0.118652
    tmp = out / "_cal"
    tmp.mkdir(exist_ok=True)
    gain = 0.12 / max(float(np.sqrt(np.mean(x[FS:74 * FS] ** 2))), 1e-9)
    WINDOW["carte"] = (1.0, 74.0)
    for _ in range(4):
        write_s16(x, gain, tmp / "cal.s16")
        run_probe(exe, tmp / "cal.s16", tmp / "cal.csv", tmp / "cal.bin", None)
        gain *= target / host_median(tmp / "cal.csv", 0.0, 1.0, "carte")
    p = out / "carte_source.s16"
    clipped = write_s16(x, gain, p)
    run_probe(exe, p, tmp / "cal.csv", tmp / "cal.bin", None)
    manifest["inputs"]["carte_source"] = {
        "song": "carte", "gain": gain, "offset_s": 0.0, "scale": 1.0, "fixture_s": None, "file_seconds": len(x) / FS,
        "device_raw_rms_median": target, "host_raw_rms_median": host_median(tmp / "cal.csv", 0.0, 1.0, "carte"),
        "window_song_s": list(WINDOW["carte"]), "holdout": True,
        "source": str(holdout_wav), "source_sha256": hashlib.sha256(holdout_wav.read_bytes()).hexdigest(),
        "files": {"carte_source": {"file": p.name, "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
                                   "ladder_db": 0.0, "clipped_fraction": clipped}}}
    for f in tmp.iterdir():
        f.unlink()
    tmp.rmdir()
    (out / "inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")


def add_song(exe: Path, out: Path, name: str, wav: Path, target: float) -> None:
    """Iteration 6: add a 24 kHz song file to an inputs folder (-> 40 kHz, one gain so the host raw RMS
    median over 1-74 s equals target, 8 s zeros appended); the existing manifest entries are kept."""
    import wave
    from scipy.signal import resample_poly
    manifest = json.loads((out / "inputs.json").read_text())
    w = wave.open(str(wav))
    if w.getframerate() != 24000 or w.getnchannels() != 1:
        raise SystemExit("song must be 24 kHz mono")
    x = resample_poly(np.frombuffer(w.readframes(w.getnframes()), dtype="<i2").astype(np.float64) / 32768.0, 5, 3)
    WINDOW[name] = (1.0, 74.0)
    tmp = out / "_cal"
    tmp.mkdir(exist_ok=True)
    gain = 0.12 / max(float(np.sqrt(np.mean(x[FS:74 * FS] ** 2))), 1e-9)
    for _ in range(4):
        write_s16(x, gain, tmp / "cal.s16")
        run_probe(exe, tmp / "cal.s16", tmp / "cal.csv", tmp / "cal.bin", None)
        gain *= target / host_median(tmp / "cal.csv", 0.0, 1.0, name)
    p = out / f"{name}.s16"
    clipped = write_s16(x, gain, p)
    run_probe(exe, p, tmp / "cal.csv", tmp / "cal.bin", None)
    manifest["inputs"][name] = {
        "song": name, "gain": gain, "offset_s": 0.0, "scale": 1.0, "fixture_s": None, "file_seconds": len(x) / FS,
        "device_raw_rms_median": target, "host_raw_rms_median": host_median(tmp / "cal.csv", 0.0, 1.0, name),
        "window_song_s": [1.0, 74.0], "source": str(wav), "source_sha256": hashlib.sha256(wav.read_bytes()).hexdigest(),
        "files": {name: {"file": p.name, "sha256": hashlib.sha256(p.read_bytes()).hexdigest(), "ladder_db": 0.0,
                         "clipped_fraction": clipped}}}
    for f in tmp.iterdir():
        f.unlink()
    tmp.rmdir()
    (out / "inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")


def calibrate(exe: Path, inputs: Path, v8: Path, work: Path, variant: str, palette: str | None = None,
              level: list[str] | None = None) -> dict:
    """Matched optical energy: per-mode gain so the calibration-corpus mean S equals v8's."""
    sys.path.insert(0, str(ROOT / "scripts"))
    import p6_evaluate
    manifest = json.loads((inputs / "inputs.json").read_text())
    target = p6_evaluate.energy(v8, manifest)
    gains, steps = [1.0, 1.0], []
    for i in range(3):  # start, then two multiplicative refinements
        out = work / f"cal-{variant}-{i}"
        run(exe, inputs, out, [variant, "none", f"{gains[0]:.6f}", f"{gains[1]:.6f}"] + ([palette] if palette else [])
            + (level or []),
            p6_evaluate.CALIBRATION)
        got = p6_evaluate.energy(out, manifest)
        steps.append({"gains": list(gains), "mean_S": got})
        if i < 2:
            gains = [min(64.0, g * t / m) for g, t, m in zip(gains, target, got)]
    return {"variant": variant, "palette": palette or "asis", "level": level, "target_mean_S_v8": target, "steps": steps,
            "final_gains": steps[-1]["gains"], "final_mean_S": steps[-1]["mean_S"]}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build"); b.add_argument("--profile", choices=("v8", "v9"), required=True); b.add_argument("--exe", type=Path, required=True)
    p = sub.add_parser("prepare"); p.add_argument("--exe", type=Path, required=True); p.add_argument("--out", type=Path, required=True)
    r = sub.add_parser("run"); r.add_argument("--exe", type=Path, required=True); r.add_argument("--inputs", type=Path, required=True)
    r.add_argument("--out", type=Path, required=True); r.add_argument("--v9", nargs="+"); r.add_argument("--only", nargs="*")
    c = sub.add_parser("calibrate"); c.add_argument("--exe", type=Path, required=True); c.add_argument("--inputs", type=Path, required=True)
    c.add_argument("--v8", type=Path, required=True); c.add_argument("--work", type=Path, required=True)
    c.add_argument("--variant", choices=("peak", "energy", "emphasis"), required=True); c.add_argument("--out", type=Path, required=True)
    c.add_argument("--palette", choices=("asis", "lifted", "range"))  # iteration 4 test variants
    c.add_argument("--level", nargs="+", metavar="ARG")  # iteration 5: CEILING EMPHASIS; iteration 6 appends 7 accent args
    h = sub.add_parser("prepare-it5"); h.add_argument("--exe", type=Path, required=True); h.add_argument("--base", type=Path, required=True)
    h.add_argument("--out", type=Path, required=True); h.add_argument("--holdout", type=Path, required=True)
    s6 = sub.add_parser("add-song"); s6.add_argument("--exe", type=Path, required=True); s6.add_argument("--out", type=Path, required=True)
    s6.add_argument("--name", required=True); s6.add_argument("--wav", type=Path, required=True); s6.add_argument("--target", type=float, required=True)
    a = ap.parse_args(argv)
    if a.cmd == "add-song":
        add_song(a.exe, a.out, a.name, a.wav, a.target)
        return 0
    if a.cmd == "prepare-it5":
        prepare_it5(a.exe, a.base, a.out, a.holdout)
        return 0
    if a.cmd == "calibrate":
        a.out.write_text(json.dumps(calibrate(a.exe, a.inputs, a.v8, a.work, a.variant, a.palette, a.level), indent=2) + "\n")
        return 0
    if a.cmd == "build":
        build(a.profile, a.exe)
    elif a.cmd == "prepare":
        prepare(a.exe, a.out)
    elif a.cmd == "run":
        run(a.exe, a.inputs, a.out, a.v9, a.only)
    return 0


if __name__ == "__main__":
    sys.exit(main())
