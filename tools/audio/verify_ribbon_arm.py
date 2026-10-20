#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. ARM object admission, no programming."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
from build_ribbon_replay import ROOT,sha
sys.path.insert(0,str(ROOT/'scripts'))
from build_scalar import TOOLCHAIN,SCALAR,SAFETY

def main(output):
    output.mkdir(parents=True,exist_ok=False)
    source=ROOT/'src/k1/core/visual/ribbon_material.cpp'
    command=[str(TOOLCHAIN/'arm-none-eabi-g++'),'-std=c++17','-O2',*SCALAR.split(),*SAFETY.split(),
             '-fno-exceptions','-fno-rtti','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src/k1'),'-c',str(source),'-o',str(output/'ribbon.o')]
    result=subprocess.run(command,capture_output=True,text=True)
    (output/'build.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
    symbols=subprocess.check_output([str(TOOLCHAIN/'arm-none-eabi-nm'),'-S',str(output/'ribbon.o')],text=True)
    (output/'symbols.txt').write_text(symbols)
    for banned in [' malloc',' calloc',' realloc','_Znwj','_Znaj']:assert banned not in symbols
    size=subprocess.check_output([str(TOOLCHAIN/'arm-none-eabi-size'),str(output/'ribbon.o')],text=True)
    probe=output/'state_size.cpp'
    probe.write_text('#include "core/visual/ribbon_profile.h"\n#include "core/visual/paired_wave_scene.h"\nextern "C" { unsigned char k1_ribbon_state_bytes[sizeof(k1::core::visual::RibbonMaterial)] = {}; unsigned char k1_profile_state_bytes[sizeof(k1::core::visual::RibbonProfile)] = {}; unsigned char k1_paired_state_bytes[sizeof(k1::core::visual::PairedWaveScene)] = {}; }\n')
    subprocess.run(command[:-4]+['-c',str(probe),'-o',str(output/'state_size.o')],check=True,capture_output=True,text=True)
    state_symbols=subprocess.check_output([str(TOOLCHAIN/'arm-none-eabi-nm'),'-S',str(output/'state_size.o')],text=True)
    state_bytes=int(next(line.split()[1] for line in state_symbols.splitlines() if 'k1_ribbon_state_bytes' in line),16)
    modules=[]
    for name in ['ribbon_profile','paired_wave_scene']:
        module=ROOT/('src/k1/core/visual/'+name+'.cpp')
        argv=command[:-4]+['-c',str(module),'-o',str(output/(name+'.o'))]
        compiled=subprocess.run(argv,capture_output=True,text=True)
        (output/(name+'.log')).write_text(compiled.stdout+compiled.stderr)
        compiled.check_returncode()
        module_symbols=subprocess.check_output([str(TOOLCHAIN/'arm-none-eabi-nm'),'-S',str(output/(name+'.o'))],text=True)
        for banned in [' malloc',' calloc',' realloc','_Znwj','_Znaj']:assert banned not in module_symbols
        modules.append(dict(name=name,command=argv,source_sha256=sha(module),header_sha256=sha(module.with_suffix('.h')),object_sha256=sha(output/(name+'.o'))))
    (output/'RESULT.json').write_text(json.dumps(dict(command=command,source_sha256=sha(source),header_sha256=sha(source.with_suffix('.h')),
        object_sha256=sha(output/'ribbon.o'),object_size=size,additional_modules=modules,state_bytes=state_bytes,all_state_bytes={line.split()[-1]:int(line.split()[1],16) for line in state_symbols.splitlines() if "_state_bytes" in line},steady_state_heap_symbols=False,
        target_executed=False,remaining=['Whole-image integration','Target timing and resource admission','Optical judgement']),indent=2)+'\n')
    print(size)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);main(p.parse_args().output.resolve())
