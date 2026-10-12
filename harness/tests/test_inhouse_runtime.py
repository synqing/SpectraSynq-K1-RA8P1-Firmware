#!/usr/bin/env python3
"""Native160 mixed-family dispatch/peer independence regression; host only."""
from pathlib import Path
import json,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'_scratch'/'test_inhouse_runtime';OUT.mkdir(parents=True,exist_ok=True)
sources=['harness/tests/test_inhouse_runtime.cpp','platform/ra8p1/palette_runtime.cpp','platform/ra8p1/k1_live_protocol.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DK1_EMOTISCOPE_PORT=1','-DK1_PALETTE_MORPH=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_EMOTISCOPE_TEST_SEAM=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(OUT/'runtime')]
subprocess.run(cmd,cwd=ROOT,check=True)
results=[]
for board in range(2):
    pair=[]
    for changed in range(2):
        result=json.loads(subprocess.check_output([str(OUT/'runtime'),str(board),str(changed)],cwd=ROOT,text=True));pair.append(result);results.append(result)
    assert pair[0]['crc'][1-board]==pair[1]['crc'][1-board], 'Hybrid control mutated donor peer payload'
    assert pair[0]['crc'][board]!=pair[1]['crc'][board], 'Hybrid brightness has no output consumer'
(OUT/'receipt.json').write_text(json.dumps({'scope':'host dispatch and payload only; physical qualification required','results':results},indent=2)+'\n')
print(json.dumps(results,indent=2))
