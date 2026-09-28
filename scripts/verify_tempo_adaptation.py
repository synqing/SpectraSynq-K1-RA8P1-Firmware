#!/usr/bin/env python3
"""Explicit hash-pinned TempoField adaptation; the default import gate stays strict."""
from __future__ import annotations
import argparse, csv, hashlib, json
from pathlib import Path
from verify_imports import ROOT, REFERENCE, PIN, safe_path, verify
MANIFEST = 'docs/tempo-field-adaptation-v1.json'
ADAPTED = frozenset({
    'core/audio/tempo_tracker.h', 'core/audio/tempo_tracker.cpp',
    'core/audio/audio_pipeline.h', 'core/audio/audio_pipeline.cpp',
    'core/visual/visual_audio_frame.h',
})
LOCAL_FILES = (
    'contract/tempo_field_v1.h',
    'core/audio/tempo_field_publisher.h', 'core/audio/tempo_field_publisher.cpp',
    'core/audio/tempo_phase_tracks.h', 'core/audio/tempo_phase_tracks.cpp',
)
AUXILIARY = frozenset({
    'src/k1/core/audio/tempo_acf_slice.h', 'src/k1/core/audio/tempo_acf_slice.cpp',
    'scripts/tempo_acf_slice_overlay.py',
})
def digest(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def verify_tempo_adaptation(root=ROOT, reference=REFERENCE):
    root=Path(root); reference=Path(reference)
    manifest=json.loads(safe_path(root,MANIFEST).read_text())
    if manifest.get('schema')!='k1.tempo-field-adaptation.v1' or manifest.get('source_commit')!=PIN:
        raise ValueError('Invalid tempo adaptation identity')
    if set(manifest.get('adapted',{}))!=ADAPTED or set(manifest.get('local',{}))!=set(LOCAL_FILES):
        raise ValueError('Tempo adaptation scope differs from the fixed allowlist')
    if set(manifest.get('auxiliary',{}))!=AUXILIARY:
        raise ValueError('Tempo auxiliary scope differs from the fixed allowlist')
    for rel in ('docs/IMPORT-MANIFEST.tsv','docs/import-slices.json'):
        if digest(safe_path(root,rel))!=manifest['authority_sha256'].get(rel):
            raise ValueError('Import authority drift: '+rel)
    rows={row['path']:row for row in csv.DictReader((root/'docs/IMPORT-MANIFEST.tsv').open(),delimiter='\t')}
    for rel,item in manifest['adapted'].items():
        if item.get('base_sha256')!=rows[rel]['sha256']:
            raise ValueError('Pinned adaptation base mismatch: '+rel)
        if digest(safe_path(root/'src/k1',rel))!=item['candidate_sha256']:
            raise ValueError('Adapted source hash mismatch: '+rel)
    for rel,expected in manifest['local'].items():
        if rel in rows: raise ValueError('Local file overlaps pinned import: '+rel)
        if digest(safe_path(root/'src/k1',rel))!=expected:
            raise ValueError('Local source hash mismatch: '+rel)
    for rel,expected in manifest['auxiliary'].items():
        if digest(safe_path(root,rel))!=expected:
            raise ValueError('Auxiliary source hash mismatch: '+rel)
    strict=verify(root,reference,'product',True)
    allowed={'destination differs from pinned source: '+name for name in ADAPTED}
    unexpected=[error for error in strict['errors'] if error not in allowed]
    observed={error for error in strict['errors'] if error in allowed}
    if unexpected or observed!=allowed or strict['checked']!=strict['required']:
        raise ValueError('Strict import result outside exact adaptation: '+json.dumps(strict))
    return {'pass':True,'mode':'EXPLICIT_TEMPO_FIELD_ADAPTATION',
            'manifest_sha256':digest(root/MANIFEST),'source_commit':PIN,
            'strict_default_pass':strict['pass'],'pinned_source_checked':strict['checked'],
            'unchanged_imports':strict['checked']-len(ADAPTED),
            'adapted':manifest['adapted'],'local':manifest['local'],
            'auxiliary':manifest['auxiliary'],'strict_errors_admitted':sorted(observed)}
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--reference',type=Path,default=REFERENCE)
    args=parser.parse_args()
    try: result=verify_tempo_adaptation(args.root,args.reference)
    except (ValueError,KeyError,OSError,TypeError) as error:
        print(json.dumps({'pass':False,'error':str(error)},indent=2));return 2
    print(json.dumps(result,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
