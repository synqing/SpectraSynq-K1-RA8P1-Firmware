"""Execute the production renderer, retain red/green receipts and source identity."""
from pathlib import Path
import argparse, hashlib, json, subprocess

T=Path(__file__).resolve().parent
F=T.parents[1]
p=argparse.ArgumentParser()
p.add_argument('--output-dir',type=Path,required=True)
p.add_argument('--expect-red',action='store_true')
p.add_argument('--expect-amplitude-red',action='store_true')
p.add_argument('--reference-source',type=Path)
p.add_argument('--source',type=Path)
a=p.parse_args(); O=a.output_dir; O.mkdir(parents=True,exist_ok=False)
records=[]
def command(argv, failure=False, expected_expression=None):
    result=subprocess.run([str(x) for x in argv],capture_output=True,text=True,timeout=90)
    records.append(dict(argv=[str(x) for x in argv],returncode=result.returncode,
                        expected_assertion=failure,expected_expression=expected_expression,
                        stdout=result.stdout,stderr=result.stderr))
    (O/'commands.json').write_text(json.dumps(records,indent=2)+'\n')
    if failure:
        assert expected_expression is not None
        assert result.returncode!=0 and f'Assertion failed: ({expected_expression}),' in result.stderr,records[-1]
    else:
        assert result.returncode==0,records[-1]
    return result

source=a.source or F/'src/k1/core/visual/inhouse_waveform.cpp'
copy=O/'actual-renderer.cpp';copy.write_bytes(source.read_bytes())
inputs=[source,F/'src/k1/core/visual/inhouse_waveform.h',T/'june_renderer_response_cases.cpp',Path(__file__)]
(O/'inputs.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs},indent=2)+'\n')
flags=['-std=c++17','-O1','-g','-fno-exceptions','-fno-rtti','-fsanitize=address,undefined',
       '-fno-sanitize-recover=all','-ffp-contract=off','-DK1_EMOTISCOPE_PORT=1','-DK1_INHOUSE_MAPPING_V9=1',
       '-I'+str(F/'src'),'-I'+str(F/'src/k1')]
units=[F/'src/k1/core/visual'/x for x in ['product_palette.cpp','product_palette_data.cpp']]
command(['/usr/bin/clang++',*flags,T/'june_renderer_response_cases.cpp',copy,*units,'-o',O/'actual'])
if a.expect_amplitude_red:
    command([O/'actual','amplitude'],True,'std::fabs(main_high/main_low-1.95454545F)<.005F')
elif a.expect_red:
    command([O/'actual','peak'],True,'energy(high)>energy(low)*1.5')
    command([O/'actual','shape'],True,'changes>10')
else:
    command([O/'actual','all'])
    actual_tempo=command([O/'actual','tempo']).stdout
    if a.reference_source:
        reference=O/'reference-renderer.cpp';reference.write_bytes(a.reference_source.read_bytes())
        command(['/usr/bin/clang++',*flags,T/'june_renderer_response_cases.cpp',reference,*units,'-o',O/'reference'])
        assert command([O/'reference','tempo']).stdout==actual_tempo
    mutations=[
        ('peak_ignored','hybrid_seed = std::max(peak, std::max(observation_.peak_stage1,\n        fresh ? clamp01(audio.vu_average) : 0.0F));','hybrid_seed = 0.8F;','peak'),
        ('shape_ignored','shape = ::sqrtf(clamp01((static_cast<float>(sum) / 20.0F) / audio.raw_peak_pcm));','shape = 1.0F;','shape'),
        ('old_transport','float rate = 120.0F * motion;','float rate = 405.0F * motion;','transport'),
        ('old_max_hold','intensity_[index] = value;','intensity_[index] = std::max(intensity_[index], value);','replace'),
        ('double_seed','const float amplitude = (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','const float amplitude = hybrid_seed * (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','amplitude'),
        ('legacy_gain','const float amplitude = (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','const float amplitude = kInhouseV9Gain32 * (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','amplitude'),
        ('legacy_presence','const float amplitude = (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','const float amplitude = ::powf(std::min(level, 1.0F), 1.5F) * (main_brightness * blend + fallback_brightness * (1.0F - blend)) *','amplitude'),
        ('missing_helper_energy','main_brightness = std::max(main_brightness, palette_energy);','main_brightness = main_brightness;','amplitude'),
    ]
    intended_assertions={
        'peak_ignored':'energy(high)>energy(low)*1.5',
        'shape_ignored':'changes>10',
        'old_transport':'moves>=119 && moves<=121',
        'old_max_hold':'p[80].red==0 && p[80].green==0 && p[80].blue==0',
        'double_seed':'std::fabs(main_high/main_low-1.95454545F)<.005F',
        'legacy_gain':'main_low>.30F && main_low<.34F',
        'legacy_presence':'std::fabs(centre_amplitude(.2F,true,.1F)-main_low)<.001F',
        'missing_helper_energy':'main_low>.30F && main_low<.34F',
    }
    for name,old,new,case in mutations:
        text=source.read_text();assert text.count(old)==1,(name,'mutation anchor changed')
        mutant=O/(name+'.cpp');mutant.write_text(text.replace(old,new))
        command(['/usr/bin/clang++',*flags,T/'june_renderer_response_cases.cpp',mutant,*units,'-o',O/name])
        command([O/name,case],True,intended_assertions[name])
