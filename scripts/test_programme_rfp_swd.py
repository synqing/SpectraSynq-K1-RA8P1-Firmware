#!/usr/bin/env python3
"""Host negatives for programme_rfp_swd identity enforcement. Never opens hardware."""
from __future__ import annotations

import json
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import programme_rfp_swd as m  # noqa: E402

RESIDENT = {
    "build_id": "3a7ebd7c0d75f5d415fee6257e54bb6410d1c2d2c07a30fca2cc59ccdcc9ed2f",
    "hex": "2490e8cd4a391f8d827184872021a623a7be1f95a9a50a936efea18e83ccb8a6",
    "elf": "e3fea0e774f1412d27ba4a8abb54977d81f6952f3287faea5e724f360ff4ddd3",
    "source_pin": "6b1e7bc5c9f9871e6ea4e900455bcb37d756304a",
    "uid": "545433931bd25436593630352d068363",
}


def _seed_build(build: Path, *, build_id: str, hex_bytes: bytes, elf_bytes: bytes, source_pin: str) -> None:
    import hashlib

    hex_digest = hashlib.sha256(hex_bytes).hexdigest()
    elf_digest = hashlib.sha256(elf_bytes).hexdigest()
    (build / "rtthread.hex").write_bytes(hex_bytes)
    (build / "rtthread.elf").write_bytes(elf_bytes)
    receipt = {
        "pass": True,
        "build_id": build_id,
        "source_pin": source_pin,
        "artifacts": {"rtthread.hex": hex_digest, "rtthread.elf": elf_digest},
        "debug": False,
        "debug_info": False,
        "flags": "-O2",
    }
    (build / "receipt.json").write_text(json.dumps(receipt) + "\n")


def _run(build: Path, out: Path, *extra: str) -> tuple[int, dict]:
    if out.exists():
        shutil.rmtree(out)
    rc = m.main(["--build", str(build), "--output", str(out), *extra])
    receipt = json.loads((out / "receipt.json").read_text())
    return rc, receipt


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        build = root / "build"
        build.mkdir()
        # Embed build_id in ELF so the positive path can pass.
        elf = b"\x7fELF" + RESIDENT["build_id"].encode() + b"\0payload"
        hex_bytes = b":020000040200FA\n:00000001FF\n"
        _seed_build(build, build_id=RESIDENT["build_id"], hex_bytes=hex_bytes, elf_bytes=elf, source_pin=RESIDENT["source_pin"])

        # Positive dry-run
        rc, receipt = _run(build, root / "ok")
        assert rc == 0 and receipt["pass"] is True, receipt
        assert receipt["command"][0].endswith("rfp-cli")
        assert "-run" in receipt["command"]
        assert receipt["range"] == "02000000,02c9f01f"

        # Wrong HEX (mutate file after receipt sealed)
        (build / "rtthread.hex").write_bytes(hex_bytes + b"corrupt")
        rc, receipt = _run(build, root / "wrong-hex")
        assert rc == 2 and receipt["pass"] is False and "HEX hash mismatch" in receipt["error"]
        _seed_build(build, build_id=RESIDENT["build_id"], hex_bytes=hex_bytes, elf_bytes=elf, source_pin=RESIDENT["source_pin"])

        # Wrong ELF (no build_id inside)
        _seed_build(build, build_id=RESIDENT["build_id"], hex_bytes=hex_bytes, elf_bytes=b"\x7fELF-no-id", source_pin=RESIDENT["source_pin"])
        rc, receipt = _run(build, root / "wrong-elf-id")
        assert rc == 2 and "does not contain receipt build_id" in receipt["error"]
        _seed_build(build, build_id=RESIDENT["build_id"], hex_bytes=hex_bytes, elf_bytes=elf, source_pin=RESIDENT["source_pin"])

        # Wrong build_id pin
        rc, receipt = _run(build, root / "wrong-build", "--expected-build-id", "deadbeef" * 8)
        assert rc == 2 and "build_id mismatch" in receipt["error"]

        # Wrong UID
        rc, receipt = _run(build, root / "wrong-uid", "--expected-uid", "00000000000000000000000000000000")
        assert rc == 2 and "UID mismatch" in receipt["error"]

        # Wrong source
        rc, receipt = _run(build, root / "wrong-source", "--expected-source", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
        assert rc == 2 and "source mismatch" in receipt["error"]

        # Wrong region (into config)
        rc, receipt = _run(build, root / "wrong-range", "--range-end", "02C9F020")
        assert rc == 2 and ("config area" in receipt["error"] or "must be 02C9F01F" in receipt["error"] or "02c9f01f" in receipt["error"].lower())

        # Wrong speed
        rc, receipt = _run(build, root / "wrong-speed", "--speed", "100000")
        assert rc == 2 and "SWD speed" in receipt["error"]

        # Missing -run
        rc, receipt = _run(build, root / "no-run", "--no-run")
        assert rc == 2 and "-run is required" in receipt["error"]

        # Forbidden flag (use = form so argparse does not eat -erase-chip as a switch)
        rc, receipt = _run(build, root / "erase", "--extra-flag=-erase-chip")
        assert rc == 2 and "forbidden flag" in receipt["error"]

        # Full-register attach path must be refused
        rc, receipt = _run(build, root / "gdb", "--debug-mode", "getAllRegisters")
        assert rc == 2 and "full-register GDB attach is prohibited" in receipt["error"]
        rc, receipt = _run(build, root / "gdb2", "--debug-mode", "gdb")
        assert rc == 2 and "full-register GDB attach is prohibited" in receipt["error"]

        # --execute without env gate refused
        rc, receipt = _run(build, root / "exec-denied", "--execute")
        assert rc == 2 and "refused without" in receipt["error"]

        # Deliberately mismatched image vs resident pin
        other_build_id = "e5d51385318c6ee75bf0d40ab835ee2d94551e7ac744bdc0cb8de22309f4ea1f"
        other_elf = b"\x7fELF" + other_build_id.encode() + b"\0"
        _seed_build(build, build_id=other_build_id, hex_bytes=hex_bytes, elf_bytes=other_elf, source_pin=RESIDENT["source_pin"])
        rc, receipt = _run(
            build,
            root / "mismatch-resident",
            "--expected-build-id", RESIDENT["build_id"],
            "--expected-hex-sha256", RESIDENT["hex"],
            "--expected-elf-sha256", RESIDENT["elf"],
        )
        assert rc == 2 and receipt["pass"] is False
        assert "mismatch" in receipt["error"]

    print("K1_PROGRAMME_RFP_SWD_HOST=PASS (ok, wrong hex/elf/build/uid/source/range/speed/run/flag, getAllRegisters refuse, execute gate, mismatched image)")


if __name__ == "__main__":
    main()
