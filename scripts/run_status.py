#!/usr/bin/env python3
"""Query/identify/ack/wait the Titan onboard status LED over exclusive CDC."""
import argparse, json, struct, sys, time, zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from serial import Serial
from serial.tools import list_ports
from run_led_smoke import packet, read_exact, UID

def transact(port, request, op, payload=b'', expected=0):
    request[0]+=1
    data=packet(op, request[0], payload=payload)
    assert port.write(data)==len(data); port.flush()
    header=read_exact(port,32,15)
    magic,status,rid,sequence,size,cycles,crc,header_crc=struct.unpack('<4s7I',header)
    body=read_exact(port,size,15)
    if zlib.crc32(body)!=crc: raise RuntimeError('crc')
    if status!=expected: raise RuntimeError(f'status={status}')
    return body

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('action',choices=('status','identify','ack','ack-recovery','wait','wait-clear'))
    parser.add_argument('--run-id',type=int,default=0)
    parser.add_argument('--reason',type=int,default=1)
    args=parser.parse_args()
    matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045B,0x5310)]
    if len(matches)!=1: raise SystemExit('expected exactly one Titan application USB')
    port=Serial(matches[0].device,115200,timeout=0.5,write_timeout=2,exclusive=True)
    request=[0]
    try:
        info=json.loads(transact(port,request,1))
        if info.get('uid')!=UID: raise SystemExit('wrong UID')
        if args.action=='status':
            print(transact(port,request,20).decode())
        elif args.action=='identify':
            print(transact(port,request,20,bytes([1])).decode())
        elif args.action=='ack':
            print(transact(port,request,20,bytes([2])+struct.pack('<Q',args.run_id)).decode())
        elif args.action=='ack-recovery':
            print(transact(port,request,20,bytes([3])).decode())
        elif args.action=='wait':
            print(transact(port,request,20,bytes([4])+struct.pack('<I',args.reason)).decode())
        elif args.action=='wait-clear':
            print(transact(port,request,20,bytes([5])).decode())
    finally:
        port.close()

if __name__=='__main__':
    main()
