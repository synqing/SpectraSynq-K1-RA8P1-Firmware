#!/usr/bin/env python3
"""Lossless resident G4 input preparation; this does not run or qualify a schedule."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib
from fixture_wire import read_schema,decode
from run_product_host import compare,sha


def pack(pcm):
    if not pcm or len(pcm)%360: raise ValueError('nonempty complete 180-sample hops required')
    dictionary=[]; lookup={}; indices=[]
    for start in range(0,len(pcm),360):
        hop=pcm[start:start+360]
        if hop not in lookup:
            lookup[hop]=len(dictionary); dictionary.append(hop)
        indices.append(lookup[hop])
    if len(dictionary)>65535: raise ValueError('dictionary exceeds uint16 index range')
    return dictionary,indices


def unpack(dictionary,indices):
    if not dictionary or not indices or any(len(h)!=360 for h in dictionary): raise ValueError('invalid dictionary')
    if any(i<0 or i>=len(dictionary) for i in indices): raise ValueError('invalid dictionary index')
    return b''.join(dictionary[i] for i in indices)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--corpus',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='HOST PREPARATION',schedule_qualified=False,**{'pass':False})
    try:
        corpus=json.loads((args.corpus/'receipt.json').read_text())
        if not corpus['pass'] or corpus.get('platform_math',{}).get('profile')!='arm_newlib_4_4_fma':
            raise ValueError('explicit accepted HOST platform profile required')
        fixture=corpus['fixtures'][0]; pcm=Path(fixture['path']).read_bytes()
        if hashlib.sha256(pcm).hexdigest()!=fixture['sha256']: raise ValueError('PCM changed')
        if len(pcm)!=24000*45*2: raise ValueError('expected named 45-second production controls')
        dictionary,indices=pack(pcm)
        if unpack(dictionary,indices)!=pcm: raise ValueError('lossless reconstruction failed')
        schema_file=args.corpus/'trace-schema.txt'
        if sha(schema_file)!=corpus['schema_sha256']: raise ValueError('schema changed')
        schema=read_schema(schema_file.read_text())
        reference=args.corpus/'controls-reference.trace'
        frozen=next(case for case in corpus['cases'] if case['input']==fixture['path'])
        if sha(reference)!=frozen['traces']['reference']['sha256']: raise ValueError('reference changed')
        binary=args.output/'donor-binary.values'
        with binary.open('wb') as output:
            subprocess.run([str(args.corpus/'reference'),'--binary'],input=pcm,stdout=output,check=True,timeout=240)
        crcs=[]; lengths=[]
        recovered=args.output/'donor-roundtrip.trace'
        with binary.open('rb') as source,recovered.open('wb') as output:
            while header:=source.read(4):
                if len(header)!=4: raise ValueError('truncated donor binary header')
                size=struct.unpack('<I',header)[0]
                if size>16384: raise ValueError('oversized donor trace')
                body=source.read(size)
                output.write(decode(body,schema)+b'END\n')
                crcs.append(zlib.crc32(body)); lengths.append(len(body))
        receipt['reference_roundtrip']=compare(reference,recovered)
        if len(crcs)!=len(indices): raise ValueError('missing donor frames')
        # Format only the already validated lossless bytes into a build input.
        header=args.output/'resident_controls.h'
        with header.open('x') as target:
            target.write('#pragma once\n#include <stdint.h>\n')
            target.write(f'#define K1_RESIDENT_HOPS {len(indices)}U\n')
            target.write(f'static const int16_t k1_resident_pcm[{len(dictionary)}][180] __attribute__((aligned(4))) = {{\n')
            for hop in dictionary: target.write('{'+','.join(map(str,struct.unpack('<180h',hop)))+'},\n')
            target.write('};\n')
            for typ,name,values in [('uint16_t','index',indices),('uint32_t','crc',crcs),('uint16_t','length',lengths)]:
                target.write(f'static const {typ} k1_resident_{name}[K1_RESIDENT_HOPS] = {{\n')
                for start in range(0,len(values),32): target.write(','.join(map(str,values[start:start+32]))+',\n')
                target.write('};\n')
        receipt.update(hops=len(indices),unique_hops=len(dictionary),pcm_bytes=len(pcm),
                       resident_bytes=len(dictionary)*360+len(indices)*8,pcm_sha256=fixture['sha256'],
                       corpus_receipt_sha256=sha(args.corpus/'receipt.json'),header_sha256=sha(header),
                       donor_binary_sha256=sha(binary),source_sha256=sha(Path(__file__)),
                       boundary='No audible playback, no board writes, no change to PCM or expected AP/VP fields',**{'pass':True})
        print(json.dumps({k:v for k,v in receipt.items() if k not in ('reference_roundtrip',)},indent=2))
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
