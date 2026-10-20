#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from run_mode32_real_audio import NATIVE_RGB_BYTES, authored_stats


class Mode32Observation(unittest.TestCase):
    def test_short_frame_is_not_black(self):
        with self.assertRaisesRegex(ValueError, 'incomplete native RGB frame'):
            authored_stats(bytes(479))

    def test_exact_length_black_is_zero(self):
        stats = authored_stats(bytes(NATIVE_RGB_BYTES))
        self.assertEqual(stats['nz'], 0)
        self.assertEqual(stats['max'], 0)
        self.assertIsNone(stats['centroid'])


if __name__ == '__main__':
    unittest.main()
