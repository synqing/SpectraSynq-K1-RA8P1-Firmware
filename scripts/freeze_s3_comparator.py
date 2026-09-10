#!/usr/bin/env python3
"""Freeze immutable S3 source/restore identity without touching its dirty tree."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git(repo: Path, *args: str) -> bytes:
    return subprocess.check_output(['git', '-C', str(repo), *args])


def freeze(manifest_path: Path) -> dict:
    manifest = json.loads(manifest_path.read_text())
    if manifest.get('schema') != 1 or manifest.get('gate') != 'F01_S3_COMPARATOR_IDENTITY':
        raise RuntimeError('S3 authority schema/gate missing or divergent')
    repo = Path(manifest['source_repo'])
    product = manifest['product']
    commit = product['commit']
    resolved = git(repo, 'rev-parse', commit + '^{commit}').decode().strip()
    if resolved != commit:
        raise RuntimeError('S3 product commit missing or divergent')
    tree = git(repo, 'show', '-s', '--format=%T', commit).decode().strip()
    if tree != product['tree']:
        raise RuntimeError('S3 product tree missing or divergent')
    blobs = product.get('blobs')
    if not isinstance(blobs, dict) or not blobs:
        raise RuntimeError('S3 required source set is empty')
    checked = {}
    for name, expected in blobs.items():
        try:
            data = git(repo, 'show', f'{commit}:{name}')
        except subprocess.CalledProcessError as error:
            raise RuntimeError(f'S3 required source missing: {name}') from error
        observed = sha256(data)
        if observed != expected:
            raise RuntimeError(f'S3 required source divergent: {name}')
        checked[name] = observed

    restore = manifest['restore']
    restore_path = repo / restore['path']
    if not restore_path.is_file():
        raise RuntimeError('S3 restore dump missing')
    if restore_path.stat().st_size != restore['bytes']:
        raise RuntimeError('S3 restore dump size divergent')
    if sha256(restore_path.read_bytes()) != restore['sha256']:
        raise RuntimeError('S3 restore dump bytes divergent')

    factory = manifest['factory_app']
    record_path = repo / factory['hash_record_path']
    if not record_path.is_file() or record_path.read_text().strip() != factory['sha256']:
        raise RuntimeError('S3 factory-app hash record missing or divergent')
    if factory.get('binary_present') is not False:
        raise RuntimeError('S3 factory-app presence must remain explicit')

    authorities = manifest.get('current_authorities')
    if not isinstance(authorities, dict) or not authorities:
        raise RuntimeError('S3 current authority set is empty')
    authority_checked = {}
    for name, expected in authorities.items():
        path = repo / name
        if not path.is_file():
            raise RuntimeError(f'S3 current authority missing: {name}')
        observed = sha256(path.read_bytes())
        if observed != expected:
            raise RuntimeError(f'S3 current authority divergent: {name}')
        authority_checked[name] = observed

    boundary = manifest['campaign_boundary']
    if (boundary.get('current_live_image_is_product_comparator') is not False or
            boundary.get('production_repo_mutation_authorised_here') is not False or
            not str(boundary.get('same_workload_arm', '')).startswith('OPEN_') or
            not str(boundary.get('as_shipped_arm', '')).startswith('OPEN_')):
        raise RuntimeError('S3 campaign boundary would create a false closure')
    dirty = git(repo, 'status', '--porcelain', '--untracked-files=all').decode().splitlines()
    return {
        'label': 'HOST',
        'gate': manifest['gate'],
        'pass': True,
        'source': {
            'repo': str(repo), 'commit': commit, 'tree': tree,
            'environment': product['environment'],
            'device_chip_id': product['device_chip_id'],
            'usb_identity': product['usb_identity'],
            'required': len(blobs), 'checked': len(checked),
            'missing': 0, 'divergent': 0, 'blobs': checked,
        },
        'restore': restore,
        'factory_app': factory,
        'current_authorities': authority_checked,
        'working_tree_dirty_entries': len(dirty),
        'working_tree_used_as_source': False,
        'campaign_boundary': boundary,
        'decision': 'SOURCE_AND_RESTORE_FROZEN_CAMPAIGN_NOT_RUN',
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {'label': 'HOST', 'gate': 'F01_S3_COMPARATOR_IDENTITY',
               'start': datetime.now(timezone.utc).isoformat(), 'pass': False}
    try:
        receipt.update(freeze(args.manifest))
    except Exception as error:
        receipt['error'] = str(error)
        raise
    finally:
        receipt['end'] = datetime.now(timezone.utc).isoformat()
        (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'gate': receipt['gate'], 'decision': receipt['decision'],
                      'required': receipt['source']['required'],
                      'checked': receipt['source']['checked'], 'pass': True}, indent=2))


if __name__ == '__main__':
    main()
