#!/usr/bin/env python3
"""Run the frozen generic P4/E1 schedule on one identity-gated Titan."""
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

MODES={'dsp-alone':1,'npu-alone':2,'concurrent':3,'saturation':4}
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--profile',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mode',choices=MODES,required=True)
    parser.add_argument('--qualification',action='store_true')
    parser.add_argument('--mutation',action='store_true')
    parser.add_argument('--failure-campaign',action='store_true')
    parser.add_argument('--observer',choices=('on','off'),default='on')
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='ON-SILICON',gate='GENERIC_EDGEAI_P4_E1',mode=args.mode,
                 qualification=args.qualification,mutation=args.mutation,
                 failure_campaign=args.failure_campaign,observer=args.observer,
                 start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    port=None
    try:
        import serial
        from serial.tools import list_ports
        profile=json.loads(args.profile.read_text())
        build=json.loads((args.build/'receipt.json').read_text())
        fixture=json.loads((args.fixture/'receipt.json').read_text())
        receipt['identities']={'profile_sha256':sha(args.profile),'build_receipt_sha256':sha(args.build/'receipt.json'),
                               'fixture_receipt_sha256':sha(args.fixture/'receipt.json')}
        if profile['status']!='FROZEN_BEFORE_ON_SILICON_CAPTURE' or profile['profile_id']!='generic-p4-seed0-v1':
            raise RuntimeError('wrong P4 profile')
        if not build.get('pass') or not build.get('p4') or not build.get('npu') or build['source_pin']!=PIN:
            raise RuntimeError('build is not an accepted P4+NPU target image')
        implementation=profile['implementation']
        if build['flags']!=implementation['compiler_flags'] or build['compiler']!=implementation['compiler'] or build['bsp_pin']!=implementation['bsp_pin']:
            raise RuntimeError('P4 build toolchain/profile mismatch')
        expected_sources={'external/p4/kernels.c':implementation['kernels_c_sha256'],
                          'external/p4/kernels.h':implementation['kernels_h_sha256'],
                          'platform/ra8p1/p4_runtime.cpp':implementation['runtime_cpp_sha256'],
                          'platform/ra8p1/p4_runtime.h':implementation['runtime_h_sha256'],
                          'external/p4_fixture.h':profile['fixture']['packed_header_sha256'],
                          'external/npu_inputs.h':profile['npu']['input_header_sha256']}
        for name,digest in expected_sources.items():
            if build['sources'].get(name)!=digest: raise RuntimeError(f'P4 source identity mismatch: {name}')
        for name,digest in profile['npu']['generated_sources'].items():
            if build['sources'].get('external/npu/'+name)!=digest: raise RuntimeError(f'NPU source identity mismatch: {name}')
        if not fixture.get('pass') or fixture['header_sha256']!=profile['fixture']['packed_header_sha256']:
            raise RuntimeError('packed P4 fixture identity mismatch')
        source_fixtures=fixture.get('fixtures',{})
        if set(source_fixtures)!= {'pcm.npy','rfft_mag.npy','hann_rfft_mag.npy','meta.json'}:
            raise RuntimeError('P4 source fixture set mismatch')
        for name,digest in source_fixtures.items():
            profile_name={'rfft_mag.npy':'rfft_sha256','hann_rfft_mag.npy':'hann_rfft_sha256'}.get(name,name.replace('.npy','_sha256').replace('.json','_sha256'))
            if profile['fixture'].get(profile_name)!=digest: raise RuntimeError(f'P4 source fixture mismatch: {name}')
        releases=profile['schedule']['qualification_releases'] if args.qualification else profile['schedule']['preflight_releases']
        if args.qualification and args.mode not in profile['schedule']['required_qualification_modes']:
            raise RuntimeError('qualification is only defined for required concurrent modes')
        if args.qualification and args.observer!=profile['schedule']['qualification_observer']:
            raise RuntimeError('qualification must use the frozen observer mode')
        if args.mutation and (args.qualification or args.mode!='dsp-alone'):
            raise RuntimeError('numeric mutation is one DSP-alone preflight')
        if args.failure_campaign and (not args.qualification or args.mode!='concurrent'):
            raise RuntimeError('failure campaign belongs to concurrent qualification')
        matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045b,0x5310)]
        if len(matches)!=1: raise RuntimeError('expected exactly one Titan application USB')
        owners=subprocess.run(['lsof','-t',matches[0].device],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip(): raise RuntimeError('USB has another owner')
        port=serial.Serial(matches[0].device,115200,timeout=.2,write_timeout=2,exclusive=True)
        receipt['usb']={'path':matches[0].device,'location':matches[0].location,'vid':matches[0].vid,'pid':matches[0].pid}
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
        info=json.loads(transact(1)); receipt['runtime']=info
        if (info['uid']!=UID or info['build']!=build['build_id'] or info['source']!=PIN or
            info['contract']!='sr24000.hop180.bins80.xover40' or info['clock_hz']<=0 or
            info['cpu1_actcsr']&0x80 or not info['u55_opened'] or not info['cpp_initialised']):
            raise RuntimeError('runtime identity/core state mismatch')
        receipt['resources_before']=json.loads(transact(6))
        transact(9,struct.pack('<III',0,MODES[args.mode],0),expected=3)
        transact(9,struct.pack('<III',releases,0,0),expected=3)
        flags=(2 if args.mutation else 0)|(4 if args.failure_campaign else 0)
        if transact(9,struct.pack('<III',releases,MODES[args.mode],flags))!=b'STARTED': raise RuntimeError('P4 schedule did not start')
        transact(9,struct.pack('<III',1,MODES[args.mode],0),expected=3)
        duration=releases*profile['schedule']['release_period_us']/1000000
        deadline=time.monotonic()+duration+120
        status={}; last_report=0.0; observations=[]
        while time.monotonic()<deadline:
            if args.observer=='on':
                status=json.loads(transact(10))
                observations.append({'wall_s':round(time.monotonic()-(deadline-duration-120),3),
                                     'dsp_count':status['dsp_count'],'npu_count':status['npu_count'],
                                     'deadline_misses':status['deadline_misses'],'numeric_failures':status['numeric_failures']})
                print(f'P4 mode={args.mode} dsp={status["dsp_count"]}/{releases} npu={status["npu_count"]} misses={status["deadline_misses"]} numeric={status["numeric_failures"]}',flush=True)
                if status['finished']: break
                time.sleep(5)
            else:
                elapsed=time.monotonic()-(deadline-duration-120)
                if elapsed>=duration+2: status=json.loads(transact(10)); break
                if elapsed-last_report>=30: print(f'P4_OBSERVER_OFF elapsed={elapsed:.1f}/{duration:.1f}s',flush=True); last_report=elapsed
                time.sleep(min(30,max(0.1,duration+2-elapsed)))
        if not status.get('finished'): raise RuntimeError('P4 schedule timeout')
        receipt['observations']=observations
        receipt['result']=status; receipt['resources_after']=json.loads(transact(6))
        expected_dsp=0 if args.mode=='npu-alone' else releases
        if status['dsp_count']!=expected_dsp: raise RuntimeError('P4 DSP count mismatch')
        expected_numeric=1 if args.mutation else profile['acceptance']['numeric_failures']
        if status['numeric_failures']!=expected_numeric: raise RuntimeError('P4 numerical comparison failed')
        if not args.mutation and (status['maximum_ulp']>profile['numerical']['rfft_all_bins_max_ulp'] or status['maximum_goertzel_error']>profile['numerical']['goertzel_absolute_error_max']):
            raise RuntimeError('P4 numerical tolerance failed')
        if args.mutation and status['first_bad_bin']!=1024: raise RuntimeError('P4 mutation did not identify final bin')
        if args.mode=='dsp-alone' and status['npu_count']!=0: raise RuntimeError('NPU ran in DSP-alone mode')
        if args.mode in ('npu-alone','concurrent'):
            expected_npu=releases*profile['schedule']['release_period_us']//profile['schedule']['npu_period_us']
            if status['npu_count']!=expected_npu: raise RuntimeError('scheduled NPU count mismatch')
        if args.mode=='saturation' and status['npu_count']<=0: raise RuntimeError('NPU saturation activity missing')
        if args.mode!='dsp-alone' and not all(status[name]>0 for name in ('npu_cycles','npu_active_cycles','mac_active_cycles')):
            raise RuntimeError('NPU PMU activity missing')
        if args.mode!='dsp-alone' and (status['npu_wall_sum_us']<=0 or not 0<status['npu_duty_ppm']<=1000000):
            raise RuntimeError('NPU duty witness invalid')
        if args.mode=='dsp-alone' and (status['npu_wall_sum_us'] or status['npu_duty_ppm']):
            raise RuntimeError('NPU duty reported in DSP-alone mode')
        for name in ('npu_failures','npu_output_failures','deadline_misses','release_guard_failures'):
            if status[name]!=profile['acceptance'][name]: raise RuntimeError(f'{name}={status[name]}')
        if status['backlog_highwater']>profile['acceptance']['backlog_highwater_max']: raise RuntimeError('P4 backlog exceeded')
        if status['queue_capacity']!=profile['schedule']['queue_capacity'] or status['drops'] or status['coalesces']:
            raise RuntimeError('P4 bounded queue contract failed')
        if args.failure_campaign and (not status['semantic_valid'] or status['semantic_loss']!=1 or status['semantic_recoveries']!=9):
            raise RuntimeError('P4 semantic failure recovery failed')
        before=receipt['resources_before']; after=receipt['resources_after']
        validate_resources(before,after,profile['acceptance'],'P4')
        for metrics in (receipt['resources_before'],after):
            observed=metrics['clock_check_cycles']*metrics['tick_hz']/metrics['clock_check_ticks']
            if abs(observed/info['clock_hz']-1)>0.02: raise RuntimeError('DWT/tick clock consistency failed')
        receipt['pass']=True
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        if port is not None: port.close()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
