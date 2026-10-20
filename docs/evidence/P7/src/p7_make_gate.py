#!/usr/bin/env python3
"""Lane P7: write the (unfrozen) synthetic gate criterion. Freeze it next with scripts/freeze_tolerance.py.

  p7_make_gate.py OUT_GATE.json TRUTH_DIR
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import p7_gate  # noqa: E402
import p7_synth  # noqa: E402

CAPTAIN_PASS = [
    "lock within 4 beats;",
    "reported BPM within ±4 % of the annotated level (±2 % on synthetic cases);",
    "**zero** octave or 3:2 flips while locked;",
    "re-acquire within 2 beats after a tempo step;",
    "low confidence (never a false lock) on the drumless and silence cases.",
]


def main(out: Path, truth_dir: Path) -> int:
    truth_dir.mkdir(parents=True, exist_ok=True)
    cases = []
    for c in p7_synth.case_list():
        x, truth = p7_synth.render(c)
        s16 = p7_synth.to_s16(x).tobytes()
        tp = truth_dir / f"{c['id']}.truth.json"
        tp.write_text(json.dumps(truth) + "\n")
        cases.append({**c, "pcm_sha256": hashlib.sha256(s16).hexdigest(), "truth_sha256": p7_gate.file_sha256(tp),
                      "samples": len(s16) // 2})
    gate = {
        "gate_id": "P7-GATE-SYNTHETIC-v1",
        "lane": "Titan lag-free lane/titan-lagfree-20261014, package P7 step 2(a)",
        "captain_ruling": "Tempo half-time lock: MUST be fixed to a standard that guarantees it works — see the gate "
                          "in P7. Treat any gate failure as no-ship.",
        "captain_pass_criteria_verbatim": CAPTAIN_PASS,
        "captain_rule": "The gate is never widened. Adding corpus tracks re-freezes it.",
        "scope": "Synthetic suite (a) only. The real corpus (b) is a proposal, not part of this freeze.",
        "suite_pass_rule": "100 % of cases pass every applicable criterion. Any failing case is a gate failure (no-ship).",
        "tolerance": {"bpm_relative": 0.02},
        "definitions": {
            "update": "one TempoTrackerEvent with updated=true (every 3rd 7.5 ms AP frame, 44.44 Hz); t_s is the end of "
                      "the analysed 24 kHz hop in input time",
            "locked": "TempoTrackerEvent.locked",
            "asserting": f"locked or confidence >= {p7_gate.ASSERT_CONFIDENCE} (renderer tempo gate floor, "
                         "inhouse_waveform.cpp:347 clamp01((c-0.30)/0.30))",
            "truth(u)": "60 / interval of the annotated beats holding u (kick/backbeat level; first and last interval "
                        "extended outward)",
            "correct(u)": "|bpm - truth| <= tol * truth",
            "P": "annotated beat period at the start of the judged segment",
            "grace_beats": p7_gate.GRACE_BEATS,
            "grace_meaning": "acquire: case start and music after a silence gap (lock within 4 beats); step: first "
                             "segment after a tempo step (re-acquire within 2 beats); release: a silence gap may keep "
                             "asserting for 2 beats of the preceding tempo; none_from_start: pad and lead-in, no grace",
            "metrical_relatives": p7_gate.RELATIVES,
        },
        "criteria": {
            "LOCK": "every update in [a + G*P, b) of every tempo segment is locked and correct (locked within 4 beats / "
                    "re-acquired within 2 beats, and held to the end of the segment)",
            "BPM": "every asserting update in [a + G*P, b) of every tempo segment is correct",
            "FLIP": "zero locked updates anywhere in a tempo segment at a metrical relative (within tol) that are not "
                    "correct; exempt only: a step segment's grace window while bpm equals the previous segment's "
                    "truth within tol (hold-over)",
            "LOWCONF": "no asserting update in [a + G, b) of any none segment (drumless pad, lead-in, silence gap)",
        },
        "reported_not_gating": {
            "acquired_beats": "start of the final run of locked+correct updates, in beats from the segment start",
            "phase_ms": "beat_tick update instants against the nearest annotated beat (median and p95 |error|). The P7 "
                        "defaults name phase +-25 ms; the P7 pass list does not. Adding it as a criterion is a "
                        "narrowing re-freeze (RULING NEEDED).",
        },
        "input_chain": "p7_synth.py 40 kHz int16 mono (dBFS peak; -72 dBFS RMS seeded noise floor) -> k1_poly24 "
                       "(source 40000 Hz) -> InhouseAudio analysis_rate_hz 24000, hop 180 -> tempo demand on board 1 "
                       "-> product tempo_tracker.cpp / tempo_acf.cpp (unmodified); the probe's external baseline copy "
                       "is asserted bit-identical (bpm, confidence) to the product tracker on every update",
        "probe_build": {"flags": ["-O2", "-fno-fast-math", "-DK1_EMOTISCOPE_PORT=1", "-Iplatform/ra8p1", "-Isrc",
                                  "-Isrc/k1"],
                        "product_sources": ["platform/ra8p1/inhouse_audio.cpp", "src/k1/core/visual/inhouse_waveform.cpp",
                                            "src/k1/core/audio/gdft_goertzel.cpp", "src/k1/core/audio/tempo_tracker.cpp",
                                            "src/k1/core/audio/tempo_acf.cpp", "src/k1/core/visual/product_palette.cpp",
                                            "src/k1/core/visual/product_palette_data.cpp"],
                        "compiler": "host Apple clang (oracle), not the M85 binary"},
        "tools": {
            "generator": {"file": "p7_synth.py", "sha256": p7_gate.file_sha256(HERE / "p7_synth.py")},
            "scorer": {"file": "p7_gate.py", "sha256": p7_gate.file_sha256(HERE / "p7_gate.py")},
            "probe": {"file": "p7_tempo_probe.cpp", "sha256": p7_gate.file_sha256(HERE / "p7_tempo_probe.cpp")},
            "runner": {"file": "p7_run_gate.py", "sha256": p7_gate.file_sha256(HERE / "p7_run_gate.py")},
        },
        "numpy": np.__version__,
        "case_count": len(cases),
        "cases": cases,
    }
    out.write_text(json.dumps(gate, indent=1) + "\n")
    print(json.dumps({"cases": len(cases), "out": str(out)}))
    return 0


if __name__ == "__main__":
    sys.exit(main(Path(sys.argv[1]), Path(sys.argv[2])))
