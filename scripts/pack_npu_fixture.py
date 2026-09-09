#!/usr/bin/env python3
"""Prepare two hash-bound INT8 inputs for the K1 U55 load campaign."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from datetime import datetime,timezone
import numpy as np

INPUT_SCALE=np.float32(0.00784291885793209)
REQUIRED_GRAPH={
    'source_onnx_sha256':'5d0097e83fe269acb6ce92c5ab84cfe897561d3858328f2f0c610262c3801bd6',
    'same_compilation_tflite_sha256':'1fbdbb2878f5223697f70366036bf6260026f2e7ebbdb0dd0e0987d726f96fed',
    'github_run_id':'34216631562',
}

def sha(path:Path)->str: return hashlib.sha256(path.read_bytes()).hexdigest()

def quantise(values:np.ndarray)->np.ndarray:
    scaled=np.asarray(values,dtype=np.float32)/INPUT_SCALE
    rounded=np.copysign(np.floor(np.abs(scaled)+np.float32(.5)),scaled)
    return np.clip(rounded,-128,127).astype(np.int8).reshape(-1)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); args.output.mkdir(parents=True,exist_ok=False)
    receipt=dict(label='HOST PREPARATION',start=datetime.now(timezone.utc).isoformat(),**{'pass':False})
    try:
        manifest_path=args.bundle/'manifest.json'; manifest=json.loads(manifest_path.read_text())
        if manifest.get('schema')!='spectrasynq.titan-p2-bundle.v1' or not isinstance(manifest.get('graph'),dict):
            raise RuntimeError('invalid P2 bundle manifest')
        for key,value in REQUIRED_GRAPH.items():
            if manifest['graph'].get(key)!=value: raise RuntimeError(f'wrong graph {key}')
        if manifest.get('case_count')!=32 or manifest.get('acceptance',{}).get('raw_int8_max_lsb')!=0:
            raise RuntimeError('unaccepted P2 bundle')
        arrays=[]; cases=[]
        for case_id in (0,1):
            case=manifest['cases'][case_id]
            if case.get('case_id')!=case_id: raise RuntimeError('case identity mismatch')
            path=args.bundle/case['input']['path']
            if sha(path)!=case['input']['sha256']: raise RuntimeError(f'case {case_id} input changed')
            values=np.load(path,allow_pickle=False)
            if values.shape!=(1,1,64,100) or values.dtype!=np.float32 or not np.isfinite(values).all():
                raise RuntimeError(f'case {case_id} tensor contract mismatch')
            array=quantise(values)
            if hashlib.sha256(array.tobytes()).hexdigest()!=case['quantised_input_sha256']:
                raise RuntimeError(f'case {case_id} quantisation mismatch')
            arrays.append(array); cases.append(dict(case_id=case_id,input_sha256=case['input']['sha256'],
                quantised_sha256=case['quantised_input_sha256'],expected_raw_int8=case['raw_output']))
        header=args.output/'npu_inputs.h'
        with header.open('x') as target:
            target.write('#pragma once\n#include <stdint.h>\n#define K1_NPU_INPUT_COUNT 2U\n#define K1_NPU_INPUT_BYTES 6400U\n')
            target.write('static const int8_t k1_npu_inputs[K1_NPU_INPUT_COUNT][K1_NPU_INPUT_BYTES] __attribute__((aligned(16))) = {\n')
            for array in arrays:
                target.write('{\n')
                for start in range(0,len(array),64): target.write(','.join(map(str,array[start:start+64]))+',\n')
                target.write('},\n')
            target.write('};\nstatic const int8_t k1_npu_expected[K1_NPU_INPUT_COUNT][3] = {')
            target.write(','.join('{'+','.join(map(str,case['expected_raw_int8']))+'}' for case in cases))
            target.write('};\n')
        receipt.update(bundle_manifest_sha256=sha(manifest_path),graph=manifest['graph'],cases=cases,
                       header_sha256=sha(header),input_count=2,input_bytes=6400,
                       boundary='Smoke graph load fixture only; constant outputs are not musical discrimination',**{'pass':True})
    except Exception as error:
        receipt['error']=str(error); raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
