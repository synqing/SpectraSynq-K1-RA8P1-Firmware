#!/usr/bin/env python3
"""Compile diagnostic packing and test runner rejection without opening CDC."""
from pathlib import Path
import copy
import struct
import tempfile
import zlib
from run_host import run
from verify_imports import ROOT
from run_ws281x_diag import request_and_wire, check_led_reply

with tempfile.TemporaryDirectory(prefix="k1-ws281x-diag-") as temp:
    binary = Path(temp) / "test"
    run(["c++", "-std=c++17", "-O2", "-I" + str(ROOT / "platform/ra8p1"),
         str(ROOT / "tests/host/test_ws281x_diag.cpp"), "-o", str(binary)])
    print(run([str(binary)]), end="")

request, wire = request_and_wire("ws2812", "P601", 128, 8, 0x12, 0x34, 0x56)
assert len(request) == 32 and struct.unpack("<8I", request) == (1,1,0,128,8,0x12,0x34,0x56)
assert wire == bytes([0x34,0x12,0x56]) * 8 + bytes(360)
_, wire16 = request_and_wire("ws2816c", "P004", 128, 8, 0x12ab, 0x34cd, 0x56ef)
assert wire16[:6] == bytes.fromhex("34cd12ab56ef") and len(wire16) == 768
for values in [("ws2812","P601",0,0,1,0,0), ("ws2812","P601",129,8,1,0,0),
               ("ws2812","P601",128,8,256,0,0), ("ws2812","P603",128,8,1,0,0),
               ("ws2812","P601",128,129,1,0,0)]:
    try:
        request_and_wire(*values)
    except ValueError:
        pass
    else:
        raise AssertionError("bad runner request accepted")
reply = dict(op=13,version=1,profile=1,pin=0,pixels=128,lit_pixels=8,bytes=384,
             crc=zlib.crc32(wire),result=0,pin_config_error=0,pfs_after=4,
             emit_cycles=2400000,latch_cycles=300000,bit_period_min_cycles=1250,
             bit_period_max_cycles=1260,wire_timing_measured=False,photons="NOT_CLAIMED")
check_led_reply(reply,"ws2812","P601",128,8,wire,1000000000)
for key, value in [("crc",0),("pfs_after",65540),("latch_cycles",1),
                   ("profile",3),("pin_config_error",1),("photons","PASS")]:
    bad=copy.deepcopy(reply); bad[key]=value
    try:
        check_led_reply(bad,"ws2812","P601",128,8,wire,1000000000)
    except RuntimeError:
        pass
    else:
        raise AssertionError(f"bad reply accepted: {key}")
print("K1_WS281X_RUNNER=PASS request_rejection=PASS readback_rejection=PASS")
