#!/usr/bin/env python3
"""Compile real motion header, retain assertions and host-only RGB frame evidence."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '_scratch' / 'test_composed_motion'
OUT.mkdir(parents=True, exist_ok=True)
flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wdouble-promotion',
         '-fno-exceptions', '-fno-rtti', '-fsanitize=address,undefined',
         '-fno-sanitize-recover=all']
command = ['clang++', *flags, '-I.', 'harness/tests/test_composed_motion.cpp', '-o', str(OUT/'test_composed_motion')]
records = []
def run(cmd, expected=0):
    p = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    records.append({'argv':cmd, 'exit_code':p.returncode, 'output':p.stdout})
    print(p.stdout, end='')
    assert (p.returncode == 0) == (expected == 0), p.stdout
run(command)
run([str(OUT/'test_composed_motion'), str(OUT/'frames.rgbf')])
# Fault injection shortens outgoing tail protection. The same assertions must fail.
mutant = OUT/'mutant'
(mutant/'platform/ra8p1').mkdir(parents=True, exist_ok=True)
header = ROOT/'platform/ra8p1/composed_motion.h'
source = header.read_text()
needle = 'std::ceil(latched_.out_width*0.5f*static_cast<float>(latched_.out_travel_ms)))+latched_.trail_ms'
assert source.count(needle) == 1
(mutant/'platform/ra8p1/composed_motion.h').write_text(source.replace(needle, 'std::ceil(0.0f))+0u'))
run(['clang++',*flags,'-I'+str(mutant),'-I.','harness/tests/test_composed_motion.cpp','-o',str(OUT/'test_composed_motion_mutant')])
run([str(OUT/'test_composed_motion_mutant')], expected=1)
# A time/position raster uses the actual float RGB output without normalisation.
import numpy as np
from PIL import Image, ImageDraw
frames=np.fromfile(OUT/'frames.rgbf',dtype=np.float32).reshape(-1,2,128,3)
canvas=Image.new('RGB',(1140,940),(15,17,24));draw=ImageDraw.Draw(canvas)
draw.text((24,15),'HOST ONLY - real C++ RGB frames / synthetic level input / no optical claim',fill='white')
draw.text((24,35),'Position: left edge -> centre pair <- right edge. Time runs down. Linear RGB x255; no gain normalisation.',fill=(190,197,211))
for j,title in enumerate(['Centre Ribbon: two calls / clear-edge quiet / captured weaker return','Bloom Return: continuous RGB injection stops at 3 s; history returns']):
    x=60+j*550
    draw.text((x,63),title,fill='white')
    image=Image.fromarray(np.uint8(np.clip(frames[:,j]*255,0,255))).resize((512,851),Image.Resampling.NEAREST)
    canvas.paste(image,(x,85))
    for t in range(9):
        y=85+t*100
        if y<936:draw.text((x-33,y),str(t)+' s',fill=(180,186,200))
    draw.line((x+256,85,x+256,936),fill=(45,48,60),width=1)
canvas.save(OUT/'host-frames.png')
receipt={'utc':datetime.now(timezone.utc).isoformat(), 'target':'host only',
 'records':records,'sha256_inputs':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [header,ROOT/'harness/tests/test_composed_motion.cpp']},
 'sha256_outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [OUT/'frames.rgbf',OUT/'host-frames.png']}}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(OUT/'host-frames.png')
