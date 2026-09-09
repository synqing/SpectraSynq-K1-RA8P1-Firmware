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
