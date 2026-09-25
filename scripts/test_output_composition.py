#!/usr/bin/env python3
from pathlib import Path
import hashlib
import subprocess
import tempfile
from run_host import run
from verify_imports import PIN, REFERENCE, ROOT

DUALMCU = {
    'core/visual/frame_blend.h': '610519b4942f88e5e8ddf0f171e554b20a4befc5715b2a7cb69a04ca4d74e497',
    'core/visual/frame_blend.cpp': '03b9f47d30d8410e93dd381650d495e5047f5a222fac6e1b533dc517abe9d8e6',
    'core/visual/product_runtime_policy.h': 'ec1a5375d561673701310dc6437e11d1f3917a37c8d0c42e8ca4e12d70cff7a3',
    'core/visual/product_runtime_policy.cpp': '543fc91ab37aac87335b047807672ff930c2dd17cea68abd4c8b469a849d092f',
}

names = [
    'core/visual/frame_blend.cpp',
    'core/visual/product_runtime_policy.cpp',
    'core/visual/product_output_treatment.cpp',
]
with tempfile.TemporaryDirectory(prefix='k1-output-composition-') as temp:
    binary = Path(temp) / 'test'
    command = [
        'c++', '-std=c++17', '-O2', '-ffp-contract=off',
        '-I' + str(ROOT / 'src/k1'),
        '-I' + str(ROOT / 'platform/ra8p1'),
        str(ROOT / 'tests/host/test_output_composition.cpp'),
    ]
    command += [str(ROOT / 'src/k1' / name) for name in names]
    command += ['-o', str(binary)]
    run(command)
    print(run([str(binary)]), end='')

print(f'PRODUCT_OUTPUT_DERIVATIVE pin={PIN}')
for name, expected in DUALMCU.items():
    source = subprocess.run(
        ['git', '-C', str(REFERENCE), 'show', f'{PIN}:{name}'],
        capture_output=True, check=True).stdout
    digest = hashlib.sha256(source).hexdigest()
    if digest != expected:
        raise SystemExit(f'DualMCU identity drifted: {name}')
    local = (ROOT / 'src/k1' / name).read_bytes()
    if hashlib.sha256(local).hexdigest() == digest:
        raise SystemExit(f'{name} is byte-identical; keep it a named derivative')
    print(f'  {name} dual={digest} local_derivative=true')
print('PRODUCT_OUTPUT_DERIVATIVE=PASS')
