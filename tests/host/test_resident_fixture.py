import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).parents[2]/'scripts'))
from pack_schedule_fixture import pack,unpack


class ResidentFixtureTests(unittest.TestCase):
    def test_lossless_repeated_hops(self):
        pcm=bytes(360)+bytes([127])*360+bytes(360)
        dictionary,indices=pack(pcm)
        self.assertEqual(len(dictionary),2)
        self.assertEqual(unpack(dictionary,indices),pcm)
        altered=list(indices); altered[0]=1
        self.assertNotEqual(unpack(dictionary,altered),pcm)
        damaged=list(dictionary); damaged[0]=b'\x01'+damaged[0][1:]
        self.assertNotEqual(unpack(damaged,indices),pcm)

    def test_bad_inputs_and_indices_rejected(self):
        for pcm in (b'',b'x',bytes(361)):
            with self.assertRaises(ValueError): pack(pcm)
        for indices in ([],[-1],[1]):
            with self.assertRaises(ValueError): unpack([bytes(360)],indices)


if __name__=='__main__': unittest.main()
