#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Source-bound host verification."""
import argparse
import array
import json
from pathlib import Path
import subprocess
from build_ribbon_replay import ROOT, build, sha
from serve_ribbon_audition import export,ALLOWED
from verify_bloom_sources import verify
from verify_cross_strip_source import verify as verify_cross_strip

def main(output):
    output.mkdir(parents=True,exist_ok=False)
    logs=[]
    def run(command,negative=False):
        p=subprocess.run(list(map(str,command)),cwd=ROOT,capture_output=True,text=True)
        logs.append(dict(command=list(map(str,command)),exit=p.returncode,stdout=p.stdout,stderr=p.stderr,expected_failure=negative))
        (output/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if (p.returncode==0)==negative:raise RuntimeError(p.stdout+p.stderr)
    exe=build(output/'build')
    test=output/'material-test'
    run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-Isrc/k1','src/k1/core/visual/ribbon_material.cpp','src/k1/core/visual/product_palette_data.cpp','tests/host/test_ribbon_material.cpp','-o',test]);run([test])
    profile=output/'profile-test'
    run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-Isrc/k1','src/k1/core/visual/ribbon_material.cpp','src/k1/core/visual/ribbon_profile.cpp','src/k1/core/visual/product_palette_data.cpp','tests/host/test_ribbon_profile.cpp','-o',profile]);run([profile])
    paired=output/'paired-test'
    run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-Isrc/k1','src/k1/core/visual/ribbon_material.cpp','src/k1/core/visual/paired_wave_scene.cpp','src/k1/core/visual/product_palette_data.cpp','tests/host/test_paired_wave_scene.cpp','-o',paired]);run([paired])
    verify_cross_strip(output/'cross-strip-source')
    for mutation in [False,True]:
        donor=output/('donor-mutation' if mutation else 'donor-test')
        run(['/opt/homebrew/bin/g++-15','-std=c++17','-O2','-Isrc/k1','-Itests/reference',*(['-DMUTATE_HISTORY_EXPECTATION'] if mutation else []),'tests/reference/test_bloom_donors.cpp','-o',donor]);run([donor],mutation)
    if verify(output/'original-statements'):raise RuntimeError('Official-source mismatch')
    fixture_results={}
    for fixture in ALLOWED:
        receipt=export(exe,output/'playback',dict(fixture=fixture))
        folder=output/'playback'/receipt['key'];meta=json.loads((folder/'frames.json').read_text())
        data=array.array('H');data.frombytes((folder/'frames.rgb16le').read_bytes())
        native=(folder/'frames.native-grb16').read_bytes()
        assert len(data)==meta['count']*5*160*3 and len(native)==meta['count']*160*6
        nonbyte=0
        for frame in range(meta['count']):
            pixels=data[(frame*5+3)*480:(frame*5+4)*480]
            assert max(pixels)<=6168
            for i in range(160):
                r,g,b=pixels[i*3:i*3+3];j=frame*960+i*6
                assert native[j:j+6]==bytes([g>>8,g&255,r>>8,r&255,b>>8,b&255])
                assert pixels[i*3:i*3+3]==pixels[(159-i)*3:(160-i)*3]
                nonbyte+=any(c%257 for c in [r,g,b])
        pairs=array.array('H');pairs.frombytes((folder/'frames.paired.rgb16le').read_bytes())
        pair_wire=(folder/'frames.paired.native-grb16').read_bytes()
        assert len(pairs)==meta['count']*3*2*160*3 and len(pair_wire)==meta['count']*1920
        unequal=0
        for frame in range(meta['count']):
            for scene in range(3):
                base=(frame*3+scene)*960
                top=pairs[base:base+480];bottom=pairs[base+480:base+960]
                assert max(top+bottom)<=6168
                if scene==1:assert top==bottom
                if scene==2:unequal+=top!=bottom
                for edge,pixels in enumerate([top,bottom]):
                    for i in range(160):
                        r,g,b=pixels[i*3:i*3+3]
                        assert pixels[i*3:i*3+3]==pixels[(159-i)*3:(160-i)*3]
                        if scene==2:
                            j=(frame*320+edge*160+i)*6
                            assert pair_wire[j:j+6]==bytes([g>>8,g&255,r>>8,r&255,b>>8,b&255])
        if fixture=='silence':assert meta['paired_metrics'][2]['channel_sum_seconds']==0
        else:assert unequal>0 and meta['paired_metrics'][2]['channel_sum_seconds']>0
        if fixture=='silence':assert meta['metrics'][3]['channel_sum_seconds']==0
        else:assert nonbyte>0 and meta['metrics'][3]['channel_sum_seconds']>0
        if fixture=='synthetic':
            # Isolated impulse's quiet interval after full final treatment.
            window=[x for x in meta['frames'][60:1200] if x['active'] and not x['outgoing'] and not x['returning']]
            assert len(window)>10 and max(x['energy'] for x in window)==0
        fixture_results[fixture]=dict(identity=receipt['identity'],metrics=meta['metrics'],frames=meta['count'],hops=meta['hops'],low_byte_pixels=nonbyte)
        print(fixture,meta['count'],'frames verified',flush=True)
    # Deterministic replay compares every frame, including native staging.
    first=export(exe,output/'playback',dict(fixture='song-0'))
    second=export(exe,output/'repeat',dict(fixture='song-0'))
    for name in ['frames.rgb16le','frames.native-grb16','frames.json','frames.paired.rgb16le','frames.paired.native-grb16']:
        assert sha(output/'playback'/first['key']/name)==sha(output/'repeat'/second['key']/name)
    (output/'RESULT.json').write_text(json.dumps(dict(fixtures=fixture_results,deterministic=True,
        limitations=['Host only','No optical acceptance','Song-1 untouched','AP-G3 remains open'],source_sha256=sha(ROOT/'src/k1/core/visual/ribbon_material.cpp')),indent=2)+'\n')
    print('Host source, material, causal PCM, native packing and negative witness checks passed')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);main(p.parse_args().output.resolve())
