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
        saturation=0.75, background=0.25, effective_hue=0.33, flags=39, applicability=0xDFDF,
        output_brightness=100, last_status=0, accepted_request_id=0)
    add(get, parse=0, apply=0, dedup=1, replay_of=0)
    add(struct.pack("<I", 8) + b"\x00" * 28, parse=3)
    golden = codec.FROZEN_GOLDEN
    if codec.encode_patch(1, 0x11, 0, {"brightness": 0.5}) != golden:
        raise RuntimeError("Python encoder drifted from the frozen golden packet")
    add(golden, parse=0, apply=0, revision=1, dedup=0, changed=1, mode_transition=0,
        brightness=0.5, softness=0.25, mode=1, output_brightness=100, last_status=0,
        flags=39, applicability=0xDFDF)
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
    add(codec.encode_patch(10, 0x11, 3, {"warmth": 0.15, "color": 0.40}),
        parse=0, apply=0, revision=4, changed=1, dedup=0, warmth=0.15, color=0.40,
        effective_hue=0.40, brightness=0.2, softness=0.5, mode=1, flags=39,
        applicability=0xDFDF, output_brightness=100, mode_transition=0)
    scene = {
        "brightness": 0.10, "softness": 0.20, "color": 0.30, "warmth": 0.40,
        "color_range": 0.50, "speed": 0.60, "saturation": 0.70, "background": 0.80,
        "current_mode": 6, "mirror_mode": 0, "screensaver": 0, "temporal_dithering": 0,
        "auto_color_cycle": 1, "reverse_color_range": 1,
        "hardware_output_brightness": 80, "emission_enabled": 0,
    }
    add(codec.encode_patch(8, 0x11, 4, scene),
        parse=0, apply=0, revision=5, changed=1, dedup=0, mode_transition=1, mode=6,
        brightness=0.10, softness=0.20, color=0.30, warmth=0.40, color_range=0.50,
        speed=0.60, saturation=0.70, background=0.80, effective_hue=0.30,
        output_brightness=80, flags=24, applicability=0xFFDF, last_status=0,
        accepted_request_id=8)
    add(codec.encode_get(9, 0x11),
        parse=0, apply=0, revision=5, changed=0, dedup=0, mode=6, brightness=0.10,
        output_brightness=80, flags=24, applicability=0xFFDF, accepted_request_id=8,
        request_id=9, last_status=0)
    # Output precision: field 17 bool, owner-held, GET flag bit 6 (64). Not a donor field.
    add(codec.encode_patch(11, 0x11, 5, {"output_precision": 1}),
        parse=0, apply=0, revision=6, changed=1, dedup=0, mode=6, flags=24 | 64,
        applicability=0xFFDF, output_brightness=80, accepted_request_id=11, last_status=0)
    add(codec.encode_get(12, 0x11),
        parse=0, apply=0, revision=6, changed=0, dedup=0, flags=24 | 64, request_id=12)
    add(codec.encode_patch(13, 0x11, 6, {"output_precision": 1}),
        parse=0, apply=0, revision=6, changed=0, dedup=0, flags=24 | 64)
    add(codec.raw_patch(59, 0x11, 6, 1 << 16, [(17, codec.KIND_BOOL, 2)]), parse=3)
    add(codec.raw_patch(60, 0x11, 6, 1 << 17, [(18, codec.KIND_BOOL, 1)]), parse=3)
    add(codec.raw_patch(61, 0x11, 6, 1 << 16, [(17, codec.KIND_F32, half_bits)]), parse=3)
    add(codec.encode_patch(14, 0x11, 6, {"output_precision": 0}),
        parse=0, apply=0, revision=7, changed=1, dedup=0, flags=24, accepted_request_id=14)
    # Palette: field 18 u32 0..44 (0 native), owner-held, state offset 140.
    add(codec.encode_patch(15, 0x11, 7, {"palette": 24}),
        parse=0, apply=0, revision=8, changed=1, dedup=0, palette=24, flags=24, accepted_request_id=15)
    add(codec.encode_get(16, 0x11), parse=0, apply=0, revision=8, changed=0, palette=24, request_id=16)
    add(codec.raw_patch(62, 0x11, 8, 1 << 17, [(18, codec.KIND_U32, 45)]), parse=3)
    add(codec.raw_patch(63, 0x11, 8, 1 << 18, [(19, codec.KIND_U32, 1)]), parse=3)
    add(codec.encode_patch(17, 0x11, 8, {"palette": 0}),
        parse=0, apply=0, revision=9, changed=1, dedup=0, palette=0, accepted_request_id=17)
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
    full_fields = dict(scene, output_precision=0, palette=0,
                       output_intensity_a=0.5, output_intensity_b=0.5)
    full_packet = codec.encode_patch(120, 0x11, 12, full_fields)
    if len(full_packet) != 184 or struct.unpack_from("<II", full_packet, 16) != (0xFFFFF, 20):
        raise RuntimeError("full 20-field packet shape")
    add(full_packet, parse=0, apply=0, revision=13, changed=1,
        output_intensity_a=0.5, output_intensity_b=0.5)
    add(full_packet + b"\x00", parse=3)
    return steps


def check_state_versions(codec):
    """Independent byte fixtures exercise legacy support and version-2 rejection."""
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
    return {"legacy_unsupported": True, "version_2_offsets": True, "version_3_capabilities": True,
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
        if len(body) != (176 if top_output else 160):
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
        if (state["magic"] != codec.STATE_MAGIC or state["version"] != (3 if top_output else 2)
                or state["output_intensity_supported"] is not True or state["reserved"] != b"\x00" * 8):
            failures.append({"i": index, "error": "framing"})
        if top_output and (state["board_capabilities"] != 3 or state["primary_intensity"] != 1
                           or state["secondary_intensity"] != 1 or state["secondary_output_enabled"] is not False):
            failures.append({"i": index, "error": "default board controls changed by legacy packet"})
    return failures


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
        str(root / "tests/host/test_emotiscope_controls.cpp"),
        str(root / "platform/ra8p1/emotiscope_port.cpp"),
        "-o", str(binary),
    ]
    if args.top_output:
        command.insert(1, "-DK1_TITAN_TOP_OUTPUT")
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
            and state["applicability"] == 0xDFDF and state["output_brightness"] == 100
            and close(state["saturation"], 0.75) and close(state["effective_hue"], 0.33)
            and state["version"] == (3 if args.top_output else 2) and state["output_intensity_supported"] is True
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
