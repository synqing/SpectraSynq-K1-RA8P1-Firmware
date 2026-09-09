import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).parents[2]/'scripts'))
from build_scalar import assert_scalar_generated_code

class ScalarCodegen(unittest.TestCase):
    def test_scalar_vfp_and_literal_pool_are_not_mve(self):
        assert_scalar_generated_code(' 200:	eee0 0a20\tvadd.f32 s0, s0, s1\n 20301e4:\te21d 1ddc a6e2 d62a 3771 3cca be44 5207     ......*.q7.<D..R\n','Tag_FP_arch: FPv5/FP-D16')

    def test_mve_instruction_rejected(self):
        for instruction in ['vldrw.u32 q0, [r1]', 'vadd.f32 q2, q0, q1', 'vpst', 'vctp.32 r0', 'wlstp.32 lr, r0, 200']:
            with self.assertRaisesRegex(AssertionError,'vector'):
                assert_scalar_generated_code(' 200:\teee0 0a20\t'+instruction+'\n','')

    def test_mve_elf_attribute_rejected(self):
        with self.assertRaisesRegex(AssertionError,'MVE'):
            assert_scalar_generated_code('', 'Tag_MVE_arch: MVE Integer and FP')
