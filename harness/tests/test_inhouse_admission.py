#!/usr/bin/env python3
"""Production PCM/Hybrid admission, final bytes and independent edges; host only."""
from pathlib import Path
import argparse,csv,hashlib,json,re,subprocess
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
O=a.output.resolve();O.mkdir(parents=True,exist_ok=True)
sources=['harness/tests/test_inhouse_admission.cpp','platform/ra8p1/palette_runtime.cpp',
 'platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp',
 'platform/ra8p1/inhouse_audio.cpp','src/k1/core/audio/gdft_goertzel.cpp',
 'src/k1/core/audio/tempo_tracker.cpp','src/k1/core/audio/tempo_acf.cpp',
 'src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp',
 'src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
inputs=sources+['platform/ra8p1/palette_runtime.h','platform/ra8p1/composed_scene_runtime.h',
 'platform/ra8p1/inhouse_audio.h','src/k1/core/visual/inhouse_waveform.h',
 'platform/ra8p1/fixture_app.cpp','tests/target/trajectory.h',str(Path(__file__).relative_to(R))]
hashes={x:hashlib.sha256((R/x).read_bytes()).hexdigest() for x in inputs}
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined',
 '-fno-sanitize-recover=all','-ffp-contract=off','-DK1_EMOTISCOPE_PORT=1','-DK1_PALETTE_MORPH=1',
 '-DK1_TITAN_TOP_OUTPUT=1','-DK1_EMOTISCOPE_TEST_SEAM=1','-Isrc','-Isrc/k1','-Iplatform/ra8p1',
 '-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(O/'check')]
commands=[]
def run(argv):
 r=subprocess.run(argv,cwd=R,text=True,capture_output=True)
 commands.append(dict(argv=argv,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
 (O/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 assert r.returncode==0,r.stdout+r.stderr
 return r.stdout
run(cmd)
rows=[];frames=[]
for variant in range(2):
 status=json.loads(run([str(O/'check'),str(variant),str(O/f'{variant}.csv'),str(O/f'{variant}.grb48')]))
 assert all(x['source_silence']==1 and x['admission_open']==1 for x in status['inhouse_frames'])
 assert status['inhouse_donor_sequence']==701 and status['inhouse_donor_peak_squared']>.0048
 rows.append(list(csv.DictReader((O/f'{variant}.csv').open())))
 frames.append(np.fromfile(O/f'{variant}.grb48',dtype='>u2').reshape(2400,2,160,3))
assert np.array_equal(frames[0][:,1],frames[1][:,1]),'Peer output changed under invalid/mode/epoch edits'
assert np.any(frames[0][1452:,0]!=frames[1][1452:,0]),'Boundary edits did not reach output'
base=rows[0]
for board in range(2):
 births=np.array([int(x[f'injection{board}']) for x in base])
 assert births[650]>births[649],'.079 weak attack did not create material immediately'
 assert births[849]>births[750],'Hysteresis sustain did not create material'
 assert np.all(births[850:1450]==births[849]),'Room created new material'
 assert not np.any(frames[0][1200:1450,board]),'History failed to drain'
 assert births[1999]>births[1552],'Brief low-RMS musical gaps lost source dwell'
 assert np.all(births[2120:]==births[2119]),'Confirmed quiet with strong peak spikes created material'
 assert not np.any(frames[0][2380:,board]),'Confirmed quiet spike history failed to drain'
changed=rows[1]
for start,stop in [(1452,1453),(1455,1456),(1461,1462),(1464,1465),(1467,1468)]:
 assert int(changed[stop]['injection0'])==int(changed[start-1]['injection0']),'Rejected input created material'

# Exercise the exact four production snprintf formats, including GPT pair and
# fixture control/top-output append, at conservative maxima. Preprocess the
# actual format strings under target feature defines; no hand-copied JSON.
palette=(R/'platform/ra8p1/palette_runtime.cpp').read_text().split('std::size_t PaletteRuntime::statusJson(',1)[1]
fixture=(R/'platform/ra8p1/fixture_app.cpp').read_text().split('std::size_t control_palette_status(',1)[1].split('void donor_queue_config',1)[0]
formats=[]
for part in [palette,fixture]:
 for call in part.split('std::snprintf(')[1:]:
  start=call.index('"');end=re.search(r'"(?:\\.|[^"\\])*}"\s*,',call[start:])
  assert end,'Production status format boundary moved'
  formats.append(call[start:start+end.end()].rstrip().removesuffix(','))
assert len(formats)==4
defs=['K1_EMOTISCOPE_PORT','K1_TITAN_TOP_OUTPUT','K1_PALETTE_MORPH','K1_LIVE_RUNTIME','K1_PALETTE_WS2816_GPT_PAIR']
fmtfile=O/'status-formats.cpp';fmtfile.write_text('\n'.join(f'const char* f{i}=\n{x}\n;' for i,x in enumerate(formats)))
pre=run(['clang++','-E','-P','-x','c++',*[f'-D{x}=1' for x in defs],str(fmtfile)])
capacity=int(re.search(r'char data\[(\d+)\]',(R/'tests/target/trajectory.h').read_text())[1])
calls=[]
for i,block in enumerate(pre.split(';')[:-1]):
 fmt=''.join(json.loads(x) for x in re.findall(r'"(?:\\.|[^"\\])*"',block))
 args=[]
 for spec in re.findall(r'%(?:\.\d+)?(?:llu|lu|u|s|f)',fmt):
  if spec=='%llu':args.append('UINT64_MAX')
  elif spec=='%lu':args.append('static_cast<unsigned long>(UINT32_MAX)')
  elif spec=='%u':args.append('UINT32_MAX')
  elif spec=='%s':args.append('"'+'X'*96+'"') # Above every finite production name/boolean/backend string.
  else:args.append('static_cast<double>(FLT_MAX)')
 calls.append(f'int n{i}=std::snprintf(out+used,sizeof(out)-used,f{i},{",".join(args)}); assert(n{i}>0 && std::size_t(n{i})<sizeof(out)-used); used+=n{i}-1;')
capacity_src=O/'status-capacity.cpp'
capacity_src.write_text('#include <cstdio>\n#include <cstdint>\n#include <cassert>\n#include <cfloat>\n'+pre+
 f'\nint main(){{char out[{capacity}]{{}};std::size_t used=0;'+''.join(calls)+'std::printf("%zu\\n",used+1);}\n')
run(['clang++','-std=c++17',str(capacity_src),'-o',str(O/'status-capacity')])
worst_bytes=int(run([str(O/'status-capacity')]))
assert worst_bytes<capacity

fig,ax=plt.subplots(3,1,figsize=(11,7),sharex=True)
t=(np.arange(2400)+1)*.01
ax[0].plot(t,[float(x['raw_rms']) for x in base],label='Original source RMS')
ax[0].axhline(.04,color='grey',ls=':',label='Existing quiet entry (after source dwell)')
ax[0].axhline(.08,color='grey',ls='--',label='Unchanged source wake threshold')
ax[0].legend(loc='upper right');ax[0].set_ylabel('RMS / full scale')
ax[1].plot(t,[int(x['admission0']) for x in base],label='Titan admission')
ax[1].plot(t,[int(x['source_silence']) for x in base],ls='--',label='Original source silence')
ax[1].legend(loc='upper right');ax[1].set_ylabel('Boolean')
rgb=frames[0][:,0,:,[1,0,2]].transpose(1,2,0).astype(float);rgb=(rgb/max(1,rgb.max()))**.45
ax[2].imshow(rgb.transpose(1,0,2),aspect='auto',origin='lower',extent=[.01,24,0,160])
ax[2].set_ylabel('Actual edge pixel');ax[2].set_xlabel('Seconds')
fig.suptitle('Host PCM → Hybrid: spiky room, .079 RMS wake, sustain, source quiet dwell\nActual final RGB; display gain/gamma for visibility, no physical quality claim')
fig.tight_layout();fig.savefig(O/'actual-output.png',dpi=150);plt.close(fig)
assert hashes=={x:hashlib.sha256((R/x).read_bytes()).hexdigest() for x in inputs},'Inputs changed during test'
receipt=dict(sources=hashes,commands=commands,frames_per_run=2400,peer_bytes_identical=True,
 weak_rms_079_birth_next_frame=True,original_source_silence_preserved=True,
 room_no_new_material=True,history_drained=True,invalid_stale_regressive_epoch_mode_checks=True,
 donor_stale_gap_rewarm_checked=True,production_status_capacity=capacity,status_conservative_max_bytes=worst_bytes,
 confirmed_quiet_peak_spikes_blocked=True,existing_source_dwell_preserved=True,
 limitation='After source-confirmed quiet, sub-.04 RMS music remains blocked by the inherited source quiet classification. Existing Bloom peak admission also applies. No universal noise/music discrimination or physical qualification claim.')
(O/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:v for k,v in receipt.items() if k not in ['sources','commands']},indent=2))
