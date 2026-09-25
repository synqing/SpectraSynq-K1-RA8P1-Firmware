#!/usr/bin/env python3
"""Inspect/recover this Mac's verified Renesas E2 Lite host lock. No target access.
Default: inspect only. --clear-stale: remove the exact orphaned semaphore after
checking for active clients twice. Never use this to override a live debugger.
This workaround is pinned to the library inspected on 2026-09-22, not a vendor fix.
"""
from __future__ import annotations
import argparse
import ctypes as C
import datetime as D
import errno
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time

RUNTIME = Path('/Users/spectrasynq/Applications/Renesas/E2_Debug_Runtime_10.6.0')
LIBRARY = RUNTIME / 'ARM/E2_v2.8.1/libCommuni.dylib'
DIGEST = '23fc04e562f716b83366d09c8c885758648c66654082523544daec924e9d963f'
SERIAL = 'E2L: 5AS079228B'
NAME = ('/CommuniDLL_Sem' + SERIAL).encode('ascii')
FAILED = C.c_void_p(-1).value


def consumers() -> dict:
    result = subprocess.run(['/bin/ps', '-axo', 'pid=,comm='],
                            check=True, capture_output=True, text=True, timeout=10)
    matches = []
    for line in result.stdout.splitlines():
        fields = line.strip().split(None, 1)
        if len(fields) != 2 or int(fields[0]) == os.getpid():
            continue
        base = Path(fields[1]).name.lower()
        if any(x in base for x in ('e2-server', 'e2studio', 'rfp-cli',
                                   'devicepartition', 'renesas')):
            matches.append(line.strip())
    opened = subprocess.run(['/usr/sbin/lsof', '-nP', str(LIBRARY)],
                            capture_output=True, text=True, timeout=15)
    if opened.returncode not in (0, 1) or opened.stderr.strip():
        raise RuntimeError('Cannot reliably inspect library consumers; no cleanup: '
                           + opened.stderr.strip())
    return {'processes': matches, 'library_consumers': opened.stdout.strip()}


def refuse_active(snapshot: dict) -> None:
    if snapshot['processes'] or snapshot['library_consumers']:
        raise RuntimeError('Possible active debugger. Close it normally first; '
                           'refusing to remove its lock. ' + json.dumps(snapshot))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clear-stale', action='store_true',
                        help='Remove only this probe\'s stale host lock, never an active lock')
    parser.add_argument('--receipt', type=Path,
                        help='Write JSON to a NEW file (existing files are never overwritten)')
    args = parser.parse_args()
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        raise RuntimeError('This helper is verified only for the inspected Apple Silicon Mac.')
    if hashlib.sha256(LIBRARY.read_bytes()).hexdigest() != DIGEST:
        raise RuntimeError('Renesas runtime changed. Revalidate its lock mechanism before clearing.')
    libc = C.CDLL('/usr/lib/libSystem.B.dylib', use_errno=True)
    libc.sem_open.argtypes = [C.c_char_p, C.c_int]
    libc.sem_open.restype = C.c_void_p
    libc.sem_close.argtypes = [C.c_void_p]
    libc.sem_close.restype = C.c_int
    libc.sem_unlink.argtypes = [C.c_char_p]
    libc.sem_unlink.restype = C.c_int

    def exists() -> bool:
        C.set_errno(0)
        handle = libc.sem_open(NAME, 0)  # No O_CREAT: cannot create a lock.
        error = C.get_errno()
        if handle == FAILED:
            if error == errno.ENOENT:
                return False
            raise OSError(error, os.strerror(error))
        if libc.sem_close(handle) != 0:
            raise OSError(C.get_errno(), 'sem_close failed')
        return True

    report = {'timestamp': D.datetime.now(D.timezone.utc).isoformat(),
              'serial': SERIAL, 'semaphore': NAME.decode(),
              'library_sha256': DIGEST, 'clear_requested': args.clear_stale,
              'before_exists': exists(), 'consumers_before': consumers(),
              'removed': False, 'target_operations': [], 'usb_operations': []}
    if args.clear_stale and report['before_exists']:
        refuse_active(report['consumers_before'])
        time.sleep(0.3)
        report['consumers_recheck'] = consumers()
        refuse_active(report['consumers_recheck'])
        C.set_errno(0)
        rc = libc.sem_unlink(NAME)
        error = C.get_errno()
        if rc != 0 and error != errno.ENOENT:
            raise OSError(error, 'sem_unlink failed')
        report['removed'] = rc == 0
    report['after_exists'] = exists()
    report['status'] = 'LOCK_PRESENT' if report['after_exists'] else 'HOST_LOCK_CLEAR'
    rendered = json.dumps(report, indent=2) + '\n'
    if args.receipt:
        with args.receipt.open('x') as stream:
            stream.write(rendered)
    print(rendered, end='')
    return 2 if report['after_exists'] else 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print('STOP: ' + str(error), file=sys.stderr)
        raise SystemExit(1)
