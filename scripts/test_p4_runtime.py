#!/usr/bin/env python3
"""Compile and exercise the generic P4 target scheduler on HOST."""
import argparse
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--kernels',type=Path,required=True)
parser.add_argument('--fixture',type=Path,required=True)
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='k1-p4-runtime-') as temp:
    executable=Path(temp)/'test'
    kernel_object=Path(temp)/'kernels.o'
    run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math',
         '-I'+str(args.kernels),'-c',str(args.kernels/'kernels.c'),'-o',str(kernel_object)])
    run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-DK1_NPU_LOAD=1','-DK1_P4_LOAD=1',
         '-I'+str(ROOT/'platform/ra8p1'),'-I'+str(args.kernels),'-I'+str(args.fixture.parent),
         str(ROOT/'tests/host/test_p4_runtime.cpp'),str(ROOT/'platform/ra8p1/p4_runtime.cpp'),
         str(ROOT/'platform/ra8p1/semantic_sidecar.cpp'),str(kernel_object),'-o',str(executable)])
    print(run([str(executable)]),end='')
