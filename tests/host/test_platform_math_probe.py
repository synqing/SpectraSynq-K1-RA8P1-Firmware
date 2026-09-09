import ctypes
import math
from pathlib import Path
import random
import struct
import subprocess
import tempfile
import unittest


class PlatformMathProbeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        lib=Path(cls.temp.name)/'probe.dylib'
        subprocess.run(['c++','-std=c++17','-O2','-ffp-contract=off','-dynamiclib',
                        str(Path(__file__).with_name('arm_logf_probe.cpp')),'-o',str(lib)],check=True)
        cls.lib=ctypes.CDLL(str(lib))
        for name in ('logf','expf','log2f'):
            fn=getattr(cls.lib,name); fn.argtypes=[ctypes.c_float]; fn.restype=ctypes.c_float

    def test_independent_double_precision_reference(self):
        # Documented Arm single-function <1 ULP bounds, not an AP tolerance.
        rng=random.Random(142)
        def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
        def ordered(x):
            bits=struct.unpack('<I',struct.pack('<f',x))[0]
            return (~bits & 0xffffffff) if bits&0x80000000 else bits|0x80000000
        for name,reference in [('logf',math.log),('log2f',math.log2),('expf',math.exp)]:
            fn=getattr(self.lib,name)
            for i in range(10000):
                value=f32(rng.uniform(-80,80) if name=='expf' else 2**rng.uniform(-100,100))
                actual=fn(value); expected=f32(reference(value))
                self.assertLessEqual(abs(ordered(actual)-ordered(expected)),1,(name,value,actual,expected))


if __name__=='__main__': unittest.main()
