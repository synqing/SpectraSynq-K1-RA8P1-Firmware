#!/usr/bin/env python3
"""Paired loudness metadata, response controls and production capture/runtime regression."""
from pathlib import Path
import argparse,subprocess,json,hashlib,sys
T=Path(__file__).resolve().parent; S=T.parents[1]
p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path,required=True);args=p.parse_args();OUT=args.output_dir.resolve();OUT.mkdir(parents=True,exist_ok=False)
commands=[]
def run(argv,expected=0):
 r=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=110)
 commands.append(dict(argv=[str(x) for x in argv],exit=r.returncode,expected=expected,stdout=r.stdout,stderr=r.stderr));(OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 assert (r.returncode==0 if expected==0 else r.returncode!=0 and 'Assertion failed' in r.stderr),commands[-1]
 return r
flags=['-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-ffp-contract=off','-DK1_EMOTISCOPE_PORT=1','-DK1_INHOUSE_MAPPING_V9=1','-I'+str(S/'platform/ra8p1'),'-I'+str(S/'src'),'-I'+str(S/'src/k1')]
units=[S/'src/k1/core/audio'/x for x in ['gdft_goertzel.cpp','tempo_tracker.cpp','tempo_acf.cpp']]
original=(S/'platform/ra8p1/inhouse_audio.cpp').read_text()
variants=[('actual',original,0),('wrong_epoch_admitted',original.replace('level.stream_epoch == hop.stream_epoch','true'),1),('stale_level_leaks',original.replace('  out.unguarded_vu_valid = false;','  /* planted stale validity leak */'),1)]
for name,text,expect in variants:
 path=OUT/(name+'.cpp');path.write_text(text)
 run(['/usr/bin/clang++',*flags,T/'paired_audio_publication_cases.cpp',path,*units,'-o',OUT/name])
 run([OUT/name],expect)
# Extract the real consumer block; keep exactly its response/clamp arithmetic.
palette=(S/'platform/ra8p1/palette_runtime.cpp').read_text()
block=palette[palette.index('  const float peak_limit='):palette.index('  if(!inhouse_renderer_[board].render')]
consumer='''#include "core/visual/inhouse_waveform.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <limits>
#include <cstdio>
using k1::core::visual::InhouseWaveformAudio;
void control(InhouseWaveformAudio& input,float response) {
 struct {int current_mode=18;} config;
'''+block+'''
}
int main(){
 for(float gain:{0.f,.5f,1.f,2.f}){
  InhouseWaveformAudio loud{};loud.vu_level=.65f;loud.unguarded_vu=6.4f;loud.unguarded_vu_valid=true;loud.unguarded_vu_guarded=true;
  control(loud,gain);assert(loud.unguarded_vu_valid);assert(loud.unguarded_vu==6.4f*gain);
  for(float quiet:{.1f,.8f}){InhouseWaveformAudio q{};q.vu_level=q.unguarded_vu=quiet;q.unguarded_vu_valid=true;
   control(q,gain);assert(q.unguarded_vu_valid);assert(q.unguarded_vu==std::clamp(quiet*gain,0.f,1.f));}
 }
 for(float gain:{-1.f,2.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  InhouseWaveformAudio a{};a.unguarded_vu=6.4f;a.unguarded_vu_valid=true;a.unguarded_vu_guarded=true;control(a,gain);assert(!a.unguarded_vu_valid&&a.unguarded_vu==0);
 }
 InhouseWaveformAudio overflow{};overflow.unguarded_vu=std::numeric_limits<float>::max();overflow.unguarded_vu_valid=true;overflow.unguarded_vu_guarded=true;control(overflow,2);assert(!overflow.unguarded_vu_valid);
 puts("PASS: actual palette response block 0/0.5/1/2, quiet exact clamp, loud unbounded headroom, invalid gain and overflow rejection");
}
'''
(OUT/'consumer.cpp').write_text(consumer)
run(['/usr/bin/clang++',*flags,OUT/'consumer.cpp','-o',OUT/'consumer']);run([OUT/'consumer'])
mutant=consumer.replace('const float scaled=input.unguarded_vu_guarded?input.unguarded_vu*response:input.vu_level;','const float scaled=std::clamp(input.unguarded_vu*response,0.0f,1.0f);')
assert mutant!=consumer;(OUT/'consumer-clamp-mutant.cpp').write_text(mutant)
run(['/usr/bin/clang++',*flags,OUT/'consumer-clamp-mutant.cpp','-o',OUT/'consumer-clamp-mutant']);run([OUT/'consumer-clamp-mutant'],1)
# The production-body pipeline owns its output directory and preserves failed runs.
run([sys.executable,T/'paired_audio_pipeline.py','--output-dir',OUT/'pipeline'])
run([sys.executable,T/'paired_audio_pipeline.py','--output-dir',OUT/'pipeline-raw-guarded-mutant','--raw-is-guarded'],1)
summary=dict(status='PASS',scope='Actual repository source with ASan/UBSan; no peripheral timing or full renderer acceptance',
 checks=['paired publication','nine absent/malformed metadata fallbacks','quiet VU exact','stale/board/gap/epoch controls','zero runtime allocations','actual palette response control','actual capture/runtime/publication path'],
 negative_controls=['wrong metadata epoch admitted','stale level valid leak','loud level clamped to unity','guarded PCM substituted for raw RMS'],
 product_source_sha256={name:hashlib.sha256((S/name).read_bytes()).hexdigest() for name in ['platform/ra8p1/k1_poly24.c', 'platform/ra8p1/k1_poly24.h', 'src/k1/core/audio/k1_audio_hop.h', 'platform/ra8p1/pdm_target.c', 'platform/ra8p1/inhouse_audio.h', 'platform/ra8p1/inhouse_audio.cpp', 'src/k1/core/visual/inhouse_waveform.h', 'platform/ra8p1/k1_live_runtime.cpp', 'platform/ra8p1/palette_runtime.cpp', 'src/k1/core/visual/inhouse_waveform.cpp']})
(OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
