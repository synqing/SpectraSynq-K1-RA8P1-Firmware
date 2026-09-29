#!/usr/bin/env python3
"""Execute the actual PDM ownership/admission bodies with host DMA/time seams.

No copied implementation: extract production C bodies; link actual capture, ASRC
and audio-hop units. Peripheral register setup/ISR timing is outside this proof.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / 'platform/ra8p1'

def function(source, name):
    match = re.search(r'^(?:static )?(?:void|int|k1_audio_read_result_t|const k1_capture_block_meta_t \*)\s*' + name + r'\([^;]*?\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('definition missing: ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        if source[end] == '{': depth += 1
        if source[end] == '}': depth -= 1
        end += 1
    return source[match.start():end]

def harness(source):
    state = source[source.index('typedef struct {'):source.index('static pdm_instance_ctrl_t capture_fall_pdm_ctrl;')]
    state += source[source.index('static volatile int capture_initialised;'):source.index('static uint64_t k1_pdm_target_now_cycles(void)')]
    names = ['k1_pdm_target_reset_logical_stream', 'k1_pdm_target_meta_for', 'k1_pdm_target_retire_meta',
             'k1_pdm_target_acquire', 'k1_pdm_target_release', 'k1_pdm_target_observe_rate',
             'k1_pdm_target_process_lane', 'k1_pdm_target_process_pair',
             'k1_pdm_target_poll_stale', 'k1_pdm_target_poll', 'k1_pdm_target_try_read_ap_hop']
    prefix = '''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "pdm_target.h"
#include "pdm_capture.h"
#include "k1_pdm_fifo16.h"
#include "k1_pdm_sensitivity.h"
typedef int fsp_err_t;
#define FSP_SUCCESS 0
'''
    seams = '''static uint64_t host_time_us = 1000000u;
static uint64_t k1_pdm_target_now_us(void) { return host_time_us; }
static uint64_t k1_pdm_target_now_cycles(void) { return host_time_us * 1000u; }
static void k1_pdm_target_invalidate_slot(uint32_t lane, uint32_t slot) {(void)lane;(void)slot;}
static void k1_pdm_target_stop_drivers(void) { capture_running=0; }
static int k1_pdm_target_start_drivers(void) { return 0; }
'''
    return prefix + state + seams + '\n\n'.join(function(source, n) for n in names) + '\n' + (ROOT/'tests/host/test_pdm_asrc_admission_cases.c').read_text()

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path);args=ap.parse_args()
    source=(P/'pdm_target.c').read_text()
    mutations={
        'provisional_must_not_wait_two_seconds': ('elapsed_us < 250000ull', 'elapsed_us < 2000000ull'),
        'provisional_must_not_claim_locked': ('capture_rate_locked = elapsed_us >= 2000000ull ? 1u : 0u;', 'capture_rate_locked = 1u;'),
        'push_loss_must_reset_logical_stream': ('k1_pdm_target_reset_logical_stream(2u);', '/* mutation: hide loss */'),
        'asrc_loss_must_keep_raw_calibration': ('if (reason != 2u)', 'if (1)'),
        'rate_must_use_raw_sequence': ('(programme->sequence - capture_rate_first_sequence)', '(capture_metrics[0].processed_slots)'),
        'loss_requires_discontinuity': ('capture_discontinuity_pending = reason != 0u ? 1u : 0u;', 'capture_discontinuity_pending = 0u;'),
    }
    results=[]
    with tempfile.TemporaryDirectory(prefix='titan-asrc-admission-') as tmp:
        td=Path(tmp)
        for name, changed in [('actual', source)]+[(n,source.replace(a,b,1)) for n,(a,b) in mutations.items()]:
            if name=='push_loss_must_reset_logical_stream':
                changed=changed.replace('k1_pdm_target_reset_logical_stream(2u);','/* mutation: hide loss */')
            if name!='actual' and changed==source: raise RuntimeError('mutation did not apply: '+name)
            c=td/(name+'.c');c.write_text(harness(changed));binary=td/name
            cmd=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-variable','-Wno-unused-function','-fsanitize=address,undefined','-fno-omit-frame-pointer','-DK1_PDM_STREAM_SLOT_COUNT=4u','-I'+str(P),'-I'+str(ROOT/'src'),str(c),str(P/'k1_asrc_24k.c'),str(P/'pdm_capture.c'),str(ROOT/'src/k1/core/audio/k1_audio_hop.c'),'-o',str(binary)]
            built=subprocess.run(cmd,capture_output=True,text=True)
            if built.returncode: raise RuntimeError(built.stderr)
            ran=subprocess.run([str(binary)],capture_output=True,text=True)
            ok=(ran.returncode==0) if name=='actual' else (ran.returncode!=0 and 'Assertion failed' in ran.stderr)
            results.append({'case':name,'accepted':ok,'exit_code':ran.returncode,'stdout':ran.stdout,'stderr':ran.stderr,'compile_command':cmd})
            print(name, 'PASS' if ok else 'FAIL', 'exit',ran.returncode)
            if not ok: print(ran.stdout,ran.stderr)
        receipt={'pass':all(x['accepted']for x in results),'scope':'host production functions, no peripheral timing proof','sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [P/'pdm_target.c',P/'k1_asrc_24k.c',P/'k1_asrc_24k.h',P/'pdm_capture.c',ROOT/'tests/host/test_pdm_asrc_admission_cases.c',Path(__file__).resolve()]},'results':results}
        if args.output:
            args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(receipt,indent=2)+'\n')
        assert receipt['pass']
if __name__=='__main__':main()
