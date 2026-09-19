#!/usr/bin/env python3
"""Score actual DMA IRQs, hardware stop and latched frames without a host pixel stream."""
from __future__ import annotations
import argparse,hashlib,json,struct,subprocess,time,zlib
from datetime import datetime,timezone
from pathlib import Path
from run_led_smoke import UID,packet,read_exact
from verify_imports import PIN
FIELDS=json.loads(Path(__file__).with_name('gpt_snapshot_fields.json').read_text())
HEAD='version bytes attempts dma_irqs stop_irqs frames errors first_fault state owned clock_hz bits'.split()
def decode_snapshot(body):
    size=4*(len(HEAD)+5*len(FIELDS))
    if len(body)!=size: raise ValueError(f'diagnostic length {len(body)} != {size}')
    words=struct.unpack('<'+'I'*(size//4),body)
    out=dict(zip(HEAD,words))
    if out['version']!=1 or out['bytes']!=size: raise ValueError('diagnostic version/size mismatch')
    out['snapshots']=[dict(zip(FIELDS,words[len(HEAD)+i*len(FIELDS):len(HEAD)+(i+1)*len(FIELDS)])) for i in range(5)]
    return out

def score(first,second):
    for name in ['dma_irqs','stop_irqs','frames']:
        if second[name]<=first[name]: raise ValueError(f'{name} did not advance')
    if first['errors'] or second['errors'] or first['first_fault'] or second['first_fault']:
        raise ValueError('driver fault recorded')
    if second['frames']>min(second['dma_irqs'],second['stop_irqs']):
        raise ValueError('frame count exceeds actual completions')
    arm,start,dma,stop,fault=second['snapshots']
    if [s['tag'] for s in [arm,start,dma,stop]]!=[1,2,3,4] or fault['tag']:
        raise ValueError('missing first-transaction stages or fault snapshot present')
    if len({s['attempt'] for s in [arm,start,dma,stop]})!=1:
        raise ValueError('snapshots are from different submissions')
    if stop['gpt6_gtcr']&1 or stop['dmac_count'] or dma['dmac_count']:
        raise ValueError('hardware stop / exhausted DMA not observed')
    if not arm['elc_enable'] or arm['elc_a']!=0x1b7 or arm['elc_b']!=0x187 or arm['delink']!=0x1bd:
        raise ValueError('incorrect event routes')
    for s in [arm,start,dma,stop]:
        if not s['gpt_irq_owner'] or not s['dma_irq_owner']: raise ValueError('IRQ owner changed')
    if second['bits']!=128*24 or not second['clock_hz']:
        raise ValueError('unexpected WS2812 frame/clock')
    if (arm['gpt6_gtior'] & (1 << 4)) == 0:
        raise ValueError('GTIOR initial HIGH (bit 4) not set')
    return {'dma_irqs':second['dma_irqs']-first['dma_irqs'],
            'hardware_stops':second['stop_irqs']-first['stop_irqs'],
            'latched_frames':second['frames']-first['frames']}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--pause-s',type=float,default=5.0)
    a=p.parse_args()
    if not 2<=a.pause_s<=60:p.error('--pause-s must be 2..60')
    a.output.mkdir(parents=True,exist_ok=False)
    receipt={'pass':False,'start':datetime.now(timezone.utc).isoformat(),
             'operation':'TITAN_GPT_DMA_AUTONOMOUS','photons':'NOT_CLAIMED',
             'waveform':'WAVEFORM_NOT_CAPTURED','g4':'NOT_RUN_THIS_IMAGE',
             'host_generated_pixel_frames':0,'bitbang_fallback':False,'pause_s':a.pause_s}
    port=None
    try:
        b=json.loads((a.build/'receipt.json').read_text())
        if not b.get('pass') or not b.get('palette_gpt_dma') or not b.get('palette_autostart'):
            raise ValueError('requires accepted autonomous GPT image')
        sha=hashlib.sha256((a.build/'rtthread.hex').read_bytes()).hexdigest()
        if sha!=b['artifacts']['rtthread.hex']:raise ValueError('image hash changed')
        receipt.update(build_id=b['build_id'],hex_sha256=sha,source_pin=b['source_pin'])
        import serial
        from serial.tools import list_ports
        matches=[p for p in list_ports.comports() if (p.vid,p.pid)==(0x045b,0x5310)]
        if len(matches)!=1:raise ValueError('expected one Titan app CDC')
        device=matches[0].device
        owners=subprocess.run(['lsof','-t',device,device.replace('/cu.','/tty.')],capture_output=True,text=True)
        if owners.returncode not in (0,1) or owners.stdout.strip():raise ValueError('CDC owned; no commands sent')
        port=serial.Serial(device,115200,timeout=.5,write_timeout=2,exclusive=True)
        req=0
        def transact(op):
            nonlocal req
            req+=1;out=packet(op,req,payload=b'')
            if port.write(out)!=len(out):raise ValueError('short USB write')
            port.flush();head=read_exact(port,32,15)
            magic,status,rid,seq,size,cycles,crc,hcrc=struct.unpack('<4s7I',head)
            if magic!=b'K1R1' or rid!=req or zlib.crc32(head[:28])!=hcrc or size>19968:
                raise ValueError('invalid response envelope')
            body=read_exact(port,size,15)
            if zlib.crc32(body)!=crc or status:raise ValueError(f'opcode {op} failed: {status}')
            return body
        info=json.loads(transact(1));receipt['runtime']=info
        if info.get('uid')!=UID or info.get('build')!=b['build_id'] or info.get('source')!=PIN:
            raise ValueError('UID/build/source mismatch')
        # Collect both diagnostics even on engine failure; do not quit on backend name.
        first=decode_snapshot(transact(22));receipt['first']=first
        receipt['palette_first']=json.loads(transact(17))
        time.sleep(a.pause_s) # No commands or frames sent during observation.
        second=decode_snapshot(transact(22));receipt['second']=second
        receipt['palette_second']=json.loads(transact(17))
        again=json.loads(transact(1));receipt['runtime_end']=again
        if any(again.get(k)!=info.get(k) for k in ['uid','build','source']):raise ValueError('identity changed during run')
        receipt['delta']=score(first,second)
        receipt['pass']=True
        print(json.dumps({'pass':True,'build':b['build_id'],'delta':receipt['delta'],'g4':receipt['g4'],'waveform':receipt['waveform']},indent=2))
    except Exception as e:
        receipt['error']=str(e);raise
    finally:
        if port is not None:port.close();receipt['cdc_released']=True
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (a.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
if __name__=='__main__':main()
