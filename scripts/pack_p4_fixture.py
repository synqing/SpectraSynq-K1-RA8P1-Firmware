#!/usr/bin/env python3
"""Pack the pinned generic P4 Float32 fixture into an exact target header."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct
from datetime import datetime,timezone
import numpy as np

REQUIRED={
    'pcm.npy':'0a01d1ffcb8ba09cf7e5af3dfca9cad3823490d8a59192201f733a5eca981885',
    'rfft_mag.npy':'e06be946434134d2bb67404e1367e90f1c3c5f86d4f211db4b2932392e8b8b60',
    'hann_rfft_mag.npy':'760bb7a8c54cde4b90d0837ccbeb0b43b3dd40b391bbbe5b985bcbf2f3a58051',
    'meta.json':'1d735a0741969bcc57dbbab6a281643acbb86b5d9500fbe9a031243aea85a448',
}

def sha(path:Path)->str: return hashlib.sha256(path.read_bytes()).hexdigest()

def words(values:np.ndarray):
    return np.asarray(values,dtype='<f4').view('<u4').reshape(-1)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='HOST PREPARATION',scope='generic P4 fixture; not K1',start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    try:
        for name,digest in REQUIRED.items():
            if sha(args.fixtures/name)!=digest: raise RuntimeError(f'P4 fixture changed: {name}')
        meta=json.loads((args.fixtures/'meta.json').read_text())
        if (meta['sr'],meta['n'],meta['seed'],meta['rfft_peak_bin'])!=(16000,2048,0,56):
            raise RuntimeError('P4 fixture contract changed')
        arrays={name:np.load(args.fixtures/(name+'.npy'),allow_pickle=False) for name in ('pcm','rfft_mag','hann_rfft_mag')}
        if arrays['pcm'].shape!=(2048,) or arrays['rfft_mag'].shape!=(1025,) or arrays['hann_rfft_mag'].shape!=(1025,):
            raise RuntimeError('P4 fixture shape changed')
        if any(value.dtype!=np.float32 or not np.isfinite(value).all() for value in arrays.values()):
            raise RuntimeError('P4 fixture dtype/non-finite failure')
        header=args.output/'p4_fixture.h'
        with header.open('x') as target:
            target.write('#pragma once\n#include <stdint.h>\n')
            for name,value in arrays.items():
                data=words(value)
                target.write(f'static const uint32_t p4_{name}_bits[{len(data)}]={{\n')
                for start in range(0,len(data),16):
                    target.write(','.join(f'0x{int(word):08x}U' for word in data[start:start+16])+',\n')
                target.write('};\n')
            bits=struct.unpack('<Q',struct.pack('<d',meta['goertzel_440_abs']))[0]
            target.write(f'static const uint64_t p4_goertzel_bits=0x{bits:016x}ULL;\n')
        receipt.update(fixtures=REQUIRED,header_sha256=sha(header),sample_rate=16000,samples=2048,
                       bins=1025,goertzel_bin=56,goertzel_hz=437.5,**{'pass':True})
        print(json.dumps({'header_sha256':receipt['header_sha256'],'pass':True},indent=2))
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
