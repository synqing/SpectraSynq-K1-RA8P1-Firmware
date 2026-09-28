from pathlib import Path
import json, hashlib, shutil, subprocess, datetime
root = Path('/Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware')
new = root / '.worktrees/titan-effects-language-001'
ap = root / '.worktrees/titan-music-intelligence-001'
source = Path('/Users/spectrasynq/Downloads/K1-Swarm-Handoff/source-inputs/k1-pipeline-integration')
out = new / 'docs/effects-language-001'
base = 'a96278c3cb1ab5346a0010412438cb7ecb29be03'
assert subprocess.check_output(['git', '-C', str(new), 'rev-parse', 'HEAD'], text=True).strip() == base
jobs = []
for name, rel in [
    ('Connor-Light-Engines-Teardown.md', 'history/Connor-Light-Engines-Teardown.md'),
    ('K1-Modular-Pipeline-Architecture.md', 'K1-Modular-Pipeline-Architecture.md'),
    ('visual-modularity.md', 'visual-modularity.md'),
    ('precision-audit.md', 'precision-audit.md'),
    ('architecture-decision.md', 'architecture-decision.md')]:
    jobs.append((source / rel, out / 'reference' / name, None))
for origin, destination in [
    (Path('/Users/spectrasynq/Downloads/Sensorybridge.sourcecode/SensoryBridge-4.1.1/SENSORY_BRIDGE_FIRMWARE'),
     out / 'reference/donor-local/sensory-4.1.1/SENSORY_BRIDGE_FIRMWARE'),
    (source / 'donor-1.1', out / 'reference/donor-local/emotiscope-1.1')]:
    for f in sorted(origin.rglob('*')):
        if f.is_file() and not any(p.startswith('.') for p in f.relative_to(origin).parts):
            jobs.append((f, destination / f.relative_to(origin), None))
jobs.append((Path('/Users/spectrasynq/Downloads/Sensorybridge.sourcecode/SensoryBridge-4.1.1/LICENSE'),
             out / 'reference/donor-local/sensory-4.1.1/LICENSE', None))
fixture_rel = Path('tests/fixtures/tempo_field/v1/pcm')
manifest = json.loads((new / fixture_rel / 'manifest.json').read_text())
for f in manifest['fixtures']:
    jobs.append((ap / fixture_rel / f['path'], new / fixture_rel / f['path'], f['sha256']))
records = []
for src, dst, expected in jobs:
    data = src.read_bytes()
    h = hashlib.sha256(data).hexdigest()
    if expected is not None and h != expected:
        raise RuntimeError('PCM hash mismatch: ' + str(src))
    if dst.exists() and hashlib.sha256(dst.read_bytes()).hexdigest() != h:
        raise RuntimeError('Destination differs: ' + str(dst))
    records.append({'source': str(src), 'path': str(dst.relative_to(new)),
                    'sha256': h, 'bytes': len(data), 'frozen_pcm_match': expected is not None})
for (src, dst, _), record in zip(jobs, records):
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not dst.exists():
        shutil.copy2(src, dst)
    if hashlib.sha256(dst.read_bytes()).hexdigest() != record['sha256']:
        raise RuntimeError('Copy mismatch')
report = (out / 'reference/Connor-Light-Engines-Teardown.md').read_text()
mark = '# K1 appearance transfer — implementation and evaluation handoff'
assert mark in report
(out / 'reference/appearance-transfer-original.md').write_text(report[report.index(mark):])
receipt = {
    'schema': 'k1.effects-lane-inputs.v1',
    'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'base_commit': base, 'branch': 'feat/titan-effects-language-001',
    'records': records, 'pcm_fixture_count': len(manifest['fixtures']),
    'donor_correspondence': 'Execution-day local hashes only; verify upstream correspondence before donor-parity claims.',
    'render_code_changed': False, 'device_mutated': False,
    'appearance_implementation': 'NOT STARTED',
    'source_platform_edits_excluded': ['platform/ra8p1/fixture_app.cpp', 'platform/ra8p1/stage_probe.h',
                                     'scripts/run_scalar_schedule.py', 'scripts/stage_profile.py']}
(out / 'lane-inputs.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps({'copied_files': len(records), 'copied_bytes': sum(r['bytes'] for r in records),
                  'verified_pcm_fixtures': len(manifest['fixtures']),
                  'full_appearance_assignment_retained': True, 'base_commit': base}))
