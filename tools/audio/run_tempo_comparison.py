#!/usr/bin/env python3
"""Replay frozen PCM through the complete runtime and compare every legacy field."""
import argparse,hashlib,json,subprocess,sys,itertools
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from replay_live_k1 import compile_replay
E=ROOT/'docs/evidence/tempo-field/20260928'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=E)
    parser.add_argument('--baseline',type=Path,default=E/'baseline')
    args=parser.parse_args()
    evidence=args.output.resolve(); evidence.mkdir(parents=True,exist_ok=True)
    build=evidence/'candidate-build';build.mkdir(exist_ok=False)
    exe=compile_replay(build)
    manifest=ROOT/'tests/fixtures/tempo_field/v1/pcm/manifest.json'
    fixtures=json.loads(manifest.read_text())['fixtures']
    results=[]
    # Music first supplies the actual audition to the visual owner while other rows run.
    fixtures.sort(key=lambda f: (not f['id'].startswith('song'),f['id']))
    for f in fixtures:
        pcm=manifest.parent/f['path']
        for label,field,phase in [('candidate','on','on'),('field-off','off','off'),('phase-off','on','off')]:
            out=evidence/label;out.mkdir(exist_ok=True)
            trace=out/(f['id']+'.jsonl')
            args=[str(exe),str(pcm),'normal',str(trace),field,phase,'scene' if label=='candidate' else 'plain']
            run=subprocess.run(args,capture_output=True,text=True)
            base=args.baseline/(f['id']+'.jsonl');diff=None;count=0
            if run.returncode==0:
                with base.open() as a,trace.open() as b:
                    for index,(al,bl) in enumerate(itertools.zip_longest(a,b),1):
                        if al is None or bl is None:diff={'hop':index,'field':'length'};break
                        aa=json.loads(al);bb=json.loads(bl);count+=1
                        for key,value in aa.items():
                            if bb.get(key)!=value:diff={'hop':index,'field':key,'baseline':value,'candidate':bb.get(key)};break
                        if diff:break
            row={'fixture':f['id'],'variant':label,'returncode':run.returncode,'hops_compared':count,'first_difference':diff,'parity':run.returncode==0 and diff is None,'trace_sha256':sha(trace),'pcm_sha256':sha(pcm),'baseline_sha256':sha(base),'summary':run.stdout,'stderr':run.stderr}
            results.append(row)
            (evidence/'comparison-results.json').write_text(json.dumps({'executable_sha256':sha(exe),'manifest_sha256':sha(manifest),'rows':results},indent=2)+'\n')
            print(f['id'],label,'PASS' if row['parity'] else 'FAIL',count,flush=True)
    return 0 if all(r['parity'] for r in results) else 1
if __name__=='__main__':raise SystemExit(main())
