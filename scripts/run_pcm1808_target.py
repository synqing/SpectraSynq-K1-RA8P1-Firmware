#!/usr/bin/env python3
"""Reserved identity-gated PCM1808 proof; blocked until adapter hardware exists."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib
from datetime import datetime, timezone

from run_scalar_target import UID, packet, read_exact
from verify_imports import PIN


def validate_snapshot(metrics: dict, require_signal: bool = True) -> list[str]:
    errors: list[str] = []
    pcm = metrics.get("pcm1808_target")
    if not isinstance(pcm, dict):
        return ["pcm1808_target metrics missing"]
    expected = {
        "contract": "k1-ra8p1-pcm1808-aux-v1",
        "peripheral": "SSIE1",
        "role": "slave_receiver",
        "transfer": "DTC",
        "bclk": "P702/U11-24",
        "lrck": "P701/U11-33",
        "data": "P700/U11-26",
        "nominal_input_hz": 48000,
        "canonical_hz": 12800,
        "hop_in": 360,
        "hop_out": 96,
        "mono": "MID",
        "trim_q15": 2048,
        "channel_map": "UNVALIDATED_ON_TITAN",
        "clock_gate": "DMA_RATE_ONLY",
    }
    for name, value in expected.items():
        if pcm.get(name) != value:
            errors.append(f"{name} changed or is missing")
    if not pcm.get("initialised") or not pcm.get("running"):
        errors.append("PCM1808 SSIE1 target is not initialised and running")
    if pcm.get("last_fsp_error") != 0:
        errors.append("FSP reported an error")
    if pcm.get("overflow_events") != 0 or pcm.get("idle_events") != 0:
        errors.append("SSIE1 stopped or overflowed")
    hops = pcm.get("processed_hops", 0)
    samples = pcm.get("processed_samples", 0)
    if samples != hops * 96:
        errors.append("canonical sample count does not match complete hops")
    measured = pcm.get("measured_input_hz", 0)
    if hops and not 47000 <= measured <= 49000:
        errors.append("measured SSIE input rate is outside the 48 kHz gate")
    if require_signal and (hops < 1 or pcm.get("sample_peak", 0) <= 0 or
                           pcm.get("sample_square_sum", 0) <= 0):
        errors.append("no non-zero canonical line signal was captured")
    return errors


def validate_progress(before: dict, after: dict) -> list[str]:
    errors = validate_snapshot(before, require_signal=False) + validate_snapshot(after)
    first = before.get("pcm1808_target", {})
    second = after.get("pcm1808_target", {})
    if second.get("callbacks", 0) <= first.get("callbacks", 0):
        errors.append("SSIE callbacks did not advance")
    if second.get("processed_hops", 0) <= first.get("processed_hops", 0):
        errors.append("canonical hops did not advance")
    if second.get("sample_hash") == first.get("sample_hash"):
        errors.append("canonical sample hash did not change")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--observe-seconds", type=float, default=3.0)
    args = parser.parse_args()
    contract_path = Path(__file__).resolve().parents[1] / "docs/pcm1808-source-contract.json"
    contract = json.loads(contract_path.read_text())
    if contract.get("status") == "BLOCKED_NO_PRACTICAL_CONNECTOR_ROUTE":
        raise SystemExit(
            "PCM1808 silicon run blocked: Titan U18 has no complete framed input; "
            "fit a proper U11 mating breakout or a digital-audio bridge first"
        )
    if not 0.25 <= args.observe_seconds <= 10.0:
        raise SystemExit("--observe-seconds must be between 0.25 and 10 seconds")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "label": "ON-SILICON",
        "qualification": "PCM1808_SSIE1_AUX_CAPTURE",
        "started_at": datetime.now(timezone.utc).isoformat(),
        "pass": False,
    }
    port = None
    try:
        import serial
        from serial.tools import list_ports

        build = json.loads((args.build / "receipt.json").read_text())
        if not build.get("pass") or not build.get("pcm1808_target"):
            raise RuntimeError("build is not an accepted PCM1808 target image")
        image_sha = hashlib.sha256((args.build / "rtthread.hex").read_bytes()).hexdigest()
        if image_sha != build["artifacts"]["rtthread.hex"]:
            raise RuntimeError("target image changed after build")
        matches = [p for p in list_ports.comports() if (p.vid, p.pid) == (0x045B, 0x5310)]
        if len(matches) != 1:
            raise RuntimeError(f"expected exactly one Titan application USB, found {len(matches)}")
        device = matches[0].device
        owners = subprocess.run(["lsof", "-t", device], capture_output=True, text=True)
        if owners.returncode not in (0, 1) or owners.stdout.strip():
            raise RuntimeError("Titan USB has another owner")
        port = serial.Serial(device, 115200, timeout=0.2, write_timeout=2, exclusive=True)
        request = 0

        def transact(op: int) -> bytes:
            nonlocal request
            request += 1
            frame = packet(op, request)
            if port.write(frame) != len(frame):
                raise RuntimeError("short USB write")
            port.flush()
            header = read_exact(port, 32)
            magic, status, reply_id, _, size, _, crc, header_crc = struct.unpack("<4s7I", header)
            if magic != b"K1R1" or zlib.crc32(header[:28]) != header_crc or reply_id != request:
                raise RuntimeError("invalid response identity/header")
            body = read_exact(port, size)
            if status != 0 or zlib.crc32(body) != crc:
                raise RuntimeError(f"target rejected request with status {status}")
            return body

        info = json.loads(transact(1))
        if info.get("uid") != UID or info.get("build") != build.get("build_id") or info.get("source") != PIN:
            raise RuntimeError("runtime UID/build/source identity mismatch")
        before = json.loads(transact(15))
        time.sleep(args.observe_seconds)
        after = json.loads(transact(15))
        errors = validate_progress(before, after)
        receipt.update(
            build_id=build["build_id"],
            image_sha256=image_sha,
            runtime=info,
            observation_seconds=args.observe_seconds,
            before=before,
            after=after,
            errors=errors,
            limitations=[
                "DMA-rate evidence is not an oscilloscope BCLK/LRCK measurement.",
                "Physical LEFT/RIGHT mapping remains open until channel-isolated Titan fixtures run.",
                "This short signal proof is not AP/VP deadline or microphone coexistence proof.",
                "No song loop or generic soak is used.",
            ],
        )
        if errors:
            raise RuntimeError("; ".join(errors))
        receipt["status"] = "PASS_PCM1808_SSIE1_AUX_CAPTURE"
        receipt["pass"] = True
        return 0
    except Exception as error:
        receipt["status"] = "FAIL"
        receipt["error"] = str(error)
        raise
    finally:
        if port is not None:
            port.close()
        receipt["completed_at"] = datetime.now(timezone.utc).isoformat()
        (args.output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
