#!/usr/bin/env python3
from pathlib import Path
import csv, hashlib, sys
ROOT=Path(__file__).resolve().parents[1]
manifest=ROOT/'docs/IMPORT-MANIFEST.tsv'
fail=0
with manifest.open() as f:
    for row in csv.DictReader(f, delimiter='\t'):
        # Imported sources are expected under src/k1 with the leading core/ or contract/ path preserved.
        p=ROOT/'src/k1'/row['path']
        if not p.exists():
            continue
        digest=hashlib.sha256(p.read_bytes()).hexdigest()
        if digest != row['sha256']:
            print(f"IMPORT_DIVERGED path={p.relative_to(ROOT)} expected={row['sha256']} got={digest}")
            fail=1
        else:
            print(f"IMPORT_EXACT path={p.relative_to(ROOT)} sha256={digest}")
sys.exit(fail)
