#!/usr/bin/env python3
"""Host compile-and-run for the live wire codec plus Python unpacker."""
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from verify_imports import ROOT

NAMES = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-live-protocol-") as temp:
        out = Path(temp)
        sources = out / "k1"
        shutil.copytree(ROOT / "src/k1", sources)
        exe = out / "test_live_protocol"
        command = [
            "c++",
            "-std=c++17",
            "-O2",
            "-ffp-contract=off",
            "-I" + str(sources),
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/k1_live_clock.c"),
            str(ROOT / "platform/ra8p1/k1_live_runtime.cpp"),
            str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
            str(ROOT / "src/k1/core/audio/k1_audio_hop.c"),
            *[str(sources / name) for name in NAMES if name.endswith(".cpp")],
            str(ROOT / "tests/host/test_live_protocol.cpp"),
            "-o",
            str(exe),
        ]
        subprocess.run(command, check=True)
        output = subprocess.check_output([str(exe)], text=True)
        sys.stdout.write(output)
        if "LIVE_PROTOCOL_HOST_PASS" not in output:
            raise SystemExit("missing LIVE_PROTOCOL_HOST_PASS")
        apply_exe = out / "apply_patch_hex"
        apply_cmd = [
            "c++",
            "-std=c++17",
            "-O2",
            "-ffp-contract=off",
            "-I" + str(sources),
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
            str(ROOT / "tests/host/apply_patch_hex.cpp"),
            "-o",
            str(apply_exe),
        ]
        subprocess.run(apply_cmd, check=True)
        sys.path.insert(0, str(ROOT / "tools/serial-studio"))
        import live_protocol as proto

        packed = proto.pack_config({"mode_a": 32, "emit_on": 0, "flags": 1,
                                    "last_patch_request_id": 99, "boot_id": 7,
                                    "version": 2})
        decoded = proto.unpack_config(packed)
        if len(packed) != proto.CONFIG_BYTES_V2:
            raise SystemExit("python v2 config size is not 1156")
        if decoded["mode_a"] != 32 or decoded["emit_on"] != 0:
            raise SystemExit("python config roundtrip failed")
        if decoded["last_patch_request_id"] != 99:
            raise SystemExit("python last_patch_request_id roundtrip failed")
        if decoded["boot_id"] != 7:
            raise SystemExit("python boot_id roundtrip failed")
        legacy = packed[: proto.CONFIG_BYTES_V2_LEGACY]
        legacy_decoded = proto.unpack_config(legacy)
        if legacy_decoded["boot_id"] != 0:
            raise SystemExit("legacy 1152 config must not invent a boot_id")
        snap = bytearray(proto.SNAPSHOT_BYTES)
        import struct as _st
        _st.pack_into("<I", snap, 0, proto.SNAP_MAGIC)
        _st.pack_into("<I", snap, 812, 12)
        _st.pack_into("<I", snap, 828, 11)
        unpacked_snap = proto.unpack_snapshot(bytes(snap))
        if unpacked_snap["last_rendered_request_id"] != 12 or unpacked_snap["superseded_request_id"] != 11:
            raise SystemExit("python snapshot render receipt offsets failed")
        patch = proto.pack_patch(8, {"visual_a.hue_position": 0.42}, request_id=7, boot_id=3)
        unpacked = proto.unpack_patch(patch)
        if unpacked["revision"] != 8 or unpacked["request_id"] != 7 or unpacked["boot_id"] != 3:
            raise SystemExit("python patch header failed")
        if abs(unpacked["fields"]["visual_a.hue_position"] - 0.42) > 1e-6:
            raise SystemExit("python patch roundtrip failed")
        b_patch = proto.pack_patch(8, {"visual_b.hue_position": 0.37}, request_id=9)
        b_unpacked = proto.unpack_patch(b_patch)
        if b_unpacked["channel"] != 1 or "visual_b.hue_position" not in b_unpacked["fields"]:
            raise SystemExit("visual_b encoded as visual_a")
        try:
            proto.pack_patch(8, {"unrelated.hue_position": 0.2}, request_id=1)
        except ValueError:
            pass
        else:
            raise SystemExit("unknown prefix must be rejected")
        try:
            proto.pack_patch(8, {"emit_on": 0}, request_id=1)
        except ValueError:
            pass
        else:
            raise SystemExit("emit_on must be a forbidden patch field")
        if proto.SCHEMA_SHA256 != proto.CONTRACT.get("schema_version", 1) and len(proto.SCHEMA_SHA256) != 64:
            raise SystemExit("schema hash missing")
        b_hex = proto.pack_patch(1, {"visual_b.hue_position": 0.37}, request_id=99, boot_id=1).hex()
        apply_out = subprocess.check_output([str(apply_exe), b_hex], text=True)
        sys.stdout.write(apply_out)
        if "ch=1" not in apply_out or "b=0.370000" not in apply_out or "a=0.110000" not in apply_out:
            raise SystemExit("python encoder / C++ apply channel B failed")
        print("LIVE_PROTOCOL_PY_PASS", proto.SCHEMA_SHA256)

        consume_exe = out / "test_r4_consumption"
        consume_cmd = [
            "c++",
            "-std=c++17",
            "-O2",
            "-ffp-contract=off",
            "-fno-fast-math",
            "-DK1_LIVE_RUNTIME=1",
            "-DK1_PALETTE_RUNTIME=1",
            "-I" + str(sources),
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
            str(ROOT / "platform/ra8p1/palette_runtime.cpp"),
            *[str(sources / name) for name in NAMES if name.endswith(".cpp")],
            str(ROOT / "src/k1/core/visual/frame_blend.cpp"),
            str(ROOT / "src/k1/core/visual/product_runtime_policy.cpp"),
            str(ROOT / "tests/host/test_r4_consumption.cpp"),
            "-o",
            str(consume_exe),
        ]
        subprocess.run(consume_cmd, check=True)
        consume_out = subprocess.check_output([str(consume_exe)], text=True)
        sys.stdout.write(consume_out)
        for stamp in (
            "R4_CONSUMPTION_PREVIEW_PASS",
            "R4_CONSUMPTION_EFFECT_PASS",
            "R4_CONSUMPTION_DWELL_PASS",
            "R4_CONSUMPTION_PENDING_PASS",
            "R4_CONSUMPTION_PASS",
        ):
            if stamp not in consume_out:
                raise SystemExit(f"missing {stamp}")

        boot_exe = out / "test_live_boot_identity"
        boot_cmd = [
            "c++",
            "-std=c++17",
            "-O2",
            "-ffp-contract=off",
            "-I" + str(sources),
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "platform/ra8p1"),
            "-I" + str(ROOT / "tests/target"),
            str(ROOT / "platform/ra8p1/k1_live_boot_identity.cpp"),
            str(ROOT / "platform/ra8p1/k1_live_protocol.cpp"),
            str(ROOT / "tests/host/test_live_boot_identity.cpp"),
            "-o",
            str(boot_exe),
        ]
        subprocess.run(boot_cmd, check=True)
        boot_out = subprocess.check_output([str(boot_exe)], text=True)
        sys.stdout.write(boot_out)
        if "LIVE_BOOT_IDENTITY_HOST_PASS" not in boot_out:
            raise SystemExit("missing LIVE_BOOT_IDENTITY_HOST_PASS")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
