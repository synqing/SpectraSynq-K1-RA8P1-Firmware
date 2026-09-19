#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
import cdc_lock  # noqa: E402


def test_second_lock_fails_and_crash_releases():
    device = "/dev/cu.usbmodem-ss03-test"
    first = cdc_lock.acquire(device, "a")
    try:
        try:
            cdc_lock.acquire(device, "b")
            raise AssertionError("second lock must fail")
        except RuntimeError:
            pass
    finally:
        cdc_lock.release(first)
    child = os.fork()
    if child == 0:
        handle = cdc_lock.acquire(device, "child")
        os._exit(0)
        _ = handle
    os.waitpid(child, 0)
    time.sleep(0.05)
    third = cdc_lock.acquire(device, "after-crash")
    cdc_lock.release(third)


def test_tiocexcl_scope_on_pty():
    import pty
    import fcntl

    master, slave = pty.openpty()
    notes = cdc_lock.apply_tiocexcl(slave)
    try:
        second = os.open(os.ttyname(slave), os.O_RDWR | os.O_NOCTTY)
        os.close(second)
        notes["second_open"] = "allowed"
    except OSError as exc:
        notes["second_open"] = f"blocked:{exc}"
    cdc_lock.drop_tiocexcl(slave)
    os.close(slave)
    os.close(master)
    print(__import__("json").dumps({"tiocexcl": notes}))
    return notes


if __name__ == "__main__":
    test_second_lock_fails_and_crash_releases()
    test_tiocexcl_scope_on_pty()
    print("ss03_lock_ok")
