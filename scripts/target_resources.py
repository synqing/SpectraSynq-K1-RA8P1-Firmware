#!/usr/bin/env python3
"""Shared fail-closed resource acceptance for identified target campaigns."""


def validate_resources(before, after, acceptance, label):
    if before['heap_total'] != after['heap_total']:
        raise RuntimeError(f'{label} heap pool changed during measured schedule')
    if (after['stack_untouched_bytes'] < acceptance['stack_untouched_min_bytes'] or
        after['heap_total'] - after['heap_maximum'] < acceptance['heap_free_at_maximum_min_bytes']):
        raise RuntimeError(f'{label} resource reserve failed')
    if (after['heap_used'] - before['heap_used'] > acceptance['heap_used_growth_max_bytes'] or
        after['heap_maximum'] - before['heap_maximum'] > acceptance['heap_maximum_growth_max_bytes']):
        raise RuntimeError(f'{label} heap growth detected during measured schedule')


def validate_cache_state(metrics, expected, label):
    if expected not in ('disabled', 'enabled'):
        raise RuntimeError(f'{label} cache expectation missing or divergent')
    required = ('scb_ccr', 'dcache_enabled', 'icache_enabled')
    if any(name not in metrics for name in required):
        raise RuntimeError(f'{label} cache readback missing')
    ccr = metrics['scb_ccr']
    if not isinstance(ccr, int) or isinstance(ccr, bool) or ccr < 0:
        raise RuntimeError(f'{label} CCR readback invalid')
    dcache = bool(ccr & (1 << 16))
    icache = bool(ccr & (1 << 17))
    if metrics['dcache_enabled'] is not dcache or metrics['icache_enabled'] is not icache:
        raise RuntimeError(f'{label} cache readback inconsistent')
    if dcache != (expected == 'enabled'):
        raise RuntimeError(f'{label} D-cache mode mismatch')
