#!/usr/bin/env python3
"""Three-second, identity-gated Titan onboard LMD2718 capture proof."""
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


def validate_snapshot(metrics: dict) -> list[str]:
    errors: list[str] = []
    pdm = metrics.get("pdm_target")
    if not isinstance(pdm, dict):
        return ["pdm_target metrics missing"]
    if not pdm.get("initialised") or not pdm.get("running"):
        errors.append("dual PDM target is not initialised and running")
    if pdm.get("sample_rate_hz") != 16000 or pdm.get("working_source_sample_rate_hz") != 12800:
        errors.append("declared target/source sample-rate boundary changed")
    if pdm.get("sample_rate_match") is not False:
        errors.append("16 kHz target must not claim 12.8 kHz source parity")
    if pdm.get("slot_elements") != 120 or pdm.get("slot_duration_us") != 7500:
        errors.append("7.5 ms target capture geometry changed")
    if pdm.get("shared_clock_and_data") is not True or pdm.get("programme_lane") != 0:
        errors.append("working shared-wire/programme-lane contract changed")
    if pdm.get("mpn") != "LMD2718T261-OA1":
        errors.append("onboard microphone MPN is not LMD2718T261-OA1")
    if pdm.get("profile") != "diagnostic_16k":
        errors.append("16 kHz diagnostic profile label missing")
    if pdm.get("last_fsp_error") != 0:
        errors.append("FSP reported an error")
    if pdm.get("rearm_denied", 0) != 0:
        errors.append("DMA rearm was denied")
    if pdm.get("pair_skew_drops") != 0:
        errors.append("dual-lane sequence pairing dropped a slot")
    if pdm.get("startup_discard_pairs", 0) < 1:
        errors.append("independently drained startup pair was not discarded")
    if pdm.get("max_pair_skew_us", 7500) >= 7500:
        errors.append("paired callbacks are separated by a complete capture slot")

    lanes = pdm.get("lanes")
    if not isinstance(lanes, list) or len(lanes) != 2:
        return errors + ["exactly two microphone lanes were not reported"]
    expected = [
        ("U14", "LOW", "RISE", 2, 0, "programme"),
        ("U13", "HIGH", "FALL", 0, 1, "measurement"),
    ]
    for index, (microphone, select, edge, channel, dma, role) in enumerate(expected):
        lane = lanes[index]
        observed = tuple(lane.get(key) for key in (
            "microphone", "select", "edge", "pdm_channel", "dma_channel", "role"
        ))
        if observed != (microphone, select, edge, channel, dma, role):
            errors.append(f"lane {index} identity/role contract changed")
        slots = lane.get("processed_slots", 0)
        samples = lane.get("processed_samples", 0)
        if slots < 1 or samples != slots * 120:
            errors.append(f"lane {index} did not process complete 120-sample slots")
        if lane.get("data_callbacks", 0) < slots:
            errors.append(f"lane {index} callback count is below processed slots")
        if lane.get("sample_peak", 0) <= 0 or lane.get("sample_square_sum", 0) <= 0:
            errors.append(f"lane {index} contains no non-zero microphone signal")
        if lane.get("sample_min", 0) == 0 and lane.get("sample_max", 0) == 0:
            errors.append(f"lane {index} sample range is identically zero")
        for key in ("error_callbacks", "error_flags", "overflow_events", "drop_events", "recovery_count"):
            if lane.get(key) != 0:
                errors.append(f"lane {index} {key} is non-zero")
    if lanes[0].get("processed_slots") != lanes[1].get("processed_slots"):
        errors.append("microphone lanes processed different slot counts")
    if lanes[0].get("sample_hash") == lanes[1].get("sample_hash"):
        errors.append("microphone lanes produced identical sample hashes")
    return errors


def validate_progress(before: dict, after: dict) -> list[str]:
    errors = validate_snapshot(before) + validate_snapshot(after)
    first = before.get("pdm_target", {})
    second = after.get("pdm_target", {})
    first_lanes = first.get("lanes", [{}, {}])
    second_lanes = second.get("lanes", [{}, {}])
    if len(first_lanes) == 2 and len(second_lanes) == 2:
        for lane in range(2):
            if second_lanes[lane].get("processed_slots", 0) <= first_lanes[lane].get("processed_slots", 0):
                errors.append(f"lane {lane} did not advance during the observation interval")
    if after.get("heap_used") != before.get("heap_used"):
        errors.append("heap usage changed during bounded capture")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--observe-seconds", type=float, default=3.0)
    args = parser.parse_args()
    if not 0.25 <= args.observe_seconds <= 10.0:
        raise SystemExit("--observe-seconds must be between 0.25 and 10 seconds")
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {
        "label": "ON-SILICON",
        "qualification": "DUAL_LMD2718_CAPTURE_16K_DIAGNOSTIC_OPEN",
        "started_at": datetime.now(timezone.utc).isoformat(),
        "pass": False,
    }
    port = None
    try:
        import serial
        from serial.tools import list_ports

        build = json.loads((args.build / "receipt.json").read_text())
        if not build.get("pass") or not build.get("pdm_target"):
            raise RuntimeError("build is not an accepted PDM target image")
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
        receipt["usb"] = {
            "path": device,
            "location": matches[0].location,
            "vid": matches[0].vid,
            "pid": matches[0].pid,
        }
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
        before = json.loads(transact(6))
        time.sleep(args.observe_seconds)
        after = json.loads(transact(6))
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
                "This proves identified-Titan dual capture, not 12.8 kHz sample-rate parity.",
                "U14 is the expected programme lane; this receipt does not admit U13 mixing into AP.",
                "16 kHz is diagnostic only and is not admitted to the 24 kHz/180 AP.",
                "Acoustic U13/U14 identity is unproven until a localized stimulus distinguishes the capsules.",
                "No music playback or generic soak was used.",
            ],
        )
        if errors:
            raise RuntimeError("; ".join(errors))
        receipt["status"] = "PASS_DUAL_CAPTURE_16K_DIAGNOSTIC_OPEN"
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
