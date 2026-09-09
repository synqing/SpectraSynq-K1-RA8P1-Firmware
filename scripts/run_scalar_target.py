#!/usr/bin/env python3
"""Identity-gated, single-owner M85 fixture replay. Not a realtime qualification."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib
from datetime import datetime, timezone
from run_product_host import compare
from verify_imports import PIN
UID='545433931bd25436593630352d068363'

def packet(op,request,sequence=0,payload=b'',length=None,bad_crc=False):
    header=struct.pack('<4s6I',b'K1S1',op,request,sequence,len(payload) if length is None else length,zlib.crc32(payload)^int(bad_crc),1)
    return header+struct.pack('<I',zlib.crc32(header))+payload

def read_exact(port,size,timeout=15):
    data=bytearray(); deadline=time.monotonic()+timeout
    while len(data)<size and time.monotonic()<deadline: data.extend(port.read(size-len(data)))
    if len(data)!=size: raise RuntimeError(f'USB timeout {len(data)}/{size}')
    return bytes(data)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--corpus',type=Path)
    parser.add_argument('--stage',choices=['info','smoke','corpus'],default='smoke')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='ON-SILICON',stage=args.stage,start=datetime.now(timezone.utc).isoformat(),
                 boundary='USB-paced fixture execution; no realtime/physical-output claim',float_acceptance='strict exact diagnostic; no relaxed target budget authorised',**{'pass':False})
    port=None
    try:
        import serial
        from serial.tools import list_ports
        build=json.loads((args.build/'receipt.json').read_text())
        if not build['pass']: raise RuntimeError('build receipt failed')
        if hashlib.sha256((args.build/'rtthread.hex').read_bytes()).hexdigest()!=build['artifacts']['rtthread.hex']: raise RuntimeError('image changed')
        matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045b,0x5310)]
        if len(matches)!=1: raise RuntimeError('expected exactly one Titan application USB')
        device=matches[0].device
        owners=subprocess.run(['lsof','-t',device],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip(): raise RuntimeError('USB has another owner')
        port=serial.Serial(device,115200,timeout=.2,write_timeout=2,exclusive=True)
        receipt['usb']=dict(path=device,location=matches[0].location,vid=matches[0].vid,pid=matches[0].pid)
        request=0
        def transact(op,sequence=0,payload=b'',expected_status=0,**kwargs):
            nonlocal request
            request+=1
            data=packet(op,request,sequence,payload,**kwargs)
            if port.write(data)!=len(data): raise RuntimeError('short USB write')
            port.flush()
            header=read_exact(port,32)
            magic,status,rid,seq,size,cycles,crc,header_crc=struct.unpack('<4s7I',header)
            if magic!=b'K1R1' or zlib.crc32(header[:28])!=header_crc or rid!=request or size>19968:
                raise RuntimeError('response identity/header/length invalid')
            body=read_exact(port,size)
            if zlib.crc32(body)!=crc or status!=expected_status: raise RuntimeError(f'response rejected: status={status} expected={expected_status} crc={crc}')
            return body,seq,cycles
        body,_,_=transact(1)
        info=json.loads(body)
        receipt['runtime']=info
        if info['uid']!=UID or info['build']!=build['build_id'] or info['source']!=PIN:
            raise RuntimeError('runtime UID/build/source mismatch')
        if info['protocol']!=1 or info['contract']!='sr24000.hop180.bins80.xover40' or info['clock_hz']<=0:
            raise RuntimeError('runtime protocol/configuration/clock invalid')
        if info['cpu1_actcsr']&0x80 or info['u55_opened'] or not info['cpp_initialised']:
            raise RuntimeError('scalar park/startup contract failed')
        if args.stage=='info': receipt['pass']=True; return
        body,_,cycles=transact(4)
        receipt['time_probe']=dict(result=json.loads(body),cycles=cycles)
        if receipt['time_probe']['result']['time_probe_failure']: raise RuntimeError('target time assertions failed')
        transact(3,payload=struct.pack('<Q',1))
        receipt['negatives']=[]
        for name,kwargs in [('bad_sequence',dict(op=2,sequence=2,payload=bytes(360),expected_status=6)),
                            ('bad_payload_crc',dict(op=2,sequence=1,payload=bytes(360),bad_crc=True,expected_status=4)),
                            ('oversized',dict(op=2,sequence=1,length=361,expected_status=2)),
                            ('partial_timeout',dict(op=2,sequence=1,length=360,expected_status=9)),
                            ('bad_opcode',dict(op=99,expected_status=3))]:
            _,seq,_=transact(**kwargs)
            if seq: raise RuntimeError('rejected command advanced state')
            receipt['negatives'].append(dict(name=name,rejected=True))
        if args.corpus is None: raise RuntimeError('--corpus required for frozen independent traces')
        corpus=json.loads((args.corpus/'receipt.json').read_text())
        if not corpus['pass']: raise RuntimeError('HOST corpus not accepted')
        # Exact compiled adapter binding. A changed harness requires new goldens.
        if corpus['adapter_sha256']!=build['sources']['tests/target/trajectory.h']: raise RuntimeError('fixture adapter mismatch')
        receipt['cases']=[]
        for fixture in corpus['fixtures'][:1] if args.stage=='smoke' else corpus['fixtures']:
            pcm=Path(fixture['path'])
            if hashlib.sha256(pcm.read_bytes()).hexdigest()!=fixture['sha256']: raise RuntimeError('PCM changed')
            transact(3,payload=struct.pack('<Q',1))
            trace=args.output/(pcm.stem+'-target.trace')
            samples=[]
            with pcm.open('rb') as source,trace.open('wb') as output:
                sequence=0
                while True:
                    hop=source.read(360)
                    if not hop: break
                    if len(hop)!=360: raise RuntimeError('partial PCM hop')
                    sequence+=1
                    body,seq,cycles=transact(2,sequence,hop)
                    if seq!=sequence: raise RuntimeError('output sequence mismatch')
                    output.write(body+b'END\n'); samples.append(cycles)
                    if sequence%500==0: print(f'TARGET_REPLAY {pcm.stem} hops={sequence}',flush=True)
                    if args.stage=='smoke' and sequence==2: break
            reference=args.corpus/(pcm.stem+'-reference.trace')
            frozen=next(case for case in corpus['cases'] if case['input']==str(pcm))
            if hashlib.sha256(reference.read_bytes()).hexdigest()!=frozen['traces']['reference']['sha256']:
                raise RuntimeError('independent reference trace changed')
            if args.stage=='smoke':
                limited=args.output/'reference-first-two.trace'
                with reference.open() as source,limited.open('w') as output:
                    ends=0
                    for line in source:
                        output.write(line); ends+=line=='END\n'
                        if ends==2: break
                reference=limited
            result=compare(reference,trace)
            result.update(fixture=fixture,trace_sha256=hashlib.sha256(trace.read_bytes()).hexdigest(),cycles_max=max(samples),cycles_min=min(samples))
            (args.output/(pcm.stem+'-cycles.json')).write_text(json.dumps(samples)+'\n')
            receipt['cases'].append(result)
        receipt['pass']=True
    except Exception as error:
        receipt['error']=str(error)
        raise
    finally:
        if port is not None: port.close()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
