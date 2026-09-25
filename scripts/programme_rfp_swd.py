#!/usr/bin/env python3
"""Identity-gated E2 Lite SWD writer for Titan RA8P1.

Builds and optionally runs the proven rfp-cli command. Default is dry-run:
validate identity bindings and print the exact argv. Never opens CDC.
Never starts GDB. Never calls getAllRegisters.

Hardware execution is for owner H when --execute is set. Lane D produces
this tool offline; swarm policy keeps WRITE_AUTONOMY disabled workers from
running --execute against the board.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

RFP_CLI = Path("/Users/spectrasynq/Applications/Renesas/RFP_CLI_V3.24.00/rfp-cli")
PROBE = "e2l:5AS079228B"
DEVICE_FAMILY = "RA"
INTERFACE = "swd"
SPEED_HZ = 1000000
RANGE_START = "02000000"
RANGE_END = "02C9F01F"
CONFIG_AREA_START = 0x02C9F020
CODE_MRAM_END = 0x020FFFFF
EXPECTED_UID = "545433931bd25436593630352d068363"
FORBIDDEN_FLAGS = ("-voltage", "-erase-chip")
PROHIBITED_DEBUG = ("getAllRegisters", "arm-none-eabi-gdb", "target remote", "e2-server-gdb")


class IdentityError(RuntimeError):
    """Wrong image, UID, source, region, or execution flag."""


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_receipt(build: Path) -> dict:
    receipt = json.loads((build / "receipt.json").read_text())
    if not receipt.get("pass"):
        raise IdentityError("build receipt pass is not true")
    return receipt


def require_hex_binding(build: Path, receipt: dict) -> str:
    hex_path = build / "rtthread.hex"
    if not hex_path.is_file():
        raise IdentityError("missing rtthread.hex")
    digest = sha256_file(hex_path)
    expected = (receipt.get("artifacts") or {}).get("rtthread.hex")
    if expected and digest != expected:
        raise IdentityError(f"HEX hash mismatch: file={digest} receipt={expected}")
    return digest


def require_elf_binding(build: Path, receipt: dict) -> str:
    elf_path = build / "rtthread.elf"
    if not elf_path.is_file():
        raise IdentityError("missing rtthread.elf")
    digest = sha256_file(elf_path)
    expected = (receipt.get("artifacts") or {}).get("rtthread.elf")
    if expected and digest != expected:
        raise IdentityError(f"ELF hash mismatch: file={digest} receipt={expected}")
    build_id = receipt.get("build_id")
    if not build_id:
        raise IdentityError("receipt missing build_id")
    # Executable identity must appear in the ELF before any symbol load.
    blob = elf_path.read_bytes()
    if build_id.encode() not in blob:
        raise IdentityError("ELF does not contain receipt build_id; refuse symbol/programme binding")
    return digest


def require_uid(expected_uid: str) -> str:
    uid = expected_uid.strip().lower()
    if uid != EXPECTED_UID:
        raise IdentityError(f"UID mismatch: {expected_uid}")
    return uid


def require_source(receipt: dict, expected_source: str | None) -> str | None:
    source = receipt.get("source_pin") or receipt.get("source_commit")
    if expected_source is None:
        return source
    if source != expected_source:
        raise IdentityError(f"source mismatch: receipt={source} expected={expected_source}")
    return source


def require_region(range_start: str, range_end: str) -> tuple[str, str]:
    start = int(range_start, 16)
    end = int(range_end, 16)
    if start != 0x02000000:
        raise IdentityError(f"write range start must be 02000000, got {range_start}")
    if end >= CONFIG_AREA_START:
        raise IdentityError(f"write range end {range_end} reaches config area {CONFIG_AREA_START:#x}")
    if end > CODE_MRAM_END and end != int(RANGE_END, 16):
        # Allow the sealed authority end 02C9F01F; reject anything past config.
        raise IdentityError(f"write range end outside sealed authority: {range_end}")
    if end != int(RANGE_END, 16):
        raise IdentityError(f"write range end must be {RANGE_END}, got {range_end}")
    return range_start.lower(), range_end.lower()


def require_execution_flags(speed_hz: int, run: bool, extra: list[str]) -> None:
    if speed_hz != SPEED_HZ:
        raise IdentityError(f"SWD speed must be {SPEED_HZ} Hz; {speed_hz} is refused")
    if not run:
        raise IdentityError("-run is required; omitting it can leave the application down")
    joined = " ".join(extra)
    for flag in FORBIDDEN_FLAGS:
        if flag in extra or flag in joined:
            raise IdentityError(f"forbidden flag {flag}")
    for token in PROHIBITED_DEBUG:
        if token in joined:
            raise IdentityError(f"prohibited debug path token refused: {token}")


def refuse_full_register_attach(mode: str) -> None:
    """Any workflow that would attach for getAllRegisters must fail closed."""
    lowered = mode.strip().lower().replace("_", "-")
    tokens = (
        "gdb",
        "full-register",
        "getallregisters",
        "target-remote",
        "e2-server-gdb",
        "arm-none-eabi-gdb",
    )
    if any(token in lowered for token in tokens):
        raise IdentityError(
            "full-register GDB attach is prohibited: E2ARMRegisters::getAllRegisters hangs"
        )


def build_argv(hex_path: Path, range_start: str, range_end: str) -> list[str]:
    return [
        str(RFP_CLI),
        "-d", DEVICE_FAMILY,
        "-t", PROBE,
        "-if", INTERFACE,
        "-s", str(SPEED_HZ),
        "-noquery",
        "-run",
        "-range", f"{range_start},{range_end}",
        "-pv", str(hex_path),
    ]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True, help="directory with receipt.json + rtthread.hex/elf")
    parser.add_argument("--output", type=Path, required=True, help="fresh receipt directory")
    parser.add_argument("--expected-uid", default=EXPECTED_UID)
    parser.add_argument("--expected-source", default=None, help="optional source_pin / commit pin")
    parser.add_argument("--expected-build-id", default=None, help="optional explicit build_id pin")
    parser.add_argument("--expected-hex-sha256", default=None)
    parser.add_argument("--expected-elf-sha256", default=None)
    parser.add_argument("--range-start", default=RANGE_START)
    parser.add_argument("--range-end", default=RANGE_END)
    parser.add_argument("--speed", type=int, default=SPEED_HZ)
    parser.add_argument("--no-run", action="store_true", help="illegal; retained so the negative can fire")
    parser.add_argument("--extra-flag", action="append", default=[], help="illegal extras for negative tests")
    parser.add_argument("--debug-mode", default="none", help="must not be a full-register attach path")
    parser.add_argument("--execute", action="store_true", help="H-only: run rfp-cli after identity PASS")
    parser.add_argument("--allow-execute-env", default="K1_RFP_SWD_EXECUTE_OK")
    args = parser.parse_args(argv)

    args.output.mkdir(parents=True, exist_ok=False)
    receipt_out: dict = {
        "label": "RFP-SWD",
        "start": datetime.now(timezone.utc).isoformat(),
        "pass": False,
        "execute": bool(args.execute),
        "dry_run": not args.execute,
        "programmer": "rfp-cli",
        "probe": PROBE,
        "interface": INTERFACE,
        "speed_hz": args.speed,
    }
    try:
        refuse_full_register_attach(args.debug_mode)
        build_receipt = load_receipt(args.build)
        build_id = build_receipt["build_id"]
        if args.expected_build_id and build_id != args.expected_build_id:
            raise IdentityError(f"build_id mismatch: receipt={build_id} expected={args.expected_build_id}")
        hex_digest = require_hex_binding(args.build, build_receipt)
        elf_digest = require_elf_binding(args.build, build_receipt)
        if args.expected_hex_sha256 and hex_digest != args.expected_hex_sha256:
            raise IdentityError(f"HEX hash mismatch vs pin: {hex_digest}")
        if args.expected_elf_sha256 and elf_digest != args.expected_elf_sha256:
            raise IdentityError(f"ELF hash mismatch vs pin: {elf_digest}")
        uid = require_uid(args.expected_uid)
        source = require_source(build_receipt, args.expected_source)
        range_start, range_end = require_region(args.range_start, args.range_end)
        require_execution_flags(args.speed, run=not args.no_run, extra=args.extra_flag)
        hex_path = (args.build / "rtthread.hex").resolve()
        argv_cmd = build_argv(hex_path, range_start, range_end)
        receipt_out.update(
            build_id=build_id,
            uid=uid,
            source_pin=source,
            hex_sha256=hex_digest,
            elf_sha256=elf_digest,
            range=f"{range_start},{range_end}",
            flags=["-noquery", "-run"],
            command=argv_cmd,
            debug_mode=args.debug_mode,
        )
        if not args.execute:
            receipt_out["pass"] = True
            receipt_out["note"] = "identity bindings PASS; command not executed"
            return 0
        if os.environ.get(args.allow_execute_env) != "1":
            raise IdentityError(
                f"--execute refused without {args.allow_execute_env}=1 (H owns board writes)"
            )
        if not RFP_CLI.is_file():
            raise IdentityError(f"rfp-cli missing: {RFP_CLI}")
        run = subprocess.run(argv_cmd, capture_output=True, text=True)
        receipt_out["returncode"] = run.returncode
        receipt_out["stdout"] = run.stdout
        receipt_out["stderr"] = run.stderr
        if run.returncode != 0:
            raise IdentityError(f"rfp-cli failed rc={run.returncode}")
        receipt_out["pass"] = True
        return 0
    except Exception as exc:  # noqa: BLE001 — receipt must record every refusal
        receipt_out["error"] = str(exc)
        receipt_out["pass"] = False
        return 2
    finally:
        receipt_out["end"] = datetime.now(timezone.utc).isoformat()
        (args.output / "receipt.json").write_text(json.dumps(receipt_out, indent=2) + "\n")
        print(json.dumps(receipt_out, indent=2))


if __name__ == "__main__":
    sys.exit(main())
