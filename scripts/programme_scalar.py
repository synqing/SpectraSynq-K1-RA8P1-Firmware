#!/usr/bin/env python3
"""Retired serial-ROM programmer wrapper. SWD writes use programme_rfp_swd.py.

UID/build identity helpers remain for host tests. --execute is refused unless
K1_ALLOW_RETIRED_SERIAL_ROM=1 is set for an explicit offline mock of the old path.
"""
import argparse
import contextlib
import hashlib
import json
import os
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

def require_identity(info, expected_build):
    """Refuse a live symbol or programme result before the build matches. UID alone is not enough."""
    if info.get('uid')!=UID:
        raise RuntimeError('application uid mismatch: '+str(info.get('uid')))
    if info.get('build')!=expected_build:
        raise RuntimeError('application build mismatch: '+str(info.get('build')))

def verify_application_identity(port, event, expected_build):
    """Opcode 1 on the rebound application CDC. Does not use the control-surface server."""
    ss=Path(__file__).resolve().parents[1]/'tools'/'serial-studio'
    if str(ss) not in sys.path:
        sys.path.insert(0,str(ss))
    import cdc_lock
    from titan_broker import close_serial, open_serial
    from titan_transport import Transport
    handle=cdc_lock.acquire(port,'programme-identity')
    serial_port=open_serial(port, handle)
    try:
        transport=Transport(serial_port, allowed={1}, campaign=True)
        info=transport.info(timeout=3.0)['info']
        event('APP_IDENTITY',uid=info.get('uid'),build=info.get('build'),runtime_kind=info.get('runtime_kind'))
        require_identity(info, expected_build)
        return info
    finally:
        close_serial(serial_port, handle)
        cdc_lock.release(handle)

from datetime import datetime, timezone

LAB=Path('/Users/spectrasynq/SpectraSynq-EdgeAI-Lab')
UID='545433931bd25436593630352d068363'
sys.path.insert(0,str(LAB/'scripts'))
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'/'control-surface'))
import titan_ra8p1_boot as boot
import programme_handoff as handoff

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--wait-seconds',type=float,default=180)
    parser.add_argument('--wait-app-seconds',type=float,default=45,help='wait for application CDC and opcode-1 identity after verified write; never sends a reset')
    parser.add_argument('--execute',action='store_true')
    args=parser.parse_args()
    if args.execute and os.environ.get('K1_ALLOW_RETIRED_SERIAL_ROM') != '1':
        raise RuntimeError(
            'scripts/programme_scalar.py --execute is retired on this bench; '
            'use scripts/programme_rfp_swd.py (rfp-cli SWD 1 MHz -run range 02000000,02C9F01F). '
            'Host mocks may set K1_ALLOW_RETIRED_SERIAL_ROM=1.'
        )
    args.output.mkdir(parents=True,exist_ok=False)
    def event(name, **details):
        row=dict(event=name,at=datetime.now(timezone.utc).isoformat(),monotonic_ns=time.monotonic_ns(),**details)
        with (args.output/'events.jsonl').open('a') as stream:
            stream.write(json.dumps(row,sort_keys=True)+'\n'); stream.flush()
        print(json.dumps(row,sort_keys=True),flush=True)
    receipt=dict(label='ON-SILICON' if args.execute else 'PRE-SILICON',start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    claimed=False
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
        handoff.claim('programme_scalar')
        claimed=True
        receipt['handoff']=handoff.quiesce_server()
        handoff.wait_cdc_idle(8.0)
        receipt['cdc_idle']=True
        event('CDC_EXCLUSIVE',handoff=receipt['handoff'])
        event('WAITING_FOR_IDENTIFIED_ROM',build_id=build['build_id'],hex_sha256=image.sha256)
        path,usb=boot._select_port(None,args.wait_seconds)
        event('ROM_SEEN',port=path)
        port=boot._open(path)
        identity_ok=False
        try:
            with (args.output/'programming.log').open('w') as log,contextlib.redirect_stdout(log):
                device=boot.RA8P1Boot(port)
                signature,areas,security=boot._identify(device,usb)
                if signature.device_id.lower()!=UID: raise RuntimeError('wrong board UID: '+signature.device_id)
                receipt.update(uid=signature.device_id,usb=usb)
            event('ROM_IDENTIFIED_WRITING',uid=signature.device_id)
            with (args.output/'programming.log').open('a') as log,contextlib.redirect_stdout(log):
                boot._programme(device,areas,security,image)
            event('WRITE_VERIFIED',uid=signature.device_id,hex_sha256=image.sha256)
        finally:
            port.close()
        if args.wait_app_seconds > 0:
            app_port=wait_for_application(args.wait_app_seconds,event)
            receipt['application_port_seen']=app_port
            if not app_port:
                raise RuntimeError('application CDC did not reappear after verified write')
            info=verify_application_identity(app_port,event,build['build_id'])
            receipt['application_identity']=info
            receipt['application_identity_verified']=True
            identity_ok=True
        else:
            receipt['application_identity_verified']=False
            identity_ok=True
        receipt['pass']=True
        if identity_ok:
            receipt['server_resume']=handoff.resume_server()
            event('SERVER_RESUMED')
    except Exception as error:
        receipt['error']=str(error)
        event('PROGRAMME_FAILED',error=str(error))
        raise
    finally:
        if claimed:
            handoff.release()
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
