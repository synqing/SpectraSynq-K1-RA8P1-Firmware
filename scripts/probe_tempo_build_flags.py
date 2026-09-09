#!/usr/bin/env python3
"""Read-only pinned-source experiment: isolate compiler-profile dependence of the tempo golden."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from verify_imports import REFERENCE,PIN

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=False)
names=['core/audio/tempo_acf.cpp','core/audio/tempo_acf.h','core/audio/tempo_tracker.cpp','core/audio/tempo_tracker.h','core/audio/audio_rate_config.h','test/test_tempo_acf_parity/test_main.cpp','test/fixtures/tempo_v2_flywheel.golden.jsonl','platformio.ini']
profiles={'strict_O2':['-O2','-fno-fast-math','-ffp-contract=off'],
          'pinned_native_O0':['-O0','-ffast-math','-fno-finite-math-only','-fno-unsafe-math-optimizations'],
          'pinned_native_O2':['-O2','-ffast-math','-fno-finite-math-only','-fno-unsafe-math-optimizations'],
          'pinned_native_no_contract_O2':['-O2','-ffast-math','-fno-finite-math-only','-fno-unsafe-math-optimizations','-ffp-contract=off']}
receipt={'label':'HOST','source_pin':PIN,'sources':{},'profiles':{}}
with tempfile.TemporaryDirectory(prefix='k1-tempo-profile-') as scratch:
    root=Path(scratch)
    for name in names:
        path=root/name; path.parent.mkdir(parents=True,exist_ok=True)
        data=subprocess.check_output(['git','-C',str(REFERENCE),'show',f'{PIN}:{name}']); path.write_bytes(data)
        receipt['sources'][name]=hashlib.sha256(data).hexdigest()
    for label,flags in profiles.items():
        executable=args.output/label
        command=['c++','-std=gnu++17',*flags,'-I'+str(root),*[str(root/n) for n in names if n.endswith('.cpp')],'-o',str(executable)]
        subprocess.run(command,check=True,capture_output=True)
        result=subprocess.run([str(executable)],cwd=root,capture_output=True,text=True,timeout=120)
        (args.output/(label+'.log')).write_text(result.stdout+result.stderr)
        receipt['profiles'][label]={'command':command,'exit':result.returncode,'output':result.stdout,'stderr':result.stderr}
        print(label,result.returncode,result.stdout[:600],flush=True)
(args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
