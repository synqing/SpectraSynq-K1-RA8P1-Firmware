"""Compile actual command handling and fail on any observer-driven pair lifecycle."""
from pathlib import Path
import json
import re
import shutil
from palette_renderer_overlay import apply_palette_overlay
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/serial-studio"))
import live_protocol as proto

with tempfile.TemporaryDirectory(prefix="titan-owner-") as tmp:
    out = Path(tmp)
    sources=out/'k1'
    shutil.copytree(ROOT/'src/k1', sources)
    apply_palette_overlay(sources)
    (out / "build_identity.h").write_text('#define K1_BUILD_ID "' + '0'*64 + '"\n#define K1_SOURCE_PIN "HOST"\n')
    (out / "patch.bin").write_bytes(proto.pack_patch(0, {"visual_a.vp_waveform_shift_rate": 200.0}, request_id=7, boot_id=1))
    (out / 'performance.bin').write_bytes(proto.pack_performance(0, {'focus_a.level_gain': 2., 'visual_a.vp_waveform_shift_rate': 20.}, request_id=81, boot_id=1))
    (out / 'scene.bin').write_bytes(proto.pack_performance(0, {'focus_a.level_gain': 1., 'visual_a.vp_waveform_shift_rate': 160.}, request_id=82, boot_id=1, palette=36))
    # Uninteresting hardware observations return immutable zero records. The
    # lifecycle calls above are hostile, counted stubs and cannot be generic.
    header = (ROOT / "platform/ra8p1/ws281x_gpt_dma_hw_pair.h").read_text()
    exclude = {"poll", "submit", "transmitting", "busy", "ready"}
    definitions = ['#include "ws281x_gpt_dma_hw_pair.h"']
    for ret, name, args in re.findall(r"\n([a-zA-Z_][\w\s*]*?)\s*(k1_ws281x_gpt_dma_hw_pair_\w+)\(([^;]*)\);", header):
        if name.removeprefix("k1_ws281x_gpt_dma_hw_pair_") in exclude:
            continue
        if '*' in ret:
            if 'char' in ret: body='return "host-pair";'
            else: body=f'static {ret.replace("const", "").replace("*", "")} value{{}}; return &value;'
        elif ret.strip() == 'void': body=''
        else: body='return 0;'
        definitions.append(f'extern "C" {ret} {name}({args}) {{ {body} }}')
    (out / "stubs.cpp").write_text('\n'.join(definitions))
    names = json.loads((ROOT / "docs/import-slices.json").read_text())["product"]
    extra = ["platform/ra8p1/k1_live_runtime.cpp", "platform/ra8p1/k1_live_protocol.cpp",
             "platform/ra8p1/k1_live_clock.c", "platform/ra8p1/k1_live_boot_identity.cpp",
             "platform/ra8p1/palette_runtime.cpp", "src/k1/core/audio/k1_audio_hop.c",
             "src/k1/core/visual/frame_blend.cpp", "src/k1/core/visual/product_runtime_policy.cpp",
             "platform/ra8p1/k1_status_led.c", "platform/ra8p1/titan_status_gpio.c",
             "platform/ra8p1/titan_led2_phy.c"]
    cmd = ['c++','-std=c++17','-O2','-ffunction-sections','-Wl,-dead_strip',
           '-DK1_LIVE_RUNTIME=1','-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_WS2816_GPT_PAIR=1',
           '-DK1_STATUS_GPIO_STUB=1','-DK1_LED2_PHY_STUB=1',
           '-DK1_LIVE_EMIT=1','-DK1_LIVE_BOOT_EMIT=0','-DK1_LIVE_BOOT_MODE=8u',
           '-DK1_PALETTE_AUTOSTART=1','-DK1_PALETTE_MORPH=1',
           *['-I'+str(p) for p in [sources,ROOT/'src',ROOT/'platform/ra8p1',ROOT/'tests/target',out]],
           *[str(ROOT/p) for p in extra],
           *[str(sources/p) for p in names if p.endswith('.cpp')],
           str(ROOT/'tests/host/test_control_owner.cpp'),str(out/'stubs.cpp'),'-o',str(out/'test')]
    subprocess.run(cmd, check=True, timeout=120)
    subprocess.run([str(out/'test'),str(out/'patch.bin'),str(out/'performance.bin'),str(out/'scene.bin')], check=True, timeout=15)

    # Mutation proof: reintroduce the historical command-entry lifecycle call in
    # a temporary copy only. The real request test must reject it as an assertion,
    # not merely fail compilation. No product source is changed by this check.
    fixture = (ROOT / "platform/ra8p1/fixture_app.cpp").read_text()
    (out / "bad_fixture.cpp").write_text(fixture.replace(
        "void execute() {", "void execute() { k1_ws281x_gpt_dma_hw_pair_poll();", 1))
    harness = (ROOT / "tests/host/test_control_owner.cpp").read_text()
    (out / "bad_test.cpp").write_text(harness.replace(
        '#include "fixture_app.cpp"', '#include "bad_fixture.cpp"'))
    bad_cmd = [str(out/'bad_test.cpp') if p == str(ROOT/'tests/host/test_control_owner.cpp')
               else str(out/'bad_test') if p == str(out/'test') else p for p in cmd]
    subprocess.run(bad_cmd, check=True, timeout=120)
    bad = subprocess.run([str(out/'bad_test'),str(out/'patch.bin'),str(out/'performance.bin'),str(out/'scene.bin')],
                         capture_output=True, text=True, timeout=15)
    assert bad.returncode != 0 and "Assertion failed" in bad.stderr, bad
    print("CONTROL_OWNER_MUTATION_REJECTED legacy command-side lifecycle poll")
