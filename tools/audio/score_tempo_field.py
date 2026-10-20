#!/usr/bin/env python3
"""Run the independent host tests and actual source-mutation negative controls."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/evidence/tempo-field/20260928/oracle-results.json'

def main():
    runs=[]
    for mutation in (None,'phase_sign','time_endpoint','freshness'):
        env=os.environ.copy();env.pop('AP_TEMPO_PHASE_MUTATION',None)
        if mutation:env['AP_TEMPO_PHASE_MUTATION']=mutation
        run=subprocess.run([sys.executable,str(ROOT/'scripts/test_tempo_field.py')],
                           cwd=ROOT,env=env,capture_output=True,text=True)
        log=ROOT/f'docs/evidence/tempo-field/20260928/oracle-{mutation or "baseline"}.log'
        log.write_text(run.stdout+run.stderr)
        # Build/import errors are never mutation-kill evidence.
        killed=run.returncode!=0 and 'FAIL:' in run.stderr and 'ERROR:' not in run.stderr
        runs.append({'mutation':mutation,'returncode':run.returncode,
                     'expected_result': 'tests fail' if mutation else 'tests pass',
                     'satisfied':killed if mutation else run.returncode==0,
                     'log':str(log.relative_to(ROOT))})
    paths=['scripts/test_tempo_field.py','tests/audio/tempo_oracle_bridge.cpp',
           'tools/audio/phase_oracle.py','tools/audio/score_tempo_field.py',
           'src/k1/core/audio/tempo_phase_tracks.cpp','src/k1/core/audio/tempo_phase_tracks.h',
           'src/k1/core/audio/tempo_field_publisher.cpp','src/k1/core/audio/tempo_tracker.cpp',
           'src/k1/core/visual/tempo_reference_scene.cpp','tests/audio/tempo_reference_scene_test.cpp',
           'src/k1/contract/tempo_field_v1.h','docs/evidence/tempo-field/20260928/oracle-freeze.md']
    result={'scope':'host mathematical phase and direct lifecycle seam only',
            'not_tested':['musical beat accuracy','target timing/capture','physical light'],
            'runs':runs,'sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}}
    OUT.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'all_expected_results':all(r['satisfied'] for r in runs),'evidence':str(OUT)},indent=2))
    return 0 if all(r['satisfied'] for r in runs) else 1

if __name__=='__main__':sys.exit(main())
