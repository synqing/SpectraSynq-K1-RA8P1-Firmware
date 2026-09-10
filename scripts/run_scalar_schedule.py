#!/usr/bin/env python3
"""Run the resident actual-K1 scalar schedule on one identity-gated Titan."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib
from datetime import datetime,timezone
from run_scalar_target import packet,read_exact,UID
from target_resources import validate_resources
from verify_imports import PIN
MODES={'scalar':0,'scheduled':1,'saturation':2,'npu-alone':3}

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

PROFILE_FULL_HOP_STAGES={
    'ap_total','gdft_raw','gdft_postprocess','features','onset_saliency',
    'tempo_total','musical_time','telemetry',
}
PROFILE_UPDATE_STAGES={
    'tempo_history','tempo_acf_total','acf_prepare','acf_correlate','acf_comb',
    'acf_normalise','tempo_bank','tempo_flywheel','tempo_output',
}

def validate_stage_profile(status,expected_hops,expected_renders):
    profile=status.get('stage_profile')
    if not isinstance(profile,dict) or profile.get('bin_width_cycles')!=16384:
        raise RuntimeError('stage profile metadata missing or divergent')
    stages=profile.get('stages')
    expected=PROFILE_FULL_HOP_STAGES|PROFILE_UPDATE_STAGES|{'vp_render'}
    if not isinstance(stages,dict) or set(stages)!=expected:
        raise RuntimeError('stage profile stage set missing or divergent')
    for name in PROFILE_FULL_HOP_STAGES:
        if stages[name].get('count')!=expected_hops:
            raise RuntimeError(f'stage profile count mismatch: {name}')
    if stages['vp_render'].get('count')!=expected_renders:
        raise RuntimeError('stage profile count mismatch: vp_render')
    update_counts={stages[name].get('count') for name in PROFILE_UPDATE_STAGES}
    if len(update_counts)!=1 or not update_counts or next(iter(update_counts))<=0:
        raise RuntimeError('tempo-update stage counts missing or divergent')
    if next(iter(update_counts))>expected_hops:
        raise RuntimeError('tempo-update stage count exceeds hop count')
    for name,measurement in stages.items():
        required={'count','mean_cycles','p50_bin_lower_cycles','p95_bin_lower_cycles','p99_bin_lower_cycles','max_cycles'}
        if set(measurement)!=required or any(measurement[field]<0 for field in required):
            raise RuntimeError(f'invalid stage profile measurement: {name}')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--resident',type=Path,required=True)
    parser.add_argument('--profile',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--loops',type=int,default=1)
    parser.add_argument('--qualification',action='store_true')
    parser.add_argument('--mutation',action='store_true',help='prove the target comparator rejects one deliberately wrong expected CRC')
    parser.add_argument('--mode',choices=MODES,default='scalar')
    parser.add_argument('--failure-campaign',action='store_true',help='inject the frozen semantic-sidecar failure and recovery cells')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='ON-SILICON',gate='G4_SCALAR_SUBPROFILE',start=datetime.now(timezone.utc).isoformat(),
                 loops=args.loops,qualification=args.qualification,mutation=args.mutation,mode=args.mode,
                 failure_campaign=args.failure_campaign,**{'pass':False})
    port=None
    try:
        import serial
        from serial.tools import list_ports
        profile=json.loads(args.profile.read_text()); build=json.loads((args.build/'receipt.json').read_text())
        resident=json.loads((args.resident/'receipt.json').read_text())
        receipt['identities']=dict(profile_sha256=sha(args.profile),build_receipt_sha256=sha(args.build/'receipt.json'),
                                  resident_receipt_sha256=sha(args.resident/'receipt.json'))
        if profile['status'] not in ('FROZEN_SCALAR_SUBPROFILE_NOT_YET_QUALIFIED','FROZEN_NPU_LOAD_PROFILE_NOT_YET_QUALIFIED') or profile['source_pin']!=PIN:
            raise RuntimeError('wrong or unfrozen scalar profile')
        if not build['pass'] or build['source_pin']!=PIN or 'external/resident_controls.h' not in build['sources']:
            raise RuntimeError('build is not an accepted resident K1 image')
        if not resident['pass'] or resident['schedule_qualified']:
            raise RuntimeError('resident preparation receipt invalid')
        if build['sources']['external/resident_controls.h']!=profile['fixture']['resident_header_sha256'] or resident['header_sha256']!=profile['fixture']['resident_header_sha256']:
            raise RuntimeError('resident header identity mismatch')
        if resident['pcm_sha256']!=profile['fixture']['pcm_sha256'] or resident['hops']!=profile['fixture']['hops_per_loop']:
            raise RuntimeError('resident fixture identity mismatch')
        mode=MODES[args.mode]
        if mode and not build.get('npu'): raise RuntimeError('NPU mode requires an identified NPU build')
        if profile['status'].startswith('FROZEN_NPU'):
            if not mode: raise RuntimeError('NPU profile requires an NPU mode')
            baseline=profile['scalar_baseline']
            if build['flags']!=baseline['compiler_flags'] or build['compiler']!=baseline['compiler'] or build['bsp_pin']!=baseline['bsp_pin']:
                raise RuntimeError('NPU build toolchain/profile mismatch')
            if build['sources'].get('external/npu_inputs.h')!=profile['npu']['input_header_sha256']:
                raise RuntimeError('NPU input identity mismatch')
            for name,digest in profile['npu']['generated_sources'].items():
                if build['sources'].get('external/npu/'+name)!=digest: raise RuntimeError(f'NPU source mismatch {name}')
            if build['sources'].get('platform/ra8p1/semantic_sidecar.cpp')!=profile['semantic']['implementation_sha256'] or build['sources'].get('platform/ra8p1/semantic_sidecar.h')!=profile['semantic']['interface_sha256']:
                raise RuntimeError('semantic seam identity mismatch')
        elif mode:
            raise RuntimeError('scalar profile cannot run an NPU mode')
        qualification_loops=profile['schedule'].get('qualification_loops',profile['schedule'].get('qualification_loops_per_mode'))
        if args.qualification and args.loops!=qualification_loops:
            raise RuntimeError('qualification must use the exact frozen loop count')
        if args.qualification and args.mutation:
            raise RuntimeError('qualification cannot inject a comparator mutation')
        if args.mutation and mode: raise RuntimeError('comparator mutation is scalar-only')
        if args.failure_campaign and (args.mode!='scheduled' or args.qualification or args.loops!=1 or not profile['status'].startswith('FROZEN_NPU')):
            raise RuntimeError('failure campaign is one non-qualification scheduled-NPU loop')
        if not args.qualification and args.loops!=1:
            raise RuntimeError('non-qualification validation is exactly one loop')
        matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045b,0x5310)]
        if len(matches)!=1: raise RuntimeError('expected exactly one Titan application USB')
        owners=subprocess.run(['lsof','-t',matches[0].device],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip(): raise RuntimeError('USB has another owner')
        port=serial.Serial(matches[0].device,115200,timeout=.2,write_timeout=2,exclusive=True)
        receipt['usb']=dict(path=matches[0].device,location=matches[0].location,vid=matches[0].vid,pid=matches[0].pid)
        request=0
        def transact(op,payload=b'',expected=0,timeout=15):
            nonlocal request
            request+=1; data=packet(op,request,payload=payload)
            if port.write(data)!=len(data): raise RuntimeError('short USB write')
            port.flush(); header=read_exact(port,32,timeout)
            magic,status,rid,sequence,size,cycles,crc,header_crc=struct.unpack('<4s7I',header)
            if magic!=b'K1R1' or rid!=request or zlib.crc32(header[:28])!=header_crc or size>19968:
                raise RuntimeError('invalid response header')
            body=read_exact(port,size,timeout)
            if zlib.crc32(body)!=crc or status!=expected: raise RuntimeError(f'response status={status} expected={expected}')
            return body
        info=json.loads(transact(1))
        receipt['runtime']=info
        if (info['uid']!=UID or info['build']!=build['build_id'] or info['source']!=PIN or
            info['protocol']!=1 or info['contract']!='sr24000.hop180.bins80.xover40' or
            info['clock_hz']<=0 or info['cpu1_actcsr']&0x80 or bool(info['u55_opened'])!=bool(build.get('npu')) or not info['cpp_initialised']):
            raise RuntimeError('runtime identity/parked baseline mismatch')
        receipt['resources_pre_observer']=json.loads(transact(6))
        idle_status=json.loads(transact(8))
        if idle_status['active']:
            raise RuntimeError('schedule unexpectedly active before observer priming')
        receipt['observer_prime']=idle_status
        receipt['resources_before']=json.loads(transact(6))
        transact(7,struct.pack('<II',0,1),expected=3)
        transact(7,struct.pack('<II',args.loops,4),expected=3)
        flags=(3 if args.mutation else 1)|(mode<<4)|(0x40 if args.failure_campaign else 0)
        if transact(7,struct.pack('<II',args.loops,flags))!=b'STARTED': raise RuntimeError('schedule did not start')
        transact(7,struct.pack('<II',1,1),expected=3)
        deadline=time.monotonic()+args.loops*profile['fixture']['seconds_per_loop']+60
        status={}
        while time.monotonic()<deadline:
            status=json.loads(transact(8))
            print(f'SCALAR_SCHEDULE loops={status["loops_complete"]}/{args.loops} hop={status["hop"]} misses={status["deadline_misses"]} correctness={status["correctness_failures"]}',flush=True)
            if status['finished']: break
            time.sleep(5)
        if not status.get('finished'): raise RuntimeError('resident schedule timeout')
        receipt['result']=status
        receipt['resources_after']=json.loads(transact(6))
        acceptance=profile['acceptance']; expected=args.loops*profile['fixture']['hops_per_loop']
        expected_k1=0 if args.mode=='npu-alone' else expected
        if status['total']['count']!=expected_k1 or status['tempo']['count']+status['ordinary']['count']!=expected_k1:
            raise RuntimeError('measurement count mismatch')
        expected_renders=0 if args.mode=='npu-alone' else args.loops*profile['fixture']['renders_per_loop']
        if status['render']['count']!=expected_renders: raise RuntimeError('render measurement count mismatch')
        if build.get('stage_profile'):
            if args.mode=='npu-alone': raise RuntimeError('stage profile does not support npu-alone mode')
            validate_stage_profile(status,expected_k1,expected_renders)
        elif 'stage_profile' in status:
            raise RuntimeError('unexpected stage profile from ordinary build')
        if status['mode']!=mode: raise RuntimeError('target mode mismatch')
        if status['queue_capacity']!=profile['schedule']['queue_capacity'] or status['drops'] or status['coalesces']:
            raise RuntimeError('bounded queue contract failed')
        expected_correctness=1 if args.mutation else acceptance['correctness_failures']
        if status['correctness_failures']!=expected_correctness:
            raise RuntimeError(f'correctness_failures={status["correctness_failures"]} expected={expected_correctness}')
        if bool(status['crc_mutation_injected'])!=args.mutation:
            raise RuntimeError('target mutation witness mismatch')
        if mode:
            if not status['npu_ready'] or status['npu_invoke_failures']!=acceptance['npu_invoke_failures'] or status['npu_output_failures']!=acceptance['npu_output_failures']:
                raise RuntimeError('NPU activity/correctness failed')
            if args.mode in ('scheduled','npu-alone'):
                expected_npu=args.loops*profile['fixture']['seconds_per_loop']*1000000//profile['schedule']['scheduled_npu_period_us']
            else: expected_npu=expected*profile['schedule']['saturation_invocations_per_hop']
            if status['npu_invocations']!=expected_npu or status['npu_wall']['count']!=expected_npu:
                raise RuntimeError('NPU invocation count mismatch')
            if not all(status[field]>0 for field in ('npu_cycles','npu_active_cycles','mac_active_cycles')):
                raise RuntimeError('NPU PMU activity witness missing')
            if status['npu_wall_sum_us']<=0 or not 0<status['npu_duty_ppm']<=1000000:
                raise RuntimeError('NPU duty witness invalid')
        if args.failure_campaign:
            expected_semantic=profile['semantic']['failure_campaign']['expected']
            for field,value in expected_semantic.items():
                observed=status['semantic_'+field]
                if observed!=value: raise RuntimeError(f'semantic_{field}={observed} expected={value}')
        if not args.mutation and args.mode!='npu-alone':
            for field in ('deadline_misses','render_misses','release_guard_failures'):
                if status[field]!=0: raise RuntimeError(f'{field}={status[field]}')
        if status['backlog_highwater']>acceptance['backlog_highwater_max']: raise RuntimeError('backlog exceeded')
        before=receipt['resources_before']; after=receipt['resources_after']
        validate_resources(before,after,acceptance,'K1')
        for metrics in (receipt['resources_before'],after):
            observed=metrics['clock_check_cycles']*metrics['tick_hz']/metrics['clock_check_ticks']
            if abs(observed/info['clock_hz']-1)>0.02: raise RuntimeError('DWT/tick clock consistency failed')
        receipt['gate']='G4_TARGET_COMPARATOR_NEGATIVE' if args.mutation else ('K1-RA8P1-002-NPU-COEXIST' if mode else 'G4_SCALAR_SUBPROFILE')
        receipt['pass']=True
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        if port is not None: port.close()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
