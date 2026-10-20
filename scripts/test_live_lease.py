#!/usr/bin/env python3
"""Host negatives for the campaign lease."""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / "tools/serial-studio"))
import live_lease  # noqa: E402


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="k1-lease-") as temp:
        run = Path(temp)
        first = live_lease.acquire(run, "a")
        assert Path(first["socket"]).is_socket()
        try:
            live_lease.acquire(run, "b")
            raise SystemExit("second holder must fail")
        except live_lease.LeaseError:
            pass
        live_lease.release(run, first["token"])
        second = live_lease.acquire(run, "b")
        live_lease.release(run, second["token"])
        assert not (run / "campaign.lease.json").exists()
        assert not (run / "campaign.sock").exists()
    print("LIVE_LEASE_HOST_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
