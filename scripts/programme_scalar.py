#!/usr/bin/env python3
"""UID-gated wrapper around the already proven Titan ROM programmer; no security writes."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import subprocess
import sys
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
    parser.add_argument('--execute',action='store_true')
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='ON-SILICON' if args.execute else 'PRE-SILICON',start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    try:
        build=json.loads((args.build/'receipt.json').read_text())
        if not build['pass']: raise RuntimeError('build not accepted')
        image=boot.parse_ihex(args.build/'rtthread.hex')
        if image.sha256!=build['artifacts']['rtthread.hex']: raise RuntimeError('image hash mismatch')
        if not 0x02000000<=image.start<=image.end<=0x020fffff: raise RuntimeError('image outside code area')
        restore=Path('/Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/p3-2026-09-09/build-staged-v2/rtthread.hex')
        if hashlib.sha256(restore.read_bytes()).hexdigest()!='a167833b7f35f2efa8ba296c772ab59c477bda2f2621faabe5510500aab5929a':
            raise RuntimeError('known-good restore image changed')
        receipt.update(build_id=build['build_id'],image_sha256=image.sha256,image_start=hex(image.start),image_end=hex(image.end),restore=str(restore),programmer_sha256=hashlib.sha256(Path(boot.__file__).read_bytes()).hexdigest())
        if not args.execute:
            receipt.update(dry_run=True,**{'pass':True}); return
        print('WAITING_FOR_IDENTIFIED_ROM: hold USER/BOOT, press and release RESET; keep USER/BOOT held.',flush=True)
        path,usb=boot._select_port(None,args.wait_seconds)
        owners=subprocess.run(['lsof','-t',path],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip(): raise RuntimeError('serial ownership is not free: '+owners.stdout+owners.stderr)
        port=boot._open(path)
        try:
            with (args.output/'programming.log').open('w') as log,contextlib.redirect_stdout(log):
                device=boot.RA8P1Boot(port)
                signature,areas,security=boot._identify(device,usb)
                if signature.device_id.lower()!=UID: raise RuntimeError('wrong board UID: '+signature.device_id)
                receipt.update(uid=signature.device_id,usb=usb)
                boot._programme(device,areas,security,image)
            receipt['pass']=True
            print('PROGRAMME_VERIFY_PASS: release USER/BOOT, then press and release RESET normally.',flush=True)
        finally:
            port.close()
    except Exception as error:
        receipt['error']=str(error)
        raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
