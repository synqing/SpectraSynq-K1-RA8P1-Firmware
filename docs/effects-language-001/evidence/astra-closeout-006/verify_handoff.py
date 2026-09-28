#!/usr/bin/env python3
"""THE TITAN MUST ALWAYS BE VISUALISING MUSIC. Read-only artifact integrity check.

This checks the local handoff, not any VP/CTL/MOD/IVV or optical contract.
Run from any cwd. It reads all retained local artifact bytes (about 12 GB).
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    manifest = json.loads((HERE / 'local-artifacts.json').read_text())
    for row in manifest['files']:
        path = ROOT / row['path']
        assert path.stat().st_size == row['bytes'], row['path']
        assert sha(path) == row['sha256'], row['path']
    receipt = json.loads((HERE / 'build-receipt-copy.json').read_text())
    for name, digest in receipt['sources'].items():
        assert sha(ROOT / name) == digest, name
    executable = ROOT / 'out/paired-capacity-verification-006-final/build/ribbon_replay'
    assert sha(executable) == receipt['executable_sha256']
    original = json.loads((ROOT / 'docs/effects-language-001/lane-inputs.json').read_text())
    for row in original['records']:
        assert sha(ROOT / row['path']) == row['sha256'], row['path']
    excluded = json.loads((HERE / 'exclusions.json').read_text())['registry']
    assert sha(ROOT / excluded['path']) == excluded['sha256'], 'Registry changed after snapshot'
    doc = ROOT / 'docs/effects-language-001/ASTRA-CLOSEOUT-006.md'
    for link in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
        if not link.startswith(('http:', 'https:')):
            assert (doc.parent / link).exists(), link
    checkpoint = '20f025cc529a647d384906e4f6155f0e1499668a'
    subprocess.run(['git', '-C', str(ROOT), 'merge-base', '--is-ancestor', checkpoint, 'HEAD'], check=True)
    print(json.dumps(dict(
        schema='astra-handoff-integrity-observation.v1',
        source_checkpoint=checkpoint,
        local_files_verified=len(manifest['files']),
        local_bytes_verified=manifest['total_bytes'],
        build_inputs_verified=len(receipt['sources']),
        original_inputs_verified=len(original['records']),
        executable_sha256=receipt['executable_sha256'],
        registry_snapshot_unchanged=True,
        local_links_exist=True,
        scope='Artifact integrity only; no parent admission, artistic verdict or target observation',
    ), indent=2))


if __name__ == '__main__':
    main()
