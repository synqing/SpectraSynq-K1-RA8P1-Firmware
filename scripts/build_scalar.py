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
from verify_tempo_adaptation import verify_tempo_adaptation, LOCAL_FILES as TEMPO_FIELD_LOCAL_FILES, MANIFEST as TEMPO_FIELD_MANIFEST
from stage_profile import instrument_stage_sources
from palette_renderer_overlay import apply_palette_overlay
from tempo_acf_slice_overlay import apply_tempo_acf_slice_overlay
from tempo_dtcm_overlay import (
    apply_tempo_placement,
    verify_tempo_elf_placement,
    verify_tempo_placement_receipt,
    validate_tempo_placement_owner,
)

from live_boot_composition import validate_composition, render_composition

from repo_layout import sibling

BSP = sibling(ROOT, 'sdk-bsp-ra8p1-titan-mini')
BSP_PIN = '6dd0a705d00ffbd6397c9a8c0199cbaa8eec41b7'
TOOLCHAIN = sibling(ROOT, 'toolchains/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi/bin')
# Build profiles. Canon (Kx-Titan TITAN-CANON-2026-10-13 §4/§5): the Titan product is
# never down-rated. 'titan-product' keeps MVE + hardware double precision (no sp-only
# -mfpu, which turns every double into a software call), contraction and vectorisation
# on, and keeps -fno-fast-math so finite/NaN guards survive. 'resident-scalar' exists
# only to reproduce historical (hobbled) images and as a measured BEFORE baseline.
PROFILES = {
    'resident-scalar': ('-march=armv8.1-m.main+fp.dp -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard',
                        '-ffp-contract=off -fno-fast-math -fno-tree-vectorize -fno-tree-slp-vectorize -fstack-usage'),
    'titan-product': ('-march=armv8.1-m.main+mve.fp+fp.dp -mthumb -mfpu=auto -mfloat-abi=hard',
                      '-ffp-contract=fast -fno-fast-math -fstack-usage'),
}
SCALAR, SAFETY = PROFILES['titan-product']
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
    'ws281x_p601_loopback.h', 'ws281x_p601_loopback.c',
    'k1_dmac_lane_guard.h',
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
TOP_PLATFORM_FILES = ['ws281x_gpt_dma_hw_top.h', 'ws281x_gpt_dma_hw_top.c']
LIVE_RUNTIME_FILES = [
    'k1_live_runtime.cpp',
    'k1_live_runtime.h',
    'k1_live_protocol.cpp',
    'k1_live_protocol.h',
    'k1_performance_control.h',
    'k1_live_boot_identity.cpp',
    'k1_live_boot_identity.h',
    'k1_crc32.h',
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
ANALYSIS24_FILES = ['k1_poly24.c', 'k1_poly24.h']
PCM1808_TARGET_FILES = [
    'pcm1808_core.cpp', 'pcm1808_core.h', 'pcm1808_target.c', 'pcm1808_target.h',
]
PCM1808_DONOR_HEADERS = [
    'k1_pcm1808_map.h', 'k1_pcm1808_resampler_coeffs.h', 'k1_pcm1808_ring.h',
    'k1_pcm1808_unpack.h', 'k1_resample_48k_to_12k8.h',
]
PCM1808_SOURCE_CONTRACT = ROOT / 'docs/pcm1808-source-contract.json'
EMOTISCOPE_DONOR_COMMIT = '7f86939bd7c16c4a6dd5789813e9d551246ed134'
EMOTISCOPE_MANIFEST = ROOT / 'docs/emotiscope-port-001/DONOR-SOURCE-MANIFEST.json'
EMOTISCOPE_CLOCK_DELTA = ROOT / 'docs/emotiscope-port-001/CLOCK-PORT-DELTA.json'
EMOTISCOPE_PLATFORM_FILES = ['emotiscope_port.h', 'emotiscope_port.cpp',
                            'emotiscope_quantize.inc', 'emotiscope_output.h',
                            'emotiscope_control.h', 'composed_scene_config.h', 'emotiscope_edge_response.h', 'emotiscope_edge_colour.h',
                            'inhouse_audio.cpp', 'inhouse_audio.h']
EMOTISCOPE_SECONDARY_FILES = ['emotiscope_secondary.cpp', 'emotiscope_secondary.h',
                              'musical_conditioning.h', 'emotiscope_ml.h', 'emotiscope_ml.cpp',
                              'composed_motion.h', 'composed_scene_runtime.h', 'composed_timing.h']
INHOUSE_WAVEFORM_FILES = ['core/visual/inhouse_waveform.cpp', 'core/visual/inhouse_waveform.h']
EMOTISCOPE_CXXFLAGS = '-Wno-narrowing'

def emotiscope_inputs():
    """Bind the frozen donor and every actual adaptation, without importing ESP IO."""
    platform = ROOT / 'platform/ra8p1'
    manifest = json.loads(EMOTISCOPE_MANIFEST.read_text())
    assert manifest['commit'] == EMOTISCOPE_DONOR_COMMIT, 'Emotiscope donor commit changed'
    assert manifest['native_pixels'] == 128, 'Emotiscope native geometry changed'
    material = [EMOTISCOPE_MANIFEST, EMOTISCOPE_CLOCK_DELTA] + [platform / name for name in EMOTISCOPE_PLATFORM_FILES]
    donor = platform / 'emotiscope_donor'
    shims = platform / 'emotiscope_shims'
    for directory in (donor, shims):
        assert directory.is_dir(), f'missing Emotiscope source directory: {directory}'
        material += sorted(p for p in directory.rglob('*') if p.is_file())
    assert all(p.is_file() for p in material), 'required Emotiscope wrapper source missing'
    changes = {}
    for name, original in manifest['files'].items():
        path = donor / name
        assert path.is_file(), f'frozen Emotiscope donor source missing: {name}'
        current = hashlib.sha256(path.read_bytes()).hexdigest()
        if current != original:
            changes[name] = {'before_sha256': original, 'after_sha256': current}
    return material, {
        'donor_commit': manifest['commit'],
        'manifest_sha256': hashlib.sha256(EMOTISCOPE_MANIFEST.read_bytes()).hexdigest(),
        'clock_port_delta_sha256': hashlib.sha256(EMOTISCOPE_CLOCK_DELTA.read_bytes()).hexdigest(),
        'modified_donor_sources': changes,
    }

def render_live_schema_include(data: bytes, source: str) -> str:
    """Generate the existing wire schema symbols from exact profile JSON bytes."""
    digest = hashlib.sha256(data).hexdigest()
    lines = [f'/* generated from {source} - do not edit by hand */', '#pragma once',
             f'static const unsigned char kTitanLiveSchemaJson[{len(data)}] = {{']
    for start in range(0, len(data), 16):
        row = ', '.join(f'{byte:3d}' for byte in data[start:start + 16])
        lines.append('    ' + row + (',' if start + 16 < len(data) else ''))
    lines.extend(['};', f'static const unsigned kTitanLiveSchemaLen = {len(data)};',
                  f'static const char kTitanLiveSchemaSha256[] = "{digest}";', ''])
    return '\n'.join(lines)

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

INT64_CONVERSIONS = {'__aeabi_d2lz', '__aeabi_d2ulz', '__aeabi_l2d', '__aeabi_ul2d'}
# Every double-precision AEABI helper (d*, and the int/float->double conversions),
# reached by a call (bl/blx) or a tail call (b, b.w, conditional b<cc>[.w]).
# Branches to <helper+0x..> are jumps inside a helper and are not counted.
SOFT_DOUBLE_CALL = re.compile(
    r'\t(?:blx?|b(?:eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al)?(?:\.[wn])?)\s+'
    r'[0-9a-f]+ <(__aeabi_(?:d\w+|u?i2d|u?l2d|f2d))>')

def assert_product_generated_code(dump, attributes, macros):
    """titan-product: the unhobbled properties, measured on the built image."""
    assert 'Tag_MVE_arch' in attributes, 'ELF does not advertise MVE'
    assert '#define __ARM_FEATURE_MVE 3' in macros, 'compiler does not enable MVE integer+float'
    fp = re.search(r'#define __ARM_FP (\d+)', macros)
    assert fp and int(fp.group(1)) & 8, 'compiler lacks hardware double precision'
    soft = SOFT_DOUBLE_CALL.findall(dump)
    # FPv5-D16 has no double<->64-bit-integer instruction; these four are library
    # conversions in every hardware-double build. All other helpers mean software
    # double arithmetic and are refused.
    arithmetic = [name for name in soft if name not in INT64_CONVERSIONS]
    assert not arithmetic, f'software double-precision helpers still called: {sorted(set(arithmetic))}'
    return {name: soft.count(name) for name in sorted(set(soft))}

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

def stage_top_output_vectors(stage: Path) -> None:
    """Append secondary stop/completion IRQs without changing primary vectors."""
    header = stage / 'ra_gen/vector_data.h'
    text = header.read_text()
    entries = ((78, 'GPT3_COUNTER_OVERFLOW', 'gpt_counter_overflow_isr'),
               (79, 'GPT4_COUNTER_OVERFLOW', 'gpt_counter_overflow_isr'),
               (80, 'DMAC6_INT', 'dmac_int_isr'),
               (81, 'DMAC7_INT', 'dmac_int_isr'))
    assert text.count('#define VECTOR_DATA_IRQ_COUNT    (78)') == 1
    assert text.count('#define BSP_ICU_VECTOR_NUM_ENTRIES (78)') == 1
    for _, name, _ in entries:
        assert f'VECTOR_NUMBER_{name}' not in text, f'existing top IRQ owner: {name}'
    marker = '        /* The number of entries required for the ICU vector table. */'
    assert text.count(marker) == 1
    additions = ''.join(
        f'        #define VECTOR_NUMBER_{name} ((IRQn_Type) {irq})\n'
        f'        #define {name}_IRQn ((IRQn_Type) {irq})\n'
        for irq, name, _ in entries)
    text = text.replace(marker, additions + marker)
    text = text.replace('#define VECTOR_DATA_IRQ_COUNT    (78)',
                        '#define VECTOR_DATA_IRQ_COUNT    (82)')
    text = text.replace('#define BSP_ICU_VECTOR_NUM_ENTRIES (78)',
                        '#define BSP_ICU_VECTOR_NUM_ENTRIES (82)')
    header.write_text(text)
    source = stage / 'ra_gen/vector_data.c'
    text = source.read_text()
    isr_anchor = '            [77] = dmac_int_isr, /* DMAC3 INT (DMAC3 transfer end) */\n'
    event_anchor = ('            [77] = BSP_PRV_VECT_ENUM(EVENT_DMAC3_INT,FIXED), '
                    '/* DMAC3 INT (DMAC3 transfer end) */\n')
    assert text.count(isr_anchor) == text.count(event_anchor) == 1
    text = text.replace(isr_anchor, isr_anchor + ''.join(
        f'            [{irq}] = {isr}, /* {name} */\n' for irq, name, isr in entries))
    text = text.replace(event_anchor, event_anchor + ''.join(
        f'            [{irq}] = BSP_PRV_VECT_ENUM(EVENT_{name},FIXED), /* {name} */\n'
        for irq, name, _ in entries))
    source.write_text(text)


def stage_pcm1808_vectors(stage: Path) -> None:
    """Fail closed: U18 has no complete, framed PCM1808 receive route."""
    del stage
    raise RuntimeError(
        'PCM1808 target is blocked: SSIE1 is only on the inaccessible U11 camera '
        'connector and U18 SPI_B exposes SSLB2/3, not the SSLB0 input required in '
        'slave mode. Use a proper U11 mating breakout or a bridge board.'
    )

def main():
    global SCALAR, SAFETY
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--debug', action='store_true',
                        help='separately identified -O0 image; RT-Thread debug build already adds DWARF')
    parser.add_argument('--debug-info', action='store_true',
                        help='keep the selected optimisation and add DWARF; not the -O0 --debug build')
    parser.add_argument('--optimisation',choices=OPTIMISATIONS,default=None,
                        help='default: o3-unroll for titan-product (chosen by measured cycles, lane P2 14 Oct 2026), o2 for resident-scalar')
    parser.add_argument('--profile',choices=tuple(PROFILES),default='titan-product',
                        help='titan-product (default, canon) or resident-scalar (historical/BEFORE only)')
    parser.add_argument('--asrc-tcm',choices=('data','data+code'),default=None,
                        help='measured experiment: ASRC state in DTCM (data), plus FIR push in ITCM (data+code; failed to boot 14 Oct)')
    parser.add_argument('--allow-unbootable-itcm-experiment',action='store_true',
                        help='required with --asrc-tcm data+code; the image is marked deploy_admissible=false')
    parser.add_argument('--dcache',choices=('disabled','enabled'),default=None,
                        help='default: enabled for titan-product, disabled for resident-scalar')
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
                        help='concurrent GPT6/P601 + GPT8/P604 WS2816 pair; exclusive of single-lane GPT and GPIO WS2816')
    parser.add_argument('--titan-top-output', action='store_true',
                        help='optional default-disabled P603/GPT7A + P714/GPT12B secondary board')
    parser.add_argument('--dual-edge-donor',action='store_true',help='live May Bloom/Waveform adaptation on the continuous 160-pixel strip')
    parser.add_argument('--emotiscope-port',action='store_true',
                        help='complete Emotiscope v1.1_build AP/VP: 12800 Hz/64 samples, native 128 pixels mapped to continuous 160')
    parser.add_argument('--inhouse-analysis24',action='store_true',
                        help='lane P4: in-house analysis on k1_poly24 24 kHz/180 (donor stays 12.8 kHz/64); needs --emotiscope-port --pdm-target --live-runtime')
    parser.add_argument('--tempo-field',action='store_true',
                        help='admit only the hash-pinned TempoField portable adaptation')
    parser.add_argument('--tempo-acf-slice',action='store_true',
                        help='split tempo ACF lag rows across hops; DualMCU files stay unmodified on disk')
    parser.add_argument('--tempo-placement',choices=('empty-tcm','dtcm'),default=None,
                        help='explicit ACF scratch placement for resident timing or live Emotiscope with --tempo-field')
    parser.add_argument('--live-runtime',action='store_true',
                        help='named live K1 AP/VP owner; first candidate defaults emit off')
    parser.add_argument('--live-emit',action='store_true',
                        help='opt in to physical emit on a --live-runtime image')
    parser.add_argument('--p601-loopback', action='store_true',
                        help='GPT2/DMAC4 PORT6 PIDR sampler on P601; does not change PWM stop')
    parser.add_argument('--live-defaults',type=Path,
                        help='optional hashed live-default JSON; omitted uses firmware canonical defaults')
    args=parser.parse_args()
    SCALAR, SAFETY = PROFILES[args.profile]
    if args.dcache is None:
        args.dcache = 'enabled' if args.profile == 'titan-product' else 'disabled'
    if args.optimisation is None:
        args.optimisation = 'o3-unroll' if args.profile == 'titan-product' else 'o2'
    if args.asrc_tcm == 'data+code' and not args.allow_unbootable_itcm_experiment:
        parser.error('--asrc-tcm data+code did not boot on the Titan (14 Oct 2026, open defect); '
                     'it needs --allow-unbootable-itcm-experiment and is never deploy-admissible')
    if args.titan_top_output and (not args.emotiscope_port or args.p601_loopback):
        parser.error('--titan-top-output requires --emotiscope-port and excludes diagnostic loopback')
    if args.debug and args.debug_info:
        parser.error('--debug-info keeps the selected optimisation; --debug is the separate -O0 build')
    if args.emotiscope_port:
        if args.dual_edge_donor:
            parser.error('--emotiscope-port and --dual-edge-donor are mutually exclusive')
        if not (args.palette_ws2816_gpt_pair and args.live_runtime and args.live_emit and args.pdm_target):
            parser.error('--emotiscope-port requires live emit, onboard PDM and the paired GPT backend')
        if args.dmac_lane_map != 'led-first':
            parser.error('--emotiscope-port preserves microphone DMA1/2 and LED DMA0/3: use --dmac-lane-map led-first')
        if args.tempo_acf_slice or args.resident_controls or args.stage_profile:
            parser.error('--emotiscope-port does not run the legacy K1 AP timing overlays')
        if not args.live_defaults:
            parser.error('--emotiscope-port requires an explicit hash-bound boot-on live-defaults file')
    if args.dual_edge_donor and not (args.palette_ws2816_gpt_pair and args.live_runtime and args.live_emit):
        parser.error('--dual-edge-donor requires live emit and the paired GPT backend')
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
    try:
        validate_tempo_placement_owner(args.tempo_placement, bool(args.resident_controls),
                                       args.emotiscope_port, args.tempo_field, args.live_runtime)
    except ValueError as error:
        parser.error(str(error))
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
        receipt['imports']=(verify_tempo_adaptation(ROOT,REFERENCE) if args.tempo_field
                            else verify(ROOT,REFERENCE,'product',True))
        assert receipt['imports']['pass'], 'import gate failed'
        if bool(args.npu_model)!=bool(args.npu_input): raise RuntimeError('NPU model and input must be supplied together')
        if args.npu_model and not args.resident_controls: raise RuntimeError('NPU load requires the resident K1 schedule')
        if bool(args.p4_source)!=bool(args.p4_fixture): raise RuntimeError('P4 source and fixture must be supplied together')
        if args.p4_source and not args.npu_model: raise RuntimeError('P4/E1 target image requires the identified NPU load')
        if args.stage_profile and not args.resident_controls: raise RuntimeError('stage profiling requires the resident K1 schedule')
        names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
        material=[ROOT/'src/k1'/p for p in names]+[ROOT/'src/k1'/p for p in RA8P1_LOCAL_K1_FILES]+[ROOT/'platform/ra8p1'/p for p in PLATFORM_FILES]+list((ROOT/'tests/target').glob('*.h'))+[Path(__file__)]
        if args.titan_top_output:
            material += [ROOT/'platform/ra8p1'/name for name in TOP_PLATFORM_FILES]
        if args.tempo_field:
            material += [ROOT/'src/k1'/name for name in TEMPO_FIELD_LOCAL_FILES]
            material += [ROOT/TEMPO_FIELD_MANIFEST, ROOT/'scripts/verify_tempo_adaptation.py']
        if args.palette_runtime:
            material += [ROOT/'platform/ra8p1'/name for name in PALETTE_PLATFORM_FILES]
            material += [ROOT/'src/k1'/name for name in PRODUCT_OUTPUT_CHAIN_FILES]
        if args.palette_morph:
            material += [ROOT/'platform/ra8p1'/name for name in PALETTE_MORPH_FILES]
            material.append(ROOT/'scripts/palette_renderer_overlay.py')
        if args.tempo_acf_slice:
            material.append(ROOT/'scripts/tempo_acf_slice_overlay.py')
        if args.tempo_placement:
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
            if args.inhouse_analysis24:
                material += [ROOT/'platform/ra8p1'/name for name in ANALYSIS24_FILES]
        if args.pcm1808_target:
            material += [ROOT/'platform/ra8p1'/name for name in PCM1808_TARGET_FILES]
            material += [ROOT/'platform/ra8p1/pcm1808'/name for name in PCM1808_DONOR_HEADERS]
            material.append(PCM1808_SOURCE_CONTRACT)
        if args.live_runtime:
            material += [ROOT/'platform/ra8p1'/name for name in LIVE_RUNTIME_FILES
                         if not (args.emotiscope_port and name == 'k1_live_schema.inc')]
            material += [ROOT/'src/k1'/name for name in (
                'core/audio/k1_audio_hop.h', 'core/audio/k1_audio_hop.c')]
            schema_path = ROOT / ('docs/contracts/titan-emotiscope-v1.json' if args.emotiscope_port
                                  else 'docs/contracts/titan-live-v1.json')
            material.append(schema_path)
            schema = schema_path.read_bytes()
            digest = hashlib.sha256(schema).hexdigest()
            if args.emotiscope_port:
                # Generate only in the disposable stage; preserve the original
                # source include and its legacy profile verification.
                schema_include = render_live_schema_include(schema, str(schema_path.relative_to(ROOT)))
            else:
                inc = (ROOT/'platform/ra8p1/k1_live_schema.inc').read_text()
                if digest not in inc:
                    raise RuntimeError('k1_live_schema.inc does not match titan-live-v1.json')
            receipt['live_schema'] = {
                'source': str(schema_path.relative_to(ROOT)),
                'sha256': digest, 'bytes': len(schema)}
            if args.live_defaults:
                if not args.live_defaults.is_file():
                    raise RuntimeError('live defaults missing')
                material.append(args.live_defaults)
                material.append(ROOT/'scripts/live_boot_composition.py')
        if args.dual_edge_donor:
            material += [ROOT/'platform/ra8p1/k1_dual_edge_donor.cpp',ROOT/'platform/ra8p1/k1_dual_edge_donor.h']
            material += [p for p in (ROOT/'platform/ra8p1/donor_fixed').rglob('*') if p.is_file()]
        if args.emotiscope_port:
            port_material, port_provenance = emotiscope_inputs()
            if args.titan_top_output:
                secondary_material = [ROOT/'platform/ra8p1'/name for name in EMOTISCOPE_SECONDARY_FILES]
                assert all(path.is_file() for path in secondary_material), 'required secondary visual source missing'
                port_material += secondary_material
            material += [ROOT/'src/k1'/name for name in INHOUSE_WAVEFORM_FILES]
            material += port_material
            receipt['emotiscope_port'] = dict(
                enabled=True, **port_provenance,
                contract='sr12800.hop64.bins64.emotiscope',
                audio_sample_rate_hz=12800, audio_hop_samples=64,
                audio_hop_period_us=5000, spectrum_bins=64, tempo_bins=96,
                native_pixels=128, logical_pixels=160, pixels_per_din=80,
                physical_strips=2 if args.titan_top_output else 1,
                mapping=('per board DIN-A=0..79; DIN-B=80..159; independent continuous scenes'
                         if args.titan_top_output else
                         'DIN-A=0..79; DIN-B=80..159; one continuous scene'),
                spatial_adapter='endpoint- and centre-preserving nearest-neighbour native 128 to 160',
                source_precision='donor_post_gamma_temporal_dither_rgb8_to_rgb16',
                render_period_us=10000, render_cadence='explicit port choice, not historical S3 parity',
                initial_donor_mode='Spectrum (1)', legacy_k1_ap_vp='retained compatibility source; bypassed by live port',
                target_timing='NOT_MEASURED', physical_appearance='NOT_TESTED',
                cxxflags=EMOTISCOPE_CXXFLAGS)
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
        identity=hashlib.sha256(json.dumps(dict(sources=receipt['sources'],bsp=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,profile=args.profile,asrc_tcm=args.asrc_tcm,debug=args.debug,debug_info=bool(args.debug_info),resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,pdm_target=args.pdm_target,pcm1808_target=args.pcm1808_target,dcache=args.dcache,palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph,palette_ws2816=args.palette_ws2816,palette_ws2816_gpt_pair=args.palette_ws2816_gpt_pair,palette_gpt_dma=args.palette_gpt_dma,dmac_priority=args.dmac_priority,dmac_lane_map=args.dmac_lane_map,dual_edge_donor=bool(args.dual_edge_donor),emotiscope_port=bool(args.emotiscope_port),titan_top_output=bool(args.titan_top_output),emotiscope_cxxflags=EMOTISCOPE_CXXFLAGS if args.emotiscope_port else '',tempo_field=bool(args.tempo_field),tempo_placement=args.tempo_placement,product_output_chain=bool(args.palette_runtime and not args.emotiscope_port),live_runtime=args.live_runtime,live_emit=args.live_emit,p601_loopback=bool(args.p601_loopback),**({'inhouse_analysis24':True} if args.inhouse_analysis24 else {})),sort_keys=True).encode()).hexdigest()
        receipt.update(deploy_admissible=args.asrc_tcm != 'data+code')
        receipt.update(build_id=identity,source_pin=EMOTISCOPE_DONOR_COMMIT if args.emotiscope_port else PIN,bsp_pin=BSP_PIN,flags=SCALAR+' '+SAFETY+' '+optimisation,profile=args.profile,asrc_tcm=args.asrc_tcm,debug=args.debug,debug_info=bool(args.debug_info),
                       resident=bool(args.resident_controls),npu=bool(args.npu_model),p4=bool(args.p4_source),stage_profile=args.stage_profile,
                       pdm_target=args.pdm_target,pcm1808_target=args.pcm1808_target,dcache=args.dcache,
                       palette_runtime=args.palette_runtime,palette_autostart=args.palette_autostart,palette_morph=args.palette_morph,palette_ws2816=args.palette_ws2816,palette_ws2816_gpt_pair=args.palette_ws2816_gpt_pair,palette_gpt_dma=args.palette_gpt_dma,
                       dmac_priority=args.dmac_priority,
                       dmac_lane_map=args.dmac_lane_map,
                       titan_top_output=args.titan_top_output,
                       tempo_placement=args.tempo_placement,
                       product_output_chain=bool(args.palette_runtime and not args.emotiscope_port),
                       live_runtime=args.live_runtime,
                       live_emit=args.live_emit,
                       inhouse_analysis24=bool(args.inhouse_analysis24),
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
        assert text.count('-mfpu=fpv5-sp-d16')==1
        if args.profile == 'titan-product':
            text=text.replace('-mfpu=fpv5-sp-d16','-mfpu=auto')
        else:
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
        else:
            if args.pdm_target:
                defines.extend([
                    '-DK1_PDM_TARGET_RISE_DMA_CHANNEL=0u',
                    '-DK1_PDM_TARGET_RISE_DMA_IRQ=DMAC0_INT_IRQn',
                    '-DK1_PDM_TARGET_FALL_DMA_CHANNEL=1u',
                    '-DK1_PDM_TARGET_FALL_DMA_IRQ=DMAC1_INT_IRQn',
                ])
            if args.palette_gpt_dma or args.palette_ws2816_gpt_pair:
                defines.extend([
                    '-DK1_WS281X_GPT_DMA_CHANNEL=2u',
                    '-DK1_WS281X_GPT_DMA_IRQ=DMAC2_INT_IRQn',
                ])
            if args.palette_ws2816_gpt_pair:
                defines.extend([
                    '-DK1_WS281X_GPT_DMA_CHANNEL_A1=3u',
                    '-DK1_WS281X_GPT_DMA_IRQ_A1=DMAC3_INT_IRQn',
                ])
        owners = []
        for item in defines:
            if 'DMA_CHANNEL=' in item:
                owners.append(item.split('=', 1)[1])
        if len(owners) != len(set(owners)):
            raise RuntimeError('DMAC channel collision in compiler defines: ' + ' '.join(defines))
        if args.palette_gpt_dma: defines.append('-DK1_PALETTE_GPT_DMA=1')
        if args.palette_ws2816_gpt_pair: defines.append('-DK1_PALETTE_WS2816_GPT_PAIR=1')
        if args.dual_edge_donor: defines.append('-DK1_DUAL_EDGE_DONOR=1')
        if args.emotiscope_port: defines.append('-DK1_EMOTISCOPE_PORT=1')
        if args.inhouse_analysis24:
            if not (args.emotiscope_port and args.pdm_target and args.live_runtime):
                raise RuntimeError('--inhouse-analysis24 needs --emotiscope-port --pdm-target --live-runtime')
            defines.append('-DK1_INHOUSE_ANALYSIS24=1')
        if args.titan_top_output: defines.append('-DK1_TITAN_TOP_OUTPUT=1')
        if args.palette_runtime: defines.append('-DK1_PALETTE_RUNTIME=1')
        if args.palette_morph: defines.append('-DK1_PALETTE_MORPH=1')
        if args.palette_autostart: defines.append('-DK1_PALETTE_AUTOSTART=1')
        if args.palette_ws2816: defines.append('-DK1_PALETTE_WS2816=1')
        if args.npu_model: defines.append('-DK1_NPU_LOAD=1')
        if args.p4_source: defines.append('-DK1_P4_LOAD=1')
        if args.pdm_target:
            # Four fixed slots cover observed 12.259 ms service gaps at 7.4 ms
            # per block; DMA allocation and interrupt priorities are unchanged.
            defines.extend(['-DK1_PDM_TARGET=1', '-DK1_PDM_STREAM_SLOT_COUNT=4u'])
        if args.pcm1808_target: defines.append('-DK1_PCM1808_TARGET=1')
        if args.p601_loopback: defines.append('-DK1_P601_LOOPBACK=1')
        if args.live_runtime: defines.append('-DK1_LIVE_RUNTIME=1')
        if args.live_emit: defines.append('-DK1_LIVE_EMIT=1')
        if args.live_defaults:
            defaults=json.loads(args.live_defaults.read_text())
            if not isinstance(defaults, dict) or defaults.get('mode_a') is None:
                raise RuntimeError('live defaults must name mode_a')
            boot_mode=int(defaults['mode_a'])
            defines.append(f'-DK1_LIVE_BOOT_MODE={boot_mode}u')
            receipt['live_boot_mode']=boot_mode
            receipt['live_defaults']=str(args.live_defaults)
            emit_default=defaults.get('emit_default')
            if emit_default not in ('on','off'):
                raise RuntimeError('live defaults must name emit_default on or off')
            if emit_default == 'on' and not args.live_emit:
                raise RuntimeError('boot-on defaults require live emitter capability')
            defines.append(f'-DK1_LIVE_BOOT_EMIT={1 if emit_default == "on" else 0}')
            receipt['live_emit_default']=emit_default
            brightness=defaults.get('brightness')
            if type(brightness) is not int or not 100 <= brightness <= 255:
                raise RuntimeError('live boot brightness must be an integer in 100..255 (user mandate)')
            defines.append(f'-DK1_LIVE_BOOT_BRIGHTNESS={brightness}u')
            receipt['live_boot_brightness']=brightness
            composition=validate_composition(defaults,json.loads(schema),schema)
            if composition is not None:
                if not (args.emotiscope_port and args.titan_top_output and args.palette_autostart and args.live_emit and args.palette_ws2816_gpt_pair):
                    raise RuntimeError('composition requires donor, top output, palette autostart and live pair emitter')
                generated=render_composition(composition)
                (stage/'src/live_boot_composition.h').write_text(generated)
                defines.append('-DK1_LIVE_BOOT_COMPOSITION=1')
                receipt['live_boot_composition']={'settings':composition,'generated_sha256':hashlib.sha256(generated.encode()).hexdigest(),'consumer':'owning setters after live_runtime.initialise'}
            if args.emotiscope_port:
                if emit_default != 'on':
                    raise RuntimeError('Emotiscope port must restore autonomous emission on application return')
                if composition is None and (boot_mode != 1 or defaults.get('mode_b') != 1):
                    raise RuntimeError('initial Emotiscope profile must select donor Spectrum mode 1 on both controls')
                if composition is None and (type(defaults.get('donor_palette')) is not int or defaults['donor_palette'] != 1):
                    raise RuntimeError('Emotiscope boot defaults must bind curated palette 1 Sunset_Real_gp')
                receipt['emotiscope_port']['boot_donor_palette'] = 1 if composition is None else [composition['boards'][str(i)]['palette'] for i in range(2)]
                receipt['emotiscope_port']['boot_palette_consumer'] = ('source-enforced k1_emotiscope_init; palette_a/b retain compatibility metadata' if composition is None else 'generated full composition owning setters')
                receipt['emotiscope_port']['global_gain'] = f'{brightness}/255 once in final hardware packer'
        def _define_value(name):
            prefix = f'-D{name}='
            for item in defines:
                if item.startswith(prefix):
                    return item.split('=', 1)[1]
            return None
        receipt['dmac_resolved'] = {
            'lane_map': args.dmac_lane_map,
            'pdm_rise': _define_value('K1_PDM_TARGET_RISE_DMA_CHANNEL'),
            'pdm_fall': _define_value('K1_PDM_TARGET_FALL_DMA_CHANNEL'),
            'led_a': _define_value('K1_WS281X_GPT_DMA_CHANNEL'),
            'led_b': _define_value('K1_WS281X_GPT_DMA_CHANNEL_A1'),
            'pdm_rise_irq': _define_value('K1_PDM_TARGET_RISE_DMA_IRQ'),
            'pdm_fall_irq': _define_value('K1_PDM_TARGET_FALL_DMA_IRQ'),
            'led_a_irq': _define_value('K1_WS281X_GPT_DMA_IRQ'),
            'led_b_irq': _define_value('K1_WS281X_GPT_DMA_IRQ_A1'),
        }
        irq_for_ch = {
            '0u': 'DMAC0_INT_IRQn',
            '1u': 'DMAC1_INT_IRQn',
            '2u': 'DMAC2_INT_IRQn',
            '3u': 'DMAC3_INT_IRQn',
        }
        for ch_name, irq_name in (
            ('K1_PDM_TARGET_RISE_DMA_CHANNEL', 'K1_PDM_TARGET_RISE_DMA_IRQ'),
            ('K1_PDM_TARGET_FALL_DMA_CHANNEL', 'K1_PDM_TARGET_FALL_DMA_IRQ'),
            ('K1_WS281X_GPT_DMA_CHANNEL', 'K1_WS281X_GPT_DMA_IRQ'),
            ('K1_WS281X_GPT_DMA_CHANNEL_A1', 'K1_WS281X_GPT_DMA_IRQ_A1'),
        ):
            ch = _define_value(ch_name)
            irq = _define_value(irq_name)
            if ch is None and irq is None:
                continue
            if irq_for_ch.get(ch) != irq:
                raise RuntimeError(f'{ch_name}={ch} requires {irq_for_ch.get(ch)}, got {irq}')
        if defines:
            assert text.count('-Dgcc')==1
            text=text.replace('-Dgcc',' '.join(defines)+' -Dgcc')
        if args.asrc_tcm:
            assert text.count('-Dgcc')==1
            text=text.replace('-Dgcc','-DK1_ASRC_TCM=1 '+('-DK1_ASRC_ITCM=1 ' if args.asrc_tcm=='data+code' else '')+'-Dgcc')
        if args.dcache=='disabled':
            assert text.count('-Dgcc')==1
            text=text.replace('-Dgcc','-DK1_FORCE_DCACHE_OFF=1 -Dgcc')
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
        if args.titan_top_output:
            stage_top_output_vectors(stage)
        for name in PLATFORM_FILES:
            shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.titan_top_output:
            for name in TOP_PLATFORM_FILES:
                shutil.copy2(ROOT/'platform/ra8p1'/name, stage/'src'/name)
            scon = stage/'src/SConscript'
            text = scon.read_text()
            assert text.count('src += ws281x_diagnostic') == 1
            scon.write_text(text.replace('src += ws281x_diagnostic',
                            "src += ws281x_diagnostic + Glob('ws281x_gpt_dma_hw_top.c')"))
        if args.pdm_target:
            for name in PDM_TARGET_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
            if args.inhouse_analysis24:
                for name in ANALYSIS24_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
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
        if args.dual_edge_donor:
            for name in ('k1_dual_edge_donor.cpp','k1_dual_edge_donor.h'):
                shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
            shutil.copytree(ROOT/'platform/ra8p1/donor_fixed',stage/'src/donor_fixed')
            receipt['dual_edge_donor']={'enabled':True,'pixels_per_din':80,'physical_strips':1,'logical_pixels':160,'mapping':'DIN-A=0..79; DIN-B=80..159; same selected scene','renderer':'selected donor scene (Bloom default; Waveform-Fast alternate)','global_gain':str(receipt.get('live_boot_brightness',100))+'/255 once in packer','period_us':16667,'historical_reconstruction':False}
        if args.emotiscope_port:
            for name in EMOTISCOPE_PLATFORM_FILES:
                shutil.copy2(ROOT/'platform/ra8p1'/name, stage/'src'/name)
            if args.titan_top_output:
                for name in EMOTISCOPE_SECONDARY_FILES:
                    shutil.copy2(ROOT/'platform/ra8p1'/name, stage/'src'/name)
            for name in ('emotiscope_donor', 'emotiscope_shims'):
                shutil.copytree(ROOT/'platform/ra8p1'/name, stage/'src'/name)
            scon = stage/'src/SConscript'
            text = scon.read_text()
            marker = "CPPPATH=[cwd, "
            assert text.count(marker) == 1, 'Emotiscope include insertion point changed'
            text = text.replace(marker, "CPPPATH=[cwd, os.path.join(cwd, 'emotiscope_shims'), os.path.join(cwd, 'emotiscope_donor'), ")
            marker = "LOCAL_CXXFLAGS=' "
            assert text.count(marker) == 1, 'Emotiscope C++ flag insertion point changed'
            text = text.replace(marker, marker + EMOTISCOPE_CXXFLAGS + ' ')
            scon.write_text(text)
            receipt['emotiscope_port']['staged_sconscript_sha256'] = hashlib.sha256(scon.read_bytes()).hexdigest()
            receipt['emotiscope_port']['k1_compatibility_source_pin'] = PIN
            # A concurrent source edit must not produce an image with stale identity.
            for path in port_material:
                if path in (EMOTISCOPE_MANIFEST, EMOTISCOPE_CLOCK_DELTA):
                    continue
                staged = stage/'src'/path.relative_to(ROOT/'platform/ra8p1')
                expected = receipt['sources'][str(path.relative_to(ROOT))]
                assert hashlib.sha256(staged.read_bytes()).hexdigest() == expected, f'Emotiscope staging race: {path}'
        if args.palette_morph:
            for name in PALETTE_MORPH_FILES: shutil.copy2(ROOT/'platform/ra8p1'/name,stage/'src'/name)
        if args.live_runtime:
            for name in LIVE_RUNTIME_FILES:
                if args.emotiscope_port and name == 'k1_live_schema.inc':
                    (stage/'src'/name).write_text(schema_include)
                    receipt['live_schema']['generated_include_sha256'] = hashlib.sha256(
                        (stage/'src'/name).read_bytes()).hexdigest()
                else:
                    shutil.copy2(ROOT/'platform/ra8p1'/name, stage/'src'/name)
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
        (stage/'src/build_identity.h').write_text(f'#define K1_BUILD_ID "{identity}"\n#define K1_SOURCE_PIN "{receipt["source_pin"]}"\n')
        for name in names:
            target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(ROOT/'src/k1'/name,target)
        if args.tempo_field:
            for name in TEMPO_FIELD_LOCAL_FILES:
                target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(ROOT/'src/k1'/name,target)
            receipt['tempo_field_adaptation']=receipt['imports']
        for name in RA8P1_LOCAL_K1_FILES:
            target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(ROOT/'src/k1'/name,target)
        if args.emotiscope_port:
            for name in INHOUSE_WAVEFORM_FILES:
                target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(ROOT/'src/k1'/name,target)
            receipt['inhouse_waveform_sources']={'files':INHOUSE_WAVEFORM_FILES,'mode_ids':[32,18,8,7,11],'precision_bits':16,'pixels_per_board':160}
        if args.palette_runtime:
            for name in PRODUCT_OUTPUT_CHAIN_FILES:
                target=stage/'src/k1'/name; target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(ROOT/'src/k1'/name,target)
            receipt['product_output_chain_sources']={
                'status':'local-derivative',
                'dualmcu_pin':PIN,
                'byte_identical':False,
                'files':PRODUCT_OUTPUT_CHAIN_FILES,
                'executed_by_live_owner':not args.emotiscope_port,
            }
        if args.palette_morph:
            receipt['palette_derivative_sources']=apply_palette_overlay(stage/'src/k1')
        if args.tempo_acf_slice:
            receipt['tempo_acf_slice_sources']=apply_tempo_acf_slice_overlay(stage/'src/k1')
        # Probes must match imported ACF text. DTCM then rewrites the arrays.
        if args.stage_profile:
            receipt['instrumented_sources']=instrument_stage_sources(stage/'src/k1')
        if args.tempo_placement:
            placement=apply_tempo_placement(stage/'src/k1', args.tempo_placement)
            receipt['tempo_placement_sources']=placement
            if args.tempo_placement=='dtcm':
                receipt['tempo_dtcm_sources']=placement['overlay']
            verify_tempo_placement_receipt(receipt)
        macros=command([TOOLCHAIN/'arm-none-eabi-g++',*SCALAR.split(),'-dM','-E','-x','c++','/dev/null'])
        (args.output/'compiler-macros.txt').write_text(macros)
        if args.profile == 'resident-scalar':
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
        if args.profile == 'resident-scalar':
            # Historical: scalar VFP s/d registers allowed; Q-register MVE/tail predication not.
            assert_scalar_generated_code(dump,attrs)
        else:
            conversions = assert_product_generated_code(dump,attrs,macros)
            receipt['product_profile_checks'] = {
                'mve_attribute': True, 'mve_macro': 3,
                'software_double_arithmetic_call_sites': 0,
                'int64_conversion_call_sites': conversions,
                'vector_instructions': len(re.findall(r'\t(?:v\w+(?:\.\w+)?\s+[^\n]*\bq[0-7]\b|(?:vctp|vpst|wlstp|dlstp|letp)(?:\.\w+)?\b)', dump))}
        active_symbols = (['k1_emotiscope_init', 'k1_emotiscope_push_pcm64', 'k1_emotiscope_render',
                           'k1_emotiscope_status', 'LiveAudioRuntime::consume', 'PaletteRuntime::step']
                          if args.emotiscope_port else
                          ['AudioPipeline::process', 'renderProductChannel', 'k1_fixture_consume', 'k1_fixture_initialise'])
        for symbol in active_symbols:
            assert symbol in dump, f'missing selected runtime symbol {symbol}'
        if args.emotiscope_port:
            donor_objects = [stage/'build/src/emotiscope_port.o']
            if args.titan_top_output:
                donor_objects.extend([stage/'build/src/emotiscope_secondary.o', stage/'build/src/emotiscope_ml.o'])
            undefined = '\n'.join(command([TOOLCHAIN/'arm-none-eabi-nm', '-u', '-C', obj])
                                  for obj in donor_objects)
            (args.output/'emotiscope-object-undefined.txt').write_text(undefined)
            assert not re.search(r'\b(?:malloc|calloc|realloc|free|heap_caps_malloc|heap_caps_free)\b|operator new',
                                 undefined), 'Emotiscope object directly references an allocator'
            receipt['emotiscope_port']['allocator_check'] = {
                'direct_allocator_references': False,
                'objects': [obj.name for obj in donor_objects],
                'scope': 'undefined symbols in the donor wrapper objects; not a target allocation trace'}
            # Presence alone is not an execution claim. Require the compiled live
            # owner to call the donor and to omit the old AP invocation.
            consume = re.search(r'^[0-9a-f]+ <[^\n]*LiveAudioRuntime::consume[^\n]*>:\n(.*?)(?=^[0-9a-f]+ <|\Z)',
                                dump, re.MULTILINE | re.DOTALL)
            assert consume, 'compiled live consumer missing'
            assert 'k1_emotiscope_push_pcm64' in consume.group(1), 'live consumer does not call Emotiscope AP'
            assert 'AudioPipeline::process' not in consume.group(1), 'legacy K1 AP remains active in live consumer'
            step = re.search(r'^[0-9a-f]+ <[^\n]*PaletteRuntime::step[^\n]*>:\n(.*?)(?=^[0-9a-f]+ <|\Z)',
                             dump, re.MULTILINE | re.DOTALL)
            assert step and 'k1_emotiscope_render' in step.group(1), 'palette owner does not call Emotiscope VP'
            assert 'renderProductChannel' not in step.group(1), 'legacy K1 VP remains active in palette owner'
            if args.titan_top_output:
                for symbol in ('k1_emotiscope_audio_publication',
                               'k1_secondary_emotiscope_accept_audio',
                               'k1_secondary_emotiscope_render'):
                    assert symbol in step.group(1), f'palette owner missing secondary publication/VP call: {symbol}'
                assert 'k1_secondary_emotiscope_push_pcm64' not in dump, 'secondary PCM/AP ingress linked into top image'
            receipt['emotiscope_port']['compiled_live_callchain'] = {
                'consumer_calls_donor_ap': True, 'consumer_calls_legacy_ap': False,
                'palette_calls_donor_vp': True, 'palette_calls_legacy_vp': False,
                'palette_calls_secondary_vp': bool(args.titan_top_output),
                'secondary_consumes_primary_publication': bool(args.titan_top_output),
                'scope': 'compiled owner function call sites; not target timing or optical proof'}

        if not args.palette_gpt_dma and not args.palette_ws2816_gpt_pair:
            assert 'k1_ws281x_diag_emit' in dump, 'GPIO diagnostic emitter was dropped'
        if args.palette_runtime:
            pack_symbol = 'PaletteRuntime::packBenchGrb48Lane' if (args.palette_ws2816 or args.palette_ws2816_gpt_pair) else 'PaletteRuntime::packBenchGrb('
            for symbol in ['PaletteRuntime::configure','PaletteRuntime::step','PaletteRuntime::catalogueJson',pack_symbol]:
                assert symbol in dump, f'missing native palette runtime symbol {symbol}'
        if args.resident_controls:
            for symbol in ['k1_fixture_schedule_step','k1_resident_pcm','k1_resident_crc','k1_resident_length']:
                assert symbol in symbols, f'missing resident schedule symbol {symbol}'
        if args.tempo_placement:
            receipt['tempo_placement_evidence']=verify_tempo_elf_placement(
                symbols, args.tempo_placement, scratch_only=args.emotiscope_port and not args.asrc_tcm)
        if args.asrc_tcm:
            placed={}
            for line in symbols.splitlines():
                parts=line.split()
                if len(parts)>=3 and parts[-1] in ('k1_asrc24_push','capture_asrc'):
                    placed[parts[-1]]=int(parts[0],16)
            if args.asrc_tcm=='data+code':
                assert placed.get('k1_asrc24_push',-1) in range(0x00000000,0x00020000), 'ASRC push not in ITCM'
            else:
                assert placed.get('k1_asrc24_push',-1) not in range(0x00000000,0x00020000), 'data-only build placed code in ITCM'
            assert placed.get('capture_asrc',-1) in range(0x20000000,0x20020000), 'ASRC state not in DTCM'
            receipt['asrc_tcm_evidence']={k:f'0x{v:08x}' for k,v in placed.items()}
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
            pins=(ROOT/'platform/ra8p1/titan_led_pins.h').read_text()
            pair_src=(ROOT/'platform/ra8p1/ws281x_gpt_dma_hw_pair.c').read_text()
            palette_src=(ROOT/'platform/ra8p1/palette_runtime.cpp').read_text()
            assert 'P604' in pins and 'GPT8' in pins, 'DIN-B GPT pin header is not P604/GPT8'
            assert '.channel = 8u' in pair_src and 'K1_TITAN_DIN_B_GPT' in pair_src
            assert 'din_b\\":\\"P604' in palette_src
            receipt['pair_allocation']={
                'lane_a0':{'pin':'P601','gpt_pwm':6,'stop_gpt':0,
                           'dma':0 if args.dmac_lane_map=='led-first' else 2,
                           'elc':['GPT_A','GPT_B']},
                'lane_a1':{'pin':'P604','gpt_pwm':8,'stop_gpt':1,'dma':3,
                           'elc':['GPT_C','GPT_D']},
                'profile':3,
                'gtstr_mask':'GPT6|GPT8',
                'source_precision':('donor_post_gamma_temporal_dither_rgb8_to_rgb16' if args.emotiscope_port
                                    else 'donor_q15_16_to_rgb16' if args.dual_edge_donor else 'rgb8_to_grb48'),
                'pixels_per_din':80,
            }
        if args.titan_top_output:
            for symbol in ('k1_ws281x_gpt_dma_hw_top_init',
                           'k1_ws281x_gpt_dma_hw_top_submit',
                           'k1_ws281x_gpt_dma_hw_top_poll'):
                assert symbol in dump, f'missing secondary execution symbol {symbol}'
            receipt['top_allocation'] = {
                'lane_a': {'pin': 'P603', 'gpt_pwm': 7, 'output': 'A',
                           'stop_gpt': 3, 'dma': 6, 'elc': ['GPT_E', 'GPT_F']},
                'lane_b': {'pin': 'P714', 'gpt_pwm': 12, 'output': 'B',
                           'stop_gpt': 4, 'dma': 7, 'elc': ['GPT_G', 'GPT_H']},
                'pixels_per_din': 80,
                'enabled_default': False,
                'checkpoint': 'SOURCE_BOUND_ONLY_NO_TOP_ACTIVATION_PROOF',
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
                'profile':'ap_measured_source_asrc12k8_emotiscope' if args.emotiscope_port else 'ap_40k_asrc24',
                'sample_rate_hz':40000,
                'working_source_sample_rate_hz':12800 if args.emotiscope_port else 24000,
                'analysis_hop_samples':64 if args.emotiscope_port else 180,
                'analysis_hop_period_us':5000 if args.emotiscope_port else 7500,
                'analysis_contract_version':2 if args.emotiscope_port else 1,
                'sample_rate_parity':False,
                'admitted_to_24k_ap':not args.emotiscope_port,
                'slot_elements':296,
                'slot_duration_us':7500,
                'dma_interrupt_threshold_samples':8,
                'dma_completion_isr':'k1_pdm_target_dmac_isr',
                'stock_pdm_rxi_dmac_isr':'not_used',
                'slot_count':4,
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
                'emotiscope_12k8_compiled':bool(args.emotiscope_port),
                'physical_capture_admission':'NOT_RUN' if args.emotiscope_port else 'unchanged',
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
                'analysis_engine':'Emotiscope v1.1_build' if args.emotiscope_port else 'K1 AudioPipeline',
                'contract':'sr12800.hop64.bins64.emotiscope' if args.emotiscope_port else 'sr24000.hop180.bins80.xover40',
                'mailbox':'removed',
                'emit_default':receipt.get('live_emit_default', 'on' if args.live_emit else 'off'),
                'emit_capable':bool(args.live_emit),
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
        if args.tempo_placement:
            receipt['tempo_placement_evidence']['size']=receipt['size']
        receipt['artifacts']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.is_file()}
        if args.emotiscope_port:
            for path in material:
                if not path.is_file():
                    raise RuntimeError(f'build input disappeared: {path}')
                try:
                    key = str(path.relative_to(ROOT))
                except ValueError:
                    key = 'external/live_defaults.json' if path == args.live_defaults else None
                if key in receipt['sources']:
                    assert hashlib.sha256(path.read_bytes()).hexdigest() == receipt['sources'][key], f'source changed during Emotiscope build: {path}'
        receipt['pass']=True
        print(json.dumps(dict(build_id=identity,size=receipt['size'],**{'pass':True}),indent=2))
    except Exception as error:
        receipt['error']=str(error)
        raise
    finally:
        receipt['end']=datetime.now(timezone.utc).isoformat()
        (args.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__': main()
