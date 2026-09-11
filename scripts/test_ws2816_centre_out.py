#!/usr/bin/env python3
"""Host-only frame and rejection checks for the dual-lane WS2816 runner."""
import copy
import zlib

from run_ws2816_centre_out import (
    LANE_BYTES,
    P004,
    P601,
    centre_out_lanes,
    check_frame_reply,
    pack_grb48,
)

assert pack_grb48(0x12AB, 0x34CD, 0x56EF) == bytes.fromhex("34cd12ab56ef")

lane_a, lane_b = centre_out_lanes(0, 0x7A3C)
assert len(lane_a) == LANE_BYTES and len(lane_b) == LANE_BYTES
assert lane_a[79 * 6 : 80 * 6] == bytes.fromhex("00007a3c0000")
assert lane_b[0:6] == bytes.fromhex("000000007a3c")
assert not any(lane_a[: 79 * 6]) and not any(lane_b[6:])

edge_a, edge_b = centre_out_lanes(79, 0x7A3C)
assert edge_a[0:6] == bytes.fromhex("00007a3c0000")
assert edge_b[79 * 6 : 80 * 6] == bytes.fromhex("000000007a3c")
assert any(edge_a[index * 6 : index * 6 + 6] for index in range(8))
assert any(edge_b[index * 6 : index * 6 + 6] for index in range(72, 80))

for values in ((-1, 1), (80, 1), (0, 0), (0, 65536)):
    try:
        centre_out_lanes(*values)
    except ValueError:
        pass
    else:
        raise AssertionError(f"bad WS2816 motion request accepted: {values}")

reply = {
    "op": 14,
    "version": 1,
    "profile": 4,
    "pin": P601,
    "pixels": 80,
    "bytes": LANE_BYTES,
    "crc": zlib.crc32(lane_a),
    "result": 0,
    "pin_config_error": 0,
    "pfs_after": 4,
    "emit_cycles": 5112000,
    "latch_cycles": 300000,
    "wire_timing_measured": False,
    "photons": "NOT_CLAIMED",
}
check_frame_reply(reply, P601, lane_a)
reply_b = dict(reply, pin=P004, crc=zlib.crc32(lane_b))
check_frame_reply(reply_b, P004, lane_b)
for key, value in (
    ("profile", 1),
    ("pixels", 79),
    ("bytes", 479),
    ("crc", 0),
    ("result", 1),
    ("pfs_after", 65540),
    ("emit_cycles", 0),
    ("photons", "PASS"),
):
    bad = copy.deepcopy(reply)
    bad[key] = value
    try:
        check_frame_reply(bad, P601, lane_a)
    except RuntimeError:
        pass
    else:
        raise AssertionError(f"bad WS2816 reply accepted: {key}")

print("K1_WS2816_CENTRE_OUT=PASS centre_79_80=PASS edges_0_159=PASS true16=PASS rejection=PASS")
