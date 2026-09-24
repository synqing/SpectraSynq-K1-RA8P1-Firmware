#!/usr/bin/env python3
"""DUR-010 evidence, compiled and run twice: this project's normal Titan
host flags, and the same flags plus -ffast-math. hd_pixel16.h's finiteness
test is bit-level (isFiniteBits), not std::isfinite(), specifically so the
second run is not a silent behaviour change: -ffast-math implies
-ffinite-math-only, under which std::isfinite() is permitted to fold to a
constant true, which would flip quantiseHd16's +Inf case from 0 to 65535
with no diagnostic. Both runs must print the identical PASS line.
"""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

BASE_FLAGS = ['-std=c++17', '-O2', '-ffp-contract=off', '-fno-fast-math']
SOURCES = [
    'tests/host/test_hd_pixel16.cpp',
    'src/k1/core/visual/product_palette.cpp',
    'src/k1/core/visual/product_palette_data.cpp',
    'src/k1/core/visual/wide/wide_endpoint.cpp',
]

with tempfile.TemporaryDirectory(prefix='k1-hd-pixel16-') as temp:
    directory = Path(temp)
    outputs = {}
    for label, extra_flags in [('normal', []), ('fast_math', ['-ffast-math'])]:
        executable = directory / label
        # -fno-fast-math from BASE_FLAGS is deliberately last-wins-overridden
        # by an explicit -ffast-math for the fast_math build: GCC/Clang both
        # take the last of a repeated -f[no-]fast-math pair.
        command = ['c++', *BASE_FLAGS, *extra_flags,
                   '-I' + str(ROOT / 'platform/ra8p1'), '-I' + str(ROOT / 'src/k1'),
                   *[str(ROOT / s) for s in SOURCES], '-o', str(executable)]
        run(command)
        outputs[label] = run([str(executable)])
        print(f'{label.upper()} {outputs[label]}', end='')
    assert outputs['normal'] == outputs['fast_math'], (
        f"-ffast-math changed hd_pixel16.h's observable behaviour: "
        f"normal={outputs['normal']!r} fast_math={outputs['fast_math']!r}")
    print('HD_PIXEL16_FASTMATH_INVARIANT=PASS normal_and_fast_math_identical=true')
