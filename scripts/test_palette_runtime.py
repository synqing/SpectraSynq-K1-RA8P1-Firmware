#!/usr/bin/env python3
"""Pinned renderer vs palette-transition derivative.

Live-audio move/fade/colour in the derivative is allowed to differ from pinned.
A second overlay apply must still be rejected.
"""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from verify_imports import ROOT
from palette_renderer_overlay import apply_palette_overlay

COMPILE_TIMEOUT = 120
RUN_TIMEOUT = 60

names=json.loads((ROOT/'docs/import-slices.json').read_text())['product']
digests=[]
with tempfile.TemporaryDirectory(prefix='k1-palettes-') as temp:
    out=Path(temp)
    (out/'build_identity.h').write_text('#define K1_BUILD_ID "HOST-PALETTES"\n#define K1_SOURCE_PIN "HOST-PALETTES"\n')
    for morph in (False,True):
        sources=ROOT/'src/k1'
        if morph:
            sources=out/'k1'
            shutil.copytree(ROOT/'src/k1',sources)
            apply_palette_overlay(sources)
            # Overlay rejects unexpected/already-transformed source.
            try: apply_palette_overlay(sources)
            except RuntimeError: pass
            else: raise AssertionError('overlay accepted a source mutation')
        common=['c++','-std=c++17','-O2','-ffp-contract=off','-fno-fast-math',
                '-I'+str(sources),'-I'+str(ROOT/'platform/ra8p1'),
                '-I'+str(ROOT/'tests/target'),'-I'+str(out),
                str(ROOT/'platform/ra8p1/palette_runtime.cpp'),
                *[str(sources/p) for p in names if p.endswith('.cpp')]]
        if morph: common+=['-DK1_PALETTE_MORPH=1']
        for suite in (('runtime','protocol','transition','centre') if morph else ('runtime','protocol')):
            executable=out/(suite+str(morph))
            command=common+[str(ROOT/f'tests/host/test_palette_{suite}.cpp'),'-o',str(executable)]
            if suite=='protocol': command+=['-DK1_PALETTE_RUNTIME=1','-DK1_STATUS_GPIO_STUB=1','-DK1_LED2_PHY_STUB=1',
                str(ROOT/'platform/ra8p1/fixture_app.cpp'),
                str(ROOT/'platform/ra8p1/k1_status_led.c'),
                str(ROOT/'platform/ra8p1/titan_status_gpio.c'),
                str(ROOT/'platform/ra8p1/titan_led2_phy.c')]
            subprocess.run(command,check=True,timeout=COMPILE_TIMEOUT)
            result=subprocess.check_output([str(executable)],text=True,timeout=RUN_TIMEOUT)
            print(('MORPH ' if morph else 'PINNED ')+result,end='')
            if suite=='protocol':
                subprocess.run(command+['-DK1_PALETTE_WS2816=1'],check=True,timeout=COMPILE_TIMEOUT)
                print('WS2816 '+subprocess.check_output([str(executable)],text=True,timeout=RUN_TIMEOUT),end='')
                for backend in ([], ['-DK1_PALETTE_WS2816=1']):
                    subprocess.run(command+backend+['-DK1_PALETTE_AUTOSTART=1'],check=True,timeout=COMPILE_TIMEOUT)
                    print('AUTOSTART '+('WS2816 ' if backend else 'WS2812 ')+
                          subprocess.check_output([str(executable)],text=True,timeout=RUN_TIMEOUT),end='')
                    if morph and backend:
                        # Reintroduce the actual reset regression in a disposable source.
                        bad_source=out/'fixture_bad_boot.cpp'
                        source=(ROOT/'platform/ra8p1/fixture_app.cpp').read_text()
                        marker='config.mode_a = k1::titan::kLiveAudioBootMode;'
                        assert source.count(marker)==1
                        bad_source.write_text(source.replace(
                            marker,
                            'config.mode_a = k1::titan::kDiagnosticBounceMode;'))
                        bad_command=[str(bad_source) if part==str(ROOT/'platform/ra8p1/fixture_app.cpp') else part for part in command]
                        subprocess.run(bad_command+backend+['-DK1_PALETTE_AUTOSTART=1'],check=True,timeout=COMPILE_TIMEOUT)
                        broken=subprocess.run([str(executable)],cwd=out,capture_output=True,text=True,timeout=30)
                        assert broken.returncode!=0, 'bounce autostart regression escaped'
                        print('AUTOSTART_MUTATION_PASS bounce_boot_rejected=true')
            if suite=='runtime':
                digests.append(next(line for line in result.splitlines() if line.startswith('COMPATIBILITY_DIGEST=')))
                if not morph:
                    source = (ROOT/'platform/ra8p1/palette_runtime.cpp').read_text()
                    marker = (
                        'channel->prepareAudio(governed_audio);\n'
                        '        // Live/silence path: clear then DualMCU previousFrame seed.\n'
                        '        // Skipping clear here saturates (snap-freeze).\n'
                        '        channel->clearFrame();')
                    assert source.count(marker) == 1
                    bad_source = out/'palette_runtime_no_clear.cpp'
                    bad_source.write_text(source.replace(
                        marker, 'channel->prepareAudio(governed_audio);'))
                    bad_command = [
                        str(bad_source) if part == str(ROOT/'platform/ra8p1/palette_runtime.cpp')
                        else part for part in command]
                    define_at = bad_command.index('-o')
                    bad_command = (bad_command[:define_at] +
                                   ['-DK1_SNAP_FREEZE_ONLY=1'] +
                                   bad_command[define_at:])
                    subprocess.run(bad_command, check=True, timeout=COMPILE_TIMEOUT)
                    broken = subprocess.run(
                        [str(executable)], cwd=out, capture_output=True, text=True,
                        timeout=30)
                    assert broken.returncode != 0 and 'SNAP_FREEZE' in broken.stderr, (
                        'snap-freeze escaped without caller clearFrame')
                    print('SNAP_FREEZE_MUTATION_PASS caller_clear_required=true')
    backend_src=ROOT/'tests/host/test_palette_backend.cpp'
    backend_common=['c++','-std=c++17','-O2','-ffp-contract=off',
                    '-I'+str(ROOT/'src/k1'),'-I'+str(ROOT/'platform/ra8p1'),
                    '-I'+str(ROOT/'tests/target'),'-I'+str(out),str(backend_src)]
    for label,defs in (
        ('unknown',[]),
        ('gpio_diagnostic',['-DK1_PALETTE_RUNTIME=1']),
        ('ws2816_gpio',['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_WS2816=1']),
        ('gpt_dma',['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_GPT_DMA=1']),
        ('ws2816_gpt_pair',['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_WS2816_GPT_PAIR=1']),
    ):
        executable=out/('backend-'+label)
        subprocess.run(backend_common+defs+['-o',str(executable)],check=True,timeout=COMPILE_TIMEOUT)
        text=subprocess.check_output([str(executable)],text=True,timeout=RUN_TIMEOUT)
        assert f'configured={label}' in text, text
        assert 'physical_admission=unproven' in text
        print('BACKEND '+text,end='')
    conflict=out/'backend-conflict'
    conflicted=subprocess.run(
        backend_common+['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_GPT_DMA=1',
                        '-DK1_PALETTE_WS2816=1','-o',str(conflict)],
        capture_output=True,text=True,timeout=COMPILE_TIMEOUT)
    assert conflicted.returncode!=0, 'GPT DMA and WS2816 backends compiled together'
    print('BACKEND_EXCLUSIVE_PASS gpt_dma_vs_ws2816=true')
    pair_vs_gpt=subprocess.run(
        backend_common+['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_GPT_DMA=1',
                        '-DK1_PALETTE_WS2816_GPT_PAIR=1','-o',str(out/'backend-pair-gpt')],
        capture_output=True,text=True,timeout=COMPILE_TIMEOUT)
    assert pair_vs_gpt.returncode!=0, 'pair and single-lane GPT backends compiled together'
    pair_vs_gpio=subprocess.run(
        backend_common+['-DK1_PALETTE_RUNTIME=1','-DK1_PALETTE_WS2816=1',
                        '-DK1_PALETTE_WS2816_GPT_PAIR=1','-o',str(out/'backend-pair-gpio')],
        capture_output=True,text=True,timeout=COMPILE_TIMEOUT)
    assert pair_vs_gpio.returncode!=0, 'pair and GPIO WS2816 backends compiled together'
    print('BACKEND_EXCLUSIVE_PASS pair_vs_gpt=true pair_vs_gpio=true')
    assert len(digests)==2, 'need pinned and morph runtime digests'
    assert digests[0]!=digests[1], 'morph live-audio path did not differ from pinned'
    print('PALETTE_COMPATIBILITY_PASS independent_executables=2 overlay_mutation_rejected=true live_audio_differs=true')
