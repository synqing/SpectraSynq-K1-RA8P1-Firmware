#!/usr/bin/env python3
"""Production packer check for the observed top DIN half assignment; host only."""
import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'_scratch/waveform-migration-001/top-direction-001/mapping-host';O.mkdir(parents=True,exist_ok=True)
sources=['harness/tests/test_top_output_mapping.cpp','platform/ra8p1/palette_runtime.cpp','platform/ra8p1/emotiscope_port.cpp','platform/ra8p1/emotiscope_secondary.cpp','platform/ra8p1/emotiscope_ml.cpp','src/k1/core/visual/channel_render_state.cpp','src/k1/core/visual/audio_focus.cpp','src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp','src/k1/core/visual/inhouse_waveform.cpp']
cmd=['clang++','-std=c++17','-O1','-g','-Wno-narrowing','-fsanitize=address,undefined','-DK1_EMOTISCOPE_PORT=1','-DK1_TITAN_TOP_OUTPUT=1','-DK1_PALETTE_MORPH=1','-Isrc/k1','-Iplatform/ra8p1','-Iplatform/ra8p1/emotiscope_donor','-Iplatform/ra8p1/emotiscope_shims',*sources,'-o',str(O/'mapping')]
subprocess.run(cmd,cwd=R,check=True);subprocess.run([str(O/'mapping')],cwd=R,check=True)
(O/'receipt.json').write_text(json.dumps({'scope':'actual production packer, not independent photons','command':cmd,'sources':{p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in sources}},indent=2)+'\n')
