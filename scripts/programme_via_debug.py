#!/usr/bin/env python3
"""Flash Titan Mini over USB-DBG (on-board DAP/J-Link). No USER/BOOT/RESET.

ROM USB boot (MD pin low at reset) is recovery only. Vendor download path is
USB-DBG. Requires the debug Type-C plugged; USB-DEV CDC is the app port.
"""
from __future__ import annotations
import argparse, json, shutil, subprocess, sys
from datetime import datetime, timezone
from pathlib import Path

def main() -> None:
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('--build', type=Path, required=True)
    a.add_argument('--output', type=Path, required=True)
    a.add_argument('--execute', action='store_true')
    args = a.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = json.loads((args.build / 'receipt.json').read_text())
    elf = args.build / 'rtthread.elf'
    hex_path = args.build / 'rtthread.hex'
    pyocd = shutil.which('pyocd') or str(Path.home() / '.local/bin/pyocd')
    cmd = [pyocd, 'load', str(hex_path if hex_path.exists() else elf),
           '-t', 'r7ka8p1kflcac', '-f', '10000000']
    out = {
        'start': datetime.now(timezone.utc).isoformat(),
        'pass': False,
        'path': 'USB-DBG',
        'build_id': receipt.get('build_id'),
        'command': cmd,
        'execute': bool(args.execute),
    }
    if not args.execute:
        out['dry_run'] = True
        out['pass'] = True
        (args.output / 'receipt.json').write_text(json.dumps(out, indent=2) + '\n')
        print(json.dumps(out, indent=2))
        return
    listed = subprocess.run([pyocd, 'list'], capture_output=True, text=True)
    out['probes'] = listed.stdout
    if listed.returncode != 0 or 'no connected probes' in listed.stdout.lower() or not listed.stdout.strip():
        out['error'] = 'USB-DBG probe not seen. Plug Titan USB-DBG, not USB-DEV.'
        (args.output / 'receipt.json').write_text(json.dumps(out, indent=2) + '\n')
        raise SystemExit(out['error'])
    run = subprocess.run(cmd, capture_output=True, text=True)
    out['stdout'] = run.stdout
    out['stderr'] = run.stderr
    out['returncode'] = run.returncode
    out['pass'] = run.returncode == 0
    out['end'] = datetime.now(timezone.utc).isoformat()
    (args.output / 'receipt.json').write_text(json.dumps(out, indent=2) + '\n')
    if run.returncode != 0:
        raise SystemExit(run.stderr or run.stdout or 'pyocd load failed')
    print(json.dumps({'pass': True, 'build_id': out['build_id']}, indent=2))

if __name__ == '__main__':
    main()
