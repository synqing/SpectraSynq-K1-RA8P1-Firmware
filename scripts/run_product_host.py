#!/usr/bin/env python3
"""Independent pinned-donor/AP/VP replay; silent PCM, full field and pixel traces."""
from __future__ import annotations
import argparse
import hashlib
import itertools
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile
from datetime import datetime, timezone
from verify_imports import ROOT, REFERENCE, PIN, verify, commit_for
from run_host import run
from fixture_wire import read_schema, decode

BASE_FLAGS = ['-std=c++17', '-ffp-contract=off', '-fno-fast-math']
OPTIMISATIONS = {'o2':['-O2'], 'o3-unroll':['-O3','-funroll-loops','-frename-registers']}
# Compatibility name used by the independently compiled target-libm probe.
FLAGS = BASE_FLAGS + OPTIMISATIONS['o2']

def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()

def compare(reference, candidate):
    """Exact HOST comparison. Reject bad schema, truncation and non-finite values."""
    rows = fields = 0
    schema = None
    keys = []
    with Path(reference).open() as left, Path(candidate).open() as right:
        for line, (a, b) in enumerate(itertools.zip_longest(left, right), 1):
            if a != b:
                raise ValueError(f'first divergence line={line} hop={rows + 1}: reference={a!r} candidate={b!r}')
            if a == 'END\n':
                if len(keys) != len(set(keys)) or not all(f'pixel[{i}]' in keys for i in range(320)):
                    raise ValueError('incomplete or duplicate output schema')
                if schema is None: schema = keys
                if keys != schema: raise ValueError('schema changed')
                rows += 1
                keys = []
            else:
                key, value = a.rstrip().split('=', 1)
                if not math.isfinite(float(value)): raise ValueError(f'non-finite field {key}')
                keys.append(key)
                fields += 1
    if not rows or keys: raise ValueError('empty or incomplete trace')
    return dict(hops=rows, fields=fields, fields_per_hop=len(schema), schema_sha256=hashlib.sha256('\n'.join(schema).encode()).hexdigest())

def mutation_checks(reference, directory):
    # One complete real reference hop: independent mutation of feature, time and last pixel.
    lines = []
    with reference.open() as source:
        for line in source:
            lines.append(line)
            if line == 'END\n': break
    control = directory / 'mutation-control.trace'
    control.write_text(''.join(lines))
    results = []
    for field in ['features.spectrum[79]', 'features.source_frame_ms', 'predicted_next_beat.beat_result_available_frame', 'musical_time.anchor.epoch_id', 'pixel[319]']:
        altered = list(lines)
        index = next(i for i, text in enumerate(lines) if text.startswith(field + '='))
        altered[index] = field + '=999999\n'
        mutant = directory / ('mutation-' + str(len(results)) + '.trace')
        mutant.write_text(''.join(altered))
        try: compare(control, mutant)
        except ValueError as error: results.append(dict(field=field, rejected=True, diagnostic=str(error)))
        else: raise RuntimeError('comparator accepted mutation')
    for kind, content in [('truncated', ''.join(lines[:-2])), ('nan', ''.join(lines).replace('features.peak_scaled=0\n', 'features.peak_scaled=nan\n'))]:
        mutant = directory / ('mutation-' + kind + '.trace')
        mutant.write_text(content)
        # Identical poisoned inputs must also fail structural/non-finite validation.
        try: compare(mutant, mutant)
        except ValueError as error: results.append(dict(field=kind, rejected=True, diagnostic=str(error)))
        else: raise RuntimeError(f'comparator accepted {kind}')
    return results

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--song', type=Path, action='append', default=[])
    parser.add_argument('--reuse-corpus',type=Path,help='reuse hash-checked PCM; never regenerate goldens in place')
    parser.add_argument('--math-probe-build',type=Path,help='explicit ELF-verified newlib HOST platform profile')
    parser.add_argument('--optimisation',choices=OPTIMISATIONS,default='o2')
    args = parser.parse_args()
    flags=BASE_FLAGS+OPTIMISATIONS[args.optimisation]
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = dict(label='HOST', start=datetime.now(timezone.utc).isoformat(), source_commit=PIN,
                   candidate_commit=run(['git', 'rev-parse', 'HEAD'], cwd=ROOT).strip(), flags=flags,
                   contract='sr24000.hop180.bins80.xover40', ap_period_us=7500, render_rate_hz=120,
                   time_model='canonical 48k; zero-cost fixture publication; no physical latency claim',
                   float_acceptance='HOST exact textual roundtrip; integers and pixels exact; target budget NOT FROZEN',
                   compiler=run(['c++', '--version']).splitlines()[0], cases=[], **{'pass': False})
    try:
        receipt['imports'] = verify(ROOT, REFERENCE, 'product', True)
        if not receipt['imports']['pass']: raise RuntimeError('import gate failed')
        names = json.loads((ROOT / 'docs/import-slices.json').read_text())['product']
        receipt['adapter_sha256'] = sha(ROOT / 'tests/target/trajectory.h')
        receipt['runner_sha256'] = sha(Path(__file__))
        receipt['fixtures'] = []
        synthetic = args.output / 'controls.pcm'
        # 45 s: silence, impulses/transients, sustained tone, tempo switch, dropout/re-entry.
        if args.reuse_corpus is None:
          with synthetic.open('wb') as target:
            for i in range(24000 * 45):
                t = i / 24000
                x = 0
                if 2 <= t < 12: x = .3*math.sin(2*math.pi*440*t) + .1*math.sin(2*math.pi*880*t)
                if 12 <= t < 40:
                    bpm = 120 if t < 26 else 150
                    phase = (t - 12) % (60/bpm)
                    x = .6*math.exp(-phase*35)*math.sin(2*math.pi*75*t) + .1*math.sin(2*math.pi*660*t)
                if 33 < t < 36: x = 0
                if i in (24000, 120000, 1008000): x = .99
                target.write(struct.pack('<h', round(x*32767)))
        inputs = [synthetic]
        if args.reuse_corpus is not None:
            original=json.loads((args.reuse_corpus/'receipt.json').read_text())
            if not original['pass'] or args.song: raise RuntimeError('invalid corpus reuse')
            receipt['fixtures']=original['fixtures']
            inputs=[Path(f['path']) for f in receipt['fixtures']]
            for f in receipt['fixtures']:
                if sha(f['path'])!=f['sha256']: raise RuntimeError('reused PCM changed')
            receipt['original_corpus_receipt_sha256']=sha(args.reuse_corpus/'receipt.json')
        else:
            receipt['fixtures'].append(dict(path=str(synthetic), sha256=sha(synthetic), kind='synthetic controls', samples=24000*45))
        for index, song in enumerate(args.song):
            target = args.output / f'song-{index}.pcm'
            command = ['ffmpeg', '-nostdin', '-v', 'error', '-ss', '30', '-i', str(song), '-map', '0:a:0', '-t', '30', '-ac', '1', '-ar', '24000', '-f', 's16le', str(target)]
            run(command)
            receipt['fixtures'].append(dict(path=str(target), sha256=sha(target), source=str(song), source_sha256=sha(song), decode_command=command, kind='MUSDB mixture; research dataset, not commercial clearance'))
            inputs.append(target)
        receipt['ffmpeg'] = run(['ffmpeg', '-version']).splitlines()[0]
        extra=[]
        if args.math_probe_build is not None:
            if args.reuse_corpus is None: raise RuntimeError('explicit math profile requires the original frozen corpus')
            probe=args.output/'platform-math-proof'
            run(['python3',str(ROOT/'scripts/probe_target_libm.py'),'--build',str(args.math_probe_build),
                 '--corpus',str(args.reuse_corpus),'--output',str(probe)])
            receipt['platform_math']=dict(profile='arm_newlib_4_4_fma',proof=json.loads((probe/'receipt.json').read_text()),
                                         receipt_sha256=sha(probe/'receipt.json'))
            receipt['float_acceptance']='exact against pinned K1 plus explicit verified platform math; no numerical tolerance'
            extra=['-fno-builtin-logf','-fno-builtin-expf','-fno-builtin-log2f',str(ROOT/'tests/host/arm_logf_probe.cpp')]
        with tempfile.TemporaryDirectory(prefix='k1-product-host-') as scratch:
            donor = Path(scratch) / 'donor'
            for name in names:
                path = donor / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(subprocess.check_output(['git', '-C', str(REFERENCE), 'show', f'{commit_for(name)}:{name}']))
            binaries = {}
            for label, source in [('reference', donor), ('candidate', ROOT / 'src/k1')]:
                executable = args.output / label
                command = ['c++', *flags, *extra, '-I'+str(source), '-I'+str(ROOT / 'tests/target'),
                           str(ROOT / 'tests/host/trajectory_main.cpp'), *[str(source / p) for p in names if p.endswith('.cpp')], '-o', str(executable)]
                run(command)
                binaries[label] = executable
            schema=args.output/'trace-schema.txt'
            with inputs[0].open('rb') as source, schema.open('wb') as target:
                subprocess.run([str(binaries['reference']),'--schema'],stdin=source,stdout=target,check=True)
            schema_rows=read_schema(schema.read_text())
            receipt['schema_sha256']=sha(schema)
            for pcm in inputs:
                traces = {}
                for label, executable in binaries.items():
                    trace = args.output / f'{pcm.stem}-{label}.trace'
                    with pcm.open('rb') as source, trace.open('wb') as target:
                        result = subprocess.run([str(executable)], stdin=source, stdout=target, stderr=subprocess.PIPE, timeout=240)
                    if result.returncode: raise RuntimeError(f'{label} failed {pcm} rc={result.returncode} stderr={result.stderr!r}')
                    traces[label] = trace
                case = compare(traces['reference'], traces['candidate'])
                if case['hops'] != pcm.stat().st_size // 360: raise RuntimeError('missing hops')
                case.update(input=str(pcm), traces={k: dict(path=str(v), sha256=sha(v)) for k,v in traces.items()}, **{'pass': True})
                receipt['cases'].append(case)
                binary=args.output/(pcm.stem+'-binary.values')
                with pcm.open('rb') as source,binary.open('wb') as output:
                    subprocess.run([str(binaries['candidate']),'--binary'],stdin=source,stdout=output,check=True,timeout=240)
                recovered=args.output/(pcm.stem+'-binary-roundtrip.trace')
                with binary.open('rb') as source,recovered.open('wb') as output:
                    while header:=source.read(4):
                        if len(header)!=4: raise RuntimeError('truncated HOST binary header')
                        size=struct.unpack('<I',header)[0]
                        if size>16384: raise RuntimeError('oversized HOST binary trace')
                        output.write(decode(source.read(size),schema_rows)+b'END\n')
                case['binary_roundtrip']=compare(traces['candidate'],recovered)
                print(f'K1_PRODUCT_HOST {pcm.stem} hops={case["hops"]} fields={case["fields"]} PASS', flush=True)
            receipt['mutations'] = mutation_checks(args.output / 'controls-reference.trace', args.output)
        receipt['pass'] = True
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        receipt['end'] = datetime.now(timezone.utc).isoformat()
        (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')

if __name__ == '__main__': main()
