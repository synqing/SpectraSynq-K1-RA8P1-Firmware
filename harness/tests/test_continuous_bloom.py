"""Bounded actual donor PCM -> two independent Bloom histories -> final GRB48."""
from pathlib import Path
import subprocess,json,hashlib
import numpy as np
R=Path(__file__).resolve().parents[2];O=R/'_scratch/continuous-bloom-r2/host';O.mkdir(parents=True,exist_ok=True)
sources=['harness/tests/test_continuous_bloom.cpp','platform/ra8p1/palette_runtime.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-fno-sanitize-recover=all','-DK1_EMOTISCOPE_PORT=1','-DK1_PALETTE_MORPH=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_EMOTISCOPE_TEST_SEAM=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(O/'check')]
rows=[]
def run(args):
 p=subprocess.run(args,cwd=R,text=True,capture_output=True);rows.append(dict(argv=args,code=p.returncode,output=p.stdout+p.stderr));(O/'commands.json').write_text(json.dumps(rows,indent=2));print(p.stdout+p.stderr,end='');assert p.returncode==0
run(cmd)
for v in range(6):run([str(O/'check'),str(v),str(O/f'{v}.grb48')])
a,b,c=[np.fromfile(O/f'{v}.grb48',np.uint8).reshape(1200,2,960) for v in range(3)]
assert np.array_equal(a[:,1],b[:,1]),'editing body response changed peer payload'
assert np.array_equal(a[:300],b[:300]),'paired controls diverge before edit'
assert np.any(a[300:500,0]!=b[300:500,0]),'Edge Response has no consumer'
assert np.any(a[:,0]!=a[:,1]),'body/detail payloads identical'
assert not np.any(a[1000:]) and not np.any(c[1000:]),'silent/stale history failed to drain'
# Every later attack inside a five-second interval changes fresh centre material.
wide=a.view('>u2').reshape(1200,2,160,3).astype(float)
energy=wide[:,:,76:84].sum(axis=(2,3))
for at in (80,160,240,320,400):
 assert energy[at:at+30,0].max()>energy[at-15:at,0].min(),f'later attack {at} suppressed'
metrics=dict(peer_payload_identical=True,response_changed_body=True,silence_drained=True,stale_drained=True,body_detail_different=True,peak_u16=wide.max(axis=(0,2,3)).tolist(),scope='host deterministic PCM; no physical quality claim')
weak=np.fromfile(O/'3.grb48',np.uint8).view('>u2').reshape(1200,2,160,3).astype(float)
weak_energy=weak[:,:,76:84].sum(axis=(2,3))
for at in (80,160,240,320,400):
 assert weak_energy[at:at+30,0].max()>weak_energy[at-15:at,0].min(),f'weak attack {at} suppressed'
assert np.count_nonzero(weak_energy[100:500,1])>350,'quiet sustained detail lost'
assert not np.any(weak[1000:]),'weak music history failed to drain'
metrics.update(weak_attacks_survive=True,quiet_detail_sustained=True)
returns=np.fromfile(O/'5.grb48',np.uint8).reshape(1200,2,960)
assert np.array_equal(a[:,1],returns[:,1]),'body return edit changed detail payload'
assert np.any(a[:,0]!=returns[:,0]),'body return edit has no consumer'
metrics.update(body_return_peer_payload_identical=True)
(O/'result.json').write_text(json.dumps(metrics,indent=2));print(json.dumps(metrics))
