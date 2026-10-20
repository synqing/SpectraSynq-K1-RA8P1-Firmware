#!/usr/bin/env python3
"""R05 feasibility. Score OnsetBeat events already written by p6_onset_pre_agc.

No detector is designed here. A cue is one of the detector's own event flags.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

DROPS = [
    ("boom_source", 55.7, "BOOM 55.7 s", (47.0, 48.5)),
    ("sonic_source", 27.7, "Sonic 27.7 s", (19.7, 21.2)),
    ("sonic_source", 54.7, "Sonic 54.7 s", (45.6, 47.1)),
    ("carte_source", 27.4, "Carte 27.4 s (seen)", (24.4, 25.9)),
]
CLASSES = (
    ("kick", "kick_strength", "kick_flux", "#c8361f"),
    ("snare", "snare_strength", "snare_flux", "#1d4e89"),
    ("hat", "hat_strength", "hat_flux", "#b8860b"),
    ("transient", "transient_strength", "full_flux", "#2f6b3a"),
)


def p95(x: np.ndarray) -> float:
    if len(x) == 0:
        return float("nan")
    return float(np.percentile(x, 95))


def analyse(hop: dict[str, pd.DataFrame]) -> dict:
    per_drop = []
    class_hits = {name: [] for name, *_ in CLASSES}
    class_climax = {name: [] for name, *_ in CLASSES}
    for tag, td, title, climax in DROPS:
        d = hop[tag]
        window = d[(d.t_song_s >= td - 0.300) & (d.t_song_s <= td + 0.300)]
        build = d[(d.t_song_s >= climax[0]) & (d.t_song_s < climax[1])]
        pre = d[(d.t_song_s >= td - 8.0) & (d.t_song_s < td - 0.5)]
        row = {"drop": title, "tag": tag, "t_d": td, "climax": list(climax), "classes": {}}
        for name, strength, flux, _ in CLASSES:
            events = window[window[name] == 1]
            after = events[events.t_song_s >= td - 1e-9]
            hit = events[(events.t_song_s >= td - 1e-9) & (events.t_song_s <= td + 0.050 + 1e-9)]
            climax_events = build[build[name] == 1]
            first = None if events.empty else float(events.t_song_s.iloc[0] - td)
            first_after = None if after.empty else float(after.t_song_s.iloc[0] - td)
            use = after.iloc[0] if len(after) else (events.iloc[0] if len(events) else None)
            flux_p95_climax = p95(build[flux].to_numpy())
            flux_p95_pre = p95(pre[flux].to_numpy())
            info = {
                "events_in_plot": int(len(events)),
                "first_event_rel_s": first,
                "first_event_at_or_after_anchor_s": first_after,
                "in_0_to_50ms": int(len(hit)),
                "climax_events": int(len(climax_events)),
                "strength": None if use is None else float(use[strength]),
                "uncapped_flux": None if use is None else float(use[flux]),
                "flux_p95_climax": flux_p95_climax,
                "flux_p95_build_8s": flux_p95_pre,
                "strength_over_climax_p95": None if use is None or not np.isfinite(flux_p95_climax) or flux_p95_climax <= 0
                    else float(use[flux]) / flux_p95_climax,
            }
            row["classes"][name] = info
            class_hits[name].append(len(hit) > 0)
            class_climax[name].append(len(climax_events) == 0)
        per_drop.append(row)
    cues = {}
    for name, *_ in CLASSES:
        hits = class_hits[name]
        quiet = class_climax[name]
        cues[name] = {
            "hits_every_drop_inside_50ms": all(hits),
            "silent_on_every_climax": all(quiet),
            "feasible": all(hits) and all(quiet),
            "per_drop_hit": hits,
            "per_drop_climax_silent": quiet,
        }
    return {"drops": per_drop, "cues": cues,
            "any_feasible_cue": any(c["feasible"] for c in cues.values())}


def figures(hop: dict[str, pd.DataFrame], out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(4, 1, figsize=(12.4, 12.8), sharex=True)
    for ax, (tag, td, title, climax) in zip(axes, DROPS):
        d = hop[tag]
        w = d[(d.t_song_s >= td - 0.300) & (d.t_song_s <= td + 0.300)].copy()
        x = (w.t_song_s - td) * 1000.0
        ax.axvspan(0, 50, color="#c8361f", alpha=0.12, label="[0, +50] ms")
        ax.axvline(0, color="black", lw=0.8)
        for name, strength, flux, colour in CLASSES:
            series = w[flux].to_numpy()
            scale = np.percentile(series, 99) if np.any(series > 0) else 1.0
            ax.plot(x, series / max(scale, 1e-9), color=colour, lw=1.1, alpha=0.85, label=f"{name} flux")
            ev = w[w[name] == 1]
            if len(ev):
                y = np.full(len(ev), 1.08 + 0.12 * [c[0] for c in CLASSES].index(name))
                ax.scatter((ev.t_song_s - td) * 1000.0, y, s=36, color=colour, zorder=3, label=f"{name} event")
        ax.set_ylim(-0.05, 1.7)
        ax.set_xlim(-300, 300)
        ax.set_ylabel("flux / its 99th\npercentile in this window")
        ax.set_title(title, loc="left", fontsize=13)
        ax.grid(alpha=0.25)
        if ax is axes[0]:
            handles, labels = ax.get_legend_handles_labels()
    axes[-1].set_xlabel("milliseconds after the annotated drop")
    fig.legend(handles, labels, ncol=4, fontsize=9, loc="upper center", bbox_to_anchor=(0.5, 1.02))
    fig.suptitle("OnsetBeat on pre-AGC linear_shape. Dots are the detector's own events.", fontsize=14, y=1.06)
    fig.tight_layout()
    fig.savefig(out / "onset-around-drops.png", dpi=140)
    plt.close(fig)

    # One close panel per drop so the 50 ms window is readable on its own.
    for tag, td, title, _ in DROPS:
        d = hop[tag]
        w = d[(d.t_song_s >= td - 0.300) & (d.t_song_s <= td + 0.300)]
        fig, ax = plt.subplots(figsize=(11.2, 4.2))
        x = (w.t_song_s - td) * 1000.0
        ax.axvspan(0, 50, color="#c8361f", alpha=0.12)
        ax.axvline(0, color="black", lw=0.8)
        for name, _, flux, colour in CLASSES:
            series = w[flux].to_numpy()
            scale = np.percentile(series, 99) if np.any(series > 0) else 1.0
            ax.plot(x, series / max(scale, 1e-9), color=colour, lw=1.3, label=f"{name} flux")
            ev = w[w[name] == 1]
            ax.scatter((ev.t_song_s - td) * 1000.0, np.ones(len(ev)) * (1.15 if name != "kick" else 1.35),
                       s=42, color=colour, zorder=3, label=f"{name} event")
        ax.set_xlim(-300, 300)
        ax.set_ylim(-0.05, 1.6)
        ax.set_xlabel("milliseconds after the drop")
        ax.set_ylabel("flux, each band scaled by its own 99th percentile")
        ax.set_title(title)
        ax.grid(alpha=0.25)
        ax.legend(ncol=4, fontsize=8)
        fig.tight_layout()
        slug = title.split()[0].lower() + f"-{td:.1f}".replace(".", "p")
        fig.savefig(out / f"{slug}.png", dpi=140)
        plt.close(fig)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--hop", type=Path, required=True)
    ap.add_argument("--out-json", type=Path, required=True)
    ap.add_argument("--figures", type=Path, required=True)
    a = ap.parse_args()
    hop = {name: pd.read_csv(a.hop / f"{name}.csv") for name in ("boom_source", "sonic_source", "carte_source")}
    res = analyse(hop)
    # Gate scale, so a silent detector is explained.
    res["energy"] = {}
    for name, d in hop.items():
        music = d[d.silence == 0]
        res["energy"][name] = {
            "rows": int(len(d)),
            "events_kick": int(d.kick.sum()),
            "events_snare": int(d.snare.sum()),
            "events_hat": int(d.hat.sum()),
            "events_transient": int(d.transient.sum()),
            "energy_pre_p50": float(music.energy_pre.median()) if len(music) else None,
            "energy_post_p50": float(music.energy_post.median()) if len(music) else None,
        }
    a.out_json.parent.mkdir(parents=True, exist_ok=True)
    a.out_json.write_text(json.dumps(res, indent=2) + "\n")
    figures(hop, a.figures)
    print("any_feasible_cue", res["any_feasible_cue"])
    for name, cue in res["cues"].items():
        print(f"{name}: hit_all={cue['hits_every_drop_inside_50ms']} climax_silent={cue['silent_on_every_climax']} feasible={cue['feasible']}")


if __name__ == "__main__":
    main()
