#!/usr/bin/env python3
"""Exact state/output comparison against frozen candidate-006 ASRC; no device I/O."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
P=ROOT/'platform/ra8p1'
REFERENCE=ROOT/'tests/host/fixtures/asrc_006_reference.c'
PIN='005202d73d26e8e3ed2fd49b124ee1d3a702c59788653f3a1fa532d0ee025797'
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    assert digest(REFERENCE)==PIN
    sources=[REFERENCE,P/'k1_asrc_24k.c',P/'k1_asrc_24k.h',Path(__file__),ROOT/'tests/host/test_asrc_exact.c']
    before={str(p.relative_to(ROOT)):digest(p) for p in sources}
    rows=[]
    for profile in ['emotiscope','legacy']:
        flags=['cc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(P)]
        if profile=='emotiscope':flags+=['-DK1_EMOTISCOPE_PORT=1']
        renames=['-Dk1_asrc24_'+name+'=oracle_'+name for name in ['reset','set_rate','push','pull180','phase_increment_q16','required_for_pull180']]
        commands=[flags+renames+['-c',str(REFERENCE),'-o',str(out/(profile+'-oracle.o'))], flags+[str(ROOT/'tests/host/test_asrc_exact.c'),str(P/'k1_asrc_24k.c'),str(out/(profile+'-oracle.o')),'-lm','-o',str(out/profile)], [str(out/profile)]]
        for i,cmd in enumerate(commands):
            result=subprocess.run(cmd,capture_output=True,text=True)
            (out/(profile+'-'+str(i)+'.log')).write_text(result.stdout+result.stderr)
            rows.append({'profile':profile,'command':cmd,'exit':result.returncode})
            assert result.returncode==0,result.stdout+result.stderr
        print(profile,'PASS')
    unchanged=before=={str(p.relative_to(ROOT)):digest(p) for p in sources}
    (out/'receipt.json').write_text(json.dumps({'pass':unchanged,'source_unchanged':unchanged,'sources':before,'results':rows,'scope':'host exact arithmetic/state parity; no physical timing claim'},indent=2)+'\n')
    assert unchanged
if __name__=='__main__':main()
