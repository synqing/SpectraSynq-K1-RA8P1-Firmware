#!/usr/bin/env python3
"""Score the DBS pilot arms: P6-DBS-PILOT-CRITERIA-v1 (frozen 2026-10-14T22:48:39Z) as amended
by P6-DBS-PILOT-CRITERIA-v2-AMENDMENT (frozen 2026-10-14T23:02:04Z).

Measure: births on the board-0 deposit observation A_k (A_k >= 0.05, local maximum,
prominence >= 1.20 over the minimum of the 3 frames on each side), matched one-to-one in
time order to reference onsets in [t_o - 5 ms, t_o + 45 ms]. BOOM: salient set R (reference
picked_up_to_delta > 29.158) is matched first, then the ambiguous set A; births matched to
neither are false. The missed set is predeclared from the baseline only. Sonic: retention
and unanchored growth only (v2). Probes: exact planted times. Nothing reads light to pick a
reference event.

  uv run --quiet --with pandas==2.2.3 --with numpy==2.1.3 python scripts/p6_dbs_score.py missed RUN INPUT
  uv run ... python scripts/p6_dbs_score.py all RUN
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
RUNDIR = ROOT / "_scratch/waveform-migration-001/runs/P6-DBS-20261015"
V1 = RUNDIR / "DBS-CRITERIA-FROZEN.json"
V2 = RUNDIR / "DBS-CRITERIA-v2-AMENDMENT-FROZEN.json"
BOOM_REF = ROOT / "docs/evidence/P6/dbs/reference/boom-onsets.json"
SONIC_REF = ROOT / "docs/evidence/P6/dbs/reference/sonic-6-30-onsets.json"
STEP1 = RUNDIR / "step1"
PROBES = RUNDIR / "probes"

FLOOR = 0.05
PROMINENCE = 1.20
SENSITIVITY = (1.10, 1.33)
LEAD_S, LAG_S = 0.005, 0.045
CUE_WINDOW_S, CUE_PRE_S, CUE_RATIO = 0.030, 0.060, 1.5
SALIENCE = 29.158
ARMS = ["arm1", "arm2", "arm3", "arm4"]
PLANTS = ["plant_uniform", "plant_random", "plant_oracle"]


def check_frozen() -> None:
    v1 = json.loads(V1.read_text())
    v2 = json.loads(V2.read_text())
    assert v1["frozen_sha256"].startswith("bd48725f35a28e1c"), "v1 criteria changed"
    assert v2["frozen_sha256"].startswith("5df8497213f234a0"), "v2 amendment changed"
    assert hashlib.sha256(BOOM_REF.read_bytes()).hexdigest() == v2["boom_reference"]["sha256"], "BOOM reference changed"


def inputs() -> dict:
    return json.loads(V1.read_text())["inputs"]


def song_time(ticks: pd.DataFrame, inp: str) -> np.ndarray:
    spec = inputs()[inp]
    return ((ticks.t_s - spec["offset_s"]) / spec["scale"]).to_numpy()


def births(ticks: pd.DataFrame, t: np.ndarray, prominence: float = PROMINENCE) -> pd.DataFrame:
    a = ticks.deposit_peak.to_numpy(dtype=float)
    out = []
    for k in range(3, len(a) - 3):
        if a[k] < FLOOR or a[k] < a[k - 1] or a[k] < a[k + 1]:
            continue
        side = max(a[k - 3:k].min(), a[k + 1:k + 4].min())
        ratio = a[k] / side if side > 0 else np.inf
        if ratio >= prominence:
            out.append({"k": k, "t": t[k], "amp": a[k], "ratio": ratio})
    return pd.DataFrame(out, columns=["k", "t", "amp", "ratio"])


def greedy(onset_t: np.ndarray, birth_t: np.ndarray, used: set) -> dict[int, int]:
    pairs: dict[int, int] = {}
    for i, to in enumerate(onset_t):
        for j in np.where((birth_t >= to - LEAD_S) & (birth_t <= to + LAG_S))[0]:
            if int(j) not in used:
                used.add(int(j))
                pairs[i] = int(j)
                break
    return pairs


def board0(run: Path, inp: str, tag: str) -> pd.DataFrame:
    t = pd.read_csv(run / "arms" / inp / f"{tag}.ticks.csv")
    return t[t.board == 0].reset_index(drop=True)


def boom_reference() -> tuple[pd.DataFrame, pd.DataFrame]:
    rows = pd.DataFrame(json.loads(BOOM_REF.read_text())["onsets"])
    rows = rows[rows.scored].sort_values("time_s").reset_index(drop=True)
    return (rows[rows.picked_up_to_delta > SALIENCE].reset_index(drop=True),
            rows[rows.picked_up_to_delta <= SALIENCE].reset_index(drop=True))


def boom_eval(run: Path, inp: str, tag: str, prominence: float = PROMINENCE) -> dict:
    lo, hi = inputs()[inp]["score_song_s"]
    salient, ambiguous = boom_reference()
    ticks = board0(run, inp, tag)
    born = births(ticks, song_time(ticks, inp), prominence)
    used: set = set()
    pr = greedy(salient.time_s.to_numpy(), born.t.to_numpy(), used)
    pa = greedy(ambiguous.time_s.to_numpy(), born.t.to_numpy(), used)
    false = [j for j in range(len(born)) if j not in used and lo <= born.t.iloc[j] <= hi]
    return {"pairs": pr, "ambiguous_matched": len(pa), "false": len(false), "births": int(((born.t >= lo) & (born.t <= hi)).sum())}


def cue_present(inp: str, onset_t: np.ndarray) -> np.ndarray:
    spec = inputs()[inp]
    h = pd.read_csv(STEP1 / f"{inp}.csv")
    h = h[h.status == "accepted"].reset_index(drop=True)
    cap = ((h.capture_us - 1_000_000) / 1e6 - spec["offset_s"]) / spec["scale"]
    flux = h.raw_full_flux.to_numpy()
    out = np.zeros(len(onset_t), dtype=bool)
    for i, to in enumerate(onset_t):
        idx = np.where((cap >= to) & (cap <= to + CUE_WINDOW_S))[0]
        pre = np.where((cap >= to - CUE_PRE_S) & (cap < to))[0]
        if len(idx) and len(pre):
            out[i] = flux[idx[0]] >= CUE_RATIO * max(flux[pre].min(), 1e-9)
    return out


def cmd_missed(run: Path, inp: str) -> None:
    check_frozen()
    salient, _ = boom_reference()
    base = boom_eval(run, inp, "arm0")
    dense = (salient.context == "dense").to_numpy()
    cue = cue_present(inp, salient.time_s.to_numpy())
    missed = [float(salient.time_s[i]) for i in range(len(salient)) if dense[i] and i not in base["pairs"] and cue[i]]
    path = run / f"missed-set-{inp}.json"
    if path.exists():
        raise SystemExit(f"REFUSED: {path} exists (a predeclared set is never regenerated)")
    out = {"input": inp, "criteria": ["v1 bd48725f35a28e1c", "v2 5df8497213f234a0"],
           "baseline_tag": "arm0", "baseline_ticks_sha256": hashlib.sha256((run / "arms" / inp / "arm0.ticks.csv").read_bytes()).hexdigest(),
           "salient_onsets": int(len(salient)), "salient_dense": int(dense.sum()),
           "salient_cue_present": int(cue.sum()), "baseline_matched_salient": len(base["pairs"]),
           "baseline_false_births": base["false"], "baseline_births": base["births"],
           "missed_count": len(missed), "missed_onset_times_s": missed}
    path.write_text(json.dumps(out, indent=2) + "\n")
    path.chmod(0o444)
    print(json.dumps({k: v for k, v in out.items() if k != "missed_onset_times_s"}, indent=1))
    print("sha256", hashlib.sha256(path.read_bytes()).hexdigest())


def score_boom(run: Path, inp: str, tags: list[str]) -> list[dict]:
    salient, _ = boom_reference()
    missed = set(json.loads((run / f"missed-set-{inp}.json").read_text())["missed_onset_times_s"])
    idx = [i for i, t in enumerate(salient.time_s) if float(t) in missed]
    rows = []
    for p in (PROMINENCE, *SENSITIVITY):
        base = boom_eval(run, inp, "arm0", p)
        for tag in tags:
            r = boom_eval(run, inp, tag, p)
            rec = sum(1 for i in idx if i in r["pairs"])
            retained = sum(1 for i in base["pairs"] if i in r["pairs"])
            row = {"input": inp, "tag": tag, "prominence": p, "births": r["births"], "matched_salient": len(r["pairs"]),
                   "ambiguous_matched": r["ambiguous_matched"], "false_births": r["false"],
                   "baseline_matched_salient": len(base["pairs"]), "baseline_false_births": base["false"],
                   "missed_set": len(idx), "recovered": rec, "recovery_rate": rec / max(1, len(idx)),
                   "retained": retained, "retention_rate": retained / max(1, len(base["pairs"]))}
            row["M1"] = len(idx) > 0 and rec >= 10 and rec / len(idx) >= 0.25
            row["M2"] = row["retention_rate"] >= 0.95
            row["M3"] = r["false"] <= base["false"]
            rows.append(row)
    return rows


def score_sonic(run: Path, tags: list[str]) -> list[dict]:
    inp = "sonic_room"
    lo, hi = inputs()[inp]["score_song_s"]
    ref = pd.DataFrame(json.loads(SONIC_REF.read_text())["onsets"])
    ref_t = np.sort(ref.time_frame_s.to_numpy())
    def anchored(born: pd.DataFrame) -> tuple[pd.DataFrame, int]:
        born = born[(born.t >= lo) & (born.t <= hi)].reset_index(drop=True)
        used: set = set()
        greedy(ref_t, born.t.to_numpy(), used)
        return born, len(born) - len(used)
    tb = board0(run, inp, "arm0")
    base, base_un = anchored(births(tb, song_time(tb, inp)))
    rows = []
    for tag in tags:
        ta = board0(run, inp, tag)
        arm, un = anchored(births(ta, song_time(ta, inp)))
        kept = sum(1 for k in base.k if np.any(np.abs(arm.k.to_numpy() - k) <= 1))
        rows.append({"input": inp, "tag": tag, "baseline_births": len(base), "births": len(arm),
                     "retained_baseline_births": kept, "S1_retention_rate": kept / max(1, len(base)),
                     "unanchored": un, "baseline_unanchored": base_un,
                     "S1": kept / max(1, len(base)) >= 0.95, "S2": un <= base_un})
    return rows


def score_probes(run: Path, tags: list[str]) -> list[dict]:
    rows = []
    for name in ("sustain", "weak_alone", "weak_over_sustain", "quiet_recovery", "spacing_200", "spacing_100", "spacing_75"):
        meta = json.loads((PROBES / f"{name}.json").read_text())
        ev = np.array(sorted(e["t_s"] for e in meta["events"]))
        if name == "quiet_recovery":
            ev = np.array(sorted(e["t_s"] for e in meta["events"] if e["level_db_re_ref"] <= -12.0))
        res = {}
        for tag in ["arm0", *tags]:
            t = pd.read_csv(run / "arms" / f"probe_{name}" / f"{tag}.ticks.csv")
            t = t[t.board == 0].reset_index(drop=True)
            born = births(t, t.t_s.to_numpy())
            if name == "sustain":
                res[tag] = int(((born.t >= 2.0) & (born.t < 12.0)).sum())
            else:
                res[tag] = len(greedy(ev, born.t.to_numpy(), set()))
        for tag in tags:
            ok = res[tag] <= res["arm0"] if name == "sustain" else res[tag] >= res["arm0"]
            rows.append({"probe": name, "tag": tag, "value": res[tag], "baseline": res["arm0"],
                         "events": int(len(ev)), "measure": "births in layer" if name == "sustain" else "planted matched",
                         "gating": not name.startswith("spacing"), "ok": bool(ok)})
    return rows


def cmd_all(run: Path) -> None:
    check_frozen()
    out = {"boom": [], "sonic": [], "probes": []}
    for inp in ("boom_source", "boom_room"):
        out["boom"] += score_boom(run, inp, ARMS + [p for p in PLANTS if (run / "arms" / inp / f"{p}.ticks.csv").exists()])
    out["sonic"] = score_sonic(run, ARMS)
    out["probes"] = score_probes(run, ARMS)
    (run / "scores").mkdir(exist_ok=True)
    (run / "scores" / "scores.json").write_text(json.dumps(out, indent=2) + "\n")
    for r in out["boom"]:
        if r["prominence"] == PROMINENCE:
            print(f"{r['input']:12s} {r['tag']:14s} births {r['births']:4d} matchedR {r['matched_salient']:3d}/{r['baseline_matched_salient']:3d}b "
                  f"false {r['false_births']:4d}/{r['baseline_false_births']:4d}b recovered {r['recovered']:3d}/{r['missed_set']:3d} "
                  f"retain {r['retention_rate']:.3f}  M1 {r['M1']} M2 {r['M2']} M3 {r['M3']}")
    for r in out["sonic"]:
        print(f"sonic_room   {r['tag']:14s} births {r['births']}/{r['baseline_births']}b retain {r['S1_retention_rate']:.3f} unanchored {r['unanchored']}/{r['baseline_unanchored']}b S1 {r['S1']} S2 {r['S2']}")
    for r in out["probes"]:
        print(f"probe {r['probe']:18s} {r['tag']:6s} {r['measure']:16s} {r['value']:3d} vs base {r['baseline']:3d} of {r['events']:3d} ok {r['ok']} gating {r['gating']}")


if __name__ == "__main__":
    command, run = sys.argv[1], Path(sys.argv[2])
    if command == "missed":
        cmd_missed(run, sys.argv[3])
    elif command == "all":
        cmd_all(run)
    else:
        raise SystemExit("unknown command")
