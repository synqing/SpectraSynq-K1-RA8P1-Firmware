#!/usr/bin/env python3
"""Host proof for the Emotiscope donor control codec.

The C++ binary is the firmware implementation. This script's packet bytes and
expected field values are an independent specification. A green run is this
candidate's result only.
"""

import argparse
import importlib.util
import json
import struct
import subprocess
import sys
import zlib
from pathlib import Path


def load_codec(root: Path):
    path = root / "tools/serial-studio/emotiscope_control.py"
    spec = importlib.util.spec_from_file_location("emotiscope_control", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def close(got, want):
    return abs(float(got) - float(want)) <= 1e-5


def source_witness(root: Path):
    fixture = (root / "platform/ra8p1/fixture_app.cpp").read_text()
    build = (root / "scripts/build_scalar.py").read_text()
    checks = {
        "early_reject_without_pair_owner":
            "#if defined(K1_EMOTISCOPE_PORT) && !defined(K1_PALETTE_WS2816_GPT_PAIR)" in fixture
            and "if (command == k1::titan::kLiveConfigOpcode) { error(3); return; }" in fixture,
        "donor_queue_on_opcode_25": "donor_queue_config(size);" in fixture,
        "opcode_16_rejected_under_donor_profile":
            "#ifdef K1_EMOTISCOPE_PORT\n    error(3); return;\n#endif" in fixture,
        "performance_ramp_excluded": "!defined(K1_EMOTISCOPE_PORT)" in fixture,
        "production_build_omits_test_seam": "K1_EMOTISCOPE_TEST_SEAM" not in build,
    }
    return checks


def corpus(codec):
    nan_bits = struct.unpack("<I", struct.pack("<f", float("nan")))[0]
    half_bits = struct.unpack("<I", struct.pack("<f", 0.5))[0]
    steps = []

    def add(packet, **expect):
        steps.append((packet, expect))

    get = codec.encode_get(7, 0x11)
    add(get, parse=0, apply=0, revision=0, dedup=0, changed=0, mode=1, mode_transition=0,
        brightness=1.0, softness=0.25, color=0.33, warmth=0.0, color_range=0.0, speed=0.5,
        saturation=0.75, background=0.25, effective_hue=0.33, flags=39, applicability=0xFF8B,
        output_brightness=100, last_status=0, accepted_request_id=0)
    add(get, parse=0, apply=0, dedup=1, replay_of=0)
    add(struct.pack("<I", 8) + b"\x00" * 28, parse=3)
    golden = codec.FROZEN_GOLDEN
    if codec.encode_patch(1, 0x11, 0, {"brightness": 0.5}) != golden:
        raise RuntimeError("Python encoder drifted from the frozen golden packet")
    add(golden, parse=0, apply=0, revision=1, dedup=0, changed=1, mode_transition=0,
        brightness=0.5, softness=0.25, mode=1, output_brightness=100, last_status=0,
        flags=39, applicability=0xFF8B)
    add(golden, parse=0, apply=0, dedup=1, replay_of=3)
    add(codec.encode_patch(1, 0x11, 1, {"brightness": 0.2}),
        parse=0, apply=7, revision=1, dedup=0, changed=0, last_status=7, brightness=0.5)
    add(codec.encode_patch(4, 0x99, 1, {"brightness": 0.2}),
        parse=0, apply=6, revision=1, last_status=6, brightness=0.5, changed=0, dedup=0)
    add(codec.encode_patch(4, 0x11, 1, {"brightness": 0.2}),
        parse=0, apply=0, revision=2, changed=1, dedup=0, brightness=0.2, last_status=0)
    add(codec.encode_patch(5, 0x11, 0, {"softness": 0.5}),
        parse=0, apply=5, revision=2, last_status=5, changed=0, softness=0.25, brightness=0.2)
    add(codec.encode_patch(5, 0x11, 2, {"softness": 0.5}),
        parse=0, apply=0, revision=3, changed=1, softness=0.5, brightness=0.2, dedup=0)
    empty = codec.encode_patch(6, 0x11, 3, {})
    add(empty, parse=0, apply=0, revision=3, changed=0, dedup=0, brightness=0.2, softness=0.5)
    add(empty, parse=0, apply=0, dedup=1, replay_of=10)
    add(codec.raw_patch(50, 0x11, 3, 0x3, [(2, codec.KIND_F32, half_bits), (1, codec.KIND_F32, half_bits)]), parse=3)
    add(codec.raw_patch(51, 0x11, 3, 0x3, [(1, codec.KIND_F32, half_bits), (1, codec.KIND_F32, half_bits)]), parse=3)
    add(codec.raw_patch(52, 0x11, 3, 0x1, [(1, codec.KIND_BOOL, 1)]), parse=3)
    add(codec.raw_patch(53, 0x11, 3, 1 << 8, [(9, codec.KIND_I32, 7)]), parse=3)
    add(codec.raw_patch(54, 0x11, 3, 1 << 8, [(9, codec.KIND_I32, 8)]), parse=3)
    add(codec.raw_patch(55, 0x11, 3, 1 << 14, [(15, codec.KIND_U32, 256)]), parse=3)
    add(codec.raw_patch(56, 0x11, 3, 1 << 9, [(10, codec.KIND_BOOL, 2)]), parse=3)
    add(codec.raw_patch(57, 0x11, 3, 0x1, [(1, codec.KIND_F32, nan_bits)]), parse=3)
    add(codec.raw_patch(0, 0x11, 3, 0, []), parse=3)
    add(struct.pack("<IIII", codec.GET_SUB, 3, 0x11, 1), parse=3)
    add(struct.pack("<I", codec.GET_SUB) + b"\x00" * 8, parse=3)
    add(codec.raw_patch(58, 0x11, 3, 0x10000, [(1, codec.KIND_F32, half_bits)]), parse=3)
    add(struct.pack("<I", 1) + b"\x00" * 20, parse=3)
    add(codec.encode_patch(10, 0x11, 3, {"warmth": 0.15}),
        parse=0, apply=0, revision=4, changed=1, dedup=0, warmth=0.15, color=0.33,
        effective_hue=0.33, brightness=0.2, softness=0.5, mode=1, flags=39,
        applicability=0xFF8B, output_brightness=100, mode_transition=0)
    scene = {
        "brightness": 0.10, "softness": 0.20, "warmth": 0.40,
        "speed": 0.60, "background": 0.80,
        "current_mode": 6, "mirror_mode": 0, "screensaver": 0, "temporal_dithering": 0,
        "auto_color_cycle": 1, "reverse_color_range": 1,
        "hardware_output_brightness": 80, "emission_enabled": 0,
    }
    add(codec.encode_patch(8, 0x11, 4, scene),
        parse=0, apply=0, revision=5, changed=1, dedup=0, mode_transition=1, mode=6,
        brightness=0.10, softness=0.20, color=0.33, warmth=0.40, color_range=0.0,
        speed=0.60, saturation=0.75, background=0.80, effective_hue=0.33,
        output_brightness=80, flags=24, applicability=0xFF8B, last_status=0,
        accepted_request_id=8)
    add(codec.encode_get(9, 0x11),
        parse=0, apply=0, revision=5, changed=0, dedup=0, mode=6, brightness=0.10,
        output_brightness=80, flags=24, applicability=0xFF8B, accepted_request_id=8,
        request_id=9, last_status=0)
    # Output precision: field 17 bool, owner-held, GET flag bit 6 (64). Not a donor field.
    add(codec.encode_patch(11, 0x11, 5, {"output_precision": 1}),
        parse=0, apply=0, revision=6, changed=1, dedup=0, mode=6, flags=24 | 64,
        applicability=0xFF8B, output_brightness=80, accepted_request_id=11, last_status=0)
    add(codec.encode_get(12, 0x11),
        parse=0, apply=0, revision=6, changed=0, dedup=0, flags=24 | 64, request_id=12)
    add(codec.encode_patch(13, 0x11, 6, {"output_precision": 1}),
        parse=0, apply=0, revision=6, changed=0, dedup=0, flags=24 | 64)
    add(codec.raw_patch(59, 0x11, 6, 1 << 16, [(17, codec.KIND_BOOL, 2)]), parse=3)
    add(codec.raw_patch(60, 0x11, 6, 1 << 17, [(18, codec.KIND_BOOL, 1)]), parse=3)
    add(codec.raw_patch(61, 0x11, 6, 1 << 16, [(17, codec.KIND_F32, half_bits)]), parse=3)
    add(codec.encode_patch(14, 0x11, 6, {"output_precision": 0}),
        parse=0, apply=0, revision=7, changed=1, dedup=0, flags=24, accepted_request_id=14)
    # Palette: field18 u32 1..44 only, selected-scene state offset140.
    add(codec.encode_patch(15, 0x11, 7, {"palette": 24}),
        parse=0, apply=0, revision=8, changed=1, dedup=0, palette=24, flags=24, accepted_request_id=15)
    add(codec.encode_get(16, 0x11), parse=0, apply=0, revision=8, changed=0, palette=24, request_id=16)
    add(codec.raw_patch(62, 0x11, 8, 1 << 17, [(18, codec.KIND_U32, 45)]), parse=3)
    add(codec.raw_patch(63, 0x11, 8, 1 << 18, [(19, codec.KIND_U32, 1)]), parse=3)
    add(codec.encode_patch(17, 0x11, 8, {"palette": 1}),
        parse=0, apply=0, revision=9, changed=1, dedup=0, palette=1, accepted_request_id=17)
    # Independent physical gains preserve the donor scene and the other lane.
    gain_a = codec.encode_patch(100, 0x11, 9, {"output_intensity_a": 0.25})
    gain_a_index = len(steps)
    add(gain_a, parse=0, apply=0, revision=10, changed=1, dedup=0,
        output_intensity_a=0.25, output_intensity_b=1.0, brightness=0.10,
        output_brightness=80, accepted_request_id=100)
    add(gain_a, parse=0, apply=0, dedup=1, replay_of=gain_a_index)
    add(codec.encode_patch(101, 0x11, 10, {"output_intensity_b": 0.75}),
        parse=0, apply=0, revision=11, changed=1, dedup=0,
        output_intensity_a=0.25, output_intensity_b=0.75, accepted_request_id=101)
    add(codec.encode_patch(102, 0x11, 9, {"output_intensity_a": 0.5}),
        parse=0, apply=5, revision=11, changed=0,
        output_intensity_a=0.25, output_intensity_b=0.75)
    add(codec.encode_patch(102, 0x11, 11, {"output_intensity_a": 0.5}),
        parse=0, apply=0, revision=12, changed=1,
        output_intensity_a=0.5, output_intensity_b=0.75)
    for field in (19, 20):
        for raw in (nan_bits, 0x7F800000, 0xFF800000, 0x3F800001, 0xBF000000, 0x80000001):
            add(codec.raw_patch(110, 0x11, 12, 1 << (field - 1),
                                [(field, codec.KIND_F32, raw)]), parse=3)
    full_fields = dict(scene, output_precision=0, palette=1,
                       output_intensity_a=0.5, output_intensity_b=0.5)
    full_packet = codec.encode_patch(120, 0x11, 12, full_fields)
    if len(full_packet) != 160 or struct.unpack_from("<II", full_packet, 16) != (0xFFFAB, 17):
        raise RuntimeError("full 17-applicable-field packet shape")
    add(full_packet, parse=0, apply=0, revision=13, changed=1,
        output_intensity_a=0.5, output_intensity_b=0.5)
    add(full_packet + b"\x00", parse=3)
    for field in (3, 5, 7):
        add(codec.raw_patch(121, 0x11, 13, 1 | (1 << (field-1)), [(1, codec.KIND_F32, half_bits), (field, codec.KIND_F32, half_bits)]), parse=3)
    add(codec.raw_patch(122, 0x11, 13, 1 | (1 << 17), [(1, codec.KIND_F32, half_bits), (18, codec.KIND_U32, 0)]), parse=3)
    return steps


def check_state_versions(codec):
    """Independent byte fixtures exercise legacy support and version-2 rejection."""
    for mode in range(7):
        for historical_range in (0.0, 0.5):
            expected = 0xFFAB if mode == 0 else 0xFF8B
            if codec.applicability(mode, historical_range) != expected:
                raise RuntimeError("palette reversal incorrectly depends on obsolete range")
    legacy = bytearray(160)
    struct.pack_into("<III", legacy, 0, 0x454D5354, 1, 1)
    state = codec.decode_state(bytes(legacy))
    if (state["output_intensity_supported"] is not False
            or state["output_intensity_a"] is not None or state["output_intensity_b"] is not None
            or state["reserved"] != bytes(16)):
        raise RuntimeError("legacy state falsely advertises independent intensity")
    current = bytearray(legacy)
    struct.pack_into("<I", current, 4, 2)
    struct.pack_into("<ff", current, 144, 0.25, 0.75)
    state = codec.decode_state(bytes(current))
    if (state["output_intensity_supported"] is not True
            or state["output_intensity_a"] != 0.25 or state["output_intensity_b"] != 0.75
            or state["reserved"] != bytes(8)):
        raise RuntimeError("version-2 intensity offsets")
    if state["primary_intensity"] is not None or state["secondary_intensity"] is not None or state["secondary_output_supported"]:
        raise RuntimeError("installed v2 falsely advertises board controls")
    top = bytearray(current) + bytearray(16)
    struct.pack_into("<I", top, 4, 3)
    struct.pack_into("<ffII", top, 152, 0.25, 0.75, 0, 3)
    top_state = codec.decode_state(bytes(top))
    if (top_state["primary_intensity"] != 0.25 or top_state["secondary_intensity"] != 0.75
            or top_state["secondary_output_enabled"] is not False or top_state["board_capabilities"] != 3):
        raise RuntimeError("optional v3 board offsets")
    for offset, raw in ((152, 0x7FC00000), (156, 0x80000001), (160, 2), (164, 4), (168, 1)):
        bad = bytearray(top)
        struct.pack_into("<I", bad, offset, raw)
        try:
            codec.decode_state(bytes(bad))
        except ValueError:
            pass
        else:
            raise RuntimeError("malformed v3 board state accepted")
    unsupported = bytearray(top)
    struct.pack_into("<II", unsupported, 160, 1, 1)
    try:
        codec.decode_state(bytes(unsupported))
    except ValueError:
        pass
    else:
        raise RuntimeError("enabled secondary without capability")
    for field in ("primary_intensity", "secondary_intensity", "secondary_output_enabled"):
        try:
            codec.encode_patch(1, 0x11, 0, {field: 1})
        except ValueError:
            pass
        else:
            raise RuntimeError("unconfirmed board control encoded")
    packet = codec.encode_patch(1, 0x11, 0, {"primary_intensity": 0.25, "secondary_intensity": 0.75},
                                state_version=3, board_capabilities=3)
    if codec.parse(packet)[0] != 3 or codec.parse(packet, state_version=3)[0] != 0:
        raise RuntimeError("profile-specific field acceptance")
    malformed = []
    for offset in (144, 148):
        for raw in (0x7FC00000, 0x7F800000, 0xFF800000, 0x3F800001, 0xBF000000, 0x80000001):
            body = bytearray(current)
            struct.pack_into("<I", body, offset, raw)
            malformed.append(body)
    for offset, value in ((0, 0), (4, 3), (152, 1)):
        body = bytearray(current)
        struct.pack_into("<I", body, offset, value)
        malformed.append(body)
    for body in malformed:
        try:
            codec.decode_state(bytes(body))
        except ValueError:
            continue
        raise RuntimeError("malformed version-2 state accepted")
    for field in ("output_intensity_a", "output_intensity_b"):
        for invalid in (float("nan"), float("inf"), -0.01, 1.01):
            try:
                codec.encode_patch(1, 0x11, 0, {field: invalid})
            except ValueError:
                continue
            raise RuntimeError("invalid intensity encoded")
    dual = bytearray(top) + bytearray(32)
    struct.pack_into("<I", dual, 4, 4)
    struct.pack_into("<I", dual, 164, 7)
    struct.pack_into("<IfIf", dual, 176, 1, 1.75, 2, 0.25)
    state = codec.decode_state(bytes(dual))
    if (state["selected_board"], state["response_gain"], state["edge_response"], state["edge_depth"]) != (1, 1.75, 2, 0.25) or not state["independent_scenes_supported"]:
        raise RuntimeError("v4 selected-scene offsets")
    if top_state["independent_scenes_supported"] or top_state["response_gain"] is not None:
        raise RuntimeError("historical v3 falsely advertises independent scenes")
    fault = bytearray(dual)
    struct.pack_into("<I", fault, 164, 5)
    state = codec.decode_state(bytes(fault))
    if state["selected_board"] != 1 or state["response_gain"] != 1.75 or state["secondary_output_supported"]:
        raise RuntimeError("failed top admission lost independent scene readback")
    if len(codec.encode_get(49, 0x11, 1, state_version=4, board_capabilities=5)) != 20:
        raise RuntimeError("faulted secondary scene cannot be read")
    for offset, raw in ((164, 8), (168, 1), (176, 2), (180, 0x40000001), (180, 0x7FC00000),
                        (184, 3), (188, 0x80000001), (192, 1), (204, 1)):
        bad = bytearray(dual)
        struct.pack_into("<I", bad, offset, raw)
        try:
            codec.decode_state(bytes(bad))
        except ValueError:
            pass
        else:
            raise RuntimeError("malformed v4 state accepted")
    get = codec.encode_get(50, 0x11, 1, state_version=4, board_capabilities=7)
    if get != struct.pack("<IIIII", codec.GET_SUB, 50, 0x11, 0, 1) or codec.parse(get, state_version=4)[1]["board"] != 1:
        raise RuntimeError("secondary GET wire target")
    if len(codec.encode_get(51, 0x11, state_version=4, board_capabilities=7)) != 16 or codec.parse(get, state_version=3)[0] != 3:
        raise RuntimeError("legacy GET compatibility")
    fields = {"board": 1, "response_gain": 1.75, "edge_response": 2, "edge_depth": 0.25}
    packet = codec.encode_patch(52, 0x11, 3, fields, state_version=4, board_capabilities=7)
    parsed, info = codec.parse(packet, state_version=4)
    if parsed or info["board"] != 1 or info["fields"] != {24: 1, 25: 1.75, 26: 2, 27: 0.25}:
        raise RuntimeError("v4 scene response packet")
    for fields, version, caps in (({"board": 1}, 3, 3), ({"response_gain": 1}, 4, 3),
                                  ({"board": 1, "output_intensity_a": 0.5}, 4, 7),
                                  ({"response_gain": float("nan")}, 4, 7),
                                  ({"response_gain": 2.01}, 4, 7), ({"edge_response": 3}, 4, 7),
                                  ({"edge_depth": -0.1}, 4, 7), ({"palette": 0}, 4, 7),
                                  ({"brightness": 0.1, "color": 0.2}, 4, 7)):
        try:
            codec.encode_patch(53, 0x11, 3, fields, state_version=version, board_capabilities=caps)
        except ValueError:
            pass
        else:
            raise RuntimeError("unsupported or invalid scene patch encoded")
    return {"legacy_unsupported": True, "version_2_offsets": True, "version_3_capabilities": True,
            "version_4_selected_scene": True,
            "malformed_states_rejected": len(malformed), "invalid_patch_values_rejected": 8}


def check_python_parser(codec, steps):
    mismatches = []
    for index, (packet, expect) in enumerate(steps):
        status, _info = codec.parse(packet)
        if status != expect["parse"]:
            mismatches.append({"i": index, "python": status, "spec": expect["parse"]})
    return mismatches


def check_lines(codec, steps, lines, top_output=False):
    failures = []
    bodies = []
    if len(lines) != len(steps):
        return [{"error": "line count", "got": len(lines), "want": len(steps)}]
    float_keys = {"brightness", "softness", "color", "warmth", "color_range", "speed",
                  "saturation", "background", "effective_hue", "output_intensity_a", "output_intensity_b"}
    for index, ((packet, expect), line) in enumerate(zip(steps, lines)):
        got = json.loads(line)
        if got["i"] != index or got["parse"] != expect["parse"]:
            failures.append({"i": index, "got": got, "want_parse": expect["parse"]})
            bodies.append(None)
            continue
        if expect["parse"] != 0:
            if "body" in got:
                failures.append({"i": index, "error": "invalid packet was applied"})
            bodies.append(None)
            continue
        if got["apply"] != expect["apply"]:
            failures.append({"i": index, "apply": got["apply"], "want": expect["apply"]})
        body = bytes.fromhex(got["body"])
        bodies.append(body)
        if len(body) != (208 if top_output else 160):
            failures.append({"i": index, "error": "body length", "n": len(body)})
            continue
        state = codec.decode_state(body)
        if "replay_of" in expect:
            previous = bodies[expect["replay_of"]]
            if previous is None or body[:96] != previous[:96] or body[100:] != previous[100:]:
                failures.append({"i": index, "error": "duplicate body was not the cached response"})
            if state["dedup"] != 1:
                failures.append({"i": index, "error": "dedup flag", "dedup": state["dedup"]})
        for key, want in expect.items():
            if key in ("parse", "apply", "replay_of"):
                continue
            got_value = state[key]
            if key in float_keys:
                if not close(got_value, want):
                    failures.append({"i": index, "field": key, "got": got_value, "want": want})
            elif got_value != want:
                failures.append({"i": index, "field": key, "got": got_value, "want": want})
        if (state["magic"] != codec.STATE_MAGIC or state["version"] != (4 if top_output else 2)
                or state["output_intensity_supported"] is not True or state["reserved"] != b"\x00" * (16 if top_output else 8)):
            failures.append({"i": index, "error": "framing"})
        if top_output and (state["board_capabilities"] != 7 or state["selected_board"] != 0 or state["response_gain"] != 1 or state["edge_response"] != 0 or state["edge_depth"] != 0 or state["primary_intensity"] != 1
                           or state["secondary_intensity"] != 1 or state["secondary_output_enabled"] is not False):
            failures.append({"i": index, "error": "default board controls changed by legacy packet"})
    return failures


def check_feature_codec(codec):
    """Hand-written v5 packets/layout, independent of the firmware codec."""
    fields = {"board": 1, "edge_colour_mode": 6, "edge_colour_spread": 60,
              "pair_link": 3, "musical_role": 2, "musical_attack_ms": 20,
              "musical_release_ms": 250, "ml_enabled": True, "ml_strength": .5}
    body = codec.encode_patch(99, 0x11, 7, fields, state_version=5, board_capabilities=63)
    expected_entries = [(24, 4, 1), (29, 4, 6), (31, 1, 0x42700000), (33, 4, 3),
                        (39, 4, 2), (41, 1, 0x41A00000), (42, 1, 0x437A0000),
                        (43, 2, 1), (44, 1, 0x3F000000)]
    mask = sum(1 << (f-1) for f, _, _ in expected_entries)
    expected = struct.pack("<IIIIIIII", 0x454D0202, 0x11, 99, 7, mask & 0xFFFFFFFF,
                           mask >> 32, 9, 0) + b"".join(struct.pack("<HHI", *e) for e in expected_entries)
    if body != expected or codec.parse(body, state_version=5)[0] != 0:
        raise RuntimeError("extended packet wire mismatch")
    if codec.encode_get(100, 0x11, 1, state_version=5, board_capabilities=63) != struct.pack("<IIIII", 0x454D0201, 100, 0x11, 0, 1):
        raise RuntimeError("v5 explicit GET negotiation")
    if codec.parse(body, state_version=4)[0] != 3:
        raise RuntimeError("old profile accepts extended packet")
    for index, raw in ((20, 0x1000), (28, 1)):
        bad = bytearray(body); struct.pack_into("<I", bad, index, raw)
        if codec.parse(bytes(bad), state_version=5)[0] != 3:
            raise RuntimeError("extended mask/reserved accepted")
    for field, value in ((31, float("nan")), (31, 60.01), (41, 0), (42, 3001), (44, float("inf"))):
        try: codec.encode_patch(100, 0x11, 0, {field: value}, state_version=5, board_capabilities=63)
        except ValueError: pass
        else: raise RuntimeError("v5 invalid field accepted")
    for version, caps in ((4, 7), (5, 7)):
        try: codec.encode_patch(100, 0x11, 0, {"ml_enabled": True}, state_version=version, board_capabilities=caps)
        except ValueError: pass
        else: raise RuntimeError("unsupported ML write accepted")
    state = bytearray(320)
    struct.pack_into("<III", state, 0, 0x454D5354, 5, 1)
    struct.pack_into("<ffffII", state, 144, 1., 1., .25, .25, 0, 63)
    struct.pack_into("<IfIf", state, 176, 1, 1., 2, .65)
    for offset, value in ((192, 1), (196, 6), (208, 1), (212, 3), (216, 2), (220, 1),
                          (224, 2), (236, 0), (240, 2), (256, 1), (264, 1), (268, 8),
                          (272, 7), (276, 10000), (280, 1234), (284, 11), (288, 20),
                          (292, 7), (296, 8), (304, 25), (308, 0)):
        struct.pack_into("<I", state, offset, value)
    for offset, value in ((200, .5), (204, 30.), (228, .75), (232, 60.), (244, .5),
                          (248, 20.), (252, 250.), (260, .5), (300, .4)):
        struct.pack_into("<f", state, offset, value)
    decoded = codec.decode_state(bytes(state))
    if decoded["version"] != 5 or decoded["selected_board"] != 1 or decoded["pair_link"] != 3 or decoded["effective_pair_link"] != 2 or decoded["ml_model_id"] != 1234 or decoded["ml_inference_count"] != 11 or decoded["reserved"] != bytes(8):
        raise RuntimeError("v5 state offsets")
    for offset, raw in ((192, 2), (196, 7), (200, 0x7FC00000), (240, 3), (248, 0), (264, 2), (312, 1)):
        bad = bytearray(state); struct.pack_into("<I", bad, offset, raw)
        try: codec.decode_state(bytes(bad))
        except ValueError: pass
        else: raise RuntimeError("malformed v5 state accepted")
    # Read-only secondary fault state remains readable when transport drops.
    struct.pack_into("<I", state, 164, 61)
    if codec.decode_state(bytes(state))["selected_board"] != 1:
        raise RuntimeError("faulted top lost independent scene readback")
    codec.encode_patch(101, 0x11, 0, {"board": 1, "softness": .5}, state_version=5, board_capabilities=61)
    for fields, caps in (({"board": 1, "softness": .5}, 59), ({"secondary_output_enabled": True}, 61), ({"secondary_intensity": .5}, 61)):
        try: codec.encode_patch(101, 0x11, 0, fields, state_version=5, board_capabilities=caps)
        except ValueError: pass
        else: raise RuntimeError("scene/transport capability separation")
    return {"extended_wire": True, "old_profile_rejected": True, "v5_offsets": True,
            "finite_and_capability_checks": True, "secondary_fault_readback": True}


def check_ml_witness(codec):
    # Literal binary oracle: distinct sequences, times and arrays expose offset aliases.
    raw = bytearray(592); raw[:4] = b"MLR1"
    struct.pack_into("<II9I", raw, 4, 1, 10001, 8, 7, 9900, 1, 0x87654321, 12, 20, 25, 3)
    struct.pack_into("<64f", raw, 48, *(i/64 for i in range(64)))
    struct.pack_into("<8f", raw, 304, *(i/8 for i in range(8)))
    struct.pack_into("<64f", raw, 336, *(1-i/64 for i in range(64)))
    state = codec.decode_ml_state(bytes(raw))
    if (state["query_us"],state["sequence"],state["input_sequence"],state["now_us"],state["model_id"],state["inference_count"],state["last_cycles"],state["max_cycles"],state["rejected_results"]) != (10001,8,7,9900,0x87654321,12,20,25,3):
        raise RuntimeError("ML witness metadata offsets")
    if state["input"][63] != 63/64 or state["latent"][7] != 7/8 or state["reconstruction"][0] != 1 or state["reconstruction"][63] != 1/64:
        raise RuntimeError("ML witness array offsets")
    invalid = bytearray(raw); struct.pack_into("<I", invalid, 24, 0); invalid[48:] = bytes(544)
    decoded = codec.decode_ml_state(bytes(invalid))
    if decoded["valid"] or decoded["inference_count"] != 12 or any(decoded["input"] + decoded["latent"] + decoded["reconstruction"]):
        raise RuntimeError("invalid ML witness must retain counters and clear arrays")
    bads = [bytes(raw[:-1]), b"XXXX" + bytes(raw[4:])]
    for offset, value in ((4,2),(24,2),(48,0x7fc00000),(304,0x7f800000),(336,0x3f800001)):
        bad=bytearray(raw);struct.pack_into("<I",bad,offset,value);bads.append(bytes(bad))
    bad=bytearray(invalid);struct.pack_into("<f",bad,304,.25);bads.append(bytes(bad))
    for bad in bads:
        try: codec.decode_ml_state(bad)
        except ValueError: pass
        else: raise RuntimeError("malformed ML witness accepted")
    return {"metadata_offsets":True,"array_offsets":True,"invalid_zero_arrays_retained_metadata":True,"rejected_shapes":len(bads)}


def run(cmd, log_path: Path):
    completed = subprocess.run([str(x) for x in cmd], capture_output=True, text=True)
    log_path.write_text(completed.stdout + completed.stderr)
    return completed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--compiler", default="/usr/bin/clang++")
    parser.add_argument("--top-output", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    codec = load_codec(root)
    state_versions = check_state_versions(codec)
    feature_codec = check_feature_codec(codec)
    ml_witness = check_ml_witness(codec)
    if codec.golden_patch() != codec.FROZEN_GOLDEN or len(codec.FROZEN_GOLDEN) != 32:
        raise SystemExit("frozen golden packet does not match the Python encoder")
    if codec.crc32(b"123456789") != (zlib.crc32(b"123456789") & 0xFFFFFFFF):
        raise SystemExit("Python CRC diverges from zlib")
    contract = json.loads((root / "docs/contracts/titan-emotiscope-v1.json").read_text())
    donor = contract["donor_control"]
    if (donor["get_sub"] != codec.GET_SUB or donor["patch_sub"] != codec.PATCH_SUB
            or donor["state_magic"] != codec.STATE_MAGIC):
        raise SystemExit("contract discriminators diverged from the codec")
    if donor["state_bytes"] != 160 or contract["capabilities"]["config"] is not True:
        raise SystemExit("contract shape is not the donor control surface")
    if (donor["version"] != 2 or donor["patch_max_bytes"] != 184
            or donor["patch_field_count"] != 20 or donor["patch_field_mask"] != 0xFFFFF
            or len(donor["fields"]) != 20):
        raise SystemExit("independent intensity contract shape")
    for name, field, offset in (("output_intensity_a", 19, 144), ("output_intensity_b", 20, 148)):
        item = donor["fields"][name]
        if (item["id"], item["kind"], item["state_offset"], item["default"]) != (field, "f32", offset, 1.0):
            raise SystemExit("independent intensity contract field")
    top_contract = donor["optional_top_profile"]
    if (top_contract["version"], top_contract["state_bytes"], top_contract["patch_max_bytes"],
            top_contract["patch_field_count"], top_contract["patch_field_mask"], top_contract["profile_capabilities"]) != (3, 176, 208, 23, 0x7FFFFF, 3):
        raise SystemExit("optional top contract shape")
    for name, field, offset, kind in (("primary_intensity", 21, 152, "f32"),
                                     ("secondary_intensity", 22, 156, "f32"),
                                     ("secondary_output_enabled", 23, 160, "bool")):
        item = top_contract["fields"][name]
        if (item["id"], item["kind"], top_contract["state_layout"][name]) != (field, kind, offset):
            raise SystemExit("optional top contract field")
    dual_contract = donor["independent_scene_profile"]
    if tuple(dual_contract[key] for key in ("version", "state_bytes", "patch_max_bytes", "patch_field_count", "patch_field_mask", "profile_capabilities")) != (4, 208, 240, 27, 0x7FFFFFF, 7):
        raise SystemExit("independent scene contract shape")
    witness = source_witness(root)
    if not all(witness.values()):
        (output / "source-witness.json").write_text(json.dumps(witness, indent=2))
        raise SystemExit("source witness failed: " + ",".join(k for k, v in witness.items() if not v))
    steps = corpus(codec)
    parser_gaps = check_python_parser(codec, steps)
    if parser_gaps:
        (output / "parser-gaps.json").write_text(json.dumps(parser_gaps, indent=2))
        raise SystemExit("Python parser disagrees with the handwritten status table")
    (output / "corpus.hex").write_text("".join(packet.hex() + "\n" for packet, _expect in steps))
    binary = output / "emotiscope-controls"
    command = [
        args.compiler, "-std=c++17", "-O1", "-g", "-Wno-narrowing",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-DK1_EMOTISCOPE_TEST_SEAM",
        "-I" + str(root / "platform/ra8p1"),
        "-I" + str(root / "platform/ra8p1/emotiscope_donor"),
        "-I" + str(root / "platform/ra8p1/emotiscope_shims"),
        "-I" + str(root / "tests/host"),
        "-I" + str(root / "src/k1"),
        str(root / "tests/host/test_emotiscope_controls.cpp"),
        str(root / "platform/ra8p1/emotiscope_port.cpp"),
        str(root / "src/k1/core/visual/product_palette.cpp"),
        str(root / "src/k1/core/visual/product_palette_data.cpp"),
        "-o", str(binary),
    ]
    if args.top_output:
        command.insert(1, "-DK1_TITAN_TOP_OUTPUT")
        command.insert(-2, str(root / "platform/ra8p1/emotiscope_secondary.cpp"))
        command.insert(-2, str(root / "platform/ra8p1/emotiscope_ml.cpp"))
    built = run(command, output / "build.log")
    if built.returncode != 0:
        raise SystemExit("build failed: " + str(output / "build.log"))
    behaved = run([binary], output / "behavioral.log")
    corpus_run = run([binary, "--corpus", str(output / "corpus.hex")], output / "corpus.log")
    text = behaved.stdout
    golden_line = next((line.split(" ", 1)[1] for line in text.splitlines() if line.startswith("GOLDEN ")), "")
    default_line = next((line.split(" ", 1)[1] for line in text.splitlines() if line.startswith("DEFAULT ")), "")
    raw_line = next((line.split(" ", 1)[1] for line in text.splitlines() if line.startswith("RAWGET ")), "")
    crc_line = next((line.split(" ", 1)[1] for line in text.splitlines() if line.startswith("CRC_VECTOR ")), "")
    wire = {
        "behavioral_exit": behaved.returncode,
        "corpus_exit": corpus_run.returncode,
        "crc_matches_zlib": crc_line.lower() == "cbf43926",
        "golden_matches_frozen": golden_line == codec.FROZEN_GOLDEN.hex(),
        "default_state": {},
        "raw_color_state": {},
        "corpus_failures": [],
        "state_versions": state_versions,
        "feature_codec": feature_codec,
        "ml_witness": ml_witness,
    }
    if default_line:
        state = codec.decode_state(bytes.fromhex(default_line))
        wire["default_state"] = {
            "brightness": state["brightness"], "mode": state["mode"], "flags": state["flags"],
            "applicability": state["applicability"], "output_brightness": state["output_brightness"],
            "effective_hue": state["effective_hue"], "saturation": state["saturation"],
        }
        wire["default_ok"] = (
            close(state["brightness"], 1) and state["mode"] == 1 and state["flags"] == 39
            and state["applicability"] == 0xFF8B and state["output_brightness"] == 100
            and state["palette"] == 1 and close(state["saturation"], 0.75) and close(state["effective_hue"], 0.33)
            and state["version"] == (4 if args.top_output else 2) and state["output_intensity_supported"] is True
            and state["output_intensity_a"] == 1.0 and state["output_intensity_b"] == 1.0
        )
    if raw_line:
        state = codec.decode_state(bytes.fromhex(raw_line))
        wire["raw_color_state"] = {"color": state["color"], "effective_hue": state["effective_hue"]}
        wire["raw_ok"] = close(state["color"], 1.4) and close(state["effective_hue"], 0.4)
    if corpus_run.returncode == 0:
        lines = [line for line in corpus_run.stdout.splitlines() if line.startswith("{")]
        wire["corpus_failures"] = check_lines(codec, steps, lines, args.top_output)
    wire["source_witness"] = witness
    wire["pass"] = (
        behaved.returncode == 0 and corpus_run.returncode == 0 and wire["crc_matches_zlib"]
        and wire["golden_matches_frozen"] and wire.get("default_ok") and wire.get("raw_ok")
        and not wire["corpus_failures"]
    )
    (output / "result.json").write_text(json.dumps(wire, indent=2))
    if not wire["pass"]:
        raise SystemExit("controls proof failed: " + str(output / "result.json"))
    print(json.dumps({"pass": True, "result": str(output / "result.json")}))


if __name__ == "__main__":
    main()
