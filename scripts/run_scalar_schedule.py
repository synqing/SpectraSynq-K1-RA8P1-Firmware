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
from verify_imports import PIN

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--resident',type=Path,required=True)
    parser.add_argument('--profile',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--loops',type=int,default=1)
    parser.add_argument('--qualification',action='store_true')
    parser.add_argument('--mutation',action='store_true',help='prove the target comparator rejects one deliberately wrong expected CRC')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='ON-SILICON',gate='G4_SCALAR_SUBPROFILE',start=datetime.now(timezone.utc).isoformat(),
                 loops=args.loops,qualification=args.qualification,mutation=args.mutation,**{'pass':False})
    port=None
    try:
        import serial
        from serial.tools import list_ports
        profile=json.loads(args.profile.read_text()); build=json.loads((args.build/'receipt.json').read_text())
        resident=json.loads((args.resident/'receipt.json').read_text())
        receipt['identities']=dict(profile_sha256=sha(args.profile),build_receipt_sha256=sha(args.build/'receipt.json'),
                                  resident_receipt_sha256=sha(args.resident/'receipt.json'))
        if profile['status']!='FROZEN_SCALAR_SUBPROFILE_NOT_YET_QUALIFIED' or profile['source_pin']!=PIN:
            raise RuntimeError('wrong or unfrozen scalar profile')
        if not build['pass'] or build['source_pin']!=PIN or 'external/resident_controls.h' not in build['sources']:
            raise RuntimeError('build is not an accepted resident K1 image')
        if not resident['pass'] or resident['schedule_qualified']:
            raise RuntimeError('resident preparation receipt invalid')
        if build['sources']['external/resident_controls.h']!=profile['fixture']['resident_header_sha256'] or resident['header_sha256']!=profile['fixture']['resident_header_sha256']:
            raise RuntimeError('resident header identity mismatch')
        if resident['pcm_sha256']!=profile['fixture']['pcm_sha256'] or resident['hops']!=profile['fixture']['hops_per_loop']:
            raise RuntimeError('resident fixture identity mismatch')
        if args.qualification and args.loops!=profile['schedule']['qualification_loops']:
            raise RuntimeError('qualification must use the exact frozen loop count')
        if args.qualification and args.mutation:
            raise RuntimeError('qualification cannot inject a comparator mutation')
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
            info['clock_hz']<=0 or info['cpu1_actcsr']&0x80 or info['u55_opened'] or not info['cpp_initialised']):
            raise RuntimeError('runtime identity/parked baseline mismatch')
        receipt['resources_before']=json.loads(transact(6))
        transact(7,struct.pack('<II',0,1),expected=3)
        transact(7,struct.pack('<II',args.loops,4),expected=3)
        flags=3 if args.mutation else 1
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
        if status['total']['count']!=expected or status['tempo']['count']+status['ordinary']['count']!=expected:
            raise RuntimeError('measurement count mismatch')
        if status['render']['count']!=args.loops*profile['fixture']['renders_per_loop']:
            raise RuntimeError('render measurement count mismatch')
        if status['queue_capacity']!=profile['schedule']['queue_capacity'] or status['drops'] or status['coalesces']:
            raise RuntimeError('bounded queue contract failed')
        expected_correctness=1 if args.mutation else acceptance['correctness_failures']
        if status['correctness_failures']!=expected_correctness:
            raise RuntimeError(f'correctness_failures={status["correctness_failures"]} expected={expected_correctness}')
        if bool(status['crc_mutation_injected'])!=args.mutation:
            raise RuntimeError('target mutation witness mismatch')
        for field in ('deadline_misses','render_misses','release_guard_failures'):
            if status[field]!=0: raise RuntimeError(f'{field}={status[field]}')
        if status['backlog_highwater']>acceptance['backlog_highwater_max']: raise RuntimeError('backlog exceeded')
        after=receipt['resources_after']
        if after['stack_untouched_bytes']<acceptance['stack_untouched_min_bytes'] or after['heap_total']-after['heap_maximum']<acceptance['heap_free_at_maximum_min_bytes']:
            raise RuntimeError('resource reserve failed')
        for metrics in (receipt['resources_before'],after):
            observed=metrics['clock_check_cycles']*metrics['tick_hz']/metrics['clock_check_ticks']
            if abs(observed/info['clock_hz']-1)>0.02: raise RuntimeError('DWT/tick clock consistency failed')
        receipt['gate']='G4_TARGET_COMPARATOR_NEGATIVE' if args.mutation else 'G4_SCALAR_SUBPROFILE'
        receipt['pass']=True
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        if port is not None: port.close()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
