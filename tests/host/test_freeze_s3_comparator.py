import hashlib
import json
from pathlib import Path
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
from freeze_s3_comparator import freeze


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run(repo: Path, *args: str) -> str:
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def fixture(tmp_path: Path):
    repo = tmp_path / 's3'
    repo.mkdir()
    run(repo, 'init')
    run(repo, 'config', 'user.email', 'test@example.invalid')
    run(repo, 'config', 'user.name', 'Test')
    source = b'product-source\n'
    (repo / 'product.cpp').write_bytes(source)
    run(repo, 'add', 'product.cpp')
    run(repo, 'commit', '-m', 'product')
    commit = run(repo, 'rev-parse', 'HEAD')
    tree = run(repo, 'show', '-s', '--format=%T', commit)
    restore = b'full-flash'
    (repo / 'restore.bin').write_bytes(restore)
    factory_hash = digest(b'factory-app')
    (repo / 'factory.sha256').write_text(factory_hash + '\n')
    authority = b'current authority\n'
    (repo / 'authority.md').write_bytes(authority)
    manifest = {
        'schema': 1, 'gate': 'F01_S3_COMPARATOR_IDENTITY',
        'source_repo': str(repo),
        'product': {
            'commit': commit, 'tree': tree, 'environment': 'product',
            'device_chip_id': 'device', 'usb_identity': 'usb',
            'blobs': {'product.cpp': digest(source)},
        },
        'restore': {'path': 'restore.bin', 'bytes': len(restore),
                    'sha256': digest(restore), 'scope': 'test'},
        'factory_app': {'expected_bytes': 1, 'sha256': factory_hash,
                        'hash_record_path': 'factory.sha256', 'binary_present': False},
        'current_authorities': {'authority.md': digest(authority)},
        'campaign_boundary': {
            'current_live_image': 'probe',
            'current_live_image_is_product_comparator': False,
            'same_workload_arm': 'OPEN_TEST',
            'as_shipped_arm': 'OPEN_TEST',
            'production_repo_mutation_authorised_here': False,
        },
    }
    path = tmp_path / 'manifest.json'
    path.write_text(json.dumps(manifest))
    return repo, path, manifest


def test_frozen_git_object_ignores_dirty_worktree(tmp_path):
    repo, path, _ = fixture(tmp_path)
    (repo / 'product.cpp').write_text('dirty change\n')
    receipt = freeze(path)
    assert receipt['pass'] is True
    assert receipt['working_tree_used_as_source'] is False
    assert receipt['working_tree_dirty_entries'] >= 1


def test_empty_source_set_rejected(tmp_path):
    _, path, manifest = fixture(tmp_path)
    manifest['product']['blobs'] = {}
    path.write_text(json.dumps(manifest))
    with pytest.raises(RuntimeError, match='source set is empty'):
        freeze(path)


def test_missing_source_rejected(tmp_path):
    _, path, manifest = fixture(tmp_path)
    manifest['product']['blobs'] = {'missing.cpp': digest(b'missing')}
    path.write_text(json.dumps(manifest))
    with pytest.raises(RuntimeError, match='required source missing'):
        freeze(path)


def test_wrong_source_bytes_rejected(tmp_path):
    _, path, manifest = fixture(tmp_path)
    manifest['product']['blobs']['product.cpp'] = digest(b'wrong')
    path.write_text(json.dumps(manifest))
    with pytest.raises(RuntimeError, match='required source divergent'):
        freeze(path)


def test_changed_restore_rejected(tmp_path):
    repo, path, _ = fixture(tmp_path)
    (repo / 'restore.bin').write_bytes(b'changed')
    with pytest.raises(RuntimeError, match='restore dump size divergent'):
        freeze(path)


def test_false_campaign_closure_rejected(tmp_path):
    _, path, manifest = fixture(tmp_path)
    manifest['campaign_boundary']['as_shipped_arm'] = 'PASS'
    path.write_text(json.dumps(manifest))
    with pytest.raises(RuntimeError, match='false closure'):
        freeze(path)
