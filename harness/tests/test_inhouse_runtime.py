#!/usr/bin/env python3
"""Native160 mixed-family dispatch/peer independence regression; host only."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--peak-overload-only',action='store_true',help='Run only the source peak>1 integration regression; retain the full matrix separately.')
parser.add_argument('--output',type=Path)
args=parser.parse_args()
OUT=(args.output or ROOT/'_scratch'/'test_inhouse_runtime').resolve();OUT.mkdir(parents=True,exist_ok=True)
sources=['harness/tests/test_inhouse_runtime.cpp','platform/ra8p1/palette_runtime.cpp','platform/ra8p1/k1_live_protocol.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DK1_EMOTISCOPE_PORT=1','-DK1_PALETTE_MORPH=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_EMOTISCOPE_TEST_SEAM=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(OUT/'runtime')]
subprocess.run(cmd,cwd=ROOT,check=True)
results=[]
if args.peak_overload_only:
    for mode in (7,8,11):
        for board in range(2):
            modes=[6,6];modes[board]=mode
            pair=[json.loads(subprocess.check_output([str(OUT/'runtime'),str(board),str(clipped),*map(str,modes),'--peak-overload'],cwd=ROOT,text=True)) for clipped in (0,1)]
            high,clipped=[row['overload_status']['inhouse_frames'][board] for row in pair]
            assert high['fresh'] and high['admission_open'], (mode,board,high)
            assert high['peak']>1.5 and clipped['peak']<=1.0, ('source peak was clipped before follower',mode,board,high,clipped)
            assert pair[0]['crc'][1-board]==pair[1]['crc'][1-board], ('overload mutated peer',mode,board)
            if mode==8:
                assert high['position']>clipped['position']+10, ('mode8 lost sensitivity displacement',high,clipped)
            if mode in (8,11):
                assert pair[0]['crc'][board]!=pair[1]['crc'][board], ('overload follower lost output consequence',mode,board)
            else:
                # Fast updates the follower for observation; raw amplitude,
                # already saturated at both1 and2, owns dot placement.
                assert high['position']==clipped['position']==159
                assert pair[0]['crc'][board]==pair[1]['crc'][board], 'Fast wrongly positions from its follower'
            results.append({'mode':mode,'board':board,'unclamped':pair[0],'clipped_input_control':pair[1]})
    # Compile the actual runtime with precisely the old erroneous clamp planted.
    runtime_source=ROOT/'platform/ra8p1/palette_runtime.cpp'
    text=runtime_source.read_text()
    old='input.peak_scaled=std::clamp(input.peak_scaled*response,0.0f,peak_limit);'
    assert text.count(old)==1, 'production clamp seam changed; inspect before mutating'
    mutant_source=OUT/'palette_runtime_clamped.cpp'
    mutant_source.write_text(text.replace(old,'input.peak_scaled=std::clamp(input.peak_scaled*response,0.0f,1.0f);'))
    mutant_cmd=[str(mutant_source) if arg=='platform/ra8p1/palette_runtime.cpp' else arg for arg in cmd]
    mutant_cmd[-1]=str(OUT/'runtime-clamped')
    subprocess.run(mutant_cmd,cwd=ROOT,check=True)
    rejected=[]
    for mode in (7,8,11):
        result=json.loads(subprocess.check_output([str(OUT/'runtime-clamped'),'0','0',str(mode),'6','--peak-overload'],cwd=ROOT,text=True))
        observed=result['overload_status']['inhouse_frames'][0]['peak']
        try:
            assert observed>1.5, 'source peak was clipped before follower'
        except AssertionError:
            rejected.append({'mode':mode,'observed_peak':observed,'failure':'source peak was clipped before follower'})
        else:
            raise AssertionError(f'mode {mode} regression failed to reject planted production clamp')
    (OUT/'peak-overload-receipt.json').write_text(json.dumps({'scope':'actual production runtime status/final GRB48; source follower range and peer independence only','results':results,'planted_clamp_rejected':rejected,'commands':[cmd,mutant_cmd],'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}},indent=2)+'\n')
    print(json.dumps([{'mode':r['mode'],'board':r['board'],'peak':r['unclamped']['overload_status']['inhouse_frames'][r['board']]['peak']} for r in results]))
    raise SystemExit(0)
for board in range(2):
    pair=[]
    for changed in range(2):
        result=json.loads(subprocess.check_output([str(OUT/'runtime'),str(board),str(changed)],cwd=ROOT,text=True));pair.append(result);results.append(result)
    assert pair[0]['crc'][1-board]==pair[1]['crc'][1-board], 'Hybrid control mutated donor peer payload'
    assert pair[0]['crc'][board]!=pair[1]['crc'][board], 'Hybrid brightness has no output consumer'
modes=list(range(7))+[32,18,8,7,11]
for first in modes:
    for second in modes:
        pair_modes=[str(first),str(second)]
        baseline=json.loads(subprocess.check_output([str(OUT/'runtime'),'0','0',*pair_modes],cwd=ROOT,text=True))
        for board in range(2):
            changed=json.loads(subprocess.check_output([str(OUT/'runtime'),str(board),'1',*pair_modes],cwd=ROOT,text=True))
            assert baseline['crc'][1-board]==changed['crc'][1-board], f'Peer changed: {pair_modes} board {board}'
            if (first,second)[board] in (32,18,8,7,11):
                assert baseline['crc'][board]!=changed['crc'][board], f'Brightness inert: {pair_modes} board {board}'
        results.append({'modes':[first,second], 'baseline':baseline,'peer_isolation':True})
# Preserve the exact compiled sources, including the quiet-admission contract.
inputs=sources+['src/k1/core/visual/inhouse_waveform.h','platform/ra8p1/palette_runtime.h',str(Path(__file__).relative_to(ROOT))]
(OUT/'receipt.json').write_text(json.dumps({'scope':'production C++ runtime dispatch and payload only; not full host transport or physical qualification','commands':[cmd],'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in inputs},'matrix_composed_scene':0,'results':results},indent=2)+'\n')
print(json.dumps(results,indent=2))
