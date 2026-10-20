"""Score a coherent Titan submission tap; never infer measured light from bytes."""
import struct
import zlib

WORDS = ('version', 'sequence', 'output_channel', 'brightness', 'lanes',
         'pixels_per_lane', 'bits_per_pixel', 'wire_profile', 'mode', 'palette',
         'flags', 'period_us', 'result_a', 'result_b', 'time_low', 'time_high')


def score_snapshot(payload: bytes) -> dict:
    if len(payload) != 1504:
        raise ValueError('snapshot must contain 64 metadata, 480 RGB8 and 960 GRB48 bytes')
    state = dict(zip(WORDS, struct.unpack('<16I', payload[:64])))
    if (state['version'], state['lanes'], state['pixels_per_lane'],
            state['bits_per_pixel'], state['wire_profile']) not in ((1, 2, 80, 48, 4), (1, 2, 80, 48, 3)):
        raise ValueError('unsupported wire snapshot layout')
    if (state['output_channel'] > 1 or state['brightness'] > 255 or
            state['palette'] >= 44 or not state['sequence']):
        raise ValueError('invalid snapshot identity or configuration')
    native, wire = payload[64:544], payload[544:]
    mismatches, lit, mirror_mismatches = 0, [], 0
    for pixel in range(160):
        rgb = native[pixel*3:pixel*3+3]
        expected = struct.pack('>3H', *(rgb[i]*257*state['brightness']//255 for i in (1, 0, 2)))
        actual = wire[pixel*6:pixel*6+6]
        mismatches += sum(a != b for a, b in zip(expected, actual))
        if any(actual):
            lit.append(pixel)
        opposite = wire[(159-pixel)*6:(160-pixel)*6]
        mirror_mismatches += actual != opposite
    state.update(time_us=state['time_low'] | state['time_high'] << 32,
                 native_rgb8_hex=native.hex(), submitted_grb48_hex=wire.hex(),
                 wire_crc32=zlib.crc32(wire), scaling_byte_mismatches=mismatches,
                 mirror_pixel_mismatches=mirror_mismatches, nonzero_pixels=lit,
                 lane_nonzero_counts=[sum(p//80 == lane for p in lit) for lane in (0, 1)],
                 pass_wire=(mismatches == 0 and state['result_a'] == 0 and state['result_b'] == 0),
                 evidence_level='ON_TARGET_SUBMISSION_BUFFER_NOT_GPIO_OR_PHOTONS')
    return state
