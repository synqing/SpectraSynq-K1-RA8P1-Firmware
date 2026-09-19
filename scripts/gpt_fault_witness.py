"""Strict HOST decoder for opcode 22/v2. Packed bytes are not wire evidence."""
import hashlib
import json
import struct
from pathlib import Path

HEAD = 'version bytes attempts dma_irqs stop_irqs frames errors first_fault state owned clock_hz bits'.split()
FIELDS = json.loads(Path(__file__).with_name('gpt_snapshot_fields.json').read_text())
SUMMARY = 'dmctl last_prepare_cycles maximum_prepare_cycles'.split()
WITNESS = ('valid frame_id profile payload_bytes payload_fnv1a32 duty_fnv1a32 '
           'expected_dma_words dma_source_start start_requested elapsed_cycles '
           'cpu_hz dma_complete waveform_complete reset_ready dmsts dmctl').split()
V1_BYTES = 728
PAYLOAD_CAP = 480
V2_BYTES = 1420
PROFILES = {1: (250, 875, 1250, 3, 128), 2: (225, 580, 1225, 3, 128),
            3: (250, 650, 1250, 6, 80), 4: (250, 875, 1250, 6, 80)}


def fnv1a32(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def decode_v2(body):
    if len(body) != V2_BYTES or len(FIELDS) != 34:
        raise ValueError('v2 diagnostic layout mismatch')
    out = dict(zip(HEAD, struct.unpack_from('<12I', body)))
    if out['version'] != 2 or out['bytes'] != V2_BYTES:
        raise ValueError('v2 diagnostic version/byte count mismatch')
    out['snapshots'] = [dict(zip(FIELDS, struct.unpack_from('<34I', body, 48 + i * 136)))
                        for i in range(5)]
    out.update(zip(SUMMARY, struct.unpack_from('<3I', body, V1_BYTES)))
    offset = V1_BYTES + 12
    witness = dict(zip(WITNESS, struct.unpack_from('<16I', body, offset)))
    offset += 64
    witness['terminal'] = dict(zip(FIELDS, struct.unpack_from('<34I', body, offset)))
    offset += 136
    payload_storage = body[offset:offset + PAYLOAD_CAP]
    if witness['valid'] not in (0, 1):
        raise ValueError('invalid witness publication flag')
    if not witness['valid']:
        if out['first_fault'] or any(body[V1_BYTES + 12:]):
            raise ValueError('absent witness with fault or unpublished data')
    else:
        terminal = witness['terminal']
        if (not out['first_fault'] or terminal['tag'] != 5
                or terminal['fault'] != out['first_fault']):
            raise ValueError('fault witness does not match latched fault')
        size = witness['payload_bytes']
        if size > PAYLOAD_CAP or any(payload_storage[size:]):
            raise ValueError('invalid payload length/padding')
        payload = payload_storage[:size]
        if size:
            if (witness['frame_id'] != terminal['attempt']
                    or not 0 < witness['frame_id'] <= out['attempts']):
                raise ValueError('fault frame identity mismatch')
            profile = PROFILES.get(witness['profile'])
            if not profile or size % profile[3] or size // profile[3] > profile[4]:
                raise ValueError('invalid fault payload profile/length')
            if witness['expected_dma_words'] != size * 8 - 2:
                raise ValueError('preloads/expected transfer length mismatch')
            if fnv1a32(payload) != witness['payload_fnv1a32']:
                raise ValueError('packed payload hash mismatch')
            hz = out['clock_hz']
            if not hz:
                raise ValueError('missing GPT clock for duty reconstruction')
            duty = b''.join(struct.pack('<I', ((hz * profile[bool(byte & (0x80 >> bit))]
                                             + 999999999) // 1000000000) - 1)
                            for byte in payload for bit in range(8))
            if fnv1a32(duty) != witness['duty_fnv1a32']:
                raise ValueError('encoded duty hash mismatch')
            witness['packed_grb_hex'] = payload.hex()
            witness['packed_grb_sha256'] = hashlib.sha256(payload).hexdigest()
        elif witness['frame_id'] or witness['start_requested']:
            raise ValueError('submitted frame without payload')
    out['fault_witness'] = witness
    return out
