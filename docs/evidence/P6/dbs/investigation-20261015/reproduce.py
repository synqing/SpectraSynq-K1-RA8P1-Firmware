#!/usr/bin/env python3
"""Read-only reproduction of the DBS scorer plateau defect and v9 transfer curve.

Run from the product checkout with its existing NumPy/Pandas environment:
  uv run --quiet --with numpy==2.1.3 --with pandas==2.2.3 python \
    docs/evidence/P6/dbs/investigation-20261015/reproduce.py --root "$PWD"
No device access, product mutation, or revision of the frozen verdict.
"""
import argparse
import ast
import hashlib
import json
import math
from pathlib import Path
import numpy as np
import pandas as pd

p = argparse.ArgumentParser()
p.add_argument('--root', type=Path)
p.add_argument('--scorer', type=Path)
args = p.parse_args()
scorer = args.scorer or args.root / 'scripts/p6_dbs_score.py'
source = scorer.read_text()
tree = ast.parse(source)
keep = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ('births', 'greedy')]
assert len(keep) == 2
ns = dict(np=np, pd=pd, FLOOR=.05, PROMINENCE=1.20, LEAD_S=.005, LAG_S=.045)
exec(compile(ast.Module(body=keep, type_ignores=[]), str(scorer), 'exec'), ns)
times = np.arange(20, dtype=float) / 100
amplitude = np.full(20, .435)
amplitude[8:11] = .58
births = ns['births'](pd.DataFrame({'deposit_peak': amplitude}), times)
matched = ns['greedy'](np.array([.08]), births.t.to_numpy(), set())
assert births.t.to_list() == [.08, .09, .1]
assert len(matched) == 1
curve = []
for db in (-33, -24, -18, -12, -9, -6, -3, 0):
    level = min(1., max(0., (db + 33) / 24))
    curve.append({'vu_db': db, 'level': level, 'requested_amplitude': .580092 * level ** 1.5})
assert len(set(x['requested_amplitude'] for x in curve if x['vu_db'] >= -9)) == 1
print(json.dumps({
    'scope': 'diagnosis only; no new acceptance verdict',
    'scorer_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'single_plateau': {'onset_s': .08, 'births_s': births.t.to_list(),
                       'matched': len(matched), 'false': len(births)-len(matched)},
    'curve': curve,
    'ceiling_vu': 10 ** (-9/20),
    'attenuation_gain_range': [.75, 1.],
    'maximum_flat_baseline_contrast_ratio': 1/.75,
    'same_location_history_after_109ms': math.exp(-.109/.29),
    'nominal_spacing_pixels_at_109ms': 405*.109,
    'note': 'History decay is not overlap proof: advection, deposition position and optics matter.'
}, indent=2))
