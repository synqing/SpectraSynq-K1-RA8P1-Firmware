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
from palette_renderer_overlay import apply_palette_overlay

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
    'ws281x_diag.h', 'ws281x_diag.c',
]
RA8P1_LOCAL_K1_FILES = [
    'core/visual/ws2816_pack.h',
]
PALETTE_PLATFORM_FILES = ['palette_runtime.cpp', 'palette_runtime.h']
PALETTE_MORPH_FILES = ['palette_transition.h']
P4_PLATFORM_FILES = ['p4_runtime.cpp', 'p4_runtime.h']
PDM_TARGET_FILES = ['pdm_capture.c', 'pdm_capture.h', 'pdm_target.c', 'pdm_target.h']
PDM_SOURCE_CONTRACT = ROOT / 'docs/dual-im69d130-source-contract.json'

def command(args, **kwargs):
    return subprocess.check_output([str(a) for a in args], text=True, **kwargs)

def assert_scalar_generated_code(dump, attributes):
    assert 'Tag_MVE_arch' not in attributes, 'ELF advertises MVE'
    assert not re.search(r'\t(?:v\w+(?:\.\w+)?\s+[^\n]*\bq[0-7]\b|(?:vctp|vpst|wlstp|dlstp|letp)(?:\.\w+)?\b)',dump), 'vector instructions in generated code'

def stage_dual_pdm_vectors(stage: Path) -> None:
    """Add the second DMAC and falling-lane PDM error vectors to the copy.

    The pinned BSP allocates DMAC0 and PDM_ERR2 only. The dual microphone path
    needs one further DMAC completion vector and the corresponding channel-0
    PDM error vector. This mutates only the disposable build stage.
    """
    header = stage / 'ra_gen/vector_data.h'
    text = header.read_text()
    assert text.count('#define VECTOR_DATA_IRQ_COUNT    (74)') == 1
    assert text.count('#define BSP_ICU_VECTOR_NUM_ENTRIES (74)') == 1
    marker = '        /* The number of entries required for the ICU vector table. */'
    assert text.count(marker) == 1
    additions = (
        '        #define VECTOR_NUMBER_DMAC1_INT ((IRQn_Type) 74) '
        '/* DMAC1 INT (DMAC1 transfer end) */\n'
        '        #define DMAC1_INT_IRQn          ((IRQn_Type) 74) '
        '/* DMAC1 INT (DMAC1 transfer end) */\n'
        '        #define VECTOR_NUMBER_PDM_ERR0 ((IRQn_Type) 75) '
        '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
        '        #define PDM_ERR0_IRQn          ((IRQn_Type) 75) '
        '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
    )
    text = text.replace('#define VECTOR_DATA_IRQ_COUNT    (74)',
                        '#define VECTOR_DATA_IRQ_COUNT    (76)')
    text = text.replace(marker, additions + marker)
    text = text.replace('#define BSP_ICU_VECTOR_NUM_ENTRIES (74)',
                        '#define BSP_ICU_VECTOR_NUM_ENTRIES (76)')
    header.write_text(text)

    source = stage / 'ra_gen/vector_data.c'
    text = source.read_text()
    isr_marker = '            [73] = ipc_isr, /* IPC IRQ1 (CPU Mutual Interrupt 1) */\n        };'
    event_marker = ('            [73] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ1,FIXED), '
                    '/* IPC IRQ1 (CPU Mutual Interrupt 1) */\n        };')
    assert text.count(isr_marker) == 1
    assert text.count(event_marker) == 1
    text = text.replace(
        isr_marker,
        '            [73] = ipc_isr, /* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
        '            [74] = dmac_int_isr, /* DMAC1 INT (DMAC1 transfer end) */\n'
        '            [75] = pdm_err_isr, /* PDM ERR0 (Error detection interrupt channel 0) */\n'
        '        };')
    text = text.replace(
        event_marker,
        '            [73] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ1,FIXED), '
        '/* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
        '            [74] = BSP_PRV_VECT_ENUM(EVENT_DMAC1_INT,FIXED), '
        '/* DMAC1 INT (DMAC1 transfer end) */\n'
        '            [75] = BSP_PRV_VECT_ENUM(EVENT_PDM_ERR0,FIXED), '
        '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
        '        };')
    source.write_text(text)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--optimisation',choices=OPTIMISATIONS,default='o2')
    parser.add_argument('--dcache',choices=('disabled','enabled'),default='disabled',
                        help='retain the proven disabled default or build an identified BSP-enabled experiment')
    parser.add_argument('--resident-controls',type=Path,help='hash-bound generated schedule header')
    parser.add_argument('--npu-model',type=Path,help='hash-bound generated smoke-graph C source directory')
    parser.add_argument('--npu-input',type=Path,help='hash-bound generated NPU input header')
    parser.add_argument('--p4-source',type=Path,help='hash-bound generic P4 kernels.c/kernels.h directory')
    parser.add_argument('--p4-fixture',type=Path,help='hash-bound packed generic P4 fixture header')
    parser.add_argument('--stage-profile',action='store_true',help='instrument disposable K1 copies with the fixed stage probe')
    parser.add_argument('--pdm-target',action='store_true',help='bind both IM69D130 edge lanes to bounded DMAC capture')
    parser.add_argument('--palette-runtime',action='store_true',help='enable all K1 palettes and the native VP palette controls')
    parser.add_argument('--palette-autostart',action='store_true',help='boot into native all-palette preview on the identified WS2812/P601 bench strip')
    parser.add_argument('--palette-morph',action='store_true',help='enable explicit VP palette-transition derivative')
    args=parser.parse_args()
    if args.palette_morph and not args.palette_runtime: parser.error('--palette-morph requires --palette-runtime')
    if args.palette_autostart and not args.palette_runtime: parser.error('--palette-autostart requires --palette-runtime')
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
        if args.palette_runtime:
            material += [ROOT/'platform/ra8p1'/name for name in PALETTE_PLATFORM_FILES]
        if args.palette_morph:
            material += [ROOT/'platform/ra8p1'/name for name in PALETTE_MORPH_FILES]
            material.append(ROOT/'scripts/palette_renderer_overlay.py')
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
        if args.pdm_target:
            material += [ROOT/'platform/ra8p1'/name for name in PDM_TARGET_FILES]
            material.append(PDM_SOURCE_CONTRACT)
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
        identity=hashlib.sha256(json.dumps(dict(sources=receipt['sources'],bsp=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,pdm_target=args.pdm_target,dcache=args.dcache,palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph),sort_keys=True).encode()).hexdigest()
        receipt.update(build_id=identity,source_pin=PIN,bsp_pin=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,
                       resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,
                       pdm_target=args.pdm_target,dcache=args.dcache,
                       palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph,
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
        defines=[]
        if args.palette_runtime: defines.append('-DK1_PALETTE_RUNTIME=1')
        if args.palette_morph: defines.append('-DK1_PALETTE_MORPH=1')
        if args.palette_autostart: defines.append('-DK1_PALETTE_AUTOSTART=1')
        if args.npu_model: defines.append('-DK1_NPU_LOAD=1')
        if args.p4_source: defines.append('-DK1_P4_LOAD=1')
        if args.pdm_target: defines.append('-DK1_PDM_TARGET=1')
        if defines:
            assert text.count('-Dgcc')==1
            text=text.replace('-Dgcc',' '.join(defines)+' -Dgcc')
        if args.dcache=='enabled':
            assert text.count('-Dgcc')==1
            text=text.replace('-Dgcc','-DK1_KEEP_DCACHE_ENABLED=1 -Dgcc')
        rtconfig.write_text(text)
        config=stage/'rtconfig.h'
        text=config.read_text()
        assert text.count('#define RT_MAIN_THREAD_STACK_SIZE 2048')==1
        config.write_text(text.replace('#define RT_MAIN_THREAD_STACK_SIZE 2048','#define RT_MAIN_THREAD_STACK_SIZE 32768'))
        if args.pdm_target:
            pdm_config=stage/'ra_cfg/fsp_cfg/r_pdm_cfg.h'
            text=pdm_config.read_text()
            assert text.count('#define PDM_CFG_DMAC_ENABLE (0)')==1
            pdm_config.write_text(text.replace('#define PDM_CFG_DMAC_ENABLE (0)',
                                               '#define PDM_CFG_DMAC_ENABLE (1)'))
            stage_dual_pdm_vectors(stage)
        for name in PLATFORM_FILES:
            shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.pdm_target:
            for name in PDM_TARGET_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.palette_runtime:
            for name in PALETTE_PLATFORM_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.palette_morph:
            for name in PALETTE_MORPH_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
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
        if args.palette_morph:
            receipt['palette_derivative_sources']=apply_palette_overlay(stage/'src/k1')
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
        if args.palette_runtime:
            for symbol in ['PaletteRuntime::configure','PaletteRuntime::step','PaletteRuntime::catalogueJson','PaletteRuntime::packBenchGrb']:
                assert symbol in dump, f'missing native palette runtime symbol {symbol}'
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
        if args.pdm_target:
            for symbol in ['R_PDM_Open','R_PDM_Start','R_DMAC_Open','pdm_rxi_dmac_isr','k1_pdm_target_initialise']:
                assert symbol in dump, f'missing PDM/DMAC target symbol {symbol}'
            for symbol in ['capture_rise_dmac_ctrl','capture_fall_dmac_ctrl','capture_fall_pdm_ctrl','pdm_callback']:
                assert symbol in symbols, f'missing dual-PDM target symbol {symbol}'
            for symbol in ['g_transfer0','g_transfer1','g_transfer2','g_transfer3','g_transfer4']:
                assert not re.search(rf'\b{symbol}\b',symbols), f'unrelated generated transfer linked: {symbol}'
            capture_buffer=re.search(r'^([0-9a-f]+)\s+[bB]\s+capture_buffer$',symbols,re.MULTILINE)
            assert capture_buffer, 'PDM capture buffer symbol missing'
            assert int(capture_buffer.group(1),16)%32==0, 'PDM capture buffer is not cache-line aligned'
            receipt['pdm_target_configuration']={
                'source_contract':str(PDM_SOURCE_CONTRACT.relative_to(ROOT)),
                'source_contract_sha256':hashlib.sha256(PDM_SOURCE_CONTRACT.read_bytes()).hexdigest(),
                'sample_rate_hz':16000,
                'working_source_sample_rate_hz':12800,
                'sample_rate_parity':False,
                'slot_elements':120,
                'slot_duration_us':7500,
                'dma_interrupt_threshold_samples':8,
                'slot_count':2,
                'lane_count':2,
                'programme_lane':{'microphone':'IM1','select':'HIGH','pdm_channel':2,
                                  'edge':'RISE','dma_channel':0,
                                  'dma_activation':'ELC_EVENT_PDM_DAT2'},
                'measurement_lane':{'microphone':'IM2','select':'LOW','pdm_channel':0,
                                    'edge':'FALL','dma_channel':1,
                                    'dma_activation':'ELC_EVENT_PDM_DAT0'},
                'shared_clock_and_data':True,
                'pdm_cpu_data_irq':'disabled',
                'dcache_completed_slot_invalidation':True,
                'capture_buffer_address':capture_buffer.group(1),
                'capture_buffer_alignment':32,
                'generated_transfer_symbols_linked':[],
                'product_12k8_admitted':False,
            }
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
