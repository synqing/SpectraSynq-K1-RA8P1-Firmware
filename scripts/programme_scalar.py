#!/usr/bin/env python3
"""UID-gated wrapper around the already proven Titan ROM programmer; no security writes."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

def wait_for_application(seconds, event):
    from serial.tools import list_ports
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        matches=[p.device for p in list_ports.comports() if (p.vid,p.pid)==(0x045b,0x5310)]
        if len(matches)==1:
            event("APP_CDC_SEEN",port=matches[0],identity_verified=False)
            return matches[0]
        time.sleep(0.25)
    return None

from datetime import datetime, timezone

LAB=Path('/Users/spectrasynq/SpectraSynq-EdgeAI-Lab')
UID='545433931bd25436593630352d068363'
sys.path.insert(0,str(LAB/'scripts'))
import titan_ra8p1_boot as boot

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--wait-seconds',type=float,default=180)
    parser.add_argument('--wait-app-seconds',type=float,default=0,help='observe app reappearance after verified write; never sends a reset')
    parser.add_argument('--execute',action='store_true')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    def event(name, **details):
        row=dict(event=name,at=datetime.now(timezone.utc).isoformat(),monotonic_ns=time.monotonic_ns(),**details)
        with (args.output/'events.jsonl').open('a') as stream:
            stream.write(json.dumps(row,sort_keys=True)+'\n'); stream.flush()
        print(json.dumps(row,sort_keys=True),flush=True)
    receipt=dict(label='ON-SILICON' if args.execute else 'PRE-SILICON',start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    try:
        build=json.loads((args.build/'receipt.json').read_text())
        if not build['pass']: raise RuntimeError('build not accepted')
        image=boot.parse_ihex(args.build/'rtthread.hex')
        if image.sha256!=build['artifacts']['rtthread.hex']: raise RuntimeError('image hash mismatch')
        if image.sha256=='42bd1d6d8f2ece9882ec3cfbd64778535cdd1e33aa10b04c024549f3bc2ee75e':
            raise RuntimeError('known-failed GPT candidate is retired; prepare a discriminating new image')
        if not 0x02000000<=image.start<=image.end<=0x020fffff: raise RuntimeError('image outside code area')
        restore=Path('/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex')
        if hashlib.sha256(restore.read_bytes()).hexdigest()!='a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a':
            raise RuntimeError('known-good restore image changed')
        receipt.update(build_id=build['build_id'],image_sha256=image.sha256,image_start=hex(image.start),image_end=hex(image.end),restore=str(restore),programmer_sha256=hashlib.sha256(Path(boot.__file__).read_bytes()).hexdigest())
        if not args.execute:
            receipt.update(dry_run=True,**{'pass':True}); return
        event('WAITING_FOR_IDENTIFIED_ROM',build_id=build['build_id'],hex_sha256=image.sha256)
        path,usb=boot._select_port(None,args.wait_seconds)
        event('ROM_SEEN',port=path)
        port=boot._open(path)
        try:
            with (args.output/'programming.log').open('w') as log,contextlib.redirect_stdout(log):
                device=boot.RA8P1Boot(port)
                signature,areas,security=boot._identify(device,usb)
                if signature.device_id.lower()!=UID: raise RuntimeError('wrong board UID: '+signature.device_id)
                receipt.update(uid=signature.device_id,usb=usb)
            event('ROM_IDENTIFIED_WRITING',uid=signature.device_id)
            with (args.output/'programming.log').open('a') as log,contextlib.redirect_stdout(log):
                boot._programme(device,areas,security,image)
            receipt['pass']=True
            # The chat operator bridge owns finger instructions. Logs contain
            # evidence events only, preventing a second, delayed reset prompt.
            event('WRITE_VERIFIED',uid=signature.device_id,hex_sha256=image.sha256)
        finally:
            port.close()
        if args.wait_app_seconds > 0:
            receipt['application_port_seen']=wait_for_application(args.wait_app_seconds,event)
            receipt['application_identity_verified']=False
    except Exception as error:
        receipt['error']=str(error)
        event('PROGRAMME_FAILED',error=str(error))
        raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
