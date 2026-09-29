"""Emotiscope donor opcode-25 body codec.

Wire values are the donor's native raw numbers. The saturation square-root
is a host display transform and is applied by the mixer strategy, not here.
This module does not include or generate from the C++ header.
"""

import math
import struct

GET_SUB = 0x454D0101
PATCH_SUB = 0x454D0102
STATE_MAGIC = 0x454D5354
STATE_BYTES = 160
GET_BYTES = 16
PATCH_HEADER = 24
ENTRY_BYTES = 8
PATCH_MAX = 160
FIELD_COUNT = 17
FIELD_MASK = 0x1FFFF
FLAG_OUTPUT_PRECISION = 64

KIND_F32 = 1
KIND_BOOL = 2
KIND_I32 = 3
KIND_U32 = 4
STATUS_OK = 0
STATUS_INVALID = 3

FIELDS = {
    1: ("brightness", KIND_F32),
    2: ("softness", KIND_F32),
    3: ("color", KIND_F32),
    4: ("warmth", KIND_F32),
    5: ("color_range", KIND_F32),
    6: ("speed", KIND_F32),
    7: ("saturation", KIND_F32),
    8: ("background", KIND_F32),
    9: ("current_mode", KIND_I32),
    10: ("mirror_mode", KIND_BOOL),
    11: ("screensaver", KIND_BOOL),
    12: ("temporal_dithering", KIND_BOOL),
    13: ("auto_color_cycle", KIND_BOOL),
    14: ("reverse_color_range", KIND_BOOL),
    15: ("hardware_output_brightness", KIND_U32),
    16: ("emission_enabled", KIND_BOOL),
    17: ("output_precision", KIND_BOOL),
}
NAMES = {name: field for field, (name, _kind) in FIELDS.items()}

FROZEN_GOLDEN = bytes.fromhex(
    "02014d450000000001000000110000000100000001000000010001000000003f"
)


def crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return (~crc) & 0xFFFFFFFF


def field_kind(field: int) -> int:
    spec = FIELDS.get(field)
    return spec[1] if spec else 0


def golden_patch() -> bytes:
    return struct.pack("<IIIIII", PATCH_SUB, 0, 1, 0x11, 1, 1) + struct.pack(
        "<HHI", 1, KIND_F32, 0x3F000000
    )


def encode_get(request_id: int, boot_id: int) -> bytes:
    if int(request_id) == 0:
        raise ValueError("request_id")
    return struct.pack("<IIII", GET_SUB, int(request_id) & 0xFFFFFFFF, int(boot_id) & 0xFFFFFFFF, 0)


def _raw_value(field: int, value):
    kind = field_kind(field)
    if kind == KIND_F32:
        number = float(value)
        if not math.isfinite(number) or number < 0.0 or number > 1.0:
            raise ValueError(field)
        raw = struct.unpack("<I", struct.pack("<f", number))[0]
    elif kind == KIND_BOOL:
        if value not in (0, 1, False, True):
            raise ValueError(field)
        raw = 1 if value else 0
    elif kind == KIND_I32:
        raw = int(value)
        if raw < 0 or raw > 6:
            raise ValueError(field)
        raw &= 0xFFFFFFFF
    elif kind == KIND_U32:
        raw = int(value)
        if raw < 0 or raw > 255:
            raise ValueError(field)
    else:
        raise ValueError(field)
    return kind, raw


def encode_patch(request_id: int, boot_id: int, expected_revision: int, fields: dict) -> bytes:
    if int(request_id) == 0:
        raise ValueError("request_id")
    items = []
    for key, value in fields.items():
        field = key if isinstance(key, int) else NAMES[key]
        kind, raw = _raw_value(field, value)
        items.append((int(field), kind, raw))
    items.sort()
    if len(items) != len({item[0] for item in items}):
        raise ValueError("duplicate")
    mask = 0
    blob = bytearray()
    previous = 0
    for field, kind, raw in items:
        if field <= previous:
            raise ValueError("order")
        previous = field
        mask |= 1 << (field - 1)
        blob += struct.pack("<HHI", field, kind, raw)
    if mask > FIELD_MASK or len(items) > FIELD_COUNT:
        raise ValueError("mask")
    body = struct.pack(
        "<IIIIII",
        PATCH_SUB,
        int(expected_revision) & 0xFFFFFFFF,
        int(request_id) & 0xFFFFFFFF,
        int(boot_id) & 0xFFFFFFFF,
        mask,
        len(items),
    ) + bytes(blob)
    if len(body) > PATCH_MAX:
        raise ValueError("size")
    return body


def raw_patch(request_id: int, boot_id: int, expected_revision: int, mask: int, entries: list) -> bytes:
    """Unvalidated packet builder for negative tests. entries are (field, kind, raw)."""
    blob = bytearray()
    for field, kind, raw in entries:
        blob += struct.pack("<HHI", int(field) & 0xFFFF, int(kind) & 0xFFFF, int(raw) & 0xFFFFFFFF)
    return struct.pack(
        "<IIIIII",
        PATCH_SUB,
        int(expected_revision) & 0xFFFFFFFF,
        int(request_id) & 0xFFFFFFFF,
        int(boot_id) & 0xFFFFFFFF,
        int(mask) & 0xFFFFFFFF,
        len(entries),
    ) + bytes(blob)


def parse(body: bytes):
    if len(body) < 4:
        return STATUS_INVALID, None
    sub = struct.unpack_from("<I", body, 0)[0]
    if sub not in (GET_SUB, PATCH_SUB):
        return STATUS_INVALID, None
    if sub == GET_SUB:
        if len(body) != GET_BYTES:
            return STATUS_INVALID, None
        request, boot, tail = struct.unpack_from("<III", body, 4)
        if request == 0 or tail != 0:
            return STATUS_INVALID, None
        return STATUS_OK, {"sub": sub, "request_id": request, "boot_id": boot, "crc": crc32(body)}
    if len(body) < PATCH_HEADER or len(body) > PATCH_MAX:
        return STATUS_INVALID, None
    revision, request, boot, mask, count = struct.unpack_from("<IIIII", body, 4)
    if request == 0 or (mask & ~FIELD_MASK) or count > FIELD_COUNT:
        return STATUS_INVALID, None
    if len(body) != PATCH_HEADER + count * ENTRY_BYTES or count != bin(mask).count("1"):
        return STATUS_INVALID, None
    fields = {}
    built = 0
    previous = 0
    for index in range(count):
        offset = PATCH_HEADER + index * ENTRY_BYTES
        field, kind = struct.unpack_from("<HH", body, offset)
        raw = struct.unpack_from("<I", body, offset + 4)[0]
        if field < 1 or field > FIELD_COUNT or field <= previous or kind != field_kind(field):
            return STATUS_INVALID, None
        previous = field
        built |= 1 << (field - 1)
        if kind == KIND_F32:
            value = struct.unpack("<f", struct.pack("<I", raw))[0]
            if not math.isfinite(value) or not (0.0 <= value <= 1.0):
                return STATUS_INVALID, None
        elif kind == KIND_BOOL:
            if raw > 1:
                return STATUS_INVALID, None
            value = int(raw)
        elif kind == KIND_I32:
            value = struct.unpack("<i", struct.pack("<I", raw))[0]
            if value < 0 or value > 6:
                return STATUS_INVALID, None
        else:
            if raw > 255:
                return STATUS_INVALID, None
            value = int(raw)
        fields[field] = value
    if built != mask:
        return STATUS_INVALID, None
    return STATUS_OK, {
        "sub": sub,
        "request_id": request,
        "boot_id": boot,
        "expected_revision": revision,
        "mask": mask,
        "fields": fields,
        "crc": crc32(body),
    }


def decode_state(body: bytes) -> dict:
    if len(body) != STATE_BYTES:
        raise ValueError("state length %s" % len(body))
    def u32(offset):
        return struct.unpack_from("<I", body, offset)[0]
    def f32(offset):
        return struct.unpack_from("<f", body, offset)[0]
    return {
        "magic": u32(0),
        "version": u32(4),
        "engine": u32(8),
        "boot_id": u32(12),
        "revision": u32(16),
        "request_id": u32(20),
        "brightness": f32(24),
        "softness": f32(28),
        "color": f32(32),
        "warmth": f32(36),
        "color_range": f32(40),
        "speed": f32(44),
        "saturation": f32(48),
        "background": f32(52),
        "effective_hue": f32(56),
        "mode": struct.unpack_from("<i", body, 60)[0],
        "flags": u32(64),
        "output_brightness": u32(68),
        "applicability": u32(72),
        "accepted_request_id": u32(76),
        "rendered_request_id": u32(80),
        "emitted_request_id": u32(84),
        "rendered_frame": u32(88),
        "last_status": u32(92),
        "dedup": u32(96),
        "changed": u32(100),
        "mode_transition": u32(104),
        "schema": body[108:140],
        "reserved": body[140:160],
    }


def applicability(mode: int, color_range: float) -> int:
    bits = 0xFFFF
    if mode != 0:
        bits &= ~(1 << 5)
    if not (color_range > 0.0):
        bits &= ~(1 << 13)
    return bits


def flag_word(mirror, screensaver, dither, auto_color, reverse, emission, output_precision=False) -> int:
    word = 0
    if mirror:
        word |= 1
    if screensaver:
        word |= 2
    if dither:
        word |= 4
    if auto_color:
        word |= 8
    if reverse:
        word |= 16
    if emission:
        word |= 32
    if output_precision:
        word |= FLAG_OUTPUT_PRECISION
    return word


def effective_hue(color: float) -> float:
    if not math.isfinite(color):
        return 0.0
    wrapped = color - math.floor(color)
    if wrapped < 0.0:
        wrapped += 1.0
    if wrapped >= 1.0:
        wrapped = 0.0
    return wrapped
