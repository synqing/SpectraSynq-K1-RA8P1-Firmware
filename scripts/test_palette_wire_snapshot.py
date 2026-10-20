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
    def test_top_mapping_keeps_raw_lanes_and_recovers_logical_pixels(self):
        native = b''.join(bytes((i, 255-i, 31)) for i in range(160))
        logical = b''.join(struct.pack('>3H', *(native[p*3+i]*257 for i in (1,0,2))) for p in range(160))
        wire = logical[480:] + logical[:480]
        payload = struct.pack('<16I',1,1,1,255,2,80,48,3,32,31,1<<30,10000,9,9,99,0)+native+wire
        score = score_snapshot(payload)
        self.assertTrue(score['pass_wire'])
        self.assertEqual(score['pixel_order'],'TOP_DIN0_RIGHT_DIN1_LEFT')
        self.assertEqual(score['submitted_grb48_hex'],wire.hex())
        self.assertEqual(score['logical_rgb16'][0],[0,65535,31*257])
        self.assertEqual(score['submitted_rgb16'][0],[80*257,175*257,31*257])
        # Wrong wiring model must not be made to pass by recolouring the preview.
        wrong = payload[:544]+logical
        self.assertFalse(score_snapshot(wrong)['pass_wire'])
        invalid = bytearray(payload);struct.pack_into('<I',invalid,8,0)
        with self.assertRaises(ValueError):score_snapshot(invalid)

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

    def test_pair_generation_is_not_gpio_status(self):
        changed = bytearray(fixture(24))
        struct.pack_into('<I', changed, 28, 3)
        struct.pack_into('<2I', changed, 48, 924, 924)
        score = score_snapshot(changed)
        self.assertTrue(score['pass_wire'])
        self.assertEqual(score['attempted_generation'], 924)
        self.assertEqual(score['submission_accepted'], 'UNKNOWN')
        self.assertEqual(score['optical'], 'UNKNOWN')
        for a, b in ((0, 0), (924, 925)):
            struct.pack_into('<2I', changed, 48, a, b)
            self.assertFalse(score_snapshot(changed)['pass_wire'])
        struct.pack_into('<2I', changed, 48, 924, 924)
        changed[1503] ^= 1
        self.assertFalse(score_snapshot(changed)['pass_wire'])

    def test_invalid_layout(self):
        for bad in (b'', fixture()[:-1], fixture()+b'x'):
            with self.assertRaises(ValueError): score_snapshot(bad)
        for offset, value in ((0, 2), (8, 2), (12, 256), (16, 1), (20, 128), (36, 44)):
            changed = bytearray(fixture()); struct.pack_into('<I', changed, offset, value)
            with self.assertRaises(ValueError): score_snapshot(changed)

    def test_native16_preview_is_not_a_scaling_or_wire_pass(self):
        changed = bytearray(fixture())
        struct.pack_into('<I', changed, 8, 1)
        struct.pack_into('<I', changed, 40, (1 << 31) | 23)
        changed[544:550] = bytes.fromhex('0123abcd0042')
        result = score_snapshot(changed)
        self.assertEqual(result['output_channel'], 1)
        self.assertEqual(result['source_precision'], 'NATIVE_RGB16')
        self.assertIsNone(result['pass_wire'])
        self.assertIsNone(result['scaling_byte_mismatches'])
        self.assertEqual(result['submitted_rgb16'][0], [0xabcd, 0x0123, 0x0042])
        # Independent byte-wise decoding retains emitted order and low bytes.
        expected = []
        for i in range(544,1504,6):
            g=changed[i]*256+changed[i+1];r=changed[i+2]*256+changed[i+3];b=changed[i+4]*256+changed[i+5]
            expected.append([r,g,b])
        self.assertEqual(result['submitted_rgb16'], expected)


if __name__ == '__main__':
    unittest.main()
