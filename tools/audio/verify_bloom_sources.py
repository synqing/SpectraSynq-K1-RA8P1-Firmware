#!/usr/bin/env python3
"""Execute retained official statements against the small reference adapters.

THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Host only. This extracts transport,
injection and history call order, not donor analysis/HSV/output firmware.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
DONORS=ROOT/'docs/effects-language-001/reference/donor-local'

def function(text, signature):
    start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

def verify(output):
    output.mkdir(parents=True,exist_ok=False)
    sb=DONORS/'sensory-4.1.1/SENSORY_BRIDGE_FIRMWARE'
    emo=DONORS/'emotiscope-1.1/src'
    inputs=[sb/'led_utilities.h',sb/'lightshow_modes.h',emo/'leds.h',emo/'light_modes/active/bloom.h']
    original=function(inputs[1].read_text(),'void light_mode_bloom()')
    before=original[original.index('{')+1:original.index('//-------------------------------------------------------')]
    after=original[original.index('  leds_16[63] = { temp_col.r'):original.rfind('}')]
    cpp='''#include "bloom_donors.h"
#include <cassert>
#include <cstring>
#include <cstdio>
namespace original_sb {
using CRGB16=k1::reference::SensoryRgbBloom::FixedRgb;
constexpr int NATIVE_RESOLUTION=128;
CRGB16 leds_16[128]{},leds_16_prev[128]{};
struct {float MOOD=0;} CONFIG;
struct RGB8 {unsigned char r,g,b;};
'''+function(inputs[0].read_text(),'void draw_sprite(CRGB16')+'\nvoid step(RGB8 temp_col){'+before+after+'}\n}\n'
    cpp+='''namespace original_emo {
using std::min;
constexpr int NUM_LEDS=128;
struct CRGBF {float r,g,b;};
CRGBF leds[128]{};float vu_level=0,num_leds_float_lookup[128]{};
struct {float speed=0,saturation=1;bool mirror_mode=true;} configuration;
float clip_float(float x){return std::clamp(x,0.F,1.F);}
float get_color_range_hue(float){return 0;}
CRGBF hsv(float,float,float value){return {value,0,0};} // Declared red mapping boundary.
'''+function(inputs[2].read_text(),'void draw_sprite(float')+'\n'+inputs[3].read_text()+'\n}\n'
    cpp+='''int main(){
 k1::reference::SensoryRgbBloom sb; k1::reference::EmotiscopeScalarBloom emo;
 std::array<k1::reference::Rgb,64> red{};red.fill({1,0,0});
 for(unsigned frame=0;frame<1800;++frame){
  const unsigned char r=frame%83<9?static_cast<unsigned char>((frame*37)%256):0;
  const unsigned char g=frame%71<5?static_cast<unsigned char>((frame*19)%256):0;
  const float mood=frame<900?.2F:.73F,vu=r/255.F;
  original_sb::CONFIG.MOOD=mood;original_sb::step({r,g,0});sb.step({r/255.F,g/255.F,0},mood);
  original_emo::configuration.speed=mood;original_emo::vu_level=vu;original_emo::draw_bloom();emo.step(vu,mood,red);
  for(unsigned i=0;i<128;++i){
   assert(float(sb.fixed_history[i].r)==float(original_sb::leds_16_prev[i].r));
   assert(float(sb.fixed_history[i].g)==float(original_sb::leds_16_prev[i].g));
   assert(sb.frame[i].red==float(original_sb::leds_16[i].r));
   assert(sb.frame[i].green==float(original_sb::leds_16[i].g));
   assert(emo.history[i]==original_emo::novelty_image_prev[i]);
   assert(emo.frame[i].red==original_emo::leds[i].r);
  }
 }
 std::puts("1800 frames: original retained statements and both adapters match exactly at declared boundaries");
}
'''
    source=output/'official_statement_probe.cpp';source.write_text(cpp)
    exe=output/'probe'
    command=['/opt/homebrew/bin/g++-15','-std=c++17','-O2','-ffp-contract=off','-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'tests/reference'),str(source),'-o',str(exe)]
    compiled=subprocess.run(command,capture_output=True,text=True)
    run=subprocess.run([str(exe)],capture_output=True,text=True) if compiled.returncode==0 else None
    result=dict(source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                command=command,compile_exit=compiled.returncode,compile_log=compiled.stdout+compiled.stderr,
                run_exit=run.returncode if run else None,run_log=run.stdout+run.stderr if run else None,
                boundary='Original draw_sprite and retained-history statements; supplied post-colour bytes for SB, red HSV mapping for Emotiscope. Not full donor AP/output parity.')
    (output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2));return 0 if run and run.returncode==0 else 1

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);raise SystemExit(verify(p.parse_args().output))
