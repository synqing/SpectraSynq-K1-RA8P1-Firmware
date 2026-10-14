#!/usr/bin/env python3
"""Device-side latency budget for the P5 click+BOOM capture.

Host-only. Reads the timing log and the stimulus truth, writes budget.json
and budget.png in this folder. Does not touch a device.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(
    "/Users/spectrasynq/Workspace_Management/Software/"
    "SpectraSynq-K1-RA8P1-Firmware/.worktrees/titan-emotiscope-port-001"
)
OUT = ROOT / "_scratch/waveform-migration-001/runs/P5-20261014T1315Z/device-budget"
TIMING = (
    ROOT
    / "_scratch/waveform-migration-001/runs/P5-20261014T1315Z"
    / "permitted/device-capture/timing.jsonl"
)
TRUTH = ROOT / "_scratch/waveform-migration-001/audio-p5-001/p5-clicks-boom.truth.json"
INFO = (
    ROOT
    / "_scratch/waveform-migration-001/runs/P5-20261014T1315Z"
    / "permitted/device-capture/info.json"
)
REPORT = (
    ROOT
    / "_scratch/waveform-migration-001/runs/P5-20261014T1315Z"
    / "permitted/device-capture/report.json"
)

# Profile 3, one DIN, 80 pixels. Lanes run together, so the wall time is one lane.
# bits = 80 * 6 * 8; data_ns = bits * 1250; reset_ns = 300 * 1000.
WIRE_MS = (80 * 6 * 8 * 1250 + 300 * 1000) / 1e6  # 5.100
FRAME_MS = 10.0
SLOT_SAMPLES = 296
FIR_SAMPLES = 47
DONOR_HOP_MS = 64 / 12800 * 1000.0  # 5.000
CPU_HZ = 1_000_000_000


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def pct(values: np.ndarray) -> dict:
    a = np.asarray(values, dtype=np.float64)
    a = a[np.isfinite(a)]
    if a.size == 0:
        return {"count": 0, "min": None, "p50": None, "p99": None, "max": None, "mean": None}
    return {
        "count": int(a.size),
        "min": float(np.min(a)),
        "p50": float(np.percentile(a, 50)),
        "p99": float(np.percentile(a, 99)),
        "max": float(np.max(a)),
        "mean": float(np.mean(a)),
    }


def deduped(values: np.ndarray, mask: np.ndarray) -> np.ndarray:
    """Values on the rows where the field changes, inside mask.

    The first masked row counts. A repeated identical cost is one event.
    """
    idx = np.flatnonzero(mask)
    if idx.size == 0:
        return np.empty(0, dtype=np.float64)
    v = values[idx]
    keep = np.empty(v.shape, dtype=bool)
    keep[0] = True
    keep[1:] = v[1:] != v[:-1]
    return v[keep]


def main() -> None:
    truth = json.loads(TRUTH.read_text())
    info = json.loads(INFO.read_text())
    report = json.loads(REPORT.read_text())
    sr = int(truth["sample_rate"])
    clicks = [int(x) for x in truth["clicks"]]
    first_click_s = clicks[0] / sr
    last_click_s = clicks[-1] / sr
    boom_start_s = int(truth["boom_start"]) / sr
    boom_len_s = 75.0
    boom_end_s = boom_start_s + boom_len_s
    sync_start_s = [x / sr for x in truth["sync_start"]]
    sync_end_s = [x / sr for x in truth["sync_end"]]

    n = 0
    rows = []
    with TIMING.open() as f:
        for line in f:
            rows.append(json.loads(line))
            n += 1

    def col(name, dtype=np.float64):
        return np.fromiter((r[name] for r in rows), dtype=dtype, count=n)

    write_seq = col("write_seq", np.uint64)
    hop_seq = col("hop_sequence", np.uint64)
    capture = col("capture_time_us")
    publish = col("publish_time_us")
    ap = col("ap_cycles")
    vp = col("vp_cycles")
    hop_dt = col("hop_dt_us")
    measured_hz = col("measured_hz")
    poll_sum = col("poll_cycles_sum")
    poll_max = col("poll_cycles_max")
    poll_calls = col("poll_calls")
    asrc = col("asrc_cycles_sum")
    queue = col("queue_filled_at_consume")
    emit = col("emit_cycles_last")
    donor = col("donor_cycles")
    inhouse = col("inhouse_cycles")
    a24 = col("a24_cycles_div16")
    flags = col("flags")
    starved = col("asrc_starved")
    music = col("_music_s")
    host_read = col("_host_read_s")

    # Integrity
    dseq = np.diff(write_seq.astype(np.int64))
    gaps = dseq - 1
    gap_missing = int(np.maximum(gaps, 0).sum())
    gap_events = int(np.sum(dseq > 1))
    dup_events = int(np.sum(dseq == 0))
    back_events = int(np.sum(dseq < 0))
    first_gaps = []
    for i, d in enumerate(dseq.tolist()):
        if d != 1 and len(first_gaps) < 8:
            first_gaps.append(
                {
                    "index": i,
                    "from_seq": int(write_seq[i]),
                    "to_seq": int(write_seq[i + 1]),
                    "step": int(d),
                }
            )

    cap_zero = int(np.sum(capture == 0))
    pub_lt_cap = int(np.sum(publish < capture))
    nonzero = capture > 0
    # Monotonic among successive non-zero stamps, file order.
    nz_idx = np.flatnonzero(nonzero)
    cap_back = 0
    cap_back_examples = []
    if nz_idx.size > 1:
        steps = np.diff(capture[nz_idx])
        bad = np.flatnonzero(steps <= 0)
        cap_back = int(bad.size)
        for j in bad[:8]:
            a = int(nz_idx[j])
            b = int(nz_idx[j + 1])
            cap_back_examples.append(
                {
                    "from_index": a,
                    "to_index": b,
                    "from_seq": int(write_seq[a]),
                    "to_seq": int(write_seq[b]),
                    "from_capture_us": int(capture[a]),
                    "to_capture_us": int(capture[b]),
                    "step_us": float(capture[b] - capture[a]),
                }
            )

    valid = (capture > 0) & (publish >= capture)
    c_us = publish - capture
    c_ms = c_us / 1000.0
    ap_ms = ap / (CPU_HZ / 1000.0)  # cycles at 1 GHz -> ms
    # (b) is the part of (c) before the measured analysis span. Algebraic split.
    b_ms = c_ms - ap_ms
    vp_ms = vp / (CPU_HZ / 1000.0)
    emit_ms = emit / (CPU_HZ / 1000.0)
    donor_ms = donor / (CPU_HZ / 1000.0)
    inhouse_ms = inhouse / (CPU_HZ / 1000.0)
    slot_ms = np.where(measured_hz > 0, SLOT_SAMPLES / measured_hz * 1000.0, np.nan)
    fir_ms = np.where(measured_hz > 0, FIR_SAMPLES / measured_hz * 1000.0, np.nan)
    joint_ms = c_ms + vp_ms + emit_ms + WIRE_MS

    music_neg = music < 0
    in_clicks = (music >= first_click_s) & (music < boom_start_s)
    in_boom = (music >= boom_start_s) & (music < boom_end_s)
    in_lead = (music >= 0) & (music < first_click_s)
    in_tail = music >= boom_end_s

    unique_music = np.unique(music[music >= 0])
    music_step = np.diff(np.sort(unique_music)) if unique_music.size > 1 else np.empty(0)

    def section_block(mask: np.ndarray) -> dict:
        m = mask & valid
        d_vp = deduped(vp_ms, m)
        d_emit = deduped(emit_ms, m)
        d_vp_all = vp_ms[m]
        d_emit_all = emit_ms[m]
        c = pct(c_ms[m])
        b = pct(b_ms[m])
        aps = pct(ap_ms[m])
        d = pct(d_vp)
        e_cpu = pct(d_emit)
        slot = pct(slot_ms[m])
        fir = pct(fir_ms[m])
        joint = pct(joint_ms[m])
        hop = pct(hop_dt[m] / 1000.0)
        donor_p = pct(donor_ms[m])
        inhouse_p = pct(inhouse_ms[m])
        inhouse_idle = int(np.sum(inhouse[m] < 10_000))
        a24_zero = int(np.sum(a24[m] == 0))
        a24_sat = int(np.sum(a24[m] >= 65535))
        # Stage headline figures.
        stages = {
            "a_slot": {
                "kind": "derived_from_constants",
                "in_additive_sum": False,
                "what": "PDM slot duration 296/measured_hz. Block granularity. Acoustic travel, the microphone and the PDM sinc group delay are unmeasured and are not given a millisecond.",
                "stats_ms": slot,
            },
            "a_fir": {
                "kind": "derived_from_constants",
                "in_additive_sum": False,
                "what": "47-sample causal FIR at the measured PDM rate. Already subtracted inside the capture stamp, so it already sits inside stage (c).",
                "stats_ms": fir,
            },
            "b_before_analysis": {
                "kind": "inferred",
                "in_additive_sum": False,
                "what": "publish-capture minus ap_cycles. Everything from the content stamp to the start of the measured analysis span, including the FIR, the rest of the DMA block, the queue and descriptor pull. This is a split of (c), not a second clock.",
                "stats_ms": b,
                "negative_rows": int(np.sum(b_ms[m] < 0)),
            },
            "c_capture_to_publish": {
                "kind": "measured_on_device",
                "in_additive_sum": True,
                "what": "publish_time_us - capture_time_us on rows with a valid capture stamp.",
                "stats_ms": c,
                "stats_us": pct(c_us[m]),
            },
            "c_analysis_cpu": {
                "kind": "measured_on_device",
                "in_additive_sum": False,
                "what": "ap_cycles at 1 GHz. Donor plus in-house work inside (c). Not added on top of (c).",
                "stats_ms": aps,
                "donor_ms": donor_p,
                "inhouse_ms": inhouse_p,
                "inhouse_under_10000_cycles": inhouse_idle,
                "inhouse_idle_fraction": (inhouse_idle / int(m.sum())) if int(m.sum()) else None,
            },
            "d_render": {
                "kind": "measured_on_device",
                "in_additive_sum": True,
                "what": "vp_cycles at 1 GHz. One cost per change in the sticky field (the last completed render). All-row figures are stored beside it.",
                "stats_ms": d,
                "all_rows_ms": pct(d_vp_all),
                "zero_rows_in_section_valid": int(np.sum(vp[m] == 0)),
            },
            "e_pack_cpu": {
                "kind": "measured_on_device",
                "in_additive_sum": True,
                "what": "emit_cycles_last at 1 GHz. Pack and submit of both DINs. One cost per change in the sticky field. The wire time is not inside this number.",
                "stats_ms": e_cpu,
                "all_rows_ms": pct(d_emit_all),
            },
            "e_wire": {
                "kind": "derived_from_constants",
                "in_additive_sum": True,
                "what": "WS2816 profile 3, 80 pixels, one lane, because the two DINs shift bits at the same time.",
                "stats_ms": {
                    "count": int(m.sum()),
                    "min": WIRE_MS,
                    "p50": WIRE_MS,
                    "p99": WIRE_MS,
                    "max": WIRE_MS,
                    "mean": WIRE_MS,
                },
            },
            "e_pack_plus_wire": {
                "kind": "measured_on_device_plus_derived",
                "in_additive_sum": True,
                "stats_ms": {
                    "p50": None if e_cpu["p50"] is None else e_cpu["p50"] + WIRE_MS,
                    "p99": None if e_cpu["p99"] is None else e_cpu["p99"] + WIRE_MS,
                    "max": None if e_cpu["max"] is None else e_cpu["max"] + WIRE_MS,
                },
            },
            "f_frame_wait": {
                "kind": "inferred",
                "in_additive_sum": True,
                "what": "Uniform wait from publish to the next 10 ms palette tick. Not stamped per hop.",
                "stats_ms": {
                    "count": int(m.sum()),
                    "min": 0.0,
                    "p50": FRAME_MS / 2.0,
                    "p99": 0.99 * FRAME_MS,
                    "max": FRAME_MS,
                    "mean": FRAME_MS / 2.0,
                },
            },
            "joint_measured_to_end_of_wire": {
                "kind": "measured_on_device_plus_derived",
                "in_additive_sum": False,
                "what": "Per valid row: (publish-capture) + sticky vp + sticky emit + 5.100 ms wire. This is the percentile of the sum. It does not include the frame wait.",
                "stats_ms": joint,
            },
        }
        # Sum of the headline stage percentiles. Not the percentile of the sum.
        def add_stages(key):
            parts = [
                c[key],
                d[key],
                e_cpu[key],
                WIRE_MS,
                stages["f_frame_wait"]["stats_ms"][key],
            ]
            if any(v is None for v in parts):
                return None
            return float(sum(parts))

        return {
            "rows_in_section": int(mask.sum()),
            "rows_valid": int(m.sum()),
            "rows_capture_zero": int(np.sum(mask & (capture == 0))),
            "rows_publish_before_capture": int(np.sum(mask & (publish < capture))),
            "music_s_min": float(np.min(music[mask])) if mask.any() else None,
            "music_s_max": float(np.max(music[mask])) if mask.any() else None,
            "stages": stages,
            "sum_of_stage_percentiles_ms": {
                "p50": add_stages("p50"),
                "p99": add_stages("p99"),
                "max": add_stages("max"),
                "formula": "c + d + e_pack + 5.100 + f. Sum of each stage's own percentile.",
            },
            "context": {
                "hop_dt_ms": hop,
                "hop_dt_under_1ms": int(np.sum(hop_dt[m] < 1000)),
                "hop_dt_5_to_8ms": int(np.sum((hop_dt[m] >= 5000) & (hop_dt[m] < 8000))),
                "measured_hz": pct(measured_hz[m]),
                "queue_filled_samples": pct(queue[m]),
                "poll_cycles_max": pct(poll_max[m]),
                "poll_calls": pct(poll_calls[m]),
                "asrc_cycles_sum": pct(asrc[m]),
                "a24_cycles_div16_zero": a24_zero,
                "a24_cycles_div16_saturated": a24_sat,
                "flags_nonzero": int(np.sum(flags[m] != 0)),
                "asrc_starved_nonzero": int(np.sum(starved[m] != 0)),
            },
        }

    clicks_b = section_block(in_clicks)
    boom_b = section_block(in_boom)

    pub_span_s = float((publish[-1] - publish[0]) / 1e6) if n > 1 else 0.0
    cap_span_s = None
    if nz_idx.size > 1:
        cap_span_s = float((capture[nz_idx[-1]] - capture[nz_idx[0]]) / 1e6)

    # Successive content-stamp step, valid rows only.
    v_idx = np.flatnonzero(valid)
    cap_step_ms = pct(np.diff(capture[v_idx]) / 1000.0) if v_idx.size > 1 else pct(np.empty(0))

    emitted = int(report["deltas"]["emitted"])
    frame_rate_hz = emitted / pub_span_s if pub_span_s else None

    # Series for the chart: valid rows with a playback clock.
    series_mask = valid & (music >= 0)
    chart = {
        "clicks": clicks_b,
        "boom": boom_b,
    }

    def bar_values(block):
        st = block["stages"]
        return {
            "c": [st["c_capture_to_publish"]["stats_ms"]["p50"], st["c_capture_to_publish"]["stats_ms"]["p99"]],
            "d": [st["d_render"]["stats_ms"]["p50"], st["d_render"]["stats_ms"]["p99"]],
            "e_cpu": [st["e_pack_cpu"]["stats_ms"]["p50"], st["e_pack_cpu"]["stats_ms"]["p99"]],
            "e_wire": [WIRE_MS, WIRE_MS],
            "f": [FRAME_MS / 2.0, 0.99 * FRAME_MS],
            "b": [st["b_before_analysis"]["stats_ms"]["p50"], st["b_before_analysis"]["stats_ms"]["p99"]],
            "ap": [st["c_analysis_cpu"]["stats_ms"]["p50"], st["c_analysis_cpu"]["stats_ms"]["p99"]],
            "slot": [st["a_slot"]["stats_ms"]["p50"], st["a_slot"]["stats_ms"]["p99"]],
        }

    cv, bv = bar_values(clicks_b), bar_values(boom_b)
    # Order: clicks p50, clicks p99, boom p50, boom p99
    def across(key):
        return [cv[key][0], cv[key][1], bv[key][0], bv[key][1]]

    def bands(mask: np.ndarray) -> dict:
        m = mask & valid
        short = m & (hop_dt < 1000)
        ordinary = m & (hop_dt >= 5000)
        denom = int(m.sum()) or 1
        return {
            "short_hop_under_1ms": {
                "rows": int(short.sum()),
                "fraction": int(short.sum()) / denom,
                "capture_to_publish_ms": pct(c_ms[short]),
                "hop_dt_ms": pct(hop_dt[short] / 1000.0),
            },
            "ordinary_hop_at_least_5ms": {
                "rows": int(ordinary.sum()),
                "fraction": int(ordinary.sum()) / denom,
                "capture_to_publish_ms": pct(c_ms[ordinary]),
                "hop_dt_ms": pct(hop_dt[ordinary] / 1000.0),
            },
        }

    band_clicks = bands(in_clicks)
    band_boom = bands(in_boom)

    labels = ["Clicks\np50", "Clicks\np99", "BOOM\np50", "BOOM\np99"]
    xs = np.arange(4)

    fig = plt.figure(figsize=(13.6, 12.0), dpi=140, facecolor="white")
    gs = fig.add_gridspec(2, 2, height_ratios=[1.22, 1.0], hspace=0.52, wspace=0.28)
    ax = fig.add_subplot(gs[0, 0])
    ax2 = fig.add_subplot(gs[0, 1])
    ax3 = fig.add_subplot(gs[1, :])

    stacks = [
        ("(c)  Analysis and publish", across("c"), "#1f4e79", "white"),
        ("(d)  Render", across("d"), "#1e7a52", "white"),
        ("(e)  LED pack, CPU", across("e_cpu"), "#c47b17", "black"),
        ("(e)  LED wire, 5.100 ms", across("e_wire"), "#5e3c99", "white"),
        ("(f)  Wait for 100 fps tick", across("f"), "#5c6770", "white"),
    ]
    bottoms = np.zeros(4)
    for name, heights, color, tc in stacks:
        h = np.array(heights, dtype=float)
        ax.bar(xs, h, bottom=bottoms, color=color, width=0.72, label=name, zorder=2)
        for x, hh, b in zip(xs, h, bottoms):
            if hh >= 0.85:
                ax.text(x, b + hh / 2.0, f"{hh:.2f}", ha="center", va="center", color=tc, fontsize=8)
        bottoms = bottoms + h
    for x, total in zip(xs, bottoms):
        ax.text(x, total + 0.35, f"{total:.1f}", ha="center", va="bottom", fontsize=9, color="#1a1a1a")
    ax.set_xticks(xs, labels)
    ax.set_ylabel("Milliseconds")
    ax.set_ylim(0, float(bottoms.max()) * 1.12)
    ax.set_title("Path from the stamped sample to the end of the LED frame")
    ax.set_xlabel(
        "Render and LED-pack CPU are in this stack, between blue and purple.\n"
        "Clicks p50 / p99: render 0.197 / 0.277 ms, pack 0.128 / 0.131 ms. BOOM matches to 0.01 ms.",
        fontsize=8,
        labelpad=8,
    )
    ax.legend(
        loc="upper center", bbox_to_anchor=(0.5, -0.28), ncol=2, frameon=False, fontsize=8
    )
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.yaxis.grid(True, linestyle=":", color="#d0d5dd", zorder=0)
    ax.set_axisbelow(True)

    inside = [
        ("(b)  Before analysis, inside (c)", across("b"), "#d4760a", "black"),
        ("Analysis CPU, inside (c)", across("ap"), "#1f4e79", "white"),
    ]
    bottoms = np.zeros(4)
    for name, heights, color, tc in inside:
        h = np.array(heights, dtype=float)
        ax2.bar(xs, h, bottom=bottoms, color=color, width=0.72, label=name, zorder=2)
        for x, hh, b in zip(xs, h, bottoms):
            if hh >= 0.45:
                ax2.text(x, b + hh / 2.0, f"{hh:.2f}", ha="center", va="center", color=tc, fontsize=8)
        bottoms = bottoms + h
    slot_h = float(np.array(across("slot"), dtype=float)[0])
    ax2.axhline(slot_h, color="#9a3412", ls=":", lw=1.1, zorder=3, label="(a)  PDM slot 7.15 ms, not added")
    ax2.set_xticks(xs, labels)
    ax2.set_ylabel("Milliseconds")
    ax2.set_ylim(0, max(float(bottoms.max()), slot_h) * 1.22)
    ax2.set_title("What already sits inside the blue bar")
    ax2.set_xlabel(
        "Orange plus blue equals the left-hand blue bar.\nThe dotted line is the microphone block length. It is not stacked.",
        fontsize=8,
        labelpad=8,
    )
    ax2.legend(
        loc="upper center", bbox_to_anchor=(0.5, -0.28), ncol=1, frameon=False, fontsize=8
    )
    ax2.spines["top"].set_visible(False)
    ax2.spines["right"].set_visible(False)
    ax2.yaxis.grid(True, linestyle=":", color="#d0d5dd", zorder=0)
    ax2.set_axisbelow(True)

    # Time series. Subsample for the scatter if huge; 95k is fine rasterized.
    mx = music[series_mask]
    cy = c_ms[series_mask]
    click_pts = (mx >= first_click_s) & (mx < boom_start_s)
    boom_pts = (mx >= boom_start_s) & (mx < boom_end_s)
    other_pts = ~(click_pts | boom_pts)
    ax3.axvspan(first_click_s, boom_start_s, color="#1f4e79", alpha=0.06, label="Click section")
    ax3.axvspan(boom_start_s, boom_end_s, color="#9a3412", alpha=0.08, label="BOOM section")
    ax3.scatter(mx[other_pts], cy[other_pts], s=2, c="#98a2b3", rasterized=True, label="Sync and tail")
    ax3.scatter(mx[click_pts], cy[click_pts], s=2, c="#1f4e79", rasterized=True, label="Clicks")
    ax3.scatter(mx[boom_pts], cy[boom_pts], s=2, c="#9a3412", rasterized=True, label="BOOM")
    ax3.axvline(boom_start_s, color="#9a3412", lw=0.8, ls="--")
    ax3.axvline(boom_end_s, color="#9a3412", lw=0.6, ls=":")
    ax3.set_xlim(0, float(np.max(mx)) * 1.01)
    y_hi = float(np.percentile(cy, 99.9)) * 1.25
    ax3.set_ylim(0, max(12.0, y_hi))
    ax3.set_xlabel("Seconds into the playback (host clock at the moment the row was read)")
    ax3.set_ylabel("Publish − capture (ms)")
    ax3.set_title("Capture-to-publish delay across the recording")
    ax3.legend(loc="upper right", frameon=False, fontsize=8, markerscale=4)
    ax3.spines["top"].set_visible(False)
    ax3.spines["right"].set_visible(False)
    ax3.yaxis.grid(True, linestyle=":", color="#d0d5dd")
    n_above = int(np.sum(cy > ax3.get_ylim()[1]))
    fig.suptitle(
        "Titan device latency budget  ·  P5  ·  14 Oct 2026  ·  image f2f1983a",
        fontsize=14,
        color="#1a1a1a",
        x=0.02,
        ha="left",
    )
    fig.text(
        0.02,
        0.008,
        "Each left bar stacks that stage's own p50 or p99. The stack is not the percentile of one hop. "
        "The lower cloud is a hop published within 1 ms of the previous one (about 3.8 ms). "
        "The upper cloud is the ordinary hop (about 7 ms). "
        f"Points above the axis: {n_above}.",
        fontsize=8,
        color="#344054",
    )
    png_path = OUT / "budget.png"
    fig.savefig(png_path, dpi=140, bbox_inches="tight", facecolor="white")
    plt.close(fig)

    payload = {
        "title": "P5 device-side latency budget",
        "image_build": info["build"],
        "image_short": info["build"][:8],
        "source": info.get("source"),
        "uid": info.get("uid"),
        "clock_hz_reported": info.get("clock_hz"),
        "contract": info.get("contract"),
        "render_hz_reported": info.get("capabilities", {}).get("render_hz"),
        "timing_sha256": sha256(TIMING),
        "truth_sha256": sha256(TRUTH),
        "rows": n,
        "clock": {
            "cpu_hz": CPU_HZ,
            "fact": "info.json clock_hz is 1000000000. hal_entry.c passes SystemCoreClock into k1_fixture_initialise, which initialises the live clock. Capture microseconds divide the same DWT by SystemCoreClock.",
        },
        "wire": {
            "profile": 3,
            "pixels_per_din": 80,
            "bytes_per_pixel": 6,
            "period_ns": 1250,
            "reset_us": 300,
            "lanes_parallel": True,
            "formula": "emit_ns = pixels * bytes_per_pixel * 8 * period_ns + reset_us * 1000",
            "emit_ms": WIRE_MS,
            "source": "platform/ra8p1/ws281x_waveform.c k1_ws281x_gpt_cost_lanes; ws281x_diag.h profile 3; fixture_app.cpp submit profile 3",
        },
        "frame": {
            "period_us": 10000,
            "model": "uniform 0 to 10 ms, independent of the hop phase",
            "p50_ms": 5.0,
            "p99_ms": 9.9,
            "max_ms": 10.0,
        },
        "sections_definition": {
            "sample_rate": sr,
            "truth_seconds": truth["seconds"],
            "n_clicks": len(clicks),
            "first_click_s": first_click_s,
            "last_click_s": last_click_s,
            "click_s": truth["click_s"],
            "boom_start_sample": int(truth["boom_start"]),
            "boom_start_s": boom_start_s,
            "boom_end_s": boom_end_s,
            "boom_length_s": boom_len_s,
            "boom_source": truth["boom_source"],
            "sync_start_s": sync_start_s,
            "sync_end_s": sync_end_s,
            "rule": "Clicks: first truth click <= _music_s < boom_start. BOOM: boom_start <= _music_s < boom_start+75. _music_s is host monotonic time since afplay started, stamped once per USB read.",
        },
        "row_counts": {
            "total": n,
            "music_before_playback": int(music_neg.sum()),
            "lead_in_including_sync": int(in_lead.sum()),
            "clicks": int(in_clicks.sum()),
            "boom": int(in_boom.sum()),
            "tail_after_boom": int(in_tail.sum()),
            "music_s_min": float(np.min(music)),
            "music_s_max": float(np.max(music)),
            "unique_music_stamps": int(unique_music.size),
            "music_stamp_step_s": pct(music_step),
        },
        "integrity": {
            "write_seq_first": int(write_seq[0]),
            "write_seq_last": int(write_seq[-1]),
            "write_seq_span_inclusive": int(write_seq[-1] - write_seq[0] + 1),
            "missing_steps": gap_missing,
            "gap_events": gap_events,
            "duplicate_steps": dup_events,
            "backward_steps": back_events,
            "first_non_unit_steps": first_gaps,
            "hop_sequence_matches_write_seq": bool(np.all(hop_seq == write_seq)),
            "capture_time_zero": cap_zero,
            "publish_before_capture": pub_lt_cap,
            "capture_nonincreasing_among_nonzero": cap_back,
            "capture_nonincreasing_examples": cap_back_examples,
            "successive_capture_step_ms": cap_step_ms,
            "publish_span_s": pub_span_s,
            "capture_span_s": cap_span_s,
            "valid_rows": int(valid.sum()),
        },
        "device_frame_witness": {
            "emitted_during_capture": emitted,
            "publish_span_s": pub_span_s,
            "implied_fps": frame_rate_hz,
            "skipped_releases": report["deltas"].get("skipped_releases"),
            "emit_errors": report["deltas"].get("emit_errors"),
            "poll_max_cycles_since_boot": report["capture_service_after"]["poll_max_cycles"],
            "poll_max_ms_since_boot": report["capture_service_after"]["poll_max_cycles"] / 1e6,
            "analysis24_note": report.get("analysis24"),
        },
        "donor_hop_ms": DONOR_HOP_MS,
        "bands": {"clicks": band_clicks, "boom": band_boom},
        "clicks": clicks_b,
        "boom": boom_b,
        "chart_points_above_ylim": n_above,
        "png": str(png_path),
    }
    (OUT / "budget.json").write_text(json.dumps(payload, indent=2) + "\n")

    c50 = clicks_b["stages"]["c_capture_to_publish"]["stats_ms"]
    b50 = boom_b["stages"]["c_capture_to_publish"]["stats_ms"]
    j50 = clicks_b["stages"]["joint_measured_to_end_of_wire"]["stats_ms"]
    jb = boom_b["stages"]["joint_measured_to_end_of_wire"]["stats_ms"]
    lines = [
        f"rows {n}  valid {int(valid.sum())}  capture_zero {cap_zero}  publish<capture {pub_lt_cap}",
        f"write_seq {int(write_seq[0])}..{int(write_seq[-1])}  missing {gap_missing}  dup {dup_events}  back {back_events}",
        f"capture non-increasing (nonzero) {cap_back}",
        f"section rows  preroll {int(music_neg.sum())}  lead {int(in_lead.sum())}  clicks {int(in_clicks.sum())}  boom {int(in_boom.sum())}  tail {int(in_tail.sum())}",
        f"clicks capture->publish ms  p50 {c50['p50']:.3f}  p99 {c50['p99']:.3f}  max {c50['max']:.3f}",
        f"BOOM   capture->publish ms  p50 {b50['p50']:.3f}  p99 {b50['p99']:.3f}  max {b50['max']:.3f}",
        f"clicks to end of wire ms   p50 {j50['p50']:.3f}  p99 {j50['p99']:.3f}  max {j50['max']:.3f}  (no frame wait)",
        f"BOOM   to end of wire ms   p50 {jb['p50']:.3f}  p99 {jb['p99']:.3f}  max {jb['max']:.3f}  (no frame wait)",
        f"wire {WIRE_MS:.3f} ms profile 3, 80 px, one lane. frame wait model 0..10 ms, p50 5, p99 9.9",
        f"sum of stage percentiles clicks p50 {clicks_b['sum_of_stage_percentiles_ms']['p50']:.2f}  p99 {clicks_b['sum_of_stage_percentiles_ms']['p99']:.2f}",
        f"sum of stage percentiles BOOM   p50 {boom_b['sum_of_stage_percentiles_ms']['p50']:.2f}  p99 {boom_b['sum_of_stage_percentiles_ms']['p99']:.2f}",
        f"emitted frames {emitted} over {pub_span_s:.2f} s -> {frame_rate_hz:.2f} fps",
        f"wrote {OUT / 'budget.json'}",
        f"wrote {png_path}",
    ]
    print("\n".join(lines))


if __name__ == "__main__":
    main()
