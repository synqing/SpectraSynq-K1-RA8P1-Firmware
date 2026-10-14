#!/usr/bin/env python3
"""Planted-truth self-test for scripts/p5_latency.py (lane P5). Host only; no device, no audio playback.

A 240 fps synthetic recording is built with real-time pts: the click-only stimulus (same click()/sync()
generators as scripts/p5_click_track.py, 60 clicks) reaches the "phone" 0.8 s into the recording with a
+40 ppm clock skew, encoded as 48 kHz AAC; the lower board (board0) brightens 9 frames after each click,
the upper board (board1) 11 frames after. The tool is run through its real entry point (subprocess).

Must be green: recovery of 9 and 11 frames within one frame, despite a planted one-frame blip 2 frames
after every 6th board0 click; an iPhone-Slo-mo-style time-remapping
edit list handled explicitly; frozen rules accepted, unfrozen or edited rules refused.
Planted faults that must go red: (a) 120 fps; (b) video stretched 8x against the audio; (c) one board
never responds; (d) wrong ROI (flickering region, a region busy enough that only the sham-onset null control stops a
number, plain background); (e) audio moved 40 ms earlier
shifts the recovered latency by +40 ms; plus a wrong-speed recording refused by the SYNC fit.
"""
from __future__ import annotations

import csv
import json
import math
import os
import shutil
import stat
import struct
import subprocess
import sys
from pathlib import Path

import numpy as np
import pytest
import soundfile as sf

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import p5_click_track as ct  # noqa: E402
import p5_latency as pl  # noqa: E402

TOOL = ROOT / "scripts/p5_latency.py"
FREEZE = ROOT / "scripts/freeze_tolerance.py"
FFMPEG = pl.FFMPEG
FPS = 240
LEAD_S = 0.8
SKEW = 40e-6
K0, K1 = 9, 11  # planted latency in frames: board0 (lower), board1 (upper)
W, H = 192, 160
ROWS = {"busy": (4, 20), "board1": (24, 40), "flicker": (56, 72), "dead": (88, 104), "board0": (120, 136)}
ROI = {name: f"{16},{r0},{160},{r1 - r0}" for name, (r0, r1) in ROWS.items()}
BACKGROUND_ROI = "16,142,160,14"
PERIOD_MS = 1000.0 / FPS


def short_stimulus(n_clicks: int = 60, seed: int = 7) -> tuple[np.ndarray, dict]:
    rng = np.random.default_rng(seed)
    sr = ct.SR
    parts, pos = [], 0
    truth = {"sync_start": [], "clicks": [], "boom_start": None, "sync_end": []}

    def add(x):
        nonlocal pos
        start = pos
        parts.append(x)
        pos += len(x)
        return start

    add(np.zeros(sr))
    s, st = ct.sync(rng, 0.7)
    base = add(s)
    truth["sync_start"] = [base + k for k in st]
    add(np.zeros(sr))
    for g in rng.uniform(0.3, 1.2, size=n_clicks):
        truth["clicks"].append(add(ct.click(rng, 0.7)))
        add(np.zeros(int(round(g * sr)) - int(ct.CLICK_S * sr)))
    add(np.zeros(sr))
    s, st = ct.sync(rng, 0.7)
    base = add(s)
    truth["sync_end"] = [base + k for k in st]
    add(np.zeros(sr))
    mono = np.concatenate(parts)
    truth.update(sample_rate=sr, samples=len(mono), seconds=len(mono) / sr, n_clicks=n_clicks,
                 wav_sha256="test-stimulus-not-p5")
    return mono, truth


def recording_audio(mono: np.ndarray, sr_in: int, lead: float, dur: float, speed: float = 1.0) -> np.ndarray:
    sr = 48000
    t = np.arange(int(dur * sr)) / sr
    src = (t - lead) / ((1 + SKEW) * speed) * sr_in
    y = np.interp(src, np.arange(len(mono)), mono, left=0.0, right=0.0)
    y += np.random.default_rng(3).standard_normal(len(y)) * 10 ** (-66 / 20)
    return y


def ffmpeg(*args):
    r = subprocess.run([FFMPEG, "-hide_banner", "-nostdin", "-v", "error", "-y", *args],
                       capture_output=True, text=True)
    assert r.returncode == 0, r.stderr


def write_video(path: Path, wav: Path, onsets_rec: np.ndarray, dur: float):
    n = int(math.ceil(dur * FPS))
    t = np.arange(n) / FPS
    rng = np.random.default_rng(11)

    def flash(k):
        f = np.ceil(onsets_rec * FPS + k - 1e-9).astype(int)
        out = np.zeros(n)
        for fk in f:
            if fk < n:
                out[fk:] += np.exp(-(np.arange(n - fk)) / (0.08 * FPS))
        return out

    i0, i1 = flash(K0), flash(K1)
    # a one-frame +8 luma blip 2 frames after every 6th click on board0 (a codec/reflection glitch):
    # the photon hold check must reject it, so the planted 9-frame latency is still recovered
    i0[np.ceil(onsets_rec[::6] * FPS + 2 - 1e-9).astype(int)] += 8.0 / 91.0
    flick = (rng.random(n) < 3.0 / FPS).astype(float)  # random flashes, about 3 per second
    flick = np.convolve(flick, np.exp(-np.arange(24) / 6.0))[:n]
    busy = np.zeros(n)  # a strobe at about 4 Hz, unrelated to clicks: a rise in every 300 ms window
    busy[np.arange(7, n, 58)] = 1.0
    busy = np.convolve(busy, np.exp(-np.arange(12) / 2.0))[:n]
    base = np.zeros((H, W, 3), np.float32)
    base[:] = np.linspace(18, 34, W, dtype=np.float32)[None, :, None]
    gold_on = np.array([40, 32, 10], np.float32)
    gold_d = np.array([120, 90, 20], np.float32)
    noise = rng.standard_normal((97, H, W, 1)).astype(np.float32) * 1.5
    glob = rng.standard_normal(n).astype(np.float32) * 0.4
    cmd = [FFMPEG, "-hide_banner", "-nostdin", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
           "-s", f"{W}x{H}", "-r", str(FPS), "-i", "-", "-i", str(wav), "-map", "0:v", "-map", "1:a",
           "-c:v", "libx264", "-preset", "ultrafast", "-crf", "14", "-pix_fmt", "yuv420p",
           "-c:a", "aac", "-b:a", "192k", str(path)]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
    for a in range(0, n, 240):
        b = min(n, a + 240)
        fr = np.repeat(base[None], b - a, axis=0)
        for name, inten in (("board0", i0), ("board1", i1), ("flicker", flick * 1.2), ("busy", np.minimum(busy, 1.2)),
                            ("dead", np.zeros(n))):
            r0, r1 = ROWS[name]
            fr[:, r0:r1, 16:176, :] = gold_on + inten[a:b, None, None, None] * gold_d
        fr += noise[np.arange(a, b) % 97] + glob[a:b, None, None, None]
        proc.stdin.write(np.clip(np.rint(fr), 0, 255).astype(np.uint8).tobytes())
    proc.stdin.close()
    assert proc.wait() == 0, proc.stderr.read().decode()
    return t


def run_tool(out: Path, video: Path, truth: Path, rules: Path, rois: list[str], *extra):
    cmd = [sys.executable, str(TOOL), "analyse", "--video", str(video), "--truth", str(truth),
           "--rules", str(rules), "--out", str(out), *extra]
    for r in rois:
        cmd += ["--roi", r]
    r = subprocess.run(cmd, capture_output=True, text=True)
    rep = out / "report.json"
    ref = out / "refusal.json"
    return (r.returncode, json.loads(rep.read_text()) if rep.exists() else None,
            json.loads(ref.read_text()) if ref.exists() else None, r.stderr)


def patch_video_elst(src: Path, dst: Path, rate: float = 0.125):
    """Turn the video edit into a slowed (rate 1/8, 8x segment) edit, like an iPhone Slo-mo original."""
    data = bytearray(src.read_bytes())
    vt = next(t for t in pl.parse_tracks(src)["tracks"] if t["handler"] == "vide")
    p = vt["elst_at"]
    assert p is not None, "the muxer wrote no video edit list"
    ver, count = data[p], struct.unpack(">I", data[p + 4:p + 8])[0]
    q = p + 8
    esz = 20 if ver == 1 else 12
    for _ in range(count):
        mt = struct.unpack(">q" if ver == 1 else ">i", data[q + (8 if ver == 1 else 4):q + (16 if ver == 1 else 8)])[0]
        if mt != -1:
            if ver == 1:
                seg = struct.unpack(">Q", data[q:q + 8])[0]
                data[q:q + 8] = struct.pack(">Q", seg * 8)
            else:
                seg = struct.unpack(">I", data[q:q + 4])[0]
                data[q:q + 4] = struct.pack(">I", seg * 8)
            r_at = q + esz - 4
            data[r_at:r_at + 4] = struct.pack(">hH", int(rate), int(round((rate % 1) * 65536)))
            break
        q += esz
    dst.write_bytes(bytes(data))


@pytest.fixture(scope="module")
def media(tmp_path_factory):
    d = tmp_path_factory.mktemp("p5")
    mono, truth = short_stimulus()
    sr_in = truth["sample_rate"]
    dur = LEAD_S + len(mono) / sr_in * (1 + SKEW) + 0.5
    truth_path = d / "short.truth.json"
    truth_path.write_text(json.dumps(truth))
    onsets_rec = LEAD_S + np.array(truth["clicks"]) / sr_in * (1 + SKEW)
    wav = d / "rec.wav"
    sf.write(wav, recording_audio(mono, sr_in, LEAD_S, dur), 48000, subtype="FLOAT")
    base = d / "base.mov"
    write_video(base, wav, onsets_rec, dur)
    # (e) the same video with the audio moved 40 ms earlier
    wav_e = d / "rec-early40.wav"
    sf.write(wav_e, recording_audio(mono, sr_in, LEAD_S - 0.040, dur), 48000, subtype="FLOAT")
    early = d / "early40.mov"
    ffmpeg("-i", str(base), "-i", str(wav_e), "-map", "0:v", "-map", "1:a", "-c:v", "copy", "-c:a", "aac",
           "-b:a", "192k", str(early))
    # (a) 120 fps, otherwise identical
    f120 = d / "f120.mov"
    ffmpeg("-i", str(base), "-vf", "fps=120", "-c:v", "libx264", "-preset", "ultrafast", "-c:a", "copy", str(f120))
    # (b) video timeline 8x the audio (time-remapped export)
    stretched = d / "stretched8x.mov"
    ffmpeg("-f", "lavfi", "-i", f"color=c=gray:s={W}x{H}:r={FPS}:d={8 * 3}", "-f", "lavfi", "-i",
           "sine=f=200:r=48000:d=3", "-c:v", "libx264", "-preset", "ultrafast", "-c:a", "aac", str(stretched))
    # iPhone-Slo-mo-like original: slowed edit list over real-time media
    slomo = d / "slomo-editlist.mov"
    patch_video_elst(base, slomo)
    # rules: proposed by the tool, plus a frozen copy
    rules = d / "rules.json"
    subprocess.run([sys.executable, str(TOOL), "rules", "--out", str(rules)], check=True, capture_output=True)
    frozen = d / "rules-frozen.json"
    shutil.copy(rules, frozen)
    subprocess.run([sys.executable, str(FREEZE), "freeze", str(frozen), "--lane", "p5-selftest"], check=True,
                   capture_output=True)
    # synthetic device timing: 7.5 ms hops on a skewed device clock, transient spikes at sync + clicks
    dev_path = d / "timing.jsonl"
    events_rec = LEAD_S + np.array([*truth["sync_start"], *truth["clicks"], *truth["sync_end"]]) / sr_in * (1 + SKEW)
    dev_of = lambda rec_t: 5.0 + rec_t * (1 + 25e-6)  # noqa: E731
    hops = np.arange(5.0, dev_of(dur), 0.0075)
    lvl = 0.01 + 0.002 * np.random.default_rng(5).random(len(hops))
    lvl[np.searchsorted(hops, dev_of(events_rec), side="right") - 1] = 1.0
    with dev_path.open("w") as f:
        for k, (h, v) in enumerate(zip(hops, lvl)):
            f.write(json.dumps({"write_seq": k, "capture_time_us": int(h * 1e6),
                                "publish_time_us": int((h + 0.0135) * 1e6), "transient": float(v),
                                "_host_read_s": -1.0, "_music_s": -1.0}) + "\n")
    return {"dir": d, "truth": truth_path, "truth_obj": truth, "mono": mono, "base": base, "early": early,
            "f120": f120, "stretched": stretched, "slomo": slomo, "rules": rules, "frozen": frozen,
            "timing": dev_path, "dur": dur}


def _p50(rep, board):
    return rep["boards"][board]["latency"]["p50_ms"]


def _assert_planted(rep):
    for board, k in (("board0", K0), ("board1", K1)):
        b = rep["boards"][board]
        assert b["status"] == "MEASURED", b
        assert b["misses"] == 0 and b["hits"] == 60, b
        # the light changed within the frame after K frames: latency in [K, K+1) frames, +/- one frame
        assert abs(b["latency"]["p50_ms"] - (k + 0.5) * PERIOD_MS) <= PERIOD_MS, b["latency"]
        assert b["latency"]["min_ms"] >= (k - 1) * PERIOD_MS and b["latency"]["max_ms"] <= (k + 2) * PERIOD_MS


def test_planted_latency_recovered_with_frozen_rules_and_device_alignment(media):
    out = media["dir"] / "out-base"
    rc, rep, ref, err = run_tool(out, media["base"], media["truth"], media["frozen"],
                                 [f"board0={ROI['board0']}", f"board1={ROI['board1']}"],
                                 "--device-timing", str(media["timing"]))
    assert rc == 0 and ref is None, (err, ref)
    _assert_planted(rep)
    assert rep["rules"]["frozen"] and rep["rules"]["tool_matches_rules"]
    assert rep["timeline"]["kind"].startswith("container")
    fit = rep["alignment"]
    assert abs(fit["a_s"] - LEAD_S) < 0.001, fit  # the recording's own audio places the stimulus at 0.8 s
    assert abs(fit["skew_ppm"] - SKEW * 1e6) < 20, fit
    assert rep["onsets"]["onset_misses"] == 0
    # audio onsets land on the planted acoustic arrival (AAC pre-echo guard: 0.90 within 0.5 ms; 0.65 without)
    truth = media["truth_obj"]
    with (out / "events.csv").open() as f:
        rows = [r for r in csv.DictReader(f) if r["board"] == "board0"]
    onset_err = np.array([(float(r["onset_s"]) - (LEAD_S + int(r["truth_sample"]) / truth["sample_rate"] * (1 + SKEW)))
                          * 1e3 for r in rows])
    assert abs(np.median(onset_err)) <= 0.2 and np.mean(np.abs(onset_err) <= 0.5) >= 0.85, np.sort(onset_err)[:8]
    for b in ("board0", "board1"):
        assert rep["boards"][b]["null_hits"] == 0 and rep["boards"][b]["null_trials"] > 10
    # not admissible as P5 evidence: test stimulus, and N < 500
    assert rep["p5_admissible"] is False
    assert any("not the P5 click track" in w for w in rep["p5_admissible_reasons"])
    dev = rep["device_alignment"]
    assert dev["available"] is True, dev
    assert max(abs(r) for r in dev["fit_device_s"]["residuals_ms"]) <= 7.5
    assert abs(dev["median_ms"]["capture_to_publish_ms"] - 13.5) < 0.5, dev["median_ms"]
    for b in ("board0", "board1"):  # per click: onset->capture + capture->publish + publish->photon = latency
        assert dev["median_ms"][f"publish_to_photon_{b}_ms"] is not None, dev["median_ms"]
    row = next(r for r in dev["per_click"] if "publish_to_photon_board0_ms" in r)
    parts = row["onset_to_capture_ms"] + row["capture_to_publish_ms"] + row["publish_to_photon_board0_ms"]
    with (out / "events.csv").open() as f:
        lat = next(float(r["latency_ms"]) for r in csv.DictReader(f)
                   if r["board"] == "board0" and int(r["click"]) == row["click"])
    assert abs(parts - lat) < 1e-3, (parts, lat)
    for name in ("events.csv", "latency.png", "roi-check.png"):
        assert (out / name).stat().st_size > 0


def test_slomo_edit_list_is_handled_explicitly(media):
    rc, rep, ref, err = run_tool(media["dir"] / "out-slomo", media["slomo"], media["truth"], media["rules"],
                                 [f"board0={ROI['board0']}", f"board1={ROI['board1']}"], "--allow-unfrozen-rules")
    assert rc == 0, (err, ref)
    assert rep["timeline"]["kind"].startswith("media")
    vt = rep["timeline"]["video_track"]
    assert any(abs(e["media_rate"] - 0.125) < 1e-6 for e in vt["edits"])
    _assert_planted(rep)
    assert rep["p5_admissible"] is False and "rules not frozen" in rep["p5_admissible_reasons"]


def test_a_120fps_refused(media):
    rc, rep, ref, _ = run_tool(media["dir"] / "out-120", media["f120"], media["truth"], media["rules"],
                               [f"board0={ROI['board0']}"], "--allow-unfrozen-rules")
    assert rc == 2 and rep is None
    assert ref["stage"] == "media" and "240 fps" in ref["reason"], ref


def test_b_video_stretched_8x_refused(media):
    rc, rep, ref, _ = run_tool(media["dir"] / "out-8x", media["stretched"], media["truth"], media["rules"],
                               [f"board0={ROI['board0']}"], "--allow-unfrozen-rules")
    assert rc == 2 and rep is None
    assert ref["stage"] == "media" and "time-remapped" in ref["reason"] and "8.00x" in ref["reason"], ref


def test_c_board_never_responding_claims_no_latency(media):
    rc, rep, ref, _ = run_tool(media["dir"] / "out-dead", media["base"], media["truth"], media["rules"],
                               [f"board0={ROI['board0']}", f"board1={ROI['dead']}"], "--allow-unfrozen-rules")
    assert rc == 3 and ref is None
    b = rep["boards"]["board1"]
    assert b["status"] == "NO_CONFIDENT_LATENCY" and b["hits"] == 0 and b["misses"] == 60
    assert b["latency"] is None and b["latency_claimed"] is False
    assert rep["boards"]["board0"]["status"] == "MEASURED"


@pytest.mark.parametrize("roi", [ROI["flicker"], ROI["busy"], BACKGROUND_ROI],
                         ids=["flickering-region", "busy-region-null-control", "background"])
def test_d_wrong_roi_gives_no_confident_number(media, roi):
    rc, rep, ref, _ = run_tool(media["dir"] / f"out-wrong-{roi.replace(',', '_')}", media["base"], media["truth"],
                               media["rules"], [f"board0={roi}"], "--allow-unfrozen-rules")
    assert rc in (2, 3), rc
    if rc == 3:
        b = rep["boards"]["board0"]
        assert b["status"] == "NO_CONFIDENT_LATENCY" and b["latency"] is None, b
        if roi == ROI["busy"]:  # rises in nearly every window: only the null control can stop a number
            assert b["hit_fraction"] >= 0.95 and any("null-control" in r for r in b["reasons"]), b


def test_e_audio_offset_shifts_latency_by_40ms(media):
    rois = [f"board0={ROI['board0']}", f"board1={ROI['board1']}"]
    rc0, rep0, *_ = run_tool(media["dir"] / "out-e-ref", media["base"], media["truth"], media["rules"], rois,
                             "--allow-unfrozen-rules")
    rc1, rep1, ref1, err = run_tool(media["dir"] / "out-e", media["early"], media["truth"], media["rules"], rois,
                                    "--allow-unfrozen-rules")
    assert rc0 == 0 and rc1 == 0, (err, ref1)
    for b in ("board0", "board1"):
        shift = _p50(rep1, b) - _p50(rep0, b)
        assert abs(shift - 40.0) <= PERIOD_MS, (b, shift)
    assert abs(rep1["alignment"]["a_s"] - (LEAD_S - 0.040)) < 0.001


def test_unfrozen_or_edited_rules_refused(media, tmp_path):
    rois = [f"board0={ROI['board0']}"]
    rc, _, ref, _ = run_tool(tmp_path / "o1", media["base"], media["truth"], media["rules"], rois)
    assert rc == 2 and ref["stage"] == "rules" and "not frozen" in ref["reason"]
    edited = tmp_path / "edited.json"
    shutil.copy(media["frozen"], edited)
    edited.chmod(stat.S_IRUSR | stat.S_IWUSR)
    data = json.loads(edited.read_text())
    data["photon"]["k_sigma"] = 3.0
    edited.write_text(json.dumps(data))
    edited.chmod(stat.S_IRUSR)
    rc, _, ref, _ = run_tool(tmp_path / "o2", media["base"], media["truth"], edited, rois)
    assert rc == 2 and ref["stage"] == "rules" and "verification" in ref["reason"], ref


def test_wrong_speed_recording_refused_by_sync_fit(media):
    rules = json.loads(media["rules"].read_text())
    truth = media["truth_obj"]
    x = recording_audio(media["mono"], truth["sample_rate"], LEAD_S, media["dur"] * 1.02, speed=1.01)
    with pytest.raises(pl.Refusal) as ei:
        pl.align({"x": x, "sr": 48000, "t0": 0.0}, truth, rules)
    assert "does not run at real time" in ei.value.reason


def test_device_alignment_not_faked_without_audio_field(tmp_path, media):
    p = tmp_path / "timing.jsonl"
    p.write_text("".join(json.dumps({"write_seq": k, "capture_time_us": 1000 * k, "publish_time_us": 1000 * k + 6,
                                     "_host_read_s": k / 1e3, "_music_s": k / 1e3}) + "\n" for k in range(100)))
    rules = json.loads(media["rules"].read_text())
    out = pl.device_alignment(p, media["truth_obj"], {"a_s": 0.0, "b": 1.0}, [], rules)
    assert out["available"] is False and "no audio-derived" in out["reason"]
    assert "_music_s" in out["reason"]
