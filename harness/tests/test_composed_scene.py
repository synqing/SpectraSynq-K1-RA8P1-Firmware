#!/usr/bin/env python3
"""Real pair coordinator regression and host-only actual RGB visual evidence."""
from pathlib import Path
import subprocess
import hashlib
import json
from datetime import datetime, timezone
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'_scratch'/'test_composed_scene';OUT.mkdir(parents=True,exist_ok=True)
records=[]
def run(cmd):
    p=subprocess.run(cmd,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    records.append({'argv':cmd,'exit_code':p.returncode,'output':p.stdout});print(p.stdout,end='');return p.returncode
cmd=['clang++','-std=c++17','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I.','harness/tests/test_composed_scene.cpp','platform/ra8p1/emotiscope_ml.cpp','-o',str(OUT/'test_composed_scene')]
assert run(cmd)==0
assert run([str(OUT/'test_composed_scene'),str(OUT/'frames.rgbf')])==0
frames=np.fromfile(OUT/'frames.rgbf',dtype=np.float32).reshape(-1,2,128,3)
im=Image.new('RGB',(1140,800),(15,17,24));draw=ImageDraw.Draw(im)
draw.text((24,15),'HOST ONLY - actual pair coordinator RGB / synthetic publications / no optical claim',fill='white')
draw.text((24,35),'Position: left edge -> centre pair <- right edge; time down. Linear RGB x255; no gain normalisation.',fill=(190,197,211))
for j,title in enumerate(['Board A - caller and retained inward reply','Board B - delayed answer with transferred palette identity']):
    x=60+j*550;draw.text((x,63),title,fill='white')
    strip=Image.fromarray(np.uint8(np.clip(frames[:,j]*255,0,255))).resize((512,701),Image.Resampling.NEAREST)
    im.paste(strip,(x,85))
    for t in range(15):draw.text((x-38,85+t*50),str(t)+' s',fill=(180,186,200))
    draw.line((x+256,85,x+256,786),fill=(45,48,60))
im.save(OUT/'pair-host-frames.png')
results=[run([str(OUT/'test_composed_scene'),case]) for case in ['reconfigure','phrase_reconfigure','linked_bloom','mismatched_quiet']]
inputs=['platform/ra8p1/composed_motion.h','platform/ra8p1/composed_scene_runtime.h','harness/tests/test_composed_scene.cpp']
receipt={'utc':datetime.now(timezone.utc).isoformat(),'target':'host only','records':records,'sha256_inputs':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs},'image_sha256':hashlib.sha256((OUT/'pair-host-frames.png').read_bytes()).hexdigest()}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(OUT/'pair-host-frames.png')
raise SystemExit(0 if all(result==0 for result in results) else 1)
