#!/usr/bin/env python3
"""HOST proof for the stage-profile software-double counter."""
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

REQUIRED_WRAPS = (
    'WRAP_DD(__aeabi_dadd)',
    'WRAP_DD(__aeabi_dsub)',
    'WRAP_DD(__aeabi_dmul)',
    'WRAP_DD(__aeabi_ddiv)',
    'WRAP_DD(__aeabi_drsub)',
    'WRAP_D_F(__aeabi_d2f)',
    'WRAP_D_I(__aeabi_d2iz)',
    '__wrap___aeabi_d2ulz',
    'WRAP_CMP(__aeabi_dcmpeq)',
    'WRAP_CMP(__aeabi_dcmplt)',
    'WRAP_CMP(__aeabi_dcmple)',
    'WRAP_CMP(__aeabi_dcmpge)',
    'WRAP_CMP(__aeabi_dcmpgt)',
    'WRAP_CMP(__aeabi_dcmpun)',
    '__wrap___aeabi_i2d',
    '__wrap___aeabi_ui2d',
    '__wrap___aeabi_l2d',
)

source = (ROOT / 'platform/ra8p1/k1_double_probe.c').read_text()
missing = [name for name in REQUIRED_WRAPS if name not in source]
if missing:
    raise SystemExit(f'missing software-double wraps: {missing}')
neutered = source.replace('WRAP_DD(__aeabi_dmul)', 'WRAP_DD(__aeabi_dmul_removed)', 1)
neutered_missing = [name for name in REQUIRED_WRAPS if name not in neutered]
if 'WRAP_DD(__aeabi_dmul)' not in neutered_missing:
    raise SystemExit('mutation oracle blind to a missing dmul wrap')

harness = r'''
#include "k1_double_probe.h"
#include <assert.h>
#include <stdio.h>
static unsigned g_cycles;
extern "C" unsigned k1_cycle_count(void) { return ++g_cycles; }
int main() {
  unsigned calls = 1, cycles = 1;
  k1_double_probe_reset();
  k1_double_probe_snapshot(&calls, &cycles);
  assert(calls == 0 && cycles == 0);
  k1_double_probe_inject_for_test(3);
  k1_double_probe_snapshot(&calls, &cycles);
  assert(calls == 3 && cycles > 0);
  k1_double_probe_reset();
  k1_double_probe_snapshot(&calls, &cycles);
  assert(calls == 0 && cycles == 0);
  puts("K1_DOUBLE_PROBE=PASS reset=PASS inject=PASS");
  return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='k1-double-probe-') as temp:
    directory = Path(temp)
    (directory / 'harness.cpp').write_text(harness)
    executable = directory / 'test-double-probe'
    run([
        'c++', '-std=c++17', '-O2', '-DK1_DOUBLE_PROBE_HOST=1',
        '-I' + str(ROOT / 'platform/ra8p1'),
        str(directory / 'harness.cpp'),
        str(ROOT / 'platform/ra8p1/k1_double_probe.c'),
        '-o', str(executable),
    ])
    print(run([str(executable)]), end='')
