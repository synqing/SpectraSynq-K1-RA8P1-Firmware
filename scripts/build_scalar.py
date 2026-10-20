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
from tempo_acf_slice_overlay import apply_tempo_acf_slice_overlay
from tempo_dtcm_overlay import (
    apply_tempo_placement,
    verify_tempo_elf_placement,
    verify_tempo_placement_receipt,
)

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
    'ws281x_waveform.h', 'ws281x_waveform.c',
    'ws281x_gpt_dma.h', 'ws281x_gpt_dma.c',
    'ws281x_gpt_dma_pair.h', 'ws281x_gpt_dma_pair.c',
    'ws281x_gpt_dma_hw.h', 'ws281x_gpt_dma_hw.c',
    'ws281x_gpt_dma_hw_pair.h', 'ws281x_gpt_dma_hw_pair.c',
    'pair_fixture_admit.h', 'pair_fixture_admit.c',
    'k1_status_led.h', 'k1_status_led.c',
    'titan_status_gpio.h', 'titan_status_gpio.c',
    'titan_led2_phy.h', 'titan_led2_phy.c',
    'k1_pdm_fifo16.h', 'titan_onboard_mic.h',
    'k1_asrc_24k.h', 'k1_asrc_24k.c',
    'k1_cycle_clock.h',
    'k1_pdm_sensitivity.h',
    'k1_live_clock.h', 'k1_live_clock.c',
]
RA8P1_LOCAL_K1_FILES = [
    'core/visual/ws2816_pack.h',
    'core/audio/tempo_acf_slice.h',
    'core/audio/tempo_acf_slice.cpp',
    'core/audio/k1_audio_hop.h',
    'core/audio/k1_audio_hop.c',
]
LIVE_RUNTIME_FILES = [
    'k1_live_runtime.cpp',
    'k1_live_runtime.h',
    'k1_live_protocol.cpp',
    'k1_live_protocol.h',
    'k1_live_schema.inc',
]
# Local DualMCU ports. Not byte-identical; do not add to import-slices.
# Staged only with --palette-runtime so frozen timing images stay uncontaminated.
PRODUCT_OUTPUT_CHAIN_FILES = [
    'core/visual/frame_blend.h',
    'core/visual/frame_blend.cpp',
    'core/visual/product_runtime_policy.h',
    'core/visual/product_runtime_policy.cpp',
]
PALETTE_PLATFORM_FILES = ['palette_runtime.cpp', 'palette_runtime.h', 'palette_clock.h', 'hd_pixel16.h']
PALETTE_MORPH_FILES = ['palette_transition.h', 'centre_palette_engine.h']
P4_PLATFORM_FILES = ['p4_runtime.cpp', 'p4_runtime.h']
PDM_TARGET_FILES = ['pdm_capture.c', 'pdm_capture.h', 'pdm_target.c', 'pdm_target.h',
                    'k1_asrc_24k.c', 'k1_asrc_24k.h', 'k1_cycle_clock.h',
                    'k1_pdm_sensitivity.h']
PDM_SOURCE_CONTRACT = ROOT / 'docs/titan-onboard-lmd2718-source-contract.json'
PCM1808_TARGET_FILES = [
    'pcm1808_core.cpp', 'pcm1808_core.h', 'pcm1808_target.c', 'pcm1808_target.h',
]
PCM1808_DONOR_HEADERS = [
    'k1_pcm1808_map.h', 'k1_pcm1808_resampler_coeffs.h', 'k1_pcm1808_ring.h',
    'k1_pcm1808_unpack.h', 'k1_resample_48k_to_12k8.h',
]
PCM1808_SOURCE_CONTRACT = ROOT / 'docs/pcm1808-source-contract.json'

def command(args, **kwargs):
    return subprocess.check_output([str(a) for a in args], text=True, **kwargs)

def stage_dmac_control_initialisation(stage: Path) -> dict:
    """Patch the disposable FSP copy, never the pinned BSP.

    RA8P1 Rev 1.30 section 17.2.22 requires activation/transfers disabled
    when changing DMCTL. Reopening a channel must not rewrite global policy
    while another channel is running. Refuse a policy change after activation;
    matching policy needs no register write. Both experiment arms use this.
    """
    path = stage / 'ra/fsp/src/r_dmac/r_dmac.c'
    before = path.read_text()
    old = '''    R_DMA->DMAST = 1;

#if BSP_FEATURE_DMAC_HAS_DMCTL
    R_DMA->DMCTL = (DMAC_CFG_PRIORITY_MODE << R_DMA_DMCTL_PR_Pos) |
                   (DMAC_CFG_ERROR_CHANNEL_CLEAR << R_DMA_DMCTL_ERCH_Pos);
#endif'''
    new = '''#if BSP_FEATURE_DMAC_HAS_DMCTL
    uint8_t const desired_dmctl = (DMAC_CFG_PRIORITY_MODE << R_DMA_DMCTL_PR_Pos) |
                                 (DMAC_CFG_ERROR_CHANNEL_CLEAR << R_DMA_DMCTL_ERCH_Pos);
    if (R_DMA->DMCTL != desired_dmctl)
    {
        /* K1: global arbitration is set only before DMA activation. */
        FSP_ERROR_RETURN(0 == (R_DMA->DMAST & 1U), FSP_ERR_IN_USE);
        R_DMA->DMCTL = desired_dmctl;
    }
#endif
    R_DMA->DMAST = 1;'''
    if before.count(old) != 1:
        raise RuntimeError('FSP DMCTL initialisation source mismatch')
    after = before.replace(old, new)
    path.write_text(after)
    return {'source': str(path.relative_to(stage)),
            'before_sha256': hashlib.sha256(before.encode()).hexdigest(),
            'after_sha256': hashlib.sha256(after.encode()).hexdigest(),
            'live_policy_change': 'REFUSED', 'unchanged_policy_write': False}

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

def stage_led_gpt_vectors(stage: Path) -> None:
    """Add DMAC2 completion for the GPT6 LED transmitter.

    PDM occupies DMAC0/DMAC1. Generated OSPI uses DMAC0. LED uses DMAC2.
    GPT0 overflow IRQ 63 already exists in the USB PCDC vector table.
    """
    header = stage / 'ra_gen/vector_data.h'
    text = header.read_text()
    if 'VECTOR_NUMBER_DMAC2_INT' in text:
        return
    if 'VECTOR_NUMBER_DMAC1_INT' in text:
        old_count, new_count, irq = '76', '77', '76'
        header_anchor = (
            '        #define PDM_ERR0_IRQn          ((IRQn_Type) 75) '
            '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
        )
        isr_anchor = (
            '            [75] = pdm_err_isr, /* PDM ERR0 (Error detection interrupt channel 0) */\n'
            '        };'
        )
        event_anchor = (
            '            [75] = BSP_PRV_VECT_ENUM(EVENT_PDM_ERR0,FIXED), '
            '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
            '        };'
        )
        isr_insert = (
            '            [75] = pdm_err_isr, /* PDM ERR0 (Error detection interrupt channel 0) */\n'
            '            [76] = dmac_int_isr, /* DMAC2 INT (DMAC2 transfer end) */\n'
            '        };'
        )
        event_insert = (
            '            [75] = BSP_PRV_VECT_ENUM(EVENT_PDM_ERR0,FIXED), '
            '/* PDM ERR0 (Error detection interrupt channel 0) */\n'
            '            [76] = BSP_PRV_VECT_ENUM(EVENT_DMAC2_INT,FIXED), '
            '/* DMAC2 INT (DMAC2 transfer end) */\n'
            '        };'
        )
    else:
        old_count, new_count, irq = '74', '75', '74'
        header_anchor = (
            '        /* The number of entries required for the ICU vector table. */'
        )
        isr_anchor = (
            '            [73] = ipc_isr, /* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
            '        };'
        )
        event_anchor = (
            '            [73] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ1,FIXED), '
            '/* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
            '        };'
        )
        isr_insert = (
            '            [73] = ipc_isr, /* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
            '            [74] = dmac_int_isr, /* DMAC2 INT (DMAC2 transfer end) */\n'
            '        };'
        )
        event_insert = (
            '            [73] = BSP_PRV_VECT_ENUM(EVENT_IPC_IRQ1,FIXED), '
            '/* IPC IRQ1 (CPU Mutual Interrupt 1) */\n'
            '            [74] = BSP_PRV_VECT_ENUM(EVENT_DMAC2_INT,FIXED), '
            '/* DMAC2 INT (DMAC2 transfer end) */\n'
            '        };'
        )
    additions = (
        f'        #define VECTOR_NUMBER_DMAC2_INT ((IRQn_Type) {irq}) '
        '/* DMAC2 INT (DMAC2 transfer end) */\n'
        f'        #define DMAC2_INT_IRQn          ((IRQn_Type) {irq}) '
        '/* DMAC2 INT (DMAC2 transfer end) */\n'
    )
    assert text.count(f'#define VECTOR_DATA_IRQ_COUNT    ({old_count})') == 1
    text = text.replace(f'#define VECTOR_DATA_IRQ_COUNT    ({old_count})',
                        f'#define VECTOR_DATA_IRQ_COUNT    ({new_count})')
    text = text.replace(f'#define BSP_ICU_VECTOR_NUM_ENTRIES ({old_count})',
                        f'#define BSP_ICU_VECTOR_NUM_ENTRIES ({new_count})')
    if header_anchor == '        /* The number of entries required for the ICU vector table. */':
        assert text.count(header_anchor) == 1
        text = text.replace(header_anchor, additions + header_anchor)
    else:
        assert text.count(header_anchor) == 1
        text = text.replace(header_anchor, header_anchor + additions)
    header.write_text(text)

    source = stage / 'ra_gen/vector_data.c'
    text = source.read_text()
    assert text.count(isr_anchor) == 1
    assert text.count(event_anchor) == 1
    source.write_text(text.replace(isr_anchor, isr_insert).replace(event_anchor, event_insert))

def stage_led_pair_vectors(stage: Path) -> None:
    """Add DMAC3 completion for GPT7 lane A1. PDM fall keeps DMAC2."""
    header = stage / 'ra_gen/vector_data.h'
    text = header.read_text()
    if 'VECTOR_NUMBER_DMAC3_INT' in text:
        return
    if 'VECTOR_NUMBER_DMAC2_INT' not in text:
        raise RuntimeError('DMAC3 staging requires DMAC2 already present for PDM fall')
    irq = '77'
    old_count, new_count = '77', '78'
    header_anchor = (
        '        #define DMAC2_INT_IRQn          ((IRQn_Type) 76) '
        '/* DMAC2 INT (DMAC2 transfer end) */\n'
    )
    isr_anchor = (
        '            [76] = dmac_int_isr, /* DMAC2 INT (DMAC2 transfer end) */\n'
        '        };'
    )
    event_anchor = (
        '            [76] = BSP_PRV_VECT_ENUM(EVENT_DMAC2_INT,FIXED), '
        '/* DMAC2 INT (DMAC2 transfer end) */\n'
        '        };'
    )
    isr_insert = (
        '            [76] = dmac_int_isr, /* DMAC2 INT (DMAC2 transfer end) */\n'
        '            [77] = dmac_int_isr, /* DMAC3 INT (DMAC3 transfer end) */\n'
        '        };'
    )
    event_insert = (
        '            [76] = BSP_PRV_VECT_ENUM(EVENT_DMAC2_INT,FIXED), '
        '/* DMAC2 INT (DMAC2 transfer end) */\n'
        '            [77] = BSP_PRV_VECT_ENUM(EVENT_DMAC3_INT,FIXED), '
        '/* DMAC3 INT (DMAC3 transfer end) */\n'
        '        };'
    )
    additions = (
        f'        #define VECTOR_NUMBER_DMAC3_INT ((IRQn_Type) {irq}) '
        '/* DMAC3 INT (DMAC3 transfer end) */\n'
        f'        #define DMAC3_INT_IRQn          ((IRQn_Type) {irq}) '
        '/* DMAC3 INT (DMAC3 transfer end) */\n'
    )
    assert text.count(f'#define VECTOR_DATA_IRQ_COUNT    ({old_count})') == 1
    text = text.replace(f'#define VECTOR_DATA_IRQ_COUNT    ({old_count})',
                        f'#define VECTOR_DATA_IRQ_COUNT    ({new_count})')
    text = text.replace(f'#define BSP_ICU_VECTOR_NUM_ENTRIES ({old_count})',
                        f'#define BSP_ICU_VECTOR_NUM_ENTRIES ({new_count})')
    assert text.count(header_anchor) == 1
    text = text.replace(header_anchor, header_anchor + additions)
    header.write_text(text)

    source = stage / 'ra_gen/vector_data.c'
    text = source.read_text()
    assert text.count(isr_anchor) == 1
    assert text.count(event_anchor) == 1
    source.write_text(text.replace(isr_anchor, isr_insert).replace(event_anchor, event_insert))

def stage_pcm1808_vectors(stage: Path) -> None:
    """Fail closed: U18 has no complete, framed PCM1808 receive route."""
    del stage
    raise RuntimeError(
        'PCM1808 target is blocked: SSIE1 is only on the inaccessible U11 camera '
        'connector and U18 SPI_B exposes SSLB2/3, not the SSLB0 input required in '
        'slave mode. Use a proper U11 mating breakout or a bridge board.'
    )

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--debug', action='store_true',
                        help='separately identified -O0 image; RT-Thread debug build already adds DWARF')
    parser.add_argument('--debug-info', action='store_true',
                        help='keep the selected optimisation and add DWARF; not the -O0 --debug build')
    parser.add_argument('--optimisation',choices=OPTIMISATIONS,default='o2')
    parser.add_argument('--dcache',choices=('disabled','enabled'),default='disabled',
                        help='retain the proven disabled default or build an identified BSP-enabled experiment')
    parser.add_argument('--resident-controls',type=Path,help='hash-bound generated schedule header')
    parser.add_argument('--npu-model',type=Path,help='hash-bound generated smoke-graph C source directory')
    parser.add_argument('--npu-input',type=Path,help='hash-bound generated NPU input header')
    parser.add_argument('--p4-source',type=Path,help='hash-bound generic P4 kernels.c/kernels.h directory')
    parser.add_argument('--p4-fixture',type=Path,help='hash-bound packed generic P4 fixture header')
    parser.add_argument('--stage-profile',action='store_true',help='instrument disposable K1 copies with the fixed stage probe')
    parser.add_argument('--pdm-target',action='store_true',help='bind both onboard LMD2718 edge lanes to bounded DMAC capture')
    parser.add_argument('--pcm1808-target',action='store_true',help='fail closed until a practical PCM1808 adapter route exists')
    parser.add_argument('--palette-gpt-dma',action='store_true',help='GPT6/DMAC2/GPT0 P601 WS2812 bench; no GPIO fallback')
    parser.add_argument('--dmac-priority', choices=('fixed', 'round-robin'), default='fixed',
                        help='explicit GPT/PDM arbitration experiment; no runtime policy changes')
    parser.add_argument('--dmac-lane-map', choices=('pdm-first', 'led-first'), default='pdm-first',
                        help='build-identified DMAC channel ownership; led-first isolates fixed-priority starvation')
    parser.add_argument('--palette-runtime',action='store_true',help='enable all K1 palettes and the native VP palette controls')
    parser.add_argument('--palette-autostart',action='store_true',help='boot live WaveformK1 mode 32 with active+emit (flags 5); not a catalogue carousel')
    parser.add_argument('--palette-morph',action='store_true',help='enable explicit VP palette-transition derivative')
    parser.add_argument('--palette-ws2816',action='store_true',help='native 160-pixel palette output on P601/P004 with profile 4 and a 60 Hz schedule')
    parser.add_argument('--palette-ws2816-gpt-pair',action='store_true',
                        help='concurrent GPT6/P601 + GPT7/P603 WS2816 pair; exclusive of single-lane GPT and GPIO WS2816')
    parser.add_argument('--tempo-acf-slice',action='store_true',
                        help='split tempo ACF lag rows across hops; DualMCU files stay unmodified on disk')
    parser.add_argument('--tempo-placement',choices=('empty-tcm','dtcm'),default=None,
                        help='resident hot-array placement; empty-tcm is the original timing brief, dtcm is the named historical overlay')
    parser.add_argument('--live-runtime',action='store_true',
                        help='named live K1 AP/VP owner; first candidate defaults emit off')
    parser.add_argument('--live-emit',action='store_true',
                        help='opt in to physical emit on a --live-runtime image')
    parser.add_argument('--live-defaults',type=Path,
                        help='optional hashed live-default JSON; omitted uses firmware canonical defaults')
    args=parser.parse_args()
    if args.debug and args.debug_info:
        parser.error('--debug-info keeps the selected optimisation; --debug is the separate -O0 build')
    gpt_output = bool(args.palette_gpt_dma) or bool(args.palette_ws2816_gpt_pair)
    if args.palette_gpt_dma and args.palette_ws2816_gpt_pair:
        parser.error('--palette-gpt-dma and --palette-ws2816-gpt-pair are mutually exclusive')
    if args.palette_gpt_dma and (not args.palette_runtime or args.palette_ws2816 or args.resident_controls):
        parser.error('--palette-gpt-dma requires --palette-runtime and forbids WS2816/resident-controls')
    if args.palette_ws2816_gpt_pair and (not args.palette_runtime or args.palette_ws2816 or args.resident_controls):
        parser.error('--palette-ws2816-gpt-pair requires --palette-runtime and forbids GPIO WS2816/resident-controls')
    if args.dmac_priority != 'fixed' and not (gpt_output and args.pdm_target):
        parser.error('--dmac-priority round-robin requires GPT plus live PDM')
    if args.dmac_lane_map != 'pdm-first' and not (gpt_output and args.pdm_target):
        parser.error('--dmac-lane-map led-first requires GPT plus live PDM')
    if args.palette_morph and not args.palette_runtime: parser.error('--palette-morph requires --palette-runtime')
    if args.palette_autostart and not args.palette_runtime: parser.error('--palette-autostart requires --palette-runtime')
    if args.palette_ws2816 and not args.palette_runtime: parser.error('--palette-ws2816 requires --palette-runtime')
    if args.palette_ws2816 and (args.pdm_target or args.resident_controls):
        parser.error('--palette-ws2816 is a standalone LED bench image')
    if bool(args.resident_controls) != bool(args.tempo_placement):
        parser.error('--tempo-placement empty-tcm|dtcm is required with --resident-controls and forbidden without it')
    if args.live_runtime and not (
            args.pdm_target and args.palette_runtime and args.palette_morph and
            gpt_output and args.palette_autostart):
        parser.error('--live-runtime requires --pdm-target --palette-runtime --palette-morph --palette-gpt-dma or --palette-ws2816-gpt-pair --palette-autostart')
    if args.live_emit and not args.live_runtime:
        parser.error('--live-emit requires --live-runtime')
    if args.live_defaults and not args.live_runtime:
        parser.error('--live-defaults requires --live-runtime')
    if args.pcm1808_target:
        stage_pcm1808_vectors(Path('.'))
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
            material += [ROOT/'src/k1'/name for name in PRODUCT_OUTPUT_CHAIN_FILES]
        if args.palette_morph:
            material += [ROOT/'platform/ra8p1'/name for name in PALETTE_MORPH_FILES]
            material.append(ROOT/'scripts/palette_renderer_overlay.py')
        if args.tempo_acf_slice:
            material.append(ROOT/'scripts/tempo_acf_slice_overlay.py')
        if args.resident_controls:
            material.append(ROOT/'scripts/tempo_dtcm_overlay.py')
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
            material += [
                ROOT/'platform/ra8p1/stage_probe.h',
                ROOT/'platform/ra8p1/k1_double_probe.h',
                ROOT/'platform/ra8p1/k1_double_probe.c',
                ROOT/'scripts/stage_profile.py',
            ]
        if args.pdm_target:
            material += [ROOT/'platform/ra8p1'/name for name in PDM_TARGET_FILES]
            material.append(PDM_SOURCE_CONTRACT)
        if args.pcm1808_target:
            material += [ROOT/'platform/ra8p1'/name for name in PCM1808_TARGET_FILES]
            material += [ROOT/'platform/ra8p1/pcm1808'/name for name in PCM1808_DONOR_HEADERS]
            material.append(PCM1808_SOURCE_CONTRACT)
        if args.live_runtime:
            material += [ROOT/'platform/ra8p1'/name for name in LIVE_RUNTIME_FILES]
            material += [ROOT/'src/k1'/name for name in (
                'core/audio/k1_audio_hop.h', 'core/audio/k1_audio_hop.c')]
            material.append(ROOT/'docs/contracts/titan-live-v1.json')
            schema = (ROOT/'docs/contracts/titan-live-v1.json').read_bytes()
            digest = hashlib.sha256(schema).hexdigest()
            inc = (ROOT/'platform/ra8p1/k1_live_schema.inc').read_text()
            if digest not in inc:
                raise RuntimeError('k1_live_schema.inc does not match titan-live-v1.json')
            if args.live_defaults:
                if not args.live_defaults.is_file():
                    raise RuntimeError('live defaults missing')
                material.append(args.live_defaults)
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
                elif args.live_defaults and path==args.live_defaults: key='external/live_defaults.json'
                else: raise
            receipt['sources'][key]=hashlib.sha256(path.read_bytes()).hexdigest()
        optimisation='-O0' if args.debug else OPTIMISATIONS[args.optimisation]
        identity=hashlib.sha256(json.dumps(dict(sources=receipt['sources'],bsp=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,debug_info=bool(args.debug_info),resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,pdm_target=args.pdm_target,pcm1808_target=args.pcm1808_target,dcache=args.dcache,palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph,palette_ws2816=args.palette_ws2816,palette_ws2816_gpt_pair=args.palette_ws2816_gpt_pair,palette_gpt_dma=args.palette_gpt_dma,dmac_priority=args.dmac_priority,dmac_lane_map=args.dmac_lane_map,tempo_placement=args.tempo_placement,product_output_chain=bool(args.palette_runtime),live_runtime=args.live_runtime,live_emit=args.live_emit),sort_keys=True).encode()).hexdigest()
        receipt.update(build_id=identity,source_pin=PIN,bsp_pin=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,debug=args.debug,debug_info=bool(args.debug_info),
                       resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,
                       pdm_target=args.pdm_target,pcm1808_target=args.pcm1808_target,dcache=args.dcache,
                       palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph,palette_ws2816=args.palette_ws2816,palette_ws2816_gpt_pair=args.palette_ws2816_gpt_pair,palette_gpt_dma=args.palette_gpt_dma,
                       dmac_priority=args.dmac_priority,
                       dmac_lane_map=args.dmac_lane_map,
                       tempo_placement=args.tempo_placement,
                       product_output_chain=bool(args.palette_runtime),
                       live_runtime=args.live_runtime,
                       live_emit=args.live_emit,
                       compiler=command([TOOLCHAIN/'arm-none-eabi-g++','--version']).splitlines()[0])
        stage=args.output/'stage'
        shutil.copytree(BSP/'project/Titan_Mini_usb_pcdc',stage)
        for name,target in {'rt-thread':BSP/'rt-thread','libraries':BSP/'libraries','ra':BSP/'FSPConfiguration/ra','ra_cfg':BSP/'FSPConfiguration/ra_cfg','ra_gen':BSP/'FSPConfiguration/ra_gen','configuration.xml':BSP/'FSPConfiguration/configuration.xml'}.items():
            # SCons auxiliary outputs can follow source paths. Copy the build
            # inputs into this disposable stage; never generate into the oracle.
            if target.is_dir(): shutil.copytree(target,stage/name)
            else: shutil.copy2(target,stage/name)
        if args.palette_gpt_dma or args.palette_ws2816_gpt_pair:
            receipt['dmac_control_initialisation'] = stage_dmac_control_initialisation(stage)
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
            if args.debug_info:
                opt_flag=f"CFLAGS += ' {optimisation}'"
                assert text.count(opt_flag)==1
                text=text.replace(opt_flag, f"CFLAGS += ' {optimisation} -gdwarf-2 -g'", 1)
        defines=['-DK1_RA8P1_TARGET=1']
        if args.palette_gpt_dma or args.palette_ws2816_gpt_pair:
            defines.append('-DDMAC_CFG_PRIORITY_MODE=' +
                           ('1' if args.dmac_priority == 'round-robin' else '0'))
        if args.dmac_lane_map == 'led-first':
            defines.extend([
                '-DK1_WS281X_GPT_DMA_CHANNEL=0u',
                '-DK1_WS281X_GPT_DMA_IRQ=DMAC0_INT_IRQn',
                '-DK1_PDM_TARGET_RISE_DMA_CHANNEL=1u',
                '-DK1_PDM_TARGET_RISE_DMA_IRQ=DMAC1_INT_IRQn',
                '-DK1_PDM_TARGET_FALL_DMA_CHANNEL=2u',
                '-DK1_PDM_TARGET_FALL_DMA_IRQ=DMAC2_INT_IRQn',
            ])
            if args.palette_ws2816_gpt_pair:
                defines.extend([
                    '-DK1_WS281X_GPT_DMA_CHANNEL_A1=3u',
                    '-DK1_WS281X_GPT_DMA_IRQ_A1=DMAC3_INT_IRQn',
                ])
        if args.palette_gpt_dma: defines.append('-DK1_PALETTE_GPT_DMA=1')
        if args.palette_ws2816_gpt_pair: defines.append('-DK1_PALETTE_WS2816_GPT_PAIR=1')
        if args.palette_runtime: defines.append('-DK1_PALETTE_RUNTIME=1')
        if args.palette_morph: defines.append('-DK1_PALETTE_MORPH=1')
        if args.palette_autostart: defines.append('-DK1_PALETTE_AUTOSTART=1')
        if args.palette_ws2816: defines.append('-DK1_PALETTE_WS2816=1')
        if args.npu_model: defines.append('-DK1_NPU_LOAD=1')
        if args.p4_source: defines.append('-DK1_P4_LOAD=1')
        if args.pdm_target: defines.append('-DK1_PDM_TARGET=1')
        if args.pcm1808_target: defines.append('-DK1_PCM1808_TARGET=1')
        if args.live_runtime: defines.append('-DK1_LIVE_RUNTIME=1')
        if args.live_emit: defines.append('-DK1_LIVE_EMIT=1')
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
        if args.pcm1808_target:
            stage_pcm1808_vectors(stage)
        stage_led_gpt_vectors(stage)
        if args.palette_ws2816_gpt_pair:
            stage_led_pair_vectors(stage)
        for name in PLATFORM_FILES:
            shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.pdm_target:
            for name in PDM_TARGET_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.pcm1808_target:
            for name in PCM1808_TARGET_FILES:
                shutil.copy2(ROOT/'platform/ra8p1'/name, stage/'src'/name)
            pcm_stage = stage/'src/pcm1808'
            pcm_stage.mkdir()
            for name in PCM1808_DONOR_HEADERS:
                shutil.copy2(ROOT/'platform/ra8p1/pcm1808'/name, pcm_stage/name)
            scon = stage/'src/SConscript'
            text = scon.read_text()
            assert "src += ws281x_diagnostic" in text
            scon.write_text(text.replace("src += ws281x_diagnostic",
                                         "src += ws281x_diagnostic + Glob('pcm1808_target.c')"))
        if args.palette_runtime:
            for name in PALETTE_PLATFORM_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.palette_morph:
            for name in PALETTE_MORPH_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.live_runtime:
            for name in LIVE_RUNTIME_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        for header in (ROOT/'tests/target').glob('*.h'): shutil.copy2(header,stage/'src'/header.name)
        if args.resident_controls:
            shutil.copy2(args.resident_controls,stage/'src/resident_controls.h')
            scon=stage/'src/SConscript'
            scon.write_text(scon.read_text().replace("LOCAL_CXXFLAGS=' -std=c++17", "LOCAL_CXXFLAGS=' -DK1_RESIDENT_SCHEDULE=1 -std=c++17"))
        if args.stage_profile:
            shutil.copy2(ROOT/'platform/ra8p1/stage_probe.h',stage/'src/stage_probe.h')
            shutil.copy2(ROOT/'platform/ra8p1/k1_double_probe.h',stage/'src/k1_double_probe.h')
            shutil.copy2(ROOT/'platform/ra8p1/k1_double_probe.c',stage/'src/k1_double_probe.c')
            scon=stage/'src/SConscript'
            text=scon.read_text()
            assert text.count("LOCAL_CXXFLAGS=' ")==1
            assert "src += ws281x_diagnostic" in text
            text=text.replace("src += ws281x_diagnostic",
                              "src += ws281x_diagnostic + Glob('k1_double_probe.c')")
            text=text.replace("LOCAL_CXXFLAGS=' ","LOCAL_CXXFLAGS=' -DK1_ENABLE_STAGE_PROBE=1 ")
            scon.write_text(text)
            wraps=','.join(
                '--wrap='+name for name in (
                    '__aeabi_dadd','__aeabi_dsub','__aeabi_dmul','__aeabi_ddiv',
                    '__aeabi_drsub','__aeabi_d2f','__aeabi_d2iz','__aeabi_d2ulz',
                    '__aeabi_dcmpeq','__aeabi_dcmplt','__aeabi_dcmple',
                    '__aeabi_dcmpge','__aeabi_dcmpgt','__aeabi_dcmpun',
                    '__aeabi_i2d','__aeabi_ui2d','__aeabi_l2d',
                )
            )
            rtconfig=stage/'rtconfig.py'
            config=rtconfig.read_text()
            marker="LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler"
            assert config.count(marker)==1
            rtconfig.write_text(config.replace(
                marker,
                "LFLAGS = DEVICE + ' -Wl,"+wraps+",--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler",
                1,
            ))
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
        if args.palette_runtime:
            for name in PRODUCT_OUTPUT_CHAIN_FILES:
                target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(ROOT/'src/k1'/name,target)
            receipt['product_output_chain_sources']={
                'status':'local-derivative',
                'dualmcu_pin':PIN,
                'byte_identical':False,
                'files':PRODUCT_OUTPUT_CHAIN_FILES,
            }
        if args.palette_morph:
            receipt['palette_derivative_sources']=apply_palette_overlay(stage/'src/k1')
        if args.tempo_acf_slice:
            receipt['tempo_acf_slice_sources']=apply_tempo_acf_slice_overlay(stage/'src/k1')
        # Probes must match imported ACF text. DTCM then rewrites the arrays.
        if args.stage_profile:
            receipt['instrumented_sources']=instrument_stage_sources(stage/'src/k1')
        if args.resident_controls:
            placement=apply_tempo_placement(stage/'src/k1', args.tempo_placement)
            receipt['tempo_placement_sources']=placement
            if args.tempo_placement=='dtcm':
                receipt['tempo_dtcm_sources']=placement['overlay']
            verify_tempo_placement_receipt(receipt)
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
        if not args.palette_gpt_dma and not args.palette_ws2816_gpt_pair:
            assert 'k1_ws281x_diag_emit' in dump, 'GPIO diagnostic emitter was dropped'
        if args.palette_runtime:
            pack_symbol = 'PaletteRuntime::packBenchGrb48Lane' if (args.palette_ws2816 or args.palette_ws2816_gpt_pair) else 'PaletteRuntime::packBenchGrb('
            for symbol in ['PaletteRuntime::configure','PaletteRuntime::step','PaletteRuntime::catalogueJson',pack_symbol]:
                assert symbol in dump, f'missing native palette runtime symbol {symbol}'
        if args.resident_controls:
            for symbol in ['k1_fixture_schedule_step','k1_resident_pcm','k1_resident_crc','k1_resident_length']:
                assert symbol in symbols, f'missing resident schedule symbol {symbol}'
            receipt['tempo_placement_evidence']=verify_tempo_elf_placement(symbols, args.tempo_placement)
        if args.npu_model:
            for symbol in ['RM_ETHOSU_Open','sub_0001_invoke','k1_npu_invoke']:
                assert symbol in dump, f'missing NPU execution symbol {symbol}'
        else:
            assert 'RM_ETHOSU_Open' not in dump, 'NPU linked into scalar-only image'
        if args.palette_ws2816_gpt_pair:
            for symbol in ['k1_ws281x_gpt_dma_hw_pair_submit', 'k1_ws281x_gpt_dma_pair_lanes_complete',
                           'dmac_int_isr', 'gpt_counter_overflow_isr']:
                assert symbol in dump, f'missing GPT pair execution symbol {symbol}'
            assert 'k1_ws281x_gpt_dma_hw_submit' not in dump, 'single-lane GPT submit linked into pair image'
            receipt['gpt_checkpoint']='PRE_SILICON_ONLY'
            receipt['pair_allocation']={
                'lane_a0':{'pin':'P601','gpt_pwm':6,'stop_gpt':0,
                           'dma':0 if args.dmac_lane_map=='led-first' else 2,
                           'elc':['GPT_A','GPT_B']},
                'lane_a1':{'pin':'P603','gpt_pwm':7,'stop_gpt':1,'dma':3,
                           'elc':['GPT_C','GPT_D']},
                'profile':3,
                'gtstr_mask':'GPT6|GPT7',
                'source_precision':'rgb8_to_grb48',
            }
        if args.palette_gpt_dma:
            for symbol in ['k1_ws281x_gpt_dma_hw_submit', 'k1_ws281x_gpt_dma_hw_snapshot', 'dmac_int_isr', 'gpt_counter_overflow_isr']:
                assert symbol in dump, f'missing GPT execution symbol {symbol}'
            for symbol in ['g_timer0_ctrl', 'timer0_callback']:
                assert not re.search(rf'\b{symbol}\b',symbols), f'competing GPT0 owner linked: {symbol}'
            receipt['gpt_checkpoint']='PRE_SILICON_ONLY'
        assert 'R_BSP_SecondaryCoreStart' not in dump, 'secondary core linked'
        if args.p4_source:
            for symbol in ['p4_kernels','k1_p4_step','k1_p4_status']:
                assert symbol in dump, f'missing generic P4 target symbol {symbol}'
        if args.pdm_target:
            for symbol in ['R_PDM_Open','R_PDM_Start','R_DMAC_Open','k1_pdm_target_dmac_isr','k1_pdm_target_initialise']:
                assert symbol in dump, f'missing PDM/DMAC target symbol {symbol}'
            assert 'pdm_rxi_dmac_isr' not in dump, 'stock FSP DMA ISR still linked; live path must use k1_pdm_target_dmac_isr'
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
                'mpn':'LMD2718T261-OA1',
                'data_pin':'P502',
                'clock_pin':'P812',
                'acoustic_identity':'unproven',
                'profile':'ap_40k_asrc24',
                'sample_rate_hz':40000,
                'working_source_sample_rate_hz':24000,
                'sample_rate_parity':False,
                'admitted_to_24k_ap':True,
                'slot_elements':296,
                'slot_duration_us':7500,
                'dma_interrupt_threshold_samples':8,
                'dma_completion_isr':'k1_pdm_target_dmac_isr',
                'stock_pdm_rxi_dmac_isr':'not_used',
                'slot_count':2,
                'lane_count':2,
                'programme_lane':{'microphone':'U14','select':'LOW','pdm_channel':2,
                                  'edge':'RISE','dma_channel':1 if args.dmac_lane_map == 'led-first' else 0,
                                  'dma_activation':'ELC_EVENT_PDM_DAT2'},
                'measurement_lane':{'microphone':'U13','select':'HIGH','pdm_channel':0,
                                    'edge':'FALL','dma_channel':2 if args.dmac_lane_map == 'led-first' else 1,
                                    'dma_activation':'ELC_EVENT_PDM_DAT0'},
                'shared_clock_and_data':True,
                'pdm_cpu_data_irq':'disabled',
                'dcache_completed_slot_invalidation':True,
                'capture_buffer_address':capture_buffer.group(1),
                'capture_buffer_alignment':32,
                'generated_transfer_symbols_linked':[],
                'dmac_lane_map':args.dmac_lane_map,
                'led_dma_channel':0 if args.dmac_lane_map == 'led-first' else 2,
                'product_12k8_admitted':False,
            }
        if args.live_runtime:
            for symbol in ['LiveAudioRuntime::consume','LiveAudioRuntime::serviceOneHop',
                           'k1_pdm_target_try_read_ap_hop','k1_asrc24_required_for_pull180',
                           'LiveAudioRuntime::encodeSnapshot','LiveAudioRuntime::encodeEvents',
                           'LiveAudioRuntime::encodeTiming','k1::titan::liveEncodeSnapshot']:
                assert symbol in dump, f'missing live-runtime symbol {symbol}'
            assert 'capture_ap_hop_ready' not in symbols, 'pending-hop mailbox still linked'
            receipt['live_runtime']={
                'owner':'LiveAudioRuntime',
                'mailbox':'removed',
                'emit_default':'off' if not args.live_emit else 'on',
                'phase_8_recording':'scheduled_after_dev_ready',
            }
        if args.pcm1808_target:
            for symbol in ['R_SSI_Open','R_SSI_Read','ssi_rxi_isr','ssi_int_isr',
                           'R_DTC_Open','k1_pcm1808_target_initialise',
                           'k1_pcm1808_make_canonical_hop']:
                assert symbol in dump, f'missing PCM1808/SSIE1 target symbol {symbol}'
            for symbol in ['receive', 'core_storage', 'ssi1_ctrl']:
                assert re.search(rf'\b{symbol}\b', symbols), f'missing PCM1808 target symbol {symbol}'
            receipt['pcm1808_target_configuration'] = {
                'source_contract': str(PCM1808_SOURCE_CONTRACT.relative_to(ROOT)),
                'source_contract_sha256': hashlib.sha256(PCM1808_SOURCE_CONTRACT.read_bytes()).hexdigest(),
                'peripheral': 'SSIE1',
                'role': 'slave_receiver',
                'transfer': 'DTC',
                'input_rate_hz': 48000,
                'canonical_rate_hz': 12800,
                'input_frames_per_hop': 360,
                'output_samples_per_hop': 96,
                'bclk': {'pin': 'P702', 'board_net': 'VIO_D6', 'u11_pin': 24},
                'lrck': {'pin': 'P701', 'board_net': 'VIO_D5', 'u11_pin': 33},
                'data': {'pin': 'P700', 'board_net': 'VIO_D4', 'u11_pin': 26},
                'mono_policy': 'MID',
                'trim_q15': 2048,
                'channel_map_validated_on_titan': False,
                'physical_capture': 'NOT_RUN',
            }
        assert '__init_array_start' in (args.output/'rtthread.map').read_text(), 'constructor table missing'
        receipt['size']=command([TOOLCHAIN/'arm-none-eabi-size',args.output/'rtthread.elf'])
        if args.resident_controls:
            receipt['tempo_placement_evidence']['size']=receipt['size']
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
