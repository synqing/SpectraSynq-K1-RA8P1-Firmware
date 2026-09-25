#!/usr/bin/env python3
"""Non-hanging debug workflow preparer for Titan E2 Lite.

Supported offline actions:
  prepare-identity-command  — rfp-cli -sig argv (H may run)
  prepare-write-command     — delegates identity gate to programme_rfp_swd dry-run
  reject-full-register      — must FAIL if asked for GDB/getAllRegisters

Does not open the board. Does not start e2-server-gdb. Does not attach GDB.
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import programme_rfp_swd as rfp  # noqa: E402

RFP_CLI = str(rfp.RFP_CLI)
PROBE = rfp.PROBE


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "action",
        choices=["prepare-identity-command", "prepare-write-command", "reject-full-register"],
    )
    parser.add_argument("--build", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--attach", default="", help="illegal full-register request surface")
    parser.add_argument("--expected-build-id", default=None)
    parser.add_argument("--expected-uid", default=rfp.EXPECTED_UID)
    parser.add_argument("--expected-source", default=None)
    args = parser.parse_args(argv)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    out: dict = {
        "task": "D4",
        "action": args.action,
        "start": datetime.now(timezone.utc).isoformat(),
        "pass": False,
    }
    try:
        # Required negative: any full-register path is refused by this workflow.
        if args.action == "reject-full-register" or args.attach:
            try:
                rfp.refuse_full_register_attach(args.attach or "getAllRegisters")
                out["error"] = "prohibited path was not refused"
                return 2
            except rfp.IdentityError as exc:
                out["pass"] = True
                out["rejected"] = str(exc)
                out["prohibited"] = True
                return 0

        if args.action == "prepare-identity-command":
            out["command"] = [
                RFP_CLI, "-d", "RA", "-t", PROBE, "-if", "swd",
                "-s", "1000000", "-noquery", "-run", "-sig",
            ]
            out["effects"] = "signature read restarts the application"
            out["pass"] = True
            out["note"] = "H may run this; D does not. Prefer USB INFO when session must stay up."
            return 0

        if args.action == "prepare-write-command":
            if not args.build:
                out["error"] = "--build required"
                return 2
            work = args.output.with_suffix(".rfp-dry")
            if work.exists():
                import shutil
                shutil.rmtree(work)
            extra = []
            if args.expected_build_id:
                extra += ["--expected-build-id", args.expected_build_id]
            if args.expected_uid:
                extra += ["--expected-uid", args.expected_uid]
            if args.expected_source:
                extra += ["--expected-source", args.expected_source]
            rc = rfp.main(["--build", str(args.build), "--output", str(work), "--debug-mode", "none", *extra])
            nested = json.loads((work / "receipt.json").read_text())
            out["rfp_dry_run"] = nested
            out["pass"] = rc == 0 and nested.get("pass") is True
            return 0 if out["pass"] else 2

        out["error"] = f"unknown action {args.action}"
        return 2
    finally:
        out["end"] = datetime.now(timezone.utc).isoformat()
        args.output.write_text(json.dumps(out, indent=2) + "\n")
        print(json.dumps(out, indent=2))


if __name__ == "__main__":
    sys.exit(main())
