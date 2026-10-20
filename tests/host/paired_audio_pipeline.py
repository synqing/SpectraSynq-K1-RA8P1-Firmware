#!/usr/bin/env python3
"""Exercise actual paired capture/runtime bodies; peripheral timing is outside this host proof."""
from pathlib import Path
import importlib.util,json,subprocess,re,shutil,argparse
T=Path(__file__).resolve().parent; F=T.parents[1]; S=F; P=F/'platform/ra8p1'
parser=argparse.ArgumentParser();parser.add_argument('--output-dir',type=Path,required=True);parser.add_argument('--raw-is-guarded',action='store_true');args=parser.parse_args();OUT=args.output_dir.resolve();OUT.mkdir(parents=True,exist_ok=False)
spec=importlib.util.spec_from_file_location('original_harness',F/'tests/host/test_pdm_asrc_admission.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
source=(S/'platform/ra8p1/pdm_target.c').read_text()
if args.raw_is_guarded:
 old='k1_poly24_push_paired(&capture_poly24, capture_pcm, capture_raw_pcm,'
 assert source.count(old)==1;source=source.replace(old,'k1_poly24_push_paired(&capture_poly24, capture_pcm, capture_pcm,')
# Reuse the retained production-body extraction recipe, with its documented I/O seams.
base=module.harness(source).split('/* Appended by test_pdm_asrc_admission.py')[0]
base=base.replace('#include <assert.h>','#include <assert.h>\n#include "k1_poly24.h"')
def extract(name):
 match=re.search(r'^(?:static )?(?:void|uint32_t|k1_audio_read_result_t)\s*'+name+r'\([^;]*?\)\s*\{',source,re.M)
 assert match,name
 end=source.index('{',match.start())+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[match.start():end]
for name in ['k1_pdm_target_set_analysis24','k1_pdm_target_try_read_analysis24','k1_pdm_target_last_hop_gain_q8','k1_pdm_target_take_hop_cost','k1_pdm_target_take_analysis24_cycles','k1_pdm_target_analysis24_status']:
 base+='\n'+extract(name)+'\n'
# Feed the actual raw-slot owner, sensitivity guard, both resamplers and descriptor body.
base+='''
void host_capture_init(void) {
 memset(capture_stream,0,sizeof capture_stream);memset(capture_owner,0,sizeof capture_owner);
 memset(capture_metrics,0,sizeof capture_metrics);memset(&capture_asrc,0,sizeof capture_asrc);
 memset(&capture_poly24,0,sizeof capture_poly24);
 capture_stream_epoch=0;capture_a24_epoch=0;capture_rate_segment=0;
 capture_stale_active=0;capture_meta_count=0;capture_discontinuity_pending=0;
 capture_asrc.source_hz=41422;
 k1_pdm_target_reset_logical_stream(0);
 capture_initialised=1;capture_running=1;host_time_us=1000000;
}
void host_capture_raw(const int16_t raw[296], uint64_t now) {
 host_time_us=now;
 capture_owner[0].slot=0;capture_owner[0].sequence++;
 capture_owner[0].epoch=1;capture_owner[0].capture_end_us=now;capture_owner[0].valid=1;
 for(unsigned i=0;i<296;++i)capture_buffer[0][0][i]=(int32_t)((uint32_t)(int32_t)raw[i]&0xfffffu);
 k1_pdm_target_process_lane(0);
}
uint32_t host_capture_drops(void) {return capture_a24_drops+capture_asrc_push_drop;}
'''
(OUT/'capture.c').write_text(base)
flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-ffp-contract=off','-DK1_EMOTISCOPE_PORT=1','-DK1_INHOUSE_MAPPING_V9=1','-DK1_INHOUSE_ANALYSIS24=1','-DK1_PDM_TARGET=1','-DK1_PDM_STREAM_SLOT_COUNT=4u','-I'+str(S/'platform/ra8p1'),'-I'+str(S/'src'),'-I'+str(S/'src/k1'),'-I'+str(P),'-I'+str(F/'src'),'-I'+str(F/'src/k1')]
commands=[]
def run(argv):
 p=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=110)
 commands.append(dict(argv=[str(x) for x in argv],exit=p.returncode,stdout=p.stdout,stderr=p.stderr));(OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 if p.returncode:raise RuntimeError(commands[-1])
 return p
objects=[]
for file in [OUT/'capture.c',S/'platform/ra8p1/k1_poly24.c',P/'k1_asrc_24k.c',P/'pdm_capture.c',P/'k1_live_clock.c',S/'src/k1/core/audio/k1_audio_hop.c']:
 obj=OUT/(file.stem+'.o');run(['/usr/bin/clang','-std=c11',*flags,'-c',file,'-o',obj]);objects.append(obj)
cpp=[T/'paired_audio_pipeline_cases.cpp',S/'platform/ra8p1/k1_live_runtime.cpp',S/'platform/ra8p1/inhouse_audio.cpp',P/'k1_live_protocol.cpp',
 *[S/'src/k1/core/audio'/x for x in ['gdft_goertzel.cpp','tempo_tracker.cpp','tempo_acf.cpp']],F/'src/k1/core/visual/visual_audio_frame.cpp']
run(['/usr/bin/clang++','-std=c++17',*flags,*cpp,*objects,'-o',OUT/'pipeline'])
p=run([OUT/'pipeline']);print(p.stdout)
