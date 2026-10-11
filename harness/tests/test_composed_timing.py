#!/usr/bin/env python3
"""Host-only tempo candidate clock proof; never a musical downbeat claim."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'_scratch'/'test_composed_timing';OUT.mkdir(parents=True,exist_ok=True)
# Bind the independent test's literal48..143 BPM oracle to the real donor formula.
defines=(ROOT/'platform/ra8p1/emotiscope_donor/global_defines.h').read_text()
tempo=(ROOT/'platform/ra8p1/emotiscope_donor/tempo.h').read_text()
assert re.search(r'#define\s+NUM_TEMPI\s+\(96\)',defines)
assert re.search(r'#define\s+TEMPO_LOW\s+\(48\)',defines)
assert '#define TEMPO_HIGH (TEMPO_LOW + NUM_TEMPI)' in defines
for text in ('float progress = float(i) / NUM_TEMPI;', 'float tempi_range = TEMPO_HIGH - TEMPO_LOW;',
             'float tempo = tempi_range * progress + TEMPO_LOW;', 'tempi_bpm_values_hz[i] = tempo / 60.0;',
             'tempi[i].target_tempo_hz = tempi_bpm_values_hz[i];'):
    assert text in tempo, 'donor frequency source changed: '+text
records=[]
commands=[['clang++','-std=c++17','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
           '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I.',
           'harness/tests/test_composed_timing.cpp','-o',str(OUT/'test_composed_timing')],
          [str(OUT/'test_composed_timing')]]
code=0
for cmd in commands:
    p=subprocess.run(cmd,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    records.append({'argv':cmd,'exit_code':p.returncode,'output':p.stdout});print(p.stdout,end='')
    if p.returncode:code=p.returncode;break
inputs=['platform/ra8p1/composed_timing.h','harness/tests/test_composed_timing.cpp',
        'harness/tests/test_composed_timing.py','platform/ra8p1/emotiscope_donor/tempo.h',
        'platform/ra8p1/emotiscope_donor/global_defines.h']
(OUT/'receipt.json').write_text(json.dumps({'utc':datetime.now(timezone.utc).isoformat(),
    'target':'host only','records':records,'sha256_inputs':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs}},indent=2)+'\n')
raise SystemExit(code)
