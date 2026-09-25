#!/usr/bin/env python3
from pathlib import Path
import tempfile
from run_host import run
from verify_imports import ROOT

src = (ROOT / 'platform/ra8p1/pdm_target.c').read_text()
applied = src.count('sincrng = K1_PDM_TARGET_SINCRNG_40K')
if applied not in (0, 2):
    raise SystemExit('sincrng apply must be both lanes or neither until Table 50.7 qualification')
header = (ROOT / 'platform/ra8p1/pdm_target.h').read_text()
if 'K1_PDM_TARGET_SINCRNG_40K 10u' not in header:
    raise SystemExit('SINCRNG 10 is the documented M=50 candidate, not an accepted flash')
if '#error "SINCRNG 5 is Table 50.7 M=125; M=50 cannot keep that window"' not in header:
    raise SystemExit('missing compile guard against leftover BSP SINCRNG 5')
with tempfile.TemporaryDirectory(prefix='k1-pdm-sincrng-') as temp:
    out = Path(temp) / 'test'
    # pdm_target.h includes k1/core/audio/k1_audio_hop.h → need -I src
    run(['cc', '-std=c11', '-O2',
         '-I' + str(ROOT / 'platform/ra8p1'),
         '-I' + str(ROOT / 'src'),
         str(ROOT / 'tests/host/test_pdm_sincrng.c'), '-o', str(out)])
    print(run([str(out)]), end='')
    broken = Path(temp) / 'broken.h'
    broken.write_text(header.replace(
        '#define K1_PDM_TARGET_SINCRNG_40K 10u',
        '#define K1_PDM_TARGET_SINCRNG_40K 5u'))
    failed = Path(temp) / 'fail.c'
    failed.write_text('#include "broken.h"\nint main(void) { return 0; }\n')
    try:
        run(['cc', '-std=c11', '-I' + str(temp), str(failed),
             '-o', str(Path(temp) / 'fail')])
    except Exception:
        print('SINCRNG_MUTATION_PASS leftover_5_rejected=true')
    else:
        raise SystemExit('SINCRNG 5 compile guard did not fire')
