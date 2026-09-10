#!/usr/bin/env python3
"""Stage the pinned Titan USB BSP and compile the actual K1 C++ scalar fixture shell."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from datetime import datetime, timezone
from verify_imports import ROOT, verify, REFERENCE, PIN
from stage_profile import instrument_stage_sources

BSP = ROOT.parent / 'sdk-bsp-ra8p1-titan-mini'
BSP_PIN = '6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7'
TOOLCHAIN = ROOT.parent / 'toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin'
SCALAR = '-march=armv8.1-m.main+fp.dp -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard'
SAFETY = '-ffp-contract=off -fno-fast-math -fno-tree-vectorize -fno-tree-slp-vectorize -fstack-usage'
OPTIMISATIONS = {
    'o2': '-O2',
    'o3-unroll': '-O3 -funroll-loops -frename-registers',
}
NPU_REQUIRED = [
    'ethosu_common.h', 'sub_0001_command_stream.c', 'sub_0001_command_stream.h',
    'sub_0001_invoke.c', 'sub_0001_invoke.h', 'sub_0001_model_data.c',
    'sub_0001_model_data.h', 'sub_0001_tensors.c', 'sub_0001_tensors.h',
]
PLATFORM_FILES = [
    'SConscript', 'fixture_app.cpp', 'fixture_app.h', 'hal_entry.c',
    'semantic_sidecar.cpp', 'semantic_sidecar.h',
    'titan_led_pins.h', 'ws2816_gpio_emit.h', 'ws2816_gpio_emit.c',
]
RA8P1_LOCAL_K1_FILES = [
    'core/visual/ws2816_pack.h',
]
P4_PLATFORM_FILES = ['p4_runtime.cpp', 'p4_runtime.h']

def command(args, **kwargs):
    return subprocess.check_output([str(a) for a in args], text=True, **kwargs)

def assert_scalar_generated_code(dump, attributes):
    assert 'Tag_MVE_arch' not in attributes, 'ELF advertises MVE'
    assert not re.search(r'\t(?:v\w+(?:\.\w+)?\s+[^\n]*\bq[0-7]\b|(?:vctp|vpst|wlstp|dlstp|letp)(?:\.\w+)?\b)',dump), 'vector instructions in generated code'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--optimisation',choices=OPTIMISATIONS,default='o2')
    parser.add_argument('--resident-controls',type=Path,help='hash-bound generated schedule header')
    parser.add_argument('--npu-model',type=Path,help='hash-bound generated smoke-graph C source directory')
    parser.add_argument('--npu-input',type=Path,help='hash-bound generated NPU input header')
    parser.add_argument('--p4-source',type=Path,help='hash-bound generic P4 kernels.c/kernels.h directory')
    parser.add_argument('--p4-fixture',type=Path,help='hash-bound packed generic P4 fixture header')
    parser.add_argument('--stage-profile',action='store_true',help='instrument disposable K1 copies with the fixed stage probe')
    args=parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt=dict(label='PRE-SILICON', start=datetime.now(timezone.utc).isoformat(), **{'pass':False})
    try:
        assert command(['git','-C',BSP,'rev-parse','HEAD']).strip()==BSP_PIN, 'BSP pin moved'
        assert not command(['git','-C',BSP,'status','--porcelain','--untracked-files=all']).strip(), 'BSP dirty'
        receipt['imports']=verify(ROOT,REFERENCE,'product',True)
        assert receipt['imports']['pass'], 'import gate failed'
        if bool(args.npu_model)!=bool(args.npu_input): raise RuntimeError('NPU model and input must be supplied together')
        if args.npu_model and not args.resident_controls: raise RuntimeError('NPU load requires the resident K1 schedule')
        if bool(args.p4_source)!=bool(args.p4_fixture): raise RuntimeError('P4 source and fixture must be supplied together')
        if args.p4_source and not args.npu_model: raise RuntimeError('P4/E1 target image requires the identified NPU load')
        if args.stage_profile and not args.resident_controls: raise RuntimeError('stage profiling requires the resident K1 schedule')
        names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
        material=[ROOT/'src/k1'/p for p in names]+[ROOT/'src/k1'/p for p in RA8P1_LOCAL_K1_FILES]+[ROOT/'platform/ra8p1'/p for p in PLATFORM_FILES]+list((ROOT/'tests/target').glob('*.h'))+[Path(__file__)]
        if args.resident_controls:
            if not args.resident_controls.is_file(): raise RuntimeError('resident controls missing')
            material.append(args.resident_controls)
        if args.npu_model:
            material += [ROOT/'platform/ra8p1'/'npu_load.c',ROOT/'platform/ra8p1'/'npu_load.h',args.npu_input]
            for name in NPU_REQUIRED:
                path=args.npu_model/name
                if not path.is_file(): raise RuntimeError(f'missing NPU model source {name}')
                material.append(path)
        if args.p4_source:
            material += [ROOT/'platform/ra8p1'/name for name in P4_PLATFORM_FILES]+[args.p4_fixture]
            for name in ['kernels.c','kernels.h']:
                path=args.p4_source/name
                if not path.is_file(): raise RuntimeError(f'missing P4 source {name}')
                material.append(path)
        if args.stage_profile:
            material += [ROOT/'platform/ra8p1/stage_probe.h',ROOT/'scripts/stage_profile.py']
        receipt['sources']={}
        for path in sorted(material):
            if not path.is_file(): continue
            try: key=str(path.relative_to(ROOT))
            except ValueError:
                if path==args.resident_controls: key='external/resident_controls.h'
                elif path==args.npu_input: key='external/npu_inputs.h'
                elif args.npu_model and path.parent==args.npu_model: key='external/npu/'+path.name
                elif path==args.p4_fixture: key='external/p4_fixture.h'
                elif args.p4_source and path.parent==args.p4_source: key='external/p4/'+path.name
                else: raise
            receipt['sources'][key]=hashlib.sha256(path.read_bytes()).hexdigest()
        optimisation='-O0' if args.debug else OPTIMISATIONS[args.optimisation]
        identity=hashlib.sha256(json.dumps(dict(sources=receipt['sources'],bsp=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile),sort_keys=True).encode()).hexdigest()
        receipt.update(build_id=identity,source_pin=PIN,bsp_pin=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,
                       resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,
                       compiler=command([TOOLCHAIN/'arm-none-eabi-g++','--version']).splitlines()[0])
        stage=args.output/'stage'
        shutil.copytree(BSP/'project/Titan_Mini_usb_pcdc',stage)
        for name,target in {'rt-thread':BSP/'rt-thread','libraries':BSP/'libraries','ra':BSP/'FSPConfiguration/ra','ra_cfg':BSP/'FSPConfiguration/ra_cfg','ra_gen':BSP/'FSPConfiguration/ra_gen','configuration.xml':BSP/'FSPConfiguration/configuration.xml'}.items():
            # SCons auxiliary outputs can follow source paths. Copy the build
            # inputs into this disposable stage; never generate into the oracle.
            if target.is_dir(): shutil.copytree(target,stage/name)
            else: shutil.copy2(target,stage/name)
        rtconfig=stage/'rtconfig.py'
        text=rtconfig.read_text()
        assert text.count('-march=armv8.1-m.main+mve.fp+fp.dp')==1
        text=text.replace('-march=armv8.1-m.main+mve.fp+fp.dp','-march=armv8.1-m.main+fp.dp')
        text=text.replace("CFLAGS = DEVICE + ' -Dgcc'", "CFLAGS = DEVICE + ' -Dgcc "+SAFETY+"'")
        if not args.debug:
            text=text.replace("BUILD = 'debug'", "BUILD = 'release'")
            if optimisation!='-O2':
                assert text.count("CFLAGS += ' -O2'")==1
                text=text.replace("CFLAGS += ' -O2'",f"CFLAGS += ' {optimisation}'")
        if args.npu_model:
            text=text.replace("CFLAGS = DEVICE + ' -Dgcc", "CFLAGS = DEVICE + ' -DK1_NPU_LOAD=1 -Dgcc")
        if args.p4_source:
            text=text.replace('-DK1_NPU_LOAD=1 -Dgcc','-DK1_NPU_LOAD=1 -DK1_P4_LOAD=1 -Dgcc')
        rtconfig.write_text(text)
        config=stage/'rtconfig.h'
        text=config.read_text()
        assert text.count('#define RT_MAIN_THREAD_STACK_SIZE 2048')==1
        config.write_text(text.replace('#define RT_MAIN_THREAD_STACK_SIZE 2048','#define RT_MAIN_THREAD_STACK_SIZE 32768'))
        for name in PLATFORM_FILES:
            shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        for header in (ROOT/'tests/target').glob('*.h'): shutil.copy2(header,stage/'src'/header.name)
        if args.resident_controls:
            shutil.copy2(args.resident_controls,stage/'src/resident_controls.h')
            scon=stage/'src/SConscript'
            scon.write_text(scon.read_text().replace("LOCAL_CXXFLAGS=' -std=c++17", "LOCAL_CXXFLAGS=' -DK1_RESIDENT_SCHEDULE=1 -std=c++17"))
        if args.stage_profile:
            shutil.copy2(ROOT/'platform/ra8p1/stage_probe.h',stage/'src/stage_probe.h')
            scon=stage/'src/SConscript'
            text=scon.read_text()
            assert text.count("LOCAL_CXXFLAGS=' ")==1
            scon.write_text(text.replace("LOCAL_CXXFLAGS=' ","LOCAL_CXXFLAGS=' -DK1_ENABLE_STAGE_PROBE=1 "))
        if args.npu_model:
            shutil.copy2(ROOT/'platform/ra8p1'/'npu_load.c',stage/'src/npu_load.c')
            shutil.copy2(ROOT/'platform/ra8p1'/'npu_load.h',stage/'src/npu_load.h')
            shutil.copy2(args.npu_input,stage/'src/npu_inputs.h')
            model_stage=stage/'src/models'; model_stage.mkdir()
            for name in NPU_REQUIRED: shutil.copy2(args.npu_model/name,model_stage/name)
            (model_stage/'SConscript').write_text("from building import *\ncwd=GetCurrentDir()\nobjs=DefineGroup('K1 identified NPU load',Glob('*.c'),depend=[],CPPPATH=[cwd])\nReturn('objs')\n")
        if args.p4_source:
            for name in P4_PLATFORM_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
            shutil.copy2(args.p4_fixture,stage/'src/p4_fixture.h')
            p4_stage=stage/'src/p4'; p4_stage.mkdir()
            for name in ['kernels.c','kernels.h']: shutil.copy2(args.p4_source/name,p4_stage/name)
            (p4_stage/'SConscript').write_text("from building import *\ncwd=GetCurrentDir()\nobjs=DefineGroup('Generic P4 kernels',Glob('*.c'),depend=[],CPPPATH=[cwd])\nReturn('objs')\n")
        (stage/'src/build_identity.h').write_text(f'#define K1_BUILD_ID "{identity}"\n#define K1_SOURCE_PIN "{PIN}"\n')
        for name in names:
            target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(ROOT/'src/k1'/name,target)
        for name in RA8P1_LOCAL_K1_FILES:
            target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(ROOT/'src/k1'/name,target)
        if args.stage_profile:
            receipt['instrumented_sources']=instrument_stage_sources(stage/'src/k1')
        macros=command([TOOLCHAIN/'arm-none-eabi-g++',*SCALAR.split(),'-dM','-E','-x','c++','/dev/null'])
        (args.output/'compiler-macros.txt').write_text(macros)
        assert '__ARM_FEATURE_MVE ' not in macros, 'compiler enables MVE'
        env=dict(os.environ,RTT_EXEC_PATH=str(TOOLCHAIN),SOURCE_DATE_EPOCH='1788912000')
        with (args.output/'build.log').open('w') as log:
            result=subprocess.run(['uvx','--from','scons','scons','-j8','--verbose'],cwd=stage,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=240)
        if result.returncode: raise RuntimeError(f'build exited {result.returncode}; inspect build.log')
        for name in ['rtthread.elf','rtthread.hex','rtthread.map']:
            shutil.copy2(stage/name,args.output/name)
        dump=command([TOOLCHAIN/'arm-none-eabi-objdump','-d','-C',args.output/'rtthread.elf'])
        (args.output/'disassembly.txt').write_text(dump)
        symbols=command([TOOLCHAIN/'arm-none-eabi-nm','-a','-C',args.output/'rtthread.elf'])
        (args.output/'symbols.txt').write_text(symbols)
        attrs=command([TOOLCHAIN/'arm-none-eabi-readelf','-A',args.output/'rtthread.elf'])
        (args.output/'attributes.txt').write_text(attrs)
        # Scalar VFP s/d registers allowed. Q-register MVE and tail predication are not.
        assert_scalar_generated_code(dump,attrs)
        for symbol in ['AudioPipeline::process','renderProductChannel','k1_fixture_consume','k1_fixture_initialise']:
            assert symbol in dump, f'missing executed K1 symbol {symbol}'
        if args.resident_controls:
            for symbol in ['k1_fixture_schedule_step','k1_resident_pcm','k1_resident_crc','k1_resident_length']:
                assert symbol in symbols, f'missing resident schedule symbol {symbol}'
        if args.npu_model:
            for symbol in ['RM_ETHOSU_Open','sub_0001_invoke','k1_npu_invoke']:
                assert symbol in dump, f'missing NPU execution symbol {symbol}'
        else:
            assert 'RM_ETHOSU_Open' not in dump, 'NPU linked into scalar-only image'
        assert 'R_BSP_SecondaryCoreStart' not in dump, 'secondary core linked'
        if args.p4_source:
            for symbol in ['p4_kernels','k1_p4_step','k1_p4_status']:
                assert symbol in dump, f'missing generic P4 target symbol {symbol}'
        assert '__init_array_start' in (args.output/'rtthread.map').read_text(), 'constructor table missing'
        receipt['size']=command([TOOLCHAIN/'arm-none-eabi-size',args.output/'rtthread.elf'])
        receipt['artifacts']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.is_file()}
        receipt['pass']=True
        print(json.dumps(dict(build_id=identity,size=receipt['size'],**{'pass':True}),indent=2))
    except Exception as error:
        receipt['error']=str(error)
        raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
