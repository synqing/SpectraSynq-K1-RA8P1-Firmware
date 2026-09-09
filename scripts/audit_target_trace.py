#!/usr/bin/env python3
"""Explain exact trace failures without changing the acceptance threshold."""
from __future__ import annotations
import argparse
import itertools
import json
import math
from pathlib import Path
import struct


def ordered_f32(value):
    bits = struct.unpack('<I', struct.pack('<f', value))[0]
    return (~bits & 0xffffffff) if bits & 0x80000000 else bits | 0x80000000


def audit(reference, candidate, prefix=False):
    result = dict(acceptance='exact; diagnostics never grant tolerance',
                  complete=not prefix, hops=0, fields=0, differing_fields=0,
                  fields_by_name={}, first_difference=None)
    keys = []
    schema = None
    with Path(reference).open() as left, Path(candidate).open() as right:
        for line, pair in enumerate(itertools.zip_longest(left, right), 1):
            a, b = pair
            if b is None and prefix:
                result['partial_fields'] = len(keys)
                break
            if a is None or b is None:
                raise ValueError(f'trace length mismatch at line {line}')
            if a == 'END\n' or b == 'END\n':
                if a != b: raise ValueError(f'frame boundary mismatch at line {line}')
                if len(keys) != 526 or len(set(keys)) != 526 or not all(f'pixel[{i}]' in keys for i in range(320)):
                    raise ValueError('incomplete/duplicate schema')
                if schema is None: schema = list(keys)
                if keys != schema: raise ValueError('schema changed')
                result['hops'] += 1
                keys = []
                continue
            ka, va = a.strip().split('=', 1)
            kb, vb = b.strip().split('=', 1)
            if ka != kb: raise ValueError(f'schema mismatch at line {line}')
            keys.append(ka)
            result['fields'] += 1
            if not math.isfinite(float(va)) or not math.isfinite(float(vb)):
                raise ValueError(f'non-finite value at line {line}')
            if va == vb: continue
            result['differing_fields'] += 1
            detail = dict(line=line, hop=result['hops']+1, field=ka, reference=va, candidate=vb)
            if result['first_difference'] is None: result['first_difference'] = detail
            # Decimal integers retain exact 64-bit arithmetic; no double rounding.
            integers = all(v.lstrip('-').isdigit() for v in (va, vb))
            x, y = (int(va), int(vb)) if integers else (float(va), float(vb))
            entry = result['fields_by_name'].setdefault(ka, dict(count=0, max_absolute=0,
                                                               max_relative=0, max_f32_ulp=0, first=detail))
            entry['count'] += 1
            entry['max_absolute'] = max(entry['max_absolute'], abs(x-y))
            entry['max_relative'] = max(entry['max_relative'], abs(x-y)/max(abs(x), abs(y), 1e-300))
            if not integers:
                entry['max_f32_ulp'] = max(entry['max_f32_ulp'], abs(ordered_f32(x)-ordered_f32(y)))
    if not result['hops'] or (keys and not prefix): raise ValueError('empty/incomplete trace')
    result['exact_pass'] = not prefix and not result['differing_fields']
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--prefix', action='store_true', help='diagnostic only; can never pass')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.reference, args.candidate, args.prefix)
    with args.output.open('x') as target: json.dump(result, target, indent=2); target.write('\n')
    print(json.dumps({k:v for k,v in result.items() if k != 'fields_by_name'}, indent=2))
    return 0 if result['exact_pass'] else 2


if __name__ == '__main__': raise SystemExit(main())
