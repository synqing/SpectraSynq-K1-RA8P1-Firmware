"""Compare real production units with independent source rules and planted faults."""
from pathlib import Path
import argparse,json,subprocess
T=Path(__file__).resolve().parent; F=T.parents[1]
p=argparse.ArgumentParser();p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args();O=a.output_dir;O.mkdir(parents=True,exist_ok=False)
records=[]
def run(argv,negative=False):
    r=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=90)
    records.append({'argv':[str(x) for x in argv],'returncode':r.returncode,'negative_control':negative,'stdout':r.stdout,'stderr':r.stderr})
    (O/'commands.json').write_text(json.dumps(records,indent=2))
    assert r.returncode!=0 and 'Assertion failed' in r.stderr if negative else r.returncode==0, records[-1]
san=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-ffp-contract=off']
inc=['-I'+str(F/'platform/ra8p1'),'-I'+str(F/'src'),'-I'+str(F/'src/k1')]
run(['/usr/bin/clang','-std=c11',*san,*inc,'-c',F/'platform/ra8p1/k1_poly24.c','-o',O/'poly.o'])
audio=(F/'platform/ra8p1/inhouse_audio.cpp').read_text(); visual=(F/'src/k1/core/visual/inhouse_waveform.cpp').read_text()
for name,ap,vp,negative in [
    ('actual',audio,visual,False),
    ('guarded_peak_fault',audio.replace('if (paired_level_.version == 2U && paired_level_.guarded_support != 0U) {','if (false) {'),visual,True),
    ('outer_seed_fault',audio,visual.replace('    position = 80.0F;','    position = 159.0F;'),True),
]:
    (O/(name+'-audio.cpp')).write_text(ap);(O/(name+'-visual.cpp')).write_text(vp)
    units=[F/'src/k1/core/audio'/x for x in ('gdft_goertzel.cpp','tempo_tracker.cpp','tempo_acf.cpp')]
    units += [F/'src/k1/core/visual'/x for x in ('product_palette.cpp','product_palette_data.cpp')]
    run(['/usr/bin/clang++','-std=c++17',*san,*inc,'-DK1_EMOTISCOPE_PORT=1','-DK1_INHOUSE_MAPPING_V9=1',T/'june_music_response_cases.cpp',O/(name+'-audio.cpp'),O/(name+'-visual.cpp'),O/'poly.o',*units,'-o',O/name])
    run([O/name],negative)
print('Production units passed; guarded-peak and outer-seed faults both failed the independent assertions.')
