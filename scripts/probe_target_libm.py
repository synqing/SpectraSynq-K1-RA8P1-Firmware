#!/usr/bin/env python3
"""Isolate target newlib log/exp/log2 from native HOST libm; not target acceptance."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from build_scalar import TOOLCHAIN
from run_product_host import FLAGS, sha
from verify_imports import ROOT, REFERENCE, PIN


def elf_bytes(elf, address, size):
    data = elf.read_bytes()
    if data[:6] != b'\x7fELF\x01\x01': raise ValueError('expected little-endian ELF32')
    shoff = struct.unpack_from('<I', data, 32)[0]
    entsize, count = struct.unpack_from('<HH', data, 46)
    for i in range(count):
        section = struct.unpack_from('<10I', data, shoff+i*entsize)
        _, typ, _, start, offset, length, *_ = section
        if typ != 8 and start <= address and address+size <= start+length:
            return data[offset+address-start:offset+address-start+size]
    raise ValueError('symbol not in a file-backed section')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--corpus', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = dict(label='HOST DIAGNOSTIC', target_acceptance=False, source_pin=PIN)
    probe = ROOT/'tests/host/arm_logf_probe.cpp'
    receipt['probe_sha256'] = sha(probe)
    receipt['adapter_sha256'] = sha(ROOT/'tests/target/trajectory.h')
    receipt['upstream']='https://github.com/ARM-software/optimized-routines/tree/v23.01/math'
    receipt['functions']=['logf','expf','log2f']
    receipt['domain']='K1 finite normal log2 inputs and exp inputs with abs(x)<88; others abort'
    build = json.loads((args.build/'receipt.json').read_text())
    if not build['pass'] or build['source_pin']!=PIN: raise ValueError('unaccepted build or different K1 source')
    elf = args.build/'rtthread.elf'
    if sha(elf) != build['artifacts']['rtthread.elf']: raise ValueError('ELF changed')
    symbols = subprocess.check_output([TOOLCHAIN/'arm-none-eabi-nm', '-S', elf], text=True)
    match = re.search(r'^(\w+) (\w+) \w __logf_data$', symbols, re.M)
    if not match or int(match[2], 16) != 288: raise ValueError('unexpected logf table')
    target = elf_bytes(elf, int(match[1],16), 288)
    shared = args.output/'probe.dylib'
    subprocess.run(['c++', *FLAGS, '-dynamiclib', str(probe), '-o', str(shared)], check=True)
    library = ctypes.CDLL(str(shared))
    native = bytes((ctypes.c_double*36).in_dll(library, 'k1_probe_logf_data'))
    if target != native: raise ValueError('diagnostic table differs from actual linked M85 table')
    receipt['target_table_sha256'] = hashlib.sha256(target).hexdigest()
    match = re.search(r'^(\w+) (\w+) \w __exp2f_data$', symbols, re.M)
    if not match or int(match[2], 16) != 328: raise ValueError('unexpected expf table')
    target = elf_bytes(elf, int(match[1],16), 328)
    native = bytes((ctypes.c_uint64*32).in_dll(library, 'k1_probe_expf_table'))
    native += bytes((ctypes.c_double*9).in_dll(library, 'k1_probe_expf_constants'))
    if target != native: raise ValueError('diagnostic expf table differs from linked M85 table')
    receipt['target_expf_table_sha256'] = hashlib.sha256(target).hexdigest()
    match = re.search(r'^(\w+) (\w+) \w __log2f_data$', symbols, re.M)
    if not match or int(match[2], 16) != 288: raise ValueError('unexpected log2f table')
    target = elf_bytes(elf, int(match[1],16), 288)
    native = bytes((ctypes.c_double*36).in_dll(library, 'k1_probe_log2f_data'))
    if target != native: raise ValueError('diagnostic log2f table differs from linked M85 table')
    receipt['target_log2f_table_sha256'] = hashlib.sha256(target).hexdigest()
    receipt['target_build'] = build['build_id']
    corpus = json.loads((args.corpus/'receipt.json').read_text())
    names = json.loads((ROOT/'docs/import-slices.json').read_text())['product']
    with tempfile.TemporaryDirectory(prefix='k1-libm-donor-') as scratch:
        donor = Path(scratch)
        for name in names:
            path=donor/name; path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(subprocess.check_output(['git','-C',REFERENCE,'show',f'{PIN}:{name}']))
        binary = args.output/'reference-with-arm-logf'
        command = ['c++', *FLAGS, '-fno-builtin-logf', '-fno-builtin-expf', '-fno-builtin-log2f', '-I'+str(donor), '-I'+str(ROOT/'tests/target'),
                   str(ROOT/'tests/host/trajectory_main.cpp'), str(probe),
                   *[str(donor/p) for p in names if p.endswith('.cpp')], '-o', str(binary)]
        subprocess.run(command, check=True)
        receipt['command'] = command
        receipt['traces'] = []
        for fixture in corpus['fixtures']:
            pcm = Path(fixture['path'])
            if sha(pcm) != fixture['sha256']: raise ValueError('PCM changed')
            trace = args.output/(pcm.stem+'-arm-logf.trace')
            with pcm.open('rb') as source, trace.open('wb') as output:
                subprocess.run([binary], stdin=source, stdout=output, check=True, timeout=180)
            receipt['traces'].append(dict(path=str(trace), sha256=sha(trace)))
    (args.output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print('TARGET_LIBM_DIAGNOSTIC_COMPLETE; target acceptance remains false')


if __name__ == '__main__': main()
