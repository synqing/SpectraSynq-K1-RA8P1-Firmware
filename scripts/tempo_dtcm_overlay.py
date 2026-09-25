"""Tempo hot-array placement. DualMCU files stay on disk.

empty-tcm: work/acf remain automatic storage (original timing brief).
dtcm: named historical overlay into CPU-view DTCM 0x20000000–0x20020000.
"""
from __future__ import annotations

import hashlib
import re

ITCM_CPU_VIEW = range(0x00000000, 0x00020000)
DTCM_CPU_VIEW = range(0x20000000, 0x20020000)
HOT_ARRAY_BYTES = (512 + 200) * 4
PLACEMENTS = ('empty-tcm', 'dtcm')


def apply_tempo_dtcm_overlay(root):
    path = root / 'core/audio/tempo_acf.cpp'
    before = path.read_text()
    after = before
    old = '    std::array<float, kTempoAcfHistoryLength> work{};'
    new = ('    static std::array<float, kTempoAcfHistoryLength> work '
           '__attribute__((section(".dtcm"), aligned(32)));\n'
           '    work = {};')
    if after.count(old) != 1:
        raise RuntimeError('tempo DTCM overlay work mismatch')
    after = after.replace(old, new)
    old = '    std::array<float, kAcfTableLength> acf{};'
    new = ('    static std::array<float, kAcfTableLength> acf '
           '__attribute__((section(".dtcm"), aligned(32)));\n'
           '    acf = {};')
    if after.count(old) != 1:
        raise RuntimeError('tempo DTCM overlay acf mismatch')
    after = after.replace(old, new)
    path.write_text(after)
    return {
        'core/audio/tempo_acf.cpp': {
            'before': hashlib.sha256(before.encode()).hexdigest(),
            'after': hashlib.sha256(after.encode()).hexdigest(),
        }
    }


def apply_tempo_placement(root, placement):
    if placement == 'empty-tcm':
        path = root / 'core/audio/tempo_acf.cpp'
        text = path.read_text()
        if 'section(".dtcm")' in text:
            raise RuntimeError('empty-tcm staged tempo_acf.cpp still names .dtcm')
        digest = hashlib.sha256(text.encode()).hexdigest()
        return {
            'placement': 'empty-tcm',
            'hot_array_bytes': HOT_ARRAY_BYTES,
            'residence': 'automatic-storage',
            'source_sha256': digest,
        }
    if placement == 'dtcm':
        overlay = apply_tempo_dtcm_overlay(root)
        return {
            'placement': 'dtcm',
            'hot_array_bytes': HOT_ARRAY_BYTES,
            'residence': 'section .dtcm',
            'overlay': overlay,
        }
    raise RuntimeError(f'unknown tempo placement: {placement!r}')


def verify_tempo_placement_receipt(receipt):
    placement = receipt.get('tempo_placement')
    overlay = receipt.get('tempo_dtcm_sources')
    if placement == 'empty-tcm':
        if overlay:
            raise RuntimeError('empty-tcm receipt must not record a DTCM overlay')
        return
    if placement == 'dtcm':
        if not overlay:
            raise RuntimeError('dtcm receipt missing tempo_dtcm_sources')
        return
    raise RuntimeError(f'unknown tempo placement: {placement!r}')


def _hot_array_addresses(symbols_text):
    hits = {'work': [], 'acf': []}
    for line in symbols_text.splitlines():
        if 'computeTempoAcfAtRate' not in line:
            continue
        stripped = line.strip()
        kind = None
        if stripped.endswith('::work') or re.search(r'\bwork$', stripped):
            kind = 'work'
        elif stripped.endswith('::acf') or re.search(r'\bacf$', stripped):
            kind = 'acf'
        if kind is None:
            continue
        parts = stripped.split()
        try:
            addr = int(parts[0], 16)
        except (ValueError, IndexError):
            continue
        hits[kind].append(addr)
    return hits


def verify_tempo_elf_placement(symbols_text, placement):
    hits = _hot_array_addresses(symbols_text)
    evidence = {
        'placement': placement,
        'hot_array_bytes': HOT_ARRAY_BYTES,
        'itcm': f'0x{ITCM_CPU_VIEW.start:08x}-0x{ITCM_CPU_VIEW.stop:08x}',
        'dtcm': f'0x{DTCM_CPU_VIEW.start:08x}-0x{DTCM_CPU_VIEW.stop:08x}',
        'work_addresses': [f'0x{addr:08x}' for addr in hits['work']],
        'acf_addresses': [f'0x{addr:08x}' for addr in hits['acf']],
    }
    in_itcm = [addr for addrs in hits.values() for addr in addrs if addr in ITCM_CPU_VIEW]
    in_dtcm = [addr for addrs in hits.values() for addr in addrs if addr in DTCM_CPU_VIEW]
    if placement == 'empty-tcm':
        if in_itcm or in_dtcm:
            raise RuntimeError('empty-tcm image placed tempo hot arrays in TCM')
        if hits['work'] or hits['acf']:
            raise RuntimeError('empty-tcm image exported static tempo hot arrays')
        evidence['residence'] = 'automatic-storage'
        evidence['stack_delta_bytes'] = HOT_ARRAY_BYTES
        return evidence
    if placement == 'dtcm':
        if not hits['work'] or not hits['acf']:
            raise RuntimeError('dtcm image is missing named tempo hot-array symbols')
        if in_itcm:
            raise RuntimeError('dtcm image placed tempo hot arrays in ITCM')
        if any(addr not in DTCM_CPU_VIEW for addr in hits['work'] + hits['acf']):
            raise RuntimeError('dtcm image did not place tempo hot arrays in DTCM')
        evidence['residence'] = 'section .dtcm'
        evidence['stack_delta_bytes'] = 0
        return evidence
    raise RuntimeError(f'unknown tempo placement: {placement!r}')
