#!/usr/bin/env python3
"""Lane P5: sound-to-light latency from an iPhone 240 fps recording (EXECUTION-BRIEF §12 P5).

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. This tool is host-only: it reads a recording, the
stimulus truth and (optionally) device timing records. It never touches a device.

  p5_latency.py rules --out RULES.json
      Write the rule set this exact tool version applies (thresholds, windows, refusals,
      bound to this file's SHA-256). Freeze it with scripts/freeze_tolerance.py before any
      real recording is analysed.

  p5_latency.py analyse --video REC.mov --truth STIM.truth.json --rules RULES.json
                        --roi board0=x,y,w,h --roi board1=x,y,w,h --out DIR
                        [--device-timing timing.jsonl] [--allow-unfrozen-rules]

Pipeline (every rule value comes from the rules file; nothing is tuned on the recording):
  1. Media checks. Parse the container's edit lists. Identity edit lists use the container
     timeline; a time-remapping edit list (rate != 1 or several media segments, as in an iPhone
     Slo-mo original) is handled explicitly: both tracks are read on their real-time media
     timelines, each shifted by its first edit. Refuse unless the video is ~240 fps
     (r_frame_rate, avg_frame_rate, decoded pts spacing) and the video and audio last equally
     long (a slowed export is refused). An iOS-rendered export (30 fps, frames re-timed onto a
     grid, tracks equally long) is refused by name: its frame times are not capture times.
  2. Alignment (v2). Clicks are identified by their seeded gap pattern: an envelope event anchors
     click k when the next N clicks' truth gaps all land on recording events. A robust line through
     the anchors identifies the stimulus (refused if too few clicks are identified or |b - 1| is
     out of rule). Each click's onset (step 3) is then mapped on a local clock: a least-squares
     line through the on-clock onsets within +/-20 s of truth time, so a playback chain whose delay
     drifts while it wakes up is followed. Onsets off that clock are excluded and counted; clicks
     before a declared settle time are excluded and counted. The SYNC triplets are a coarse,
     report-only anchor: never used for timing, never required.
  3. Onset = first recording-audio sample above threshold inside +/-60 ms of each click's
     mapped time; threshold from the 100 ms before that window.
  4. Photon = first frame after the onset whose ROI mean Rec.709 luma (Y' = 0.2126 R' +
     0.7152 G' + 0.0722 B', on decoded 8-bit R'G'B') rises frame-to-frame by more than a
     threshold set from the slope noise in the 300 ms before the onset. Search onset to
     onset + 300 ms. No rise is a MISS, counted and reported.
  5. Latency = photon frame time - onset time, per board, with a sham-onset null control.
     BOOM is excluded: only the truth file's clicks are scored.

Exit codes: 0 analysed and every board has a confident latency; 2 refused (media, alignment,
rules); 3 analysed but at least one board has no confident latency (no number is claimed).
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
import shutil
import struct
import subprocess
import sys
import threading
from dataclasses import dataclass
from datetime import datetime, timezone
from fractions import Fraction
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
FFMPEG = shutil.which("ffmpeg") or "/opt/homebrew/bin/ffmpeg"
FFPROBE = shutil.which("ffprobe") or "/opt/homebrew/bin/ffprobe"
P5_STIMULUS_SHA256 = "638c7fa9eda058e3bcfef318b9e899fefec26be9c0b178ec4cec3adc943338a5"
RULES_SCHEMA = "p5-latency-rules/2"
REC709 = (0.2126, 0.7152, 0.0722)


class Refusal(Exception):
    """A rule says this input cannot yield a trustworthy latency."""

    def __init__(self, stage: str, reason: str, **detail):
        super().__init__(f"{stage}: {reason}")
        self.stage, self.reason, self.detail = stage, reason, detail


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 22), b""):
            h.update(chunk)
    return h.hexdigest()


def tool_sha256() -> str:
    return sha256_file(Path(__file__).resolve())


def proposed_rules() -> dict:
    """Every threshold, window and refusal rule the analysis applies. Proposed, not tuned on data."""
    return {
        "schema": RULES_SCHEMA,
        "status": "PROPOSED — freeze with scripts/freeze_tolerance.py before analysing any real recording",
        "bound_tool": "scripts/p5_latency.py",
        "bound_tool_sha256": tool_sha256(),
        "p5_stimulus_wav_sha256": P5_STIMULUS_SHA256,
        "brief_min_onsets": 500,
        "media": {
            "nominal_fps": 240.0,
            "fps_rel_tol": 0.03,
            "rule_fps": "REFUSE unless r_frame_rate, avg_frame_rate and the median decoded pts step "
                        "are each within fps_rel_tol of nominal_fps (a 120 fps or 30 fps file is refused). A file "
                        "whose frame rate is nominal_fps / an integer >= 4 and whose tracks last equally long is "
                        "named as an iOS-rendered (re-timed) export in the refusal: its frames sit on a resampled "
                        "grid (a 239.69 fps camera on a 240 fps grid repeats one frame every ~769), so the "
                        "one-frame timing bound below holds for real-time pts input only",
            "pts_irregular_band": [0.5, 1.5],
            "pts_irregular_max_fraction": 0.005,
            "rule_pts": "REFUSE if more than pts_irregular_max_fraction of decoded frame steps fall "
                        "outside pts_irregular_band x the nominal frame period",
            "av_duration_max_diff_s": 0.5,
            "av_duration_max_diff_rel": 0.02,
            "rule_av_duration": "REFUSE if |video span - audio span| > max(av_duration_max_diff_s, "
                                "av_duration_max_diff_rel x audio span) on the timeline used "
                                "(a time-remapped slow-motion export is refused)",
            "rule_edit_list": "Parse every track's elst. Identity (at most leading empty edits plus one "
                              "media edit at rate 1.0) on both used tracks: container timeline (edit lists "
                              "applied by the demuxer). Otherwise (rate != 1 or several media edits, as in an "
                              "iPhone Slo-mo original): media timeline (edit lists ignored) with each track "
                              "shifted by its first media edit: t = media_t + empty_lead_s - media_time_s. "
                              "REFUSE a non-identity edit list on a track with no media edit.",
            "audio_codecs_accepted": ["aac", "alac", "pcm_s16le", "pcm_s16be", "pcm_s24le", "pcm_s24be",
                                      "pcm_f32le", "pcm_f32be", "mp3"],
            "audio_max_discontinuity_s": 0.001,
            "rule_audio_continuity": "REFUSE if consecutive decoded audio frames' timestamps differ from "
                                     "their sample count by more than audio_max_discontinuity_s",
            "roi_min_pixels": 64,
        },
        "alignment": {
            "envelope_bin_s": 0.001,
            "envelope_k_median": 10.0,
            "envelope_frac_p999": 0.1,
            "rule_envelope": "1 ms max|x| bins; event threshold = max(envelope_k_median x median, "
                             "envelope_frac_p999 x 99.9th percentile); an event is a bin above threshold "
                             "preceded by at least event_quiet_s of bins below it",
            "event_quiet_s": 0.030,
            "fingerprint_clicks": 8,
            "fingerprint_tol_s": 0.010,
            "fingerprint_speed_tol": 0.02,
            "rule_fingerprint": "anchor (k, e): envelope event e such that for every j < fingerprint_clicks an "
                                "event lies within fingerprint_tol_s + fingerprint_speed_tol x (tc[k+j] - tc[k]) "
                                "of e + tc[k+j] - tc[k]; of several such e the one with the least summed error",
            "identify_line": "Theil-Sen line e = a + b x tc over the anchors; inlier anchors lie within "
                             "identify_inlier_tol_s of it",
            "identify_inlier_tol_s": 0.060,
            "min_identified_fraction": 0.5,
            "max_abs_slope_error": 0.002,
            "rule_identify": "REFUSE unless inlier anchors >= min_identified_fraction of the clicks that can "
                             "anchor (the gap pattern is not this stimulus, or is shifted), and |b - 1| <= "
                             "max_abs_slope_error (wrong speed or time remap; v1 value)",
            "clock_local_halfwidth_s": 20.0,
            "clock_outlier_s": 0.010,
            "clock_iterations": 3,
            "rule_clock": "declared before any recording: piecewise-smooth clock, not one line. mapped(k) = least "
                          "squares line through the on-clock onsets of clicks with |tc - tc[k]| <= "
                          "clock_local_halfwidth_s, evaluated at tc[k] (the identify line where fewer than 3 such "
                          "onsets exist); on-clock = |onset - mapped| <= clock_outlier_s; iterated "
                          "clock_iterations times starting from all onsets. An off-clock onset is excluded from "
                          "pairing (OFF_CLOCK, counted): it is not this click's arrival (e.g. a noise burst inside "
                          "the window). Reason: a playback chain can drift by tens of ms while it wakes up; a "
                          "single line would misplace windows, and the drift is common to the phone and the Titan "
                          "(both hear the same speaker), so it is not latency.",
            "max_off_clock_fraction": 0.05,
            "settle_after_sync_start_s": 10.0,
            "rule_settle": "declared from the stimulus truth, before any result: clicks whose truth time is earlier "
                           "than the first SYNC burst + settle_after_sync_start_s are excluded (SETTLE_EXCLUDED, "
                           "counted) so a speaker or route that is still waking up cannot contribute events. For "
                           "the P5 stimulus this excludes clicks 0-11 (truth < 12.0 s) and leaves 508 clicks: at "
                           "most 8 may miss for the 500-pair brief minimum.",
            "triplet_spacing_tol_s": 0.005,
            "sync_coarse_tol_s": 0.025,
            "sync_search_s": 2.0,
            "sync_anchor_tol_s": 0.100,
            "rule_sync": "coarse identity anchor only, never timing, never required. Triplets are counted under the "
                         "v1 rule (triplet_spacing_tol_s) for the report; coarse triplets use sync_coarse_tol_s. "
                         "For the start and the end SYNC, the expected recording time is the local clock "
                         "extrapolated to the burst's truth time; if a coarse triplet starts within sync_search_s "
                         "of it, the nearest must lie within sync_anchor_tol_s, else REFUSE (clicks identified at a "
                         "shifted offset; the shortest click gap is 0.3 s). No triplet nearby = not found, "
                         "reported, not a refusal.",
        },
        "onset": {
            "window_s": 0.060,
            "noise_s": 0.100,
            "k_rms": 8.0,
            "k_peak": 2.0,
            "floor_dbfs": -60.0,
            "pre_echo_guard_db": -30.0,
            "rule": "x = decoded audio downmixed to mono; window = mapped +/- window_s; noise = the noise_s "
                    "before the window; dc = mean(noise); threshold = max(k_rms x rms(noise - dc), "
                    "k_peak x max|noise - dc|, 10^(floor_dbfs/20), 10^(pre_echo_guard_db/20) x "
                    "max|window - dc|); onset = first window sample with |x - dc| > threshold; none = "
                    "ONSET_MISS (event excluded, counted). The pre-echo guard keeps AAC pre-echo (measured "
                    "on the self-test: up to 0.7 ms early at -40 dB re peak, none at -30 dB) from "
                    "becoming the onset.",
            "max_onset_miss_fraction": 0.05,
            "max_median_abs_onset_residual_s": 0.003,
            "rule_clock_check": "over the clicks not settle-excluded: REFUSE if ONSET_MISS exceeds "
                                "max_onset_miss_fraction, OFF_CLOCK exceeds alignment.max_off_clock_fraction, or the "
                                "median |onset - mapped| over all found onsets exceeds "
                                "max_median_abs_onset_residual_s",
        },
        "photon": {
            "luma": "Rec.709 Y' = 0.2126 R' + 0.7152 G' + 0.0722 B' on decoded 8-bit R'G'B' (gamma-encoded), "
                    "mean over the ROI per frame; ROI in displayed (auto-rotated) frame pixels",
            "noise_s": 0.300,
            "search_s": 0.300,
            "k_sigma": 6.0,
            "min_rise": 0.5,
            "rule": "slope[n] = Y[n] - Y[n-1]; noise slopes = frames with onset - noise_s <= t < onset; "
                    "sigma = 1.4826 x MAD; threshold = median + max(k_sigma x sigma, min_rise); photon = "
                    "first frame n with onset < t[n] <= onset + search_s, slope[n] > threshold and the rise "
                    "holding one frame later (Y[n+1] - Y[n-1] > threshold); none = MISS. Rises only: a "
                    "darkening response is a MISS; a response shorter than one frame is a MISS. The hold "
                    "check rejects one-frame codec blips (self-test: +1.20 then -1.21 luma, 6.3 sigma).",
            "latency": "photon frame pts - onset time",
            "null_control": "sham onset at next_click_onset - search_s for every gap >= 2 x search_s; same "
                            "detector; a hit there is a false trigger",
            "early_check_s": 0.060,
            "early_check": "report-only: rises above threshold within early_check_s before the onset "
                           "(light preceding sound suggests an audio/video offset)",
        },
        "board_status": {
            "min_hit_fraction": 0.95,
            "min_hits": 30,
            "max_null_hit_fraction": 0.05,
            "rule": "MEASURED only if hits/valid onsets >= min_hit_fraction, hits >= min_hits and null "
                    "hits/null trials <= max_null_hit_fraction; otherwise NO_CONFIDENT_LATENCY and no "
                    "latency statistic is reported for that board",
        },
        "device_alignment": {
            "level_fields": ["transient", "flux", "onset_strength", "bass_onset", "level", "rms"],
            "event_k_mad": 8.0,
            "event_quiet_s": 0.030,
            "triplet_spacing_tol_s": 0.012,
            "max_fit_residual_s": 0.015,
            "rule": "Only with an audio-derived per-hop field: locate both SYNC triplets in device "
                    "capture time, fit truth_s -> device_s, compose with the recording fit. Otherwise "
                    "report NOT AVAILABLE with the reason; host-clock fields (_host_read_s, _music_s) are "
                    "never used for alignment.",
        },
        "quantisation": {
            "per_event_ms": 1000.0 / 240.0,
            "statement": "real-time pts input only (re-timed exports are refused). "
                         "Each latency is a frame pts minus an audio onset; the light changed within the "
                         "frame period ending at the detected frame, so every event carries one frame "
                         "(4.17 ms at 240 fps) of quantisation; its random part averages out over N "
                         "(about 4.17/sqrt(12 N) ms on the mean) but it biases latency late by 0 to 1 frame",
        },
    }


# ---------------------------------------------------------------- container / edit lists

def iter_boxes(buf: bytes, start: int, end: int):
    """Yield (type, payload_start, box_end, box_start) for ISO-BMFF boxes in buf[start:end]."""
    pos = start
    while pos + 8 <= end:
        size, typ = struct.unpack(">I4s", buf[pos:pos + 8])
        hdr = 8
        if size == 1:
            size = struct.unpack(">Q", buf[pos + 8:pos + 16])[0]
            hdr = 16
        elif size == 0:
            size = end - pos
        if size < hdr or pos + size > end:
            raise Refusal("media", f"malformed box {typ!r} at {pos}")
        yield typ, pos + hdr, pos + size, pos
        pos += size


def read_moov(path: Path) -> tuple[bytes, int]:
    with path.open("rb") as f:
        f.seek(0, 2)
        n = f.tell()
        pos = 0
        while pos + 8 <= n:
            f.seek(pos)
            h = f.read(16)
            size, typ = struct.unpack(">I4s", h[:8])
            hdr = 8
            if size == 1:
                size, hdr = struct.unpack(">Q", h[8:16])[0], 16
            elif size == 0:
                size = n - pos
            if size < hdr:
                break
            if typ == b"moov":
                f.seek(pos)
                return f.read(size), pos
            pos += size
    raise Refusal("media", "no moov box: not a QuickTime/MP4 container")


def _timescale(buf: bytes, p: int) -> tuple[int, int]:
    """(timescale, duration) of an mvhd/mdhd payload at p."""
    if buf[p] == 1:
        return struct.unpack(">IQ", buf[p + 20:p + 32])
    return struct.unpack(">II", buf[p + 12:p + 20])


def parse_tracks(path: Path) -> dict:
    moov, moov_at = read_moov(path)
    movie_ts, tracks = None, []
    for typ, p, e, _ in iter_boxes(moov, 8, len(moov)):
        if typ == b"mvhd":
            movie_ts = _timescale(moov, p)[0]
        if typ != b"trak":
            continue
        tr = {"track_id": None, "handler": None, "media_timescale": None, "edits": [], "elst_at": None}
        for t2, p2, e2, _ in iter_boxes(moov, p, e):
            if t2 == b"tkhd":
                off = 20 if moov[p2] == 1 else 12
                tr["track_id"] = struct.unpack(">I", moov[p2 + off:p2 + off + 4])[0]
            elif t2 == b"edts":
                for t3, p3, e3, s3 in iter_boxes(moov, p2, e2):
                    if t3 != b"elst":
                        continue
                    ver = moov[p3]
                    count = struct.unpack(">I", moov[p3 + 4:p3 + 8])[0]
                    q = p3 + 8
                    tr["elst_at"] = moov_at + p3
                    for _ in range(count):
                        if ver == 1:
                            seg, mt = struct.unpack(">Qq", moov[q:q + 16])
                            q += 16
                        else:
                            seg, mt = struct.unpack(">Ii", moov[q:q + 8])
                            q += 8
                        ri, rf = struct.unpack(">hH", moov[q:q + 4])
                        q += 4
                        tr["edits"].append({"segment_duration": seg, "media_time": mt,
                                            "media_rate": ri + rf / 65536.0})
            elif t2 == b"mdia":
                for t3, p3, e3, _ in iter_boxes(moov, p2, e2):
                    if t3 == b"mdhd":
                        tr["media_timescale"] = _timescale(moov, p3)[0]
                    elif t3 == b"hdlr":
                        tr["handler"] = moov[p3 + 8:p3 + 12].decode("latin-1")
        tracks.append(tr)
    if movie_ts is None:
        raise Refusal("media", "moov has no mvhd")
    for tr in tracks:
        tr.update(edit_policy(tr, movie_ts))
    return {"movie_timescale": movie_ts, "tracks": tracks}


def edit_policy(tr: dict, movie_ts: int) -> dict:
    edits = tr["edits"]
    if not edits:
        return {"edit_identity": True, "media_offset_s": 0.0, "first_media_edit": None}
    lead = 0
    first = None
    for e in edits:
        if e["media_time"] == -1 and first is None:
            lead += e["segment_duration"]
        elif e["media_time"] != -1 and first is None:
            first = e
    media = [e for e in edits if e["media_time"] != -1]
    identity = len(media) == 1 and abs(media[0]["media_rate"] - 1.0) < 1e-9 and \
        all(e["media_time"] == -1 for e in edits[:edits.index(media[0])])
    if first is None:
        return {"edit_identity": identity, "media_offset_s": None, "first_media_edit": None}
    off = lead / movie_ts - first["media_time"] / (tr["media_timescale"] or movie_ts)
    return {"edit_identity": identity, "media_offset_s": off, "first_media_edit": first}


def ffprobe(path: Path, media_timeline: bool) -> dict:
    cmd = [FFPROBE, "-v", "error", *(["-ignore_editlist", "1"] if media_timeline else []),
           "-show_format", "-show_streams", "-of", "json", str(path)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise Refusal("media", "ffprobe failed", stderr=r.stderr[-2000:])
    return json.loads(r.stdout)


def rate(s: str | None) -> float | None:
    if not s or s in ("0/0", "N/A"):
        return None
    f = Fraction(s)
    return float(f) if f else None


# ---------------------------------------------------------------- decoding

_FRAME = re.compile(r"\bn:\s*(\d+)\s+pts:\s*(-?\d+)\s+pts_time:\s*(\S+)")
_ARATE = re.compile(r"rate:(\d+)\s+nb_samples:(\d+)")
_TB = re.compile(r"config in time_base:\s*(\d+)/(\d+)")


def _drain(stream, sink: list):
    for raw in iter(stream.readline, b""):
        sink.append(raw.decode("utf-8", "replace"))


def decode_audio(path: Path, stream_index: int, media_timeline: bool, rules: dict) -> dict:
    cmd = [FFMPEG, "-hide_banner", "-nostdin", "-v", "info",
           *(["-ignore_editlist", "1"] if media_timeline else []), "-copyts", "-i", str(path),
           "-map", f"0:{stream_index}", "-af", "ashowinfo,aformat=sample_fmts=flt:channel_layouts=mono",
           "-f", "f32le", "-"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    lines: list[str] = []
    th = threading.Thread(target=_drain, args=(proc.stderr, lines), daemon=True)
    th.start()
    pcm = proc.stdout.read()
    proc.wait()
    th.join()
    if proc.returncode != 0:
        raise Refusal("media", "audio decode failed", stderr="".join(lines[-20:]))
    frames = []
    for ln in lines:
        if "ashowinfo" not in ln:
            continue
        m, a = _FRAME.search(ln), _ARATE.search(ln)
        if m and a:
            frames.append((int(m.group(2)), float(m.group(3)), int(a.group(1)), int(a.group(2))))
    if not frames:
        raise Refusal("media", "audio decode produced no frames")
    x = np.frombuffer(pcm, dtype="<f4").astype(np.float64)
    sr = frames[0][2]
    if any(f[2] != sr for f in frames):
        raise Refusal("media", "audio sample rate changes mid-stream")
    if sum(f[3] for f in frames) != len(x):
        raise Refusal("media", "audio sample count does not match the decoded frames")
    # exact frame times: integer pts in 1/sr ticks, checked against the printed (6-digit) pts_time
    tb = 1.0 / sr
    if not all(abs(f[0] * tb - f[1]) <= max(2e-6 * abs(f[1]), 1e-6) for f in frames):
        raise Refusal("media", "audio filter time base is not 1/sample_rate; frames cannot be timed exactly")
    starts = np.array([f[0] * tb for f in frames])
    cum = np.concatenate([[0], np.cumsum([f[3] for f in frames])[:-1]]) / sr
    disc = np.abs((starts - starts[0]) - cum)
    lim = rules["media"]["audio_max_discontinuity_s"]
    if disc.max() > lim:
        raise Refusal("media", f"audio track is discontinuous by {disc.max() * 1e3:.3f} ms; "
                               f"its sample index is not a clock", limit_s=lim)
    return {"x": x, "sr": sr, "t0": float(starts[0]), "frames": len(frames),
            "max_discontinuity_s": float(disc.max())}


@dataclass
class Roi:
    name: str
    x: int
    y: int
    w: int
    h: int


def decode_rois(path: Path, rois: list[Roi], media_timeline: bool, hwaccel: str | None) -> dict:
    hmax = max(r.h for r in rois)
    parts, labels = [], []
    head = f"[0:v:0]split={len(rois)}" + "".join(f"[v{i}]" for i in range(len(rois))) if len(rois) > 1 \
        else "[0:v:0]null[v0]"
    for i, r in enumerate(rois):
        parts.append(f"[v{i}]crop={r.w}:{r.h}:{r.x}:{r.y}:exact=1,scale=in_color_matrix=auto,"
                     f"format=rgb24,pad={r.w}:{hmax}:0:0[c{i}]")
        labels.append(f"[c{i}]")
    tail = (f"{''.join(labels)}hstack=inputs={len(rois)},showinfo[out]" if len(rois) > 1
            else "[c0]showinfo[out]")
    graph = ";".join([head, *parts, tail])
    cmd = [FFMPEG, "-hide_banner", "-nostdin", "-v", "info",
           *(["-hwaccel", hwaccel] if hwaccel else []),
           *(["-ignore_editlist", "1"] if media_timeline else []), "-copyts", "-i", str(path),
           "-filter_complex", graph, "-map", "[out]", "-fps_mode", "passthrough",
           "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    lines: list[str] = []
    th = threading.Thread(target=_drain, args=(proc.stderr, lines), daemon=True)
    th.start()
    wsum = sum(r.w for r in rois)
    fbytes = hmax * wsum * 3
    batch = 256
    means: list[np.ndarray] = []
    coef = np.array(REC709, dtype=np.float32)
    while True:
        buf = proc.stdout.read(fbytes * batch)
        if not buf:
            break
        k = len(buf) // fbytes
        if k * fbytes != len(buf):
            raise Refusal("media", "partial video frame from decoder")
        a = np.frombuffer(buf, dtype=np.uint8).reshape(k, hmax, wsum, 3).astype(np.float32)
        y = a @ coef
        cols, x0 = [], 0
        for r in rois:
            cols.append(y[:, :r.h, x0:x0 + r.w].mean(axis=(1, 2)))
            x0 += r.w
        means.append(np.stack(cols, axis=1))
    proc.wait()
    th.join()
    if proc.returncode != 0:
        raise Refusal("media", "video decode failed", stderr="".join(lines[-20:]))
    luma = np.concatenate(means) if means else np.zeros((0, len(rois)))
    tb = None
    pts, ptime = [], []
    for ln in lines:
        if "showinfo" not in ln:
            continue
        m = _TB.search(ln)
        if m and tb is None:
            tb = Fraction(int(m.group(1)), int(m.group(2)))
        m = _FRAME.search(ln)
        if m:
            pts.append(int(m.group(2)))
            ptime.append(float(m.group(3)))
    if len(pts) != len(luma):
        raise Refusal("media", f"decoder gave {len(luma)} frames but {len(pts)} timestamps")
    if tb is None:
        raise Refusal("media", "filter time base not reported")
    t = np.array(pts, dtype=np.float64) * float(tb)
    if len(t) and not np.allclose(t, ptime, rtol=2e-6, atol=1e-6):
        raise Refusal("media", "exact pts x time base disagrees with the printed pts_time")
    return {"t": t, "luma": luma, "time_base": str(tb)}


def frame_grab(path: Path, at_s: float, media_timeline: bool, out_png: Path) -> np.ndarray:
    import cv2
    cmd = [FFMPEG, "-hide_banner", "-nostdin", "-v", "error", "-y",
           *(["-ignore_editlist", "1"] if media_timeline else []), "-ss", f"{max(at_s, 0):.3f}",
           "-i", str(path), "-frames:v", "1", str(out_png)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    img = cv2.imread(str(out_png)) if r.returncode == 0 else None
    if img is None:
        raise Refusal("media", "cannot grab a frame for the ROI check", stderr=r.stderr[-1000:])
    return img


# ---------------------------------------------------------------- audio rules

def onset_rule(x: np.ndarray, sr: int, t0: float, win: tuple[float, float], noise_s: float,
               o: dict) -> tuple[float | None, float]:
    """Return (onset time, threshold) or (None, threshold) for a window [win0, win1) in recording s."""
    i0 = int(math.ceil((win[0] - t0) * sr))
    i1 = int(math.floor((win[1] - t0) * sr))
    n0 = i0 - int(round(noise_s * sr))
    if n0 < 0 or i1 > len(x) or i1 <= i0:
        return None, float("nan")
    noise = x[n0:i0]
    dc = noise.mean()
    nz = noise - dc
    w = np.abs(x[i0:i1] - dc)
    thr = max(o["k_rms"] * math.sqrt(float(np.mean(nz * nz))), o["k_peak"] * float(np.abs(nz).max()),
              10.0 ** (o["floor_dbfs"] / 20.0), 10.0 ** (o["pre_echo_guard_db"] / 20.0) * float(w.max()))
    above = np.nonzero(w > thr)[0]
    if len(above) == 0:
        return None, thr
    return t0 + (i0 + int(above[0])) / sr, thr


def envelope_events(x: np.ndarray, sr: int, t0: float, a: dict) -> np.ndarray:
    nb = max(1, int(round(a["envelope_bin_s"] * sr)))
    m = len(x) // nb
    b = np.abs(x[:m * nb]).reshape(m, nb).max(axis=1)
    thr = max(a["envelope_k_median"] * float(np.median(b)),
              a["envelope_frac_p999"] * float(np.percentile(b, 99.9)))
    above = b > thr
    quiet = int(round(a["event_quiet_s"] / a["envelope_bin_s"]))
    ev = []
    last_above = -10 ** 9
    for i in np.nonzero(above)[0]:
        if i - last_above > quiet:
            ev.append(i)
        last_above = i
    return t0 + np.array(ev, dtype=np.float64) * nb / sr


def find_triplets(ev: np.ndarray, d1: float, d2: float, tol: float) -> list[int]:
    out = []
    for i in range(len(ev) - 2):
        if abs(ev[i + 1] - ev[i] - d1) <= tol and abs(ev[i + 2] - ev[i + 1] - d2) <= tol:
            out.append(i)
    return out


def fingerprint_anchors(ev: np.ndarray, tc: np.ndarray, a: dict) -> np.ndarray:
    """Rows (k, event time): events whose following clicks' truth gaps all land on recording events."""
    n_fp = a["fingerprint_clicks"]
    out = []
    if len(ev) < 2:
        return np.zeros((0, 2))
    for k in range(len(tc) - n_fp + 1):
        span = tc[k:k + n_fp] - tc[k]
        pred = ev[:, None] + span[None, :]
        j = np.clip(np.searchsorted(ev, pred), 1, len(ev) - 1)
        err = np.minimum(np.abs(ev[j] - pred), np.abs(ev[j - 1] - pred))
        ok = np.nonzero((err <= a["fingerprint_tol_s"] + a["fingerprint_speed_tol"] * span[None, :]).all(axis=1))[0]
        if len(ok):
            out.append((k, ev[ok[np.argmin(err[ok].sum(axis=1))]]))
    return np.array(out, dtype=np.float64).reshape(-1, 2)


def theil_sen(x: np.ndarray, y: np.ndarray) -> tuple[float, float]:
    i, j = np.triu_indices(len(x), 1)
    dx = x[j] - x[i]
    keep = dx > 0
    b = float(np.median((y[j] - y[i])[keep] / dx[keep]))
    return float(np.median(y - b * x)), b


def local_clock(tc: np.ndarray, on: np.ndarray, use: np.ndarray, fallback: tuple[float, float],
                halfwidth: float, at=None) -> np.ndarray:
    """At each truth time q (default: the clicks): the least-squares line through on[use] for clicks within
    +/-halfwidth of q (q clamped into the click span to choose them), evaluated at q."""
    q_all = tc if at is None else np.asarray(at, dtype=np.float64)
    mapped = np.empty(len(q_all))
    for i, q in enumerate(q_all):
        qc = min(max(q, tc[0]), tc[-1])
        sel = use & (np.abs(tc - qc) <= halfwidth)
        if sel.sum() >= 3:
            _, c = np.polyfit(tc[sel] - q, on[sel], 1)
            mapped[i] = c
        else:
            mapped[i] = fallback[0] + fallback[1] * q
    return mapped


def align(audio: dict, truth: dict, rules: dict) -> dict:
    """Identify the clicks by their gap pattern and map each on the local onset clock (v2)."""
    a, o = rules["alignment"], rules["onset"]
    sr_t = truth["sample_rate"]
    tc = np.array(truth["clicks"], dtype=np.float64) / sr_t
    ss = np.array(truth["sync_start"], dtype=np.float64) / sr_t
    x, sr, t0 = audio["x"], audio["sr"], audio["t0"]
    ev = envelope_events(x, sr, t0, a)
    anchors = fingerprint_anchors(ev, tc, a)
    can_anchor = len(tc) - a["fingerprint_clicks"] + 1
    ident = {"envelope_events": int(len(ev)), "anchors": int(len(anchors)), "clicks_that_can_anchor": can_anchor}
    if len(anchors) < 3:
        raise Refusal("alignment", f"click gap pattern not identified: {len(anchors)} anchors", identify=ident)
    ka = anchors[:, 0].astype(int)
    ia, ib = theil_sen(tc[ka], anchors[:, 1])
    inl = np.abs(anchors[:, 1] - (ia + ib * tc[ka])) <= a["identify_inlier_tol_s"]
    ident.update(identify_a_s=ia, identify_b=ib, inlier_anchors=int(inl.sum()),
                 identified_fraction=float(inl.sum() / can_anchor))
    if inl.sum() < a["min_identified_fraction"] * can_anchor:
        raise Refusal("alignment", f"click gap pattern not identified: {int(inl.sum())} of {can_anchor} clicks "
                                   f"anchor on one line (the recording is not this stimulus, or its gaps are "
                                   f"shifted)", identify=ident)
    if abs(ib - 1) > a["max_abs_slope_error"]:
        raise Refusal("alignment", f"fitted speed b = {ib:.6f}; the recording does not run at real time "
                                   f"(time-remapped or wrong speed)", identify=ident)

    # onsets at the identify line, then the local clock
    on = np.full(len(tc), np.nan)
    thr = np.full(len(tc), np.nan)
    for k in range(len(tc)):
        m = ia + ib * tc[k]
        t, th = onset_rule(x, sr, t0, (m - o["window_s"], m + o["window_s"]), o["noise_s"], o)
        thr[k] = th
        if t is not None:
            on[k] = t
    found = np.isfinite(on)
    use = found.copy()
    mapped = np.full(len(tc), np.nan)
    for _ in range(a["clock_iterations"]):
        mapped = local_clock(tc, on, use, (ia, ib), a["clock_local_halfwidth_s"])
        use = found & (np.abs(on - mapped) <= a["clock_outlier_s"])
    resid = on - mapped
    settle_s = ss[0] + a["settle_after_sync_start_s"]
    status = np.where(tc < settle_s, "SETTLE_EXCLUDED",
                      np.where(~found, "ONSET_MISS", np.where(use, "VALID", "OFF_CLOCK")))
    clicks = [{"index": k, "truth_sample": int(truth["clicks"][k]), "truth_s": float(tc[k]),
               "mapped_s": float(mapped[k]), "onset_raw_s": float(on[k]) if found[k] else None,
               "onset_s": float(on[k]) if status[k] == "VALID" else None, "status": str(status[k]),
               "onset_threshold": float(thr[k]),
               "onset_minus_mapped_ms": float(resid[k] * 1e3) if found[k] else None} for k in range(len(tc))]
    g = (status == "VALID")
    ga, gb = theil_sen(tc[g], on[g]) if g.sum() >= 2 else (ia, ib)  # robust global line (report, device)

    # SYNC: coarse identity anchor only (never timing, never required)
    se = np.array(truth["sync_end"], dtype=np.float64) / sr_t
    d1, d2 = ss[1] - ss[0], ss[2] - ss[1]
    v1_trip = find_triplets(ev, d1, d2, a["triplet_spacing_tol_s"])
    coarse = find_triplets(ev, d1, d2, a["sync_coarse_tol_s"])
    sync = {"v1_rule_triplets": len(v1_trip), "coarse_triplets": len(coarse),
            "used_for": "coarse identity anchor only; timing comes from the click onsets"}
    for name, t_sync in (("start", ss[0]), ("end", se[0])):
        want = float(local_clock(tc, on, use, (ia, ib), a["clock_local_halfwidth_s"], [t_sync])[0])
        cand = [ev[i] for i in coarse if abs(ev[i] - want) <= a["sync_search_s"]]
        off = float(min(cand, key=lambda e: abs(e - want)) - want) if cand else None
        sync[name] = {"expected_rec_s": want, "found": bool(cand), "offset_ms": off * 1e3 if cand else None}
        if cand and abs(off) > a["sync_anchor_tol_s"]:
            raise Refusal("alignment", f"click identification disagrees with the {name} SYNC anchor by "
                                       f"{off * 1e3:.0f} ms (clicks identified at a shifted offset)",
                          sync=sync, identify=ident)

    active = status != "SETTLE_EXCLUDED"
    n_act = int(active.sum())
    fr = found & active
    summ = {"clicks": int(len(tc)), "settle_excluded": int((~active).sum()), "settle_before_truth_s": float(settle_s),
            "active_clicks": n_act, "onset_misses": int((active & ~found).sum()),
            "off_clock": int((status == "OFF_CLOCK").sum()), "valid": int(g.sum()),
            "median_onset_minus_mapped_ms": float(np.median(resid[fr])) * 1e3 if fr.any() else float("inf"),
            "median_abs_onset_minus_mapped_ms": float(np.median(np.abs(resid[fr]))) * 1e3 if fr.any() else float("inf"),
            "p95_abs_onset_minus_mapped_ms": float(np.percentile(np.abs(resid[fr]), 95)) * 1e3 if fr.any() else float("inf")}
    fit = {"method": "v2: click gap fingerprint + local onset clock", "a_s": float(ga), "b": float(gb),
           "skew_ppm": float((gb - 1) * 1e6), "identify": ident, "sync": sync, "onsets": summ, "clicks": clicks}
    if n_act == 0 or summ["onset_misses"] / n_act > o["max_onset_miss_fraction"]:
        raise Refusal("onset", f"{summ['onset_misses']} of {n_act} clicks have no audio onset; the recording does "
                               "not carry the stimulus", onsets=summ)
    if summ["off_clock"] / n_act > a["max_off_clock_fraction"]:
        raise Refusal("onset", f"{summ['off_clock']} of {n_act} onsets are off the local clock", onsets=summ)
    if summ["median_abs_onset_minus_mapped_ms"] > o["max_median_abs_onset_residual_s"] * 1e3:
        raise Refusal("onset", "clicks do not sit on the click-fitted clock", onsets=summ)
    return fit


# ---------------------------------------------------------------- photon rule

def photon_rule(t: np.ndarray, y: np.ndarray, onset: float, p: dict) -> dict:
    lo = np.searchsorted(t, onset - p["noise_s"], side="left")
    hi = np.searchsorted(t, onset, side="left")  # frames with t < onset
    idx = np.arange(max(lo, 1), hi)
    if len(idx) < 10:
        return {"status": "NO_NOISE_WINDOW"}
    s_noise = y[idx] - y[idx - 1]
    med = float(np.median(s_noise))
    sigma = 1.4826 * float(np.median(np.abs(s_noise - med)))
    thr = med + max(p["k_sigma"] * sigma, p["min_rise"])
    a = np.searchsorted(t, onset, side="right")  # first frame with t > onset
    b = np.searchsorted(t, onset + p["search_s"], side="right")
    for n in range(max(a, 1), min(b, len(y) - 1)):
        if y[n] - y[n - 1] > thr and y[n + 1] - y[n - 1] > thr:
            return {"status": "HIT", "frame": int(n), "photon_s": float(t[n]),
                    "latency_ms": float((t[n] - onset) * 1e3), "slope": float(y[n] - y[n - 1]),
                    "threshold": thr, "sigma": sigma}
    return {"status": "MISS", "threshold": thr, "sigma": sigma}


def early_rise(t: np.ndarray, y: np.ndarray, onset: float, thr: float, p: dict) -> bool:
    a = np.searchsorted(t, onset - p["early_check_s"], side="left")
    b = np.searchsorted(t, onset, side="left")
    return any(y[n] - y[n - 1] > thr and y[n + 1] - y[n - 1] > thr for n in range(max(a, 1), min(b, len(y) - 1)))


def stats(lat_ms: list[float]) -> dict:
    v = np.array(lat_ms)
    p50, p99 = float(np.percentile(v, 50)), float(np.percentile(v, 99))
    return {"n": int(len(v)), "p50_ms": p50, "p99_ms": p99, "mean_ms": float(v.mean()),
            "stdev_ms": float(v.std(ddof=1)) if len(v) > 1 else 0.0, "p99_minus_p50_ms": p99 - p50,
            "min_ms": float(v.min()), "max_ms": float(v.max())}


# ---------------------------------------------------------------- device timing

def device_alignment(path: Path, truth: dict, fit: dict, clicks: list[dict], rules: dict) -> dict:
    d = rules["device_alignment"]
    recs = [json.loads(ln) for ln in path.read_text().splitlines() if ln.strip()]
    base = {"source": str(path), "source_sha256": sha256_file(path), "records": len(recs)}
    if not recs:
        return {**base, "available": False, "reason": "the timing file holds no records"}
    keys = sorted(set().union(*(r.keys() for r in recs)))
    if "capture_time_us" not in keys:
        return {**base, "available": False, "fields": keys,
                "reason": "records carry no capture_time_us, so no device time base exists"}
    level = next((f for f in d["level_fields"] if all(f in r for r in recs)), None)
    if level is None:
        return {**base, "available": False, "fields": keys,
                "reason": ("the records carry no audio-derived per-hop field (looked for "
                           f"{', '.join(d['level_fields'])}), so the SYNC bursts cannot be located in "
                           "device time. _host_read_s and _music_s are host-clock times (serial read and "
                           "afplay launch); they include unmeasured host audio-output latency and serial/"
                           "poll delay, so they are not used to align device stamps to the video. Render "
                           "and wire-complete stamps are also absent from LiveTimingRecord v2.")}
    recs.sort(key=lambda r: r["capture_time_us"])
    tc = np.array([r["capture_time_us"] for r in recs], dtype=np.float64) / 1e6
    lv = np.array([float(r[level]) for r in recs])
    med = float(np.median(lv))
    thr = med + d["event_k_mad"] * 1.4826 * float(np.median(np.abs(lv - med)))
    ev, last = [], -1e18
    for i in np.nonzero(lv > thr)[0]:
        if tc[i] - last > d["event_quiet_s"]:
            ev.append(tc[i])
        last = tc[i]
    ev = np.array(ev)
    sr_t = truth["sample_rate"]
    ss, se = truth["sync_start"], truth["sync_end"]
    trip = find_triplets(ev, (ss[1] - ss[0]) / sr_t, (ss[2] - ss[1]) / sr_t, d["triplet_spacing_tol_s"])
    if len(trip) < 2:
        return {**base, "available": False, "level_field": level,
                "reason": f"found {len(trip)} SYNC triplet(s) in device '{level}'; need start and end"}
    dev = np.array([*ev[trip[0]:trip[0] + 3], *ev[trip[-1]:trip[-1] + 3]])
    ts = np.array([*ss, *se], dtype=np.float64) / sr_t
    A = np.stack([np.ones_like(ts), ts], axis=1)
    (c0, c1), *_ = np.linalg.lstsq(A, dev, rcond=None)
    resid = dev - (c0 + c1 * ts)
    out = {**base, "level_field": level, "fit_device_s": {"c_s": float(c0), "d": float(c1),
                                                          "residuals_ms": (resid * 1e3).tolist()}}
    if np.abs(resid).max() > d["max_fit_residual_s"]:
        return {**out, "available": False, "reason": "device SYNC fit residual exceeds the rule"}

    def dev_to_rec(dev_s):
        return fit["a_s"] + fit["b"] * ((dev_s - c0) / c1)

    tp = np.array([r.get("publish_time_us", np.nan) for r in recs], dtype=np.float64) / 1e6
    rows = []
    for c in clicks:
        if c["onset_s"] is None:
            continue
        k = int(np.searchsorted(tc, c0 + c1 * c["truth_s"], side="left"))
        if k >= len(tc):
            continue
        row = {"click": c["index"], "hop_capture_rec_s": float(dev_to_rec(tc[k])),
               "publish_rec_s": float(dev_to_rec(tp[k])) if np.isfinite(tp[k]) else None}
        row["onset_to_capture_ms"] = (row["hop_capture_rec_s"] - c["onset_s"]) * 1e3
        row["capture_to_publish_ms"] = ((row["publish_rec_s"] - row["hop_capture_rec_s"]) * 1e3
                                        if row["publish_rec_s"] is not None else None)
        for name, ph in c["boards"].items():
            if ph["status"] == "HIT" and row["publish_rec_s"] is not None:
                row[f"publish_to_photon_{name}_ms"] = (ph["photon_s"] - row["publish_rec_s"]) * 1e3
        rows.append(row)

    def med_of(key):
        v = [r[key] for r in rows if r.get(key) is not None]
        return float(np.median(v)) if v else None

    summary = {k: med_of(k) for k in sorted({k for r in rows for k in r if k.endswith("_ms")})}
    return {**out, "available": True, "per_click": rows, "median_ms": summary,
            "not_available": "render and wire-complete stamps (absent from LiveTimingRecord v2)",
            "uncertainty": "device events are hop-quantised: about one hop plus the difference in "
                           "acoustic path (speaker to Titan mic versus speaker to phone mic) [INFERENCE]"}


# ---------------------------------------------------------------- figure

def figure(report: dict, events: list[dict], out_png: Path) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    names = list(report["boards"].keys())
    fig, ax = plt.subplots(2, len(names), figsize=(6.2 * len(names), 7.2), squeeze=False)
    q = report["quantisation_ms_per_event"]
    for j, name in enumerate(names):
        b = report["boards"][name]
        hits = [(e["onset_s"], e["boards"][name]["latency_ms"]) for e in events
                if e["onset_s"] is not None and e["boards"][name]["status"] == "HIT"]
        miss = [e["onset_s"] for e in events if e["onset_s"] is not None and e["boards"][name]["status"] != "HIT"]
        h, s = ax[0][j], ax[1][j]
        title = f"{name}: {b['status']}  (hits {b['hits']}, misses {b['misses']}, null {b['null_hits']}/{b['null_trials']})"
        if b["status"] == "MEASURED":
            st = b["latency"]
            lat = np.array([v for _, v in hits])
            lo, hi = math.floor(lat.min() / q) * q - q, math.ceil(lat.max() / q) * q + q
            h.hist(lat, bins=np.arange(lo, hi + 0.25, 0.5), color="#c8a24a", edgecolor="#5a4a20", lw=0.5)
            for f in np.arange(lo, hi + 1e-9, q):
                h.axvline(f, color="#bbb", lw=0.6, zorder=0)
            h.axvline(st["p50_ms"], color="#222", lw=1.2, label=f"p50 {st['p50_ms']:.1f} ms")
            h.axvline(st["p99_ms"], color="#222", lw=1.2, ls="--", label=f"p99 {st['p99_ms']:.1f} ms")
            h.legend(loc="upper right", fontsize=8, title=(f"N {st['n']}, jitter sd {st['stdev_ms']:.2f} ms,\n"
                                                            f"p99-p50 {st['p99_minus_p50_ms']:.2f} ms"),
                     title_fontsize=8)
            h.set_xlim(lo, hi)
            h.set_xlabel(f"latency, ms (0.5 ms bins; grey lines = frame steps of {q:.2f} ms)")
            h.set_ylabel("events")
            s.plot([t for t, _ in hits], lat, ".", color="#8a6d1f", ms=4, label="hit")
            s.axhline(st["p50_ms"], color="#222", lw=0.8)
            s.set_ylim(lo, hi)
        else:
            h.text(0.5, 0.5, "NO CONFIDENT LATENCY\n" + "\n".join(b["reasons"]), ha="center", va="center",
                   transform=h.transAxes, fontsize=10, color="#9b1c1c", wrap=True)
            h.set_xticks([])
            h.set_yticks([])
            s.set_ylim(0, report["search_window_ms"])
            if hits:
                s.plot([t for t, _ in hits], [v for _, v in hits], ".", color="#999", ms=4,
                       label="unclaimed rise (not a latency)")
        if miss:  # misses as a rug on the time axis, independent of the latency scale
            s.plot(miss, np.full(len(miss), 0.02), "|", color="#9b1c1c", ms=12, mew=1.2,
                   transform=s.get_xaxis_transform(), label=f"miss ({len(miss)})")
        s.set_xlabel("onset time in recording, s")
        s.set_ylabel("latency, ms")
        if s.get_legend_handles_labels()[0]:
            s.legend(loc="upper right", fontsize=8)
        h.set_title(title, fontsize=10)
    fig.suptitle(f"P5 photon latency — {Path(report['inputs']['video']).name}; "
                 f"quantisation ±{q:.2f} ms per event", fontsize=11)
    fig.tight_layout()
    fig.savefig(out_png, dpi=110)
    plt.close(fig)


# ---------------------------------------------------------------- main analysis

def load_rules(path: Path, allow_unfrozen: bool) -> tuple[dict, dict]:
    rules = json.loads(path.read_text())
    if rules.get("schema") != RULES_SCHEMA:
        raise Refusal("rules", f"rules schema {rules.get('schema')!r} is not {RULES_SCHEMA}")
    info = {"path": str(path), "sha256": sha256_file(path), "frozen": "frozen_utc" in rules,
            "frozen_utc": rules.get("frozen_utc"), "tool_sha256": tool_sha256(),
            "bound_tool_sha256": rules.get("bound_tool_sha256")}
    info["tool_matches_rules"] = info["tool_sha256"] == info["bound_tool_sha256"]
    if info["frozen"]:
        sys.path.insert(0, str(ROOT / "scripts"))
        import freeze_tolerance
        problems = freeze_tolerance.verify(path, [])
        info["freeze_verify"] = problems or "ok"
        if problems:
            raise Refusal("rules", "frozen rules fail verification", problems=problems)
        if not info["tool_matches_rules"]:
            raise Refusal("rules", "this tool's SHA-256 differs from the one the frozen rules are bound to")
    elif not allow_unfrozen:
        raise Refusal("rules", "rules are not frozen; freeze them with scripts/freeze_tolerance.py "
                               "(or pass --allow-unfrozen-rules for a non-admissible test run)")
    return rules, info


def parse_roi(s: str) -> Roi:
    name, _, spec = s.partition("=")
    vals = [int(v) for v in spec.split(",")]
    if not name or len(vals) != 4:
        raise argparse.ArgumentTypeError(f"ROI must be name=x,y,w,h, got {s!r}")
    return Roi(name, *vals)


def analyse(a) -> int:
    out: Path = a.out
    out.mkdir(parents=True, exist_ok=False)
    started = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    ctx: dict = {"started_utc": started, "inputs": {"video": str(a.video), "truth": str(a.truth)}}
    try:
        rules, rinfo = load_rules(a.rules, a.allow_unfrozen_rules)
        ctx["rules"] = rinfo
        truth = json.loads(a.truth.read_text())
        ctx["inputs"].update(truth_sha256=sha256_file(a.truth), video_sha256=sha256_file(a.video),
                             stimulus_wav_sha256=truth.get("wav_sha256"))
        m = rules["media"]

        # 1. container, edit lists, streams
        cont = parse_tracks(a.video)
        probe0 = ffprobe(a.video, media_timeline=False)
        streams = probe0["streams"]
        vids = [s for s in streams if s["codec_type"] == "video" and
                not s.get("disposition", {}).get("attached_pic")]
        if len(vids) != 1:
            raise Refusal("media", f"expected one video stream, found {len(vids)}")
        auds = [s for s in streams if s["codec_type"] == "audio" and s.get("codec_name") in m["audio_codecs_accepted"]]
        if not auds:
            raise Refusal("media", "no decodable audio track: onsets must come from the recording's own audio",
                          audio_codecs=[s.get("codec_name") for s in streams if s["codec_type"] == "audio"])
        vs, as_ = vids[0], auds[0]

        def track_of(s):
            tid = int(s["id"], 16) if str(s.get("id", "")).startswith("0x") else None
            return next((t for t in cont["tracks"] if t["track_id"] == tid), None)

        vt, at = track_of(vs), track_of(as_)
        if vt is None or at is None:
            raise Refusal("media", "cannot match streams to container tracks")
        media_timeline = not (vt["edit_identity"] and at["edit_identity"])
        for t in (vt, at):
            if media_timeline and t["media_offset_s"] is None:
                raise Refusal("media", f"track {t['track_id']} has a non-identity edit list with no media edit")
        timeline = {"kind": "media (time-remapping edit list detected; edit lists ignored, per-track first-edit offset applied)"
                    if media_timeline else "container (identity edit lists applied by the demuxer)",
                    "video_track": {k: vt[k] for k in ("track_id", "edits", "edit_identity", "media_offset_s")},
                    "audio_track": {k: at[k] for k in ("track_id", "edits", "edit_identity", "media_offset_s")}}
        ctx["timeline"] = timeline
        probe = ffprobe(a.video, media_timeline) if media_timeline else probe0
        pv = next(s for s in probe["streams"] if s["index"] == vs["index"])
        pa = next(s for s in probe["streams"] if s["index"] == as_["index"])
        nominal = m["nominal_fps"]
        rfr, afr = rate(pv.get("r_frame_rate")), rate(pv.get("avg_frame_rate"))
        media = {"video_codec": pv.get("codec_name"), "audio_codec": pa.get("codec_name"),
                 "r_frame_rate": pv.get("r_frame_rate"), "avg_frame_rate": pv.get("avg_frame_rate"),
                 "color_transfer": pv.get("color_transfer"), "width": pv.get("width"),
                 "height": pv.get("height"), "video_stream_duration_s": pv.get("duration"),
                 "audio_stream_duration_s": pa.get("duration"), "audio_sample_rate": pa.get("sample_rate")}
        ctx["media"] = media
        vd, ad = pv.get("duration"), pa.get("duration")
        for label, v in (("r_frame_rate", rfr), ("avg_frame_rate", afr)):
            if v is None or abs(v / nominal - 1) > m["fps_rel_tol"]:
                why = ""
                ratio = nominal / v if v else 0.0
                if ratio >= 4 and abs(ratio - round(ratio)) < 0.01 and vd and ad and \
                        abs(float(vd) - float(ad)) <= max(m["av_duration_max_diff_s"],
                                                          m["av_duration_max_diff_rel"] * float(ad)):
                    why = (f" This looks like an iOS-rendered (re-timed) export: {v:g} fps x {round(ratio)} with "
                           "equally long tracks, so frame times are grid slots, not capture times. Transfer the "
                           "unmodified original (AirDrop Options > All Photos Data, or Image Capture).")
                raise Refusal("media", f"{label} = {media[label]} is not ~{nominal:g} fps; real-time "
                                       f"240 fps capture is required.{why}", media=media)
        if vd and ad:
            vd, ad = float(vd), float(ad)
            if abs(vd - ad) > max(m["av_duration_max_diff_s"], m["av_duration_max_diff_rel"] * ad):
                raise Refusal("media", f"video lasts {vd:.3f} s but audio {ad:.3f} s ({vd / ad:.2f}x): "
                                       "a time-remapped (slowed) file; real-time pts are required", media=media)

        # 2. decode audio and ROIs
        audio = decode_audio(a.video, as_["index"], media_timeline, rules)
        if media_timeline:
            audio["t0"] += at["media_offset_s"]
        media.update(audio_decoded_sr=audio["sr"], audio_t0_s=audio["t0"],
                     audio_span_s=len(audio["x"]) / audio["sr"],
                     audio_max_discontinuity_s=audio["max_discontinuity_s"])
        grab_at = (truth["clicks"][len(truth["clicks"]) // 2] / truth["sample_rate"])
        img = frame_grab(a.video, grab_at, media_timeline, out / "roi-check.png")
        H, W = img.shape[:2]
        media.update(displayed_width=W, displayed_height=H)
        for r in a.roi:
            if r.x < 0 or r.y < 0 or r.x + r.w > W or r.y + r.h > H or r.w * r.h < m["roi_min_pixels"]:
                raise Refusal("media", f"ROI {r.name}={r.x},{r.y},{r.w},{r.h} is outside the {W}x{H} "
                                       f"displayed frame or smaller than {m['roi_min_pixels']} px")
        import cv2
        for r in a.roi:
            th, fs = max(1, W // 640), max(0.4, W / 1600)
            cv2.rectangle(img, (r.x, r.y), (r.x + r.w - 1, r.y + r.h - 1), (0, 200, 255), th)
            cv2.putText(img, r.name, (r.x, max(r.y - 4 * th, 10)), cv2.FONT_HERSHEY_SIMPLEX, fs, (0, 200, 255), th)
        cv2.imwrite(str(out / "roi-check.png"), img)
        vid = decode_rois(a.video, a.roi, media_timeline, a.hwaccel)
        t = vid["t"] + (vt["media_offset_s"] if media_timeline else 0.0)
        if len(t) < 2:
            raise Refusal("media", "fewer than two video frames decoded")
        steps = np.diff(t)
        period = 1.0 / nominal
        med_step = float(np.median(steps))
        irregular = float(np.mean((steps < m["pts_irregular_band"][0] * period) |
                                  (steps > m["pts_irregular_band"][1] * period)))
        v_span = float(t[-1] - t[0] + med_step)
        media.update(frames=int(len(t)), video_t0_s=float(t[0]), video_span_s=v_span,
                     median_pts_step_ms=med_step * 1e3, pts_irregular_fraction=irregular,
                     filter_time_base=vid["time_base"])
        if abs((1.0 / med_step) / nominal - 1) > m["fps_rel_tol"]:
            raise Refusal("media", f"median decoded pts step {med_step * 1e3:.3f} ms is not 1/{nominal:g} s",
                          media=media)
        if irregular > m["pts_irregular_max_fraction"]:
            raise Refusal("media", f"{irregular:.4f} of frame steps are irregular (dropped/duplicated frames)",
                          media=media)
        a_span = media["audio_span_s"]
        if abs(v_span - a_span) > max(m["av_duration_max_diff_s"], m["av_duration_max_diff_rel"] * a_span):
            raise Refusal("media", f"decoded video spans {v_span:.3f} s but audio {a_span:.3f} s "
                                   f"({v_span / a_span:.2f}x): time-remapped; real-time pts are required",
                          media=media)

        # 3. alignment and onsets
        fit = align(audio, truth, rules)
        events = [{**c, "boards": {}} for c in fit.pop("clicks")]
        ctx["alignment"] = fit
        ctx["onsets"] = fit["onsets"]
        p = rules["photon"]
        valid = [e for e in events if e["status"] == "VALID"]

        # 4. photons, null control (only VALID clicks are paired; the others keep their status)
        boards = {}
        for j, r in enumerate(a.roi):
            y = vid["luma"][:, j].astype(np.float64)
            hits, misses, nonoise, early = [], 0, 0, 0
            for e in events:
                if e["status"] != "VALID":
                    e["boards"][r.name] = {"status": e["status"]}
                    continue
                ph = photon_rule(t, y, e["onset_s"], p)
                e["boards"][r.name] = ph
                if ph["status"] == "HIT":
                    hits.append(ph["latency_ms"])
                elif ph["status"] == "MISS":
                    misses += 1
                else:
                    nonoise += 1
                if "threshold" in ph and early_rise(t, y, e["onset_s"], ph["threshold"], p):
                    early += 1
            null_hits = null_trials = 0
            for e0, e1 in zip(valid, valid[1:]):
                if e1["onset_s"] - e0["onset_s"] >= 2 * p["search_s"]:
                    null_trials += 1
                    if photon_rule(t, y, e1["onset_s"] - p["search_s"], p)["status"] == "HIT":
                        null_hits += 1
            n_valid = len(hits) + misses
            bs = rules["board_status"]
            reasons = []
            hit_frac = len(hits) / n_valid if n_valid else 0.0
            null_frac = null_hits / null_trials if null_trials else 1.0
            if hit_frac < bs["min_hit_fraction"]:
                reasons.append(f"hit fraction {hit_frac:.2f} < {bs['min_hit_fraction']}")
            if len(hits) < bs["min_hits"]:
                reasons.append(f"hits {len(hits)} < {bs['min_hits']}")
            if null_frac > bs["max_null_hit_fraction"]:
                reasons.append(f"null-control hit fraction {null_frac:.2f} > {bs['max_null_hit_fraction']}"
                               f" (detector fires without a click)")
            bd = {"roi": [r.x, r.y, r.w, r.h], "status": "MEASURED" if not reasons else "NO_CONFIDENT_LATENCY",
                  "reasons": reasons, "valid_onsets": n_valid, "hits": len(hits), "misses": misses,
                  "no_noise_window": nonoise, "hit_fraction": hit_frac, "null_hits": null_hits,
                  "null_trials": null_trials, "early_rises_before_onset": early,
                  "latency_claimed": not reasons,
                  "latency": stats(hits) if not reasons else None,
                  "brief_n_met": len(hits) >= rules["brief_min_onsets"]}
            boards[r.name] = bd
        report = {**ctx, "tool": "scripts/p5_latency.py", "boards": boards,
                  "quantisation_ms_per_event": rules["quantisation"]["per_event_ms"],
                  "search_window_ms": p["search_s"] * 1e3,
                  "quantisation_statement": rules["quantisation"]["statement"],
                  "boom_excluded": "only truth clicks are scored; BOOM is never paired",
                  "limits": LIMITS}
        if a.device_timing:
            report["device_alignment"] = device_alignment(a.device_timing, truth, fit, events, rules)
        else:
            report["device_alignment"] = {"available": False, "reason": "no --device-timing given"}
        why = []
        if not rinfo["frozen"]:
            why.append("rules not frozen")
        if not rinfo["tool_matches_rules"]:
            why.append("tool SHA-256 differs from the rules' bound tool")
        if truth.get("wav_sha256") != rules["p5_stimulus_wav_sha256"]:
            why.append("stimulus is not the P5 click track (test or other stimulus)")
        for name, bd in boards.items():
            if bd["status"] != "MEASURED":
                why.append(f"{name}: no confident latency")
            elif not bd["brief_n_met"]:
                why.append(f"{name}: {bd['hits']} paired events < brief minimum {rules['brief_min_onsets']}")
        report["p5_admissible"] = not why
        report["p5_admissible_reasons"] = why
        write_outputs(out, report, events, [r.name for r in a.roi])
        return 0 if all(b["status"] == "MEASURED" for b in boards.values()) else 3
    except Refusal as exc:
        ref = {**ctx, "refused": True, "stage": exc.stage, "reason": exc.reason,
               "detail": json.loads(json.dumps(exc.detail, default=str))}
        (out / "refusal.json").write_text(json.dumps(ref, indent=1) + "\n")
        print(f"REFUSED [{exc.stage}]: {exc.reason}", file=sys.stderr)
        return 2


LIMITS = [
    "Emission versus exposure: the frame pts names a frame, not the instant the LEDs changed; the light "
    "changed within the preceding frame period (quantisation, stated per event).",
    "Rolling shutter: each sensor row is read at a different time within the frame; an ROI's rows are "
    "exposed later than the frame's pts by its row offset times the readout time. Bound: up to the "
    "sensor readout fraction of 1/240 s (at most 4.17 ms; an ROI high in the frame sees less) "
    "[INFERENCE, not measured].",
    "iPhone audio/video track offset: any constant offset between the phone's audio and video timelines "
    "shifts every latency equally; this tool cannot measure it without an event that is both seen and heard "
    "at a known instant [INFERENCE].",
    "Acoustic path: onsets are when the sound reached the phone's microphone, not the Titan's "
    "(about 2.9 ms per metre of path difference).",
    "AAC pre-echo can move an audio onset earlier by up to a few milliseconds [INFERENCE].",
    "Responses that darken, or that rise slower than the noise-relative threshold within 300 ms, are "
    "MISSES; responses earlier than the onset (negative latency, e.g. from an A/V offset) are not searched "
    "(the early-rise count flags them).",
    "Auto-exposure and white balance changes alter luma; lock exposure (AE/AF lock) when recording.",
]


def write_outputs(out: Path, report: dict, events: list[dict], names: list[str]) -> None:
    (out / "report.json").write_text(json.dumps(report, indent=1, default=float) + "\n")
    with (out / "events.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["click", "truth_sample", "mapped_s", "onset_s", "onset_minus_mapped_ms", "board",
                    "status", "frame", "photon_s", "latency_ms", "slope", "threshold"])
        for e in events:
            for n in names:
                ph = e["boards"].get(n, {})
                w.writerow([e["index"], e["truth_sample"], f"{e['mapped_s']:.6f}",
                            "" if e["onset_s"] is None else f"{e['onset_s']:.6f}",
                            "" if e["onset_minus_mapped_ms"] is None else f"{e['onset_minus_mapped_ms']:.3f}",
                            n, ph.get("status"), ph.get("frame", ""),
                            "" if "photon_s" not in ph else f"{ph['photon_s']:.6f}",
                            "" if "latency_ms" not in ph else f"{ph['latency_ms']:.3f}",
                            "" if "slope" not in ph else f"{ph['slope']:.3f}",
                            "" if "threshold" not in ph else f"{ph['threshold']:.3f}"])
    figure(report, events, out / "latency.png")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("rules", help="write the rule set bound to this tool version")
    r.add_argument("--out", type=Path, required=True)
    an = sub.add_parser("analyse", help="analyse one recording")
    an.add_argument("--video", type=Path, required=True)
    an.add_argument("--truth", type=Path, required=True)
    an.add_argument("--rules", type=Path, required=True)
    an.add_argument("--roi", type=parse_roi, action="append", required=True,
                    help="name=x,y,w,h in displayed frame pixels; board0 = lower board, board1 = upper")
    an.add_argument("--out", type=Path, required=True)
    an.add_argument("--device-timing", type=Path)
    an.add_argument("--allow-unfrozen-rules", action="store_true",
                    help="test runs only; the report is then marked not admissible")
    an.add_argument("--hwaccel", help="optional ffmpeg -hwaccel (e.g. videotoolbox)")
    a = ap.parse_args(argv)
    if a.cmd == "rules":
        if a.out.exists():
            raise SystemExit(f"REFUSED: {a.out} exists (a rules file is never overwritten)")
        a.out.write_text(json.dumps(proposed_rules(), indent=2) + "\n")
        print(json.dumps({"rules": str(a.out), "sha256": sha256_file(a.out), "bound_tool_sha256": tool_sha256()}))
        return 0
    names = [r.name for r in a.roi]
    if len(set(names)) != len(names):
        raise SystemExit("REFUSED: duplicate ROI names")
    return analyse(a)


if __name__ == "__main__":
    raise SystemExit(main())
