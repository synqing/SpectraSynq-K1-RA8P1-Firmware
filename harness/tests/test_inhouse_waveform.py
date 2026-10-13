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
struct RenderParams {bool MIRROR_ENABLED=true;float CHROMA=.05f,SATURATION=1.f,SQUARE_ITER=1.f,SENSITIVITY=2.4f,PHOTONS=1.f,SWEET_SPOT_MIN_LEVEL=350.f;unsigned SAMPLES_PER_CHUNK=96;};
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
struct K1AudioSnapshot {float peak_scaled=0,vu_level=0;bool silence=true;float spectral_energy=0,novelty=0,low_energy=0,mid_energy=0,high_energy=0,chroma_strength=0;unsigned frame_ms=0;};
inline K1AudioSnapshot snap;
inline K1AudioSnapshot k1_vp_audio_snapshot_read(){return snap;}
struct Input{float peak=0,raw_peak=0,vu_average=0;bool history_valid=false;int16_t history[4][96]{};};inline Input input;
inline const Input& k1_emo_waveform_audio(){return input;}
inline float motion=1.f;
inline float k1_emo_motion_scale(){return motion;}
struct ChannelEffectState {
 uint32_t tempo_last_ms=0;float tempo_scroll_accum=0;
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
inline bool chromatic_mode=true,debug_mode=false,led_thread_halt=false;
inline SQ15x16 chroma_val;
inline float waveform_peak_scaled=0,max_waveform_val_raw=0,audio_vu_level=0;
inline CRGB16 leds_16_prev_secondary[160];
struct Serial {template<class T>void print(T){} template<class T>void println(T){}};inline Serial USBSerial;
inline CRGB16 hsv(SQ15x16,SQ15x16,SQ15x16){return {};}
inline CRGB force_saturation(CRGB c,uint8_t){return c;}
inline CRGB force_hue(CRGB c,uint8_t){return c;}
inline void shift_leds_up(CRGB16* p,uint16_t steps){for(unsigned s=0;s<steps;++s){for(unsigned i=159;i>0;--i)p[i]=p[i-1];p[0]={};}}
#define ENABLE_WAVEFORM_CHROMAGRAM_COLOR 1
constexpr float WAVEFORM_REACTIVE_RAW_MARGIN=1.10f,WAVEFORM_REACTIVE_PEAK_FLOOR=.08f,WAVEFORM_IDLE_FADE=.85f;
inline float VP_WAVEFORM_SHIFT_RATE=120.f,VP_WAVEFORM_IDLE_FADE=.985f,VP_WAVEFORM_REACTIVE_RAW_MARGIN=1.10f,
 VP_WAVEFORM_REACTIVE_PEAK_FLOOR=.08f,VP_WAVEFORM_ACTIVE_FADE_REDUCTION=.04f,VP_WAVEFORM_VU_FLOOR=.02f;
struct K1TempoEvent{float bpm=0,phase01=0,confidence=0,beat_strength=0;bool locked=false,beat_tick=false;};
inline K1TempoEvent tempo;
inline K1TempoEvent k1_vp_tempo_read(){return tempo;}
inline CRGB16 effect_palette_or_chroma_colour(const RenderParams*,bool,SQ15x16);
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
         'k1_vibrancy_blend_hue(', 'palette_chroma_colour_with_offset(',
         'palette_chroma_colour(', 'effect_palette_or_chroma_colour(',
         'waveform_full_strip_position(', 'waveform_upper_half_source_position(',
         'waveform_shift_upper_half_up(', 'waveform_shift_outward(']
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
for name in ['k1_emo_controls.h','k1_emo_waveform_audio.h','k1_audio_snapshot.h','k1_vp_audio_access.h','k1_tempo.h']:
    (OUT/name).write_text('#pragma once\n')
(OUT/'Arduino.h').write_text('#pragma once\n#include <cstdint>\n#include <cstddef>\n')
effects=[effect]+[REF/'effects'/name for name in ['light_mode_waveform_tempo.cpp','light_mode_waveform.cpp','light_mode_waveform_fast.cpp','light_mode_waveform_hybrid.cpp']]
(OUT/'oracle.h').write_text('#include "lightshow_modes.h"\n'+''.join('#include "'+str(e)+'"\n' for e in effects))
fixed = REF.parent/'.pio/libdeps/k1_bench_scheduling_gdft_cross80_probe/FixedPoints/src'
sources = ['harness/tests/test_inhouse_waveform.cpp','src/k1/core/visual/inhouse_waveform.cpp',
           'src/k1/core/visual/product_palette.cpp','src/k1/core/visual/product_palette_data.cpp',
           'platform/ra8p1/inhouse_audio.cpp','src/k1/core/audio/gdft_goertzel.cpp','src/k1/core/audio/tempo_tracker.cpp','src/k1/core/audio/tempo_acf.cpp']
cmd=['/opt/homebrew/bin/g++-16','-std=c++17','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all',
     '-ffp-contract=off','-Iplatform/ra8p1','-Isrc','-Isrc/k1','-I'+str(OUT),'-I'+str(fixed),*sources,'-o',str(OUT/'check')]
tracked_inputs=sources+['src/k1/core/visual/inhouse_waveform.h','harness/tests/test_inhouse_waveform.py']
source_hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in tracked_inputs}
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

family_results=[]
for mode in [18,8,7,11]:
    run=subprocess.run([str(OUT/'check'),str(OUT/f'mode-{mode}.rgb16'),'--mode',str(mode)],cwd=ROOT,capture_output=True,text=True)
    family_results.append({'mode':mode,'code':run.returncode,'stdout':run.stdout,'stderr':run.stderr})
    (OUT/'family-results.json').write_text(json.dumps(family_results,indent=2)+'\n')
    print(run.stdout+run.stderr,end='');assert run.returncode==0
    fault_run=subprocess.run([str(OUT/'check'),str(OUT/f'mode-{mode}-fault.rgb16'),'--mode',str(mode),'--fault'],cwd=ROOT,capture_output=True,text=True)
    assert fault_run.returncode==2, f'mode {mode} comparator accepted corruption'
    family_results[-1]['planted_corruption_exit']=fault_run.returncode
    (OUT/'family-results.json').write_text(json.dumps(family_results,indent=2)+'\n')

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
fig,axes=plt.subplots(4,1,figsize=(12,12),sharex=True)
for ax,mode in zip(axes,[18,8,7,11]):
    pixels=np.fromfile(OUT/f'mode-{mode}.rgb16',dtype=np.uint16).reshape(900,160,3)/65535.
    ax.imshow(np.power(pixels,1/2.2),aspect='auto',extent=[0,160,9,0])
    ax.axvline(80,color='white',alpha=.3,linewidth=.5)
    ax.axhline(7,color='white',alpha=.5,linewidth=.5)
    ax.set(ylabel=f'Mode {mode} / seconds')
axes[0].set_title('Actual renderer RGB16 buffers | synthetic features, gamma 2.2\nPalette edit 3s; contrast 4s; sensitivity/photons 4.5s; mirror off 5s; speed 6s; quiet 7s')
axes[-1].set_xlabel('Logical pixel (centre 79/80); source reconstruction, not an optical simulation')
fig.tight_layout();fig.savefig(OUT/'family-actual-output.png',dpi=150);plt.close(fig)


assert source_hashes=={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in tracked_inputs}
(OUT/'receipt.json').write_text(json.dumps({'pass':True,'sources':source_hashes,'reference_effect_sha256':hashlib.sha256(effect.read_bytes()).hexdigest(),'family_references':{str(e):hashlib.sha256(e.read_bytes()).hexdigest() for e in effects+[REF/'visual/lightshow_modes.h',REF/'visual/led_utilities.h']},'family_results':family_results,'reconstruction':{'render_hz':100,'first_dt_ms':10,'per_render_followers':'elapsed-time conversion against100Hz; exact oracle at100Hz','flags':['K1_PALETTE_VIBRANCY_V1','K1_FALLBACK_HELD_U_V1','K1_POSITION_SMOOTH_V1','K1_EMO_WAVEFORMS_V1','ENABLE_WAVEFORM_CHROMAGRAM_COLOR=1'],'quiet':'confirmed silence and invalidity prohibit birth; intentional divergence from old source peak veto','controls':'mode8 sensitivity0.1..20 default2.4, square_iter0..8 default1 for7/8/11, mode8photons.05..1 default1; speed8 inert'},'mode11_variant':'unflagged PCM radius contract; K1_WAVEFORM_HYBRID_TRAIL_DEPOSIT_V1 disabled deliberately per M04','commands':rows,'audible_frames_exact':700,'quiet_frames_intentional_deviation':200,'reference_oracle_unchanged':True,'output_fault_rejected':fault.returncode==2,'scope':'host arithmetic/output and PCM classifier only; no physical quiet or weak-music qualification'},indent=2)+'\n')
