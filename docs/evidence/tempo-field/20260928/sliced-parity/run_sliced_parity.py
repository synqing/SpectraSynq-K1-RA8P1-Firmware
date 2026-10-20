#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Host-only sliced parity, no device I/O.
Retain the frozen legacy baseline and compare every recorded legacy field at every
hop, using exact JSON numeric equality (float round-trip precision in trace).
"""
import hashlib, json, shutil, subprocess, sys, time
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
CONTROL=ROOT.parent/'titan-live-control-001'
EVIDENCE=HERE.parent
RUN=HERE/(sys.argv[1] if len(sys.argv)>1 else ".")
RUN.mkdir(parents=True,exist_ok=True)
FROZEN=json.loads((EVIDENCE/'baseline-source.json').read_text())
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def write(path,obj): Path(path).write_text(json.dumps(obj,indent=2)+'\n')

def build(label):
    folder=RUN/label
    if folder.exists(): raise RuntimeError('Refuse overwriting retained build '+str(folder))
    folder.mkdir()
    source=ROOT if label=='candidate' else CONTROL
    portable=ROOT/'src/k1' if label=='candidate' else EVIDENCE/'baseline-build/k1'
    shutil.copytree(portable,folder/'src/k1')
    for rel in ['platform/ra8p1','tests/host','tests/target']:
        shutil.copytree(source/rel,folder/rel)
    if label=='baseline':
        trace=(ROOT/'tests/host/replay_trace.h').read_text()
        trace=trace[:trace.index('  std::fprintf(file, ",\\\"epoch')]
        trace=trace.replace('#include "core/visual/tempo_reference_scene.h"\n','')
        trace=trace.replace(', bool scene = false','')
        trace+='  std::fprintf(file,"}\\n");\n}\n'
        (folder/'tests/host/replay_trace.h').write_text(trace)
        main=(ROOT/'tests/host/replay_live_k1.cpp').read_text()
        main='\n'.join(line for line in main.split('\n') if 'runtime.setTempoEvidence' not in line)
        main=main.replace(', argc > 6 && std::strcmp(argv[6], "scene") == 0','')
        (folder/'tests/host/replay_live_k1.cpp').write_text(main)
    provenance={}
    for rel in ['platform/ra8p1/k1_live_runtime.cpp','platform/ra8p1/k1_live_runtime.h',
                'platform/ra8p1/k1_live_clock.c','platform/ra8p1/k1_live_protocol.cpp',
                'tests/host/replay_live_k1.cpp','tests/host/replay_trace.h']:
        actual=sha(folder/rel)
        if label=='baseline' and rel!='tests/host/replay_trace.h' and FROZEN.get(rel)!=actual:
            raise RuntimeError('Baseline control-tree mismatch '+rel)
        provenance[rel]=actual
    before={str(p.relative_to(folder)):sha(p) for p in sorted((folder/'src/k1').rglob('*')) if p.is_file()}
    if label=='baseline':
        for rel,digest in before.items():
            if FROZEN.get(rel)!=digest: raise RuntimeError('Frozen portable mismatch '+rel)
    overlay=source/'scripts/tempo_acf_slice_overlay.py'
    shutil.copy2(overlay,folder/'tempo_acf_slice_overlay.py')
    namespace={};exec(compile(overlay.read_text(),str(overlay),'exec'),namespace)
    if label=='baseline': shutil.copytree(folder/'src/k1',folder/'unsliced-k1')
    overlay_result=namespace['apply_tempo_acf_slice_overlay'](folder/'src/k1')
    names=json.loads((source/'docs/import-slices.json').read_text())['product']
    if label=='candidate': names+=['core/audio/tempo_field_publisher.cpp','core/audio/tempo_phase_tracks.cpp','core/visual/tempo_reference_scene.cpp']
    sources=[folder/'platform/ra8p1/k1_live_clock.c',folder/'platform/ra8p1/k1_live_runtime.cpp',
       folder/'platform/ra8p1/k1_live_protocol.cpp',folder/'src/k1/core/audio/k1_audio_hop.c']
    sources += [folder/'src/k1'/n for n in names if n.endswith('.cpp')]
    sources += [folder/'src/k1/core/audio/tempo_acf_slice.cpp',folder/'tests/host/replay_live_k1.cpp']
    exe=folder/'replay_live_k1'
    command=['c++','-std=c++17','-O2','-ffp-contract=off',
      '-I'+str(folder/'src/k1'),'-I'+str(folder/'src'),
      '-I'+str(folder/'platform/ra8p1'),'-I'+str(folder/'tests/target'),
      *map(str,sources),'-o',str(exe)]
    if label=='baseline':
        unsliced_command=[x.replace(str(folder/'src/k1'),str(folder/'unsliced-k1')) for x in command]
        unsliced_command[-1]=str(folder/'replay_unsliced')
        result=subprocess.run(unsliced_command,capture_output=True,text=True)
        (folder/'unsliced-build.log').write_text(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError('Unsliced reconstructed baseline compile failed')
    started=time.monotonic()
    run=subprocess.run(command,capture_output=True,text=True)
    (folder/'build.log').write_text(run.stdout+run.stderr)
    receipt={'command':command,'exit':run.returncode,'seconds':time.monotonic()-started,
      'control_source':str(source),'compiler':subprocess.check_output(['c++','--version'],text=True),
      'overlay_source_sha256':sha(overlay),'overlay_effective':overlay_result,
      'before_portable':before,'runtime_and_trace':provenance,
      'effective_portable':{str(p.relative_to(folder)):sha(p) for p in sorted((folder/'src/k1').rglob('*')) if p.is_file()}}
    if run.returncode==0:receipt['executable_sha256']=sha(exe)
    write(folder/'build.json',receipt)
    if run.returncode:raise RuntimeError('Compile failed: '+str(folder/'build.log'))
    return exe

def difference(a,b,path=''):
    if isinstance(a,dict):
        if not isinstance(b,dict):return [path,a,b]
        for k,v in a.items():
            if k not in b:return [path+'/'+k,v,'MISSING']
            out=difference(v,b[k],path+'/'+k)
            if out:return out
    elif a!=b:return [path,a,b]
    return None

def main():
    baseline=build('baseline');candidate=build('candidate')
    manifest=ROOT/'tests/fixtures/tempo_field/v1/pcm/manifest.json'
    fixtures=json.loads(manifest.read_text())['fixtures']
    results=[]
    reconstruction=[]
    for fixture in fixtures:
      trace=RUN/'baseline'/('unsliced-'+fixture['id']+'.jsonl')
      cmd=[str(RUN/'baseline/replay_unsliced'),str(manifest.parent/fixture['path']),'normal',str(trace)]
      run=subprocess.run(cmd,capture_output=True,text=True)
      retained=EVIDENCE/'baseline'/(fixture['id']+'.jsonl')
      actual=[json.loads(x) for x in trace.read_text().splitlines()]
      expected=[json.loads(x) for x in retained.read_text().splitlines()]
      same=run.returncode==0 and actual==expected
      row={'fixture':fixture['id'],'exact_serialised_fields_equal':same,'hops':len(actual),
           'reconstructed_trace_sha256':sha(trace),'retained_trace_sha256':sha(retained)}
      reconstruction.append(row)
      write(RUN/'baseline-reconstruction.json',reconstruction)
      if not same:raise RuntimeError('Reconstructed baseline does not match '+fixture['id'])
    print('RECONSTRUCTED_BASELINE_ALL_10_EXACT',flush=True)
    for fixture in fixtures:
      pcm=manifest.parent/fixture['path']
      if sha(pcm)!=fixture['sha256']:raise RuntimeError('Fixture drift '+fixture['id'])
      for variant,args in [('field-on-phase-on',['on','on']),('field-off',['off','on']),('phase-off',['on','off'])]:
        case=RUN/(fixture['id']+'-'+variant);case.mkdir()
        btrace=case/'baseline.jsonl';ctrace=case/'candidate.jsonl'
        runs=[]
        for exe,trace in [(baseline,btrace),(candidate,ctrace)]:
          start=time.monotonic()
          cmd=[str(exe),str(pcm),'normal',str(trace),*args,'plain']
          run=subprocess.run(cmd,capture_output=True,text=True)
          runs.append({'command':cmd,'exit':run.returncode,'seconds':time.monotonic()-start,
                       'stdout':run.stdout,'stderr':run.stderr,'trace_sha256':sha(trace) if trace.exists() else None})
        first=None;count=0
        if all(r['exit']==0 for r in runs):
          import itertools
          for hop,(a,b) in enumerate(itertools.zip_longest(btrace.open(),ctrace.open()),1):
            if a is None or b is None:first={'hop':hop,'difference':'row count'};break
            d=difference(json.loads(a),json.loads(b));count+=1
            if d:first={'hop':hop,'difference':d};break
        else:first={'process_failure':True}
        row={'fixture':fixture['id'],'pcm_sha256':sha(pcm),'variant':variant,'compared_hops':count,
             'exact_legacy_parity':first is None,'first_mismatch':first,'runs':runs}
        write(case/'result.json',row);results.append(row)
        print(fixture['id'],variant,'MATCH' if first is None else first,flush=True)
    receipt={'schema':'k1.tempo.sliced.complete-pipeline-parity.v1','script_sha256':sha(__file__),
      'manifest_sha256':sha(manifest),'baseline_tracer_reconstruction':reconstruction,'results':results,
      'all_exact':all(r['exact_legacy_parity'] for r in results),
      'claim_scope':'HOST complete LiveAudioRuntime::consume with sliced generated portable tempo source. No target timing or optical claim.'}
    write(RUN/'RESULTS.json',receipt)
    return 0 if receipt['all_exact'] else 1
if __name__=='__main__':raise SystemExit(main())
