#!/usr/bin/env python3
"""Compare pinned VP composition with the optional palette-transition derivative."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from verify_imports import ROOT
from palette_renderer_overlay import apply_palette_overlay

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
            subprocess.run(command,check=True)
            result=subprocess.check_output([str(executable)],text=True)
            print(('MORPH ' if morph else 'PINNED ')+result,end='')
            if suite=='protocol':
                subprocess.run(command+['-DK1_PALETTE_WS2816=1'],check=True)
                print('WS2816 '+subprocess.check_output([str(executable)],text=True),end='')
                for backend in ([], ['-DK1_PALETTE_WS2816=1']):
                    subprocess.run(command+backend+['-DK1_PALETTE_AUTOSTART=1'],check=True)
                    print('AUTOSTART '+('WS2816 ' if backend else 'WS2812 ')+
                          subprocess.check_output([str(executable)],text=True),end='')
                    if morph and backend:
                        # Reintroduce the actual reset regression in a disposable source.
                        bad_source=out/'fixture_bad_boot.cpp'
                        source=(ROOT/'platform/ra8p1/fixture_app.cpp').read_text()
                        marker='config.palette_a = 33U; config.palette_b = 43U; config.brightness = 128U;'
                        assert source.count(marker)==1
                        bad_source.write_text(source.replace(marker,marker+'\n  config.flags = 5U;'))
                        bad_command=[str(bad_source) if part==str(ROOT/'platform/ra8p1/fixture_app.cpp') else part for part in command]
                        subprocess.run(bad_command+backend+['-DK1_PALETTE_AUTOSTART=1'],check=True)
                        broken=subprocess.run([str(executable)],cwd=out,capture_output=True,text=True)
                        assert broken.returncode!=0 and 'automatic_cycle' in broken.stderr, 'boot regression escaped'
                        print('AUTOSTART_MUTATION_PASS single_palette_reset_regression_rejected=true')
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
                    subprocess.run(bad_command, check=True)
                    broken = subprocess.run(
                        [str(executable)], cwd=out, capture_output=True, text=True)
                    assert broken.returncode != 0 and 'SNAP_FREEZE' in broken.stderr, (
                        'snap-freeze escaped without caller clearFrame')
                    print('SNAP_FREEZE_MUTATION_PASS caller_clear_required=true')
    assert len(digests)==2 and digests[0]==digests[1], 'disabled-transition VP differs from pinned renderer'
    print('PALETTE_COMPATIBILITY_PASS independent_executables=2 overlay_mutation_rejected=true')
