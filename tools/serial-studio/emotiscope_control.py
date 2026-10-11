"""Emotiscope donor opcode-25 body codec.

Wire values are the donor's native raw numbers. The saturation square-root
is a host display transform and is applied by the mixer strategy, not here.
This module does not include or generate from the C++ header.
"""

import math
import struct

GET_SUB = 0x454D0101
PATCH_SUB = 0x454D0102
EXTENDED_GET_SUB = 0x454D0201
EXTENDED_PATCH_SUB = 0x454D0202
STATE_MAGIC = 0x454D5354
STATE_BYTES = 160
GET_BYTES = 16
PATCH_HEADER = 24
ENTRY_BYTES = 8
PATCH_MAX = 184
FIELD_COUNT = 20
FIELD_MASK = 0xFFFFF
TOP_STATE_BYTES = 176
TOP_PATCH_MAX = 208
TOP_FIELD_COUNT = 23
TOP_FIELD_MASK = 0x7FFFFF
DUAL_STATE_BYTES = 208
DUAL_GET_BYTES = 20
DUAL_PATCH_MAX = 240
DUAL_FIELD_COUNT = 27
DUAL_FIELD_MASK = 0x7FFFFFF
CAP_PRIMARY_INTENSITY = 1
CAP_SECONDARY_OUTPUT = 2
CAP_INDEPENDENT_SCENES = 4
CAP_EDGE_COLOUR = 8
CAP_MUSICAL_CONDITIONING = 16
CAP_ML = 32
CAP_COMPOSED_SCENES = 64
COMPOSED_STATE_BYTES = 424
COMPOSED_PATCH_MAX = 536
COMPOSED_FIELD_COUNT = 63
COMPOSED_CAP_MASK = 127
FEATURE_STATE_BYTES = 320
FEATURE_PATCH_HEADER = 32
FEATURE_PATCH_MAX = 384
FEATURE_FIELD_COUNT = 44
FEATURE_FIELD_MASK = (1 << FEATURE_FIELD_COUNT) - 1
FEATURE_CAP_MASK = 63
ML_STATE_QUERY = b"MLST"
ML_STATE_BYTES = 592
PALETTE_MAX = 44  # Current firmware accepts only K1 product palettes 1..44.
U32_MAX = {15: 255, 18: PALETTE_MAX, 24: 1, 26: 2, 29: 6, 32: 1, 33: 3, 35: 6, 38: 1, 39: 2}
FLOAT_RANGES = {25: (0.0, 2.0), 31: (0.0, 60.0), 37: (0.0, 60.0),
                41: (1.0, 1000.0), 42: (1.0, 3000.0)}
UNSUPPORTED_PALETTE_FIELDS = frozenset((3, 5, 7))
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
    18: ("palette", KIND_U32),
    19: ("output_intensity_a", KIND_F32),
    20: ("output_intensity_b", KIND_F32),
    21: ("primary_intensity", KIND_F32),
    22: ("secondary_intensity", KIND_F32),
    23: ("secondary_output_enabled", KIND_BOOL),
    24: ("board", KIND_U32),
    25: ("response_gain", KIND_F32),
    26: ("edge_response", KIND_U32),
    27: ("edge_depth", KIND_F32),
    28: ("edge_colour_enabled", KIND_BOOL),
    29: ("edge_colour_mode", KIND_U32),
    30: ("edge_colour_strength", KIND_F32),
    31: ("edge_colour_spread", KIND_F32),
    32: ("edge_colour_spatial", KIND_U32),
    33: ("pair_link", KIND_U32),
    34: ("pair_colour_enabled", KIND_BOOL),
    35: ("pair_colour_mode", KIND_U32),
    36: ("pair_colour_strength", KIND_F32),
    37: ("pair_colour_spread", KIND_F32),
    38: ("pair_colour_spatial", KIND_U32),
    39: ("musical_role", KIND_U32),
    40: ("musical_strength", KIND_F32),
    41: ("musical_attack_ms", KIND_F32),
    42: ("musical_release_ms", KIND_F32),
    43: ("ml_enabled", KIND_BOOL),
    44: ("ml_strength", KIND_F32),
    45: ("scene", KIND_U32),
    46: ("scene_outgoing_width", KIND_F32),
    47: ("scene_outgoing_ms", KIND_F32),
    48: ("scene_separation_ms", KIND_F32),
    49: ("scene_quiet_ms", KIND_F32),
    50: ("scene_return_width", KIND_F32),
    51: ("scene_return_ms", KIND_F32),
    52: ("scene_return_strength", KIND_F32),
    53: ("scene_trail_ms", KIND_F32),
    54: ("scene_trips", KIND_U32),
    55: ("scene_outgoing_feather", KIND_F32),
    56: ("scene_return_feather", KIND_F32),
    57: ("scene_recovery_ms", KIND_F32),
    58: ("scene_choreography", KIND_U32),
    59: ("scene_initiator", KIND_U32),
    60: ("scene_answer_delay_ms", KIND_F32),
    61: ("scene_phrase", KIND_U32),
    62: ("scene_ml_target", KIND_U32),
    63: ("scene_quiet_scope", KIND_U32),
}
U32_MAX.update({45: 3, 54: 2, 58: 2, 59: 1, 61: 4, 62: 3, 63: 3})
FLOAT_RANGES.update({46: (.02, .8), 47: (200., 8000.), 48: (0., 5000.),
                     49: (0., 8000.), 50: (.02, .8), 51: (200., 8000.),
                     52: (0., .95), 53: (0., 2000.), 57: (0., 8000.), 60: (0., 8000.)})
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


def encode_get(request_id: int, boot_id: int, board: int = 0, *,
               state_version: int = 2, board_capabilities: int = 0) -> bytes:
    if int(request_id) == 0:
        raise ValueError("request_id")
    if board not in (0, 1):
        raise ValueError("board")
    if state_version not in (1, 2, 3, 4, 5, 6):
        raise ValueError("state version")
    if board_capabilities & ~(COMPOSED_CAP_MASK if state_version == 6 else FEATURE_CAP_MASK if state_version == 5 else 7 if state_version == 4 else 3):
        raise ValueError("board capabilities")
    sub = EXTENDED_GET_SUB if state_version >= 5 else GET_SUB
    body = struct.pack("<IIII", sub, int(request_id) & 0xFFFFFFFF, int(boot_id) & 0xFFFFFFFF, 0)
    if board:
        if state_version < 4 or not board_capabilities & CAP_INDEPENDENT_SCENES:
            raise ValueError("unsupported secondary scene")
        body += struct.pack("<I", board)
    return body


def _raw_value(field: int, value):
    if field in UNSUPPORTED_PALETTE_FIELDS:
        raise ValueError("unsupported fixed HSV control")
    kind = field_kind(field)
    if kind == KIND_F32:
        number = float(value)
        lower, upper = FLOAT_RANGES.get(field, (0.0, 1.0))
        rounded = struct.unpack("<f", struct.pack("<f", number))[0] if math.isfinite(number) and abs(number) <= 3.4028235e38 else number
        lower32, upper32 = (struct.unpack("<f", struct.pack("<f", x))[0] for x in (lower, upper))
        in_range = lower32 <= rounded <= upper32 if field >= 45 else lower <= number <= upper
        if not math.isfinite(number) or not in_range:
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
        if raw < (1 if field == 18 else 0) or raw > U32_MAX[field]:
            raise ValueError(field)
    else:
        raise ValueError(field)
    return kind, raw


def encode_patch(request_id: int, boot_id: int, expected_revision: int, fields: dict,
                 *, state_version: int = 2, board_capabilities: int = 0) -> bytes:
    if int(request_id) == 0:
        raise ValueError("request_id")
    items = []
    if state_version not in (1, 2, 3, 4, 5, 6):
        raise ValueError("state version")
    top_profile = state_version >= 3
    dual_profile = state_version >= 4
    feature_profile = state_version >= 5
    if board_capabilities & ~(COMPOSED_CAP_MASK if state_version == 6 else FEATURE_CAP_MASK if feature_profile else 7 if dual_profile else 3) or (board_capabilities and not top_profile):
        raise ValueError("board capabilities")
    for key, value in fields.items():
        field = key if isinstance(key, int) else NAMES[key]
        if field >= 28:
            required = CAP_EDGE_COLOUR if field <= 38 else CAP_MUSICAL_CONDITIONING if field <= 42 else CAP_ML if field <= 44 else CAP_COMPOSED_SCENES
            if (field >= 45 and state_version < 6) or not feature_profile or not board_capabilities & required:
                raise ValueError("unsupported feature control")
        if state_version == 1 and field in (19, 20):
            raise ValueError("unsupported legacy intensity control")
        required = (CAP_PRIMARY_INTENSITY if field == 21 else CAP_SECONDARY_OUTPUT
                    if field in (22, 23) else CAP_INDEPENDENT_SCENES)
        if FIELD_COUNT < field <= 27 and (not top_profile or not board_capabilities & required):
            raise ValueError("unsupported board control")
        if field >= 24 and not dual_profile:
            raise ValueError("unsupported scene control")
        kind, raw = _raw_value(field, value)
        items.append((int(field), kind, raw))
    items.sort()
    if len(items) != len({item[0] for item in items}):
        raise ValueError("duplicate")
    board = next((raw for field, _kind, raw in items if field == 24), 0)
    if board and (not board_capabilities & CAP_INDEPENDENT_SCENES or any(field in (19, 20) for field, _kind, _raw in items)):
        raise ValueError("unsupported secondary DIN control")
    mask = 0
    blob = bytearray()
    previous = 0
    for field, kind, raw in items:
        if field <= previous:
            raise ValueError("order")
        previous = field
        mask |= 1 << (field - 1)
        blob += struct.pack("<HHI", field, kind, raw)
    if mask > (((1 << COMPOSED_FIELD_COUNT) - 1) if state_version == 6 else FEATURE_FIELD_MASK if feature_profile else DUAL_FIELD_MASK if dual_profile else TOP_FIELD_MASK if top_profile else FIELD_MASK) or len(items) > (COMPOSED_FIELD_COUNT if state_version == 6 else FEATURE_FIELD_COUNT if feature_profile else DUAL_FIELD_COUNT if dual_profile else TOP_FIELD_COUNT if top_profile else FIELD_COUNT):
        raise ValueError("mask")
    if feature_profile:
        body = struct.pack("<IIIIIIII", EXTENDED_PATCH_SUB, int(boot_id) & 0xFFFFFFFF,
                           int(request_id) & 0xFFFFFFFF, int(expected_revision) & 0xFFFFFFFF,
                           mask & 0xFFFFFFFF, mask >> 32, len(items), 0) + bytes(blob)
    else:
        body = struct.pack(
        "<IIIIII",
        PATCH_SUB,
        int(expected_revision) & 0xFFFFFFFF,
        int(request_id) & 0xFFFFFFFF,
        int(boot_id) & 0xFFFFFFFF,
        mask,
        len(items),
    ) + bytes(blob)
    if len(body) > (COMPOSED_PATCH_MAX if state_version == 6 else FEATURE_PATCH_MAX if feature_profile else DUAL_PATCH_MAX if dual_profile else TOP_PATCH_MAX if top_profile else PATCH_MAX):
        raise ValueError("size")
    return body


def raw_patch(request_id: int, boot_id: int, expected_revision: int, mask: int, entries: list, *, extended=False) -> bytes:
    """Unvalidated packet builder for negative tests. entries are (field, kind, raw)."""
    blob = bytearray()
    for field, kind, raw in entries:
        blob += struct.pack("<HHI", int(field) & 0xFFFF, int(kind) & 0xFFFF, int(raw) & 0xFFFFFFFF)
    if extended:
        return struct.pack("<IIIIIIII", EXTENDED_PATCH_SUB, int(boot_id) & 0xFFFFFFFF,
                           int(request_id) & 0xFFFFFFFF, int(expected_revision) & 0xFFFFFFFF,
                           int(mask) & 0xFFFFFFFF, int(mask) >> 32, len(entries), 0) + bytes(blob)
    return struct.pack(
        "<IIIIII",
        PATCH_SUB,
        int(expected_revision) & 0xFFFFFFFF,
        int(request_id) & 0xFFFFFFFF,
        int(boot_id) & 0xFFFFFFFF,
        int(mask) & 0xFFFFFFFF,
        len(entries),
    ) + bytes(blob)


def parse(body: bytes, *, state_version: int = 2):
    if state_version not in (1, 2, 3, 4, 5, 6):
        return STATUS_INVALID, None
    if len(body) < 4:
        return STATUS_INVALID, None
    sub = struct.unpack_from("<I", body, 0)[0]
    extended = sub in (EXTENDED_GET_SUB, EXTENDED_PATCH_SUB)
    if sub not in (GET_SUB, PATCH_SUB, EXTENDED_GET_SUB, EXTENDED_PATCH_SUB) or (extended and state_version < 5):
        return STATUS_INVALID, None
    if sub in (GET_SUB, EXTENDED_GET_SUB):
        if len(body) != GET_BYTES and not (state_version >= 4 and len(body) == DUAL_GET_BYTES):
            return STATUS_INVALID, None
        request, boot, tail = struct.unpack_from("<III", body, 4)
        if request == 0 or tail != 0:
            return STATUS_INVALID, None
        board = struct.unpack_from("<I", body, 16)[0] if len(body) == DUAL_GET_BYTES else 0
        if board > 1:
            return STATUS_INVALID, None
        return STATUS_OK, {"sub": sub, "request_id": request, "boot_id": boot, "board": board, "crc": crc32(body)}
    top_profile = state_version >= 3
    dual_profile = state_version >= 4
    header = FEATURE_PATCH_HEADER if extended else PATCH_HEADER
    field_count = COMPOSED_FIELD_COUNT if extended and state_version == 6 else FEATURE_FIELD_COUNT if extended else DUAL_FIELD_COUNT if dual_profile else TOP_FIELD_COUNT if top_profile else FIELD_COUNT
    field_mask = (1 << field_count) - 1
    if len(body) < header or len(body) > header + field_count * ENTRY_BYTES:
        return STATUS_INVALID, None
    if extended:
        boot, request, revision, lo, hi, count, reserved = struct.unpack_from("<IIIIIII", body, 4)
        mask = lo | (hi << 32)
        if reserved:
            return STATUS_INVALID, None
    else:
        revision, request, boot, mask, count = struct.unpack_from("<IIIII", body, 4)
    if request == 0 or (mask & ~field_mask) or count > field_count:
        return STATUS_INVALID, None
    if len(body) != header + count * ENTRY_BYTES or count != bin(mask).count("1"):
        return STATUS_INVALID, None
    fields = {}
    built = 0
    previous = 0
    for index in range(count):
        offset = header + index * ENTRY_BYTES
        field, kind = struct.unpack_from("<HH", body, offset)
        raw = struct.unpack_from("<I", body, offset + 4)[0]
        if field < 1 or field > field_count or field <= previous or kind != field_kind(field) or field in UNSUPPORTED_PALETTE_FIELDS:
            return STATUS_INVALID, None
        previous = field
        built |= 1 << (field - 1)
        if kind == KIND_F32:
            value = struct.unpack("<f", struct.pack("<I", raw))[0]
            lower, upper = (struct.unpack("<f", struct.pack("<f", x))[0] for x in FLOAT_RANGES.get(field, (0.0, 1.0)))
            if not math.isfinite(value) or not lower <= value <= upper:
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
            if raw < (1 if field == 18 else 0) or raw > U32_MAX[field]:
                return STATUS_INVALID, None
            value = int(raw)
        fields[field] = value
    if built != mask:
        return STATUS_INVALID, None
    board = fields.get(24, 0)
    if board and (19 in fields or 20 in fields):
        return STATUS_INVALID, None
    return STATUS_OK, {
        "sub": sub,
        "request_id": request,
        "boot_id": boot,
        "expected_revision": revision,
        "mask": mask,
        "fields": fields,
        "board": board,
        "crc": crc32(body),
    }


def decode_ml_state(body: bytes) -> dict:
    """Decode the existing opcode-17 read-only actual inference witness."""
    if len(body) != ML_STATE_BYTES or body[:4] != b"MLR1":
        raise ValueError("ML state framing")
    version, query_us = struct.unpack_from("<II", body, 4)
    if version != 1:
        raise ValueError("ML state version")
    names = ("sequence", "input_sequence", "now_us", "valid", "model_id",
             "inference_count", "last_cycles", "max_cycles", "rejected_results")
    result = dict(zip(names, struct.unpack_from("<9I", body, 12)))
    if result["valid"] not in (0, 1):
        raise ValueError("ML state valid flag")
    for name, offset, count in (("input", 48, 64), ("latent", 304, 8), ("reconstruction", 336, 64)):
        values = struct.unpack_from("<%df" % count, body, offset)
        if any(not math.isfinite(value) or not 0 <= value <= 1 for value in values):
            raise ValueError("ML state nonfinite or nonunit " + name)
        if not result["valid"] and any(values):
            raise ValueError("invalid ML record must have zero arrays")
        result[name] = list(values)
    result.update(version=version, query_us=query_us, valid=bool(result["valid"]))
    return result


def decode_state(body: bytes) -> dict:
    if len(body) not in (STATE_BYTES, TOP_STATE_BYTES, DUAL_STATE_BYTES, FEATURE_STATE_BYTES, COMPOSED_STATE_BYTES):
        raise ValueError("state length %s" % len(body))
    def u32(offset):
        return struct.unpack_from("<I", body, offset)[0]
    def f32(offset):
        return struct.unpack_from("<f", body, offset)[0]
    version = u32(4)
    if u32(0) != STATE_MAGIC or version not in (1, 2, 3, 4, 5, 6):
        raise ValueError("unsupported state framing")
    if len(body) != (COMPOSED_STATE_BYTES if version == 6 else FEATURE_STATE_BYTES if version == 5 else DUAL_STATE_BYTES if version == 4 else TOP_STATE_BYTES if version == 3 else STATE_BYTES):
        raise ValueError("state version length")
    intensity_supported = version >= 2
    intensity_a = f32(144) if intensity_supported else None
    intensity_b = f32(148) if intensity_supported else None
    if intensity_supported and any(
        not math.isfinite(value) or not 0.0 <= value <= 1.0
        for value in (intensity_a, intensity_b)
    ):
        raise ValueError("invalid output intensity")
    board_capabilities = u32(164) if version >= 3 else 0
    if board_capabilities & ~(COMPOSED_CAP_MASK if version == 6 else FEATURE_CAP_MASK if version == 5 else 7 if version == 4 else 3):
        raise ValueError("unknown board capability")
    primary_intensity = f32(152) if version >= 3 else None
    secondary_intensity = f32(156) if version >= 3 else None
    secondary_enabled = u32(160) if version >= 3 else None
    if version >= 3:
        if any(not math.isfinite(value) or not 0.0 <= value <= 1.0
               for value in (primary_intensity, secondary_intensity)):
            raise ValueError("invalid board intensity")
        if secondary_enabled not in (0, 1) or (secondary_enabled and not board_capabilities & CAP_SECONDARY_OUTPUT):
            raise ValueError("unsupported secondary output state")
    if version >= 4 and any(body[168:176]):
        raise ValueError("nonzero legacy reserved state bytes")
    board = u32(176) if version >= 4 else 0
    response_gain = f32(180) if version >= 4 else None
    edge_response = u32(184) if version >= 4 else None
    edge_depth = f32(188) if version >= 4 else None
    independent = bool(board_capabilities & CAP_INDEPENDENT_SCENES)
    if version >= 4:
        if board > 1 or (board and not independent):
            raise ValueError("unsupported selected board")
        if not math.isfinite(response_gain) or not 0.0 <= response_gain <= 2.0 or edge_response > 2 or not math.isfinite(edge_depth) or not 0.0 <= edge_depth <= 1.0:
            raise ValueError("invalid scene response")
    reserved = body[312:320] + body[420:424] if version == 6 else body[312:320] if version == 5 else body[192:208] if version == 4 else body[168:176] if version == 3 else body[152:160] if intensity_supported else body[144:160]
    if any(reserved):
        raise ValueError("nonzero reserved state bytes")
    features = {}
    if version >= 5:
        layout = {
            "edge_colour_enabled": (192, False), "edge_colour_mode": (196, False),
            "edge_colour_strength": (200, True), "edge_colour_spread": (204, True),
            "edge_colour_spatial": (208, False), "pair_link": (212, False),
            "effective_pair_link": (216, False), "pair_colour_enabled": (220, False),
            "pair_colour_mode": (224, False), "pair_colour_strength": (228, True),
            "pair_colour_spread": (232, True), "pair_colour_spatial": (236, False),
            "musical_role": (240, False), "musical_strength": (244, True),
            "musical_attack_ms": (248, True), "musical_release_ms": (252, True),
            "ml_enabled": (256, False), "ml_strength": (260, True), "ml_valid": (264, False),
            "ml_result_sequence": (268, False), "ml_input_sequence": (272, False),
            "ml_now_us": (276, False), "ml_model_id": (280, False),
            "ml_inference_count": (284, False), "ml_last_cycles": (288, False),
            "publication_sequence": (292, False), "event_cursor": (296, False),
            "conditioning_value": (300, True), "ml_max_cycles": (304, False),
            "ml_rejected_results": (308, False),
        }
        features = {name: f32(offset) if floating else u32(offset)
                    for name, (offset, floating) in layout.items()}
        for field in range(28, 45):
            _raw_value(field, features[FIELDS[field][0]])
        if features["effective_pair_link"] > 3 or features["ml_valid"] > 1:
            raise ValueError("invalid effective feature state")
        if not math.isfinite(features["conditioning_value"]) or features["conditioning_value"] < 0.0:
            raise ValueError("invalid musical conditioning")
        for name in ("edge_colour_enabled", "pair_colour_enabled", "ml_enabled", "ml_valid"):
            features[name] = bool(features[name])
        if features["ml_valid"] and not board_capabilities & CAP_ML:
            raise ValueError("unsupported ML result")
    if version == 6:
        for field in range(45, 64):
            name, kind = FIELDS[field]
            offset = 320 + 4 * (field - 45)
            features[name] = f32(offset) if kind == KIND_F32 else u32(offset)
            _raw_value(field, features[name])
        for index, name in enumerate(("active_scene", "scene_phase", "scene_pending", "scene_gesture_id", "scene_phrase_state", "scene_quiet")):
            features[name] = u32(396 + 4 * index)
        if features["active_scene"] > 3 or features["scene_pending"] > 1 or features["scene_quiet"] > 1:
            raise ValueError("invalid composed scene status")
    features.update(composed_scenes_supported=bool(board_capabilities & CAP_COMPOSED_SCENES))
    features.update(edge_colour_supported=bool(board_capabilities & CAP_EDGE_COLOUR),
                    musical_conditioning_supported=bool(board_capabilities & CAP_MUSICAL_CONDITIONING),
                    ml_supported=bool(board_capabilities & CAP_ML))
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
        "palette": u32(140),
        "output_intensity_supported": intensity_supported,
        "output_intensity_a": intensity_a,
        "output_intensity_b": intensity_b,
        "board_capabilities": board_capabilities,
        "primary_intensity_supported": bool(board_capabilities & CAP_PRIMARY_INTENSITY),
        "secondary_output_supported": bool(board_capabilities & CAP_SECONDARY_OUTPUT),
        "primary_intensity": primary_intensity if board_capabilities & CAP_PRIMARY_INTENSITY else None,
        "secondary_intensity": secondary_intensity if board_capabilities & CAP_SECONDARY_OUTPUT else None,
        "secondary_output_enabled": bool(secondary_enabled) if board_capabilities & CAP_SECONDARY_OUTPUT else None,
        "selected_board": board,
        "independent_scenes_supported": independent,
        "response_gain": response_gain if independent else None,
        "edge_response": edge_response if independent else None,
        "edge_depth": edge_depth if independent else None,
        "reserved": reserved,
        **features,
    }


def applicability(mode: int, color_range: float) -> int:
    bits = 0xFFFF & ~0x54
    if mode != 0:
        bits &= ~(1 << 5)
    # Retain the API argument for callers; palette reversal ignores obsolete HSV range.
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
