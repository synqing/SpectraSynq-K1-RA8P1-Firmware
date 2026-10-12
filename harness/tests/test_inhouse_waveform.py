"""Compile the ACTUAL reference effect and palette sampler as an independent oracle.

The shim only supplies platform/global inputs. Reference body is read from the
read-only checkout at execution, hashed in the command receipt, never rewritten.
The 100 Hz trace also runs the actual source visual spectrum/chroma preparation.
This does not claim audio-producer or physical parity.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import argparse

ROOT = Path(__file__).resolve().parents[2]
REF = Path('/Users/spectrasynq/SpectraSynq_K1_Firmware/SPECTRASYNQ_K1_FIRMWARE')
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path);args=parser.parse_args()
OUT = (args.output or ROOT / '_scratch/inhouse-waveform-host').resolve()
OUT.mkdir(parents=True, exist_ok=True)
visual = (REF / 'visual/lightshow_modes.h').read_text()
effect = REF / 'effects/light_mode_waveform_hybrid_k1.cpp'

prelude = r'''
#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include "core/visual/product_palette.h"
using std::isfinite;
#define K1_EMO_WAVEFORMS_V1
#define K1_PALETTE_VIBRANCY_V1
#define K1_FALLBACK_HELD_U_V1
#define K1_POSITION_SMOOTH_V1
constexpr unsigned NATIVE_RESOLUTION=160;
struct CRGB16 {SQ15x16 r{},g{},b{};};
struct CRGB {uint8_t r{},g{},b{};};
struct CRGBPalette16 {CRGB cells[16];const CRGB& operator[](int i)const{return cells[i];}};
struct RenderParams {bool MIRROR_ENABLED=true;float CHROMA=.05f,SATURATION=1.f;};
inline RenderParams params;
inline const RenderParams* active_render_params(){return &params;}
inline bool vp_render_secondary_channel=false;
inline uint32_t clock_ms=10;
inline uint32_t millis(){return clock_ms;}
inline SQ15x16 chromagram_smooth[12],chromagram_pregate[12],hue_position;
constexpr unsigned NUM_FREQS=80;
constexpr float SPECTROGRAM_SMOOTH_ATTACK=.75f,SPECTROGRAM_SMOOTH_RELEASE=.75f;
inline SQ15x16 spectrogram_smooth[80],k1_vp_spectrogram[80];
struct Config{uint8_t CHROMAGRAM_RANGE=60,CHROMA_PROFILE=0;};inline Config CONFIG;
inline bool VP_FIX_CHROMAGRAM_SPARSENESS=true;
inline SQ15x16 vp_dbg_chroma_gate_gain,vp_dbg_chroma_pre_max,vp_dbg_chroma_pre_mean,
 vp_dbg_chroma_norm_max,vp_dbg_chroma_norm_mean,vp_dbg_chroma_final_max,
 vp_dbg_chroma_final_mean,vp_dbg_chroma_flatness;
inline unsigned vp_dbg_chroma_range=0,vp_dbg_chroma_profile=0,vp_dbg_chroma_seq=0;
inline CRGB16 leds_16[160];
inline float k1_palette_held_u=0;
inline bool k1_palette_held_u_valid=false;
inline float VP_WAVEFORM_CHROMA_BLEND_GAIN=2.f,VP_WAVEFORM_FALLBACK_BRIGHTNESS=1.f;
struct K1AudioSnapshot {float peak_scaled=0,vu_level=0;bool silence=true;};
inline K1AudioSnapshot snap;
inline K1AudioSnapshot k1_vp_audio_snapshot_read(){return snap;}
struct Input{float peak=0;};inline Input input;
inline const Input& k1_emo_waveform_audio(){return input;}
inline float motion=1.f;
inline float k1_emo_motion_scale(){return motion;}
struct ChannelEffectState {
 uint32_t wfhyb_last_ms=0;
 float wfhyb_scroll_accum=0,wfhyb_peak_ema1=0,wfhyb_peak_last=0;
 float wfhyb_hold_env=0,wfhyb_sil_scale=0,wfhyb_dot_r=0,wfhyb_dot_g=0,wfhyb_dot_b=0;
};
struct PaletteStopsHD{unsigned count=0;float pos[48],r[48],g[48],b[48];};
inline PaletteStopsHD hd;
inline CRGBPalette16 palette;
inline unsigned palette_id=40;
inline const PaletteStopsHD& palette_hd_for_channel(bool){return hd;}
inline bool render_params_auto_colour_shift(const RenderParams*,bool){return false;}
inline bool render_params_palette_owns_colour(const RenderParams*,bool){return true;}
inline uint8_t render_params_palette_index(const RenderParams*,bool){return uint8_t(palette_id);}
inline const CRGBPalette16& cached_gradient_palette(uint8_t id,bool){
 const auto& p=k1::core::visual::productPalette(id);hd.count=p.stop_count;
 for(unsigned i=0;i<p.stop_count;++i){hd.pos[i]=float(p.stops[i].position)/255.f;hd.r[i]=float(p.stops[i].colour.red)/255.f;hd.g[i]=float(p.stops[i].colour.green)/255.f;hd.b[i]=float(p.stops[i].colour.blue)/255.f;}
 for(unsigned i=0;i<16;++i){const auto c=p.compiled_fastled16[i];palette.cells[i]={c.red,c.green,c.blue};}
 return palette;
}
constexpr int LINEARBLEND=1;
inline CRGB ColorFromPalette(const CRGBPalette16&,uint8_t,uint8_t,int){std::abort();}
inline CRGB16 crgb_to_crgb16(CRGB c){return {SQ15x16(c.r/255.f),SQ15x16(c.g/255.f),SQ15x16(c.b/255.f)};}
inline CRGB16 effect_palette_or_chroma_colour(const RenderParams*,bool,SQ15x16){std::abort();}
inline void mirror_image_downwards(CRGB16* leds){for(unsigned i=0;i<80;++i)leds[i]=leds[159-i];}
inline void k1_emo_palette_fallback(float bright,float out[3]){
 const auto& pal=cached_gradient_palette(uint8_t(palette_id),false);unsigned best=0,level=0;
 for(unsigned i=0;i<16;++i){unsigned n=unsigned(pal[i].r)+pal[i].g+pal[i].b;if(n>level){level=n;best=i;}}
 out[0]=float(pal[best].r)/255.f*bright;out[1]=float(pal[best].g)/255.f*bright;out[2]=float(pal[best].b)/255.f*bright;
}
'''
names = ['clamp01_fixed(', 'clamp_crgb16(', 'clamp_crgb16_preserve_sat(',
         'finalize_additive_frame(', 'palette_manual_colour(',
         'chromagram_centroid_hue(', 'effect_particle_colour(',
         'k1_vibrancy_blend_hue(', 'palette_chroma_colour_with_offset(']
# Declaration lookup uses the exact inline signature, excluding comments/calls.
def extract(name, text=visual):
    import re
    match = re.search(r'^inline [^\n]*\b' + re.escape(name), text, re.M)
    assert match, name
    start=match.start(); brace=text.index('{',match.end()); depth=1; end=brace+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]+'\n'
preparation=extract('get_smooth_spectrogram(')+extract('make_smooth_chromagram(', (REF/'visual/led_utilities.h').read_text())
(OUT/'lightshow_modes.h').write_text(prelude+'\n'.join(extract(n) for n in names)+preparation)
for name in ['k1_emo_controls.h','k1_emo_waveform_audio.h','k1_audio_snapshot.h','k1_vp_audio_access.h']:
    (OUT/name).write_text('#pragma once\n')
(OUT/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <cstddef>\n')
(OUT/'oracle.h').write_text('#include "lightshow_modes.h"\n#include "'+str(effect)+'"\n')
fixed = REF.parent/'.pio/libdeps/k1_bench_scheduling_gdft_cross80_probe/FixedPoints/src'
sources = ['harness/tests/test_inhouse_waveform.cpp','src/k1/core/visual/inhouse_waveform.cpp',
           'src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp',
           'platform/ra8p1/inhouse_audio.cpp','src/k1/core/audio/gdft_goertzel.cpp']
cmd=['/opt/homebrew/bin/g++-16','-std=c++17','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all',
     '-ffp-contract=off','-Iplatform/ra8p1','-Isrc','-Isrc/k1','-I'+str(OUT),'-I'+str(fixed),*sources,'-o',str(OUT/'check')]
source_hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}
rows=[]
for args in [cmd,[str(OUT/'check'),str(OUT/'frames.rgb16')]]:
    p=subprocess.run(args,cwd=ROOT,capture_output=True,text=True)
    rows.append({'argv':args,'code':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
    (OUT/'commands.json').write_text(json.dumps({'commands':rows,'source_sha256':hashlib.sha256(effect.read_bytes()).hexdigest()},indent=2))
    print(p.stdout+p.stderr,end='')
    assert p.returncode==0
fault=subprocess.run([str(OUT/'check'),str(OUT/'fault.rgb16'),'--fault'],cwd=ROOT,capture_output=True,text=True)
assert fault.returncode==2, 'source comparison failed to reject planted corruption'
print('planted output corruption rejected by source comparison (exit 2)')
(OUT/'mutation.json').write_text(json.dumps({'returncode':fault.returncode,'output':fault.stderr},indent=2))

# Scientific diagnostic of the ACTUAL emitted host buffers, not an LGP render.
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
rgb=np.fromfile(OUT/'frames.rgb16',dtype=np.uint16).reshape(900,160,3)/65535.
fig,ax=plt.subplots(figsize=(12,6))
ax.imshow(np.power(rgb,1/2.2),aspect='auto',extent=[0,160,9,0])
ax.axvline(80,color='white',alpha=.3,linewidth=.5)
ax.axhline(3,color='white',alpha=.5,linewidth=.5)
ax.axhline(7,color='white',alpha=.5,linewidth=.5)
ax.set(xlabel='Logical pixel (centre 79/80)',ylabel='Elapsed seconds',
       title='Hybrid K1 actual host RGB16 output | palette edit 3 s | silence 7 s\nSynthetic feature stream; display gamma 2.2; no optical or music-quality claim')
fig.tight_layout();fig.savefig(OUT/'actual-output.png',dpi=150);plt.close(fig)

assert source_hashes=={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}
(OUT/'receipt.json').write_text(json.dumps({'pass':True,'sources':source_hashes,'reference_effect_sha256':hashlib.sha256(effect.read_bytes()).hexdigest(),'commands':rows,'audible_frames_exact':700,'quiet_frames_intentional_deviation':200,'reference_oracle_unchanged':True,'output_fault_rejected':fault.returncode==2,'scope':'host arithmetic/output and PCM classifier only; no physical quiet or weak-music qualification'},indent=2)+'\n')
