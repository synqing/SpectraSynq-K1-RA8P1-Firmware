#!/usr/bin/env python3
"""Actual donor PCM->pair->RGB8 adapter->GRB48 path. Host proof only."""
from pathlib import Path
import subprocess,json,hashlib
from datetime import datetime,timezone
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'_scratch'/'test_composed_donor';OUT.mkdir(parents=True,exist_ok=True)
records=[]
def run(cmd):
    p=subprocess.run(cmd,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    records.append({'argv':cmd,'exit_code':p.returncode,'output':p.stdout});print(p.stdout,end='');return p
sources=['harness/tests/test_composed_donor.cpp','platform/ra8p1/palette_runtime.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DK1_EMOTISCOPE_PORT=1','-DK1_PALETTE_MORPH=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_EMOTISCOPE_TEST_SEAM=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(OUT/'donor')]
inputs=sources+['platform/ra8p1/composed_scene_runtime.h','platform/ra8p1/composed_motion.h','harness/tests/test_composed_donor.py']
before={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs}
code=run(cmd).returncode;data={};metrics=[]
if not code:
    for scene,background,change in [(s,b,0) for s in (1,2,3) for b in (0,1)]+[(1,0,1),(3,0,1),(0,0,0),(4,1,0)]:
        key=f'{scene}-{background}-{change}';path=OUT/(key+'.rgb8')
        p=run([str(OUT/'donor'),str(scene),str(background),str(change),str(path)])
        if p.returncode:code=p.returncode;break
        metrics.append(json.loads(p.stdout.strip().splitlines()[-1]));data[key]=np.fromfile(path,np.uint8).reshape(1600,2,160,3)
if not code:
    # Same process seeds/PCM. Unchanged past frames establish a paired control.
    for scene in (1,3):
        base=data[f'{scene}-0-0'];changed=data[f'{scene}-0-1']
        assert np.array_equal(base[:400],changed[:400])
        assert np.any(base[500:]!=changed[500:]),'palette edit has no actual consumer'
        assert np.count_nonzero(base[400:410])>100, 'history oracle must contain actual lit material'
        assert np.array_equal(base[400:410],changed[400:410]), 'retained material recoloured immediately'
        # Outer deposited history does not instantly recolour at a palette edit.
        assert np.array_equal(base[400:410,:,0:8],changed[400:410,:,0:8]),'old outer history recoloured'
        assert np.array_equal(base[400:410,:,-8:],changed[400:410,:,-8:]),'old outer history recoloured'
    assert np.any(data['3-0-0']!=data['0-0-0']),'new scene is just existing Bloom'
    image=Image.new('RGB',(1170,1000),(14,16,22));draw=ImageDraw.Draw(image)
    draw.text((20,15),'HOST ONLY | actual donor synthetic PCM -> pair renderer -> 160-pixel RGB8 adapter',fill='white')
    draw.text((20,35),'No exposure normalisation. Pre-hardware-gain RGB8; packer checked at brightness100 and board gains0.25/0.40.',fill=(205,205,215))
    for col,scene in enumerate((1,2,3)):
        x=45+col*385;draw.text((x,65),['Paired ribbons','Call / answer','Bloom stream'][col],fill='white')
        for board in (0,1):
            draw.text((x+board*175,85),'Board '+('A' if board==0 else 'B'),fill='white')
            tile=Image.fromarray(data[f'{scene}-0-0'][:,board]).resize((160,850),Image.Resampling.NEAREST)
            image.paste(tile,(x+board*175,108))
        for seconds in (0,4,8,12,16):draw.text((x-26,108+int(seconds/16*850)),str(seconds)+'s',fill=(190,190,200))
    image.save(OUT/'donor-rgb8-raster.png')
after={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs}
assert before==after,'compiled source changed while running donor proof'
peaks={key:[{'max_channel':int(value[:,b].max()),'p95_frame_peak':float(np.percentile(value[:,b].max(axis=(1,2)),95))} for b in range(2)] for key,value in data.items()}
receipt={'rgb8_peaks':peaks,'utc':datetime.now(timezone.utc).isoformat(),'scope':'host real donor synthetic PCM only; no optical or music-quality claim','records':records,'metrics':metrics,'sha256_inputs':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources+['platform/ra8p1/composed_scene_runtime.h','platform/ra8p1/composed_motion.h','harness/tests/test_composed_donor.py']}}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');raise SystemExit(code)
