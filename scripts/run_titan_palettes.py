#!/usr/bin/env python3
"""Control Titan's native K1 palette runtime; never stream animation pixels."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib
from run_led_smoke import UID, packet, read_exact
from verify_imports import PIN

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path)
    action=parser.add_mutually_exclusive_group()
    action.add_argument('--list',action='store_true')
    action.add_argument('--status',action='store_true')
    action.add_argument('--stop',action='store_true')
    action.add_argument('--verify-all',action='store_true')
    parser.add_argument('--palette',default='0',help='canonical ID or exact palette name')
    parser.add_argument('--palette-b',default='1')
    parser.add_argument('--mode-a',type=int,default=0,help='0: native palette preview; other values: existing K1 mode')
    parser.add_argument('--mode-b',type=int,default=0)
    parser.add_argument('--brightness',type=int,default=24)
    parser.add_argument('--output-channel',type=int,choices=(0,1),default=0)
    parser.add_argument('--cycle',action='store_true',help='advance through all 44 palettes every four seconds on Titan')
    parser.add_argument('--no-emit',action='store_true',help='render on-device without physical GPIO emission')
    parser.add_argument('--transition-ms',type=int,default=0,help='0: cut; 1..10000: native smooth palette transition')
    effects={'ribbons':100,'aurora':101,'embers':102,'pulse':103}
    parser.add_argument('--effect-a',choices=effects)
    parser.add_argument('--effect-b',choices=effects)
    parser.add_argument('--inward',action='store_true',help='new centre effects travel from both edges towards the centre')
    parser.add_argument('--showcase',action='store_true',help='cycle four native centre effects every 12 seconds')
    parser.add_argument('--travel-ms',type=int,default=4000,help='500..30000 ms from centre to edge (or edge to centre)')
    args=parser.parse_args()
    if not 500<=args.travel_ms<=30000: parser.error('travel-ms must be 500..30000')
    if args.effect_a: args.mode_a=effects[args.effect_a]
    elif args.showcase: args.mode_a=100
    if args.effect_b: args.mode_b=effects[args.effect_b]
    elif args.showcase: args.mode_b=101
    if not 0<=args.transition_ms<=10000: parser.error('transition-ms must be 0..10000')
    if not 0<=args.brightness<=255: parser.error('brightness must be 0..255')
    if args.verify_all and args.output is None: parser.error('--verify-all requires --output')
    build=json.loads((args.build/'receipt.json').read_text())
    image=args.build/'rtthread.hex'
    if not build.get('pass') or not build.get('palette_runtime'):
        raise RuntimeError('build does not contain the accepted native palette runtime')
    if hashlib.sha256(image.read_bytes()).hexdigest()!=build['artifacts']['rtthread.hex']:
        raise RuntimeError('build image identity mismatch')
    if args.transition_ms and not build.get('palette_morph'):
        raise RuntimeError('this image does not support palette morphing')
    receipt={'pass':False,'start':datetime.now(timezone.utc).isoformat(),
             'operation':'K1_NATIVE_PALETTES','host_generated_pixel_frames':0,
             'photons':'NOT_CLAIMED','production_audio_coexistence':'NOT_TESTED'}
    if args.output: args.output.mkdir(parents=True,exist_ok=False)
    port=None
    try:
        import serial
        from serial.tools import list_ports
        matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045B,0x5310)]
        if len(matches)!=1: raise RuntimeError('expected one Titan application CDC')
        device=matches[0].device
        owners=subprocess.run(['lsof','-t',device,device.replace('/cu.','/tty.')],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip():
            raise RuntimeError('CDC has another owner; no commands sent')
        port=serial.Serial(device,115200,timeout=0.5,write_timeout=2,exclusive=True)
        request_id=0
        def transact(op,payload=b'',raw=False):
            nonlocal request_id
            request_id+=1
            outgoing=packet(op,request_id,payload=payload)
            if port.write(outgoing)!=len(outgoing): raise RuntimeError('short CDC write')
            port.flush()
            header=read_exact(port,32,15)
            magic,status,received_id,sequence,size,cycles,crc,hcrc=struct.unpack('<4s7I',header)
            if magic!=b'K1R1' or zlib.crc32(header[:28])!=hcrc or received_id!=request_id or size>19968:
                raise RuntimeError('invalid response envelope')
            body=read_exact(port,size,15)
            if zlib.crc32(body)!=crc: raise RuntimeError('response payload CRC mismatch')
            if status: raise RuntimeError(f'opcode {op} rejected: {status}')
            return body if raw else json.loads(body)
        info=transact(1)
        if info.get('uid')!=UID or info.get('source')!=PIN or info.get('build')!=build['build_id']:
            raise RuntimeError('target identity mismatch; no palette configuration sent')
        receipt['runtime']=info
        catalogue=transact(15)
        entries=catalogue['palettes']
        if catalogue.get('count')!=44 or [p['id'] for p in entries]!=list(range(44)):
            raise RuntimeError('complete K1 palette catalogue not present')
        receipt['catalogue']=catalogue
        def palette_id(text):
            if text.isdecimal() and 0<=int(text)<len(entries): return int(text)
            matches=[p['id'] for p in entries if p['name'].casefold()==text.casefold()]
            if len(matches)!=1: raise ValueError('unknown palette: '+text)
            return matches[0]
        a=palette_id(args.palette); b=palette_id(args.palette_b)
        def configure(a,b,flags,brightness=None,mode_a=None,mode_b=None,transition_ms=None):
            words=(1,a,b,args.mode_a if mode_a is None else mode_a,
                   args.mode_b if mode_b is None else mode_b,flags,
                   args.brightness if brightness is None else brightness,args.output_channel)
            duration=args.transition_ms if transition_ms is None else transition_ms
            if args.inward or args.showcase or args.travel_ms!=4000 or words[3]>=100 or words[4]>=100:
                words=(3,*words[1:],duration,args.travel_ms)
                return transact(16,struct.pack('<10I',*words))
            if duration:
                words=(2,*words[1:],duration)
                return transact(16,struct.pack('<9I',*words))
            return transact(16,struct.pack('<8I',*words))
        if args.list:
            for item in entries: print(f"{item['id']:2d}  {item['name']}")
        elif args.status:
            print(json.dumps(transact(17),indent=2))
        elif args.stop:
            configure(a,b,5,brightness=0,mode_a=0,mode_b=0)
            time.sleep(0.05)
            receipt['final']=configure(a,b,0,brightness=0,mode_a=0,mode_b=0)
            print('K1 palette output stopped; black frame submitted.')
        elif args.verify_all:
            receipt['palettes_checked']=[]
            before=transact(17)
            for item in entries:
                i=item['id']
                configure(i,43-i,1 if args.no_emit else 5,mode_a=0,mode_b=0,transition_ms=0)
                time.sleep(0.04)
                state=transact(17)
                if state['palette_a']!=i or state['palette_b']!=43-i:
                    raise RuntimeError('palette selection was not retained')
                if state['name_a']!=item['name'] or state['waiting_for_audio']:
                    raise RuntimeError('native palette preview not running')
                frame=transact(18,struct.pack('<I',args.output_channel),raw=True)
                if len(frame)!=480: raise RuntimeError('incomplete native 160-pixel frame')
                if not args.no_emit and (state['emitted']<=before['emitted'] or state['emit_errors']):
                    raise RuntimeError('native physical-output callback failed')
                receipt['palettes_checked'].append({'id':i,'name':item['name'],'state':state,
                                                    'native_frame_crc':zlib.crc32(frame)})
                before=state
            receipt['final']=configure(a,b,3 if args.no_emit else 7,mode_a=0,mode_b=0)
            receipt['all_44_selected']=True
            print('K1_NATIVE_PALETTES_PASS count=44; autonomous catalogue cycle remains running.')
        else:
            flags=1 | (2 if args.cycle else 0) | (0 if args.no_emit else 4) | (8 if args.inward else 0) | (16 if args.showcase else 0)
            receipt['final']=configure(a,b,flags)
            print(json.dumps(receipt['final'],indent=2))
        receipt['pass']=True
    except Exception as error:
        receipt['error']=str(error)
        raise
    finally:
        if port is not None: port.close()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        if args.output: (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
if __name__=='__main__': main()
