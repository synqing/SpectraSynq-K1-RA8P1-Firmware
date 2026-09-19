"""Decode a binary emitted by the actual production C driver with HOST seams."""
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from gpt_fault_witness import decode_v2


class GptFaultWitness(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory(prefix='k1-fault-decode-') as temp:
            binary = Path(temp) / 'production-driver'
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(ROOT / 'tests/target_mock'),
                            '-I' + str(ROOT / 'platform/ra8p1'),
                            str(ROOT / 'tests/test_ws281x_gpt_dma_hw.c'),
                            str(ROOT / 'platform/ra8p1/ws281x_waveform.c'),
                            '-o', str(binary)], check=True)
            cls.body = subprocess.check_output([str(binary), '--witness'])

    def test_actual_fault_frame_and_hashes(self):
        result = decode_v2(self.body)
        witness = result['fault_witness']
        self.assertEqual(witness['frame_id'], 2)
        self.assertEqual(witness['packed_grb_hex'], '18b100')
        self.assertEqual(witness['terminal']['dmac_count'], 2)
        self.assertEqual(witness['elapsed_cycles'], 30000)
        self.assertEqual(result['dmctl'], 1)

    def test_mutated_layout_publication_identity_or_payload_rejected(self):
        for offset, value in ((0, 1), (4, 728), (740, 0), (740, 2),
                              (744, 1), (752, 481), (764, 23), (948, 1)):
            with self.subTest(offset=offset):
                changed = bytearray(self.body)
                struct.pack_into('<I', changed, offset, value)
                with self.assertRaises(ValueError):
                    decode_v2(changed)
        for bad in (b'', self.body[:-1], self.body + b'x'):
            with self.assertRaises(ValueError):
                decode_v2(bad)


if __name__ == '__main__':
    unittest.main()
