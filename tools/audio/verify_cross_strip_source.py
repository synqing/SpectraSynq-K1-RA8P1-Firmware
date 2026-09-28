#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Host original-kernel comparison."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'docs/effects-language-001/reference/lightwave-cross-strip'

def verify(output):
    output.mkdir(parents=True,exist_ok=False)
    records=json.loads((SOURCE/'provenance.json').read_text())['records']
    for record in records:
        assert hashlib.sha256((ROOT/record['path']).read_bytes()).hexdigest()==record['sha256']
    original=(SOURCE/'CrossStripWaveInterferenceEffect.cpp').read_text()
    # Verbatim helpers, init and render. The shim supplies only context storage
    # and FastLED's declared fixed-scale CRGB primitive, not another wave model.
    kernel=original[original.index('namespace {'):original.index('const plugins::EffectMetadata&')]
    preamble=r'''
#include "cross_strip_reference.h"
#include <cassert>
#include <cstdint>
#include <cmath>
struct CRGB {
 uint8_t r=0,g=0,b=0;
 CRGB()=default;
 CRGB(uint8_t a,uint8_t c,uint8_t d):r(a),g(c),b(d){}
 void nscale8(uint8_t scale) {r=(uint16_t(r)*(1+uint16_t(scale)))>>8;g=(uint16_t(g)*(1+uint16_t(scale)))>>8;b=(uint16_t(b)*(1+uint16_t(scale)))>>8;}
};
namespace plugins {
struct EffectContext {
 unsigned stripCount=2,stripLength=160,stripCenter=79,speed=50,intensity=255,brightness=24;
 CRGB* stripLeds[2]; bool dualChannelMode=false;float rawDeltaTimeSeconds=1.F/120;
 float getSafeRawDeltaSeconds() const {float dt=rawDeltaTimeSeconds;if(dt<.0001F)dt=.0001F;if(dt>.05F)dt=.05F;return dt;}
};
}
class CrossStripWaveInterferenceEffect {
 public:
 bool init(plugins::EffectContext&);
 void render(plugins::EffectContext&);
 float m_phaseOffsetCycles=.375F,m_wavePhase=0,m_speedHz=.12F,m_contrast=.88F,m_wavelengthLeds=12;
};
'''
    main=r'''
int main() {
 CRGB top[160],bottom[160];plugins::EffectContext ctx;ctx.stripLeds[0]=top;ctx.stripLeds[1]=bottom;
 CrossStripWaveInterferenceEffect original;original.init(ctx);
 k1::reference::CrossStripReference adapter;
 for(unsigned frame=0;frame<1800;++frame) {
  ctx.rawDeltaTimeSeconds=frame<900?1.F/120:frame<1200?1.F/30:frame<1500?.1F:0.F;
  if(frame==500){original.m_phaseOffsetCycles=.25F;adapter.phase_offset=.25F;}
  if(frame==1000){original.m_wavelengthLeds=20;adapter.wavelength=20;}
  if(frame==1400){original.m_speedHz=.24F;adapter.drift_hz=.24F;}
  original.render(ctx);const auto& out=adapter.step(ctx.rawDeltaTimeSeconds);
  assert(ctx.dualChannelMode);
  for(unsigned i=0;i<160;++i) {
   auto a=out.top[i],b=out.bottom[i];
#ifdef COLLAPSE_EDGES
   b=a;
#endif
   assert(a.red==unsigned(top[i].r)*257 && a.green==unsigned(top[i].g)*257 && a.blue==unsigned(top[i].b)*257);
   assert(b.red==unsigned(bottom[i].r)*257 && b.green==unsigned(bottom[i].g)*257 && b.blue==unsigned(bottom[i].b)*257);
  }
 }
}
'''
    cpp=output/'original_kernel_probe.cpp';cpp.write_text(preamble+kernel+main)
    results=[]
    for mutation in [False,True]:
        exe=output/('edge-collapse' if mutation else 'original-parity')
        argv=['c++','-std=c++17','-O2','-ffp-contract=off','-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'tests/reference'),*(['-DCOLLAPSE_EDGES'] if mutation else []),str(cpp),'-o',str(exe)]
        subprocess.run(argv,check=True,capture_output=True,text=True)
        run=subprocess.run([str(exe)],capture_output=True,text=True)
        results.append(dict(command=argv,run=[str(exe)],exit=run.returncode,stderr=run.stderr,expected_failure=mutation))
        (output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
        assert (run.returncode!=0)==mutation,run.stderr
    return results

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True)
    verify(p.parse_args().output.resolve());print('1800 original kernel frames matched; edge-collapse mutation rejected')
