"""Positive and corruption tests for the post-gain, two-DIN snapshot scorer."""
import struct
import unittest
from palette_wire_snapshot import score_snapshot


def fixture(brightness=128):
    native = b''.join(bytes((i % 80, 255-i % 80, 31)) for i in range(160))
    wire = b''.join(struct.pack('>3H', *(native[p*3+i]*257*brightness//255 for i in (1, 0, 2)))
                    for p in range(160))
    return struct.pack('<16I', 1, 1, 0, brightness, 2, 80, 48, 4, 100, 33, 23, 16667, 0, 0, 99, 0)+native+wire


class SnapshotTests(unittest.TestCase):
    def test_native16_words(self):
        # Native words off the x257 lattice: the lift law fails them, the
        # native16 scorer accepts them and reports the precision evidence.
        payload = bytearray(fixture(255))
        struct.pack_into('>H', payload, 544, struct.unpack_from('>H', payload, 544)[0] ^ 1)
        struct.pack_into('>H', payload, 544+6, struct.unpack_from('>H', payload, 544+6)[0] ^ 3)
        self.assertFalse(score_snapshot(bytes(payload))['pass_wire'])
        native = score_snapshot(bytes(payload), native16=True)
        self.assertTrue(native['pass_wire'])
        self.assertTrue(native['native16'])
        self.assertEqual(native['offlattice_words'], 2)
        legacy = score_snapshot(fixture(255))
        self.assertEqual((legacy['offlattice_words'], legacy['native16']), (0, False))
        # A failed emitter status still fails the native path.
        failed = bytearray(payload); struct.pack_into('<I', failed, 48, 7)
        self.assertFalse(score_snapshot(bytes(failed), native16=True)['pass_wire'])

    def test_positive_and_black(self):
        for brightness in (0, 24, 128, 255):
            result = score_snapshot(fixture(brightness))
            self.assertTrue(result['pass_wire'])
            self.assertEqual(result['lane_nonzero_counts'], [80, 80] if brightness else [0, 0])

    def test_both_lanes_and_low_bytes_checked(self):
        for offset in (544, 545, 1024, 1503):
            changed = bytearray(fixture()); changed[offset] ^= 1
            self.assertFalse(score_snapshot(changed)['pass_wire'])

    def test_failed_emitter_is_not_pass(self):
        for offset in (48, 52):
            changed = bytearray(fixture()); struct.pack_into('<I', changed, offset, 7)
            self.assertFalse(score_snapshot(changed)['pass_wire'])

    def test_invalid_layout(self):
        for bad in (b'', fixture()[:-1], fixture()+b'x'):
            with self.assertRaises(ValueError): score_snapshot(bad)
        for offset, value in ((0, 2), (8, 2), (12, 256), (16, 1), (20, 128), (36, 44)):
            changed = bytearray(fixture()); struct.pack_into('<I', changed, offset, value)
            with self.assertRaises(ValueError): score_snapshot(changed)


if __name__ == '__main__':
    unittest.main()
